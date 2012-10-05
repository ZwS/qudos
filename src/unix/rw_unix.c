/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
 
#include "../ref_gl/gl_local.h"
#include <dlfcn.h>

#ifdef Joystick
#include "../unix/joystick.h"
#endif

#include "../client/keys.h"
#include "../unix/rw_unix.h"

static qboolean	mlooking;

/* state struct passed in Init */
static in_state_t *in_state;

static cvar_t  *sensitivity;
static cvar_t  *exponential_speedup;
static cvar_t  *lookstrafe;
static cvar_t  *m_side;
static cvar_t  *m_yaw;
static cvar_t  *m_pitch;
static cvar_t  *m_forward;
static cvar_t  *freelook;
static cvar_t  *cl_run;

static cvar_t  *m_filter;
static cvar_t  *in_mouse;
static cvar_t  *autosensitivity;

static int	mouse_x, mouse_y;
static int	old_mouse_x, old_mouse_y;
static qboolean	mouse_avail;
static int	mouse_buttonstate;
#if defined (REF_SDLGL)
static int	mouse_oldbuttonstate;
#endif

static cvar_t  *use_stencil;

extern cursor_t	cursor;
int     mx, my;

in_state_t     *
getState(void)
{
	return in_state;
}

static void
Force_CenterView_f(void)
{
	in_state->viewangles[PITCH] = 0;
}

static void
RW_IN_MLookDown(void)
{
	mlooking = true;
}

static void
RW_IN_MLookUp(void)
{
	mlooking = false;
	in_state->IN_CenterView_fp();
}

void
RW_IN_Init(in_state_t * in_state_p)
{
	in_state = in_state_p;

	/* mouse variables */
	m_filter = ri.Cvar_Get("m_filter", "0", 0);
	in_mouse = ri.Cvar_Get("in_mouse", "0", CVAR_ARCHIVE);

	cl_run = ri.Cvar_Get("cl_run", "1", CVAR_ARCHIVE);
	freelook = ri.Cvar_Get("freelook", "1", CVAR_ARCHIVE);
	lookstrafe = ri.Cvar_Get("lookstrafe", "0", 0);
	sensitivity = ri.Cvar_Get("sensitivity", "4", 0);
	exponential_speedup = ri.Cvar_Get("exponential_speedup", "0", 0);
	autosensitivity = ri.Cvar_Get("autosensitivity", "1", 0);
	use_stencil = ri.Cvar_Get("use_stencil", "1", CVAR_ARCHIVE);

	m_pitch = ri.Cvar_Get("m_pitch", "0.022", 0);
	m_yaw = ri.Cvar_Get("m_yaw", "0.022", 0);
	m_forward = ri.Cvar_Get("m_forward", "1", 0);
	m_side = ri.Cvar_Get("m_side", "0.8", 0);

	ri.Cmd_AddCommand("+mlook", RW_IN_MLookDown);
	ri.Cmd_AddCommand("-mlook", RW_IN_MLookUp);
	ri.Cmd_AddCommand("force_centerview", Force_CenterView_f);

	mouse_x = mouse_y = 0.0;
	mouse_avail = true;

	RW_IN_PlatformInit();

#ifdef Joystick
	RW_IN_InitJoystick();
#endif
}


void
RW_IN_Shutdown(void)
{
	if (mouse_avail) {

		mouse_avail = false;

		ri.Cmd_RemoveCommand("+mlook");
		ri.Cmd_RemoveCommand("-mlook");
		ri.Cmd_RemoveCommand("force_centerview");
	}
#ifdef Joystick
	ri.Cmd_RemoveCommand("joy_advancedupdate");
	CloseJoystick();
#endif
}

/*
 * =========== IN_Commands ===========
 */
void
RW_IN_Commands(void)
{
#if defined (REF_SDLGL)
	int		i;

	if (mouse_avail) {
		getMouse(&mouse_x, &mouse_y, &mouse_buttonstate);

		for (i = 0; i < 3; i++) {
			if ((mouse_buttonstate & (1 << i)) && !(mouse_oldbuttonstate & (1 << i)))
				in_state->Key_Event_fp(K_MOUSE1 + i, true);
			if (!(mouse_buttonstate & (1 << i)) && (mouse_oldbuttonstate & (1 << i)))
				in_state->Key_Event_fp(K_MOUSE1 + i, false);
		}

		if ((mouse_buttonstate & (1 << 3)) && !(mouse_oldbuttonstate & (1 << 3)))
			in_state->Key_Event_fp(K_MOUSE4, true);
		if (!(mouse_buttonstate & (1 << 3)) && (mouse_oldbuttonstate & (1 << 3)))
			in_state->Key_Event_fp(K_MOUSE4, false);
		if ((mouse_buttonstate & (1 << 4)) && !(mouse_oldbuttonstate & (1 << 4)))
			in_state->Key_Event_fp(K_MOUSE5, true);
		if (!(mouse_buttonstate & (1 << 4)) && (mouse_oldbuttonstate & (1 << 4)))
			in_state->Key_Event_fp(K_MOUSE5, false);

		mouse_oldbuttonstate = mouse_buttonstate;
	}
#endif
#ifdef Joystick
	RW_IN_JoystickCommands();
#endif
}

/*
 * =========== IN_Move ===========
 */
void
RW_IN_Move(usercmd_t * cmd, int *mcoords)
{
	if (!mouse_avail)
		return;

	if (mouse_avail) {
		getMouse(&mouse_x, &mouse_y, &mouse_buttonstate);

		if (m_filter->value) {
			mouse_x = (mx + old_mouse_x) * 0.5;
			mouse_y = (my + old_mouse_y) * 0.5;
		} else {
			mouse_x = mx;
			mouse_y = my;
		}

		old_mouse_x = mx;
		old_mouse_y = my;

		/* raw coords for menu mouse */
		mcoords[0] = mx;
		mcoords[1] = my;

		if (autosensitivity->value) {
			mx *= sensitivity->value * (r_refdef.fov_x / 90.0);
			my *= sensitivity->value * (r_refdef.fov_y / 90.0);
		} else {
			mx *= sensitivity->value;
			my *= sensitivity->value;
		}


		/* add mouse X/Y movement to cmd */
		if ((*in_state->in_strafe_state & 1) ||
		    (lookstrafe->value && mlooking))
			cmd->sidemove += m_side->value * mx;
		else
			in_state->viewangles[YAW] -= m_yaw->value * mx;

		if ((mlooking || freelook->value) &&
		    !(*in_state->in_strafe_state & 1)) {
			in_state->viewangles[PITCH] += m_pitch->value * my;
		} else {
			cmd->forwardmove -= m_forward->value * my;
		}
		mx = my = 0;
	}
#ifdef Joystick
	RW_IN_JoystickMove(cmd, mlooking, lookstrafe, m_pitch);
#endif
}

void
RW_IN_Frame(void)
{
}
