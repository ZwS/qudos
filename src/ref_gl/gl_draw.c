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

/* draw.c */
#include "gl_local.h"

image_t        *draw_chars;

/* vertex arrays */
float		tex_array[MAX_ARRAY][2];
float		vert_array[MAX_ARRAY][3];
float		col_array[MAX_ARRAY][4];

extern qboolean	scrap_dirty;
void		Scrap_Upload(void);

/*
 * =============== RefreshFont ===============
 */

void
RefreshFont(void)
{
	font_color->modified = false;

	draw_chars = GL_FindImage(va("fonts/%s.pcx", font_color->string), it_pic);
	if (!draw_chars)	/* fall back on default font */
		draw_chars = GL_FindImage("fonts/default.pcx", it_pic);
	if (!draw_chars)	/* fall back on old Q2 conchars */
		draw_chars = GL_FindImage("pics/conchars.pcx", it_pic);
	if (!draw_chars)	/* Knightmare- prevent crash caused by missing font */
		ri.Sys_Error(ERR_FATAL, "RefreshFont: couldn't load pics/conchars");

	GL_Bind(draw_chars->texnum);
}

/*
 * =============== Draw_InitLocal ===============
 */
image_t        *Draw_FindPic(char *name);
void
Draw_InitLocal(void)
{
	/* load console characters (don't bilerp characters) */
	draw_chars = Draw_FindPic("conchars");
	if (!draw_chars)
		ri.Sys_Error(ERR_FATAL, "Couldn't load pics/conchars");

	GL_Bind(draw_chars->texnum);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (font_color->string) {
		RefreshFont();
	}
}



/*
 * ================ Draw_Char
 *
 * Draws one 8*8 graphics character with 0 being transparent. It can be clipped
 * to the top of the screen to allow the console to be smoothly scrolled off.
 * ================
 */
void
Draw_Char(int x, int y, int num, int alpha)
{
	int		row, col;
	float		frow, fcol, size;

	num &= 255;

	if (alpha >= 254)
		alpha = 254;
	else if (alpha <= 1)
		alpha = 1;

	if ((num & 127) == 32)
		return;		/* space */

	if (y <= -8)
		return;		/* totally off screen */

	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	{
		/* GLSTATE_DISABLE_ALPHATEST */
		qglDisable(GL_ALPHA_TEST);
		GL_TexEnv(GL_MODULATE);
		qglColor4ub(255, 255, 255, alpha);
		/* GLSTATE_ENABLE_BLEND */
		qglEnable(GL_BLEND);
		qglDepthMask(false);
	}

	GL_Bind(draw_chars->texnum);

	qglBegin(GL_QUADS);
	qglTexCoord2f(fcol, frow);
	qglVertex2f(x, y);
	qglTexCoord2f(fcol + size, frow);
	qglVertex2f(x + 8, y);
	qglTexCoord2f(fcol + size, frow + size);
	qglVertex2f(x + 8, y + 8);
	qglTexCoord2f(fcol, frow + size);
	qglVertex2f(x, y + 8);
	qglEnd();

	{
		qglDepthMask(true);
		GL_TexEnv(GL_REPLACE);
		/* GLSTATE_DISABLE_BLEND */
		qglDisable(GL_BLEND);
		qglColor4f(1,1,1,1);
		/* GLSTATE_ENABLE_ALPHATEST */
		qglEnable(GL_ALPHA_TEST);
	}
}

/*
 * ============= Draw_FindPic =============
 */
image_t        *
Draw_FindPic(char *name)
{
	image_t        *gl;
	char		fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\') {
		Com_sprintf(fullname, sizeof(fullname), "pics/%s.pcx", name);
		gl = GL_FindImage(fullname, it_pic);
	} else
		gl = GL_FindImage(name + 1, it_pic);

	return gl;
}

/*
 * ============= Draw_GetPicSize =============
 */
void
Draw_GetPicSize(int *w, int *h, char *pic)
{
	image_t        *gl;

	gl = Draw_FindPic(pic);
	if (!gl) {
		*w = *h = -1;
		return;
	}
	*w = gl->width;
	*h = gl->height;
}

/*
 * ============= Draw_StretchPic -- only used for drawing console...
 * =============
 */
void
Draw_StretchPic(int x, int y, int w, int h, char *pic, float alpha)
{
	image_t        *gl;

	gl = Draw_FindPic(pic);
	if (!gl) {
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}
	if (scrap_dirty)
		Scrap_Upload();

	if (((gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION))
	    && !gl->has_alpha)
		qglDisable(GL_ALPHA_TEST);

	/* add alpha support */
	if (gl->has_alpha || alpha < 1) {
		qglDisable(GL_ALPHA_TEST);

		GL_Bind(gl->texnum);

		GL_TexEnv(GL_MODULATE);
		qglColor4f(1, 1, 1, alpha);
		qglEnable(GL_BLEND);
		qglDepthMask(false);
	} else
		GL_Bind(gl->texnum);

	qglBegin(GL_QUADS);
	qglTexCoord2f(gl->sl, gl->tl);
	qglVertex2f(x, y);
	qglTexCoord2f(gl->sh, gl->tl);
	qglVertex2f(x + w, y);
	qglTexCoord2f(gl->sh, gl->th);
	qglVertex2f(x + w, y + h);
	qglTexCoord2f(gl->sl, gl->th);
	qglVertex2f(x, y + h);
	qglEnd();

	/* add alpha support */
	if (gl->has_alpha || alpha < 1) {
		qglDepthMask(true);
		GL_TexEnv(GL_REPLACE);
		qglDisable(GL_BLEND);
		qglColor4f(1, 1, 1, 1);

		qglEnable(GL_ALPHA_TEST);
	}
	if (((gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION))
	    && !gl->has_alpha)
		qglEnable(GL_ALPHA_TEST);
}

/*
 * ============= Draw_Pic =============
 */
void
Draw_Pic(int x, int y, char *pic, float alpha)
{
	image_t        *gl;

	gl = Draw_FindPic(pic);
	if (!gl) {
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}
	if (scrap_dirty)
		Scrap_Upload();

	if (((gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION)) && !gl->has_alpha)
		qglDisable(GL_ALPHA_TEST);

	/* add alpha support */
	{
		qglDisable(GL_ALPHA_TEST);

		qglBindTexture(GL_TEXTURE_2D, gl->texnum);

		GL_TexEnv(GL_MODULATE);
		qglColor4f(1, 1, 1, 0.999);	/* need <1 for trans to work */
		qglEnable(GL_BLEND);
		qglDepthMask(false);
	}

	GL_Bind(gl->texnum);
	qglBegin(GL_QUADS);
	qglTexCoord2f(gl->sl, gl->tl);
	qglVertex2f(x, y);
	qglTexCoord2f(gl->sh, gl->tl);
	qglVertex2f(x + gl->width, y);
	qglTexCoord2f(gl->sh, gl->th);
	qglVertex2f(x + gl->width, y + gl->height);
	qglTexCoord2f(gl->sl, gl->th);
	qglVertex2f(x, y + gl->height);
	qglEnd();

	/* add alpha support */
	{
		qglDepthMask(true);
		GL_TexEnv(GL_REPLACE);
		qglDisable(GL_BLEND);
		qglColor4f(1, 1, 1, 1);

		qglEnable(GL_ALPHA_TEST);
	}

	if (((gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION)) && !gl->has_alpha)
		qglEnable(GL_ALPHA_TEST);
}

/*
 * ============= Draw_ScaledPic =============
 */

void
Draw_ScaledPic(int x, int y, float scale, float alpha, char *pic, float red, float green, float blue, qboolean fixcoords, qboolean repscale)
{
	float		xoff, yoff;
	image_t        *gl;

	gl = Draw_FindPic(pic);
	if (!gl) {
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}
	if (scrap_dirty)
		Scrap_Upload();

	if (((gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION)) && !gl->has_alpha)
		qglDisable(GL_ALPHA_TEST);

	/* add alpha support */
	{
		qglDisable(GL_ALPHA_TEST);

		qglBindTexture(GL_TEXTURE_2D, gl->texnum);

		GL_TexEnv(GL_MODULATE);
		qglColor4f(red, green, blue, alpha);
		qglEnable(GL_BLEND);
		qglDepthMask(false);
	}

	/* NOTE: replace this with shaders as soon as they are supported */
	if (repscale)
		scale *= gl->replace_scale;	/* scale down if replacing a pcx image */

	if (fixcoords) {	/* Knightmare- whether to adjust coordinates for scaling */

		xoff = (gl->width * scale - gl->width) / 2;
		yoff = (gl->height * scale - gl->height) / 2;

		GL_Bind(gl->texnum);
		qglBegin(GL_QUADS);
		qglTexCoord2f(gl->sl, gl->tl);
		qglVertex2f(x - xoff, y - yoff);
		qglTexCoord2f(gl->sh, gl->tl);
		qglVertex2f(x + gl->width + xoff, y - yoff);
		qglTexCoord2f(gl->sh, gl->th);
		qglVertex2f(x + gl->width + xoff, y + gl->height + yoff);
		qglTexCoord2f(gl->sl, gl->th);
		qglVertex2f(x - xoff, y + gl->height + yoff);
		qglEnd();

	} else {
		xoff = gl->width * scale - gl->width;
		yoff = gl->height * scale - gl->height;

		GL_Bind(gl->texnum);
		qglBegin(GL_QUADS);
		qglTexCoord2f(gl->sl, gl->tl);
		qglVertex2f(x, y);
		qglTexCoord2f(gl->sh, gl->tl);
		qglVertex2f(x + gl->width + xoff, y);
		qglTexCoord2f(gl->sh, gl->th);
		qglVertex2f(x + gl->width + xoff, y + gl->height + yoff);
		qglTexCoord2f(gl->sl, gl->th);
		qglVertex2f(x, y + gl->height + yoff);
		qglEnd();
	}

	/* add alpha support */
	{
		qglDepthMask(true);
		GL_TexEnv(GL_REPLACE);
		qglDisable(GL_BLEND);
		qglColor4f(1, 1, 1, 1);

		qglEnable(GL_ALPHA_TEST);
	}

	if (((gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION)) && !gl->has_alpha)
		qglEnable(GL_ALPHA_TEST);
}


/*
 * ============= Draw_TileClear
 *
 * This repeats a 64*64 tile graphic to fill the screen around a sized down
 * refresh window. =============
 */
void
Draw_TileClear(int x, int y, int w, int h, char *pic)
{
	image_t        *image;

	image = Draw_FindPic(pic);
	if (!image) {
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}
	if (((gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION)) && !image->has_alpha)
		qglDisable(GL_ALPHA_TEST);

	GL_Bind(image->texnum);
	qglBegin(GL_QUADS);
	qglTexCoord2f(x / 64.0, y / 64.0);
	qglVertex2f(x, y);
	qglTexCoord2f((x + w) / 64.0, y / 64.0);
	qglVertex2f(x + w, y);
	qglTexCoord2f((x + w) / 64.0, (y + h) / 64.0);
	qglVertex2f(x + w, y + h);
	qglTexCoord2f(x / 64.0, (y + h) / 64.0);
	qglVertex2f(x, y + h);
	qglEnd();

	if (((gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION)) && !image->has_alpha)
		qglEnable(GL_ALPHA_TEST);
}


/*
 * ============= Draw_Fill
 *
 * Fills a box of pixels with a single color =============
 */
void
Draw_Fill(int x, int y, int w, int h, int c)
{
	union {
		unsigned	c;
		byte		v[4];
	} color;

	if ((unsigned)c > 255)
		ri.Sys_Error(ERR_FATAL, "Draw_Fill: bad color");

	qglDisable(GL_TEXTURE_2D);

	color.c = d_8to24table[c];

	qglColor3f(color.v[0] / 255.0, color.v[1] / 255.0, color.v[2] / 255.0);

	qglBegin(GL_QUADS);

	qglVertex2f(x, y);
	qglVertex2f(x + w, y);
	qglVertex2f(x + w, y + h);
	qglVertex2f(x, y + h);

	qglEnd();
	qglColor3f(1, 1, 1);
	qglEnable(GL_TEXTURE_2D);
}

/*
 * ===========================================================================
 * ==
 */

/*
 * ================ Draw_FadeScreen
 *
 * ================
 */
void
Draw_FadeScreen(void)
{
	qglEnable(GL_BLEND);
	qglDisable(GL_TEXTURE_2D);
	qglColor4f(0, 0, 0, 0.8);
	qglBegin(GL_QUADS);

	qglVertex2f(0, 0);
	qglVertex2f(vid.width, 0);
	qglVertex2f(vid.width, vid.height);
	qglVertex2f(0, vid.height);

	qglEnd();
	qglColor4f(1, 1, 1, 1);
	qglEnable(GL_TEXTURE_2D);
	qglDisable(GL_BLEND);
}


/* ==================================================================== */


/*
 * ============= Draw_StretchRaw =============
 */
extern unsigned	r_rawpalette[256];

void
Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, byte * data)
{
	unsigned	image32[256 * 256];
	unsigned char	image8[256 * 256];
	int		i, j, trows;
	byte           *source;
	int		frac, fracstep;
	float		hscale;
	int		row;
	float		t;

	GL_Bind(0);

	if (rows <= 256) {
		hscale = 1;
		trows = rows;
	} else {
		hscale = rows * DIV256;
		trows = 256;
	}
	t = rows * hscale / 256;

	if (!qglColorTableEXT) {
		unsigned       *dest;

		for (i = 0; i < trows; i++) {
			row = (int)(i * hscale);
			if (row > rows)
				break;
			source = data + cols * row;
			dest = &image32[i * 256];
			fracstep = cols * 0x10000 * DIV256;
			frac = fracstep >> 1;
			for (j = 0; j < 256; j++) {
				dest[j] = r_rawpalette[source[frac >> 16]];
				frac += fracstep;
			}
		}

		qglTexImage2D(GL_TEXTURE_2D, 0, gl_tex_solid_format, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, image32);
	} else {
		unsigned char  *dest;

		for (i = 0; i < trows; i++) {
			row = (int)(i * hscale);
			if (row > rows)
				break;
			source = data + cols * row;
			dest = &image8[i * 256];
			fracstep = cols * 0x10000 * DIV256;
			frac = fracstep >> 1;
			for (j = 0; j < 256; j++) {
				dest[j] = source[frac >> 16];
				frac += fracstep;
			}
		}

		qglTexImage2D(GL_TEXTURE_2D,
		    0,
		    GL_COLOR_INDEX8_EXT,
		    256, 256,
		    0,
		    GL_COLOR_INDEX,
		    GL_UNSIGNED_BYTE,
		    image8);
	}
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if ((gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION))
		qglDisable(GL_ALPHA_TEST);

	qglBegin(GL_QUADS);
	qglTexCoord2f(0, 0);
	qglVertex2f(x, y);
	qglTexCoord2f(1, 0);
	qglVertex2f(x + w, y);
	qglTexCoord2f(1, t);
	qglVertex2f(x + w, y + h);
	qglTexCoord2f(0, t);
	qglVertex2f(x, y + h);
	qglEnd();

	if ((gl_config.renderer == GL_RENDERER_MCD) || (gl_config.renderer & GL_RENDERER_RENDITION))
		qglEnable(GL_ALPHA_TEST);
}

static vec3_t	modelorg;	/* relative to viewpoint */

/* sul's minimap thing */
void
R_RecursiveRadarNode(mnode_t * node)
{
	int		c, side, sidebit;
	cplane_t       *plane;
	msurface_t     *surf, **mark;
	mleaf_t        *pleaf;
	float		dot, distance;
	glpoly_t       *p;
	float          *v;
	int		i;

	if (node->contents == CONTENTS_SOLID)
		return;		/* solid */

	if (gl_minimap_zoom->value >= 0.1)
		distance = 1024.0 / gl_minimap_zoom->value;
	else
		distance = 1024.0;

	if (r_origin[0] + distance < node->minmaxs[0] ||
	    r_origin[0] - distance > node->minmaxs[3] ||

	    r_origin[1] + distance < node->minmaxs[1] ||
	    r_origin[1] - distance > node->minmaxs[4] ||

	    r_origin[2] + 256 < node->minmaxs[2] ||
	    r_origin[2] - 256 > node->minmaxs[5])
		return;

	/* if a leaf node, draw stuff */
	if (node->contents != -1) {
		pleaf = (mleaf_t *) node;
		/* check for door connected areas */
		if (r_refdef.areabits) {
			if (!(r_refdef.areabits[pleaf->area >> 3] & (1 << (pleaf->area & 7))))
				return;	/* not visible */
		}
		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;
		if (c) {
			do {
				(*mark)->visframe = r_framecount;
				mark++;
			} while (--c);
		}
		return;
	}
	/* node is just a decision point, so go down the apropriate sides */
	/* find which side of the node we are on */
	plane = node->plane;

	switch (plane->type) {
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct(modelorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0) {
		side = 0;
		sidebit = 0;
	} else {
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

	/* recurse down the children, front side first */
	R_RecursiveRadarNode(node->children[side]);

	if (plane->normal[2]) {
		/* draw stuff     */
		if (plane->normal[2] > 0)
			for (c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c; c--, surf++) {

				if (surf->texinfo->flags & SURF_SKY) {
					continue;
				}
				if (surf->texinfo->flags & (SURF_TRANS33 | SURF_TRANS66))
					qglColor4f(0, 1, 0, 0.5);
				else if (surf->texinfo->flags & (SURF_WARP | SURF_FLOWING))
					qglColor4f(0, 0, 1, 0.5);
				else
					qglColor4f(1, 1, 1, 1);

				for (p = surf->polys; p; p = p->chain) {
					v = p->verts[0];

					qglBegin(GL_TRIANGLE_FAN);
					for (i = 0; i < p->numverts; i++, v += VERTEXSIZE)
						qglVertex3fv(v);
					qglEnd();
				}

			}
	} else {
		qglDisable(GL_TEXTURE_2D);

		for (c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c; c--, surf++) {
			float		sColor  , C[4];

			if (surf->texinfo->flags & SURF_SKY) {
				continue;
			}
			if (surf->texinfo->flags & (SURF_WARP | SURF_FLOWING | SURF_TRANS33 | SURF_TRANS66)) {
				sColor = 0.5;
			} else {
				sColor = 0;
			}

			for (p = surf->polys; p; p = p->chain) {
				v = p->verts[0];

				qglBegin(GL_LINE_STRIP);
				for (i = 0; i < p->numverts; i++, v += VERTEXSIZE) {
					C[3] = (v[2] - r_origin[2]) / 256.0;
					if (C[3] > 0) {
						C[0] = 0.5;
						C[1] = 0.5 + sColor;
						C[2] = 0.5;
						C[3] = 1 - C[3];
					} else {
						C[0] = 0.5;
						C[1] = sColor;
						C[2] = 0;
						C[3] += 1;
					}
					if (C[3] < 0)
						C[3] = 0;
					qglColor4fv(C);
					qglVertex3fv(v);
				}
				qglEnd();
			}

		}
		qglEnable(GL_TEXTURE_2D);
	}
	/* recurse down the back side */
	R_RecursiveRadarNode(node->children[!side]);
}

int		numRadarEnts = 0;
RadarEnt_t	RadarEnts[MAX_RADAR_ENTS];
extern qboolean	have_stencil;

void
GL_DrawRadar(void)
{

	int		i;
	float		fS[4] = {0, 0, -1.0 / 512.0, 0};

	if (r_refdef.rdflags & RDF_NOWORLDMODEL)
		return;

	if (!gl_minimap->value)
		return;

	qglViewport(gl_minimap_x->value - gl_minimap_size->value, 
                    gl_minimap_y->value - gl_minimap_size->value,
                    gl_minimap_size->value, gl_minimap_size->value);

	GL_TexEnv(GL_MODULATE);
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity();

	if (gl_minimap_style->value) {
		qglOrtho(-1024, 1024, -1024, 1024, -256, 256);
	} else {
		qglOrtho(-1024, 1024, -512, 1536, -256, 256);
	}

	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
	qglLoadIdentity();

	qglDisable(GL_DEPTH_TEST);

	if (have_stencil) {
		qglClearStencil(0);
		qglClear(GL_STENCIL_BUFFER_BIT);
		qglEnable(GL_STENCIL_TEST);
		qglStencilFunc(GL_ALWAYS, 4, 4);
		qglStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

		GLSTATE_ENABLE_ALPHATEST;
		qglAlphaFunc(GL_LESS, 0.1);
		qglColorMask(0, 0, 0, 0);
		qglColor4f(1, 1, 1, 1);

		GL_Bind(r_around->texnum);
		qglBegin(GL_TRIANGLE_FAN);

		if (gl_minimap_style->value) {
			qglTexCoord2f(0, 1);
			qglVertex3f(1024, -1024, 1);
			qglTexCoord2f(1, 1);
			qglVertex3f(-1024, -1024, 1);
			qglTexCoord2f(1, 0);
			qglVertex3f(-1024, 1024, 1);
			qglTexCoord2f(0, 0);
			qglVertex3f(1024, 1024, 1);
		} else {
			qglTexCoord2f(0, 1);
			qglVertex3f(1024, -512, 1);
			qglTexCoord2f(1, 1);
			qglVertex3f(-1024, -512, 1);
			qglTexCoord2f(1, 0);
			qglVertex3f(-1024, 1536, 1);
			qglTexCoord2f(0, 0);
			qglVertex3f(1024, 1536, 1);
		}

		qglEnd();

		qglColorMask(1, 1, 1, 1);
		GLSTATE_DISABLE_ALPHATEST;
		qglAlphaFunc(GL_GREATER, 0.666);
		qglStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		qglStencilFunc(GL_NOTEQUAL, 4, 4);
	}
	if (gl_minimap_zoom->value >= 0.1)
		qglScalef(gl_minimap_zoom->value, gl_minimap_zoom->value, gl_minimap_zoom->value);

	if (gl_minimap_style->value) {
		qglPushMatrix();
		qglRotatef(90 - r_refdef.viewangles[1], 0, 0, -1);
		qglDisable(GL_TEXTURE_2D);
		qglBegin(GL_TRIANGLES);
		qglColor4f(1, 1, 0, 0.5);
		qglVertex3f(0, 32, 0);
		qglColor4f(1, 1, 0, 0.5);
		qglVertex3f(24, -32, 0);
		qglVertex3f(-24, -32, 0);
		qglEnd();
		qglPopMatrix();
	} else {
		qglDisable(GL_TEXTURE_2D);
		qglBegin(GL_TRIANGLES);
		qglColor4f(1, 1, 0, 0.5);
		qglVertex3f(0, 32, 0);
		qglColor4f(1, 1, 0, 0.5);
		qglVertex3f(24, -32, 0);
		qglVertex3f(-24, -32, 0);
		qglEnd();
		qglRotatef(90 - r_refdef.viewangles[1], 0, 0, 1);
	}

	qglTranslatef(-r_refdef.vieworg[0], -r_refdef.vieworg[1], -r_refdef.vieworg[2]);

	if (gl_minimap->value == 2) {
		qglBegin(GL_QUADS);
	} else {
		qglBegin(GL_TRIANGLES);
	}

	for (i = 0; i < numRadarEnts; i++) {
		float		x = RadarEnts[i].org[0];
		float		y = RadarEnts[i].org[1];
		float		z = RadarEnts[i].org[2];

		qglColor4fv(RadarEnts[i].color);

		if (gl_minimap->value == 2) {
			qglVertex3f(x + 9, y + 9, z);
			qglVertex3f(x + 9, y - 9, z);
			qglVertex3f(x - 9, y - 9, z);
			qglVertex3f(x - 9, y + 9, z);
		} else {
			qglVertex3f(x, y + 32, z);
			qglVertex3f(x + 27.7128, y - 16, z);
			qglVertex3f(x - 27.7128, y - 16, z);

			qglVertex3f(x, y - 32, z);
			qglVertex3f(x - 27.7128, y + 16, z);
			qglVertex3f(x + 27.7128, y + 16, z);
		}
	}

	qglEnd();

	qglEnable(GL_TEXTURE_2D);

	GL_Bind(r_radarmap->texnum);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE);
	GLSTATE_ENABLE_BLEND;
	qglColor3f(1, 1, 1);

	fS[3] = 0.5 + r_refdef.vieworg[2] / 512.0;
	qglTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

	GLSTATE_ENABLE_TEXGEN;
	qglTexGenfv(GL_S, GL_OBJECT_PLANE, fS);

	/* draw the stuff */
	R_RecursiveRadarNode(r_worldmodel->nodes);

	R_DrawAlphaSurfaces_Jitspoe();

	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GLSTATE_DISABLE_TEXGEN;

	qglPopMatrix();

	if (have_stencil)
		qglDisable(GL_STENCIL_TEST);

	qglViewport(gl_minimap_x->value, gl_minimap_y->value, vid.width, vid.height);

	GL_TexEnv(GL_REPLACE);

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);

}
