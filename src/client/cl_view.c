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
/* cl_view.c -- player rendering positioning */

#include "client.h"

/* ============= */
//
/* development tools for weapons */
//
int		gun_frame;
struct model_s *gun_model;
struct model_s *clientGun;

/* ============= */

cvar_t         *crosshair;
cvar_t         *crosshair_x;
cvar_t         *crosshair_y;
cvar_t         *crosshair_scale;
cvar_t         *crosshair_red;
cvar_t         *crosshair_green;
cvar_t         *crosshair_blue;
cvar_t         *crosshair_alpha;
cvar_t         *crosshair_pulse;
cvar_t         *cl_hud_red;
cvar_t         *cl_hud_green;
cvar_t         *cl_hud_blue;
cvar_t         *cl_draw_playername;
cvar_t         *cl_draw_playername_x;
cvar_t         *cl_draw_playername_y;
cvar_t         *cl_testparticles;
cvar_t         *cl_testentities;
cvar_t         *cl_testlights;
cvar_t         *cl_testblend;

cvar_t         *cl_stats;


int		r_numdlights;
dlight_t	r_dlights[MAX_DLIGHTS];

int		r_numentities;
entity_t	r_entities[MAX_ENTITIES];

int		r_numparticles;
particle_t	r_particles[MAX_PARTICLES];

lightstyle_t	r_lightstyles[MAX_LIGHTSTYLES];

int		r_numstains;
stain_t		r_stains[MAX_STAINS];


char		cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
int		num_cl_weaponmodels;

qboolean	flashlight_eng;

/*
 * ==================== V_ClearScene
 *
 * Specifies the model that will be used as the world ====================
 */
void
V_ClearScene(void)
{
	r_numdlights = 0;
	r_numentities = 0;
	r_numparticles = 0;
}

/*
 * ===================== 3D Cam Stuff -psychospaz
 *
 * =====================
 */

#define CAM_MAXALPHADIST 0.000111

float viewermodelalpha;
void
AddViewerEntAlpha(entity_t * ent)
{
	if (viewermodelalpha == 1 || !cl_3dcam_alpha->value)
		return;

	ent->alpha *= viewermodelalpha;
	if (ent->alpha < .9)
		ent->flags |= RF_TRANSLUCENT;
}

#define ANGLEMAX 90.0
void
CalcViewerCamTrans(float distance)
{
	float		alphacalc = cl_3dcam_dist->value;

	/* no div by 0 */
	if (alphacalc < 1)
		alphacalc = 1;

	viewermodelalpha = distance / alphacalc;

	if (viewermodelalpha > 1)
		viewermodelalpha = 1;
}

/*
 * ===================== V_AddEntity
 *
 * =====================
 */
void
V_AddEntity(entity_t * ent)
{

	if (ent->flags & RF_VIEWERMODEL) {	/* here is our client */
		int		i;

		for (i = 0; i < 3; i++)
			ent->oldorigin[i] = ent->origin[i] = cl.predicted_origin[i];


		/* saber hack */
		if (ent->renderfx & RF2_CAMERAMODEL)
			ent->flags &= ~RF_VIEWERMODEL;

		if (cl_3dcam->value && !(cl.attractloop && !(cl.cinematictime > 0 && cls.realtime - cl.cinematictime > 1000))) {
			AddViewerEntAlpha(ent);
			ent->flags &= ~RF_VIEWERMODEL;
			ent->renderfx |= RF2_FORCE_SHADOW | RF2_CAMERAMODEL;
		} else if (ent->renderfx & RF2_CAMERAMODEL) {
			ent->flags &= ~RF_VIEWERMODEL;
			ent->renderfx |= RF2_FORCE_SHADOW | RF2_CAMERAMODEL;
		}
	}
	if (r_numentities >= MAX_ENTITIES)
		return;
	r_entities[r_numentities++] = *ent;
}


/*
 * ===================== V_AddParticle
 *
 * =====================
 */
#ifdef QMAX
void
V_AddParticle(vec3_t org, vec3_t angle, vec3_t color, float alpha, int alpha_src, int alpha_dst, float size, int image, int flags)
{
	int		i;
	particle_t     *p;

	if (r_numparticles >= MAX_PARTICLES)
		return;
	p = &r_particles[r_numparticles++];

	for (i = 0; i < 3; i++) {
		p->origin[i] = org[i];
		p->angle[i] = angle[i];
	}
	p->red = color[0];
	p->green = color[1];
	p->blue = color[2];
	p->alpha = alpha;
	p->image = image;
	p->flags = flags;
	p->size = size;
	
	p->blendfunc_src = alpha_src;
	p->blendfunc_dst = alpha_dst;

}
#else
void
V_AddParticle(vec3_t org, int color, float alpha)
{
	particle_t     *p;

	if (r_numparticles >= MAX_PARTICLES)
		return;
	p = &r_particles[r_numparticles++];
	VectorCopy(org, p->origin);
	p->color = color;
	p->alpha = alpha;
}
#endif

/*
 * =================== V_AddStain
 *
 * Stainmaps ===================
 */
void
V_AddStain(vec3_t org, vec3_t color, float size)
{
	stain_t        *s;

	if (r_numstains >= MAX_STAINS) {
		return;
	}
	s = &r_stains[r_numstains++];
	VectorCopy(org, s->origin);
	VectorCopy(color, s->color);
	s->size = size;
}

/*
 * ===================== V_AddLight
 *
 * =====================
 */
void
V_AddLight(vec3_t org, float intensity, float r, float g, float b)
{
	dlight_t       *dl;

	if (r_numdlights >= MAX_DLIGHTS)
		return;
	dl = &r_dlights[r_numdlights++];
	VectorCopy(org, dl->origin);
	dl->intensity = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
}


/*
 * ===================== V_AddLightStyle
 *
 * =====================
 */
void
V_AddLightStyle(int style, float r, float g, float b)
{
	lightstyle_t   *ls;

	if (style < 0 || style > MAX_LIGHTSTYLES)
		Com_Error(ERR_DROP, "Bad light style %i", style);
	ls = &r_lightstyles[style];

	ls->white = r + g + b;
	ls->rgb[0] = r;
	ls->rgb[1] = g;
	ls->rgb[2] = b;
}

/*
 * ================ V_TestParticles
 *
 * If cl_testparticles is set, create 4096 particles in the view
 * ================
 */
void
V_TestParticles(void)
{
	particle_t     *p;
	int		i, j;
	float		d, r, u;

	r_numparticles = MAX_PARTICLES;
	for (i = 0; i < r_numparticles; i++) {
		d = i * 0.25;
		r = 4 * ((i & 7) - 3.5);
		u = 4 * (((i >> 3) & 7) - 3.5);
		p = &r_particles[i];

		for (j = 0; j < 3; j++)
			p->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j] * d +
			    cl.v_right[j] * r + cl.v_up[j] * u;

		p->color = 8;
		p->alpha = cl_testparticles->value;
	}
}

/*
 * ================ V_TestEntities
 *
 * If cl_testentities is set, create 32 player models ================
 */
void
V_TestEntities(void)
{
	int		i, j;
	float		f, r;
	entity_t       *ent;

	r_numentities = 32;
	memset(r_entities, 0, sizeof(r_entities));

	for (i = 0; i < r_numentities; i++) {
		ent = &r_entities[i];

		r = 64 * ((i % 4) - 1.5);
		f = 64 * (i / 4) + 128;

		for (j = 0; j < 3; j++)
			ent->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j] * f +
			    cl.v_right[j] * r;

		ent->model = cl.baseclientinfo.model;
		ent->skin = cl.baseclientinfo.skin;
	}
}

/*
 * ================ V_TestLights
 *
 * If cl_testlights is set, create 32 lights models ================
 */
void
V_TestLights(void)
{
	int		i, j;
	float		f, r;
	dlight_t       *dl;

	r_numdlights = 32;
	memset(r_dlights, 0, sizeof(r_dlights));

	for (i = 0; i < r_numdlights; i++) {
		dl = &r_dlights[i];

		r = 64 * ((i % 4) - 1.5);
		f = 64 * (i / 4) + 128;

		for (j = 0; j < 3; j++)
			dl->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j] * f +
			    cl.v_right[j] * r;
		dl->color[0] = ((i % 6) + 1) & 1;
		dl->color[1] = (((i % 6) + 1) & 2) >> 1;
		dl->color[2] = (((i % 6) + 1) & 4) >> 2;
		dl->intensity = 200;
	}
}

/*
*
* ================
* V_FlashLight
*
* Flashlight managed by the client. It uses r_dlights. Creates two lights, one
* near the player (emited by the flashlight to the sides), and the main one
* where the player is aiming at. Not available in DM.
* ================
*/
void
V_FlashLight(void)
{
	float		dist;	/* Distance from player to light. */
	dlight_t       *dl;	/* Light array pointer. */
	trace_t		trace;	/* Trace to the destination. */
	vec3_t		vdist;	/* Distance from player to light (vector). */
	vec3_t		end;	/* Flashlight max distance. */

	if (deathmatch->value)
		return;

	r_numdlights = 1;
	memset(r_dlights, 0, sizeof(r_dlights));
	dl = r_dlights;

	/* Small light near the player. */
	VectorMA(cl.predicted_origin, 50, cl.v_forward, dl->origin);
	VectorSet(dl->color, cl_flashlight_red->value,
	    cl_flashlight_green->value, cl_flashlight_blue->value);
	dl->intensity = cl_flashlight_intensity->value * 0.60;

	VectorMA(cl.predicted_origin, cl_flashlight_distance->value,
	    cl.v_forward, end);
	trace = CL_Trace(cl.predicted_origin, end, 0, MASK_SOLID);
	/* VectorMA(trace.endpos, -5, cl.v_forward, trace.endpos); */

	if (trace.fraction == 1)
		return;

	/* Main light. */
	r_numdlights++;
	dl++;

	VectorSubtract(cl.predicted_origin, trace.endpos, vdist);
	dist = VectorLength(vdist);

	VectorCopy(trace.endpos, dl->origin);
	VectorSet(dl->color, cl_flashlight_red->value,
	    cl_flashlight_green->value, cl_flashlight_blue->value);
	dl->intensity = cl_flashlight_intensity->value *
	    (1.0 - (dist / cl_flashlight_distance->value) *
	     cl_flashlight_decscale->value);
}

/* =================================================================== */

/*
 * =================
 * CL_PrepRefresh
 *
 * Call before entering a new level, or after changing dlls.
 * =================
 */
void
CL_PrepRefresh(void)
{
	char		mapname[32];
	int		i;
	char		name[MAX_QPATH];
	float		rotate;
	vec3_t		axis;

	if (!cl.configstrings[CS_MODELS + 1][0])
		return;		/* no map loaded */

	SCR_AddDirtyPoint(0, 0);
	SCR_AddDirtyPoint(viddef.width - 1, viddef.height - 1);

	/* let the render dll load the map */
	Q_strncpyz(mapname, cl.configstrings[CS_MODELS + 1] + 5, sizeof(mapname));	/* skip "maps/" */
	mapname[strlen(mapname) - 4] = 0;	/* cut off ".bsp" */

	/* register models, pics, and skins */
	Com_Printf("Map: %s\r", mapname);
	SCR_UpdateScreen();
	re.BeginRegistration(mapname);
	Com_Printf("                                     \r");

	/* precache status bar pics */
	Com_Printf("pics\r");
	SCR_UpdateScreen();
	SCR_TouchPics();
	Com_Printf("                                     \r");

	CL_RegisterTEntModels();

	num_cl_weaponmodels = 1;
	Q_strncpyz(cl_weaponmodels[0], "weapon.md2", sizeof(cl_weaponmodels[0]));

	for (i = 1; i < MAX_MODELS && cl.configstrings[CS_MODELS + i][0]; i++) {
		Q_strncpyz(name, cl.configstrings[CS_MODELS + i], sizeof(name));
		name[37] = 0;	/* never go beyond one line */
		if (name[0] != '*')
			Com_Printf("%s\r", name);
		SCR_UpdateScreen();
		Sys_SendKeyEvents();	/* pump message loop */
		if (name[0] == '#') {
			/* special player weapon model */
			if (num_cl_weaponmodels < MAX_CLIENTWEAPONMODELS) {
				Q_strncpyz(cl_weaponmodels[num_cl_weaponmodels], cl.configstrings[CS_MODELS + i] + 1,
				    sizeof(cl_weaponmodels[num_cl_weaponmodels]));
				num_cl_weaponmodels++;
			}
		} else {
			cl.model_draw[i] = re.RegisterModel(cl.configstrings[CS_MODELS + i]);
			if (name[0] == '*')
				cl.model_clip[i] = CM_InlineModel(cl.configstrings[CS_MODELS + i]);
			else
				cl.model_clip[i] = NULL;
		}
		if (name[0] != '*')
			Com_Printf("                                     \r");
	}

	Com_Printf("images\r", i);
	SCR_UpdateScreen();
	for (i = 1; i < MAX_IMAGES && cl.configstrings[CS_IMAGES + i][0]; i++) {
		cl.image_precache[i] = re.RegisterPic(cl.configstrings[CS_IMAGES + i]);
		Sys_SendKeyEvents();	/* pump message loop */
	}

	Com_Printf("                                     \r");
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (!cl.configstrings[CS_PLAYERSKINS + i][0])
			continue;
		Com_Printf("client %i\r", i);
		SCR_UpdateScreen();
		Sys_SendKeyEvents();	/* pump message loop */
		CL_ParseClientinfo(i);
		Com_Printf("                                     \r");
	}

	CL_LoadClientinfo(&cl.baseclientinfo, "unnamed\\male/grunt");
	/* Knightmare- refresh the player model/skin info */
	userinfo_modified = true;

	/* set sky textures and speed */
	Com_Printf("sky\r", i);
	SCR_UpdateScreen();
	rotate = atof(cl.configstrings[CS_SKYROTATE]);
	sscanf(cl.configstrings[CS_SKYAXIS], "%f %f %f",
	    &axis[0], &axis[1], &axis[2]);
	re.SetSky(cl.configstrings[CS_SKY], rotate, axis);
	Com_Printf("                                     \r");

	/* the renderer can now free unneeded stuff */
	re.EndRegistration();

	/* clear any lines of console text */
	Con_ClearNotify();

	CL_LoadLoc();

	SCR_UpdateScreen();
	cl.refresh_prepped = true;
	cl.force_refdef = true;	/* make sure we have a valid refdef */

	/* start the cd track */
	if (Cvar_VariableValue("cd_shuffle")) {
		CDAudio_RandomPlay();
	} else {
		CDAudio_Play(atoi(cl.configstrings[CS_CDTRACK]), true);
	}
}

void
vectoangles(vec3_t value1, vec3_t angles)
{
	float		forward;
	float		yaw, pitch;

	if (value1[1] == 0 && value1[0] == 0) {
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	} else {
		if (value1[0])
			yaw = (int)(atan2(value1[1], value1[0]) * 180 / M_PI);
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = -90;
		if (yaw < 0)
			yaw += 360;

		forward = sqrt(value1[0] * value1[0] + value1[1] * value1[1]);
		pitch = (int)(atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

qboolean
CL_IsVisible(vec3_t org1, vec3_t org2)
{
	trace_t		trace;

	trace = CL_Trace(org1, org2, 0, MASK_SOLID);
	if (trace.fraction != 1)
		return (false);

	return (true);
}

/*
 * ==================== CalcFov ====================
 */
float
CalcFov(float fov_x, float width, float height)
{
	float		a;
	float		x;

	if (fov_x < 1 || fov_x > 179)
		Com_Error(ERR_DROP, "Bad fov: %f", fov_x);

	x = width / tan(fov_x / 360 * M_PI);

	a = atan(height / x);

	a = a * 360 / M_PI;

	return a;
}

/*
 * ===========================================================================
 * =
 */

/* gun frame debugging functions */
void
V_Gun_Next_f(void)
{
	gun_frame++;
	Com_Printf("frame %i\n", gun_frame);
}

void
V_Gun_Prev_f(void)
{
	gun_frame--;
	if (gun_frame < 0)
		gun_frame = 0;
	Com_Printf("frame %i\n", gun_frame);
}

void
V_Gun_Model_f(void)
{
	char		name      [MAX_QPATH];

	if (Cmd_Argc() != 2) {
		gun_model = NULL;
		return;
	}
	Com_sprintf(name, sizeof(name), "models/%s/tris.md2", Cmd_Argv(1));
	gun_model = re.RegisterModel(name);
}

/*
 * ===========================================================================
 * =
 */


/*
 * ================= SCR_AdjustFrom640 =================
 */
void
SCR_AdjustFrom640(float *x, float *y, float *w, float *h, qboolean refdef)
{
	float		xscale, yscale, xofs, yofs;

	if (refdef) {
		xscale = cl.refdef.width / 640.0;
		yscale = cl.refdef.height / 480.0;
		xofs = cl.refdef.x;
		yofs = cl.refdef.y;
	} else {
		xscale = viddef.width / 640.0;
		yscale = viddef.height / 480.0;
		xofs = 0;
		yofs = 0;
	}

	if (x) {
		*x *= xscale;
		*x += xofs;
	}
	if (y) {
		*y *= yscale;
		*y += yofs;
	}
	if (w) {
		*w *= xscale;
	}
	if (h) {
		*h *= yscale;
	}
}

/*
 * ================= SCR_ScanForEntityNames from Q2Pro =================
 */
void
SCR_ScanForEntityNames(void)
{
	trace_t		trace, worldtrace;
	vec3_t		end;
	entity_state_t *ent;
	int		i, x, zd, zu;
	int		headnode;
	int		num;
	vec3_t		bmins, bmaxs;
	vec3_t		forward;

	AngleVectors(cl.refdef.viewangles, forward, NULL, NULL);
	VectorMA(cl.refdef.vieworg, 131072, forward, end);

	worldtrace = CM_BoxTrace(cl.refdef.vieworg, end, vec3_origin, vec3_origin, 0, MASK_SOLID);

	for (i = 0; i < cl.frame.num_entities; i++) {
		num = (cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
		ent = &cl_parse_entities[num];

		if (ent->number == cl.playernum + 1) {
			continue;
		}
		if (ent->modelindex != 255) {
			continue;	/* not a player */
		}
		if (ent->frame >= 173 || ent->frame <= 0) {
			continue;	/* player is dead */
		}
		if (!ent->solid || ent->solid == 31) {
			continue;	/* not solid or bmodel */
		}
		x = 8 * (ent->solid & 31);
		zd = 8 * ((ent->solid >> 5) & 31);
		zu = 8 * ((ent->solid >> 10) & 63) - 32;

		bmins[0] = bmins[1] = -x;
		bmaxs[0] = bmaxs[1] = x;
		bmins[2] = -zd;
		bmaxs[2] = zu;

		headnode = CM_HeadnodeForBox(bmins, bmaxs);

		trace = CM_TransformedBoxTrace(cl.refdef.vieworg, end,
		    vec3_origin, vec3_origin, headnode, MASK_PLAYERSOLID,
		    ent->origin, vec3_origin);

		if (trace.allsolid || trace.startsolid || trace.fraction < worldtrace.fraction) {
			cl.playername = &cl.clientinfo[ent->skinnum & 0xFF];
			cl.playernametime = cls.realtime;
			break;
		}
	}
}


/*
 * ================= SCR_DrawPlayerNames =================
 */

void
SCR_DrawPlayerNames(void)
{
	float		x, y;

	if (!cl_draw_playername->value) {
		return;
	}
	
	SCR_ScanForEntityNames();

	if (cls.realtime - cl.playernametime > 1000 * cl_draw_playername->value) {
		return;
	}
	
	if (!cl.playername) {
		return;
	}
	
	x = cl_draw_playername_x->value;
	y = cl_draw_playername_y->value;

	SCR_AdjustFrom640(&x, &y, NULL, NULL, true);

	DrawString(x, y, cl.playername->name);

}


/*
 * ================= SCR_DrawCrosshair =================
 */

void
SCR_DrawCrosshair(void)
{
	float		scale;
	int		x, y;

	if (!crosshair->value)
		return;

	if (crosshair->modified) {
		crosshair->modified = false;
		SCR_TouchPics();
	}
	if (crosshair_scale->modified) {
		crosshair_scale->modified = false;
		if (crosshair_scale->value > 1)
			Cvar_SetValue("crosshair_scale", 1);
		else if (crosshair_scale->value < 0.10)
			Cvar_SetValue("crosshair_scale", 0.10);
	}
	if (!crosshair_pic[0])
		return;

	x = max(0, crosshair_x->value);
	y = max(0, crosshair_y->value);

	scale = crosshair_scale->value * (viddef.width * DIV640);

	re.DrawScaledPic(scr_vrect.x + x + ((scr_vrect.width - crosshair_width) >> 1),	/* width */
	    scr_vrect.y + y + ((scr_vrect.height - crosshair_height) >> 1),	/* height */
	    scale,		/* scale */
            (0.75 * crosshair_alpha->value) + (0.25 * crosshair_alpha->value) * sin(anglemod((cl.time * 0.005) * crosshair_pulse->value)),	/* alpha */
	    crosshair_pic, crosshair_red->value, crosshair_green->value, crosshair_blue->value, true, false);	/* pic */


}

void SCR_DrawSniperCrosshair (void)
{
	if (modType("dday"))
	{	
		if (!clientGun)
			return;	
		if (cl.refdef.fov_x>=85)
			return;

		/* continue if model is sniper */
		if ((int)(clientGun)!=(int)(re.RegisterModel("models/weapons/usa/v_m1903/tris.md2")) &&
		    (int)(clientGun)!=(int)(re.RegisterModel("models/weapons/grm/v_m98ks/tris.md2")) &&
		    (int)(clientGun)!=(int)(re.RegisterModel("models/weapons/gbr/v_303s/tris.md2")) &&
		    (int)(clientGun)!=(int)(re.RegisterModel("models/weapons/jpn/v_arisakas/tris.md2")) &&
		    (int)(clientGun)!=(int)(re.RegisterModel("models/weapons/rus/v_m9130s/tris.md2")))
			return;
	}
	else if (modType("action"))
	{
		if (clientGun)
			return;	
		if (cl.refdef.fov_x>=85)
			return;
	}
	else return;
	
	/* re.DrawStretchPic (0, 0, viddef.width, viddef.height, "reticle", 0.1); */
	re.DrawStretchPic (0, 0, viddef.width, viddef.height, "sniper", 0.9);
}

/*
 * ================== V_RenderView
 *
 * ==================
 */
void
V_RenderView(float stereo_separation)
{
	extern int	entitycmpfnc(const entity_t *, const entity_t *);
	float		f;

	if (cls.state != ca_active)
		return;

	if (!cl.refresh_prepped)
		return;		/* still loading */

	if (cl_timedemo->value) {
		if (!cl.timedemo_start)
			cl.timedemo_start = Sys_Milliseconds();
		cl.timedemo_frames++;
	}
	/* an invalid frame will just use the exact previous refdef */

	/*
	 * we can't use the old frame if the video mode has changed,
	 * though...
	 */
	if (cl.frame.valid && (cl.force_refdef || !cl_paused->value)) {
		cl.force_refdef = false;

		V_ClearScene();

		/* build a refresh entity list and calc cl.sim* */
		/* this also calls CL_CalcViewValues which loads */
		/* v_forward, etc. */
		CL_AddEntities();

		if (cl_testparticles->value)
			V_TestParticles();
		if (cl_testentities->value)
			V_TestEntities();
		if (cl_testlights->value)
			V_TestLights();
		if (cl_testblend->value) {
			cl.refdef.blend[0] = 1;
			cl.refdef.blend[1] = 0.5;
			cl.refdef.blend[2] = 0.25;
			cl.refdef.blend[3] = 0.5;
		}
		if (flashlight_eng)
			V_FlashLight();

		/*
		 * offset vieworg appropriately if we're doing stereo
		 * separation
		 */
		if (stereo_separation != 0) {
			vec3_t		tmp;

			VectorScale(cl.v_right, stereo_separation, tmp);
			VectorAdd(cl.refdef.vieworg, tmp, cl.refdef.vieworg);
		}

		/*
		 * never let it sit exactly on a node line, because a water
		 * plane can
		 */
		/* dissapear when viewed with the eye exactly on it. */

		/*
		 * the server protocol only specifies to 1/8 pixel, so add
		 * 1/16 in each axis
		 */
		cl.refdef.vieworg[0] += 1.0 / 16;
		cl.refdef.vieworg[1] += 1.0 / 16;
		cl.refdef.vieworg[2] += 1.0 / 16;

		cl.refdef.x = scr_vrect.x;
		cl.refdef.y = scr_vrect.y;
		cl.refdef.width = scr_vrect.width;
		cl.refdef.height = scr_vrect.height;
		cl.refdef.fov_y = CalcFov(cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);
		cl.refdef.time = cl.time * 0.001;

		if ((cl_underwater_movement->value) && cl.refdef.rdflags & RDF_UNDERWATER) {
			f = sin(cl.time * 0.001 * 0.4 * (M_PI * 2.7));
			cl.refdef.fov_x += f;
			cl.refdef.fov_y -= f;
		}
		cl.refdef.areabits = cl.frame.areabits;

		if (!cl_add_entities->value)
			r_numentities = 0;
		if (!cl_add_particles->value)
			r_numparticles = 0;
		if (!cl_add_lights->value)
			r_numdlights = 0;
		if (!cl_add_blend->value) {
			VectorClear(cl.refdef.blend);
		}
		if (!cl_add_blend->value || cl_underwater_trans->value) {
			if (!deathmatch->value) {
				cl.refdef.blend[0] = 0;
				cl.refdef.blend[1] = 0;
				cl.refdef.blend[2] = 0;
				cl.refdef.blend[3] = 0;
				VectorClear(cl.refdef.blend);
			}
		}
		cl.refdef.num_newstains = r_numstains;
		cl.refdef.newstains = r_stains;
		r_numstains = 0;

		cl.refdef.num_entities = r_numentities;
		cl.refdef.entities = r_entities;
		cl.refdef.num_particles = r_numparticles;
		cl.refdef.particles = r_particles;
		cl.refdef.num_dlights = r_numdlights;
		cl.refdef.dlights = r_dlights;
		cl.refdef.lightstyles = r_lightstyles;
		cl.refdef.rdflags = cl.frame.playerstate.rdflags;

		/* sort entities for better cache locality */
		qsort(cl.refdef.entities, cl.refdef.num_entities, sizeof(cl.refdef.entities[0]), (int (*) (const void *, const void *))entitycmpfnc);
	}
	cl.refdef.rdflags |= RDF_BLOOM;	/* BLOOMS */

	re.RenderFrame(&cl.refdef);
	if (cl_stats->value)
		Com_Printf("ent:%i  lt:%i  part:%i\n", r_numentities, r_numdlights, r_numparticles);
	if (log_stats->value && (log_stats_file != 0))
		fprintf(log_stats_file, "%i,%i,%i,", r_numentities, r_numdlights, r_numparticles);


	SCR_AddDirtyPoint(scr_vrect.x, scr_vrect.y);
	SCR_AddDirtyPoint(scr_vrect.x + scr_vrect.width - 1,
	    scr_vrect.y + scr_vrect.height - 1);

	/* if (!modType("dday")) //dday has no crosshair (FORCED) */
	SCR_DrawCrosshair();
/*	
	if (modType("dday")) {
		Cvar_ForceSet("crosshair", "0");
	}
*/
	
	SCR_DrawPlayerNames();
	
	if (!(cl.refdef.rdflags & RDF_UNDERWATER))
		SCR_DrawSniperCrosshair();

}


/*
 * ============= V_Viewpos_f =============
 */
void
V_Viewpos_f(void)
{
	Com_Printf("(%i %i %i) : %i\n", (int)cl.refdef.vieworg[0],
	    (int)cl.refdef.vieworg[1], (int)cl.refdef.vieworg[2],
	    (int)cl.refdef.viewangles[YAW]);
}

// Knightmare- diagnostic commands from Lazarus
/*
=============
V_Texture_f
=============
*/
void V_Texture_f (void)
{
	trace_t	tr;
	vec3_t	forward, start, end;
#if 0
	if (!developer->value) /* only works in developer mode */
		return;
#endif
	VectorCopy(cl.refdef.vieworg, start);
	AngleVectors(cl.refdef.viewangles, forward, NULL, NULL);
	VectorMA(start, 8192, forward, end);
	tr = CL_PMSurfaceTrace(cl.playernum+1, start,NULL,NULL,end,MASK_ALL);
	if (!tr.ent)
		Com_Printf("Nothing hit?\n");
	else {
		if (!tr.surface)
			Com_Printf("Not a brush\n");
		else {
			if (developer->value) 
				Com_Printf("Texture: %s, Surface: 0x%08x, Value: %d\n",
				tr.surface->name,tr.surface->flags,tr.surface->value);
			
			else 
				Com_Printf("Texture: %s\n", tr.surface->name);
		}	
	}
}

/*
=============
V_Surf_f
=============
*/
void V_Surf_f (void)
{
	trace_t	tr;
	vec3_t	forward, start, end;
	int		s;

	if (!developer->value) /* only works in developer mode */
		return;

	/* Disable this in multiplayer */
	if ( cl.configstrings[CS_MAXCLIENTS][0] &&
		strcmp(cl.configstrings[CS_MAXCLIENTS], "1") )
		return;

	if (Cmd_Argc() < 2)
	{
		Com_Printf("Syntax: surf <value>\n");
		return;
	}
	else
		s = atoi(Cmd_Argv(1));

	VectorCopy(cl.refdef.vieworg, start);
	AngleVectors(cl.refdef.viewangles, forward, NULL, NULL);
	VectorMA(start, 8192, forward, end);
	tr = CL_PMSurfaceTrace(cl.playernum+1, start,NULL,NULL,end,MASK_ALL);
	if (!tr.ent)
		Com_Printf("Nothing hit?\n");
	else
	{
		if (!tr.surface)
			Com_Printf("Not a brush\n");
		else
			tr.surface->flags = s;
	}
}

/*
=============
V_FlashLight_f
=============
*/
void
V_FlashLight_f(void)
{
	static const char *flashlight_sounds[] = {
		"world/lite_out.wav",
		"boss3/w_loop.wav"
	};
/*	
	if (deathmatch->value || !cl_flashlight->value ) {
		if (!modType("gloom")) {
			Com_Printf("Flashlight disabled in Death Match Mode\n");
		}
		return;
	}
*/	
	flashlight_eng = !flashlight_eng;
	Cbuf_AddText(va("echo Flashlight %s\n", flashlight_eng ? "on" : "off"));

	if (cl_flashlight_sound->value != 0)
		Cbuf_AddText(va("play %s\n", flashlight_sounds[flashlight_eng]));
}

/*
 * ============= V_Init =============
 */
void
V_Init(void)
{
	Cmd_AddCommand("gun_next", V_Gun_Next_f);
	Cmd_AddCommand("gun_prev", V_Gun_Prev_f);
	Cmd_AddCommand("gun_model", V_Gun_Model_f);

	Cmd_AddCommand("viewpos", V_Viewpos_f);
	// Knightmare- diagnostic commands from Lazarus
	Cmd_AddCommand("texture", V_Texture_f);
	Cmd_AddCommand("surf", V_Surf_f);
	Cmd_AddCommand("flashlight_eng", V_FlashLight_f);
	
	crosshair = Cvar_Get("crosshair", "1", CVAR_ARCHIVE);
	crosshair_x = Cvar_Get("crosshair_x", "0", 0);
	crosshair_y = Cvar_Get("crosshair_y", "0", 0);
	crosshair_scale = Cvar_Get("crosshair_scale", "0.6", CVAR_ARCHIVE);
	crosshair_red = Cvar_Get("crosshair_red", "1", CVAR_ARCHIVE);
	crosshair_green = Cvar_Get("crosshair_green", "1", CVAR_ARCHIVE);
	crosshair_blue = Cvar_Get("crosshair_blue", "1", CVAR_ARCHIVE);
	crosshair_alpha = Cvar_Get("crosshair_alpha", "1", CVAR_ARCHIVE);
	crosshair_pulse = Cvar_Get("crosshair_pulse", "1", CVAR_ARCHIVE);

	cl_draw_playername = Cvar_Get("cl_draw_playername", "1", CVAR_ARCHIVE);
	cl_draw_playername_x = Cvar_Get("cl_draw_playername_x", "300", 0);
	cl_draw_playername_y = Cvar_Get("cl_draw_playername_y", "220", 0);

	cl_testblend = Cvar_Get("cl_testblend", "0", 0);
	cl_testparticles = Cvar_Get("cl_testparticles", "0", 0);
	cl_testentities = Cvar_Get("cl_testentities", "0", 0);
	cl_testlights = Cvar_Get("cl_testlights", "0", 0);

	cl_stats = Cvar_Get("cl_stats", "0", 0);
	
	flashlight_eng = false;
}
