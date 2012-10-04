#include "../client/client.h"
#include "../client/qmenu.h"

/*
 * ====================================================================
 *
 * REF stuff ... Used to dynamically load the menu with only those vid_ref's
 * that are present on this system
 *
 * ====================================================================
 */

/* this will have to be updated if ref's are added/removed from ref_t */
#define NUMBER_OF_REFS 2

/* all the refs should be initially set to 0 */
static char    *refs[NUMBER_OF_REFS + 1] = {0};

/* make all these have illegal values, as they will be redefined */
static int	REF_GLX = NUMBER_OF_REFS;
static int	REF_SDLGL = NUMBER_OF_REFS;

/* static int REF_FXGL    = NUMBER_OF_REFS; */

static int	GL_REF_START = NUMBER_OF_REFS;

typedef struct {
	char		menuname  [32];
	char		realname  [32];
	int            *pointer;
} ref_t;

static const ref_t possible_refs[NUMBER_OF_REFS] =
{
	{"[OpenGL GLX  ]", "q2glx", &REF_GLX},
	{"[SDL OpenGL  ]", "q2sdlgl", &REF_SDLGL}
};

/*
 * ====================================================================
 */

extern cvar_t  *vid_ref;
extern cvar_t  *vid_fullscreen;
extern cvar_t  *vid_gamma;
extern cvar_t  *scr_viewsize;

static cvar_t  *gl_mode;
static cvar_t  *gl_driver;
static cvar_t  *gl_picmip;

static cvar_t  *_windowed_mouse;

static cvar_t  *gl_texturemode;
static cvar_t  *gl_anisotropic;
static cvar_t  *gl_anisotropic_avail;
static cvar_t  *gl_bitdepth;

extern void	M_ForceMenuOff(void);
extern qboolean	VID_CheckRefExists(const char *name);

/*
 * ====================================================================
 *
 * MENU INTERACTION
 *
 * ====================================================================
 */
#define OPENGL_MENU   1

static menuframework_s s_opengl_menu;
static menuframework_s *s_current_menu;
static int	s_current_menu_index;

static menulist_s s_mode_list[2];
static menulist_s s_ref_list[2];
static menuslider_s s_tq_slider;
static menulist_s s_qudos_texfilter;
static menulist_s s_qudos_aniso;
static menulist_s s_qudos_texcompress;
static menulist_s s_qudos_bitdepth;
static menuslider_s s_screensize_slider[2];
static menuslider_s s_brightness_slider[2];
static menulist_s s_fs_box[2];
static menulist_s s_stipple_box;
static menulist_s s_paletted_texture_box;
static menulist_s s_windowed_mouse;
static menuaction_s s_apply_action[2];
static menuaction_s s_defaults_action[2];

static void
DriverCallback(void *unused)
{
	s_ref_list[!s_current_menu_index].curvalue = s_ref_list[s_current_menu_index].curvalue;
	s_current_menu = &s_opengl_menu;
	s_current_menu_index = 1;

}

static void
ScreenSizeCallback(void *s)
{
	menuslider_s   *slider = (menuslider_s *) s;

	Cvar_SetValue("viewsize", slider->curvalue * 10);
}

static void
BrightnessCallback(void *s)
{
	menuslider_s   *slider = (menuslider_s *) s;

	if (s_current_menu_index == 0)
		s_brightness_slider[1].curvalue = s_brightness_slider[0].curvalue;
	else
		s_brightness_slider[0].curvalue = s_brightness_slider[1].curvalue;

	if ( stricmp(vid_ref->string, "q2sdlgl") == 0 ||
	     stricmp(vid_ref->string, "q2glx") == 0) {
		float		gamma = (0.8 - (slider->curvalue / 10.0 - 0.5)) + 0.5;

		Cvar_SetValue("vid_gamma", gamma);
	}
}

static void
ResetDefaults(void *unused)
{
	VID_MenuInit();
}

static void
ApplyChanges(void *unused)
{
	float		gamma;
	int		ref;

	/*
	 * * make values consistent
	 */
	s_fs_box[!s_current_menu_index].curvalue = s_fs_box[s_current_menu_index].curvalue;
	s_brightness_slider[!s_current_menu_index].curvalue = s_brightness_slider[s_current_menu_index].curvalue;
	s_ref_list[!s_current_menu_index].curvalue = s_ref_list[s_current_menu_index].curvalue;

	/*
	 * * invert sense so greater = brighter, and scale to a range of 0.5
	 * to 1.3
	 */
	gamma = (0.8 - (s_brightness_slider[s_current_menu_index].curvalue / 10.0 - 0.5)) + 0.5;

	Cvar_SetValue("vid_gamma", gamma);

	if (s_qudos_texfilter.curvalue == 0)
		Cvar_Set("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST");
	else if (s_qudos_texfilter.curvalue == 1)
		Cvar_Set("gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR");

	switch ((int)s_qudos_aniso.curvalue) {
	case 1:
		Cvar_SetValue("gl_anisotropic", 2.0);
		break;
	case 2:
		Cvar_SetValue("gl_anisotropic", 4.0);
		break;
	case 3:
		Cvar_SetValue("gl_anisotropic", 8.0);
		break;
	case 4:
		Cvar_SetValue("gl_anisotropic", 16.0);
		break;
	default:
	case 0:
		Cvar_SetValue("gl_anisotropic", 0.0);
		break;
	}

	Cvar_SetValue("sw_stipplealpha", s_stipple_box.curvalue);
	Cvar_SetValue("gl_picmip", 3 - s_tq_slider.curvalue);
	Cvar_SetValue("vid_fullscreen", s_fs_box[s_current_menu_index].curvalue);
	Cvar_SetValue("gl_ext_palettedtexture", s_paletted_texture_box.curvalue);
	Cvar_SetValue("gl_mode", s_mode_list[OPENGL_MENU].curvalue);
	Cvar_SetValue("_windowed_mouse", s_windowed_mouse.curvalue);
	Cvar_SetValue("gl_ext_texture_compression", s_qudos_texcompress.curvalue);
	Cvar_SetValue("gl_bitdepth", s_qudos_bitdepth.curvalue * 16);

	/*
	 * * must use an if here (instead of a switch), since the REF_'s are
	 * now variables * and not #DEFINE's (constants)
	 */
	ref = s_ref_list[s_current_menu_index].curvalue;
	if (ref == REF_GLX) {
		Cvar_Set("vid_ref", "q2glx");

		/*
		 * below is wrong if we use different libs for different GL
		 * reflibs
		 */
		Cvar_Get("gl_driver", "libGL.so.1", CVAR_ARCHIVE);	/* ??? create if it doesn't exit */
		if (gl_driver->modified)
			vid_ref->modified = true;
	} else if (ref == REF_SDLGL) {
		Cvar_Set("vid_ref", "q2sdlgl");

		/*
		 * below is wrong if we use different libs for different GL
		 * reflibs
		 */
		Cvar_Get("gl_driver", "libGL.so.1", CVAR_ARCHIVE);	/* ??? create if it doesn't exist */
		if (gl_driver->modified)
			vid_ref->modified = true;
	}
	M_ForceMenuOff();
}



/* Knightmare */
int
texfilter_box_setval(void)
{
	char           *texmode = Cvar_VariableString("gl_texturemode");

	if (!Q_strcasecmp(texmode, "GL_LINEAR_MIPMAP_NEAREST"))
		return 0;
	else
		return 1;
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

static const char *mip_names[] =
{
	"Bilinear ",
	"Trilinear",
	0
};


static const char *aniso0_names[] =
{
	"Not supported",
	0
};

static const char *aniso2_names[] =
{
	"Off",
	"2x ",
	0
};

static const char *aniso4_names[] =
{
	"Off",
	"2x ",
	"4x ",
	0
};

static const char *aniso8_names[] =
{
	"Off",
	"2x ",
	"4x ",
	"8x ",
	0
};

static const char *aniso16_names[] =
{
	"Off",
	"2x ",
	"4x ",
	"8x ",
	"16x",
	0
};

static const char **
GetAnisoNames()
{
	float		aniso_avail = Cvar_VariableValue("gl_anisotropic_avail");

	if (aniso_avail < 2.0)
		return aniso0_names;
	else if (aniso_avail < 4.0)
		return aniso2_names;
	else if (aniso_avail < 8.0)
		return aniso4_names;
	else if (aniso_avail < 16.0)
		return aniso8_names;
	else			/* >= 16.0 */
		return aniso16_names;
}


float
GetAnisoCurValue()
{
	float		aniso_avail = Cvar_VariableValue("gl_anisotropic_avail");
	float		anisoValue = ClampCvar(0, aniso_avail, Cvar_VariableValue("gl_anisotropic"));

	if (aniso_avail == 0)	/* not available */
		return 0;
	if (anisoValue < 2.0)
		return 0;
	else if (anisoValue < 4.0)
		return 1;
	else if (anisoValue < 8.0)
		return 2;
	else if (anisoValue < 16.0)
		return 3;
	else			/* >= 16.0 */
		return 4;
}

/*
 * * VID_MenuInit
 */
void
VID_MenuInit(void)
{
	int		i         , counter;

	static const char *resolutions[] =
	{
		"[640 480  ]",
		"[800 600  ]",
		"[960 720  ]",
		"[1024 768 ]",
		"[1152 864 ]",
		"[1280 1024]",
		"[1600 1200]",
		"[2048 1536]",
		"[1024 480 ]",	/* sony vaio pocketbook */
		"[1152 768 ]",	/* Apple TiBook */
		"[1152 854 ]",	/* Apple TiBook */
		"[1280 800 ]",	/* apple TiBook */
		"[1280 854 ]",	/* apple TiBook */
		"[1280 960 ]",	/* apple TiBook */
		"[640 400  ]",	/* generic 16:10 widescreen resolutions */
		"[800 500  ]",	/* as found on many modern notebooks    */
		"[1024 640 ]",
		"[1280 800 ]",
		"[1366 768 ]",
		"[1440 900 ]",
		"[1680 1050]",
		"[1920 1200]",
		"[1024 600 ]",	/*netbooks	*/
		"[1024 574 ]",
		0
	};
	static const char *yesno_names[] =
	{
		"No ",
		"Yes",
		0
	};

	static const char *bitdepth_names[] =
	{
		"Desktop",
		"16 bit",
		"32 bit",
		0
	};

	/* make sure these are invalided before showing the menu again */
	REF_GLX = NUMBER_OF_REFS;
	REF_SDLGL = NUMBER_OF_REFS;


	GL_REF_START = NUMBER_OF_REFS;

	/* now test to see which ref's are present */
	i = counter = 0;
	while (i < NUMBER_OF_REFS) {
		if (VID_CheckRefExists(possible_refs[i].realname)) {
			*(possible_refs[i].pointer) = counter;

			/* free any previous string */
			if (refs[i])
				free(refs[i]);
			refs[counter] = strdup(possible_refs[i].menuname);

			/*
			 * * if we reach the 3rd item in the list, this
			 * indicates that a * GL ref has been found; this
			 * will change if more software * modes are added to
			 * the possible_ref's array
			 */
			if (i == 3)
				GL_REF_START = counter;

			counter++;
		}
		i++;
	}
	refs[counter] = (char *)0;

	if (!gl_driver)
		gl_driver = Cvar_Get("gl_driver", "libGL.so.1", 0);
	if (!gl_picmip)
		gl_picmip = Cvar_Get("gl_picmip", "0", 0);
	if (!gl_mode)
		gl_mode = Cvar_Get("gl_mode", "4", 0);
	if (!gl_anisotropic)
		gl_anisotropic = Cvar_Get("gl_anisotropic", "0", CVAR_ARCHIVE);
	if (!gl_texturemode)
		gl_texturemode = Cvar_Get("gl_texturemode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
	if (!gl_anisotropic_avail)
		gl_anisotropic_avail = Cvar_Get("gl_anisotropic_avail", "0", 0);
	if (!gl_bitdepth)
		gl_bitdepth = Cvar_Get("gl_bitdepth", "0", CVAR_ARCHIVE);

	if (!_windowed_mouse)
		_windowed_mouse = Cvar_Get("_windowed_mouse", "1", CVAR_ARCHIVE);

	s_mode_list[OPENGL_MENU].curvalue = gl_mode->value;

	if (!scr_viewsize)
		scr_viewsize = Cvar_Get("viewsize", "100", CVAR_ARCHIVE);
		
	s_screensize_slider[OPENGL_MENU].curvalue = scr_viewsize->value / 10;

	if (strcmp(vid_ref->string, "q2glx") == 0) {
		s_current_menu_index = OPENGL_MENU;
		s_ref_list[s_current_menu_index].curvalue = REF_GLX;
	} else if (strcmp(vid_ref->string, "q2sdlgl") == 0) {
		s_current_menu_index = OPENGL_MENU;
		s_ref_list[s_current_menu_index].curvalue = REF_SDLGL;
	}
	
	s_opengl_menu.x = viddef.width * 0.50;
	s_opengl_menu.nitems = 0;

	for (i = 0; i < 2; i++) {
		s_ref_list[i].generic.type = MTYPE_SPINCONTROL;
		s_ref_list[i].generic.name = "Driver";
		s_ref_list[i].generic.x = 0;
		s_ref_list[i].generic.y = 10;
		s_ref_list[i].generic.callback = DriverCallback;
		s_ref_list[i].itemnames = (const char **)refs;

		s_mode_list[i].generic.type = MTYPE_SPINCONTROL;
		s_mode_list[i].generic.name = "Video mode";
		s_mode_list[i].generic.x = 0;
		s_mode_list[i].generic.y = 20;
		s_mode_list[i].itemnames = resolutions;

		s_fs_box[i].generic.type = MTYPE_SPINCONTROL;
		s_fs_box[i].generic.x = 0;
		s_fs_box[i].generic.y = 30;
		s_fs_box[i].generic.name = "Fullscreen";
		s_fs_box[i].itemnames = yesno_names;
		s_fs_box[i].curvalue = vid_fullscreen->value;

		s_qudos_texfilter.generic.type = MTYPE_SPINCONTROL;
		s_qudos_texfilter.generic.x = 0;
		s_qudos_texfilter.generic.y = 50;
		s_qudos_texfilter.generic.name = "Texture filter";
		s_qudos_texfilter.curvalue = texfilter_box_setval();
		s_qudos_texfilter.itemnames = mip_names;

		s_qudos_aniso.generic.type = MTYPE_SPINCONTROL;
		s_qudos_aniso.generic.x = 0;
		s_qudos_aniso.generic.y = 60;
		s_qudos_aniso.generic.name = "Anisotropic filter";
		s_qudos_aniso.curvalue = GetAnisoCurValue();
		s_qudos_aniso.itemnames = GetAnisoNames();

		s_qudos_texcompress.generic.type = MTYPE_SPINCONTROL;
		s_qudos_texcompress.generic.x = 0;
		s_qudos_texcompress.generic.y = 70;
		s_qudos_texcompress.generic.name = "Texture compression";
		s_qudos_texcompress.itemnames = yesno_names;
		s_qudos_texcompress.curvalue = Cvar_VariableValue("gl_ext_texture_compression");

		s_tq_slider.generic.type = MTYPE_SLIDER;
		s_tq_slider.generic.x = 0;
		s_tq_slider.generic.y = 80;
		s_tq_slider.generic.name = "Texture quality";
		s_tq_slider.minvalue = 0;
		s_tq_slider.maxvalue = 3;
		s_tq_slider.curvalue = 3 - gl_picmip->value;

		s_qudos_bitdepth.generic.type = MTYPE_SPINCONTROL;
		s_qudos_bitdepth.generic.x = 0;
		s_qudos_bitdepth.generic.y = 90;
		s_qudos_bitdepth.generic.name = "Colour depth";
		s_qudos_bitdepth.curvalue = (gl_bitdepth->value) / 16;
		s_qudos_bitdepth.itemnames = bitdepth_names;

		s_screensize_slider[i].generic.type = MTYPE_SLIDER;
		s_screensize_slider[i].generic.x = 0;
		s_screensize_slider[i].generic.y = 110;
		s_screensize_slider[i].generic.name = "Screen size";
		s_screensize_slider[i].minvalue = 3;
		s_screensize_slider[i].maxvalue = 12;
		s_screensize_slider[i].generic.callback = ScreenSizeCallback;

		s_brightness_slider[i].generic.type = MTYPE_SLIDER;
		s_brightness_slider[i].generic.x = 0;
		s_brightness_slider[i].generic.y = 120;
		s_brightness_slider[i].generic.name = "Brightness";
		s_brightness_slider[i].generic.callback = BrightnessCallback;
		s_brightness_slider[i].minvalue = 5;
		s_brightness_slider[i].maxvalue = 13;
		s_brightness_slider[i].curvalue = (1.3 - vid_gamma->value + 0.5) * 10;

		s_defaults_action[i].generic.type = MTYPE_ACTION;
		s_defaults_action[i].generic.name = "Reset to defaults";
		s_defaults_action[i].generic.x = 0;
		s_defaults_action[i].generic.y = 150;
		s_defaults_action[i].generic.callback = ResetDefaults;

		s_apply_action[i].generic.type = MTYPE_ACTION;
		s_apply_action[i].generic.name = "Apply";
		s_apply_action[i].generic.x = 0;
		s_apply_action[i].generic.y = 160;
		s_apply_action[i].generic.callback = ApplyChanges;
	}

	Menu_AddItem(&s_opengl_menu, (void *)&s_ref_list[OPENGL_MENU]);
	Menu_AddItem(&s_opengl_menu, (void *)&s_mode_list[OPENGL_MENU]);
	Menu_AddItem(&s_opengl_menu, (void *)&s_fs_box[OPENGL_MENU]);

	Menu_AddItem(&s_opengl_menu, (void *)&s_qudos_texfilter);
	Menu_AddItem(&s_opengl_menu, (void *)&s_qudos_aniso);
	Menu_AddItem(&s_opengl_menu, (void *)&s_qudos_texcompress);
	Menu_AddItem(&s_opengl_menu, (void *)&s_tq_slider);
	Menu_AddItem(&s_opengl_menu, (void *)&s_qudos_bitdepth);
	Menu_AddItem(&s_opengl_menu, (void *)&s_screensize_slider[OPENGL_MENU]);
	Menu_AddItem(&s_opengl_menu, (void *)&s_brightness_slider[OPENGL_MENU]);

	Menu_AddItem(&s_opengl_menu, (void *)&s_defaults_action[OPENGL_MENU]);
	Menu_AddItem(&s_opengl_menu, (void *)&s_apply_action[OPENGL_MENU]);

	Menu_Center(&s_opengl_menu);
	s_opengl_menu.x -= 8;
}

/*
 * ================ VID_MenuShutdown ================
 */
void
VID_MenuShutdown(void)
{
	int		i;

	for (i = 0; i < NUMBER_OF_REFS; i++) {
		if (refs[i])
			free(refs[i]);
	}
}

/*
 * ================ VID_MenuDraw ================
 */
void
VID_MenuDraw(void)
{
	int		w         , h;

	s_current_menu = &s_opengl_menu;

	/*
	 * * draw the banner
	 */
	re.DrawGetPicSize(&w, &h, "m_banner_video");
	re.DrawPic(viddef.width / 2 - w / 2, viddef.height / 2 - 120, "m_banner_video", 1.0f);

	/*
	 * * move cursor to a reasonable starting position
	 */
	Menu_AdjustCursor(s_current_menu, 1);

	/*
	 * * draw the menu
	 */
	Menu_Draw(s_current_menu);
}

/*
 * ================ VID_MenuKey ================
 */
const char     *
VID_MenuKey(int key)
{
	extern void	M_PopMenu(void);

	menuframework_s *m = s_current_menu;
	static const char *sound = "misc/menu1.wav";

	switch (key) {
	case K_ESCAPE:
		M_PopMenu();
		return NULL;
	case K_UPARROW:
		m->cursor--;
		Menu_AdjustCursor(m, -1);
		break;
	case K_DOWNARROW:
		m->cursor++;
		Menu_AdjustCursor(m, 1);
		break;
	case K_LEFTARROW:
		Menu_SlideItem(m, -1);
		break;
	case K_RIGHTARROW:
		Menu_SlideItem(m, 1);
		break;
	case K_ENTER:
		Menu_SelectItem(m);
		break;
	}

	return sound;
}
