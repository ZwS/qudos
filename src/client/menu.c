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
#include "client.h"
#include "qmenu.h"

#define	NUM_CURSOR_FRAMES	15
#define	MOUSEBUTTON1		0
#define	MOUSEBUTTON2		1

static char    *menu_in_sound = "misc/menu1.wav";
static char    *menu_move_sound = "misc/menu2.wav";
static char    *menu_out_sound = "misc/menu3.wav";

static int	m_main_cursor;

qboolean	m_entersound;	/* Play after drawing a frame, so caching won't disrupt the sound. */

void            (*m_drawfunc) (void);
const char     *(*m_keyfunc) (int key);

void		M_Menu_Main_f(void);
void		M_Menu_Game_f(void);
void		M_Menu_LoadGame_f(void);
void		M_Menu_SaveGame_f(void);
void		M_Menu_PlayerConfig_f(void);
void		M_Menu_DownloadOptions_f(void);
void		M_Menu_Credits_f(void);
void		M_Menu_Multiplayer_f(void);
void		M_Menu_JoinServer_f(void);
void		M_Menu_AddressBook_f(void);
void		M_Menu_StartServer_f(void);
void		M_Menu_DMOptions_f(void);
void		M_Menu_Video_f(void);
void		M_Menu_Options_f(void);
void		M_Menu_Keys_f(void);
void		M_Menu_Quit_f(void);
void		M_Menu_Credits(void);

/*
 * =======================================================================
 *
 * SUPPORT ROUTINES
 *
 * =======================================================================
 */

#define	MAX_MENU_DEPTH		8

typedef struct {
	void            (*draw)(void);
	const char     *(*key)(int k);
} menulayer_t;

int		m_menudepth;
menulayer_t	m_layers[MAX_MENU_DEPTH];

void
refreshCursorButtons(void)
{
	cursor.buttonused[MOUSEBUTTON2] = true;
	cursor.buttonclicks[MOUSEBUTTON2] = 0;
	cursor.buttonused[MOUSEBUTTON1] = true;
	cursor.buttonclicks[MOUSEBUTTON1] = 0;
	cursor.mousewheeldown = 0;
	cursor.mousewheelup = 0;
}

static void
M_Banner(char *name)
{
	int	w, h;

	re.DrawGetPicSize(&w, &h, name);
	re.DrawPic(viddef.width / 2 - w / 2, viddef.height / 2 - /* 110 */ 250, name, 1.0);
}

void
M_PushMenu(void (*draw) (void), const char *(*key) (int k))
{

	int		i;

	if (Cvar_VariableValue("maxclients") == 1 && Com_ServerState())
		Cvar_Set("paused", "1");

	/*
	 * If this menu is already present, drop back to that level to avoid
	 * stacking menus by hotkeys.
	 */
	for (i = 0; i < m_menudepth; i++)
		if (m_layers[i].draw == draw && m_layers[i].key == key)
			m_menudepth = i;

	if (i == m_menudepth) {
		if (m_menudepth >= MAX_MENU_DEPTH)
			Com_Error(ERR_FATAL, "M_PushMenu: MAX_MENU_DEPTH");
		m_layers[m_menudepth].draw = m_drawfunc;
		m_layers[m_menudepth].key = m_keyfunc;
		m_menudepth++;
	}
	m_drawfunc = draw;
	m_keyfunc = key;

	m_entersound = true;

	refreshCursorLink();
	refreshCursorButtons();

	cls.key_dest = key_menu;
}

void
M_ForceMenuOff(void)
{
	refreshCursorLink();

	m_drawfunc = 0;
	m_keyfunc = 0;
	cls.key_dest = key_game;
	m_menudepth = 0;
	Key_ClearStates();
	Cvar_Set("paused", "0");
}

void
M_PopMenu(void)
{
	S_StartLocalSound(menu_out_sound);
	if (m_menudepth < 1)
		Com_Error(ERR_FATAL, "M_PopMenu: depth < 1");
	m_menudepth--;

	m_drawfunc = m_layers[m_menudepth].draw;
	m_keyfunc = m_layers[m_menudepth].key;


	refreshCursorLink();
	refreshCursorButtons();

	if (!m_menudepth)
		M_ForceMenuOff();
}

const char     *
Default_MenuKey(menuframework_s * m, int key)
{
	const char     *sound = NULL;
	menucommon_s   *item;

	if (m != NULL)
		if ((item = Menu_ItemAtCursor(m)) != 0)
			if (item->type == MTYPE_FIELD)
				if (Field_Key((menufield_s *) item, key))
					return (NULL);

	switch (key) {
	case K_ESCAPE:
		M_PopMenu();
		return (menu_out_sound);
	case K_KP_UPARROW:
	case K_UPARROW:
		if (m != NULL) {
			m->cursor--;
			refreshCursorLink();
			Menu_AdjustCursor(m, -1);
			sound = menu_move_sound;
		}
		break;
	case K_TAB:
		if (m != NULL) {
			m->cursor++;
			Menu_AdjustCursor(m, 1);
			sound = menu_move_sound;
		}
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		if (m != NULL) {
			m->cursor++;
			refreshCursorLink();
			Menu_AdjustCursor(m, 1);
			sound = menu_move_sound;
		}
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		if (m != NULL) {
			Menu_SlideItem(m, -1);
			sound = menu_move_sound;
		}
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		if (m != NULL) {
			Menu_SlideItem(m, 1);
			sound = menu_move_sound;
		}
		break;
#ifdef Joystick
	case K_JOY1:
	case K_JOY2:
	case K_JOY3:
	case K_JOY4:
#endif
	case K_AUX1:
	case K_AUX2:
	case K_AUX3:
	case K_AUX4:
	case K_AUX5:
	case K_AUX6:
	case K_AUX7:
	case K_AUX8:
	case K_AUX9:
	case K_AUX10:
	case K_AUX11:
	case K_AUX12:
	case K_AUX13:
	case K_AUX14:
	case K_AUX15:
	case K_AUX16:
	case K_AUX17:
	case K_AUX18:
	case K_AUX19:
	case K_AUX20:
	case K_AUX21:
	case K_AUX22:
	case K_AUX23:
	case K_AUX24:
	case K_AUX25:
	case K_AUX26:
	case K_AUX27:
	case K_AUX28:
	case K_AUX29:
	case K_AUX30:
	case K_AUX31:
	case K_AUX32:
	case K_KP_ENTER:
	case K_ENTER:
		if (m != NULL)
			Menu_SelectItem(m);
		sound = menu_move_sound;
		break;
	}

	return (sound);
}

/*
 * M_DrawCharacter
 *
 * Draws one solid graphics character. cx and cy are in 320 * 240 coordinates,
 * and will be centered on higher res screens.
 */
void
M_DrawCharacter(int cx, int cy, int num)
{
	re.DrawChar(cx + ((viddef.width - 320) >> 1), 
	             cy + ((viddef.height - 240) >> 1), num, 256);
}

void
M_Print(int cx, int cy, char *str)
{
	while (*str != '\0') {
		M_DrawCharacter(cx, cy, (*str) + 128);
		str++;
		cx += 8;
	}
}

void
M_PrintWhite(int cx, int cy, char *str)
{
	while (*str != '\0') {
		M_DrawCharacter(cx, cy, *str);
		str++;
		cx += 8;
	}
}

void
M_DrawPic(int x, int y, char *pic)
{
	re.DrawPic(x + ((viddef.width - 320) >> 1), y + ((viddef.height - 240) >> 1), pic, 1.0);
}

/*
 * M_DrawCursor
 *
 * Draws an animating cursor with the point at x,y.  The pic will extend to the
 * left of x, and both above and below y.
 */
void
M_DrawCursor(int x, int y, int f)
{
	static qboolean	cached;
	char		cursorname[80];
	int		i;

	if (!cached) {
		for (i = 0; i < NUM_CURSOR_FRAMES; i++) {
			Com_sprintf(cursorname, sizeof(cursorname), "m_cursor%d", i);
			re.RegisterPic(cursorname);
		}
		cached = true;
	}
	Com_sprintf(cursorname, sizeof(cursorname), "m_cursor%d", f);
	re.DrawPic(x, y, cursorname, 1.0);
}

void
M_DrawTextBox(int x, int y, int width, int lines)
{
	int		cx, cy;
	int		n;

	/* Draw left side. */
	cx = x;
	cy = y;

	M_DrawCharacter(cx, cy, 1);

	for (n = 0; n < lines; n++) {
		cy += 8;
		M_DrawCharacter(cx, cy, 4);
	}

	M_DrawCharacter(cx, cy + 8, 7);

	/* Draw middle. */
	cx += 8;
	while (width > 0) {
		cy = y;
		M_DrawCharacter(cx, cy, 2);
		for (n = 0; n < lines; n++) {
			cy += 8;
			M_DrawCharacter(cx, cy, 5);
		}
		M_DrawCharacter(cx, cy + 8, 8);
		width -= 1;
		cx += 8;
	}

	/* Draw right side. */
	cy = y;
	M_DrawCharacter(cx, cy, 3);

	for (n = 0; n < lines; n++) {
		cy += 8;
		M_DrawCharacter(cx, cy, 6);
	}

	M_DrawCharacter(cx, cy + 8, 9);
}

void 
M_Main_DrawQuad(float x, float y, float alpha)
{
	extern float CalcFov(float fov_x, float w, float h);
	refdef_t refdef;
	char scratch[MAX_QPATH];
	static int yaw;
	entity_t entity;

	memset(&refdef, 0, sizeof(refdef));

	refdef.x = x;
	refdef.y = y;
	refdef.width = 60;
	refdef.height = 90;
	refdef.fov_x = 40;
	refdef.fov_y = CalcFov(refdef.fov_x, refdef.width, refdef.height);
	refdef.time = cls.realtime*0.001;

	memset(&entity, 0, sizeof( entity ) );

	Com_sprintf(scratch, sizeof(scratch), "models/items/quaddama/tris.md2");
	entity.model = re.RegisterModel(scratch);
	Com_sprintf( scratch, sizeof(scratch), "models/items/quaddama/skin.pcx");
	entity.skin = re.RegisterSkin(scratch);
	entity.flags = RF_FULLBRIGHT;
	entity.origin[0] = 80;
	entity.origin[1] = 0;
	entity.origin[2] = 0;
	VectorCopy(entity.origin, entity.oldorigin);
	entity.frame = 0;
	entity.oldframe = 0;
	entity.backlerp = 0.0;
	entity.angles[1] = yaw++;
	if ( ++yaw > 360 )
		yaw -= 360;

	refdef.areabits = 0;
	refdef.num_entities = 1;
	refdef.entities = &entity;
	refdef.lightstyles = 0;
	refdef.rdflags = RDF_NOWORLDMODEL | RDF_NOCLEAR;

	refdef.height += 4;
	re.RenderFrame(&refdef);
}


/*
 * =======================================================================
 *
 * MAIN MENU
 *
 * =======================================================================
 */

#define	MAIN_ITEMS	5

typedef struct {
	int		min[2];
	int		max[2];
	void            (*OpenMenu)(void);
} mainmenuobject_t;

char           *main_names[] = {
	"m_main_game",
	"m_main_multiplayer",
	"m_main_options",
	"m_main_video",
	"m_main_quit",
	NULL
};

int		MainMenuMouseHover;

void
findMenuCoords(int *xoffset, int *ystart, int *totalheight, int *widest)
{
	int		w, h, i;

	*totalheight = 0;
	*widest = -1;

	for (i = 0; main_names[i] != 0; i++) {
		re.DrawGetPicSize(&w, &h, main_names[i]);

		if (w > *widest)
			*widest = w;
		*totalheight += (h + 12);
	}

	*ystart = (viddef.height / 2 - 110);
	*xoffset = (viddef.width - *widest + 70) / 2;
}

void
M_Main_Draw(void)
{
	char		litname[80];
	int		i, w, h;
	int		ystart, xoffset, widest = -1, totalheight = 0;

	findMenuCoords(&xoffset, &ystart, &totalheight, &widest);

	for (i = 0; main_names[i] != 0; i++) {
		if (i != m_main_cursor)
			re.DrawPic(xoffset, ystart + i * 40 + 13, main_names[i], 1.0);
	}

	Q_strncpyz(litname, main_names[m_main_cursor], sizeof(litname));
	strncat(litname, "_sel", sizeof(litname) - strlen(litname) - 1);

	re.DrawPic(xoffset, ystart + m_main_cursor * 40 + 13, litname, 1.0);

	M_DrawCursor(xoffset - 25, ystart + m_main_cursor * 40 + 11, 
	             (int)(cls.realtime / 100) % NUM_CURSOR_FRAMES);

	/* now add top plaque */
	re.DrawGetPicSize(&w, &h, "m_main_plaque");
	re.DrawPic(xoffset - 30 - w, ystart, "m_main_plaque", 1.0);
	re.DrawPic(xoffset - 30 - w, ystart + h + 5, "m_main_logo", 1.0);
	/* FIXME
	M_Main_DrawQuad(xoffset - 45, ystart + m_main_cursor * 40 + 5, 1.0); */
}

void
addButton(mainmenuobject_t * thisObj, int index, int x, int y)
{
	int		w         , h;
	float		ratio;

	re.DrawGetPicSize(&w, &h, main_names[index]);

	if (w) {
		ratio = 32.0 / (float)h;
		h = 32;
		w *= ratio;
	}
	thisObj->min[0] = x;
	thisObj->max[0] = x + w;
	thisObj->min[1] = y;
	thisObj->max[1] = y + h;

	switch (index) {
	case 0:
		thisObj->OpenMenu = M_Menu_Game_f;
	case 1:
		thisObj->OpenMenu = M_Menu_Multiplayer_f;
	case 2:
		thisObj->OpenMenu = M_Menu_Options_f;
	case 3:
		thisObj->OpenMenu = M_Menu_Video_f;
	case 4:
		thisObj->OpenMenu = M_Menu_Quit_f;
	}
}

void
openMenuFromMain(void)
{
	switch (m_main_cursor) {
		case 0:
		M_Menu_Game_f();
		break;

	case 1:
		M_Menu_Multiplayer_f();
		break;

	case 2:
		M_Menu_Options_f();
		break;

	case 3:
		M_Menu_Video_f();
		break;

	case 4:
		M_Menu_Quit_f();
		break;
	}
}

void
CheckMainMenuMouse(void)
{
	int		ystart;
	int		xoffset;
	int		widest;
	int		totalheight;
	int		i         , oldhover;
	char           *sound = NULL;
	mainmenuobject_t buttons[MAIN_ITEMS];

	oldhover = MainMenuMouseHover;
	MainMenuMouseHover = 0;

	findMenuCoords(&xoffset, &ystart, &totalheight, &widest);
	for (i = 0; main_names[i] != 0; i++)
		addButton(&buttons[i], i, xoffset, ystart + i * 40 + 13);

	/* Exit with double click 2nd mouse button */
	if (!cursor.buttonused[MOUSEBUTTON2] && cursor.buttonclicks[MOUSEBUTTON2] == 2) {
		M_PopMenu();
		sound = menu_out_sound;
		cursor.buttonused[MOUSEBUTTON2] = true;
		cursor.buttonclicks[MOUSEBUTTON2] = 0;
	}
	for (i = MAIN_ITEMS - 1; i >= 0; i--) {
		if (cursor.x >= buttons[i].min[0] && cursor.x <= buttons[i].max[0] &&
		    cursor.y >= buttons[i].min[1] && cursor.y <= buttons[i].max[1]) {
			if (cursor.mouseaction)
				m_main_cursor = i;

			MainMenuMouseHover = 1 + i;

			if (oldhover == MainMenuMouseHover && MainMenuMouseHover - 1 == m_main_cursor &&
			    !cursor.buttonused[MOUSEBUTTON1] && cursor.buttonclicks[MOUSEBUTTON1] == 1) {
				openMenuFromMain();
				sound = menu_move_sound;
				cursor.buttonused[MOUSEBUTTON1] = true;
				cursor.buttonclicks[MOUSEBUTTON1] = 0;
			}
			break;
		}
	}

	if (!MainMenuMouseHover) {
		cursor.buttonused[MOUSEBUTTON1] = false;
		cursor.buttonclicks[MOUSEBUTTON1] = 0;
		cursor.buttontime[MOUSEBUTTON1] = 0;
	}
	cursor.mouseaction = false;

	if (sound)
		S_StartLocalSound(sound);
}

const char     *
M_Main_Key(int key)
{
	const char     *sound = menu_move_sound;

	switch (key) {
	case K_ESCAPE:
		M_PopMenu();
		break;

	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		return sound;

	case K_KP_UPARROW:
	case K_UPARROW:
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		return sound;

	case K_KP_ENTER:
	case K_ENTER:
		m_entersound = true;
		openMenuFromMain();
	}

	return NULL;
}


void
M_Menu_Main_f(void)
{
	M_PushMenu(M_Main_Draw, M_Main_Key);
}

static float
ClampCvar(float min, float max, float value)
{
	if (value < min)
		return min;
	if (value > max)
		return max;
	return value;
}

/*
 * =======================================================================
 *
 * MULTIPLAYER MENU
 *
 * =======================================================================
 */

static menuframework_s	s_multiplayer_menu;
static menuaction_s	s_join_network_server_action;
static menuaction_s	s_start_network_server_action;
static menuaction_s	s_player_setup_action;

static void
Multiplayer_MenuDraw(void)
{
	M_Banner("m_banner_multiplayer");

	Menu_AdjustCursor(&s_multiplayer_menu, 1);
	Menu_Draw(&s_multiplayer_menu);
}

static void
PlayerSetupFunc(void *unused)
{
	M_Menu_PlayerConfig_f();
}

static void
JoinNetworkServerFunc(void *unused)
{
	M_Menu_JoinServer_f();
}

static void
StartNetworkServerFunc(void *unused)
{
	M_Menu_StartServer_f();
}

void
Multiplayer_MenuInit(void)
{
	s_multiplayer_menu.x = viddef.width * 0.50 - 64;
	s_multiplayer_menu.nitems = 0;

	s_join_network_server_action.generic.type = MTYPE_ACTION;
	s_join_network_server_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_join_network_server_action.generic.x = 0;
	s_join_network_server_action.generic.y = -200;
	s_join_network_server_action.generic.name = " Join network server";
	s_join_network_server_action.generic.callback = JoinNetworkServerFunc;

	s_start_network_server_action.generic.type = MTYPE_ACTION;
	s_start_network_server_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_start_network_server_action.generic.x = 0;
	s_start_network_server_action.generic.y = -190;
	s_start_network_server_action.generic.name = " Start network server";
	s_start_network_server_action.generic.callback = StartNetworkServerFunc;

	s_player_setup_action.generic.type = MTYPE_ACTION;
	s_player_setup_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_player_setup_action.generic.x = 0;
	s_player_setup_action.generic.y = -180;
	s_player_setup_action.generic.name = " Player setup";
	s_player_setup_action.generic.callback = PlayerSetupFunc;

	Menu_AddItem(&s_multiplayer_menu, (void *)&s_join_network_server_action);
	Menu_AddItem(&s_multiplayer_menu, (void *)&s_start_network_server_action);
	Menu_AddItem(&s_multiplayer_menu, (void *)&s_player_setup_action);

	Menu_SetStatusBar(&s_multiplayer_menu, NULL);

	Menu_Center(&s_multiplayer_menu);
}

const char *
Multiplayer_MenuKey(int key)
{
	return (Default_MenuKey(&s_multiplayer_menu, key));
}

void
M_Menu_Multiplayer_f(void)
{
	Multiplayer_MenuInit();
	M_PushMenu(Multiplayer_MenuDraw, Multiplayer_MenuKey);
}

/*
 * =======================================================================
 *
 * KEYS MENU
 *
 * =======================================================================
 */

char           *bindnames[][2] =
{
	{"+attack", "Attack"},
	{"weapnext", "Next weapon"},
	{"weapprev", "Previous weapon"},
	{"MENUSPACE", ""},
	{"+forward", "Walk forward"},
	{"+back", "Backpedal"},
	{"+moveleft", "Step left"},
	{"+moveright", "Step right"},
	{"+left", "Turn left"},
	{"+right", "Turn right"},
	{"MENUSPACE", ""},
	{"+speed", "Run"},
	{"+strafe", "Sidestep"},
	{"+lookup", "Look up"},
	{"+lookdown", "Look down"},
	{"centerview", "Center view"},
	{"MENUSPACE", ""},
	{"+mlook", "Mouse look"},
	{"+klook", "Keyboard look"},
	{"+moveup", "Up / Jump"},
	{"+movedown", "Down / Crouch"},
	{"MENUSPACE", ""},
	{"inven", "Inventory"},
	{"invuse", "Use item"},
	{"invdrop", "Drop item"},
	{"invprev", "Prev item"},
	{"invnext", "Next item"},
	{"MENUSPACE", ""},
	{"flashlight_eng", "Flashlight"},
	{"MENUSPACE", ""},
	{"cmd help", "Help computer"},
#ifdef GAME_MOD
	{"MENUSPACE", ""},
	{"helpmin", "Help computer min"},
	{"score", "Score board"},
#endif
	{0, 0}
};

int		keys_cursor;
static int	bind_grab;

static menuframework_s	s_keys_menu;
static menuaction_s	s_keys_binds[64];

static void
M_UnbindCommand(char *command)
{
	int		j;
	int		l;
	char           *b;

	l = strlen(command);

	for (j = 0; j < 256; j++) {
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp(b, command, l))
			Key_SetBinding(j, "");
	}
}

static void
M_FindKeysForCommand(char *command, int *twokeys)
{
	int		count;
	int		j;
	int		l;
	char           *b;

	twokeys[0] = twokeys[1] = -1;
	l = strlen(command);
	count = 0;

	for (j = 0; j < 256; j++) {
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp(b, command, l)) {
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
}

static void
KeyCursorDrawFunc(menuframework_s * menu)
{
	if (bind_grab)
		re.DrawChar(menu->x, menu->y + menu->cursor * 9, '=', 256);
	else
		re.DrawChar(menu->x, menu->y + menu->cursor * 9, 12 + ((int)(Sys_Milliseconds() / 250) & 1), 255);
}

static void
DrawKeyBindingFunc(void *self)
{
	int		keys       [2];
	menuaction_s   *a = (menuaction_s *) self;

	M_FindKeysForCommand(bindnames[a->generic.localdata[0]][0], keys);

	if (keys[0] == -1) {
		Menu_DrawString(a->generic.x + a->generic.parent->x + 16, a->generic.y + a->generic.parent->y, "???", 255);
	} else {
		int		x;
		const char     *name;

		name = Key_KeynumToString(keys[0]);

		Menu_DrawString(a->generic.x + a->generic.parent->x + 16, a->generic.y + a->generic.parent->y, name, 255);

		x = strlen(name) * 8;

		if (keys[1] != -1) {
			Menu_DrawString(a->generic.x + a->generic.parent->x + 24 + x, a->generic.y + a->generic.parent->y, "or", 255);
			Menu_DrawString(a->generic.x + a->generic.parent->x + 48 + x, a->generic.y + a->generic.parent->y, Key_KeynumToString(keys[1]), 255);
		}
	}
}

static void
KeyBindingFunc(void *self)
{
	menuaction_s   *a = (menuaction_s *) self;
	int		keys       [2];

	M_FindKeysForCommand(bindnames[a->generic.localdata[0]][0], keys);

	if (keys[1] != -1)
		M_UnbindCommand(bindnames[a->generic.localdata[0]][0]);

	bind_grab = true;

	Menu_SetStatusBar(&s_keys_menu, "Press a key or button for this action");
}

int
listSize(char *list[][2])
{
	int		i = 0;
	while (list[i][1])
		i++;

	return i;
}

void
addBindOption(int i, char *list[][2])
{
	int		y = 0;

	s_keys_binds[i].generic.type = MTYPE_ACTION;
	s_keys_binds[i].generic.flags = QMF_GRAYED;
	s_keys_binds[i].generic.x = 0;
	s_keys_binds[i].generic.y = y += 9 * i;
	s_keys_binds[i].generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_binds[i].generic.localdata[0] = i;
	s_keys_binds[i].generic.name = list[s_keys_binds[i].generic.localdata[0]][1];
	s_keys_binds[i].generic.callback = KeyBindingFunc;

	if (strstr("MENUSPACE", list[i][0]))
		s_keys_binds[i].generic.type = MTYPE_SEPARATOR;
}

static void
Keys_MenuInit(void)
{
	int		BINDS_MAX;
	int		i = 0;

	s_keys_menu.x = viddef.width * 0.50;
	s_keys_menu.y = 0;
	s_keys_menu.nitems = 0;
	s_keys_menu.cursordraw = KeyCursorDrawFunc;

	BINDS_MAX = listSize(bindnames);
	for (i = 0; i < BINDS_MAX; i++)
		addBindOption(i, bindnames);


	for (i = 0; i < BINDS_MAX; i++)
		Menu_AddItem(&s_keys_menu, (void *)&s_keys_binds[i]);

	Menu_SetStatusBar(&s_keys_menu, "ENTER to change, BACKSPACE to clear");
	Menu_Center(&s_keys_menu);
}

static void
Keys_MenuDraw(void)
{
	Menu_AdjustCursor(&s_keys_menu, 1);
	Menu_Draw(&s_keys_menu);
}

static const char *
Keys_MenuKey(int key)
{
	menuaction_s   *item = (menuaction_s *) Menu_ItemAtCursor(&s_keys_menu);

	if (bind_grab && !(cursor.buttonused[MOUSEBUTTON1] && key == K_MOUSE1)) {
		if (key != K_ESCAPE && key != '`') {
			char		cmd       [1024];

			Com_sprintf(cmd, sizeof(cmd), "bind \"%s\" \"%s\"\n", Key_KeynumToString(key), bindnames[item->generic.localdata[0]][0]);
			Cbuf_InsertText(cmd);
		}
		/* Knightmare- added Psychospaz's mouse support */
		/* dont let selecting with mouse buttons screw everything up */
		refreshCursorButtons();
		if (key == K_MOUSE1)
			cursor.buttonclicks[MOUSEBUTTON1] = -1;
		Menu_SetStatusBar(&s_keys_menu, "ENTER to change, BACKSPACE to clear");
		bind_grab = false;
		return menu_out_sound;
	}
	switch (key) {
	case K_KP_ENTER:
	case K_ENTER:
		KeyBindingFunc(item);
		return menu_in_sound;
	case K_BACKSPACE:	/* delete bindings */
	case K_DEL:		/* delete bindings */
	case K_KP_DEL:
		M_UnbindCommand(bindnames[item->generic.localdata[0]][0]);
		return menu_out_sound;
	default:
		return Default_MenuKey(&s_keys_menu, key);
	}
}

void
M_Menu_Keys_f(void)
{
	Keys_MenuInit();
	M_PushMenu(Keys_MenuDraw, Keys_MenuKey);
}

/*
 * =======================================================================
 *
 * QUDOS MENU
 *
 * =======================================================================
 */
void
ControlsSetMenuItemValues(void);

static menuframework_s	s_qudos_menu;

static menuseparator_s	s_qudos_client_side;

static menulist_s	s_qudos_3dcam;
static menuslider_s	s_qudos_3dcam_distance;
static menuslider_s	s_qudos_3dcam_angle;
static menulist_s	s_qudos_weap_shell;
static menulist_s	s_qudos_bobbing;
static menuslider_s	s_qudos_gunalpha;
static menuslider_s	s_qudos_flashlight_red;
static menuslider_s	s_qudos_flashlight_green;
static menuslider_s	s_qudos_flashlight_blue;
static menuslider_s	s_qudos_flashlight_intensity;
static menuslider_s	s_qudos_flashlight_distance;
static menuslider_s	s_qudos_flashlight_decscale;
#ifdef QMAX
static menulist_s	s_qudos_blaster_bubble;
static menulist_s	s_qudos_blaster_color;
static menulist_s	s_qudos_hyperblaster_particles;
static menulist_s	s_qudos_hyperblaster_bubble;
static menulist_s	s_qudos_hyperblaster_color;
static menulist_s	s_qudos_max_railtrail;
static menuslider_s	s_qudos_max_railtrail_red;
static menuslider_s	s_qudos_max_railtrail_green;
static menuslider_s	s_qudos_max_railtrail_blue;
static menulist_s	s_qudos_gun_smoke;
static menulist_s	s_qudos_explosions;
static menulist_s	s_qudos_flame_enh;
static menuslider_s	s_qudos_flame_size;
static menulist_s	s_qudos_particles_type;
static menulist_s	s_qudos_teleport;
static menulist_s	s_qudos_nukeblast;
#else
static menulist_s	s_qudos_gl_particles;
static menulist_s	s_qudos_railstyle;
static menulist_s	s_qudos_railtrail;
static menulist_s	s_qudos_railtrail_color;
#endif
static menufield_s      s_qudos_option_maxfps;

static menuseparator_s	s_qudos_graphic;

static menuslider_s	s_qudos_gl_reflection;
static menulist_s	s_qudos_gl_reflection_shader;
static menulist_s	s_qudos_texshader_warp;
static menuslider_s	s_qudos_waves;
static menulist_s	s_qudos_water_trans;
static menulist_s	s_qudos_underwater_mov;
static menulist_s	s_qudos_caustics;
static menuslider_s	s_qudos_det_tex;
static menulist_s	s_qudos_fog;
static menuslider_s	s_qudos_fog_density;
static menuslider_s	s_qudos_fog_red;
static menuslider_s	s_qudos_fog_green;
static menuslider_s	s_qudos_fog_blue;
static menulist_s	s_qudos_fog_underwater;
static menulist_s	s_qudos_minimap;
static menulist_s	s_qudos_minimap_style;
static menuslider_s	s_qudos_minimap_size;
static menuslider_s	s_qudos_minimap_zoom;
static menuslider_s	s_qudos_minimap_x;
static menuslider_s	s_qudos_minimap_y;
static menulist_s	s_qudos_shadows;
static menulist_s	s_qudos_motionblur;
static menuslider_s	s_qudos_motionblur_intens;
static menulist_s	s_qudos_gl_stainmaps;
static menulist_s	s_qudos_cellshading;
static menulist_s	s_qudos_shading;
static menulist_s	s_qudos_blooms;
static menuslider_s	s_qudos_blooms_alpha;
static menuslider_s	s_qudos_blooms_diamond_size;
static menuslider_s	s_qudos_blooms_intensity;
static menuslider_s	s_qudos_blooms_darken;
static menuslider_s	s_qudos_blooms_sample_size;
static menulist_s	s_qudos_blooms_fast_sample;
static menulist_s	s_qudos_lensflare;
static menuslider_s	s_qudos_lensflare_intens;
static menulist_s	s_qudos_lensflare_style;
static menuslider_s	s_qudos_lensflare_scale;
static menuslider_s	s_qudos_lensflare_maxdist;
static menulist_s	s_qudos_glares;
static menuslider_s	s_qudos_lightmap_scale;

static menuseparator_s	s_qudos_hud;

static menulist_s	s_qudos_hudscale;
static menulist_s	s_qudos_drawclock;
static menulist_s	s_qudos_drawdate;
static menulist_s	s_qudos_drawmaptime;
static menuslider_s	s_qudos_draw_x;
static menuslider_s	s_qudos_draw_y;
static menulist_s	s_qudos_highlight;
static menulist_s	s_qudos_drawchathud;
static menulist_s	s_qudos_drawplayername;
static menulist_s	s_qudos_drawping;
static menulist_s	s_qudos_drawfps;
static menulist_s	s_qudos_drawrate;
static menulist_s	s_qudos_drawnetgraph;
static menulist_s	s_qudos_drawnetgraph_pos;
static menuslider_s	s_qudos_drawnetgraph_pos_x;
static menuslider_s	s_qudos_drawnetgraph_pos_y;
static menuslider_s	s_qudos_drawnetgraph_alpha;
static menuslider_s	s_qudos_drawnetgraph_scale;
static menufield_s	s_qudos_netgraph_image;
static menulist_s	s_qudos_crosshair_box;
static menuslider_s	s_qudos_cross_scale;
static menuslider_s	s_qudos_cross_red;
static menuslider_s	s_qudos_cross_green;
static menuslider_s	s_qudos_cross_blue;
static menuslider_s	s_qudos_cross_pulse;
static menuslider_s	s_qudos_cross_alpha;
static menuslider_s	s_qudos_hud_red;
static menuslider_s	s_qudos_hud_green;
static menuslider_s	s_qudos_hud_blue;
static menuslider_s	s_qudos_hud_alpha;
static menufield_s	s_qudos_font_color;
static menufield_s	s_qudos_conback_image;
static menuslider_s	s_qudos_menu_alpha;

extern cvar_t          *scr_netgraph_image;
extern cvar_t          *scr_netgraph_pos_x;
extern cvar_t          *scr_netgraph_pos_y;
extern cvar_t          *font_color;
#ifdef QMAX
extern cvar_t          *cl_flame_enh;
extern cvar_t          *cl_flame_size;
#endif


static void
ThirdPersonCamFunc(void *unused)
{
	Cvar_SetValue("cl_3dcam", s_qudos_3dcam.curvalue);
}

static void
ThirdPersonDistFunc(void *unused)
{
	Cvar_SetValue( "cl_3dcam_dist", s_qudos_3dcam_distance.curvalue * 25 );
}

static void
ThirdPersonAngleFunc(void *unused)
{
	Cvar_SetValue( "cl_3dcam_angle", s_qudos_3dcam_angle.curvalue * 10 );
}

static void
WeapShellFunc(void *unused)
{
	Cvar_SetValue("weap_shell", s_qudos_weap_shell.curvalue);
}

static void
BobbingFunc(void *unused)
{
	Cvar_SetValue("cl_bobbing", s_qudos_bobbing.curvalue);
}

static void
GunAlphaFunc(void *unused)
{
	Cvar_SetValue("cl_gunalpha", s_qudos_gunalpha.curvalue / 10);
}

static void
FlashLightColorRedFunc(void *unused)
{
	Cvar_SetValue("cl_flashlight_red", s_qudos_flashlight_red.curvalue / 10);
}

static void
FlashLightColorGreenFunc(void *unused)
{
	Cvar_SetValue("cl_flashlight_green", s_qudos_flashlight_green.curvalue / 10);
}

static void
FlashLightColorBlueFunc(void *unused)
{
	Cvar_SetValue("cl_flashlight_blue", s_qudos_flashlight_blue.curvalue / 10);
}

static void
FlashLightIntensityFunc(void *unused)
{
	Cvar_SetValue("cl_flashlight_intensity", s_qudos_flashlight_intensity.curvalue * 10);
}

static void
FlashLightDistanceFunc(void *unused)
{
	Cvar_SetValue("cl_flashlight_distance", s_qudos_flashlight_distance.curvalue * 10);
}

static void
FlashLightDecreaseScaleFunc(void *unused)
{
	Cvar_SetValue("cl_flashlight_decscale", s_qudos_flashlight_decscale.curvalue / 10);
}

#ifdef QMAX
static void
BlasterBubbleFunc(void *unused)
{
	Cvar_SetValue("cl_blaster_bubble", s_qudos_blaster_bubble.curvalue);
}

static void
BlasterColorFunc(void *unused)
{
	Cvar_SetValue("cl_blaster_color", s_qudos_blaster_color.curvalue);
}

static void
HyperBlasterParticlesFunc(void *unused)
{
	Cvar_SetValue("cl_hyperblaster_particles", s_qudos_hyperblaster_particles.curvalue);
}

static void
HyperBlasterBubbleFunc(void *unused)
{
	Cvar_SetValue("cl_hyperblaster_bubble", s_qudos_hyperblaster_bubble.curvalue);
}

static void
HyperBlasterColorFunc(void *unused)
{
	Cvar_SetValue("cl_hyperblaster_color", s_qudos_hyperblaster_color.curvalue);
}

static void
MaxRailTrailFunc(void *unused)
{
	Cvar_SetValue("cl_railtype", s_qudos_max_railtrail.curvalue);
}

static void
MaxRailTrailColorRedFunc(void *unused)
{
	Cvar_SetValue("cl_railred", s_qudos_max_railtrail_red.curvalue*16);
}

static void
MaxRailTrailColorGreenFunc(void *unused)
{
	Cvar_SetValue("cl_railgreen", s_qudos_max_railtrail_green.curvalue*16);
}

static void
MaxRailTrailColorBlueFunc(void *unused)
{
	Cvar_SetValue("cl_railblue", s_qudos_max_railtrail_blue.curvalue*16);
}
static void
GunSmokeFunc(void *unused)
{
	Cvar_SetValue("cl_gunsmoke", s_qudos_gun_smoke.curvalue);
}
static void
ExplosionsFunc(void *unused)
{
	Cvar_SetValue("cl_explosion", s_qudos_explosions.curvalue);
}

static void
FlameEnhFunc(void *unused)
{
	Cvar_SetValue("cl_flame_enh", s_qudos_flame_enh.curvalue);
}

static void
FlameEnhSizeFunc(void *unused)
{
	Cvar_SetValue("cl_flame_size", s_qudos_flame_size.curvalue / 1 );
}

static void
ParticlesTypeFunc(void *unused)
{
	Cvar_SetValue("cl_particles_type", s_qudos_particles_type.curvalue);
}

static void
TelePortFunc(void *unused)
{
	Cvar_SetValue("cl_teleport_particles", s_qudos_teleport.curvalue);
}

static void
NukeBlastFunc(void *unused)
{
	Cvar_SetValue("cl_nukeblast_enh", s_qudos_nukeblast.curvalue);
}
#else
static void
GLParticleFunc(void *unused)
{
	Cvar_SetValue("gl_particles", s_qudos_gl_particles.curvalue);
}

static void
RailTrailStyleFunc(void *unused)
{
	Cvar_SetValue("cl_railstyle", s_qudos_railstyle.curvalue);
}

static void
RailTrailFunc(void *unused)
{
	Cvar_SetValue("cl_railtrail", s_qudos_railtrail.curvalue);
}

static void
RailTrailColorFunc(void *unused)
{
	Cvar_SetValue("cl_railtrail_color", s_qudos_railtrail_color.curvalue);
}
#endif


static void
WaterReflection(void *unused)
{
	Cvar_SetValue("gl_reflection", s_qudos_gl_reflection.curvalue / 10);
}

static void
WaterShaderReflection(void *unused)
{
	Cvar_SetValue("gl_reflection_shader", s_qudos_gl_reflection_shader.curvalue);
}

static void
TexShaderWarp(void *unused)
{
	Cvar_SetValue("gl_water_pixel_shader_warp", s_qudos_texshader_warp.curvalue);
}

static void
DetTexFunc(void *unused)
{
	Cvar_SetValue("gl_detailtextures", s_qudos_det_tex.curvalue);
}

static void
WaterWavesFunc(void *unused)
{
	Cvar_SetValue("gl_water_waves", s_qudos_waves.curvalue);
}

static void
TexCausticFunc(void *unused)
{
	Cvar_SetValue("gl_water_caustics", s_qudos_caustics.curvalue);
}

static void
TransWaterFunc(void *unused)
{
	Cvar_SetValue("cl_underwater_trans", s_qudos_water_trans.curvalue);
}

static void
UnderWaterMovFunc(void *unused)
{
	Cvar_SetValue("cl_underwater_movement", s_qudos_underwater_mov.curvalue);
}

static void
FogFunc(void *unused)
{
	Cvar_SetValue("gl_fogenable", s_qudos_fog.curvalue);
}

static void
FogDensityFunc(void *unused)
{
	Cvar_SetValue("gl_fogdensity", s_qudos_fog_density.curvalue / 10000);
}

static void
FogRedFunc(void *unused)
{
	Cvar_SetValue("gl_fogred", s_qudos_fog_red.curvalue / 10);
}

static void
FogGreenFunc(void *unused)
{
	Cvar_SetValue("gl_foggreen", s_qudos_fog_green.curvalue / 10);
}

static void
FogBlueFunc(void *unused)
{
	Cvar_SetValue("gl_fogblue", s_qudos_fog_blue.curvalue / 10);
}

static void
FogUnderWaterFunc(void *unused)
{
	Cvar_SetValue("gl_fogunderwater", s_qudos_fog_underwater.curvalue);
}

static void
ShadowsFunc(void *unused)
{
	Cvar_SetValue("gl_shadows", s_qudos_shadows.curvalue);
}

static void
MotionBlurFunc(void *unused)
{
	Cvar_SetValue("gl_motionblur", s_qudos_motionblur.curvalue);
}

static void
MotionBlurIntensFunc(void *unused)
{
	Cvar_SetValue("gl_motionblur_intensity", s_qudos_motionblur_intens.curvalue / 10);
}

static void
BloomsFunc(void *unused)
{
	Cvar_SetValue("gl_blooms", s_qudos_blooms.curvalue);
}

static void
BloomsAlphaFunc(void *unused)
{
	Cvar_SetValue("gl_blooms_alpha", s_qudos_blooms_alpha.curvalue / 10);
}

static void
BloomsDiamondSizeFunc(void *unused)
{
	Cvar_SetValue("gl_blooms_diamond_size", s_qudos_blooms_diamond_size.curvalue * 2);
}

static void
BloomsIntensityFunc(void *unused)
{
	Cvar_SetValue("gl_blooms_intensity", s_qudos_blooms_intensity.curvalue / 10);
}

static void
BloomsDarkenFunc(void *unused)
{
	Cvar_SetValue("gl_blooms_darken", s_qudos_blooms_darken.curvalue);
}

static void
BloomsSampleSizeFunc(void *unused)
{
	Cvar_SetValue("gl_blooms_sample_size", pow(2, s_qudos_blooms_sample_size.curvalue));
}

static void
BloomsFastSampleFunc(void *unused)
{
	Cvar_SetValue("gl_blooms_fast_sample", s_qudos_blooms_fast_sample.curvalue);
}

static void
GlaresFunc(void *unused)
{
	Cvar_SetValue("gl_glares", s_qudos_glares.curvalue);
}

static void
MiniMapFunc(void *unused)
{
	Cvar_SetValue("gl_minimap", s_qudos_minimap.curvalue);
}

static void
MiniMapStyleFunc(void *unused)
{
	Cvar_SetValue("gl_minimap_style", s_qudos_minimap_style.curvalue);
}

static void
MiniMapSizeFunc(void *unused)
{
	Cvar_SetValue("gl_minimap_size", s_qudos_minimap_size.curvalue);
}

static void
MiniMapZoomFunc(void *unused)
{
	Cvar_SetValue("gl_minimap_zoom", s_qudos_minimap_zoom.curvalue / 10);
}

static void
MiniMapXPosFunc(void *unused)
{
	Cvar_SetValue("gl_minimap_x", s_qudos_minimap_x.curvalue / 1);
}

static void
MiniMapYPosFunc(void *unused)
{
	Cvar_SetValue("gl_minimap_y", s_qudos_minimap_y.curvalue / 1);
}

static void
StainMapsFunc(void *unused)
{
	Cvar_SetValue("gl_stainmaps", s_qudos_gl_stainmaps.curvalue);
}

static void
CellShadingFunc(void *unused)
{
	Cvar_SetValue("r_cellshading", s_qudos_cellshading.curvalue);
}

static void
ShadingFunc(void *unused)
{
	Cvar_SetValue("gl_shading", s_qudos_shading.curvalue);
}

static void
LensFlareFunc(void *unused)
{
	Cvar_SetValue("gl_flares", s_qudos_lensflare.curvalue);
}

static void
LensFlareIntensFunc(void *unused)
{
	Cvar_SetValue("gl_flare_intensity", s_qudos_lensflare_intens.curvalue / 10);
}

static void
LensFlareStyleFunc(void *unused)
{
	Cvar_SetValue("gl_flare_force_style", s_qudos_lensflare_style.curvalue);
}

static void
LensFlareScaleFunc(void *unused)
{
	Cvar_SetValue("gl_flare_scale", s_qudos_lensflare_scale.curvalue / 10);
}

static void
LensFlareDistFunc(void *unused)
{
	Cvar_SetValue("gl_flare_maxdist", s_qudos_lensflare_maxdist.curvalue);
}


static void
LightMapScaleFunc(void *unused)
{
	Cvar_SetValue("gl_modulate", s_qudos_lightmap_scale.curvalue / 10 + 1 );
}

static void
DrawPingFunc(void *unused)
{
	Cvar_SetValue("cl_drawping", s_qudos_drawping.curvalue);
}

static void
UpdateHudXFunc(void *unused)
{
	Cvar_SetValue("cl_draw_x", s_qudos_draw_x.curvalue / 20.0);
}

static void
UpdateHudYFunc(void *unused)
{
	Cvar_SetValue("cl_draw_y", s_qudos_draw_y.curvalue / 20.0);
}

static void
DrawFPSFunc(void *unused)
{
	Cvar_SetValue("cl_drawfps", s_qudos_drawfps.curvalue);
}

static void
HudScaleFunc(void *unused)
{
	Cvar_SetValue("cl_hudscale", s_qudos_hudscale.curvalue);
}

static void
DrawClockFunc(void *unused)
{
	Cvar_SetValue("cl_drawclock", s_qudos_drawclock.curvalue);
}

static void
DrawDateFunc(void *unused)
{
	Cvar_SetValue("cl_drawdate", s_qudos_drawdate.curvalue);
}

static void
DrawMapTimeFunc(void *unused)
{
	Cvar_SetValue("cl_drawmaptime", s_qudos_drawmaptime.curvalue);
}

static void
DrawRateFunc(void *unused)
{
	Cvar_SetValue("cl_drawrate", s_qudos_drawrate.curvalue);
}

static void
DrawNetGraphFunc(void *unused)
{
	Cvar_SetValue("netgraph", s_qudos_drawnetgraph.curvalue);
}

static void
DrawNetGraphPosFunc(void *unused)
{
	Cvar_SetValue("netgraph_pos", s_qudos_drawnetgraph_pos.curvalue);
}

static void
DrawNetGraphPosXFunc(void *unused)
{
	Cvar_SetValue("netgraph_pos_x", s_qudos_drawnetgraph_pos_x.curvalue / 1);
}

static void
DrawNetGraphPosYFunc(void *unused)
{
	Cvar_SetValue("netgraph_pos_y", s_qudos_drawnetgraph_pos_y.curvalue / 1);
}

static void
DrawNetGraphAlphaFunc(void *unused)
{
	Cvar_SetValue("netgraph_trans", s_qudos_drawnetgraph_alpha.curvalue / 10);
}

static void
DrawNetGraphScaleFunc(void *unused)
{
	Cvar_SetValue("netgraph_size", s_qudos_drawnetgraph_scale.curvalue / 1);
}

static void
HighlightFunc(void *unused)
{
	Cvar_SetValue("cl_highlight", s_qudos_highlight.curvalue);
}

static void
DrawChatHudFunc(void *unused)
{
	Cvar_SetValue("cl_drawchathud", s_qudos_drawchathud.curvalue);
}

static void
DrawPlayerNameFunc(void *unused)
{
	Cvar_SetValue("cl_draw_playername", s_qudos_drawplayername.curvalue);
}

static void
CrosshairFunc(void *unused)
{
	Cvar_SetValue("crosshair", s_qudos_crosshair_box.curvalue);
}

static void
CrossScaleFunc(void *unused)
{
	Cvar_SetValue("crosshair_scale", s_qudos_cross_scale.curvalue / 10);
}

static void
CrossColorRedFunc(void *unused)
{
	Cvar_SetValue("crosshair_red", s_qudos_cross_red.curvalue / 10);
}

static void
CrossColorGreenFunc(void *unused)
{
	Cvar_SetValue("crosshair_green", s_qudos_cross_green.curvalue / 10);
}

static void
CrossColorBlueFunc(void *unused)
{
	Cvar_SetValue("crosshair_blue", s_qudos_cross_blue.curvalue / 10);
}

static void
CrossPulseFunc(void *unused)
{
	Cvar_SetValue("crosshair_pulse", s_qudos_cross_pulse.curvalue / 1);
}

static void
CrossAlphaFunc(void *unused)
{
	Cvar_SetValue("crosshair_alpha", s_qudos_cross_alpha.curvalue / 10);
}

static void
HudColorRedFunc(void *unused)
{
	Cvar_SetValue("cl_hud_red", s_qudos_hud_red.curvalue / 10);
}

static void
HudColorGreenFunc(void *unused)
{
	Cvar_SetValue("cl_hud_green", s_qudos_hud_green.curvalue / 10);
}

static void
HudColorBlueFunc(void *unused)
{
	Cvar_SetValue("cl_hud_blue", s_qudos_hud_blue.curvalue / 10);
}

static void
HudAlphaFunc(void *unused)
{
	Cvar_SetValue("cl_hudalpha", s_qudos_hud_alpha.curvalue / 10);
}

static void
MenuAlphaFunc(void *unused)
{
	Cvar_SetValue("cl_menu_alpha", s_qudos_menu_alpha.curvalue / 10);
}

static void
QuDos_MenuDraw(void)
{
	Menu_AdjustCursor(&s_qudos_menu, 1);
	Menu_Draw(&s_qudos_menu);
}

const char *
QuDos_MenuKey_Client(int key)
{
	if (key == K_INS) {
		Cvar_Set("cl_maxfps", s_qudos_option_maxfps.buffer);
	}
	return Default_MenuKey(&s_qudos_menu, key);
}

const char *
QuDos_MenuKey_Graphic(int key)
{
	return Default_MenuKey(&s_qudos_menu, key);
}

const char *
QuDos_MenuKey_Hud(int key)
{
	if (key == K_INS) {
		Cvar_Set("netgraph_image", s_qudos_netgraph_image.buffer);
		Cvar_Set("cl_conback_image", s_qudos_conback_image.buffer);
		Cvar_Set("font_color", s_qudos_font_color.buffer);
	}
	return Default_MenuKey(&s_qudos_menu, key);
}

void
QuDos_MenuInit_Client(void)
{
	static const char *yesno_names[] = {
		"Disabled",
		"Enabled ",
		NULL
	};

	static const char *effect_colors[] = {
		"Orange",
		"Red   ",
		"Green ",
		"Blue  ",
		NULL
	};

#ifdef QMAX
	static const char *bubble_modes[] = {
		"Disabled",
		"Always",
		"Underwater ",
		NULL
	};
	static const char *railtrail_effect[] = {
		"Beam      ",
		"Spiral    ",
		"Laser     ",
		"Devastator",
		NULL
	};
	static const char *explosion_effect[] = {
		"Old 3D          ",
		"QMax            ",
		"QMax with Sparks",
		NULL
	};
	static const char *teleport_effect[] = {
		"Generic Particles    ",
		"Exploding Bubbles    ",
		"Exploding Lens Flares",
		"Rotating Bubbles     ",
		"Rotating Lens Flares ",
		NULL
	};
	static const char *particles_type[] = {
		"Generic Particles",
		"Bubbles          ",
		"Lens Flares      ",
		NULL
	};
	static const char *nukeblast_effect[] = {
		"Generic Particles",
		"Thunder          ",
		NULL
	};
#else
	static const char *railtrail_style[] = {
		"Original Rail Gun",
		"Rail Only        ",
		"Snake Rail       ",
		NULL
	};
	static const char *railtrail_effect[] = {
		"Rail Gun        ",
		"Xania Rail Trail",
		NULL
	};
#endif

	s_qudos_menu.x = viddef.width / 2;
	s_qudos_menu.y = viddef.height / 2 - 250;
	s_qudos_menu.nitems = 0;

	s_qudos_client_side.generic.type = MTYPE_SEPARATOR;
	s_qudos_client_side.generic.name = "Client Side Options";
	s_qudos_client_side.generic.x = 70;
	s_qudos_client_side.generic.y = 0;

	s_qudos_3dcam.generic.type = MTYPE_SPINCONTROL;
	s_qudos_3dcam.generic.x = 0;
	s_qudos_3dcam.generic.y = 20;
	s_qudos_3dcam.generic.name = "Third Person Camera";
	s_qudos_3dcam.generic.callback = ThirdPersonCamFunc;
	s_qudos_3dcam.itemnames = yesno_names;
	s_qudos_3dcam.generic.statusbar = "Third Person Camera";

	s_qudos_3dcam_distance.generic.type = MTYPE_SLIDER;
	s_qudos_3dcam_distance.generic.x = 0;
	s_qudos_3dcam_distance.generic.y = 30;
	s_qudos_3dcam_distance.generic.name = "Camera distance";
	s_qudos_3dcam_distance.generic.callback = ThirdPersonDistFunc;
	s_qudos_3dcam_distance.minvalue = 1;
	s_qudos_3dcam_distance.maxvalue = 8;
	s_qudos_3dcam_distance.generic.statusbar = "Third Person Camera Distance";

	s_qudos_3dcam_angle.generic.type = MTYPE_SLIDER;
	s_qudos_3dcam_angle.generic.x = 0;
	s_qudos_3dcam_angle.generic.y = 40;
	s_qudos_3dcam_angle.generic.name = "Camera angle";
	s_qudos_3dcam_angle.generic.callback = ThirdPersonAngleFunc;
	s_qudos_3dcam_angle.minvalue = 0;
	s_qudos_3dcam_angle.maxvalue = 10;
	s_qudos_3dcam_angle.generic.statusbar = "Third Person Camera Angle";

	s_qudos_weap_shell.generic.type = MTYPE_SPINCONTROL;
	s_qudos_weap_shell.generic.x = 0;
	s_qudos_weap_shell.generic.y = 60;
	s_qudos_weap_shell.generic.name = "Weapons & Models Shell";
	s_qudos_weap_shell.generic.callback = WeapShellFunc;
	s_qudos_weap_shell.itemnames = yesno_names;
	s_qudos_weap_shell.generic.statusbar = "Weapons & Model Shell, requires a map restart";

	s_qudos_bobbing.generic.type = MTYPE_SPINCONTROL;
	s_qudos_bobbing.generic.x = 0;
	s_qudos_bobbing.generic.y = 70;
	s_qudos_bobbing.generic.name = "Bobbing Items & Weapons";
	s_qudos_bobbing.generic.callback = BobbingFunc;
	s_qudos_bobbing.itemnames = yesno_names;
	s_qudos_bobbing.generic.statusbar = "Bobbing Weapons & Items";

	s_qudos_gunalpha.generic.type = MTYPE_SLIDER;
	s_qudos_gunalpha.generic.x = 0;
	s_qudos_gunalpha.generic.y = 80;
	s_qudos_gunalpha.generic.name = "Weapon Alpha";
	s_qudos_gunalpha.generic.callback = GunAlphaFunc;
	s_qudos_gunalpha.minvalue = 0;
	s_qudos_gunalpha.maxvalue = 10;
	s_qudos_gunalpha.generic.statusbar = "Weapon Alpha Level";

	s_qudos_flashlight_red.generic.type = MTYPE_SLIDER;
	s_qudos_flashlight_red.generic.x = 0;
	s_qudos_flashlight_red.generic.y = 100;
	s_qudos_flashlight_red.generic.name = "Flashlight Red";
	s_qudos_flashlight_red.generic.callback = FlashLightColorRedFunc;
	s_qudos_flashlight_red.minvalue = 0;
	s_qudos_flashlight_red.maxvalue = 10;
	s_qudos_flashlight_red.generic.statusbar = "Flashlight Red";

	s_qudos_flashlight_green.generic.type = MTYPE_SLIDER;
	s_qudos_flashlight_green.generic.x = 0;
	s_qudos_flashlight_green.generic.y = 110;
	s_qudos_flashlight_green.generic.name = "Flashlight Green";
	s_qudos_flashlight_green.generic.callback = FlashLightColorGreenFunc;
	s_qudos_flashlight_green.minvalue = 0;
	s_qudos_flashlight_green.maxvalue = 10;
	s_qudos_flashlight_green.generic.statusbar = "Flashlight Green";

	s_qudos_flashlight_blue.generic.type = MTYPE_SLIDER;
	s_qudos_flashlight_blue.generic.x = 0;
	s_qudos_flashlight_blue.generic.y = 120;
	s_qudos_flashlight_blue.generic.name = "Flashlight Blue";
	s_qudos_flashlight_blue.generic.callback = FlashLightColorBlueFunc;
	s_qudos_flashlight_blue.minvalue = 0;
	s_qudos_flashlight_blue.maxvalue = 10;
	s_qudos_flashlight_blue.generic.statusbar = "Flashlight Red";

	s_qudos_flashlight_intensity.generic.type = MTYPE_SLIDER;
	s_qudos_flashlight_intensity.generic.x = 0;
	s_qudos_flashlight_intensity.generic.y = 130;
	s_qudos_flashlight_intensity.generic.name = "Flashlight Intensity";
	s_qudos_flashlight_intensity.generic.callback = FlashLightIntensityFunc;
	s_qudos_flashlight_intensity.minvalue = 5;
	s_qudos_flashlight_intensity.maxvalue = 40;
	s_qudos_flashlight_intensity.generic.statusbar = "Flashlight Intensity";

	s_qudos_flashlight_distance.generic.type = MTYPE_SLIDER;
	s_qudos_flashlight_distance.generic.x = 0;
	s_qudos_flashlight_distance.generic.y = 140;
	s_qudos_flashlight_distance.generic.name = "Flashlight Distance";
	s_qudos_flashlight_distance.generic.callback = FlashLightDistanceFunc;
	s_qudos_flashlight_distance.minvalue = 1;
	s_qudos_flashlight_distance.maxvalue = 150;
	s_qudos_flashlight_distance.generic.statusbar = "Maximum effective flashlight distance";

	s_qudos_flashlight_decscale.generic.type = MTYPE_SLIDER;
	s_qudos_flashlight_decscale.generic.x = 0;
	s_qudos_flashlight_decscale.generic.y = 150;
	s_qudos_flashlight_decscale.generic.name = "Flashlight Decrease scale";
	s_qudos_flashlight_decscale.generic.callback = FlashLightDecreaseScaleFunc;
	s_qudos_flashlight_decscale.minvalue = 0;
	s_qudos_flashlight_decscale.maxvalue = 20;
	s_qudos_flashlight_decscale.generic.statusbar = "How much the intensity decreases with distance";

	s_qudos_option_maxfps.generic.type = MTYPE_FIELD;
	s_qudos_option_maxfps.generic.name = "Max FPS";
	s_qudos_option_maxfps.generic.flags = QMF_NUMBERSONLY;
	s_qudos_option_maxfps.generic.x = 0;
	s_qudos_option_maxfps.generic.y = 170;
	s_qudos_option_maxfps.length = 4;
	s_qudos_option_maxfps.visible_length = 4;
	Q_strncpyz(s_qudos_option_maxfps.buffer, Cvar_VariableString("cl_maxfps"), sizeof(s_qudos_option_maxfps.buffer));
	s_qudos_option_maxfps.generic.statusbar =
	"Type the maximum desired for FPS in the box and press the key INSERT";

#ifdef QMAX
	s_qudos_blaster_color.generic.type = MTYPE_SPINCONTROL;
	s_qudos_blaster_color.generic.x = 0;
	s_qudos_blaster_color.generic.y = 190;
	s_qudos_blaster_color.generic.name = "Blaster Particles Color";
	s_qudos_blaster_color.generic.callback = BlasterColorFunc;
	s_qudos_blaster_color.itemnames = effect_colors;
	s_qudos_blaster_color.generic.statusbar = "Blaster Particles Color";

	s_qudos_blaster_bubble.generic.type = MTYPE_SPINCONTROL;
	s_qudos_blaster_bubble.generic.x = 0;
	s_qudos_blaster_bubble.generic.y = 200;
	s_qudos_blaster_bubble.generic.name = "Blaster Bubbles";
	s_qudos_blaster_bubble.generic.callback = BlasterBubbleFunc;
	s_qudos_blaster_bubble.itemnames = bubble_modes;
	s_qudos_blaster_bubble.generic.statusbar = "Nice blaster bubbles particles effect";

	s_qudos_hyperblaster_particles.generic.type = MTYPE_SPINCONTROL;
	s_qudos_hyperblaster_particles.generic.x = 0;
	s_qudos_hyperblaster_particles.generic.y = 220;
	s_qudos_hyperblaster_particles.generic.name = "Hyper Blaster Particles";
	s_qudos_hyperblaster_particles.generic.callback = HyperBlasterParticlesFunc;
	s_qudos_hyperblaster_particles.itemnames = yesno_names;
	s_qudos_hyperblaster_particles.generic.statusbar = "Hyper Blaster particles";

	s_qudos_hyperblaster_color.generic.type = MTYPE_SPINCONTROL;
	s_qudos_hyperblaster_color.generic.x = 0;
	s_qudos_hyperblaster_color.generic.y = 230;
	s_qudos_hyperblaster_color.generic.name = "Hyper Blaster Particles Color";
	s_qudos_hyperblaster_color.generic.callback = HyperBlasterColorFunc;
	s_qudos_hyperblaster_color.itemnames = effect_colors;
	s_qudos_hyperblaster_color.generic.statusbar = "Hyper Blaster Particles Color";

	s_qudos_hyperblaster_bubble.generic.type = MTYPE_SPINCONTROL;
	s_qudos_hyperblaster_bubble.generic.x = 0;
	s_qudos_hyperblaster_bubble.generic.y = 240;
	s_qudos_hyperblaster_bubble.generic.name = "Hyper Blaster Bubbles";
	s_qudos_hyperblaster_bubble.generic.callback = HyperBlasterBubbleFunc;
	s_qudos_hyperblaster_bubble.itemnames = bubble_modes;
	s_qudos_hyperblaster_bubble.generic.statusbar = "Nice blaster bubbles particles effect";

	s_qudos_particles_type.generic.type = MTYPE_SPINCONTROL;
	s_qudos_particles_type.generic.x = 0;
	s_qudos_particles_type.generic.y = 260;
	s_qudos_particles_type.generic.name = "Particles Type";
	s_qudos_particles_type.generic.callback = ParticlesTypeFunc;
	s_qudos_particles_type.itemnames = particles_type;
	s_qudos_particles_type.generic.statusbar = 
	"Particle type effects, Flag trail, Respawn, BFG explosion, Teleport...";

	s_qudos_teleport.generic.type = MTYPE_SPINCONTROL;
	s_qudos_teleport.generic.x = 0;
	s_qudos_teleport.generic.y = 270;
	s_qudos_teleport.generic.name = "Teleport Effect";
	s_qudos_teleport.generic.callback = TelePortFunc;
	s_qudos_teleport.itemnames = teleport_effect;
	s_qudos_teleport.generic.statusbar = "Teleport particle effects";

	s_qudos_nukeblast.generic.type = MTYPE_SPINCONTROL;
	s_qudos_nukeblast.generic.x = 0;
	s_qudos_nukeblast.generic.y = 280;
	s_qudos_nukeblast.generic.name = "Enhanced Nuke Blast";
	s_qudos_nukeblast.generic.callback = NukeBlastFunc;
	s_qudos_nukeblast.itemnames = nukeblast_effect;
	s_qudos_nukeblast.generic.statusbar =  "Nuke Blast particle effects";

	s_qudos_max_railtrail.generic.type = MTYPE_SPINCONTROL;
	s_qudos_max_railtrail.generic.x = 0;
	s_qudos_max_railtrail.generic.y = 300;
	s_qudos_max_railtrail.generic.name = "Railgun Railtrail";
	s_qudos_max_railtrail.generic.callback = MaxRailTrailFunc;
	s_qudos_max_railtrail.itemnames = railtrail_effect;
	s_qudos_max_railtrail.generic.statusbar = "Railgun Railtrail";

	s_qudos_max_railtrail_red.generic.type = MTYPE_SLIDER;
	s_qudos_max_railtrail_red.generic.x = 0;
	s_qudos_max_railtrail_red.generic.y = 310;
	s_qudos_max_railtrail_red.generic.name = "Red";
	s_qudos_max_railtrail_red.generic.callback = MaxRailTrailColorRedFunc;
	s_qudos_max_railtrail_red.minvalue = 0;
	s_qudos_max_railtrail_red.maxvalue = 16;
	s_qudos_max_railtrail_red.generic.statusbar = "Railtrail Red";

	s_qudos_max_railtrail_green.generic.type = MTYPE_SLIDER;
	s_qudos_max_railtrail_green.generic.x = 0;
	s_qudos_max_railtrail_green.generic.y = 320;
	s_qudos_max_railtrail_green.generic.name = "Green";
	s_qudos_max_railtrail_green.generic.callback = MaxRailTrailColorGreenFunc;
	s_qudos_max_railtrail_green.minvalue = 0;
	s_qudos_max_railtrail_green.maxvalue = 16;
	s_qudos_max_railtrail_green.generic.statusbar = "Railtrail Green";

	s_qudos_max_railtrail_blue.generic.type = MTYPE_SLIDER;
	s_qudos_max_railtrail_blue.generic.x = 0;
	s_qudos_max_railtrail_blue.generic.y = 330;
	s_qudos_max_railtrail_blue.generic.name = "Blue";
	s_qudos_max_railtrail_blue.generic.callback = MaxRailTrailColorBlueFunc;
	s_qudos_max_railtrail_blue.minvalue = 0;
	s_qudos_max_railtrail_blue.maxvalue = 16;
	s_qudos_max_railtrail_blue.generic.statusbar = "Railtrail Blue";

	s_qudos_flame_enh.generic.type = MTYPE_SPINCONTROL;
	s_qudos_flame_enh.generic.x = 0;
	s_qudos_flame_enh.generic.y = 350;
	s_qudos_flame_enh.generic.name = "Enhanced Flames";
	s_qudos_flame_enh.generic.callback = FlameEnhFunc;
	s_qudos_flame_enh.itemnames = yesno_names;
	s_qudos_flame_enh.generic.statusbar = 
	"Flamethrower, used in mods such as dday, awaken, awaken2, lox, ...";

	s_qudos_flame_size.generic.type = MTYPE_SLIDER;
	s_qudos_flame_size.generic.x = 0;
	s_qudos_flame_size.generic.y = 360;
	s_qudos_flame_size.generic.name = "Flames Size";
	s_qudos_flame_size.generic.callback = FlameEnhSizeFunc;
	s_qudos_flame_size.minvalue = 5;
	s_qudos_flame_size.maxvalue = 35;
	s_qudos_flame_size.generic.statusbar = 
	"Flamethrower flame size, note that more value is less size";

	s_qudos_explosions.generic.type = MTYPE_SPINCONTROL;
	s_qudos_explosions.generic.x = 0;
	s_qudos_explosions.generic.y = 380;
	s_qudos_explosions.generic.name = "Explosions";
	s_qudos_explosions.generic.callback = ExplosionsFunc;
	s_qudos_explosions.itemnames = explosion_effect;
	s_qudos_explosions.generic.statusbar = "Explosions effects";

	s_qudos_gun_smoke.generic.type = MTYPE_SPINCONTROL;
	s_qudos_gun_smoke.generic.x = 0;
	s_qudos_gun_smoke.generic.y = 390;
	s_qudos_gun_smoke.generic.name = "Gun Smoke";
	s_qudos_gun_smoke.generic.callback = GunSmokeFunc;
	s_qudos_gun_smoke.itemnames = yesno_names;
	s_qudos_gun_smoke.generic.statusbar = "Gun Smoke puff";
#else
	s_qudos_gl_particles.generic.type = MTYPE_SPINCONTROL;
	s_qudos_gl_particles.generic.x = 0;
	s_qudos_gl_particles.generic.y = 190;
	s_qudos_gl_particles.generic.name = "Particle Effects";
	s_qudos_gl_particles.generic.callback = GLParticleFunc;
	s_qudos_gl_particles.itemnames = yesno_names;
	s_qudos_gl_particles.generic.statusbar = "Particle Effects";

	s_qudos_railstyle.generic.type = MTYPE_SPINCONTROL;
	s_qudos_railstyle.generic.x = 0;
	s_qudos_railstyle.generic.y = 210;
	s_qudos_railstyle.generic.name = "Railgun Style";
	s_qudos_railstyle.generic.callback = RailTrailStyleFunc;
	s_qudos_railstyle.itemnames = railtrail_style;
	s_qudos_railstyle.generic.statusbar = "Railgun Railtrail Style";

	s_qudos_railtrail.generic.type = MTYPE_SPINCONTROL;
	s_qudos_railtrail.generic.x = 0;
	s_qudos_railtrail.generic.y = 220;
	s_qudos_railtrail.generic.name = "Railgun Railtrail";
	s_qudos_railtrail.generic.callback = RailTrailFunc;
	s_qudos_railtrail.itemnames = railtrail_effect;
	s_qudos_railtrail.generic.statusbar = "Railgun Railtrail";

	s_qudos_railtrail_color.generic.type = MTYPE_SPINCONTROL;
	s_qudos_railtrail_color.generic.x = 0;
	s_qudos_railtrail_color.generic.y = 230;
	s_qudos_railtrail_color.generic.name = "Railgun Railtrail Color";
	s_qudos_railtrail_color.generic.callback = RailTrailColorFunc;
	s_qudos_railtrail_color.itemnames = effect_colors;
	s_qudos_railtrail_color.generic.statusbar = "Railgun Railtrail Color";
#endif

	ControlsSetMenuItemValues();

	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_client_side);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_3dcam);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_3dcam_distance);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_3dcam_angle);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_weap_shell);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_bobbing);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_gunalpha);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_flashlight_red);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_flashlight_green);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_flashlight_blue);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_flashlight_intensity);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_flashlight_distance);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_flashlight_decscale);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_option_maxfps);
#ifdef QMAX
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_blaster_color);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_blaster_bubble);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_hyperblaster_particles);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_hyperblaster_color);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_hyperblaster_bubble);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_particles_type);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_teleport);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_nukeblast);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_max_railtrail);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_max_railtrail_red);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_max_railtrail_green);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_max_railtrail_blue);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_flame_enh);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_flame_size);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_explosions);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_gun_smoke);
#else
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_gl_particles);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_railstyle);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_railtrail);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_railtrail_color);
#endif
}

void
QuDos_MenuInit_Graphic(void)
{
	static const char *yesno_names[] = {
		"Disabled",
		"Enabled ",
		NULL
	};

	static const char *shadows_names[] = {
		"Disabled",
		"Basic   ",
		"Stencil ",
		NULL
	};

	static const char *minimap_names[] = {
		"Disabled     ",
		"Star entities",
		"Dot entities ",
		NULL
	};

	static const char *minimap_style_names[] = {
		"Rotating",
		"Fixed   ",
		NULL
	};

	static const char *caustics_names[] = {
		"Disabled",
		"Basic   ",
		"Double  ",
		NULL
	};

	static const char *flare_style_names[] = {
		"Random           ",
		"Center Corona    ",
		"Top Right Star   ",
		"Bottom Right Star",
		"Top Left Star    ",
		"Bottom Left Star ",
		"Center Star      ",
		NULL
	};

	static const char *blur_names[] = {
		"Disabled        ",
		"Underwater only ",
		"Global          ",
		NULL
	};

	s_qudos_menu.x = viddef.width / 2;
	s_qudos_menu.y = viddef.height / 2 - 250;
	s_qudos_menu.nitems = 0;

	s_qudos_graphic.generic.type = MTYPE_SEPARATOR;
	s_qudos_graphic.generic.name = "Graphic Options";
	s_qudos_graphic.generic.x = 50;
	s_qudos_graphic.generic.y = 0;

	s_qudos_gl_reflection.generic.type = MTYPE_SLIDER;
	s_qudos_gl_reflection.generic.x = 0;
	s_qudos_gl_reflection.generic.y = 20;
	s_qudos_gl_reflection.generic.name = "Water Reflection";
	s_qudos_gl_reflection.generic.callback = WaterReflection;
	s_qudos_gl_reflection.minvalue = 0;
	s_qudos_gl_reflection.maxvalue = 10;
	s_qudos_gl_reflection.generic.statusbar = "Nice water reflections";

	s_qudos_gl_reflection_shader.generic.type = MTYPE_SPINCONTROL;
	s_qudos_gl_reflection_shader.generic.x = 0;
	s_qudos_gl_reflection_shader.generic.y = 30;
	s_qudos_gl_reflection_shader.generic.name = "Water Shader Reflection";
	s_qudos_gl_reflection_shader.generic.callback = WaterShaderReflection;
	s_qudos_gl_reflection_shader.itemnames = yesno_names;
	s_qudos_gl_reflection_shader.generic.statusbar = "Nice water shader reflections";

	s_qudos_texshader_warp.generic.type = MTYPE_SPINCONTROL;
	s_qudos_texshader_warp.generic.x = 0;
	s_qudos_texshader_warp.generic.y = 40;
	s_qudos_texshader_warp.generic.name = "Water Texture Shader Warp";
	s_qudos_texshader_warp.generic.callback = TexShaderWarp;
	s_qudos_texshader_warp.itemnames = yesno_names;
	s_qudos_texshader_warp.generic.statusbar = "Water pixel shaders";

	s_qudos_waves.generic.type = MTYPE_SLIDER;
	s_qudos_waves.generic.x = 0;
	s_qudos_waves.generic.y = 50;
	s_qudos_waves.generic.name = "Water Waves";
	s_qudos_waves.generic.callback = WaterWavesFunc;
	s_qudos_waves.minvalue = 0;
	s_qudos_waves.maxvalue = 10;
	s_qudos_waves.generic.statusbar = "Water Waves Level";

	s_qudos_water_trans.generic.type = MTYPE_SPINCONTROL;
	s_qudos_water_trans.generic.x = 0;
	s_qudos_water_trans.generic.y = 60;
	s_qudos_water_trans.generic.name = "Water Transparency";
	s_qudos_water_trans.generic.callback = TransWaterFunc;
	s_qudos_water_trans.itemnames = yesno_names;
	s_qudos_water_trans.generic.statusbar = "Water Transparency, no available in Deathmatch or CTF";

	s_qudos_caustics.generic.type = MTYPE_SPINCONTROL;
	s_qudos_caustics.generic.x = 0;
	s_qudos_caustics.generic.y = 70;
	s_qudos_caustics.generic.name = "Water Caustics";
	s_qudos_caustics.generic.callback = TexCausticFunc;
	s_qudos_caustics.itemnames = caustics_names;
	s_qudos_caustics.generic.statusbar = "Water Caustics";

	s_qudos_underwater_mov.generic.type = MTYPE_SPINCONTROL;
	s_qudos_underwater_mov.generic.x = 0;
	s_qudos_underwater_mov.generic.y = 80;
	s_qudos_underwater_mov.generic.name = "Under Water Movement";
	s_qudos_underwater_mov.generic.callback = UnderWaterMovFunc;
	s_qudos_underwater_mov.itemnames = yesno_names;
	s_qudos_underwater_mov.generic.statusbar = "Under water movement, ala Quake III";

	s_qudos_fog.generic.type = MTYPE_SPINCONTROL;
	s_qudos_fog.generic.x = 0;
	s_qudos_fog.generic.y = 100;
	s_qudos_fog.generic.name = "Fog Ambience";
	s_qudos_fog.generic.callback = FogFunc;
	s_qudos_fog.itemnames = yesno_names;
	s_qudos_fog.generic.statusbar = "Fog Ambience";

	s_qudos_fog_density.generic.type = MTYPE_SLIDER;
	s_qudos_fog_density.generic.x = 0;
	s_qudos_fog_density.generic.y = 110;
	s_qudos_fog_density.generic.name = "Fog density";
	s_qudos_fog_density.generic.callback = FogDensityFunc;
	s_qudos_fog_density.generic.statusbar = "Fog density";
	s_qudos_fog_density.minvalue = 1;
	s_qudos_fog_density.maxvalue = 40;

	s_qudos_fog_red.generic.type = MTYPE_SLIDER;
	s_qudos_fog_red.generic.x = 0;
	s_qudos_fog_red.generic.y = 120;
	s_qudos_fog_red.generic.name = "Fog Red";
	s_qudos_fog_red.generic.callback = FogRedFunc;
	s_qudos_fog_red.minvalue = 0;
	s_qudos_fog_red.maxvalue = 10;
	s_qudos_fog_red.generic.statusbar = "Fog Red";

	s_qudos_fog_green.generic.type = MTYPE_SLIDER;
	s_qudos_fog_green.generic.x = 0;
	s_qudos_fog_green.generic.y = 130;
	s_qudos_fog_green.generic.name = "Fog Green";
	s_qudos_fog_green.generic.callback = FogGreenFunc;
	s_qudos_fog_green.minvalue = 0;
	s_qudos_fog_green.maxvalue = 10;
	s_qudos_fog_green.generic.statusbar = "Fog Green";

	s_qudos_fog_blue.generic.type = MTYPE_SLIDER;
	s_qudos_fog_blue.generic.x = 0;
	s_qudos_fog_blue.generic.y = 140;
	s_qudos_fog_blue.generic.name = "Fog Blue";
	s_qudos_fog_blue.generic.callback = FogBlueFunc;
	s_qudos_fog_blue.minvalue = 0;
	s_qudos_fog_blue.maxvalue = 10;
	s_qudos_fog_blue.generic.statusbar = "Fog Blue";
	
	s_qudos_fog_underwater.generic.type = MTYPE_SPINCONTROL;
	s_qudos_fog_underwater.generic.x = 0;
	s_qudos_fog_underwater.generic.y = 150;
	s_qudos_fog_underwater.generic.name = "Under Water Fog";
	s_qudos_fog_underwater.generic.callback = FogUnderWaterFunc;
	s_qudos_fog_underwater.itemnames = yesno_names;
	s_qudos_fog_underwater.generic.statusbar = "Under Water Fog";

	s_qudos_minimap.generic.type = MTYPE_SPINCONTROL;
	s_qudos_minimap.generic.x = 0;
	s_qudos_minimap.generic.y = 170;
	s_qudos_minimap.generic.name = "Mini Map";
	s_qudos_minimap.generic.callback = MiniMapFunc;
	s_qudos_minimap.itemnames = minimap_names;
	s_qudos_minimap.generic.statusbar = "Mini Map, no available in Deathmatch or CTF";

	s_qudos_minimap_style.generic.type = MTYPE_SPINCONTROL;
	s_qudos_minimap_style.generic.x = 0;
	s_qudos_minimap_style.generic.y = 180;
	s_qudos_minimap_style.generic.name = "Mini Map Style";
	s_qudos_minimap_style.generic.callback = MiniMapStyleFunc;
	s_qudos_minimap_style.itemnames = minimap_style_names;
	s_qudos_minimap_style.generic.statusbar = "Mini Map Style";

	s_qudos_minimap_size.generic.type = MTYPE_SLIDER;
	s_qudos_minimap_size.generic.x = 0;
	s_qudos_minimap_size.generic.y = 190;
	s_qudos_minimap_size.generic.name = "Mini Map Size";
	s_qudos_minimap_size.generic.callback = MiniMapSizeFunc;
	s_qudos_minimap_size.minvalue = 200;
	s_qudos_minimap_size.maxvalue = 600;
	s_qudos_minimap_size.generic.statusbar = "Mini Map Size";

	s_qudos_minimap_zoom.generic.type = MTYPE_SLIDER;
	s_qudos_minimap_zoom.generic.x = 0;
	s_qudos_minimap_zoom.generic.y = 200;
	s_qudos_minimap_zoom.generic.name = "Mini Map Zoom";
	s_qudos_minimap_zoom.generic.callback = MiniMapZoomFunc;
	s_qudos_minimap_zoom.minvalue = 0;
	s_qudos_minimap_zoom.maxvalue = 20;
	s_qudos_minimap_zoom.generic.statusbar = "Mini Map Zoom";

	s_qudos_minimap_x.generic.type = MTYPE_SLIDER;
	s_qudos_minimap_x.generic.x = 0;
	s_qudos_minimap_x.generic.y = 210;
	s_qudos_minimap_x.generic.name = "X Position";
	s_qudos_minimap_x.generic.callback = MiniMapXPosFunc;
	s_qudos_minimap_x.minvalue = 200;
	s_qudos_minimap_x.maxvalue = 2048;
	s_qudos_minimap_x.generic.statusbar = "Mini Map X position on the screen";

	s_qudos_minimap_y.generic.type = MTYPE_SLIDER;
	s_qudos_minimap_y.generic.x = 0;
	s_qudos_minimap_y.generic.y = 220;
	s_qudos_minimap_y.generic.name = "Y Position";
	s_qudos_minimap_y.generic.callback = MiniMapYPosFunc;
	s_qudos_minimap_y.minvalue = 200;
	s_qudos_minimap_y.maxvalue = 1600;
	s_qudos_minimap_y.generic.statusbar = "Mini Map Y position on the screen";

	s_qudos_det_tex.generic.type = MTYPE_SLIDER;
	s_qudos_det_tex.generic.x = 0;
	s_qudos_det_tex.generic.y = 240;
	s_qudos_det_tex.generic.name = "Detailed Textures";
	s_qudos_det_tex.generic.callback = DetTexFunc;
	s_qudos_det_tex.minvalue = 0;
	s_qudos_det_tex.maxvalue = 20;
	s_qudos_det_tex.generic.statusbar = "Detailed textures value";

	s_qudos_shadows.generic.type = MTYPE_SPINCONTROL;
	s_qudos_shadows.generic.x = 0;
	s_qudos_shadows.generic.y = 250;
	s_qudos_shadows.generic.name = "Shadows";
	s_qudos_shadows.generic.callback = ShadowsFunc;
	s_qudos_shadows.itemnames = shadows_names;
	s_qudos_shadows.generic.statusbar = "Shadows";

	s_qudos_gl_stainmaps.generic.type = MTYPE_SPINCONTROL;
	s_qudos_gl_stainmaps.generic.x = 0;
	s_qudos_gl_stainmaps.generic.y = 260;
	s_qudos_gl_stainmaps.generic.name = "Stain Maps";
	s_qudos_gl_stainmaps.generic.callback = StainMapsFunc;
	s_qudos_gl_stainmaps.itemnames = yesno_names;
	s_qudos_gl_stainmaps.generic.statusbar = "Stain Maps";

	s_qudos_cellshading.generic.type = MTYPE_SPINCONTROL;
	s_qudos_cellshading.generic.x = 0;
	s_qudos_cellshading.generic.y = 270;
	s_qudos_cellshading.generic.name = "Cell Shading";
	s_qudos_cellshading.generic.callback = CellShadingFunc;
	s_qudos_cellshading.itemnames = yesno_names;
	s_qudos_cellshading.generic.statusbar = "Cell Shading";

	s_qudos_shading.generic.type = MTYPE_SPINCONTROL;
	s_qudos_shading.generic.x = 0;
	s_qudos_shading.generic.y = 280;
	s_qudos_shading.generic.name = "Model Lightning";
	s_qudos_shading.generic.callback = ShadingFunc;
	s_qudos_shading.itemnames = yesno_names;
	s_qudos_shading.generic.statusbar = "Model Lightning";

	s_qudos_motionblur.generic.type = MTYPE_SPINCONTROL;
	s_qudos_motionblur.generic.x = 0;
	s_qudos_motionblur.generic.y = 300;
	s_qudos_motionblur.generic.name = "Motion Blur";
	s_qudos_motionblur.generic.callback = MotionBlurFunc;
	s_qudos_motionblur.itemnames = blur_names;
	s_qudos_motionblur.generic.statusbar = "Motion Blur";

	s_qudos_motionblur_intens.generic.type = MTYPE_SLIDER;
	s_qudos_motionblur_intens.generic.x = 0;
	s_qudos_motionblur_intens.generic.y = 310;
	s_qudos_motionblur_intens.generic.name = "Motion Blur Intensity";
	s_qudos_motionblur_intens.generic.callback = MotionBlurIntensFunc;
	s_qudos_motionblur_intens.minvalue = 0;
	s_qudos_motionblur_intens.maxvalue = 10;
	s_qudos_motionblur_intens.generic.statusbar = "Motion Blur Intensity";

	s_qudos_blooms.generic.type = MTYPE_SPINCONTROL;
	s_qudos_blooms.generic.x = 0;
	s_qudos_blooms.generic.y = 330;
	s_qudos_blooms.generic.name = "Blooms";
	s_qudos_blooms.generic.callback = BloomsFunc;
	s_qudos_blooms.itemnames = yesno_names;
	s_qudos_blooms.generic.statusbar = "Blooms effects";

	s_qudos_blooms_alpha.generic.type = MTYPE_SLIDER;
	s_qudos_blooms_alpha.generic.x = 0;
	s_qudos_blooms_alpha.generic.y = 340;
	s_qudos_blooms_alpha.generic.name = "Blooms alpha";
	s_qudos_blooms_alpha.generic.callback = BloomsAlphaFunc;
	s_qudos_blooms_alpha.generic.statusbar = "Blooms alpha";
	s_qudos_blooms_alpha.minvalue = 0;
	s_qudos_blooms_alpha.maxvalue = 10;

	s_qudos_blooms_diamond_size.generic.type = MTYPE_SLIDER;
	s_qudos_blooms_diamond_size.generic.x = 0;
	s_qudos_blooms_diamond_size.generic.y = 350;
	s_qudos_blooms_diamond_size.generic.name = "Blooms diamond size";
	s_qudos_blooms_diamond_size.generic.callback = BloomsDiamondSizeFunc;
	s_qudos_blooms_diamond_size.generic.statusbar = "Blooms diamond size";
	s_qudos_blooms_diamond_size.minvalue = 2;
	s_qudos_blooms_diamond_size.maxvalue = 4;

	s_qudos_blooms_intensity.generic.type = MTYPE_SLIDER;
	s_qudos_blooms_intensity.generic.x = 0;
	s_qudos_blooms_intensity.generic.y = 360;
	s_qudos_blooms_intensity.generic.name = "Blooms intensity";
	s_qudos_blooms_intensity.generic.callback = BloomsIntensityFunc;
	s_qudos_blooms_intensity.generic.statusbar = "Blooms intensity";
	s_qudos_blooms_intensity.minvalue = 0;
	s_qudos_blooms_intensity.maxvalue = 10;

	s_qudos_blooms_darken.generic.type = MTYPE_SLIDER;
	s_qudos_blooms_darken.generic.x = 0;
	s_qudos_blooms_darken.generic.y = 370;
	s_qudos_blooms_darken.generic.name = "Blooms darken";
	s_qudos_blooms_darken.generic.callback = BloomsDarkenFunc;
	s_qudos_blooms_darken.generic.statusbar = "Blooms darken";
	s_qudos_blooms_darken.minvalue = 0;
	s_qudos_blooms_darken.maxvalue = 40;

	s_qudos_blooms_sample_size.generic.type = MTYPE_SLIDER;
	s_qudos_blooms_sample_size.generic.x = 0;
	s_qudos_blooms_sample_size.generic.y = 380;
	s_qudos_blooms_sample_size.generic.name = "Blooms sample size";
	s_qudos_blooms_sample_size.generic.callback = BloomsSampleSizeFunc;
	s_qudos_blooms_sample_size.generic.statusbar =
	"Blooms sample size, scale is logarithmic instead of linear. Needs vid_restart.";
	s_qudos_blooms_sample_size.minvalue = 5;
	s_qudos_blooms_sample_size.maxvalue = 11;

	s_qudos_blooms_fast_sample.generic.type = MTYPE_SPINCONTROL;
	s_qudos_blooms_fast_sample.generic.x = 0;
	s_qudos_blooms_fast_sample.generic.y = 390;
	s_qudos_blooms_fast_sample.generic.name = "Blooms fast sample";
	s_qudos_blooms_fast_sample.generic.callback = BloomsFastSampleFunc;
	s_qudos_blooms_fast_sample.generic.statusbar = "Blooms fast sample";
	s_qudos_blooms_fast_sample.itemnames = yesno_names;

	s_qudos_lensflare.generic.type = MTYPE_SPINCONTROL;
	s_qudos_lensflare.generic.x = 0;
	s_qudos_lensflare.generic.y = 410;
	s_qudos_lensflare.generic.name = "Lens Flares";
	s_qudos_lensflare.generic.callback = LensFlareFunc;
	s_qudos_lensflare.itemnames = yesno_names;
	s_qudos_lensflare.generic.statusbar = "Lens Flares";

	s_qudos_lensflare_style.generic.type = MTYPE_SPINCONTROL;
	s_qudos_lensflare_style.generic.x = 0;
	s_qudos_lensflare_style.generic.y = 420;
	s_qudos_lensflare_style.generic.name = "Lens Flares Style";
	s_qudos_lensflare_style.generic.callback = LensFlareStyleFunc;
	s_qudos_lensflare_style.itemnames = flare_style_names;
	s_qudos_lensflare_style.generic.statusbar = "Lens Flares Style";

	s_qudos_lensflare_intens.generic.type = MTYPE_SLIDER;
	s_qudos_lensflare_intens.generic.x = 0;
	s_qudos_lensflare_intens.generic.y = 430;
	s_qudos_lensflare_intens.generic.name = "Lens Flares Intensity";
	s_qudos_lensflare_intens.generic.callback = LensFlareIntensFunc;
	s_qudos_lensflare_intens.minvalue = 0;
	s_qudos_lensflare_intens.maxvalue = 20;
	s_qudos_lensflare_intens.generic.statusbar = "Lens Flares Intensity";

	s_qudos_lensflare_scale.generic.type = MTYPE_SLIDER;
	s_qudos_lensflare_scale.generic.x = 0;
	s_qudos_lensflare_scale.generic.y = 440;
	s_qudos_lensflare_scale.generic.name = "Lens Flares Scale";
	s_qudos_lensflare_scale.generic.callback = LensFlareScaleFunc;
	s_qudos_lensflare_scale.minvalue = 0;
	s_qudos_lensflare_scale.maxvalue = 40;
	s_qudos_lensflare_scale.generic.statusbar = "Lens Flares Scale";

	s_qudos_lensflare_maxdist.generic.type = MTYPE_SLIDER;
	s_qudos_lensflare_maxdist.generic.x = 0;
	s_qudos_lensflare_maxdist.generic.y = 450;
	s_qudos_lensflare_maxdist.generic.name = "Lens Flares Max Size";
	s_qudos_lensflare_maxdist.generic.callback = LensFlareDistFunc;
	s_qudos_lensflare_maxdist.minvalue = 0;
	s_qudos_lensflare_maxdist.maxvalue = 300;
	s_qudos_lensflare_maxdist.generic.statusbar
	= "Lens Flares maximun size, note that 0 is unlimited";

	s_qudos_glares.generic.type = MTYPE_SPINCONTROL;
	s_qudos_glares.generic.x = 0;
	s_qudos_glares.generic.y = 465;
	s_qudos_glares.generic.name = "Glares";
	s_qudos_glares.generic.callback = GlaresFunc;
	s_qudos_glares.itemnames = yesno_names;
	s_qudos_glares.generic.statusbar = "Glares effects";

	s_qudos_lightmap_scale.generic.type = MTYPE_SLIDER;
	s_qudos_lightmap_scale.generic.x = 0;
	s_qudos_lightmap_scale.generic.y = 475;
	s_qudos_lightmap_scale.generic.name = "Lightmap Scale";
	s_qudos_lightmap_scale.generic.callback = LightMapScaleFunc;
	s_qudos_lightmap_scale.minvalue = 1;
	s_qudos_lightmap_scale.maxvalue = 20;
	s_qudos_lightmap_scale.generic.statusbar = "Light map scale";

	ControlsSetMenuItemValues();

	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_graphic);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_gl_reflection);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_gl_reflection_shader);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_texshader_warp);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_waves);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_water_trans);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_caustics);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_underwater_mov);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_fog);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_fog_density);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_fog_red);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_fog_green);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_fog_blue);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_fog_underwater);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_minimap);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_minimap_style);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_minimap_size);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_minimap_zoom);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_minimap_x);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_minimap_y);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_det_tex);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_shadows);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_gl_stainmaps);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_cellshading);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_shading);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_motionblur);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_motionblur_intens);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_blooms);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_blooms_alpha);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_blooms_diamond_size);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_blooms_intensity);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_blooms_darken);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_blooms_sample_size);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_blooms_fast_sample);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_lensflare);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_lensflare_style);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_lensflare_intens);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_lensflare_scale);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_lensflare_maxdist);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_glares);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_lightmap_scale);

}

void
QuDos_MenuInit_Hud(void)
{
	static const char *yesno_names[] = {
		"Disabled",
		"Enabled ",
		NULL
	};

	static const char *hudscale_names[] = {
		"Default ",
		"1024x768",
		"800x600 ",
		"600x480 ",
		"400x320 ",
		NULL
	};

	static const char *clock_names[] = {
		"Disabled   ",
		"24 H Format",
		"12 H Format",
		NULL
	};

	static const char *highlight_names[] = {
		"Disabled     ",
		"Only non-chat",
		"All          ",
		NULL
	};

	static const char *netgraph_names[] = {
		"Disabled     ",
		"Original     ",
		"Box          ",
		"Box plus Info",
		NULL
	};

	static const char *netgraph_pos_names[] = {
		"Custom      ",
		"Bottom Right",
		"Bottom Left ",
		"Top Right   ",
		"Top Left    ",
		NULL
	};

	static const char *crosshair_names[] = {
		"None    ",
		"Cross 1 ",
		"Cross 2 ",
		"Cross 3 ",
		"Cross 4 ",
		"Cross 5 ",
		"Cross 6 ",
		"Cross 7 ",
		"Cross 8 ",
		"Cross 9 ",
		"Cross 10",
		"Cross 11",
		"Cross 12",
		"Cross 13",
		"Cross 14",
		"Cross 15",
		"Cross 16",
		"Cross 17",
		"Cross 18",
		"Cross 19",
		"Cross 20",
		NULL
	};

	s_qudos_menu.x = viddef.width / 2;
	s_qudos_menu.y = viddef.height / 2 - 250;
	s_qudos_menu.nitems = 0;

	s_qudos_hud.generic.type = MTYPE_SEPARATOR;
	s_qudos_hud.generic.name = "Hud Options";
	s_qudos_hud.generic.x = 50;
	s_qudos_hud.generic.y = 0;

	s_qudos_hudscale.generic.type = MTYPE_SPINCONTROL;
	s_qudos_hudscale.generic.x = 0;
	s_qudos_hudscale.generic.y = 20;
	s_qudos_hudscale.generic.name = "Hud Size";
	s_qudos_hudscale.generic.callback = HudScaleFunc;
	s_qudos_hudscale.itemnames = hudscale_names;
	s_qudos_hudscale.generic.statusbar =
	"Warning: Menus, hud text and also console text are affected too, buggy";

	s_qudos_drawclock.generic.type = MTYPE_SPINCONTROL;
	s_qudos_drawclock.generic.x = 0;
	s_qudos_drawclock.generic.y = 30;
	s_qudos_drawclock.generic.name = "Clock";
	s_qudos_drawclock.generic.callback = DrawClockFunc;
	s_qudos_drawclock.itemnames = clock_names;
	s_qudos_drawclock.generic.statusbar =
	"Draw a clock in the screen, type cl_drawclock 2 for 12/24 format";

	s_qudos_drawdate.generic.type = MTYPE_SPINCONTROL;
	s_qudos_drawdate.generic.x = 0;
	s_qudos_drawdate.generic.y = 40;
	s_qudos_drawdate.generic.name = "Date";
	s_qudos_drawdate.generic.callback = DrawDateFunc;
	s_qudos_drawdate.itemnames = yesno_names;
	s_qudos_drawdate.generic.statusbar = "Draw the date in the screen";

	s_qudos_drawmaptime.generic.type = MTYPE_SPINCONTROL;
	s_qudos_drawmaptime.generic.x = 0;
	s_qudos_drawmaptime.generic.y = 50;
	s_qudos_drawmaptime.generic.name = "Ingame Time";
	s_qudos_drawmaptime.generic.callback = DrawMapTimeFunc;
	s_qudos_drawmaptime.itemnames = yesno_names;
	s_qudos_drawmaptime.generic.statusbar = "Draw a clock playing the game";

	s_qudos_drawfps.generic.type = MTYPE_SPINCONTROL;
	s_qudos_drawfps.generic.x = 0;
	s_qudos_drawfps.generic.y = 60;
	s_qudos_drawfps.generic.name = "FPS";
	s_qudos_drawfps.generic.callback = DrawFPSFunc;
	s_qudos_drawfps.itemnames = yesno_names;
	s_qudos_drawfps.generic.statusbar = "Draw frames-per-second on the hud";

	s_qudos_drawping.generic.type = MTYPE_SPINCONTROL;
	s_qudos_drawping.generic.x = 0;
	s_qudos_drawping.generic.y = 70;
	s_qudos_drawping.generic.name = "Ping";
	s_qudos_drawping.generic.callback = DrawPingFunc;
	s_qudos_drawping.itemnames = yesno_names;
	s_qudos_drawping.generic.statusbar = "Draw ping on the hud";

	s_qudos_drawrate.generic.type = MTYPE_SPINCONTROL;
	s_qudos_drawrate.generic.x = 0;
	s_qudos_drawrate.generic.y = 80;
	s_qudos_drawrate.generic.name = "Net rate";
	s_qudos_drawrate.generic.callback = DrawRateFunc;
	s_qudos_drawrate.itemnames = yesno_names;
	s_qudos_drawrate.generic.statusbar = "Draw upload/download rate on the hud";

	s_qudos_drawchathud.generic.type = MTYPE_SPINCONTROL;
	s_qudos_drawchathud.generic.x = 0;
	s_qudos_drawchathud.generic.y = 90;
	s_qudos_drawchathud.generic.name = "Chat Messages";
	s_qudos_drawchathud.generic.callback = DrawChatHudFunc;
	s_qudos_drawchathud.itemnames = yesno_names;
	s_qudos_drawchathud.generic.statusbar = "Keep the chat messages on the hud, 4 lines";

	s_qudos_draw_x.generic.type = MTYPE_SLIDER;
	s_qudos_draw_x.generic.x = 0;
	s_qudos_draw_x.generic.y = 110;
	s_qudos_draw_x.generic.name = "'X' location";
	s_qudos_draw_x.generic.callback = UpdateHudXFunc;
	s_qudos_draw_x.minvalue = 0;
	s_qudos_draw_x.maxvalue = 20;
	s_qudos_draw_x.generic.statusbar = "Percentage down screen for hud";

	s_qudos_draw_y.generic.type = MTYPE_SLIDER;
	s_qudos_draw_y.generic.x = 0;
	s_qudos_draw_y.generic.y = 120;
	s_qudos_draw_y.generic.name = "'Y' location";
	s_qudos_draw_y.generic.callback = UpdateHudYFunc;
	s_qudos_draw_y.minvalue = 0;
	s_qudos_draw_y.maxvalue = 20;
	s_qudos_draw_y.generic.statusbar = "Percentage across screen for hud";

	s_qudos_drawplayername.generic.type = MTYPE_SPINCONTROL;
	s_qudos_drawplayername.generic.x = 0;
	s_qudos_drawplayername.generic.y = 140;
	s_qudos_drawplayername.generic.name = "Draw Player Name";
	s_qudos_drawplayername.generic.callback = DrawPlayerNameFunc;
	s_qudos_drawplayername.itemnames = yesno_names;
	s_qudos_drawplayername.generic.statusbar = "Draw the player name over the crosshair";

	s_qudos_highlight.generic.type = MTYPE_SPINCONTROL;
	s_qudos_highlight.generic.x = 0;
	s_qudos_highlight.generic.y = 150;
	s_qudos_highlight.generic.name = "Message highlight";
	s_qudos_highlight.generic.callback = HighlightFunc;
	s_qudos_highlight.itemnames = highlight_names;
	s_qudos_highlight.generic.statusbar = "Highlight player names";

	s_qudos_drawnetgraph.generic.type = MTYPE_SPINCONTROL;
	s_qudos_drawnetgraph.generic.x = 0;
	s_qudos_drawnetgraph.generic.y = 170;
	s_qudos_drawnetgraph.generic.name = "Net Graphic";
	s_qudos_drawnetgraph.generic.callback = DrawNetGraphFunc;
	s_qudos_drawnetgraph.itemnames = netgraph_names;
	s_qudos_drawnetgraph.generic.statusbar =
	"Draw net graphic in mode basic or showing fps, ping, download and upload, time, ...";

	s_qudos_drawnetgraph_pos.generic.type = MTYPE_SPINCONTROL;
	s_qudos_drawnetgraph_pos.generic.x = 0;
	s_qudos_drawnetgraph_pos.generic.y = 180;
	s_qudos_drawnetgraph_pos.generic.name = "Net Graphic Position";
	s_qudos_drawnetgraph_pos.generic.callback = DrawNetGraphPosFunc;
	s_qudos_drawnetgraph_pos.itemnames = netgraph_pos_names;
	s_qudos_drawnetgraph_pos.generic.statusbar = "Net graphic hud location";

	s_qudos_drawnetgraph_alpha.generic.type = MTYPE_SLIDER;
	s_qudos_drawnetgraph_alpha.generic.x = 0;
	s_qudos_drawnetgraph_alpha.generic.y = 190;
	s_qudos_drawnetgraph_alpha.generic.name = "Net Graphic Alpha";
	s_qudos_drawnetgraph_alpha.generic.callback = DrawNetGraphAlphaFunc;
	s_qudos_drawnetgraph_alpha.minvalue = 0;
	s_qudos_drawnetgraph_alpha.maxvalue = 10;
	s_qudos_drawnetgraph_alpha.generic.statusbar = "Net graphic Alpha";

	s_qudos_drawnetgraph_scale.generic.type = MTYPE_SLIDER;
	s_qudos_drawnetgraph_scale.generic.x = 0;
	s_qudos_drawnetgraph_scale.generic.y = 200;
	s_qudos_drawnetgraph_scale.generic.name = "Net Graphic Scale";
	s_qudos_drawnetgraph_scale.generic.callback = DrawNetGraphScaleFunc;
	s_qudos_drawnetgraph_scale.minvalue = 110;
	s_qudos_drawnetgraph_scale.maxvalue = 200;
	s_qudos_drawnetgraph_scale.generic.statusbar = "Net graphic Scale";

	s_qudos_netgraph_image.generic.type = MTYPE_FIELD;
	s_qudos_netgraph_image.generic.name = "Netgraph Image";
	s_qudos_netgraph_image.generic.callback = 0;
	s_qudos_netgraph_image.generic.x = 0;
	s_qudos_netgraph_image.generic.y = 215;
	s_qudos_netgraph_image.length = 20;
	s_qudos_netgraph_image.visible_length = 10;
	Q_strncpyz(s_qudos_netgraph_image.buffer, scr_netgraph_image->string, sizeof(s_qudos_netgraph_image.buffer));
	s_qudos_netgraph_image.cursor = strlen(scr_netgraph_image->string);
	s_qudos_netgraph_image.generic.statusbar =
	"Type the name for the netgraph image in the box and press the key INSERT, only available on Box mode";

	s_qudos_drawnetgraph_pos_x.generic.type = MTYPE_SLIDER;
	s_qudos_drawnetgraph_pos_x.generic.x = 0;
	s_qudos_drawnetgraph_pos_x.generic.y = 230;
	s_qudos_drawnetgraph_pos_x.generic.name = "X Position";
	s_qudos_drawnetgraph_pos_x.generic.callback = DrawNetGraphPosXFunc;
	s_qudos_drawnetgraph_pos_x.minvalue = 0;
	s_qudos_drawnetgraph_pos_x.maxvalue = 2048;
	s_qudos_drawnetgraph_pos_x.generic.statusbar = 
	"Net graphic X position on the hud, only available on Box mode";

	s_qudos_drawnetgraph_pos_y.generic.type = MTYPE_SLIDER;
	s_qudos_drawnetgraph_pos_y.generic.x = 0;
	s_qudos_drawnetgraph_pos_y.generic.y = 240;
	s_qudos_drawnetgraph_pos_y.generic.name = "Y Position";
	s_qudos_drawnetgraph_pos_y.generic.callback = DrawNetGraphPosYFunc;
	s_qudos_drawnetgraph_pos_y.minvalue = 0;
	s_qudos_drawnetgraph_pos_y.maxvalue = 1600;
	s_qudos_drawnetgraph_pos_y.generic.statusbar = 
	"Net graphic Y position on the hud, only available on Box mode";

	s_qudos_crosshair_box.generic.type = MTYPE_SPINCONTROL;
	s_qudos_crosshair_box.generic.x = 0;
	s_qudos_crosshair_box.generic.y = 260;
	s_qudos_crosshair_box.generic.name = "Crosshair";
	s_qudos_crosshair_box.generic.callback = CrosshairFunc;
	s_qudos_crosshair_box.itemnames = crosshair_names;

	s_qudos_cross_scale.generic.type = MTYPE_SLIDER;
	s_qudos_cross_scale.generic.x = 0;
	s_qudos_cross_scale.generic.y = 270;
	s_qudos_cross_scale.generic.name = "Size";
	s_qudos_cross_scale.generic.callback = CrossScaleFunc;
	s_qudos_cross_scale.minvalue = 3;
	s_qudos_cross_scale.maxvalue = 10;

	s_qudos_cross_red.generic.type = MTYPE_SLIDER;
	s_qudos_cross_red.generic.x = 0;
	s_qudos_cross_red.generic.y = 280;
	s_qudos_cross_red.generic.name = "Red";
	s_qudos_cross_red.generic.callback = CrossColorRedFunc;
	s_qudos_cross_red.minvalue = 0;
	s_qudos_cross_red.maxvalue = 10;

	s_qudos_cross_green.generic.type = MTYPE_SLIDER;
	s_qudos_cross_green.generic.x = 0;
	s_qudos_cross_green.generic.y = 290;
	s_qudos_cross_green.generic.name = "Green";
	s_qudos_cross_green.generic.callback = CrossColorGreenFunc;
	s_qudos_cross_green.minvalue = 0;
	s_qudos_cross_green.maxvalue = 10;

	s_qudos_cross_blue.generic.type = MTYPE_SLIDER;
	s_qudos_cross_blue.generic.x = 0;
	s_qudos_cross_blue.generic.y = 300;
	s_qudos_cross_blue.generic.name = "Blue";
	s_qudos_cross_blue.generic.callback = CrossColorBlueFunc;
	s_qudos_cross_blue.minvalue = 0;
	s_qudos_cross_blue.maxvalue = 10;

	s_qudos_cross_pulse.generic.type = MTYPE_SLIDER;
	s_qudos_cross_pulse.generic.x = 0;
	s_qudos_cross_pulse.generic.y = 310;
	s_qudos_cross_pulse.generic.name = "Pulse";
	s_qudos_cross_pulse.generic.callback = CrossPulseFunc;
	s_qudos_cross_pulse.minvalue = 0;
	s_qudos_cross_pulse.maxvalue = 3;

	s_qudos_cross_alpha.generic.type = MTYPE_SLIDER;
	s_qudos_cross_alpha.generic.x = 0;
	s_qudos_cross_alpha.generic.y = 320;
	s_qudos_cross_alpha.generic.name = "Transparency";
	s_qudos_cross_alpha.generic.callback = CrossAlphaFunc;
	s_qudos_cross_alpha.minvalue = 2;
	s_qudos_cross_alpha.maxvalue = 10;

	s_qudos_hud_red.generic.type = MTYPE_SLIDER;
	s_qudos_hud_red.generic.x = 0;
	s_qudos_hud_red.generic.y = 340;
	s_qudos_hud_red.generic.name = "Hud Red";
	s_qudos_hud_red.generic.callback = HudColorRedFunc;
	s_qudos_hud_red.minvalue = 0;
	s_qudos_hud_red.maxvalue = 10;
	s_qudos_hud_red.generic.statusbar = "Color red intensity for numbers on the hud";

	s_qudos_hud_green.generic.type = MTYPE_SLIDER;
	s_qudos_hud_green.generic.x = 0;
	s_qudos_hud_green.generic.y = 350;
	s_qudos_hud_green.generic.name = "Hud Green";
	s_qudos_hud_green.generic.callback = HudColorGreenFunc;
	s_qudos_hud_green.minvalue = 0;
	s_qudos_hud_green.maxvalue = 10;
	s_qudos_hud_green.generic.statusbar = "Color green intensity for numbers on the hud";

	s_qudos_hud_blue.generic.type = MTYPE_SLIDER;
	s_qudos_hud_blue.generic.x = 0;
	s_qudos_hud_blue.generic.y = 360;
	s_qudos_hud_blue.generic.name = "Hud Blue";
	s_qudos_hud_blue.generic.callback = HudColorBlueFunc;
	s_qudos_hud_blue.minvalue = 0;
	s_qudos_hud_blue.maxvalue = 10;
	s_qudos_hud_blue.generic.statusbar = "Color blue intensity for numbers on the hud";

	s_qudos_hud_alpha.generic.type = MTYPE_SLIDER;
	s_qudos_hud_alpha.generic.x = 0;
	s_qudos_hud_alpha.generic.y = 370;
	s_qudos_hud_alpha.generic.name = "Hud Transparency";
	s_qudos_hud_alpha.generic.callback = HudAlphaFunc;
	s_qudos_hud_alpha.minvalue = 2;
	s_qudos_hud_alpha.maxvalue = 10;
	s_qudos_hud_alpha.generic.statusbar = "Alpha level for numbers, pics, icons, ... on the hud";

	s_qudos_font_color.generic.type = MTYPE_FIELD;
	s_qudos_font_color.generic.name = "Font Color";
	s_qudos_font_color.generic.callback = 0;
	s_qudos_font_color.generic.x = 0;
	s_qudos_font_color.generic.y = 390;
	s_qudos_font_color.length = 20;
	s_qudos_font_color.visible_length = 10;
	Q_strncpyz(s_qudos_font_color.buffer, font_color->string, sizeof(s_qudos_font_color.buffer));
	s_qudos_font_color.cursor = strlen(font_color->string);
	s_qudos_font_color.generic.statusbar =
	"Type the name for the font color in the box and press the key INSERT, from color1 to color10";

	s_qudos_conback_image.generic.type = MTYPE_FIELD;
	s_qudos_conback_image.generic.name = "Conback Image";
	s_qudos_conback_image.generic.callback = 0;
	s_qudos_conback_image.generic.x = 0;
	s_qudos_conback_image.generic.y = 410;
	s_qudos_conback_image.length = 20;
	s_qudos_conback_image.visible_length = 10;
	Q_strncpyz(s_qudos_conback_image.buffer, cl_conback_image->string, sizeof(s_qudos_conback_image.buffer));
	s_qudos_conback_image.cursor = strlen(cl_conback_image->string);
	s_qudos_conback_image.generic.statusbar =
	"Type the name for the conback in the box and press the key INSERT";

	s_qudos_menu_alpha.generic.type = MTYPE_SLIDER;
	s_qudos_menu_alpha.generic.x = 0;
	s_qudos_menu_alpha.generic.y = 425;
	s_qudos_menu_alpha.generic.name = "Menu Transparency";
	s_qudos_menu_alpha.generic.callback = MenuAlphaFunc;
	s_qudos_menu_alpha.minvalue = 0;
	s_qudos_menu_alpha.maxvalue = 10;
	s_qudos_menu_alpha.generic.statusbar = "Menu alpha level ingame";

	ControlsSetMenuItemValues();
	
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_hud);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_hudscale);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_drawclock);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_drawdate);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_drawmaptime);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_drawfps);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_drawping);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_drawrate);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_drawchathud);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_draw_x);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_draw_y);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_drawplayername);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_highlight);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_drawnetgraph);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_drawnetgraph_pos);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_drawnetgraph_alpha);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_drawnetgraph_scale);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_netgraph_image);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_drawnetgraph_pos_x);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_drawnetgraph_pos_y);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_crosshair_box);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_cross_scale);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_cross_red);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_cross_green);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_cross_blue);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_cross_pulse);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_cross_alpha);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_hud_red);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_hud_green);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_hud_blue);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_hud_alpha);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_font_color);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_conback_image);
	Menu_AddItem(&s_qudos_menu, (void *)&s_qudos_menu_alpha);

}

void
M_Menu_QuDos_f_Client(void)
{
	QuDos_MenuInit_Client();
	M_PushMenu(QuDos_MenuDraw, QuDos_MenuKey_Client);
}

void
M_Menu_QuDos_f_Graphic(void)
{
	QuDos_MenuInit_Graphic();
	M_PushMenu(QuDos_MenuDraw, QuDos_MenuKey_Graphic);
}

void
M_Menu_QuDos_f_Hud(void)
{
	QuDos_MenuInit_Hud();
	M_PushMenu(QuDos_MenuDraw, QuDos_MenuKey_Hud);
}


/*
 * =======================================================================
 *
 * CONTROLS MENU
 *
 * =======================================================================
 */
#ifdef Joystick
extern cvar_t  *in_joystick;
#endif
static menuframework_s s_options_menu;
static menuaction_s s_options_defaults_action;
static menuaction_s s_options_qudos_action;
static menuaction_s s_options_qudos_action_client;
static menuaction_s s_options_qudos_action_graphic;
static menuaction_s s_options_qudos_action_hud;
static menuaction_s s_options_customize_action;
static menuslider_s s_options_sensitivity_slider;
static menulist_s s_options_freelook_box;
static menulist_s s_options_alwaysrun_box;
static menulist_s s_options_invertmouse_box;
static menulist_s s_options_lookspring_box;
static menulist_s s_options_lookstrafe_box;
static menuslider_s s_options_sfxvolume_slider;
static menuslider_s s_options_ogg_volume;
static menulist_s s_options_ogg_sequence;
static menulist_s s_options_ogg_autoplay;
static menulist_s s_options_footsteps;
#ifdef Joystick
static menulist_s s_options_joystick_box;
#endif
static menuslider_s s_qudos_menucursor_scale;
static menuslider_s s_qudos_menucursor_red;
static menuslider_s s_qudos_menucursor_green;
static menuslider_s s_qudos_menucursor_blue;
static menuslider_s s_qudos_menucursor_alpha;
static menuslider_s s_qudos_menucursor_sensitivity;
static menulist_s   s_qudos_menucursor_type;
static menulist_s s_options_cdvolume_box;
static menulist_s s_options_cdshuffle_box;
static menulist_s s_options_quality_list;
static menulist_s s_options_console_action;
static menuslider_s s_options_console_height;
static menuslider_s s_options_console_trans;

static const char *ogg_autoplay_values[] = {
	"",
	"#1",
	"#-1",
	"?",
	NULL,
	NULL
};

static void
QuDosClientFunc(void *unused)
{
	M_Menu_QuDos_f_Client();
}

static void
QuDosGraphicFunc(void *unused)
{
	M_Menu_QuDos_f_Graphic();
}

static void
QuDosHudFunc(void *unused)
{
	M_Menu_QuDos_f_Hud();
}

static void
CustomizeControlsFunc(void *unused)
{
	M_Menu_Keys_f();
}

static void
AlwaysRunFunc(void *unused)
{
	Cvar_SetValue("cl_run", s_options_alwaysrun_box.curvalue);
}

static void
FreeLookFunc(void *unused)
{
	Cvar_SetValue("freelook", s_options_freelook_box.curvalue);
}

static void
MouseSpeedFunc(void *unused)
{
	Cvar_SetValue("sensitivity", s_options_sensitivity_slider.curvalue / 2.0);
}

void
ControlsSetMenuItemValues(void)
{
	s_options_sfxvolume_slider.curvalue = Cvar_VariableValue("s_volume") * 10;
	s_options_ogg_volume.curvalue = Cvar_VariableValue("ogg_volume") * 10;
	s_options_footsteps.curvalue = Cvar_VariableValue("cl_footsteps_override");
	s_options_cdvolume_box.curvalue = !Cvar_VariableValue("cd_nocd");

	s_options_cdshuffle_box.curvalue = Cvar_VariableValue("cd_shuffle");

	s_options_quality_list.curvalue = !Cvar_VariableValue("s_loadas8bit");

	switch ((int)Cvar_VariableValue("s_khz")) {
	case 44:
		s_options_quality_list.curvalue = 2;
		break;
	case 22:
		s_options_quality_list.curvalue = 1;
		break;
	default:
		s_options_quality_list.curvalue = 0;
		break;
	}

	s_options_sensitivity_slider.curvalue = (sensitivity->value) * 2;

	Cvar_SetValue("cl_run", ClampCvar(0, 1, cl_run->value));
	s_options_alwaysrun_box.curvalue = cl_run->value;

	s_options_invertmouse_box.curvalue = m_pitch->value < 0;

	Cvar_SetValue("lookspring", ClampCvar(0, 1, lookspring->value));
	s_options_lookspring_box.curvalue = lookspring->value;

	Cvar_SetValue("lookstrafe", ClampCvar(0, 1, lookstrafe->value));
	s_options_lookstrafe_box.curvalue = lookstrafe->value;

	Cvar_SetValue("freelook", ClampCvar(0, 1, freelook->value));
	s_options_freelook_box.curvalue = freelook->value;

#ifdef Joystick
	Cvar_SetValue("in_joystick", ClampCvar(0, 1, in_joystick->value));
	s_options_joystick_box.curvalue = in_joystick->value;
#endif

	Cvar_SetValue("cl_3dcam", ClampCvar(0, 1, Cvar_VariableValue("cl_3dcam")));
	s_qudos_3dcam.curvalue = Cvar_VariableValue("cl_3dcam");
	s_qudos_3dcam_distance.curvalue = Cvar_VariableValue("cl_3dcam_dist") / 25;
	s_qudos_3dcam_angle.curvalue = Cvar_VariableValue("cl_3dcam_angle") / 10;

	Cvar_SetValue("weap_shell", ClampCvar(0, 1, Cvar_VariableValue("weap_shell")));
	s_qudos_weap_shell.curvalue = Cvar_VariableValue("weap_shell");
	Cvar_SetValue("cl_bobbing", ClampCvar(0, 1, Cvar_VariableValue("cl_bobbing")));
	s_qudos_bobbing.curvalue = Cvar_VariableValue("cl_bobbing");
	s_qudos_gunalpha.curvalue = Cvar_VariableValue("cl_gunalpha") * 10;

	s_qudos_flashlight_red.curvalue = Cvar_VariableValue("cl_flashlight_red") * 10;
	s_qudos_flashlight_green.curvalue = Cvar_VariableValue("cl_flashlight_green") * 10;
	s_qudos_flashlight_blue.curvalue = Cvar_VariableValue("cl_flashlight_blue") * 10;
	s_qudos_flashlight_intensity.curvalue = Cvar_VariableValue("cl_flashlight_intensity") / 10;
	s_qudos_flashlight_distance.curvalue = Cvar_VariableValue("cl_flashlight_distance") / 10;
	s_qudos_flashlight_decscale.curvalue = Cvar_VariableValue("cl_flashlight_decscale") * 10;

#ifdef QMAX
	Cvar_SetValue("cl_blaster_color", ClampCvar(0, 3, Cvar_VariableValue("cl_blaster_color")));
	s_qudos_blaster_color.curvalue = Cvar_VariableValue("cl_blaster_color");
	Cvar_SetValue("cl_blaster_bubble", ClampCvar(0, 2, Cvar_VariableValue("cl_blaster_bubble")));
	s_qudos_blaster_bubble.curvalue = Cvar_VariableValue("cl_blaster_bubble");
	Cvar_SetValue("cl_hyperblaster_particles", ClampCvar(0, 1, Cvar_VariableValue("cl_hyperblaster_particles")));
	s_qudos_hyperblaster_particles.curvalue = Cvar_VariableValue("cl_hyperblaster_particles");
	Cvar_SetValue("cl_hyperblaster_color", ClampCvar(0, 3, Cvar_VariableValue("cl_hyperblaster_color")));
	s_qudos_hyperblaster_color.curvalue = Cvar_VariableValue("cl_hyperblaster_color");
	Cvar_SetValue("cl_hyperblaster_bubble", ClampCvar(0, 2, Cvar_VariableValue("cl_hyperblaster_bubble")));
	s_qudos_hyperblaster_bubble.curvalue = Cvar_VariableValue("cl_hyperblaster_bubble");

	Cvar_SetValue("cl_railtype", ClampCvar(0, 3, Cvar_VariableValue("cl_railtype")));
	s_qudos_max_railtrail.curvalue = Cvar_VariableValue("cl_railtype");
	s_qudos_max_railtrail_red.curvalue = Cvar_VariableValue("cl_railred") / 16;
	s_qudos_max_railtrail_green.curvalue = Cvar_VariableValue("cl_railgreen") / 16;
	s_qudos_max_railtrail_blue.curvalue = Cvar_VariableValue("cl_railblue") / 16;

	Cvar_SetValue("cl_gunsmoke", ClampCvar(0, 1, Cvar_VariableValue("cl_gunsmoke")));
	s_qudos_gun_smoke.curvalue = Cvar_VariableValue("cl_gunsmoke");
	Cvar_SetValue("cl_explosion", ClampCvar(0, 1, Cvar_VariableValue("cl_explosion")));
	s_qudos_explosions.curvalue = Cvar_VariableValue("cl_explosion");
	Cvar_SetValue("cl_flame_enh", ClampCvar(0, 1, Cvar_VariableValue("cl_flame_enh")));
	s_qudos_flame_enh.curvalue = Cvar_VariableValue("cl_flame_enh");
	s_qudos_flame_size.curvalue = Cvar_VariableValue("cl_flame_size");
	Cvar_SetValue("cl_teleport_particles", ClampCvar(0, 4, Cvar_VariableValue("cl_teleport_particles")));
	s_qudos_teleport.curvalue = Cvar_VariableValue("cl_teleport_particles");
	Cvar_SetValue("cl_particles_type", ClampCvar(0, 3, Cvar_VariableValue("cl_particles_type")));
	s_qudos_particles_type.curvalue = Cvar_VariableValue("cl_particles_type");
	Cvar_SetValue("cl_nukeblast_enh", ClampCvar(0, 1, Cvar_VariableValue("cl_nukeblast_enh")));
	s_qudos_nukeblast.curvalue = Cvar_VariableValue("cl_nukeblast_enh");
#else
	Cvar_SetValue("gl_particles", ClampCvar(0, 1, Cvar_VariableValue("gl_particles")));
	s_qudos_gl_particles.curvalue = Cvar_VariableValue("gl_particles");
	Cvar_SetValue("cl_railstyle", ClampCvar(0, 2, Cvar_VariableValue("cl_railstyle")));
	s_qudos_railstyle.curvalue = Cvar_VariableValue("cl_railstyle");
	Cvar_SetValue("cl_railtrail", ClampCvar(0, 1, Cvar_VariableValue("cl_railtrail")));
	s_qudos_railtrail.curvalue = Cvar_VariableValue("cl_railtrail");
	s_qudos_railtrail_color.curvalue = Cvar_VariableValue("cl_railtrail_color");
#endif
	s_qudos_gl_reflection.curvalue = Cvar_VariableValue("gl_reflection") * 10;
	Cvar_SetValue("gl_reflection_shader", ClampCvar(0, 1, Cvar_VariableValue("gl_reflection_shader")));
	s_qudos_gl_reflection_shader.curvalue = Cvar_VariableValue("gl_reflection_shader");
	Cvar_SetValue("gl_water_pixel_sharer_warp", ClampCvar(0, 1, Cvar_VariableValue("gl_water_pixel_sharer_warp")));
	s_qudos_texshader_warp.curvalue = Cvar_VariableValue("gl_water_pixel_shader_warp");
	s_qudos_waves.curvalue = Cvar_VariableValue("gl_water_waves");
	Cvar_SetValue("cl_underwater_trans", ClampCvar(0, 1, Cvar_VariableValue("cl_underwater_trans")));
	s_qudos_water_trans.curvalue = Cvar_VariableValue("cl_underwater_trans");
	Cvar_SetValue("gl_water_caustics", ClampCvar(0, 2, Cvar_VariableValue("gl_water_caustics")));
	s_qudos_caustics.curvalue = Cvar_VariableValue("gl_water_caustics");
	Cvar_SetValue("cl_underwater_movement", ClampCvar(0, 1, Cvar_VariableValue("cl_underwater_movement")));
	s_qudos_underwater_mov.curvalue = Cvar_VariableValue("cl_underwater_movement");

	Cvar_SetValue("gl_fogenable", ClampCvar(0, 1, Cvar_VariableValue("gl_fogenable")));
	s_qudos_fog.curvalue = Cvar_VariableValue("gl_fogenable");
	s_qudos_fog_density.curvalue = Cvar_VariableValue("gl_fogdensity") * 10000;
	s_qudos_fog_red.curvalue = Cvar_VariableValue("gl_fogred") * 10;
	s_qudos_fog_green.curvalue = Cvar_VariableValue("gl_foggreen") * 10;
	s_qudos_fog_blue.curvalue = Cvar_VariableValue("gl_fogblue") * 10;
	Cvar_SetValue("gl_fogunderwater", ClampCvar(0, 1, Cvar_VariableValue("gl_fogunderwater")));
	s_qudos_fog_underwater.curvalue = Cvar_VariableValue("gl_fogunderwater");

	Cvar_SetValue("gl_minimap", ClampCvar(0, 2, Cvar_VariableValue("gl_minimap")));
	s_qudos_minimap.curvalue = Cvar_VariableValue("gl_minimap");
	Cvar_SetValue("gl_minimap_style", ClampCvar(0, 1, Cvar_VariableValue("gl_minimap_style")));
	s_qudos_minimap_style.curvalue = Cvar_VariableValue("gl_minimap_style");
	s_qudos_minimap_size.curvalue = Cvar_VariableValue("gl_minimap_size");
	s_qudos_minimap_zoom.curvalue = Cvar_VariableValue("gl_minimap_zoom") * 10;
	s_qudos_minimap_x.curvalue = Cvar_VariableValue("gl_minimap_x");	
	s_qudos_minimap_y.curvalue = Cvar_VariableValue("gl_minimap_y");	

	s_qudos_det_tex.curvalue = Cvar_VariableValue("gl_detailtextures");
	Cvar_SetValue("gl_shadows", ClampCvar(0, 2, Cvar_VariableValue("gl_shadows")));
	s_qudos_shadows.curvalue = Cvar_VariableValue("gl_shadows");
	Cvar_SetValue("gl_stainmaps", ClampCvar(0, 1, Cvar_VariableValue("gl_stainmaps")));
	s_qudos_gl_stainmaps.curvalue = Cvar_VariableValue("gl_stainmaps");
	Cvar_SetValue("r_cellshading", ClampCvar(0, 1, Cvar_VariableValue("r_cellshading")));
	s_qudos_cellshading.curvalue = Cvar_VariableValue("r_cellshading");
	Cvar_SetValue("gl_shading", ClampCvar(0, 1, Cvar_VariableValue("gl_shading")));
	s_qudos_shading.curvalue = Cvar_VariableValue("gl_shading");

	Cvar_SetValue("gl_motionblur", ClampCvar(0, 2, Cvar_VariableValue("gl_motionblur")));
	s_qudos_motionblur.curvalue = Cvar_VariableValue("gl_motionblur");
	s_qudos_motionblur_intens.curvalue = Cvar_VariableValue("gl_motionblur_intensity") * 10;

	Cvar_SetValue("gl_blooms", ClampCvar(0, 1, Cvar_VariableValue("gl_blooms")));
	s_qudos_blooms.curvalue = Cvar_VariableValue("gl_blooms");
	s_qudos_blooms_alpha.curvalue = Cvar_VariableValue("gl_blooms_alpha") * 10;
	s_qudos_blooms_diamond_size.curvalue = Cvar_VariableValue("gl_blooms_diamond_size") / 2;
	s_qudos_blooms_intensity.curvalue = Cvar_VariableValue("gl_blooms_intensity") * 10;
	s_qudos_blooms_darken.curvalue = Cvar_VariableValue("gl_blooms_darken");
	s_qudos_blooms_sample_size.curvalue = log10(Cvar_VariableValue("gl_blooms_sample_size"))/log10(2);
	Cvar_SetValue("gl_blooms_fast_sample", ClampCvar(0, 1, Cvar_VariableValue("gl_blooms_fast_sample")));
	s_qudos_blooms_fast_sample.curvalue = Cvar_VariableValue("gl_blooms_fast_sample");

	Cvar_SetValue("gl_glares", ClampCvar(0, 1, Cvar_VariableValue("gl_glares")));
	s_qudos_glares.curvalue = Cvar_VariableValue("gl_glares");

	Cvar_SetValue("gl_flares", ClampCvar(0, 1, Cvar_VariableValue("gl_flares")));
	s_qudos_lensflare.curvalue = Cvar_VariableValue("gl_flares");
	Cvar_SetValue("gl_flare_style", ClampCvar(0, 6, Cvar_VariableValue("gl_flare_style")));
	s_qudos_lensflare_style.curvalue = Cvar_VariableValue("gl_flare_force_style");
	s_qudos_lensflare_intens.curvalue = Cvar_VariableValue("gl_flare_intensity") * 10;
	s_qudos_lensflare_scale.curvalue = Cvar_VariableValue("gl_flare_scale") * 10;
	s_qudos_lensflare_maxdist.curvalue = Cvar_VariableValue("gl_flare_maxdist");

	s_qudos_lightmap_scale.curvalue = (Cvar_VariableValue("gl_modulate") -1)  * 10 ;

	Cvar_SetValue("cl_hudscale", ClampCvar(0, 4, Cvar_VariableValue("cl_hudscale")));
	s_qudos_hudscale.curvalue = Cvar_VariableValue("cl_hudscale");
	Cvar_SetValue("cl_drawclock", ClampCvar(0, 2, Cvar_VariableValue("cl_drawclock")));
	s_qudos_drawclock.curvalue = Cvar_VariableValue("cl_drawclock");
	Cvar_SetValue("cl_drawrate", ClampCvar(0, 1, Cvar_VariableValue("cl_drawrate")));
	s_qudos_drawdate.curvalue = Cvar_VariableValue("cl_drawdate");
	Cvar_SetValue("cl_drawmaptime", ClampCvar(0, 1, Cvar_VariableValue("cl_drawmaptime")));
	s_qudos_drawmaptime.curvalue = Cvar_VariableValue("cl_drawmaptime");
	Cvar_SetValue("cl_drawfps", ClampCvar(0, 1, Cvar_VariableValue("cl_drawfps")));
	s_qudos_drawfps.curvalue = Cvar_VariableValue("cl_drawfps");
	Cvar_SetValue("cl_drawping", ClampCvar(0, 1, Cvar_VariableValue("cl_drawping")));
	s_qudos_drawping.curvalue = Cvar_VariableValue("cl_drawping");
	Cvar_SetValue("cl_drawrate", ClampCvar(0, 1, Cvar_VariableValue("cl_drawrate")));
	s_qudos_drawrate.curvalue = Cvar_VariableValue("cl_drawrate");
	Cvar_SetValue("netgrapth", ClampCvar(0, 1, Cvar_VariableValue("netgrapth")));
	s_qudos_drawnetgraph.curvalue = Cvar_VariableValue("netgraph");
	s_qudos_drawnetgraph_pos.curvalue = Cvar_VariableValue("netgraph_pos");
	s_qudos_drawnetgraph_alpha.curvalue = Cvar_VariableValue("netgraph_trans") * 10;
	s_qudos_drawnetgraph_scale.curvalue = Cvar_VariableValue("netgraph_size") * 1;
	s_qudos_drawnetgraph_pos_x.curvalue = Cvar_VariableValue("netgraph_pos_x");
	s_qudos_drawnetgraph_pos_y.curvalue = Cvar_VariableValue("netgraph_pos_y");
	Cvar_SetValue("cl_highlight", ClampCvar(0, 2, Cvar_VariableValue("cl_highlight")));
	s_qudos_highlight.curvalue = Cvar_VariableValue("cl_highlight");
	Cvar_SetValue("cl_drawchathud", ClampCvar(0, 2, Cvar_VariableValue("cl_drawchathud")));
	s_qudos_drawchathud.curvalue = Cvar_VariableValue("cl_drawchathud");
	s_qudos_drawplayername.curvalue = Cvar_VariableValue("cl_draw_playername");
	s_qudos_draw_x.curvalue = Cvar_VariableValue("cl_draw_x") * 20;
	s_qudos_draw_y.curvalue = Cvar_VariableValue("cl_draw_y") * 20;

	Cvar_SetValue("crosshair", ClampCvar(0, 20, Cvar_VariableValue("crosshair")));
	s_qudos_crosshair_box.curvalue = Cvar_VariableValue("crosshair");
	s_qudos_cross_scale.curvalue = Cvar_VariableValue("crosshair_scale") * 10;
	s_qudos_cross_red.curvalue = Cvar_VariableValue("crosshair_red") * 10;
	s_qudos_cross_green.curvalue = Cvar_VariableValue("crosshair_green") * 10;
	s_qudos_cross_blue.curvalue = Cvar_VariableValue("crosshair_blue") * 10;
	s_qudos_cross_pulse.curvalue = Cvar_VariableValue("crosshair_pulse") * 1;
	s_qudos_cross_alpha.curvalue = Cvar_VariableValue("crosshair_alpha") * 10;

	s_qudos_hud_red.curvalue = Cvar_VariableValue("cl_hud_red") * 10;
	s_qudos_hud_green.curvalue = Cvar_VariableValue("cl_hud_green") * 10;
	s_qudos_hud_blue.curvalue = Cvar_VariableValue("cl_hud_blue") * 10;
	s_qudos_hud_alpha.curvalue = Cvar_VariableValue("cl_hudalpha") * 10;

	s_qudos_menu_alpha.curvalue = Cvar_VariableValue("cl_menu_alpha") * 10;
}

static void
ControlsResetDefaultsFunc(void *unused)
{
	Cbuf_AddText("exec default.cfg\n");
	Cbuf_Execute();

	ControlsSetMenuItemValues();
}

static void
InvertMouseFunc(void *unused)
{
	Cvar_SetValue("m_pitch", -m_pitch->value);
}

static void
LookspringFunc(void *unused)
{
	Cvar_SetValue("lookspring", !lookspring->value);
}

static void
LookstrafeFunc(void *unused)
{
	Cvar_SetValue("lookstrafe", !lookstrafe->value);
}

#ifdef Joystick
static void
JoystickFunc(void *unused)
{
	Cvar_SetValue("in_joystick", s_options_joystick_box.curvalue);
}
#endif

static void
MenuCursorScaleFunc(void *unused)
{
	Cvar_SetValue("cl_mouse_scale", s_qudos_menucursor_scale.curvalue / 10);
}

static void
ColorMenuCursorRedFunc(void *unused)
{
	Cvar_SetValue("cl_mouse_red", s_qudos_menucursor_red.curvalue / 10);
}

static void
ColorMenuCursorGreenFunc(void *unused)
{
	Cvar_SetValue("cl_mouse_green", s_qudos_menucursor_green.curvalue / 10);
}

static void
ColorMenuCursorBlueFunc(void *unused)
{
	Cvar_SetValue("cl_mouse_blue", s_qudos_menucursor_blue.curvalue / 10);
}

static void
MenuCursorAlphaFunc(void *unused)
{
	Cvar_SetValue("cl_mouse_alpha", s_qudos_menucursor_alpha.curvalue / 10);
}

static void
MenuCursorSensitivityFunc(void *unused)
{
	Cvar_SetValue("cl_mouse_sensitivity", s_qudos_menucursor_sensitivity.curvalue / 2);
}

static void
MenuCursorTypeFunc(void *unused)
{
	Cvar_SetValue("cl_mouse_cursor", s_qudos_menucursor_type.curvalue);
}

static void
UpdateVolumeFunc(void *unused)
{
	Cvar_SetValue("s_volume", s_options_sfxvolume_slider.curvalue / 10);
}

static void
UpdateVolumeOggFunc(void *unused)
{
	Cvar_SetValue("ogg_volume", s_options_ogg_volume.curvalue / 10);
}

static void
OggSequenceFunc(void *unused)
{
	Cvar_Set("ogg_sequence", (char *)s_options_ogg_sequence.itemnames[s_options_ogg_sequence.curvalue]);
}

static void
OggAutoplayFunc(void *unused)
{
	Cvar_Set("ogg_autoplay", (char *)ogg_autoplay_values[s_options_ogg_autoplay.curvalue]);
}

static void
FootStepsFunc(void *unused)
{
	Cvar_SetValue("cl_footsteps_override", s_options_footsteps.curvalue);
}

static void
CDShuffleFunc(void *unused)
{
	Cvar_SetValue("cd_shuffle", s_options_cdshuffle_box.curvalue);
}

static void
ConsoleHeightFunc(void *unused)
{
	Cvar_SetValue("con_height", s_options_console_height.curvalue / 10);
}

static void
ConsoleTransFunc(void *unused)
{
	Cvar_SetValue("con_transparency", s_options_console_trans.curvalue / 10);
}

static void
UpdateCDVolumeFunc(void *unused)
{
	Cvar_SetValue("cd_nocd", !s_options_cdvolume_box.curvalue);

	if (s_options_cdvolume_box.curvalue) {
		CDAudio_Init();
		if (s_options_cdshuffle_box.curvalue) {
			CDAudio_RandomPlay();
		} else {
			CDAudio_Play(atoi(cl.configstrings[CS_CDTRACK]), true);
		}
	} else {
		CDAudio_Stop();
	}
}

static void
ConsoleFunc(void *unused)
{
	/*
	 * * the proper way to do this is probably to have ToggleConsole_f
	 * accept a parameter
	 */
	extern void	Key_ClearTyping(void);

	if (cl.attractloop) {
		Cbuf_AddText("killserver\n");
		return;
	}
	Key_ClearTyping();
	Con_ClearNotify();

	M_ForceMenuOff();
	cls.key_dest = key_console;
}

static void
UpdateSoundQualityFunc(void *unused)
{
	/* Knightmare- added DMP's 44/48 KHz sound support */
	/* DMP check the newly added sound quality menu options */
	switch (s_options_quality_list.curvalue) {
		case 1:
		Cvar_SetValue("s_khz", 22);
		Cvar_SetValue("s_loadas8bit", false);
		break;
	case 2:
		Cvar_SetValue("s_khz", 44);
		Cvar_SetValue("s_loadas8bit", false);
		break;
	default:
		Cvar_SetValue("s_khz", 11);
		Cvar_SetValue("s_loadas8bit", true);
		break;
	}
	/* DMP end sound menu changes */

	M_DrawTextBox(8, 120 - 48, 36, 3);
	M_Print(16 + 16, 120 - 48 + 8, "Restarting the sound system. This");
	M_Print(16 + 16, 120 - 48 + 16, "could take up to a minute, so");
	M_Print(16 + 16, 120 - 48 + 24, "please be patient.");

	/* the text box won't show up unless we do a buffer swap */
	re.EndFrame();

	CL_Snd_Restart_f();
}

void
Options_MenuInit(void)
{
	static const char *cd_music_items[] =
	{
		"Disabled",
		"Enabled ",
		NULL
	};

	static const char *quality_items[] =
	{
		"Low (11khz 8bit)    ",
		"Medium (22khz 16bit)",
		"High (44khz 16bit)  ",
		NULL
	};

	static const char *yesno_names[] =
	{
		"No ",
		"Yes",
		NULL
	};

	static const char *ogg_sequence_names[] =
	{
		"none",
		"next",
		"prev",
		"random",
		"loop",
		NULL
	};

	static const char *ogg_autoplay_names[] =
	{
		"none",
		"first",
		"last",
		"random",
		NULL,
		NULL
	};

	static const char *menucursor_names[] =
	{
		"Default",
		"Custom ",
		NULL
	};

	/*
	 * * configure controls menu and menu items
	 */
	s_options_menu.x = viddef.width / 2;
	s_options_menu.y = viddef.height / 2 - 200;	/* was 58 */
	s_options_menu.nitems = 0;

	s_options_sfxvolume_slider.generic.type = MTYPE_SLIDER;
	s_options_sfxvolume_slider.generic.x = 0;
	s_options_sfxvolume_slider.generic.y = 0;
	s_options_sfxvolume_slider.generic.name = "Effects volume";
	s_options_sfxvolume_slider.generic.callback = UpdateVolumeFunc;
	s_options_sfxvolume_slider.minvalue = 0;
	s_options_sfxvolume_slider.maxvalue = 10;
	s_options_sfxvolume_slider.curvalue = Cvar_VariableValue("s_volume") * 10;

	s_options_ogg_volume.generic.type = MTYPE_SLIDER;
	s_options_ogg_volume.generic.x = 0;
	s_options_ogg_volume.generic.y = 10;
	s_options_ogg_volume.generic.name = "Ogg volume";
	s_options_ogg_volume.generic.callback = UpdateVolumeOggFunc;
	s_options_ogg_volume.minvalue = 0;
	s_options_ogg_volume.maxvalue = 20;
	s_options_ogg_volume.curvalue = Cvar_VariableValue("ogg_volume") * 10;

	s_options_ogg_sequence.generic.type = MTYPE_SPINCONTROL;
	s_options_ogg_sequence.generic.x = 0;
	s_options_ogg_sequence.generic.y = 20;
	s_options_ogg_sequence.generic.name = "Ogg sequence";
	s_options_ogg_sequence.generic.callback = OggSequenceFunc;
	s_options_ogg_sequence.itemnames = ogg_sequence_names;
	s_options_ogg_sequence.curvalue =
	GetPosInList(s_options_ogg_sequence.itemnames, Cvar_VariableString("ogg_sequence"));

	s_options_ogg_autoplay.generic.type = MTYPE_SPINCONTROL;
	s_options_ogg_autoplay.generic.x = 0;
	s_options_ogg_autoplay.generic.y = 30;
	s_options_ogg_autoplay.generic.name = "Ogg Autoplay";
	s_options_ogg_autoplay.generic.callback = OggAutoplayFunc;
	s_options_ogg_autoplay.itemnames = ogg_autoplay_names;

	ogg_autoplay_names[4] = ogg_autoplay_values[4] = NULL;

	s_options_ogg_autoplay.curvalue = GetPosInList(ogg_autoplay_values,
	    Cvar_VariableString("ogg_autoplay"));

	if (s_options_ogg_autoplay.curvalue == -1) {
		s_options_ogg_autoplay.curvalue = 4;
		ogg_autoplay_names[4] = "custom";
		ogg_autoplay_values[4] = Cvar_VariableString("ogg_autoplay");
	}

	s_options_cdvolume_box.generic.type = MTYPE_SPINCONTROL;
	s_options_cdvolume_box.generic.x = 0;
	s_options_cdvolume_box.generic.y = 50;
	s_options_cdvolume_box.generic.name = "CD music";
	s_options_cdvolume_box.generic.callback = UpdateCDVolumeFunc;
	s_options_cdvolume_box.itemnames = cd_music_items;
	s_options_cdvolume_box.curvalue = !Cvar_VariableValue("cd_nocd");

	s_options_cdshuffle_box.generic.type = MTYPE_SPINCONTROL;
	s_options_cdshuffle_box.generic.x = 0;
	s_options_cdshuffle_box.generic.y = 70;
	s_options_cdshuffle_box.generic.name = "CD shuffle";
	s_options_cdshuffle_box.generic.callback = CDShuffleFunc;
	s_options_cdshuffle_box.itemnames = cd_music_items;
	s_options_cdshuffle_box.curvalue = Cvar_VariableValue("cd_shuffle");;

	s_options_quality_list.generic.type = MTYPE_SPINCONTROL;
	s_options_quality_list.generic.x = 0;
	s_options_quality_list.generic.y = 80;;
	s_options_quality_list.generic.name = "Sound Quality";
	s_options_quality_list.generic.callback = UpdateSoundQualityFunc;
	s_options_quality_list.itemnames = quality_items;
	s_options_quality_list.curvalue = !Cvar_VariableValue("s_loadas8bit");

	s_options_footsteps.generic.type = MTYPE_SPINCONTROL;
	s_options_footsteps.generic.x = 0;
	s_options_footsteps.generic.y = 90;
	s_options_footsteps.generic.name = "Footstep Sounds";
	s_options_footsteps.generic.callback = FootStepsFunc;
	s_options_footsteps.itemnames = yesno_names;
	s_options_footsteps.curvalue = Cvar_VariableValue("cl_footsteps_override");
	s_options_footsteps.generic.statusbar = "Override footstep sounds, definitions localized in scripts/texsurfs.txt";

	s_options_sensitivity_slider.generic.type = MTYPE_SLIDER;
	s_options_sensitivity_slider.generic.x = 0;
	s_options_sensitivity_slider.generic.y = 110;
	s_options_sensitivity_slider.generic.name = "Mouse speed";
	s_options_sensitivity_slider.generic.callback = MouseSpeedFunc;
	s_options_sensitivity_slider.minvalue = 2;
	s_options_sensitivity_slider.maxvalue = 22;

	s_options_alwaysrun_box.generic.type = MTYPE_SPINCONTROL;
	s_options_alwaysrun_box.generic.x = 0;
	s_options_alwaysrun_box.generic.y = 120;
	s_options_alwaysrun_box.generic.name = "Always run";
	s_options_alwaysrun_box.generic.callback = AlwaysRunFunc;
	s_options_alwaysrun_box.itemnames = yesno_names;
	s_options_alwaysrun_box.curvalue = Cvar_VariableValue("cl_run");

	s_options_invertmouse_box.generic.type = MTYPE_SPINCONTROL;
	s_options_invertmouse_box.generic.x = 0;
	s_options_invertmouse_box.generic.y = 130;
	s_options_invertmouse_box.generic.name = "Invert mouse";
	s_options_invertmouse_box.generic.callback = InvertMouseFunc;
	s_options_invertmouse_box.itemnames = yesno_names;

	s_options_lookspring_box.generic.type = MTYPE_SPINCONTROL;
	s_options_lookspring_box.generic.x = 0;
	s_options_lookspring_box.generic.y = 140;
	s_options_lookspring_box.generic.name = "Lookspring";
	s_options_lookspring_box.generic.callback = LookspringFunc;
	s_options_lookspring_box.itemnames = yesno_names;

	s_options_lookstrafe_box.generic.type = MTYPE_SPINCONTROL;
	s_options_lookstrafe_box.generic.x = 0;
	s_options_lookstrafe_box.generic.y = 150;
	s_options_lookstrafe_box.generic.name = "Lookstrafe";
	s_options_lookstrafe_box.generic.callback = LookstrafeFunc;
	s_options_lookstrafe_box.itemnames = yesno_names;

	s_options_freelook_box.generic.type = MTYPE_SPINCONTROL;
	s_options_freelook_box.generic.x = 0;
	s_options_freelook_box.generic.y = 160;
	s_options_freelook_box.generic.name = "Free look";
	s_options_freelook_box.generic.callback = FreeLookFunc;
	s_options_freelook_box.itemnames = yesno_names;

	s_qudos_menucursor_scale.generic.type = MTYPE_SLIDER;
	s_qudos_menucursor_scale.generic.x = 0;
	s_qudos_menucursor_scale.generic.y = 180;
	s_qudos_menucursor_scale.generic.name = "Menu Mouse Size";
	s_qudos_menucursor_scale.generic.callback = MenuCursorScaleFunc;
	s_qudos_menucursor_scale.minvalue = 3;
	s_qudos_menucursor_scale.maxvalue = 10;
	s_qudos_menucursor_scale.curvalue = Cvar_VariableValue("cl_mouse_scale") * 10;

	s_qudos_menucursor_red.generic.type = MTYPE_SLIDER;
	s_qudos_menucursor_red.generic.x = 0;
	s_qudos_menucursor_red.generic.y = 190;
	s_qudos_menucursor_red.generic.name = "Red";
	s_qudos_menucursor_red.generic.callback = ColorMenuCursorRedFunc;
	s_qudos_menucursor_red.minvalue = 0;
	s_qudos_menucursor_red.maxvalue = 10;
	s_qudos_menucursor_red.curvalue = Cvar_VariableValue("cl_mouse_red") * 10;

	s_qudos_menucursor_green.generic.type = MTYPE_SLIDER;
	s_qudos_menucursor_green.generic.x = 0;
	s_qudos_menucursor_green.generic.y = 200;
	s_qudos_menucursor_green.generic.name = "Green";
	s_qudos_menucursor_green.generic.callback = ColorMenuCursorGreenFunc;
	s_qudos_menucursor_green.minvalue = 0;
	s_qudos_menucursor_green.maxvalue = 10;
	s_qudos_menucursor_green.curvalue = Cvar_VariableValue("cl_mouse_green") * 10;

	s_qudos_menucursor_blue.generic.type = MTYPE_SLIDER;
	s_qudos_menucursor_blue.generic.x = 0;
	s_qudos_menucursor_blue.generic.y = 210;
	s_qudos_menucursor_blue.generic.name = "Blue";
	s_qudos_menucursor_blue.generic.callback = ColorMenuCursorBlueFunc;
	s_qudos_menucursor_blue.minvalue = 0;
	s_qudos_menucursor_blue.maxvalue = 10;
	s_qudos_menucursor_blue.curvalue = Cvar_VariableValue("cl_mouse_blue") * 10;

	s_qudos_menucursor_alpha.generic.type = MTYPE_SLIDER;
	s_qudos_menucursor_alpha.generic.x = 0;
	s_qudos_menucursor_alpha.generic.y = 220;
	s_qudos_menucursor_alpha.generic.name = "Transparency";
	s_qudos_menucursor_alpha.generic.callback = MenuCursorAlphaFunc;
	s_qudos_menucursor_alpha.minvalue = 2;
	s_qudos_menucursor_alpha.maxvalue = 10;
	s_qudos_menucursor_alpha.curvalue = Cvar_VariableValue("cl_mouse_alpha") * 10;

	s_qudos_menucursor_sensitivity.generic.type = MTYPE_SLIDER;
	s_qudos_menucursor_sensitivity.generic.x = 0;
	s_qudos_menucursor_sensitivity.generic.y = 230;
	s_qudos_menucursor_sensitivity.generic.name = "Sensitivity";
	s_qudos_menucursor_sensitivity.generic.callback = MenuCursorSensitivityFunc;
	s_qudos_menucursor_sensitivity.minvalue = 0.5;
	s_qudos_menucursor_sensitivity.maxvalue = 4;
	s_qudos_menucursor_sensitivity.curvalue = Cvar_VariableValue("cl_mouse_sensitivity") * 2;

	s_qudos_menucursor_type.generic.type = MTYPE_SPINCONTROL;
	s_qudos_menucursor_type.generic.x = 0;
	s_qudos_menucursor_type.generic.y = 240;
	s_qudos_menucursor_type.generic.name = "Cursor Type";
	s_qudos_menucursor_type.generic.callback = MenuCursorTypeFunc;
	s_qudos_menucursor_type.itemnames = menucursor_names;
	s_qudos_menucursor_type.curvalue = Cvar_VariableValue("cl_mouse_cursor");
	s_qudos_menucursor_type.generic.statusbar = 
	"Requires QuDos_extra.pk3 or your own cursor pics";

	s_options_customize_action.generic.type = MTYPE_ACTION;
	s_options_customize_action.generic.x = 0;
	s_options_customize_action.generic.y = 260;
	s_options_customize_action.generic.name = "Customize Controls";
	s_options_customize_action.generic.callback = CustomizeControlsFunc;

	s_options_console_height.generic.type = MTYPE_SLIDER;
	s_options_console_height.generic.x = 0;
	s_options_console_height.generic.y = 280;
	s_options_console_height.generic.name = "Console Height";
	s_options_console_height.generic.callback = ConsoleHeightFunc;
	s_options_console_height.minvalue = 0;
	s_options_console_height.maxvalue = 10;
	s_options_console_height.curvalue = Cvar_VariableValue("con_height") * 10;

	s_options_console_trans.generic.type = MTYPE_SLIDER;
	s_options_console_trans.generic.x = 0;
	s_options_console_trans.generic.y = 290;
	s_options_console_trans.generic.name = "Console Transparency";
	s_options_console_trans.generic.callback = ConsoleTransFunc;
	s_options_console_trans.minvalue = 0;
	s_options_console_trans.maxvalue = 10;
	s_options_console_trans.curvalue = Cvar_VariableValue("con_transparency") * 10;

	s_options_console_action.generic.type = MTYPE_ACTION;
	s_options_console_action.generic.x = 0;
	s_options_console_action.generic.y = 300;
	s_options_console_action.generic.name = "Go to console";
	s_options_console_action.generic.callback = ConsoleFunc;

	s_options_qudos_action.generic.type = MTYPE_SEPARATOR;
	s_options_qudos_action.generic.x = 70;
	s_options_qudos_action.generic.y = 320;
	s_options_qudos_action.generic.name = "QuDos - New Options";

	s_options_qudos_action_client.generic.type = MTYPE_ACTION;
	s_options_qudos_action_client.generic.x = 0;
	s_options_qudos_action_client.generic.y = 330;
	s_options_qudos_action_client.generic.name = "Client Side";
	s_options_qudos_action_client.generic.callback = QuDosClientFunc;

	s_options_qudos_action_graphic.generic.type = MTYPE_ACTION;
	s_options_qudos_action_graphic.generic.x = 0;
	s_options_qudos_action_graphic.generic.y = 340;
	s_options_qudos_action_graphic.generic.name = "Graphics";
	s_options_qudos_action_graphic.generic.callback = QuDosGraphicFunc;

	s_options_qudos_action_hud.generic.type = MTYPE_ACTION;
	s_options_qudos_action_hud.generic.x = 0;
	s_options_qudos_action_hud.generic.y = 350;
	s_options_qudos_action_hud.generic.name = "Hud";
	s_options_qudos_action_hud.generic.callback = QuDosHudFunc;

	s_options_defaults_action.generic.type = MTYPE_ACTION;
	s_options_defaults_action.generic.x = 0;
	s_options_defaults_action.generic.y = 370;
	s_options_defaults_action.generic.name = "Reset to defaults";
	s_options_defaults_action.generic.callback = ControlsResetDefaultsFunc;

#ifdef Joystick
	s_options_joystick_box.generic.type = MTYPE_SPINCONTROL;
	s_options_joystick_box.generic.x = 0;
	s_options_joystick_box.generic.y = 390;
	s_options_joystick_box.generic.name = "Use joystick";
	s_options_joystick_box.generic.callback = JoystickFunc;
	s_options_joystick_box.itemnames = yesno_names;
#endif

	ControlsSetMenuItemValues();

	Menu_AddItem(&s_options_menu, (void *)&s_options_sfxvolume_slider);
	Menu_AddItem(&s_options_menu, (void *)&s_options_ogg_volume);
	Menu_AddItem(&s_options_menu, (void *)&s_options_ogg_sequence);
	Menu_AddItem(&s_options_menu, (void *)&s_options_ogg_autoplay);
	Menu_AddItem(&s_options_menu, (void *)&s_options_cdvolume_box);
	Menu_AddItem(&s_options_menu, (void *)&s_options_cdshuffle_box);
	Menu_AddItem(&s_options_menu, (void *)&s_options_quality_list);
	Menu_AddItem(&s_options_menu, (void *)&s_options_footsteps);

	Menu_AddItem(&s_options_menu, (void *)&s_options_sensitivity_slider);
	Menu_AddItem(&s_options_menu, (void *)&s_options_alwaysrun_box);
	Menu_AddItem(&s_options_menu, (void *)&s_options_invertmouse_box);
	Menu_AddItem(&s_options_menu, (void *)&s_options_lookspring_box);
	Menu_AddItem(&s_options_menu, (void *)&s_options_lookstrafe_box);
	Menu_AddItem(&s_options_menu, (void *)&s_options_freelook_box);
	Menu_AddItem(&s_options_menu, (void *)&s_qudos_menucursor_scale);
	Menu_AddItem(&s_options_menu, (void *)&s_qudos_menucursor_red);
	Menu_AddItem(&s_options_menu, (void *)&s_qudos_menucursor_green);
	Menu_AddItem(&s_options_menu, (void *)&s_qudos_menucursor_blue);
	Menu_AddItem(&s_options_menu, (void *)&s_qudos_menucursor_alpha);
	Menu_AddItem(&s_options_menu, (void *)&s_qudos_menucursor_sensitivity);
	Menu_AddItem(&s_options_menu, (void *)&s_qudos_menucursor_type);
	Menu_AddItem(&s_options_menu, (void *)&s_options_customize_action);
	Menu_AddItem(&s_options_menu, (void *)&s_options_console_height);
	Menu_AddItem(&s_options_menu, (void *)&s_options_console_trans);
	Menu_AddItem(&s_options_menu, (void *)&s_options_console_action);
	Menu_AddItem(&s_options_menu, (void *)&s_options_qudos_action);

	Menu_AddItem(&s_options_menu, (void *)&s_options_qudos_action_client);
	Menu_AddItem(&s_options_menu, (void *)&s_options_qudos_action_graphic);
	Menu_AddItem(&s_options_menu, (void *)&s_options_qudos_action_hud);
	Menu_AddItem(&s_options_menu, (void *)&s_options_defaults_action);
#ifdef Joystick
	Menu_AddItem(&s_options_menu, (void *)&s_options_joystick_box);
#endif
}

void
Options_MenuDraw(void)
{
	M_Banner("m_banner_options");
	Menu_AdjustCursor(&s_options_menu, 1);
	Menu_Draw(&s_options_menu);
}

const char *
Options_MenuKey(int key)
{
	return Default_MenuKey(&s_options_menu, key);
}

void
M_Menu_Options_f(void)
{
	Options_MenuInit();
	M_PushMenu(Options_MenuDraw, Options_MenuKey);
}

/*
 * =======================================================================
 *
 * VIDEO MENU
 *
 * =======================================================================
 */

void
M_Menu_Video_f(void)
{
	VID_MenuInit();
	M_PushMenu(VID_MenuDraw, VID_MenuKey);
}

/*
 *
 * =======================================================================
 *
 * END GAME MENU
 *
 * =======================================================================
 */

static int		credits_start_time;
static const char     **credits;
static char            *creditsIndex[256];
static char            *creditsBuffer;

static const char      *idcredits[] = {
	"+QUAKE II BY ID SOFTWARE",
	"",
	"+PROGRAMMING",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"",
	"+ART",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"",
	"+LEVEL DESIGN",
	"Tim Willits",
	"American McGee",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"",
	"+BIZ",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Donna Jackson",
	"",
	"",
	"+SPECIAL THANKS",
	"Ben Donges for beta testing",
	"",
	"",
	"",
	"",
	"",
	"",
	"+ADDITIONAL SUPPORT",
	"",
	"+LINUX PORT AND CTF",
	"Dave \"Zoid\" Kirsch",
	"",
	"+CINEMATIC SEQUENCES",
	"Ending Cinematic by Blur Studio - ",
	"Venice, CA",
	"",
	"Environment models for Introduction",
	"Cinematic by Karl Dolgener",
	"",
	"Assistance with environment design",
	"by Cliff Iwai",
	"",
	"+SOUND EFFECTS AND MUSIC",
	"Sound Design by Soundelux Media Labs.",
	"Music Composed and Produced by",
	"Soundelux Media Labs.  Special thanks",
	"to Bill Brown, Tom Ozanich, Brian",
	"Celano, Jeff Eisner, and The Soundelux",
	"Players.",
	"",
	"\"Level Music\" by Sonic Mayhem",
	"www.sonicmayhem.com",
	"",
	"\"Quake II Theme Song\"",
	"(C) 1997 Rob Zombie. All Rights",
	"Reserved.",
	"",
	"Track 10 (\"Climb\") by Jer Sypult",
	"",
	"Voice of computers by",
	"Carly Staehlin-Taylor",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"John Tam",
	"Steve Rosenthal",
	"Marty Stratton",
	"Henk Hartong",
	"",
	"Quake II(tm) (C)1997 Id Software, Inc.",
	"All Rights Reserved.  Distributed by",
	"Activision, Inc. under license.",
	"Quake II(tm), the Id Software name,",
	"the \"Q II\"(tm) logo and id(tm)",
	"logo are trademarks of Id Software,",
	"Inc. Activision(R) is a registered",
	"trademark of Activision, Inc. All",
	"other trademarks and trade names are",
	"properties of their respective owners.",
	0
};

static const char *xatcredits[] = {
	"+QUAKE II MISSION PACK: THE RECKONING",
	"+BY",
	"+XATRIX ENTERTAINMENT, INC.",
	"",
	"+DESIGN AND DIRECTION",
	"Drew Markham",
	"",
	"+PRODUCED BY",
	"Greg Goodrich",
	"",
	"+PROGRAMMING",
	"Rafael Paiz",
	"",
	"+LEVEL DESIGN / ADDITIONAL GAME DESIGN",
	"Alex Mayberry",
	"",
	"+LEVEL DESIGN",
	"Mal Blackwell",
	"Dan Koppel",
	"",
	"+ART DIRECTION",
	"Michael \"Maxx\" Kaufman",
	"",
	"+COMPUTER GRAPHICS SUPERVISOR AND",
	"+CHARACTER ANIMATION DIRECTION",
	"Barry Dempsey",
	"",
	"+SENIOR ANIMATOR AND MODELER",
	"Jason Hoover",
	"",
	"+CHARACTER ANIMATION AND",
	"+MOTION CAPTURE SPECIALIST",
	"Amit Doron",
	"",
	"+ART",
	"Claire Praderie-Markham",
	"Viktor Antonov",
	"Corky Lehmkuhl",
	"",
	"+INTRODUCTION ANIMATION",
	"Dominique Drozdz",
	"",
	"+ADDITIONAL LEVEL DESIGN",
	"Aaron Barber",
	"Rhett Baldwin",
	"",
	"+3D CHARACTER ANIMATION TOOLS",
	"Gerry Tyra, SA Technology",
	"",
	"+ADDITIONAL EDITOR TOOL PROGRAMMING",
	"Robert Duffy",
	"",
	"+ADDITIONAL PROGRAMMING",
	"Ryan Feltrin",
	"",
	"+PRODUCTION COORDINATOR",
	"Victoria Sylvester",
	"",
	"+SOUND DESIGN",
	"Gary Bradfield",
	"",
	"+MUSIC BY",
	"Sonic Mayhem",
	"",
	"",
	"",
	"+SPECIAL THANKS",
	"+TO",
	"+OUR FRIENDS AT ID SOFTWARE",
	"",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"Tim Willits",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Dave \"Zoid\" Kirsch",
	"Donna Jackson",
	"",
	"",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"Marty Stratton",
	"Henk \"The Original Ripper\" Hartong",
	"Kevin Kraff",
	"Jamey Gottlieb",
	"Chris Hepburn",
	"",
	"+AND THE GAME TESTERS",
	"",
	"Tim Vanlaw",
	"Doug Jacobs",
	"Steven Rosenthal",
	"David Baker",
	"Chris Campbell",
	"Aaron Casillas",
	"Steve Elwell",
	"Derek Johnstone",
	"Igor Krinitskiy",
	"Samantha Lee",
	"Michael Spann",
	"Chris Toft",
	"Juan Valdes",
	"",
	"+THANKS TO INTERGRAPH COMPUTER SYTEMS",
	"+IN PARTICULAR:",
	"",
	"Michael T. Nicolaou",
	"",
	"",
	"Quake II Mission Pack: The Reckoning",
	"(tm) (C)1998 Id Software, Inc. All",
	"Rights Reserved. Developed by Xatrix",
	"Entertainment, Inc. for Id Software,",
	"Inc. Distributed by Activision Inc.",
	"under license. Quake(R) is a",
	"registered trademark of Id Software,",
	"Inc. Quake II Mission Pack: The",
	"Reckoning(tm), Quake II(tm), the Id",
	"Software name, the \"Q II\"(tm) logo",
	"and id(tm) logo are trademarks of Id",
	"Software, Inc. Activision(R) is a",
	"registered trademark of Activision,",
	"Inc. Xatrix(R) is a registered",
	"trademark of Xatrix Entertainment,",
	"Inc. All other trademarks and trade",
	"names are properties of their",
	"respective owners.",
	0
};

static const char *roguecredits[] = {
	"+QUAKE II MISSION PACK 2: GROUND ZERO",
	"+BY",
	"+ROGUE ENTERTAINMENT, INC.",
	"",
	"+PRODUCED BY",
	"Jim Molinets",
	"",
	"+PROGRAMMING",
	"Peter Mack",
	"Patrick Magruder",
	"",
	"+LEVEL DESIGN",
	"Jim Molinets",
	"Cameron Lamprecht",
	"Berenger Fish",
	"Robert Selitto",
	"Steve Tietze",
	"Steve Thoms",
	"",
	"+ART DIRECTION",
	"Rich Fleider",
	"",
	"+ART",
	"Rich Fleider",
	"Steve Maines",
	"Won Choi",
	"",
	"+ANIMATION SEQUENCES",
	"Creat Studios",
	"Steve Maines",
	"",
	"+ADDITIONAL LEVEL DESIGN",
	"Rich Fleider",
	"Steve Maines",
	"Peter Mack",
	"",
	"+SOUND",
	"James Grunke",
	"",
	"+GROUND ZERO THEME",
	"+AND",
	"+MUSIC BY",
	"Sonic Mayhem",
	"",
	"+VWEP MODELS",
	"Brent \"Hentai\" Dill",
	"",
	"",
	"",
	"+SPECIAL THANKS",
	"+TO",
	"+OUR FRIENDS AT ID SOFTWARE",
	"",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"Tim Willits",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Katherine Anna Kang",
	"Donna Jackson",
	"Dave \"Zoid\" Kirsch",
	"",
	"",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"Marty Stratton",
	"Henk Hartong",
	"Mitch Lasky",
	"Steve Rosenthal",
	"Steve Elwell",
	"",
	"+AND THE GAME TESTERS",
	"",
	"The Ranger Clan",
	"Dave \"Zoid\" Kirsch",
	"Nihilistic Software",
	"Robert Duffy",
	"",
	"And Countless Others",
	"",
	"",
	"",
	"Quake II Mission Pack 2: Ground Zero",
	"(tm) (C)1998 Id Software, Inc. All",
	"Rights Reserved. Developed by Rogue",
	"Entertainment, Inc. for Id Software,",
	"Inc. Distributed by Activision Inc.",
	"under license. Quake(R) is a",
	"registered trademark of Id Software,",
	"Inc. Quake II Mission Pack 2: Ground",
	"Zero(tm), Quake II(tm), the Id",
	"Software name, the \"Q II\"(tm) logo",
	"and id(tm) logo are trademarks of Id",
	"Software, Inc. Activision(R) is a",
	"registered trademark of Activision,",
	"Inc. Rogue(R) is a registered",
	"trademark of Rogue Entertainment,",
	"Inc. All other trademarks and trade",
	"names are properties of their",
	"respective owners.",
	0
};

void
M_Credits_MenuDraw(void)
{
	int		i, y;

	/* Draw the credits. */
	for (i = 0, y = viddef.height - ((cls.realtime - credits_start_time) / 40.0); credits[i] && y < viddef.height; y += 10, i++) {
		int		j         , stringoffset = 0;
		int		bold = false;

		if (y <= -8)
			continue;

		if (credits[i][0] == '+') {
			bold = true;
			stringoffset = 1;
		} else {
			bold = false;
			stringoffset = 0;
		}

		for (j = 0; credits[i][j + stringoffset]; j++) {
			int		x;

			x = (viddef.width - strlen(credits[i]) * 8 - stringoffset * 8) / 2 + (j + stringoffset) * 8;

			if (bold)
				re.DrawChar(x, y, credits[i][j + stringoffset] + 128, 256);
			else
				re.DrawChar(x, y, credits[i][j + stringoffset], 256);
		}
	}

	if (y < 0)
		credits_start_time = cls.realtime;
}

const char     *
M_Credits_Key(int key)
{
	switch (key) {
		case K_ESCAPE:
		if (creditsBuffer)
			FS_FreeFile(creditsBuffer);
		M_PopMenu();
		break;
	}

	return menu_out_sound;

}

extern int	Developer_searchpath(int who);

void
M_Menu_Credits_f(void)
{
	int		n;
	int		count;
	char           *p;
	int		isdeveloper = 0;

	creditsBuffer = NULL;
	count = FS_LoadFile("credits", (void **)&creditsBuffer);
	if (count != -1) {
		p = creditsBuffer;
		for (n = 0; n < 255; n++) {
			creditsIndex[n] = p;
			while (*p != '\r' && *p != '\n') {
				p++;
				if (--count == 0)
					break;
			}
			if (*p == '\r') {
				*p++ = 0;
				if (--count == 0)
					break;
			}
			*p++ = 0;
			if (--count == 0)
				break;
		}
		creditsIndex[++n] = 0;
		credits = (const char **)creditsIndex;
	} else {
		isdeveloper = Developer_searchpath(1);

		if (isdeveloper == 1)	/* xatrix */
			credits = xatcredits;
		else if (isdeveloper == 2)	/* ROGUE */
			credits = roguecredits;
		else {
			credits = idcredits;
		}

	}

	credits_start_time = cls.realtime;
	M_PushMenu(M_Credits_MenuDraw, M_Credits_Key);
}

/*
 *
 * ============================================================================
 * =
 *
 * GAME MENU
 *
 * =============================================================================
 */

static int	m_game_cursor;

static menuframework_s s_game_menu;
static menuaction_s s_easy_game_action;
static menuaction_s s_medium_game_action;
static menuaction_s s_hard_game_action;
static menuaction_s s_nightmare_game_action;
static menuaction_s s_load_game_action;
static menuaction_s s_save_game_action;
static menuaction_s s_credits_action;
static menuseparator_s s_blankline;

static void
StartGame(void)
{
	/* disable updates and start the cinematic going */
	cl.servercount = -1;
	M_ForceMenuOff();
	Cvar_SetValue("deathmatch", 0);
	Cvar_SetValue("coop", 0);

	Cvar_SetValue("gamerules", 0);	/* PGM */

	Cbuf_AddText("loading ; killserver ; wait ; newgame\n");
	cls.key_dest = key_game;
}

static void
EasyGameFunc(void *data)
{
	Cvar_ForceSet("skill", "0");
	StartGame();
}

static void
MediumGameFunc(void *data)
{
	Cvar_ForceSet("skill", "1");
	StartGame();
}

static void
HardGameFunc(void *data)
{
	Cvar_ForceSet("skill", "2");
	StartGame();
}

static void
NightmareGameFunc(void *data)
{
	Cvar_ForceSet("skill", "3");
	StartGame();
}

static void
LoadGameFunc(void *unused)
{
	M_Menu_LoadGame_f();
}

static void
SaveGameFunc(void *unused)
{
	M_Menu_SaveGame_f();
}

static void
CreditsFunc(void *unused)
{
	M_Menu_Credits_f();
}

void
Game_MenuInit(void)
{
	s_game_menu.x = viddef.width * 0.50;
	s_game_menu.nitems = 0;

	s_easy_game_action.generic.type = MTYPE_ACTION;
	s_easy_game_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_easy_game_action.generic.x = 0;
	s_easy_game_action.generic.y = -200;
	s_easy_game_action.generic.name = "Easy";
	s_easy_game_action.generic.callback = EasyGameFunc;

	s_medium_game_action.generic.type = MTYPE_ACTION;
	s_medium_game_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_medium_game_action.generic.x = 0;
	s_medium_game_action.generic.y = -190;
	s_medium_game_action.generic.name = "Medium";
	s_medium_game_action.generic.callback = MediumGameFunc;

	s_hard_game_action.generic.type = MTYPE_ACTION;
	s_hard_game_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_hard_game_action.generic.x = 0;
	s_hard_game_action.generic.y = -180;
	s_hard_game_action.generic.name = "Hard";
	s_hard_game_action.generic.callback = HardGameFunc;

	s_nightmare_game_action.generic.type = MTYPE_ACTION;
	s_nightmare_game_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_nightmare_game_action.generic.x = 0;
	s_nightmare_game_action.generic.y = -170;
	s_nightmare_game_action.generic.name = "Nightmare";
	s_nightmare_game_action.generic.callback = NightmareGameFunc;

	s_blankline.generic.type = MTYPE_SEPARATOR;

	s_load_game_action.generic.type = MTYPE_ACTION;
	s_load_game_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_load_game_action.generic.x = 0;
	s_load_game_action.generic.y = -150;
	s_load_game_action.generic.name = "Load game";
	s_load_game_action.generic.callback = LoadGameFunc;

	s_save_game_action.generic.type = MTYPE_ACTION;
	s_save_game_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_save_game_action.generic.x = 0;
	s_save_game_action.generic.y = -140;
	s_save_game_action.generic.name = "Save game";
	s_save_game_action.generic.callback = SaveGameFunc;

	s_credits_action.generic.type = MTYPE_ACTION;
	s_credits_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_credits_action.generic.x = 0;
	s_credits_action.generic.y = -120;
	s_credits_action.generic.name = "Credits";
	s_credits_action.generic.callback = CreditsFunc;

	Menu_AddItem(&s_game_menu, (void *)&s_easy_game_action);
	Menu_AddItem(&s_game_menu, (void *)&s_medium_game_action);
	Menu_AddItem(&s_game_menu, (void *)&s_hard_game_action);
	Menu_AddItem(&s_game_menu, (void *)&s_nightmare_game_action);
	Menu_AddItem(&s_game_menu, (void *)&s_blankline);
	Menu_AddItem(&s_game_menu, (void *)&s_load_game_action);
	Menu_AddItem(&s_game_menu, (void *)&s_save_game_action);
	Menu_AddItem(&s_game_menu, (void *)&s_blankline);
	Menu_AddItem(&s_game_menu, (void *)&s_credits_action);

	Menu_Center(&s_game_menu);
}

void
Game_MenuDraw(void)
{
	M_Banner("m_banner_game");
	Menu_AdjustCursor(&s_game_menu, 1);
	Menu_Draw(&s_game_menu);
}

const char     *
Game_MenuKey(int key)
{
	return Default_MenuKey(&s_game_menu, key);
}

void
M_Menu_Game_f(void)
{
	Game_MenuInit();
	M_PushMenu(Game_MenuDraw, Game_MenuKey);
	m_game_cursor = 1;
}

/*
 *
 * ============================================================================
 * =
 *
 * LOADGAME MENU
 *
 * =============================================================================
 */

#define	MAX_SAVEGAMES	15

static menuframework_s s_savegame_menu;

static menuframework_s s_loadgame_menu;
static menuaction_s s_loadgame_actions[MAX_SAVEGAMES];

char		m_savestrings[MAX_SAVEGAMES][32];
qboolean	m_savevalid[MAX_SAVEGAMES];

void
Create_Savestrings(void)
{
	char		name      [MAX_OSPATH];
	int		i;
	fileHandle_t	f;

	for (i = 0; i < MAX_SAVEGAMES; i++) {
		Com_sprintf(name, sizeof(name), "save/save%i/server.ssv", i);
		FS_FOpenFile(name, &f, FS_READ);
		if (!f) {
			Q_strncpyz(m_savestrings[i], "<EMPTY>", sizeof(m_savestrings[0]));
			m_savevalid[i] = false;
		} else {
			FS_Read(m_savestrings[i], sizeof(m_savestrings[i]), f);
			FS_FCloseFile(f);
			m_savevalid[i] = true;
		}
	}
}

void
LoadGameCallback(void *self)
{
	menuaction_s   *a = (menuaction_s *) self;

	if (m_savevalid[a->generic.localdata[0]])
		Cbuf_AddText(va("load save%i\n", a->generic.localdata[0]));
	M_ForceMenuOff();
}

void
LoadGame_MenuInit(void)
{
	int		i;

	s_loadgame_menu.x = viddef.width / 2 - 120;
	s_loadgame_menu.y = viddef.height / 2 - 58;
	s_loadgame_menu.nitems = 0;

	Create_Savestrings();

	for (i = 0; i < MAX_SAVEGAMES; i++) {
		s_loadgame_actions[i].generic.name = m_savestrings[i];
		s_loadgame_actions[i].generic.flags = QMF_LEFT_JUSTIFY;
		s_loadgame_actions[i].generic.localdata[0] = i;
		s_loadgame_actions[i].generic.callback = LoadGameCallback;

		s_loadgame_actions[i].generic.x = 0;
		s_loadgame_actions[i].generic.y = (i) * 10;
		if (i > 0)	/* separate from autosave */
			s_loadgame_actions[i].generic.y += 10;

		s_loadgame_actions[i].generic.type = MTYPE_ACTION;

		Menu_AddItem(&s_loadgame_menu, &s_loadgame_actions[i]);
	}
}

void
LoadGame_MenuDraw(void)
{
	M_Banner("m_banner_load_game");
	/* Menu_AdjustCursor( &s_loadgame_menu, 1 ); */
	Menu_Draw(&s_loadgame_menu);
}

const char     *
LoadGame_MenuKey(int key)
{
	if (key == K_ESCAPE || key == K_ENTER) {
		s_savegame_menu.cursor = s_loadgame_menu.cursor - 1;
		if (s_savegame_menu.cursor < 0)
			s_savegame_menu.cursor = 0;
	}
	return Default_MenuKey(&s_loadgame_menu, key);
}

void
M_Menu_LoadGame_f(void)
{
	LoadGame_MenuInit();
	M_PushMenu(LoadGame_MenuDraw, LoadGame_MenuKey);
}


/*
 *
 * ============================================================================
 * =
 *
 * SAVEGAME MENU
 *
 * =============================================================================
 */
static menuframework_s s_savegame_menu;
static menuaction_s s_savegame_actions[MAX_SAVEGAMES];

void
SaveGameCallback(void *self)
{
	menuaction_s   *a = (menuaction_s *) self;

	Cbuf_AddText(va("save save%i\n", a->generic.localdata[0]));
	M_ForceMenuOff();
}

void
SaveGame_MenuDraw(void)
{
	M_Banner("m_banner_save_game");
	Menu_AdjustCursor(&s_savegame_menu, 1);
	Menu_Draw(&s_savegame_menu);
}

void
SaveGame_MenuInit(void)
{
	int		i;

	s_savegame_menu.x = viddef.width / 2 - 120;
	s_savegame_menu.y = viddef.height / 2 - 58;
	s_savegame_menu.nitems = 0;

	Create_Savestrings();

	/* don't include the autosave slot */
	for (i = 0; i < MAX_SAVEGAMES - 1; i++) {
		s_savegame_actions[i].generic.name = m_savestrings[i + 1];
		s_savegame_actions[i].generic.localdata[0] = i + 1;
		s_savegame_actions[i].generic.flags = QMF_LEFT_JUSTIFY;
		s_savegame_actions[i].generic.callback = SaveGameCallback;

		s_savegame_actions[i].generic.x = 0;
		s_savegame_actions[i].generic.y = (i) * 10;

		s_savegame_actions[i].generic.type = MTYPE_ACTION;

		Menu_AddItem(&s_savegame_menu, &s_savegame_actions[i]);
	}
}

const char     *
SaveGame_MenuKey(int key)
{
	if (key == K_ENTER || key == K_ESCAPE) {
		s_loadgame_menu.cursor = s_savegame_menu.cursor - 1;
		if (s_loadgame_menu.cursor < 0)
			s_loadgame_menu.cursor = 0;
	}
	return Default_MenuKey(&s_savegame_menu, key);
}

void
M_Menu_SaveGame_f(void)
{
	if (!Com_ServerState())
		return;		/* not playing a game */

	SaveGame_MenuInit();
	M_PushMenu(SaveGame_MenuDraw, SaveGame_MenuKey);
	Create_Savestrings();
}


/*
 *
 * ============================================================================
 * =
 *
 * JOIN SERVER MENU
 *
 * =============================================================================
 */
#define MAX_LOCAL_SERVERS 8

static menuframework_s s_joinserver_menu;
static menuseparator_s s_joinserver_server_title;
static menuaction_s s_joinserver_search_action;
static menuaction_s s_joinserver_address_book_action;
static menuaction_s s_joinserver_server_actions[MAX_LOCAL_SERVERS];

int		m_num_servers;
#define	NO_SERVER_STRING	"<no server>"

/* user readable information */
static char	local_server_names[MAX_LOCAL_SERVERS][80];

/* network address */
static netadr_t	local_server_netadr[MAX_LOCAL_SERVERS];

void
M_AddToServerList(netadr_t adr, char *info)
{
	int		i;

	if (m_num_servers == MAX_LOCAL_SERVERS)
		return;
	while (*info == ' ')
		info++;

	/* Ignore if duplicated. */
	for (i = 0; i < m_num_servers; i++)
		if (!strcmp(info, local_server_names[i]))
			return;

	local_server_netadr[m_num_servers] = adr;
#ifdef HAVE_IPV6
	/*
	 * Show the IP address as well. Useful to identify whether the server
	 * is IPv6 or IPv4.
	 */
	Com_sprintf(local_server_names[m_num_servers], sizeof(local_server_names[0]) - 1,
		    "%s %s", info, NET_AdrToString(adr));
#else
	Q_strncpyz(local_server_names[m_num_servers], info, sizeof(local_server_names[0]));
#endif
	m_num_servers++;
}

void
JoinServerFunc(void *self)
{
	char		buffer[128];
	int		index;

	index = (menuaction_s *) self - s_joinserver_server_actions;

	if (Q_stricmp(local_server_names[index], NO_SERVER_STRING) == 0)
		return;

	if (index >= m_num_servers)
		return;

	Com_sprintf(buffer, sizeof(buffer), "connect %s\n", NET_AdrToString(local_server_netadr[index]));
	Cbuf_AddText(buffer);
	M_ForceMenuOff();
}

void
AddressBookFunc(void *self)
{
	M_Menu_AddressBook_f();
}

void
NullCursorDraw(void *self)
{
}

void
SearchLocalGames(void)
{
	int		i;

	m_num_servers = 0;
	for (i = 0; i < MAX_LOCAL_SERVERS; i++)
		Q_strncpyz(local_server_names[i], NO_SERVER_STRING, sizeof(local_server_names[0]));

	M_DrawTextBox(8, 120 - 48, 36, 3);
	M_Print(16 + 16, 120 - 48 + 8, "Searching for local servers, this");
	M_Print(16 + 16, 120 - 48 + 16, "could take up to a minute, so");
	M_Print(16 + 16, 120 - 48 + 24, "please be patient.");

	/* the text box won't show up unless we do a buffer swap */
	re.EndFrame();

	/* send out info packets */
	CL_PingServers_f();
}

void
SearchLocalGamesFunc(void *self)
{
	SearchLocalGames();
}

void
JoinServer_MenuInit(void)
{
	int		i;

	s_joinserver_menu.x = viddef.width * 0.50 - 120;
	s_joinserver_menu.nitems = 0;

	s_joinserver_address_book_action.generic.type = MTYPE_ACTION;
	s_joinserver_address_book_action.generic.name = "Address book";
	s_joinserver_address_book_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_joinserver_address_book_action.generic.x = 0;
	s_joinserver_address_book_action.generic.y = 0;
	s_joinserver_address_book_action.generic.callback = AddressBookFunc;

	s_joinserver_search_action.generic.type = MTYPE_ACTION;
	s_joinserver_search_action.generic.name = "Refresh server list";
	s_joinserver_search_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_joinserver_search_action.generic.x = 0;
	s_joinserver_search_action.generic.y = 10;
	s_joinserver_search_action.generic.callback = SearchLocalGamesFunc;
	s_joinserver_search_action.generic.statusbar = "Search for servers";

	s_joinserver_server_title.generic.type = MTYPE_SEPARATOR;
	s_joinserver_server_title.generic.name = "Connect to...";
	s_joinserver_server_title.generic.x = 80;
	s_joinserver_server_title.generic.y = 30;

	for (i = 0; i < MAX_LOCAL_SERVERS; i++) {
		s_joinserver_server_actions[i].generic.type = MTYPE_ACTION;
		Q_strncpyz(local_server_names[i], NO_SERVER_STRING, sizeof(local_server_names[0]));
		s_joinserver_server_actions[i].generic.name = local_server_names[i];
		s_joinserver_server_actions[i].generic.flags = QMF_LEFT_JUSTIFY;
		s_joinserver_server_actions[i].generic.x = 0;
		s_joinserver_server_actions[i].generic.y = 40 + i * 10;
		s_joinserver_server_actions[i].generic.callback = JoinServerFunc;
		s_joinserver_server_actions[i].generic.statusbar = "press ENTER to connect";
	}

	Menu_AddItem(&s_joinserver_menu, &s_joinserver_address_book_action);
	Menu_AddItem(&s_joinserver_menu, &s_joinserver_server_title);
	Menu_AddItem(&s_joinserver_menu, &s_joinserver_search_action);

	for (i = 0; i < MAX_LOCAL_SERVERS; i++)
		Menu_AddItem(&s_joinserver_menu, &s_joinserver_server_actions[i]);

	Menu_Center(&s_joinserver_menu);

	SearchLocalGames();
}

void
JoinServer_MenuDraw(void)
{
	M_Banner("m_banner_join_server");
	Menu_Draw(&s_joinserver_menu);
}


const char     *
JoinServer_MenuKey(int key)
{
	return Default_MenuKey(&s_joinserver_menu, key);
}

void
M_Menu_JoinServer_f(void)
{
	JoinServer_MenuInit();
	M_PushMenu(JoinServer_MenuDraw, JoinServer_MenuKey);
}


/*
 * ============================================================================
 *
 * START SERVER MENU
 *
 * =============================================================================
 */
static menuframework_s s_startserver_menu;
static char   **mapnames;
static int	nummaps;

static menuaction_s s_startserver_start_action;
static menuaction_s s_startserver_dmoptions_action;
static menufield_s s_timelimit_field;
static menufield_s s_fraglimit_field;
static menufield_s s_maxclients_field;
static menufield_s s_hostname_field;
static menulist_s s_startmap_list;
static menulist_s s_rules_box;

void
DMOptionsFunc(void *self)
{
	if (s_rules_box.curvalue == 1)
		return;
	M_Menu_DMOptions_f();
}

void
RulesChangeFunc(void *self)
{
	/* DM */
	if (s_rules_box.curvalue == 0) {
		s_maxclients_field.generic.statusbar = NULL;
		s_startserver_dmoptions_action.generic.statusbar = NULL;
	} else if (s_rules_box.curvalue == 1) {	/* coop		PGM */
		s_maxclients_field.generic.statusbar = "4 maximum for cooperative";
		if (atoi(s_maxclients_field.buffer) > 4)
			Q_strncpyz(s_maxclients_field.buffer, "4", sizeof(s_maxclients_field.buffer));
		s_startserver_dmoptions_action.generic.statusbar = "N/A for cooperative";
	}
	/* PGM */
	/* ROGUE GAMES */
	else if (Developer_searchpath(2) == 2) {
		if (s_rules_box.curvalue == 2) {	/* tag	 */
			s_maxclients_field.generic.statusbar = NULL;
			s_startserver_dmoptions_action.generic.statusbar = NULL;
		}
#if 0
		else if (s_rules_box.curvalue == 3) {	/* deathball */
			s_maxclients_field.generic.statusbar = NULL;
			s_startserver_dmoptions_action.generic.statusbar = NULL;
		}
#endif
	}
	/* PGM */
}

void
StartServerActionFunc(void *self)
{
	char		startmap[1024];
	int		timelimit;
	int		fraglimit;
	int		maxclients;
	char           *spot;

	Q_strncpyz(startmap, strchr(mapnames[s_startmap_list.curvalue], '\n') + 1, sizeof(startmap));

	maxclients = atoi(s_maxclients_field.buffer);
	timelimit = atoi(s_timelimit_field.buffer);
	fraglimit = atoi(s_fraglimit_field.buffer);

	Cvar_SetValue("maxclients", ClampCvar(0, maxclients, maxclients));
	Cvar_SetValue("timelimit", ClampCvar(0, timelimit, timelimit));
	Cvar_SetValue("fraglimit", ClampCvar(0, fraglimit, fraglimit));
	Cvar_Set("hostname", s_hostname_field.buffer);
	/* Cvar_SetValue ("deathmatch", !s_rules_box.curvalue ); */
	/* Cvar_SetValue ("coop", s_rules_box.curvalue ); */

	/* PGM */
	if ((s_rules_box.curvalue < 2) || (Developer_searchpath(2) != 2)) {
		Cvar_SetValue("deathmatch", !s_rules_box.curvalue);
		Cvar_SetValue("coop", s_rules_box.curvalue);
		Cvar_SetValue("gamerules", 0);
	} else {
		Cvar_SetValue("deathmatch", 1);	/* deathmatch is always true for rogue games, right? */
		Cvar_SetValue("coop", 0);	/* FIXME - this might need to depend on which game we're running */
		Cvar_SetValue("gamerules", s_rules_box.curvalue);
	}
	/* PGM */

	spot = NULL;
	if (s_rules_box.curvalue == 1) {	/* PGM */
		if (Q_stricmp(startmap, "bunk1") == 0)
			spot = "start";
		else if (Q_stricmp(startmap, "mintro") == 0)
			spot = "start";
		else if (Q_stricmp(startmap, "fact1") == 0)
			spot = "start";
		else if (Q_stricmp(startmap, "power1") == 0)
			spot = "pstart";
		else if (Q_stricmp(startmap, "biggun") == 0)
			spot = "bstart";
		else if (Q_stricmp(startmap, "hangar1") == 0)
			spot = "unitstart";
		else if (Q_stricmp(startmap, "city1") == 0)
			spot = "unitstart";
		else if (Q_stricmp(startmap, "boss1") == 0)
			spot = "bosstart";
	}
	if (spot) {
		if (Com_ServerState())
			Cbuf_AddText("disconnect\n");
		Cbuf_AddText(va("gamemap \"*%s$%s\"\n", startmap, spot));
	} else {
		Cbuf_AddText(va("map %s\n", startmap));
	}

	M_ForceMenuOff();
}

void
StartServer_MenuInit(void)
{
	static const char *dm_coop_names[] =
	{
		"Deathmatch",
		"Cooperative",
		0
	};

	/* PGM */
	static const char *dm_coop_names_rogue[] =
	{
		"Deathmatch",
		"Cooperative",
		"Tag",
		/* "deathball", */
		0
	};
	/* PGM */

	char           *buffer;
	char		mapsname  [1024];
	char           *s;
	int		length;
	int		i;
	FILE           *fp;

	/*
	 * * load the list of map names
	 */
	Com_sprintf(mapsname, sizeof(mapsname), "%s/maps.lst", FS_Gamedir());
	if ((fp = fopen(mapsname, "rb")) == 0) {
		if ((length = FS_LoadFile("maps.lst", (void **)&buffer)) == -1)
			Com_Error(ERR_DROP, "couldn't find maps.lst\n");
	} else {
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buffer = malloc(length);
		fread(buffer, length, 1, fp);
	}

	s = buffer;

	i = 0;
	while (i < length) {
		if (s[i] == '\r')
			nummaps++;
		i++;
	}

	if (nummaps == 0)
		Com_Error(ERR_DROP, "no maps in maps.lst\n");

	mapnames = malloc(sizeof(char *) * (nummaps + 1));
	memset(mapnames, 0, sizeof(char *) * (nummaps + 1));

	s = buffer;

	for (i = 0; i < nummaps; i++) {
		char		shortname[MAX_TOKEN_CHARS];
		char		longname[MAX_TOKEN_CHARS];
		char		scratch[200];
		int		j, l;

		Q_strncpyz(shortname, COM_Parse(&s), sizeof(shortname));
		l = strlen(shortname);
		for (j = 0; j < l; j++)
			shortname[j] = toupper(shortname[j]);
		Q_strncpyz(longname, COM_Parse(&s), sizeof(longname));
		Com_sprintf(scratch, sizeof(scratch), "%s\n%s", longname, shortname);

		l = strlen(scratch) + 1;
		mapnames[i] = malloc(l);
		Q_strncpyz(mapnames[i], scratch, l);
	}
	mapnames[nummaps] = 0;

	if (fp != 0) {
		fp = 0;
		free(buffer);
	} else {
		FS_FreeFile(buffer);
	}

	/*
	 * * initialize the menu stuff
	 */
	s_startserver_menu.x = viddef.width * 0.50;
	s_startserver_menu.nitems = 0;

	s_startmap_list.generic.type = MTYPE_SPINCONTROL;
	s_startmap_list.generic.x = 0;
	s_startmap_list.generic.y = 0;
	s_startmap_list.generic.name = "Initial map";
	s_startmap_list.itemnames = (const char **)mapnames;

	s_rules_box.generic.type = MTYPE_SPINCONTROL;
	s_rules_box.generic.x = 0;
	s_rules_box.generic.y = 20;
	s_rules_box.generic.name = "Rules";

	/* PGM - rogue games only available with rogue DLL. */
	if (Developer_searchpath(2) == 2)
		s_rules_box.itemnames = dm_coop_names_rogue;
	else
		s_rules_box.itemnames = dm_coop_names;
	/* PGM */

	if (Cvar_VariableValue("coop"))
		s_rules_box.curvalue = 1;
	else
		s_rules_box.curvalue = 0;
	s_rules_box.generic.callback = RulesChangeFunc;

	s_timelimit_field.generic.type = MTYPE_FIELD;
	s_timelimit_field.generic.name = "Time limit";
	s_timelimit_field.generic.flags = QMF_NUMBERSONLY;
	s_timelimit_field.generic.x = 0;
	s_timelimit_field.generic.y = 36;
	s_timelimit_field.generic.statusbar = "0 = no limit";
	s_timelimit_field.length = 3;
	s_timelimit_field.visible_length = 3;
	Q_strncpyz(s_timelimit_field.buffer, Cvar_VariableString("timelimit"), sizeof(s_timelimit_field.buffer));

	s_fraglimit_field.generic.type = MTYPE_FIELD;
	s_fraglimit_field.generic.name = "Frag limit";
	s_fraglimit_field.generic.flags = QMF_NUMBERSONLY;
	s_fraglimit_field.generic.x = 0;
	s_fraglimit_field.generic.y = 54;
	s_fraglimit_field.generic.statusbar = "0 = no limit";
	s_fraglimit_field.length = 3;
	s_fraglimit_field.visible_length = 3;
	Q_strncpyz(s_fraglimit_field.buffer, Cvar_VariableString("fraglimit"), sizeof(s_fraglimit_field.buffer));

	/*
	 * * maxclients determines the maximum number of players that can
	 * join * the game.  If maxclients is only "1" then we should default
	 * the menu * option to 8 players, otherwise use whatever its current
	 * value is. * Clamping will be done when the server is actually
	 * started.
	 */
	s_maxclients_field.generic.type = MTYPE_FIELD;
	s_maxclients_field.generic.name = "Max players";
	s_maxclients_field.generic.flags = QMF_NUMBERSONLY;
	s_maxclients_field.generic.x = 0;
	s_maxclients_field.generic.y = 72;
	s_maxclients_field.generic.statusbar = NULL;
	s_maxclients_field.length = 3;
	s_maxclients_field.visible_length = 3;
	Q_strncpyz(s_maxclients_field.buffer, (Cvar_VariableValue("maxclients") == 1) ? "8" : Cvar_VariableString("maxclients"), sizeof(s_maxclients_field.buffer));

	s_hostname_field.generic.type = MTYPE_FIELD;
	s_hostname_field.generic.name = "Hostname";
	s_hostname_field.generic.flags = 0;
	s_hostname_field.generic.x = 0;
	s_hostname_field.generic.y = 90;
	s_hostname_field.generic.statusbar = NULL;
	s_hostname_field.length = 12;
	s_hostname_field.visible_length = 12;
	Q_strncpyz(s_hostname_field.buffer, Cvar_VariableString("hostname"), sizeof(s_hostname_field.buffer));

	s_startserver_dmoptions_action.generic.type = MTYPE_ACTION;
	s_startserver_dmoptions_action.generic.name = " Deathmatch flags";
	s_startserver_dmoptions_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_startserver_dmoptions_action.generic.x = 24;
	s_startserver_dmoptions_action.generic.y = 108;
	s_startserver_dmoptions_action.generic.statusbar = NULL;
	s_startserver_dmoptions_action.generic.callback = DMOptionsFunc;

	s_startserver_start_action.generic.type = MTYPE_ACTION;
	s_startserver_start_action.generic.name = " Begin";
	s_startserver_start_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_startserver_start_action.generic.x = 24;
	s_startserver_start_action.generic.y = 128;
	s_startserver_start_action.generic.callback = StartServerActionFunc;

	Menu_AddItem(&s_startserver_menu, &s_startmap_list);
	Menu_AddItem(&s_startserver_menu, &s_rules_box);
	Menu_AddItem(&s_startserver_menu, &s_timelimit_field);
	Menu_AddItem(&s_startserver_menu, &s_fraglimit_field);
	Menu_AddItem(&s_startserver_menu, &s_maxclients_field);
	Menu_AddItem(&s_startserver_menu, &s_hostname_field);
	Menu_AddItem(&s_startserver_menu, &s_startserver_dmoptions_action);
	Menu_AddItem(&s_startserver_menu, &s_startserver_start_action);

	Menu_Center(&s_startserver_menu);

	/* call this now to set proper inital state */
	RulesChangeFunc(NULL);
}

void
StartServer_MenuDraw(void)
{
	Menu_Draw(&s_startserver_menu);
}

const char     *
StartServer_MenuKey(int key)
{
	if (key == K_ESCAPE) {
		if (mapnames) {
			int		i;

			for (i = 0; i < nummaps; i++)
				free(mapnames[i]);
			free(mapnames);
		}
		mapnames = 0;
		nummaps = 0;
	}
	return Default_MenuKey(&s_startserver_menu, key);
}

void
M_Menu_StartServer_f(void)
{
	StartServer_MenuInit();
	M_PushMenu(StartServer_MenuDraw, StartServer_MenuKey);
}

/*
 * =======================================================================
 *
 * DMOPTIONS BOOK MENU
 *
 * =======================================================================
 */
static char		dmoptions_statusbar[128];

static menuframework_s	s_dmoptions_menu;
static menulist_s	s_friendlyfire_box;
static menulist_s	s_falls_box;
static menulist_s	s_weapons_stay_box;
static menulist_s	s_instant_powerups_box;
static menulist_s	s_powerups_box;
static menulist_s	s_health_box;
static menulist_s	s_spawn_farthest_box;
static menulist_s	s_teamplay_box;
static menulist_s	s_samelevel_box;
static menulist_s	s_force_respawn_box;
static menulist_s	s_armor_box;
static menulist_s	s_allow_exit_box;
static menulist_s	s_infinite_ammo_box;
static menulist_s	s_fixed_fov_box;
static menulist_s	s_quad_drop_box;

/* ROGUE */
static menulist_s	s_no_mines_box;
static menulist_s	s_no_nukes_box;
static menulist_s	s_stack_double_box;
static menulist_s	s_no_spheres_box;
/* ROGUE */

static void
DMFlagCallback(void *self)
{
	menulist_s     *f = (menulist_s *) self;
	int		flags;
	int		bit = 0;

	flags = Cvar_VariableValue("dmflags");

	if (f == &s_friendlyfire_box) {
		if (f->curvalue)
			flags &= ~DF_NO_FRIENDLY_FIRE;
		else
			flags |= DF_NO_FRIENDLY_FIRE;
		goto setvalue;
	} else if (f == &s_falls_box) {
		if (f->curvalue)
			flags &= ~DF_NO_FALLING;
		else
			flags |= DF_NO_FALLING;
		goto setvalue;
	} else if (f == &s_weapons_stay_box) {
		bit = DF_WEAPONS_STAY;
	} else if (f == &s_instant_powerups_box) {
		bit = DF_INSTANT_ITEMS;
	} else if (f == &s_allow_exit_box) {
		bit = DF_ALLOW_EXIT;
	} else if (f == &s_powerups_box) {
		if (f->curvalue)
			flags &= ~DF_NO_ITEMS;
		else
			flags |= DF_NO_ITEMS;
		goto setvalue;
	} else if (f == &s_health_box) {
		if (f->curvalue)
			flags &= ~DF_NO_HEALTH;
		else
			flags |= DF_NO_HEALTH;
		goto setvalue;
	} else if (f == &s_spawn_farthest_box) {
		bit = DF_SPAWN_FARTHEST;
	} else if (f == &s_teamplay_box) {
		if (f->curvalue == 1) {
			flags |= DF_SKINTEAMS;
			flags &= ~DF_MODELTEAMS;
		} else if (f->curvalue == 2) {
			flags |= DF_MODELTEAMS;
			flags &= ~DF_SKINTEAMS;
		} else {
			flags &= ~(DF_MODELTEAMS | DF_SKINTEAMS);
		}

		goto setvalue;
	} else if (f == &s_samelevel_box) {
		bit = DF_SAME_LEVEL;
	} else if (f == &s_force_respawn_box) {
		bit = DF_FORCE_RESPAWN;
	} else if (f == &s_armor_box) {
		if (f->curvalue)
			flags &= ~DF_NO_ARMOR;
		else
			flags |= DF_NO_ARMOR;
		goto setvalue;
	} else if (f == &s_infinite_ammo_box) {
		bit = DF_INFINITE_AMMO;
	} else if (f == &s_fixed_fov_box) {
		bit = DF_FIXED_FOV;
	} else if (f == &s_quad_drop_box) {
		bit = DF_QUAD_DROP;
	}
	/* ROGUE */
	else if (Developer_searchpath(2) == 2) {
		if (f == &s_no_mines_box) {
			bit = DF_NO_MINES;
		} else if (f == &s_no_nukes_box) {
			bit = DF_NO_NUKES;
		} else if (f == &s_stack_double_box) {
			bit = DF_NO_STACK_DOUBLE;
		} else if (f == &s_no_spheres_box) {
			bit = DF_NO_SPHERES;
		}
	}
	/* ROGUE */

	if (f) {
		if (f->curvalue == 0)
			flags &= ~bit;
		else
			flags |= bit;
	}
setvalue:
	Cvar_SetValue("dmflags", flags);

	Com_sprintf(dmoptions_statusbar, sizeof(dmoptions_statusbar), "dmflags = %d", flags);

}

void
DMOptions_MenuInit(void)
{
	static const char *yes_no_names[] = {
		"No", 
		"Yes", 
		0
	};

	static const char *teamplay_names[] = {
		"Disabled", 
		"By Skin", 
		"By Model", 0
	};

	int		dmflags = Cvar_VariableValue("dmflags");
	int		y = 0;

	s_dmoptions_menu.x = viddef.width * 0.50;
	s_dmoptions_menu.nitems = 0;

	s_falls_box.generic.type = MTYPE_SPINCONTROL;
	s_falls_box.generic.x = 0;
	s_falls_box.generic.y = y;
	s_falls_box.generic.name = "Falling damage";
	s_falls_box.generic.callback = DMFlagCallback;
	s_falls_box.itemnames = yes_no_names;
	s_falls_box.curvalue = (dmflags & DF_NO_FALLING) == 0;

	s_weapons_stay_box.generic.type = MTYPE_SPINCONTROL;
	s_weapons_stay_box.generic.x = 0;
	s_weapons_stay_box.generic.y = y += 10;
	s_weapons_stay_box.generic.name = "Weapons stay";
	s_weapons_stay_box.generic.callback = DMFlagCallback;
	s_weapons_stay_box.itemnames = yes_no_names;
	s_weapons_stay_box.curvalue = (dmflags & DF_WEAPONS_STAY) != 0;

	s_instant_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_instant_powerups_box.generic.x = 0;
	s_instant_powerups_box.generic.y = y += 10;
	s_instant_powerups_box.generic.name = "Instant powerups";
	s_instant_powerups_box.generic.callback = DMFlagCallback;
	s_instant_powerups_box.itemnames = yes_no_names;
	s_instant_powerups_box.curvalue = (dmflags & DF_INSTANT_ITEMS) != 0;

	s_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_powerups_box.generic.x = 0;
	s_powerups_box.generic.y = y += 10;
	s_powerups_box.generic.name = "Allow powerups";
	s_powerups_box.generic.callback = DMFlagCallback;
	s_powerups_box.itemnames = yes_no_names;
	s_powerups_box.curvalue = (dmflags & DF_NO_ITEMS) == 0;

	s_health_box.generic.type = MTYPE_SPINCONTROL;
	s_health_box.generic.x = 0;
	s_health_box.generic.y = y += 10;
	s_health_box.generic.callback = DMFlagCallback;
	s_health_box.generic.name = "Allow health";
	s_health_box.itemnames = yes_no_names;
	s_health_box.curvalue = (dmflags & DF_NO_HEALTH) == 0;

	s_armor_box.generic.type = MTYPE_SPINCONTROL;
	s_armor_box.generic.x = 0;
	s_armor_box.generic.y = y += 10;
	s_armor_box.generic.name = "Allow armor";
	s_armor_box.generic.callback = DMFlagCallback;
	s_armor_box.itemnames = yes_no_names;
	s_armor_box.curvalue = (dmflags & DF_NO_ARMOR) == 0;

	s_spawn_farthest_box.generic.type = MTYPE_SPINCONTROL;
	s_spawn_farthest_box.generic.x = 0;
	s_spawn_farthest_box.generic.y = y += 10;
	s_spawn_farthest_box.generic.name = "Spawn farthest";
	s_spawn_farthest_box.generic.callback = DMFlagCallback;
	s_spawn_farthest_box.itemnames = yes_no_names;
	s_spawn_farthest_box.curvalue = (dmflags & DF_SPAWN_FARTHEST) != 0;

	s_samelevel_box.generic.type = MTYPE_SPINCONTROL;
	s_samelevel_box.generic.x = 0;
	s_samelevel_box.generic.y = y += 10;
	s_samelevel_box.generic.name = "Same map";
	s_samelevel_box.generic.callback = DMFlagCallback;
	s_samelevel_box.itemnames = yes_no_names;
	s_samelevel_box.curvalue = (dmflags & DF_SAME_LEVEL) != 0;

	s_force_respawn_box.generic.type = MTYPE_SPINCONTROL;
	s_force_respawn_box.generic.x = 0;
	s_force_respawn_box.generic.y = y += 10;
	s_force_respawn_box.generic.name = "Force respawn";
	s_force_respawn_box.generic.callback = DMFlagCallback;
	s_force_respawn_box.itemnames = yes_no_names;
	s_force_respawn_box.curvalue = (dmflags & DF_FORCE_RESPAWN) != 0;

	s_teamplay_box.generic.type = MTYPE_SPINCONTROL;
	s_teamplay_box.generic.x = 0;
	s_teamplay_box.generic.y = y += 10;
	s_teamplay_box.generic.name = "Teamplay";
	s_teamplay_box.generic.callback = DMFlagCallback;
	s_teamplay_box.itemnames = teamplay_names;

	s_allow_exit_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_exit_box.generic.x = 0;
	s_allow_exit_box.generic.y = y += 10;
	s_allow_exit_box.generic.name = "Allow exit";
	s_allow_exit_box.generic.callback = DMFlagCallback;
	s_allow_exit_box.itemnames = yes_no_names;
	s_allow_exit_box.curvalue = (dmflags & DF_ALLOW_EXIT) != 0;

	s_infinite_ammo_box.generic.type = MTYPE_SPINCONTROL;
	s_infinite_ammo_box.generic.x = 0;
	s_infinite_ammo_box.generic.y = y += 10;
	s_infinite_ammo_box.generic.name = "Infinite ammo";
	s_infinite_ammo_box.generic.callback = DMFlagCallback;
	s_infinite_ammo_box.itemnames = yes_no_names;
	s_infinite_ammo_box.curvalue = (dmflags & DF_INFINITE_AMMO) != 0;

	s_fixed_fov_box.generic.type = MTYPE_SPINCONTROL;
	s_fixed_fov_box.generic.x = 0;
	s_fixed_fov_box.generic.y = y += 10;
	s_fixed_fov_box.generic.name = "Fixed FOV";
	s_fixed_fov_box.generic.callback = DMFlagCallback;
	s_fixed_fov_box.itemnames = yes_no_names;
	s_fixed_fov_box.curvalue = (dmflags & DF_FIXED_FOV) != 0;

	s_quad_drop_box.generic.type = MTYPE_SPINCONTROL;
	s_quad_drop_box.generic.x = 0;
	s_quad_drop_box.generic.y = y += 10;
	s_quad_drop_box.generic.name = "Quad drop";
	s_quad_drop_box.generic.callback = DMFlagCallback;
	s_quad_drop_box.itemnames = yes_no_names;
	s_quad_drop_box.curvalue = (dmflags & DF_QUAD_DROP) != 0;

	s_friendlyfire_box.generic.type = MTYPE_SPINCONTROL;
	s_friendlyfire_box.generic.x = 0;
	s_friendlyfire_box.generic.y = y += 10;
	s_friendlyfire_box.generic.name = "Friendly fire";
	s_friendlyfire_box.generic.callback = DMFlagCallback;
	s_friendlyfire_box.itemnames = yes_no_names;
	s_friendlyfire_box.curvalue = (dmflags & DF_NO_FRIENDLY_FIRE) == 0;

	/* ROGUE */
	if (Developer_searchpath(2) == 2) {
		s_no_mines_box.generic.type = MTYPE_SPINCONTROL;
		s_no_mines_box.generic.x = 0;
		s_no_mines_box.generic.y = y += 10;
		s_no_mines_box.generic.name = "Remove mines";
		s_no_mines_box.generic.callback = DMFlagCallback;
		s_no_mines_box.itemnames = yes_no_names;
		s_no_mines_box.curvalue = (dmflags & DF_NO_MINES) != 0;

		s_no_nukes_box.generic.type = MTYPE_SPINCONTROL;
		s_no_nukes_box.generic.x = 0;
		s_no_nukes_box.generic.y = y += 10;
		s_no_nukes_box.generic.name = "Remove nukes";
		s_no_nukes_box.generic.callback = DMFlagCallback;
		s_no_nukes_box.itemnames = yes_no_names;
		s_no_nukes_box.curvalue = (dmflags & DF_NO_NUKES) != 0;

		s_stack_double_box.generic.type = MTYPE_SPINCONTROL;
		s_stack_double_box.generic.x = 0;
		s_stack_double_box.generic.y = y += 10;
		s_stack_double_box.generic.name = "2x/4x Stacking off";
		s_stack_double_box.generic.callback = DMFlagCallback;
		s_stack_double_box.itemnames = yes_no_names;
		s_stack_double_box.curvalue = (dmflags & DF_NO_STACK_DOUBLE) != 0;

		s_no_spheres_box.generic.type = MTYPE_SPINCONTROL;
		s_no_spheres_box.generic.x = 0;
		s_no_spheres_box.generic.y = y += 10;
		s_no_spheres_box.generic.name = "Remove spheres";
		s_no_spheres_box.generic.callback = DMFlagCallback;
		s_no_spheres_box.itemnames = yes_no_names;
		s_no_spheres_box.curvalue = (dmflags & DF_NO_SPHERES) != 0;

	}
	/* ROGUE */

	Menu_AddItem(&s_dmoptions_menu, &s_falls_box);
	Menu_AddItem(&s_dmoptions_menu, &s_weapons_stay_box);
	Menu_AddItem(&s_dmoptions_menu, &s_instant_powerups_box);
	Menu_AddItem(&s_dmoptions_menu, &s_powerups_box);
	Menu_AddItem(&s_dmoptions_menu, &s_health_box);
	Menu_AddItem(&s_dmoptions_menu, &s_armor_box);
	Menu_AddItem(&s_dmoptions_menu, &s_spawn_farthest_box);
	Menu_AddItem(&s_dmoptions_menu, &s_samelevel_box);
	Menu_AddItem(&s_dmoptions_menu, &s_force_respawn_box);
	Menu_AddItem(&s_dmoptions_menu, &s_teamplay_box);
	Menu_AddItem(&s_dmoptions_menu, &s_allow_exit_box);
	Menu_AddItem(&s_dmoptions_menu, &s_infinite_ammo_box);
	Menu_AddItem(&s_dmoptions_menu, &s_fixed_fov_box);
	Menu_AddItem(&s_dmoptions_menu, &s_quad_drop_box);
	Menu_AddItem(&s_dmoptions_menu, &s_friendlyfire_box);

	/* ROGUE */
	if (Developer_searchpath(2) == 2) {
		Menu_AddItem(&s_dmoptions_menu, &s_no_mines_box);
		Menu_AddItem(&s_dmoptions_menu, &s_no_nukes_box);
		Menu_AddItem(&s_dmoptions_menu, &s_stack_double_box);
		Menu_AddItem(&s_dmoptions_menu, &s_no_spheres_box);
	}
	/* ROGUE */

	Menu_Center(&s_dmoptions_menu);

	/* set the original dmflags statusbar */
	DMFlagCallback(0);
	Menu_SetStatusBar(&s_dmoptions_menu, dmoptions_statusbar);
}

void
DMOptions_MenuDraw(void)
{
	Menu_Draw(&s_dmoptions_menu);
}

const char     *
DMOptions_MenuKey(int key)
{
	return Default_MenuKey(&s_dmoptions_menu, key);
}

void
M_Menu_DMOptions_f(void)
{
	DMOptions_MenuInit();
	M_PushMenu(DMOptions_MenuDraw, DMOptions_MenuKey);
}

/*
 *
 * ============================================================================
 * =
 *
 * DOWNLOADOPTIONS BOOK MENU
 *
 * =============================================================================
 */
static menuframework_s s_downloadoptions_menu;

static menuseparator_s s_download_title;
static menulist_s s_allow_download_box;
static menulist_s s_allow_download_maps_box;
static menulist_s s_allow_download_models_box;
static menulist_s s_allow_download_players_box;
static menulist_s s_allow_download_sounds_box;

static void
DownloadCallback(void *self)
{
	menulist_s     *f = (menulist_s *) self;

	if (f == &s_allow_download_box) {
		Cvar_SetValue("allow_download", f->curvalue);
	} else if (f == &s_allow_download_maps_box) {
		Cvar_SetValue("allow_download_maps", f->curvalue);
	} else if (f == &s_allow_download_models_box) {
		Cvar_SetValue("allow_download_models", f->curvalue);
	} else if (f == &s_allow_download_players_box) {
		Cvar_SetValue("allow_download_players", f->curvalue);
	} else if (f == &s_allow_download_sounds_box) {
		Cvar_SetValue("allow_download_sounds", f->curvalue);
	}
}

void
DownloadOptions_MenuInit(void)
{
	static const char *yes_no_names[] =
	{
		"No", 
		"Yes", 
		0
	};
	int		y = 0;

	s_downloadoptions_menu.x = viddef.width * 0.50;
	s_downloadoptions_menu.nitems = 0;

	s_download_title.generic.type = MTYPE_SEPARATOR;
	s_download_title.generic.name = "Download Options";
	s_download_title.generic.x = 48;
	s_download_title.generic.y = y;

	s_allow_download_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_box.generic.x = 0;
	s_allow_download_box.generic.y = y += 20;
	s_allow_download_box.generic.name = "Allow downloading";
	s_allow_download_box.generic.callback = DownloadCallback;
	s_allow_download_box.itemnames = yes_no_names;
	s_allow_download_box.curvalue = (Cvar_VariableValue("allow_download") != 0);

	s_allow_download_maps_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_maps_box.generic.x = 0;
	s_allow_download_maps_box.generic.y = y += 20;
	s_allow_download_maps_box.generic.name = "Maps";
	s_allow_download_maps_box.generic.callback = DownloadCallback;
	s_allow_download_maps_box.itemnames = yes_no_names;
	s_allow_download_maps_box.curvalue = (Cvar_VariableValue("allow_download_maps") != 0);

	s_allow_download_players_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_players_box.generic.x = 0;
	s_allow_download_players_box.generic.y = y += 10;
	s_allow_download_players_box.generic.name = "Player Models/Skins";
	s_allow_download_players_box.generic.callback = DownloadCallback;
	s_allow_download_players_box.itemnames = yes_no_names;
	s_allow_download_players_box.curvalue = (Cvar_VariableValue("allow_download_players") != 0);

	s_allow_download_models_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_models_box.generic.x = 0;
	s_allow_download_models_box.generic.y = y += 10;
	s_allow_download_models_box.generic.name = "Models";
	s_allow_download_models_box.generic.callback = DownloadCallback;
	s_allow_download_models_box.itemnames = yes_no_names;
	s_allow_download_models_box.curvalue = (Cvar_VariableValue("allow_download_models") != 0);

	s_allow_download_sounds_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_sounds_box.generic.x = 0;
	s_allow_download_sounds_box.generic.y = y += 10;
	s_allow_download_sounds_box.generic.name = "Sounds";
	s_allow_download_sounds_box.generic.callback = DownloadCallback;
	s_allow_download_sounds_box.itemnames = yes_no_names;
	s_allow_download_sounds_box.curvalue = (Cvar_VariableValue("allow_download_sounds") != 0);

	Menu_AddItem(&s_downloadoptions_menu, &s_download_title);
	Menu_AddItem(&s_downloadoptions_menu, &s_allow_download_box);
	Menu_AddItem(&s_downloadoptions_menu, &s_allow_download_maps_box);
	Menu_AddItem(&s_downloadoptions_menu, &s_allow_download_players_box);
	Menu_AddItem(&s_downloadoptions_menu, &s_allow_download_models_box);
	Menu_AddItem(&s_downloadoptions_menu, &s_allow_download_sounds_box);

	Menu_Center(&s_downloadoptions_menu);

	/* skip over title */
	if (s_downloadoptions_menu.cursor == 0)
		s_downloadoptions_menu.cursor = 1;
}

void
DownloadOptions_MenuDraw(void)
{
	Menu_Draw(&s_downloadoptions_menu);
}

const char     *
DownloadOptions_MenuKey(int key)
{
	return Default_MenuKey(&s_downloadoptions_menu, key);
}

void
M_Menu_DownloadOptions_f(void)
{
	DownloadOptions_MenuInit();
	M_PushMenu(DownloadOptions_MenuDraw, DownloadOptions_MenuKey);
}
/*
 *
 * ============================================================================
 * =
 *
 * ADDRESS BOOK MENU
 *
 * =============================================================================
 */
#define NUM_ADDRESSBOOK_ENTRIES 9

static menuframework_s s_addressbook_menu;
static menufield_s s_addressbook_fields[NUM_ADDRESSBOOK_ENTRIES];

void
AddressBook_MenuInit(void)
{
	int		i;

	s_addressbook_menu.x = viddef.width / 2 - 142;
	s_addressbook_menu.y = viddef.height / 2 - 58;
	s_addressbook_menu.nitems = 0;

	for (i = 0; i < NUM_ADDRESSBOOK_ENTRIES; i++) {
		cvar_t         *adr;
		char		buffer    [20];

		Com_sprintf(buffer, sizeof(buffer), "adr%d", i);

		adr = Cvar_Get(buffer, "", CVAR_ARCHIVE);

		s_addressbook_fields[i].generic.type = MTYPE_FIELD;
		s_addressbook_fields[i].generic.name = 0;
		s_addressbook_fields[i].generic.callback = 0;
		s_addressbook_fields[i].generic.x = 0;
		s_addressbook_fields[i].generic.y = i * 18 + 0;
		s_addressbook_fields[i].generic.localdata[0] = i;
		s_addressbook_fields[i].cursor = 0;
		s_addressbook_fields[i].length = 60;
		s_addressbook_fields[i].visible_length = 30;

		Q_strncpyz(s_addressbook_fields[i].buffer, adr->string, sizeof(s_addressbook_fields[0].buffer));

		Menu_AddItem(&s_addressbook_menu, &s_addressbook_fields[i]);
	}
}

const char     *
AddressBook_MenuKey(int key)
{
	if (key == K_ESCAPE) {
		int		index;
		char		buffer    [20];

		for (index = 0; index < NUM_ADDRESSBOOK_ENTRIES; index++) {
			Com_sprintf(buffer, sizeof(buffer), "adr%d", index);
			Cvar_Set(buffer, s_addressbook_fields[index].buffer);
		}
	}
	return Default_MenuKey(&s_addressbook_menu, key);
}

void
AddressBook_MenuDraw(void)
{
	M_Banner("m_banner_addressbook");
	Menu_Draw(&s_addressbook_menu);
}

void
M_Menu_AddressBook_f(void)
{
	AddressBook_MenuInit();
	M_PushMenu(AddressBook_MenuDraw, AddressBook_MenuKey);
}

/*
 * =======================================================================
 *
 * PLAYER CONFIG MENU
 *
 * =======================================================================
 */

#define	MAX_DISPLAYNAME		16
#define	MAX_PLAYERMODELS	1024

typedef struct {
	int		nskins;
	char          **skindisplaynames;
	char		displayname[MAX_DISPLAYNAME];
	char		directory[MAX_QPATH];
} playermodelinfo_s;

static menuframework_s	s_player_config_menu;
static menufield_s	s_player_name_field;
static menulist_s	s_player_model_box;
static menulist_s	s_player_skin_box;
static menulist_s	s_player_handedness_box;
static menulist_s	s_player_rate_box;
static menuseparator_s	s_player_skin_title;
static menuseparator_s	s_player_model_title;
static menuseparator_s	s_player_hand_title;
static menuseparator_s	s_player_rate_title;
static menuaction_s	s_player_download_action;

static int	s_numplayermodels;
static char    *s_pmnames[MAX_PLAYERMODELS];
static playermodelinfo_s s_pmi[MAX_PLAYERMODELS];

static int	rate_tbl[] = {
	2500, 3200, 5000, 10000, 25000, 0
};

static const char *rate_names[] = {
	"28.8 Modem",
	"33.6 Modem",
	"Single ISDN",
	"Dual ISDN/Cable",
	"T1/LAN",
	"User defined",
	NULL
};

void
DownloadOptionsFunc(void *self)
{
	M_Menu_DownloadOptions_f();
}

static void
HandednessCallback(void *unused)
{
	Cvar_SetValue("hand", s_player_handedness_box.curvalue);
}

static void
RateCallback(void *unused)
{
	if (s_player_rate_box.curvalue != sizeof(rate_tbl) / sizeof(*rate_tbl) - 1)
		Cvar_SetValue("rate", rate_tbl[s_player_rate_box.curvalue]);
}

static void
ModelCallback(void *unused)
{
	s_player_skin_box.itemnames = (const char **)s_pmi[s_player_model_box.curvalue].skindisplaynames;
	s_player_skin_box.curvalue = 0;
}

static qboolean
IconOfSkinExists(char *skin, char **pcxfiles, int npcxfiles)
{
	int		i;
	char           *ext;
	char		scratch[1024];

	Q_strncpyz(scratch, skin, sizeof(scratch));

	if ((ext = strrchr(scratch, '.')) != NULL)
		*ext = '\0';

	strncat(scratch, "_i.pcx", sizeof(scratch) - strlen(scratch) - 1);

	for (i = 0; i < npcxfiles; i++)
		if (strcmp(pcxfiles[i], scratch) == 0)
			return (true);

	return (false);
}

static qboolean
PlayerConfig_ScanDirectories(void)
{
	char		path[MAX_OSPATH];
	char           *ptr;
	char          **dirnames;
	char          **pcxnames;
	char          **skinnames;
	int		i, j, k;
	int		ndirs, npms;
	int		npcxfiles;
	int		nskins;

	s_numplayermodels = 0;
	ndirs = npms = 0;

	/* Get a list of directories. */
	dirnames = FS_ListFiles2("players/*", &ndirs, SFF_SUBDIR, 0);
	ndirs--;

	if (dirnames == NULL)
		return (false);

	/* Go through the subdirectories. */
	npms = ndirs;
	if (npms > MAX_PLAYERMODELS)
		npms = MAX_PLAYERMODELS;

	for (i = 0; i < npms; i++) {
		if (*dirnames[i] == '\0')
			continue;

		/* Verify the existence of tris.md2. */
		if (!FS_FileExists(va("%s/tris.md2", dirnames[i]))) {
			*dirnames[i] = '\0';
			continue;
		}

		/* Verify the existence of at least one pcx skin. */
		pcxnames = FS_ListFiles2(va("%s/*.pcx", dirnames[i]),
		    &npcxfiles, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM);

		if (pcxnames == NULL) {
			*dirnames[i] = '\0';
			continue;
		}

		/*
		 * Count valid skins, which consist of a skin with a matching
		 * "_i" icon.
		 */
		for (j = 0, nskins = 0; j < npcxfiles - 1; j++)
			if (strstr(pcxnames[j], "_i.pcx") == 0 &&
			    IconOfSkinExists(pcxnames[j],
			    pcxnames, npcxfiles - 1))
				nskins++;

		if (nskins == 0)
			continue;

		skinnames = malloc(sizeof(char *) * (nskins + 1));
		memset(skinnames, 0, sizeof(char *) * (nskins + 1));

		/* Copy the valid skins. */
		for (k = 0, j = 0; j < npcxfiles - 1; j++) {
			if (strstr(pcxnames[j], "_i.pcx") == 0 &&
			    IconOfSkinExists(pcxnames[j], pcxnames,
			    npcxfiles - 1)) {
				COM_FileBase(pcxnames[j], path);
				skinnames[k++] = strdup(path);
			}
		}

		/* At this point we have a valid player model. */
		s_pmi[s_numplayermodels].nskins = nskins;
		s_pmi[s_numplayermodels].skindisplaynames = skinnames;

		/* Make short name for the model. */
		ptr = strrchr(dirnames[i], '/');
		Q_strncpyz(s_pmi[s_numplayermodels].displayname, ptr + 1, MAX_DISPLAYNAME);
		Q_strncpyz(s_pmi[s_numplayermodels].directory, ptr + 1, MAX_QPATH);
		s_numplayermodels++;

		FS_FreeList(pcxnames, npcxfiles);
	}

	FS_FreeList(dirnames, ndirs + 1);

	return (true);
}

static int
pmicmpfnc(const void *_a, const void *_b)
{
	const playermodelinfo_s *a = (const playermodelinfo_s *)_a;
	const playermodelinfo_s *b = (const playermodelinfo_s *)_b;

	/*
	 * * sort by male, female, then alphabetical
	 */
	if (strcmp(a->directory, "male") == 0)
		return -1;
	else if (strcmp(b->directory, "male") == 0)
		return 1;

	if (strcmp(a->directory, "female") == 0)
		return -1;
	else if (strcmp(b->directory, "female") == 0)
		return 1;

	return strcmp(a->directory, b->directory);
}

qboolean
PlayerConfig_MenuInit(void)
{
	extern cvar_t  *name;
	extern cvar_t  *skin;
	char		currentdirectory[1024];
	char		currentskin[1024];
	char           *ptr;
	int		i = 0;

	int		currentdirectoryindex = 0;
	int		currentskinindex = 0;

	cvar_t         *hand = Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);

	static const char *handedness[] = {"right", "left", "center", 0};

	PlayerConfig_ScanDirectories();

	if (s_numplayermodels == 0)
		return false;

	if (hand->value < 0 || hand->value > 2)
		Cvar_SetValue("hand", 0);

	Q_strncpyz(currentdirectory, skin->string, sizeof(currentdirectory));

	if ((ptr = strchr(currentdirectory, '/')) != NULL) {
		Q_strncpyz(currentskin, ptr + 1, sizeof(currentskin));
		*ptr = '\0';
	} else {
		Q_strncpyz(currentdirectory, "male", sizeof(currentdirectory));
		Q_strncpyz(currentskin, "grunt", sizeof(currentskin));
	}

	qsort(s_pmi, s_numplayermodels, sizeof(s_pmi[0]), pmicmpfnc);

	memset(s_pmnames, 0, sizeof(s_pmnames));
	for (i = 0; i < s_numplayermodels; i++) {
		s_pmnames[i] = s_pmi[i].displayname;
		if (Q_stricmp(s_pmi[i].directory, currentdirectory) == 0) {
			int		j;

			currentdirectoryindex = i;

			for (j = 0; j < s_pmi[i].nskins; j++) {
				if (Q_stricmp(s_pmi[i].skindisplaynames[j], currentskin) == 0) {
					currentskinindex = j;
					break;
				}
			}
		}
	}

	s_player_config_menu.x = viddef.width / 2 - 95;
	s_player_config_menu.y = viddef.height / 2 - 97;
	s_player_config_menu.nitems = 0;

	s_player_name_field.generic.type = MTYPE_FIELD;
	s_player_name_field.generic.name = "Name";
	s_player_name_field.generic.callback = 0;
	s_player_name_field.generic.x = 0;
	s_player_name_field.generic.y = 0;
	s_player_name_field.length = 20;
	s_player_name_field.visible_length = 20;
	Q_strncpyz(s_player_name_field.buffer, name->string, sizeof(s_player_name_field.buffer));
	s_player_name_field.cursor = strlen(name->string);

	s_player_model_title.generic.type = MTYPE_SEPARATOR;
	s_player_model_title.generic.name = "Model";
	s_player_model_title.generic.x = -8;
	s_player_model_title.generic.y = 60;

	s_player_model_box.generic.type = MTYPE_SPINCONTROL;
	s_player_model_box.generic.x = -56;
	s_player_model_box.generic.y = 70;
	s_player_model_box.generic.callback = ModelCallback;
	s_player_model_box.generic.cursor_offset = -48;
	s_player_model_box.curvalue = currentdirectoryindex;
	s_player_model_box.itemnames = (const char **)s_pmnames;

	s_player_skin_title.generic.type = MTYPE_SEPARATOR;
	s_player_skin_title.generic.name = "Skin";
	s_player_skin_title.generic.x = -16;
	s_player_skin_title.generic.y = 84;

	s_player_skin_box.generic.type = MTYPE_SPINCONTROL;
	s_player_skin_box.generic.x = -56;
	s_player_skin_box.generic.y = 94;
	s_player_skin_box.generic.name = 0;
	s_player_skin_box.generic.callback = 0;
	s_player_skin_box.generic.cursor_offset = -48;
	s_player_skin_box.curvalue = currentskinindex;
	s_player_skin_box.itemnames = (const char **)s_pmi[currentdirectoryindex].skindisplaynames;

	s_player_hand_title.generic.type = MTYPE_SEPARATOR;
	s_player_hand_title.generic.name = "Handedness";
	s_player_hand_title.generic.x = 32;
	s_player_hand_title.generic.y = 108;

	s_player_handedness_box.generic.type = MTYPE_SPINCONTROL;
	s_player_handedness_box.generic.x = -56;
	s_player_handedness_box.generic.y = 118;
	s_player_handedness_box.generic.name = 0;
	s_player_handedness_box.generic.cursor_offset = -48;
	s_player_handedness_box.generic.callback = HandednessCallback;
	s_player_handedness_box.curvalue = Cvar_VariableValue("hand");
	s_player_handedness_box.itemnames = handedness;

	for (i = 0; i < sizeof(rate_tbl) / sizeof(*rate_tbl) - 1; i++)
		if (Cvar_VariableValue("rate") == rate_tbl[i])
			break;

	s_player_rate_title.generic.type = MTYPE_SEPARATOR;
	s_player_rate_title.generic.name = "Connect speed";
	s_player_rate_title.generic.x = 56;
	s_player_rate_title.generic.y = 156;

	s_player_rate_box.generic.type = MTYPE_SPINCONTROL;
	s_player_rate_box.generic.x = -56;
	s_player_rate_box.generic.y = 166;
	s_player_rate_box.generic.name = 0;
	s_player_rate_box.generic.cursor_offset = -48;
	s_player_rate_box.generic.callback = RateCallback;
	s_player_rate_box.curvalue = i;
	s_player_rate_box.itemnames = rate_names;

	s_player_download_action.generic.type = MTYPE_ACTION;
	s_player_download_action.generic.name = "Download options";
	s_player_download_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_player_download_action.generic.x = -24;
	s_player_download_action.generic.y = 186;
	s_player_download_action.generic.statusbar = NULL;
	s_player_download_action.generic.callback = DownloadOptionsFunc;

	Menu_AddItem(&s_player_config_menu, &s_player_name_field);
	Menu_AddItem(&s_player_config_menu, &s_player_model_title);
	Menu_AddItem(&s_player_config_menu, &s_player_model_box);
	if (s_player_skin_box.itemnames) {
		Menu_AddItem(&s_player_config_menu, &s_player_skin_title);
		Menu_AddItem(&s_player_config_menu, &s_player_skin_box);
	}
	Menu_AddItem(&s_player_config_menu, &s_player_hand_title);
	Menu_AddItem(&s_player_config_menu, &s_player_handedness_box);
	Menu_AddItem(&s_player_config_menu, &s_player_rate_title);
	Menu_AddItem(&s_player_config_menu, &s_player_rate_box);
	Menu_AddItem(&s_player_config_menu, &s_player_download_action);

	return true;
}

void
PlayerConfig_MenuDraw( void )
{
	extern float	CalcFov(float fov_x, float w, float h );
	refdef_t	refdef;
	char		scratch[MAX_QPATH];
	extern cvar_t	*hand;

	memset(&refdef, 0, sizeof(refdef));

	refdef.x = viddef.width / 2;
	refdef.y = viddef.height / 2 - 72;
	refdef.width = 144;
	refdef.height = 168;
	refdef.fov_x = 34;
	refdef.fov_y = CalcFov(refdef.fov_x, refdef.width, refdef.height);
	refdef.time = cls.realtime * 0.001;

	if (s_pmi[s_player_model_box.curvalue].skindisplaynames)
	{
		static int	 yaw;
		vec3_t 		modelOrg;
		entity_t 	entity[2], *ent;

		refdef.num_entities = 0;
		refdef.entities = &entity[0];

		yaw = anglemod(cl.time/10);

		VectorSet(modelOrg, 150, (hand->value == 1)? 35 : -35, 0);

		/* Set Up Player Model */
		ent = &entity[0];
		{
			memset(&entity[0], 0, sizeof(entity[0]));

			Com_sprintf(scratch, sizeof(scratch), "players/%s/tris.md2",
				s_pmi[s_player_model_box.curvalue].directory );

			ent->model = re.RegisterModel(scratch);

			Com_sprintf(scratch, sizeof(scratch), "players/%s/%s.pcx",
				s_pmi[s_player_model_box.curvalue].directory,
				s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue]);

			ent->skin = re.RegisterSkin(scratch);

			ent->flags = RF_FULLBRIGHT|RF_DEPTHHACK;
			if (hand->value == 1)
				ent->flags |= RF_MIRRORMODEL;

			ent->origin[0] = 80;
			ent->origin[1] = 0;
			ent->origin[2] = 0;

			VectorCopy(ent->origin, ent->oldorigin);

			ent->frame = 0;
			ent->oldframe = 0;
			ent->backlerp = 0.0;
			ent->angles[1] = yaw++;

			if (++yaw > 360)
				yaw -= 360;

			ent->scale = 1;

			if (hand->value == 1)
				ent->angles[1] = 360 - ent->angles[1];

			refdef.num_entities++;
		}
		ent = &entity[1];
		{
			memset(&entity[1], 0, sizeof(entity[1]));

			Com_sprintf(scratch, sizeof(scratch), "players/%s/weapon.md2",
				s_pmi[s_player_model_box.curvalue].directory);

			ent->model = re.RegisterModel(scratch);

			if (ent->model)
			{
				ent->skinnum = 0;

				ent->flags = RF_FULLBRIGHT|RF_DEPTHHACK;
				if (hand->value == 1)
					ent->flags |= RF_MIRRORMODEL;
				ent->origin[0] = 80;
				ent->origin[1] = 0;
				ent->origin[2] = 0;

				VectorCopy(ent->origin, ent->oldorigin);

				ent->frame = 0;
				ent->oldframe = 0;
				ent->backlerp = 0.0;
				ent->angles[1] = yaw++;

				if (++yaw > 360)
					yaw -= 360;

				ent->scale = 1;

				if (hand->value == 1)
					ent->angles[1] = 360 - ent->angles[1];

				refdef.num_entities++;
			}
		}

		refdef.areabits = 0;
		refdef.lightstyles = 0;
		refdef.rdflags = RDF_NOWORLDMODEL | RDF_NOCLEAR;

		Menu_Draw(&s_player_config_menu);
		
		/*
		 * M_DrawTextBox((refdef.x) * (320.0 / viddef.width) - 8,
		 *		(viddef.height / 2) * (240.0 / viddef.height) - 77,
		 *		refdef.width / 8, refdef.height / 8);
		*/
		
		refdef.height += 4;

		re.RenderFrame(&refdef);

		Com_sprintf(scratch, sizeof(scratch), "/players/%s/%s_i.pcx",
			    s_pmi[s_player_model_box.curvalue].directory,
			    s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue]);

		re.DrawPic(s_player_config_menu.x - 40, refdef.y, scratch, 1.0);
	}
}

const char *
PlayerConfig_MenuKey(int key)
{
	int		i;

	if (key == K_ESCAPE) {
		char		scratch   [1024];

		Cvar_Set("name", s_player_name_field.buffer);

		Com_sprintf(scratch, sizeof(scratch), "%s/%s",
			    s_pmi[s_player_model_box.curvalue].directory,
			    s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue]);

		Cvar_Set("skin", scratch);

		for (i = 0; i < s_numplayermodels; i++) {
			int		j;

			for (j = 0; j < s_pmi[i].nskins; j++) {
				if (s_pmi[i].skindisplaynames[j])
					free(s_pmi[i].skindisplaynames[j]);
				s_pmi[i].skindisplaynames[j] = 0;
			}
			free(s_pmi[i].skindisplaynames);
			s_pmi[i].skindisplaynames = 0;
			s_pmi[i].nskins = 0;
		}
	}
	return Default_MenuKey(&s_player_config_menu, key);
}

void
M_Menu_PlayerConfig_f(void)
{
	if (!PlayerConfig_MenuInit()) {
		Menu_SetStatusBar(&s_multiplayer_menu, "No valid player models found");
		return;
	}
	Menu_SetStatusBar(&s_multiplayer_menu, NULL);
	M_PushMenu(PlayerConfig_MenuDraw, PlayerConfig_MenuKey);
}

/*
 * =======================================================================
 *
 * QUIT MENU
 *
 * =======================================================================
 */

const char     *
M_Quit_Key(int key)
{
	switch (key) {
		case K_ESCAPE:
		case 'n':
		case 'N':
		M_PopMenu();
		break;

	case 'Y':
	case 'y':
		cls.key_dest = key_console;
		CL_Quit_f();
		break;

	default:
		break;
	}

	return NULL;

}


void
M_Quit_Draw(void)
{
	int		w         , h;

	re.DrawGetPicSize(&w, &h, "quit");
	re.DrawPic((viddef.width - w) / 2, (viddef.height - h) / 2, "quit", 1.0);
}

void
CheckQuitMenuMouse(void)
{
	int		maxs       [2], mins[2];
	int		w         , h;

	re.DrawGetPicSize(&w, &h, "quit");

	mins[0] = (viddef.width - w) / 2;
	maxs[0] = mins[0] + w;
	mins[1] = (viddef.height - h) / 2;
	maxs[1] = mins[1] + h;

	if (cursor.x >= mins[0] && cursor.x <= maxs[0] &&
	    cursor.y >= mins[1] && cursor.y <= maxs[1]) {
		if (!cursor.buttonused[MOUSEBUTTON2] && cursor.buttonclicks[MOUSEBUTTON2] == 2) {
			M_PopMenu();
			cursor.buttonused[MOUSEBUTTON2] = true;
			cursor.buttonclicks[MOUSEBUTTON2] = 0;
		}
		if (!cursor.buttonused[MOUSEBUTTON1] && cursor.buttonclicks[MOUSEBUTTON1] == 2) {
			CL_Quit_f();
			cursor.buttonused[MOUSEBUTTON1] = true;
			cursor.buttonclicks[MOUSEBUTTON1] = 0;
		}
	}
}

void
M_Menu_Quit_f(void)
{
	M_PushMenu(M_Quit_Draw, M_Quit_Key);
}

/* Menu Subsystem */


/*
 * ================= M_Init =================
 */
void
M_Init(void)
{
	Cmd_AddCommand("menu_main", M_Menu_Main_f);
	Cmd_AddCommand("menu_game", M_Menu_Game_f);
	Cmd_AddCommand("menu_loadgame", M_Menu_LoadGame_f);
	Cmd_AddCommand("menu_savegame", M_Menu_SaveGame_f);
	Cmd_AddCommand("menu_joinserver", M_Menu_JoinServer_f);
	Cmd_AddCommand("menu_addressbook", M_Menu_AddressBook_f);
	Cmd_AddCommand("menu_startserver", M_Menu_StartServer_f);
	Cmd_AddCommand("menu_dmoptions", M_Menu_DMOptions_f);
	Cmd_AddCommand("menu_playerconfig", M_Menu_PlayerConfig_f);
	Cmd_AddCommand("menu_downloadoptions", M_Menu_DownloadOptions_f);
	Cmd_AddCommand("menu_credits", M_Menu_Credits_f);
	Cmd_AddCommand("menu_multiplayer", M_Menu_Multiplayer_f);
	Cmd_AddCommand("menu_video", M_Menu_Video_f);
	Cmd_AddCommand("menu_options", M_Menu_Options_f);
	Cmd_AddCommand("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand("menu_quit", M_Menu_Quit_f);
}

/*
 * ================================= Menu Mouse Cursor - psychospaz
 * =================================
 */

void
refreshCursorMenu(void)
{
	cursor.menu = NULL;
}

void
refreshCursorLink(void)
{
	cursor.menuitem = NULL;
}

int
Slider_CursorPositionX(menuslider_s * s)
{
	float		range;

	range = (s->curvalue - s->minvalue) / (float)(s->maxvalue - s->minvalue);

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;

	return (int)(8 + RCOLUMN_OFFSET + (SLIDER_RANGE) * 8 * range);
}

int
newSliderValueForX(int x, menuslider_s * s)
{
	float		newValue;
	int		newValueInt;
	int		pos = x - (8 + RCOLUMN_OFFSET + s->generic.parent->x + s->generic.x);

	newValue = ((float)pos) / ((SLIDER_RANGE - 1) * 8.0);
	newValueInt = s->minvalue + newValue * (float)(s->maxvalue - s->minvalue);

	return newValueInt;
}

void
Slider_CheckSlide(menuslider_s * s)
{
	if (s->curvalue > s->maxvalue)
		s->curvalue = s->maxvalue;
	else if (s->curvalue < s->minvalue)
		s->curvalue = s->minvalue;

	if (s->generic.callback)
		s->generic.callback(s);
}

void
Menu_DragSlideItem(menuframework_s * menu, void *menuitem)
{
	menuslider_s   *slider = (menuslider_s *) menuitem;

	slider->curvalue = newSliderValueForX(cursor.x, slider);
	Slider_CheckSlide(slider);
}

void
Menu_ClickSlideItem(menuframework_s * menu, void *menuitem)
{
	int		min       , max;
	menucommon_s   *item = (menucommon_s *) menuitem;
	menuslider_s   *slider = (menuslider_s *) menuitem;

	min = menu->x + (item->x + Slider_CursorPositionX(slider) - 4);
	max = menu->x + (item->x + Slider_CursorPositionX(slider) + 4);

	if (cursor.x < min)
		Menu_SlideItem(menu, -1);
	if (cursor.x > max)
		Menu_SlideItem(menu, 1);
}

void
M_Think_MouseCursor(void)
{
	char           *sound = NULL;
	menuframework_s *m = (menuframework_s *) cursor.menu;

	if (m_drawfunc == M_Main_Draw) {	/* have to hack for main menu
						 * :p */
		CheckMainMenuMouse();
		return;
	}
	if (m_drawfunc == M_Quit_Draw) {	/* have to hack for main menu
						 * :p */
		CheckQuitMenuMouse();
		return;
	}
	if (m_drawfunc == M_Credits_MenuDraw) {	/* have to hack for credits
						 * :p */
		if (cursor.buttonclicks[MOUSEBUTTON2]) {
			cursor.buttonused[MOUSEBUTTON2] = true;
			cursor.buttonclicks[MOUSEBUTTON2] = 0;
			cursor.buttonused[MOUSEBUTTON1] = true;
			cursor.buttonclicks[MOUSEBUTTON1] = 0;
			S_StartLocalSound(menu_out_sound);
			if (creditsBuffer)
				FS_FreeFile(creditsBuffer);
			M_PopMenu();
			return;
		}
	}
	if (!m)
		return;

	/* Exit with double click 2nd mouse button */

	if (cursor.menuitem) {
		/* MOUSE1 */
		if (cursor.buttondown[MOUSEBUTTON1]) {
			if (cursor.menuitemtype == MENUITEM_SLIDER) {
				Menu_DragSlideItem(m, cursor.menuitem);
			} else if (!cursor.buttonused[MOUSEBUTTON1] && cursor.buttonclicks[MOUSEBUTTON1]) {
				if (cursor.menuitemtype == MENUITEM_ROTATE) {
					if (cl_mouse_rotate->value)
						Menu_SlideItem(m, -1);
					else
						Menu_SlideItem(m, 1);
					sound = menu_move_sound;
					cursor.buttonused[MOUSEBUTTON1] = true;
				} else {
					cursor.buttonused[MOUSEBUTTON1] = true;
					Menu_MouseSelectItem(cursor.menuitem);
					sound = menu_move_sound;
				}
			}
		}
		/* MOUSE2 */
		if (cursor.buttondown[MOUSEBUTTON2] && cursor.buttonclicks[MOUSEBUTTON2]) {
			if (cursor.menuitemtype == MENUITEM_SLIDER && !cursor.buttonused[MOUSEBUTTON2]) {
				Menu_ClickSlideItem(m, cursor.menuitem);
				sound = menu_move_sound;
				cursor.buttonused[MOUSEBUTTON2] = true;
			} else if (!cursor.buttonused[MOUSEBUTTON2]) {
				if (cursor.menuitemtype == MENUITEM_ROTATE) {
					if (cl_mouse_rotate->value)
						Menu_SlideItem(m, 1);
					else
						Menu_SlideItem(m, -1);
					sound = menu_move_sound;
					cursor.buttonused[MOUSEBUTTON2] = true;
				}
			}
		}
		/* MOUSEWHEELS */
		if (cursor.mousewheeldown) {
			if (cursor.menuitemtype == MENUITEM_SLIDER) {
				Menu_SlideItem(m, -1);
				sound = NULL;
			} else if (cursor.menuitemtype == MENUITEM_ROTATE) {
				Menu_SlideItem(m, -1);
				sound = menu_move_sound;
			}
		}
		if (cursor.mousewheelup) {
			if (cursor.menuitemtype == MENUITEM_SLIDER) {
				Menu_SlideItem(m, 1);
				sound = NULL;
			} else if (cursor.menuitemtype == MENUITEM_ROTATE) {
				Menu_SlideItem(m, 1);
				sound = menu_move_sound;
			}
		}
	} else if (!cursor.buttonused[MOUSEBUTTON2] && cursor.buttonclicks[MOUSEBUTTON2] == 2 && cursor.buttondown[MOUSEBUTTON2]) {
		if (m_drawfunc == Options_MenuDraw) {
			Cvar_SetValue("options_menu", 0);
			refreshCursorLink();
			M_PopMenu();
		} else
			M_PopMenu();

		sound = menu_out_sound;
		cursor.buttonused[MOUSEBUTTON2] = true;
		cursor.buttonclicks[MOUSEBUTTON2] = 0;
		cursor.buttonused[MOUSEBUTTON1] = true;
		cursor.buttonclicks[MOUSEBUTTON1] = 0;
	}
	cursor.mousewheeldown = 0;
	cursor.mousewheelup = 0;


	if (sound)
		S_StartLocalSound(sound);
}

void
M_Draw_Cursor(void)
{
	int	w, h;
	char *cur_img = NULL;

	cur_img = "/gfx/mouse_cursor.pcx";
	
	/* get sizing vars */
	re.DrawGetPicSize(&w, &h, cur_img);
	re.DrawScaledPic(cursor.x - w * 0.5, cursor.y - h * 0.5, 
	                  cl_mouse_scale->value, cl_mouse_alpha->value, cur_img, 
			  cl_mouse_red->value, cl_mouse_green->value, cl_mouse_blue->value, 
			  true, false);
}

void 
M_Draw_Cursor_Opt (void)
{
	float alpha = 1;
	int w,h;
	char *overlay = NULL;
	char *cur_img = NULL;

	if (m_drawfunc == M_Main_Draw)
	{
		if (MainMenuMouseHover)
		{
			if ((cursor.buttonused[0] && cursor.buttonclicks[0])
				|| (cursor.buttonused[1] && cursor.buttonclicks[1]))
			{
				cur_img = "/gfx/m_cur_click.pcx";
				alpha = 0.85 + 0.15*sin(anglemod(cl.time*0.005));
			}
			else
			{
				cur_img = "/gfx/m_cur_hover.pcx";
				alpha = 0.85 + 0.15*sin(anglemod(cl.time*0.005));
			}
		}
		else
			cur_img = "/gfx/m_cur_main.pcx";
			overlay = "/gfx/m_cur_over.pcx";
	}
	else
	{
		if (cursor.menuitem)
		{
			if (cursor.menuitemtype == MENUITEM_TEXT)
			{
				cur_img = "/gfx/m_cur_text.pcx";
			}
			else
			{
				if ((cursor.buttonused[0] && cursor.buttonclicks[0])
					|| (cursor.buttonused[1] && cursor.buttonclicks[1]))
				{
					cur_img = "/gfx/m_cur_click.pcx";
					alpha = 0.85 + 0.15*sin(anglemod(cl.time*0.005));
				}
				else
				{
					cur_img = "/gfx/m_cur_hover.pcx";
					alpha = 0.85 + 0.15*sin(anglemod(cl.time*0.005));
				}
				overlay = "/gfx/m_cur_over.pcx";
			}
		}
		else
		{
			cur_img = "/gfx/m_cur_main.pcx";
			overlay = "/gfx/m_cur_over.pcx";
		}
	}
	
	if (cur_img)
	{
		re.DrawGetPicSize( &w, &h, cur_img );
		re.DrawScaledPic(cursor.x - w * 0.5, cursor.y - h * 0.5, 
	                          cl_mouse_scale->value, cl_mouse_alpha->value, cur_img, 
			          cl_mouse_red->value, cl_mouse_green->value, 
				  cl_mouse_blue->value, true, false);

		if (overlay)
		{
			re.DrawGetPicSize( &w, &h, "/gfx/m_cur_over.pcx" );
			re.DrawScaledPic(cursor.x - w * 0.5, cursor.y - h * 0.5, 
	                                  cl_mouse_scale->value, 1, overlay, 
			                  cl_mouse_red->value, cl_mouse_green->value, 
					  cl_mouse_blue->value, true, false);
		}
	}
}

/*
 * ===========
 * M_Draw
 * ===========
 */
void
M_Draw(void)
{
	char           *conback_image;


	conback_image = cl_conback_image->string;

	if (cls.key_dest != key_menu)
		return;

	/* repaint everything next frame */
	SCR_DirtyScreen();

	/* dim everything behind it down */
	if (cl.cinematictime > 0 || cls.state == ca_disconnected)
		re.DrawFadeScreen();
	else if (re.RegisterPic("conback"))
		re.DrawStretchPic(0, 0, viddef.width, viddef.height, conback_image, cl_menu_alpha->value);
	else
		re.DrawFadeScreen();

	/* Knigthmare- added Psychospaz's mouse support */
	refreshCursorMenu();

	m_drawfunc();

	/* delay playing the enter sound until after the */
	/* menu has been drawn, to avoid delay while */
	/* caching images */
	if (m_entersound) {
		S_StartLocalSound(menu_in_sound);
		m_entersound = false;
	}
	/* Knigthmare- added Psychospaz's mouse support */
	/* menu cursor for mouse usage :) */
	if (cl_mouse_cursor->value >= 1)
		M_Draw_Cursor_Opt();
	else
		M_Draw_Cursor();
}

/*
 * ===========
 * M_Keydown
 * ===========
 */
void
M_Keydown(int key)
{
	const char     *s;

	if (m_keyfunc)
		if ((s = m_keyfunc(key)) != 0)
			S_StartLocalSound((char *)s);
}

/*
 * GetPosInList
 *
 * Get the position of "name" in "list", where "list" is a string pointer. The
 * final element (string) is expected to be NULL.
 */
int
GetPosInList(const char **list, const char *name)
{
	int		i;

	for (i = 0; list[i] != NULL; i++)
		if (strcmp(list[i], name) == 0)
			return (i);

	return (-1);
}
