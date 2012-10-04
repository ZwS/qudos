/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
--------------------------------------------------------------
The ACE Bot is a product of Steve Yeager, and is available from
the ACE Bot homepage, at http://www.axionfx.com/ace.

This program is a modification of the ACE Bot, and is therefore
in NO WAY supported by Steve Yeager.
*/

#include "../g_local.h"
#include "../m_player.h"
#include "ai_local.h"


//==========================================
// AIDebug_ToogleBotDebug
//==========================================
void AIDebug_ToogleBotDebug(void)
{
/*	if (AIDevel.debugMode || !sv_cheats->integer )
	{
//		G_Printf ("BOT: Debug Mode Off\n");
		AIDevel.debugMode = false;
		return;
	}

	//Activate debug mode
	G_Printf ("\n======================================\n");
	G_Printf ("--==[ D E B U G ]==--\n");
	G_Printf ("======================================\n");
	G_Printf ("'addnode [nodetype]' -- Add [specified] node to players current location\n");
	G_Printf ("'movenode [node] [x y z]' -- Move [node] to [x y z] coordinates\n");
	G_Printf ("'findnode' -- Finds closest node\n");
	G_Printf ("'removelink [node1 node2]' -- Removes link between two nodes\n");
	G_Printf ("'addlink [node1 node2]' -- Adds a link between two nodes\n");
	G_Printf ("======================================\n\n");

	G_Printf ("BOT: Debug Mode On\n");

	AIDevel.debugMode = true;
*/
}


//==========================================
// AIDebug_SetChased
// Theorically, only one client
//	at the time will chase. Otherwise it will
//	be a really fucked up situation.
//==========================================
void AIDebug_SetChased(edict_t *ent)
{
/*	int i;
	AIDevel.chaseguy = NULL;
	AIDevel.debugChased = false;

	if (!AIDevel.debugMode || !sv_cheats->integer)
		return;

	//find if anyone is chasing this bot
	for(i=0;i<game.maxclients+1;i++)
	{
//		AIDevel.chaseguy = game.edicts + i + 1;
		AIDevel.chaseguy = g_edicts + i + 1;
		if(AIDevel.chaseguy->inuse && AIDevel.chaseguy->client){
			if( AIDevel.chaseguy->client->chase_target &&
				AIDevel.chaseguy->client->chase_target == ent)
				break;
		}
		AIDevel.chaseguy = NULL;
	}

	if (!AIDevel.chaseguy)
		return;

	AIDevel.debugChased = true;
*/
}



//=======================================================================
//							NODE TOOLS	
//=======================================================================



//==========================================
// AITools_DrawLine
// Just so I don't hate to write the event every time
//==========================================
void AITools_DrawLine(vec3_t origin, vec3_t dest)
{
/*
	edict_t		*event;
	
	event = G_SpawnEvent ( EV_BFG_LASER, 0, origin );
	event->svflags = SVF_FORCEOLDORIGIN;
	VectorCopy ( dest, event->s.origin2 );
*/
}


//==========================================
// AITools_DrawPath
// Draws the current path (floods as hell also)
//==========================================
static int	drawnpath_timeout;
void AITools_DrawPath(edict_t *self, int node_from, int node_to)
{
/*
	int			count = 0;
	int			pos = 0;

	//don't draw it every frame (flood)
	if (level.time < drawnpath_timeout)
		return;
	drawnpath_timeout = level.time + 4*FRAMETIME;

	if( self->ai.path->goalNode != node_to )
		return;

	//find position in stored path
	while( self->ai.path->nodes[pos] != node_from )
	{
		pos++;
		if( self->ai.path->goalNode == self->ai.path->nodes[pos] )
			return;	//failed
	}

	// Now set up and display the path
	while( self->ai.path->nodes[pos] != node_to && count < 32)
	{
		edict_t		*event;
		
		event = G_SpawnEvent ( EV_BFG_LASER, 0, nodes[self->ai.path->nodes[pos]].origin );
		event->svflags = SVF_FORCEOLDORIGIN;
		VectorCopy ( nodes[self->ai.path->nodes[pos+1]].origin, event->s.origin2 );		
		pos++;
		count++;
	}
*/
}

//==========================================
// AITools_ShowPlinks
// Draws lines from the current node to it's plinks nodes
//==========================================
static int	debugdrawplinks_timeout;
void AITools_ShowPlinks( void )
{
/*	int		current_node;
	int		plink_node;
	int		i;

	if (AIDevel.plinkguy == NULL)
		return;

	//don't draw it every frame (flood)
	if (level.time < debugdrawplinks_timeout)
		return;
	debugdrawplinks_timeout = level.time + 4*FRAMETIME;

	//do it
	current_node = AI_FindClosestReachableNode(AIDevel.plinkguy,NODE_DENSITY*3,NODE_ALL);
	if (!pLinks[current_node].numLinks)
		return;

	for (i=0; i<nav.num_items;i++){
		if (nav.items[i].node == current_node){
			if( !nav.items[i].ent->classname )
				G_CenterPrintMsg(AIDevel.plinkguy, "no classname");
			else
				G_CenterPrintMsg(AIDevel.plinkguy, "%s", nav.items[i].ent->classname);
			break;
		}
	}

	for (i=0; i<pLinks[current_node].numLinks; i++) 
	{
		plink_node = pLinks[current_node].nodes[i];
		AITools_DrawLine(nodes[current_node].origin, nodes[plink_node].origin);
	}
*/
}


//=======================================================================
//=======================================================================


//==========================================
// AITools_Frame
// Gives think time to the debug tools found
// in this archive (those witch need it)
//==========================================
void AITools_Frame(void)
{
	//debug
	if( AIDevel.showPLinks )
		AITools_ShowPlinks();
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
