/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - Wrapper for Sound Control
------------------------------------------------------------
 * Copyright 2007, 2009 David Olofson
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

#ifndef KOBO_SOUND_H
#define KOBO_SOUND_H

#include "config.h"
#include "audiality2.h"


/* Sound effects, songs and other program handles */
#define KOBO_ALLSOUNDS	\
	KOBO_DEFS(OAJINGLE,	"OAJingle")\
	KOBO_DEFS(ONEUP,	"GiveEnergyMega")\
	KOBO_DEFS(FINE,		"")\
	KOBO_DEFS(BULLET,	"FireElectro")\
	KOBO_DEFS(RING,		"")\
	KOBO_DEFS(BOMB_DETO,	"Bomb")\
	KOBO_DEFS(EXPLO_NODE,	"SegmentDeath")\
	KOBO_DEFS(EXPLO_CORE,	"MegaDeath")\
	KOBO_DEFS(EXPLO_ENEMY,	"BoomerangDeath")\
	KOBO_DEFS(EXPLO_PLAYER,	"KoboDeath")\
	KOBO_DEFS(EXPLO_RING,	"LavaImpact")\
	KOBO_DEFS(EXPLO_ROCK,	"")\
	KOBO_DEFS(BZZZT,	"")\
	KOBO_DEFS(GAMEOVER,	"")\
	KOBO_DEFS(READY,	"WarbleUp")\
	KOBO_DEFS(PLAY,		"")\
	KOBO_DEFS(PAUSE,	"ScratchDown")\
	KOBO_DEFS(CANCEL,	"")\
	KOBO_DEFS(ERROR,	"")\
	KOBO_DEFS(LAUNCH2,	"Launch")\
	KOBO_DEFS(LAUNCH,	"Launch")\
	KOBO_DEFS(RUMBLE,	"BaseRumble")\
	KOBO_DEFS(SHOT_START,	"")\
	KOBO_DEFS(SHOT,		"FirePlasma")\
	KOBO_DEFS(SHOT_END,	"")\
	KOBO_DEFS(METALLIC,	"Klank")\
	KOBO_DEFS(DAMAGE,	"Impact")\
	KOBO_DEFS(MOVE,		"")\
	KOBO_DEFS(HIT_ROCK,	"ImpactGround")\
	KOBO_DEFS(TICK,		"Tick")\
	KOBO_DEFS(ENEMYM,	"Teleport")


#define	KOBO_DEFS(x, y)	SOUND_##x,
enum KOBO_sounds
{
	KOBO_ALLSOUNDS
	SOUND__COUNT
};
#undef	KOBO_DEFS


class KOBO_sound
{
	static int	sounds_loaded;
	static int	music_loaded;

	// In-game sfx stuff
	static int	time;
	static int	_period;
	static int	listener_x;
	static int	listener_y;
	static int	wrap_x;
	static int	wrap_y;
	static int	scale;
	static int	panscale;
	static int	firing;
	static unsigned	rumble;

	// Audiality 2 interface
	static A2_state *state;
	static A2_handle rootvoice;
	static A2_handle sfxbank;
	static A2_handle sounds[SOUND__COUNT];

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

	// Play a user interface sound
	static void play(unsigned wid, int vol = 65536, int pitch = 60<<16,
			int pan = 0);

	// Play a sound at a specific location on the map
	static void g_play(unsigned wid, int x, int y,
			int vol = 65536, int pitch = 60<<16);

	// Play a sound right where the listener is
	static void g_play0(unsigned wid, int vol = 65536, int pitch = 60<<16);

	/*--------------------------------------------------
		In-game sound
	--------------------------------------------------*/
	// Play in-game music
	static void g_music(unsigned scene);

	// Set listener position
	static void g_position(int x, int y);

	// Set world size for wrapping
	static void g_wrap(int w, int h);

	// Set world scale. 'maxrange' is the distance where a sound is out of
	// the audible range.
	static void g_scale(int maxrange, int pan_maxrange);

	// Various sound effects
	static void g_player_fire();
	static void g_player_fire_off();
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
	static void g_launch_bullet(int x, int y);
	static void g_launch_bomb(int x, int y);
	static void g_launch(int x, int y);
	static void g_m_launch_ring(int x, int y);
	static void g_m_launch_bullet(int x, int y);
	static void g_m_launch_bomb(int x, int y);
	static void g_m_launch(int x, int y);

	/*--------------------------------------------------
		UI sound effects
	--------------------------------------------------*/
	// Low level interface
	static void ui_music_title();

	// Various UI effects
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
