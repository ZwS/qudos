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
 * NoCheat LOC support by NiceAss
 */

#include "client.h"

typedef struct {
	short		origin[3];
	char		name[64];
	qboolean	used;
} loc_t;

#define MAX_LOCATIONS 768

loc_t		locations[MAX_LOCATIONS];

int
CL_FreeLoc(void)
{
	int		i;

	for (i = 0; i < MAX_LOCATIONS; i++) {
		if (locations[i].used == false)
			return i;
	}

	/* just keep overwriting the last one.... */
	return MAX_LOCATIONS - 1;
}

void
CL_LoadLoc(void)
{
	char		mapname[32];
	FILE           *f;

	memset(locations, 0, sizeof(loc_t) * MAX_LOCATIONS);

	/* format map pathname */
	Q_strncpyz(mapname, cl.configstrings[CS_MODELS + 1] + 5, sizeof(mapname));
	mapname[strlen(mapname) - 4] = 0;

	f = fopen(va("locs/%s.loc", mapname), "r");
	if (!f)
		return;

	while (!feof(f)) {
		char           *token1, *token2, *token3, *token4;
		char		line      [128], *nl;
		int		index;

		/* read a line */
		fgets(line, sizeof(line), f);

		/* skip comments */
		if (line[0] == ':' || line[0] == ';' || line[0] == '/')
			continue;

		/* overwrite new line characters with null */
		nl = strchr(line, '\n');
		if (nl)
			*nl = '\0';

		/* break the line up into 4 tokens */
		token1 = line;

		token2 = strchr(token1, ' ');
		if (token2 == NULL)
			continue;
		*token2 = '\0';
		token2++;

		token3 = strchr(token2, ' ');
		if (token3 == NULL)
			continue;
		*token3 = '\0';
		token3++;

		token4 = strchr(token3, ' ');
		if (token4 == NULL)
			continue;
		*token4 = '\0';
		token4++;

		/* copy the data to the structure */
		index = CL_FreeLoc();
		locations[index].origin[0] = atoi(token1);
		locations[index].origin[1] = atoi(token2);
		locations[index].origin[2] = atoi(token3);
		Q_strncpyz(locations[index].name, token4, sizeof(locations[0].name));
		locations[index].used = true;
	}

	Com_Printf("CL_LoadLoc: %s.loc found and loaded\n", mapname);

	fclose(f);
}

/* return the index of the closest location */
int
CL_LocIndex(short origin[3])
{
	short		diff[3];
	float		minDist = -1;
	int		locIndex = -1;
	int		i;

	for (i = 0; i < MAX_LOCATIONS; i++) {
		float		dist;

		if (locations[i].used == false)
			continue;

		VectorSubtract(origin, locations[i].origin, diff);

		dist = diff[0] * diff[0] / 64.0 + diff[1] * diff[1] / 64.0 + diff[2] * diff[2] / 64.0;

		if (dist < minDist || minDist == -1) {
			minDist = dist;
			locIndex = i;
		}
	}

	return locIndex;
}

void
CL_LocDelete(void)
{
	int		index = CL_LocIndex(cl.frame.playerstate.pmove.origin);

	if (index != -1)
		locations[index].used = false;
	else
		Com_Printf("Warning: No location to delete\n");
}

void
CL_LocAdd(char *name)
{
	int		index = CL_FreeLoc();

	locations[index].origin[0] = cl.frame.playerstate.pmove.origin[0];
	locations[index].origin[1] = cl.frame.playerstate.pmove.origin[1];
	locations[index].origin[2] = cl.frame.playerstate.pmove.origin[2];

	Q_strncpyz(locations[index].name, name, sizeof(locations[0].name));
	locations[index].used = true;

	Com_Printf("Location %s added at (%d %d %d) %d\n", name,
	    cl.frame.playerstate.pmove.origin[0],
	    cl.frame.playerstate.pmove.origin[1],
	    cl.frame.playerstate.pmove.origin[2]);
}

void
CL_LocWrite(char *filename)
{
	int		i;
	FILE           *f;

	Sys_Mkdir("locs");

	f = fopen(va("locs/%s.loc", filename), "w");
	if (!f) {
		Com_Printf("Warning: Unable to open locs/%s.loc for writing.\n", filename);
		return;
	}
	for (i = 0; i < MAX_LOCATIONS; i++) {
		if (!locations[i].used)
			continue;

		fprintf(f, "%d %d %d %s\n",
		    locations[i].origin[0], locations[i].origin[1], locations[i].origin[2], locations[i].name);
	}

	fclose(f);

	Com_Printf("locs/%s.loc successfully written.\n", filename);
}

void
CL_LocPlace(void)
{
	trace_t		tr;
	vec3_t		end;
	short		there[3];
	int		index1, index2 = -1;

	index1 = CL_LocIndex(cl.frame.playerstate.pmove.origin);

	VectorMA(cl.predicted_origin, 8192, cl.v_forward, end);
	tr = CM_BoxTrace(cl.predicted_origin, end, vec3_origin, vec3_origin, 0, MASK_PLAYERSOLID);
	there[0] = tr.endpos[0] * 8;
	there[1] = tr.endpos[1] * 8;
	there[2] = tr.endpos[2] * 8;
	index2 = CL_LocIndex(there);

	if (index1 != -1)
		Cvar_ForceSet("loc_here", locations[index1].name);
	else
		Cvar_ForceSet("loc_here", "");

	if (index2 != -1)
		Cvar_ForceSet("loc_there", locations[index2].name);
	else
		Cvar_ForceSet("loc_there", "");
}

void
CL_AddViewLocs(void)
{
	int		index = CL_LocIndex(cl.frame.playerstate.pmove.origin);
	int		i;
	int		num = 0;

	if (!cl_drawlocs->value)
		return;

	for (i = 0; i < MAX_LOCATIONS; i++) {
		int		dist;
		entity_t	ent;

		if (locations[i].used == false)
			continue;

		dist = (cl.frame.playerstate.pmove.origin[0] - locations[i].origin[0]) *
		    (cl.frame.playerstate.pmove.origin[0] - locations[i].origin[0]) +
		    (cl.frame.playerstate.pmove.origin[1] - locations[i].origin[1]) *
		    (cl.frame.playerstate.pmove.origin[1] - locations[i].origin[1]) +
		    (cl.frame.playerstate.pmove.origin[2] - locations[i].origin[2]) *
		    (cl.frame.playerstate.pmove.origin[2] - locations[i].origin[2]);

		if (dist > 4000 * 4000)
			continue;

		memset(&ent, 0, sizeof(entity_t));
		ent.origin[0] = locations[i].origin[0] * 0.125;
		ent.origin[1] = locations[i].origin[1] * 0.125;
		ent.origin[2] = locations[i].origin[2] * 0.125;
		ent.skinnum = 0;
		ent.skin = NULL;
		ent.model = NULL;

		if (i == index)
			ent.origin[2] += sin(cl.time * 0.01) * 10.0;

		V_AddEntity(&ent);
		num++;
	}
}

void
CL_LocHelp_f(void)
{
	/* Xile/jitspoe - simple help cmd for reference */
	Com_Printf(
	    "Loc Commands\n"
	    "-----------\n"
	    "loc_add <label/description>\n"
	    "loc_del\n"
	    "loc_save\n"
	    "cl_drawlocs\n"
	    "say_team $loc_here\n"
	    "say_team $loc_there\n"
	    "-----------\n");
}
