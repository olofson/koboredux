/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - Wrapper for Sound Control
------------------------------------------------------------
 * Copyright (C) 2007, 2009 David Olofson
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

#ifndef KOBO_SOUND_H
#define KOBO_SOUND_H

#include "kobosfx.h"

/* Sound groups */
#define	SOUND_GROUP_UI		0
#define	SOUND_GROUP_SFX		1
#define	SOUND_GROUP_UIMUSIC	2
#define	SOUND_GROUP_BGMUSIC	3


class KOBO_sound
{
	static int	sounds_loaded;
	static int	music_loaded;

	/* In-game sfx stuff */
	static int	time;
	static int	_period;
	static int	sfx2d_tag;
	static int	listener_x;
	static int	listener_y;
	static int	wrap_x;
	static int	wrap_y;
	static int	scale;
	static int	panscale;
	static int	firing;
	static int	overheat;
	static unsigned	rumble;

  public:
	KOBO_sound();
	~KOBO_sound();

	/*--------------------------------------------------
		Open/close
	--------------------------------------------------*/
	static int open();
	static void prefschange();
	static int load(int (*prog)(const char *msg), int force);
	static void stop();
	static void close();

	/*--------------------------------------------------
		Main controls
	--------------------------------------------------*/
	static void master_reverb(float rvb);
	static void set_boost(int vol_boost);
	static void sfx_volume(float vol);
	static void intro_volume(float vol);
	static void music_volume(float vol);
	static void period(int ms);	// 0 to reset time
	static void frame();
	static void run();

	/* Play a user interface sound */
	static void play(int wid, int vol = 65536, int pitch = 60<<16,
			int pan = 0);

	/* Play a sound at a specific location on the map */
	static void g_play(int wid, int x, int y,
			int vol = 65536, int pitch = 60<<16);

	/* Play a sound right where the listener is. */
	static void g_play0(int wid, int vol = 65536, int pitch = 60<<16);

	/*--------------------------------------------------
		In-game sound
	--------------------------------------------------*/
	/* Play in-game music */
	static void g_music(int wid);

	/* Set listener position */
	static void g_position(int x, int y);

	/* Set world size for wrapping */
	static void g_wrap(int w, int h);

	/*
	 * Set world scale. 'maxrange' is the distance where
	 * a sound is out of the audible range.
	 */
	static void g_scale(int maxrange, int pan_maxrange);

	/* Various sound effects */
	static void g_player_fire();
	static void g_player_fire_off();
	static void g_player_overheat(int classic = 0);
	static void g_player_damage();
	static void g_player_explo_start();
	static void g_bolt_hit(int x, int y);
	static void g_bolt_hit_rock(int x, int y);
	static void g_base_node_explo(int x, int y);
	static void g_base_core_explo(int x, int y);
	static void g_pipe_rumble(int x, int y);
	static void g_enemy_explo(int x, int y);
	static void g_rock_explo(int x, int y);
	static void g_ring_explo(int x, int y);
	static void g_bomb_deto(int x, int y);
	static void g_launch_ring(int x, int y);
	static void g_launch_beam(int x, int y);
	static void g_launch_bomb(int x, int y);
	static void g_launch(int x, int y);
	static void g_m_launch_ring(int x, int y);
	static void g_m_launch_beam(int x, int y);
	static void g_m_launch_bomb(int x, int y);
	static void g_m_launch(int x, int y);

	/*--------------------------------------------------
		UI sound effects
	--------------------------------------------------*/
	/* Low level interface */
	static void ui_music(int wid);

	/* Various UI effects */
	static void ui_noise(int n);
	static void ui_ok();
	static void ui_cancel();
	static void ui_move();
	static void ui_tick();
	static void ui_error();
	static void ui_play();
	static void ui_pause();
	static void ui_ready();
	static void ui_countdown(int remain);
	static void ui_oneup();
	static void ui_gameover();
};

#endif /* KOBO_SOUND_H */
