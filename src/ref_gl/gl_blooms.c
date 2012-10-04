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
/* r_bloom.c: 2D lighting post process effect */

#include "gl_local.h"

/*
 *
 * ============================================================================
 * ==
 *
 * LIGHT BLOOMS
 *
 * ============
 * =================================================================
 */

static float	Diamond8x[8][8] = {
	{0.0, 0.0, 0.0, 0.1, 0.1, 0.0, 0.0, 0.0},
	{0.0, 0.0, 0.2, 0.3, 0.3, 0.2, 0.0, 0.0},
	{0.0, 0.2, 0.4, 0.6, 0.6, 0.4, 0.2, 0.0},
	{0.1, 0.3, 0.6, 0.9, 0.9, 0.6, 0.3, 0.1},
	{0.1, 0.3, 0.6, 0.9, 0.9, 0.6, 0.3, 0.1},
	{0.0, 0.2, 0.4, 0.6, 0.6, 0.4, 0.2, 0.0},
	{0.0, 0.0, 0.2, 0.3, 0.3, 0.2, 0.0, 0.0},
	{0.0, 0.0, 0.0, 0.1, 0.1, 0.0, 0.0, 0.0}
};

static float	Diamond6x[6][6] = {
	{0.0, 0.0, 0.1, 0.1, 0.0, 0.0},
	{0.0, 0.3, 0.5, 0.5, 0.3, 0.0},
	{0.1, 0.5, 0.9, 0.9, 0.5, 0.1},
	{0.1, 0.5, 0.9, 0.9, 0.5, 0.1},
	{0.0, 0.3, 0.5, 0.5, 0.3, 0.0},
	{0.0, 0.0, 0.1, 0.1, 0.0, 0.0}
};

static float	Diamond4x[4][4] = {
	{0.3, 0.4, 0.4, 0.3},
	{0.4, 0.9, 0.9, 0.4},
	{0.4, 0.9, 0.9, 0.4},
	{0.3, 0.4, 0.4, 0.3}
};

static int	BLOOM_SIZE;

cvar_t         *gl_blooms_alpha;
cvar_t         *gl_blooms_diamond_size;
cvar_t         *gl_blooms_intensity;
cvar_t         *gl_blooms_darken;
cvar_t         *gl_blooms_sample_size;
cvar_t         *gl_blooms_fast_sample;

image_t        *r_bloomscreentexture;
image_t        *r_bloomeffecttexture;
image_t        *r_bloombackuptexture;
image_t        *r_bloomdownsamplingtexture;

static int	r_screendownsamplingtexture_size;
static int	screen_texture_width, screen_texture_height;
static int	r_screenbackuptexture_size;

/* current refdef size: */
static int	curView_x;
static int	curView_y;
static int	curView_width;
static int	curView_height;

/* texture coordinates of screen data inside screentexture */
static float	screenText_tcw;
static float	screenText_tch;

static int	sample_width;
static int	sample_height;

/* texture coordinates of adjusted textures */
static float	sampleText_tcw;
static float	sampleText_tch;

/* this macro is in sample size workspace coordinates */
#define R_Bloom_SamplePass( xpos, ypos ) \
	qglBegin(GL_QUADS); \
	qglTexCoord2f( 0, sampleText_tch); \
	qglVertex2f( xpos, ypos); \
	qglTexCoord2f( 0, 0); \
	qglVertex2f( xpos, ypos+sample_height); \
	qglTexCoord2f( sampleText_tcw, 0); \
	qglVertex2f( xpos+sample_width, ypos+sample_height); \
	qglTexCoord2f( sampleText_tcw, sampleText_tch); \
	qglVertex2f( xpos+sample_width, ypos); \
	qglEnd();

#define R_Bloom_Quad( x, y, width, height, textwidth, textheight ) \
	qglBegin(GL_QUADS); \
	qglTexCoord2f( 0, textheight); \
	qglVertex2f( x, y); \
	qglTexCoord2f( 0, 0); \
	qglVertex2f( x, y+height); \
	qglTexCoord2f( textwidth, 0); \
	qglVertex2f( x+width, y+height); \
	qglTexCoord2f( textwidth, textheight); \
	qglVertex2f( x+width, y); \
	qglEnd();



/*
 * ================= R_Bloom_InitBackUpTexture =================
 */
void
R_Bloom_InitBackUpTexture(int width, int height)
{
	byte           *data;

	data = malloc(width * height * 4);
	memset(data, 0, width * height * 4);

	r_screenbackuptexture_size = width;

	r_bloombackuptexture = GL_LoadPic("***r_bloombackuptexture***", (byte *) data, width, height, it_pic, 3);

	free(data);
}

/*
 * ================= R_Bloom_InitEffectTexture =================
 */
void
R_Bloom_InitEffectTexture(void)
{
	byte           *data;
	float		bloomsizecheck;

	if ((int)gl_blooms_sample_size->value < 32)
		ri.Cvar_SetValue("gl_blooms_sample_size", 32);

	/* make sure bloom size is a power of 2 */
	BLOOM_SIZE = (int)gl_blooms_sample_size->value;
	bloomsizecheck = (float)BLOOM_SIZE;
	while (bloomsizecheck > 1.0)
		bloomsizecheck /= 2.0;
	if (bloomsizecheck != 1.0) {
		BLOOM_SIZE = 32;
		while (BLOOM_SIZE < (int)gl_blooms_sample_size->value)
			BLOOM_SIZE *= 2;
	}
	/* make sure bloom size doesn't have stupid values */
	if (BLOOM_SIZE > screen_texture_width ||
	    BLOOM_SIZE > screen_texture_height)
		BLOOM_SIZE = min(screen_texture_width, screen_texture_height);

	if (BLOOM_SIZE != (int)gl_blooms_sample_size->value)
		ri.Cvar_SetValue("gl_blooms_sample_size", BLOOM_SIZE);

	data = malloc(BLOOM_SIZE * BLOOM_SIZE * 4);
	memset(data, 0, BLOOM_SIZE * BLOOM_SIZE * 4);

	r_bloomeffecttexture = GL_LoadPic("***r_bloomeffecttexture***", (byte *) data, BLOOM_SIZE, BLOOM_SIZE, it_pic, 3);

	free(data);
}

/*
 * ================= R_Bloom_InitTextures =================
 */
void
R_Bloom_InitTextures(void)
{
	byte           *data;
	int		size;

	/* find closer power of 2 to screen size  */
	for (screen_texture_width = 1; screen_texture_width < vid.width; screen_texture_width *= 2);
	for (screen_texture_height = 1; screen_texture_height < vid.height; screen_texture_height *= 2);

	/* disable blooms if we can't handle a texture of that size */

	/*
	 * if( screen_texture_width > gl_config.maxTextureSize ||
	 * screen_texture_height > glConfig.maxTextureSize ) {
	 * screen_texture_width = screen_texture_height = 0; Cvar_SetValue
	 * ("gl_blooms", 0); Com_Printf( "WARNING: 'R_InitBloomScreenTexture'
	 * too high resolution for Light Bloom. Effect disabled\n" ); return;
	 * }
	 */

	/* init the screen texture */
	size = screen_texture_width * screen_texture_height * 4;
	data = malloc(size);
	memset(data, 255, size);
	r_bloomscreentexture = GL_LoadPic("***r_bloomscreentexture***", (byte *) data, 
					screen_texture_width, screen_texture_height, it_pic, 3);
	free(data);

	/* validate bloom size and init the bloom effect texture */
	R_Bloom_InitEffectTexture();

	/*
	 * if screensize is more than 2x the bloom effect texture, set up for
	 * stepped downsampling
	 */
	r_bloomdownsamplingtexture = NULL;
	r_screendownsamplingtexture_size = 0;
	if (vid.width > (BLOOM_SIZE * 2) && !gl_blooms_fast_sample->value) {
		r_screendownsamplingtexture_size = (int)(BLOOM_SIZE * 2);
		data = malloc(r_screendownsamplingtexture_size * r_screendownsamplingtexture_size * 4);
		memset(data, 0, r_screendownsamplingtexture_size * r_screendownsamplingtexture_size * 4);
		r_bloomdownsamplingtexture = GL_LoadPic("***r_bloomdownsamplingtexture***", (byte *) data,
		 				r_screendownsamplingtexture_size, 
						r_screendownsamplingtexture_size, it_pic, 3);
		free(data);
	}
	/* Init the screen backup texture */
	if (r_screendownsamplingtexture_size)
		R_Bloom_InitBackUpTexture(r_screendownsamplingtexture_size, 
						r_screendownsamplingtexture_size);
	else
		R_Bloom_InitBackUpTexture(BLOOM_SIZE, BLOOM_SIZE);

}

/*
 * ================= R_InitBloomTextures =================
 */
void
R_InitBloomTextures(void)
{

	gl_blooms_alpha = ri.Cvar_Get("gl_blooms_alpha", "0.5", CVAR_ARCHIVE);
	gl_blooms_diamond_size = ri.Cvar_Get("gl_blooms_diamond_size", "8", CVAR_ARCHIVE);
	gl_blooms_intensity = ri.Cvar_Get("gl_blooms_intensity", "0.6", CVAR_ARCHIVE);
	gl_blooms_darken = ri.Cvar_Get("gl_blooms_darken", "4", CVAR_ARCHIVE);
	gl_blooms_sample_size = ri.Cvar_Get("gl_blooms_sample_size", "128", CVAR_ARCHIVE);
	gl_blooms_fast_sample = ri.Cvar_Get("gl_blooms_fast_sample", "0", CVAR_ARCHIVE);

	BLOOM_SIZE = 0;
	
	if (!gl_blooms->value)
		return;

	R_Bloom_InitTextures();
}


/*
 * ================= R_Bloom_DrawEffect =================
 */
void
R_Bloom_DrawEffect(void)
{
	GL_Bind(r_bloomeffecttexture->texnum);
	qglEnable(GL_BLEND);
	qglBlendFunc(GL_ONE, GL_ONE);
	qglColor4f(gl_blooms_alpha->value, gl_blooms_alpha->value, gl_blooms_alpha->value, 1.0);
	GL_TexEnv(GL_MODULATE);
	qglBegin(GL_QUADS);
	qglTexCoord2f(0, sampleText_tch);
	qglVertex2f(curView_x, curView_y);
	qglTexCoord2f(0, 0);
	qglVertex2f(curView_x, curView_y + curView_height);
	qglTexCoord2f(sampleText_tcw, 0);
	qglVertex2f(curView_x + curView_width, curView_y + curView_height);
	qglTexCoord2f(sampleText_tcw, sampleText_tch);
	qglVertex2f(curView_x + curView_width, curView_y);
	qglEnd();

	qglDisable(GL_BLEND);
}

/*
 * ================= R_Bloom_GeneratexDiamonds =================
 */
void
R_Bloom_GeneratexDiamonds(void)
{
	int		i, j;
	static float	blooms_intensity;

	/* set up sample size workspace */
	qglViewport(0, 0, sample_width, sample_height);
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	qglOrtho(0, sample_width, sample_height, 0, -10, 100);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	/* copy small scene into r_bloomeffecttexture */
	GL_Bind(r_bloomeffecttexture->texnum);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sample_width, sample_height);

	/* start modifying the small scene corner */
	qglColor4f(1.0, 1.0, 1.0, 1.0);
	qglEnable(GL_BLEND);

	/* darkening passes */
	if (gl_blooms_darken->value) {
		qglBlendFunc(GL_DST_COLOR, GL_ZERO);
		GL_TexEnv(GL_MODULATE);

		for (i = 0; i < gl_blooms_darken->value; i++) {
			R_Bloom_SamplePass(0, 0);
		}
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sample_width, sample_height);
	}
	/* bluring passes */
	/* qglBlendFunc(GL_ONE, GL_ONE); */
	qglBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);

	if (gl_blooms_diamond_size->value > 7 || gl_blooms_diamond_size->value <= 3) {
		if ((int)gl_blooms_diamond_size->value != 8)
			ri.Cvar_SetValue("gl_blooms_diamond_size", 8);

		for (i = 0; i < gl_blooms_diamond_size->value; i++) {
			for (j = 0; j < gl_blooms_diamond_size->value; j++) {
				blooms_intensity = gl_blooms_intensity->value * 0.3 * Diamond8x[i][j];
				if (blooms_intensity < 0.01)
					continue;
				qglColor4f(blooms_intensity, blooms_intensity, blooms_intensity, 1.0);
				R_Bloom_SamplePass(i - 4, j - 4);
			}
		}
	} else if (gl_blooms_diamond_size->value > 5) {

		if (gl_blooms_diamond_size->value != 6)
			ri.Cvar_SetValue("gl_blooms_diamond_size", 6);

		for (i = 0; i < gl_blooms_diamond_size->value; i++) {
			for (j = 0; j < gl_blooms_diamond_size->value; j++) {
				blooms_intensity = gl_blooms_intensity->value * 0.5 * Diamond6x[i][j];
				if (blooms_intensity < 0.01)
					continue;
				qglColor4f(blooms_intensity, blooms_intensity, blooms_intensity, 1.0);
				R_Bloom_SamplePass(i - 3, j - 3);
			}
		}
	} else if (gl_blooms_diamond_size->value > 3) {

		if ((int)gl_blooms_diamond_size->value != 4)
			ri.Cvar_SetValue("gl_blooms_diamond_size", 4);

		for (i = 0; i < gl_blooms_diamond_size->value; i++) {
			for (j = 0; j < gl_blooms_diamond_size->value; j++) {
				blooms_intensity = gl_blooms_intensity->value * 0.8 * Diamond4x[i][j];
				if (blooms_intensity < 0.01)
					continue;
				qglColor4f(blooms_intensity, blooms_intensity, blooms_intensity, 1.0);
				R_Bloom_SamplePass(i - 2, j - 2);
			}
		}
	}
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sample_width, sample_height);

	/* restore full screen workspace */
	qglViewport(0, 0, vid.width, vid.height);
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	qglOrtho(0, vid.width, vid.height, 0, -10, 100);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();
}

/*
 * ================= R_Bloom_DownsampleView =================
 */
void
R_Bloom_DownsampleView(void)
{
	qglDisable(GL_BLEND);
	qglColor4f(1.0, 1.0, 1.0, 1.0);

	/* stepped downsample */
	if (r_screendownsamplingtexture_size) {
		int		midsample_width = r_screendownsamplingtexture_size * sampleText_tcw;
		int		midsample_height = r_screendownsamplingtexture_size * sampleText_tch;

		/* copy the screen and draw resized */
		GL_Bind(r_bloomscreentexture->texnum);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, curView_x, 
					vid.height - (curView_y + curView_height), curView_width, curView_height);
		R_Bloom_Quad(0, vid.height - midsample_height, midsample_width, midsample_height, 
						screenText_tcw, screenText_tch);

		/* now copy into Downsampling (mid-sized) texture */
		GL_Bind(r_bloomdownsamplingtexture->texnum);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, midsample_width, midsample_height);

		/* now draw again in bloom size */
		qglColor4f(0.5, 0.5, 0.5, 1.0);
		R_Bloom_Quad(0, vid.height - sample_height, sample_width, sample_height, 
						sampleText_tcw, sampleText_tch);

		/*
		 * now blend the big screen texture into the bloom generation
		 * space (hoping it adds some blur)
		 */
		qglEnable(GL_BLEND);
		qglBlendFunc(GL_ONE, GL_ONE);
		qglColor4f(0.5, 0.5, 0.5, 1.0);
		GL_Bind(r_bloomscreentexture->texnum);
		R_Bloom_Quad(0, vid.height - sample_height, sample_width, sample_height, 
						screenText_tcw, screenText_tch);
		qglColor4f(1.0, 1.0, 1.0, 1.0);
		qglDisable(GL_BLEND);

	} else {		/* downsample simple */

		GL_Bind(r_bloomscreentexture->texnum);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, curView_x, 
					vid.height - (curView_y + curView_height), curView_width, curView_height);
		R_Bloom_Quad(0, vid.height - sample_height, sample_width, sample_height, 
					screenText_tcw, screenText_tch);
	}
}

/*
 * ================= R_BloomBlend =================
 */
void
R_BloomBlend(refdef_t * fd)
{
	if (!(fd->rdflags & RDF_BLOOM) || !gl_blooms->value)
		return;

	if (!BLOOM_SIZE)
		R_Bloom_InitTextures();

	if (screen_texture_width < BLOOM_SIZE ||
	    screen_texture_height < BLOOM_SIZE)
		return;

	/* set up full screen workspace */
	qglViewport(0, 0, vid.width, vid.height);
	qglDisable(GL_DEPTH_TEST);
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	qglOrtho(0, vid.width, vid.height, 0, -10, 100);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();
	qglDisable(GL_CULL_FACE);

	qglDisable(GL_BLEND);
	qglEnable(GL_TEXTURE_2D);

	qglColor4f(1, 1, 1, 1);

	/* set up current sizes */
	curView_x = fd->x;
	curView_y = fd->y;
	curView_width = fd->width;
	curView_height = fd->height;
	screenText_tcw = ((float)fd->width / (float)screen_texture_width);
	screenText_tch = ((float)fd->height / (float)screen_texture_height);
	if (fd->height > fd->width) {
		sampleText_tcw = ((float)fd->width / (float)fd->height);
		sampleText_tch = 1.0;
	} else {
		sampleText_tcw = 1.0;
		sampleText_tch = ((float)fd->height / (float)fd->width);
	}
	sample_width = BLOOM_SIZE * sampleText_tcw;
	sample_height = BLOOM_SIZE * sampleText_tch;

	/* copy the screen space we'll use to work into the backup texture */
	GL_Bind(r_bloombackuptexture->texnum);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 
			r_screenbackuptexture_size * sampleText_tcw, 
			r_screenbackuptexture_size * sampleText_tch);

	/* create the bloom image */
	R_Bloom_DownsampleView();
	R_Bloom_GeneratexDiamonds();
	/* R_Bloom_GeneratexCross(); */

	/* restore the screen-backup to the screen */
	qglDisable(GL_BLEND);
	GL_Bind(r_bloombackuptexture->texnum);
	qglColor4f(1, 1, 1, 1);
	R_Bloom_Quad(0,
	    vid.height - (r_screenbackuptexture_size * sampleText_tch),
	    r_screenbackuptexture_size * sampleText_tcw,
	    r_screenbackuptexture_size * sampleText_tch,
	    sampleText_tcw,
	    sampleText_tch);

	R_Bloom_DrawEffect();

	/* Knightmare added */
	R_SetupGL();
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglEnable(GL_TEXTURE_2D);
	qglColor4f(1, 1, 1, 1);
}

/*
 * ================================== GLARES
 * ==================================
 */

byte
mulc(int i1, const float mult)
{
	/* amplify colors a bit */
	int		r = i1 * mult;

	if (r > 255)
		return 255;
	else
		return r;
}

void
ProcessGlare(byte glarepixels[][4], int width, int height, const float mult)
{
	int		i;

	for (i = 0; i < width * height; i++) {

		if ((glarepixels[i][0] != 0) ||
		    (glarepixels[i][1] != 0) ||
		    (glarepixels[i][2] != 0)) {

			glarepixels[i][0] = mulc(glarepixels[i][0], mult);
			glarepixels[i][1] = mulc(glarepixels[i][1], mult);
			glarepixels[i][2] = mulc(glarepixels[i][2], mult);
		} else {
			glarepixels[i][0] = 0;
			glarepixels[i][1] = 0;
			glarepixels[i][2] = 0;
		}
	}
}

/*
 * Fast Idea Blurring from
 * http://www.gamasutra.com/features/20010209/evans_01.htm
 */

typedef struct bytecolor_s {
	byte		r , g, b, a;
} bytecolor_t;

typedef struct longcolor_s {
	unsigned long	r, g, b, a;
} longcolor_t;

void
DoPreComputation(bytecolor_t * src, int src_w, int src_h, longcolor_t * dst)
{
	longcolor_t	tot;
	int		y         , x;

	for (y = 0; y < src_h; y++) {
		for (x = 0; x < src_w; x++) {
			tot.r = src[0].r;
			tot.g = src[0].g;
			tot.b = src[0].b;
			tot.a = 255;

			if (x > 0) {
				tot.r += dst[-1].r;
				tot.g += dst[-1].g;
				tot.b += dst[-1].b;
			}
			if (y > 0) {
				tot.r += dst[-src_w].r;
				tot.g += dst[-src_w].g;
				tot.b += dst[-src_w].b;
			}
			if (x > 0 && y > 0) {
				tot.r -= dst[-src_w - 1].r;
				tot.g -= dst[-src_w - 1].g;
				tot.b -= dst[-src_w - 1].b;
			}
			dst->r = tot.r;
			dst->g = tot.g;
			dst->b = tot.b;
			dst->a = 255;
			dst++;
			src++;
		}
	}
}

/* this is a utility function used by DoBoxBlur below */
longcolor_t    *
ReadP(longcolor_t * p, int w, int h, int x, int y)
{
	if (x < 0)
		x = 0;
	else if (x >= w)
		x = w - 1;
	if (y < 0)
		y = 0;
	else if (y >= h)
		y = h - 1;
	return &p[x + y * w];
}

/* the main meat of the algorithm lies here */
void
DoBoxBlur(bytecolor_t * src, int src_w, int src_h, bytecolor_t * dst, longcolor_t * p, int boxw, int boxh)
{
	longcolor_t    *to1, *to2, *to3, *to4;
	int		y         , x;
	float		mul;

	if (boxw < 0 || boxh < 0) {
		memcpy(dst, src, src_w * src_h * 4);	/* deal with degenerate kernel sizes */
		return;
	}
	mul = 1.0 / ((boxw * 2 + 1) * (boxh * 2 + 1));
	for (y = 0; y < src_h; y++) {
		for (x = 0; x < src_w; x++) {
			to1 = ReadP(p, src_w, src_h, x + boxw, y + boxh);
			to2 = ReadP(p, src_w, src_h, x - boxw, y - boxh);
			to3 = ReadP(p, src_w, src_h, x - boxw, y + boxh);
			to4 = ReadP(p, src_w, src_h, x + boxw, y - boxh);

			dst->r = (to1->r + to2->r - to3->r - to4->r) * mul;
			dst->g = (to1->g + to2->g - to3->g - to4->g) * mul;
			dst->b = (to1->b + to2->b - to3->b - to4->b) * mul;

			dst->a = 255;

			dst++;
			src++;
		}
	}
}

void
R_RenderPic(int x, int y, int w, int h)
{
	qglBegin(GL_QUADS);
	qglTexCoord2f(0, 0);
	qglVertex2f(x, y);
	qglTexCoord2f(1, 0);
	qglVertex2f(x + w, y);
	qglTexCoord2f(1, -1);
	qglVertex2f(x + w, y + h);
	qglTexCoord2f(0, -1);
	qglVertex2f(x, y + h);

	qglEnd();
}

byte	imagepixels[256 * 256][4];
byte	glareblurpixels[256 * 256][4];
long	glaresum  [256 * 256][4];

const int 
powerof2s[] = {
	8, 16, 32, 64, 128, 256
};

int
checkResolution(int check)
{
	int		i;

	for (i = 5; i > 0; i--)
		if (check >= powerof2s[i])
			return powerof2s[i];

	return powerof2s[0];
}

float
CalcFov(float fov_x, float width, float height)
{
	float		a;
	float		x;

	if (fov_x < 1 || fov_x > 179)
		ri.Sys_Error(ERR_DROP, "Bad fov: %f", fov_x);

	x = width / tan(fov_x / 360 * M_PI);
	a = atan(height / x) * 360 / M_PI;
	return a;
}

void
R_PreRenderDynamic(refdef_t * fd)
{
	refdef_t	refdef;


	if (fd->rdflags & RDF_NOWORLDMODEL)
		return;

	if (gl_glares->value) {
		int	width = checkResolution(gl_glares_size->value);
		int	height = checkResolution(gl_glares_size->value);

		/* lower res wont get messed up */
		if (vid.width < width || vid.height < height)
			width = height = 128;

		memcpy(&refdef, fd, sizeof(refdef));

		refdef.x = 0;
		refdef.y = vid.height - height;
		refdef.width = width;
		refdef.height = height;
		refdef.fov_y = CalcFov(refdef.fov_x, refdef.width, refdef.height);
		refdef.rdflags |= RDF_LIGHTBLEND;


		R_Clear();
		R_RenderView(&refdef);

		GL_Bind(r_lblendimage->texnum);

		qglReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, imagepixels);

		ProcessGlare(imagepixels, width, height, 1 + gl_glares_intens->value * .5);
		DoPreComputation((bytecolor_t *) imagepixels, width, height, (longcolor_t *) glaresum);
		DoBoxBlur((bytecolor_t *) imagepixels, width, height,
		    (bytecolor_t *) glareblurpixels, (longcolor_t *) glaresum, 
		    gl_glares_intens->value * 3, gl_glares_intens->value * 3);
		    
		if (gl_glares->value != 1)
			ProcessGlare(glareblurpixels, width, height, 1 + gl_glares_intens->value / 3);
			
		qglTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, glareblurpixels);

		r_lblendimage->upload_width = width;
		r_lblendimage->upload_height = height;
	}
	R_Clear();
}

image_t        *r_lblendimage;
void
R_RenderGlares(refdef_t * fd)
{
	int		x, y, w, h;


	if (fd->rdflags & RDF_NOWORLDMODEL)
		return;
	if (!gl_glares->value)
		return;

	w = fd->width;
	h = fd->width;
	x = 0;
	y = (fd->width - fd->height) * -0.5;

	GL_Bind(r_lblendimage->texnum);
	qglBlendFunc(GL_ONE, GL_ONE);

	GLSTATE_DISABLE_ALPHATEST
	    GLSTATE_ENABLE_BLEND
	    GL_TexEnv(GL_MODULATE);
	qglColor4f(1, 1, 1, 1);

	qglBegin(GL_QUADS);
	R_RenderPic(x, y, w, h);
	qglEnd();

	GLSTATE_ENABLE_ALPHATEST
	    GLSTATE_DISABLE_BLEND
	    GL_TexEnv(GL_REPLACE);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
