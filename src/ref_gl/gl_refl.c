/*******************************************************
	gl_refl.c

	by:
		Matt Ownby	- original code
		dukey		- rewrote most of the above :)
		spike		- helped me with the matrix maths
		jitspoe		- shaders


	adds reflective water to the Quake2 engine

*******************************************************/

#include "gl_local.h"
#include "gl_refl.h"

/*
 * ================================global and local
 * variables==============================
 */
unsigned int	REFL_TEXW;		/* width  */
unsigned int	REFL_TEXH;		/* and height of the texture we are gonna use to capture our reflection */

unsigned int	g_reflTexW;		/* dynamic size of reflective texture */
unsigned int	g_reflTexH;

int		g_num_refl = 0;		/* how many reflections we need to generate */
int		g_active_refl = 0;	/* which reflection is being renderedat the moment */

float          *g_refl_X;
float          *g_refl_Y;
float          *g_refl_Z;		/* the Z (vertical) value of each reflection */
float          *g_waterDistance;	/* the rough distance from player to water vertice .. 
					 * we want to render the closest water surface. */
					 
float          *g_waterDistance2;	/* the distance from the player to
					 * water plane, different from above
					 * (player can be miles away from
					 * water but close to water plane) */
					 
vec3_t         *waterNormals;		/* water normal */
int            *g_tex_num;		/* corresponding texture numbers for each reflection */
int		maxReflections;		/* maximum number of reflections */

entity_t       *playerEntity = NULL;	/* used to create a player reflection (hopefully) */

qboolean	g_drawing_refl = false;
qboolean	g_refl_enabled = true;	/* whether reflections should be drawn at all */

qboolean	brightenTexture = true;	/* needed to stop glUpload32 method
					 * brightening fragment textures..
					 * fucks them up dirty hack tbh */

float		g_last_known_fov = 90.0;

unsigned int	gWaterProgramId;

image_t        *distortTex = NULL;	/* for shaders */
image_t        *waterNormalTex = NULL;	/* for shaders */

/*
 * ================================global and local
 * variables==============================
 */

void
setupShaders(void)
{
	int		len;
	char           *fragmentProgramText;
	char           *fragmentProgramText2;

	qglGenProgramsARB(1, &gWaterProgramId);
	qglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, gWaterProgramId);

	len = ri.FS_LoadFile("scripts/water1.arbf", (void **)&fragmentProgramText);

	if (len == -1) {

		ri.Con_Printf(PRINT_ALL, "\x02""Water fragment program (scripts/water1.arbf) not found, disabling it.\n");
		gl_state.fragment_program = false;
		return;
	}
	fragmentProgramText2 = (char *)malloc(len + 1);
	memcpy(fragmentProgramText2, fragmentProgramText, len);
	fragmentProgramText2[len] = 0;	/* null terminate  */

	qglProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, len, fragmentProgramText2);

	brightenTexture = false;/* dont brighten these textures we need them
				 * as they are.  */

	distortTex = Draw_FindPic(gl_reflection_shader_image->string);
	waterNormalTex = Draw_FindPic("/textures/water/normal.pcx");

	brightenTexture = true;	/* reset so normal textures load with extra
				 * brightness  */

	ri.FS_FreeFile(fragmentProgramText);	/* clear up  */
	free(fragmentProgramText2);	/* clear up  */

	if (!waterNormalTex || !distortTex) {
		gl_state.fragment_program = false;
		ri.Con_Printf(PRINT_ALL,"\x02" "Water distortion texture not found, disabling shader reflection.\n");
	}
}



/*
 * ================ R_shutdown_refl
 *
 * free allocated mem ================
 */
void
R_shutdown_refl(void)
{

	if (gl_reflection_fragment_program->value) {
		qglDeleteProgramsARB(1, &gWaterProgramId);	/* free fragment program */
	}
	R_clear_refl();		/* set number of reflections to 0 */

	free(g_refl_X);		/* free all other malloc'd stuff */
	free(g_refl_Y);
	free(g_refl_Z);
	free(g_tex_num);
	free(g_waterDistance);
	free(g_waterDistance2);
	free(waterNormals);
	/* free( playerEntity		);	//fix this later */
}

/*
 * ================ R_init_refl
 *
 * sets everything up ================
 */
void
R_init_refl(int maxNoReflections)
{

	int		power;
	int		maxSize;
	int		i;
	unsigned char  *buf = NULL;

	R_setupArrays(maxNoReflections);	/* setup number of reflections */

	/* okay we want to set REFL_TEXH etc to be less than the resolution  */
	/* otherwise white boarders are left .. we dont want that. */

	/*
	 * if waves function is turned off we can set reflection size to
	 * resolution.
	 */

	/*
	 * however if it is turned on in game white marks round the sides
	 * will be left again
	 */
	/* so maybe its best to leave this alone. */

	for (power = 2; power < vid.height; power *= 2) {

		REFL_TEXW = power;
		REFL_TEXH = power;
	}

	qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);	/* get max supported
							 * texture size */

	if (REFL_TEXW > maxSize) {

		for (power = 2; power < maxSize; power *= 2) {

			REFL_TEXW = power;
			REFL_TEXH = power;
		}
	}
	g_reflTexW = REFL_TEXW;
	g_reflTexH = REFL_TEXH;

	for (i = 0; i < maxReflections; i++) {

		buf = (unsigned char *)malloc(REFL_TEXW * REFL_TEXH * 3);	/* create empty buffer
										 * for texture */

		if (buf) {

			memset(buf, 255, (REFL_TEXW * REFL_TEXH * 3));	/* fill it with white
									 * color so we can
									 * easily see where our
									 * tex border is */
			g_tex_num[i] = txm_genTexObject(buf, REFL_TEXW, REFL_TEXH, GL_RGB, false, true);	/* make this texture */
			free(buf);	/* once we've made texture memory, we
					 * don't need the sys ram anymore */
		} else {
			fprintf(stderr, "Malloc failed?\n");
			exit(1);/* unsafe exit, but we don't ever expect
				 * malloc to fail anyway */
		}
	}

	/*
	 * if screen dimensions are smaller than texture size, we have to use
	 * screen dimensions instead (doh!)
	 */
	g_reflTexW = (vid.width < REFL_TEXW) ? vid.width : REFL_TEXW;	/* keeping these in for
									 * now .. */
	g_reflTexH = (vid.height < REFL_TEXH) ? vid.height : REFL_TEXH;

	/*
	 * ri.Con_Printf (PRINT_ALL,
	 * "<><><><><><><><><><><><><><><><><><>\n\n" );
	 */
	ri.Con_Printf(PRINT_ALL, "Initialising reflective textures\n\n");
	ri.Con_Printf(PRINT_ALL, "...reflective texture size set at %d\n", g_reflTexH);
	ri.Con_Printf(PRINT_ALL, "...maximum reflective textures %d\n\n", maxReflections);

	/*
	 * ri.Con_Printf (PRINT_ALL,
	 * "<><><><><><><><><><><><><><><><><><>\n\n" );
	 */

	if (gl_state.fragment_program)
		setupShaders();	/* setup shaders */
}


/*
 * ================ R_setupArrays
 *
 * creates the actual arrays to hold the reflections in ================
 */
void
R_setupArrays(int maxNoReflections)
{

	R_shutdown_refl();

	g_refl_X = (float *)malloc(sizeof(float) * maxNoReflections);
	g_refl_Y = (float *)malloc(sizeof(float) * maxNoReflections);
	g_refl_Z = (float *)malloc(sizeof(float) * maxNoReflections);
	g_waterDistance = (float *)malloc(sizeof(float) * maxNoReflections);
	g_waterDistance2 = (float *)malloc(sizeof(float) * maxNoReflections);
	g_tex_num = (int *)malloc(sizeof(int) * maxNoReflections);
	waterNormals = (vec3_t *) malloc(sizeof(vec3_t) * maxNoReflections);

	maxReflections = maxNoReflections;
}

/*
 * ================ R_clear_refl
 *
 * clears the relfection array ================
 */
void
R_clear_refl()
{

	g_num_refl = 0;
}

/*
 * ================ calculateDistance
 *
 * private method to work out distance from water to player .. ================
 */
float
calculateDistance(float x, float y, float z)
{

	vec3_t		distanceVector;
	vec3_t		v;
	float		distance;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	VectorSubtract(v, r_refdef.vieworg, distanceVector);
	distance = VectorLength(distanceVector);

	return distance;
}


/*
 * ================ R_add_refl
 *
 * creates an array of reflections ================
 */
void
R_add_refl(float x, float y, float z, float normalX, float normalY, float normalZ, float distance2)
{

	float		distance;
	int		i;

	if (!maxReflections)
		return;		/* safety check. */

	if (gl_reflection_max->value != maxReflections) {
		R_init_refl(gl_reflection_max->value);
	}
	/* check for duplicates .. */
	for (i = 0; i < g_num_refl; i++) {

		if (normalX == waterNormals[i][0] &&
		    normalY == waterNormals[i][1] &&
		    normalZ == waterNormals[i][2] &&
		    distance2 == g_waterDistance2[i])
			return;

		/* same water normal and same distance from plane */
	}

	distance = calculateDistance(x, y, z);	/* used to calc closest water
						 * surface */

	/* make sure we have room to add */
	if (g_num_refl < maxReflections) {

		g_refl_X[g_num_refl] = x;			/* needed to set pvs */
		g_refl_Y[g_num_refl] = y;			/* needed to set pvs */
		g_refl_Z[g_num_refl] = z;			/* needed to set pvs */
		g_waterDistance[g_num_refl] = distance;		/* needed to find closest water surface */
		g_waterDistance2[g_num_refl] = distance2;	/* needed for reflection transform */
		waterNormals[g_num_refl][0] = normalX;
		waterNormals[g_num_refl][1] = normalY;
		waterNormals[g_num_refl][2] = normalZ;
		g_num_refl++;
	} else {
		/* we want to use the closest surface */
		/* not just any random surface */
		/* good for when 1 reflection enabled. */

		for (i = 0; i < g_num_refl; i++) {

			if (distance < g_waterDistance[i]) {

				g_refl_X[i] = x;			/* needed to set pvs */
				g_refl_Y[i] = y;			/* needed to set pvs */
				g_refl_Z[i] = z;			/* needed to set pvs */
				g_waterDistance[i] = distance;		/* needed to find closest water surface */
				g_waterDistance2[i] = distance2;	/* needed for reflection transform */
				waterNormals[i][0] = normalX;
				waterNormals[i][1] = normalY;
				waterNormals[i][2] = normalZ;

				return;	/* lets go */
			}
		}

	}

}



int
txm_genTexObject(unsigned char *texData, int w, int h,
    int format, qboolean repeat, qboolean mipmap)
{
	unsigned int	texNum;

	qglGenTextures(1, &texNum);

	repeat = false;
	mipmap = false;

	if (texData) {

		qglBindTexture(GL_TEXTURE_2D, texNum);
		/* qglPixelStorei(GL_UNPACK_ALIGNMENT, 1); */

		/* Set the tiling mode */
		if (repeat) {
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		} else {
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
		}

		/* Set the filtering */
		if (mipmap) {
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			gluBuild2DMipmaps(GL_TEXTURE_2D, format, w, h, format,
			    GL_UNSIGNED_BYTE, texData);

		} else {
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format,
			    GL_UNSIGNED_BYTE, texData);
		}
	}
	return texNum;
}

/* based off of R_RecursiveWorldNode, */
/* this locates all reflective surfaces and their associated height */
void
R_RecursiveFindRefl(mnode_t * node)
{
	int		c, side, sidebit;
	cplane_t       *plane;
	msurface_t     *surf, **mark;
	mleaf_t        *pleaf;
	float		dot;

	if (node->contents == CONTENTS_SOLID)
		return;		/* solid */

	if (node->visframe != r_visframecount)
		return;

	/*
	 * MPO : if this function returns true, it means that the polygon is
	 * not visible
	 */
	/* in the frustum, therefore drawing it would be a waste of resources */
	if (R_CullBox(node->minmaxs, node->minmaxs + 3))
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
		dot = r_refdef.vieworg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = r_refdef.vieworg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = r_refdef.vieworg[2] - plane->dist;
		break;
	default:
		dot = DotProduct(r_refdef.vieworg, plane->normal) - plane->dist;
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
	R_RecursiveFindRefl(node->children[side]);

	for (c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c; c--, surf++) {
		if (surf->visframe != r_framecount)
			continue;

		if ((surf->flags & SURF_PLANEBACK) != sidebit)
			continue;	/* wrong side */

		/*
		 * MPO : from this point onward, we should be dealing with
		 * visible surfaces
		 */
		/* start MPO */

		/* if this is a reflective surface ... */
#if 0
		if ((surf->flags & SURF_DRAWTURB & (SURF_TRANS33|SURF_TRANS66) ) &&
			(surf->texinfo->flags & (SURF_TRANS33|SURF_TRANS66)))
#else
		if (surf->flags & (SURF_DRAWTURB)) 
#endif
		{
			
			/* and if it is flat on the Z plane ... */
			/* if (plane->type == PLANE_Z) */
			/* { */
			R_add_refl(surf->polys->verts[0][0], surf->polys->verts[0][1], surf->polys->verts[0][2],
			    surf->plane->normal[0], surf->plane->normal[1], surf->plane->normal[2],
			    plane->dist);
		}
		/* stop MPO */
	}

	/* recurse down the back side */
	R_RecursiveFindRefl(node->children[!side]);
}

/*
 * ================ R_DrawDebugReflTexture
 *
 * draws debug texture in game so you can see whats going on ================
 */
void
R_DrawDebugReflTexture()
{

	qglBindTexture(GL_TEXTURE_2D, g_tex_num[0]);	/* do the first texture */
	qglBegin(GL_QUADS);
	qglTexCoord2f(1, 1);
	qglVertex3f(0, 0, 0);
	qglTexCoord2f(0, 1);
	qglVertex3f(200, 0, 0);
	qglTexCoord2f(0, 0);
	qglVertex3f(200, 200, 0);
	qglTexCoord2f(1, 0);
	qglVertex3f(0, 200, 0);
	qglEnd();
}

/*
 * ================ R_UpdateReflTex
 *
 * this method renders the reflection into the right texture (slow) we have to
 * draw everything a 2nd time ================
 */
void
R_UpdateReflTex(refdef_t * fd)
{
	if (!g_num_refl)
		return;		/* nothing to do here */

	g_drawing_refl = true;	/* begin drawing reflection */

	g_last_known_fov = fd->fov_y;

	/* go through each reflection and render it */
	for (g_active_refl = 0; g_active_refl < g_num_refl; g_active_refl++) {

		qglClearColor(0, 0, 0, 1);	/* clear screen */
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		R_RenderView(fd);	/* draw the scene here! */

		qglBindTexture(GL_TEXTURE_2D, g_tex_num[g_active_refl]);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0,
		    (REFL_TEXW - g_reflTexW) >> 1,
		    (REFL_TEXH - g_reflTexH) >> 1,
		    0, 0, g_reflTexW, g_reflTexH);

	}

	g_drawing_refl = false;					/* done drawing refl */
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	/* clear stuff now cause we want to render scene */
}

/*
 * ================ mirrorMatrix
 *
 * creates a mirror matrix from the reflection plane this matrix is then
 * multiplied by the current world matrix to invert all the geometry thanks
 * spike :> ================
 */
void
mirrorMatrix(float normalX, float normalY, float normalZ, float distance)
{

	float		mirror[16];
	float		nx = normalX;
	float		ny = normalY;
	float		nz = normalZ;
	float		k = distance;

	mirror[0] = 1 - 2 * nx * nx;
	mirror[1] = -2 * nx * ny;
	mirror[2] = -2 * nx * nz;
	mirror[3] = 0;

	mirror[4] = -2 * ny * nx;
	mirror[5] = 1 - 2 * ny * ny;
	mirror[6] = -2 * ny * nz;
	mirror[7] = 0;

	mirror[8] = -2 * nz * nx;
	mirror[9] = -2 * nz * ny;
	mirror[10] = 1 - 2 * nz * nz;
	mirror[11] = 0;

	mirror[12] = -2 * nx * k;
	mirror[13] = -2 * ny * k;
	mirror[14] = -2 * nz * k;
	mirror[15] = 1;

	qglMultMatrixf(mirror);
}

/*
 * ================ R_LoadReflMatrix()
 *
 * alters texture matrix to handle our reflection ================
 */
void
R_LoadReflMatrix()
{
	float		aspect = (float)r_refdef.width / r_refdef.height;

	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();

	qglTranslatef(0.5, 0.5, 0);	/* Center texture */

	qglScalef(
	    0.5 * (float)g_reflTexW / REFL_TEXW,
	    0.5 * (float)g_reflTexH / REFL_TEXH,
	    1.0
	    );			/* Scale and bias */

	MYgluPerspective(g_last_known_fov, aspect, 4, 4096);

	qglRotatef(-90, 1, 0, 0);	/* put Z going up */
	qglRotatef(90, 0, 0, 1);	/* put Z going up */

	R_DoReflTransform(false);	/* do transform */
	qglTranslatef(0, 0, 0);
	qglMatrixMode(GL_MODELVIEW);
}


/*
 * ================ R_ClearReflMatrix()
 *
 * Load identity into texture matrix ================
 */
void
R_ClearReflMatrix()
{

	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
}





/*
 * ================ R_DoReflTransform
 *
 * sets modelview to reflection instead of normal view ================
 */
void
R_DoReflTransform(qboolean update)
{

	float		a       , b, c, d;	/* to define a plane .. */
	vec3_t		mirrorNormal;
	float		worldMatrix[16];

	mirrorNormal[0] = waterNormals[g_active_refl][0];
	mirrorNormal[1] = waterNormals[g_active_refl][1];
	mirrorNormal[2] = waterNormals[g_active_refl][2];

	a = mirrorNormal[0] * g_refl_X[g_active_refl];
	b = mirrorNormal[1] * g_refl_Y[g_active_refl];
	c = mirrorNormal[2] * g_refl_Z[g_active_refl];
	d = a + b + c;
	d = d * -1;

	qglRotatef(-r_refdef.viewangles[2], 1, 0, 0);	/* MPO : this handles rolling 
							* (ie when we strafe side to side we roll slightly) 
							*/
	qglRotatef(-r_refdef.viewangles[0], 0, 1, 0);	/* MPO : this handles up/down rotation */
	qglRotatef(-r_refdef.viewangles[1], 0, 0, 1);	/* MPO : this handles left/right rotation */
	
	qglTranslatef(-r_refdef.vieworg[0], -r_refdef.vieworg[1], -r_refdef.vieworg[2]);

	mirrorMatrix(mirrorNormal[0], mirrorNormal[1], mirrorNormal[2], d);

	if (!update)
		return;

	qglGetFloatv(GL_MODELVIEW_MATRIX, worldMatrix);

	vright[0] = worldMatrix[0];	/* setup these so setup frustrum works right .. */
	vup[0] = worldMatrix[1];
	vpn[0] = -worldMatrix[2];

	vright[1] = worldMatrix[4];
	vup[1] = worldMatrix[5];
	vpn[1] = -worldMatrix[6];

	vright[2] = worldMatrix[8];
	vup[2] = worldMatrix[9];
	vpn[2] = -worldMatrix[10];
}


/* does this need any explanation ? ;p */
void
setupClippingPlanes()
{

	double		clipPlane[] = {0, 0, 0, 0};
	float		normalX;
	float		normalY;
	float		normalZ;
	float		distance;

	if (!g_drawing_refl)
		return;

	/* setup variables */

	normalX = waterNormals[g_active_refl][0];
	normalY = waterNormals[g_active_refl][1];
	normalZ = waterNormals[g_active_refl][2];
	distance = g_waterDistance2[g_active_refl];

	if (r_refdef.rdflags & RDF_UNDERWATER) {

		clipPlane[0] = -normalX;
		clipPlane[1] = -normalY;
		clipPlane[2] = -normalZ;
		clipPlane[3] = distance;
	} else {

		clipPlane[0] = normalX;
		clipPlane[1] = normalY;
		clipPlane[2] = normalZ;
		clipPlane[3] = -distance;
	}

	qglEnable(GL_CLIP_PLANE0);
	qglClipPlane(GL_CLIP_PLANE0, clipPlane);
}

/* so we can see ourselves in reflection ! */
/* this is fine for water as the reflection is distored (with shaders) */
/* but for perfect mirrors it sucks a bit :) */
void
drawPlayerReflection(void)
{

	qboolean	shadows = false;

	if (!g_drawing_refl)
		return;

	if (playerEntity == NULL) {

		playerEntity = (entity_t *) malloc(sizeof(entity_t));

		memset(playerEntity, 0, sizeof(entity_t));

		playerEntity->skin = GL_FindImage("players/male/grunt.pcx", it_skin);
		playerEntity->model = R_RegisterModel("players/male/tris.md2");
	}
	playerEntity->origin[0] = r_refdef.vieworg[0];
	playerEntity->origin[1] = r_refdef.vieworg[1];
	playerEntity->origin[2] = r_refdef.vieworg[2];

	playerEntity->oldorigin[0] = r_refdef.vieworg[0];
	playerEntity->oldorigin[1] = r_refdef.vieworg[1];
	playerEntity->oldorigin[2] = r_refdef.vieworg[2];

	playerEntity->frame = 30;
	playerEntity->oldframe = 30;

	playerEntity->angles[0] = r_refdef.viewangles[0];
	playerEntity->angles[1] = r_refdef.viewangles[1];
	playerEntity->angles[2] = r_refdef.viewangles[2];

	currententity = playerEntity;
	currentmodel = playerEntity->model;

	if (gl_shadows->value) {
		gl_shadows->value = 0;
		shadows = true;	/* we dont want a shadow on the water .. looks stupid */
	}
	R_DrawAliasModel(playerEntity);

	if (shadows)
		gl_shadows->value = 1;

}
