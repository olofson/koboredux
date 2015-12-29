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

// Sound effects, songs and other program handles
#define KOBO_ALLSOUNDS						\
	KOBO_DEFS(NONE,		"")				\
	KOBO_DEFS(G_MASTER,	"MasterGroup")			\
	KOBO_DEFS(G_UI,		"UIGroup")			\
	KOBO_DEFS(G_SFX,	"SFXGroup")			\
	KOBO_DEFS(G_MUSIC,	"MusicGroup")			\
	KOBO_DEFS(G_TITLE,	"TitleMusicGroup")		\
	KOBO_DEFS(OAJINGLE,	"OAJingle")			\
	KOBO_DEFS(TITLESONG,	"TitleSong")			\
	KOBO_DEFS(INGAMESONG1,	"IngameSong")			\
	KOBO_DEFS(INGAMESONG2,	"IngameSong")			\
	KOBO_DEFS(INGAMESONG3,	"IngameSong")			\
	KOBO_DEFS(INGAMESONG4,	"IngameSong")			\
	KOBO_DEFS(INGAMESONG5,	"IngameSong")			\
	KOBO_DEFS(ONEUP,	"GiveEnergyMega")		\
	KOBO_DEFS(FINE,		"")				\
	KOBO_DEFS(LAUNCH,	"Launch")			\
	KOBO_DEFS(LAUNCH_BULLET,"FireElectro")			\
	KOBO_DEFS(LAUNCH_RING,	"")				\
	KOBO_DEFS(LAUNCH_BOMB,	"Launch")			\
	KOBO_DEFS(DAMAGE,	"Impact")			\
	KOBO_DEFS(DAMAGE_CORE,	"DamageCore")			\
	KOBO_DEFS(DAMAGE_M1,	"Impact")			\
	KOBO_DEFS(DAMAGE_M2,	"Impact")			\
	KOBO_DEFS(DAMAGE_M3,	"Impact")			\
	KOBO_DEFS(DAMAGE_M4,	"Impact")			\
	KOBO_DEFS(HIT_ROCK,	"ImpactGround")			\
	KOBO_DEFS(EXPLO_NODE,	"SegmentDeath")			\
	KOBO_DEFS(EXPLO_CORE,	"MegaDeath")			\
	KOBO_DEFS(EXPLO_ENEMY,	"BoomerangDeath")		\
	KOBO_DEFS(EXPLO_PLAYER,	"KoboDeath")			\
	KOBO_DEFS(EXPLO_RING,	"LavaImpact")			\
	KOBO_DEFS(EXPLO_ROCK,	"")				\
	KOBO_DEFS(EXPLO_M1,	"MegaDeath")			\
	KOBO_DEFS(EXPLO_M2,	"MegaDeath")			\
	KOBO_DEFS(EXPLO_M3,	"MegaDeath")			\
	KOBO_DEFS(EXPLO_M4,	"MegaDeath")			\
	KOBO_DEFS(BZZZT,	"")				\
	KOBO_DEFS(RUMBLE,	"BaseRumble")			\
	KOBO_DEFS(SHOT,		"FirePlasma")			\
	KOBO_DEFS(METALLIC,	"Klank")			\
	KOBO_DEFS(BOMB_DETO,	"Bomb")				\
	KOBO_DEFS(ENEMYM,	"Teleport")			\
	KOBO_DEFS(UI_OPEN,	"UIOpen")			\
	KOBO_DEFS(UI_OK,	"UIOK")				\
	KOBO_DEFS(UI_MOVE,	"UIMove")			\
	KOBO_DEFS(UI_TICK,	"UITick")			\
	KOBO_DEFS(UI_CDTICK,	"UICountdownTick")		\
	KOBO_DEFS(UI_GAMEOVER,	"UIGameOver")			\
	KOBO_DEFS(UI_READY,	"UIReady")			\
	KOBO_DEFS(UI_PLAY,	"UIPlay")			\
	KOBO_DEFS(UI_PAUSE,	"UIPause")			\
	KOBO_DEFS(UI_CANCEL,	"UICancel")			\
	KOBO_DEFS(UI_ERROR,	"UIError")			\
	KOBO_DEFS(UI_LOADER,	"UILoader")			\
	KOBO_DEFS(UI_NOISE,	"UINoise")

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
	static int	tsdcounter;

	// In-game sfx stuff
	static int	listener_x;
	static int	listener_y;
	static int	wrap_x;
	static int	wrap_y;
	static int	scale;
	static int	panscale;
	static unsigned	rumble;
	static bool	firing;

	// Audiality 2 interface
	static A2_state *state;
	static A2_handle rootvoice;	// Engine root voice
	static A2_handle master_g;	// Master group
	static A2_handle ui_g;		// UI sfx group
	static A2_handle sfx_g;		// Ingame sfx group
	static A2_handle music_g;	// Ingame music group
	static A2_handle title_g;	// Title music group
	static A2_handle noisehandle;	// Transition noise effect
	static A2_handle gunhandle;	// Currently playing player gun sound
	static A2_handle *modules;	// Loaded A2S modules
	static A2_handle sounds[SOUND__COUNT];	// Sounds, songs etc
	static float buffer_latency;

	// Currently playing song
	static int current_song;	// enum index
	static A2_handle musichandle;	// A2 handle
	static bool music_is_ingame;	// Title or ingame group?

	static int load_a2s(const char *path);
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

  public:
	KOBO_sound();
	~KOBO_sound();

	/*--------------------------------------------------
		Open/close
	--------------------------------------------------*/
	static int open();
	static void prefschange();
	static int load(int (*prog)(const char *msg), int force);
	static void close();

	/*--------------------------------------------------
		Info
	--------------------------------------------------*/
	static inline float latency()	{ return buffer_latency; }

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

	// Play a sound at a specific location on the map
	static void g_play(unsigned wid, int x, int y);

	// Play a sound right where the listener is
	static void g_play0(unsigned wid, int vol = 65536, int pitch = 60<<16);

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

	// Various sound effects
	static void g_player_fire(bool on);
	static void g_player_damage(float level = 1.0f);
	static void g_player_explo_start();

	/*--------------------------------------------------
		UI sound effects
	--------------------------------------------------*/
	// Low level interface
	static void ui_music_title();

	// Various UI effects
	static void ui_noise(int n);
	static void ui_countdown(int remain);
};

#endif /* KOBO_SOUND_H */
