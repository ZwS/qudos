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
 */

#include "gl_local.h"

int		r_numflares;
flare_t		*r_flares[MAX_FLARES];

void
R_RenderFlares(void)
{
	int		i;

	if (gl_flares->value == 0)
		return;

	qglDepthMask(0);
	qglDisable(GL_TEXTURE_2D);
	qglShadeModel(GL_SMOOTH);
	qglEnable(GL_BLEND);

	/* Fog bug fix by Kirk Barnes. */
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE);

	for (i = 0; i < r_numflares; i++)
		if (ri.CL_IsVisible(r_origin, r_flares[i]->origin))
			R_RenderFlare(r_flares[i]);

	qglColor3f(1, 1, 1);
	qglDisable(GL_BLEND);
	qglEnable(GL_TEXTURE_2D);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask(1);
}

void
R_RenderFlare(flare_t *light)
{
	char		pathname[MAX_QPATH];	/* Flare image path. */
	int		size;			/* Flare size. */
	int		style;			/* Flare style. */
	float		dist;			/* Distance to flare. */
	image_t        *flare;			/* Flare image. */
	vec3_t		color;			/* Flare color. */
	vec3_t		vec;			/* Vector distance to flare. */
	vec3_t		point;			/* For flare coordinates. */

	/* Check for flare style override. */
	if (gl_flare_force_style->value > 0 &&
	    gl_flare_force_style->value <= FLARE_STYLES)
		style = gl_flare_force_style->value;
	else
		style = light->style;

	/* Check if style is valid. */
	if (style > 0 && style <= FLARE_STYLES)
		Com_sprintf(pathname, sizeof(pathname),
		    "gfx/flare%d.png", style);
	else
		ri.Sys_Error(ERR_DROP,
		    "R_RenderFlare: invalid flare style: %d", style);

	flare = GL_FindImage(pathname, it_sprite);

	if (flare == NULL)
		flare = r_notexture;

	/* Check for flare size override. */
	if (gl_flare_force_size->value != 0)
		size = gl_flare_force_size->value;
	else
		size = light->size * gl_flare_scale->value;

	/* Adjust flare intensity. */
	VectorScale(light->color, gl_flare_intensity->value, color);

	/* Calculate size based on distance. */
	VectorSubtract(light->origin, r_origin, vec);
	dist = VectorLength(vec) * (size * 0.01);

	/* Limit flare size. */
	if (gl_flare_maxdist->value != 0)
		if (dist > gl_flare_maxdist->value)
			dist = gl_flare_maxdist->value;

	qglDisable(GL_DEPTH_TEST);
	qglEnable(GL_TEXTURE_2D);
	qglColor4f(color[0] / 2, color[1] / 2, color[2] / 2, 1);
	GL_Bind(flare->texnum);

	GL_TexEnv(GL_MODULATE);

	qglBegin(GL_QUADS);

#if 0
	/* Shorter, but expensive (needs more calculations). */
	for (i = 0; i < 2; i++)
		for (i ? (j = 0) : (j = 1);
		    i ? (j < 2) : (j >= 0);
		    i ? (j++) : (j--)) {
			qglTexCoord2f(i, j);
			VectorMA(light->origin, i ? (1 + dist) : (-1 - dist),
			    vup, point);
			VectorMA(point, j ? (1 + dist) : (-1 - dist),
			    vright, point);
			qglVertex3fv(point);
		}
#endif

	qglTexCoord2f(0, 1);
	VectorMA(light->origin, -1-dist, vup, point);
	VectorMA(point, 1+dist, vright, point);
	qglVertex3fv(point);

	qglTexCoord2f(0, 0);
	VectorMA(light->origin, -1-dist, vup, point);
	VectorMA(point, -1-dist, vright, point);
	qglVertex3fv (point);

	qglTexCoord2f(1, 0);
	VectorMA(light->origin, 1+dist, vup, point);
	VectorMA(point, -1-dist, vright, point);
	qglVertex3fv(point);

	qglTexCoord2f(1, 1);
	VectorMA(light->origin, 1+dist, vup, point);
	VectorMA(point, 1+dist, vright, point);
	qglVertex3fv(point);

	qglEnd();

	GL_TexEnv(GL_REPLACE);
	qglEnable(GL_DEPTH_TEST);
	qglDisable(GL_TEXTURE_2D);
	qglColor3f(0, 0, 0);
}

void
GL_AddFlareSurface(msurface_t * surf)
{
	int		intens;	/* Light intensity. */
	flare_t        *light;	/* New flare. */
	vec3_t		origin;	/* Center of surface. */
	vec3_t		color;	/* Flare color. */
	vec3_t		normal;	/* Normal vector. */
	
	/* Check for free flares. */
	if (r_numflares >= MAX_FLARES)
		return;
	
	/* Initialization. */
	VectorClear(origin);
	VectorSet(color, 1.0, 1.0, 1.0);

	intens = surf->texinfo->value;

	/* Skip very small or null lights. */
	if (intens <= 1000) {
		ri.Con_Printf(PRINT_DEVELOPER,
		    "Skipped flare surface with intensity of %d.\n", intens);
		return;
	}

	/* Create new flare. */
	light = Hunk_Alloc(sizeof(flare_t));
	r_flares[r_numflares++] = light;

	VectorCopy(surf->center, origin);

#if 0
	/* Calculate color average of light surface texture. */
	GL_Bind(surf->texinfo->image->texnum);
	width = surf->texinfo->image->upload_width;
	height = surf->texinfo->image->upload_height;

	buffer = malloc(width * height * 3);
	qglGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	VectorClear(rgbSum);

	for (i = 0, p = buffer; i < width * height; i++, p += 3)
		rgbSum[i % 3] += (float)p[i % 3] * (1.0 / 255);

	free(buffer);

	VectorScale(rgbSum, 1.0 / (width*height), color);
	VectorCopy(color, light->color);
#else
	if (surf->wallLight != NULL)
		VectorCopy(surf->wallLight->color, light->color);
	else
		VectorSet(light->color, 1.0, 1.0, 1.0);
#endif

	/* Move flare 2 units far from the surface. */
	if (surf->flags & SURF_PLANEBACK)
		VectorNegate(surf->plane->normal, normal);
	else
		VectorCopy(surf->plane->normal, normal);

	VectorMA(origin, 2, normal, origin);
	VectorCopy(origin, light->origin);

	light->style = r_numflares % FLARE_STYLES + 1;	/* Pseudo-random. */
	light->size = intens / 1000;
	
	ri.Con_Printf(PRINT_DEVELOPER, "Added flare on light surface %d: "
	    "size = %d, style = %d, red = %f, green = %f, blue = %f,"
	    "x = %f, y = %f, z = %f.\n", r_numflares, light->size,
	    light->style, light->color[0], light->color[1], light->color[2],
	    light->origin[0], light->origin[1], light->origin[2]);
}
