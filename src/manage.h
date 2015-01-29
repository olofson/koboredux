/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2002 Jeremy Sheeley
 * Copyright 2001-2003, 2007, 2009 David Olofson
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

#ifndef _KOBO_MANAGE_H_
#define _KOBO_MANAGE_H_

#include "SDL.h"
#include "score.h"

class _manage
{
	static int blank;
	static int next_state_out;
	static int next_state_next;
	static int game_seed;
	static int bonus_next;
	static int scroll_jump;
	static int delay_count;
	static int introtime;
	static int rest_cores;
	static int exit_manage;
	static int playing;
	static int _get_ready;
	static int _game_over;
	static s_hiscore_t hi;
	static int noise_flash;
	static int noise_duration;
	static int noise_timer;
	static float noise_level;
	static int intro_x;
	static int intro_y;
	static int show_bars;

	static void next_scene();
	static void retry();

	static int scene_num;
	static int score;
	static float disp_health;
	static float disp_temp;
	static int flush_score_count;
	static int flush_ships_count;
	static int score_changed;
	static int ships_changed;
	static void put_health(int force = 0);
	static void put_temp(int force = 0);
	static void put_info();
	static void put_score();
	static void put_ships();
	static void flush_score();
	static void flush_ships();
	static void game_stop();
	static void run_noise();
  public:
	static int ships;
	static void init();
	static void set_bars();
	static void init_resources_title();
	static void init_resources_to_play(int newship);
	static void noise(int duration, int flash);
	static void noise_out(int duration);
	static void noise_damage(float amt);
	static void update();
	static void run_intro();
	static void run_pause();
	static void run_game();
	static void lost_myship();
	static void destroyed_a_core();
	static void add_score(int sc);
	static void key_down(SDL_Keycode sym);
	static void key_up(SDL_Keycode sym);
	static int title_blank()	{ return blank; }
	static void select_next(int redraw_map = 1);
	static void select_prev(int redraw_map = 1);
	static void regenerate();
	static void select_scene(int scene, int redraw_map = 1);
	static int scene()		{ return scene_num; }
	static void abort();
	static void freeze_abort();
	static int aborted()		{ return exit_manage; }
	static void reenter();
	static int game_stopped()	{ return !playing; }
	static int get_ready();
	static void game_start();
	static int game_over();
	static int game_time()		{ return hi.playtime; }
};

extern _manage manage;

#endif	//_KOBO_MANAGE_H_
