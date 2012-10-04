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

/*
d*_t structures are on-disk representations m*_t structures are in-memory
 *
 */

/*
 *
 * ============================================================================
 * ==
 *
 * BRUSH MODELS
 *
 * ============
 * =================================================================
 */


//
/* in memory representation */
//
/* !!! if this is changed, it must be changed in asm_draw.h too !!! */
typedef struct {
	vec3_t		position;
} mvertex_t;

typedef struct {
	vec3_t		mins, maxs;
	vec3_t		origin;	/* for sounds or lights */
	float		radius;
	int		headnode;
	int		visleafs;	/* not including the solid leaf 0 */
	int		firstface, numfaces;
} mmodel_t;


#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2


#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWTURB		0x10
#define SURF_DRAWBACKGROUND	0x40
#define SURF_UNDERWATER		0x80

/* !!! if this is changed, it must be changed in asm_draw.h too !!! */
typedef struct {
	unsigned short	v[2];
	unsigned int	cachededgeoffset;
} medge_t;

typedef struct mtexinfo_s {
	float		vecs[2][4];
	int		flags;
	int		numframes;
	struct mtexinfo_s *next;/* animation chain */
	image_t        *image;
	int		value;
} mtexinfo_t;

#define	VERTEXSIZE	9	/* was 7 Dukey */

typedef struct glpoly_s {
	vec3_t		center;
	struct glpoly_s *next;
	struct glpoly_s *chain;
	int		numverts;
	int		flags;	/* for SURF_UNDERWATER (not needed anymore?) */
	float		verts[4][VERTEXSIZE];	/* variable sized (xyz s1t1 s2t2) */
} glpoly_t;

typedef struct wallLight_s {

	vec3_t		color;
	vec3_t		origin;
	vec3_t		normal;	/* for light cut off need normal */
	float		intensity;
} wallLight_t;

typedef struct msurface_s {
	int		visframe;	/* should be drawn when node is
					 * crossed */

	cplane_t       *plane;
	int		flags;

	int		firstedge;	/* look up in model->surfedges[], negative numbers */
	int		numedges;	/* are backwards edges */

	short		texturemins[2];
	short		extents[2];

	int		light_s, light_t;	/* gl lightmap coordinates */
	int		dlight_s, dlight_t;	/* gl lightmap coordinates for dynamic lightmaps */

	glpoly_t       *polys;	/* multiple if warped */
	struct msurface_s *texturechain;
	struct msurface_s *lightmapchain;

	/* MH - detail textures begin */

	/*
	 * Q1 uses polys here where Q2 puts it in surfs.  much of a muchness
	 * really, although
	 */
	/* we'll keep it consistent with the Q2 way of doing things */
	struct msurface_s *detailchain;
	/* MH - detail textures end */

	mtexinfo_t     *texinfo;

	/* lighting info */
	int		dlightframe;
	int		dlightbits;

	int		lightmaptexturenum;
	byte		styles[MAXLIGHTMAPS];
	float		cached_light[MAXLIGHTMAPS];	/* values currently used in lightmap */
	byte           *samples;			/* [numstyles*surfsize] */
	byte           *stain_samples;

	wallLight_t    *wallLight;			/* used to create opengl lights. */
	int		fragmentframe;			/* Vic's awesome decals */
	vec3_t		center;
} msurface_t;

#define CONTENTS_NODE -1

typedef struct mnode_s {
	/* common with leaf */
	int		contents;	/* -1, to differentiate from leafs */
	int		visframe;	/* node needs to be traversed if current */

	float		minmaxs[6];	/* for bounding box culling */

	struct mnode_s *parent;

	/* node specific */
	cplane_t       *plane;
	struct mnode_s *children[2];

	unsigned short	firstsurface;
	unsigned short	numsurfaces;
} mnode_t;



typedef struct mleaf_s {
	/* common with node */
	int		contents;	/* wil be a negative contents number */
	int		visframe;	/* node needs to be traversed if current */

	float		minmaxs[6];	/* for bounding box culling */

	struct mnode_s *parent;

	/* leaf specific */
	int		cluster;
	int		area;

	msurface_t    **firstmarksurface;
	int		nummarksurfaces;
} mleaf_t;

/*
 *
 * ============================================================================
 * ==
 *
 * MD3 MODELS
 *
 * ==========
 * ===================================================================
 */
//
/* MD3 representation in memory */
/* Vic code with a few my changes */
/* -Harven */
//

typedef struct maliascoord_s {
	vec2_t		st;
} maliascoord_t;

typedef struct maliasvertex_s {
	vec3_t		point;
	vec3_t		normal;
} maliasvertex_t;

typedef struct {
	vec3_t		mins, maxs;
	vec3_t		translate;
	float		radius;
} maliasframe_t;

typedef struct {
	char		name[MD3_MAX_PATH];
	dorientation_t	orient;
} maliastag_t;

typedef struct {
	char		name[MD3_MAX_PATH];
	int		shader;
} maliasskin_t;

typedef struct {
	int		num_verts;
	char		name[MD3_MAX_PATH];
	maliasvertex_t *vertexes;
	maliascoord_t  *stcoords;

	int		num_tris;
	index_t        *indexes;
	int            *trneighbors;

	int		num_skins;
	maliasskin_t   *skins;
} maliasmesh_t;

typedef struct maliasmodel_s {
	int		num_frames;
	maliasframe_t  *frames;

	int		num_tags;
	maliastag_t    *tags;

	int		num_meshes;
	maliasmesh_t   *meshes;

	int		num_skins;
	maliasskin_t   *skins;
} maliasmodel_t;


/* =================================================================== */

//
/* Whole model */
//

/* Harven++ MD3 added mod_alias_md3 */
typedef enum {
	mod_bad, mod_brush, mod_sprite, mod_alias, mod_alias_md3
} modtype_t;

typedef struct model_s {
	char		name[MAX_QPATH];

	int		registration_sequence;

	modtype_t	type;
	int		numframes;

	int		flags;

	/* volume occupied by the model graphics */
	vec3_t	mins  , maxs;
	float		radius;
	
	/* solid volume for clipping  */
	qboolean	clipbox;
	vec3_t		clipmins, clipmaxs;


	/* brush model */
	int		firstmodelsurface, nummodelsurfaces;
	int		lightmap;	/* only for submodels */

	int		numsubmodels;
	mmodel_t       *submodels;

	int		numplanes;
	cplane_t       *planes;

	int		numleafs;	/* number of visible leafs, not counting 0 */
	mleaf_t        *leafs;

	int		numvertexes;
	mvertex_t      *vertexes;

	int		numedges;
	medge_t        *edges;

	int		numnodes;
	int		firstnode;
	mnode_t        *nodes;

	int		numtexinfo;
	mtexinfo_t     *texinfo;

	int		numsurfaces;
	msurface_t     *surfaces;

	int		numsurfedges;
	int            *surfedges;

	int		nummarksurfaces;
	msurface_t    **marksurfaces;

	dvis_t         *vis;

	byte           *lightdata;
	byte           *staindata;

	/* for alias models and skins */
	image_t        *skins[MAX_MD2SKINS];

	int		extradatasize;
	void           *extradata;
} model_t;

/*
 * ===========================================================================
 * =
 */

void		Mod_Init  (void);
void		Mod_ClearAll(void);
model_t        *Mod_ForName(char *name, qboolean crash);
mleaf_t        *Mod_PointInLeaf(float *p, model_t * model);
byte           *Mod_ClusterPVS(int cluster, model_t * model);

void		Mod_Modellist_f(void);

void		Mod_FreeAll(void);
void		Mod_Free  (model_t * mod);

struct model_s *R_RegisterModel(char *name);	/* MPO need this */
