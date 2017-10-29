/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2005, 2007, 2009 David Olofson
 * Copyright 2008 Robert Schuster
 * Copyright 2015-2017 David Olofson (Kobo Redux)
 * 
 * This program  is free software; you can redistribute it and/or modify it
 * under the terms  of  the GNU General Public License  as published by the
 * Free Software Foundation;  either version 2 of the License,  or (at your
 * option) any later version.
 *
 * This program is  distributed  in  the hope that  it will be useful,  but
 * WITHOUT   ANY   WARRANTY;   without   even   the   implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received  a copy of the GNU General Public License along
 * with this program; if not,  write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _KOBO_H_
#define _KOBO_H_

#include "config.h"

#include "gfxengine.h"
#include "window.h"
#include "filemap.h"
#include "prefs.h"
#include "radar.h"
#include "dashboard.h"
#include "sound.h"
#include "spinplanet.h"
#include "fire.h"
#include "themeparser.h"
#include "savemanager.h"


  /////////////////////////////////////////////////////////////////////////////
 //	Constants
/////////////////////////////////////////////////////////////////////////////

// Number of weapon slots
#define	WEAPONSLOTS	4

// Sprite priority levels
#define	LAYER_OVERLAY	0	// Mouse crosshair
#define	LAYER_BULLETS	1	// Bullets - most important!
#define	LAYER_FX	2	// Explosions and similar effects
#define	LAYER_PLAYER	3	// Player and fire bolts
#define	LAYER_ENEMIES	4	// Enemies
#define	LAYER_BASES	5	// Bases and stationary enemies

// NOTE: Needs to be a layer with 1:1 scroll ratio!
#define	LAYER_PLANET	LAYER_BASES	// Spinning planet control

#define	NOALPHA_THRESHOLD	64

typedef enum
{
	STARFIELD_NONE = 0,
	STARFIELD_OLD,
	STARFIELD_PARALLAX
} KOBO_StarfieldModes;


  /////////////////////////////////////////////////////////////////////////////
 //	Top level classes
/////////////////////////////////////////////////////////////////////////////

enum kobo_vmswitch_t {
	KOBO_VIDEOMODE_TOGGLE,
	KOBO_VIDEOMODE_WINDOWED,
	KOBO_VIDEOMODE_FULLSCREEN,
	KOBO_VIDEOMODE_SAFE
};

enum KOBO_TransitionStyle {
	KOBO_TRS_INSTANT,
	KOBO_TRS_FULLSCREEN_SLOW,
	KOBO_TRS_FULLSCREEN_FAST,
	KOBO_TRS_FULLSCREEN_IN_ONLY,
	KOBO_TRS_GAME_SLOW,
	KOBO_TRS_GAME_FAST,
	KOBO_TRS_GAME_NOISE
};

class kobo_gfxengine_t : public gfxengine_t
{
#ifdef ENABLE_TOUCHSCREEN
	bool pointer_margin_used;
	int pointer_margin_width_min;
	int pointer_margin_width_max;
	int pointer_margin_height_min;
	int pointer_margin_height_max;
#endif
	int st_sound;
	int st_handle;
	int st_x;
	int st_y;
	void pre_loop();
	void input(float fractional_frame);
	void pre_advance(float fractional_frame);
	void st_update_sound_displays();
	bool soundtools_event(SDL_Event &ev);
	void mouse_motion(SDL_Event &ev);
	void mouse_button_down(SDL_Event &ev);
	void mouse_button_up(SDL_Event &ev);
	void frame();
	void pre_render();
	void pre_sprite_render();
	void post_sprite_render();
	void post_render();
	void post_loop();
	float timestamp_delay();
  public:
	kobo_gfxengine_t();
	void switch_video_mode(kobo_vmswitch_t vms);
#ifdef ENABLE_TOUCHSCREEN
	void setup_pointer_margin(int, int);
#endif
};


class backdrop_t : public window_t
{
  protected:
	bool	_reverse;
	int	_image;
  public:
	backdrop_t(gfxengine_t *e);
	void reverse(bool rv)		{ _reverse = rv; }
	void image(int bank)		{ _image = bank; }
	void refresh(SDL_Rect *r);
};


class KOBO_main
{
	static int		exit_game;
	static int		exit_game_fast;
  public:
	static SDL_Joystick	*joystick;
	static int		js_lr;
	static int		js_ud;
	static int		js_primary;
	static int		js_secondary;
	static int		js_start;

	static FILE		*logfile;
	static FILE		*userlogfile;

	static Uint32		esc_tick;
	static int		esc_count;
	static int		esc_hammering_trigs;

	// Frame rate counter
	static int		fps_count;
	static int		fps_starttime;
	static int		fps_nextresult;
	static int		fps_lastresult;
	static float		*fps_results;
	static float		fps_last;

	// Frame rate limiter
	static float		maxfps_filter;
	static int		maxfps_begin;

	// Dashboard offset ("native" 640x360 pixels)
	static int		xoffs;
	static int		yoffs;

	// Backup in case we screw up we can't get back up
	static prefs_t		safe_prefs;

	// Sound design tools
	static label_t		*st_hotkeys;
	static display_t	*st_symname;

	// Themes
	static KOBO_ThemeData	*themes;

	// Screenshot video
	static int ss_frames;
	static int ss_last_frame;

	// Level messages
	static int smsg_stage;
	static char *smsg_header;
	static char *smsg_message;

	static int restart_audio();
	static int restart_video();
	static int reload_sounds();
	static int reload_graphics();

	static int open();
	static void close();
	static int run();

	static int open_logging(prefs_t *p);
	static void close_logging(bool final = false);
	static void load_config(prefs_t *p);
	static void save_config(prefs_t *p);

	static void init_dash_layout();
	static int init_display(prefs_t *p);
	static void close_display();

	static void noiseburst();
	static void show_progress(prefs_t *p);
	static void progress();
	static void doing(const char *msg);

	static void discover_themes(const char *ref = NULL);
	static KOBO_ThemeData *get_next_theme(const char *type,
			KOBO_ThemeData *td);
	static void list_themes();
	static void free_themes();

	static int load_palette();
	static int load_graphics();
	static int load_sounds(bool progress = true);

	static int init_js(prefs_t *p);
	static void close_js();

	static bool escape_hammering();
	static bool escape_hammering_quit();
	static bool quit_requested();
	static bool skip_requested();
	static void pause_game();

	static void quit();
	static void brutal_quit(bool force = false);
	bool quitting()		{ return exit_game || exit_game_fast; }

	static void print_fps_results();

	static void place(windowbase_t *w, KOBO_TD_Items td);

	static void set_stagemessage(int stage, const char *hdr,
			const char *msg)
	{
		// HAX: "There can be only one!"
		smsg_stage = stage;
		free(smsg_header);
		smsg_header = strdup(hdr);
		free(smsg_message);
		smsg_message = strdup(msg);
	}
	static void clear_messages()
	{
		smsg_stage = 0;
		free(smsg_header);
		smsg_header = NULL;
		free(smsg_message);
		smsg_message = NULL;
	}
};


  /////////////////////////////////////////////////////////////////////////////
 //	Singletons
/////////////////////////////////////////////////////////////////////////////

extern KOBO_sound		sound;
extern KOBO_ThemeData		themedata;
extern KOBO_main		km;
extern KOBO_save_manager	savemanager;

#define THD(x, y)	(themedata.get(KOBO_D_##x, (y)))
#define	DASHX(x)	((int)THD(DASH_##x, 0))
#define	DASHY(x)	((int)THD(DASH_##x, 1))
#define	DASHW(x)	((int)THD(DASH_##x, 2))
#define	DASHH(x)	((int)THD(DASH_##x, 3))


  /////////////////////////////////////////////////////////////////////////////
 //	Globals
/////////////////////////////////////////////////////////////////////////////

extern kobo_gfxengine_t		*gengine;
extern filemapper_t		*fmap;
extern prefs_t			*prefs;

extern screen_window_t		*wscreen;
extern dashboard_window_t	*wdash;
extern shieldbar_t		*whealth;
extern chargebar_t		*wcharge;
extern KOBO_radar_map		*wmap;
extern KOBO_radar_window	*wradar;

extern backdrop_t		*wbackdrop;
extern spinplanet_t		*wplanet;
extern lowsprites_t		*wlowsprites;
extern highsprites_t		*whighsprites;
extern KOBO_Fire		*wfire;
extern KOBO_Fire		*wmenufire;
extern window_t			*woverlay;

extern display_t		*dhigh;
extern display_t		*dscore;
extern display_t		*dregion;
extern display_t		*dlevel;
extern weaponslot_t		*wslots[WEAPONSLOTS];
extern hledbar_t		*pxtop;
extern hledbar_t		*pxbottom;
extern vledbar_t		*pxleft;
extern vledbar_t		*pxright;

extern int mouse_x, mouse_y;
extern int mouse_left, mouse_middle, mouse_right;
extern bool mouse_visible;

#endif // _KOBO_H_
