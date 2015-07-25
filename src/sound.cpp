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

#include "sound.h"
#include "kobo.h"
#include "kobolog.h"
#include "random.h"

int KOBO_sound::sounds_loaded = 0;
int KOBO_sound::music_loaded = 0;

int KOBO_sound::time = 0;
int KOBO_sound::_period = 30;
int KOBO_sound::listener_x = 0;
int KOBO_sound::listener_y = 0;
int KOBO_sound::wrap_x = 0;
int KOBO_sound::wrap_y = 0;
int KOBO_sound::scale = 65536 / 1000;
int KOBO_sound::panscale = 65536 / 700;
int KOBO_sound::firing = 0;
unsigned KOBO_sound::rumble = 0;

A2_state *KOBO_sound::state = NULL;
A2_handle KOBO_sound::rootvoice = 0;
A2_handle KOBO_sound::sfxbank = 0;
A2_handle KOBO_sound::sounds[SOUND__COUNT];


#define	KOBO_DEFS(x, y)	y,
static const char *kobo_soundnames[] =
{
	KOBO_ALLSOUNDS
};
#undef	KOBO_DEFS


KOBO_sound::KOBO_sound()
{
}


KOBO_sound::~KOBO_sound()
{
	close();
}


/*--------------------------------------------------
	Open/close
--------------------------------------------------*/


int KOBO_sound::load(int (*prog)(const char *msg), int force)
{
	int i, h;
	const char *p = fmap->get("SFX>>sfx.a2s");
	if(!p)
	{
		log_printf(ELOG, "Couldn't find sound effects!\n");
		return -1;
	}

	if((h = a2_Load(state, p, 0)) < 0)
	{
		log_printf(ELOG, "Couldn't load sound effects! (%s)\n",
				a2_ErrorString((A2_errors)-h));
		return -1;
	}
	sfxbank = h;

	for(i = 0; i < SOUND__COUNT; ++i)
	{
		h = a2_Get(state, sfxbank, kobo_soundnames[i]);
		if(h < 0)
		{
			log_printf(WLOG, "Couldn't find sound effect \"%s\"!"
					" (%s)\n", kobo_soundnames[i],
					a2_ErrorString((A2_errors)-h));
		}
		else
			sounds[i] = h;
	}
#if 0
	audio_set_path(ap);

	if(!sounds_loaded || force)
	{
		if(prog("Loading sound effects"))
			return -999;
		res = -1;
		if(prefs->cached_sounds && !force)
		{
			res = audio_wave_load(0, "sfx_c.agw", 0);
			if(res < 0)
				save_to_disk = 1;
		}
		if(res < 0)
			res = audio_wave_load(0, "sfx.agw", 0);
		if(res >= 0)
			sounds_loaded = 1;
		else
			log_printf(ELOG, "Could not load sound effects!\n");
		prog(NULL);
	}

	if((prefs->use_music && !music_loaded) || force)
	{
		if(prog("Loading music"))
			return -999;
		res = -1;
		if(prefs->cached_sounds && !force)
		{
			res = audio_wave_load(0, "music_c.agw", 0);
			if(res < 0)
				save_to_disk = 1;
		}
		if(res < 0)
			res = audio_wave_load(0, "music.agw", 0);
		if(res >= 0)
			music_loaded = 1;
		else
			log_printf(ELOG, "Could not load music!\n");
		prog(NULL);
	}

	if(save_to_disk)
		if(prog("Preparing audio engine"))
			return -999;

	audio_wave_prepare(-1);

	if(save_to_disk)
	{
		if(prog("Writing sounds to disk"))
			return -999;
		if(audio_wave_load(0, "save_waves.agw", 0) < 0)
			log_printf(ELOG, "Could not save sounds to disk!\n");
	}

	audio_wave_info(-1);
#endif
	return prog(NULL);
}


void KOBO_sound::prefschange()
{
#if 0
	/* Levels */
//	audio_master_volume((float)prefs->volume/100.0);
	audio_group_controlf(SOUND_GROUP_UI, ACC_VOLUME,
			(float)prefs->intro_vol/100.0);
	audio_group_controlf(SOUND_GROUP_UIMUSIC, ACC_VOLUME,
			(float)prefs->intro_vol/100.0);
	audio_group_controlf(SOUND_GROUP_SFX, ACC_VOLUME,
			(float)prefs->sfx_vol/100.0);
	audio_group_controlf(SOUND_GROUP_BGMUSIC, ACC_VOLUME,
			(float)prefs->music_vol/100.0);
	set_boost(prefs->vol_boost);
	audio_quality((audio_quality_t)prefs->mixquality);

	// Bus 7: Our "Master Reverb Bus"
	master_reverb((float)prefs->reverb/100.0);
#endif
}
	

int KOBO_sound::open()
{
	if(!prefs->use_sound)
	{
		log_printf(WLOG, "Sound disabled!\n");
		return 0;
	}

	log_printf(ULOG, "Initializing audio...\n");
	log_printf(ULOG, "   Sample rate: %d Hz\n", prefs->samplerate);
	log_printf(ULOG, "       Latency: %d ms\n", prefs->latency);

	A2_config *cfg = a2_OpenConfig(
			prefs->samplerate,
			prefs->samplerate * prefs->latency / 1000,
			2,
			A2_REALTIME | A2_TIMESTAMP | A2_STATECLOSE);
	if(!cfg)
	{
		log_printf(ELOG, "Couldn't create audio configuration;"
				" disabling sound effects.\n");
		return -1;
	}
#if 0
	//TODO
	if(audiodriver)
		if(a2_AddDriver(cfg, a2_NewDriver(A2_AUDIODRIVER,
				audiodriver)))
			...
#endif

	if(!(state = a2_Open(cfg)))
	{
		log_printf(ELOG, "Couldn't create audio engine state;"
				" disabling sound effects.\n");
		return -2;
	}

	rootvoice = a2_RootVoice(state);

#if 0

	if(audio_start(prefs->samplerate, prefs->latency, prefs->use_oss, prefs->cmd_midi,
			prefs->cmd_pollaudio) < 0)
	{
		log_printf(ELOG, "Couldn't initialize audio;"
				" disabling sound effects.\n");
		return -1;
	}

	// Channel grouping. We use only one chanel per group here, so we
	// just assign the first channels to the available groups.
	for(int i = 0; i < AUDIO_MAX_GROUPS; ++i)
		audio_channel_control(i, -1, ACC_GROUP, i);

	// Dirty hack for the music; the sequencer uses channels 16..31.
	for(int i = 16; i < 32; ++i)
		audio_channel_control(i, -1, ACC_GROUP, SOUND_GROUP_BGMUSIC);

	// Higher priority for music and UI effects
	audio_channel_control(SOUND_GROUP_BGMUSIC, AVT_ALL, ACC_PRIORITY, 1);
	audio_channel_control(SOUND_GROUP_UIMUSIC, AVT_ALL, ACC_PRIORITY, 2);
	audio_channel_control(SOUND_GROUP_UI, AVT_ALL, ACC_PRIORITY, 3);
	audio_channel_control(SOUND_GROUP_SFX, AVT_ALL, ACC_PRIORITY, 4);
#endif
	g_wrap(MAP_SIZEX*CHIP_SIZEX, MAP_SIZEY*CHIP_SIZEY);
	g_scale(VIEWLIMIT * 3 / 2, VIEWLIMIT);
#if 0
	// For the noise "bzzzt" effect :-)
	audio_channel_control(3, -1, ACC_PRIM_BUS, 1);

	// Bus 0: Sound effects
	audio_bus_control(0, 1, ABC_FX_TYPE, AFX_NONE);
	audio_bus_controlf(0, 0, ABC_SEND_MASTER, 1.0);
	audio_bus_controlf(0, 0, ABC_SEND_BUS_7, 0.5); // Always a little rvb!

	// Bus 1: Sound effects with less reverb
	audio_bus_control(1, 1, ABC_FX_TYPE, AFX_NONE);
	audio_bus_controlf(1, 0, ABC_SEND_MASTER, 1.0);
	audio_bus_controlf(1, 0, ABC_SEND_BUS_7, 0.1);
#endif
	prefschange();

	time = SDL_GetTicks();
	return 0;
}


void KOBO_sound::stop()
{
}


void KOBO_sound::close()
{
	if(state)
	{
		a2_Close(state);
		state = NULL;
	}
	sounds_loaded = 0;
	music_loaded = 0;
	rootvoice = 0;
	sfxbank = 0;
	memset(sounds, 0, sizeof(sounds));
}



/*--------------------------------------------------
	Main controls
--------------------------------------------------*/

void KOBO_sound::master_reverb(float rvb)
{
#if 0
	int master_rvb = (int)(rvb * 65536.0);
	audio_bus_controlf(7, 0, ABC_SEND_MASTER, 0.0);
	audio_bus_control(7, 1, ABC_SEND_MASTER, master_rvb);
	if(master_rvb)
		audio_bus_control(7, 1, ABC_FX_TYPE, AFX_REVERB);
	else
		audio_bus_control(7, 1, ABC_FX_TYPE, AFX_NONE);
#endif
}


void KOBO_sound::set_boost(int boost)
{
#if 0
	switch(boost)
	{
	  case 0:
		audio_set_limiter(1.0, 5.0);
		break;
	  case 1:
		audio_set_limiter(.9, 4.5);
		break;
	  case 2:
	  default:
		audio_set_limiter(.75, 4.0);
		break;
	  case 3:
		audio_set_limiter(.5, 3.5);
		break;
	  case 4:
		audio_set_limiter(.25, 3.0);
		break;
	}
#endif
}


void KOBO_sound::sfx_volume(float vol)
{
#if 0
	audio_group_controlf(SOUND_GROUP_SFX, ACC_VOLUME,
			(float)prefs->sfx_vol * vol / 100.0f);
#endif
}


void KOBO_sound::intro_volume(float vol)
{
#if 0
	audio_group_controlf(SOUND_GROUP_UIMUSIC, ACC_VOLUME,
			(float)prefs->intro_vol * vol / 100.0f);
#endif
}


void KOBO_sound::music_volume(float vol)
{
#if 0
	audio_group_controlf(SOUND_GROUP_BGMUSIC, ACC_VOLUME,
			(float)prefs->music_vol * vol / 100.0f);
#endif
}


void KOBO_sound::period(int ms)
{
	if(ms > 0)
		_period = ms;
	time = 0;
}


void KOBO_sound::frame()
{
#if 0
	// Positional audio test
	static int aaa = 0;
	if(++aaa > 20)
		aaa = 0;
	if(aaa == 10)
		g_play(SOUND_OVERHEAT, listener_x + 128, listener_y, 32768, 55<<16);
	else if (aaa == 20)
		g_play(SOUND_OVERHEAT, listener_x - 128, listener_y, 32768, 67<<16);
#endif
	// Advance to next game logic frame
	if(state)
		a2_Now(state);
#if 0
	int nc = audio_next_callback();
	if(time < nc)
		time = nc;
	else
		time -= 1 + (labs(time - nc) >> 5);
	audio_bump(time);
#endif
	time += _period;

	// Various sound control logic
	rumble = 0;	// Only one per logic frame!
}


void KOBO_sound::run()
{
}


/*--------------------------------------------------
	In-game sound
--------------------------------------------------*/

void KOBO_sound::g_music(unsigned scene)
{
#if 0
	if(!state || !sounds[wid])
		return;
	a2_Play(state, rootvoice, sounds[wid]);
#endif
}


void KOBO_sound::play(unsigned wid, int vol, int pitch, int pan)
{
	if(!state || !sounds[wid])
		return;
	a2_Play(state, rootvoice, sounds[wid],
			(pitch / 65536.0f - 60.0f)  / 12.0f, vol / 65536.0f,
			pan / 65536.0f);
}


void KOBO_sound::g_position(int x, int y)
{
	listener_x = x;
	listener_y = y;
}


void KOBO_sound::g_wrap(int w, int h)
{
	wrap_x = w;
	wrap_y = h;
}


void KOBO_sound::g_scale(int maxrange, int pan_maxrange)
{
	scale = 65536 / maxrange;
	panscale = 65536 / pan_maxrange;
}


void KOBO_sound::g_play(unsigned wid, int x, int y, int vol, int pitch)
{
	int volume, vx, vy, pan;

	if(!state || !sounds[wid])
		return;

	/* Calculate volume */
	x -= listener_x;
	y -= listener_y;
	if(wrap_x)
	{
		x += wrap_x / 2;
		while(x < 0)
			x += wrap_x;
		x %= wrap_x;
		x -= wrap_x / 2;
	}
	if(wrap_y)
	{
		y += wrap_y / 2;
		while(y < 0)
			y += wrap_y;
		y %= wrap_y;
		y -= wrap_y / 2;
	}

	/* Approximation of distance attenuation */
	vx = abs(x * scale);
	vy = abs(y * scale);
	if((vx | vy) & 0xffff0000)
		return;

	vx = (65536 - vx) >> 1;
	vy = (65536 - vy) >> 1;
	volume = vx * vy >> 14;
	volume = (volume>>1) * (volume>>1) >> 14;

	pan = x * panscale;
	if(pan < -65536)
		pan = -65536;
	else if(pan > 65536)
		pan = 65536;

	a2_Play(state, rootvoice, sounds[wid],
			(pitch / 65536.0f - 60.0f)  / 12.0f,
			volume / 65536.0f, pan / 65536.0f);
}


void KOBO_sound::g_play0(unsigned wid, int vol, int pitch)
{
	if(!state || !sounds[wid])
		return;
	a2_Play(state, rootvoice, sounds[wid],
			(pitch / 65536.0f - 60.0f)  / 12.0f,
			vol / 65536.0f);
}


void KOBO_sound::g_player_fire()
{
	if(firing)
		g_play0(SOUND_SHOT, (prefs->cannonloud << 13) / 100);
	else
	{
		firing = 1;
		g_play0(SOUND_SHOT_START, 20000);
	}
}


void KOBO_sound::g_player_fire_off()
{
	if(firing)
	{
		firing = 0;
		g_play0(SOUND_SHOT_END, 20000);
	}
}


void KOBO_sound::g_player_damage()
{
	int p0 = (60<<16) + (pubrand.get(9) << 8);
	g_play(SOUND_DAMAGE, listener_x - 100, listener_y,
			65536, p0 + 3000);
	g_play(SOUND_DAMAGE, listener_x + 100, listener_y,
			65536, p0 - 3000);
}


void KOBO_sound::g_player_explo_start()
{
	g_player_damage();
	g_play(SOUND_EXPLO_PLAYER, listener_x - 100, listener_y,
			65536, (55<<16) + 4000);
	g_play(SOUND_EXPLO_PLAYER, listener_x + 100, listener_y,
			65536, (55<<16) - 4000);
}


void KOBO_sound::g_bolt_hit(int x, int y)
{
	g_play(SOUND_METALLIC, CS2PIXEL(x), CS2PIXEL(y), 8000);
}


void KOBO_sound::g_bolt_hit_rock(int x, int y)
{
	g_play(SOUND_HIT_ROCK, CS2PIXEL(x), CS2PIXEL(y), 20000);
}


void KOBO_sound::g_base_node_explo(int x, int y)
{
	g_play(SOUND_EXPLO_NODE, CS2PIXEL(x), CS2PIXEL(y));
}


void KOBO_sound::g_base_core_explo(int x, int y)
{
	x = CS2PIXEL(x);
	y = CS2PIXEL(y);
	g_play(SOUND_EXPLO_CORE, x, y, 100000, 60<<16);
	g_play(SOUND_FINE, x + 30, y, 65536, (60<<16) + 4000);
	g_play(SOUND_FINE, x - 30, y, 65536, (60<<16) - 4000);
}


void KOBO_sound::g_pipe_rumble(int x, int y)
{
	if(rumble)
		return;
	g_play(SOUND_RUMBLE, CS2PIXEL(x), CS2PIXEL(y));
	rumble = 1;
}


void KOBO_sound::g_enemy_explo(int x, int y)
{
	g_play(SOUND_EXPLO_ENEMY, CS2PIXEL(x), CS2PIXEL(y), 40000);
}


void KOBO_sound::g_rock_explo(int x, int y)
{
	g_play(SOUND_EXPLO_ROCK, CS2PIXEL(x), CS2PIXEL(y), 40000);
}


void KOBO_sound::g_ring_explo(int x, int y)
{
	g_play(SOUND_EXPLO_RING, CS2PIXEL(x), CS2PIXEL(y), 32768);
}


void KOBO_sound::g_bomb_deto(int x, int y)
{
	g_play(SOUND_BOMB_DETO, CS2PIXEL(x), CS2PIXEL(y), 40000);
}


void KOBO_sound::g_launch_ring(int x, int y)
{
	g_play(SOUND_RING, CS2PIXEL(x), CS2PIXEL(y));
}


void KOBO_sound::g_launch_bullet(int x, int y)
{
	g_play(SOUND_BULLET, CS2PIXEL(x), CS2PIXEL(y));
}


void KOBO_sound::g_launch_bomb(int x, int y)
{
	g_play(SOUND_LAUNCH2, CS2PIXEL(x), CS2PIXEL(y));
}


void KOBO_sound::g_launch(int x, int y)
{
	g_play(SOUND_LAUNCH, CS2PIXEL(x), CS2PIXEL(y));
}


void KOBO_sound::g_m_launch_ring(int x, int y)
{
	g_play(SOUND_RING, CS2PIXEL(x), CS2PIXEL(y));
	g_play(SOUND_ENEMYM, CS2PIXEL(x), CS2PIXEL(y));
}


void KOBO_sound::g_m_launch_bullet(int x, int y)
{
	g_play(SOUND_BULLET, CS2PIXEL(x), CS2PIXEL(y));
	g_play(SOUND_ENEMYM, CS2PIXEL(x), CS2PIXEL(y));
}


void KOBO_sound::g_m_launch_bomb(int x, int y)
{
	g_play(SOUND_LAUNCH2, CS2PIXEL(x), CS2PIXEL(y));
	g_play(SOUND_ENEMYM, CS2PIXEL(x), CS2PIXEL(y));
}


void KOBO_sound::g_m_launch(int x, int y)
{
	g_play(SOUND_LAUNCH, CS2PIXEL(x), CS2PIXEL(y));
	g_play(SOUND_ENEMYM, CS2PIXEL(x), CS2PIXEL(y));
}


/*--------------------------------------------------
	UI sound effects
--------------------------------------------------*/

void KOBO_sound::ui_music_title()
{
#if 0
// KLUDGE UNTIL MIDI SONGS AND FX CONTROL IS FIXED!
	audio_bus_control(0, 1, ABC_FX_TYPE, AFX_NONE);
	audio_bus_controlf(0, 0, ABC_SEND_MASTER, 1.0);
	audio_bus_controlf(0, 0, ABC_SEND_BUS_7, 0.5);
// KLUDGE UNTIL MIDI SONGS AND FX CONTROL IS FIXED!
	audio_channel_control(SOUND_GROUP_UIMUSIC, AVT_FUTURE, ACC_PATCH, wid);
	audio_channel_play(SOUND_GROUP_UIMUSIC, 0, 60<<16, 65536);
#endif
}


void KOBO_sound::ui_noise(int n)
{
#if 0
	if(n < 0)
	{
		sound.sfx_volume(0.0f);
		audio_channel_control(SOUND_GROUP_UI,
				AVT_FUTURE, ACC_PATCH, SOUND_BZZZT);
		audio_channel_play(SOUND_GROUP_UI, 0, 60<<16,
				10000 + pubrand.get(14));
		return;
	}
	if(n && pubrand.get(1))
	{
		sound.sfx_volume(0.0f);
		audio_channel_control(SOUND_GROUP_UI,
				AVT_FUTURE, ACC_PATCH, SOUND_BZZZT);
		audio_channel_play(SOUND_GROUP_UI, 0, 60<<16,
				20000 + pubrand.get(14));
	}
	else
		sound.sfx_volume(1.0f);
#endif
}


void KOBO_sound::ui_ok()
{
	play(SOUND_EXPLO_NODE, 32768);
}


void KOBO_sound::ui_cancel()
{
	play(SOUND_CANCEL, 20000);
}


void KOBO_sound::ui_move()
{
	play(SOUND_MOVE, 20000);
}


void KOBO_sound::ui_tick()
{
	play(SOUND_METALLIC, 15000);
}


void KOBO_sound::ui_error()
{
	play(SOUND_ERROR, 20000);
}


void KOBO_sound::ui_play()
{
	play(SOUND_PLAY, 32768, (60<<16) + 2000, -40000);
	play(SOUND_PLAY, 32768, (60<<16) - 2000, 40000);
}


void KOBO_sound::ui_pause()
{
	play(SOUND_PAUSE, 32768, (60<<16) + 2000, -40000);
	play(SOUND_PAUSE, 32768, (60<<16) - 2000, 40000);
}


void KOBO_sound::ui_ready()
{
	play(SOUND_READY, 15000, (60<<16) + 3000, -40000);
	play(SOUND_READY, 15000, (60<<16) - 3000, 40000);
}


void KOBO_sound::ui_countdown(int remain)
{
	play(SOUND_TICK, 20000, (60 - remain)<<16);
}


void KOBO_sound::ui_oneup()
{
	play(SOUND_ONEUP, 32768, (60<<16) + 5000, -40000);
	play(SOUND_ONEUP, 32768, (60<<16) - 5000, 40000);
}


void KOBO_sound::ui_gameover()
{
	play(SOUND_GAMEOVER);
}
