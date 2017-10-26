/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2005-2007, 2009, 2011 David Olofson
 * Copyright 2005 Erik Auerswald
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

#define	DBG(x)	x

#undef	DEBUG_OUT

// Use this to benchmark and create a new percentage table!
#undef	TIME_PROGRESS

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#include "kobolog.h"
#include "kobo.h"
#include "states.h"
#include "screen.h"
#include "manage.h"
#include "gamectl.h"
#include "random.h"
#include "options.h"
#include "myship.h"
#include "enemies.h"

#include "SDL_revision.h"

#define	MAX_FPS_RESULTS	64

/* Joystick support */
#define DEFAULT_JOY_LR		0	// Joystick axis left-right default
#define DEFAULT_JOY_UD		1	// Joystick axis up-down default
#define DEFAULT_JOY_PRIMARY	0	// Default primary fire button
#define DEFAULT_JOY_SECONDARY	1	// Default secondary fire button
#define DEFAULT_JOY_START	2	// Default start/pause/select


/*----------------------------------------------------------
	Singletons
----------------------------------------------------------*/
KOBO_sound		sound;
KOBO_ThemeData		themedata;
KOBO_save_manager	savemanager;


/*----------------------------------------------------------
	Globals
----------------------------------------------------------*/
filemapper_t		*fmap = NULL;
prefs_t			*prefs = NULL;
kobo_gfxengine_t	*gengine = NULL;

screen_window_t		*wscreen = NULL;
dashboard_window_t	*wdash = NULL;
shieldbar_t		*whealth = NULL;
chargebar_t		*wcharge = NULL;
KOBO_radar_map		*wmap = NULL;
KOBO_radar_window	*wradar = NULL;

backdrop_t		*wbackdrop = NULL;
spinplanet_t		*wplanet = NULL;
engine_window_t		*wmain = NULL;
KOBO_Fire		*wfire = NULL;
window_t		*woverlay = NULL;

display_t		*dhigh = NULL;
display_t		*dscore = NULL;
display_t		*dregion = NULL;
display_t		*dlevel = NULL;
weaponslot_t		*wslots[WEAPONSLOTS] = { NULL, NULL, NULL, NULL };
hledbar_t		*pxtop = NULL;
hledbar_t		*pxbottom = NULL;
vledbar_t		*pxleft = NULL;
vledbar_t		*pxright = NULL;

int mouse_x = 0;
int mouse_y = 0;
int mouse_left = 0;
int mouse_middle = 0;
int mouse_right = 0;
bool mouse_visible = false;
int mouse_timer = 0;


/*----------------------------------------------------------
	Backdrop window
----------------------------------------------------------*/

backdrop_t::backdrop_t(gfxengine_t *e) : window_t(e)
{
	_reverse = false;
	_image = 0;
}


void backdrop_t::refresh(SDL_Rect *r)
{
	s_bank_t *b = s_get_bank(gengine->get_gfx(), _image);
	if(!b)
		return;

	// Remove centering, and handle reverse scrolling
	int vx = gengine->xoffs(LAYER_BASES);
	int vy = gengine->yoffs(LAYER_BASES);
	int mw = DASHW(MAIN);
	int mh = DASHH(MAIN);
	if(_reverse)
	{
		vx += PIXEL2CS(mw / 2);
		vy += PIXEL2CS(mh / 2);
	}
	else
	{
		vx = 2 * PIXEL2CS(WORLD_SIZEX) - vx - PIXEL2CS(mw / 2);
		vy = 2 * PIXEL2CS(WORLD_SIZEY) - vy - PIXEL2CS(mh / 2);
	}

	// If the backdrop aspect ration differs from that of the may, assume
	// that the backdrop is supposed to repeat in one direction, so that
	// the vertical and horizontal scroll speed ratios are the same. Note
	// that we assume integer ratios here; anything else will produce weird
	// results...
	float a = ((float)b->w / b->h) / ((float)WORLD_SIZEX / WORLD_SIZEY);
	if(a > 1.5f)
		vy *= floor(a);
	else if(1.0f / a > 1.5f)
		vx *= floor(1.0f / a);

	// Scale and center so all layers align when the player is at (0, 0)
	int xo = vx * b->w / WORLD_SIZEX + PIXEL2CS(mw / 2);
	int yo = vy * b->h / WORLD_SIZEY + PIXEL2CS(mh / 2);
	int bw = b->w << 8;
	int bh = b->h << 8;
	int x = xo % bw;
	int y = yo % bh;
	sprite_fxp(x - bw, y - bh, _image, 0);
	sprite_fxp(x, y - bh, _image, 0);
	sprite_fxp(x - bw, y, _image, 0);
	sprite_fxp(x, y, _image, 0);
}


/*----------------------------------------------------------
	Various functions
----------------------------------------------------------*/

static int main_init()
{
	prefs = new prefs_t;
	prefs->initialize();
	fmap = new filemapper_t;
	gengine = new kobo_gfxengine_t;
	return 0;
}


static void main_cleanup()
{
	km.close_logging(true);
	delete gengine;
	gengine = NULL;
	delete fmap;
	fmap = NULL;
	delete prefs;
	prefs = NULL;
}


// Hardwired paths
static void setup_dirs(char *xpath)
{
	char *p = SDL_GetBasePath();
	if(p)
	{
		fmap->exepath(p);
		SDL_free(p);
	}
	else
		fmap->exepath(xpath);

	// Data paths (sound and graphics, maps etc)
#ifdef KOBO_DATADIR
	fmap->addpath("DATA", KOBO_DATADIR);	// System wide install data dir
#endif
#ifdef KOBO_USERDIR
	fmap->addpath("DATA", KOBO_USERDIR);	// User dir (custom themes)
#endif
	fmap->addpath("DATA", "EXE>>");		// Next to the executable

	// Directory layout within a DATA directory
	fmap->addpath("GFX", "DATA>>gfx");
	fmap->addpath("SFX", "DATA>>sfx");

	// Configuration files
	if((p = SDL_GetPrefPath(KOBO_ORGANIZATION, KOBO_APPLICATION)))
	{
		fmap->addpath("CONFIG", p);
		SDL_free(p);
	}
#ifdef KOBO_USERDIR
	// (LEGACY FALLBACK: We should now rely on SDL_GetPrefPath()!)
	// For Un*x systems; typically "~/.koboredux". Try to create it right
	// away if it doesn't exist, because the only situation where it won't
	// be used is if the user just quits without starting a game, and
	// without touching the options. Also, "SAVES>>" should be in here when
	// possible, and filemapper_t won't create paths recursively, so...
	fmap->addpath("CONFIG", KOBO_USERDIR);
	fmap->mkdir(KOBO_USERDIR);
#endif
#ifdef KOBO_SYSCONFDIR
	// System local (custom default configs in official distro packages)
	fmap->addpath("CONFIG", KOBO_SYSCONFDIR);
#endif
	// (LEGACY FALLBACK: We should now rely on SDL_GetPrefPath()!)
	// For Win32, or packaging a custom default config in a Linux bz2 pkg
	fmap->addpath("CONFIG", "EXE>>");

	// Campaign and demo saves
	fmap->addpath("SAVES", "CONFIG>>saves");
	fmap->mkdir("SAVES>>");

	// Log files
	fmap->addpath("LOG", "CONFIG>>log");
	fmap->mkdir("LOG>>");
}


// Additional paths from the config
static void add_dirs(prefs_t *p)
{
	if(p->data[0])
	{
		char *pth = fmap->sys2fm(p->data);
		log_printf(ULOG, "Adding alternate data path \"%s\" (%s)\n",
				p->data, pth);
		fmap->addpath("DATA", pth, 1);
	}
}


SDL_Joystick	*KOBO_main::joystick = NULL;
int		KOBO_main::js_lr = DEFAULT_JOY_LR;
int		KOBO_main::js_ud = DEFAULT_JOY_UD;
int		KOBO_main::js_primary = DEFAULT_JOY_PRIMARY;
int		KOBO_main::js_secondary = DEFAULT_JOY_SECONDARY;
int		KOBO_main::js_start = DEFAULT_JOY_START;

FILE		*KOBO_main::logfile = NULL;
FILE		*KOBO_main::userlogfile = NULL;

Uint32		KOBO_main::esc_tick = 0;
int		KOBO_main::esc_count = 0;
int		KOBO_main::esc_hammering_trigs = 0;
int		KOBO_main::exit_game = 0;
int		KOBO_main::exit_game_fast = 0;

int		KOBO_main::fps_count = 0;
int		KOBO_main::fps_starttime = 0;
int		KOBO_main::fps_nextresult = 0;
int		KOBO_main::fps_lastresult = 0;
float		*KOBO_main::fps_results = NULL;
float		KOBO_main::fps_last = 0.0f;

float		KOBO_main::maxfps_filter = 0.0f;
int		KOBO_main::maxfps_begin = 0;

int		KOBO_main::xoffs = 0;
int		KOBO_main::yoffs = 0;

prefs_t		KOBO_main::safe_prefs;

label_t		*KOBO_main::st_hotkeys = NULL;
display_t	*KOBO_main::st_symname = NULL;

KOBO_ThemeData	*KOBO_main::themes = NULL;

int		KOBO_main::ss_frames = 0;
int		KOBO_main::ss_last_frame = 0;

int		KOBO_main::smsg_stage = 0;
char		*KOBO_main::smsg_header = NULL;
char		*KOBO_main::smsg_message = NULL;

KOBO_main km;


void KOBO_main::print_fps_results()
{
	int i, r = fps_nextresult;
	if(fps_lastresult != MAX_FPS_RESULTS-1)
		r = 0;
	for(i = 0; i < fps_lastresult; ++i)
	{
		log_printf(ULOG, "%.1f fps\n", fps_results[r++]);
		if(r >= MAX_FPS_RESULTS)
			r = 0;
	}

	free(fps_results);
	fps_nextresult = 0;
	fps_lastresult = 0;
	fps_results = NULL;
}


bool KOBO_main::escape_hammering()
{
	Uint32 nt = SDL_GetTicks();
	if(nt - esc_tick > 300)
		esc_count = 1;
	else
		++esc_count;
	if(nt - esc_tick > 3000)
		esc_hammering_trigs = 0;
	esc_tick = nt;
	if(esc_count == 5)
	{
		++esc_hammering_trigs;
		return true;
	}
	return false;
}


bool KOBO_main::escape_hammering_quit()
{
	return esc_hammering_trigs >= 2;
}


bool KOBO_main::quit_requested()
{
	SDL_Event e;
	while(SDL_PollEvent(&e))
	{
		switch(e.type)
		{
		  case SDL_QUIT:
			km.brutal_quit();
			break;
		  case SDL_WINDOWEVENT:
			switch(e.window.event)
			{
			  case SDL_WINDOWEVENT_SHOWN:
			  case SDL_WINDOWEVENT_EXPOSED:
			  case SDL_WINDOWEVENT_RESIZED:
			  case SDL_WINDOWEVENT_MAXIMIZED:
			  case SDL_WINDOWEVENT_RESTORED:
				break;
			  case SDL_WINDOWEVENT_CLOSE:
				km.brutal_quit();
				break;
			}
			break;
		  case SDL_KEYDOWN:
			switch(e.key.keysym.sym)
			{
			  case SDLK_PRINTSCREEN:
			  case SDLK_SYSREQ:
			  case SDLK_s:
				gengine->screenshot();
				break;
			}
			break;
		  case SDL_KEYUP:
			switch(e.key.keysym.sym)
			{
			  case SDLK_ESCAPE:
				if(escape_hammering())
					km.brutal_quit();
				break;
			  default:
				break;
			}
			break;
		}
	}
	if(exit_game_fast)
		return true;
	return SDL_QuitRequested();
}


bool KOBO_main::skip_requested()
{
	SDL_Event e;
	bool do_skip = false;
	while(SDL_PollEvent(&e))
	{
		switch(e.type)
		{
		  case SDL_QUIT:
			km.brutal_quit();
			break;
		  case SDL_WINDOWEVENT:
			switch(e.window.event)
			{
			  case SDL_WINDOWEVENT_SHOWN:
			  case SDL_WINDOWEVENT_EXPOSED:
			  case SDL_WINDOWEVENT_RESIZED:
			  case SDL_WINDOWEVENT_MAXIMIZED:
			  case SDL_WINDOWEVENT_RESTORED:
				break;
			  case SDL_WINDOWEVENT_CLOSE:
				km.brutal_quit();
				break;
			}
			break;
		  case SDL_KEYDOWN:
			switch(e.key.keysym.sym)
			{
			  case SDLK_PRINTSCREEN:
			  case SDLK_SYSREQ:
			  case SDLK_s:
				gengine->screenshot();
				break;
			}
			break;
		  case SDL_KEYUP:
			switch(e.key.keysym.sym)
			{
			  case SDLK_ESCAPE:
				if(escape_hammering())
					km.brutal_quit();
			  case SDLK_SPACE:
			  case SDLK_RETURN:
				do_skip = true;
				break;
			  default:
				break;
			}
			break;
		}
	}
	if(exit_game_fast)
		return true;
	return do_skip;
}


void KOBO_main::close_logging(bool final)
{
	// Flush logs to disk, close log files etc.
	log_close();

	if(userlogfile)
	{
		fclose(userlogfile);
		userlogfile = NULL;
	}

	// NOTE: We do NOT close 'logfile' here until we're about to quit the
	// application, because we don't want the debug log to be flushed when
	// the log config is changed, and there are no options that affect this
	// file anyway.
	if(final && logfile)
	{
		fclose(logfile);
		logfile = NULL;
	}
}


int KOBO_main::open_logging(prefs_t *p)
{
	bool force_console = false;

	close_logging();

	if(log_open(p ? 0 : LOG_RESET_TIME) < 0)
		return -1;

	// Disable everything!
	log_disable_level_target(-1, -1);

	// Set up stdout and stderr
	log_set_target_stream(KOBO_LOG_TARGET_STDOUT, stdout);
	log_set_target_stream(KOBO_LOG_TARGET_STDERR, stderr);

	// All levels output to stdout...
	log_enable_level_target(-1, KOBO_LOG_TARGET_STDOUT);

	// ...except these, that output to stderr.
	log_disable_level_target(ELOG, KOBO_LOG_TARGET_STDOUT);
	log_enable_level_target(ELOG, KOBO_LOG_TARGET_STDERR);
	log_disable_level_target(CELOG, KOBO_LOG_TARGET_STDOUT);
	log_enable_level_target(CELOG, KOBO_LOG_TARGET_STDERR);

	if(p)
	{
		// Set console log format
		int clf;
		switch(p->conlogformat)
		{
		  default:
			clf = LOG_TIMESTAMP;
			break;
		  case 1:
			clf = LOG_ANSI | LOG_TIMESTAMP;
			break;
		  case 2:
			clf = LOG_HTML | LOG_TIMESTAMP;
			break;
		}
		log_set_target_flags(KOBO_LOG_TARGET_STDOUT, clf);
		log_set_target_flags(KOBO_LOG_TARGET_STDERR, clf);

		// Set up user log file
		if(p->logfile)
		{
			switch (p->logformat)
			{
			  case 2:
				userlogfile = fopen("log.html", "wb");
				break;
			  default:
				userlogfile = fopen("log.txt", "wb");
				break;
			}
		}
		if(userlogfile)
		{
			log_set_target_stream(KOBO_LOG_TARGET_USER,
					userlogfile);
			switch(p->logformat)
			{
			  default:
				clf = LOG_TIMESTAMP;
				break;
			  case 1:
				clf = LOG_ANSI | LOG_TIMESTAMP;
				break;
			  case 2:
				clf = LOG_HTML | LOG_TIMESTAMP;
				break;
			}
			log_set_target_flags(KOBO_LOG_TARGET_USER, clf);
			log_enable_level_target(-1, KOBO_LOG_TARGET_USER);
		}
		else if(p->logfile)
		{
			// No user log file! Fall back to stdout/stderr.
			log_printf(ELOG, "Couldn't open user log file!\n");
			force_console = true;
		}
	}

	// Some fancy colors...
	log_set_level_attr(ULOG, LOG_YELLOW);
	log_set_level_attr(WLOG, LOG_YELLOW | LOG_BRIGHT);
	log_set_level_attr(ELOG, LOG_RED | LOG_BRIGHT);
	log_set_level_attr(CELOG, LOG_RED | LOG_BRIGHT | LOG_BLINK);
	log_set_level_attr(DLOG, LOG_CYAN);
	log_set_level_attr(D2LOG, LOG_BLUE | LOG_BRIGHT);
	log_set_level_attr(D3LOG, LOG_BLUE);

	if(p)
	{
		// Disable console output if disabled in config, unless forced!
		if(!(p->logconsole || force_console))
		{
			log_disable_level_target(-1, KOBO_LOG_TARGET_STDOUT);
			log_disable_level_target(-1, KOBO_LOG_TARGET_STDERR);
		}

		// Disable levels as desired
		switch(p->logverbosity)
		{
		  case 0:
			log_disable_level_target(ELOG, -1);
		  case 1:
			log_disable_level_target(WLOG, -1);
		  case 2:
			log_disable_level_target(DLOG, -1);
		  case 3:
			log_disable_level_target(D2LOG, -1);
		  case 4:
			log_disable_level_target(D3LOG, -1);
		  case 5:
			break;
		}

		// Set up the hardwired debug log
		if(!logfile)
			logfile = fmap->fopen(KOBO_DEBUGLOGFILE, "wb");
		if(logfile)
		{
			log_set_target_flags(KOBO_LOG_TARGET_DEBUG,
					LOG_TIMESTAMP);
			log_set_target_stream(KOBO_LOG_TARGET_DEBUG, logfile);
			log_enable_level_target(-1, KOBO_LOG_TARGET_DEBUG);
		}
		else
			log_printf(ELOG, "Couldn't open debug log file!\n");
	}

	return 0;
}


void KOBO_main::place(windowbase_t *w, KOBO_TD_Items td)
{
	int x = xoffs + themedata.get(td, 0);
	int y = yoffs + themedata.get(td, 1);
	if(themedata.defined(td, 4))
	{
		KOBO_TD_Items rel_to = (KOBO_TD_Items)themedata.get(td, 4);
		x += themedata.get(rel_to, 0);
		y += themedata.get(rel_to, 1);
	}
	w->place(x, y, themedata.get(td, 2), themedata.get(td, 3));
}


void KOBO_main::init_dash_layout()
{
	// Dashboard framework
	wdash->place(xoffs, yoffs, SCREEN_WIDTH, SCREEN_HEIGHT);

	// Console window: Score
	place(dhigh, KOBO_D_DASH_HIGH);
	dhigh->font(B_NORMAL_FONT);
	dhigh->caption("HIGHSCORE");
	dhigh->text("000000000");

	place(dscore, KOBO_D_DASH_SCORE);
	dscore->font(B_NORMAL_FONT);
	dscore->caption("SCORE");
	dscore->text("000000000");

	// Console window: Weapon slots
	for(int i = 0; i < WEAPONSLOTS; ++i)
	{
		place(wslots[i], (KOBO_TD_Items)(KOBO_D_DASH_WSLOT1 + i));
		wslots[i]->slot(i);
	}

	// Console window: Game progress
	place(dregion, KOBO_D_DASH_REGION);
	dregion->font(B_NORMAL_FONT);
	dregion->caption("REGION");
	dregion->text("-");

	place(dlevel, KOBO_D_DASH_LEVEL);
	dlevel->font(B_NORMAL_FONT);
	dlevel->caption("LEVEL");
	dlevel->text("-");

	// Ship health bar
	place(whealth, KOBO_D_DASH_HEALTH);
	whealth->set_leds(B_HEALTHLEDS);

	// Weapon charge bar
	place(wcharge, KOBO_D_DASH_CHARGE);
	wcharge->set_leds(B_CHARGELEDS);

	// Scrolling backdrop
	place(wbackdrop, KOBO_D_DASH_MAIN);

	// Spinning planet backdrop (placed by dashboard_window_t::mode())
	wplanet->track_layer(LAYER_PLANET);

	// Main playfield layer
	place(wmain, KOBO_D_DASH_MAIN);

	// Fire/smoke overlay
	wfire->SetWorldSize(WORLD_SIZEX, WORLD_SIZEY);
	wfire->SetViewMargin(FIRE_VIEW_MARGIN);
	place(wfire, KOBO_D_DASH_MAIN);

	// Playfield overlay
	place(woverlay, KOBO_D_DASH_MAIN);

	// Set up the map at 1 physical pixel per tile
	wmap->offscreen();
	wmap->scale(1.0f, 1.0f);
	wmap->place(0, 0, MAP_SIZEX, MAP_SIZEY);

	// Have the radar window scale up to 2x2 "native" pixels per tile
	place(wradar, KOBO_D_DASH_RADAR);
	wradar->scale(-2.0f, -2.0f);

	// Indicator LEDs around the playfield window
	pxtop->set_leds(B_TOPLEDS);
	pxbottom->set_leds(B_BOTTOMLEDS);
	pxleft->set_leds(B_LEFTLEDS);
	pxright->set_leds(B_RIGHTLEDS);
	place(pxtop, KOBO_D_DASH_TOPLEDS);
	place(pxbottom, KOBO_D_DASH_BOTTOMLEDS);
	place(pxleft, KOBO_D_DASH_LEFTLEDS);
	place(pxright, KOBO_D_DASH_RIGHTLEDS);

	// Sound design tools
	place(st_hotkeys, KOBO_D_SOUNDTOOLS_HOTKEYS);
	st_hotkeys->color(wdash->map_rgb(16, 16, 16));
	st_hotkeys->font(B_TOOL_FONT);
	st_hotkeys->caption(
			"F1: Reload sounds\n"
			"CTRL+F1: Audio restart\n"
			"F2/F3: Next/prev SFX\n"
			"\r"
			"F4: Play detached\n"
			"F5: Play while holding\n"
			"F6: Start\n"
			"F7: Stop\n"
			"F8: Kill all sounds\n"
			"\r"
			"F9-F12: Send(2, 0.25-1.0)");
	place(st_symname, KOBO_D_SOUNDTOOLS_SYMNAME);
	st_symname->color(wdash->map_rgb(24, 24, 24));
	st_symname->font(B_TOOL_FONT);
	st_symname->caption("SFX Symbol Name");
}


int KOBO_main::init_display(prefs_t *p)
{
	int dw, dh;		// Display size
	int gw, gh;		// Game "window" size
	int desktopres = 0;

	gengine->hide();

	gengine->title("Kobo Redux " KOBO_VERSION_STRING, "kobord");
	gengine->vsync(p->vsync);
	gengine->cursor(0);

	vmm_Init();

	// Hack to force windowed mode if the config has fullscreen == 0
	if(!p->fullscreen)
	{
		switch((VMM_ModeID)p->videomode)
		{
		  case VMID_DESKTOP:
		  case VMID_FULLWINDOW:
			gengine->switch_video_mode(KOBO_VIDEOMODE_WINDOWED);
			break;
		  default:
			break;
		}
	}

	gengine->mode((VMM_ModeID)p->videomode, p->fullscreen);

	VMM_Mode *vm = vmm_GetMode(p->videomode);
	if(vm && (vm->id != VMID_CUSTOM))
	{
		log_printf(WLOG, "Video mode: %s (%x)\n", vm->name,
				p->videomode);
		if(vm->width && vm->height)
		{
			// Specific resolution
			dw = vm->width;
			dh = vm->height;
			log_printf(WLOG, "Video mode size: %d x %d\n", dw, dh);
		}
		else
		{
			// Desktop resolution. To do this, we need to open the
			// engine *first*, to find out what resolution we're
			// actually using!
			if(gengine->open(ENEMY_MAX) < 0)
				return -1;
			dw = gengine->width();
			dh = gengine->height();
			log_printf(WLOG, "Desktop size: %d x %d\n", dw, dh);
			desktopres = 1;
		}
	}
	else
	{
		// Custom resolution
		dw = p->width;
		dh = p->height;
		if(dw <= 0)
			dw = 640;
		if(dh <= 0)
			dh = 360;
		log_printf(WLOG, "Requested size: %d x %d\n", dw, dh);
	}

	// This game assumes 1:1 pixel aspect ratio, or 16:9
	// width:height ratio, so we need to adjust accordingly.
	//
	// NOTE:
	//	This code assumes 1:1 pixels for all resolutions!
	//	This does not hold true for 1280x1024 on a CRT
	//	for example, but that's an incorrect (although
	//	all too common) setup anyway. (1280x1024 is for
	//	5:4 TFT displays only!)
	if(dw * 9 >= dh * 16)
	{
		// 16:9 or wider; Height defines size
		gw = dh * 16 / 9;
		gh = dh;
	}
	else
	{
		// "tallscreen" (4:3 or rotated display)
		// Width defines size
		gw = dw;
		gh = dw * 9 / 16;
	}
	log_printf(WLOG, "Aspect corrected display size: %d x %d\n", gw, gh);

	// Kobo Redux only allows integer scaling factors above 1:1, and
	// shrinking is restricted to 16ths granularity, to guarantee an
	// integer sprite/tile size.
	if((gw >= SCREEN_WIDTH) && (gh >= SCREEN_HEIGHT))
		gengine->scale(floor(gw / SCREEN_WIDTH),
				floor(gh / SCREEN_HEIGHT));
	else
		gengine->scale((int)((gw * 16 + 8) / SCREEN_WIDTH) / 16.f,
				(int)((gh * 16 + 8) / SCREEN_HEIGHT) / 16.f);
	log_printf(WLOG, "Graphics scale factors: %f x %f\n",
			gengine->xscale(), gengine->yscale());

	// Read back and recalculate, in case the engine has some ideas...
	gw = (int)(SCREEN_WIDTH * gengine->xscale() + 0.5f);
	gh = (int)(SCREEN_HEIGHT * gengine->yscale() + 0.5f);
	log_printf(WLOG, "Recalculated display size: %d x %d\n", gw, gh);

	if(!p->fullscreen && !desktopres)
	{
		//Add thin black border in windowed mode.
		dw += 8;
		dh += 8;
		log_printf(WLOG, "Window border added: %d x %d\n", dw, dh);
	}

#ifdef ENABLE_TOUCHSCREEN
	gengine->setup_pointer_margin(dw, dh);
#endif

	if(!desktopres)
		gengine->size(dw, dh);

	// Center the framework on the screen/in the window
	xoffs = (int)((dw - gw) / (2 * gengine->xscale()) + 0.5f);
	yoffs = (int)((dh - gh) / (2 * gengine->yscale()) + 0.5f);
	log_printf(WLOG, "Display offsets: %d, %d\n", xoffs, yoffs);

	gengine->period(game.speed);
	gengine->timefilter(p->timefilter * 0.01f);
	gengine->interpolation(p->filter);

	gengine->scroll_ratio(LAYER_OVERLAY, 0.0f, 0.0f);
	gengine->scroll_ratio(LAYER_BULLETS, 1.0f, 1.0f);
	gengine->scroll_ratio(LAYER_FX, 1.0f, 1.0f);
	gengine->scroll_ratio(LAYER_PLAYER, 1.0f, 1.0f);
	gengine->scroll_ratio(LAYER_ENEMIES, 1.0f, 1.0f);
	gengine->scroll_ratio(LAYER_BASES, 1.0f, 1.0f);
	gengine->wrap(WORLD_SIZEX, WORLD_SIZEY);

	if(!desktopres)
		if(gengine->open(ENEMY_MAX) < 0)
			return -1;

	wscreen = new screen_window_t(gengine);
	wscreen->place(0, 0,
			(int)(gengine->width() / gengine->xscale() + 0.5f),
			(int)(gengine->height() / gengine->yscale() + 0.5f));
	wscreen->border((int)(yoffs * gengine->yscale() + 0.5f),
			(int)(xoffs * gengine->xscale() + 0.5f),
			dw - gw - (int)(xoffs * gengine->xscale() + 0.5f),
			dh - gh - (int)(yoffs * gengine->yscale() + 0.5f));

	wdash = new dashboard_window_t(gengine);
	whealth = new shieldbar_t(gengine);
	wcharge = new chargebar_t(gengine);
	wbackdrop = new backdrop_t(gengine);
	wplanet = new spinplanet_t(gengine);
	wmain = new engine_window_t(gengine);
	wfire = new KOBO_Fire(gengine);
	woverlay = new window_t(gengine);
	dhigh = new display_t(gengine);
	dscore = new display_t(gengine);
	for(int i = 0; i < WEAPONSLOTS; ++i)
		wslots[i] = new weaponslot_t(gengine);
	wmap = new KOBO_radar_map(gengine);
	wradar = new KOBO_radar_window(gengine);
	dregion = new display_t(gengine);
	dlevel = new display_t(gengine);
	pxtop = new hledbar_t(gengine);
	pxbottom = new hledbar_t(gengine);
	pxleft = new vledbar_t(gengine);
	pxright = new vledbar_t(gengine);
	st_hotkeys = new label_t(gengine);
	st_symname = new display_t(gengine);

	init_dash_layout();
	screen.init_graphics();
	wdash->mode(DASHBOARD_BLACK);

	return 0;
}


void KOBO_main::close_display()
{
	delete st_hotkeys;	st_hotkeys = NULL;
	delete st_symname;	st_symname = NULL;

	delete pxright;		pxright = NULL;
	delete pxleft;		pxleft = NULL;
	delete pxbottom;	pxbottom = NULL;
	delete pxtop;		pxtop = NULL;
	delete dlevel;		dlevel = NULL;
	delete dregion;		dregion = NULL;
	delete wradar;		wradar = NULL;
	delete wmap;		wmap = NULL;
	for(int i = 0; i < WEAPONSLOTS; ++i)
	{
		delete wslots[i];
		wslots[i] = NULL;
	}
	delete dscore;		dscore = NULL;
	delete dhigh;		dhigh = NULL;
	delete woverlay;	woverlay = NULL;
	delete wfire;		wfire = NULL;
	delete wmain;		wmain = NULL;
	delete wplanet;		wplanet = NULL;
	delete wbackdrop;	wbackdrop = NULL;
	delete wcharge;		wcharge = NULL;
	delete whealth;		whealth = NULL;
	delete wdash;		wdash = NULL;
	delete wscreen;		wscreen = NULL;
}


void KOBO_main::noiseburst()
{
	if(prefs->quickstart || prefs->cmd_warp)
		return;

	sound.timestamp_reset();
	sound.ui_noise(S_UI_LOADER);
	wdash->fade(0.0f);
	wdash->mode(DASHBOARD_NOISE);
	int t0 = SDL_GetTicks();
	while(1)
	{
		int t = SDL_GetTicks();
		if(SDL_TICKS_PASSED(t, t0 + KOBO_NOISEBURST_DURATION))
			break;
		wdash->fade(sin(M_PI * (t - t0) / KOBO_NOISEBURST_DURATION) *
				3.0f);
		gengine->present();
	}
	wdash->fade(0.0f);
	sound.timestamp_reset();
	sound.ui_noise(0);
}


int KOBO_main::load_palette()
{
	// Hardwired fallback palette, to use before a theme has been loaded
	KOBO_ThemeParser tp(themedata);
	const char *fn;
	if(!(fn = fmap->get("GFX>>loader/DO64-0.24.gpl")))
	{
		log_printf(ELOG, "Couldn't find fallback palette!\n");
		return -1;
	}
	if(!gengine->load_palette(KOBO_P_LOADER, fn) ||
			!gengine->load_palette(KOBO_P_MAIN, fn))
	{
		log_printf(ELOG, "Couldn't load fallback palette!\n");
		return -2;
	}
	tp.parse("palette P_LOADER_NOISE P_LOADER 0 1 35 53 33 54 32 55");
	return 0;
}


void KOBO_main::free_themes()
{
	while(themes)
	{
		KOBO_ThemeData *td = themes;
		themes = td->next;
		delete td;
	}
}


void KOBO_main::discover_themes(const char *ref)
{
	if(!ref)
	{
		free_themes();
		discover_themes("GFX>>");
		discover_themes("SFX>>");
		return;
	}

	KOBO_ThemeData *last_td = themes;
	while(last_td && last_td->next)
		last_td = last_td->next;
	fmap->list_begin(ref, FM_DIR);
	while(1)
	{
		const char *path = fmap->list_next(FM_DIR);
		if(!path)
			break;
		KOBO_ThemeData *td = new KOBO_ThemeData;
		KOBO_ThemeParser tp(*td);
		if(!tp.examine(path))
		{
			log_printf(WLOG, "  %s is not a valid theme!\n", path);
			delete td;
			continue;
		}
		if(last_td)
			last_td = last_td->next = td;
		else
			last_td = themes = td;
	}
}


KOBO_ThemeData *KOBO_main::get_next_theme(const char *type, KOBO_ThemeData *td)
{
	if(!td)
		td = themes;
	else
		td = td->next;
	if(type)
		for( ; td; td = td->next)
		{
			const char *tt = td->get_string(KOBO_D_THEMETYPE);
			if(!tt)
				continue;	// No type? Skip!
			if(strcmp(type, tt) == 0)
				break;		// Found one!
		}
	return td;
}


void KOBO_main::list_themes()
{
	if(themes)
		log_printf(ULOG, "Discovered themes:\n");
	else
		log_printf(WLOG, "No themes discovered!\n");
	for(KOBO_ThemeData *td = themes; td; td = td->next)
	{
		log_printf(ULOG, "  %s -- %s (%s)\n",
				td->get_string(KOBO_D_THEMELABEL, "<error>"),
				td->get_string(KOBO_D_THEMENAME, "<unnamed>"),
				td->get_string(KOBO_D_THEMETYPE, "<unknown>"));
		log_printf(ULOG, "    Path:      \"%s\"\n",
				td->get_string(KOBO_D_THEMEPATH, "<unknown>"));
		log_printf(ULOG, "    Author:    %s <%s>\n",
				td->get_string(KOBO_D_AUTHOR, "<unknown>"),
				td->get_string(KOBO_D_CONTACT,
				"no contact info"));
		log_printf(ULOG, "    Copyright: %s\n",
				td->get_string(KOBO_D_COPYRIGHT,
				"<unspecified>"));
		log_printf(ULOG, "    License:   %s\n",
				td->get_string(KOBO_D_LICENSE,
				"<unspecified>"));
	}
}


int KOBO_main::load_graphics()
{
	KOBO_ThemeParser tp(themedata);

	clear_messages();

	gengine->reset_filters();
	gengine->mark_tiles(prefs->show_tiles);

	// Load the Olofson Arcade Loader graphics theme
	log_printf(ULOG, "Loading loader graphics theme '%s'...\n",
			KOBO_LOADER_GFX_THEME);
	if(!tp.load(KOBO_LOADER_GFX_THEME))
		log_printf(WLOG, "Couldn't load loader graphics theme!\n");

	wdash->show_progress();

	// Try to load fallback graphics theme, if forced. (Normally, an
	// incomplete theme should do this itself using 'fallback'!)
	if(prefs->force_fallback_gfxtheme)
	{
		log_printf(ULOG, "Loading fallback graphics theme '%s' "
				"(forced)...\n", KOBO_FALLBACK_GFX_THEME);
		if(!tp.load(KOBO_FALLBACK_GFX_THEME))
			log_printf(WLOG, "Couldn't load fallback graphics "
					"theme!\n");
	}

	// Try to load custom or default graphics theme
	const char *th = prefs->gfxtheme[0] ? prefs->gfxtheme :
			KOBO_DEFAULT_GFX_THEME;
	log_printf(ULOG, "Loading graphics theme '%s'...\n", th);
	if(!tp.load(th))
	{
		log_printf(WLOG, "Couldn't load graphics theme '%s'!\n", th);
		log_printf(ULOG, "Loading fallback graphics theme '%s'...\n",
				KOBO_FALLBACK_GFX_THEME);
		if(!tp.load(KOBO_FALLBACK_GFX_THEME))
		{
			log_printf(WLOG, "Couldn't load fallback graphics "
					"theme!\n");
			gengine->messagebox("CRITICAL: Could not load game"
					" graphics!");
			return -2;
		}
	}

	// Try to load graphics theme for authoring tools
	th = prefs->toolstheme[0] ? prefs->toolstheme :
			KOBO_DEFAULT_TOOLS_THEME;
	log_printf(ULOG, "Loading tools graphics theme '%s'...\n", th);
	if(!tp.load(th))
		log_printf(WLOG, "Couldn't load tools graphics theme!\n");

	screen.init_graphics();

	// We can try to run with missing graphics, but without the menu font,
	// the user may not even be able to find his/her way out of the game!
	if(!gengine->get_font(B_BIG_FONT))
	{
		gengine->messagebox("CRITICAL: Could not load menu font!");
		return -3;
	}

	return 0;
}


static int progress_cb(const char *msg)
{
	if(msg)
		wdash->doing(msg);
	wdash->progress(0.5f);	// FIXME
	if(km.quit_requested())
		return -999;
	return 0;
}


int KOBO_main::load_sounds(bool progress)
{
	if(!prefs->sound)
		return 0;
	if(progress)
		wdash->show_progress();
	sound.load(KOBO_SB_LOADER, KOBO_LOADER_SFX_THEME, progress_cb);
	if(progress)
		wdash->progress(0.33f);	// FIXME
	if(prefs->force_fallback_sfxtheme)
	{
		log_printf(ULOG, "Loading fallback sfx theme '%s' "
				"(forced)...\n", KOBO_FALLBACK_SFX_THEME);
		if(!sound.load(KOBO_SB_FALLBACK, KOBO_FALLBACK_SFX_THEME,
				progress_cb))
			log_printf(ELOG, "Couldn't load fallback sfx "
					"theme!\n");
	}
	if(progress)
		wdash->progress(0.67f);	// FIXME
	const char *th = prefs->sfxtheme[0] ? prefs->sfxtheme :
			KOBO_DEFAULT_SFX_THEME;
	log_printf(ULOG, "Loading sfx theme '%s'...\n", th);
	if(!sound.load(KOBO_SB_MAIN, th, progress_cb) &&
			!prefs->force_fallback_sfxtheme)
	{
		log_printf(ELOG, "Couldn't load sfx theme '%s'!\n", th);
		// If we haven't already loaded the fallback theme, load it if
		// loading the intended theme fails!
		log_printf(ULOG, "Loading falback sfx theme '%s'...\n",
				KOBO_FALLBACK_SFX_THEME);
		if(!sound.load(KOBO_SB_FALLBACK, KOBO_FALLBACK_SFX_THEME,
				progress_cb))
			log_printf(ELOG, "Couldn't load fallback sfx "
					"theme!\n");
	}
	if(progress)
		wdash->progress(1.0f);	// FIXME
	return 0;
}


int KOBO_main::init_js(prefs_t *p)
{
	/* Activate Joystick sub-sys if we are using it */
	if(p->joystick)
	{
		if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
		{
			log_printf(ELOG, "Error setting up joystick!\n");
			return -1;
		}
		p->number_of_joysticks = SDL_NumJoysticks();
		if(p->number_of_joysticks > 0)
		{
			SDL_JoystickEventState(SDL_ENABLE);
			if(p->joystick_index >= p->number_of_joysticks)
				p->joystick_index = 0;
			joystick = SDL_JoystickOpen(p->joystick_index);
			if(!joystick)
			{
				SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
				return -2;
			}
		}
		else
		{
			log_printf(ELOG, "No joysticks found!\n");
			joystick = NULL;
			return -3;
		}

	}
	return 0;
}


void KOBO_main::close_js()
{
	if(!SDL_WasInit(SDL_INIT_JOYSTICK))
		return;

	if(!joystick)
		return;

	if(joystick)
		SDL_JoystickClose(joystick);
	joystick = NULL;

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}


void KOBO_main::load_config(prefs_t *p)
{
	const char *path = "<not set>";
	FILE *f = fmap->fopen("CONFIG>>" KOBO_CONFIGFILE, "r", &path);
	if(f)
	{
		log_printf(VLOG, "Loading configuration from \"%s\".\n", path);
		p->read(f);
		fclose(f);
	}
#ifdef KOBO_SYSCONFDIR
	/*
	 * On Unixen, where they have SYSCONFDIR (usually /etc) try to get
	 * the default configuration from a file stored there before
	 * giving up.
	 *
	 * This gives packagers a chance to provide a proper default
	 * (playable) configuration for all those little Linux-based
	 * gadgets that are flying around.
	 */
	else
	{
		log_puts(VLOG, "Personal configuration not found. Trying "
				"system default-config.");
		f = fmap->fopen(KOBO_SYSCONFDIR "/koboredux/default-config",
				"r");
		if(f)
		{
			log_puts(VLOG, "Loading configuration defaults from: "
					KOBO_SYSCONFDIR
					"/koboredux/default-config");
			p->read(f);
			fclose(f);
		}
		else
			log_puts(VLOG, "System default-config not found. "
					"Using built-in defaults.");
	}
#endif
}


void KOBO_main::save_config(prefs_t *p)
{
	FILE *f;
#if defined(HAVE_GETEGID) && defined(HAVE_SETGID)
	gid_t oldgid = getegid();
	if(setgid(getgid()) != 0)
	{
		log_printf(ELOG, "Cannot save config! (%s)\n",
				strerror(errno));
		return;
	}
#endif
	const char *path = "<not set>";
	f = fmap->fopen("CONFIG>>" KOBO_CONFIGFILE, "w", &path);
	if(f)
	{
		log_printf(VLOG, "Saving configuration to \"%s\".\n", path);
		p->write(f);
		fclose(f);
	}
#if defined(HAVE_GETEGID) && defined(HAVE_SETGID)
	if(setgid(oldgid) != 0)
	{
		log_printf(ELOG, "Cannot restore GID! (%s)\n",
				strerror(errno));
		return;
	}
#endif
}


void KOBO_main::quit()
{
	exit_game = 1;
}


void KOBO_main::brutal_quit(bool force)
{
	if(exit_game_fast || force)
	{
		log_printf(ULOG, "Second try quitting; using brutal "
				"method!\n");
		atexit(SDL_Quit);
		close_logging(true);
		exit(1);
	}

	exit_game_fast = 1;
	if(gengine)
		gengine->stop();
}


void KOBO_main::pause_game()
{
	// Only (auto)pause if playing, and not already in the UI pause state!
	if(gsm.current() == &st_pause_game)
		return;
	if(manage.replay_mode() != RPM_PLAY)
		return;
	gsm.pressbtn(BTN_PAUSE);
}


static void kobo_render_highlight(ct_widget_t *wg)
{
	screen.set_highlight(wg->py() + wg->height() / 2 - woverlay->py(),
			wg->height());
}


int KOBO_main::open()
{
	if(init_display(prefs) < 0)
		return -1;

	sound.open();
	load_sounds();

	load_palette();
	noiseburst();

	if(!(prefs->quickstart || prefs->cmd_warp))
		sound.jingle(S_OAJINGLE);
	int jtime = SDL_GetTicks() + 4000;

	if(load_graphics() < 0)
		return -2;

	wdash->progress_done();

	if(!(prefs->quickstart || prefs->cmd_warp))
	{
		wdash->mode(DASHBOARD_JINGLE);
		while(!SDL_TICKS_PASSED(SDL_GetTicks(), jtime) &&
				!skip_requested())
			gengine->present();

		wdash->mode(DASHBOARD_TITLE, DASHBOARD_SLOW);
		while(wdash->busy())
			gengine->present();
	}
	else
		wdash->mode(DASHBOARD_TITLE);

	sound.timestamp_reset();
	init_dash_layout();
	ct_engine.render_highlight = kobo_render_highlight;
	wradar->mode(RM_NOISE);
	pubrand.init();
	init_js(prefs);
	gamecontrol.init();
	manage.init();

	gsm.push(&st_intro);

	if(prefs->cmd_warp)
	{
		log_printf(ULOG, "Warping to stage %d!\n", prefs->cmd_warp);
		manage.select_slot(-1);		// No saves!
		manage.select_skill((skill_levels_t)prefs->cmd_skill);
		manage.select_stage(prefs->cmd_warp, GS_SHOW);
		manage.start_new_game();
		gsm.push(&st_game);
	}

	return 0;
}


void KOBO_main::close()
{
	free_themes();
	clear_messages();
	close_js();
	close_display();
	if(gengine)
	{
		delete gengine;
		gengine = NULL;
	}
	sound.close();
	SDL_Quit();
}


int KOBO_main::restart_audio()
{
	if(!prefs->sound)
	{
		log_printf(ULOG, "--- Stopping audio...\n");
		sound.close();
		log_printf(ULOG, "--- Audio stopped.\n");
		return 0;
	}

	log_printf(ULOG, "--- Restarting audio...\n");
	sound.close();
	if(sound.open() < 0)
		return 4;
	load_sounds();
	wdash->progress_done();
	dashboard_modes_t dmd = wdash->mode();
	wdash->mode(DASHBOARD_BLACK);
	log_printf(ULOG, "--- Audio restarted.\n");
	wdash->fade(1.0f);
	wdash->mode(dmd);
	wradar->mode(RM__REINIT);
	enemies.restart_sounds();
	return 0;
}


int KOBO_main::restart_video()
{
	log_printf(ULOG, "--- Restarting video...\n");
	wdash->mode(DASHBOARD_BLACK);
	gengine->hide();
	close_display();
	gengine->unload();
	safe_prefs = *prefs;
	if(init_display(prefs) < 0)
	{
		log_printf(ELOG, "--- Video init failed!\n");
		*prefs = safe_prefs;
		if(init_display(prefs) < 0)
		{
			log_printf(ELOG, "--- Video restore failed! "
					"Giving up.\n");
			return 6;
		}
		st_error.message("Video initialization failed!",
				"Try different settings.");
		gsm.push(&st_error);
	}
	init_dash_layout();
	screen.init_graphics();
	gamecontrol.init();
	gsm.rebuild();
	log_printf(ULOG, "--- Video restarted.\n");

	// FIXME: We may not always need to reload graphics, but that's a bit
	// tricky to tell for sure, so we just take the easy, safe path here.
	global_status |= OS_RELOAD_GRAPHICS;
	return 0;
}


int KOBO_main::reload_sounds()
{
	if(!prefs->sound)
	{
		log_printf(ULOG, "--- Audio off - not reloading sounds!\n");
		return 0;
	}

	log_printf(ULOG, "--- Reloading sounds...\n");
	sound.unload(KOBO_SB_ALL);
	load_sounds(false);
	log_printf(ULOG, "--- Sounds reloaded.\n");
	enemies.restart_sounds();
	return 0;
}


int KOBO_main::reload_graphics()
{
	wdash->mode(DASHBOARD_BLACK);
	gengine->unload();
	log_printf(ULOG, "--- Reloading graphics...\n");
	if(load_graphics() < 0)
		return 7;
	wdash->progress_done();
	init_dash_layout();
	screen.init_graphics();
	wradar->mode(RM__REINIT);
	gsm.rebuild();
	log_printf(ULOG, "--- Graphics reloaded.\n");
	return 0;
}


int KOBO_main::run()
{
	while(1)
	{
		global_status = 0;

		// Run the main loop!
		gengine->run();

		// Exit game?
		if(exit_game_fast)
			break;
		if(exit_game)
		{
			sound.music(-1);
			manage.abort_game();
			if(wdash)
				wdash->mode(DASHBOARD_BLACK);
			break;
		}

		// Restart and reload stuff as needed
		int res;
		sound.ui_noise(0);
		dashboard_modes_t dmd = wdash->mode();

		if(global_status & OS_RESTART_AUDIO)
			if((res = restart_audio()))
				return res;
		if(global_status & OS_RELOAD_SOUNDS)
#if 0
// FIXME: This is unreliable! A2 bug? See Kobo Redux issue #370.
			if((res = reload_sounds()))
#else
			if((res = restart_audio()))
#endif
				return res;
		if(global_status & OS_RESTART_VIDEO)
			if((res = restart_video()))
				return res;
		if(global_status & OS_RELOAD_GRAPHICS)
			if((res = reload_graphics()))
				return res;
		if(global_status & OS_RESTART_INPUT)
		{
			close_js();
			init_js(prefs);
			gamecontrol.init();
		}
		if(global_status & OS_RESTART_LOGGER)
			open_logging(prefs);

		if(dmd != wdash->mode())
		{
			wdash->fade(1.0f);
			wdash->mode(dmd);
		}

		// Prepare to reenter the main loop
		km.pause_game();
		manage.reenter();
	}
	return 0;
}


/*----------------------------------------------------------
	Kobo Graphics Engine
----------------------------------------------------------*/

kobo_gfxengine_t::kobo_gfxengine_t()
{
#ifdef ENABLE_TOUCHSCREEN
	pointer_margin_used = false;
	pointer_margin_width_min = 0;
	pointer_margin_width_max = 0;
	pointer_margin_height_min = 0;
	pointer_margin_height_max = 0;
#endif
	st_sound = 0;
	st_handle = 0;
	st_x = st_y = 0;
}


#ifdef ENABLE_TOUCHSCREEN
void kobo_gfxengine_t::setup_pointer_margin(int dw, int dh)
{
	// Precalculates the border ranges. Mouse clicks outside these are
	// handled specially.
	pointer_margin_width_min = dw * POINTER_MARGIN_PERCENT / 100;
	pointer_margin_width_max = dw - dw * POINTER_MARGIN_PERCENT / 100;
	pointer_margin_height_min = dh * POINTER_MARGIN_PERCENT / 100;
	pointer_margin_height_max = dh - dh * POINTER_MARGIN_PERCENT / 100;

	log_printf(VLOG, "Pointer margin range [%d, %d, %d, %d]\n",
			pointer_margin_width_min,
			pointer_margin_width_max,
			pointer_margin_height_min,
			pointer_margin_height_max);
}
#endif


void kobo_gfxengine_t::switch_video_mode(kobo_vmswitch_t vms)
{
	VMM_Mode *vm = vmm_GetMode(prefs->videomode);
	VMM_Mode *dvm = vmm_GetMode(VMID_DESKTOP);

	// Force safe mode if we can't get modes!
	if(!vm || !dvm)
		vms = KOBO_VIDEOMODE_SAFE;
	else if(vms == KOBO_VIDEOMODE_TOGGLE)
	{
		if(prefs->fullscreen || (vm->flags & VMM_DESKTOP))
			vms = KOBO_VIDEOMODE_WINDOWED;
		else
			vms = KOBO_VIDEOMODE_FULLSCREEN;
	}

	switch(vms)
	{
	  case KOBO_VIDEOMODE_TOGGLE:
		// Wut?
		return;

	  case KOBO_VIDEOMODE_WINDOWED:
	  {
		// Fullscreen or similar; switch to a "reasonably sized" window
		int maxw = dvm->width * 75 / 100;
		int maxh = dvm->height * 75 / 100;
		VMM_Mode *bestm = NULL;

		// Find largest 16:9 mode that fits in 75% of the desktop size
		for(vm = NULL; (vm = vmm_FindNext(vm, VMM_16_9, VMM_DESKTOP,
				maxw, maxh)); )
			if(!bestm || (vm->width >= bestm->width) ||
					(vm->height >= bestm->height))
				bestm = vm;
		if(!bestm)
		{
			// Could not find an appropriate mode!
			switch_video_mode(KOBO_VIDEOMODE_SAFE);
			return;
		}

		if(!prefs->fullscreen && (prefs->videomode == bestm->id))
			return;	// No changes needed!

		prefs->fullscreen = 0;
		prefs->videomode = bestm->id;
		break;
	  }
	  case KOBO_VIDEOMODE_FULLSCREEN:
		// Only ever use desktop resolution fullscreen mode here!
		// Anything else is way too likely to cause trouble these days,
		// so users will just have to select those modes manually.
		if(prefs->fullscreen && (prefs->videomode == VMID_DESKTOP))
			return;	// No changes needed!

		prefs->fullscreen = 1;
		prefs->videomode = VMID_DESKTOP;
		break;

	  case KOBO_VIDEOMODE_SAFE:
		// Always force a restart/reload when selecting this mode!
		prefs->fullscreen = 0;
		prefs->videomode = 0x10920;
		break;
	}

	prefs->changed = 1;
	global_status |= OS_RELOAD_GRAPHICS | OS_RESTART_VIDEO;
	stop();
}


float kobo_gfxengine_t::timestamp_delay()
{
	return gengine->period() + prefs->tsdelay;
}


void kobo_gfxengine_t::st_update_sound_displays()
{
	if(!prefs->soundtools)
		return;
	log_printf(ULOG, "Selected sound %s\n", sound.symname(st_sound));
	km.st_symname->text(sound.symname(st_sound));
}


void kobo_gfxengine_t::pre_loop()
{
	st_update_sound_displays();
	sound.timestamp_reset();
	sound.timestamp_bump(timestamp_delay() +
			1000.0f / (prefs->maxfps > 60 ? 60 : prefs->maxfps));
}


bool kobo_gfxengine_t::soundtools_event(SDL_Event &ev)
{
	if((ev.type != SDL_KEYDOWN) && (ev.type != SDL_KEYUP))
		return false;	// Only key up/down!

	int ms = SDL_GetModState();
	if(ms & (KMOD_ALT | KMOD_SHIFT | KMOD_GUI))
		return false;	// No qualifiers!

	// CTRL + F1: Restart audio engine
	if(ms & KMOD_CTRL)
	{
		if((ev.type == SDL_KEYDOWN) && (ev.key.keysym.sym == SDLK_F1))
		{
			if(ev.key.repeat)
				return true;
			global_status |= OS_RESTART_AUDIO;
			stop();
			return true;
		}
		else
			return false;
	}

	switch (ev.type)
	{
	  case SDL_KEYDOWN:
		switch(ev.key.keysym.sym)
		{
		  case SDLK_F1:		// Reload sounds
			if(ev.key.repeat)
				return true;
			km.reload_sounds();
			return true;
		  case SDLK_F2:		// Select prev sound
			--st_sound;
			if(st_sound < 1)
				st_sound = S__COUNT - 1;
			st_update_sound_displays();
			return true;
		  case SDLK_F3:		// Select next sound
			++st_sound;
			if(st_sound >= S__COUNT)
				st_sound = 1;
			st_update_sound_displays();
			return true;
		  case SDLK_F4:		// Play detached
			if(st_handle)
			{
				sound.g_stop(st_handle);
				st_handle = 0;
			}
			st_x = myship.get_x();
			st_y = myship.get_y();
			sound.g_play(st_sound, st_x, st_y);
			return true;
		  case SDLK_F5:		// Play while holding key; press
			if(ev.key.repeat)
				return true;
			/* Fall-through! */
		  case SDLK_F6:		// Start
			if(st_handle)
				sound.g_stop(st_handle);
			st_x = myship.get_x();
			st_y = myship.get_y();
			st_handle = sound.g_start(st_sound, st_x, st_y);
			return true;
		  case SDLK_F7:		// Stop
			if(ev.key.repeat)
				return true;
			if(st_handle)
			{
				sound.g_stop(st_handle);
				st_handle = 0;
			}
			return true;
		  case SDLK_F8:		// Kill all sounds
			if(ev.key.repeat)
				return true;
			sound.g_new_scene();
			return true;
		  case SDLK_F9:		// Deal 25% damage
			sound.g_control(st_handle, 2, .25f);
			return true;
		  case SDLK_F10:	// Deal 50% damage
			sound.g_control(st_handle, 2, .5f);
			return true;
		  case SDLK_F11:	// Deal 75% damage
			sound.g_control(st_handle, 2, .75f);
			return true;
		  case SDLK_F12:	// Deal 100% damage
			sound.g_control(st_handle, 2, 1.0f);
			return true;
		  default:
			return false;
		}
	  case SDL_KEYUP:
		switch(ev.key.keysym.sym)
		{
		  case SDLK_F1:
		  case SDLK_F2:
		  case SDLK_F3:
		  case SDLK_F4:
			return true;
		  case SDLK_F5:		// Play while holding key; release
			if(st_handle)
			{
				sound.g_stop(st_handle);
				st_handle = 0;
			}
			return true;
		  case SDLK_F6:
		  case SDLK_F7:
		  case SDLK_F8:
		  case SDLK_F9:
		  case SDLK_F10:
		  case SDLK_F11:
		  case SDLK_F12:
			return true;
		  default:
			return false;
		}
	  default:
		return false;
	}
}


void kobo_gfxengine_t::mouse_motion(SDL_Event &ev)
{
	mouse_x = (int)(ev.motion.x / gengine->xscale()) - km.xoffs;
	mouse_y = (int)(ev.motion.y / gengine->yscale()) - km.yoffs;
	mouse_visible = true;
	mouse_timer = SDL_GetTicks();

	gsm.pos(mouse_x, mouse_y);

	if(prefs->mouse)
		// pass the real mouse coordinates too
		gamecontrol.mouse_position(
				mouse_x - DASHX(MAIN) - DASHW(MAIN) / 2,
				mouse_y - DASHY(MAIN) - DASHH(MAIN) / 2);
}


void kobo_gfxengine_t::mouse_button_down(SDL_Event &ev)
{
	static int explo_debug_mode = 0;
	mouse_x = (int)(ev.button.x / gengine->xscale()) - km.xoffs;
	mouse_y = (int)(ev.button.y / gengine->yscale()) - km.yoffs;
	switch(ev.button.button)
	{
	  case SDL_BUTTON_LEFT:
		mouse_left = 1;
		break;
	  case SDL_BUTTON_MIDDLE:
		mouse_middle = 1;
		break;
	  case SDL_BUTTON_RIGHT:
	  {
		if(!prefs->debug)
		{
			mouse_right = 1;
			break;
		}
		KOBO_ParticleFXDef *fxd = NULL;
		switch(explo_debug_mode++)
		{
		  default:
			explo_debug_mode = 1;
		  case 0:
			// Basic default; single system
			fxd = new KOBO_ParticleFXDef;
			fxd->Default();
			break;
		  case 1:
			// Cluster of systems
			fxd = new KOBO_ParticleFXDef;
			fxd->Default();
			fxd->init_count = 1024;
			fxd->radius.Set(15.0f, 20.0f, 0.0f);
			fxd->twist.Set(0.0f, 0.0f);
			fxd->speed.Set(5.0f, 7.0f, 0.0f);
			fxd->drag.Set(0.9f, 0.9f);
			fxd->heat.Set(0.5f, 0.5f, 0.5f);
			fxd->fade.Set(0.93f, 0.93f);
			for(int i = 0; i < 10; ++i)
			{
				KOBO_ParticleFXDef *fxd2 = fxd->Add();
				fxd2->Default();
				fxd2->init_count = 100;
				fxd2->radius.Set(1.0f, 3.0f, 0.0f);
				fxd2->xoffs.Set(-20.0f, 20.0f);
				fxd2->yoffs.Set(-20.0f, 20.0f);
			}
			break;
		  case 2:
		  {
			// Nested systems
			fxd = new KOBO_ParticleFXDef;
			fxd->Reset();
			fxd->init_count = 10;
			fxd->radius.Set(10.0f, 10.0f, 0.0f);
			fxd->speed.Set(10.0f, 10.0f, 0.0f);
			KOBO_ParticleFXDef *fxd2 = fxd->Child();
			fxd2->Default();
			fxd2->init_count = 100;
			fxd2->radius.Set(1.0f, 2.0f, 0.0f);
			break;
		  }
		}
		wfire->Spawn(PIXEL2CS(mouse_x - DASHX(MAIN)) +
				gengine->xoffs(LAYER_BASES),
				PIXEL2CS(mouse_y - DASHY(MAIN)) +
				gengine->yoffs(LAYER_BASES),
				0, -300, fxd);
		delete fxd;
		return;
	  }
	}

	// Ingame mouse control, if enabled
	if(prefs->mouse)
		switch(ev.button.button)
		{
		  case SDL_BUTTON_LEFT:
			gamecontrol.pressbtn(BTN_PRIMARY, GC_SRC_MOUSE);
			break;
		  case SDL_BUTTON_MIDDLE:
			gamecontrol.pressbtn(BTN_SECONDARY, GC_SRC_MOUSE);
			break;
		  case SDL_BUTTON_RIGHT:
			gamecontrol.pressbtn(BTN_TERTIARY, GC_SRC_MOUSE);
			break;
		}

#ifdef ENABLE_TOUCHSCREEN
	if(ev.button.x <= pointer_margin_width_min)
	{
		gsm.pressbtn(BTN_LEFT);
		pointer_margin_used = true;
	}
	else if(ev.button.x >= pointer_margin_width_max)
	{
		// Upper right corner invokes pause.
		// Lower right corner invokes exit.
		// Otherwise it is just 'right'. :)
		if(ev.button.y <= pointer_margin_height_min)
		{
			gsm.pressbtn(BTN_PAUSE);
			gamecontrol.pressbtn(BTN_PAUSE, GC_SRC_MOUSE);
		}
		else
			gsm.pressbtn((ev.button.y >= pointer_margin_height_max
					? BTN_EXIT : BTN_RIGHT));
		pointer_margin_used = true;
	}
	if(ev.button.y <= pointer_margin_height_min)
	{
		// Handle as 'up' only if it was not in
		// the 'pause' area. Still handle as
		// clicked, so 'fire' will not kick in.
		if(ev.button.x < pointer_margin_width_max)
			gsm.pressbtn(BTN_UP);
		pointer_margin_used = true;
	}
	else if(ev.button.y >= pointer_margin_height_max)
	{
		// Handle as 'down' only if it was not
		// in the 'exit' area. Still handle as
		// clicked, so 'fire' will not kick in.
		if(ev.button.x < pointer_margin_width_max)
			gsm.pressbtn(BTN_DOWN);
		pointer_margin_used = true;
	}
	if(!pointer_margin_used)
		gsm.pressbtn(BTN_FIRE);
#else
	switch(ev.button.button)
	{
	  case SDL_BUTTON_LEFT:
		gsm.pressbtn(BTN_PRIMARY);
		break;
	  case SDL_BUTTON_MIDDLE:
		gsm.pressbtn(BTN_SECONDARY);
		break;
	  case SDL_BUTTON_RIGHT:
		gsm.pressbtn(BTN_TERTIARY);
		break;
	}
#endif
}


void kobo_gfxengine_t::mouse_button_up(SDL_Event &ev)
{
	mouse_x = (int)(ev.button.x / gengine->xscale()) - km.xoffs;
	mouse_y = (int)(ev.button.y / gengine->yscale()) - km.yoffs;
	switch(ev.button.button)
	{
	  case SDL_BUTTON_LEFT:
		mouse_left = 0;
		break;
	  case SDL_BUTTON_MIDDLE:
		mouse_middle = 0;
		break;
	  case SDL_BUTTON_RIGHT:
		mouse_right = 0;
		break;
	}

	// Ingame mouse control, if enabled
	if(prefs->mouse)
		switch(ev.button.button)
		{
		  case SDL_BUTTON_LEFT:
			gamecontrol.releasebtn(BTN_PRIMARY, GC_SRC_MOUSE);
			break;
		  case SDL_BUTTON_MIDDLE:
			gamecontrol.releasebtn(BTN_SECONDARY, GC_SRC_MOUSE);
			break;
		  case SDL_BUTTON_RIGHT:
			gamecontrol.releasebtn(BTN_TERTIARY, GC_SRC_MOUSE);
			break;
		}

	// GUI mouse control
#ifdef ENABLE_TOUCHSCREEN
	// Resets all kinds of buttons that might have been activated by
	// clicking in the pointer margin.
	if(pointer_margin_used)
	{
		gsm.release(BTN_EXIT);
		gsm.release(BTN_LEFT);
		gsm.release(BTN_RIGHT);
		gsm.release(BTN_UP);
		gsm.release(BTN_DOWN);
		pointer_margin_used = false;
	}
#else
	switch(ev.button.button)
	{
	  case SDL_BUTTON_LEFT:
		gsm.releasebtn(BTN_PRIMARY);
		break;
	  case SDL_BUTTON_MIDDLE:
		gsm.releasebtn(BTN_SECONDARY);
		break;
	  case SDL_BUTTON_RIGHT:
		gsm.releasebtn(BTN_TERTIARY);
		break;
	}
#endif
}


void kobo_gfxengine_t::input(float fractional_frame)
{
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		int ms;
		if(ct_engine.rawevent(&ev))
			continue;
		if(prefs->soundtools && soundtools_event(ev))
			continue;
		switch (ev.type)
		{
		  case SDL_KEYDOWN:
			switch(ev.key.keysym.sym)
			{
			  case SDLK_RETURN:
				ms = SDL_GetModState();
				if(ms & (KMOD_CTRL | KMOD_SHIFT | KMOD_GUI))
					break;
				if(!(ms & KMOD_ALT))
					break;
				km.pause_game();
				switch_video_mode(KOBO_VIDEOMODE_TOGGLE);
				return;
			  case SDLK_PRINTSCREEN:
			  case SDLK_SYSREQ:
#if 0
// FIXME: Doesn't this trigger when entering names and stuff...?
// FIXME: Well, now it conflicts with WSAD controls, so... Disabled.
			  case SDLK_s:
#endif
				gengine->screenshot();
				break;
#if 0
			  case SDLK_r:
				manage.start_replay();
				break;
#endif
			  default:
				break;
			}
			gamecontrol.press(ev.key.keysym);
			gsm.press(ev.key.keysym);
			break;
		  case SDL_KEYUP:
			if((ev.key.keysym.sym == SDLK_ESCAPE) &&
					km.escape_hammering())
			{
				if(km.escape_hammering_quit())
					km.brutal_quit(true);
				km.pause_game();
				switch_video_mode(KOBO_VIDEOMODE_SAFE);
				prefs->scalemode = (int)GFX_SCALE_NEAREST;
				prefs->brightness = 100;
				prefs->contrast = 100;
				global_status |= OS_RELOAD_GRAPHICS |
						OS_RESTART_VIDEO;
				stop();
				st_error.message(
					"Safe video settings applied!",
					"Enter Options menu to save.");
				gsm.push(&st_error);
				return;
			}
			gamecontrol.release(ev.key.keysym);
			gsm.release(ev.key.keysym);
			break;
		  case SDL_WINDOWEVENT:
			switch(ev.window.event)
			{
			  case SDL_WINDOWEVENT_SHOWN:
			  case SDL_WINDOWEVENT_EXPOSED:
			  case SDL_WINDOWEVENT_RESIZED:
			  case SDL_WINDOWEVENT_MAXIMIZED:
			  case SDL_WINDOWEVENT_RESTORED:
				break;
			  case SDL_WINDOWEVENT_CLOSE:
				km.quit();
				break;
			  case SDL_WINDOWEVENT_ENTER:
				mouse_visible = true;
				mouse_timer = SDL_GetTicks();
				break;
			  case SDL_WINDOWEVENT_LEAVE:
				mouse_visible = false;
				// Fall-through!
			  case SDL_WINDOWEVENT_HIDDEN:
			  case SDL_WINDOWEVENT_MINIMIZED:
			  case SDL_WINDOWEVENT_FOCUS_LOST:
				// Any type of focus loss should activate
				// pause mode!
				km.pause_game();
				break;
			}
			break;
		  case SDL_QUIT:
			km.quit();
			break;
		  case SDL_JOYBUTTONDOWN:
			if(ev.jbutton.button == km.js_primary)
			{
				gamecontrol.pressbtn(BTN_PRIMARY,
						GC_SRC_JOYSTICK);
				gsm.pressbtn(BTN_PRIMARY);
			}
			else if(ev.jbutton.button == km.js_secondary)
			{
				gamecontrol.pressbtn(BTN_SECONDARY,
						GC_SRC_JOYSTICK);
				gsm.pressbtn(BTN_SECONDARY);
			}
			else if(ev.jbutton.button == km.js_start)
			{
				gamecontrol.pressbtn(BTN_PAUSE,
						GC_SRC_JOYSTICK);
				gsm.pressbtn(BTN_PAUSE);
			}
			break;
		  case SDL_JOYBUTTONUP:
			if(ev.jbutton.button == km.js_primary)
			{
				gamecontrol.releasebtn(BTN_PRIMARY,
						GC_SRC_JOYSTICK);
				gsm.releasebtn(BTN_PRIMARY);
			}
			else if(ev.jbutton.button == km.js_secondary)
			{
				gamecontrol.releasebtn(BTN_SECONDARY,
						GC_SRC_JOYSTICK);
				gsm.releasebtn(BTN_SECONDARY);
			}
			break;
		  case SDL_JOYAXISMOTION:
			// FIXME: We will want to allow these to be
			// redefined, but for now, this works ;-)
			if(ev.jaxis.axis == km.js_lr)
			{
				if(ev.jaxis.value < -3200)
				{
					gamecontrol.pressbtn(BTN_LEFT,
							GC_SRC_JOYSTICK);
					gsm.pressbtn(BTN_LEFT);
				}
				else if(ev.jaxis.value > 3200)
				{
					gamecontrol.pressbtn(BTN_RIGHT,
							GC_SRC_JOYSTICK);
					gsm.pressbtn(BTN_RIGHT);
				}
				else
				{
					gamecontrol.releasebtn(BTN_LEFT,
							GC_SRC_JOYSTICK);
					gamecontrol.releasebtn(BTN_RIGHT,
							GC_SRC_JOYSTICK);
					gsm.releasebtn(BTN_LEFT);
					gsm.releasebtn(BTN_RIGHT);
				}
			}
			else if(ev.jaxis.axis == km.js_ud)
			{
				if(ev.jaxis.value < -3200)
				{
					gamecontrol.pressbtn(BTN_UP,
							GC_SRC_JOYSTICK);
					gsm.pressbtn(BTN_UP);
				}
				else if(ev.jaxis.value > 3200)
				{
					gamecontrol.pressbtn(BTN_DOWN,
							GC_SRC_JOYSTICK);
					gsm.pressbtn(BTN_DOWN);
				}
				else
				{
					gamecontrol.releasebtn(BTN_UP,
							GC_SRC_JOYSTICK);
					gamecontrol.releasebtn(BTN_DOWN,
							GC_SRC_JOYSTICK);
					gsm.releasebtn(BTN_UP);
					gsm.releasebtn(BTN_DOWN);
				}
			}
			break;
		  case SDL_MOUSEMOTION:
			mouse_motion(ev);
			break;
		  case SDL_MOUSEBUTTONDOWN:
			mouse_button_down(ev);
			break;
		  case SDL_MOUSEBUTTONUP:
			mouse_button_up(ev);
			break;
		}
	}
}


void kobo_gfxengine_t::pre_advance(float fractional_frame)
{
	// We need to adjust for the logic time elapsed since the last logic
	// frame (because logic time is decoupled from rendering frame rate),
	// and for the desired "buffer" timestamp delay.
	float ft = fractional_frame * gengine->period();
	sound.timestamp_nudge(ft - timestamp_delay());
}


void kobo_gfxengine_t::frame()
{
	sound.frame();

	if(prefs->soundtools && st_handle)
		sound.g_move(st_handle, st_x, st_y);

	if(!gsm.current())
	{
		log_printf(CELOG, "INTERNAL ERROR: No gamestate!\n");
		km.quit();
		stop();
		return;
	}
	if(km.quitting())
	{
		stop();
		return;
	}

	// Update positional audio listener position
	sound.g_position(CS2PIXEL(gengine->xoffs(LAYER_BASES)) +
			DASHW(MAIN) / 2,
			CS2PIXEL(gengine->yoffs(LAYER_BASES)) +
			DASHH(MAIN) / 2);

	// Run the game engine for one frame
	manage.run();

	// Run the current gamestate (application/UI state) for one frame
	gsm.frame();

	// Bump audio API timestamp time to match game logic time
	sound.timestamp_bump(gengine->period());

	// Screenshot video - "every Nth logic frame" rates
	if((prefs->cmd_autoshot < 0) && (manage.game_in_progress() ||
			(manage.replay_mode() == RPM_REPLAY)))
	{
		++km.ss_frames;
		if(km.ss_frames >= -prefs->cmd_autoshot)
		{
			gengine->screenshot();
			km.ss_frames = 0;
		}
	}

	// Run filter timers, reset pressed()/released() triggering etc
	gamecontrol.frame();

	// Hide mouse cursor after some time, unless we're ingame and using
	// mouse control.
	if(mouse_visible && prefs->mouse_hidetime &&
			(!prefs->mouse || !manage.game_in_progress()))
	{
		if(SDL_TICKS_PASSED(SDL_GetTicks(), mouse_timer +
				prefs->mouse_hidetime))
			mouse_visible = false;
	}
}


void kobo_gfxengine_t::pre_render()
{
}


void kobo_gfxengine_t::pre_sprite_render()
{
	gsm.pre_render();
	gfxengine->target()->font(B_DEBUG_FONT);
	gfxengine->show_coordinates(prefs->show_coordinates);
}


void kobo_gfxengine_t::post_sprite_render()
{
	if(prefs->show_hit)
		enemies.render_hit_zones();
}


void kobo_gfxengine_t::post_render()
{
	// HACK: Custom rendering "callback!" The engine should support this...
	myship.render();

	if(prefs->soundtools)
	{
		// Make sure the GUI is visible
		km.st_hotkeys->visible(true);
		km.st_symname->visible(true);

		// Ear (listener position)
		wmain->sprite(DASHW(MAIN) / 2, DASHH(MAIN) / 2,
				B_SOUND_ICONS, 1);

		// Speaker (sound source position)
		int ssx = PIXEL2CS(st_x) - gengine->xoffs(LAYER_BASES);
		int ssy = PIXEL2CS(st_y) - gengine->yoffs(LAYER_BASES);
		ssx += PIXEL2CS(WORLD_SIZEX);
		ssy += PIXEL2CS(WORLD_SIZEY);
		ssx %= PIXEL2CS(WORLD_SIZEX);
		ssy %= PIXEL2CS(WORLD_SIZEY);
		wmain->sprite_fxp(ssx, ssy, B_SOUND_ICONS, 0);
	}
	else
	{
		// Soundtools enabled and then disabled; hide the GUI!
		km.st_hotkeys->visible(false);
		km.st_symname->visible(false);
	}

	gsm.post_render();
	::screen.render_curtains();
	wdash->render_final();	// Well, almost... Debug + fps still on top!

	if(prefs->debug)
	{
		char buf[20];
		woverlay->font(B_SMALL_FONT);

		// Objects in use
		snprintf(buf, sizeof(buf), "Obj: %d",
				gengine->objects_in_use());
		woverlay->string(120, 1, buf);

		// Particles
		snprintf(buf, sizeof(buf), "PS: %d/%d",
				wfire->StatPSystems(),
				wfire->StatParticles());
		woverlay->string(120, 10, buf);

		// Cores; left/total
		snprintf(buf, sizeof(buf), "Cores: %d/%d",
				manage.cores_remaining(),
				manage.cores_total());
		woverlay->string(180, 1, buf);

		// Mouse cursor position
		snprintf(buf, sizeof(buf), "M(%d, %d)", mouse_x, mouse_y);
		woverlay->string(DASHW(MAIN) - 60, 1, buf);

		// Current map position
		woverlay->font(B_NORMAL_FONT);
		snprintf(buf, sizeof(buf), "(%d, %d)",
				CS2PIXEL(gengine->xoffs(LAYER_BASES)),
				CS2PIXEL(gengine->yoffs(LAYER_BASES)));
		woverlay->string(4, DASHH(MAIN) - 10, buf);
	}

	// Frame rate counter
	int nt = (int)SDL_GetTicks();
	int tt = nt - km.fps_starttime;
	if((tt > 1000) && km.fps_count)
	{
		km.fps_last = km.fps_count * 1000.0 / tt;
		::screen.fps(km.fps_last);
		km.fps_count = 0;
		km.fps_starttime = nt;
		if(prefs->show_fps)
		{
			if(!km.fps_results)
				km.fps_results = (float *)
						calloc(MAX_FPS_RESULTS,
						sizeof(float));
			if(km.fps_results)
			{
				km.fps_results[km.fps_nextresult++] =
						km.fps_last;
				if(km.fps_nextresult >= MAX_FPS_RESULTS)
					km.fps_nextresult = 0;
				if(km.fps_nextresult > km.fps_lastresult)
					km.fps_lastresult = km.fps_nextresult;
			}
		}
	}
	if(prefs->show_fps)
	{
		char buf[20];
		snprintf(buf, sizeof(buf), "%.1f FPS", km.fps_last);
		wdash->font(B_NORMAL_FONT);
		wdash->string(0, wdash->height() -
				wdash->fontheight(B_NORMAL_FONT), buf);
	}
	++km.fps_count;

	// Mouse cursor
	if(mouse_visible)
	{
		if(prefs->debug)
			wmain->sprite(DASHW(MAIN) / 2, DASHH(MAIN) / 2,
					B_CROSSHAIR, 0);
		wscreen->sprite(mouse_x, mouse_y, B_CROSSHAIR, 0);
	}

	// Screenshot video - frame rates in Hz; 999 ==> every rendered frame
	if((prefs->cmd_autoshot > 0) && (manage.game_in_progress() ||
			(manage.replay_mode() == RPM_REPLAY)))
	{
		if((prefs->cmd_autoshot == 999) ||
				(nt - km.ss_last_frame >=
				1000 / prefs->cmd_autoshot))
		{
			gengine->screenshot();
			km.ss_last_frame = nt;
		}
	}

	// Frame rate limiter
	if(prefs->maxfps && (wdash->mode() != DASHBOARD_LOADING))
	{
		if(prefs->maxfps_strict)
		{
			static double nextframe = -1000000.0f;
			while(1)
			{
				double t = (double)SDL_GetTicks();
				if(fabs(nextframe - t) > 1000.0f)
					nextframe = t;
				double d = nextframe - t;
				if(d > 10.0f)
					SDL_Delay(10);
				else if(d > 1.0f)
					SDL_Delay(1);
				else
					break;
			}
			nextframe += 1000.0f / prefs->maxfps;
		}
		else
		{
			int rtime = nt - km.maxfps_begin;
			km.maxfps_begin = nt;
			km.maxfps_filter += (float)(rtime -
					km.maxfps_filter) * 0.3;
			int delay = (int)(1000.0 / prefs->maxfps -
					km.maxfps_filter + 0.5);
			if((delay > 0) && (delay < 1100))
				SDL_Delay(delay);
			km.maxfps_begin = (int)SDL_GetTicks();
		}
	}
}


void kobo_gfxengine_t::post_loop()
{
	sound.timestamp_reset();
}


/*----------------------------------------------------------
	main() and related stuff
----------------------------------------------------------*/

static void put_usage()
{
	// Check maximum space needed for names and default values
	int maxnamelen = 8;
	int maxdeflen = 4;
	int s = -1;
	while(1)
	{
		s = prefs->find_next(s);
		if(s < 0)
			break;

		int dl = strlen(prefs->get_default_s(s));
		int nl = strlen(prefs->name(s));
		switch(prefs->type(s))
		{
		  case CFG_BOOL:
			nl += 4;	// Space for "[no]" before the name
			break;
		  case CFG_INT:
		  case CFG_FLOAT:
			break;
		  case CFG_STRING:
			dl += 2;	// Space for default value quoting
			break;
		  default:
			nl = 0;
			continue;
		}
		if(dl > maxdeflen)
			maxdeflen = dl;
		if(nl > maxnamelen)
			maxnamelen = nl;
	}

	// Column padding
	maxnamelen += 2;
	maxdeflen += 4;		// Space for [] brackets!

	// Print nicely formatted documentation
	printf("\nKobo Redux %s\n", KOBO_VERSION_STRING);
	printf("Usage: kobord [<options>]\n");
	printf("Recognized options:\n");
	s = -1;
	while(1)
	{
		s = prefs->find_next(s);
		if(s < 0)
			break;

		char def[16];
		snprintf(def, sizeof(def), prefs->type(s) == CFG_STRING ?
				"[\"%s\"]" : "[%s]", prefs->get_default_s(s));

		const char *fs;
		int namelen = maxnamelen;
		switch(prefs->type(s))
		{
		  case CFG_SECTION:
			printf("\n  %s:\n", prefs->name(s));
			continue;
		  case CFG_BOOL:
			fs = "    -[no]%-*.*s%-*.*s%s%s\n";
			namelen = maxnamelen - 4;
			break;
		  case CFG_INT:
		  case CFG_FLOAT:
		  case CFG_STRING:
			fs = "    -%-*.*s%-*.*s%s%s\n";
			break;
		  default:
			continue;
		}
		printf(fs, namelen, namelen, prefs->name(s),
				maxdeflen, maxdeflen, def,
				prefs->do_save(s) ? "" : "(Not saved!) ",
				prefs->description(s));
	}
	printf("\n");
}


static void put_options_man()
{
	int s = -1;
	while(1)
	{
		const char *fs;
		s = prefs->find_next(s);
		if(s < 0)
			break;
		switch(prefs->type(s))
		{
		  case CFG_BOOL:
			fs = ".TP\n.B \\-[no]%s\n%s%s. Default: %s.\n";
			break;
		  case CFG_INT:
		  case CFG_FLOAT:
			fs = ".TP\n.B \\-%s\n%s%s. Default: %s.\n";
			break;
		  case CFG_STRING:
			fs = ".TP\n.B \\-%s\n%s%s. Default: \"%s\"\n";
			break;
		  default:
			continue;
		}
		printf(fs, prefs->name(s),
				prefs->do_save(s) ? "" : "(Not saved!) ",
				prefs->description(s),
				prefs->get_default_s(s));
	}
}


static void put_versions()
{
	printf(	PACKAGE "\n"
		"Build date: " __DATE__ " " __TIME__ "\n"
		"Build options:"
#ifdef KOBO_DEMO
		"  KOBO_DEMO"
#endif
#ifdef DEBUG
		"  DEBUG"
#endif
#ifdef ENABLE_TOUCHSCREEN
		"  ENABLE_TOUCHSCREEN"
#endif
		"\n\n");

	SDL_version sdlver;
	SDL_VERSION(&sdlver);
	unsigned a2ver = a2_HeaderVersion();
	printf(	"Built with:\n"
		"  SDL          %d.%d.%d.%d (%s)\n"
		"  Audiality 2  %d.%d.%d.%d\n\n",
			sdlver.major, sdlver.minor, sdlver.patch,
			SDL_REVISION_NUMBER, SDL_REVISION,
			A2_MAJOR(a2ver), A2_MINOR(a2ver), A2_MICRO(a2ver),
			A2_BUILD(a2ver));

	SDL_GetVersion(&sdlver);
	a2ver = a2_LinkedVersion();
	printf(	"Linked with:\n"
		"  SDL          %d.%d.%d.%d (%s)\n"
		"  Audiality 2  %d.%d.%d.%d\n\n",
			sdlver.major, sdlver.minor, sdlver.patch,
			SDL_GetRevisionNumber(), SDL_GetRevision(),
			A2_MAJOR(a2ver), A2_MINOR(a2ver), A2_MICRO(a2ver),
			A2_BUILD(a2ver));
}


static void put_copyright()
{
	printf("\n"
#ifdef WIN32
	"     ,--.   ,--.,--------. ,-------.  ,--------.\n"
	"     |  |  /  / |  ,--.  | |  ,--.  \\ |  ,--.  |\n"
	"     |  '-'  /  |__|  |__| |__`--'__/ |  |  |  |\n"
	"     |__,-.--\\  |  |  |  | |  ,--.  \\ |--|  |__|\n"
	"     |  |  \\  \\ |  `--'  | |  `--'  / |  `--'  |\n"
	"     `--'   `--'`--------' `-------'  `--------'\n"
#else
	"     ,--.   ,--.,--------. ,-------.  ,--------.\n"
	"     |  |  /  / |  ,--.  | |  ,--.  \\ |  ,--.  |\n"
	"     |  '-´  /  |__|  |__| |__`--´__/ |  |  |  |\n"
	"     |__,-.--\\  |  |  |  | |  ,--.  \\ |--|  |__|\n"
	"     |  |  \\  \\ |  `--'  | |  `--´  / |  `--´  |\n"
	"     `--´   `--'`--------´ `-------´  `--------´\n"
#endif
	"      R    -    E    -    D    -    U    -    X\n"
	"\n"
	"  " KOBO_COPYRIGHT "\n"
	"\n"
	".----------------------------------------------------.\n"
	"|    The source code of this game is Free. See the   |\n"
	"|   respective source files for copying conditions.  |\n"
	"| There is NO warranty; not even for MERCHANTABILITY |\n"
	"|        or FITNESS FOR A PARTICULAR PURPOSE.        |\n"
	"'----------------------------------------------------'\n"
	"  Copyright 1995, 1996 Akira Higuchi\n"
	"  Copyright 1997 Masanao Izumo\n"
	"  Copyright 1999-2001 Simon Peter\n"
	"  Copyright 2002 Florian Schulze\n"
	"  Copyright 2002 Jeremy Sheeley\n"
	"  Copyright 2005 Erik Auerswald\n"
	"  Copyright 2008 Robert Schuster\n"
	"  Copyright 1999-2009, 2015-2017 David Olofson\n"
	"\n");
}


extern "C" {
	static void emergency_close(void)
	{
		km.close();
	}

	static RETSIGTYPE breakhandler(int dummy)
	{
		// For platforms that drop the handlers on the first signal
		signal(SIGTERM, breakhandler);
		signal(SIGINT, breakhandler);
		km.brutal_quit();
#if (RETSIGTYPE != void)
		return 0;
#endif
	}
}


int main(int argc, char *argv[])
{
	int cmd_exit = 0;
	put_copyright();
	put_versions();

	atexit(emergency_close);
	signal(SIGTERM, breakhandler);
	signal(SIGINT, breakhandler);

	SDL_Init(0);
	if(main_init())
	{
		fprintf(stderr, "INTERNAL ERROR\n");
		return 1;
	}

	km.open_logging(NULL);

	setup_dirs(argv[0]);
	--argc;
	++argv;

	if((argc < 1) || (strcmp("-override", argv[0]) != 0))
		km.load_config(prefs);

	prefs->accept(prefs->width);
	prefs->accept(prefs->height);
	if((prefs->parse(argc, argv) < 0) || prefs->cmd_help)
	{
		put_usage();
		main_cleanup();
		return 1;
	}
	if(prefs->redefined(prefs->width) && prefs->redefined(prefs->height))
	{
		// Automatically set custom mode if 'width' and 'height' are
		// specified on the command line.
		prefs->set(prefs->get(&prefs->videomode), VMID_CUSTOM);
	}

	if(prefs->cmd_options_man)
	{
		put_options_man();
		main_cleanup();
		return 1;
	}

	km.open_logging(prefs);

	for(int a = 0; a < argc; ++a)
		log_printf(DLOG, "argv[%d] = \"%s\"\n", a, argv[a]);

	int k = -1;
	while((k = prefs->find_next(k)) >= 0)
	{
		log_printf(D3LOG, "key %d: \"%s\"\ttype=%d\t\"%s\"\n",
				k,
				prefs->name(k),
				prefs->type(k),
				prefs->description(k)
			);
	}

	add_dirs(prefs);

	if(prefs->cmd_showcfg)
	{
		printf("Configuration:\n");
		printf("----------------------------------------\n");
		prefs->write(stdout);
		printf("\nPaths:\n");
		printf("----------------------------------------\n");
		fmap->print(stdout, "*");
		printf("----------------------------------------\n");
		cmd_exit = 1;
	}

	// Config and saves were moved to new locations in 0.7.5, so if we get
	// anything older than that, we should migrate!
	if(prefs->cmd_resaveall ||
			(prefs->version < KOBO_MAKE_VERSION(0, 7, 5, 0)))
	{
		km.save_config(prefs);
		prefs->changed = 0;
		savemanager.resave_all();
	}

	km.discover_themes();
	km.list_themes();

	if(cmd_exit)
	{
		main_cleanup();
		return 0;
	}
#if 1
	if(prefs->cmd_warp)
	{
		log_printf(WLOG, "The -warp switch is currently broken and "
				"has been disabled!\n");
		prefs->cmd_warp = 0;
	}
#endif
	if(km.open() < 0)
	{
		main_cleanup();
		return 1;
	}

	km.run();

	sound.music(-1);
	if(!(prefs->quickstart || prefs->cmd_warp))
	{
#if 0
	km.noiseburst();
#else
		while(wdash->busy(true))
			gengine->present();
#endif
	}

	km.close();

	// Seems like we got all the way here without crashing, so let's save
	// the current configuration! :-)
	if(prefs->changed && prefs->cmd_savecfg)
	{
		km.save_config(prefs);
		prefs->changed = 0;
	}

	if(prefs->show_fps && km.fps_results)
		km.print_fps_results();

	main_cleanup();
	return 0;
}
