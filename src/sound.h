/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - Wrapper for Sound Control
------------------------------------------------------------
 * Copyright 2007, 2009 David Olofson
 * Copyright 2015-2016 David Olofson (Kobo Redux)
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

// Desired approximate update period for positional audio (ms)
#define	KOBO_SOUND_UPDATE_PERIOD	100

// Bank slots, for keeping track of multiple themes
#define	KOBO_SOUND_BANKS	8

#include "config.h"
#include "audiality2.h"

// Sound effects, songs and other program handles
#define KOBO_ALLSOUNDS			\
	KOBO_DEFS(NONE)			\
	KOBO_DEFS(G_MASTER)		\
	KOBO_DEFS(G_UI)			\
	KOBO_DEFS(G_SFX)		\
	KOBO_DEFS(G_MUSIC)		\
	KOBO_DEFS(G_TITLE)		\
	KOBO_DEFS(OAJINGLE)		\
	KOBO_DEFS(TITLESONG)		\
	KOBO_DEFS(INGAMESONG1)		\
	KOBO_DEFS(INGAMESONG2)		\
	KOBO_DEFS(INGAMESONG3)		\
	KOBO_DEFS(INGAMESONG4)		\
	KOBO_DEFS(INGAMESONG5)		\
	KOBO_DEFS(LAUNCH)		\
	KOBO_DEFS(LAUNCH_BULLET)	\
	KOBO_DEFS(LAUNCH_RING)		\
	KOBO_DEFS(LAUNCH_BOMB)		\
	KOBO_DEFS(DAMAGE)		\
	KOBO_DEFS(DAMAGE_M1)		\
	KOBO_DEFS(DAMAGE_M2)		\
	KOBO_DEFS(DAMAGE_M3)		\
	KOBO_DEFS(DAMAGE_M4)		\
	KOBO_DEFS(HIT_ROCK)		\
	KOBO_DEFS(EXPLO_NODE)		\
	KOBO_DEFS(EXPLO_ENEMY)		\
	KOBO_DEFS(EXPLO_PLAYER)		\
	KOBO_DEFS(EXPLO_RING)		\
	KOBO_DEFS(EXPLO_ROCK)		\
	KOBO_DEFS(EXPLO_M1)		\
	KOBO_DEFS(EXPLO_M2)		\
	KOBO_DEFS(EXPLO_M3)		\
	KOBO_DEFS(EXPLO_M4)		\
	KOBO_DEFS(RUMBLE)		\
	KOBO_DEFS(SHOT)			\
	KOBO_DEFS(METALLIC)		\
	KOBO_DEFS(BOMB_TRIG)		\
	KOBO_DEFS(BOMB_DETO)		\
	KOBO_DEFS(BOMB2_TRIG)		\
	KOBO_DEFS(BOMB2_DETO)		\
	KOBO_DEFS(BIGSPAWN)		\
	KOBO_DEFS(CORE)			\
	KOBO_DEFS(UI_OPEN)		\
	KOBO_DEFS(UI_OK)		\
	KOBO_DEFS(UI_MOVE)		\
	KOBO_DEFS(UI_TICK)		\
	KOBO_DEFS(UI_CDTICK)		\
	KOBO_DEFS(UI_GAMEOVER)		\
	KOBO_DEFS(UI_READY)		\
	KOBO_DEFS(UI_PLAY)		\
	KOBO_DEFS(UI_PAUSE)		\
	KOBO_DEFS(UI_CANCEL)		\
	KOBO_DEFS(UI_ERROR)		\
	KOBO_DEFS(UI_LOADER)		\
	KOBO_DEFS(UI_NOISE)

#define	KOBO_DEFS(x)	S_##x,
enum KOBO_sounds
{
	KOBO_ALLSOUNDS
	S__COUNT
};
#undef	KOBO_DEFS


class KOBO_sound
{
	static int	tsdcounter;

	// In-game sfx stuff
	static int	listener_x;
	static int	listener_y;
	static int	wrap_x;
	static int	wrap_y;
	static int	scale;
	static int	panscale;
	static unsigned	rumble;

	// Audiality 2 interface
	static A2_state *state;
	static A2_handle rootvoice;	// Engine root voice
	static A2_handle master_g;	// Master group
	static A2_handle ui_g;		// UI sfx group
	static A2_handle sfx_g;		// Ingame sfx group
	static A2_handle music_g;	// Ingame music group
	static A2_handle title_g;	// Title music group
	static int current_noise;	// Index of current noise playing, or 0
	static A2_handle noisehandle;	// Transition noise effect
	static A2_handle gunhandle;	// Currently playing player gun sound
	static A2_handle banks[KOBO_SOUND_BANKS];	// Loaded A2S modules
	static A2_handle sounds[S__COUNT];	// Sounds, songs etc
	static unsigned sbank[S__COUNT];	// Bank each sound belongs to
	static float buffer_latency;

	// Currently playing song
	static int current_song;	// enum index
	static A2_handle musichandle;	// A2 handle
	static bool music_is_ingame;	// Title or ingame group?

	static void init_mixdown();
	static bool checksound(int wid, const char *where);
	static void update_music(bool newsong);

	static inline float pref2vol(int v)
	{
		float f = (float)v / 100.0f;
		if(f < 1.0f)
			return f * f;
		else
			return f;
	}

	// Positional audio
	static bool eval_pos(int x, int y, float *vol, float *pan);

  public:
	KOBO_sound();
	~KOBO_sound();

	/*--------------------------------------------------
		Open/close
	--------------------------------------------------*/
	static int open();
	static void prefschange();
	static bool load(unsigned bank, const char *themepath,
			int (*prog)(const char *msg) = NULL);
	static void unload(int bank);
	static void close();

	/*--------------------------------------------------
		Info
	--------------------------------------------------*/
	static inline float latency()	{ return buffer_latency; }
	static const char *symname(unsigned wid);

	/*--------------------------------------------------
		Main controls
	--------------------------------------------------*/
	static void timestamp_reset();
	static void timestamp_nudge(float ms);
	static void timestamp_bump(float ms);
	static void frame();

	// Play anything through one of the music groups
	static void music(int sng, bool ingame = false);

	// Play sound/jingle directly through the master group
	static void jingle(int sng);

	// Play a user interface sound
	static void ui_play(unsigned wid, int vol = 65536, int pitch = 60<<16,
			int pan = 0);

	/*--------------------------------------------------
		In-game sound
	--------------------------------------------------*/
	// Play in-game music for the specified scene
	static void g_music(unsigned scene);

	// Set listener position
	static void g_position(int x, int y);

	// Set world size for wrapping
	static void g_wrap(int w, int h);

	// Set world scale. 'maxrange' is the distance where a sound is out of
	// the audible range.
	static void g_scale(int maxrange, int pan_maxrange);

	// Play a sound at a specific location on the map
	static void g_play(unsigned wid, int x, int y);

	// Start a sound at a specific location on the map. Returns handle.
	static int g_start(unsigned wid, int x, int y);

	// Update position of sound 'h', previously started with g_start().
	static void g_move(int h, int x, int y);

	// Change fx control 'c' of sound 'h' to 'v'.
	static void g_control(int h, int c, float v);

	// Stop sound 'h', previously started with g_start().
	static void g_stop(int h);

	// Play a sound right where the listener is
	static void g_play0(unsigned wid, int vol = 65536, int pitch = 60<<16);

	// Various sound effects
	static void g_player_fire(bool on);
	static void g_player_damage(float level = 1.0f);
	static void g_player_explo_start();

	// Quickly fade out and kill all in-game sound effects
	static void g_kill_all();

	/*--------------------------------------------------
		UI sound effects
	--------------------------------------------------*/
	// Low level interface
	static void ui_music_title();

	// Various UI effects
	static void ui_noise(int h);
	static void ui_countdown(int remain);
};

#endif /* KOBO_SOUND_H */
