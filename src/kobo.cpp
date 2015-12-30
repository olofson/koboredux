/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2005-2007, 2009, 2011 David Olofson
 * Copyright 2005 Erik Auerswald
 * Copyright 2008 Robert Schuster
 * Copyright 2015 David Olofson (Kobo Redux)
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
#include "score.h"
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
#define DEFAULT_JOY_FIRE	0	// Default fire button on joystick
#define DEFAULT_JOY_START	1


/*----------------------------------------------------------
	Singletons
----------------------------------------------------------*/
KOBO_sound		sound;


/*----------------------------------------------------------
	Globals
----------------------------------------------------------*/
filemapper_t		*fmap = NULL;
prefs_t			*prefs = NULL;
kobo_gfxengine_t	*gengine = NULL;

screen_window_t		*wscreen = NULL;
dashboard_window_t	*wdash = NULL;
shieldbar_t		*wshield = NULL;
radar_map_t		*wmap = NULL;
radar_window_t		*wradar = NULL;
engine_window_t		*wmain = NULL;
spinplanet_t		*wplanet = NULL;
window_t		*woverlay = NULL;
display_t		*dhigh = NULL;
display_t		*dscore = NULL;
display_t		*dstage = NULL;
display_t		*dregion = NULL;
display_t		*dlevel = NULL;
hledbar_t		*pxtop = NULL;
hledbar_t		*pxbottom = NULL;
vledbar_t		*pxleft = NULL;
vledbar_t		*pxright = NULL;

int mouse_x = 0;
int mouse_y = 0;
int mouse_left = 0;
int mouse_middle = 0;
int mouse_right = 0;

int exit_game = 0;


static int main_init()
{
	prefs = new prefs_t;
	fmap = new filemapper_t;
	gengine = new kobo_gfxengine_t;
	return 0;
}


static void main_cleanup()
{
	delete gengine;
	gengine = NULL;
	delete fmap;
	fmap = NULL;
	delete prefs;
	prefs = NULL;
}


/*----------------------------------------------------------
	Various functions
----------------------------------------------------------*/

static void setup_dirs(char *xpath)
{
	fmap->exepath(xpath);

	fmap->addpath("DATA", KOBO_DATADIR);

	/*
	 * Graphics data
	 */
	/* Current dir; from within the build tree */
	fmap->addpath("GFX", "./data/gfx");
	/* Real data dir */
	fmap->addpath("GFX", "DATA>>gfx");
	/* Current dir */
	fmap->addpath("GFX", "./gfx");

	/*
	 * Sound data
	 */
	/* Current dir; from within the build tree */
	fmap->addpath("SFX", "./data/sfx");
	/* Real data dir */
	fmap->addpath("SFX", "DATA>>sfx");
	/* Current dir */
	fmap->addpath("SFX", "./sfx");

	/*
	 * Score files (user and global)
	 */
	fmap->addpath("SCORES", KOBO_SCOREDIR);
	/* 'scores' in current dir (For importing scores, perhaps...) */
// (Disabled for now, since filemapper_t can't tell
// when it hits the same dir more than once...)
//	fmap->addpath("SCORES", "./scores");

	/*
	 * Configuration files
	 */
	fmap->addpath("CONFIG", KOBO_CONFIGDIR);
#ifdef KOBO_SYSCONFDIR
	/* System local */
	fmap->addpath("CONFIG", KOBO_SYSCONFDIR);
#endif
	/* In current dir (last resort) */
	fmap->addpath("CONFIG", "./");
}


static void add_dirs(prefs_t *p)
{
	char buf[300];
	if(p->dir[0])
	{
		char *upath = fmap->sys2unix(p->dir);
		snprintf(buf, 300, "%s/sfx", upath);
		fmap->addpath("SFX", buf, 1);
		snprintf(buf, 300, "%s/gfx", upath);
		fmap->addpath("GFX", buf, 1);
		snprintf(buf, 300, "%s/scores", upath);
		fmap->addpath("SCORES", buf, 1);
		snprintf(buf, 300, "%s", upath);
		fmap->addpath("CONFIG", buf, 0);
	}

	if(p->sfxdir[0])
		fmap->addpath("SFX", fmap->sys2unix(p->sfxdir), 1);

	if(p->gfxdir[0])
		fmap->addpath("GFX", fmap->sys2unix(p->gfxdir), 1);

	if(p->scoredir[0])
		fmap->addpath("SCORES", fmap->sys2unix(p->scoredir), 1);
}


/*----------------------------------------------------------
	The main object
----------------------------------------------------------*/
class KOBO_main
{
  public:
	static SDL_Joystick	*joystick;
	static int		js_lr;
	static int		js_ud;
	static int		js_fire;
	static int		js_start;

	static FILE		*logfile;

	static Uint32		esc_tick;
	static int		esc_count;
	static int		esc_hammering_trigs;
	static int		exit_game_fast;

	// Frame rate counter
	static int		fps_count;
	static int		fps_starttime;
	static int		fps_nextresult;
	static int		fps_lastresult;
	static float		*fps_results;
	static display_t	*dfps;

	// Frame rate limiter
	static float		maxfps_filter;
	static int		maxfps_begin;

	// Dashboard offset ("native" 640x360 pixels)
	static int		xoffs;
	static int		yoffs;

	// Backup in case we screw up we can't get back up
	static prefs_t		safe_prefs;

	static int restart_audio();
	static int restart_video();
	static int reload_graphics();

	static int open();
	static void close();
	static int run();

	static int open_logging(prefs_t *p);
	static void close_logging();
	static void load_config(prefs_t *p);
	static void save_config(prefs_t *p);

	static void build_screen();
	static int init_display(prefs_t *p);
	static void close_display();

	static void noiseburst();
	static void show_progress(prefs_t *p);
	static void progress();
	static void doing(const char *msg);
	static int load_palette();
	static int load_graphics(prefs_t *p);
	static int load_sounds(prefs_t *p, int render_all = 0);

	static int init_js(prefs_t *p);
	static void close_js();

	static bool escape_hammering();
	static bool escape_hammering_quit();
	static bool quit_requested();
	static bool skip_requested();
	static void brutal_quit(bool force = false);
	static void pause_game();

	static void print_fps_results();
};


SDL_Joystick	*KOBO_main::joystick = NULL;
int		KOBO_main::js_lr = DEFAULT_JOY_LR;
int		KOBO_main::js_ud = DEFAULT_JOY_UD;
int		KOBO_main::js_fire = DEFAULT_JOY_FIRE;
int		KOBO_main::js_start = DEFAULT_JOY_START;

FILE		*KOBO_main::logfile = NULL;

Uint32		KOBO_main::esc_tick = 0;
int		KOBO_main::esc_count = 0;
int		KOBO_main::esc_hammering_trigs = 0;
int		KOBO_main::exit_game_fast = 0;

int		KOBO_main::fps_count = 0;
int		KOBO_main::fps_starttime = 0;
int		KOBO_main::fps_nextresult = 0;
int		KOBO_main::fps_lastresult = 0;
float		*KOBO_main::fps_results = NULL;
display_t	*KOBO_main::dfps = NULL;

float		KOBO_main::maxfps_filter = 0.0f;
int		KOBO_main::maxfps_begin = 0;

int		KOBO_main::xoffs = 0;
int		KOBO_main::yoffs = 0;

prefs_t		KOBO_main::safe_prefs;


static KOBO_main km;


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
			exit_game_fast = 1;
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
				exit_game_fast = 1;
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
					exit_game_fast = 1;
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
			exit_game_fast = 1;
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
				exit_game_fast = 1;
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
					exit_game_fast = 1;
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


void KOBO_main::close_logging()
{
	/* Flush logs to disk, close log files etc. */
	log_close();

	if(logfile)
	{
		fclose(logfile);
		logfile = NULL;
	}
}


int KOBO_main::open_logging(prefs_t *p)
{
	close_logging();

	if(log_open() < 0)
		return -1;

	if(p && p->logfile)
		switch (p->logformat)
		{
		  case 2:
			logfile = fopen("log.html", "wb");
			break;
		  default:
			logfile = fopen("log.txt", "wb");
			break;
		}

	if(logfile)
	{
		log_set_target_stream(0, logfile);
		log_set_target_stream(1, logfile);
	}
	else
	{
		log_set_target_stream(0, stdout);
		log_set_target_stream(1, stderr);
	}

	log_set_target_stream(2, NULL);

	if(p)
		switch(p->logformat)
		{
		  default:
			log_set_target_flags(-1, LOG_TIMESTAMP);
			break;
		  case 1:
			log_set_target_flags(-1, LOG_ANSI | LOG_TIMESTAMP);
			break;
		  case 2:
			log_set_target_flags(-1, LOG_HTML | LOG_TIMESTAMP);
			break;
		}

	/* All levels output to stdout... */
	log_set_level_target(-1, 0);

	/* ...except these, that output to stderr. */
	log_set_level_target(ELOG, 1);
	log_set_level_target(CELOG, 1);

	/* Some fancy colors... */
	log_set_level_attr(ULOG, LOG_YELLOW);
	log_set_level_attr(WLOG, LOG_YELLOW | LOG_BRIGHT);
	log_set_level_attr(ELOG, LOG_RED | LOG_BRIGHT);
	log_set_level_attr(CELOG, LOG_RED | LOG_BRIGHT | LOG_BLINK);
	log_set_level_attr(DLOG, LOG_CYAN);
	log_set_level_attr(D2LOG, LOG_BLUE | LOG_BRIGHT);
	log_set_level_attr(D3LOG, LOG_BLUE);

	/* Disable levels as desired */
	if(p)
		switch(p->logverbosity)
		{
		  case 0:
			log_set_level_target(ELOG, 2);
		  case 1:
			log_set_level_target(WLOG, 2);
		  case 2:
			log_set_level_target(DLOG, 2);
		  case 3:
			log_set_level_target(D2LOG, 2);
		  case 4:
			log_set_level_target(D3LOG, 2);
		  case 5:
			break;
		}

	if(p && p->logfile && !logfile)
		log_printf(ELOG, "Couldn't open log file!\n");

	return 0;
}


void KOBO_main::build_screen()
{
	// Dashboard framework
	wdash->place(xoffs, yoffs, SCREEN_WIDTH, SCREEN_HEIGHT);

	// Console window
	int conx = xoffs + WCONSOLE_X;
	int cony = yoffs + WCONSOLE_Y;
	dhigh->place(conx, cony, WCONSOLE_W, 18);
	dhigh->font(B_NORMAL_FONT);
	dhigh->caption("HIGHSCORE");
	dhigh->text("000000000");

	dscore->place(conx, cony + 24, WCONSOLE_W, 18);
	dscore->font(B_NORMAL_FONT);
	dscore->caption("SCORE");
	dscore->text("000000000");

	dstage->place(conx, cony + 88, WCONSOLE_W / 3, 18);
	dstage->font(B_NORMAL_FONT);
	dstage->caption("STAGE");
	dstage->text("000");
	dregion->place(conx + WCONSOLE_W / 3, cony + 88, WCONSOLE_W / 3, 18);
	dregion->font(B_NORMAL_FONT);
	dregion->caption("REGION");
	dregion->text("0");
	dlevel->place(conx + 2 * WCONSOLE_W / 3, cony + 88,
			WCONSOLE_W / 3, 18);
	dlevel->font(B_NORMAL_FONT);
	dlevel->caption("LEVEL");
	dlevel->text("00");

	// Shield bar
	wshield->place(xoffs + WSHIELD_X, yoffs + WSHIELD_Y,
			WSHIELD_W, WSHIELD_H);
	wshield->set_leds(B_BLEDS);

	// Spinning planet backdrop
	wplanet->place(xoffs + WMAIN_X, yoffs + WMAIN_Y, WMAIN_W, WMAIN_H);
	wplanet->track_layer(LAYER_PLANET);

	// Main playfield layer
	wmain->place(xoffs + WMAIN_X, yoffs + WMAIN_Y, WMAIN_W, WMAIN_H);

	// Playfield overlay
	woverlay->place(xoffs + WMAIN_X, yoffs + WMAIN_Y, WMAIN_W, WMAIN_H);

	// Set up the map at 1 physical pixel per tile
	wmap->offscreen();
	wmap->scale(1.0f, 1.0f);
	wmap->place(0, 0, MAP_SIZEX, MAP_SIZEY);

	// Have the radar window scale up to 2x2 "native" pixels per tile
	wradar->place(xoffs + WRADAR_X, yoffs + WRADAR_Y, WRADAR_W, WRADAR_H);
	wradar->scale(-2.0f, -2.0f);

	pxtop->place(xoffs + TOPLEDS_X, yoffs + TOPLEDS_Y,
			TOPLEDS_W, TOPLEDS_H);
	pxbottom->place(xoffs + BOTTOMLEDS_X, yoffs + BOTTOMLEDS_Y,
			BOTTOMLEDS_W, BOTTOMLEDS_H);
	pxleft->place(xoffs + LEFTLEDS_X, yoffs + LEFTLEDS_Y,
			LEFTLEDS_W, LEFTLEDS_H);
	pxright->place(xoffs + RIGHTLEDS_X, yoffs + RIGHTLEDS_Y,
			RIGHTLEDS_W, RIGHTLEDS_H);

	if(prefs->cmd_fps)
	{
		dfps = new display_t(gengine);
		dfps->place(0, -9, 48, 18);
		dfps->color(wdash->map_rgb(0, 0, 0));
		dfps->font(B_NORMAL_FONT);
		dfps->caption("FPS");
	}
}


int KOBO_main::init_display(prefs_t *p)
{
	int dw, dh;		// Display size
	int gw, gh;		// Game "window" size
	int desktopres = 0;

	gengine->hide();

	gengine->title("Kobo Redux " KOBO_VERSION, "kobord");
	gengine->vsync(p->vsync);
	gengine->cursor(0);
	gengine->mode((VMM_ModeID)p->videomode, p->fullscreen);

	vmm_Init();
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

	gengine->clear();

	wscreen = new screen_window_t(gengine);
	wscreen->place(0, 0,
			(int)(gengine->width() / gengine->xscale() + 0.5f),
			(int)(gengine->height() / gengine->yscale() + 0.5f));
	wscreen->border((int)(yoffs * gengine->yscale() + 0.5f),
			(int)(xoffs * gengine->xscale() + 0.5f),
			dw - gw - (int)(xoffs * gengine->xscale() + 0.5f),
			dh - gh - (int)(yoffs * gengine->yscale() + 0.5f));

	wdash = new dashboard_window_t(gengine);
	wshield = new shieldbar_t(gengine);
	wplanet = new spinplanet_t(gengine);
	wmain = new engine_window_t(gengine);
	woverlay = new window_t(gengine);
	dhigh = new display_t(gengine);
	dscore = new display_t(gengine);
	wmap = new radar_map_t(gengine);
	wradar = new radar_window_t(gengine);
	dstage = new display_t(gengine);
	dregion = new display_t(gengine);
	dlevel = new display_t(gengine);
	pxtop = new hledbar_t(gengine);
	pxbottom = new hledbar_t(gengine);
	pxleft = new vledbar_t(gengine);
	pxright = new vledbar_t(gengine);

	build_screen();

	wdash->mode(DASHBOARD_BLACK);

	return 0;
}


void KOBO_main::close_display()
{
	delete pxtop;
	pxtop = NULL;
	delete pxbottom;
	pxbottom = NULL;
	delete pxleft;
	pxleft = NULL;
	delete pxright;
	pxright = NULL;
	delete dfps;
	dfps = NULL;
	delete dstage;
	dstage = NULL;
	delete dregion;
	dregion = NULL;
	delete dlevel;
	dlevel = NULL;
	delete wradar;
	wradar = NULL;
	delete wmap;
	wmap = NULL;
	delete dscore;
	dscore = NULL;
	delete dhigh;
	dhigh = NULL;
	delete woverlay;
	woverlay = NULL;
	delete wplanet;
	wplanet = NULL;
	delete wmain;
	wmain = NULL;
	delete wshield;
	wshield = NULL;
	delete wdash;
	wdash = NULL;
	delete wscreen;
	wscreen = NULL;
}


void KOBO_main::noiseburst()
{
	if(prefs->quickstart)
		return;

	sound.ui_noise(2);
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
	sound.ui_noise(0);
}


void KOBO_main::show_progress(prefs_t *p)
{
	wdash->fade(1.0f);
	if(s_get_bank(gfxengine->get_gfx(), B_OAPLANET))
		wdash->mode(DASHBOARD_LOADING);
	else
		wdash->mode(DASHBOARD_BLACK);
	wdash->doing("");
}


void KOBO_main::progress()
{
	wdash->progress();
}


void KOBO_main::doing(const char *msg)
{
	wdash->doing(msg);
	log_printf(ULOG, "=== %s ===\n", msg);
}


#ifndef TIME_PROGRESS
static float progtab_graphics[] = {
	0.000000,
	3.894737,
	7.052631,
	10.736842,
	12.421053,
	14.105263,
	15.894737,
	17.789474,
	20.105263,
	23.578947,
	24.000000,
	25.157894,
	27.789474,
	28.105263,
	30.210526,
	31.894737,
	33.684212,
	35.684212,
	37.263157,
	38.947369,
	40.842106,
	42.631580,
	44.105263,
	45.684212,
	47.368420,
	49.157894,
	50.947369,
	53.157894,
	54.842106,
	56.947369,
	58.526318,
	59.684212,
	61.473682,
	63.368420,
	65.157898,
	67.157898,
	68.947365,
	70.315788,
	72.210526,
	74.842102,
	77.263161,
	78.947365,
	81.263161,
	84.315788,
	87.789474,
	89.578949,
	92.210526,
	94.842102,
	96.631577,
	98.421051,
	100.0,
	0
};

static float progtab_sounds[] = {
	0.000000,
	100.0,
	0
};

static float progtab_all[] = {
	0.000000,
	4.210526,
	7.052631,
	10.631579,
	12.421053,
	14.105263,
	15.789474,
	17.789474,
	20.315790,
	23.578947,
	24.000000,
	25.157894,
	28.000000,
	28.210526,
	30.315790,
	31.789474,
	33.789474,
	35.578949,
	37.157894,
	39.052631,
	40.842106,
	42.315788,
	44.000000,
	45.578949,
	47.368420,
	49.157894,
	50.842106,
	52.947369,
	54.736843,
	56.842106,
	58.315788,
	59.578949,
	61.473682,
	63.368420,
	65.052635,
	67.368423,
	68.736839,
	70.315788,
	72.210526,
	75.157898,
	77.157898,
	78.947365,
	81.473686,
	84.315788,
	87.789474,
	89.578949,
	92.210526,
	94.842102,
	96.526314,
	98.421051,
	100.0,
	0
};
#endif


typedef enum
{
	// Clamping/wrapping options for filters
	KOBO_CLAMP =		0x0001,	// Clamp to frame edge pixels
	KOBO_CLAMP_OPAQUE =	0x0002,	// Clamp to black; not transparent
	KOBO_WRAP =		0x0004,	// Wrap around frame edges

	// Scaling filter options
	KOBO_ABSSCALE =		0x0010,	// Scale factor is absolute
	KOBO_NEAREST =		0x0020,	// Force NEAREST scale mode
	KOBO_BILINEAR =		0x0040,	// Force BILINEAR scale mode
	KOBO_SCALE2X =		0x0080,	// Force Scale2X scale mode

	// Other options
	KOBO_NOALPHA =		0x0100,	// Disable alpha channel
	KOBO_CENTER =		0x0200,	// Center hotspot in frames
	KOBO_NOBRIGHT =		0x0400,	// Disable brightness/contrast filter
	KOBO_FONT =		0x0800,	// Load as "SFont" rather than tiles!

	KOBO_MESSAGE =		0x1000	// Not a file! 'path' is a message.
} KOBO_GfxDescFlags;

typedef struct KOBO_GfxDesc
{
	const char	*path;		// Path + name (for filemapper)
	int		bank;		// Sprite bank ID
	int		w, h;		// Frame size (original image pixels)
	float		scale;		// Scale of source data rel. 640x360
	int		flags;		// KOBO_GfxDescFlags
} KOBO_GfxDesc;

static KOBO_GfxDesc gfxdesc[] = {
	// Loading screen
	{ "Loading loading screen graphics", 0, 0,0, 0.0f, KOBO_MESSAGE },
	{ "GFX>>olofsonarcade-do64-ovl.png", B_OALOGO,	0, 0,	0.0f,
			KOBO_CLAMP | KOBO_CENTER },
	{ "GFX>>oaplanet-pcb.png", B_OAPLANET, 0, 0,
			2.0f, KOBO_ABSSCALE | KOBO_SCALE2X },
	{ "GFX>>arcadefont.png", B_NORMAL_FONT,	0, 0,	0.0f,	KOBO_FONT },

	// In-game
	{ "Loading in-game graphics", 0, 0,0, 0.0f, KOBO_MESSAGE },

	// NOTE: We only have map textures for every other mip level!

	// Region 1 (maps 1-10)
	{ "GFX>>tiles-green.png", B_R1_TILES,	16, 16,	0.0f,	KOBO_CLAMP },
	{ "GFX>>tiles-green-small-space.png", B_R1_TILES_SMALL_SPACE,
			12, 12,	0.0f,	KOBO_CLAMP },
	{ "GFX>>tiles-green-tiny-space.png", B_R1_TILES_TINY_SPACE,
			8, 8,	0.0f,	KOBO_CLAMP },
	{ "GFX>>tiles-green-small-ground.png", B_R1_TILES_SMALL_GROUND,
			12, 12,	0.0f,	KOBO_CLAMP },
	{ "GFX>>tiles-green-tiny-ground.png", B_R1_TILES_TINY_GROUND,
			8, 8,	0.0f,	KOBO_CLAMP },
	{ "GFX>>planet-r1.png", B_R1_PLANET, 0, 0,
			2.0f, KOBO_ABSSCALE | KOBO_BILINEAR },
	{ "GFX>>ground-r1l8.png", B_R1L8_GROUND, 0, 0,	0.0f, 0 },
	{ "GFX>>ground-r1l9.png", B_R1L9_GROUND, 0, 0,	0.0f, 0 },
	{ "GFX>>ground-r1l10.png", B_R1L10_GROUND, 0, 0, 0.0f, 0 },

	// Region 2 (maps 11-20)

	// Region 3 (maps 21-30)

	// Region 4 (maps 31-40)

	// Region 5 (maps 41-50)

	{ "GFX>>crosshair.png", B_CROSSHAIR,	32, 32,	1.0f,	KOBO_CENTER },
	{ "GFX>>player.png", B_PLAYER,		40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>bmr-green.png", B_BMR_GREEN,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>bmr-purple.png", B_BMR_PURPLE,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>bmr-pink.png", B_BMR_PINK,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>fighter.png", B_FIGHTER,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>missile.png", B_MISSILE1,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>missile2.png", B_MISSILE2,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>missile3.png", B_MISSILE3,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>orange-bolt.png", B_BOLT,	8, 8,	1.0f,	KOBO_CENTER },
	{ "GFX>>explo1.png", B_EXPLO1,		24, 24,	0.0f,	KOBO_CENTER },
#if 0
	{ "GFX>>explo3e.png", B_EXPLO3,		64, 64,	2.0f,	KOBO_CENTER },
	{ "GFX>>explo4e.png", B_EXPLO4,		64, 64,	2.0f,	KOBO_CENTER },
	{ "GFX>>explo5e.png", B_EXPLO5,		64, 64,	2.0f,	KOBO_CENTER },
#else
	{ "GFX>>explo1.png", B_EXPLO3,		24, 24,	0.0f,	KOBO_CENTER },
	{ "GFX>>explo1.png", B_EXPLO4,		24, 24,	0.0f,	KOBO_CENTER },
	{ "GFX>>explo1.png", B_EXPLO5,		24, 24,	0.0f,	KOBO_CENTER },
#endif
#if 0
	{ "GFX>>rock1c.png", B_ROCK1,		32, 32,	2.0f,	KOBO_CENTER },
	{ "GFX>>rock2.png", B_ROCK2,		32, 32,	2.0f,	KOBO_CENTER },
	{ "GFX>>shinyrock.png", B_ROCK3,	32, 32,	2.0f,	KOBO_CENTER },
#else
	{ "GFX>>rock2.png", B_ROCK1,		32, 32,	1.0f,	KOBO_CENTER },
	{ "GFX>>rock2.png", B_ROCK2,		32, 32,	1.0f,	KOBO_CENTER },
	{ "GFX>>rock2.png", B_ROCK3,		32, 32,	1.0f,	KOBO_CENTER },
#endif
	{ "GFX>>rockexpl.png", B_ROCKEXPL,	64, 64,	1.0f,	KOBO_CENTER },

	{ "GFX>>bullet.png", B_BLT_GREEN,	8, 8,	0.0f,	KOBO_CENTER },
	{ "GFX>>red-bullet.png", B_BLT_RED,	8, 8,	0.0f,	KOBO_CENTER },
	{ "GFX>>blue-bullet.png", B_BLT_BLUE,	8, 8,	0.0f,	KOBO_CENTER },
	{ "GFX>>bulletexpl2.png", B_BLTX_GREEN,	32, 32,	2.0f,	KOBO_CENTER },
	{ "GFX>>bulletexpl2.png", B_BLTX_RED,	32, 32,	2.0f,	KOBO_CENTER },
	{ "GFX>>bulletexpl2.png", B_BLTX_BLUE,	32, 32,	2.0f,	KOBO_CENTER },

	{ "GFX>>ring.png", B_RING,		32, 32,	2.0f,	KOBO_CENTER },
	{ "GFX>>ringexpl2b.png", B_RINGEXPL,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>bomb.png", B_BOMB,		24, 24,	2.0f,	KOBO_CENTER },
	{ "GFX>>bombdeto.png", B_BOMBDETO,	40, 40,	2.0f,	KOBO_CENTER },
	{ "GFX>>bigship.png", B_BIGSHIP,	72, 72,	2.0f,	KOBO_CENTER },

	// Dashboard
	{ "Loading dashboard graphics", 0, 0,0, 0.0f, KOBO_MESSAGE },
	{ "GFX>>hleds.png", B_HLEDS,		8, 5,	0.0f,	KOBO_CLAMP },
	{ "GFX>>vleds.png", B_VLEDS,		5, 8,	0.0f,	KOBO_CLAMP },
#if 0
	{ "GFX>>barleds.png", B_BLEDS,		12, 8,	0.0f,	KOBO_CLAMP },
#else
	{ "GFX>>barleds3.png", B_BLEDS,		12, 4,	0.0f,	KOBO_CLAMP },
#endif
	{ "GFX>>dashboard.png", B_SCREEN,	0, 0,	0.0f,
			KOBO_CLAMP_OPAQUE },

	// Logo
	{ "Loading logo", 0, 0,0, 0.0f, KOBO_MESSAGE },
	{ "GFX>>logo2h.png", B_LOGO,		0, 0,	0.0f,	0 },

	// Fonts
	{ "Loading fonts", 0, 0,0, 0.0f, KOBO_MESSAGE },
/* FIXME: Dedicated medium size font? */
	{ "GFX>>arcadefont.png", B_MEDIUM_FONT,	0, 0,	0.0f,	KOBO_FONT },
	{ "GFX>>bigfont.png", B_BIG_FONT,	0, 0,	0.0f,	KOBO_FONT },
	{ "GFX>>counterfont.png", B_COUNTER_FONT, 0, 0,	2.0f,	KOBO_FONT },

	// Special FX
	{ "Loading special FX graphics", 0, 0,0, 0.0f, KOBO_MESSAGE },
	{ "GFX>>noise.png", B_NOISE,		NOISE_SIZEX, 1,	0.0f,
			KOBO_CLAMP },
	{ "GFX>>hitnoise.png", B_HITNOISE,	NOISE_SIZEX, 1,	0.0f,
			KOBO_CLAMP },
	{ "GFX>>focusfx.png", B_FOCUSFX,	0, 0,	0.0f, KOBO_CLAMP },

	{ NULL, 0,	0, 0,	0.0f,	0 }	// Terminator
};


int KOBO_main::load_palette()
{
	const char *fn;
	if(!(fn = fmap->get("GFX>>DO64-0.24.gpl")))
	{
		log_printf(ELOG, "Couldn't find palette!\n");
		return -1;
	}
	if(!gengine->load_palette(fn))
	{
		log_printf(ELOG, "Couldn't load palette!\n");
		return -2;
	}
	return 0;
}


int KOBO_main::load_graphics(prefs_t *p)
{
	const char *fn;
	KOBO_GfxDesc *gd;
	int loadergfx = 0;

	load_palette();

	gengine->reset_filters();
	show_progress(p);

	for(gd = gfxdesc; gd->path; ++gd)
	{
		// Reinit loading screen once its assets are loaded!
		if(!loadergfx && s_get_bank(gfxengine->get_gfx(), B_OAPLANET))
		{
			show_progress(p);
			loadergfx = 1;
		}

		if(gd->flags & KOBO_MESSAGE)
		{
			doing(gd->path);
			continue;
		}

		log_printf(ULOG, "Loading \"%s\"\n", gd->path);

		if(s_get_bank(gfxengine->get_gfx(), gd->bank))
			log_printf(WLOG, "WARNING: Bank %d already in use!\n",
					gd->bank);

		// Set scale (filter) mode and clamping
		int clamping;
		gfx_scalemodes_t sm;
		if(gd->flags & KOBO_CLAMP)
		{
			gengine->clampcolor(0, 0, 0, 0);
			clamping = 1;
		}
		else if(gd->flags & KOBO_CLAMP_OPAQUE)
		{
			gengine->clampcolor(0, 0, 0, 255);
			clamping = 1;
		}
		else if(gd->flags & KOBO_WRAP)
			clamping = 3;
		else
			clamping = 0;
		if(gd->flags & KOBO_NEAREST)
			sm = GFX_SCALE_NEAREST;
		else if(gd->flags & KOBO_BILINEAR)
			sm = GFX_SCALE_BILINEAR;
		else if(gd->flags & KOBO_SCALE2X)
			sm = GFX_SCALE_SCALE2X;
		else
			sm = (gfx_scalemodes_t) p->scalemode;
		gengine->scalemode(sm, clamping);

		// Disable brightness filter?
		if(gd->flags & KOBO_NOBRIGHT)
			gengine->brightness(1.0f, 1.0f);
		else
			gengine->brightness(0.01f * p->brightness,
					0.01f * p->contrast);

		// Alpha channels
		if(gd->flags & KOBO_NOALPHA || !p->alpha)
			gengine->noalpha(NOALPHA_THRESHOLD);
		else
			gengine->noalpha(0);

		// Source image scale factor
		if(gd->flags & KOBO_ABSSCALE)
			gengine->absolute_scale(gd->scale, gd->scale);
		else if(gd->scale == 0.0f)
			gengine->absolute_scale(1.0f, 1.0f);
		else
			gengine->source_scale(gd->scale, gd->scale);

		// Load!
		fn = fmap->get(gd->path);
		if(!fn)
		{
			log_printf(ELOG, "Couldn't get path to \"%s\"!\n",
					gd->path);
			continue;
		}
		int res;
		if(gd->flags & KOBO_FONT)
			res = gengine->loadfont(gd->bank, fn, gd->scale);
		else if(!gd->w || !gd->h)
			res = gengine->loadimage(gd->bank, fn);
		else
			res = gengine->loadtiles(gd->bank, gd->w, gd->h, fn);
		if(res < 0)
		{
			log_printf(ELOG, "Couldn't load \"%s\"!\n", fn);
			continue;
		}

		// Draw (real time) scale
		if(!gd->scale)
			gengine->draw_scale(gd->bank, gengine->xscale(),
					gengine->yscale());

		// Hotspot
		if(gd->flags & KOBO_CENTER)
		{
			s_bank_t *b = s_get_bank(gfxengine->get_gfx(),
					gd->bank);
			if(b)
				gengine->set_hotspot(gd->bank, -1,
						b->w / 2, b->h / 2);
		}

		// Update progress bar
		progress();
	}

	screen.init_graphics();

	// We can try to run with missing graphics, but without the menu font,
	// the user may not even be able to find his/her way out of the game!
	if(!gengine->get_font(B_BIG_FONT))
	{
		gengine->messagebox("CRITICAL: Could not load menu font!");
		return -1;
	}

	return 0;
}


static int progress_cb(const char *msg)
{
	if(msg)
		km.doing(msg);
	km.progress();
	if(km.quit_requested())
		return -999;
	return 0;
}


int KOBO_main::load_sounds(prefs_t *p, int render_all)
{
	if(!p->sound)
		return 0;
	show_progress(p);
	return sound.load(progress_cb, render_all);
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
	FILE *f = fmap->fopen(KOBO_CONFIGDIR "/" KOBO_CONFIGFILE, "r");
	if(f)
	{
		log_puts(VLOG, "Loading personal configuration from: "\
				KOBO_CONFIGDIR "/" KOBO_CONFIGFILE);
		p->read(f);
		fclose(f);
	}
#ifdef USE_SYSCONF
	/*
	 * On Unixen, where they have SYSCONF_DIR (usually /etc) try to get
	 * the default configuration from a file stored there before
	 * giving up.
	 *
	 * This gives packagers a chance to provide a proper default
	 * (playable) configuration for all those little Linux-based
	 * gadgets that are flying around.
	 */
	else
	{
		f = fmap->fopen(KOBO_SYSCONFDIR "/koboredux/default-config",
				"r");
		if(f)
		{
			log_puts(VLOG, "Loading configuration defaults from: "\
					KOBO_SYSCONFDIR
					"/koboredux/default-config");
			p->read(f);
			fclose(f);
		}
		else
			log_puts(VLOG, "Using built-in configuration defaults.");
	}
#else
	log_puts(VLOG, "Using built-in configuration defaults.");
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
	f = fmap->fopen(KOBO_CONFIGDIR "/" KOBO_CONFIGFILE, "w");
	if(f)
	{
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


void KOBO_main::brutal_quit(bool force)
{
	if(exit_game_fast || force)
	{
		log_printf(ULOG, "Second try quitting; using brutal method!\n");
		atexit(SDL_Quit);
		close_logging();
		exit(1);
	}

	exit_game_fast = 1;
	if(gengine)
		gengine->stop();
}


void KOBO_main::pause_game()
{
	if(gsm.current() != &st_pause_game)
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

	if(load_sounds(prefs) < 0)
		return -3;

	load_palette();
	noiseburst();

	if(!prefs->quickstart)
		sound.jingle(SOUND_OAJINGLE);
	int jtime = SDL_GetTicks() + 5000;

#ifdef TIME_PROGRESS
	wdash->progress_init(NULL);
#else
	wdash->progress_init(progtab_all);
#endif

	if(load_graphics(prefs) < 0)
		return -2;

#ifdef TIME_PROGRESS
	// Use these two lines to time graphics and sounds separately!
	wdash->progress_done();
	wdash->progress_init(NULL);
#endif

	wdash->progress_done();
	wdash->mode(DASHBOARD_JINGLE);

	if(!prefs->quickstart)
	{
		while(!SDL_TICKS_PASSED(SDL_GetTicks(), jtime) &&
				!skip_requested())
			gengine->present();

		noiseburst();
	}

	ct_engine.render_highlight = kobo_render_highlight;
	wradar->mode(RM_NOISE);
	pubrand.init();
	init_js(prefs);
	gamecontrol.init(prefs->always_fire);
	manage.init();

	gsm.push(&st_intro_title);
	return 0;
}


void KOBO_main::close()
{
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
#ifdef TIME_PROGRESS
	wdash->progress_init(NULL);
#else
	wdash->progress_init(progtab_sounds);
#endif
	if(sound.open() < 0)
		return 4;
	if(load_sounds(prefs) < 0)
		return 5;
	wdash->progress_done();
	wdash->mode(DASHBOARD_BLACK);
	log_printf(ULOG, "--- Audio restarted.\n");
	wdash->fade(1.0f);
	wdash->mode(manage.game_in_progress() ?
			DASHBOARD_GAME : DASHBOARD_TITLE);
	wradar->mode(screen.radar_mode);
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
	screen.init_graphics();
	gamecontrol.init(prefs->always_fire);
	log_printf(ULOG, "--- Video restarted.\n");

	// FIXME: We may not always need to reload graphics, but that's a bit
	// tricky to tell for sure, so we just take the easy, safe path here.
	global_status |= OS_RELOAD_GRAPHICS;
	return 0;
}


int KOBO_main::reload_graphics()
{
	if(!(global_status & OS_RESTART_VIDEO))
		wdash->mode(DASHBOARD_BLACK);
	gengine->unload();
	log_printf(ULOG, "--- Reloading graphics...\n");
#ifdef TIME_PROGRESS
	wdash->progress_init(NULL);
#else
	wdash->progress_init(progtab_graphics);
#endif
	if(load_graphics(prefs) < 0)
		return 7;
	wdash->progress_done();
	screen.init_graphics();
	wdash->fade(1.0f);
	wdash->mode(manage.game_in_progress() ?
			DASHBOARD_GAME : DASHBOARD_TITLE);
	wradar->mode(screen.radar_mode);
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
			if(wdash)
				wdash->mode(DASHBOARD_BLACK);
			break;
		}

		// Restart and reload stuff as needed
		int res;
		if(global_status & OS_RESTART_AUDIO)
			if((res = restart_audio()))
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
			gamecontrol.init(prefs->always_fire);
		}
		if(global_status & OS_RESTART_LOGGER)
			open_logging(prefs);

		// Prepare to reenter the main loop
		km.pause_game();
		manage.reenter();
	}
	return 0;
}


/*----------------------------------------------------------
	Kobo Graphics Engine
----------------------------------------------------------*/

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


void kobo_gfxengine_t::safe_video_mode()
{
	prefs->fullscreen = 0;
	prefs->videomode = 0x10920;
	prefs->changed = 1;
	global_status |= OS_RELOAD_GRAPHICS | OS_RESTART_VIDEO;
}


void kobo_gfxengine_t::fullscreen_toggle()
{
	VMM_Mode *vm = vmm_GetMode(prefs->videomode);
	VMM_Mode *dvm = vmm_GetMode(VMID_DESKTOP);

	// Let's just assume we'll change *something*. If all else fails, at
	// least this will cause the display subsystem to be reinitialized, and
	// if the result works, the updated config will be saved on exit.
	prefs->changed = 1;
	global_status |= OS_RELOAD_GRAPHICS | OS_RESTART_VIDEO;

	if(!vm || !dvm)
	{
		// Can't get modes! Enforce safe fallback.
		safe_video_mode();
		return;
	}

	if(prefs->fullscreen || (vm->flags & VMM_DESKTOP))
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
			safe_video_mode();
			return;
		}

		prefs->fullscreen = 0;
		prefs->videomode = bestm->id;
	}
	else
	{
		// Only ever use desktop resolution fullscreen mode here!
		// Anything else is way too likely to cause trouble these days,
		// so users will just have to select those modes manually.
		prefs->fullscreen = 1;
		prefs->videomode = VMID_DESKTOP;
	}
}


float kobo_gfxengine_t::timestamp_delay()
{
	return gengine->period() + prefs->tsdelay;
}


void kobo_gfxengine_t::pre_loop()
{
	sound.timestamp_reset();
	sound.timestamp_bump(timestamp_delay() +
			1000.0f / (prefs->maxfps > 60 ? 60 : prefs->maxfps));
}


void kobo_gfxengine_t::post_loop()
{
	sound.timestamp_reset();
}


void kobo_gfxengine_t::pre_advance()
{
	// We need to adjust for the logic time elapsed since the last logic
	// frame (because logic time is decoupled from rendering frame rate),
	// and for the desired "buffer" timestamp delay.
	float ft = fmod(csengine->time, 1.0f) * gengine->period();
	sound.timestamp_nudge(ft - timestamp_delay());
}


void kobo_gfxengine_t::frame()
{
	sound.frame();

	if(!gsm.current())
	{
		log_printf(CELOG, "INTERNAL ERROR: No gamestate!\n");
		exit_game = 1;
		stop();
		return;
	}
	if(exit_game)
	{
		stop();
		return;
	}

	/*
	 * Process input
	 */
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		int ms;
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
				fullscreen_toggle();
				stop();
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
			  case SDLK_r:
				if(!prefs->cmd_debug)
					break;
				ms = SDL_GetModState();
				if(ms & (KMOD_ALT | KMOD_SHIFT | KMOD_GUI))
					break;
				if(!(ms & KMOD_CTRL))
					break;
				global_status |= OS_RESTART_AUDIO;
				stop();
				break;
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
				safe_video_mode();
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
#if 0
			k = gamecontrol.map(ev.key.keysym.sym);
			if(k == SDLK_PAUSE)
			{
				gamecontrol.press(BTN_PAUSE);
				gsm.press(BTN_PAUSE);
			}
			else
			{
				gamecontrol.release(k);
				gsm.release(k);
			}
#endif
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
				km.brutal_quit();
				break;
			  case SDL_WINDOWEVENT_HIDDEN:
			  case SDL_WINDOWEVENT_MINIMIZED:
			  case SDL_WINDOWEVENT_LEAVE:
			  case SDL_WINDOWEVENT_FOCUS_LOST:
				// Any type of focus loss should activate
				// pause mode!
				km.pause_game();
				break;
			}
			break;
		  case SDL_QUIT:
			km.brutal_quit();
			break;
		  case SDL_JOYBUTTONDOWN:
			if(ev.jbutton.button == km.js_fire)
			{
				gamecontrol.pressbtn(BTN_FIRE,
						GC_SRC_JOYSTICK);
				gsm.pressbtn(BTN_FIRE);
			}
			else if(ev.jbutton.button == km.js_start)
			{
				gamecontrol.pressbtn(BTN_START,
						GC_SRC_JOYSTICK);
				gsm.pressbtn(BTN_START);
			}
			break;
		  case SDL_JOYBUTTONUP:
			if(ev.jbutton.button == km.js_fire)
			{
				gamecontrol.releasebtn(BTN_FIRE,
						GC_SRC_JOYSTICK);
				gsm.releasebtn(BTN_FIRE);
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
			mouse_x = (int)(ev.motion.x / gengine->xscale()) -
					km.xoffs;
			mouse_y = (int)(ev.motion.y / gengine->yscale()) -
					km.yoffs;
			if(prefs->mouse)
				gamecontrol.mouse_position(
						mouse_x - WMAIN_X - WMAIN_W/2,
						mouse_y - WMAIN_Y - WMAIN_H/2);
			break;
		  case SDL_MOUSEBUTTONDOWN:
			mouse_x = (int)(ev.motion.x / gengine->xscale()) -
					km.xoffs;
			mouse_y = (int)(ev.motion.y / gengine->yscale()) -
					km.yoffs;
			gsm.pressbtn(BTN_FIRE);
			if(prefs->mouse)
			{
#ifdef ENABLE_TOUCHSCREEN
				if(ev.motion.x <= pointer_margin_width_min)
				{
					gsm.pressbtn(BTN_LEFT);
					pointer_margin_used = true;
				}
				else if(ev.motion.x >= pointer_margin_width_max)
				{
					// Upper right corner invokes pause.
					// Lower right corner invokes exit.
					// Otherwise it is just 'right'. :)
					if(ev.motion.y <= pointer_margin_height_min)
					{
						gsm.pressbtn(BTN_PAUSE);
						gamecontrol.pressbtn(
								BTN_PAUSE,
								GC_SRC_MOUSE);
					}
					else
						gsm.pressbtn((ev.motion.y >= pointer_margin_height_max
							? BTN_EXIT
							: BTN_RIGHT));
					pointer_margin_used = true;
				}
				if(ev.motion.y <= pointer_margin_height_min)
				{
					// Handle as 'up' only if it was not in
					// the 'pause' area. Still handle as
					// clicked, so 'fire' will not kick in.
					if(ev.motion.x < pointer_margin_width_max)
						gsm.pressbtn(BTN_UP);
					pointer_margin_used = true;
				}
				else if(ev.motion.y >= pointer_margin_height_max)
				{
					// Handle as 'down' only if it was not
					// in the 'exit' area. Still handle as
					// clicked, so 'fire' will not kick in.
					if(ev.motion.x < pointer_margin_width_max)
						gsm.pressbtn(BTN_DOWN);
					pointer_margin_used = true;
				}
				if(!pointer_margin_used)
					gsm.pressbtn(BTN_FIRE);
#else
				gsm.pressbtn(BTN_FIRE);
#endif
				gamecontrol.mouse_position(
						mouse_x - WMAIN_X - WMAIN_W/2,
						mouse_y - WMAIN_Y - WMAIN_H/2);
				switch(ev.button.button)
				{
				  case SDL_BUTTON_LEFT:
					mouse_left = 1;
					break;
				  case SDL_BUTTON_MIDDLE:
					mouse_middle = 1;
					break;
				  case SDL_BUTTON_RIGHT:
					mouse_right = 1;
					break;
				}
				gamecontrol.pressbtn(BTN_FIRE, GC_SRC_MOUSE);
			}
			break;
		  case SDL_MOUSEBUTTONUP:
			mouse_x = (int)(ev.motion.x / gengine->xscale()) - km.xoffs;
			mouse_y = (int)(ev.motion.y / gengine->yscale()) - km.yoffs;
			if(prefs->mouse)
			{
#ifdef ENABLE_TOUCHSCREEN
				// Resets all kinds of buttons that might have
				// been activated by clicking in the pointer
				// margin.
				if(pointer_margin_used)
				{
					gsm.release(BTN_EXIT);
					gsm.release(BTN_LEFT);
					gsm.release(BTN_RIGHT);
					gsm.release(BTN_UP);
					gsm.release(BTN_DOWN);
					pointer_margin_used = false;
				}
#endif
				gamecontrol.mouse_position(
						mouse_x - WMAIN_X - WMAIN_W/2,
						mouse_y - WMAIN_Y - WMAIN_H/2);
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
			}
			if(!mouse_left && !mouse_middle && !mouse_right)
			{
				if(prefs->mouse)
					gamecontrol.releasebtn(BTN_FIRE,
							GC_SRC_MOUSE);
				gsm.releasebtn(BTN_FIRE);
			}
			break;
		}
	}
	gamecontrol.process();

	// Run the game engine for one frame
	manage.run();

	// Run the current gamestate (application/UI state) for one frame
	gsm.frame();

	// Bump audio API timestamp time to match game logic time
	sound.timestamp_bump(gengine->period());

	if(prefs->cmd_autoshot && manage.game_in_progress())
	{
		static int c = 0;
		++c;
		if(c >= 3)
		{
			gengine->screenshot();
			c = 0;
		}
	}
}


void kobo_gfxengine_t::pre_render()
{
	gsm.pre_render();
}


void kobo_gfxengine_t::post_render()
{
	gsm.post_render();
#if 0
	if(!prefs->cmd_noframe)
	{
		woverlay->sprite(0, 0, B_FRAME_TL, 0, 0);
		woverlay->sprite(WSIZE - 16, 0, B_FRAME_TR, 0, 0);
		woverlay->sprite(0, WSIZE - 16, B_FRAME_BL, 0, 0);
		woverlay->sprite(WSIZE - 16, WSIZE - 16, B_FRAME_BR, 0, 0);
	}
#endif
#ifdef DEBUG
	if(prefs->cmd_debug)
	{
		char buf[20];
		snprintf(buf, sizeof(buf), "Obj: %d",
				gengine->objects_in_use());
		woverlay->font(B_NORMAL_FONT);
		woverlay->string(160, 5, buf);
	}
#endif

	// Frame rate counter
	int nt = (int)SDL_GetTicks();
	int tt = nt - km.fps_starttime;
	if((tt > 1000) && km.fps_count)
	{
		float f = km.fps_count * 1000.0 / tt;
		::screen.fps(f);
		km.fps_count = 0;
		km.fps_starttime = nt;
		if(prefs->cmd_fps)
		{
			char buf[20];
			snprintf(buf, sizeof(buf), "%.1f", f);
			km.dfps->text(buf);
			if(!km.fps_results)
				km.fps_results = (float *)
						calloc(MAX_FPS_RESULTS,
						sizeof(float));
			if(km.fps_results)
			{
				km.fps_results[km.fps_nextresult++] = f;
				if(km.fps_nextresult >= MAX_FPS_RESULTS)
					km.fps_nextresult = 0;
				if(km.fps_nextresult > km.fps_lastresult)
					km.fps_lastresult = km.fps_nextresult;
			}
		}
	}
	++km.fps_count;

	// Frame rate limiter
	if(prefs->maxfps)
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


/*----------------------------------------------------------
	main() and related stuff
----------------------------------------------------------*/

static void put_usage()
{
	printf("\nKobo Redux %s\n", KOBO_VERSION);
	printf("Usage: kobord [<options>]\n");
	printf("Recognized options:\n");
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
			fs = "	-[no]%-16.16s[%s]\t%s%s\n";
			break;
		  case CFG_INT:
		  case CFG_FLOAT:
			fs = "	-%-20.20s[%s]\t%s%s\n";
			break;
		  case CFG_STRING:
			fs = "	-%-20.20s[\"%s\"]\t%s%s\n";
			break;
		  default:
			continue;
		}
		printf(fs, prefs->name(s), prefs->get_default_s(s),
				prefs->do_save(s) ? "" : "(Not saved!) ",
				prefs->description(s));
	}
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
	"     ,--.   ,--.,--------. ,-------.  ,--------.\n"
	"     |  |  /  / |  ,--.  | |  ,--.  \\ |  ,--.  |\n"
	"     |  '-´  /  |__|  |__| |__`--´__/ |  |  |  |\n"
	"     |__,-.--\\  |  |  |  | |  ,--.  \\ |--|  |__|\n"
	"     |  |  \\  \\ |  `--'  | |  `--´  / |  `--´  |\n"
	"     `--´   `--'`--------´ `-------´  `--------´\n"
	"      R    -    E    -    D    -    U    -    X\n"
	"\n"
	"  Copyright 2015 Olofson Arcade. All rights reserved.\n"
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
	"  Copyright 1999-2009, 2015 David Olofson\n"
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
	};

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

	if(prefs->cmd_hiscores)
	{
		scorefile.gather_high_scores();
		scorefile.print_high_scores();
		cmd_exit = 1;
	}

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

	if(cmd_exit)
	{
		km.close_logging();
		main_cleanup();
		return 0;
	}

	if(km.open() < 0)
	{
		km.close_logging();
		main_cleanup();
		return 1;
	}

	km.run();

	sound.music(-1);
	km.noiseburst();

	km.close();

	/* 
	 * Seems like we got all the way here without crashing,
	 * so let's save the current configuration! :-)
	 */
	if(prefs->changed)
	{
		km.save_config(prefs);
		prefs->changed = 0;
	}

	if(prefs->cmd_fps)
		km.print_fps_results();

	km.close_logging();
	main_cleanup();
	return 0;
}
