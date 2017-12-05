/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - Wrapper for Sound Control
------------------------------------------------------------
 * Copyright 2007, 2009 David Olofson
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

#ifndef KOBO_SOUND_H
#define KOBO_SOUND_H

// Desired approximate update period for positional audio (ms)
#define	KOBO_SOUND_UPDATE_PERIOD	100

// Bank slots, for keeping track of multiple themes
#define	KOBO_SOUND_BANKS		8

// Crossfade time when switching to a new ingame SFX group with g_new_scene()
#define KOBO_SFX_XFADE_TIME		1000

#include "config.h"
#include "audiality2.h"

enum KOBO_mixer_group {
	KOBO_MG_ALL = -1,
	KOBO_MG_MASTER = 0,
	KOBO_MG_UI,
	KOBO_MG_SFX,
	KOBO_MG_MUSIC,
	KOBO_MG_TITLE,
	KOBO_MG__COUNT
};

// Sound effects, songs and other program handles
#define	KOBO_ENEMY_SFX_DEFS(x)		\
	KOBO_DEFS(x)			\
	KOBO_DEFS(x##_LAUNCH)		\
	KOBO_DEFS(x##_DAMAGE)		\
	KOBO_DEFS(x##_DEATH)

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
	KOBO_DEFS(EPILOGUE)		\
					\
	KOBO_ENEMY_SFX_DEFS(BULLET1)	\
	KOBO_ENEMY_SFX_DEFS(BULLET2)	\
	KOBO_ENEMY_SFX_DEFS(BULLET3)	\
	KOBO_ENEMY_SFX_DEFS(ROCK)	\
	KOBO_ENEMY_SFX_DEFS(RING)	\
	KOBO_ENEMY_SFX_DEFS(BOMB1)	\
	KOBO_DEFS(BOMB1_TRIG)		\
	KOBO_DEFS(BOMB1_DETONATE)	\
	KOBO_ENEMY_SFX_DEFS(BOMB2)	\
	KOBO_DEFS(BOMB2_TRIG)		\
	KOBO_DEFS(BOMB2_DETONATE)	\
	KOBO_ENEMY_SFX_DEFS(BASEEXPL)	\
	KOBO_ENEMY_SFX_DEFS(RINGEXPL)	\
	KOBO_ENEMY_SFX_DEFS(BLTX_GREEN)	\
	KOBO_ENEMY_SFX_DEFS(BLTX_RED)	\
	KOBO_ENEMY_SFX_DEFS(BLTX_BLUE)	\
	KOBO_ENEMY_SFX_DEFS(BOLTEXPL)	\
	KOBO_ENEMY_SFX_DEFS(ROCKEXPL)	\
	KOBO_ENEMY_SFX_DEFS(BOMBDETO)	\
	KOBO_ENEMY_SFX_DEFS(CANNON)	\
	KOBO_ENEMY_SFX_DEFS(CORE)	\
	KOBO_ENEMY_SFX_DEFS(PIPEIN)	\
	KOBO_ENEMY_SFX_DEFS(PIPEOUT)	\
	KOBO_ENEMY_SFX_DEFS(ENEMY1)	\
	KOBO_ENEMY_SFX_DEFS(ENEMY2)	\
	KOBO_ENEMY_SFX_DEFS(ENEMY3)	\
	KOBO_ENEMY_SFX_DEFS(ENEMY4)	\
	KOBO_ENEMY_SFX_DEFS(ENEMY5)	\
	KOBO_ENEMY_SFX_DEFS(ENEMY6)	\
	KOBO_ENEMY_SFX_DEFS(ENEMY7)	\
	KOBO_ENEMY_SFX_DEFS(ENEMY_M1)	\
	KOBO_DEFS(ENEMY_M1_BAILOUT)	\
	KOBO_ENEMY_SFX_DEFS(ENEMY_M2)	\
	KOBO_DEFS(ENEMY_M2_BAILOUT)	\
	KOBO_ENEMY_SFX_DEFS(ENEMY_M3)	\
	KOBO_DEFS(ENEMY_M3_BAILOUT)	\
	KOBO_ENEMY_SFX_DEFS(ENEMY_M4)	\
	KOBO_DEFS(ENEMY_M4_BAILOUT)	\
					\
	KOBO_DEFS(PLAYER_DAMAGE)	\
	KOBO_DEFS(PLAYER_DEATH)		\
	KOBO_DEFS(PLAYER_GUN)		\
	KOBO_DEFS(PLAYER_FIRE_DENIED)	\
	KOBO_DEFS(PLAYER_SHIELD)	\
					\
	KOBO_DEFS(NO_DAMAGE)		\
					\
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
	static float	volscale;
	static float	pitchshift;
	static unsigned	rumble;

	// Audiality 2 interface
	static A2_interface *iface;
	static A2_handle rootvoice;	// Engine root voice
	static A2_handle groups[KOBO_MG__COUNT];
	static int current_noise;	// Index of current noise playing, or 0
	static A2_handle noisehandle;	// Transition noise effect
	static A2_handle gunhandle;	// Currently playing player gun sound
	static A2_handle banks[KOBO_SOUND_BANKS];	// Loaded A2S modules
	static A2_handle sounds[S__COUNT];	// Sounds, songs etc
	static unsigned sbank[S__COUNT];	// Bank each sound belongs to
	static float buffer_latency;

	static bool shield_enabled;
	static A2_handle shieldhandle;

	// Currently playing song
	static int current_song;	// enum index
	static A2_handle musichandle;	// A2 handle
	static bool music_is_ingame;	// Title or ingame group?

	static void init_mixer_group(KOBO_mixer_group grp);
	static bool checksound(int wid, const char *where);
	static void update_music(bool newsong);
	static void start_player_gun();
	static void logsetup();

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
	static const char *grpname(unsigned grp);

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

	// Release handle 'h'
	static void g_release(int h);

	// Various sound effects
	static void g_player_fire();
	static void g_player_fire_denied();
	static void g_player_charge(float charge);
	static void g_player_charged_fire(float charge);
	static void g_player_damage(float level = 1.0f);
	static void g_player_explo_start();
	static void g_player_shield(bool enable);

	// Crossfade to a new ingame sfx group, then kill the old group
	static void g_new_scene(int fadetime = 0);

	// Control volume and pitch of all sound effects. g_volume() takes a
	// linear gain argument, where 1.0 is unity/unaffected. g_pitch()
	// takes 1.0/octave linear pitch, where 0.0 is nominal pitch.
	//
	// NOTE:
	//	To work properly, g_pitch() needs a sound theme where
	//	all sound effects respond correctly to program pP arguments,
	//	and npP arguments of the 3() message handlers.
	//
	static void g_volume(float volume);
	static float g_volume()		{ return volscale; }
	static void g_pitch(float pitch);
	static float g_pitch()		{ return pitchshift; }

	/*--------------------------------------------------
		UI sound effects
	--------------------------------------------------*/

	// Various UI effects
	static void ui_noise(int h);
	static void ui_countdown(int remain);

	/*--------------------------------------------------
		Low level API
	--------------------------------------------------*/

	// Play a sound at a specific location on the map
	static void play(int grp, unsigned wid);

	// Start a sound at a specific location on the map. Returns handle.
	static int start(int grp, unsigned wid);

	// Kill all sounds on the specified group. If this is the master group,
	// the whole mixer graph will be reinitialized.
	void kill_all(int grp);
};

#endif /* KOBO_SOUND_H */
