///////////////////////////////////////////////////////////////////////
//
//  ACE - Quake II Bot Base Code
//
//  Version 1.0
//
//  This file is Copyright(c), Steve Yeager 1998, All Rights Reserved
//
//
//	All other files are Copyright(c) Id Software, Inc.
//
//	Please see liscense.txt in the source directory for the copyright
//	information regarding those files belonging to Id Software, Inc.
//	
//	Should you decide to release a modified version of ACE, you MUST
//	include the following text (minus the BEGIN and END lines) in the 
//	documentation for your modification.
//
//	--- BEGIN ---
//
//	The ACE Bot is a product of Steve Yeager, and is available from
//	the ACE Bot homepage, at http://www.axionfx.com/ace.
//
//	This program is a modification of the ACE Bot, and is therefore
//	in NO WAY supported by Steve Yeager.

//	This program MUST NOT be sold in ANY form. If you have paid for 
//	this product, you should contact Steve Yeager immediately, via
//	the ACE Bot homepage.
//
//	--- END ---
//
//	I, Steve Yeager, hold no responsibility for any harm caused by the
//	use of this source code, especially to small children and animals.
//  It is provided as-is with no implied warranty or support.
//
//  I also wish to thank and acknowledge the great work of others
//  that has helped me to develop this code.
//
//  John Cricket    - For ideas and swapping code.
//  Ryan Feltrin    - For ideas and swapping code.
//  SABIN           - For showing how to do true client based movement.
//  BotEpidemic     - For keeping us up to date.
//  Telefragged.com - For giving ACE a home.
//  Microsoft       - For giving us such a wonderful crash free OS.
//  id              - Need I say more.
//  
//  And to all the other testers, pathers, and players and people
//  who I can't remember who the heck they were, but helped out.
//
///////////////////////////////////////////////////////////////////////
	
///////////////////////////////////////////////////////////////////////
//
//  acebot_ai.c -      This file contains all of the 
//                     AI routines for the ACE II bot.
//
//
// NOTE: I went back and pulled out most of the brains from
//       a number of these functions. They can be expanded on 
//       to provide a "higher" level of AI. 
////////////////////////////////////////////////////////////////////////

#include "../g_local.h"
#include "../m_player.h"

#include "acebot.h"

///////////////////////////////////////////////////////////////////////
// Main Think function for bot
///////////////////////////////////////////////////////////////////////
void ACEAI_Think (edict_t *self)
{
	usercmd_t	ucmd;

	// Set up client movement
	VectorCopy(self->client->ps.viewangles,self->s.angles);
	VectorSet (self->client->ps.pmove.delta_angles, 0, 0, 0);
	memset (&ucmd, 0, sizeof (ucmd));
	self->enemy = NULL;
	self->movetarget = NULL;
	
	// Force respawn 
	if (self->deadflag)
	{
		self->client->buttons = 0;
		ucmd.buttons = BUTTON_ATTACK;
	}
	
	if(self->state == STATE_WANDER && self->wander_timeout < level.time)
	  ACEAI_PickLongRangeGoal(self); // pick a new long range goal

	// Kill the bot if completely stuck somewhere
	if(VectorLength(self->velocity) > 37) //
		self->suicide_timeout = level.time + 10.0;

	if(self->suicide_timeout < level.time)
	{
		self->health = 0;
		player_die (self, self, self, 100000, vec3_origin);
	}
	
	// Find any short range goal
	ACEAI_PickShortRangeGoal(self);
	
	// Look for enemies
	if(ACEAI_FindEnemy(self))
	{	
		ACEAI_ChooseWeapon(self);
		ACEMV_Attack (self, &ucmd);
	}
	else
	{
		// Execute the move, or wander
		if(self->state == STATE_WANDER)
			ACEMV_Wander(self,&ucmd);
		else if(self->state == STATE_MOVE)
			ACEMV_Move(self,&ucmd);
	}
	
	//debug_printf("State: %d\n",self->state);

	// set approximate ping
	ucmd.msec = 75 + floor (random () * 25) + 1;

	// show random ping values in scoreboard
	self->client->ping = ucmd.msec;

	// set bot's view angle
	ucmd.angles[PITCH] = ANGLE2SHORT(self->s.angles[PITCH]);
	ucmd.angles[YAW] = ANGLE2SHORT(self->s.angles[YAW]);
	ucmd.angles[ROLL] = ANGLE2SHORT(self->s.angles[ROLL]);
	
	// send command through id's code
	ClientThink (self, &ucmd);
	
	self->nextthink = level.time + FRAMETIME;
}

///////////////////////////////////////////////////////////////////////
// Evaluate the best long range goal and send the bot on
// its way. This is a good time waster, so use it sparingly. 
// Do not call it for every think cycle.
///////////////////////////////////////////////////////////////////////
void ACEAI_PickLongRangeGoal(edict_t *self)
{

	int i;
	int node;
	float weight,best_weight=0.0;
	int current_node,goal_node;
	edict_t *goal_ent;
	float cost;
	
	// look for a target 
	current_node = ACEND_FindClosestReachableNode(self,NODE_DENSITY,NODE_ALL);

	self->current_node = current_node;
	
	if(current_node == -1)
	{
		self->state = STATE_WANDER;
		self->wander_timeout = level.time + 1.0;
		self->goal_node = -1;
		return;
	}

	///////////////////////////////////////////////////////
	// Items
	///////////////////////////////////////////////////////
	for(i=0;i<num_items;i++)
	{
		if(item_table[i].ent == NULL || item_table[i].ent->solid == SOLID_NOT) // ignore items that are not there.
			continue;
		
		cost = ACEND_FindCost(current_node,item_table[i].node);
		
		if(cost == INVALID || cost < 2) // ignore invalid and very short hops
			continue;
	
		weight = ACEIT_ItemNeed(self, item_table[i].item);

		/*// If I am on team one and I have the flag for the other team....return it
		if(ctf->value && (item_table[i].item == ITEMLIST_FLAG2 || item_table[i].item == ITEMLIST_FLAG1) &&
		  (self->client->resp.ctf_team == CTF_TEAM1 && self->client->pers.inventory[ITEMLIST_FLAG2] ||
		   self->client->resp.ctf_team == CTF_TEAM2 && self->client->pers.inventory[ITEMLIST_FLAG1]))
			weight = 10.0;*/

		weight *= random(); // Allow random variations
		weight /= cost; // Check against cost of getting there
				
		if(weight > best_weight)
		{
			best_weight = weight;
			goal_node = item_table[i].node;
			goal_ent = item_table[i].ent;
		}
	}

	///////////////////////////////////////////////////////
	// Players
	///////////////////////////////////////////////////////
	// This should be its own function and is for now just
	// finds a player to set as the goal.
	for(i=0;i<num_players;i++)
	{
		if(players[i] == self)
			continue;

		node = ACEND_FindClosestReachableNode(players[i],NODE_DENSITY,NODE_ALL);
		cost = ACEND_FindCost(current_node, node);

		if(cost == INVALID || cost < 3) // ignore invalid and very short hops
			continue;

		/*// Player carrying the flag?
		if(ctf->value && (players[i]->client->pers.inventory[ITEMLIST_FLAG2] || players[i]->client->pers.inventory[ITEMLIST_FLAG1]))
		  weight = 2.0;
		else
		  weight = 0.3; */
		
		weight *= random(); // Allow random variations
		weight /= cost; // Check against cost of getting there
		
		if(weight > best_weight)
		{		
			best_weight = weight;
			goal_node = node;
			goal_ent = players[i];
		}	
	}

	// If do not find a goal, go wandering....
	if(best_weight == 0.0 || goal_node == INVALID)
	{
		self->goal_node = INVALID;
		self->state = STATE_WANDER;
		self->wander_timeout = level.time + 1.0;
		if(debug_mode)
			debug_printf("%s did not find a LR goal, wandering.\n",self->client->pers.netname);
		return; // no path? 
	}
	
	// OK, everything valid, let's start moving to our goal.
	self->state = STATE_MOVE;
	self->tries = 0; // Reset the count of how many times we tried this goal
	 
	if(goal_ent != NULL && debug_mode)
		debug_printf("%s selected a %s at node %d for LR goal.\n",self->client->pers.netname, goal_ent->classname, goal_node);

	ACEND_SetGoal(self,goal_node);

}

///////////////////////////////////////////////////////////////////////
// Pick best goal based on importance and range. This function
// overrides the long range goal selection for items that
// are very close to the bot and are reachable.
///////////////////////////////////////////////////////////////////////
void ACEAI_PickShortRangeGoal(edict_t *self)
{
	edict_t *target;
	float weight,best_weight=0.0;
	edict_t *best;
	int index;
	
	// look for a target (should make more efficent later)
	target = findradius(NULL, self->s.origin, 200);
	
	while(target)
	{
		if(target->classname == NULL)
			return;
		
		// Missle avoidance code
		// Set our movetarget to be the rocket or grenade fired at us. 
		if(strcmp(target->classname,"rocket")==0 || strcmp(target->classname,"grenade")==0)
		{
			if(debug_mode) 
				debug_printf("ROCKET ALERT!\n");

			self->movetarget = target;
			return;
		}
	
		if (ACEIT_IsReachable(self,target->s.origin))
		{
			if (infront(self, target))
			{
				index = ACEIT_ClassnameToIndex(target->classname);
				weight = ACEIT_ItemNeed(self, index);
				
				if(weight > best_weight)
				{
					best_weight = weight;
					best = target;
				}
			}
		}

		// next target
		target = findradius(target, self->s.origin, 200);
	}

	if(best_weight)
	{
		self->movetarget = best;
		
		if(debug_mode && self->goalentity != self->movetarget)
			debug_printf("%s selected a %s for SR goal.\n",self->client->pers.netname, self->movetarget->classname);
		
		self->goalentity = best;

	}

}

///////////////////////////////////////////////////////////////////////
// Scan for enemy (simplifed for now to just pick any visible enemy)
///////////////////////////////////////////////////////////////////////
qboolean ACEAI_FindEnemy(edict_t *self)
{
	int i;
	
	for(i=0;i<=num_players;i++)
	{
		if(players[i] == NULL || players[i] == self || 
		   players[i]->solid == SOLID_NOT)
		   continue;
	
		/*if(ctf->value && 
		   self->client->resp.ctf_team == players[i]->client->resp.ctf_team)
		   continue;*/

		if(!players[i]->deadflag && visible(self, players[i]) && gi.inPVS (self->s.origin, players[i]->s.origin))
		{
			self->enemy = players[i];
			return true;
		}
	}

	return false;
  
}

///////////////////////////////////////////////////////////////////////
// Hold fire with RL/BFG?
///////////////////////////////////////////////////////////////////////
qboolean ACEAI_CheckShot(edict_t *self)
{
	trace_t tr;

	tr = gi.trace (self->s.origin, tv(-8,-8,-8), tv(8,8,8), self->enemy->s.origin, self, MASK_OPAQUE);
	
	// Blocked, do not shoot
	if (tr.fraction != 1.0)
		return false; 
	
	return true;
}

///////////////////////////////////////////////////////////////////////
// Choose the best weapon for bot (simplified)
///////////////////////////////////////////////////////////////////////
void ACEAI_ChooseWeapon(edict_t *self)
{	
	float range;
	vec3_t v;
	
	// if no enemy, then what are we doing here?
	if(!self->enemy)
		return;
	
	// always favor the railgun
	if(ACEIT_ChangeWeapon(self,FindItem("railgun")))
		return;

	// Base selection on distance.
	VectorSubtract (self->s.origin, self->enemy->s.origin, v);
	range = VectorLength(v);
		
	// Longer range 
	if(range > 300)
	{
		// choose BFG if enough ammo
		if(self->client->pers.inventory[ITEMLIST_CELLS] > 50)
			if(ACEAI_CheckShot(self) && ACEIT_ChangeWeapon(self, FindItem("bfg10k")))
				return;

		if(ACEAI_CheckShot(self) && ACEIT_ChangeWeapon(self,FindItem("rocket launcher")))
			return;
	}
	
	// Only use GL in certain ranges and only on targets at or below our level
	if(range > 100 && range < 500 && self->enemy->s.origin[2] - 20 < self->s.origin[2])
		if(ACEIT_ChangeWeapon(self,FindItem("grenade launcher")))
			return;

	if(ACEIT_ChangeWeapon(self,FindItem("hyperblaster")))
		return;
	
	// Only use CG when ammo > 50
	if(self->client->pers.inventory[ITEMLIST_BULLETS] >= 50)
		if(ACEIT_ChangeWeapon(self,FindItem("chaingun")))
			return;
	
	if(ACEIT_ChangeWeapon(self,FindItem("machinegun")))
		return;
	
	if(ACEIT_ChangeWeapon(self,FindItem("super shotgun")))
		return;
	
	if(ACEIT_ChangeWeapon(self,FindItem("shotgun")))
   	   return;
	
	if(ACEIT_ChangeWeapon(self,FindItem("blaster")))
   	   return;
	
	return;

}

//========================================================
//============= BOT TALKING/TAUNTING ROUTINES ============
//========================================================
//=====================================================
// Returns Player with Highest Score.
//=====================================================
edict_t *BestScoreEnt(void) {
	
	edict_t *bestplayer=NULL;
	int i, bestscore=-999;
	edict_t *ent;

	// Search thru all clients
	for(i=0;i < game.maxclients; i++) {
		ent=g_edicts+i+1;
//	if (!G_EntExists(ent)) continue;
		if (ent->client->resp.score > bestscore) {
 			bestplayer=ent; // Found one!
 			bestscore=ent->client->resp.score; 
		} 
	}
	return bestplayer;
}



//=======================================================
// Taunt your victim! Called from ClientObituary()..
//=======================================================
void bTaunt(edict_t *bot, edict_t *other) {

	if ((rand()%5) >= 2) 
		return;

	if (level.time < bot->last_taunt) 
		return;

	// If killed enemy then Taunt them!!
	if ((other->client) && (random() < 0.4))
 		switch (rand()%4) {
 			case 0: // flipoff
 				bot->s.frame = FRAME_flip01-1;
 				bot->client->anim_end = FRAME_flip12;
 				break;
 			case 1: // salute
 				bot->s.frame = FRAME_salute01-1;
 				bot->client->anim_end = FRAME_salute11;
 				break;
 			case 2: // taunt
 				bot->s.frame = FRAME_taunt01-1;
 				bot->client->anim_end = FRAME_taunt17;
 				break;
 			case 3: // point
 				bot->s.frame = FRAME_point01-1;
 				bot->client->anim_end = FRAME_point12;
 				break; 
		}

// Taunt victim but not too often..
	bot->last_taunt = level.time + 60 + 35;
}


//========================================================
void bFakeChat(edict_t *bot) {
	
	gclient_t *bclient=bot->client;

	if (random() < .1)
 		gi.bprintf(PRINT_CHAT, "%s: Bunch of Chicken Shits!\n", bclient->pers.netname);
 	else if (random() < .2)
 		gi.bprintf(PRINT_CHAT, "%s: Tu madre!!!\n", bclient->pers.netname);
 	else if (random() < .3)
 		gi.bprintf(PRINT_CHAT, "%s: Who wants a piece of me?\n", bclient->pers.netname);
 	else if (random() < .4)
 		gi.bprintf(PRINT_CHAT, "%s: Where'd everybody go?\n", bclient->pers.netname);
 	else if (random() < .5)
 		gi.bprintf(PRINT_CHAT, "%s: Yeee pendejos venid por mi! pateare vuestro gordo culo\n", bclient->pers.netname);
 	else
 		gi.bprintf(PRINT_CHAT, "%s: Kickin' Ass!\n", bclient->pers.netname);

// Random chats between 2 minutes and 10 minutes
	bot->last_chat = level.time + 120 + (60*(rand()%8));
}





//========================================================
// Insult the player that the bot just fragged...
//========================================================
void bInsult(edict_t *bot, edict_t *loser) {

	gclient_t *bclient=bot->client;
	gclient_t *lclient=loser->client;

	if ((rand()%5) > 3) 
		return;

	if (level.time < bot->last_insult) 
		return;

	if (bclient->resp.score < lclient->resp.score) {
	
 		if (bclient->resp.score < lclient->resp.score - 20) {
 			if (random() < .1)
 				gi.bprintf(PRINT_CHAT, "%s: Heh... I'm all luck, %s\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .2)
 				gi.bprintf(PRINT_CHAT, "%s: WHEW! Finally got ya, %s!\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .3)
 				gi.bprintf(PRINT_CHAT, "%s: I...I killed %s? I...don't remember... it all happened so fast!\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .4)
 				gi.bprintf(PRINT_CHAT, "%s: Only pussies on this server!\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .5)
 				gi.bprintf(PRINT_CHAT, "%s: Sure, I'm losing by a ton, but does that mean I suck? Probably.\n", bclient->pers.netname);
 			else
 				gi.bprintf(PRINT_CHAT, "%s: Not bad for a beginner, eh %s?\n", bclient->pers.netname, lclient->pers.netname); 
		}
 		else if (bclient->resp.score < lclient->resp.score - 10) {
	
 			if (random() < .1)
 				gi.bprintf(PRINT_CHAT, "%s: Well, %s, what can I say? You're good... but not good enough\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .2)
 				gi.bprintf(PRINT_CHAT, "%s: I'll get you %s, and your little dog, too\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .3)
 				gi.bprintf(PRINT_CHAT, "%s: Oh, I get how you play now, %s... you're mine.\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .4)
 				gi.bprintf(PRINT_CHAT, "%s: What's that, %s? Do I smell smoke?\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .5)
 				gi.bprintf(PRINT_CHAT, "%s: Oops! Sorry %s, You REALLY suck!\n", bclient->pers.netname, lclient->pers.netname);
 			else
 				gi.bprintf(PRINT_CHAT, "%s: YEAH BABY, YEAH!\n", bclient->pers.netname); 
		}
 		else if (bclient->resp.score < lclient->resp.score - 5) {
 			if (random() < .1)
 				gi.bprintf(PRINT_CHAT, "%s: Ok, %s, I'm back on track now\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .2)
 				gi.bprintf(PRINT_CHAT, "%s: You aren't gonna win THAT easy, %s.\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .3)
 				gi.bprintf(PRINT_CHAT, "%s: Umm, ok %s, I'd appreciate it if you could not bleed on my clothes next time.\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .4)
 				gi.bprintf(PRINT_CHAT, "%s: You might wanna get that fixed, %s\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .5)
 				gi.bprintf(PRINT_CHAT, "%s: Uh oh... BRB, I have to clean this %s off my shirt before it sets in.\n", bclient->pers.netname, lclient->pers.netname);
 			else
 				gi.bprintf(PRINT_CHAT, "%s: hiiiiiihaaaaaa\n", bclient->pers.netname, lclient->pers.netname); 
		}
 		else {
 			if (random() < .1)
 				gi.bprintf(PRINT_CHAT, "%s: I can still catch up with you, %s, don't get cocky\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .2)
 				gi.bprintf(PRINT_CHAT, "%s: You're alllll mine, %s.\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .3)
 				gi.bprintf(PRINT_CHAT, "%s: Come on, %s, just you and me.\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .4)
 				gi.bprintf(PRINT_CHAT, "%s: The best part of wakin' uuup is %s gibs in your cuuup!\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .5)
 				gi.bprintf(PRINT_CHAT, "%s: Oh my, %s, that didn't look like it felt very nice.\n", bclient->pers.netname, lclient->pers.netname);
 			else
 				gi.bprintf(PRINT_CHAT, "%s: Well, %s, looks like things might even up.\n", bclient->pers.netname, lclient->pers.netname); 
		}
 	}
 	else if (bclient->resp.score > lclient->resp.score) {
 		if (bclient->resp.score > lclient->resp.score + 10) {
 			if (random() < .1)
 				gi.bprintf(PRINT_CHAT, "%s: You're never going to catch up to me, %s. Just give up.\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .2)
 				gi.bprintf(PRINT_CHAT, "%s: Hey %s, have you tried reading one of those ""DeathMatch for Dummies"" books?\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .3)
 				gi.bprintf(PRINT_CHAT, "%s: Oh %s, you make me feel so... alive!\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .4)
 				gi.bprintf(PRINT_CHAT, "%s: Me? Using a bot? No way %s, I'm all skill!\n", bclient->pers.netname, lclient->pers.netname);
 			else if (random() < .5)
 				gi.bprintf(PRINT_CHAT, "%s: Hey %s, are you letting your mom play again?\n", bclient->pers.netname, lclient->pers.netname);
 			else
 				gi.bprintf(PRINT_CHAT, "%s: You do know there's an autorun option, don't you?\n", bclient->pers.netname, lclient->pers.netname); 
		}
 	else if (bclient->resp.score > lclient->resp.score + 5) {
 		if (random() < .1)
 			gi.bprintf(PRINT_CHAT, "%s: HeeeeHaaaaa\n", bclient->pers.netname);
 		else if (random() < .2)
 			gi.bprintf(PRINT_CHAT, "%s: Don't feel bad %s, you just aren't gifted like me\n", bclient->pers.netname, lclient->pers.netname);
 		else if (random() < .3)
 			gi.bprintf(PRINT_CHAT, "%s: Come on %s, don't give up now!\n", bclient->pers.netname, lclient->pers.netname);
 		else if (random() < .4)
 			gi.bprintf(PRINT_CHAT, "%s: I think you just need to practice more, %s. muhhhhaahhhaaa\n", bclient->pers.netname, lclient->pers.netname);
 		else if (random() < .5)
 			gi.bprintf(PRINT_CHAT, "%s: I just want you to know that I think you're taking this beating very well, %s\n", bclient->pers.netname, lclient->pers.netname);
 		else
 			gi.bprintf(PRINT_CHAT, "%s: Is that freshly cooked whupass I smell, %s?\n", bclient->pers.netname, lclient->pers.netname); }
 	else {
 		if (random() < .1)
 			gi.bprintf(PRINT_CHAT, "%s: Come on, %s, ajajajaaaaaaja I can take you\n", bclient->pers.netname, lclient->pers.netname);
 		else if (random() < .2)
 			gi.bprintf(PRINT_CHAT, "%s: You're goin' down, %s.\n", bclient->pers.netname, lclient->pers.netname);
 		else if (random() < .3)
 			gi.bprintf(PRINT_CHAT, "%s: Oh, so %s, you want some?\n", bclient->pers.netname, lclient->pers.netname);
 		else if (random() < .4)
 			gi.bprintf(PRINT_CHAT, "%s: That's right %s, you know who yo daddy is\n", bclient->pers.netname, lclient->pers.netname);
 		else if (random() < .5)
 			gi.bprintf(PRINT_CHAT, "%s: Better get that taken care of, %s. It could get infected.\n", bclient->pers.netname, lclient->pers.netname);
 		else
 			gi.bprintf(PRINT_CHAT, "%s: Don't ya just love it?\n", bclient->pers.netname, lclient->pers.netname); }
 	}
 	else {
 		if (random() < .1)
 			gi.bprintf(PRINT_CHAT, "%s: Oh look, a tie! Well %s, we'll just have to fix that!\n", bclient->pers.netname, lclient->pers.netname);
 		else if (random() < .2)
 			gi.bprintf(PRINT_CHAT, "%s: Time to pay your pimp, %s\n", bclient->pers.netname, lclient->pers.netname);
 		else if (random() < .3)
 			gi.bprintf(PRINT_CHAT, "%s: Come on %s, it's time to ride daddy's rocket\n", bclient->pers.netname, lclient->pers.netname);
 		else if (random() < .4)
 			gi.bprintf(PRINT_CHAT, "%s: Look %s, we're tied... want me to fix that? Ok then!\n", bclient->pers.netname, lclient->pers.netname);
 		else if (random() < .5)
 			gi.bprintf(PRINT_CHAT, "%s: Damn %s, I thought you were better than this\n", bclient->pers.netname, lclient->pers.netname);
 		else
 			gi.bprintf(PRINT_CHAT, "%s: Alright, %s, I'm not showing any mercy this time.\n", bclient->pers.netname, lclient->pers.netname);
	}

// Next insult between 30 sec and 5 minutes
	bot->last_insult = level.time + 30 + (60*(rand()%5));
}

