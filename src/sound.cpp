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

#include "sound.h"

#include "kobo.h"
#include "kobolog.h"
#include "random.h"
#include "audio.h"

int KOBO_sound::sounds_loaded = 0;
int KOBO_sound::music_loaded = 0;
int KOBO_sound::time = 0;
int KOBO_sound::_period = 30;
int KOBO_sound::sfx2d_tag = 0;
int KOBO_sound::listener_x = 0;
int KOBO_sound::listener_y = 0;
int KOBO_sound::wrap_x = 0;
int KOBO_sound::wrap_y = 0;
int KOBO_sound::scale = 65536 / 1000;
int KOBO_sound::panscale = 65536 / 700;
int KOBO_sound::firing = 0;
int KOBO_sound::overheat = 0;
unsigned KOBO_sound::rumble = 0;


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
	int res;
	int save_to_disk = 0;
	const char *ap = fmap->get("SFX>>", FM_DIR);
	if(!ap)
	{
		log_printf(ELOG, "Couldn't find audio data directory!\n");
		return -1;
	}
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

	return prog(NULL);
}


void KOBO_sound::prefschange()
{
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
}
	

int KOBO_sound::open()
{
	if(!prefs->use_sound)
	{
		log_printf(WLOG, "Sound disabled!\n");
		return 0;
	}

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

	g_wrap(MAP_SIZEX*CHIP_SIZEX, MAP_SIZEY*CHIP_SIZEY);
	g_scale(VIEWLIMIT * 3 / 2, VIEWLIMIT);

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

	prefschange();

	time = SDL_GetTicks();
	return 0;
}


void KOBO_sound::stop()
{
	audio_stop();
}


void KOBO_sound::close()
{
	audio_close();
	sounds_loaded = 0;
	music_loaded = 0;
}



/*--------------------------------------------------
	Main controls
--------------------------------------------------*/

void KOBO_sound::master_reverb(float rvb)
{
	int master_rvb = (int)(rvb * 65536.0);
	audio_bus_controlf(7, 0, ABC_SEND_MASTER, 0.0);
	audio_bus_control(7, 1, ABC_SEND_MASTER, master_rvb);
	if(master_rvb)
		audio_bus_control(7, 1, ABC_FX_TYPE, AFX_REVERB);
	else
		audio_bus_control(7, 1, ABC_FX_TYPE, AFX_NONE);
}


void KOBO_sound::set_boost(int boost)
{
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
}


void KOBO_sound::sfx_volume(float vol)
{
	audio_group_controlf(SOUND_GROUP_SFX, ACC_VOLUME,
			(float)prefs->sfx_vol * vol / 100.0f);
}


void KOBO_sound::intro_volume(float vol)
{
	audio_group_controlf(SOUND_GROUP_UIMUSIC, ACC_VOLUME,
			(float)prefs->intro_vol * vol / 100.0f);
}


void KOBO_sound::music_volume(float vol)
{
	audio_group_controlf(SOUND_GROUP_BGMUSIC, ACC_VOLUME,
			(float)prefs->music_vol * vol / 100.0f);
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
	int nc = audio_next_callback();
	if(time < nc)
		time = nc;
	else
		time -= 1 + (labs(time - nc) >> 5);
	audio_bump(time);
	time += _period;

	// Various sound control logic
	++rumble;	// Rumble timer
}


void KOBO_sound::run()
{
	audio_run();
}


/*--------------------------------------------------
	In-game sound
--------------------------------------------------*/

void KOBO_sound::g_music(int wid)
{
	audio_channel_control(SOUND_GROUP_BGMUSIC, AVT_FUTURE, ACC_PATCH, wid);
	audio_channel_control(SOUND_GROUP_BGMUSIC, AVT_FUTURE, ACC_PAN, 0);
	audio_channel_play(SOUND_GROUP_BGMUSIC, 0, 60<<16, 65536);
}


void KOBO_sound::play(int wid, int vol, int pitch, int pan)
{
	audio_channel_control(SOUND_GROUP_UI, AVT_FUTURE, ACC_PATCH, wid);
	audio_channel_control(SOUND_GROUP_UI, AVT_FUTURE, ACC_PAN, pan);
	audio_channel_play(SOUND_GROUP_UI, 0, pitch, vol);
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


void KOBO_sound::g_play(int wid, int x, int y, int vol, int pitch)
{
	int volume, vx, vy, pan;
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

	audio_channel_control(SOUND_GROUP_SFX, AVT_FUTURE, ACC_PATCH, wid);
	audio_channel_control(SOUND_GROUP_SFX, AVT_FUTURE, ACC_PAN, pan);
	audio_channel_control(SOUND_GROUP_SFX, AVT_FUTURE, ACC_VOLUME, volume);
	audio_channel_play(SOUND_GROUP_SFX, sfx2d_tag, pitch, vol);
	sfx2d_tag = (sfx2d_tag + 1) & 0xffff;
}


void KOBO_sound::g_play0(int wid, int vol, int pitch)
{
	audio_channel_control(SOUND_GROUP_SFX, AVT_FUTURE, ACC_PATCH, wid);
	audio_channel_control(SOUND_GROUP_SFX, AVT_FUTURE, ACC_PAN, 0);
	audio_channel_control(SOUND_GROUP_SFX, AVT_FUTURE, ACC_VOLUME, 65536);
	audio_channel_play(SOUND_GROUP_SFX, sfx2d_tag, 60<<16, vol);
	sfx2d_tag = (sfx2d_tag + 1) & 0xffff;
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
	overheat = 0;
}


void KOBO_sound::g_player_fire_off()
{
	if(firing)
	{
		firing = 0;
		g_play0(SOUND_SHOT_END, 20000);
	}
}


void KOBO_sound::g_player_overheat(int classic)
{
	if(overheat)
		return;
	overheat = 1;
	g_play0(SOUND_SHOT_OVERHEAT, (prefs->cannonloud << 13) / 75);
	if(!classic)
		g_play(SOUND_OVERHEAT, listener_x + 100, listener_y,
				(prefs->overheatloud << 14) / 100);
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
	g_play(SOUND_METALLIC1 + (pubrand.get() % 3),
			CS2PIXEL(x), CS2PIXEL(y),
			12000);
}


void KOBO_sound::g_bolt_hit_rock(int x, int y)
{
	g_play(SOUND_HIT1 + (pubrand.get() % 3),
			CS2PIXEL(x), CS2PIXEL(y),
			20000);
}


void KOBO_sound::g_base_node_explo(int x, int y)
{
	g_play(pubrand.get(8) > 128  ? SOUND_EXPLO_NODE1 : SOUND_EXPLO_NODE2,
			CS2PIXEL(x), CS2PIXEL(y));
	rumble = 0;
}


void KOBO_sound::g_base_core_explo(int x, int y)
{
	x = CS2PIXEL(x);
	y = CS2PIXEL(y);
	g_play(pubrand.get(8) > 128  ? SOUND_EXPLO_NODE1 : SOUND_EXPLO_NODE2,
			x, y, 100000, 62<<16);
	g_play(SOUND_FINE, x + 30, y, 65536, (60<<16) + 4000);
	g_play(SOUND_FINE, x - 30, y, 65536, (60<<16) - 4000);
	rumble = 0;
}


void KOBO_sound::g_pipe_rumble(int x, int y)
{
	if(rumble < 4 + pubrand.get(3))
		return;
	g_play(pubrand.get(8) > 128  ? SOUND_RUMBLE_NODE1 : SOUND_RUMBLE_NODE2,
			CS2PIXEL(x), CS2PIXEL(y));
	rumble = 0;
}


void KOBO_sound::g_enemy_explo(int x, int y)
{
	g_play(SOUND_EXPLO_ENEMY1 + (pubrand.get() % 2),
			CS2PIXEL(x), CS2PIXEL(y), 40000);
}


void KOBO_sound::g_rock_explo(int x, int y)
{
	g_play(SOUND_EXPLO_ROCK, CS2PIXEL(x), CS2PIXEL(y), 40000);
}


void KOBO_sound::g_ring_explo(int x, int y)
{
	g_play(SOUND_EXPLO_RING1 + (pubrand.get() % 3),
			CS2PIXEL(x), CS2PIXEL(y), 32768);
}


void KOBO_sound::g_bomb_deto(int x, int y)
{
	g_play(SOUND_BOMB_DETO, CS2PIXEL(x), CS2PIXEL(y), 40000);
}


void KOBO_sound::g_launch_ring(int x, int y)
{
	g_play(SOUND_RING, CS2PIXEL(x), CS2PIXEL(y));
}


void KOBO_sound::g_launch_beam(int x, int y)
{
	g_play(SOUND_BEAM, CS2PIXEL(x), CS2PIXEL(y));
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


void KOBO_sound::g_m_launch_beam(int x, int y)
{
	g_play(SOUND_BEAM, CS2PIXEL(x), CS2PIXEL(y));
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

void KOBO_sound::ui_music(int wid)
{
// KLUDGE UNTIL MIDI SONGS AND FX CONTROL IS FIXED!
	audio_bus_control(0, 1, ABC_FX_TYPE, AFX_NONE);
	audio_bus_controlf(0, 0, ABC_SEND_MASTER, 1.0);
	audio_bus_controlf(0, 0, ABC_SEND_BUS_7, 0.5);
// KLUDGE UNTIL MIDI SONGS AND FX CONTROL IS FIXED!
	audio_channel_control(SOUND_GROUP_UIMUSIC, AVT_FUTURE, ACC_PATCH, wid);
	audio_channel_play(SOUND_GROUP_UIMUSIC, 0, 60<<16, 65536);
}


void KOBO_sound::ui_noise(int n)
{
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
}


void KOBO_sound::ui_ok()
{
	play(SOUND_EXPLO_NODE1, 32768);
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
	play(SOUND_METALLIC1, 15000);
}


void KOBO_sound::ui_error()
{
	play(SOUND_OVERHEAT, 20000);
}


void KOBO_sound::ui_play()
{
	play(SOUND_PLAY, 32768, (50<<16) + 2000, -40000);
	play(SOUND_PLAY, 32768, (50<<16) - 2000, 40000);
}


void KOBO_sound::ui_pause()
{
	play(SOUND_PAUSE, 32768, (56<<16) + 2000, -40000);
	play(SOUND_PAUSE, 32768, (56<<16) - 2000, 40000);
}


void KOBO_sound::ui_ready()
{
	play(SOUND_READY, 15000, (60<<16) + 3000, -40000);
	play(SOUND_READY, 15000, (60<<16) - 3000, 40000);
}


void KOBO_sound::ui_countdown(int remain)
{
	play(SOUND_TICK, 20000, (65 - remain)<<16);
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
