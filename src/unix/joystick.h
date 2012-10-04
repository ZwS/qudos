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
 
#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__


#include "../ref_gl/gl_local.h"

/*
 * extern cvar_t   *in_joystick; extern cvar_t	*joy_name; extern cvar_t
 * *joy_dev; extern cvar_t   *joy_advanced; extern cvar_t
 * *joy_advaxisx; extern cvar_t	*joy_advaxisy; extern cvar_t
 * *joy_advaxisz; extern cvar_t	*joy_advaxisr; extern cvar_t
 * *joy_advaxisu; extern cvar_t	*joy_advaxisv; extern cvar_t
 * *joy_forwardthreshold; extern cvar_t	*joy_sidethreshold; extern cvar_t
 * *joy_pitchthreshold; extern cvar_t	*joy_yawthreshold; extern cvar_t
 * *joy_forwardsensitivity; extern cvar_t	*joy_sidesensitivity; extern
 * cvar_t	*joy_pitchsensitivity; extern cvar_t	*joy_yawsensitivity;
 * extern cvar_t	*joy_upthreshold; extern cvar_t	*joy_upsensitivity;
 *
 * extern cvar_t  *cl_upspeed; extern cvar_t  *cl_forwardspeed; extern cvar_t
 * *cl_sidespeed; extern cvar_t  *cl_yawspeed; extern cvar_t  *cl_pitchspeed;
 * extern cvar_t  *cl_anglespeedkey;
 *
 * extern cvar_t  *cl_run;
 *
 * extern qboolean joystick_avail; extern int *axis_vals; extern int *axis_map;
 */
/* In joystick.c */
void		RW_IN_InitJoystick();
void		Joy_AdvancedUpdate_f(void);
void		RW_IN_JoystickCommands();
void		RW_IN_JoystickMove(usercmd_t *, qboolean, cvar_t *, cvar_t *);

/* Provided in platform specific manner */
qboolean	OpenJoystick(cvar_t *);
qboolean	CloseJoystick();
void		PlatformJoyCommands(int *, int *);

#endif
