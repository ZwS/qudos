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
/* r_misc.c */

#include "gl_local.h"

image_t        *r_detailtexture;

/* MH - detail textures begin */
/* blatantly plagiarized darkplaces code */
/* no bound in Q2. */

/*
 * this would be better as a #define but it's not runtime so it doesn't
 * matter
 */
/* (dukey) fixed some small issues */
/* it now multitextures the texture if your card has enough texture units (3) */

/*
 * otherwise it will revert to the old school method of render everything
 * again
 */

/*
 * and blending it with the original texture + lightmap which is slow as fuck
 * frankly
 */
/* but the option is there for slow cards to go even slower ;) enjoy */
/* my radeon 9600se has 8 texture units :) */

int
bound(int smallest, int val, int biggest)
{
	if (val < smallest)
		return smallest;
	if (val > biggest)
		return biggest;

	return val;
}


void
fractalnoise(byte * noise, int size, int startgrid)
{
	int		x, y, g, g2, amplitude, min, max, size1 = size - 1,
			sizepower, gridpower;
	int            *noisebuf;

#define n(x,y) noisebuf[((y)&size1)*size+((x)&size1)]

	for (sizepower = 0; (1 << sizepower) < size; sizepower++);
	if (size != (1 << sizepower))
		Sys_Error("fractalnoise: size must be power of 2\n");

	for (gridpower = 0; (1 << gridpower) < startgrid; gridpower++);
	if (startgrid != (1 << gridpower))
		Sys_Error("fractalnoise: grid must be power of 2\n");

	startgrid = bound(0, startgrid, size);

	amplitude = 0xFFFF;	/* this gets halved before use */
	noisebuf = malloc(size * size * sizeof(int));
	memset(noisebuf, 0, size * size * sizeof(int));

	for (g2 = startgrid; g2; g2 >>= 1) {

		/*
		 * brownian motion (at every smaller level there is random
		 * behavior)
		 */
		amplitude >>= 1;
		for (y = 0; y < size; y += g2)
			for (x = 0; x < size; x += g2)
				n(x, y) += (rand() & amplitude);

		g = g2 >> 1;
		if (g) {

			/*
			 * subdivide, diamond-square algorithm (really this
			 * has little to do with squares)
			 */
			/* diamond */
			for (y = 0; y < size; y += g2)
				for (x = 0; x < size; x += g2)
					n(x + g, y + g) = (n(x, y) + n(x + g2, y) + n(x, y + g2) + n(x + g2, y + g2)) >> 2;
			/* square */
			for (y = 0; y < size; y += g2)
				for (x = 0; x < size; x += g2) {
					n(x + g, y) = (n(x, y) + n(x + g2, y) + n(x + g, y - g) + n(x + g, y + g)) >> 2;
					n(x, y + g) = (n(x, y) + n(x, y + g2) + n(x - g, y + g) + n(x + g, y + g)) >> 2;
				}
		}
	}
	/* find range of noise values */
	min = max = 0;
	for (y = 0; y < size; y++)
		for (x = 0; x < size; x++) {
			if (n(x, y) < min)
				min = n(x, y);
			if (n(x, y) > max)
				max = n(x, y);
		}
	max -= min;
	max++;
	/* normalize noise and copy to output */
	for (y = 0; y < size; y++)
		for (x = 0; x < size; x++)
			*noise++ = (byte) (((n(x, y) - min) * 256) / max);
	free(noisebuf);

#undef n
}


void
R_BuildDetailTexture(void)
{
	int		x, y, light;
	float		vc[3], vx[3], vy[3], vn[3], lightdir[3];

	/* increase this if you want */
#define DETAILRESOLUTION 256

	byte		data      [DETAILRESOLUTION][DETAILRESOLUTION][4], noise[DETAILRESOLUTION][DETAILRESOLUTION];

	/*
	 * this looks odd, but it's necessary cos Q2's uploader will
	 * lightscale the texture, which
	 */

	/*
	 * will cause all manner of unholy havoc.  So we need to run it
	 * through GL_LoadPic before
	 */

	/*
	 * we even fill in the data just to fill in the rest of the image_t
	 * struct, then we'll
	 */
	/* build the texture for OpenGL manually later on. */
	r_detailtexture = GL_LoadPic("***detail***", (byte *) data, DETAILRESOLUTION, DETAILRESOLUTION, it_wall, 32);

	lightdir[0] = 0.5;
	lightdir[1] = 1;
	lightdir[2] = -0.25;
	VectorNormalize(lightdir);

	fractalnoise(&noise[0][0], DETAILRESOLUTION, DETAILRESOLUTION >> 4);
	for (y = 0; y < DETAILRESOLUTION; y++) {
		for (x = 0; x < DETAILRESOLUTION; x++) {
			vc[0] = x;
			vc[1] = y;
			vc[2] = noise[y][x] * (1.0 / 32.0);
			vx[0] = x + 1;
			vx[1] = y;
			vx[2] = noise[y][(x + 1) % DETAILRESOLUTION] * (1.0 / 32.0);
			vy[0] = x;
			vy[1] = y + 1;
			vy[2] = noise[(y + 1) % DETAILRESOLUTION][x] * (1.0 / 32.0);
			VectorSubtract(vx, vc, vx);
			VectorSubtract(vy, vc, vy);
			CrossProduct(vx, vy, vn);
			VectorNormalize(vn);
			light = 128 - DotProduct(vn, lightdir) * 128;
			light = bound(0, light, 255);
			data[y][x][0] = data[y][x][1] = data[y][x][2] = light;
			data[y][x][3] = 255;
		}
	}

	/*
	 * now we build the texture manually.  you can reuse this code for
	 * auto mipmap generation
	 */
	/* in other contexts if you wish.  defines are in qgl.h */

	/*
	 * first, bind the texture.  probably not necessary, but it seems
	 * like good practice
	 */
	GL_Bind(r_detailtexture->texnum);

	/*
	 * upload the correct texture data without any lightscaling
	 * interference
	 */
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, DETAILRESOLUTION, DETAILRESOLUTION, GL_RGBA, GL_UNSIGNED_BYTE, (byte *) data);

	/*
	 * set some quick and ugly filtering modes.  detail texturing works
	 * by scrunching a
	 */

	/*
	 * large image into a small space, so there's no need for sexy
	 * filtering (change this,
	 */

	/*
	 * turn off the blend in R_DrawDetailSurfaces, and compare if you
	 * don't believe me).
	 */

	/*
	 * this also helps to take some of the speed hit from detail
	 * texturing away.
	 */
	/* fucks up for some reason so using different filtering. */
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

}

/* MH - detail textures end */


/*
 * ================== R_InitParticleTexture ==================
 */
#ifndef QMAX
byte		dottexture[8][8] =
{
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 1, 0, 0, 0, 0},
	{0, 1, 1, 1, 1, 0, 0, 0},
	{0, 1, 1, 1, 1, 0, 0, 0},
	{0, 0, 1, 1, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
};
#endif

#ifdef QMAX
byte		notexture [16][16] =
#else
byte		data1     [16][16] =
#endif
{
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

#ifdef QMAX
void
SetParticlePicture(int num, char *name)
{
	r_particletextures[num] = GL_FindImage(name, it_part);
	if (!r_particletextures[num])
		r_particletextures[num] = r_notexture;
}
#endif

image_t        *
LoadPartImg(char *name)
{
	image_t        *image = GL_FindImage(name, it_part);

	if (!image)
		image = r_notexture;
	return image;
}

void
R_InitParticleTexture(void)
{
	int		x, y;

#ifdef QMAX
	byte		notex[16][16][4];
#else
	byte		data[8][8][4];
	byte		data1[16][16][4];
	int		dx2, dy, d;
#endif
	byte		env_data[256 * 256 * 4];

	/* use notexture array for bad textures */
#ifdef QMAX
	for (x = 0; x < 16; x++) {	/* was 8 */
		for (y = 0; y < 16; y++) {	/* was 8 */
			notex[y][x][0] = notexture[x][y] * 255;	/* red checkers! */
			notex[y][x][1] = 0;
			notex[y][x][2] = 0;
			notex[y][x][3] = 255;
		}
	}
#else
	for (x = 0; x < 16; x++) {
		dx2 = x - 8;
		dx2 *= dx2;
		for (y = 0; y < 16; y++) {
			dy = y - 8;
			d = 255 - 4 * (dx2 + (dy * dy));
			if (d <= 0) {
				d = 0;
				data1[y][x][0] = 0;
				data1[y][x][1] = 0;
				data1[y][x][2] = 0;
			} else {
				data1[y][x][0] = 255;
				data1[y][x][1] = 255;
				data1[y][x][2] = 255;
			}

			data1[y][x][3] = (byte) d;
		}
	}

	r_particletexture = GL_LoadPic("***particle***", (byte *) data1, 16, 16, 32, 0);

	/* also use this for bad textures, but without alpha */
	for (x = 0; x < 8; x++) {
		for (y = 0; y < 8; y++) {
			data[y][x][0] = dottexture[x & 3][y & 3] * 255;
			data[y][x][1] = 0;	/* dottexture[x&3][y&3]*255; */
			data[y][x][2] = 0;	/* dottexture[x&3][y&3]*255; */
			data[y][x][3] = 255;
		}
	}
#endif

#ifdef QMAX
	r_notexture = GL_LoadPic("***r_notexture***", (byte *) notex, 16, 16, it_wall, 32);
#else
	r_notexture = GL_LoadPic("***r_notexture***", (byte *) data, 8, 8, it_wall, 32);
#endif
	r_lblendimage = GL_LoadPic("***r_lblendimage***", (byte *) env_data, 256, 256, it_wall, 32);

#ifdef QMAX
	r_particlebeam = LoadPartImg("particles/beam.png");
	if (!r_particlebeam) {
		r_particlebeam = r_notexture;
	}
#endif

	/* c14 */
	r_shelltexture = LoadPartImg("gfx/shell.tga");
	if (!r_shelltexture) {
		r_shelltexture = r_notexture;
	}
	r_radarmap = LoadPartImg("gfx/radarmap.png");
	if (!r_radarmap) {
		r_radarmap = r_notexture;
	}
	r_around = LoadPartImg("gfx/around.png");
	if (!r_around) {
		r_around = r_notexture;
	}
	r_caustictexture = LoadPartImg("gfx/caustic.png");
	if (!r_caustictexture) {
		r_caustictexture = r_notexture;
	}
	r_bholetexture = LoadPartImg("gfx/bullethole.png");
	if (!r_bholetexture) {
		r_bholetexture = r_notexture;
	}
	/* MH - detail textures begin */
	/* builds and uploads detailed texture */
	R_BuildDetailTexture();
	/* MH - detail textures end */
#ifdef QMAX
	/* Knightmare- Psychospaz's enhanced particles */
	for (x = 0; x < PARTICLE_TYPES; x++)
		r_particletextures[x] = NULL;

	ri.SetParticlePics();
#endif
}



/*
 *
 * ============================================================================
 * ==
 *
 * SCREEN SHOTS
 *
 * ============
 * =================================================================
 */

typedef struct _TargaHeader {
	unsigned char	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
 * ==================
 * GL_ScreenShot_JPG By Robert 'Heffo' Heffernan
 * ==================
 */
void
GL_ScreenShot_JPG(void)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	byte           *rgbdata;
	JSAMPROW	s[1];
	FILE           *file;
	char		picname[80], checkname[MAX_OSPATH];
	int		i, offset;

	/* Create the scrnshots directory if it doesn't exist */
	Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot", ri.FS_Gamedir());
	Sys_Mkdir(checkname);

	/* Knightmare- changed screenshot filenames, up to 100 screenies */
	/* Find a file name to save it to  */
	/* strcpy(picname,"quake00.jpg"); */

	for (i = 0; i <= 999; i++) {
		/* picname[5] = i/10 + '0';  */
		/* picname[6] = i%10 + '0';  */
		int		one       , ten, hundred;

		hundred = i * 0.01;
		ten = (i - hundred * 100) * 0.1;
		one = i - hundred * 100 - ten * 10;

		Com_sprintf(picname, sizeof(picname), "QuDos_%i%i%i.jpg", hundred, ten, one);
		Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot/%s", ri.FS_Gamedir(), picname);
		file = fopen(checkname, "rb");
		if (!file)
			break;	/* file doesn't exist */
		fclose(file);
	}
	if (i == 1000) {
		ri.Con_Printf(PRINT_ALL, "SCR_JPGScreenShot_f: Couldn't create a file\n");
		return;
	}
	/* Open the file for Binary Output */
	file = fopen(checkname, "wb");
	if (!file) {
		ri.Con_Printf(PRINT_ALL, "SCR_JPGScreenShot_f: Couldn't create a file\n");
		return;
	}
	/* Allocate room for a copy of the framebuffer */
	rgbdata = malloc(vid.width * vid.height * 3);
	if (!rgbdata) {
		fclose(file);
		return;
	}
	/* Read the framebuffer into our storage */
	qglReadPixels(0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, rgbdata);

	/* Initialise the JPEG compression object */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, file);

	/* Setup JPEG Parameters */
	cinfo.image_width = vid.width;
	cinfo.image_height = vid.height;
	cinfo.in_color_space = JCS_RGB;
	cinfo.input_components = 3;
	jpeg_set_defaults(&cinfo);
	if ((gl_screenshot_jpeg_quality->value >= 101) || (gl_screenshot_jpeg_quality->value <= 0))
		ri.Cvar_Set("gl_screenshot_jpeg_quality", "85");
	jpeg_set_quality(&cinfo, gl_screenshot_jpeg_quality->value, TRUE);

	/* Start Compression */
	jpeg_start_compress(&cinfo, true);

	/* Feed Scanline data */
	offset = (cinfo.image_width * cinfo.image_height * 3) - (cinfo.image_width * 3);
	while (cinfo.next_scanline < cinfo.image_height) {
		s[0] = &rgbdata[offset - (cinfo.next_scanline * (cinfo.image_width * 3))];
		jpeg_write_scanlines(&cinfo, s, 1);
	}

	/* Finish Compression */
	jpeg_finish_compress(&cinfo);

	/* Destroy JPEG object */
	jpeg_destroy_compress(&cinfo);

	/* Close File */
	fclose(file);

	/* Free Temp Framebuffer */
	free(rgbdata);

	/* Done! */
	ri.Con_Printf(PRINT_ALL, "Wrote %s\n", picname);
}


/*
 * ==================
 * GL_ScreenShot_PNG
 *
 * Thanks Echon/Incith.
 * ==================
 */
void
GL_ScreenShot_PNG(void)
{
	byte           *buffer;
	char		picname[MAX_OSPATH];
	char		checkname[MAX_OSPATH];
	int		i, k;
	FILE           *f;
	png_structp	png_ptr;
	png_infop	info_ptr;
	png_bytep      *row_pointers;

	/* Create the scrnshots directory if it doesn't exist */
	Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot", ri.FS_Gamedir());
	Sys_Mkdir(checkname);
	for (i = 0; i <= 999; i++) {
		Com_sprintf(picname, sizeof(picname), "QuDos_%i%i%i.png", (int)(i / 100) % 10, (int)(i / 10) % 10, i % 10);
		Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot/%s", ri.FS_Gamedir(), picname);
		f = fopen(checkname, "rb");
		if (!f)
			break;	/* file doesn't exist */
		fclose(f);
	}
	if (i == 1000) {
		ri.Con_Printf(PRINT_ALL, "GL_ScreenShot_PNG: Couldn't create a file\n");
		return;
	}
	/* Open the file for Binary Output */
	f = fopen(checkname, "wb");
	if (!f) {
		ri.Con_Printf(PRINT_ALL, "GL_ScreenShot_PPG: Couldn't create a file\n");
		return;
	}
	/* Allocate room for a copy of the framebuffer */
	buffer = malloc(vid.width * vid.height * 3);
	if (!buffer) {
		fclose(f);
		return;
	}
	/* Read the framebuffer into our storage */
	qglReadPixels(0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer);

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr) {
		ri.Con_Printf(0, "LibPNG Error! (%s)\n", picname);
		return;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		ri.Con_Printf(0, "LibPNG Error! (%s)\n", picname);
		return;
	}
	png_init_io(png_ptr, f);

	png_set_IHDR(png_ptr, info_ptr, vid.width, vid.height, 8, PNG_COLOR_TYPE_RGB,
	    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_set_compression_level(png_ptr, Z_DEFAULT_COMPRESSION);
	png_set_compression_mem_level(png_ptr, 9);

	png_write_info(png_ptr, info_ptr);

	row_pointers = malloc(vid.height * sizeof(png_bytep));
	for (k = 0; k < vid.height; k++)
		row_pointers[k] = buffer + (vid.height - 1 - k) * 3 * vid.width;

	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(f);
	free(buffer);

	/* Done! */
	ri.Con_Printf(0, "Wrote %s\n", picname);
}


/*
 * ================== GL_ScreenShot_f ==================
 */
void
GL_ScreenShot_f(void)
{
	byte           *buffer;
	char		picname[80];
	char		checkname[MAX_OSPATH];
	int		i, c, temp;
	FILE           *f;
	qboolean	png = false;

	/* Heffo - JPEG Screenshots */
	if (gl_screenshot_jpeg->value) {
		GL_ScreenShot_JPG();
		return;
	}
	if (!Q_stricmp(ri.Cmd_Argv(0), "pngshot"))
		png = true;

	/* create the scrnshots directory if it doesn't exist */
	Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot", ri.FS_Gamedir());
	Sys_Mkdir(checkname);

	/* find a file name to save it to  */
	Q_strncpyz(picname, "quake00.tga", sizeof(picname));

	for (i = 0; i <= 99; i++) {
		picname[5] = i / 10 + '0';
		picname[6] = i % 10 + '0';
		Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot/%s", ri.FS_Gamedir(), picname);
		f = fopen(checkname, "rb");
		if (!f)
			break;	/* file doesn't exist */
		fclose(f);
	}
	if (i == 100) {
		ri.Con_Printf(PRINT_ALL, "SCR_ScreenShot_f: Couldn't create a file\n");
		return;
	}
	buffer = malloc(vid.width * vid.height * 3 + 18);
	memset(buffer, 0, 18);
	buffer[2] = 2;		/* uncompressed type */
	buffer[12] = vid.width & 255;
	buffer[13] = vid.width >> 8;
	buffer[14] = vid.height & 255;
	buffer[15] = vid.height >> 8;
	buffer[16] = 24;	/* pixel size */

	qglReadPixels(0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer + 18);

	/* swap rgb to bgr */
	c = 18 + vid.width * vid.height * 3;
	for (i = 18; i < c; i += 3) {
		temp = buffer[i];
		buffer[i] = buffer[i + 2];
		buffer[i + 2] = temp;
	}

	f = fopen(checkname, "wb");
	fwrite(buffer, 1, c, f);
	fclose(f);

	free(buffer);
	ri.Con_Printf(PRINT_ALL, "Wrote %s\n", picname);
}

/*
 * GL_Strings_f
 */
void
GL_Strings_f(void)
{
	ri.Con_Printf(PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string);
	ri.Con_Printf(PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string);
	ri.Con_Printf(PRINT_ALL, "GL_VERSION: %s\n", gl_config.version_string);
	ri.Con_Printf(PRINT_ALL, "GL_EXTENSIONS: %s\n", gl_config.extensions_string);
}

/*
 * * GL_SetDefaultState
 */
void
GL_SetDefaultState(void)
{
	qglClearColor(1, 0, 0.5, 0.5);
	qglCullFace(GL_FRONT);
	qglEnable(GL_TEXTURE_2D);

	qglEnable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_GREATER, 0.666);

	qglDisable(GL_DEPTH_TEST);
	qglDisable(GL_CULL_FACE);
	qglDisable(GL_BLEND);

	qglColor4f(1, 1, 1, 1);

	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	qglShadeModel(GL_FLAT);

	GL_TextureMode(gl_texturemode->string);
	GL_TextureAlphaMode(gl_texturealphamode->string);
	GL_TextureSolidMode(gl_texturesolidmode->string);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_TexEnv(GL_REPLACE);
#ifndef QMAX
	if (qglPointParameterfEXT) {
		float		attenuations[3];

		attenuations[0] = gl_particle_att_a->value;
		attenuations[1] = gl_particle_att_b->value;
		attenuations[2] = gl_particle_att_c->value;

		qglEnable(GL_POINT_SMOOTH);
		qglPointParameterfEXT(GL_POINT_SIZE_MIN_EXT, gl_particle_min_size->value);
		qglPointParameterfEXT(GL_POINT_SIZE_MAX_EXT, gl_particle_max_size->value);
		qglPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, attenuations);
	}
	if (qglColorTableEXT) {
		qglEnable(GL_SHARED_TEXTURE_PALETTE_EXT);

		GL_SetTexturePalette(d_8to24table);
	}
#endif
}


