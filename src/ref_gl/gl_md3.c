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
/* gl_md3.c  */

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
#include "gl_local.h"

float		shadelight_md3[3];

m_dlight_t	model_dlights_md3[MAX_MODEL_DLIGHTS];
int		model_dlights_num_md3;

extern cvar_t  *gl_shading;

/*
 * ================= Mod_LoadAliasMD3Model =================
 */

//
/* Some Vic code here not fully used */
//

int
R_FindTriangleWithEdge(index_t * indexes, int numtris, index_t start, index_t end, int ignore)
{
	int		i;
	int		match, count;

	count = 0;
	match = -1;

	for (i = 0; i < numtris; i++, indexes += 3) {
		if ((indexes[0] == start && indexes[1] == end)
		    || (indexes[1] == start && indexes[2] == end)
		    || (indexes[2] == start && indexes[0] == end)) {
			if (i != ignore)
				match = i;
			count++;
		} else if ((indexes[1] == start && indexes[0] == end)
			    || (indexes[2] == start && indexes[1] == end)
		    || (indexes[0] == start && indexes[2] == end)) {
			count++;
		}
	}

	/* detect edges shared by three triangles and make them seams */
	if (count > 2)
		match = -1;

	return match;
}

/*
 * =============== R_BuildTriangleNeighbors ===============
 */
void
R_BuildTriangleNeighbors(int *neighbors, index_t * indexes, int numtris)
{
	int		i, *n;
	index_t        *index;

	for (i = 0, index = indexes, n = neighbors; i < numtris; i++, index += 3, n += 3) {
		n[0] = R_FindTriangleWithEdge(indexes, numtris, index[1], index[0], i);
		n[1] = R_FindTriangleWithEdge(indexes, numtris, index[2], index[1], i);
		n[2] = R_FindTriangleWithEdge(indexes, numtris, index[0], index[2], i);
	}
}

//
/* And here some Vic and some my (Harven) */
//

void
Mod_LoadAliasMD3Model(model_t * mod, void *buffer)
{
	int		version, i, j, l;
	dmd3_t         *pinmodel;
	dmd3frame_t    *pinframe;
	dmd3tag_t      *pintag;
	dmd3mesh_t     *pinmesh;
	dmd3skin_t     *pinskin;
	dmd3coord_t    *pincoord;
	dmd3vertex_t   *pinvert;
	index_t        *pinindex, *poutindex;
	maliasvertex_t *poutvert;
	maliascoord_t  *poutcoord;
	maliasskin_t   *poutskin;
	maliasmesh_t   *poutmesh;
	maliastag_t    *pouttag;
	maliasframe_t  *poutframe;
	maliasmodel_t  *poutmodel;
	char		name[MAX_QPATH];
	float		lat, lng;

	pinmodel = (dmd3_t *) buffer;
	version = LittleLong(pinmodel->version);

	if (version != MD3_ALIAS_VERSION) {
		ri.Sys_Error(ERR_DROP, "%s has wrong version number (%i should be %i)",
		    mod->name, version, MD3_ALIAS_VERSION);
	}
	poutmodel = Hunk_Alloc(sizeof(maliasmodel_t));

	/* byte swap the header fields and sanity check */
	poutmodel->num_frames = LittleLong(pinmodel->num_frames);
	poutmodel->num_tags = LittleLong(pinmodel->num_tags);
	poutmodel->num_meshes = LittleLong(pinmodel->num_meshes);
	poutmodel->num_skins = 0;

	if (poutmodel->num_frames <= 0) {
		ri.Sys_Error(ERR_DROP, "model %s has no frames", mod->name);
	} else if (poutmodel->num_frames > MD3_MAX_FRAMES) {
		ri.Sys_Error(ERR_DROP, "model %s has too many frames", mod->name);
	}
	if (poutmodel->num_tags > MD3_MAX_TAGS) {
		ri.Sys_Error(ERR_DROP, "model %s has too many tags", mod->name);
	} else if (poutmodel->num_tags < 0) {
		ri.Sys_Error(ERR_DROP, "model %s has invalid number of tags", mod->name);
	}
	if (poutmodel->num_meshes <= 0) {
		ri.Sys_Error(ERR_DROP, "model %s has no meshes", mod->name);
	} else if (poutmodel->num_meshes > MD3_MAX_MESHES) {
		ri.Sys_Error(ERR_DROP, "model %s has too many meshes", mod->name);
	}

	/* load the frames */

	pinframe = (dmd3frame_t *) ((byte *) pinmodel + LittleLong(pinmodel->ofs_frames));
	poutframe = poutmodel->frames = Hunk_Alloc(sizeof(maliasframe_t) * poutmodel->num_frames);

	mod->radius = 0;
	ClearBounds(mod->mins, mod->maxs);

	for (i = 0; i < poutmodel->num_frames; i++, pinframe++, poutframe++) {
		for (j = 0; j < 3; j++) {
			poutframe->mins[j] = LittleFloat(pinframe->mins[j]);
			poutframe->maxs[j] = LittleFloat(pinframe->maxs[j]);
			poutframe->translate[j] = LittleFloat(pinframe->translate[j]);
		}

		poutframe->radius = LittleFloat(pinframe->radius);

		mod->radius = max(mod->radius, poutframe->radius);
		AddPointToBounds(poutframe->mins, mod->mins, mod->maxs);
		AddPointToBounds(poutframe->maxs, mod->mins, mod->maxs);
	}

	/* load the tags */
	pintag = (dmd3tag_t *) ((byte *) pinmodel + LittleLong(pinmodel->ofs_tags));
	pouttag = poutmodel->tags = Hunk_Alloc(sizeof(maliastag_t) * poutmodel->num_frames * poutmodel->num_tags);

	for (i = 0; i < poutmodel->num_frames; i++) {
		for (l = 0; l < poutmodel->num_tags; l++, pintag++, pouttag++) {
			memcpy(pouttag->name, pintag->name, MD3_MAX_PATH);
			for (j = 0; j < 3; j++) {
				pouttag->orient.origin[j] = LittleFloat(pintag->orient.origin[j]);
				pouttag->orient.axis[0][j] = LittleFloat(pintag->orient.axis[0][j]);
				pouttag->orient.axis[1][j] = LittleFloat(pintag->orient.axis[1][j]);
				pouttag->orient.axis[2][j] = LittleFloat(pintag->orient.axis[2][j]);
			}
			ri.Con_Printf(PRINT_ALL, "X: (%f %f %f) Y: (%f %f %f) Z: (%f %f %f)\n", pouttag->orient.axis[0][0], pouttag->orient.axis[0][1], pouttag->orient.axis[0][2], pouttag->orient.axis[1][0], pouttag->orient.axis[1][1], pouttag->orient.axis[1][2], pouttag->orient.axis[2][0], pouttag->orient.axis[2][1], pouttag->orient.axis[2][2]);
		}
	}


	/* load the meshes */
	pinmesh = (dmd3mesh_t *) ((byte *) pinmodel + LittleLong(pinmodel->ofs_meshes));
	poutmesh = poutmodel->meshes = Hunk_Alloc(sizeof(maliasmesh_t) * poutmodel->num_meshes);

	for (i = 0; i < poutmodel->num_meshes; i++, poutmesh++) {
		memcpy(poutmesh->name, pinmesh->name, MD3_MAX_PATH);

		if (strncmp((const char *)pinmesh->id, "IDP3", 4)) {
			ri.Sys_Error(ERR_DROP, "mesh %s in model %s has wrong id (%i should be %i)", poutmesh->name, mod->name, LittleLong(*((int *)pinmesh->id)), IDMD3HEADER);
		}
		poutmesh->num_tris = LittleLong(pinmesh->num_tris);
		poutmesh->num_skins = LittleLong(pinmesh->num_skins);
		poutmesh->num_verts = LittleLong(pinmesh->num_verts);

		if (poutmesh->num_skins <= 0) {
			ri.Sys_Error(ERR_DROP, "mesh %i in model %s has no skins", i, mod->name);
		} else if (poutmesh->num_skins > MD3_MAX_SHADERS) {
			ri.Sys_Error(ERR_DROP, "mesh %i in model %s has too many skins", i, mod->name);
		}
		if (poutmesh->num_tris <= 0) {
			ri.Sys_Error(ERR_DROP, "mesh %i in model %s has no triangles", i, mod->name);
		} else if (poutmesh->num_tris > MD3_MAX_TRIANGLES) {
			ri.Sys_Error(ERR_DROP, "mesh %i in model %s has too many triangles", i, mod->name);
		}
		if (poutmesh->num_verts <= 0) {
			ri.Sys_Error(ERR_DROP, "mesh %i in model %s has no vertices", i, mod->name);
		} else if (poutmesh->num_verts > MD3_MAX_VERTS) {
			ri.Sys_Error(ERR_DROP, "mesh %i in model %s has too many vertices", i, mod->name);
		}
		/* register all skins */

		pinskin = (dmd3skin_t *) ((byte *) pinmesh + LittleLong(pinmesh->ofs_skins));
		poutskin = poutmesh->skins = Hunk_Alloc(sizeof(maliasskin_t) * poutmesh->num_skins);

		for (j = 0; j < poutmesh->num_skins; j++, pinskin++, poutskin++) {
			memcpy(name, pinskin->name, MD3_MAX_PATH);
			if (name[1] == 'o')
				name[0] = 'm';
			if (name[1] == 'l')
				name[0] = 'p';
			memcpy(poutskin->name, name, MD3_MAX_PATH);
			mod->skins[i] = GL_FindImage(name, it_skin);
		}

		/* load the indexes */
		pinindex = (index_t *) ((byte *) pinmesh + LittleLong(pinmesh->ofs_tris));
		poutindex = poutmesh->indexes = Hunk_Alloc(sizeof(index_t) * poutmesh->num_tris * 3);

		for (j = 0; j < poutmesh->num_tris; j++, pinindex += 3, poutindex += 3) {
			poutindex[0] = (index_t) LittleLong(pinindex[0]);
			poutindex[1] = (index_t) LittleLong(pinindex[1]);
			poutindex[2] = (index_t) LittleLong(pinindex[2]);
		}

		/* load the texture coordinates */
		pincoord = (dmd3coord_t *) ((byte *) pinmesh + LittleLong(pinmesh->ofs_tcs));
		poutcoord = poutmesh->stcoords = Hunk_Alloc(sizeof(maliascoord_t) * poutmesh->num_verts);

		for (j = 0; j < poutmesh->num_verts; j++, pincoord++, poutcoord++) {
			poutcoord->st[0] = LittleFloat(pincoord->st[0]);
			poutcoord->st[1] = LittleFloat(pincoord->st[1]);
		}

		/* load the vertexes and normals */
		pinvert = (dmd3vertex_t *) ((byte *) pinmesh + LittleLong(pinmesh->ofs_verts));
		poutvert = poutmesh->vertexes = Hunk_Alloc(poutmodel->num_frames * poutmesh->num_verts * sizeof(maliasvertex_t));

		for (l = 0; l < poutmodel->num_frames; l++) {
			for (j = 0; j < poutmesh->num_verts; j++, pinvert++, poutvert++) {
				poutvert->point[0] = (float)LittleShort(pinvert->point[0]) * MD3_XYZ_SCALE;
				poutvert->point[1] = (float)LittleShort(pinvert->point[1]) * MD3_XYZ_SCALE;
				poutvert->point[2] = (float)LittleShort(pinvert->point[2]) * MD3_XYZ_SCALE;

				lat = (pinvert->norm >> 8) & 0xff;
				lng = (pinvert->norm & 0xff);

				lat *= M_PI / 128;
				lng *= M_PI / 128;

				poutvert->normal[0] = cos(lat) * sin(lng);
				poutvert->normal[1] = sin(lat) * sin(lng);
				poutvert->normal[2] = cos(lng);
			}
		}
		pinmesh = (dmd3mesh_t *) ((byte *) pinmesh + LittleLong(pinmesh->meshsize));

		poutmesh->trneighbors = Hunk_Alloc(sizeof(int) * poutmesh->num_tris * 3);
		R_BuildTriangleNeighbors(poutmesh->trneighbors, poutmesh->indexes, poutmesh->num_tris);

	}
	mod->type = mod_alias_md3;
}

void
light_md3_model(vec3_t normal, vec3_t color)
{
	int		i;
	vec3_t		colo[8];
	float		s1, s2, s3, angle;


	VectorClear(color);
	for (i = 0; i < model_dlights_num_md3; i++) {
		s1 = model_dlights_md3[i].direction[0] * normal[0] + model_dlights_md3[i].direction[1] * normal[1] + model_dlights_md3[i].direction[2] * normal[2];
		s2 = sqrt(model_dlights_md3[i].direction[0] * model_dlights_md3[i].direction[0] + model_dlights_md3[i].direction[1] * model_dlights_md3[i].direction[1] + model_dlights_md3[i].direction[2] * model_dlights_md3[i].direction[2]);
		s3 = sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);

		angle = s1 / (s2 * s3);
		colo[i][0] = model_dlights_md3[i].color[0] * (angle * 100);
		colo[i][1] = model_dlights_md3[i].color[1] * (angle * 100);
		colo[i][2] = model_dlights_md3[i].color[2] * (angle * 100);
		colo[i][0] /= 100;
		colo[i][1] /= 100;
		colo[i][2] /= 100;
		color[0] += colo[i][0];
		color[1] += colo[i][1];
		color[2] += colo[i][2];
	}
	color[0] /= model_dlights_num_md3;
	color[1] /= model_dlights_num_md3;
	color[2] /= model_dlights_num_md3;
	if (color[0] < 0 || color[1] < 0 || color[2] < 0)
		color[0] = color[1] = color[2] = 0;
}

void
GL_DrawAliasMD3FrameLerp(maliasmodel_t * paliashdr, maliasmesh_t mesh, float backlerp)
{
	int		i, j;
	maliasframe_t  *frame, *oldframe;
	vec3_t		move, delta, vectors[3];
	maliasvertex_t *v, *ov;
	vec3_t		tempVertexArray[4096];
	vec3_t		tempNormalsArray[4096];
	vec3_t		color1, color2, color3;
	float		alpha;
	float		frontlerp;
	vec3_t		tempangle;

	color1[0] = color1[1] = color1[2] = 0;
	color2[0] = color2[1] = color2[2] = 0;
	color3[0] = color3[1] = color3[2] = 0;

	frontlerp = 1.0 - backlerp;

	if (currententity->flags & RF_TRANSLUCENT)
		alpha = currententity->alpha;
	else
		alpha = 1.0;

	if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM))
		qglDisable(GL_TEXTURE_2D);

	frame = paliashdr->frames + currententity->frame;
	oldframe = paliashdr->frames + currententity->oldframe;

	VectorSubtract(currententity->oldorigin, currententity->origin, delta);
	VectorCopy(currententity->angles, tempangle);
	tempangle[YAW] = tempangle[YAW] - 90;
	AngleVectors(tempangle, vectors[0], vectors[1], vectors[2]);

	move[0] = DotProduct(delta, vectors[0]);	/* forward */
	move[1] = -DotProduct(delta, vectors[1]);	/* left */
	move[2] = DotProduct(delta, vectors[2]);	/* up */

	VectorAdd(move, oldframe->translate, move);

	for (i = 0; i < 3; i++) {
		move[i] = backlerp * move[i] + frontlerp * frame->translate[i];
	}

	v = mesh.vertexes + currententity->frame * mesh.num_verts;
	ov = mesh.vertexes + currententity->oldframe * mesh.num_verts;
	for (i = 0; i < mesh.num_verts; i++, v++, ov++) {
		VectorSet(tempNormalsArray[i],
		    v->normal[0] + (ov->normal[0] - v->normal[0]) * backlerp,
		    v->normal[1] + (ov->normal[1] - v->normal[1]) * backlerp,
		    v->normal[2] + (ov->normal[2] - v->normal[2]) * backlerp);
		VectorSet(tempVertexArray[i],
		    move[0] + ov->point[0] * backlerp + v->point[0] * frontlerp,
		    move[1] + ov->point[1] * backlerp + v->point[1] * frontlerp,
		    move[2] + ov->point[2] * backlerp + v->point[2] * frontlerp);
	}
	qglBegin(GL_TRIANGLES);

	for (j = 0; j < mesh.num_tris; j++) {
		/* use color instead of shadelight_md3... maybe */
		if (gl_shading->value) {
			light_md3_model(tempNormalsArray[mesh.indexes[3 * j + 0]], color1);
			light_md3_model(tempNormalsArray[mesh.indexes[3 * j + 1]], color2);
			light_md3_model(tempNormalsArray[mesh.indexes[3 * j + 2]], color3);
		}
		qglColor4f(shadelight_md3[0], shadelight_md3[1], shadelight_md3[2], alpha);
		qglTexCoord2f(mesh.stcoords[mesh.indexes[3 * j + 0]].st[0], mesh.stcoords[mesh.indexes[3 * j + 0]].st[1]);
		qglVertex3fv(tempVertexArray[mesh.indexes[3 * j + 0]]);

		qglColor4f(shadelight_md3[0], shadelight_md3[1], shadelight_md3[2], alpha);
		qglTexCoord2f(mesh.stcoords[mesh.indexes[3 * j + 1]].st[0], mesh.stcoords[mesh.indexes[3 * j + 1]].st[1]);
		qglVertex3fv(tempVertexArray[mesh.indexes[3 * j + 1]]);

		qglColor4f(shadelight_md3[0], shadelight_md3[1], shadelight_md3[2], alpha);
		qglTexCoord2f(mesh.stcoords[mesh.indexes[3 * j + 2]].st[0], mesh.stcoords[mesh.indexes[3 * j + 2]].st[1]);
		qglVertex3fv(tempVertexArray[mesh.indexes[3 * j + 2]]);
	}
	qglEnd();

	if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM))
		qglEnable(GL_TEXTURE_2D);
}

void
R_DrawAliasMD3Model(entity_t * e)
{
	maliasmodel_t  *paliashdr;
	image_t        *skin;
	int		i;

	paliashdr = (maliasmodel_t *) currentmodel->extradata;


	if (e->flags & (RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE)) {
		VectorClear(shadelight_md3);
		if (e->flags & RF_SHELL_HALF_DAM) {
			shadelight_md3[0] = 0.56;
			shadelight_md3[1] = 0.59;
			shadelight_md3[2] = 0.45;
		}
		if (e->flags & RF_SHELL_DOUBLE) {
			shadelight_md3[0] = 0.9;
			shadelight_md3[1] = 0.7;
		}
		if (e->flags & RF_SHELL_RED)
			shadelight_md3[0] = 1.0;
		if (e->flags & RF_SHELL_GREEN)
			shadelight_md3[1] = 1.0;
		if (e->flags & RF_SHELL_BLUE)
			shadelight_md3[2] = 1.0;
	} else if (e->flags & RF_FULLBRIGHT) {
		for (i = 0; i < 3; i++)
			shadelight_md3[i] = 1.0;
	} else {
		if (gl_shading->value)
			R_LightPoint(e->origin, shadelight_md3);
		else
			R_LightPointDynamics(e->origin, shadelight_md3, model_dlights_md3, &model_dlights_num_md3, 8);
		/* player lighting hack for communication back to server */
		/* big hack! */
		if (e->flags & RF_WEAPONMODEL) {

			/*
			 * pick the greatest component, which should be the
			 * same
			 */
			/* as the mono value returned by software */
			if (shadelight_md3[0] > shadelight_md3[1]) {
				if (shadelight_md3[0] > shadelight_md3[2])
					r_lightlevel->value = 150 * shadelight_md3[0];
				else
					r_lightlevel->value = 150 * shadelight_md3[2];
			} else {
				if (shadelight_md3[1] > shadelight_md3[2])
					r_lightlevel->value = 150 * shadelight_md3[1];
				else
					r_lightlevel->value = 150 * shadelight_md3[2];
			}

		}
		if (gl_monolightmap->string[0] != '0') {
			float		s = shadelight_md3[0];

			if (s < shadelight_md3[1])
				s = shadelight_md3[1];
			if (s < shadelight_md3[2])
				s = shadelight_md3[2];

			shadelight_md3[0] = s;
			shadelight_md3[1] = s;
			shadelight_md3[2] = s;
		}
	}
	if (e->flags & RF_MINLIGHT) {
		for (i = 0; i < 3; i++)
			if (shadelight_md3[i] > 0.1)
				break;
		if (i == 3) {
			shadelight_md3[0] = 0.1;
			shadelight_md3[1] = 0.1;
			shadelight_md3[2] = 0.1;
		}
	}
	if (e->flags & RF_GLOW) {	/* bonus items will pulse with time */
		float		scale;
		float		min;

		scale = 0.1 * sin(r_refdef.time * 7);
		for (i = 0; i < 3; i++) {
			min = shadelight_md3[i] * 0.8;
			shadelight_md3[i] += scale;
			if (shadelight_md3[i] < min)
				shadelight_md3[i] = min;
		}
	}
	if (e->flags & RF_DEPTHHACK)	/* hack the depth range to prevent
					 * view model from poking into walls */
		qglDepthRange(gldepthmin, gldepthmin + 0.3 * (gldepthmax - gldepthmin));

	if ((e->flags & RF_WEAPONMODEL) && (r_lefthand->value == 1.0)) {
		extern void	MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

		qglMatrixMode(GL_PROJECTION);
		qglPushMatrix();
		qglLoadIdentity();
		qglScalef(-1, 1, 1);
		MYgluPerspective(r_refdef.fov_y, (float)r_refdef.width / r_refdef.height, 4, 4096);
		qglMatrixMode(GL_MODELVIEW);

		qglCullFace(GL_BACK);
	}
	if (e->flags & RF_WEAPONMODEL) {
		if (r_lefthand->value == 2)
			return;
	}
	for (i = 0; i < paliashdr->num_meshes; i++) {
		c_alias_polys += paliashdr->meshes[i].num_tris;
	}

	qglPushMatrix();
	e->angles[PITCH] = -e->angles[PITCH];	/* sigh. */
	e->angles[YAW] = e->angles[YAW] - 90;
	R_RotateForEntity(e);
	e->angles[PITCH] = -e->angles[PITCH];	/* sigh. */
	e->angles[YAW] = e->angles[YAW] + 90;

	qglShadeModel(GL_SMOOTH);

	GL_TexEnv(GL_MODULATE);
	if (e->flags & RF_TRANSLUCENT) {
		qglEnable(GL_BLEND);
	}
	if ((e->frame >= paliashdr->num_frames) || (e->frame < 0)) {
		ri.Con_Printf(PRINT_DEVELOPER, "R_DrawAliasMD3Model %s: no such frame %d\n", currentmodel->name, e->frame);
		e->frame = 0;
		e->oldframe = 0;
	}
	if ((e->oldframe >= paliashdr->num_frames) || (e->oldframe < 0)) {
		ri.Con_Printf(PRINT_DEVELOPER, "R_DrawAliasModel %s: no such oldframe %d\n",
		    currentmodel->name, e->oldframe);
		e->frame = 0;
		e->oldframe = 0;
	}
	if (!r_lerpmodels->value)
		e->backlerp = 0;

	for (i = 0; i < paliashdr->num_meshes; i++) {
		skin = currentmodel->skins[e->skinnum];
		if (!skin)
			skin = r_notexture;
		GL_Bind(skin->texnum);
		GL_DrawAliasMD3FrameLerp(paliashdr, paliashdr->meshes[i], e->backlerp);
	}

	if ((e->flags & RF_WEAPONMODEL) && (r_lefthand->value == 1.0)) {
		qglMatrixMode(GL_PROJECTION);
		qglPopMatrix();
		qglMatrixMode(GL_MODELVIEW);
		qglCullFace(GL_FRONT);
	}
	if (e->flags & RF_TRANSLUCENT) {
		qglDisable(GL_BLEND);
	}
	if (e->flags & RF_DEPTHHACK)
		qglDepthRange(gldepthmin, gldepthmax);

	GL_TexEnv(GL_REPLACE);
	qglShadeModel(GL_FLAT);

	qglPopMatrix();
	qglColor4f(1, 1, 1, 1);
}
