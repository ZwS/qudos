/* b_coopcam.h
 */
/* blinky
 */
/* Created: 2000/06
 */
/* Edited:  2000/07/17
 */
/* 
 */

struct BlinkyClient_s 
{
	
	edict_t * cam_target;
	edict_t * cam_decoy;	/* if camming, this is our sub */
        
	/* save some stuff when camming (to restore correctly) */
	int		runrun;	               
	float		save_fov;
	int		save_hand;	               	               
	int		nosummon;	/* disallow summons */
	int		nopickup;	               
};

typedef struct BlinkyClient_s BlinkyClient_t;

void		Blinky_BeginClientThink(edict_t * ent, usercmd_t * ucmd);
void		Blinky_EndClientThink(edict_t * ent, usercmd_t * ucmd);
void		Blinky_BeginRunFrame();
void		Blinky_ClientEndServerFrame(edict_t * ent);
void		Cmd_Cam_f (edict_t * ent);
void		Cmd_Stats_f(edict_t * ent);
void		Cmd_Summon_f(edict_t * ent);
void		Cmd_NoSummon_f(edict_t * ent);
void		Cmd_Runrun_f(edict_t * ent);
void		Blinky_OnClientTerminate(edict_t * self);
void		Blinky_CalcViewOffsets(edict_t * ent, vec3_t v);
void		Blinky_SpawnEntities();


