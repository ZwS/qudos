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
#include <string.h>

#include "client.h"
#include "qmenu.h"

static void	Action_DoEnter(menuaction_s * a);
static void	Action_Draw(menuaction_s * a);
static void	Menu_DrawStatusBar(const char *string);

/* static void	 Menulist_DoEnter( menulist_s *l ); */
static void	MenuList_Draw(menulist_s * l);
static void	Separator_Draw(menuseparator_s * s);
static void	Slider_DoSlide(menuslider_s * s, int dir);
static void	Slider_Draw(menuslider_s * s);

/* static void	 SpinControl_DoEnter( menulist_s *s ); */
static void	SpinControl_Draw(menulist_s * s);
static void	SpinControl_DoSlide(menulist_s * s, int dir);

extern refexport_t re;
extern viddef_t	viddef;

#define VID_WIDTH viddef.width
#define VID_HEIGHT viddef.height

#define Draw_Char re.DrawChar

#define Draw_Fill re.DrawFill

int mouseOverAlpha( menucommon_s *m )
{
	if (cursor.menuitem == m)
	{
		int alpha;

		alpha = 125 + 25 * cos(anglemod(cl.time*0.005));

		if (alpha>255) alpha = 255;
		if (alpha<0) alpha = 0;

		return alpha;
	}
	else 
		return 255;
}


void
Action_DoEnter(menuaction_s * a)
{
	if (a->generic.callback)
		a->generic.callback(a);
}

void
Action_Draw(menuaction_s * a)
{
	int alpha = mouseOverAlpha(&a->generic);
	
	if (a->generic.flags & QMF_LEFT_JUSTIFY) {
		if (a->generic.flags & QMF_GRAYED)
			Menu_DrawStringDark(a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name, alpha );
		else
			Menu_DrawString(a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name, alpha );
	} else {
		if (a->generic.flags & QMF_GRAYED)
			Menu_DrawStringR2LDark(a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name, alpha );
		else
			Menu_DrawStringR2L(a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name, alpha );
	}
	if (a->generic.ownerdraw)
		a->generic.ownerdraw(a);
}

qboolean
Field_DoEnter(menufield_s * f)
{
	if (f->generic.callback) {
		f->generic.callback(f);
		return true;
	}
	return false;
}

void
Field_Draw(menufield_s * f)
{
	int		i;
	char		tempbuffer[128] = "";
	int 		alpha = mouseOverAlpha(&f->generic);

	if (f->generic.name)
		Menu_DrawStringR2LDark(f->generic.x + f->generic.parent->x + LCOLUMN_OFFSET, f->generic.y + f->generic.parent->y, f->generic.name, 255);

	Q_strncpyz(tempbuffer, f->buffer + f->visible_offset, f->visible_length);

	Draw_Char(f->generic.x + f->generic.parent->x + 16, f->generic.y + f->generic.parent->y - 4, 18, 255);
	Draw_Char(f->generic.x + f->generic.parent->x + 16, f->generic.y + f->generic.parent->y + 4, 24, 255);

	Draw_Char(f->generic.x + f->generic.parent->x + 24 + f->visible_length * 8, f->generic.y + f->generic.parent->y - 4, 20, 255);
	Draw_Char(f->generic.x + f->generic.parent->x + 24 + f->visible_length * 8, f->generic.y + f->generic.parent->y + 4, 26, 255);

	for (i = 0; i < f->visible_length; i++) {
		Draw_Char(f->generic.x + f->generic.parent->x + 24 + i * 8, f->generic.y + f->generic.parent->y - 4, 19, 255);
		Draw_Char(f->generic.x + f->generic.parent->x + 24 + i * 8, f->generic.y + f->generic.parent->y + 4, 25, 255);
	}

	Menu_DrawString(f->generic.x + f->generic.parent->x + 24, f->generic.y + f->generic.parent->y, tempbuffer, alpha);

	if (Menu_ItemAtCursor(f->generic.parent) == f) {
		int		offset;

		if (f->visible_offset)
			offset = f->visible_length;
		else
			offset = f->cursor;

		if (((int)(Sys_Milliseconds() / 250)) & 1) {
			Draw_Char(f->generic.x + f->generic.parent->x + (offset + 2) * 8 + 8,
			    f->generic.y + f->generic.parent->y,
			    11, 255);
		} else {
			Draw_Char(f->generic.x + f->generic.parent->x + (offset + 2) * 8 + 8,
			    f->generic.y + f->generic.parent->y,
			    ' ', 255);
		}
	}
}

qboolean
Field_Key(menufield_s * f, int key)
{
	extern int	keydown[];

	switch (key) {
	case K_KP_SLASH:
		key = '/';
		break;
	case K_KP_MINUS:
		key = '-';
		break;
	case K_KP_PLUS:
		key = '+';
		break;
	case K_KP_HOME:
		key = '7';
		break;
	case K_KP_UPARROW:
		key = '8';
		break;
	case K_KP_PGUP:
		key = '9';
		break;
	case K_KP_LEFTARROW:
		key = '4';
		break;
	case K_KP_5:
		key = '5';
		break;
	case K_KP_RIGHTARROW:
		key = '6';
		break;
	case K_KP_END:
		key = '1';
		break;
	case K_KP_DOWNARROW:
		key = '2';
		break;
	case K_KP_PGDN:
		key = '3';
		break;
	case K_KP_INS:
		key = '0';
		break;
	case K_KP_DEL:
		key = '.';
		break;
	}

	if (key > 127) {
		switch (key) {
		case K_DEL:
		default:
			return false;
		}
	}

	/*
	 * * support pasting from the clipboard
	 */
	if ((toupper(key) == 'V' && keydown[K_CTRL]) ||
	    (((key == K_INS) || (key == K_KP_INS)) && keydown[K_SHIFT])) {
		char           *cbd;

		if ((cbd = Sys_GetClipboardData()) != 0) {
			strtok(cbd, "\n\r\b");

			Q_strncpyz(f->buffer, cbd, f->length);
			f->cursor = strlen(f->buffer);
			f->visible_offset = f->cursor - f->visible_length;
			if (f->visible_offset < 0)
				f->visible_offset = 0;

			free(cbd);
		}
		return true;
	}
	switch (key) {
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
	case K_BACKSPACE:
		if (f->cursor > 0) {
			memmove(&f->buffer[f->cursor - 1], &f->buffer[f->cursor], strlen(&f->buffer[f->cursor]) + 1);
			f->cursor--;

			if (f->visible_offset) {
				f->visible_offset--;
			}
		}
		break;

	case K_KP_DEL:
	case K_DEL:
		memmove(&f->buffer[f->cursor], &f->buffer[f->cursor + 1], strlen(&f->buffer[f->cursor + 1]) + 1);
		break;

	case K_KP_ENTER:
	case K_ENTER:
	case K_ESCAPE:
	case K_TAB:
		return false;

	case K_SPACE:
	default:
		if (!isdigit(key) && (f->generic.flags & QMF_NUMBERSONLY))
			return false;

		if (f->cursor < f->length) {
			f->buffer[f->cursor++] = key;
			f->buffer[f->cursor] = 0;

			if (f->cursor > f->visible_length) {
				f->visible_offset++;
			}
		}
	}

	return true;
}

void
Menu_AddItem(menuframework_s * menu, void *item)
{
	if (menu->nitems == 0)
		menu->nslots = 0;

	if (menu->nitems < MAXMENUITEMS) {
		menu->items[menu->nitems] = item;
		((menucommon_s *) menu->items[menu->nitems])->parent = menu;
		menu->nitems++;
	}
	menu->nslots = Menu_TallySlots(menu);
}

/*
 * * Menu_AdjustCursor *
 *
 * This function takes the given menu, the direction, and attempts * to adjust
 * the menu's cursor so that it's at the next available * slot.
 */
void
Menu_AdjustCursor(menuframework_s * m, int dir)
{
	menucommon_s   *citem;

	/*
	 * * see if it's in a valid spot
	 */
	if (m->cursor >= 0 && m->cursor < m->nitems) {
		if ((citem = Menu_ItemAtCursor(m)) != 0) {
			if (citem->type != MTYPE_SEPARATOR)
				return;
		}
	}

	/*
	 * * it's not in a valid spot, so crawl in the direction indicated
	 * until we * find a valid spot
	 */
	if (dir == 1) {
		while (1) {
			citem = Menu_ItemAtCursor(m);
			if (citem)
				if (citem->type != MTYPE_SEPARATOR)
					break;
			m->cursor += dir;
			if (m->cursor >= m->nitems)
				m->cursor = 0;
		}
	} else {
		while (1) {
			citem = Menu_ItemAtCursor(m);
			if (citem)
				if (citem->type != MTYPE_SEPARATOR)
					break;
			m->cursor += dir;
			if (m->cursor < 0)
				m->cursor = m->nitems - 1;
		}
	}
}

void
Menu_Center(menuframework_s * menu)
{
	int		height;

	height = ((menucommon_s *) menu->items[menu->nitems - 1])->y;
	height += 10;

	menu->y = (VID_HEIGHT - height) / 2;
}

void
Menu_Draw(menuframework_s * menu)
{
	int		i;
	menucommon_s   *item;

	/*
	 * * draw contents
	 */
	for (i = 0; i < menu->nitems; i++) {
		switch (((menucommon_s *) menu->items[i])->type) {
		case MTYPE_FIELD:
			Field_Draw((menufield_s *) menu->items[i]);
			break;
		case MTYPE_SLIDER:
			Slider_Draw((menuslider_s *) menu->items[i]);
			break;
		case MTYPE_LIST:
			MenuList_Draw((menulist_s *) menu->items[i]);
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_Draw((menulist_s *) menu->items[i]);
			break;
		case MTYPE_ACTION:
			Action_Draw((menuaction_s *) menu->items[i]);
			break;
		case MTYPE_SEPARATOR:
			Separator_Draw((menuseparator_s *) menu->items[i]);
			break;
		}
	}

	/*
	 * * now check mouseovers - psychospaz
	 */
	cursor.menu = menu;

	if (cursor.mouseaction) {
		menucommon_s   *lastitem = cursor.menuitem;

		refreshCursorLink();

		for (i = menu->nitems; i >= 0; i--) {
			int		type;
			int		len;
			int		min        [2], max[2];

			item = ((menucommon_s *) menu->items[i]);

			if (!item || item->type == MTYPE_SEPARATOR)
				continue;

			max[0] = min[0] = menu->x + item->x + RCOLUMN_OFFSET;	/* + 2 chars for space +
										 * cursor */
			max[1] = min[1] = menu->y + item->y;
			max[1] += 8;

			switch (item->type) {
			case MTYPE_ACTION:
				{
					len = strlen(item->name);

					if (item->flags & QMF_LEFT_JUSTIFY) {
						min[0] += LCOLUMN_OFFSET * 2;
						max[0] = min[0] + len * 8;
					} else {
						min[0] -= len * 8 + 24;
					}

					type = MENUITEM_ACTION;
				}
				break;
			case MTYPE_SLIDER:
				{
					min[0] -= 16;
					max[0] += (SLIDER_RANGE + 2) * 8 + 16;
					type = MENUITEM_SLIDER;
				}
				break;
			case MTYPE_LIST:
			case MTYPE_SPINCONTROL:
				{
					int		len;
					menulist_s     *spin = menu->items[i];


					if (item->name) {
						len = strlen(item->name);
						min[0] -= len * 8 - LCOLUMN_OFFSET * 2;
					}
					len = strlen(spin->itemnames[spin->curvalue]);
					max[0] += len * 8;

					type = MENUITEM_ROTATE;
				}
				break;
			case MTYPE_FIELD:
				{
					menufield_s    *text = menu->items[i];

					len = text->visible_length + 2;

					max[0] += len * 8;
					type = MENUITEM_TEXT;
				}
				break;
			default:
				continue;
			}

			if (cursor.x >= min[0] &&
			    cursor.x <= max[0] &&
			    cursor.y >= min[1] &&
			    cursor.y <= max[1]) {
				/* new item */
				if (lastitem != item) {
					int		j;

					for (j = 0; j < MENU_CURSOR_BUTTON_MAX; j++) {
						cursor.buttonclicks[j] = 0;
						cursor.buttontime[j] = 0;
					}
				}
				cursor.menuitem = item;
				cursor.menuitemtype = type;

				menu->cursor = i;

				break;
			}
		}
	}
	cursor.mouseaction = false;

	item = Menu_ItemAtCursor(menu);

	if (item && item->cursordraw) {
		item->cursordraw(item);
	} else if (menu->cursordraw) {
		menu->cursordraw(menu);
	} else if (item && item->type != MTYPE_FIELD) {
		if (item->flags & QMF_LEFT_JUSTIFY) {
			Draw_Char(menu->x + item->x - 24 + item->cursor_offset, menu->y + item->y, 12 + ((int)(Sys_Milliseconds() / 250) & 1), 255);
		} else {
			Draw_Char(menu->x + item->cursor_offset, menu->y + item->y, 12 + ((int)(Sys_Milliseconds() / 250) & 1), 255);
		}
	}
	if (item) {
		if (item->statusbarfunc)
			item->statusbarfunc((void *)item);
		else if (item->statusbar)
			Menu_DrawStatusBar(item->statusbar);
		else
			Menu_DrawStatusBar(menu->statusbar);

	} else {
		Menu_DrawStatusBar(menu->statusbar);
	}
}

void
Menu_DrawStatusBar(const char *string)
{
	if (string) {
		int		l = strlen(string);
		int		maxcol = VID_WIDTH / 8;
		int		col = maxcol / 2 - l / 2;

		Draw_Fill(0, VID_HEIGHT - 8, VID_WIDTH, 8, 4);
		Menu_DrawString(col * 8, VID_HEIGHT - 8, string, 255);
	} else {
		Draw_Fill(0, VID_HEIGHT - 8, VID_WIDTH, 8, 0);
	}
}

void
Menu_DrawString(int x, int y, const char *string, int alpha)
{
	unsigned	i;

	for (i = 0; i < strlen(string); i++) {
		Draw_Char((x + i * 8), y, string[i], alpha);
	}
}

void
Menu_DrawStringDark(int x, int y, const char *string, int alpha)
{
	unsigned	i;

	for (i = 0; i < strlen(string); i++) {
		Draw_Char((x + i * 8), y, string[i] + 128, alpha);
	}
}

void
Menu_DrawStringR2L(int x, int y, const char *string, int alpha)
{
	unsigned	i;

	for (i = 0; i < strlen(string); i++) {
		Draw_Char((x - i * 8), y, string[strlen(string) - i - 1], alpha);
	}
}

void
Menu_DrawStringR2LDark(int x, int y, const char *string, int alpha)
{
	unsigned	i;

	for (i = 0; i < strlen(string); i++) {
		Draw_Char((x - i * 8), y, string[strlen(string) - i - 1] + 128, alpha);
	}
}

void           *
Menu_ItemAtCursor(menuframework_s * m)
{
	if (m->cursor < 0 || m->cursor >= m->nitems)
		return 0;

	return m->items[m->cursor];
}

qboolean
Menu_SelectItem(menuframework_s * s)
{
	menucommon_s   *item = (menucommon_s *) Menu_ItemAtCursor(s);

	if (item) {
		switch (item->type) {
		case MTYPE_FIELD:
			return Field_DoEnter((menufield_s *) item);
		case MTYPE_ACTION:
			Action_DoEnter((menuaction_s *) item);
			return true;
		case MTYPE_LIST:
			/* Menulist_DoEnter( ( menulist_s * ) item ); */
			return false;
		case MTYPE_SPINCONTROL:
			/* SpinControl_DoEnter( ( menulist_s * ) item ); */
			return false;
		}
	}
	return false;
}

qboolean
Menu_MouseSelectItem(menucommon_s * item)
{
	if (item) {
		switch (item->type) {
			case MTYPE_FIELD:
			return Field_DoEnter((menufield_s *) item);
		case MTYPE_ACTION:
			Action_DoEnter((menuaction_s *) item);
			return true;
		case MTYPE_LIST:
		case MTYPE_SPINCONTROL:
			return false;
		}
	}
	return false;
}

void
Menu_SetStatusBar(menuframework_s * m, const char *string)
{
	m->statusbar = string;
}

void
Menu_SlideItem(menuframework_s * s, int dir)
{
	menucommon_s   *item = (menucommon_s *) Menu_ItemAtCursor(s);

	if (item) {
		switch (item->type) {
		case MTYPE_SLIDER:
			Slider_DoSlide((menuslider_s *) item, dir);
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_DoSlide((menulist_s *) item, dir);
			break;
		}
	}
}

int
Menu_TallySlots(menuframework_s * menu)
{
	int		i;
	int		total = 0;

	for (i = 0; i < menu->nitems; i++) {
		if (((menucommon_s *) menu->items[i])->type == MTYPE_LIST) {
			int		nitems = 0;
			const char    **n = ((menulist_s *) menu->items[i])->itemnames;

			while (*n)
				nitems++, n++;

			total += nitems;
		} else {
			total++;
		}
	}

	return total;
}

#if 0				/* unused */
void
Menulist_DoEnter(menulist_s * l)
{
	int		start;

	start = l->generic.y / 10 + 1;

	l->curvalue = l->generic.parent->cursor - start;

	if (l->generic.callback)
		l->generic.callback(l);
}

#endif

void
MenuList_Draw(menulist_s * l)
{
	const char    **n;
	int		y = 0;
	int 		alpha = mouseOverAlpha(&l->generic);

	Menu_DrawStringR2LDark(l->generic.x + l->generic.parent->x + LCOLUMN_OFFSET, l->generic.y + l->generic.parent->y, l->generic.name, alpha);

	n = l->itemnames;

	Draw_Fill(l->generic.x - 112 + l->generic.parent->x, l->generic.parent->y + l->generic.y + l->curvalue * 10 + 10, 128, 10, 16);
	while (*n) {
		Menu_DrawStringR2LDark(l->generic.x + l->generic.parent->x + LCOLUMN_OFFSET, l->generic.y + l->generic.parent->y + y + 10, *n, alpha);

		n++;
		y += 10;
	}
}

void
Separator_Draw(menuseparator_s * s)
{
	int alpha = mouseOverAlpha(&s->generic);
	if (s->generic.name)
		Menu_DrawStringR2LDark(s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, s->generic.name, alpha);
}

void
Slider_DoSlide(menuslider_s * s, int dir)
{
	s->curvalue += dir;

	if (s->curvalue > s->maxvalue)
		s->curvalue = s->maxvalue;
	else if (s->curvalue < s->minvalue)
		s->curvalue = s->minvalue;

	if (s->generic.callback)
		s->generic.callback(s);
}

#define SLIDER_RANGE 10

void
Slider_Draw(menuslider_s * s)
{
	int		i;
	int		alpha = mouseOverAlpha(&s->generic);

	Menu_DrawStringR2LDark(s->generic.x + s->generic.parent->x + LCOLUMN_OFFSET,
	    s->generic.y + s->generic.parent->y,
	    s->generic.name, alpha);

	s->range = (s->curvalue - s->minvalue) / (float)(s->maxvalue - s->minvalue);

	if (s->range < 0)
		s->range = 0;
	if (s->range > 1)
		s->range = 1;
	Draw_Char(s->generic.x + s->generic.parent->x + RCOLUMN_OFFSET, s->generic.y + s->generic.parent->y, 128, 255);
	for (i = 0; i < SLIDER_RANGE; i++)
		Draw_Char(RCOLUMN_OFFSET + s->generic.x + i * 8 + s->generic.parent->x + 8, s->generic.y + s->generic.parent->y, 129, 255);
	Draw_Char(RCOLUMN_OFFSET + s->generic.x + i * 8 + s->generic.parent->x + 8, s->generic.y + s->generic.parent->y, 130, 255);
	Draw_Char((int)(8 + RCOLUMN_OFFSET + s->generic.parent->x + s->generic.x + (SLIDER_RANGE - 1) * 8 * s->range), s->generic.y + s->generic.parent->y, 131, 255);
}

#if 0				/* unused */
void
SpinControl_DoEnter(menulist_s * s)
{
	s->curvalue++;
	if (s->itemnames[s->curvalue] == 0)
		s->curvalue = 0;

	if (s->generic.callback)
		s->generic.callback(s);
}

#endif

void
SpinControl_DoSlide(menulist_s * s, int dir)
{
	s->curvalue += dir;

	if (s->curvalue < 0)
		s->curvalue = 0;
	else if (s->itemnames[s->curvalue] == 0)
		s->curvalue--;

	if (s->generic.callback)
		s->generic.callback(s);
}

void
SpinControl_Draw(menulist_s * s)
{
	char		buffer    [100];
	int		alpha = mouseOverAlpha(&s->generic);

	if (s->generic.name) {
		Menu_DrawStringR2LDark(s->generic.x + s->generic.parent->x + LCOLUMN_OFFSET,
		    s->generic.y + s->generic.parent->y,
		    s->generic.name, alpha);
	}
	if (!strchr(s->itemnames[s->curvalue], '\n')) {
		Menu_DrawString(RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, s->itemnames[s->curvalue], alpha);
	} else {
		Q_strncpyz(buffer, s->itemnames[s->curvalue], sizeof(buffer));
		*strchr(buffer, '\n') = 0;
		Menu_DrawString(RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, buffer, alpha);
		Q_strncpyz(buffer, strchr(s->itemnames[s->curvalue], '\n') + 1, sizeof(buffer));
		Menu_DrawString(RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y + 10, buffer, alpha);
	}
}
