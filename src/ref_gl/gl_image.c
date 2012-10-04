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

#include "gl_local.h"

#include "gl_refl.h"		/* MPO */

image_t		gltextures[MAX_GLTEXTURES];
int		numgltextures;
int		base_textureid;		/* gltextures[i] = base_textureid+i */

static byte	intensitytable[256];
static unsigned char gammatable[256];

cvar_t         *intensity;

unsigned	d_8to24table[256];

#ifndef QMAX
float		d_8to24tablef[256][3];
#endif

qboolean	GL_Upload8(byte * data, int width, int height, qboolean mipmap, qboolean is_sky);
qboolean	GL_Upload32(unsigned *data, int width, int height, qboolean mipmap);


int		gl_solid_format = 3;
int		gl_alpha_format = 4;

int		gl_tex_solid_format = 3;
int		gl_tex_alpha_format = 4;

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

void
GL_SetTexturePalette(unsigned palette[256])
{
	int		i;
	unsigned char	temptable[768];

	if (qglColorTableEXT) {
		for (i = 0; i < 256; i++) {
			temptable[i * 3 + 0] = (palette[i] >> 0) & 0xff;
			temptable[i * 3 + 1] = (palette[i] >> 8) & 0xff;
			temptable[i * 3 + 2] = (palette[i] >> 16) & 0xff;
		}

		qglColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT,
		    GL_RGB,
		    256,
		    GL_RGB,
		    GL_UNSIGNED_BYTE,
		    temptable);
	}
}

/* Knightmare- added some of Psychospaz's shortcuts */
GLenum		bFunc1 = -1;
GLenum		bFunc2 = -1;
void
GL_BlendFunction(GLenum sfactor, GLenum dfactor)
{
	if (sfactor != bFunc1 || dfactor != bFunc2) {
		bFunc1 = sfactor;
		bFunc2 = dfactor;

		qglBlendFunc(bFunc1, bFunc2);
	}
}

GLenum		shadeModelMode = -1;

void
GL_ShadeModel(GLenum mode)
{
	if (mode != shadeModelMode) {
		shadeModelMode = mode;
		qglShadeModel(mode);
	}
}

void
GL_EnableMultitexture(qboolean enable)
{
	if (!qglSelectTextureSGIS && !qglActiveTextureARB)
		return;

	if (enable) {
		GL_SelectTexture(GL_TEXTURE1);
		qglEnable(GL_TEXTURE_2D);
		GL_TexEnv(GL_REPLACE);
	} else {
		GL_SelectTexture(GL_TEXTURE1);
		qglDisable(GL_TEXTURE_2D);
		GL_TexEnv(GL_REPLACE);
	}

	GL_SelectTexture(GL_TEXTURE0);
	GL_TexEnv(GL_REPLACE);
}

void
GL_Enable3dTextureUnit(qboolean enable)
{
	if (!qglSelectTextureSGIS && !qglActiveTextureARB)
		return;

	if (enable) {
		GL_SelectTexture(GL_TEXTURE2);
		qglEnable(GL_TEXTURE_2D);
		GL_TexEnv(GL_REPLACE);
	} else {
		GL_SelectTexture(GL_TEXTURE2);
		qglDisable(GL_TEXTURE_2D);
		GL_TexEnv(GL_REPLACE);
	}

	GL_SelectTexture(GL_TEXTURE0);
}

void
GL_SelectTexture(GLenum texture)
{

	int		tmu;

	if (!qglSelectTextureSGIS && !qglActiveTextureARB) {
		return;
	}
	if
	    (texture == GL_TEXTURE0)
		tmu = 0;
	else if
	    (texture == GL_TEXTURE1)
		tmu = 1;
	else
		tmu = 2;

	if (tmu == gl_state.currenttmu)
		return;

	gl_state.currenttmu = tmu;

	if (qglSelectTextureSGIS) {
		qglSelectTextureSGIS(texture);
	} else if (qglActiveTextureARB) {
		qglActiveTextureARB(texture);
		qglClientActiveTextureARB(texture);
	}
}

void
GL_TexEnv(GLenum mode)
{
	static int	lastmodes[2] = {-1, -1};

	if (mode != lastmodes[gl_state.currenttmu]) {
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
		lastmodes[gl_state.currenttmu] = mode;
	}
}

void
GL_Bind(int texnum)
{
	extern image_t *draw_chars;

	if (gl_nobind->value && draw_chars)	/* performance evaluation option */
		texnum = draw_chars->texnum;
	if (gl_state.currenttextures[gl_state.currenttmu] == texnum)
		return;

	gl_state.currenttextures[gl_state.currenttmu] = texnum;
	qglBindTexture(GL_TEXTURE_2D, texnum);
}

void
GL_MBind(GLenum target, int texnum)
{

	GL_SelectTexture(target);

	if (target == GL_TEXTURE0) {
		if (gl_state.currenttextures[0] == texnum)
			return;
	} else if (target == GL_TEXTURE1) {
		if (gl_state.currenttextures[1] == texnum)
			return;
	} else {
		if (gl_state.currenttextures[2] == texnum)
			return;
	}

	GL_Bind(texnum);
}

typedef struct {
	char           *name;
	int		minimize, maximize;
} glmode_t;

glmode_t	modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUM_GL_MODES (sizeof(modes) / sizeof (glmode_t))

typedef struct {
	char           *name;
	int		mode;
} gltmode_t;

gltmode_t	gl_alpha_modes[] = {
	{"default", 4},
	{"GL_RGBA", GL_RGBA},
	{"GL_RGBA8", GL_RGBA8},
	{"GL_RGB5_A1", GL_RGB5_A1},
	{"GL_RGBA4", GL_RGBA4},
	{"GL_RGBA2", GL_RGBA2},
};

#define NUM_GL_ALPHA_MODES (sizeof(gl_alpha_modes) / sizeof (gltmode_t))

gltmode_t	gl_solid_modes[] = {
	{"default", 3},
	{"GL_RGB", GL_RGB},
	{"GL_RGB8", GL_RGB8},
	{"GL_RGB5", GL_RGB5},
	{"GL_RGB4", GL_RGB4},
	{"GL_R3_G3_B2", GL_R3_G3_B2},
#ifdef GL_RGB2_EXT
	{"GL_RGB2", GL_RGB2_EXT},
#endif
};

#define NUM_GL_SOLID_MODES (sizeof(gl_solid_modes) / sizeof (gltmode_t))

/*
 * =============== GL_TextureMode ===============
 */
void
GL_TextureMode(char *string)
{
	int		i;
	image_t        *glt;

	for (i = 0; i < NUM_GL_MODES; i++) {
		if (!Q_stricmp(modes[i].name, string))
			break;
	}

	if (i == NUM_GL_MODES) {
		ri.Con_Printf(PRINT_ALL, "bad filter name\n");
		return;
	}
	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;
	/* anisotropic */
	/* clamp selected anisotropy */
	if (gl_config.anisotropic) {
		if (gl_anisotropic->value > gl_config.max_anisotropy)
			ri.Cvar_SetValue("gl_anisotropic", gl_config.max_anisotropy);
		else if (gl_anisotropic->value < 1.0)
			ri.Cvar_SetValue("gl_anisotropic", 1.0);
	}
	/* change all the existing mipmap texture objects */
	for (i = 0, glt = gltextures; i < numgltextures; i++, glt++) {
		if (glt->type != it_pic && glt->type != it_sky && glt->type != it_part) {
			GL_Bind(glt->texnum);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
			/* anisotropic */
			/* Set anisotropic filter if supported and enabled */
			if (gl_config.anisotropic && gl_anisotropic->value) {
				qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_anisotropic->value);
				/* GLfloat largest_supported_anisotropy; */
				/* qglGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_ EXT, 
				   &largest_supported_anisotropy); */
				/* qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
				   largest_supported_anisotropy); */
			}
		}
	}
}


/*
 * =============== GL_TextureAlphaMode ===============
 */
void
GL_TextureAlphaMode(char *string)
{
	int		i;

	for (i = 0; i < NUM_GL_ALPHA_MODES; i++) {
		if (!Q_stricmp(gl_alpha_modes[i].name, string))
			break;
	}

	if (i == NUM_GL_ALPHA_MODES) {
		ri.Con_Printf(PRINT_ALL, "bad alpha texture mode name\n");
		return;
	}
	gl_tex_alpha_format = gl_alpha_modes[i].mode;
}

/*
 * =============== GL_TextureSolidMode ===============
 */
void
GL_TextureSolidMode(char *string)
{
	int		i;

	for (i = 0; i < NUM_GL_SOLID_MODES; i++) {
		if (!Q_stricmp(gl_solid_modes[i].name, string))
			break;
	}

	if (i == NUM_GL_SOLID_MODES) {
		ri.Con_Printf(PRINT_ALL, "bad solid texture mode name\n");
		return;
	}
	gl_tex_solid_format = gl_solid_modes[i].mode;
}

/*
 * =============== GL_ImageList_f ===============
 */
void
GL_ImageList_f(void)
{
	int		i;
	image_t        *image;
	int		texels;
	const char     *palstrings[2] = {
		"RGB",
		"PAL"
	};

	ri.Con_Printf(PRINT_ALL, "------------------\n");
	texels = 0;

	for (i = 0, image = gltextures; i < numgltextures; i++, image++) {
		if (image->texnum <= 0)
			continue;
		texels += image->upload_width * image->upload_height;
		switch (image->type) {
		case it_skin:
			ri.Con_Printf(PRINT_ALL, "M");
			break;
		case it_sprite:
			ri.Con_Printf(PRINT_ALL, "S");
			break;
		case it_wall:
			ri.Con_Printf(PRINT_ALL, "W");
			break;
		case it_pic:
			ri.Con_Printf(PRINT_ALL, "P");
			break;
		default:
			ri.Con_Printf(PRINT_ALL, " ");
			break;
		}

		ri.Con_Printf(PRINT_ALL, " %3i %3i %s: %s\n",
		    image->upload_width, image->upload_height, palstrings[image->paletted], image->name);
	}
	ri.Con_Printf(PRINT_ALL, "Total texel count (not counting mipmaps): %i\n", texels);
}


/*
 *
 * ============================================================================
 * =
 *
 * scrap allocation
 *
 * Allocate all the little status bar obejcts into a single texture to crutch up
 * inefficient hardware / drivers
 *
 * =============================================================================
 */

#define	MAX_SCRAPS		1
#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT		256
#define	IMAGERESMAX		1024

int		scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH * BLOCK_HEIGHT];
qboolean	scrap_dirty;

/* returns a texture number and the position inside it */
int
Scrap_AllocBlock(int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	for (texnum = 0; texnum < MAX_SCRAPS; texnum++) {
		best = BLOCK_HEIGHT;

		for (i = 0; i < BLOCK_WIDTH - w; i++) {
			best2 = 0;

			for (j = 0; j < w; j++) {
				if (scrap_allocated[texnum][i + j] >= best)
					break;
				if (scrap_allocated[texnum][i + j] > best2)
					best2 = scrap_allocated[texnum][i + j];
			}
			if (j == w) {	/* this is a valid spot */
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i = 0; i < w; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	return -1;
	/* Sys_Error ("Scrap_AllocBlock: full"); */
}

int		scrap_uploads;

void
Scrap_Upload(void)
{
	scrap_uploads++;
	GL_Bind(TEXNUM_SCRAPS);
	GL_Upload8(scrap_texels[0], BLOCK_WIDTH, BLOCK_HEIGHT, false, false);
	scrap_dirty = false;
}

/*
 * =================================================================
 *
 * PCX LOADING
 *
 * =================================================================
 */


/*
 * ============== LoadPCX ==============
 */
void
LoadPCX(char *filename, byte ** pic, byte ** palette, int *width, int *height)
{
	byte           *raw;
	pcx_t          *pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte           *out, *pix;

	*pic = NULL;
	*palette = NULL;

	/* load the file */
	len = ri.FS_LoadFile(filename, (void **)&raw);
	if (!raw) {
		ri.Con_Printf(PRINT_DEVELOPER, "Bad pcx file %s\n", filename);
		return;
	}
	
	/* parse the PCX file */
	pcx = (pcx_t *) raw;
	pcx->xmin = LittleShort(pcx->xmin);
	pcx->ymin = LittleShort(pcx->ymin);
	pcx->xmax = LittleShort(pcx->xmax);
	pcx->ymax = LittleShort(pcx->ymax);
	pcx->hres = LittleShort(pcx->hres);
	pcx->vres = LittleShort(pcx->vres);
	pcx->bytes_per_line = LittleShort(pcx->bytes_per_line);
	pcx->palette_type = LittleShort(pcx->palette_type);

	raw = &pcx->data;

	if (pcx->manufacturer != 0x0a
	    || pcx->version != 5
	    || pcx->encoding != 1
	    || pcx->bits_per_pixel != 8
	    || pcx->xmax >= 640
	    || pcx->ymax >= 480) {
		ri.Con_Printf(PRINT_ALL, "Bad pcx file %s\n", filename);
		return;
	}
	out = malloc((pcx->ymax + 1) * (pcx->xmax + 1));

	*pic = out;

	pix = out;

	if (palette) {
		*palette = malloc(768);
		memcpy(*palette, (byte *) pcx + len - 768, 768);
	}
	if (width)
		*width = pcx->xmax + 1;
	if (height)
		*height = pcx->ymax + 1;

	for (y = 0; y <= pcx->ymax; y++, pix += pcx->xmax + 1) {
		for (x = 0; x <= pcx->xmax;) {
			dataByte = *raw++;

			if ((dataByte & 0xC0) == 0xC0) {
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			} else
				runLength = 1;

			while (runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	if (raw - (byte *) pcx > len) {
		ri.Con_Printf(PRINT_DEVELOPER, "PCX file %s was malformed", filename);
		free(*pic);
		*pic = NULL;
	}
	ri.FS_FreeFile(pcx);
}

/*
 * =========================================================
 *
 * TARGA LOADING
 *
 * =========================================================
 */

typedef struct _TargaHeader {
	unsigned char	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
 * ============= LoadTGA =============
 */
/* Definitions for image types */
#define TGA_Null		0	/* no image data */
#define TGA_Map			1	/* Uncompressed, color-mapped images */
#define TGA_RGB			2	/* Uncompressed, RGB images */
#define TGA_Mono		3	/* Uncompressed, black and white images */
#define TGA_RLEMap		9	/* Runlength encoded color-mapped images */
#define TGA_RLERGB		10	/* Runlength encoded RGB images */
#define TGA_RLEMono		11	/* Compressed, black and white images */
#define TGA_CompMap		32	/* Compressed color-mapped data, using Huffman, Delta, and runlength encoding */
#define TGA_CompMap4		33	/* 4-pass quadtree-type process */
/* Definitions for interleave flag */
#define TGA_IL_None		0	/* non-interleaved */
#define TGA_IL_Two		1	/* two-way (even/odd) interleaving */
#define TGA_IL_Four		2	/* four way interleaving */
#define TGA_IL_Reserved		3	/* reserved */
/* Definitions for origin flag */
#define TGA_O_UPPER		0	/* Origin in lower left-hand corner */
#define TGA_O_LOWER		1	/* Origin in upper left-hand corner */
#define MAXCOLORS 		16384

/*
 * ============= LoadTGA NiceAss: LoadTGA() from Q2Ice, it supports more
 * formats =============
 */
void
LoadTGA(char *filename, byte ** pic, int *width, int *height)
{
	int		w, h, x, y, i, temp1, temp2;
	int		realrow, truerow, baserow, size, interleave, origin;
	int		pixel_size, map_idx, mapped, rlencoded, RLE_count, RLE_flag;
	TargaHeader	header;
	byte		tmp[2], r, g, b, a, j, k, l;
	byte           *dst, *ColorMap, *data, *pdata;

	/* load file */
	ri.FS_LoadFile(filename, (void **)&data);

	if (!data)
		return;

	pdata = data;

	header.id_length = *pdata++;
	header.colormap_type = *pdata++;
	header.image_type = *pdata++;

	tmp[0] = pdata[0];
	tmp[1] = pdata[1];
	header.colormap_index = LittleShort(*((short *)tmp));
	pdata += 2;
	tmp[0] = pdata[0];
	tmp[1] = pdata[1];
	header.colormap_length = LittleShort(*((short *)tmp));
	pdata += 2;
	header.colormap_size = *pdata++;
	header.x_origin = LittleShort(*((short *)pdata));
	pdata += 2;
	header.y_origin = LittleShort(*((short *)pdata));
	pdata += 2;
	header.width = LittleShort(*((short *)pdata));
	pdata += 2;
	header.height = LittleShort(*((short *)pdata));
	pdata += 2;
	header.pixel_size = *pdata++;
	header.attributes = *pdata++;

	if (header.id_length)
		pdata += header.id_length;

	/* validate TGA type */
	switch (header.image_type) {
	case TGA_Map:
	case TGA_RGB:
	case TGA_Mono:
	case TGA_RLEMap:
	case TGA_RLERGB:
	case TGA_RLEMono:
		break;
	default:
		ri.Sys_Error(ERR_DROP, "LoadTGA: Only type 1 (map), 2 (RGB), 3 (mono), 9 (RLEmap), 10 (RLERGB), 11 (RLEmono) TGA images supported\n");
		return;
	}

	/* validate color depth */
	switch (header.pixel_size) {
	case 8:
	case 15:
	case 16:
	case 24:
	case 32:
		break;
	default:
		ri.Sys_Error(ERR_DROP, "LoadTGA: Only 8, 15, 16, 24 and 32 bit images (with colormaps) supported\n");
		return;
	}

	r = g = b = a = l = 0;

	/* if required, read the color map information */
	ColorMap = NULL;
	mapped = (header.image_type == TGA_Map || header.image_type == TGA_RLEMap || header.image_type == TGA_CompMap || header.image_type == TGA_CompMap4) && header.colormap_type == 1;
	if (mapped) {
		/* validate colormap size */
		switch (header.colormap_size) {
		case 8:
		case 16:
		case 32:
		case 24:
			break;
		default:
			ri.Sys_Error(ERR_DROP, "LoadTGA: Only 8, 16, 24 and 32 bit colormaps supported\n");
			return;
		}

		temp1 = header.colormap_index;
		temp2 = header.colormap_length;
		if ((temp1 + temp2 + 1) >= MAXCOLORS) {
			ri.FS_FreeFile(data);
			return;
		}
		ColorMap = (byte *) malloc(MAXCOLORS * 4);
		map_idx = 0;
		for (i = temp1; i < temp1 + temp2; ++i, map_idx += 4) {

			/* read appropriate number of bytes, break into rgb & put in map */
			switch (header.colormap_size) {
			case 8:
				r = g = b = *pdata++;
				a = 255;
				break;
			case 15:
				j = *pdata++;
				k = *pdata++;
				l = ((unsigned int)k << 8) + j;
				r = (byte) (((k & 0x7C) >> 2) << 3);
				g = (byte) ((((k & 0x03) << 3) + ((j & 0xE0) >> 5)) << 3);
				b = (byte) ((j & 0x1F) << 3);
				a = 255;
				break;
			case 16:
				j = *pdata++;
				k = *pdata++;
				l = ((unsigned int)k << 8) + j;
				r = (byte) (((k & 0x7C) >> 2) << 3);
				g = (byte) ((((k & 0x03) << 3) + ((j & 0xE0) >> 5)) << 3);
				b = (byte) ((j & 0x1F) << 3);
				a = (k & 0x80) ? 255 : 0;
				break;
			case 24:
				b = *pdata++;
				g = *pdata++;
				r = *pdata++;
				a = 255;
				l = 0;
				break;
			case 32:
				b = *pdata++;
				g = *pdata++;
				r = *pdata++;
				a = *pdata++;
				l = 0;
				break;
			}
			ColorMap[map_idx + 0] = r;
			ColorMap[map_idx + 1] = g;
			ColorMap[map_idx + 2] = b;
			ColorMap[map_idx + 3] = a;
		}
	}
	/* check run-length encoding */
	rlencoded = header.image_type == TGA_RLEMap || header.image_type == TGA_RLERGB || header.image_type == TGA_RLEMono;
	RLE_count = 0;
	RLE_flag = 0;

	w = header.width;
	h = header.height;

	if (width)
		*width = w;
	if (height)
		*height = h;

	size = w * h * 4;
	*pic = (byte *) malloc(size);

	memset(*pic, 0, size);

	/* read the Targa file body and convert to portable format */
	pixel_size = header.pixel_size;
	origin = (header.attributes & 0x20) >> 5;
	interleave = (header.attributes & 0xC0) >> 6;
	truerow = 0;
	baserow = 0;
	for (y = 0; y < h; y++) {
		realrow = truerow;
		if (origin == TGA_O_UPPER)
			realrow = h - realrow - 1;

		dst = *pic + realrow * w * 4;

		for (x = 0; x < w; x++) {
			/* check if run length encoded */
			if (rlencoded) {
				if (!RLE_count) {
					/* have to restart run */
					i = *pdata++;
					RLE_flag = (i & 0x80);
					if (!RLE_flag) {
						/* stream of unencoded pixels */
						RLE_count = i + 1;
					} else {
						/* single pixel replicated */
						RLE_count = i - 127;
					}
					/* decrement count & get pixel */
					--RLE_count;
				} else {

					/* have already read count & (at least) first pixel */
					--RLE_count;
					if (RLE_flag)
						/* replicated pixels */
						goto PixEncode;
				}
			}
			/* read appropriate number of bytes, break into RGB */
			switch (pixel_size) {
			case 8:
				r = g = b = l = *pdata++;
				a = 255;
				break;
			case 15:
				j = *pdata++;
				k = *pdata++;
				l = ((unsigned int)k << 8) + j;
				r = (byte) (((k & 0x7C) >> 2) << 3);
				g = (byte) ((((k & 0x03) << 3) + ((j & 0xE0) >> 5)) << 3);
				b = (byte) ((j & 0x1F) << 3);
				a = 255;
				break;
			case 16:
				j = *pdata++;
				k = *pdata++;
				l = ((unsigned int)k << 8) + j;
				r = (byte) (((k & 0x7C) >> 2) << 3);
				g = (byte) ((((k & 0x03) << 3) + ((j & 0xE0) >> 5)) << 3);
				b = (byte) ((j & 0x1F) << 3);
				a = 255;
				break;
			case 24:
				b = *pdata++;
				g = *pdata++;
				r = *pdata++;
				a = 255;
				l = 0;
				break;
			case 32:
				b = *pdata++;
				g = *pdata++;
				r = *pdata++;
				a = *pdata++;
				l = 0;
				break;
			default:
				ri.Sys_Error(ERR_DROP, "Illegal pixel_size '%d' in file '%s'\n", filename);
				return;
			}

	PixEncode:
			if (mapped) {
				map_idx = l * 4;
				*dst++ = ColorMap[map_idx + 0];
				*dst++ = ColorMap[map_idx + 1];
				*dst++ = ColorMap[map_idx + 2];
				*dst++ = ColorMap[map_idx + 3];
			} else {
				*dst++ = r;
				*dst++ = g;
				*dst++ = b;
				*dst++ = a;
			}
		}

		if (interleave == TGA_IL_Four)
			truerow += 4;
		else if (interleave == TGA_IL_Two)
			truerow += 2;
		else
			truerow++;

		if (truerow >= h)
			truerow = ++baserow;
	}

	if (mapped)
		free(ColorMap);

	ri.FS_FreeFile(data);
}

/*
 * =================================================================
 *
 * JPEG LOADING
 *
 * By Robert 'Heffo' Heffernan
 *
 * =================================================================
 */

void
jpg_null(j_decompress_ptr cinfo)
{
}

boolean
jpg_fill_input_buffer(j_decompress_ptr cinfo)
{
	ri.Con_Printf(PRINT_ALL, "Premature end of JPEG data\n");
	return 1;
}

void
jpg_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{

	cinfo->src->next_input_byte += (size_t) num_bytes;
	cinfo->src->bytes_in_buffer -= (size_t) num_bytes;

	if (cinfo->src->bytes_in_buffer < 0)
		ri.Con_Printf(PRINT_ALL, "Premature end of JPEG data\n");
}

void
jpeg_mem_src(j_decompress_ptr cinfo, byte * mem, int len)
{
	cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_source_mgr));
	cinfo->src->init_source = jpg_null;
	cinfo->src->fill_input_buffer = jpg_fill_input_buffer;
	cinfo->src->skip_input_data = jpg_skip_input_data;
	cinfo->src->resync_to_restart = jpeg_resync_to_restart;
	cinfo->src->term_source = jpg_null;
	cinfo->src->bytes_in_buffer = len;
	cinfo->src->next_input_byte = mem;
}

/*
 * ============== LoadJPG ==============
 */
void
LoadJPG(char *filename, byte ** pic, int *width, int *height)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	byte           *rawdata, *rgbadata, *scanline, *p, *q;
	int		rawsize, i;

	*pic = NULL;

	/* Load JPEG file into memory */
	rawsize = ri.FS_LoadFile(filename, (void **)&rawdata);
	if (!rawdata)
		return;

	/* Knightmare- check for bad data  */
	if (rawdata[6] != 'J'
	    || rawdata[7] != 'F'
	    || rawdata[8] != 'I'
	    || rawdata[9] != 'F') {
		ri.Con_Printf(PRINT_ALL, "Bad jpg file %s\n", filename);
		ri.FS_FreeFile(rawdata);
		return;
	}
	/* Initialise libJpeg Object */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	/* Feed JPEG memory into the libJpeg Object */
	jpeg_mem_src(&cinfo, rawdata, rawsize);

	/* Process JPEG header */
	jpeg_read_header(&cinfo, true);

	/* Start Decompression */
	jpeg_start_decompress(&cinfo);

	/* Check Colour Components */
	if (cinfo.output_components != 3) {
		ri.Con_Printf(PRINT_ALL, "Invalid JPEG colour components\n");
		jpeg_destroy_decompress(&cinfo);
		ri.FS_FreeFile(rawdata);
		return;
	}
	/* Allocate Memory for decompressed image */
	rgbadata = malloc(cinfo.output_width * cinfo.output_height * 4);
	if (!rgbadata) {
		ri.Con_Printf(PRINT_ALL, "Insufficient RAM for JPEG buffer\n");
		jpeg_destroy_decompress(&cinfo);
		ri.FS_FreeFile(rawdata);
		return;
	}
	/* Pass sizes to output */
	*width = cinfo.output_width;
	*height = cinfo.output_height;

	/* Allocate Scanline buffer */
	scanline = malloc(cinfo.output_width * 3);
	if (!scanline) {
		ri.Con_Printf(PRINT_ALL, "Insufficient RAM for JPEG scanline buffer\n");
		free(rgbadata);
		jpeg_destroy_decompress(&cinfo);
		ri.FS_FreeFile(rawdata);
		return;
	}
	/* Read Scanlines, and expand from RGB to RGBA */
	q = rgbadata;
	while (cinfo.output_scanline < cinfo.output_height) {
		p = scanline;
		jpeg_read_scanlines(&cinfo, &scanline, 1);

		for (i = 0; i < cinfo.output_width; i++) {
			q[0] = p[0];
			q[1] = p[1];
			q[2] = p[2];
			q[3] = 255;

			p += 3;
			q += 4;
		}
	}

	/* Free the scanline buffer */
	free(scanline);

	/* Finish Decompression */
	jpeg_finish_decompress(&cinfo);

	/* Destroy JPEG object */
	jpeg_destroy_decompress(&cinfo);

	/* Return the 'rgbadata' */
	*pic = rgbadata;
}


/*
 * =============================================================
 *
 * PNG STUFF
 *
 * =============================================================
 */

typedef struct png_s {			/* TPng = class(TGraphic) */
	char           *tmpBuf;
	int		tmpi;
	long		FBgColor;	/* DL Background color Added 30/05/2000 */
	int		FTransparent;	/* DL Is this Image Transparent Added 30/05/2000 */
	long		FRowBytes;	/* DL Added 30/05/2000 */
	double		FGamma;		/* DL Added 07/06/2000 */
	double		FScreenGamma;	/* DL Added 07/06/2000 */
	char           *FRowPtrs;	/* DL Changed for consistancy 30/05/2000   */
	char           *Data;		/* property Data: pByte read fData; */
	char           *Title;
	char           *Author;
	char           *Description;
	int		BitDepth;
	int		BytesPerPixel;
	int		ColorType;
	unsigned long	Height;
	unsigned long	Width;
	int		Interlace;
	int		Compression;
	int		Filter;
	double		LastModified;
	int		Transparent;
} png_t;

png_t          *my_png = 0;
unsigned char  *pngbytes;

void
InitializeDemData()
{
	long           *cvaluep;/* ub */
	long		y;

	/* Initialize Data and RowPtrs */
	if (my_png->Data) {
		free(my_png->Data);
		my_png->Data = 0;
	}
	if (my_png->FRowPtrs) {
		free(my_png->FRowPtrs);
		my_png->FRowPtrs = 0;
	}
	my_png->Data = malloc(my_png->Height * my_png->FRowBytes);	/* DL Added 30/5/2000 */
	my_png->FRowPtrs = malloc(sizeof(void *) * my_png->Height);

	if ((my_png->Data) && (my_png->FRowPtrs)) {
		cvaluep = (long *)my_png->FRowPtrs;
		for (y = 0; y < my_png->Height; y++) {
			cvaluep[y] = (long)my_png->Data + (y * (long)my_png->FRowBytes); /* DL Added 08/07/2000 */
		}
	}
}

void
mypng_struct_create()
{
	if (my_png)
		return;
	my_png = malloc(sizeof(png_t));
	my_png->Data = 0;
	my_png->FRowPtrs = 0;
	my_png->Height = 0;
	my_png->Width = 0;
	my_png->ColorType = PNG_COLOR_TYPE_RGB;
	my_png->Interlace = PNG_INTERLACE_NONE;
	my_png->Compression = PNG_COMPRESSION_TYPE_DEFAULT;
	my_png->Filter = PNG_FILTER_TYPE_DEFAULT;
}

void
mypng_struct_destroy(qboolean keepData)
{
	if (!my_png)
		return;
	if (my_png->Data && !keepData)
		free(my_png->Data);
	if (my_png->FRowPtrs)
		free(my_png->FRowPtrs);
	free(my_png);
	my_png = 0;
}

/*
 * =============================================================
 *
 * fReadData
 *
 * =============================================================
 */

void PNGAPI
fReadData(png_structp png, png_bytep data, png_size_t length)
{				/* called by pnglib */
	int		i;

	for (i = 0; i < length; i++)
		data[i] = my_png->tmpBuf[my_png->tmpi++];	/* give pnglib a some more bytes   */
}

/*
 * =============================================================
 *
 * LoadPNG
 *
 * =============================================================
 */

void
LoadPNG(char *filename, byte ** pic, int *width, int *height)
{
	png_structp	png;
	png_infop	pnginfo;
	byte		ioBuffer  [8192];
	int		len;
	byte           *raw;

	*pic = 0;

	len = ri.FS_LoadFile(filename, (void **)&raw);

	if (!raw) {
		ri.Con_Printf(PRINT_DEVELOPER, "Bad png file %s\n", filename);
		return;
	}
	if (png_sig_cmp(raw, 0, 4))
		return;

	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png)
		return;

	pnginfo = png_create_info_struct(png);

	if (!pnginfo) {
		png_destroy_read_struct(&png, &pnginfo, 0);
		return;
	}
	png_set_sig_bytes(png, 0 /* sizeof( sig ) */ );

	mypng_struct_create();	/* creates the my_png struct */

	my_png->tmpBuf = (char *)raw;	/* buf = whole file content */
	my_png->tmpi = 0;

	png_set_read_fn(png, ioBuffer, fReadData);

	png_read_info(png, pnginfo);

	png_get_IHDR(png, pnginfo, &my_png->Width, &my_png->Height, &my_png->BitDepth, &my_png->ColorType, &my_png->Interlace, &my_png->Compression, &my_png->Filter);
	
	/* ...removed bgColor code here... */
	if (my_png->ColorType == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);
	if (my_png->ColorType == PNG_COLOR_TYPE_GRAY && my_png->BitDepth < 8)
		png_set_gray_1_2_4_to_8(png);

	/* Add alpha channel if present */
	if (png_get_valid(png, pnginfo, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	/* hax: expand 24bit to 32bit */
	if (my_png->BitDepth == 8 && my_png->ColorType == PNG_COLOR_TYPE_RGB)
		png_set_filler(png, 255, PNG_FILLER_AFTER);

	if ((my_png->ColorType == PNG_COLOR_TYPE_GRAY) || (my_png->ColorType == PNG_COLOR_TYPE_GRAY_ALPHA))
		png_set_gray_to_rgb(png);


	if (my_png->BitDepth < 8)
		png_set_expand(png);

	/* update the info structure */
	png_read_update_info(png, pnginfo);

	my_png->FRowBytes = png_get_rowbytes(png, pnginfo);
	my_png->BytesPerPixel = png_get_channels(png, pnginfo);	/* DL Added 30/08/2000 */

	InitializeDemData();
	if ((my_png->Data) && (my_png->FRowPtrs))
		png_read_image(png, (png_bytepp) my_png->FRowPtrs);

	png_read_end(png, pnginfo);	/* read last information chunks */

	png_destroy_read_struct(&png, &pnginfo, 0);


	/* only load 32 bit by now... */
	if (my_png->BitDepth == 8) {
		*pic = (byte *)my_png->Data;
		*width = my_png->Width;
		*height = my_png->Height;
	} else {
		ri.Con_Printf(PRINT_DEVELOPER, "Bad png color depth: %s\n", filename);
		*pic = NULL;
		free(my_png->Data);
	}

	mypng_struct_destroy(true);
	ri.FS_FreeFile((void *)raw);
}

/*
 * ====================================================================
 *
 * IMAGE FLOOD FILLING
 *
 * ====================================================================
 */


/*
 * ================= Mod_FloodFillSkin
 *
 * Fill background pixels so mipmapping doesn't have haloes =================
 */

typedef struct {
	short		x, y;
} floodfill_t;

/* must be a power of 2 */
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
}

void
R_FloodFillSkin(byte * skin, int skinwidth, int skinheight)
{
	byte		fillcolor = *skin;	/* assume this is the pixel to fill */
	floodfill_t	fifo[FLOODFILL_FIFO_SIZE];
	int		inpt = 0, outpt = 0;
	int		filledcolor = -1;
	int		i;

	if (filledcolor == -1) {
		filledcolor = 0;
		/* attempt to find opaque black */
		for (i = 0; i < 256; ++i)
			if (d_8to24table[i] == (255 << 0)) {	/* alpha 1.0 */
				filledcolor = i;
				break;
			}
	}

	/* can't fill to filled color or to transparent color (used as visited marker) */
	if ((fillcolor == filledcolor) || (fillcolor == 255)) {

		/* printf( "not filling skin from %d to %d\n", fillcolor, filledcolor ); */
		return;
	}
	fifo[inpt].x = 0, fifo[inpt].y = 0;
	inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

	while (outpt != inpt) {
		int		x = fifo[outpt].x, y = fifo[outpt].y;
		int		fdc = filledcolor;
		byte           *pos = &skin[x + skinwidth * y];

		outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

		if (x > 0)
			FLOODFILL_STEP(-1, -1, 0);
		if (x < skinwidth - 1)
			FLOODFILL_STEP(1, 1, 0);
		if (y > 0)
			FLOODFILL_STEP(-skinwidth, 0, -1);
		if (y < skinheight - 1)
			FLOODFILL_STEP(skinwidth, 0, 1);
		skin[x + skinwidth * y] = fdc;
	}
}

/* ======================================================= */


/*
 * ================ GL_ResampleTexture ================
 */
void
GL_ResampleTexture(unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight)
{
	int		i, j;
	unsigned       *inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[1024], p2[1024];
	byte           *pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth * 0x10000 / outwidth;

	frac = fracstep >> 2;
	for (i = 0; i < outwidth; i++) {
		p1[i] = 4 * (frac >> 16);
		frac += fracstep;
	}
	frac = 3 * (fracstep >> 2);
	for (i = 0; i < outwidth; i++) {
		p2[i] = 4 * (frac >> 16);
		frac += fracstep;
	}

	for (i = 0; i < outheight; i++, out += outwidth) {
		inrow = in + inwidth * (int)((i + 0.25) * inheight / outheight);
		inrow2 = in + inwidth * (int)((i + 0.75) * inheight / outheight);
		frac = fracstep >> 1;
		for (j = 0; j < outwidth; j++) {
			pix1 = (byte *) inrow + p1[j];
			pix2 = (byte *) inrow + p2[j];
			pix3 = (byte *) inrow2 + p1[j];
			pix4 = (byte *) inrow2 + p2[j];
			((byte *) (out + j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
			((byte *) (out + j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
			((byte *) (out + j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
			((byte *) (out + j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
		}
	}
}

/*
 * ================ GL_LightScaleTexture
 *
 * Scale up the pixel values in a texture to increase the lighting range
 * ================
 */
void
GL_LightScaleTexture(unsigned *in, int inwidth, int inheight, qboolean only_gamma)
{
	if (only_gamma) {
		double		i, c;
		byte           *p;

		p = (byte *) in;

		c = inwidth * inheight;
		for (i = 0; i < c; i++, p += 4) {
			p[0] = gammatable[p[0]];
			p[1] = gammatable[p[1]];
			p[2] = gammatable[p[2]];
		}
	} else {
		double		i, c;
		byte           *p;

		p = (byte *) in;

		c = inwidth * inheight;
		for (i = 0; i < c; i++, p += 4) {
			p[0] = gammatable[intensitytable[p[0]]];
			p[1] = gammatable[intensitytable[p[1]]];
			p[2] = gammatable[intensitytable[p[2]]];
		}
	}
}

/*
 * ================ GL_MipMap
 *
 * Operates in place, quartering the size of the texture ================
 */
void
GL_MipMap(byte * in, int width, int height)
{
	int		i, j;
	byte           *out;

	width <<= 2;
	height >>= 1;
	out = in;
	for (i = 0; i < height; i++, in += width) {
		for (j = 0; j < width; j += 8, out += 4, in += 8) {
			out[0] = (in[0] + in[4] + in[width + 0] + in[width + 4]) >> 2;
			out[1] = (in[1] + in[5] + in[width + 1] + in[width + 5]) >> 2;
			out[2] = (in[2] + in[6] + in[width + 2] + in[width + 6]) >> 2;
			out[3] = (in[3] + in[7] + in[width + 3] + in[width + 7]) >> 2;
		}
	}
}

/*
 * =============== GL_Upload32
 *
 * Returns has_alpha ===============
 */
void
GL_BuildPalettedTexture(unsigned char *paletted_texture, unsigned char *scaled, int scaled_width, int scaled_height)
{
	double		i;

	for (i = 0; i < (double)(scaled_width) * (double)(scaled_height); i++) {
		unsigned int	r , g, b, c;

		r = (scaled[0] >> 3) & 31;
		g = (scaled[1] >> 2) & 63;
		b = (scaled[2] >> 3) & 31;

		c = r | (g << 5) | (b << 11);

		paletted_texture[(int)(i)] = gl_state.d_16to8table[c];

		scaled += 4;
	}
}

/*
 * =============== here starts modified code by Heffo/changes by Nexus
 * ===============
 */


int		upload_width, upload_height;
qboolean	uploaded_paletted;

int
nearest_power_of_2(int size)
{
	int		i = 2;

	/* NeVo - infinite loop bug-fix */
	if (size == 1)
		return size;
			
	while (1) {
		i <<= 1;
		if (size == i)
			return i;
		if (size > i && size < (i << 1)) {
			if (size >= ((i + (i << 1)) / 2))
				return i << 1;
			else
				return i;
		}
	};
}

extern cvar_t  *gl_lightmap_texture_saturation;	/* jitsaturation */
void
desaturate_texture(unsigned *udata, int width, int height)
{				/* jitsaturation */
	int		i, size;
	float		r, g, b, v, s;
	unsigned char  *data;

	data = (unsigned char *)udata;

	s = gl_lightmap_texture_saturation->value;

	size = width * height * 4;

	for (i = 0; i < size; i += 4) {
		r = data[i];
		g = data[i + 1];
		b = data[i + 2];
		v = r * 0.30 + g * 0.59 + b * 0.11;

		data[i] = (1 - s) * v + s * r;
		data[i + 1] = (1 - s) * v + s * g;
		data[i + 2] = (1 - s) * v + s * b;
	}
}

/*
 * =============== GL_Upload32
 *
 * Returns has_alpha ===============
 */
qboolean
GL_Upload32(unsigned *data, int width, int height, qboolean mipmap)
{
	int		samples;
	unsigned       *scaled;
	int		scaled_width = 0, scaled_height = 0;
	int		i, c;
	byte           *scan;
	int		comp = 0;

	uploaded_paletted = false;

	/* don't ever bother with maxtexsize textures */
	if (qglColorTableEXT && gl_ext_palettedtexture->value) {	/* palettedtexture is limited to 256 */
		clamp(scaled_width, 1, 256);
		clamp(scaled_height, 1, 256);
	} else {
		clamp(scaled_width, 1, gl_state.maxtexsize);
		clamp(scaled_height, 1, gl_state.maxtexsize);
	}

	/* scan the texture for any non-255 alpha */
	c = width * height;
	scan = ((byte *) data) + 3;
	samples = gl_solid_format;
	for (i = 0; i < c; i++, scan += 4) {
		if (*scan != 255) {
			samples = gl_alpha_format;
			break;
		}
	}

	/* Heffo - ARB Texture Compression */
	qglHint(GL_TEXTURE_COMPRESSION_HINT_ARB, GL_NICEST);
	if (samples == gl_solid_format)
		comp = (gl_state.texture_compression) ? GL_COMPRESSED_RGB_ARB : gl_tex_solid_format;
	else if (samples == gl_alpha_format)
		comp = (gl_state.texture_compression) ? GL_COMPRESSED_RGBA_ARB : gl_tex_alpha_format;


	/* find sizes to scale to */
	{
		int		max_size;

		qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
		scaled_width = nearest_power_of_2(width);
		scaled_height = nearest_power_of_2(height);

		if (scaled_width > max_size)
			scaled_width = max_size;
		if (scaled_height > max_size)
			scaled_height = max_size;

		if (scaled_width < 2)
			scaled_width = 2;
		if (scaled_height < 2)
			scaled_height = 2;
	}

	/* let people sample down the world textures for speed */
	if (mipmap && (int)gl_picmip->value > 0) {
		int		maxsize;

		if ((int)gl_picmip->value == 1)		/* clamp to 512x512 */
			maxsize = 512;
		else if ((int)gl_picmip->value == 2)	/* clamp to 256x256 */
			maxsize = 256;
		else					/* clamp to 128x128 */
			maxsize = 128;

		while (1) {
			if (scaled_width <= maxsize && scaled_height <= maxsize)
				break;
			scaled_width >>= 1;
			scaled_height >>= 1;
		}
	}
	if (scaled_width != width || scaled_height != height) {
		scaled = malloc((scaled_width * scaled_height) * 4);
		GL_ResampleTexture(data, width, height, scaled, scaled_width, scaled_height);
	} else {
		scaled_width = width;
		scaled_height = height;
		scaled = data;
	}

	if (gl_lightmap_texture_saturation->value < 1)	/* jitsaturation */
		desaturate_texture(scaled, scaled_width, scaled_height);

	if (mipmap) {
		if (!gl_state.hwgamma)
			if (brightenTexture)
				GL_LightScaleTexture(scaled, scaled_width, scaled_height, !mipmap);

		if (gl_state.sgis_mipmap) {
			qglTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, true);
			qglTexImage2D(GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, 
			              GL_RGBA, GL_UNSIGNED_BYTE, scaled);
		} else
			gluBuild2DMipmaps(GL_TEXTURE_2D, samples, scaled_width, scaled_height, 
			                  GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	} else {
		if (!gl_state.hwgamma)
			if (brightenTexture)
				GL_LightScaleTexture(scaled, scaled_width, scaled_height, !mipmap);
		qglTexImage2D(GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, 
		              GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	}

	if (scaled_width != width || scaled_height != height)
		free(scaled);


	upload_width = scaled_width;
	upload_height = scaled_height;

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap) ? gl_filter_min : gl_filter_max);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);


	if (mipmap) {

		/* Set anisotropic filter if supported and enabled Knightmare */
		if (gl_config.anisotropic && gl_anisotropic->value)
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_anisotropic->value);
		else
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	}
	return (samples == gl_alpha_format);
}


/*
 * =============== GL_Upload8
 *
 * Returns has_alpha ===============
 */

qboolean
GL_Upload8(byte * data, int width, int height, qboolean mipmap, qboolean is_sky)
{
	unsigned	trans[512 * 256];
	int		i, s;
	int		p;

	s = width * height;

	if (s > sizeof(trans) / 4)
		ri.Sys_Error(ERR_DROP, "GL_Upload8: too large");

	/*
	if ( is_sky ) { 
	  	qglTexImage2D( GL_TEXTURE_2D, 0,
	                       GL_COLOR_INDEX8_EXT, width, height, 0, GL_COLOR_INDEX,
	                       GL_UNSIGNED_BYTE, data );
	  	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max); 
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max); 
	} else
	*/
	{
		for (i = 0; i < s; i++) {
			p = data[i];
			trans[i] = d_8to24table[p];

			if (p == 255) {	/* transparent, so scan around for another color */
				/* to avoid alpha fringes */
				/*
				 * FIXME: do a full flood fill so mips
				 * work...
				 */
				if (i > width && data[i - width] != 255)
					p = data[i - width];
				else if (i < s - width && data[i + width] != 255)
					p = data[i + width];
				else if (i > 0 && data[i - 1] != 255)
					p = data[i - 1];
				else if (i < s - 1 && data[i + 1] != 255)
					p = data[i + 1];
				else
					p = 0;
				/* copy rgb components */
				((byte *) & trans[i])[0] = ((byte *) & d_8to24table[p])[0];
				((byte *) & trans[i])[1] = ((byte *) & d_8to24table[p])[1];
				((byte *) & trans[i])[2] = ((byte *) & d_8to24table[p])[2];
			}
		}

		return GL_Upload32(trans, width, height, mipmap);
	}
}

/*
 * ================ GL_LoadPic
 *
 * This is also used as an entry point for the generated r_notexture
 * ================
 */
image_t        *
GL_LoadPic(char *name, byte * pic, int width, int height, imagetype_t type, int bits)
{
	image_t        *image;
	int		i;
#ifdef RETEX
	/* NiceAss: Nexus'added vars for texture scaling */
	miptex_t       *mt;
	int		len;
	char		s[128];
#endif

	/* find a free image_t */
	for (i = 0, image = gltextures; i < numgltextures; i++, image++) {
		if (!image->texnum)
			break;
	}
	if (i == numgltextures) {
		if (numgltextures == MAX_GLTEXTURES)
			ri.Sys_Error(ERR_DROP, "MAX_GLTEXTURES");
		numgltextures++;
	}
	image = &gltextures[i];

	if (strlen(name) >= sizeof(image->name))
		ri.Sys_Error(ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);
	Q_strncpyz(image->name, name, sizeof(image->name));
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;

	image->replace_scale = 1.0;

#ifdef RETEX
	/* NiceAss: Nexus's image replacement scaling code  */
	len = strlen(name);
	Q_strncpyz(s, name, sizeof(s));

	if (!strcmp(s + len - 4, ".tga") || !strcmp(s + len - 4, ".png") || !strcmp(s + len - 4, ".jpg")) {
		s[len - 3] = 'w';
		s[len - 2] = 'a';
		s[len - 1] = 'l';				/* replace extension  */
		ri.FS_LoadFile(s, (void **)&mt);		/* load .wal file  */

		if (mt) {
			image->width = LittleLong(mt->width);	/* grab size from wal  */
			image->height = LittleLong(mt->height);
			ri.FS_FreeFile((void *)mt);		/* free the picture  */
		} else {					/* if it's replacing a PCX image */
			byte           *pic, *palette;
			int		pcxwidth  , pcxheight;

			s[len - 3] = 'p';
			s[len - 2] = 'c';
			s[len - 1] = 'x';					/* replace extension */
			LoadPCX(s, &pic, &palette, &pcxwidth, &pcxheight);	/* load .pcx file */

			if (pcxwidth > 0 && pcxheight > 0) {
				image->replace_scale = ((float)pcxwidth / image->width + (float)pcxheight / image->height) * 0.5;
				if (image->replace_scale == 0.0)
					image->replace_scale = 1.0;
				image->replace_scale = MIN(image->replace_scale, 1.0);
			}
			if (pic)
				free(pic);
			if (palette)
				free(palette);
		}
	}
#endif

	if (type == it_skin && bits == 8)
		R_FloodFillSkin(pic, width, height);

	/* load little pics into the scrap */
	if (image->type == it_pic && bits == 8 && image->width < 64 && image->height < 64) {
		int		x         , y;
		int		i         , j, k;
		int		texnum;

		texnum = Scrap_AllocBlock(image->width, image->height, &x, &y);
		if (texnum == -1)
			goto nonscrap;
		scrap_dirty = true;

		/* copy the texels into the scrap block */
		k = 0;
		for (i = 0; i < image->height; i++)
			for (j = 0; j < image->width; j++, k++)
				scrap_texels[texnum][(y + i) * BLOCK_WIDTH + x + j] = pic[k];
		image->texnum = TEXNUM_SCRAPS + texnum;
		image->scrap = true;
		image->has_alpha = true;
		image->sl = (x + 0.01) / (float)BLOCK_WIDTH;
		image->sh = (x + image->width - 0.01) / (float)BLOCK_WIDTH;
		image->tl = (y + 0.01) / (float)BLOCK_WIDTH;
		image->th = (y + image->height - 0.01) / (float)BLOCK_WIDTH;
	} else {
nonscrap:
		image->scrap = false;
		image->texnum = TEXNUM_IMAGES + (image - gltextures);
		GL_Bind(image->texnum);
		if (bits == 8)
			image->has_alpha =
			    GL_Upload8(pic, width, height, (image->type != it_pic && image->type != it_sky),
			    image->type == it_sky);
		else
			image->has_alpha =
			    GL_Upload32((unsigned *)pic, width, height,
			    (image->type != it_pic && image->type != it_sky));
		image->upload_width = upload_width;	/* after power of 2 and scales */
		image->upload_height = upload_height;
		image->paletted = uploaded_paletted;
		image->sl = 0;
		image->sh = 1;
		image->tl = 0;
		image->th = 1;
	}

	return image;
}

/*
 * ================ GL_LoadWal ================
 */
image_t        *
GL_LoadWal(char *name)
{
	miptex_t       *mt;
	int		width, height, ofs;
	image_t        *image;

	ri.FS_LoadFile(name, (void **)&mt);
	if (!mt) {
		ri.Con_Printf(PRINT_ALL, "GL_FindImage: can't load %s\n", name);
		return r_notexture;
	}
	width = LittleLong(mt->width);
	height = LittleLong(mt->height);
	ofs = LittleLong(mt->offsets[0]);


	image = GL_LoadPic(name, (byte *) mt + ofs, width, height, it_wall, 8);

	ri.FS_FreeFile((void *)mt);

	return image;
}

/*
 * =============== GL_FindImage
 *
 * Finds or loads the given image ===============
 */
#define IMAGETYPES 3
char           *image_types[IMAGETYPES] = {
	".png",
	".tga",
	".jpg"
};

image_t        *
GL_FindImage(char *name, imagetype_t type)
{
	image_t        *image;
	int		i, len;
	byte           *pic, *palette;
	int		width, height;
	char           *ptr;

	if (!name)
		return NULL;	
		/* ri.Sys_Error (ERR_DROP, "GL_FindImage: NULL name"); */
	len = strlen(name);
	if (len < 5)
		return NULL;	
		/* ri.Sys_Error (ERR_DROP, "GL_FindImage: bad name: %s", name); */
		
	/* fix backslashes */
	while ((ptr=strchr(name,'\\'))) {
	        *ptr = '/';
	}
		 
	/* look for it */
	for (i = 0, image = gltextures; i < numgltextures; i++, image++) {
		if (!strcmp(name, image->name)) {
			image->registration_sequence = registration_sequence;
			return image;
		}
	}

	/* load the pic from disk */
	pic = NULL;
	palette = NULL;
	image = NULL;
	if (!strcmp(name + len - 4, ".pcx")) {
		char		basename[MAX_QPATH];

		Com_sprintf(basename, sizeof(basename), "%s", name);
		basename[strlen(basename) - 4] = 0;

		/*
		 * only try to overwrite pcxs, others are specified in model
		 * files etc
		 */
		for (i = 0; i < IMAGETYPES && !image; i++)
			image = GL_FindImage(va("%s%s", basename, image_types[i]), type);

		if (!image) {
			LoadPCX(name, &pic, &palette, &width, &height);
			if (!pic)
				return NULL;	/* ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name); */
			image = GL_LoadPic(name, pic, width, height, type, 8);
		}
	} else if (!strcmp(name + len - 4, ".wal")) {
		char		basename[MAX_QPATH];

		Com_sprintf(basename, sizeof(basename), "%s", name);
		basename[strlen(basename) - 4] = 0;

		/* only try to overwrite pcxs, others are specified in model files etc */
		for (i = 0; i < IMAGETYPES && !image; i++)
			image = GL_FindImage(va("%s%s", basename, image_types[i]), type);

		if (!image) {
			image = GL_LoadWal(name);
		}
	} else {
		if (!strcmp(name + len - 4, ".png")) {	/* heffo hax0r */
			LoadPNG(name, &pic, &width, &height);
			if (!pic)
				return NULL;	
				/* ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name); */
			image = GL_LoadPic(name, pic, width, height, type, 32);
		} else if (!strcmp(name + len - 4, ".tga")) {
			LoadTGA(name, &pic, &width, &height);
			if (!pic)
				return NULL;	
				/* ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name); */
			image = GL_LoadPic(name, pic, width, height, type, 32);
		} else if (!strcmp(name + len - 4, ".jpg")) {
			LoadJPG(name, &pic, &width, &height);
			if (!pic)
				return NULL;	
				/* ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name); */
			image = GL_LoadPic(name, pic, width, height, type, 32);
		} else
			return NULL;	
			/* ri.Sys_Error (ERR_DROP, "GL_FindImage: bad extension on: %s", name); */
	}

	if (pic)
		free(pic);
	if (palette)
		free(palette);
	return image;
}

/*
 * =============== R_RegisterSkin ===============
 */
struct image_s *
R_RegisterSkin(char *name)
{
	return GL_FindImage(name, it_skin);
}

/*
 * ================ GL_FreeUnusedImages
 *
 * Any image that was not touched on this registration sequence will be freed.
 * ================
 */
void
GL_FreeUnusedImages(void)
{
	int		i;
	image_t        *image;

	/* never free r_notexture or particle texture[s] */
	r_notexture->registration_sequence = registration_sequence;
#ifdef QMAX
	r_particlebeam->registration_sequence = registration_sequence;
#else
	r_particletexture->registration_sequence = registration_sequence;
#endif
	/* MH - detail textures begin */
	/* never free detail texture */
	r_detailtexture->registration_sequence = registration_sequence;
	/* MH - detail textures end */
	r_shelltexture->registration_sequence = registration_sequence;	/* c14 added shell texture */
	r_radarmap->registration_sequence = registration_sequence;
	r_around->registration_sequence = registration_sequence;
	r_lblendimage->registration_sequence = registration_sequence;
	r_caustictexture->registration_sequence = registration_sequence;
	r_bholetexture->registration_sequence = registration_sequence;
#ifdef QMAX
	for (i = 0; i < PARTICLE_TYPES; i++)
		if (r_particletextures[i])	/* dont mess with null ones silly :p */
			r_particletextures[i]->registration_sequence = registration_sequence;
#endif

	for (i = 0, image = gltextures; i < numgltextures; i++, image++) {
		if (image->registration_sequence == registration_sequence)
			continue;	/* used this sequence */
		if (!image->registration_sequence)
			continue;	/* free image_t slot */
		if (image->type == it_pic)
			continue;	/* don't free pics */
		/* free it */
		qglDeleteTextures(1, (GLuint *)&image->texnum);
		memset(image, 0, sizeof(*image));
	}
}


/*
 * =============== Draw_GetPalette ===============
 */
#ifndef QMAX
byte		default_pal[768] = {

	0, 0, 0, 15, 15, 15, 31, 31, 31, 47, 47, 47,
	63, 63, 63, 75, 75, 75, 91, 91, 91, 107, 107, 107,
	123, 123, 123, 139, 139, 139, 155, 155, 155, 171, 171, 171,
	187, 187, 187, 203, 203, 203, 219, 219, 219, 235, 235, 235,
	99, 75, 35, 91, 67, 31, 83, 63, 31, 79, 59, 27,
	71, 55, 27, 63, 47, 23, 59, 43, 23, 51, 39, 19,
	47, 35, 19, 43, 31, 19, 39, 27, 15, 35, 23, 15,
	27, 19, 11, 23, 15, 11, 19, 15, 7, 15, 11, 7,
	95, 95, 111, 91, 91, 103, 91, 83, 95, 87, 79, 91,
	83, 75, 83, 79, 71, 75, 71, 63, 67, 63, 59, 59,
	59, 55, 55, 51, 47, 47, 47, 43, 43, 39, 39, 39,
	35, 35, 35, 27, 27, 27, 23, 23, 23, 19, 19, 19,
	143, 119, 83, 123, 99, 67, 115, 91, 59, 103, 79, 47,
	207, 151, 75, 167, 123, 59, 139, 103, 47, 111, 83, 39,
	235, 159, 39, 203, 139, 35, 175, 119, 31, 147, 99, 27,
	119, 79, 23, 91, 59, 15, 63, 39, 11, 35, 23, 7,
	167, 59, 43, 159, 47, 35, 151, 43, 27, 139, 39, 19,
	127, 31, 15, 115, 23, 11, 103, 23, 7, 87, 19, 0,
	75, 15, 0, 67, 15, 0, 59, 15, 0, 51, 11, 0,
	43, 11, 0, 35, 11, 0, 27, 7, 0, 19, 7, 0,
	123, 95, 75, 115, 87, 67, 107, 83, 63, 103, 79, 59,
	95, 71, 55, 87, 67, 51, 83, 63, 47, 75, 55, 43,
	67, 51, 39, 63, 47, 35, 55, 39, 27, 47, 35, 23,
	39, 27, 19, 31, 23, 15, 23, 15, 11, 15, 11, 7,
	111, 59, 23, 95, 55, 23, 83, 47, 23, 67, 43, 23,
	55, 35, 19, 39, 27, 15, 27, 19, 11, 15, 11, 7,
	179, 91, 79, 191, 123, 111, 203, 155, 147, 215, 187, 183,
	203, 215, 223, 179, 199, 211, 159, 183, 195, 135, 167, 183,
	115, 151, 167, 91, 135, 155, 71, 119, 139, 47, 103, 127,
	23, 83, 111, 19, 75, 103, 15, 67, 91, 11, 63, 83,
	7, 55, 75, 7, 47, 63, 7, 39, 51, 0, 31, 43,
	0, 23, 31, 0, 15, 19, 0, 7, 11, 0, 0, 0,
	139, 87, 87, 131, 79, 79, 123, 71, 71, 115, 67, 67,
	107, 59, 59, 99, 51, 51, 91, 47, 47, 87, 43, 43,
	75, 35, 35, 63, 31, 31, 51, 27, 27, 43, 19, 19,
	31, 15, 15, 19, 11, 11, 11, 7, 7, 0, 0, 0,
	151, 159, 123, 143, 151, 115, 135, 139, 107, 127, 131, 99,
	119, 123, 95, 115, 115, 87, 107, 107, 79, 99, 99, 71,
	91, 91, 67, 79, 79, 59, 67, 67, 51, 55, 55, 43,
	47, 47, 35, 35, 35, 27, 23, 23, 19, 15, 15, 11,
	159, 75, 63, 147, 67, 55, 139, 59, 47, 127, 55, 39,
	119, 47, 35, 107, 43, 27, 99, 35, 23, 87, 31, 19,
	79, 27, 15, 67, 23, 11, 55, 19, 11, 43, 15, 7,
	31, 11, 7, 23, 7, 0, 11, 0, 0, 0, 0, 0,
	119, 123, 207, 111, 115, 195, 103, 107, 183, 99, 99, 167,
	91, 91, 155, 83, 87, 143, 75, 79, 127, 71, 71, 115,
	63, 63, 103, 55, 55, 87, 47, 47, 75, 39, 39, 63,
	35, 31, 47, 27, 23, 35, 19, 15, 23, 11, 7, 7,
	155, 171, 123, 143, 159, 111, 135, 151, 99, 123, 139, 87,
	115, 131, 75, 103, 119, 67, 95, 111, 59, 87, 103, 51,
	75, 91, 39, 63, 79, 27, 55, 67, 19, 47, 59, 11,
	35, 47, 7, 27, 35, 0, 19, 23, 0, 11, 15, 0,
	0, 255, 0, 35, 231, 15, 63, 211, 27, 83, 187, 39,
	95, 167, 47, 95, 143, 51, 95, 123, 51, 255, 255, 255,
	255, 255, 211, 255, 255, 167, 255, 255, 127, 255, 255, 83,
	255, 255, 39, 255, 235, 31, 255, 215, 23, 255, 191, 15,
	255, 171, 7, 255, 147, 0, 239, 127, 0, 227, 107, 0,
	211, 87, 0, 199, 71, 0, 183, 59, 0, 171, 43, 0,
	155, 31, 0, 143, 23, 0, 127, 15, 0, 115, 7, 0,
	95, 0, 0, 71, 0, 0, 47, 0, 0, 27, 0, 0,
	239, 0, 0, 55, 55, 255, 255, 0, 0, 0, 0, 255,
	43, 43, 35, 27, 27, 23, 19, 19, 15, 235, 151, 127,
	195, 115, 83, 159, 87, 51, 123, 63, 27, 235, 211, 199,
	199, 171, 155, 167, 139, 119, 135, 107, 87, 159, 91, 83
};
#endif

int
Draw_GetPalette(void)
{
	int		i;
	int		r, g, b;
	unsigned	v;
	byte           *pic, *pal;
	int		width, height;
#ifndef QMAX
	byte		default_pal[768];
#endif

	/* get the palette */

	LoadPCX("pics/colormap.pcx", &pic, &pal, &width, &height);
	if (!pal) {
		ri.Sys_Error(ERR_FATAL, "Couldn't load pics/colormap.pcx");
	} else
		for (i = 0; i < 256; i++) {
			r = pal[i * 3 + 0];
			g = pal[i * 3 + 1];
			b = pal[i * 3 + 2];

			v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
			d_8to24table[i] = LittleLong(v);
#ifndef QMAX
			d_8to24tablef[i][0] = r * 0.003921568627450980392156862745098;
			d_8to24tablef[i][1] = g * 0.003921568627450980392156862745098;
			d_8to24tablef[i][2] = b * 0.003921568627450980392156862745098;
#endif
		}

	d_8to24table[255] &= LittleLong(0xffffff);	/* 255 is transparent */

#ifdef QMAX
	free(pic);
	free(pal);
#else
	if (pic)
		free(pic);
	if (pal != default_pal)
		free(pal);
#endif

	return 0;
}


/*
 * =============== GL_InitImages ===============
 */
void
GL_InitImages(void)
{
	int		i, j;
	float		g = vid_gamma->value;

	registration_sequence = 1;

	/* init intensity conversions */
	/* Vic - begin */
	if (gl_config.mtexcombine)
		intensity = ri.Cvar_Get("intensity", "1.5", CVAR_ARCHIVE);
	else
		intensity = ri.Cvar_Get("intensity", "2", CVAR_ARCHIVE);
	/* Vic - end */

	if (intensity->value <= 1)
		ri.Cvar_Set("intensity", "1");

	gl_state.inverse_intensity = 1 / intensity->value;

	Draw_GetPalette();

	if (qglColorTableEXT) {
		ri.FS_LoadFile("pics/16to8.dat",
		    (void **)&gl_state.d_16to8table);
		if (!gl_state.d_16to8table)
			ri.Sys_Error(ERR_FATAL, "Couldn't load pics/16to8.pcx");
	}
	if (gl_config.renderer & (GL_RENDERER_VOODOO | GL_RENDERER_VOODOO2)) {
		g = 1.0;
	}
	for (i = 0; i < 256; i++) {
		if (g == 1 || gl_state.hwgamma) {
			gammatable[i] = i;
		} else {
			float		inf;

			inf = 255 * pow((i + 0.5) * DIV255, g) + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}

	for (i = 0; i < 256; i++) {
		j = i * intensity->value;
		if (j > 255)
			j = 255;
		intensitytable[i] = j;
	}

	R_InitBloomTextures();		/* BLOOMS */
}

/*
 * =============== GL_ShutdownImages ===============
 */
void
GL_ShutdownImages(void)
{
	int		i;
	image_t        *image;

	for (i = 0, image = gltextures; i < numgltextures; i++, image++) {
		if (!image->registration_sequence)
			continue;	/* free image_t slot */
		/* free it */
		qglDeleteTextures(1, (GLuint *)&image->texnum);
		memset(image, 0, sizeof(*image));
	}
}
