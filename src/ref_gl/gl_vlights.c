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
/* vlights.c */
#include "gl_local.h"


/* Vic did core of this - THANK YOU KING VIC!!! - psychospaz */

#define VLIGHT_CLAMP_MIN	100.0f
#define VLIGHT_CLAMP_MAX	512.0f

#define VLIGHT_GRIDSIZE_X	512
#define VLIGHT_GRIDSIZE_Y	512

vec3_t		vlightgrid[VLIGHT_GRIDSIZE_X][VLIGHT_GRIDSIZE_Y];

int		RecursiveLightPoint(mnode_t * node, vec3_t start, vec3_t end);

static vec3_t	r_avertexnormals[NUMVERTEXNORMALS] = {
#include "anorms.h"
};

void
VLight_InitAnormTable(void)
{
	int		x, y;
	float		angle;
	float		sp, sy, cp, cy;

	for (x = 0; x < VLIGHT_GRIDSIZE_X; x++) {
		angle = (x * 360 / VLIGHT_GRIDSIZE_X) * (M_PI / 180.0);
		sy = sin(angle);
		cy = cos(angle);

		for (y = 0; y < VLIGHT_GRIDSIZE_Y; y++) {
			angle = (y * 360 / VLIGHT_GRIDSIZE_X) * (M_PI / 180.0);
			sp = sin(angle);
			cp = cos(angle);

			vlightgrid[x][y][0] = cp * cy;
			vlightgrid[x][y][1] = cp * sy;
			vlightgrid[x][y][2] = -sp;
		}
	}
}

float
VLight_GetLightValue(vec3_t normal, vec3_t dir, float apitch, float ayaw, qboolean dlight)
{
	int		pitchofs, yawofs;
	float		angle1, angle2, light;

	if (normal[1] == 0 && normal[0] == 0) {
		angle2 = 0;
		if (normal[2] > 0)
			angle1 = 90;
		else
			angle1 = 270;
	} else {
		float		forward;

		angle2 = atan2(normal[1], normal[0]) * (180.0 / M_PI);
		if (angle2 < 0)
			angle2 += 360;

		forward = sqrt(normal[0] * normal[0] + normal[1] * normal[1]);
		angle1 = atan2(normal[2], forward) * (180.0 / M_PI);
		if (angle1 < 0)
			angle1 += 360;
	}

	pitchofs = (angle1 + apitch) * VLIGHT_GRIDSIZE_X / 360;
	yawofs = (angle2 + ayaw) * VLIGHT_GRIDSIZE_Y / 360;

	while (pitchofs > (VLIGHT_GRIDSIZE_X - 1))
		pitchofs -= VLIGHT_GRIDSIZE_X;
	while (pitchofs < 0)
		pitchofs += VLIGHT_GRIDSIZE_X;

	while (yawofs > (VLIGHT_GRIDSIZE_Y - 1))
		yawofs -= VLIGHT_GRIDSIZE_Y;
	while (yawofs < 0)
		yawofs += VLIGHT_GRIDSIZE_Y;

	if (dlight) {

		light = DotProduct(vlightgrid[pitchofs][yawofs], dir);

		if (light > 1)
			light = 1;

		if (light < 0)
			light = 0;

		return light;
	} else {
		light = (DotProduct(vlightgrid[pitchofs][yawofs], dir) + 2.0) * 63.5;

		if (light > VLIGHT_CLAMP_MAX)
			light = VLIGHT_CLAMP_MAX;
		if (light < VLIGHT_CLAMP_MIN)
			light = VLIGHT_CLAMP_MIN;

		return light * (1.0 / 200.0);
	}

}

float
VLight_LerpLight(int index1, int index2, float ilerp, vec3_t dir, vec3_t angles, qboolean dlight)
{
	vec3_t		normal;

	normal[0] = r_avertexnormals[index1][0] + (r_avertexnormals[index2][0] - r_avertexnormals[index1][0]) * ilerp;
	normal[1] = r_avertexnormals[index1][1] + (r_avertexnormals[index2][1] - r_avertexnormals[index1][1]) * ilerp;
	normal[2] = r_avertexnormals[index1][2] + (r_avertexnormals[index2][2] - r_avertexnormals[index1][2]) * ilerp;
	VectorNormalize(normal);

	return VLight_GetLightValue(normal, dir, angles[PITCH], angles[YAW], dlight);
}

void
VLight_Init(void)
{
	VLight_InitAnormTable();
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
		/* PMM - fixed to correct for pitch of 0 */
		if (value1[0])
			yaw = (atan2(value1[1], value1[0]) * 180 / M_PI);
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = 270;

		if (yaw < 0)
			yaw += 360;

		forward = sqrt(value1[0] * value1[0] + value1[1] * value1[1]);
		pitch = (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

void
R_ShadowLight(vec3_t pos, vec3_t lightAdd)
{
	vec3_t		end;
	float		r;
	int		lnum;
	dlight_t       *dl;
	float		len;
	vec3_t		dist, angle;
	float		add;

	if (!r_worldmodel->lightdata)	/* keep old lame shadow */
		return;

	end[0] = pos[0];
	end[1] = pos[1];
	end[2] = pos[2] - 2048;

	r = RecursiveLightPoint(r_worldmodel->nodes, pos, end);

	VectorClear(lightAdd);
	//
	/* add dynamic light shadow angles */
	    //
	    dl = r_refdef.dlights;
	for (lnum = 0; lnum < r_refdef.num_dlights; lnum++, dl++) {
		if (dl->spotlight)	/* spotlights */
			continue;

		VectorSubtract(dl->origin, pos, dist);
		add = sqrt(dl->intensity - VectorLength(dist));
		VectorNormalize(dist);
		if (add > 0) {
			VectorScale(dist, add, dist);
			VectorAdd(lightAdd, dist, lightAdd);
		}
	}

	len = VectorNormalize(lightAdd);
	if (len > 2048)
		len = 2048;
	if (len <= 0)
		return;

	/* now rotate according to model yaw */
	vectoangles(lightAdd, angle);

	/* e->angles */
	angle[YAW] = -(currententity->angles[YAW] - angle[YAW]);

	AngleVectors(angle, dist, NULL, NULL);
	VectorScale(dist, len, lightAdd);
}
