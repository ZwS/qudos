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
/* cl_fx.c -- entity effects parsing and management */

#include "client.h"

#ifdef QMAX
void		addParticleLight(cparticle_t * p, float light, float lightvel, 
                                 float lcol0, float lcol1, float lcol2);
void		CL_GunSmokeEffect(vec3_t org, vec3_t dir, float size);
#endif

void		CL_LogoutEffect(vec3_t org, int type);
void		CL_ItemRespawnParticles(vec3_t org);
trace_t		CL_Trace(vec3_t start, vec3_t end, float size, int contentmask);

static vec3_t	avelocities[NUMVERTEXNORMALS];

extern struct model_s *cl_mod_smoke;
extern struct model_s *cl_mod_flash;


/*
 * ==============================================================
 *
 * PARTICLE MANAGEMENT
 *
 * ==============================================================
 */

cparticle_t    *active_particles, *free_particles;
cparticle_t	particles[MAX_PARTICLES];

int		cl_numparticles = MAX_PARTICLES;

#ifdef QMAX
/* here i convert old 256 color to RGB -- hax0r l337 */
const byte	default_pal[768] =
{
	0, 0, 0, 15, 15, 15, 31, 31, 31, 47, 47, 47, 63, 63, 63, 75, 75, 75, 91, 91, 91, 107, 107, 107, 123, 123, 123, 139, 139, 139, 155, 155, 155, 171, 171, 171, 187, 187, 187, 203, 203, 203, 219, 219, 219, 235, 235, 235, 99, 75, 35, 91, 67, 31, 83, 63, 31, 79, 59, 27, 71, 55, 27, 63, 47,
	23, 59, 43, 23, 51, 39, 19, 47, 35, 19, 43, 31, 19, 39, 27, 15, 35, 23, 15, 27, 19, 11, 23, 15, 11, 19, 15, 7, 15, 11, 7, 95, 95, 111, 91, 91, 103, 91, 83, 95, 87, 79, 91, 83, 75, 83, 79, 71, 75, 71, 63, 67, 63, 59, 59, 59, 55, 55, 51, 47, 47, 47, 43, 43, 39,
	39, 39, 35, 35, 35, 27, 27, 27, 23, 23, 23, 19, 19, 19, 143, 119, 83, 123, 99, 67, 115, 91, 59, 103, 79, 47, 207, 151, 75, 167, 123, 59, 139, 103, 47, 111, 83, 39, 235, 159, 39, 203, 139, 35, 175, 119, 31, 147, 99, 27, 119, 79, 23, 91, 59, 15, 63, 39, 11, 35, 23, 7, 167, 59, 43,
	159, 47, 35, 151, 43, 27, 139, 39, 19, 127, 31, 15, 115, 23, 11, 103, 23, 7, 87, 19, 0, 75, 15, 0, 67, 15, 0, 59, 15, 0, 51, 11, 0, 43, 11, 0, 35, 11, 0, 27, 7, 0, 19, 7, 0, 123, 95, 75, 115, 87, 67, 107, 83, 63, 103, 79, 59, 95, 71, 55, 87, 67, 51, 83, 63,
	47, 75, 55, 43, 67, 51, 39, 63, 47, 35, 55, 39, 27, 47, 35, 23, 39, 27, 19, 31, 23, 15, 23, 15, 11, 15, 11, 7, 111, 59, 23, 95, 55, 23, 83, 47, 23, 67, 43, 23, 55, 35, 19, 39, 27, 15, 27, 19, 11, 15, 11, 7, 179, 91, 79, 191, 123, 111, 203, 155, 147, 215, 187, 183, 203,
	215, 223, 179, 199, 211, 159, 183, 195, 135, 167, 183, 115, 151, 167, 91, 135, 155, 71, 119, 139, 47, 103, 127, 23, 83, 111, 19, 75, 103, 15, 67, 91, 11, 63, 83, 7, 55, 75, 7, 47, 63, 7, 39, 51, 0, 31, 43, 0, 23, 31, 0, 15, 19, 0, 7, 11, 0, 0, 0, 139, 87, 87, 131, 79, 79,
	123, 71, 71, 115, 67, 67, 107, 59, 59, 99, 51, 51, 91, 47, 47, 87, 43, 43, 75, 35, 35, 63, 31, 31, 51, 27, 27, 43, 19, 19, 31, 15, 15, 19, 11, 11, 11, 7, 7, 0, 0, 0, 151, 159, 123, 143, 151, 115, 135, 139, 107, 127, 131, 99, 119, 123, 95, 115, 115, 87, 107, 107, 79, 99, 99,
	71, 91, 91, 67, 79, 79, 59, 67, 67, 51, 55, 55, 43, 47, 47, 35, 35, 35, 27, 23, 23, 19, 15, 15, 11, 159, 75, 63, 147, 67, 55, 139, 59, 47, 127, 55, 39, 119, 47, 35, 107, 43, 27, 99, 35, 23, 87, 31, 19, 79, 27, 15, 67, 23, 11, 55, 19, 11, 43, 15, 7, 31, 11, 7, 23,
	7, 0, 11, 0, 0, 0, 0, 0, 119, 123, 207, 111, 115, 195, 103, 107, 183, 99, 99, 167, 91, 91, 155, 83, 87, 143, 75, 79, 127, 71, 71, 115, 63, 63, 103, 55, 55, 87, 47, 47, 75, 39, 39, 63, 35, 31, 47, 27, 23, 35, 19, 15, 23, 11, 7, 7, 155, 171, 123, 143, 159, 111, 135, 151, 99,
	123, 139, 87, 115, 131, 75, 103, 119, 67, 95, 111, 59, 87, 103, 51, 75, 91, 39, 63, 79, 27, 55, 67, 19, 47, 59, 11, 35, 47, 7, 27, 35, 0, 19, 23, 0, 11, 15, 0, 0, 255, 0, 35, 231, 15, 63, 211, 27, 83, 187, 39, 95, 167, 47, 95, 143, 51, 95, 123, 51, 255, 255, 255, 255, 255,
	211, 255, 255, 167, 255, 255, 127, 255, 255, 83, 255, 255, 39, 255, 235, 31, 255, 215, 23, 255, 191, 15, 255, 171, 7, 255, 147, 0, 239, 127, 0, 227, 107, 0, 211, 87, 0, 199, 71, 0, 183, 59, 0, 171, 43, 0, 155, 31, 0, 143, 23, 0, 127, 15, 0, 115, 7, 0, 95, 0, 0, 71, 0, 0, 47,
	0, 0, 27, 0, 0, 239, 0, 0, 55, 55, 255, 255, 0, 0, 0, 0, 255, 43, 43, 35, 27, 27, 23, 19, 19, 15, 235, 151, 127, 195, 115, 83, 159, 87, 51, 123, 63, 27, 235, 211, 199, 199, 171, 155, 167, 139, 119, 135, 107, 87, 159, 91, 83
};

/* this initializes all particle images - mods play with this... */
void
SetParticleImages(void)
{
	re.SetParticlePicture(particle_generic,      "particles/basic.png");
	re.SetParticlePicture(particle_smoke,        "particles/smoke.png");
	re.SetParticlePicture(particle_blood,        "particles/blood.png");
	re.SetParticlePicture(particle_blooddrop,    "particles/blood_drop.png");
	re.SetParticlePicture(particle_blooddrip,    "particles/blood_drip.png");
	re.SetParticlePicture(particle_redblood,     "particles/blood_red.png");
	re.SetParticlePicture(particle_bubble,       "particles/bubble.png");
	re.SetParticlePicture(particle_lensflare,    "particles/lensflare.png");
	re.SetParticlePicture(particle_inferno,      "particles/inferno.png");
	re.SetParticlePicture(particle_footprint,    "particles/footprint.png");
	re.SetParticlePicture(particle_blaster,      "particles/blaster.png");

	re.SetParticlePicture(particle_beam,         "particles/beam.png");
	re.SetParticlePicture(particle_lightning,    "particles/lightning.png");
	re.SetParticlePicture(particle_lightflare,   "particles/lightflare.png");

	/* animations */
	/* explosion */
	re.SetParticlePicture(particle_rexplosion1,  "particles/r_explod_1.png");
	re.SetParticlePicture(particle_rexplosion2,  "particles/r_explod_2.png");
	re.SetParticlePicture(particle_rexplosion3,  "particles/r_explod_3.png");
	re.SetParticlePicture(particle_rexplosion4,  "particles/r_explod_4.png");
	re.SetParticlePicture(particle_rexplosion5,  "particles/r_explod_5.png");
	re.SetParticlePicture(particle_rexplosion6,  "particles/r_explod_6.png");
	re.SetParticlePicture(particle_rexplosion7,  "particles/r_explod_7.png");
	re.SetParticlePicture(particle_dexplosion1,  "particles/d_explod_1.png");
	re.SetParticlePicture(particle_dexplosion2,  "particles/d_explod_2.png");
	re.SetParticlePicture(particle_dexplosion3,  "particles/d_explod_3.png");

	re.SetParticlePicture(particle_splash,       "particles/splash.png");
	re.SetParticlePicture(particle_bubblesplash, "particles/bubble_splash.png");

}

void
pRotateThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time);

int
color8red(int color8)
{
	return (default_pal[color8 * 3 + 0]);
}
int
color8green(int color8)
{
	return (default_pal[color8 * 3 + 1]);;
}
int
color8blue(int color8)
{
	return (default_pal[color8 * 3 + 2]);;
}

void
vectoanglerolled(vec3_t value1, float angleyaw, vec3_t angles)
{
	float		forward, yaw, pitch;

	yaw = (int)(atan2(value1[1], value1[0]) * 180 / M_PI);
	forward = sqrt(value1[0] * value1[0] + value1[1] * value1[1]);
	pitch = (int)(atan2(value1[2], forward) * 180 / M_PI);

	if (pitch < 0)
		pitch += 360;

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = -angleyaw;
}

float
newParticleTime()
{
	float		lerpedTime;

	lerpedTime = cl.time;

	return lerpedTime;
}

cparticle_t    *
setupParticle(
    float angle0, float angle1, float angle2,
    float org0, float org1, float org2,
    float vel0, float vel1, float vel2,
    float accel0, float accel1, float accel2,
    float color0, float color1, float color2,
    float colorvel0, float colorvel1, float colorvel2,
    float alpha, float alphavel,
    int blendfunc_src, int blendfunc_dst,
    float size, float sizevel,
    int image,
    int flags,
    void (*think) (cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time),
    qboolean thinknext)
{
	int		j;
	cparticle_t    *p = NULL;

	if (!free_particles)
		return NULL;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;

	p->start = p->time = cl.time;

	p->angle[0] = angle0;
	p->angle[1] = angle1;
	p->angle[2] = angle2;

	p->org[0] = org0;
	p->org[1] = org1;
	p->org[2] = org2;

	p->vel[0] = vel0;
	p->vel[1] = vel1;
	p->vel[2] = vel2;

	p->accel[0] = accel0;
	p->accel[1] = accel1;
	p->accel[2] = accel2;

	p->color[0] = color0;
	p->color[1] = color1;
	p->color[2] = color2;

	p->colorvel[0] = colorvel0;
	p->colorvel[1] = colorvel1;
	p->colorvel[2] = colorvel2;

	p->blendfunc_src = blendfunc_src;
	p->blendfunc_dst = blendfunc_dst;

	p->alpha = alpha;
	p->alphavel = alphavel;
	p->size = size;
	p->sizevel = sizevel;

	p->image = image;
	p->flags = flags;

	p->src_ent = 0;
	p->dst_ent = 0;

	if (think)
		p->think = think;
	else
		p->think = NULL;
	p->thinknext = thinknext;

	for (j = 0; j < P_LIGHTS_MAX; j++) {
		cplight_t      *plight = &p->lights[j];

		plight->isactive = false;
		plight->light = 0;
		plight->lightvel = 0;
		plight->lightcol[0] = 0;
		plight->lightcol[1] = 0;
		plight->lightcol[2] = 0;
	}

	return p;
}

void
addParticleLight(cparticle_t * p,
    float light, float lightvel,
    float lcol0, float lcol1, float lcol2)
{
	int		i;

	for (i = 0; i < P_LIGHTS_MAX; i++) {
		cplight_t      *plight = &p->lights[i];

		if (!plight->isactive) {
			plight->isactive = true;
			plight->light = light;
			plight->lightvel = lightvel;
			plight->lightcol[0] = lcol0;
			plight->lightcol[1] = lcol1;
			plight->lightcol[2] = lcol2;
			return;
		}
	}
}
#endif

/*
 * ==============================================================
 *
 * LIGHT STYLE MANAGEMENT
 *
 * ==============================================================
 */

typedef struct {
	int		length;
	float		value[3];
	float		map[MAX_QPATH];
}		clightstyle_t;

clightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
int		lastofs;


/*
 * ================ CL_ClearLightStyles ================
 */
void
CL_ClearLightStyles(void)
{
	memset(cl_lightstyle, 0, sizeof(cl_lightstyle));
	lastofs = -1;
}

/*
 * ================ CL_RunLightStyles ================
 */
void
CL_RunLightStyles(void)
{
	int		ofs;
	int		i;
	clightstyle_t  *ls;

	ofs = cl.time / 100;
	if (ofs == lastofs)
		return;
	lastofs = ofs;

	for (i = 0, ls = cl_lightstyle; i < MAX_LIGHTSTYLES; i++, ls++) {
		if (!ls->length) {
			ls->value[0] = ls->value[1] = ls->value[2] = 1.0;
			continue;
		}
		if (ls->length == 1)
			ls->value[0] = ls->value[1] = ls->value[2] = ls->map[0];
		else
			ls->value[0] = ls->value[1] = ls->value[2] = ls->map[ofs % ls->length];
	}
}

void
CL_SetLightstyle(int i)
{
	char           *s;
	int		j, k;

	s = cl.configstrings[i + CS_LIGHTS];

	j = strlen(s);
	if (j >= MAX_QPATH)
		Com_Error(ERR_DROP, "svc_lightstyle length=%i", j);

	cl_lightstyle[i].length = j;

	for (k = 0; k < j; k++)
		cl_lightstyle[i].map[k] = (float)(s[k] - 'a') / (float)('m' - 'a');
}

/*
 * ================ CL_AddLightStyles ================
 */
void
CL_AddLightStyles(void)
{
	int		i;
	clightstyle_t  *ls;

	for (i = 0, ls = cl_lightstyle; i < MAX_LIGHTSTYLES; i++, ls++)
		V_AddLightStyle(i, ls->value[0], ls->value[1], ls->value[2]);
}

/*
 * ==============================================================
 *
 * DLIGHT MANAGEMENT
 *
 * ==============================================================
 */

cdlight_t	cl_dlights[MAX_DLIGHTS];

/*
 * ================ CL_ClearDlights ================
 */
void
CL_ClearDlights(void)
{
	memset(cl_dlights, 0, sizeof(cl_dlights));
}

/*
 * =============== CL_AllocDlight
 *
 * ===============
 */
cdlight_t      *
CL_AllocDlight(int key)
{
	int		i;
	cdlight_t      *dl;

	/* first look for an exact key match */
	if (key) {
		dl = cl_dlights;
		for (i = 0; i < MAX_DLIGHTS; i++, dl++) {
			if (dl->key == key) {
				memset(dl, 0, sizeof(*dl));
				dl->key = key;
				return dl;
			}
		}
	}
	/* then look for anything else */
	dl = cl_dlights;
	for (i = 0; i < MAX_DLIGHTS; i++, dl++) {
		if (dl->die < cl.time) {
			memset(dl, 0, sizeof(*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	memset(dl, 0, sizeof(*dl));
	dl->key = key;
	return dl;
}

#ifdef QMAX
/*
 * ===============
 *
 * CL_CL_AllocFreeDlight
 *
 * ===============
 */
cdlight_t      *
CL_AllocFreeDlight()
{
	int		i;
	cdlight_t      *dl;

	/* then look for anything else */
	dl = cl_dlights;
	for (i = 0; i < MAX_DLIGHTS; i++, dl++) {
		if (dl->die < cl.time) {
			memset(dl, 0, sizeof(*dl));
			dl->key = -1;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	memset(dl, 0, sizeof(*dl));
	dl->key = -1;
	return dl;
}
#endif

/*
 * =============== CL_NewDlight ===============
 */
void
CL_NewDlight(int key, float x, float y, float z, float radius, float time)
{
	cdlight_t      *dl;

	dl = CL_AllocDlight(key);
	dl->origin[0] = x;
	dl->origin[1] = y;
	dl->origin[2] = z;
	dl->radius = radius;
	dl->die = cl.time + time;
}


/*
 * =============== CL_RunDLights
 *
 * ===============
 */
void
CL_RunDLights(void)
{
	int		i;
	cdlight_t      *dl;

	dl = cl_dlights;
	for (i = 0; i < MAX_DLIGHTS; i++, dl++) {
		if (!dl->radius)
			continue;

		if (dl->die < cl.time) {
			dl->radius = 0;
			return;
		}
		dl->radius -= cls.frametime * dl->decay;
		if (dl->radius < 0)
			dl->radius = 0;
	}
}

/*
 * ============== CL_ParseMuzzleFlash ==============
 */
void
CL_ParseMuzzleFlash(void)
{
	vec3_t		fv, rv;
	cdlight_t      *dl;
	int		i, weapon;
	centity_t      *pl;
	int		silenced;
	float		volume;
	char		soundname[64];

	i = MSG_ReadShort(&net_message);
	if (i < 1 || i >= MAX_EDICTS)
		Com_Error(ERR_DROP, "CL_ParseMuzzleFlash: bad entity");

	weapon = MSG_ReadByte(&net_message);
	silenced = weapon & MZ_SILENCED;
	weapon &= ~MZ_SILENCED;

	pl = &cl_entities[i];

	dl = CL_AllocDlight(i);
	VectorCopy(pl->current.origin, dl->origin);
	AngleVectors(pl->current.angles, fv, rv, NULL);
	VectorMA(dl->origin, 18, fv, dl->origin);
	VectorMA(dl->origin, 16, rv, dl->origin);
	if (silenced)
		dl->radius = 100 + (rand() & 31);
	else
		dl->radius = 200 + (rand() & 31);
	dl->minlight = 32;
	dl->die = cl.time;	/* + 0.1; */

	if (silenced)
		volume = 0.2;
	else
		volume = 1;

	switch (weapon) {
	case MZ_BLASTER:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_BLUEHYPERBLASTER:
		dl->color[0] = 0;
		dl->color[1] = 0;
		dl->color[2] = 1;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_HYPERBLASTER:
#ifdef QMAX
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0.5;
#else
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#endif
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_MACHINEGUN:
#ifdef QMAX
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0.5;
#else
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#endif
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		break;
	case MZ_SHOTGUN:
#ifdef QMAX
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0.5;
#else
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#endif
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/shotgf1b.wav"), volume, ATTN_NORM, 0);
		S_StartSound(NULL, i, CHAN_AUTO, S_RegisterSound("weapons/shotgr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case MZ_SSHOTGUN:
#ifdef QMAX
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0.5;
#else
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#endif
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/sshotf1b.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_CHAINGUN1:
		dl->radius = 200 + (rand() & 31);
#ifdef QMAX
		dl->color[0] = 1;
		dl->color[1] = 0.5;
		dl->color[2] = 0.25;
#else
		dl->color[0] = 1;
		dl->color[1] = 0.25;
		dl->color[2] = 0;
#endif
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		break;
	case MZ_CHAINGUN2:
		dl->radius = 225 + (rand() & 31);
#ifdef QMAX
		dl->color[0] = 1;
		dl->color[1] = 0.5;
		dl->color[2] = 0.1;
#else
		dl->color[0] = 1;
		dl->color[1] = 0.5;
		dl->color[2] = 0;
#endif
		dl->die = cl.time + 0.1;	/* long delay */
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.05);
		break;
	case MZ_CHAINGUN3:
		dl->radius = 250 + (rand() & 31);
#ifdef QMAX
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0.25;
#else
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#endif
		dl->die = cl.time + 0.1;	/* long delay */
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.033);
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.066);
		break;
	case MZ_RAILGUN:
#ifdef QMAX
		dl->color[0] = cl_railred->value / 255;
		dl->color[1] = cl_railgreen->value / 255;
		dl->color[2] = cl_railblue->value / 255;
		dl->die += 10000;
		dl->decay = 100;
#else
		dl->color[0] = 0.5;
		dl->color[1] = 0.5;
		dl->color[2] = 1.0;
#endif
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/railgf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_ROCKET:
		dl->color[0] = 1;
		dl->color[1] = 0.5;
		dl->color[2] = 0.2;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/rocklf1a.wav"), volume, ATTN_NORM, 0);
		S_StartSound(NULL, i, CHAN_AUTO, S_RegisterSound("weapons/rocklr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case MZ_GRENADE:
		dl->color[0] = 1;
		dl->color[1] = 0.5;
		dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), volume, ATTN_NORM, 0);
		S_StartSound(NULL, i, CHAN_AUTO, S_RegisterSound("weapons/grenlr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case MZ_BFG:
		dl->color[0] = 0;
		dl->color[1] = 1;
		dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/bfg__f1y.wav"), volume, ATTN_NORM, 0);
		break;

	case MZ_LOGIN:
		dl->color[0] = 0;
		dl->color[1] = 1;
		dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CL_LogoutEffect(pl->current.origin, weapon);
		break;
	case MZ_LOGOUT:
		dl->color[0] = 1;
		dl->color[1] = 0;
		dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CL_LogoutEffect(pl->current.origin, weapon);
		break;
	case MZ_RESPAWN:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CL_LogoutEffect(pl->current.origin, weapon);
		break;
		/* RAFAEL */
	case MZ_PHALANX:
		dl->color[0] = 1;
		dl->color[1] = 0.5;
		dl->color[2] = 0.5;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/plasshot.wav"), volume, ATTN_NORM, 0);
		break;
		/* RAFAEL */
	case MZ_IONRIPPER:
		dl->color[0] = 1;
		dl->color[1] = 0.5;
		dl->color[2] = 0.5;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/rippfire.wav"), volume, ATTN_NORM, 0);
		break;

		/* ====================== */
		/* PGM */
	case MZ_ETF_RIFLE:
		dl->color[0] = 0.9;
		dl->color[1] = 0.7;
		dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/nail1.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_SHOTGUN2:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/shotg2.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_HEATBEAM:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
		dl->die = cl.time + 100;

		/*
		 * S_StartSound (NULL, i, CHAN_WEAPON,
		 * S_RegisterSound("weapons/bfg__l1a.wav"), volume,
		 * ATTN_NORM, 0);
		 */
		break;
	case MZ_BLASTER2:
		dl->color[0] = 0;
		dl->color[1] = 1;
		dl->color[2] = 0;
		/* FIXME - different sound for blaster2 ?? */
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_TRACKER:

		/*
		 * negative flashes handled the same in gl/soft until
		 * CL_AddDLights
		 */
		dl->color[0] = -1;
		dl->color[1] = -1;
		dl->color[2] = -1;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/disint2.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_NUKE1:
		dl->color[0] = 1;
		dl->color[1] = 0;
		dl->color[2] = 0;
		dl->die = cl.time + 100;
		break;
	case MZ_NUKE2:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
		dl->die = cl.time + 100;
		break;
	case MZ_NUKE4:
		dl->color[0] = 0;
		dl->color[1] = 0;
		dl->color[2] = 1;
		dl->die = cl.time + 100;
		break;
	case MZ_NUKE8:
		dl->color[0] = 0;
		dl->color[1] = 1;
		dl->color[2] = 1;
		dl->die = cl.time + 100;
		break;
		/* PGM */
		/* ====================== */
	}
}


/*
 * ============== CL_ParseMuzzleFlash2 ==============
 */
void
CL_ParseMuzzleFlash2(void)
{
	int		ent;
	vec3_t		origin;
	int		flash_number;
	cdlight_t      *dl;
	vec3_t		forward, right;
	char		soundname[64];

	ent = MSG_ReadShort(&net_message);
	if (ent < 1 || ent >= MAX_EDICTS)
		Com_Error(ERR_DROP, "CL_ParseMuzzleFlash2: bad entity");

	flash_number = MSG_ReadByte(&net_message);

	/* locate the origin */
	AngleVectors(cl_entities[ent].current.angles, forward, right, NULL);
	origin[0] = cl_entities[ent].current.origin[0] + forward[0] * monster_flash_offset[flash_number][0] + right[0] * monster_flash_offset[flash_number][1];
	origin[1] = cl_entities[ent].current.origin[1] + forward[1] * monster_flash_offset[flash_number][0] + right[1] * monster_flash_offset[flash_number][1];
	origin[2] = cl_entities[ent].current.origin[2] + forward[2] * monster_flash_offset[flash_number][0] + right[2] * monster_flash_offset[flash_number][1] + monster_flash_offset[flash_number][2];

	dl = CL_AllocDlight(ent);
	VectorCopy(origin, dl->origin);
	dl->radius = 200 + (rand() & 31);
	dl->minlight = 32;
	dl->die = cl.time;	/* + 0.1; */

	switch (flash_number) {
	case MZ2_INFANTRY_MACHINEGUN_1:
	case MZ2_INFANTRY_MACHINEGUN_2:
	case MZ2_INFANTRY_MACHINEGUN_3:
	case MZ2_INFANTRY_MACHINEGUN_4:
	case MZ2_INFANTRY_MACHINEGUN_5:
	case MZ2_INFANTRY_MACHINEGUN_6:
	case MZ2_INFANTRY_MACHINEGUN_7:
	case MZ2_INFANTRY_MACHINEGUN_8:
	case MZ2_INFANTRY_MACHINEGUN_9:
	case MZ2_INFANTRY_MACHINEGUN_10:
	case MZ2_INFANTRY_MACHINEGUN_11:
	case MZ2_INFANTRY_MACHINEGUN_12:
	case MZ2_INFANTRY_MACHINEGUN_13:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#ifdef QMAX
		CL_GunSmokeEffect(origin, vec3_origin, 0.5);
#else
		CL_ParticleEffect(origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
#endif
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_SOLDIER_MACHINEGUN_1:
	case MZ2_SOLDIER_MACHINEGUN_2:
	case MZ2_SOLDIER_MACHINEGUN_3:
	case MZ2_SOLDIER_MACHINEGUN_4:
	case MZ2_SOLDIER_MACHINEGUN_5:
	case MZ2_SOLDIER_MACHINEGUN_6:
	case MZ2_SOLDIER_MACHINEGUN_7:
	case MZ2_SOLDIER_MACHINEGUN_8:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#ifdef QMAX
		CL_GunSmokeEffect(origin, vec3_origin, 0.75);
#else
		CL_ParticleEffect(origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
#endif
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("soldier/solatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_GUNNER_MACHINEGUN_1:
	case MZ2_GUNNER_MACHINEGUN_2:
	case MZ2_GUNNER_MACHINEGUN_3:
	case MZ2_GUNNER_MACHINEGUN_4:
	case MZ2_GUNNER_MACHINEGUN_5:
	case MZ2_GUNNER_MACHINEGUN_6:
	case MZ2_GUNNER_MACHINEGUN_7:
	case MZ2_GUNNER_MACHINEGUN_8:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#ifdef QMAX
		CL_GunSmokeEffect(origin, vec3_origin, 1);
#else
		CL_ParticleEffect(origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
#endif
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("gunner/gunatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_ACTOR_MACHINEGUN_1:
	case MZ2_SUPERTANK_MACHINEGUN_1:
	case MZ2_SUPERTANK_MACHINEGUN_2:
	case MZ2_SUPERTANK_MACHINEGUN_3:
	case MZ2_SUPERTANK_MACHINEGUN_4:
	case MZ2_SUPERTANK_MACHINEGUN_5:
	case MZ2_SUPERTANK_MACHINEGUN_6:
	case MZ2_TURRET_MACHINEGUN:	/* PGM */
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#ifdef QMAX
		CL_GunSmokeEffect(origin, vec3_origin, 2);
#else
		CL_ParticleEffect(origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
#endif
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_BOSS2_MACHINEGUN_L1:
	case MZ2_BOSS2_MACHINEGUN_L2:
	case MZ2_BOSS2_MACHINEGUN_L3:
	case MZ2_BOSS2_MACHINEGUN_L4:
	case MZ2_BOSS2_MACHINEGUN_L5:
	case MZ2_CARRIER_MACHINEGUN_L1:	/* PMM */
	case MZ2_CARRIER_MACHINEGUN_L2:	/* PMM */
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#ifdef QMAX
		CL_GunSmokeEffect(origin, vec3_origin, 2);
#else
		CL_ParticleEffect(origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
#endif
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NONE, 0);
		break;

	case MZ2_SOLDIER_BLASTER_1:
	case MZ2_SOLDIER_BLASTER_2:
	case MZ2_SOLDIER_BLASTER_3:
	case MZ2_SOLDIER_BLASTER_4:
	case MZ2_SOLDIER_BLASTER_5:
	case MZ2_SOLDIER_BLASTER_6:
	case MZ2_SOLDIER_BLASTER_7:
	case MZ2_SOLDIER_BLASTER_8:
	case MZ2_TURRET_BLASTER:	/* PGM */
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("soldier/solatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_FLYER_BLASTER_1:
	case MZ2_FLYER_BLASTER_2:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("flyer/flyatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_MEDIC_BLASTER_1:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("medic/medatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_HOVER_BLASTER_1:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("hover/hovatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_FLOAT_BLASTER_1:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("floater/fltatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_SOLDIER_SHOTGUN_1:
	case MZ2_SOLDIER_SHOTGUN_2:
	case MZ2_SOLDIER_SHOTGUN_3:
	case MZ2_SOLDIER_SHOTGUN_4:
	case MZ2_SOLDIER_SHOTGUN_5:
	case MZ2_SOLDIER_SHOTGUN_6:
	case MZ2_SOLDIER_SHOTGUN_7:
	case MZ2_SOLDIER_SHOTGUN_8:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#ifdef QMAX
		CL_GunSmokeEffect(origin, vec3_origin, 0.75);
#else
		CL_SmokeAndFlash(origin);
#endif
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("soldier/solatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_TANK_BLASTER_1:
	case MZ2_TANK_BLASTER_2:
	case MZ2_TANK_BLASTER_3:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_TANK_MACHINEGUN_1:
	case MZ2_TANK_MACHINEGUN_2:
	case MZ2_TANK_MACHINEGUN_3:
	case MZ2_TANK_MACHINEGUN_4:
	case MZ2_TANK_MACHINEGUN_5:
	case MZ2_TANK_MACHINEGUN_6:
	case MZ2_TANK_MACHINEGUN_7:
	case MZ2_TANK_MACHINEGUN_8:
	case MZ2_TANK_MACHINEGUN_9:
	case MZ2_TANK_MACHINEGUN_10:
	case MZ2_TANK_MACHINEGUN_11:
	case MZ2_TANK_MACHINEGUN_12:
	case MZ2_TANK_MACHINEGUN_13:
	case MZ2_TANK_MACHINEGUN_14:
	case MZ2_TANK_MACHINEGUN_15:
	case MZ2_TANK_MACHINEGUN_16:
	case MZ2_TANK_MACHINEGUN_17:
	case MZ2_TANK_MACHINEGUN_18:
	case MZ2_TANK_MACHINEGUN_19:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#ifdef QMAX
		CL_GunSmokeEffect(origin, vec3_origin, 2);
#else
		CL_ParticleEffect(origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
#endif
		Com_sprintf(soundname, sizeof(soundname), "tank/tnkatk2%c.wav", 'a' + rand() % 5);
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound(soundname), 1, ATTN_NORM, 0);
		break;

	case MZ2_CHICK_ROCKET_1:
	case MZ2_TURRET_ROCKET:/* PGM */
		dl->color[0] = 1;
		dl->color[1] = 0.5;
		dl->color[2] = 0.2;
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("chick/chkatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_TANK_ROCKET_1:
	case MZ2_TANK_ROCKET_2:
	case MZ2_TANK_ROCKET_3:
		dl->color[0] = 1;
		dl->color[1] = 0.5;
		dl->color[2] = 0.2;
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("tank/tnkatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_SUPERTANK_ROCKET_1:
	case MZ2_SUPERTANK_ROCKET_2:
	case MZ2_SUPERTANK_ROCKET_3:
	case MZ2_BOSS2_ROCKET_1:
	case MZ2_BOSS2_ROCKET_2:
	case MZ2_BOSS2_ROCKET_3:
	case MZ2_BOSS2_ROCKET_4:
	case MZ2_CARRIER_ROCKET_1:
		/* case MZ2_CARRIER_ROCKET_2: */
		/* case MZ2_CARRIER_ROCKET_3: */
		/* case MZ2_CARRIER_ROCKET_4: */
		dl->color[0] = 1;
		dl->color[1] = 0.5;
		dl->color[2] = 0.2;
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("tank/rocket.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_GUNNER_GRENADE_1:
	case MZ2_GUNNER_GRENADE_2:
	case MZ2_GUNNER_GRENADE_3:
	case MZ2_GUNNER_GRENADE_4:
		dl->color[0] = 1;
		dl->color[1] = 0.5;
		dl->color[2] = 0;
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("gunner/gunatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_GLADIATOR_RAILGUN_1:
		/* PMM */
	case MZ2_CARRIER_RAILGUN:
	case MZ2_WIDOW_RAIL:
		/* pmm */
		dl->color[0] = 0.5;
		dl->color[1] = 0.5;
		dl->color[2] = 1.0;
		break;

		/* --- Xian's shit starts --- */
	case MZ2_MAKRON_BFG:
		dl->color[0] = 0.5;
		dl->color[1] = 1;
		dl->color[2] = 0.5;

		/*
		 * S_StartSound (NULL, ent, CHAN_WEAPON,
		 * S_RegisterSound("makron/bfg_fire.wav"), 1, ATTN_NORM, 0);
		 */
		break;

	case MZ2_MAKRON_BLASTER_1:
	case MZ2_MAKRON_BLASTER_2:
	case MZ2_MAKRON_BLASTER_3:
	case MZ2_MAKRON_BLASTER_4:
	case MZ2_MAKRON_BLASTER_5:
	case MZ2_MAKRON_BLASTER_6:
	case MZ2_MAKRON_BLASTER_7:
	case MZ2_MAKRON_BLASTER_8:
	case MZ2_MAKRON_BLASTER_9:
	case MZ2_MAKRON_BLASTER_10:
	case MZ2_MAKRON_BLASTER_11:
	case MZ2_MAKRON_BLASTER_12:
	case MZ2_MAKRON_BLASTER_13:
	case MZ2_MAKRON_BLASTER_14:
	case MZ2_MAKRON_BLASTER_15:
	case MZ2_MAKRON_BLASTER_16:
	case MZ2_MAKRON_BLASTER_17:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("makron/blaster.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_JORG_MACHINEGUN_L1:
	case MZ2_JORG_MACHINEGUN_L2:
	case MZ2_JORG_MACHINEGUN_L3:
	case MZ2_JORG_MACHINEGUN_L4:
	case MZ2_JORG_MACHINEGUN_L5:
	case MZ2_JORG_MACHINEGUN_L6:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#ifdef QMAX
		CL_GunSmokeEffect(origin, vec3_origin, 3);
#else
		CL_ParticleEffect(origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
#endif
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("boss3/xfire.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_JORG_MACHINEGUN_R1:
	case MZ2_JORG_MACHINEGUN_R2:
	case MZ2_JORG_MACHINEGUN_R3:
	case MZ2_JORG_MACHINEGUN_R4:
	case MZ2_JORG_MACHINEGUN_R5:
	case MZ2_JORG_MACHINEGUN_R6:
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#ifdef QMAX
		CL_GunSmokeEffect(origin, vec3_origin, 3);
#else
		CL_ParticleEffect(origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
#endif
		break;

	case MZ2_JORG_BFG_1:
		dl->color[0] = 0.5;
		dl->color[1] = 1;
		dl->color[2] = 0.5;
		break;

	case MZ2_BOSS2_MACHINEGUN_R1:
	case MZ2_BOSS2_MACHINEGUN_R2:
	case MZ2_BOSS2_MACHINEGUN_R3:
	case MZ2_BOSS2_MACHINEGUN_R4:
	case MZ2_BOSS2_MACHINEGUN_R5:
	case MZ2_CARRIER_MACHINEGUN_R1:	/* PMM */
	case MZ2_CARRIER_MACHINEGUN_R2:	/* PMM */

		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
#ifdef QMAX
		CL_GunSmokeEffect(origin, vec3_origin, 3);
#else
		CL_ParticleEffect(origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
#endif
		break;

		/* ====== */
		/* ROGUE */
	case MZ2_STALKER_BLASTER:
	case MZ2_DAEDALUS_BLASTER:
	case MZ2_MEDIC_BLASTER_2:
	case MZ2_WIDOW_BLASTER:
	case MZ2_WIDOW_BLASTER_SWEEP1:
	case MZ2_WIDOW_BLASTER_SWEEP2:
	case MZ2_WIDOW_BLASTER_SWEEP3:
	case MZ2_WIDOW_BLASTER_SWEEP4:
	case MZ2_WIDOW_BLASTER_SWEEP5:
	case MZ2_WIDOW_BLASTER_SWEEP6:
	case MZ2_WIDOW_BLASTER_SWEEP7:
	case MZ2_WIDOW_BLASTER_SWEEP8:
	case MZ2_WIDOW_BLASTER_SWEEP9:
	case MZ2_WIDOW_BLASTER_100:
	case MZ2_WIDOW_BLASTER_90:
	case MZ2_WIDOW_BLASTER_80:
	case MZ2_WIDOW_BLASTER_70:
	case MZ2_WIDOW_BLASTER_60:
	case MZ2_WIDOW_BLASTER_50:
	case MZ2_WIDOW_BLASTER_40:
	case MZ2_WIDOW_BLASTER_30:
	case MZ2_WIDOW_BLASTER_20:
	case MZ2_WIDOW_BLASTER_10:
	case MZ2_WIDOW_BLASTER_0:
	case MZ2_WIDOW_BLASTER_10L:
	case MZ2_WIDOW_BLASTER_20L:
	case MZ2_WIDOW_BLASTER_30L:
	case MZ2_WIDOW_BLASTER_40L:
	case MZ2_WIDOW_BLASTER_50L:
	case MZ2_WIDOW_BLASTER_60L:
	case MZ2_WIDOW_BLASTER_70L:
	case MZ2_WIDOW_RUN_1:
	case MZ2_WIDOW_RUN_2:
	case MZ2_WIDOW_RUN_3:
	case MZ2_WIDOW_RUN_4:
	case MZ2_WIDOW_RUN_5:
	case MZ2_WIDOW_RUN_6:
	case MZ2_WIDOW_RUN_7:
	case MZ2_WIDOW_RUN_8:
		dl->color[0] = 0;
		dl->color[1] = 1;
		dl->color[2] = 0;
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_WIDOW_DISRUPTOR:
		dl->color[0] = -1;
		dl->color[1] = -1;
		dl->color[2] = -1;
		S_StartSound(NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/disint2.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_WIDOW_PLASMABEAM:
	case MZ2_WIDOW2_BEAMER_1:
	case MZ2_WIDOW2_BEAMER_2:
	case MZ2_WIDOW2_BEAMER_3:
	case MZ2_WIDOW2_BEAMER_4:
	case MZ2_WIDOW2_BEAMER_5:
	case MZ2_WIDOW2_BEAM_SWEEP_1:
	case MZ2_WIDOW2_BEAM_SWEEP_2:
	case MZ2_WIDOW2_BEAM_SWEEP_3:
	case MZ2_WIDOW2_BEAM_SWEEP_4:
	case MZ2_WIDOW2_BEAM_SWEEP_5:
	case MZ2_WIDOW2_BEAM_SWEEP_6:
	case MZ2_WIDOW2_BEAM_SWEEP_7:
	case MZ2_WIDOW2_BEAM_SWEEP_8:
	case MZ2_WIDOW2_BEAM_SWEEP_9:
	case MZ2_WIDOW2_BEAM_SWEEP_10:
	case MZ2_WIDOW2_BEAM_SWEEP_11:
		dl->radius = 300 + (rand() & 100);
		dl->color[0] = 1;
		dl->color[1] = 1;
		dl->color[2] = 0;
		dl->die = cl.time + 200;
		break;
		/* ROGUE */
		/* ====== */

		/* --- Xian's shit ends --- */

	}
}


/*
 * =============== CL_AddDLights
 *
 * ===============
 */
void
CL_AddDLights(void)
{
	int		i;
	cdlight_t      *dl;

	dl = cl_dlights;

	/* ===== */
	/* PGM */
	for (i = 0; i < MAX_DLIGHTS; i++, dl++) {
		if (!dl->radius)
			continue;
		V_AddLight(dl->origin, dl->radius,
		    dl->color[0], dl->color[1], dl->color[2]);
	}

	/* PGM */
	/* ===== */
}

#ifdef QMAX
extern int	r_numdlights;
extern int	r_numentities;
extern entity_t	r_entities[MAX_ENTITIES];
void
CL_AddHeatDLights(void)
{
	float		timesin = 0.5 * (1 + sin(anglemod(cl.time * 0.005)));
	int		i;
	entity_t       *ent;

	ent = r_entities;

	for (i = 0; i < r_numentities; i++, ent++) {
		if (!ent || !(ent->flags & RF_IR_VISIBLE))
			continue;

		V_AddLight(ent->origin, 100 + 75 * timesin, 1, 1, 1);
	}
}
#endif

/*
 * =============== CL_ClearParticles ===============
 */
void
CL_ClearParticles(void)
{
	int		i;

	free_particles = &particles[0];
	active_particles = NULL;

	for (i = 0; i < cl_numparticles; i++)
		particles[i].next = &particles[i + 1];
	particles[cl_numparticles - 1].next = NULL;
}

#ifdef QMAX
/*
 * =============== CL_ParticleEffectSplash
 *
 * Water Splashing ===============
 */
#define SplashSize 10
#define colorAdd 25
#define WaterSplashSize 3
void
calcPartVelocity(cparticle_t * p, float scale, float *time, vec3_t velocity);
void
pWaterSplashThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	float		len;

	calcPartVelocity(p, 1, time, angle);
	VectorNormalize(angle);

	len = *alpha * WaterSplashSize;
	if (len < 1)
		len = 1;

	VectorScale(angle, len, angle);


	p->thinknext = true;
}

void
pSplashThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	float		len;

	calcPartVelocity(p, 1, time, angle);
	VectorNormalize(angle);

	len = *alpha * SplashSize;
	if (len < 1)
		len = 1;

	VectorScale(angle, len, angle);


	p->thinknext = true;
}

void
CL_ParticleWaterEffectSplash(vec3_t org, vec3_t dir, int color8, int count, int r)
{
	int		i, shaded = PART_SHADED;
	float		scale;
	vec3_t		angle  , end, dirscaled, color = {color8red(color8), color8green(color8), color8blue(color8)};

	scale = random() * 0.25 + 0.75;
	vectoanglerolled(dir, rand() % 360, angle);
	VectorScale(dir, scale, dirscaled);

	for (i = 0; i < count * 0.75; i++) {
		end[0] = org[0] + dirscaled[0] * 10 + crand() * 5;
		end[1] = org[1] + dirscaled[1] * 10 + crand() * 5;
		end[2] = org[2] + dirscaled[2] * 10 + crand() * 5;

		setupParticle(
		    end[0], end[1], end[2],
		    org[0], org[1], org[2],
		    0, 0, 0,
		    0, 0, 0,
		    color[0], color[1], color[2],
		    0, 0, 0,
		    1, -1.5 / (0.5 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    scale * 10, 10,
		    particle_splash,
		    PART_BEAM | shaded,
		    NULL, 0);
	}

	for (i = 0; i < count; i++) {
		VectorSet(angle,
		    dirscaled[0] * 65 + crand() * 25,
		    dirscaled[1] * 65 + crand() * 25,
		    dirscaled[2] * 65 + crand() * 25);
		setupParticle(
		    0, 0, 0,
		    org[0] + angle[0] * 0.25, org[1] + angle[1] * 0.25, org[2] + angle[2] * 0.25,
		    angle[0], angle[1], angle[2],
		    0, 0, 0,
		    color[0], color[1], color[2],
		    0, 0, 0,
		    1, -1.0 / (0.5 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    scale * (4 + random() * 1.75), 1,
		    particle_bubblesplash,
		    PART_DIRECTION | PART_GRAVITY,
		    pWaterSplashThink, true);
	}
}

void
CL_ParticleEffectSplash(vec3_t org, vec3_t dir, int color8, int count)
{
	int		i, flags = PART_GRAVITY | PART_DIRECTION;
	float		d;
	vec3_t		color = {color8red(color8), color8green(color8), color8blue(color8)};

	for (i = 0; i < count; i++) {
		d = rand() & 5;
		setupParticle(
		    org[0], org[1], org[2],
		    org[0] + d * dir[0], org[1] + d * dir[1], org[2] + d * dir[2],
		    dir[0] * 40 + crand() * 10, dir[1] * 40 + crand() * 10, dir[2] * 40 + crand() * 10,
		    0, 0, 0,
		    color[0], color[1], color[2],
		    0, 0, 0,
		    1, -1.0 / (0.5 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    1 - random() * 0.25, 0,
		    particle_generic,
		    flags,
		    pSplashThink, true);
	}
}

void
pExplosionBubbleThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{

	if (CM_PointContents(org, 0) & MASK_WATER)
		p->thinknext = true;
	else {
		p->think = NULL;
		p->alpha = 0;
	}
}

/*
 * =============== GENERIC PARTICLE THINKING ROUTINES ===============
 */
#define	STOP_EPSILON	0.1
void
calcPartVelocity(cparticle_t * p, float scale, float *time, vec3_t velocity)
{
	float		time1 = *time;
	float		time2 = time1 * time1;

	velocity[0] = scale * (p->vel[0] * time1 + (p->accel[0]) * time2);
	velocity[1] = scale * (p->vel[1] * time1 + (p->accel[1]) * time2);

	if (p->flags & PART_GRAVITY)
		velocity[2] = scale * (p->vel[2] * time1 + (p->accel[2] - (PARTICLE_GRAVITY)) * time2);
	else
		velocity[2] = scale * (p->vel[2] * time1 + (p->accel[2]) * time2);
}

void
ClipVelocity(vec3_t in, vec3_t normal, vec3_t out)
{
	float		backoff, change;
	int		i;

	backoff = VectorLength(in) * 0.25 + DotProduct(in, normal) * 3.0;

	for (i = 0; i < 3; i++) {
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}
}

void
pBounceThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	float		clipsize;
	trace_t		tr;
	vec3_t		velocity;

	clipsize = *size * 0.5;
	if (clipsize < 0.25)
		clipsize = 0.25;
	tr = CL_Trace(p->oldorg, org, clipsize, 1);

	if (tr.fraction < 1) {
		calcPartVelocity(p, 1, time, velocity);
		ClipVelocity(velocity, tr.plane.normal, p->vel);

		VectorCopy(vec3_origin, p->accel);
		VectorCopy(tr.endpos, p->org);
		VectorCopy(p->org, org);
		VectorCopy(p->org, p->oldorg);

		p->alpha = *alpha;
		p->size = *size;

		p->start = p->time = newParticleTime();

		if (p->flags & PART_GRAVITY && VectorLength(p->vel) < 2)
			p->flags &= ~PART_GRAVITY;
	}
	p->thinknext = true;
}

/*
 * =============== SWQ SPECIFIC STUFF... ===============
 */

void
CL_SpeedTrail(vec3_t start, vec3_t end)
{
	cparticle_t    *p;
	vec3_t		move;
	vec3_t		vec;
	float		len, dec, frac;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 4;
	VectorScale(vec, dec, vec);

	frac = len / 100;
	if (frac > 1)
		frac = 1;

	while (len > 0) {
		len -= dec;

		p = setupParticle(
		    random() * 360, random() * 15, 0,
		    move[0] + crandom() * 5.0, move[1] + crandom() * 5.0, move[2] + crandom() * 5.0,
		    0, 0, 0,
		    0, 0, 0,
		    100, 175, 255,
		    0, 0, 0,
		    frac * 0.5, -1.0 / (0.8 + frand() * 0.2),
		    GL_SRC_ALPHA, GL_ONE,
		    15.0 + random() * 15, -5 + random() * 2.5,
		    particle_smoke,
		    PART_SHADED,
		    pRotateThink, true);

		VectorAdd(move, vec, move);
	}
}

void
pStunRotateThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	vec3_t		angleVel;



	VectorSubtract(org, p->org, angleVel);
	VectorAdd(angle, angleVel, angle);
	VectorCopy(p->org, org);

	angle[YAW] = sin(angle[YAW]) * 30;
	angle[PITCH] = cos(angle[PITCH]) * 30;

	p->thinknext = true;
}

void
CL_StunBlast(vec3_t pos, vec3_t color, float size)
{
	int		i;

	for (i = 0; i < 8; i++) {
		setupParticle(
		    0, 0, 0,
		    pos[0], pos[1], pos[2],
		    crandom() * size, crandom() * size, crandom() * size,
		    0, 0, 0,
		    color[0], color[1], color[2],
		    0, 0, 0,
		    .5, -1 / (0.8 + frand() * 0.2),
		    GL_SRC_ALPHA, GL_ONE,
		    size, 0,
		    particle_generic,
		    0,
		    0, false);
	}

	setupParticle(
	    crandom() * 360, crandom() * 360, crandom() * 360,
	    pos[0], pos[1], pos[2],
	    75, 300, 75,
	    0, 0, 0,
	    color[0], color[1], color[2],
	    0, 0, 0,
	    .75, -1 / (0.8 + frand() * 0.2),
	    GL_SRC_ALPHA, GL_ONE,
	    size, size * 3,
	    particle_smoke,
	    PART_ANGLED,
	    pStunRotateThink, true);
}

void
CL_LaserStun(vec3_t pos, vec3_t direction, vec3_t color, float size)
{
	vec3_t		dir, angles;
	int		i;

	for (i = 0; i < 16; i++) {
		vectoangles2(direction, angles);
		angles[PITCH] += crandom() * 15;
		angles[YAW] += crandom() * 15;
		AngleVectors(angles, dir, NULL, NULL);
		setupParticle(
		    dir[0] * 5, dir[1] * 5, dir[2] * 5,
		    pos[0], pos[1], pos[2],
		    dir[0] * 10 * size, dir[1] * 10 * size, dir[2] * 10 * size,
		    0, 0, 0,
		    color[0], color[1], color[2],
		    0, 0, 0,
		    .5, -2.0 / (0.8 + frand() * 0.2),
		    GL_SRC_ALPHA, GL_ONE,
		    size / 5.0, 0,
		    particle_generic,
		    PART_DIRECTION,
		    0, false);
	}
}

void
CL_ForceTrail(vec3_t start, vec3_t end, qboolean light, float size)
{
	cparticle_t    *p;
	vec3_t		move;
	vec3_t		vec;
	float		len, dec, length, frac;
	int		i = 0;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = len = VectorNormalize(vec);

	dec = 1 + size / 5;
	VectorScale(vec, dec, vec);

	while (len > 0) {
		len -= dec;
		i++;
		frac = len / length;

		if (light)
			p = setupParticle(
			    random() * 360, random() * 15, 0,
			    move[0], move[1], move[2],
			    0, 0, 0,
			    0, 0, 0,
			    150, 200, 255,
			    0, 0, 0,
			    1, -2.0 / (0.8 + frand() * 0.2),
			    GL_SRC_ALPHA, GL_ONE,
			    size, size,
			    particle_smoke,
			    0,
			    pRotateThink, true);
		else
			p = setupParticle(
			    random() * 360, random() * 15, 0,
			    move[0], move[1], move[2],
			    0, 0, 0,
			    0, 0, 0,
			    255, 255, 255,
			    0, 0, 0,
			    1, -2.0 / (0.8 + frand() * 0.2),
			    GL_SRC_ALPHA, GL_ONE,
			    size, size,
			    particle_inferno,
			    0,
			    pRotateThink, true);

		VectorAdd(move, vec, move);
	}
}

/*
 * =============== CL_FlameTrail -- DDAY SPECIFIC STUFF... ===============
 */

void
CL_Tracer(vec3_t origin, vec3_t angle, int red, int green, int blue, float len, float size)
{
	vec3_t		dir;

	AngleVectors(angle, dir, NULL, NULL);
	VectorScale(dir, len, dir);

	setupParticle(
	    dir[0], dir[1], dir[2],
	    origin[0], origin[1], origin[2],
	    0, 0, 0,
	    0, 0, 0,
	    red, green, blue,
	    0, 0, 0,
	    1, 0,
	    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
	    size, 0,
	    particle_generic,
	    PART_DIRECTION | PART_INSTANT,
	    NULL, 0);
}

void
CL_BlueFlameTrail(vec3_t start, vec3_t end)
{
	cparticle_t    *p;
	vec3_t		move;
	vec3_t		vec;
	float		len, dec, length, frac;
	int		i = 0;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	length = len = VectorNormalize(vec);

	dec = 1;
	VectorScale(vec, dec, vec);

	while (len > 0) {
		len -= dec;
		i++;
		frac = len / length;

		p = setupParticle(
		    random() * 360, random() * 10, 0,
		    move[0], move[1], move[2],
		    0, 0, 0,
		    0, 0, 0,
		    40 + 50 * frac, 50, 255 - 255 * frac,
		    0, 0, 0,
		    0.5, -2.0 / (0.8 + frand() * 0.2),
		    GL_SRC_ALPHA, GL_ONE,
		    3.0 + 3.0 * frac, 0,
		    particle_inferno,
		    0,
		    pRotateThink, true);

		VectorAdd(move, vec, move);
	}
}

void
CL_InfernoTrail(vec3_t start, vec3_t end, float size)
{
	vec3_t		move;
	vec3_t		vec;
	float		len, dec, size2 = size * size;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = (20.0 * size2 + 1);
	VectorScale(vec, dec, vec);

	while (len > 0) {
		len -= dec;

		setupParticle(
		    random() * 360, random() * 45, 0,
		    move[0], move[1], move[2],
		    0, 0, 0,
		    0, 0, 0,
		    255, 255, 255,
		    0, -100, -200,
		    1, -1.0 / (0.8 + frand() * 0.2),
		    GL_SRC_ALPHA, GL_ONE,
		/*  1 + 50.0*size2*(random()*0.25 + .75), 0, */
		    1 + 20.0 * size2, 5 + 50.0 * size2,
		    particle_inferno,
		    0,
		    pRotateThink, true);

		VectorAdd(move, vec, move);
	}
}

void
CL_FlameTrail(vec3_t start, vec3_t end, float size, float grow, qboolean light)
{
	cparticle_t    *p;
	vec3_t		move;
	vec3_t		vec;
	float		len, dec;
	int		i = 0;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = (0.75 * size);
	VectorScale(vec, dec, vec);

	if (light)
		V_AddLight(start, 50 + (size / 10.0) * 75, 0.5 + random() * 0.5, 0.5, 0.1);

	while (len > 0) {
		len -= dec;
		i++;

		p = setupParticle(
		    random() * 360, random() * 15, 0,
		    move[0] + crand() * 3, move[1] + crand() * 3, move[2] + crand() * 3,
		    crand() * size, crand() * size, crand() * size,
		    0, 0, 0,
		    255, 255, 255,
		    0, 0, 0,
		    1, -2.0 / (0.8 + frand() * 0.2),
		    GL_SRC_ALPHA, GL_ONE,
		    size, grow,
		    particle_inferno,
		    0,
		    pRotateThink, true);

		VectorAdd(move, vec, move);
	}
}

void
CL_GloomFlameTrail(vec3_t start, vec3_t end, float size, float grow, qboolean light)
{
	cparticle_t    *p;
	vec3_t		move;
	vec3_t		vec;
	float		len, dec;
	int		i = 0;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = (0.75 * size);
	VectorScale(vec, dec, vec);

	if (light)
		V_AddLight(start, 50 + (size / 10.0) * 75, 0.5 + random() * 0.5, 0.5, 0.1);

	while (len > 0) {
		len -= dec;
		i++;

		p = setupParticle(
		    random() * 360, random() * 15, 0,
		    move[0] + crand() * 3, move[1] + crand() * 3, move[2] + crand() * 3,
		    crand() * size, crand() * size, crand() * size,
		    0, 0, 0,
		    50, 235, 50,
		    0, 0, 0,
		    1, -2.0 / (0.8 + frand() * 0.2),
		    GL_SRC_ALPHA, GL_ONE,
		    size, grow,
		    particle_inferno,
		    0,
		    pRotateThink, true);

		VectorAdd(move, vec, move);
	}
}

void
CL_Flame(vec3_t start, qboolean light)
{
	cparticle_t    *p;

	p = setupParticle(
	    random() * 360, random() * 15, 0,
	    start[0], start[1], start[2],
	    crand() * 10.0, crand() * 10.0, random() * 100.0,
	    0, 0, 0,
	    255, 255, 255,
	    0, 0, 0,
	    1, -2.0 / (0.8 + frand() * 0.2),
	    GL_SRC_ALPHA, GL_ONE,
	    10, -10,
	    particle_inferno,
	    0,
	    pRotateThink, true);

	if (light) {
		if (p)
			addParticleLight(p,
			    20 + random() * 20.0, 0,
			    0.5 + random() * 0.5, 0.5, 0.1);	/* weak big */
		if (p)
			addParticleLight(p,
			    250.0, 0,
			    0.01, 0.01, 0.01);
	}
}

void
CL_GloomFlame(vec3_t start, qboolean light)
{
	cparticle_t    *p;

	p = setupParticle(
	    random() * 360, random() * 15, 0,
	    start[0], start[1], start[2],
	    crand() * 10.0, crand() * 10.0, random() * 100.0,
	    0, 0, 0,
	    255, 255, 255,
	    0, 0, 0,
	    1, -2.0 / (0.8 + frand() * 0.2),
	    GL_SRC_ALPHA, GL_ONE,
	    50, -10,
	    particle_inferno,
	    0,
	    pRotateThink, true);

	if (light) {
		if (p)
			addParticleLight(p,
			    20 + random() * 20.0, 0,
			    0.5 + random() * 0.5, 0.5, 0.1);	/* weak big */
		if (p)
			addParticleLight(p,
			    250.0, 0,
			    0.01, 0.01, 0.01);
	}
}

/*
 * =============== CL_LightningBeam ===============
 */

void
CL_LightningBeam(vec3_t start, vec3_t end, int srcEnt, int dstEnt, float size)
{
	cparticle_t    *list;
	cparticle_t    *p = NULL;

	for (list = active_particles; list; list = list->next)
		if (list->src_ent == srcEnt && list->dst_ent == dstEnt && list->image == particle_lightning) {
			p = list;
			 /* p->start = */ p->time = cl.time;
			VectorCopy(start, p->angle);
			VectorCopy(end, p->org);

			return;
		}
	p = setupParticle(
	    start[0], start[1], start[2],
	    end[0], end[1], end[2],
	    0, 0, 0,
	    0, 0, 0,
	    255, 255, 255,
	    0, 0, 0,
	    1, -2,
	    GL_SRC_ALPHA, GL_ONE,
	    size, 0,
	    particle_lightning,
	    PART_LIGHTNING,
	    0, false);

	if (!p)
		return;

	p->src_ent = srcEnt;
	p->dst_ent = dstEnt;
}

void
CL_LightningFlare(vec3_t start, int srcEnt, int dstEnt)
{
	cparticle_t    *list;
	cparticle_t    *p = NULL;

	for (list = active_particles; list; list = list->next)
		if (list->src_ent == srcEnt && list->dst_ent == dstEnt && list->image == particle_lightflare) {
			p = list;
			p->start = p->time = cl.time;
			VectorCopy(start, p->org);
			return;
		}
	p = setupParticle(
	    0, 0, 0,
	    start[0], start[1], start[2],
	    0, 0, 0,
	    0, 0, 0,
	    255, 255, 255,
	    0, 0, 0,
	    1, -2.5,
	    GL_SRC_ALPHA, GL_ONE,
	    15, 0,
	    particle_lightflare,
	    0,
	    0, false);

	if (!p)
		return;

	p->src_ent = srcEnt;
	p->dst_ent = dstEnt;
}

/*
 * =============== CL_Explosion_Particle
 *
 * BOOM! ===============
 */

void
pExplosionThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{

	if (*alpha > .85)
		*image = particle_rexplosion1;
	else if (*alpha > .7)
		*image = particle_rexplosion2;
	else if (*alpha > .5)
		*image = particle_rexplosion3;
	else if (*alpha > .4)
		*image = particle_rexplosion4;
	else if (*alpha > .25)
		*image = particle_rexplosion5;
	else if (*alpha > .1)
		*image = particle_rexplosion6;
	else
		*image = particle_rexplosion7;

	*alpha *= 3.0;

	if (*alpha > 1.0)
		*alpha = 1;

	p->thinknext = true;
}

#define EXPLODESTAININTESITY 75
void
CL_Explosion_Particle(vec3_t org, float size, qboolean large, qboolean rocket)
{
	float		scale;
	cparticle_t    *p;

	scale = cl_explosion_scale->value;

	if (scale < 1)
		scale = 1;
	size *= scale;

	if (large) {
		p = setupParticle(
		    0, 0, 0,
		    org[0], org[1], org[2],
		    0, 0, 0,
		    0, 0, 0,
		    255, 255, 255,
		    0, 0, 0,
		    1, (0.5 + random() * 0.5) * (rocket) ? -2 : -1.5,
		    GL_SRC_ALPHA, GL_ONE,
		    (size != 0) ? size : (150 - (!rocket) ? 75 : 0), 0,
		    particle_rexplosion1,	/* whatever :p */
		    PART_DEPTHHACK_SHORT,
		    pExplosionThink, true);

		if (p) {	/* smooth color blend :D */
			float		lightsize = (large) ? 1.0 : 0.75;

			addParticleLight(p,
			    lightsize * 250, 0,
			    1, 1, 1);
			addParticleLight(p,
			    lightsize * 265, 0,
			    1, 0.75, 0);
			addParticleLight(p,
			    lightsize * 285, 0,
			    1, 0.25, 0);
			/* addParticleLight (p, */
			/* lightsize*300, 0, */
			/* 1, 0, 0); */
		}
	}
}

void
pDisruptExplosionThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{

	if (*alpha > .66666)
		*image = particle_dexplosion1;
	else if (*alpha > .33333)
		*image = particle_dexplosion2;
	else
		*image = particle_dexplosion3;

	*alpha *= 3.0;

	if (*alpha > 1.0)
		*alpha = 1;

	p->thinknext = true;
}

cparticle_t    *p;
void
CL_Disruptor_Explosion_Particle(vec3_t org, float size)
{
	float		alphastart = 1, alphadecel = -5;
	int		i;
	float		scale;

	scale = cl_explosion_scale->value;

	if (scale < 1)
		scale = 1;
	size *= scale;

	/* now add main sprite */

	/* now add main sprite */
	if (!cl_explosion->value) {
		p = setupParticle(
		    0, 0, 0,
		    org[0], org[1], org[2],
		    0, 0, 0,
		    0, 0, 0,
		    255, 255, 255,
		    0, 0, 0,
		    alphastart, alphadecel,
		    GL_SRC_ALPHA, GL_ONE,
		    size, 0,
		    particle_dexplosion1,
		    PART_DEPTHHACK_SHORT,
		    pDisruptExplosionThink, true);
	} else {
		for (i = 0; i < (8 + sqrt(size) * 0.5); i++) {
			vec3_t		origin =
			{
				org[0] + crandom() * size * 0.15,
				org[1] + crandom() * size * 0.15,
				org[2] + crandom() * size * 0.15
			};
			trace_t		trace = CL_Trace(org, origin, 0, 1);

			p = setupParticle(
			    random() * 360, crandom() * 90, 0,
			    trace.endpos[0], trace.endpos[1], trace.endpos[2],
			    0, 0, 0,
			    0, 0, 0,
			    255, 255, 255,
			    0, 0, 0,
			    1, -1.3 / (0.5 + frand() * 0.3),
			    GL_SRC_ALPHA, GL_ONE,
			    size * 0.75, -scale * 1.5,
			    particle_dexplosion2,
			    0,
			    pRotateThink, true);
		}
	}

	if (p) {
		/* float lightsize = size/150.0; */

		addParticleLight(p,
		    size * 1.0, 0,
		    1, 1, 1);
		addParticleLight(p,
		    size * 1.25, 0,
		    0.75, 0, 1);
		addParticleLight(p,
		    size * 1.65, 0,
		    0.25, 0, 1);
		addParticleLight(p,
		    size * 1.9, 0,
		    0, 0, 1);
	}
}


/*
 * =============== CL_WeatherFx
 *
 * weather effects ===============
 */
void 
pBubbleThink (cparticle_t *p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	if (CM_PointContents(org,0) & MASK_WATER)
	{
		if (*size>2)
			*size = 2;
		
		org[0] = org[0] + sin((org[0] + *time) * 0.05) * *size;
		org[1] = org[1] + cos((org[1] + *time) * 0.05) * *size;

		p->thinknext = true;
		pRotateThink (p, org, angle, alpha, size, image, time);
	}
	else
	{
		p->think = NULL;
		p->alpha = 0;
	}
}

void
pRainSplashThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	/* NO THINK!!!! */
}

void
pWeatherFXThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	float		length;
	int		i;
	vec3_t		len;

	VectorSubtract(p->angle, org, len);
	{
		float		time1   , time2;

		time1 = *time;
		time2 = time1 * time1;

		for (i = 0; i < 2; i++)
			angle[i] = 0.2 * (p->vel[i] * time1 + (p->accel[i]) * time2);
		angle[2] = 0.2 * (p->vel[2] * time1 + (p->accel[2] - PARTICLE_GRAVITY) * time2);

		length = VectorNormalize(angle);

		if (length > *size * 10.0)
			length = *size * 10.0;

		VectorScale(angle, length, angle);
	}

	/* now to trace for impact... */
	{
		trace_t		trace = CL_Trace(p->oldorg, org, 0.1, 1);

		if (trace.fraction < 1.0) {	/* delete and stain... */
			switch ((int)p->temp) {
			case 0:/* RAIN */

				/* PARTICLE RECONSTRUCTION */
				{
					vectoanglerolled(trace.plane.normal, rand() % 360, p->angle);
					VectorCopy(trace.endpos, p->org);
					VectorClear(p->vel);
					VectorClear(p->accel);
					p->image = particle_smoke;
					p->flags = PART_SHADED | PART_ANGLED;
					p->alpha = *alpha;
					p->alphavel = -0.5;
					p->start = cl.time;
					p->think = pRainSplashThink;
					p->size = *size;
					p->sizevel = 10 + 10 * random();
				}
				break;
			case 1:/* SNOW */
			default:
				/* kill this particle */
				p->alpha = 0;

				*alpha = 0;
				*size = 0;
				break;
			}

		}
	}


	VectorCopy(org, p->oldorg);

	p->thinknext = true;
}

void
CL_WeatherFx(vec3_t org, vec3_t vec, vec3_t color, int type, float size, float time)
{
	cparticle_t    *p;
	int		image, flags = 0;

	switch (type) {
	case 0:		/* RAIN */
		image = particle_generic;
		flags = PART_SHADED | PART_DIRECTION | PART_GRAVITY;
		break;
	case 1:		/* SNOW */
		image = particle_generic;
		break;
	default:
		image = particle_generic;
		flags = PART_TRANS | PART_SHADED | PART_DIRECTION | PART_GRAVITY;
		break;
	}

	p = setupParticle(
	    0, 0, 0,
	    org[0], org[1], org[2],
	    vec[0], vec[1], vec[2],
	    0, 0, 0,
	    color[0], color[1], color[2],
	    0, 0, 0,
	    1.0, -1 / (1 + time),
	    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
	    size, 0,
	    image,
	    flags,
	    pWeatherFXThink, true);

	if (p)
		p->temp = type;
}


/*
 * =============== CL_BloodHit
 *
 * blood spray ===============
 */
#define MAXBLEEDSIZE 5
/* drop of blood */
void
pBloodThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	float		length;
	int		i;
	vec3_t		len;

	VectorSubtract(p->angle, org, len);
	{
		float		time1   , time2;

		time1 = *time;
		time2 = time1 * time1;

		for (i = 0; i < 2; i++)
			angle[i] = 0.2 * (p->vel[i] * time1 + (p->accel[i]) * time2);
		angle[2] = 0.2 * (p->vel[2] * time1 + (p->accel[2] - PARTICLE_GRAVITY) * time2);

		length = VectorNormalize(angle);
		if (length > MAXBLEEDSIZE)
			length = MAXBLEEDSIZE;
		VectorScale(angle, length, angle);
	}

	/* now to trace for impact... */
	{
		trace_t		trace = CL_Trace(p->oldorg, org, length * 0.5, 1);

		if (trace.fraction < 1.0) {	/* delete and stain... */

			/*
			 * re.AddStain(org, 5+*alpha*10, 0, -50**alpha
			 * ,-50**alpha);
			 */

			*alpha = 0;
			*size = 0;
			p->alpha = 0;
		}
	}


	VectorCopy(org, p->oldorg);

	p->thinknext = true;
}

void
CL_BloodSmack(vec3_t org, vec3_t dir)
{
	setupParticle(
	    crand() * 180, crand() * 100, 0,
	    org[0], org[1], org[2],
	    0, 0, 0,
	    0, 0, 0,
	    255, 255, 255,
	    0, 0, 0,
	    1.0, -0.5 / (0.5 + frand() * 0.3),
	    GL_ZERO, GL_ONE_MINUS_SRC_COLOR,
	    10, 0,
	    particle_redblood,
	    PART_TRANS | PART_SHADED,
	    pRotateThink, true);
}

void
CL_BloodBleed(vec3_t org, vec3_t pos, vec3_t dir)
{
	setupParticle(
	    org[0], org[1], org[2],
	    org[0] + ((rand() & 7) - 4) + dir[0], org[1] + ((rand() & 7) - 4) + dir[1], org[2] + ((rand() & 7) - 4) + dir[2],
	    pos[0] * (random() * 3 + 5) * 1.0, pos[1] * (random() * 3 + 5) * 1.0, pos[2] * (random() * 3 + 5) * 1.0,
	    0, 0, 0,
	    255, 255, 255,
	    0, 0, 0,
	    1.0, -0.25 / (0.5 + frand() * 0.3),
	    GL_ZERO, GL_ONE_MINUS_SRC_COLOR,
	    MAXBLEEDSIZE * 0.5, 0,
	    particle_blooddrip,
	    PART_TRANS | PART_SHADED | PART_DIRECTION | PART_GRAVITY,
	    pBloodThink, true);
}

void
CL_BloodPuff(vec3_t org, vec3_t dir)
{
	int		i;
	float		d;

	for (i = 0; i < 5; i++) {
		d = rand() & 31;
		setupParticle(
		    crand() * 180, crand() * 100, 0,
		    org[0] + ((rand() & 7) - 4) + d * dir[0], org[1] + ((rand() & 7) - 4) + d * dir[1], org[2] + ((rand() & 7) - 4) + d * dir[2],
		    dir[0] * (crand() * 3 + 5), dir[1] * (crand() * 3 + 5), dir[2] * (crand() * 3 + 5),
		    0, 0, -100,
		    255, 0, 0,
		    0, 0, 0,
		    1.0, -1.0,
		    GL_SRC_ALPHA, GL_ONE,
		    10, 0,
		    particle_blood,
		    PART_SHADED,
		    pRotateThink, true);
	}
}

void
CL_GreenBloodHit(vec3_t org, vec3_t dir)
{
	int		i;
	float		d;

	for (i = 0; i < 5; i++) {
		d = rand() & 31;
		setupParticle(
		    crand() * 180, crand() * 100, 0,
		    org[0] + ((rand() & 7) - 4) + d * dir[0], org[1] + ((rand() & 7) - 4) + d * dir[1], org[2] + ((rand() & 7) - 4) + d * dir[2],
		    dir[0] * (crand() * 3 + 5), dir[1] * (crand() * 3 + 5), dir[2] * (crand() * 3 + 5),
		    0, 0, -100,
		    220, 140, 50,
		    0, 0, 0,
		    1.0, -1.0,
		    GL_SRC_ALPHA, GL_ONE,
		    10, 0,
		    particle_blood,
		    PART_SHADED,
		    pRotateThink, true);
	}

}

void
CL_BloodHit(vec3_t org, vec3_t dir)
{
	if (cl_blood->value == 1)
		CL_BloodSmack(org, dir);
	if (cl_blood->value == 2 || cl_blood->value == 3) {
		vec3_t		move;
		int		i         , j;

		if (cl_blood->value == 2)
			j = 6;
		else
			j = 16;
		VectorScale(dir, 10, move);
		for (i = 0; i < j; i++) {
			VectorSet(move,
			    dir[0] + random() * (cl_blood->value - 1) * 0.01,
			    dir[1] + random() * (cl_blood->value - 1) * 0.01,
			    dir[2] + random() * (cl_blood->value - 1) * 0.01);
			VectorScale(move, 10 + (cl_blood->value - 1) * 0.0001 * random(), move);
			CL_BloodBleed(org, move, dir);
		}
	}
}
#endif

/*
 * =============== CL_ParticleEffect
 *
 * Wall impact puffs ===============
 */
#ifdef QMAX
void
CL_ParticleEffect(vec3_t org, vec3_t dir, int color8, int count)
{
	int		i;
	float		d, size;
	vec3_t		color = {color8red(color8), color8green(color8), color8blue(color8)};
	particle_type particleType;
	
	if (cl_particles_type->value == 2) {
		size = 5.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 1.5;	
		particleType = particle_bubble;
	}
	else {
		size = 1.0;	
		particleType = particle_generic;
	}

	for (i = 0; i < count; i++) {
		d = rand() & 31;
		setupParticle(
		    0, 0, 0,
		    org[0] + ((rand() & 7) - 4) + d * dir[0], org[1] + ((rand() & 7) - 4) + d * dir[1], org[2] + ((rand() & 7) - 4) + d * dir[2],
		    crand() * 20, crand() * 20, crand() * 20,
		    0, 0, 0,
		    color[0], color[1], color[2],
		    0, 0, 0,
		    1.0, -1.0 / (0.5 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    size, 0,
		    particleType,
		    PART_GRAVITY,
		    NULL, 0);
	}
}
#else
void
CL_ParticleEffect(vec3_t org, vec3_t dir, int color, int count)
{
	int		i, j;
	cparticle_t    *p;
	float		d;

	for (i = 0; i < count; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		p->color = color + (rand() & 7);

		d = rand() & 31;
		for (j = 0; j < 3; j++) {
			p->org[j] = org[j] + ((rand() & 7) - 4) + d * dir[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand() * 0.3);
	}
}
#endif

/*
 * =============== CL_ParticleEffect2 ===============
 */
#ifdef QMAX
#define colorAdd 25
void
CL_ParticleEffect2(vec3_t org, vec3_t dir, int color8, int count)
{
	int		i;
	float		d, size;
	vec3_t		color = {color8red(color8), color8green(color8), color8blue(color8)};
	particle_type particleType;
	
	if (cl_particles_type->value == 2) {
		size = 5.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 1.0;	
		particleType = particle_bubble;
	}
	else {
		size = 1.0;	
		particleType = particle_generic;
	}

	for (i = 0; i < count; i++) {
		d = rand() & 7;
		setupParticle(
		    0, 0, 0,
		    org[0] + ((rand() & 7) - 4) + d * dir[0], org[1] + ((rand() & 7) - 4) + d * dir[1], org[2] + ((rand() & 7) - 4) + d * dir[2],
		    crand() * 20, crand() * 20, crand() * 20,
		    0, 0, 0,
		    color[0] + colorAdd, color[1] + colorAdd, color[2] + colorAdd,
		    0, 0, 0,
		    1, -1.0 / (0.5 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    size, 0,
		    particleType,
		    PART_GRAVITY,
		    NULL, 0);
	}
}
#else
void
CL_ParticleEffect2(vec3_t org, vec3_t dir, int color, int count)
{
	int		i, j;
	cparticle_t    *p;
	float		d;

	for (i = 0; i < count; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		p->color = color;

		d = rand() & 7;
		for (j = 0; j < 3; j++) {
			p->org[j] = org[j] + ((rand() & 7) - 4) + d * dir[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand() * 0.3);
	}
}
#endif

/* RAFAEL */

/*
 * =============== CL_ParticleEffect3 ===============
 */
#ifdef QMAX
void
CL_ParticleEffect3(vec3_t org, vec3_t dir, int color8, int count)
{
	int		i;
	float		d, size;
	vec3_t		color = {color8red(color8), color8green(color8), color8blue(color8)};
	particle_type particleType;
	
	if (cl_particles_type->value == 2) {
		size = 5.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 1.5;	
		particleType = particle_bubble;
	}
	else {
		size = 1.0;	
		particleType = particle_generic;
	}

	for (i = 0; i < count; i++) {
		d = rand() & 7;
		setupParticle(
		    0, 0, 0,
		    org[0] + ((rand() & 7) - 4) + d * dir[0], org[1] + ((rand() & 7) - 4) + d * dir[1], org[2] + ((rand() & 7) - 4) + d * dir[2],
		    crand() * 20, crand() * 20, crand() * 20,
		    0, 0, 0,
		    color[0] + colorAdd, color[1] + colorAdd, color[2] + colorAdd,
		    0, 0, 0,
		    1, -1.0 / (0.5 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    size, 0,
		    particleType,
		    PART_GRAVITY,
		    NULL, false);
	}
}
#else
void
CL_ParticleEffect3(vec3_t org, vec3_t dir, int color, int count)
{
	int		i, j;
	cparticle_t    *p;
	float		d;

	for (i = 0; i < count; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		p->color = color;

		d = rand() & 7;
		for (j = 0; j < 3; j++) {
			p->org[j] = org[j] + ((rand() & 7) - 4) + d * dir[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand() * 0.3);
	}
}
#endif

#ifdef QMAX
/*
 * =============== CL_ParticleEffectSparks ===============
 */
void
pSparksThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	int		i;

	/* setting up angle for sparks */
	{
		float		time1   , time2;

		time1 = *time;
		time2 = time1 * time1;

		for (i = 0; i < 2; i++)
			angle[i] = 0.75 * (p->vel[i] * time1 + (p->accel[i]) * time2);
		angle[2] = 0.75 * (p->vel[2] * time1 + (p->accel[2] - PARTICLE_GRAVITY) * time2);
	}

	p->thinknext = true;
}

cparticle_t    *p;
void
CL_ParticleEffectSparks(vec3_t org, vec3_t dir, vec3_t color, int count)
{
	int		i;
	float		d;

	for (i = 0; i < count; i++) {
		d = rand() & 7;
		setupParticle(
		    0, 0, 0,
		    org[0] + ((rand() & 3) - 2), org[1] + ((rand() & 3) - 2), org[2] + ((rand() & 3) - 2),
		    crand() * 40 + dir[0] * 100, crand() * 40 + dir[1] * 100, crand() * 40 + dir[2] * 100,
		    0, 0, 0,
		    color[0], color[1], color[2],
		    0, 0, 0,
		    0.9, -2.5 / (0.5 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    0.5, 0,
		    particle_generic,
		    PART_GRAVITY | PART_DIRECTION | PART_SPARK,
		    pSparksThink, true);
	}
	/* if (p) // added light effect */

	/*
	 * addParticleLight (p, (count>8)?130:65, 0, color[0]/255,
	 * color[1]/255, color[2]/255);
	 */
}

/*
 * =============== CL_ParticleFootPrint ===============
 */

void
CL_ParticleFootPrint(vec3_t org, vec3_t angle, float size, vec3_t color)
{
	float		alpha = DIV254BY255;

	setupParticle(
	    angle[0], angle[1], angle[2],
	    org[0], org[1], org[2],
	    0, 0, 0,
	    0, 0, 0,
	    color[0], color[1], color[2],
	    0, 0, 0,
	    alpha, -.002,
	    GL_ZERO, GL_ONE_MINUS_SRC_ALPHA,
	    size, 0,
	    particle_footprint,
	    PART_TRANS | PART_ANGLED | PART_SHADED,
	    NULL, false);
}
#endif

/*
 * =============== CL_TeleporterParticles ===============
 */
#ifdef QMAX
void
CL_TeleporterParticles(entity_state_t * ent)
{
	int		i;
	particle_type particleType;
	float 	size;
	
	if (cl_particles_type->value == 2) {
		size = 5.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 2.0;	
		particleType = particle_bubble;
	}
	else {
		size = 1.0;	
		particleType = particle_generic;
	}

	for (i = 0; i < 8; i++) {
		setupParticle(
		    0, 0, 0,
		    ent->origin[0] - 16 + (rand() & 31), ent->origin[1] - 16 + (rand() & 31), ent->origin[2] - 16 + (rand() & 31),
		    crand() * 14, crand() * 14, crand() * 14,
		    0, 0, 0,
		    200 + rand() * 50, 200 + rand() * 50, 200 + rand() * 50,
		    0, 0, 0,
		    1, -0.5,
		    GL_SRC_ALPHA, GL_ONE,
		    size, 0,
		    particleType,
		    PART_GRAVITY,
		    NULL, 0);
	}
}
#else
void
CL_TeleporterParticles(entity_state_t * ent)
{
	int		i, j;
	cparticle_t    *p;

	for (i = 0; i < 8; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		p->color = 0xdb;

		for (j = 0; j < 2; j++) {
			p->org[j] = ent->origin[j] - 16 + (rand() & 31);
			p->vel[j] = crand() * 14;
		}

		p->org[2] = ent->origin[2] - 8 + (rand() & 7);
		p->vel[2] = 80 + (rand() & 7);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.5;
	}
}
#endif

/*
 * =============== CL_LogoutEffect
 *
 * ===============
 */
#ifdef QMAX
void
CL_LogoutEffect(vec3_t org, int type)
{
	int		i;
	vec3_t		color;
	particle_type particleType;
	float 	size;
	
	if (cl_particles_type->value == 2) {
		size = 3.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 1.5;	
		particleType = particle_bubble;
	}
	else {
		size = 1.0;	
		particleType = particle_generic;
	}

	for (i = 0; i < 500; i++) {
		if (type == MZ_LOGIN) {	/* green */
			color[0] = 20;
			color[1] = 200;
			color[2] = 20;
		} else if (type == MZ_LOGOUT) {	/* red */
			color[0] = 200;
			color[1] = 20;
			color[2] = 20;
		} else {	/* yellow */
			color[0] = 200;
			color[1] = 200;
			color[2] = 20;
		}

		setupParticle(
		    0, 0, 0,
		    org[0] - 16 + frand() * 32, org[1] - 16 + frand() * 32, org[2] - 24 + frand() * 56,
		    crand() * 20, crand() * 20, crand() * 20,
		    0, 0, 0,
		    color[0], color[1], color[2],
		    0, 0, 0,
		    1, -1.0 / (1.0 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    size, 0,
		    particleType,
		    PART_GRAVITY,
		    NULL, 0);
	}
}
#else
void
CL_LogoutEffect(vec3_t org, int type)
{
	int		i, j;
	cparticle_t    *p;

	for (i = 0; i < 500; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;

		if (type == MZ_LOGIN)
			p->color = 0xd0 + (rand() & 7);	/* green */
		else if (type == MZ_LOGOUT)
			p->color = 0x40 + (rand() & 7);	/* red */
		else
			p->color = 0xe0 + (rand() & 7);	/* yellow */


		p->org[0] = org[0] - 16 + frand() * 32;
		p->org[1] = org[1] - 16 + frand() * 32;
		p->org[2] = org[2] - 24 + frand() * 56;

		for (j = 0; j < 3; j++)
			p->vel[j] = crand() * 20;

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (1.0 + frand() * 0.3);
	}
}
#endif

/*
 * =============== CL_ItemRespawnParticles
 *
 * ===============
 */
#ifdef QMAX
void
CL_ItemRespawnParticles(vec3_t org)
{
	int		i;
	particle_type particleType;
	float 	size;
	
	if (cl_particles_type->value == 2) {
		size = 3.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 1.4;	
		particleType = particle_bubble;
	}
	else {
		size = 1.0;	
		particleType = particle_generic;
	}

	for (i = 0; i < 64; i++) {
		setupParticle(
		    0, 0, 0,
		    org[0] + crand() * 8, org[1] + crand() * 8, org[2] + crand() * 8,
		    crand() * 8, crand() * 8, crand() * 8,
		    0, 0, PARTICLE_GRAVITY * 0.2,
		    0, 150 + rand() * 25, 0,
		    0, 0, 0,
		    1, -1.0 / (1.0 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    size, 0,
		    particleType,
		    PART_GRAVITY,
		    NULL, 0);
	}
}
#else
void
CL_ItemRespawnParticles(vec3_t org)
{
	int		i, j;
	cparticle_t    *p;

	for (i = 0; i < 64; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		p->color = 0xd4 + (rand() & 3);	/* green */

		p->org[0] = org[0] + crand() * 8;
		p->org[1] = org[1] + crand() * 8;
		p->org[2] = org[2] + crand() * 8;

		for (j = 0; j < 3; j++)
			p->vel[j] = crand() * 8;

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY * 0.2;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (1.0 + frand() * 0.3);
	}
}
#endif

/*
 * =============== CL_ExplosionParticles ===============
 */
#ifdef QMAX
void
pExplosionSparksThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	int		i;

	/* setting up angle for sparks */
	{
		float		time1   , time2;

		time1 = *time;
		time2 = time1 * time1;

		for (i = 0; i < 2; i++)
			angle[i] = 0.25 * (p->vel[i] * time1 + (p->accel[i]) * time2);
		angle[2] = 0.25 * (p->vel[2] * time1 + (p->accel[2] - PARTICLE_GRAVITY) * time2);

	}

	p->thinknext = true;
}

void
CL_Explosion_Sparks(vec3_t org, int size)
{
	int		i;

	for (i = 0; i < 128; i++) {	/* was 256 */
		setupParticle(
		    0, 0, 0,
		    org[0] + ((rand() % size) - 16), org[1] + ((rand() % size) - 16), org[2] + ((rand() % size) - 16),
		    (rand() % 150) - 75, (rand() % 150) - 75, (rand() % 150) - 75,
		    0, 0, 0,
		    255, 100, 25,
		    0, 0, 0,
		    1, -0.8 / (0.5 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    4, -6,	/* was 2, -3 */
		    particle_blaster,
		    PART_GRAVITY | PART_SPARK,
		    pExplosionSparksThink, true);
	}
}

/*
 * =============== CL_ExplosionParticles ===============
 */
void
CL_ExplosionParticles_Old(vec3_t org)
{
	int		i, j;
	cparticle_t    *p;

	for (i = 0; i < 256; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		p->color[0] = 255;
		p->color[1] = 0;
		p->color[2] = rand() & 7;
		for (j = 0; j < 3; j++) {
			p->org[j] = org[j] + ((rand() % 32) - 16);
			p->vel[j] = (rand() % 384) - 192;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.8 / (0.5 + frand() * 0.3);
	}
}
#if 0
void
CL_ExplosionParticles(vec3_t org, int size)
{
	int		i;

	for (i = 0; i < 256; i++) {
		setupParticle(
		    0, 0, 0,
		    org[0] + ((rand() % size) - 16), org[1] + ((rand() % size) - 16), org[2] + ((rand() % size) - 16),
		    (rand() % 150) - 75, (rand() % 150) - 75, (rand() % 150) - 75,
		    0, 0, 0,
		    255, 100, 25,
		    0, 0, 0,
		    1, -0.8 / (0.5 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    2, -3,
		    particle_blaster,
		    PART_GRAVITY | PART_SPARK,
		    pExplosionSparksThink, true);
	}
}
#else
void 
CL_ExplosionParticles (vec3_t org, float scale, int water)
{
	vec3_t vel;
	int		i, flags, img, blend[2], colorvel[3], amount;
	float size, sizevel, speed, alphavel, veladd;
	void *think;

	if (water) {
		img = particle_bubble;
		flags = PART_SHADED;
		size = (rand()%2 + 2)*0.333;
		sizevel = 3;
		blend[0] = GL_SRC_ALPHA;
		blend[1] = GL_ONE_MINUS_SRC_ALPHA;
		colorvel[0] = colorvel[1] = colorvel[2] = 0;
		alphavel = -0.1 / (0.5 + frand()*0.3);
		speed = 25;

		veladd = 10;
		amount = 4;

		think = pBubbleThink;
	}
	else {
		img = particle_blaster;
		flags = PART_DIRECTION|PART_GRAVITY;
		size = scale*0.75;
		sizevel = -scale*0.5;
		blend[0] = GL_SRC_ALPHA;
		blend[1] = GL_ONE;
		colorvel[0] = 0;
		colorvel[1] = -200;
		colorvel[2] = -400;
		alphavel = -1.0 / (0.5 + frand()*0.3);
		speed = 250;

		veladd = 0;
		amount = 2;

		think = pExplosionSparksThink;
	}

	for (i=0 ; i<256 ; i++) {
		if (i%amount!=0)
			continue;

		VectorSet(vel, crandom(), crandom(), crandom());
		VectorNormalize(vel);
		VectorScale(vel, scale*speed, vel);

		setupParticle (
			crand()*360, 0, 0,
			org[0] + ((rand()%32)-16), org[1] + ((rand()%32)-16), org[2] + ((rand()%32)-16),
			vel[0], vel[1], vel[2] + veladd,
			0, 0, 0,
			255, 255, 255,
			colorvel[0], colorvel[1], colorvel[2],
			1, alphavel,
			blend[0], blend[1],
			size, sizevel,	
			img,
			flags,
			think, true);
	}
}
#endif
#else
void
CL_ExplosionParticles(vec3_t org)
{
	int		i, j;
	cparticle_t    *p;

	for (i = 0; i < 256; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		p->color = 0xe0 + (rand() & 7);
		for (j = 0; j < 3; j++) {
			p->org[j] = org[j] + ((rand() % 32) - 16);
			p->vel[j] = (rand() % 384) - 192;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.8 / (0.5 + frand() * 0.3);
	}
}
#endif

/*
 * =============== CL_BigTeleportParticles ===============
 */
#ifdef QMAX
void
CL_BigTeleportParticles(vec3_t org)
{
	int		i, index;
	float		angle, dist, size;
	static int	colortable0[4] = {10, 50, 150, 50};
	static int	colortable1[4] = {150, 150, 50, 10};
	static int	colortable2[4] = {50, 10, 10, 150};
	particle_type particleType;
	
	if (cl_particles_type->value == 2) {
		size = 3.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 1.5;	
		particleType = particle_bubble;
	}
	else {
		size = 1.0;	
		particleType = particle_generic;
	}

	for (i = 0; i < 4096; i++) {

		index = rand() & 3;
		angle = M_PI * 2 * (rand() & 1023) / 1023.0;
		dist = rand() & 31;
		setupParticle(
		    0, 0, 0,
		    org[0] + cos(angle) * dist, org[1] + sin(angle) * dist, org[2] + 8 + (rand() % 90),
		    cos(angle) * (70 + (rand() & 63)), sin(angle) * (70 + (rand() & 63)), -100 + (rand() & 31),
		    -cos(angle) * 100, -sin(angle) * 100, PARTICLE_GRAVITY * 4,
		    colortable0[index], colortable1[index], colortable2[index],
		    0, 0, 0,
		    1, -0.3 / (0.5 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    size, 0.3 / (0.5 + frand() * 0.3),
		    particleType,
		    0,
		    NULL, 0);
	}
}
#else
void
CL_BigTeleportParticles(vec3_t org)
{
	int		i;
	cparticle_t    *p;
	float		angle, dist;
	static int	colortable[4] = {2 * 8, 13 * 8, 21 * 8, 18 * 8};

	for (i = 0; i < 4096; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;

		p->color = colortable[rand() & 3];

		angle = M_PI * 2 * (rand() & 1023) / 1023.0;
		dist = rand() & 31;
		p->org[0] = org[0] + cos(angle) * dist;
		p->vel[0] = cos(angle) * (70 + (rand() & 63));
		p->accel[0] = -cos(angle) * 100;

		p->org[1] = org[1] + sin(angle) * dist;
		p->vel[1] = sin(angle) * (70 + (rand() & 63));
		p->accel[1] = -sin(angle) * 100;

		p->org[2] = org[2] + 8 + (rand() % 90);
		p->vel[2] = -100 + (rand() & 31);
		p->accel[2] = PARTICLE_GRAVITY * 4;
		p->alpha = 1.0;

		p->alphavel = -0.3 / (0.5 + frand() * 0.3);
	}
}
#endif

/*
 * =============== CL_BlasterParticles
 *
 * Wall impact puffs ===============
 */
#ifdef QMAX
#define pBlasterMaxSize 5
void
pBlasterThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	vec3_t		len;

	VectorSubtract(p->angle, org, len);

	*size *= (float)(pBlasterMaxSize / VectorLength(len)) * 1.0 / ((4 - *size));
	if (*size > pBlasterMaxSize)
		*size = pBlasterMaxSize;

	p->thinknext = true;
}

void
CL_BlasterParticles(vec3_t org, vec3_t dir, int count)
{
	int		i;
	float		d;
	float		speed = .75;

	for (i = 0; i < count; i++) {
		d = rand() & 5;
		setupParticle(
		    org[0], org[1], org[2],
		    org[0] + ((rand() & 5) - 2) + d * dir[0], org[1] + ((rand() & 5) - 2) + d * dir[1], org[2] + ((rand() & 5) - 2) + d * dir[2],
		    (dir[0] * 75 + crand() * 20) * speed, (dir[1] * 75 + crand() * 20) * speed, (dir[2] * 75 + crand() * 20) * speed,
		    0, 0, 0,
		    255, 150, 50,
		    0, -90, -30,
		    1, -1 / (0.5 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    3.5, -0.75,
		    particle_generic,
		    PART_GRAVITY,
		    pBlasterThink, true);
	}
}

/*
 * =============== CL_BlasterParticlesColor
 *
 * Wall impact puffs ===============
 */

void
CL_BlasterParticlesColor(vec3_t org, vec3_t dir, int count, int red, int green, int blue,
    int reddelta, int greendelta, int bluedelta)
{
	int		i;
	float		speed = .75;
	cparticle_t    *p;
	vec3_t		origin;

	for (i = 0; i < count; i++) {
		VectorSet(origin,
		    org[0] + dir[0] * (1 + random() * 3 + pBlasterMaxSize / 2.0),
		    org[1] + dir[1] * (1 + random() * 3 + pBlasterMaxSize / 2.0),
		    org[2] + dir[2] * (1 + random() * 3 + pBlasterMaxSize / 2.0)
		    );

		if (cl_blaster_type->value == 1 ||
		    ((cl_blaster_type->value == 2) &&
		    (cl.refdef.rdflags & RDF_UNDERWATER)))
			p = setupParticle(
			    org[0], org[1], org[2],
			    origin[0], origin[1], origin[2],
			    (dir[0] * 75 + crand() * 40) * speed, (dir[1] * 75 + crand() * 40) * speed, (dir[2] * 75 + crand() * 40) * speed,
			    0, 0, 0,
			    red, green, blue,
			    reddelta, greendelta, bluedelta,
			    1, -0.5 / (0.5 + frand() * 0.3),
			    GL_SRC_ALPHA, GL_ONE,
			    3.5, -0.75,
			    particle_bubble,
			    PART_GRAVITY,
			    pBlasterThink, true);
		else
			p = setupParticle(
			    org[0], org[1], org[2],
			    origin[0], origin[1], origin[2],
			    (dir[0] * 75 + crand() * 40) * speed, (dir[1] * 75 + crand() * 40) * speed, (dir[2] * 75 + crand() * 40) * speed,
			    0, 0, 0,
			    red, green, blue,
			    reddelta, greendelta, bluedelta,
			    1, -0.5 / (0.5 + frand() * 0.3),
			    GL_SRC_ALPHA, GL_ONE,
			    3.5, -0.75,
			    particle_generic,
			    PART_GRAVITY,
			    pBlasterThink, true);
	}
}
#else
void
CL_BlasterParticles(vec3_t org, vec3_t dir)
{
	int		i, j;
	cparticle_t    *p;
	float		d;
	int		count;

	count = 40;
	for (i = 0; i < count; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		p->color = 0xe0 + (rand() & 7);
		d = rand() & 15;
		for (j = 0; j < 3; j++) {
			p->org[j] = org[j] + ((rand() & 7) - 4) + d * dir[j];
			p->vel[j] = dir[j] * 30 + crand() * 40;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand() * 0.3);
	}
}
#endif

/*
 * =============== CL_BlasterTrail
 *
 * ===============
 */
#ifdef QMAX
void
CL_BlasterTrail(vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		dec;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 4;
	VectorScale(vec, dec, vec);

	/* FIXME: this is a really silly way to have a loop */
	while (len > 0) {
		len -= dec;
		if (cl_blaster_type->value == 1 ||
		    ((cl_blaster_type->value == 2) &&
		    (cl.refdef.rdflags & RDF_UNDERWATER)))
			setupParticle(
			    0, 0, 0,
			    move[0] + crand(), move[1] + crand(), move[2] + crand(),
			    crand() * 5, crand() * 5, crand() * 5,
			    0, 0, 0,
			    255, 150, 50,
			    0, -90, -30,
			    1, -1.0 / (0.5 + frand() * 0.3),
			    GL_SRC_ALPHA, GL_ONE,
			    4, -6,
			    particle_bubble,
			    0,
			    NULL, 0);
		else
			setupParticle(
			    0, 0, 0,
			    move[0] + crand(), move[1] + crand(), move[2] + crand(),
			    crand() * 5, crand() * 5, crand() * 5,
			    0, 0, 0,
			    255, 150, 50,
			    0, -90, -30,
			    1, -1.0 / (0.5 + frand() * 0.3),
			    GL_SRC_ALPHA, GL_ONE,
			    4, -6,
			    particle_generic,
			    0,
			    NULL, 0);
		VectorAdd(move, vec, move);
	}
}


/*
 * =============== CL_BlasterTrailColor
 *
 * Hyperblaster particle glow effect ===============
 */
void
CL_BlasterTrailColor(vec3_t start, vec3_t end, int red, int green, int blue,
    int reddelta, int greendelta, int bluedelta)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		dec;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 4;
	VectorScale(vec, dec, vec);

	/* FIXME: this is a really silly way to have a loop */
	while (len > 0) {
		len -= dec;

		if (cl_blaster_type->value == 1 ||
		    ((cl_blaster_type->value == 2) &&
		    (cl.refdef.rdflags & RDF_UNDERWATER)))
			setupParticle(
			    0, 0, 0,
			    move[0] + crand(), move[1] + crand(), move[2] + crand(),
			    crand() * 5, crand() * 5, crand() * 5,
			    0, 0, 0,
			    red, green, blue,
			    reddelta, greendelta, bluedelta,
			    1, -1.0 / (0.5 + frand() * 0.3),
			    GL_SRC_ALPHA, GL_ONE,
			    4, -6,
			    particle_bubble,
			    0,
			    NULL, 0);
		else
			setupParticle(
			    0, 0, 0,
			    move[0] + crand(), move[1] + crand(), move[2] + crand(),
			    crand() * 5, crand() * 5, crand() * 5,
			    0, 0, 0,
			    red, green, blue,
			    reddelta, greendelta, bluedelta,
			    1, -1.0 / (0.5 + frand() * 0.3),
			    GL_SRC_ALPHA, GL_ONE,
			    4, -6,
			    particle_generic,
			    0,
			    NULL, 0);
		VectorAdd(move, vec, move);
	}
}

/*
 * =============== CL_HyperBlasterTrail
 *
 * Hyperblaster particle glow effect ===============
 */
void
CL_HyperBlasterTrail(vec3_t start, vec3_t end, int red, int green, int blue,
    int reddelta, int greendelta, int bluedelta)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		dec;
	int		i;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	VectorMA(move, 0.5, vec, move);
	len = VectorNormalize(vec);

	dec = 1;
	VectorScale(vec, dec, vec);

	for (i = 0; i < 18; i++) {
		len -= dec;

		if (cl_hyperblaster_particles_type->value == 1 ||
		    ((cl_hyperblaster_particles_type->value == 2) &&
		    (cl.refdef.rdflags & RDF_UNDERWATER)))
			setupParticle(
			    0, 0, 0,
			    move[0] + crand(), move[1] + crand(), move[2] + crand(),
			    crand() * 5, crand() * 5, crand() * 5,
			    0, 0, 0,
			    red, green, blue,
			    reddelta, greendelta, bluedelta,
			    1, -16.0 / (0.5 + frand() * 0.3),
			    GL_SRC_ALPHA, GL_ONE,
			    3, -36,
			    particle_bubble,
			    0,
			    NULL, 0);
		else
			setupParticle(
			    0, 0, 0,
			    move[0] + crand(), move[1] + crand(), move[2] + crand(),
			    crand() * 5, crand() * 5, crand() * 5,
			    0, 0, 0,
			    red, green, blue,
			    reddelta, greendelta, bluedelta,
			    1, -16.0 / (0.5 + frand() * 0.3),
			    GL_SRC_ALPHA, GL_ONE,
			    3, -36,
			    particle_generic,
			    0,
			    NULL, 0);

		VectorAdd(move, vec, move);
	}
}

/*
 * =============== CL_BlasterTracer ===============
 */
void
CL_BlasterTracer(vec3_t origin, vec3_t angle, int red, int green, int blue, float len, float size)
{
	int		i;
	vec3_t		dir;

	AngleVectors(angle, dir, NULL, NULL);
	VectorScale(dir, len, dir);
	if (cl_hyperblaster_particles_type->value == 1 ||
	    ((cl_hyperblaster_particles_type->value == 2) &&
	    (cl.refdef.rdflags & RDF_UNDERWATER)))
		for (i = 0; i < 3; i++)
			setupParticle(
			    dir[0], dir[1], dir[2],
			    origin[0], origin[1], origin[2],
			    0, 0, 0,
			    0, 0, 0,
			    red, green, blue,
			    0, 0, 0,
			    1, INSTANT_PARTICLE,
			    GL_SRC_ALPHA, GL_ONE,
			    size, 0,
			    particle_bubble,
			    PART_DIRECTION,
			    NULL, 0);
	else
		for (i = 0; i < 3; i++)
			setupParticle(
			    dir[0], dir[1], dir[2],
			    origin[0], origin[1], origin[2],
			    0, 0, 0,
			    0, 0, 0,
			    red, green, blue,
			    0, 0, 0,
			    1, INSTANT_PARTICLE,
			    GL_SRC_ALPHA, GL_ONE,
			    size, 0,
			    particle_generic,
			    PART_DIRECTION,
			    NULL, 0);
}

void
CL_HyperBlasterEffect(vec3_t start, vec3_t end, vec3_t angle, int red, int green, int blue,
    int reddelta, int greendelta, int bluedelta, float len, float size)
{
	if (cl_hyperblaster_particles->value)
		CL_HyperBlasterTrail(start, end, red, green, blue, reddelta, greendelta, bluedelta);
	else
		CL_BlasterTracer(end, angle, red, green, blue, len, size);
}
#else
void
CL_BlasterTrail(vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		j;
	cparticle_t    *p;
	int		dec;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 5;
	VectorScale(vec, 5, vec);

	/* FIXME: this is a really silly way to have a loop */
	while (len > 0) {
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = cl.time;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.3 + frand() * 0.2);
		p->color = 0xe0;
		for (j = 0; j < 3; j++) {
			p->org[j] = move[j] + crand();
			p->vel[j] = crand() * 5;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}
#endif

/*
 * =============== CL_QuadTrail
 *
 * ===============
 */
#ifdef QMAX
void
CL_QuadTrail(vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		dec;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0) {
		len -= dec;

		setupParticle(
		    0, 0, 0,
		    move[0] + crand() * 16, move[1] + crand() * 16, move[2] + crand() * 16,
		    crand() * 5, crand() * 5, crand() * 5,
		    0, 0, 0,
		    0, 0, 200,
		    0, 0, 0,
		    1, -1.0 / (0.8 + frand() * 0.2),
		    GL_SRC_ALPHA, GL_ONE,
		    1, 0,
		    particle_generic,
		    0,
		    NULL, 0);

		VectorAdd(move, vec, move);
	}
}
#else
void
CL_QuadTrail(vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		j;
	cparticle_t    *p;
	int		dec;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0) {
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = cl.time;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.8 + frand() * 0.2);
		p->color = 115;
		for (j = 0; j < 3; j++) {
			p->org[j] = move[j] + crand() * 16;
			p->vel[j] = crand() * 5;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}
#endif

/*
 * =============== CL_FlagTrail
 *
 * ===============
 */
#ifdef QMAX
void
CL_FlagTrail(vec3_t start, vec3_t end, qboolean isred, qboolean isgreen)
{
	vec3_t		move;
	vec3_t		vec;
	float		len, size;
	int		dec;
	particle_type particleType;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	
	if (cl_particles_type->value == 2) {
		size = 3.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 2.0;	
		particleType = particle_bubble;
	}
	else {
		size = 1.0;	
		particleType = particle_generic;
	}
	
	dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0) {
		len -= dec;

		setupParticle(
		    0, 0, 0,
		    move[0] + crand() * 16, move[1] + crand() * 16, move[2] + crand() * 16,
		    crand() * 5, crand() * 5, crand() * 5,
		    0, 0, 0,
		    (isred) ? 255 : 0, (isgreen) ? 255 : 0, (!isred && !isgreen) ? 255 : 0,
		    0, 0, 0,
		    1.5, -1.0 / (0.8 + frand() * 0.2),
		    GL_SRC_ALPHA, GL_ONE,
		    size, 0,
		    particleType,
		    0,
		    NULL, 0);

		VectorAdd(move, vec, move);
	}
}
#else
void
CL_FlagTrail(vec3_t start, vec3_t end, float color)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		j;
	cparticle_t    *p;
	int		dec;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0) {
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = cl.time;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.8 + frand() * 0.2);
		p->color = color;
		for (j = 0; j < 3; j++) {
			p->org[j] = move[j] + crand() * 16;
			p->vel[j] = crand() * 5;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}
#endif

/*
 * =============== CL_DiminishingTrail
 *
 * ===============
 */
#ifdef QMAX
void
CL_DiminishingTrail(vec3_t start, vec3_t end, centity_t * old, int flags)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	float		dec;
	float		orgscale;
	float		velscale;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = (flags & EF_ROCKET) ? 10 : 2;
	VectorScale(vec, dec, vec);

	if (old->trailcount > 900) {
		orgscale = 4;
		velscale = 15;
	} else if (old->trailcount > 800) {
		orgscale = 2;
		velscale = 10;
	} else {
		orgscale = 1;
		velscale = 5;
	}

	while (len > 0) {
		len -= dec;

		if (!free_particles)
			return;

		if (flags & EF_ROCKET) {
			if (cl_explosion->value && CM_PointContents(move, 0) & MASK_WATER) {
				setupParticle(
				    0, 0, crand() * 360,
				    move[0], move[1], move[2],
				    crand() * 9, crand() * 9, crand() * 9 + 5,
				    0, 0, 0,
				    255, 255, 255,
				    0, 0, 0,
				    0.75, -0.2 / (1 + frand() * 0.2),
				    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
				    1 + random() * 3, 1,
				    particle_bubble,
				    PART_SHADED,
				    pExplosionBubbleThink, true);
			} else {
				setupParticle(
				    crand() * 180, crand() * 100, 0,
				    move[0], move[1], move[2],
				    crand() * 5, crand() * 5, crand() * 5,
				    0, 0, 5,
				    150, 150, 150,
				    100, 100, 100,
				    0.5, -0.25,
				    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
				    1, 10,
				    particle_smoke,
				    PART_TRANS | PART_SHADED,
				    pRotateThink, true);
			}
		} else {
			/* drop less particles as it flies */
			if ((rand() & 1023) < old->trailcount) {
				if (flags & EF_GIB) {
					setupParticle(
					    0, 0, random() * 360,
					    move[0] + crand() * orgscale, move[1] + crand() * orgscale, move[2] + crand() * orgscale,
					    crand() * velscale, crand() * velscale, crand() * velscale,
					    0, 0, 0,
					    255, 255, 255,
					    0, 0, 0,
					    0.75, -0.75 / (1 + frand() * 0.4),
					    GL_SRC_ALPHA, GL_ONE,
					    1 + 3 * frand(), -1,
					    particle_redblood,
					    PART_TRANS | PART_GRAVITY | PART_SHADED,
					    NULL, 0);
				} else if (flags & EF_GREENGIB) {
					setupParticle(
					    0, 0, 0,
					    move[0] + crand() * orgscale, move[1] + crand() * orgscale, move[2] + crand() * orgscale,
					    crand() * velscale, crand() * velscale, crand() * velscale,
					    0, 0, 0,
					    0, 255, 0,
					    0, 0, 0,
					    0, -1.0 / (1 + frand() * 0.4),
					    GL_SRC_ALPHA, GL_ONE,
					    5, -1,
					    particle_redblood,
					    PART_GRAVITY | PART_SHADED,
					    NULL, 0);
				} else if (flags & EF_GRENADE) {	/* no overbrights on
									 * grenade trails */
					setupParticle(
					    crand() * 180, crand() * 50, 0,
					    move[0] + crand() * orgscale, move[1] + crand() * orgscale, move[2] + crand() * orgscale,
					    crand() * velscale, crand() * velscale, crand() * velscale,
					    0, 0, 20,
					    255, 255, 255,
					    0, 0, 0,
					    0.5, -0.5,
					    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
					    5, 5,
					    particle_smoke,
					    PART_TRANS | PART_SHADED,
					    pRotateThink, true);
				} else {
					setupParticle(
					    crand() * 180, crand() * 50, 0,
					    move[0] + crand() * orgscale, move[1] + crand() * orgscale, move[2] + crand() * orgscale,
					    crand() * velscale, crand() * velscale, crand() * velscale,
					    0, 0, 20,
					    255, 255, 255,
					    0, 0, 0,
					    0.5, -0.5,
					    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
					    5, 5,
					    particle_smoke,
					    PART_TRANS | PART_SHADED,
					    pRotateThink, true);
				}
			}
			old->trailcount -= 5;
			if (old->trailcount < 100)
				old->trailcount = 100;
		}

		VectorAdd(move, vec, move);
	}
}
#else
void
CL_DiminishingTrail(vec3_t start, vec3_t end, centity_t * old, int flags)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		j;
	cparticle_t    *p;
	float		dec;
	float		orgscale;
	float		velscale;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 0.5;
	VectorScale(vec, dec, vec);

	if (old->trailcount > 900) {
		orgscale = 4;
		velscale = 15;
	} else if (old->trailcount > 800) {
		orgscale = 2;
		velscale = 10;
	} else {
		orgscale = 1;
		velscale = 5;
	}

	while (len > 0) {
		len -= dec;

		if (!free_particles)
			return;

		/* drop less particles as it flies */
		if ((rand() & 1023) < old->trailcount) {
			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;
			VectorClear(p->accel);

			p->time = cl.time;

			if (flags & EF_GIB) {
				p->alpha = 1.0;
				p->alphavel = -1.0 / (1 + frand() * 0.4);
				p->color = 0xe8 + (rand() & 7);
				for (j = 0; j < 3; j++) {
					p->org[j] = move[j] + crand() * orgscale;
					p->vel[j] = crand() * velscale;
					p->accel[j] = 0;
				}
				p->vel[2] -= PARTICLE_GRAVITY;
			} else if (flags & EF_GREENGIB) {
				p->alpha = 1.0;
				p->alphavel = -1.0 / (1 + frand() * 0.4);
				p->color = 0xdb + (rand() & 7);
				for (j = 0; j < 3; j++) {
					p->org[j] = move[j] + crand() * orgscale;
					p->vel[j] = crand() * velscale;
					p->accel[j] = 0;
				}
				p->vel[2] -= PARTICLE_GRAVITY;
			} else {
				p->alpha = 1.0;
				p->alphavel = -1.0 / (1 + frand() * 0.2);
				p->color = 4 + (rand() & 7);
				for (j = 0; j < 3; j++) {
					p->org[j] = move[j] + crand() * orgscale;
					p->vel[j] = crand() * velscale;
				}
				p->accel[2] = 20;
			}
		}
		old->trailcount -= 5;
		if (old->trailcount < 100)
			old->trailcount = 100;
		VectorAdd(move, vec, move);
	}
}
#endif

void
MakeNormalVectors(vec3_t forward, vec3_t right, vec3_t up)
{
	float		d;

	/* this rotate and negat guarantees a vector */
	/* not colinear with the original */
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct(right, forward);
	VectorMA(right, -d, forward, right);
	VectorNormalize(right);
	CrossProduct(right, forward, up);
}

/*
 * =============== CL_RocketTrail
 *
 * ===============
 */
#ifdef QMAX
void
CL_RocketTrail(vec3_t start, vec3_t end, centity_t * old)
{
	vec3_t		move;
	vec3_t		vec;
	float		len, totallen;
	float		dec;

	/* smoke */
	CL_DiminishingTrail(start, end, old, EF_ROCKET);

	/* fire */
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	totallen = len = VectorNormalize(vec);

	dec = 1;
	VectorScale(vec, dec, vec);

	while (len > 0) {
		len -= dec;

		if (!free_particles)
			return;

		/* flame from rocket */
		setupParticle(
		    0, 0, 0,
		    move[0], move[1], move[2],
		    vec[0], vec[1], vec[2],
		    0, 0, 0,
		    255, 200, 100,
		    0, 0, -200,
		    1, -15,
		    GL_SRC_ALPHA, GL_ONE,
		    2.0 * (2 - len / totallen), -15,
		    particle_generic,
		    0,
		    NULL, 0);

		/* falling particles */
		if ((rand() & 7) == 0) {
			setupParticle(
			    0, 0, 0,
			    move[0] + crand() * 5, move[1] + crand() * 5, move[2] + crand() * 5,
			    crand() * 20, crand() * 20, crand() * 20,
			    0, 0, 20,
			    255, 255, 255,
			    0, -100, -200,
			    1, -1.0 / (1 + frand() * 0.2),
			    GL_SRC_ALPHA, GL_ONE,
			    1.5, -3,
			    particle_blaster,
			    PART_GRAVITY,
			    NULL, 0);
		}
		VectorAdd(move, vec, move);
	}

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	totallen = len = VectorNormalize(vec);
	dec = 1.5;
	VectorScale(vec, dec, vec);

	while (len > 0) {
		len -= dec;

		/* flame */
		setupParticle(
		    crand() * 180, crand() * 100, 0,
		    move[0], move[1], move[2],
		    crand() * 5, crand() * 5, crand() * 5,
		    0, 0, 5,
		    255, 225, 200,
		    -50, -50, -50,
		    0.75, -3,
		    GL_SRC_ALPHA, GL_ONE,
		    5, 5,
		    particle_inferno,
		    0,
		    pRotateThink, true);

		VectorAdd(move, vec, move);
	}
}
#else
void
CL_RocketTrail(vec3_t start, vec3_t end, centity_t * old)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		j;
	cparticle_t    *p;
	float		dec;

	/* smoke */
	CL_DiminishingTrail(start, end, old, EF_ROCKET);

	/* fire */
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 1;
	VectorScale(vec, dec, vec);

	while (len > 0) {
		len -= dec;

		if (!free_particles)
			return;

		if ((rand() & 7) == 0) {
			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;

			VectorClear(p->accel);
			p->time = cl.time;

			p->alpha = 1.0;
			p->alphavel = -1.0 / (1 + frand() * 0.2);
			p->color = 0xdc + (rand() & 3);
			for (j = 0; j < 3; j++) {
				p->org[j] = move[j] + crand() * 5;
				p->vel[j] = crand() * 20;
			}
			p->accel[2] = -PARTICLE_GRAVITY;
		}
		VectorAdd(move, vec, move);
	}
}
#endif

/*
 * =============== CL_RailTrail
 *
 * ===============
 */
#ifdef QMAX
#define RAILSPACE 1.0
#define DEVRAILSTEPS 2
#define RAILTRAILSPACE 15
/*
===============
FartherPoint
Returns true if the first vector
is farther from the viewpoint.
===============
*/

qboolean FartherPoint (vec3_t pt1, vec3_t pt2)
{
	vec3_t		distance1, distance2;

	VectorSubtract(pt1, cl.refdef.vieworg, distance1);
	VectorSubtract(pt2, cl.refdef.vieworg, distance2);
	return (VectorLength(distance1) > VectorLength(distance2));
}

void
CL_RailSprial(vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	vec3_t		right, up;
	int		i;
	float		d, c, s;
	vec3_t		dir;

	// Draw from closest point
	if (FartherPoint(start, end)) {
		VectorCopy (end, move);
		VectorSubtract (start, end, vec);
	}
	else {
		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
	}
	len = VectorNormalize(vec);

	MakeNormalVectors(vec, right, up);

	VectorScale(vec, RAILSPACE, vec);


	for (i = 0; i < len; i += RAILSPACE) {
		d = i * 0.1;
		c = cos(d);
		s = sin(d);

		VectorScale(right, c, dir);
		VectorMA(dir, s, up, dir);

		setupParticle(
		    0, 0, 0,
		    move[0] + dir[0] * 3, move[1] + dir[1] * 3, move[2] + dir[2] * 3,
		    dir[0] * 6, dir[1] * 6, dir[2] * 6,
		    0, 0, 0,
		    cl_railred->value, cl_railgreen->value, cl_railblue->value,
		    0, 0, 0,
		    1, -1.0,
		    GL_SRC_ALPHA, GL_ONE,
		    3 * RAILSPACE, 0,
		    particle_generic,
		    0,
		    NULL, 0);

		VectorAdd(move, vec, move);
	}
}

void
pDevRailThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	int		i;
	vec3_t		len;

	VectorSubtract(p->angle, org, len);

	*size *= (float)(SplashSize / VectorLength(len)) * 0.5 / ((4 - *size));
	if (*size > SplashSize)
		*size = SplashSize;

	/* setting up angle for sparks */
	{
		float		time1   , time2;

		time1 = *time;
		time2 = time1 * time1;

		for (i = 0; i < 2; i++)
			angle[i] = 3 * (p->vel[i] * time1 + (p->accel[i]) * time2);
		angle[2] = 3 * (p->vel[2] * time1 + (p->accel[2] - PARTICLE_GRAVITY) * time2);
	}

	p->thinknext = true;
}

void
CL_DevRailTrail(vec3_t start, vec3_t end)
{
	vec3_t		move, last;
	vec3_t		vec, point;
	float		len;
	int		dec, i = 0;

	// Draw from closest point
	if (FartherPoint(start, end)) {
		VectorCopy (end, move);
		VectorSubtract (start, end, vec);
	}
	else {
		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
	}
	len = VectorNormalize(vec);
	VectorCopy(vec, point);

	dec = 4;
	VectorScale(vec, dec, vec);

	/* FIXME: this is a really silly way to have a loop */
	while (len > 0) {
		len -= dec;
		i++;

		VectorCopy(move, last);
		VectorAdd(move, vec, move);

		if (i >= DEVRAILSTEPS) {
			for (i = 3; i > 0; i--)
				setupParticle(
				    last[0], last[1], last[2],
				    move[0], move[1], move[2],
				    0, 0, 0,
				    0, 0, 0,
				    cl_railred->value, cl_railgreen->value, cl_railblue->value,
				    0, -90, -30,
				    0.50, -1,
				    GL_SRC_ALPHA, GL_ONE,
				    dec * DEVRAILSTEPS * TWOTHIRDS, 0,
				    0,
				    PART_TRANS,
				    NULL, 0);
		}
		setupParticle(
		    0, 0, 0,
		    move[0], move[1], move[2],
		    crand() * 10, crand() * 10, crand() * 10 + 20,
		    0, 0, 0,
		    cl_railred->value, cl_railgreen->value, cl_railblue->value,
		    0, 0, 0,
		    1, -0.75 / (0.5 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    2, -0.25,
		    0,
		    PART_GRAVITY | PART_DIRECTION,
		    pDevRailThink, true);

		setupParticle(
		    crand() * 180, crand() * 100, 0,
		    move[0], move[1], move[2],
		    crand() * 10, crand() * 10, crand() * 10 + 20,
		    0, 0, 5,
		    255, 255, 255,
		    0, 0, 0,
		    0.25, -0.25,
		    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
		    5, 10,
		    particle_smoke,
		    PART_TRANS | PART_GRAVITY,
		    pRotateThink, true);
	}
}

void
CL_RailTrail(vec3_t start, vec3_t end)
{
	vec3_t		move, last;
	vec3_t		vec, point;
	/*int		i; */
	float		len;
	vec3_t		right, up;
	qboolean	colored = (cl_railtype->value != 1);
	/*vec3_t		pos, dir; */

	// Draw from closest point
	if (FartherPoint(start, end)) {
		VectorCopy (end, move);
		VectorSubtract (start, end, vec);
	}
	else {
		VectorCopy (start, move);
		VectorSubtract (end, start, vec);
	}
	len = VectorNormalize(vec);
	VectorCopy(vec, point);

	MakeNormalVectors(vec, right, up);
	VectorScale(vec, (cl_railtype->value == 2) ? RAILTRAILSPACE : RAILSPACE, vec);
	VectorCopy(start, move);


	if (cl_railtype->value == 3) {
		CL_DevRailTrail(start, end);
		return;
	}
	while (len > 0) {
		VectorCopy(move, last);
		VectorAdd(move, vec, move);

		if (cl_railtype->value == 2) {
			for (; len>0; len -= RAILTRAILSPACE) {
			
				MakeNormalVectors (vec, right, up);
				VectorScale (vec, RAILTRAILSPACE, vec);
				VectorCopy (start, move);

				setupParticle(
					last[0], last[1], last[2],
					move[0], move[1], move[2],
					0, 0, 0,
					0, 0, 0,
					cl_railred->value, cl_railgreen->value, cl_railblue->value,
					0, 0, 0,
					1, -1,
					GL_SRC_ALPHA, GL_ONE,
					10, 0,
					particle_beam,
					PART_BEAM,
					NULL,0);
			}
				setupParticle(
					start[0], start[1], start[2],
					end[0], end[1], end[2],
					0, 0, 0,
					0, 0, 0,
					cl_railred->value, cl_railgreen->value, cl_railblue->value,
					0, 0, 0,
					1, -1,
					GL_SRC_ALPHA, GL_ONE,
					10, 0,
					particle_beam,
					PART_BEAM,
					NULL,0);

		} else {
			len -= RAILSPACE;

			setupParticle(
			    0, 0, 0,
			    move[0], move[1], move[2],
			    0, 0, 0,
			    0, 0, 0,
			    (colored) ? cl_railred->value : 255, (colored) ? cl_railgreen->value : 255, (colored) ? cl_railblue->value : 255,
			    0, 0, 0,
			    1, -1.0,
			    GL_SRC_ALPHA, GL_ONE,
			    3 * RAILSPACE, 0,
			    particle_generic,
			    0,
			    NULL, 0);
		}
	}

	if (cl_railtype->value == 1)
		CL_RailSprial(start, end);
}
#else
void
CL_RailTrail(vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		j;
	cparticle_t    *p;
	float		dec;
	vec3_t		right, up;
	int		i;
	float		d, c, s;
	vec3_t		dir, vup;
	byte		clr = 0x74;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	MakeNormalVectors(vec, right, up);

	for (i = 0; i < len; i++) {
		if (!free_particles)
			return;

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		VectorClear(p->accel);

		d = i * 0.1;
		c = cos(d);
		s = sin(d);

		VectorScale(right, c, dir);
		if (cl_railstyle->value == 2) {
			VectorScale(vup, c, up);
		}
		VectorMA(dir, s, up, dir);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1 + frand() * 0.2);
		p->color = clr + (rand() & 7);
		for (j = 0; j < 3; j++) {
			p->org[j] = move[j] + dir[j] * 3;
			p->vel[j] = dir[j] * 6;
		}

		VectorAdd(move, vec, move);
	}

	if (cl_railstyle->value == 0) {
		dec = 0.75;
		VectorScale(vec, dec, vec);
		VectorCopy(start, move);

		while (len > 0) {
			len -= dec;

			if (!free_particles)
				return;
			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;

			p->time = cl.time;
			VectorClear(p->accel);

			p->alpha = 1.0;
			p->alphavel = -1.0 / (0.6 + frand() * 0.2);

			p->color = 0x0 + (rand() & 15);

			for (j = 0; j < 3; j++) {
				p->org[j] = move[j] + crand() * 3;
				p->vel[j] = crand() * 3;
				p->accel[j] = 0;
			}

			VectorAdd(move, vec, move);
		}
	}
}
#endif

/* RAFAEL */

/*
 * =============== CL_IonripperTrail ===============
 */
#ifdef QMAX
void
CL_IonripperTrail(vec3_t ent, vec3_t start)
{
	vec3_t		move, last;
	vec3_t		vec, aim;
	float		len;
	float		dec;
	float		overlap;

	VectorCopy(start, move);
	VectorSubtract(ent, start, vec);
	len = VectorNormalize(vec);
	VectorCopy(vec, aim);

	dec = len * 0.2;
	if (dec < 1)
		dec = 1;

	overlap = dec * 5.0;

	VectorScale(vec, dec, vec);

	while (len > 0) {
		len -= dec;

		VectorCopy(move, last);
		VectorAdd(move, vec, move);

		setupParticle(
		    last[0], last[1], last[2],
		    move[0] + aim[0] * overlap, move[1] + aim[1] * overlap, move[2] + aim[2] * overlap,
		    0, 0, 0,
		    0, 0, 0,
		    255, 100, 0,
		    0, 0, 0,
		    0.5, -1.0 / (0.3 + frand() * 0.2),
		    GL_SRC_ALPHA, GL_ONE,
		    3, 3,
		    particle_generic,
		    PART_BEAM,
		    NULL, 0);
	}

	setupParticle(
	    0, 0, 0,
	    start[0], start[1], start[2],
	    0, 0, 0,
	    0, 0, 0,
	    255, 100, 0,
	    0, 0, 0,
	    0.5, -1.0 / (0.3 + frand() * 0.2),
	    GL_SRC_ALPHA, GL_ONE,
	    3, 3,
	    particle_generic,
	    0,
	    NULL, 0);
}
#else
void
CL_IonripperTrail(vec3_t start, vec3_t ent)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		j;
	cparticle_t    *p;
	int		dec;
	int		left = 0;

	VectorCopy(start, move);
	VectorSubtract(ent, start, vec);
	len = VectorNormalize(vec);

	dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0) {
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = cl.time;
		p->alpha = 0.5;
		p->alphavel = -1.0 / (0.3 + frand() * 0.2);

		p->color = 0xe4 + (rand() & 3);

		for (j = 0; j < 3; j++) {
			p->org[j] = move[j];
			p->accel[j] = 0;
		}
		if (left) {
			left = 0;
			p->vel[0] = 10;
		} else {
			left = 1;
			p->vel[0] = -10;
		}

		p->vel[1] = 0;
		p->vel[2] = 0;

		VectorAdd(move, vec, move);
	}
}
#endif

/*
 * =============== CL_BubbleTrail
 *
 * ===============
 */
#ifdef QMAX
void
CL_BubbleTrail(vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		i;
	float		dec;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 32;
	VectorScale(vec, dec, vec);

	for (i = 0; i < len; i += dec) {

		setupParticle(
		    0, 0, 0,
		    move[0] + crand() * 2, move[1] + crand() * 2, move[2] + crand() * 2,
		    crand() * 5, crand() * 5, crand() * 5 + 6,
		    0, 0, 0,
		    255, 255, 255,
		    0, 0, 0,
		    0.75, -1.0 / (1 + frand() * 0.2),
		    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
		    (frand() > 0.25) ? 1 : (frand() > 0.5) ? 2 : (frand() > 0.75) ? 3 : 4, 1,
		    particle_bubble,
		    PART_TRANS | PART_SHADED,
		    NULL, 0);

		VectorAdd(move, vec, move);
	}
}
#else
void
CL_BubbleTrail(vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int		i, j;
	cparticle_t    *p;
	float		dec;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 32;
	VectorScale(vec, dec, vec);

	for (i = 0; i < len; i += dec) {
		if (!free_particles)
			return;

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		VectorClear(p->accel);
		p->time = cl.time;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1 + frand() * 0.2);
		p->color = 4 + (rand() & 7);
		for (j = 0; j < 3; j++) {
			p->org[j] = move[j] + crand() * 2;
			p->vel[j] = crand() * 5;
		}
		p->vel[2] += 6;

		VectorAdd(move, vec, move);
	}
}
#endif

/*
 * =============== CL_FlyParticles ===============
 */
#define	BEAMLENGTH	16
#ifdef QMAX
void
CL_FlyParticles(vec3_t origin, int count)
{
	int		i;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64;
	float		ltime;


	if (count > NUMVERTEXNORMALS)
		count = NUMVERTEXNORMALS;

	if (!avelocities[0][0]) {
		for (i = 0; i < NUMVERTEXNORMALS * 3; i++)
			avelocities[0][i] = (rand() & 255) * 0.01;
	}
	ltime = (float)cl.time / 1000.0;
	for (i = 0; i < count; i += 2) {
		angle = ltime * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = ltime * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = ltime * avelocities[i][2];
		sr = sin(angle);
		cr = cos(angle);

		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;

		dist = sin(ltime + i) * 64;

		setupParticle(
		    0, 0, 0,
		    origin[0] + bytedirs[i][0] * dist + forward[0] * BEAMLENGTH, origin[1] + bytedirs[i][1] * dist + forward[1] * BEAMLENGTH,
		    origin[2] + bytedirs[i][2] * dist + forward[2] * BEAMLENGTH,
		    0, 0, 0,
		    0, 0, 0,
		    0, 0, 0,
		    0, 0, 0,
		    1, -100,
		    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
		    1 + sin(i + ltime), 1,
		    particle_generic,
		    PART_TRANS,
		    NULL, 0);
	}
}
#else
void
CL_FlyParticles(vec3_t origin, int count)
{
	int		i;
	cparticle_t    *p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64;
	float		ltime;


	if (count > NUMVERTEXNORMALS)
		count = NUMVERTEXNORMALS;

	if (!avelocities[0][0]) {
		for (i = 0; i < NUMVERTEXNORMALS * 3; i++)
			avelocities[0][i] = (rand() & 255) * 0.01;
	}
	ltime = (float)cl.time / 1000.0;
	for (i = 0; i < count; i += 2) {
		angle = ltime * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = ltime * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = ltime * avelocities[i][2];
		sr = sin(angle);
		cr = cos(angle);

		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;

		dist = sin(ltime + i) * 64;
		p->org[0] = origin[0] + bytedirs[i][0] * dist + forward[0] * BEAMLENGTH;
		p->org[1] = origin[1] + bytedirs[i][1] * dist + forward[1] * BEAMLENGTH;
		p->org[2] = origin[2] + bytedirs[i][2] * dist + forward[2] * BEAMLENGTH;

		VectorClear(p->vel);
		VectorClear(p->accel);

		p->color = 0;
		p->colorvel = 0;

		p->alpha = 1;
		p->alphavel = -100;
	}
}
#endif
void
CL_FlyEffect(centity_t * ent, vec3_t origin)
{
	int		n;
	int		count;
	int		starttime;

	if (ent->fly_stoptime < cl.time) {
		starttime = cl.time;
		ent->fly_stoptime = cl.time + 60000;
	} else {
		starttime = ent->fly_stoptime - 60000;
	}

	n = cl.time - starttime;
	if (n < 20000)
		count = n * 162 / 20000.0;
	else {
		n = ent->fly_stoptime - cl.time;
		if (n < 20000)
			count = n * 162 / 20000.0;
		else
			count = 162;
	}

	CL_FlyParticles(origin, count);
}


/*
 * =============== CL_BfgParticles ===============
 */
#ifdef QMAX
void
pBFGThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	vec3_t		len;

	VectorSubtract(p->angle, p->org, len);

	*size = (float)((300 / VectorLength(len)) * 0.75);
}

void
CL_BfgParticles(entity_t * ent)
{
	int		i;
	cparticle_t    *p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64, dist2;
	vec3_t		v;
	float		ltime, size;
	particle_type particleType;
	
	if (cl_particles_type->value == 2) {
		size = 5.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 1.5;	
		particleType = particle_bubble;
	}
	else {
		size = 1.0;	
		particleType = particle_generic;
	}

	if (!avelocities[0][0]) {
		for (i = 0; i < NUMVERTEXNORMALS * 3; i++)
			avelocities[0][i] = (rand() & 255) * 0.01;
	}
	ltime = (float)cl.time / 1000.0;
	for (i = 0; i < NUMVERTEXNORMALS; i++) {
		angle = ltime * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = ltime * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = ltime * avelocities[i][2];
		sr = sin(angle);
		cr = cos(angle);

		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;

		dist2 = dist;
		dist = sin(ltime + i) * 64;

		p = setupParticle(
		    ent->origin[0], ent->origin[1], ent->origin[2],
		    ent->origin[0] + bytedirs[i][0] * dist + forward[0] * BEAMLENGTH, ent->origin[1] + bytedirs[i][1] * dist + forward[1] * BEAMLENGTH,
		    ent->origin[2] + bytedirs[i][2] * dist + forward[2] * BEAMLENGTH,
		    0, 0, 0,
		    0, 0, 0,
		    50, 200 * dist2, 20,
		    0, 0, 0,
		    1, -100,
		    GL_SRC_ALPHA, GL_ONE,
		    size, 1,
		    particleType,
		    0,
		    pBFGThink, true);

		if (!p)
			return;

		VectorSubtract(p->org, ent->origin, v);
		dist = VectorLength(v) / 90.0;
	}
}
#else
void
CL_BfgParticles(entity_t * ent)
{
	int		i;
	cparticle_t    *p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64;
	vec3_t		v;
	float		ltime;

	if (!avelocities[0][0]) {
		for (i = 0; i < NUMVERTEXNORMALS * 3; i++)
			avelocities[0][i] = (rand() & 255) * 0.01;
	}
	ltime = (float)cl.time / 1000.0;
	for (i = 0; i < NUMVERTEXNORMALS; i++) {
		angle = ltime * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = ltime * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = ltime * avelocities[i][2];
		sr = sin(angle);
		cr = cos(angle);

		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;

		dist = sin(ltime + i) * 64;
		p->org[0] = ent->origin[0] + bytedirs[i][0] * dist + forward[0] * BEAMLENGTH;
		p->org[1] = ent->origin[1] + bytedirs[i][1] * dist + forward[1] * BEAMLENGTH;
		p->org[2] = ent->origin[2] + bytedirs[i][2] * dist + forward[2] * BEAMLENGTH;

		VectorClear(p->vel);
		VectorClear(p->accel);

		VectorSubtract(p->org, ent->origin, v);
		dist = VectorLength(v) / 90.0;
		p->color = floor(0xd0 + dist * 7);
		p->colorvel = 0;
		p->alpha = 1.0 - dist;
		p->alphavel = -100;
	}
}
#endif

/*
 * =============== CL_TrapParticles ===============
 */
/* RAFAEL */
#ifdef QMAX
void
CL_TrapParticles(entity_t * ent)
{
	int		colors[][3] = {
		{255, 200, 150},
		{255, 200, 100},
		{255, 200, 50},
		{0, 0, 0}
	};
	
	vec3_t		move;
	vec3_t		vec;
	vec3_t		start, end;
	float		len;
	int		dec, index;

	ent->origin[2] -= 14;
	VectorCopy(ent->origin, start);
	VectorCopy(ent->origin, end);
	end[2] += 64;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 2.5;
	VectorScale(vec, dec, vec);

	/* FIXME: this is a really silly way to have a loop */
	while (len > 0) {
		len -= dec;

		index = rand() & 3;

		setupParticle(
		    0, 0, 0,
		    move[0] + crand(), move[1] + crand(), move[2] + crand(),
		    crand() * 15, crand() * 15, crand() * 15,
		    0, 0, PARTICLE_GRAVITY,
		    colors[index][0], colors[index][1], colors[index][2],
		    0, 0, 0,
		    1, -1.0 / (0.3 + frand() * 0.2),
		    GL_SRC_ALPHA, GL_ONE,
		    3, -5,
		    particle_generic,
		    0,
		    NULL, 0);

		VectorAdd(move, vec, move);
	}

	{

		int		i, j, k;
		float		vel;
		vec3_t		dir;
		vec3_t		org;


		ent->origin[2] += 14;
		VectorCopy(ent->origin, org);


		for (i = -2; i <= 2; i += 4)
			for (j = -2; j <= 2; j += 4)
				for (k = -2; k <= 4; k += 4) {

					dir[0] = j * 8;
					dir[1] = i * 8;
					dir[2] = k * 8;

					VectorNormalize(dir);
					vel = 50 + (rand() & 63);

					index = rand() & 3;

					setupParticle(
					    0, 0, 0,
					    org[0] + i + ((rand() & 23) * crand()), org[1] + j + ((rand() & 23) * crand()), org[2] + k + ((rand() & 23) * crand()),
					    dir[0] * vel, dir[1] * vel, dir[2] * vel,
					    0, 0, 0,
					    colors[index][0], colors[index][1], colors[index][2],
					    0, 0, 0,
					    1, -1.0 / (0.3 + frand() * 0.2),
					    GL_SRC_ALPHA, GL_ONE,
					    3, -10,
					    particle_generic,
					    PART_GRAVITY,
					    NULL, 0);
				}
	}
}
#else
void
CL_TrapParticles(entity_t * ent)
{
	vec3_t		move;
	vec3_t		vec;
	vec3_t		start, end;
	float		len;
	int		j;
	cparticle_t    *p;
	int		dec;

	ent->origin[2] -= 14;
	VectorCopy(ent->origin, start);
	VectorCopy(ent->origin, end);
	end[2] += 64;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 5;
	VectorScale(vec, 5, vec);

	/* FIXME: this is a really silly way to have a loop */
	while (len > 0) {
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = cl.time;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.3 + frand() * 0.2);
		p->color = 0xe0;
		for (j = 0; j < 3; j++) {
			p->org[j] = move[j] + crand();
			p->vel[j] = crand() * 15;
			p->accel[j] = 0;
		}
		p->accel[2] = PARTICLE_GRAVITY;

		VectorAdd(move, vec, move);
	}

	{


		int		i, j, k;
		cparticle_t    *p;
		float		vel;
		vec3_t		dir;
		vec3_t		org;


		ent->origin[2] += 14;
		VectorCopy(ent->origin, org);


		for (i = -2; i <= 2; i += 4)
			for (j = -2; j <= 2; j += 4)
				for (k = -2; k <= 4; k += 4) {
					if (!free_particles)
						return;
					p = free_particles;
					free_particles = p->next;
					p->next = active_particles;
					active_particles = p;

					p->time = cl.time;
					p->color = 0xe0 + (rand() & 3);
					p->alpha = 1.0;
					p->alphavel = -1.0 / (0.3 + (rand() & 7) * 0.02);

					p->org[0] = org[0] + i + ((rand() & 23) * crand());
					p->org[1] = org[1] + j + ((rand() & 23) * crand());
					p->org[2] = org[2] + k + ((rand() & 23) * crand());

					dir[0] = j * 8;
					dir[1] = i * 8;
					dir[2] = k * 8;

					VectorNormalize(dir);
					vel = 50 + (rand() & 63);
					VectorScale(dir, vel, p->vel);

					p->accel[0] = p->accel[1] = 0;
					p->accel[2] = -PARTICLE_GRAVITY;
				}
	}
}
#endif

/*
 * =============== CL_BFGExplosionParticles ===============
 */
/* FIXME combined with CL_ExplosionParticles */
#ifdef QMAX
void
CL_BFGExplosionParticles(vec3_t org)
{
	int		i;
	float		size;
	particle_type particleType;
	
	if (cl_particles_type->value == 2) {
		size = 12.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 6.0;	
		particleType = particle_bubble;
	}
	else {
		size = 10.0;	
		particleType = particle_generic;
	}

	for (i = 0; i < 256; i++) {
		setupParticle(
		    0, 0, 0,
		    org[0] + ((rand() % 32) - 16), org[1] + ((rand() % 32) - 16), org[2] + ((rand() % 32) - 16),
		    (rand() % 150) - 75, (rand() % 150) - 75, (rand() % 150) - 75,
		    0, 0, 0,
		    200, 100 + rand() * 50, 0,
		    0, 0, 0,
		    1, -0.8 / (0.5 + frand() * 0.3),
		    GL_SRC_ALPHA, GL_ONE,
		    size, -10,
		    particleType,
		    PART_GRAVITY,
		    NULL, 0);
	}
}

void 
CL_BFGExplosionParticles_2(vec3_t org) /* ECHON */
{
	int		i;
	float		randcolor, size;
	particle_type particleType;
	
	if (cl_particles_type->value == 2) {
		size = 8.0;	
		particleType = particle_lensflare;		
	}
	else if (cl_particles_type->value == 1) {
		size = 3.0;	
		particleType = particle_bubble;
	}
	else {
		size = 6.0;	
		particleType = particle_generic;
	}

	for (i=0 ; i<256 ; i++) {
		randcolor = (rand()&1) ? 150 + (rand()%26) : 0.0f;
		setupParticle(
			0, 0, 0,
			org[0] + (crand () * 20), org[1] + (crand () * 20), org[2] + (crand () * 20),
			crand () * 50, crand () * 50, crand () * 50,
			0, 0, -40,
			randcolor, 75 + (rand()%150) + randcolor, (rand()%50) + randcolor,
			randcolor, 75 + (rand()%150) + randcolor, (rand()%50) + randcolor,
			1.0f, -0.8f / (0.8f + (frand () * 0.3f)),
		    	GL_SRC_ALPHA, GL_ONE,
			size + (crand () * 5.5f), 0.6f + (crand () * 0.5f),
			particleType,
			PART_GRAVITY,
			pBounceThink, false);
	}
}
#else
void
CL_BFGExplosionParticles(vec3_t org)
{
	int		i, j;
	cparticle_t    *p;

	for (i = 0; i < 256; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		p->color = 0xd0 + (rand() & 7);
		for (j = 0; j < 3; j++) {
			p->org[j] = org[j] + ((rand() % 32) - 16);
			p->vel[j] = (rand() % 384) - 192;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.8 / (0.5 + frand() * 0.3);
	}
}
#endif

/*
 * =============== CL_TeleportParticles
 *
 * ===============
 */
#ifdef QMAX
void 
CL_MakeTeleportParticles_Old (vec3_t org, float min, float max, float size, 
    int red, int green, int blue, particle_type particleType)
{
	cparticle_t	*p;
	int		i, j, k;
	float		vel, resize = 1;
	vec3_t		dir, temp;

	resize += size/10;

	for (i=-16 ; i<=16 ; i+=4)
		for (j=-16 ; j<=16 ; j+=4)
			for (k=min ; k<=max ; k+=4)
			{
				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;

				VectorNormalize (dir);
				vel = (rand()&63);

				p = setupParticle(
					0, 0, 0,
					org[0]+ (i+(rand()&3))*resize, org[1]+(j+(rand()&3))*resize, org[2]+(k+(rand()&3))*resize,
					dir[0]*vel, dir[1]*vel, dir[2]*(25 + vel),
					0, 0, 0,
					red, green, blue,
					0, 0, 0,
					1, -0.75 / (0.3 + (rand()&7) * 0.02),
		    			GL_SRC_ALPHA, GL_ONE,
					(random()*.25+.75)*size*resize, 0,
					particleType,
					PART_GRAVITY,
					NULL,0);

				if (!p)
					continue;

				VectorCopy(p->org, temp); temp[2] = org[2];
				VectorSubtract(org, temp, p->vel);
				p->vel[2]+=25;
				
				VectorScale(p->vel, 3, p->accel);
			}
}

void
CL_MakeTeleportParticles_Opt1(vec3_t org, float min, float max, float size,
    int red, int green, int blue, particle_type particleType)
{
	cparticle_t	*p;
	int		i, j, k;
	float		vel, resize = 1;
	vec3_t		dir;

	resize += size/100;

	for (i=-16 ; i<=16 ; i+=4)
		for (j=-16 ; j<=16 ; j+=4)
			for (k=min ; k<=max ; k+=4)
			{
				dir[0] = j*16;
				dir[1] = i*16;
				dir[2] = k*16;

				VectorNormalize(dir);
				vel = 150 + (rand()&63);

				p = setupParticle(
				        0, 0, 0,
					org[0]+ (i+(rand()&3)) * resize, 
					org[1]+(j+(rand()&3)) * resize, 
					org[2]+(k+(rand()&3)) * resize,
					dir[0]*vel, dir[1]*vel, dir[2]*(25 + vel),
					0, 0, 0,
		    			200 + 55 * rand(), 200 + 55 * rand(), 200 + 55 * rand(),
					0, 0, 0,
		    			2.5 , -1 / (0.2 + (rand() & 7) * 0.01),
		    			GL_SRC_ALPHA, GL_ONE,
					(random()*.25+.75)*size*resize, 0,
					particleType,
					PART_GRAVITY,
					pRotateThink, 0);

				if (!p)
					continue;
			}
}

void
CL_MakeTeleportParticles_Opt2(vec3_t org, float min, float max, float size,
    int red, int green, int blue, particle_type particleType)
{
	int		i, j, k;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<=16 ; i+=4)
		for (j=-16 ; j<=16 ; j+=4)
			for (k=-16 ; k<=32 ; k+=4) {
				dir[0] = j*16;
				dir[1] = i*16;
				dir[2] = k*16;

				VectorNormalize (dir);
				vel = 150 + (rand()&63);

				setupParticle (
				    0, 0, 0,
				    org[0]+i+(rand()&3), org[1]+j+(rand()&3), org[2]+k+(rand()&3),
				    dir[0]*vel, dir[1]*vel, dir[2]*vel,
				    0, 0, 0,
				    200 + 55*rand(), 200 + 55*rand(), 200 + 55*rand(),
				    0, 0, 0,
				    1, -1.0 / (0.3 + (rand()&7) * 0.02),
				    GL_SRC_ALPHA, GL_ONE,
				    2, 3, 
				    particleType,
				    PART_GRAVITY,
				    NULL,0);
			}
}

void
CL_MakeTeleportParticles_Rotating(vec3_t org, float min, float max, float size, float grow,
    int red, int green, int blue, particle_type particleType)
{
	int		i;
	vec3_t		origin , vel;

	for (i = 0; i < 128; i++) {
		VectorSet(origin,
		    org[0] + rand() % 30 - 15,
		    org[1] + rand() % 30 - 15,
		    org[2] + min + random() * (max - min));

		VectorSubtract(org, origin, vel);
		VectorNormalize(vel);

		setupParticle(
		    random() * 360, crandom() * 45, 0,
		    origin[0], origin[1], origin[2],
		    vel[0] * 5, vel[1] * 5, vel[2] * 5,
		    vel[0] * 20, vel[1] * 20, vel[2] * 5,
		    200 + 55 * rand(), 200 + 55 * rand(), 200 + 55 * rand(),
		    0, 0, 0,
		    2, -0.25 / (1.0 + (rand() & 7) * 0.02),
		    GL_SRC_ALPHA, GL_ONE,
		    (random() * 1.0 + 1.0) * size, grow,
		    particleType,
		    PART_GRAVITY,
		    pRotateThink, true);
	}
}

void
CL_TeleportParticles(vec3_t org)
{
	extern cvar_t	*cl_teleport_particles;
        
        if (cl_teleport_particles->value == 4) {
		CL_MakeTeleportParticles_Rotating (org, -16, 32, 
		   2.5, 0.0 , 255, 255, 200, particle_lensflare);
        }      
        else if (cl_teleport_particles->value == 3) {
		CL_MakeTeleportParticles_Rotating (org, -16, 32, 
		   0.8, 0.0 , 255, 255, 200, particle_bubble);
        }     
        else if (cl_teleport_particles->value == 2) {
		CL_MakeTeleportParticles_Opt2(org, -16, 32,
	    	   3.0, 255, 255, 200, particle_lensflare);
        }
        else if (cl_teleport_particles->value == 1)  {
		CL_MakeTeleportParticles_Opt1(org, -16, 32,
	    	   2.0, 255, 255, 200, particle_bubble);
	}
        else 
		CL_MakeTeleportParticles_Old(org, -16, 32,
	    	   1.0, 200, 255, 255, particle_generic);
}

void
CL_Disintegrate(vec3_t pos, int ent)
{
	CL_MakeTeleportParticles_Old(pos, -16, 24,
	    7.5, 100, 100, 255, particle_smoke);
}

void
CL_FlameBurst(vec3_t pos, float size)
{
	CL_MakeTeleportParticles_Old(pos, -16, 24,
	    size, 255, 255, 255, particle_inferno);
}
#else
void
CL_TeleportParticles(vec3_t org)
{
	int		i, j, k;
	cparticle_t    *p;
	float		vel;
	vec3_t		dir;

	for (i = -16; i <= 16; i += 4)
		for (j = -16; j <= 16; j += 4)
			for (k = -16; k <= 32; k += 4) {
				if (!free_particles)
					return;
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;

				p->time = cl.time;
				p->color = 7 + (rand() & 7);
				p->alpha = 1.0;
				p->alphavel = -1.0 / (0.3 + (rand() & 7) * 0.02);

				p->org[0] = org[0] + i + (rand() & 3);
				p->org[1] = org[1] + j + (rand() & 3);
				p->org[2] = org[2] + k + (rand() & 3);

				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;

				VectorNormalize(dir);
				vel = 50 + (rand() & 63);
				VectorScale(dir, vel, p->vel);

				p->accel[0] = p->accel[1] = 0;
				p->accel[2] = -PARTICLE_GRAVITY;
			}
}
#endif

#ifdef QMAX
void
pLensFlareThink(cparticle_t * p, vec3_t org, vec3_t angle, float *alpha, float *size, int *image, float *time)
{
	angle[2] = anglemod(cl.refdef.viewangles[YAW]);

	p->thinknext = true;
}

void
CL_LensFlare(vec3_t pos, vec3_t color, float size, float time)
{
	setupParticle(
	    0, 0, 0,
	    pos[0], pos[1], pos[2],
	    0, 0, 0,
	    0, 0, 0,
	    color[0], color[1], color[2],
	    0, 0, 0,
	    1, -2.0 / time,
	    GL_SRC_ALPHA, GL_ONE,
	    size, 0,
	    particle_lensflare,
	    PART_LENSFLARE,
	    pLensFlareThink, true);
}
#endif

/*
 * =============== CL_AddParticles ===============
 */
#ifdef QMAX
void
CL_AddParticles(void)
{
	cparticle_t    *p, *next;
	float		alpha, size, light;
	float		time = 0, time2 = 0;
	vec3_t		org, color, angle;
	int		i, image;
	cparticle_t    *active, *tail;

	active = NULL;
	tail = NULL;

	for (p = active_particles; p; p = next) {
		next = p->next;

		/* PMM - added INSTANT_PARTICLE handling for heat beam */
		if (p->alphavel != INSTANT_PARTICLE) {
			/* this fixes jumpy particles */
			if (cl.time > p->time)
				p->time = cl.time;

			time = (p->time - p->start) * 0.001;
			alpha = p->alpha + time * p->alphavel;

			if (alpha <= 0) {	/* faded out */
				p->next = free_particles;
				free_particles = p;
				continue;
			}
		} else {
			alpha = p->alpha;
		}

		p->next = NULL;
		if (!tail)
			active = tail = p;
		else {
			tail->next = p;
			tail = p;
		}

		if (alpha > 1.0)
			alpha = 1;

		time2 = time * time;
		image = p->image;

		for (i = 0; i < 3; i++) {
			color[i] = p->color[i] + p->colorvel[i] * time;
			if (color[i] > 255)
				color[i] = 255;
			if (color[i] < 0)
				color[i] = 0;

			angle[i] = p->angle[i];
			org[i] = p->org[i] + p->vel[i] * time + p->accel[i] * time2;
		}

		if (p->flags & PART_GRAVITY)
			org[2] += time2 * -PARTICLE_GRAVITY;

		size = p->size + p->sizevel * time;

		for (i = 0; i < P_LIGHTS_MAX; i++) {
			const cplight_t *plight = &p->lights[i];

			if (plight->isactive) {
				light = plight->light * alpha + plight->lightvel * time;
				V_AddLight(org, light, plight->lightcol[0], plight->lightcol[1], plight->lightcol[2]);
			}
		}

		if (p->thinknext && p->think) {
			p->thinknext = false;
			p->think(p, org, angle, &alpha, &size, &image, &time);
		}
		V_AddParticle(org, angle, color, alpha, p->blendfunc_src, p->blendfunc_dst, size, image, p->flags);

		if (p->alphavel == INSTANT_PARTICLE) {
			p->alphavel = 0.0;
			p->alpha = 0.0;
		}
	}

	active_particles = active;
}
#else
void
CL_AddParticles(void)
{
	cparticle_t    *p, *next;
	float		alpha;
	float		time = 0, time2 = 0;
	vec3_t		org;
	int		color;
	cparticle_t    *active, *tail;

	active = NULL;
	tail = NULL;

	for (p = active_particles; p; p = next) {
		next = p->next;

		/* PMM - added INSTANT_PARTICLE handling for heat beam */
		if (p->alphavel != INSTANT_PARTICLE) {
			time = (cl.time - p->time) * 0.001;
			alpha = p->alpha + time * p->alphavel;
			if (alpha <= 0) {	/* faded out */
				p->next = free_particles;
				free_particles = p;
				continue;
			}
		} else {
			time = 0.0;
			alpha = p->alpha;
		}

		p->next = NULL;
		if (!tail)
			active = tail = p;
		else {
			tail->next = p;
			tail = p;
		}

		if (alpha > 1.0)
			alpha = 1;
		color = p->color;
		time2 = time * time;

		org[0] = p->org[0] + p->vel[0] * time + p->accel[0] * time2;
		org[1] = p->org[1] + p->vel[1] * time + p->accel[1] * time2;
		org[2] = p->org[2] + p->vel[2] * time + p->accel[2] * time2;

		V_AddParticle(org, color, alpha);
		/* PMM */
		if (p->alphavel == INSTANT_PARTICLE) {
			p->alphavel = 0.0;
			p->alpha = 0.0;
		}
	}

	active_particles = active;
}
#endif

/*
 * ============== CL_EntityEvent
 *
 * An entity has just been parsed that has an event value
 *
 * the female events are there for backwards compatability ==============
 */
 
extern struct sfx_s *cl_sfx_footsteps[4];

// Knightmare- footstep definition stuff
#define MAX_TEX_SURF 256
struct texsurf_s
{
	int	step_id;
	char	tex[32];
};

typedef struct texsurf_s texsurf_t;

texsurf_t tex_surf[MAX_TEX_SURF];
int	num_texsurfs;

//Knightmare- Lazarus footstep sounds
extern struct sfx_s	*cl_sfx_metal_footsteps[4];
extern struct sfx_s	*cl_sfx_dirt_footsteps[4];
extern struct sfx_s	*cl_sfx_vent_footsteps[4];
extern struct sfx_s	*cl_sfx_grate_footsteps[4];
extern struct sfx_s	*cl_sfx_tile_footsteps[4];
extern struct sfx_s	*cl_sfx_grass_footsteps[4];
extern struct sfx_s	*cl_sfx_snow_footsteps[4];
extern struct sfx_s	*cl_sfx_carpet_footsteps[4];
extern struct sfx_s	*cl_sfx_force_footsteps[4];
extern struct sfx_s	*cl_sfx_gravel_footsteps[4];
extern struct sfx_s	*cl_sfx_ice_footsteps[4];
extern struct sfx_s	*cl_sfx_sand_footsteps[4];
extern struct sfx_s	*cl_sfx_wood_footsteps[4];

extern struct sfx_s	*cl_sfx_slosh[4];
extern struct sfx_s	*cl_sfx_wade[4];
extern struct sfx_s	*cl_sfx_ladder[4];

/*
===============
ReadTextureSurfaceAssignments
Reads in defintions for footsteps based on texture name.
===============
*/
qboolean 
buf_gets (char *dest, int destsize, char **f)
{
	char *old = *f;
	*f = strchr (old, '\n');
	if (!*f)
	{	// no more new lines
		*f = old + strlen(old);
		if (!strlen(*f))
			return false;	// end of file, nothing else to grab
	}
	(*f)++; // advance past EOL
	Q_strncpyz(dest, old, min(destsize, (int)(*f-old)));
	return true;
}

void 
ReadTextureSurfaceAssignments()
{
	char	filename[MAX_OSPATH];
	char	*footstep_data;
	char	*parsedata;
	char	line[80];

	num_texsurfs = 0;

	Com_sprintf (filename, sizeof(filename), "scripts/texsurfs.txt");
	
	FS_LoadFile (filename, (void **)&footstep_data);
	
	parsedata = footstep_data;
	
	if (!footstep_data) 
		return;
	
	while (buf_gets(line, sizeof(line), &parsedata) && num_texsurfs < MAX_TEX_SURF) {
		sscanf(line,"%d %s",&tex_surf[num_texsurfs].step_id,tex_surf[num_texsurfs].tex);
		/* Com_Printf("%d %s\n",tex_surf[num_texsurfs].step_id,tex_surf[num_texsurfs].tex); */
		num_texsurfs++;
	}
	
	FS_FreeFile (footstep_data);
}


/*
===============
CL_FootSteps
Plays appropriate footstep sound depending on surface flags of the ground surface.
Since this is a replacement for plain Jane EV_FOOTSTEP, we already know
the player is definitely on the ground when this is called.
===============
*/
void 
CL_FootSteps (entity_state_t *ent, qboolean loud)
{
	trace_t		tr;
	vec3_t		end;
	int		r;
	int		surface;
	struct	sfx_s	*stepsound = NULL;
	float	volume = cl_footsteps_volume->value;

	r = (rand()&3);

	VectorCopy(ent->origin,end);
	end[2] -= 64;
	
	tr = CL_PMSurfaceTrace (ent->number, ent->origin,NULL,NULL,end,MASK_SOLID | MASK_WATER);
	
	if (!tr.surface)
		return;
	
	surface = tr.surface->flags & SURF_STEPMASK;
	
	switch (surface)
	{
	case SURF_METAL:
		stepsound = cl_sfx_metal_footsteps[r];
		break;
	case SURF_DIRT:
		stepsound = cl_sfx_dirt_footsteps[r];
		break;
	case SURF_VENT:
		stepsound = cl_sfx_vent_footsteps[r];
		break;
	case SURF_GRATE:
		stepsound = cl_sfx_grate_footsteps[r];
		break;
	case SURF_TILE:
		stepsound = cl_sfx_tile_footsteps[r];
		break;
	case SURF_GRASS:
		stepsound = cl_sfx_grass_footsteps[r];
		break;
	case SURF_SNOW:
		stepsound = cl_sfx_snow_footsteps[r];
		break;
	case SURF_CARPET:
		stepsound = cl_sfx_carpet_footsteps[r];
		break;
	case SURF_FORCE:
		stepsound = cl_sfx_force_footsteps[r];
		break;
	case SURF_GRAVEL:
		stepsound = cl_sfx_gravel_footsteps[r];
		break;
	case SURF_ICE:
		stepsound = cl_sfx_ice_footsteps[r];
		break;
	case SURF_SAND:
		stepsound = cl_sfx_sand_footsteps[r];
		break;
	case SURF_WOOD:
		stepsound = cl_sfx_wood_footsteps[r];
		break;
	case SURF_STANDARD:
		stepsound = cl_sfx_footsteps[r];
		volume = 1.0;
		break;
	default:
		if (cl_footsteps_override->value && num_texsurfs)
		{
			int	i;
			for (i=0; i<num_texsurfs; i++)
				if (strstr(tr.surface->name,tex_surf[i].tex) && tex_surf[i].step_id > 0)
				{
					tr.surface->flags |= (SURF_METAL << (tex_surf[i].step_id - 1));
					CL_FootSteps (ent, loud); // start over
					return;
				}
		}
		tr.surface->flags |= SURF_STANDARD;
		CL_FootSteps (ent, loud); // start over
		return;
	}

	if (loud)
	{
		if (volume == 1.0)
			S_StartSound (NULL, ent->number, CHAN_AUTO, stepsound, 1.0, ATTN_NORM, 0);
		else
			volume = 1.0;
	}
	
	S_StartSound (NULL, ent->number, CHAN_BODY, stepsound, volume, ATTN_NORM, 0);
}
//end Knightmare

void
CL_EntityEvent(entity_state_t * ent)
{
	switch (ent->event) {
		case EV_ITEM_RESPAWN:
		S_StartSound(NULL, ent->number, CHAN_WEAPON, S_RegisterSound("items/respawn1.wav"), 1, ATTN_IDLE, 0);
		CL_ItemRespawnParticles(ent->origin);
		break;
	case EV_PLAYER_TELEPORT:
		S_StartSound(NULL, ent->number, CHAN_WEAPON, S_RegisterSound("misc/tele1.wav"), 1, ATTN_IDLE, 0);
		CL_TeleportParticles(ent->origin);
		break;
	case EV_FOOTSTEP:
		if (cl_footsteps->value)
			CL_FootSteps (ent, false);
		break;
	case EV_LOUDSTEP:
		if (cl_footsteps->value)
			CL_FootSteps (ent, true);
		break;
	case EV_FALLSHORT:
		S_StartSound(NULL, ent->number, CHAN_AUTO, S_RegisterSound("player/land1.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALL:
		S_StartSound(NULL, ent->number, CHAN_AUTO, S_RegisterSound("*fall2.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALLFAR:
		S_StartSound(NULL, ent->number, CHAN_AUTO, S_RegisterSound("*fall1.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_SLOSH:
		S_StartSound (NULL, ent->number, CHAN_BODY, cl_sfx_slosh[rand()&3], 0.5, ATTN_NORM, 0);
		break;
	case EV_WADE:
		S_StartSound (NULL, ent->number, CHAN_BODY, cl_sfx_wade[rand()&3], 0.5, ATTN_NORM, 0);
		break;
	case EV_CLIMB_LADDER:
		S_StartSound (NULL, ent->number, CHAN_BODY, cl_sfx_ladder[rand()&3], 0.5, ATTN_NORM, 0);
		break;
	}
}

/*
 * ============== CL_ClearEffects
 *
 * ==============
 */
void
CL_ClearEffects(void)
{
	CL_ClearParticles();
	CL_ClearDlights();
	CL_ClearLightStyles();
}
