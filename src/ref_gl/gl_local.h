
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

#ifndef _GL_LOCAL_H_
#define _GL_LOCAL_H_

#include <stdio.h>
#include <math.h>

#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include <png.h>
#include <jpeglib.h>

#include "../client/ref.h"

#include "qgl.h"

#define	REF_VERSION	"GL 0.01"

/* up / down */
#define	PITCH	0

/* left / right */
#define	YAW		1

/* fall over */
#define	ROLL	2

#define MAX_MODEL_DLIGHTS 8

#define DIV254BY255 (0.9960784313725490196078431372549f)
#define DIV255 (0.003921568627450980392156862745098f)
#define DIV256 (0.00390625f)
#define DIV512 (0.001953125f)

#ifndef __VIDDEF_T
#define __VIDDEF_T
typedef struct {
	int	width , height;	/* coordinates from main game */
} viddef_t;

#endif

extern viddef_t	vid;


/*
 * skins will be outline flood filled and mip mapped pics and sprites with alpha
 * will be outline flood filled pic won't be mip mapped
 *
 * model skin sprite frame wall texture pic
 *
 */

typedef enum {
	it_skin,
	it_sprite,
	it_wall,
	it_pic,
	it_sky,
	it_part
} imagetype_t;

typedef struct image_s {

	char		name[MAX_QPATH];		/* game path, including extension */
	char		bare_name[MAX_QPATH];		/* filename only, as called when searching */
	imagetype_t	type;
	int		width, height;			/* source image */
	int		upload_width, upload_height;	/* after power of two and picmip */
	int		registration_sequence;		/* 0 = free */
	struct msurface_s *texturechain;		/* for sort-by-texture world drawing */
	int		texnum;				/* gl texture binding */
	float		sl, tl, sh, th;			/* 0,0 - 1,1 unless part of the scrap */
	
	qboolean	scrap;
	qboolean	has_alpha;
	qboolean	paletted;
	
	qboolean	is_cin;
	float		replace_scale;
	
} image_t;

#define	TEXNUM_LIGHTMAPS	1024
#define	TEXNUM_SCRAPS		1152
#define	TEXNUM_IMAGES		1153

#define		MAX_GLTEXTURES	1024

/* =================================================================== */

typedef enum {

	rserr_ok,
	rserr_invalid_fullscreen,
	rserr_invalid_mode,
	rserr_unknown

} rserr_t;

#include "gl_model.h"

void		GL_BeginRendering(int *x, int *y, int *width, int *height);
void		GL_EndRendering(void);

void		GL_SetDefaultState(void);
void		GL_UpdateSwapInterval(void);

extern float	gldepthmin, gldepthmax;

typedef struct {
	float		x, y, z;
	float		s, t;
	float		r, g, b;
} glvert_t;


#define	MAX_LBM_HEIGHT		480

#define BACKFACE_EPSILON	0.01


/* ==================================================== */

extern image_t	gltextures[MAX_GLTEXTURES];
extern int	numgltextures;

#ifdef QMAX
#define PARTICLE_TYPES 1024
#endif

#ifdef QMAX
extern image_t *r_particletextures[PARTICLE_TYPES];
extern image_t *r_particlebeam;
#else
extern image_t *r_particletexture;
#endif

extern image_t *r_detailtexture;
extern image_t *r_shelltexture;
extern image_t *r_radarmap;
extern image_t *r_around;
extern image_t *r_caustictexture;
extern image_t *r_bholetexture;
extern image_t *r_lblendimage;

extern image_t *r_celtexture;
extern image_t *r_notexture;
extern image_t *r_dynamicimage;
extern entity_t *currententity;
extern model_t *currentmodel;
extern int	r_visframecount;
extern int	r_framecount;
extern cplane_t	frustum[4];
extern int	c_brush_polys, c_alias_polys;


extern int	gl_filter_min, gl_filter_max;

/* view origin */
extern vec3_t	vup;
extern vec3_t	vpn;
extern vec3_t	vright;
extern vec3_t	r_origin;

/* screen size info */
extern refdef_t	r_refdef;
extern int	r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern cvar_t  *skydistance;

extern cvar_t  *r_norefresh;
extern cvar_t  *r_lefthand;
extern cvar_t  *r_drawentities;
extern cvar_t  *r_drawworld;
extern cvar_t  *r_speeds;
extern cvar_t  *r_fullbright;
extern cvar_t  *r_novis;
extern cvar_t  *r_nocull;
extern cvar_t  *r_lerpmodels;

#ifdef QMAX
extern cvar_t  *gl_partscale;
extern cvar_t  *gl_transrendersort;
extern cvar_t  *gl_particlelighting;
extern cvar_t  *gl_particledistance;
#endif

#ifndef QMAX
extern cvar_t  *gl_particle_min_size;
extern cvar_t  *gl_particle_max_size;
extern cvar_t  *gl_particle_size;
extern cvar_t  *gl_particle_att_a;
extern cvar_t  *gl_particle_att_b;
extern cvar_t  *gl_particle_att_c;
extern cvar_t  *gl_particles;
#endif

extern cvar_t  *gl_dlight_cutoff;
extern cvar_t  *gl_stainmaps;

extern cvar_t  *r_model_lightlerp;
extern cvar_t  *r_model_dlights;
extern cvar_t  *r_model_alpha;

extern cvar_t  *r_lightlevel;	/* FIXME: This is a HACK to get the client's light level */

extern cvar_t  *r_overbrightbits;
extern cvar_t  *r_cellshading;
extern cvar_t  *r_cellshading_width;

extern cvar_t  *gl_ext_mtexcombine;
extern cvar_t  *gl_ext_texture_compression;	/* Heffo - ARB Texture Compression */
extern cvar_t  *gl_anisotropic;
extern cvar_t  *gl_anisotropic_avail;
extern cvar_t  *gl_ext_nv_multisample_filter_hint;
extern cvar_t  *gl_nv_fog;

extern cvar_t  *font_color;

extern cvar_t  *gl_vertex_arrays;
extern cvar_t  *gl_colordepth;
extern cvar_t  *gl_ext_palettedtexture;
extern cvar_t  *gl_ext_multitexture;
extern cvar_t  *gl_ext_pointparameters;
extern cvar_t  *gl_ext_compiled_vertex_array;

extern cvar_t  *gl_screenshot_quality;

extern cvar_t  *gl_surftrans_light;
extern cvar_t  *gl_nosubimage;
extern cvar_t  *gl_bitdepth;
extern cvar_t  *gl_mode;
extern cvar_t  *gl_log;
extern cvar_t  *gl_lightmap;
extern cvar_t  *gl_shadows;
extern cvar_t  *gl_shadows_alpha;
extern cvar_t  *gl_shellstencil;
extern cvar_t  *gl_dynamic;
extern cvar_t  *gl_monolightmap;
extern cvar_t  *gl_nobind;
extern cvar_t  *gl_round_down;
extern cvar_t  *gl_picmip;
extern cvar_t  *gl_skymip;
extern cvar_t  *gl_showtris;
extern cvar_t  *gl_finish;
extern cvar_t  *gl_clear;
extern cvar_t  *gl_cull;
extern cvar_t  *gl_poly;
extern cvar_t  *gl_texsort;
extern cvar_t  *gl_polyblend;
extern cvar_t  *gl_flashblend;
extern cvar_t  *gl_lightmaptype;
extern cvar_t  *gl_modulate;
extern cvar_t  *gl_playermip;
extern cvar_t  *gl_drawbuffer;
extern cvar_t  *gl_3dlabs_broken;
extern cvar_t  *gl_driver;
extern cvar_t  *gl_texturemode;
extern cvar_t  *gl_texturealphamode;
extern cvar_t  *gl_texturesolidmode;
extern cvar_t  *gl_saturatelighting;
extern cvar_t  *gl_lockpvs;

extern cvar_t  *gl_screenshot_jpeg;	        /* Heffo - JPEG Screenshots */
extern cvar_t  *gl_screenshot_jpeg_quality;	/* Heffo - JPEG Screenshots */

extern cvar_t  *gl_fogenable;	  /* Enable */
extern cvar_t  *gl_fogred;	  /* Red */
extern cvar_t  *gl_foggreen;	  /* Green */
extern cvar_t  *gl_fogblue;	  /* Blue */
extern cvar_t  *gl_fogstart;	  /* Start */
extern cvar_t  *gl_fogend;	  /* End */
extern cvar_t  *gl_fogdensity;	  /* Density */
extern cvar_t  *gl_fogunderwater; /* Under Water Fog */

extern cvar_t  *gl_water_waves;   /* Water waves */
extern cvar_t  *gl_water_caustics;

extern cvar_t  *gl_flares;
extern cvar_t  *gl_flare_force_size;
extern cvar_t  *gl_flare_force_style;
extern cvar_t  *gl_flare_intensity;
extern cvar_t  *gl_flare_maxdist;
extern cvar_t  *gl_flare_scale;

/* Knightmare- allow disabling the nVidia water warp */
extern cvar_t  *gl_water_pixel_shader_warp;
extern cvar_t  *gl_blooms;
extern cvar_t  *gl_glares;
extern cvar_t  *gl_glares_size;
extern cvar_t  *gl_glares_intens;

extern cvar_t  *gl_detailtextures;

extern cvar_t  *gl_reflection_fragment_program;
extern cvar_t  *gl_reflection;			/* MPO */
extern cvar_t  *gl_reflection_debug;		/* MPO	for debugging the reflection */
extern cvar_t  *gl_reflection_max;		/* MPO  max number of water reflections */
extern cvar_t  *gl_reflection_shader;		/* MPO	use fragment shaders */
extern cvar_t  *gl_reflection_shader_image;
extern cvar_t  *gl_reflection_water_surface;

extern cvar_t  *gl_minimap;
extern cvar_t  *gl_minimap_size;
extern cvar_t  *gl_minimap_zoom;
extern cvar_t  *gl_minimap_style;
extern cvar_t  *gl_minimap_x;
extern cvar_t  *gl_minimap_y;

extern cvar_t  *gl_decals;
extern cvar_t  *gl_decals_time;
extern cvar_t  *gl_alpha_surfaces;

extern cvar_t  *vid_fullscreen;
extern cvar_t  *vid_gamma;
extern cvar_t  *cl_hudscale;
extern cvar_t  *cl_big_maps;
extern cvar_t  *cl_3dcam;
extern cvar_t  *deathmatch;

extern cvar_t  *intensity;

extern int	gl_lightmap_format;
extern int	gl_solid_format;
extern int	gl_alpha_format;
extern int	gl_tex_solid_format;
extern int	gl_tex_alpha_format;

extern int	c_visible_lightmaps;
extern int	c_visible_textures;

extern float	r_world_matrix[16];

#define	MAX_FLARES	1024
#define	FLARE_STYLES	6

typedef struct {
	int		size;
	int		style;
	vec3_t		color;
	vec3_t		origin;
} flare_t;

extern int	r_numflares;
extern flare_t	*r_flares[MAX_FLARES];


void		R_TranslatePlayerSkin(int playernum);
void		GL_Bind   (int texnum);
void		GL_MBind  (GLenum target, int texnum);
void		GL_TexEnv (GLenum value);
void		GL_EnableMultitexture(qboolean enable);
void		GL_Enable3dTextureUnit(qboolean enable);	/* Dukey */
void		GL_SelectTexture(GLenum);
void		GL_BlendFunction(GLenum sfactor, GLenum dfactor);
void		GL_ShadeModel(GLenum mode);

void		GL_Stencil(qboolean enable);
void		GL_Overbright(qboolean enable);

void		R_PushDlights(void);
void		R_MaxColorVec(vec3_t color);
void		R_LightPoint(vec3_t p, vec3_t color);
void		VLight_Init(void);
float		VLight_LerpLight(int index1, int index2, float ilerp, vec3_t dir, vec3_t angles, qboolean dlight);


void		R_SetupGL (void);
void		R_Clear   (void);
void		R_ShadowLight(vec3_t pos, vec3_t lightAdd);
void		R_LightPointDynamics(vec3_t p, vec3_t color, m_dlight_t * list, int *amount, int max);

void		drawPlayerReflection(void);
void		GL_DrawRadar(void);
void		R_PreRenderDynamic(refdef_t * fd);
void		R_RenderGlares(refdef_t * fd);

void		UpdateHardwareGamma(void);

/* ==================================================================== */

extern model_t *r_worldmodel;

extern unsigned	d_8to24table[256];

#ifndef QMAX
extern float	d_8to24tablef[256][3];

#endif

extern int	registration_sequence;

int		R_Init     (void *hinstance, void *hWnd);
void		R_Shutdown(void);

void		R_RenderView(refdef_t * fd);
void		R_RenderMirrorView(refdef_t * fd);
void		GL_ScreenShot_f(void);
void		GL_ScreenShot_JPG(void);
void		GL_ScreenShot_PNG(void);
void		R_DrawAliasModel(entity_t * e);
void		R_DrawAliasMD3Model(entity_t * e);
void		R_DrawAliasShadow(entity_t * e);
void		R_DrawBrushModel(entity_t * e);
void		R_DrawSpriteModel(entity_t * e);
void		R_DrawBeam(entity_t * e);
void		R_DrawWorld(void);
void		R_RenderDlights(void);
void		R_DrawAlphaSurfaces(void);
void		R_DrawAlphaSurfaces_Jitspoe(void);
void		R_RenderBrushPoly(msurface_t * fa);
void		R_InitParticleTexture(void);
void		Draw_InitLocal(void);
void		GL_SubdivideSurface(msurface_t * fa);
void		GL_SubdivideLightmappedSurface(msurface_t * fa, float subdivide_size);	/* Heffo surface subdivision */
qboolean	R_CullBox(vec3_t mins, vec3_t maxs);
void		R_RotateForEntity(entity_t * e);
void		R_MarkLeaves(void);
void		MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar); /* MPO */
double		calc_wave(GLfloat x, GLfloat y);	/* Dukey */

glpoly_t       *WaterWarpPolyVerts(glpoly_t * p);
void		EmitWaterPolys(msurface_t * fa);
void		R_AddSkySurface(msurface_t * fa);
void		R_ClearSkyBox(void);
void		R_DrawSkyBox(void);
void		R_MarkLights(dlight_t * light, int bit, mnode_t * node);
void		R_MarkStains(dlight_t * light, int bit, mnode_t * node);
void		GL_ClearDecals(void);
void		R_AddDecals(void);
void		R_Decals_Init(void);
void		R_AddDecal(vec3_t origin, vec3_t dir, float red, float green, float blue, float alpha, 
				float size, int type, int flags, float angle);


/* gl_blooms.c  */
void		R_BloomBlend(refdef_t * fd);
void		R_InitBloomTextures(void);

void		Draw_GetPicSize(int *w, int *h, char *name);
void		Draw_Pic  (int x, int y, char *name, float alpha);
void		Draw_StretchPic(int x, int y, int w, int h, char *name, float alpha);
void		Draw_Char (int x, int y, int num, int alpha);
void		Draw_TileClear(int x, int y, int w, int h, char *name);
void		Draw_Fill (int x, int y, int w, int h, int c);
void		Draw_FadeScreen(void);
void		Draw_FadeBox(int x, int y, int w, int h, float alpha);
void		Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, byte * data);
image_t        *Draw_FindPic(char *name);	/* MPO need this so we can call this method in gl_refl.c */

void		R_BeginFrame(float camera_separation);
void		R_SwapBuffers(int);
void		R_SetPalette(const unsigned char *palette);

int		Draw_GetPalette(void);

void		GL_ResampleTexture(unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight);

struct image_s *R_RegisterSkin(char *name);

void		LoadPCX   (char *filename, byte ** pic, byte ** palette, int *width, int *height);
void		LoadTGA   (char *filename, byte ** pic, int *width, int *height);
void		LoadJPG   (char *filename, byte ** pic, int *width, int *height);
void		LoadPNG   (char *filename, byte ** pic, int *width, int *height);


image_t        *GL_LoadPic(char *name, byte * pic, int width, int height, imagetype_t type, int bits);
image_t        *GL_FindImage(char *name, imagetype_t type);
void		GL_TextureMode(char *string);
void		GL_ImageList_f(void);

void		GL_SetTexturePalette(unsigned palette[256]);

void		GL_InitImages(void);
void		GL_ShutdownImages(void);

void		GL_FreeUnusedImages(void);

void		GL_TextureAlphaMode(char *string);
void		GL_TextureSolidMode(char *string);

void		R_ApplyStains(void);
void		R_StainNode(stain_t * st, mnode_t * node);

void		R_RenderFlares(void);
void		R_RenderFlare(flare_t *light);
void		GL_AddFlareSurface(msurface_t * surf);


/*
 * * GL extension emulation functions
 */
#ifdef QMAX
void		GL_DrawParticles(int num_particles, qboolean inWater);
#else
void		GL_DrawParticles(int n, const particle_t particles[], const unsigned colortable[768]);
#endif

/*
 * * GL config stuff
 */
#define GL_RENDERER_VOODOO		0x00000001
#define GL_RENDERER_VOODOO2   		0x00000002
#define GL_RENDERER_VOODOO_RUSH		0x00000004
#define GL_RENDERER_BANSHEE		0x00000008
#define GL_RENDERER_3DFX		0x0000000F

#define GL_RENDERER_PCX1		0x00000010
#define GL_RENDERER_PCX2		0x00000020
#define GL_RENDERER_PMX			0x00000040
#define GL_RENDERER_POWERVR		0x00000070

#define GL_RENDERER_PERMEDIA2		0x00000100
#define GL_RENDERER_GLINT_MX		0x00000200
#define GL_RENDERER_GLINT_TX		0x00000400
#define GL_RENDERER_3DLABS_MISC		0x00000800
#define GL_RENDERER_3DLABS		0x00000F00

#define GL_RENDERER_REALIZM		0x00001000
#define GL_RENDERER_REALIZM2		0x00002000
#define GL_RENDERER_INTERGRAPH		0x00003000

#define GL_RENDERER_3DPRO		0x00004000
#define GL_RENDERER_REAL3D		0x00008000
#define GL_RENDERER_RIVA128		0x00010000
#define GL_RENDERER_DYPIC		0x00020000

#define GL_RENDERER_V1000		0x00040000
#define GL_RENDERER_V2100		0x00080000
#define GL_RENDERER_V2200		0x00100000
#define GL_RENDERER_RENDITION		0x001C0000

#define GL_RENDERER_O2          	0x00100000
#define GL_RENDERER_IMPACT      	0x00200000
#define GL_RENDERER_RE			0x00400000
#define GL_RENDERER_IR			0x00800000
#define GL_RENDERER_SGI			0x00F00000

#define GL_RENDERER_MCD			0x01000000
#define GL_RENDERER_OTHER		0x80000000

typedef struct {
	int		renderer;
	const char     *renderer_string;
	const char     *vendor_string;
	const char     *version_string;
	const char     *extensions_string;

	qboolean	allow_cds;

	qboolean	mtexcombine;
	qboolean	NV_texshaders;
	qboolean	anisotropic;
	float		max_anisotropy;
	qboolean	GL_EXT_nv_multisample_filter_hint;
} glconfig_t;

#define GLSTATE_DISABLE_ALPHATEST	if (gl_state.alpha_test) { qglDisable(GL_ALPHA_TEST); gl_state.alpha_test=false; }
#define GLSTATE_ENABLE_ALPHATEST	if (!gl_state.alpha_test) { qglEnable(GL_ALPHA_TEST); gl_state.alpha_test=true; }

#define GLSTATE_DISABLE_BLEND		if (gl_state.blend) { qglDisable(GL_BLEND); gl_state.blend=false; }
#define GLSTATE_ENABLE_BLEND		if (!gl_state.blend) { qglEnable(GL_BLEND); gl_state.blend=true; }

#define GLSTATE_DISABLE_TEXGEN		if (gl_state.texgen) { qglDisable(GL_TEXTURE_GEN_S); qglDisable(GL_TEXTURE_GEN_T); qglDisable(GL_TEXTURE_GEN_R); gl_state.texgen=false; }
#define GLSTATE_ENABLE_TEXGEN		if (!gl_state.texgen) { qglEnable(GL_TEXTURE_GEN_S); qglEnable(GL_TEXTURE_GEN_T); qglEnable(GL_TEXTURE_GEN_R); gl_state.texgen=true; }


typedef struct {
	float		inverse_intensity;
	qboolean	fullscreen;

	int		prev_mode;

	unsigned char  *d_16to8table;

	int		lightmap_textures;

	int		currenttextures[3];	/* Dukey was 2  */
	int		currenttmu;

	float		camera_separation;
	qboolean	stereo_enabled;

	qboolean	alpha_test;
	qboolean	blend;
	qboolean	texgen;

	qboolean	reg_combiners;
	qboolean	sgis_mipmap;
	unsigned int	dst_texture;
	qboolean	tex_rectangle;

	qboolean	texture_compression;	/* Heffo - ARB Texture Compression */
	int		maxtexsize;

	unsigned char	originalRedGammaTable[256];
	unsigned char	originalGreenGammaTable[256];
	unsigned char	originalBlueGammaTable[256];
	
	qboolean	hwgamma;
	qboolean	fragment_program;	/* MPO does gfx support fragment programs */
	qboolean	nv_fog;

} glstate_t;

extern glconfig_t gl_config;
extern glstate_t gl_state;

/* vertex arrays */

#define MAX_ARRAY MAX_PARTICLES*4

#define VA_SetElem2(v,a,b)      ((v)[0]=(a),(v)[1]=(b))
#define VA_SetElem3(v,a,b,c)    ((v)[0]=(a),(v)[1]=(b),(v)[2]=(c))
#define VA_SetElem4(v,a,b,c,d)  ((v)[0]=(a),(v)[1]=(b),(v)[2]=(c),(v)[3]=(d))

extern float	tex_array[MAX_ARRAY][2];
extern float	vert_array[MAX_ARRAY][3];
extern float	col_array[MAX_ARRAY][4];

extern qboolean inlava;
extern qboolean inslime;
extern qboolean inwater;


/*
 * ====================================================================
 *
 * IMPORTED FUNCTIONS
 *
 * ====================================================================
 */

extern refimport_t ri;


/*
 * ====================================================================
 *
 * IMPLEMENTATION SPECIFIC FUNCTIONS
 *
 * ====================================================================
 */

#define		MAX_WALL_LIGHTS	1024

void		GLimp_BeginFrame(float camera_separation);
void		GLimp_EndFrame(void);
int		GLimp_Init (void *hinstance, void *hWnd);
void		GLimp_Shutdown(void);
int		GLimp_SetMode(int *pwidth, int *pheight, int mode, qboolean fullscreen);
void		GLimp_AppActivate(qboolean active);
void		GLimp_EnableLogging(qboolean enable);
void		GLimp_LogNewFrame(void);

extern int	g_numGlLights;
extern int	numberOfWallLights;
extern wallLight_t *wallLightArray[MAX_WALL_LIGHTS];	/* array of pointers :) */

/* sul */
#define MAX_RADAR_ENTS 128
typedef struct RadarEnt_s {
	float		color    [4];
	vec3_t		org;
} RadarEnt_t;

int		numRadarEnts;
RadarEnt_t	RadarEnts[MAX_RADAR_ENTS];

#endif /* _GL_LOCAL_H_ */
