/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2002 Jeremy Sheeley
 * Copyright 2001-2003, 2007, 2009 David Olofson
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

#ifndef _KOBO_MANAGE_H_
#define _KOBO_MANAGE_H_

#include "SDL.h"
#include "campaign.h"
#include "game.h"

enum KOBO_replaymodes
{
	RPM_PLAY,	// Playing!
	RPM_RETRY,	// Rewind and retry
	RPM_REPLAY	// Pure replay
};

enum KOBO_gamestates
{
	GS_NONE,
	GS_TITLE,
	GS_SHOW,
	GS_GETREADY,
	GS_PLAYING,
	GS_LEVELDONE,
	GS_GAMEOVER,
	GS_REPLAYEND
};

const char *enumstr(KOBO_replaymodes rpm);
const char *enumstr(KOBO_gamestates gst);

class _manage
{
	// Engine state
	static KOBO_gamestates gamestate;
	static KOBO_replaymodes replaymode;
	static bool is_paused;

	// Campaign and current replay
	static KOBO_campaign *campaign;
	static KOBO_replay *replay;
	static bool owns_replay;
	static KOBO_player_controls lastctrl;	// Previous control input state
	static unsigned ctrltimer;	// Frames since last input change

	// User interface
	static bool in_background;
	static bool show_bars;
	static float disp_health;
	static float disp_charge;
	static int flash_score_count;
	static bool score_changed;
	static int intro_x;
	static int intro_y;

	// Game logic
	static int game_seed;
	static int total_cores;
	static int remaining_cores;
	static int selected_slot;
	static int selected_stage;
	static int last_stage;
	static skill_levels_t selected_skill;
	static unsigned highscore;
	static unsigned score;
	static unsigned playtime;

	// Transition effects
	static bool scroll_jump;
	static int noise_flash;
	static int noise_duration;
	static int noise_timer;
	static float noise_level;

	// Asynchronous stage selection with transition effect
	static int delayed_stage;
	static KOBO_gamestates delayed_gamestate;

	// Delayed skips and rewinds with transition effect
	static int retry_skip;	// -1/0/+1
	static bool retry_rewind;

	// Camera lead
	static int cam_lead_x, cam_lead_y;
	static int cam_lead_xf, cam_lead_yf;

	// Screen shake
	static int shake_x, shake_y;
	static int shake_fade_x, shake_fade_y;

	// Timing
	static int delay_count;

	static void put_player_stats();
	static void put_info();
	static void put_score();
	static void flash_score();
	static void run_noise();
	static void run_leds();
	static void set_bars();

	static void next_bookmark();
	static void prev_bookmark();
	static void find_replay_forward();
	static void find_replay_reverse();
	static void next_stage();
	static void prev_stage();

	static void state(KOBO_gamestates gst);
	static void init_game(KOBO_replay *rp = NULL, bool newship = false);
	static void retry();
	static void update();
	static void run_intro();
	static void run_pause();
	static void run_game();
	static void finalize_replay();

	static KOBO_player_controls controls_play(KOBO_player_controls ctrl);
	static void controls_retry_skip(KOBO_player_controls ctrl);
	static KOBO_player_controls controls_retry(KOBO_player_controls ctrl);
	static KOBO_player_controls controls_replay(KOBO_player_controls ctrl);
  public:

	static void init();
	static void reenter();

	// Map management
	static int current_stage()	{ return selected_stage; }
	static int last_played_stage()	{ return last_stage; }

	// State management
	static void select_slot(int sl);
	static int current_slot()	{ return selected_slot; }
	static void start_intro();
	static void show_stage(int stage, KOBO_gamestates gs);
	static void select_stage(int stage, KOBO_gamestates gs = GS_SHOW);
	static void select_skill(int skill)
	{
		selected_skill = (skill_levels_t)skill;
	}
	static int current_skill()	{ return (int)selected_skill; }
	static void start_new_game();
	static bool continue_game();
	static bool start_replay(int stage);
	static void rewind();
	static void advance(int frame);
	static void player_ready();
	static void abort_game();

	static void pause(bool p);
	static bool paused()		{ return is_paused; }

	static void background(bool bg);
	static bool background()	{ return in_background; }

	static KOBO_gamestates state()	{ return gamestate; }

	// Replays
	static KOBO_replaymodes replay_mode()	{ return replaymode; }
	static float replay_progress();
	static int replay_stages();

	// Running the game
	static void run();

	// State info
	static bool game_in_progress()
	{
		// Return true only if an ACTUAL, live game is in progress!
		if(replaymode == RPM_REPLAY)
			return false;
		switch(state())
		{
		  case GS_GETREADY:
		  case GS_PLAYING:
		  case GS_LEVELDONE:
		  case GS_GAMEOVER:
		  case GS_REPLAYEND:
			return true;
		  default:
			return false;
		}
	}
	static bool ok_to_switch()
	{
		// Return true if it's OK to switch level for the nice scenery
		if(replaymode == RPM_REPLAY)
			return false;
		switch(state())
		{
		  case GS_NONE:
		  case GS_SHOW:
		  case GS_TITLE:
			return true;
		  default:
			return false;
		}
	}
	static unsigned game_time()	{ return playtime; }
	static int time_remaining();
	static int cores_total()	{ return total_cores; }
	static int cores_remaining()	{ return remaining_cores; }
	static unsigned current_score()	{ return score; }

	// Player input
	static void key_down(SDL_Keycode sym);
	static void key_up(SDL_Keycode sym);

	// Effects
	static void noise_glitch();
	static void noise_damage(float amt);
	static void screenshake(float mag_x, float mag_y, float fade);
	static void stop_screenshake();
	static void kill_screenshake();

	// Game logic interface
	static void lost_myship();
	static void destroyed_a_core();
	static void add_score(int sc);
};

extern _manage manage;

#endif	//_KOBO_MANAGE_H_
