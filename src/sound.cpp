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

#include "sound.h"
#include "kobo.h"
#include "game.h"
#include "kobolog.h"
#include "random.h"

int KOBO_sound::tsdcounter = 0;

int KOBO_sound::listener_x = 0;
int KOBO_sound::listener_y = 0;
int KOBO_sound::wrap_x = 0;
int KOBO_sound::wrap_y = 0;
int KOBO_sound::scale = 65536 / 1000;
int KOBO_sound::panscale = 65536 / 700;
unsigned KOBO_sound::rumble = 0;

A2_state *KOBO_sound::state = NULL;
A2_handle KOBO_sound::rootvoice = 0;
A2_handle KOBO_sound::master_g = 0;
A2_handle KOBO_sound::ui_g = 0;
A2_handle KOBO_sound::sfx_g = 0;
A2_handle KOBO_sound::music_g = 0;
A2_handle KOBO_sound::title_g = 0;
int KOBO_sound::current_noise = 0;
A2_handle KOBO_sound::noisehandle = 0;
A2_handle KOBO_sound::musichandle = 0;
A2_handle KOBO_sound::gunhandle = 0;
A2_handle KOBO_sound::banks[KOBO_SOUND_BANKS];
A2_handle KOBO_sound::sounds[S__COUNT];
unsigned KOBO_sound::sbank[S__COUNT];
float KOBO_sound::buffer_latency = 0.0f;

int KOBO_sound::current_song = 0;
bool KOBO_sound::music_is_ingame = false;


#define	KOBO_DEFS(x)	"S_" #x,
static const char *kobo_soundnames[] =
{
	KOBO_ALLSOUNDS
};
#undef	KOBO_DEFS


KOBO_sound::KOBO_sound()
{
	memset(banks, 0, sizeof(banks));
	memset(sounds, 0, sizeof(sounds));
	memset(sbank, 0, sizeof(sbank));
}


KOBO_sound::~KOBO_sound()
{
	close();
}


/*--------------------------------------------------
	Open/close
--------------------------------------------------*/


bool KOBO_sound::load(unsigned bank, const char *themepath,
		int (*prog)(const char *msg))
{
	if(bank >= KOBO_SOUND_BANKS)
	{
		log_printf(ELOG, "Sound bank %d out of range!\n", bank);
		return false;
	}

	unload(bank);

	char path[256];
	snprintf(path, sizeof(path), "%s/main.a2s", themepath);

	log_printf(ULOG, "Loading A2S bank \"%s\"\n", path);
	const char *p = fmap->get(path, FM_FILE, "SFX>>");
	if(!p)
	{
		log_printf(ELOG, "Couldn't find \"%s\"!\n", path);
		return false;
	}
	if((banks[bank] = a2_Load(state, p, 0)) < 0)
	{
		log_printf(ELOG, "Couldn't load\"%s\"! (%s)\n", path,
				a2_ErrorString((A2_errors)-banks[bank]));
		banks[bank] = 0;
		return false;
	}

	for(int i = 1; i < S__COUNT; ++i)
	{
		int h = a2_Get(state, banks[bank], kobo_soundnames[i]);
		if(h <= 0)
			continue;

		sounds[i] = h;
		sbank[i] = bank;
		log_printf(ULOG, "%s(%d) handle:%d bank:%d\n",
				kobo_soundnames[i], i, h, bank);

		// Kill and invalidate an previous sound this replaces. (This
		// isn't really supposed to happen, unless there are themes
		// with overlapping exports!)
		if(i == current_song)
		{
			current_song = 0;
			a2_Kill(state, musichandle);
			musichandle = 0;
		}
		if(i == current_noise)
		{
			current_noise = 0;
			a2_Kill(state, noisehandle);
			noisehandle = 0;
		}
		switch(i)
		{
		  case S_SHOT:
			a2_Kill(state, gunhandle);
			gunhandle = 0;
			break;
		  case S_G_MASTER:
			if(master_g > 0)
			{
				a2_Kill(state, master_g);
				master_g = 0;
			}
			break;
		  case S_G_UI:
			if(ui_g > 0)
			{
				a2_Kill(state, ui_g);
				ui_g = 0;
			}
			break;
		  case S_G_SFX:
			if(sfx_g > 0)
			{
				a2_Kill(state, sfx_g);
				sfx_g = 0;
			}
			break;
		  case S_G_MUSIC:
			if(music_g > 0)
			{
				a2_Kill(state, music_g);
				music_g = 0;
			}
			break;
		  case S_G_TITLE:
			if(title_g > 0)
			{
				a2_Kill(state, title_g);
				title_g = 0;
			}
			break;
		}
	}

	if(prefs->soundtools)
	{
		for(int i = 1; i < S__COUNT; ++i)
			if(!sounds[i])
			{
				log_printf(WLOG, "Remaining undefined sfx:\n");
				break;
			}
		for(int i = 1; i < S__COUNT; ++i)
			if(!sounds[i])
				log_printf(WLOG, "  %s(%d)\n",
						kobo_soundnames[i], i);
	}

	init_mixdown();
	prefschange();

	if(prog)
		prog(NULL);
	return true;
}


void KOBO_sound::unload(int bank)
{
	if(bank < 0)
	{
		for(int i = 0; i < KOBO_SOUND_BANKS; ++i)
			unload(i);
		return;
	}

	if(bank >= KOBO_SOUND_BANKS)
	{
		log_printf(ELOG, "Sound bank %d out of range!\n", bank);
		return;
	}

	if(!state)
		return;

	if(!banks[bank])
		return;

	// Unload bank! (This will kill any playing sounds from this bank.)
	a2_Release(state, banks[bank]);
	banks[bank] = 0;

	// Remove any handles that were referring to that bank
	for(int i = 1; i < S__COUNT; ++i)
		if(sounds[i] && (sbank[i] == (unsigned)bank))
		{
			if(i == current_song)
			{
				current_song = 0;
				musichandle = 0;
			}
			if(i == current_noise)
			{
				current_noise = 0;
				noisehandle = 0;
			}
			switch(i)
			{
			  case S_SHOT:
				gunhandle = 0;
				break;
			  case S_G_MASTER:
				master_g = 0;
				break;
			  case S_G_UI:
				ui_g = 0;
				break;
			  case S_G_SFX:
				sfx_g = 0;
				break;
			  case S_G_MUSIC:
				music_g = 0;
				break;
			  case S_G_TITLE:
				title_g = 0;
				break;
			}
			sounds[i] = 0;
		}
}


void KOBO_sound::init_mixdown()
{
	if(master_g <= 0)
	{
		master_g = a2_Start(state, rootvoice, sounds[S_G_MASTER]);
		if(master_g < 0)
		{
			log_printf(WLOG, "Couldn't create master mixer group!"
					" (%s)\n",
					a2_ErrorString((A2_errors)-master_g));
			master_g = a2_NewGroup(state, rootvoice);
		}
	}

	if(ui_g <= 0)
	{
		ui_g = a2_Start(state, master_g, sounds[S_G_UI]);
		if(ui_g < 0)
		{
			log_printf(WLOG, "Couldn't create UI mixer group!"
					" (%s)\n",
					a2_ErrorString((A2_errors)-ui_g));
			ui_g = a2_NewGroup(state, master_g);
		}
	}

	if(sfx_g <= 0)
	{
		sfx_g = a2_Start(state, master_g, sounds[S_G_SFX]);
		if(sfx_g < 0)
		{
			log_printf(WLOG, "Couldn't create SFX mixer group!"
					" (%s)\n",
					a2_ErrorString((A2_errors)-sfx_g));
			sfx_g = a2_NewGroup(state, master_g);
		}
	}

	if(music_g <= 0)
	{
		music_g = a2_Start(state, master_g, sounds[S_G_MUSIC]);
		if(music_g < 0)
		{
			log_printf(WLOG, "Couldn't create music mixer group!"
					" (%s)\n",
					a2_ErrorString((A2_errors)-music_g));
			music_g = a2_NewGroup(state, master_g);
		}
	}

	if(title_g <= 0)
	{
		title_g = a2_Start(state, master_g, sounds[S_G_TITLE]);
		if(title_g < 0)
		{
			log_printf(WLOG, "Couldn't create music mixer group!"
					" (%s)\n",
					a2_ErrorString((A2_errors)-title_g));
			title_g = a2_NewGroup(state, master_g);
		}
	}
}


void KOBO_sound::prefschange()
{
	if(!state)
		return;
	a2_Send(state, master_g, 3, (float)prefs->vol_boost);
	a2_Send(state, master_g, 2, pref2vol(prefs->volume));
	a2_Send(state, sfx_g, 2, pref2vol(prefs->sfx_vol));
	a2_Send(state, ui_g, 2, pref2vol(prefs->ui_vol));
	a2_Send(state, music_g, 2, pref2vol(prefs->music_vol));
	a2_Send(state, title_g, 2, pref2vol(prefs->title_vol));
	update_music(false);
}


int KOBO_sound::open()
{
	if(!prefs->sound)
	{
		log_printf(WLOG, "Sound disabled!\n");
		return 0;
	}

	log_printf(ULOG, "Initializing audio...\n");
	log_printf(ULOG, "              Driver: %s\n", prefs->audiodriver);
	log_printf(ULOG, "         Sample rate: %d Hz\n", prefs->samplerate);

	int bufsize = prefs->audiobuffer;
	if(bufsize)
		log_printf(ULOG, "         Buffer size: %d ms\n", bufsize);
	else
	{
		log_printf(ULOG, "             Latency: %d ms\n",
				prefs->latency);
		int tbs = prefs->samplerate * prefs->latency / 1000;
		for(bufsize = 16; bufsize < tbs; bufsize <<= 1)
			;
		log_printf(ULOG, "   Calc. buffer size: %d ms\n", bufsize);
	}

	A2_config *cfg = a2_OpenConfig(prefs->samplerate, bufsize, 2,
			A2_REALTIME | A2_TIMESTAMP | A2_STATECLOSE);
	if(!cfg)
	{
		log_printf(ELOG, "Couldn't create audio configuration;"
				" disabling sound effects.\n");
		return -1;
	}

	if(prefs->audiodriver[0] && a2_AddDriver(cfg,
			a2_NewDriver(A2_AUDIODRIVER, prefs->audiodriver)))
		log_printf(WLOG, "Couldn't add audio driver \"%s\";"
				"trying default.\n");

	if(!(state = a2_Open(cfg)))
	{
		log_printf(ELOG, "Couldn't create audio engine state;"
				" disabling sound effects.\n");
		return -3;
	}

	log_printf(ULOG, "  Actual sample rate: %d Hz\n", cfg->samplerate);
	log_printf(ULOG, "         Buffer size: %d frames\n", cfg->buffer);

	buffer_latency = cfg->buffer * 1000.0f / cfg->samplerate;
	log_printf(ULOG, "  Calculated latency: %f ms\n", buffer_latency);

	// We don't use this! We calculate our own margin.
	a2_SetStateProperty(state, A2_PTIMESTAMPMARGIN, 0);

	rootvoice = a2_RootVoice(state);
	if(rootvoice < 0)
		log_printf(ELOG, "Couldn't find root voice!\n");

	g_wrap(WORLD_SIZEX, WORLD_SIZEY);
	g_scale(VIEWLIMIT, VIEWLIMIT);
	return 0;
}


void KOBO_sound::close()
{
	for(int i = 0; i < KOBO_SOUND_BANKS; ++i)
		unload(i);
	if(state)
	{
		a2_Close(state);
		state = NULL;
	}
	rootvoice = 0;
	master_g = 0;
	ui_g = 0;
	sfx_g = 0;
	music_g = 0;
	title_g = 0;
	buffer_latency = 0.0f;
}


/*--------------------------------------------------
	Info
--------------------------------------------------*/

const char *KOBO_sound::symname(unsigned wid)
{
	if(wid >= S__COUNT)
		return "<OUT OF RANGE>";
	return kobo_soundnames[wid];
}


/*--------------------------------------------------
	Main controls
--------------------------------------------------*/

void KOBO_sound::timestamp_reset()
{
	if(state)
		a2_TimestampReset(state);
}


void KOBO_sound::timestamp_nudge(float ms)
{
	if(!state)
		return;
	a2_TimestampNudge(state, a2_ms2Timestamp(state, ms), 0.001f);
}


void KOBO_sound::timestamp_bump(float ms)
{
	if(!state)
		return;
	int min = 0;
	if(prefs->tsdebug && (++tsdcounter >= 10))
	{
		int avg, max;
		tsdcounter = 0;
		a2_GetStateProperty(state, A2_PTSMARGINMIN, &min);
		a2_GetStateProperty(state, A2_PTSMARGINAVG, &avg);
		a2_GetStateProperty(state, A2_PTSMARGINMAX, &max);
		printf("%d\t%d\t%d/%d/%d\n",
				(int)(buffer_latency + 0.5f), prefs->maxfps,
				(int)(a2_Timestamp2ms(state, min) + 0.5f),
				(int)(a2_Timestamp2ms(state, avg) + 0.5f),
				(int)(a2_Timestamp2ms(state, max) + 0.5f));
	}
	else
		a2_GetStateProperty(state, A2_PTSMARGINMIN, &min);
	a2_SetStateProperty(state, A2_PTSMARGINAVG, 0);
	if(min < 0)
	{
		log_printf(WLOG, "Late audio API messages. (Up to %f ms.) "
				"Timestamp bumped 1 ms.\n",
				-a2_Timestamp2ms(state, min));
		ms += 1.0f;
	}
	a2_TimestampBump(state, a2_ms2Timestamp(state, ms));
}


void KOBO_sound::frame()
{
	// Various sound control logic
	rumble = 0;	// Only one per logic frame!
	if(state)
		a2_PumpMessages(state);
}


void KOBO_sound::update_music(bool newsong)
{
	if(!state)
		return;

	// Stop any playing song, if new song, or music is disabled in prefs
	if(musichandle && (newsong || !prefs->music))
	{
		a2_Send(state, musichandle, 1);
		a2_Release(state, musichandle);
		musichandle = 0;
	}

	// If we're not supposed to play anything, we're done here!
	if(!prefs->music || (current_song <= 0) || musichandle)
		return;

	// Don't start music if the group is muted...
	if((music_is_ingame && !prefs->music_vol) ||
			(!music_is_ingame && !prefs->title_vol))
		return;

	// Start the song that's supposed to be playing
	if(checksound(current_song, "KOBO_sound::music()"))
	{
		musichandle = a2_Start(state,
				music_is_ingame ? music_g : title_g,
				sounds[current_song]);
		if(musichandle < 0)
		{
			log_printf(WLOG, "Couldn't start song! (%s)\n",
					a2_ErrorString(
					(A2_errors)-musichandle));
			musichandle = 0;
		}
	}
}

void KOBO_sound::music(int sng, bool ingame)
{
	if((sng == current_song) && (ingame == music_is_ingame))
		return;	// No change!
	current_song = sng;
	music_is_ingame = ingame;
	update_music(true);
}


void KOBO_sound::jingle(int sng)
{
	if(!state)
		return;
	if(checksound(sng, "KOBO_sound::jingle()"))
		a2_Play(state, master_g, sounds[sng], 0.0f, 0.5f);
}


/*--------------------------------------------------
	In-game sound
--------------------------------------------------*/

bool KOBO_sound::checksound(int wid, const char *where)
{
	if(wid < 0 || wid >= S__COUNT)
	{
		log_printf(ELOG, "%s: Sound index %d is out of range!\n",
				where, wid);
		return false;
	}
	if(wid == S_NONE)
		return false;	// This is not an error...
	if(!sounds[wid])
	{
		if(prefs->debug)
			log_printf(ELOG, "%s: Sound %s (%d) not loaded!\n",
					where, kobo_soundnames[wid], wid);
		return false;
	}
	return true;
}


void KOBO_sound::g_music(unsigned scene)
{
	music(S_INGAMESONG1 + scene / 10 % 5, true);
}


void KOBO_sound::ui_play(unsigned wid, int vol, int pitch, int pan)
{
	if(!state)
		return;
	if(wid < 0 || wid >= S__COUNT)
	{
		log_printf(ELOG, "KOBO_sound::up_play(): Sound index %d is "
				"out of range!\n", wid);
		return;
	}
	if(!sounds[wid])
		return;
	a2_Play(state, ui_g, sounds[wid], (pitch / 65536.0f - 60.0f)  / 12.0f,
			vol / 65536.0f, pan / 65536.0f);
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

bool KOBO_sound::eval_pos(int x, int y, float *vol, float *pan)
{
	// Calculate volume
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

	// Approximation of distance attenuation
	int vx = abs(x * scale);
	int vy = abs(y * scale);
	if((vx | vy) & 0xffff0000)
	{
		*vol = *pan = 0.0f;
		return false;
	}

	vx = (65536 - vx) >> 1;
	vy = (65536 - vy) >> 1;
	int v = vx * vy >> 14;
	v = (v >> 1) * (v >> 1) >> 14;

	int p = x * panscale;
	if(p < -65536)
		p = -65536;
	else if(p > 65536)
		p = 65536;

	*vol = v / 65536.0f;
	*pan = p / 65536.0f;
	return true;
}


void KOBO_sound::g_play(unsigned wid, int x, int y)
{
	if(!state)
		return;
	if(!checksound(wid, "KOBO_sound::g_play()"))
		return;

	float vol, pan;
	if(!eval_pos(x, y, &vol, &pan))
		return;	// We don't start "short" sounds that are out of range!

	a2_Play(state, sfx_g, sounds[wid], 0.0f, vol, pan);
}


int KOBO_sound::g_start(unsigned wid, int x, int y)
{
	if(!state)
		return -1;
	if(!checksound(wid, "KOBO_sound::g_start()"))
		return -1;

	float vol, pan;
	eval_pos(x, y, &vol, &pan);
	return a2_Start(state, sfx_g, sounds[wid], 0.0f, vol, pan);
}

void KOBO_sound::g_move(int h, int x, int y)
{
	if(!state || h <= 0)
		return;
	float vol, pan;
	eval_pos(x, y, &vol, &pan);
	a2_Send(state, h, 3, vol, pan);
}

void KOBO_sound::g_control(int h, int c, float v)
{
	if(!state || h <= 0)
		return;
	a2_Send(state, h, c, v);
}

void KOBO_sound::g_stop(int h)
{
	if(!state || h <= 0)
		return;
	a2_Send(state, h, 1);
	a2_Release(state, h);
}


void KOBO_sound::g_play0(unsigned wid, int vol, int pitch)
{
	if(!state)
		return;
	if(!checksound(wid, "KOBO_sound::g_play0()"))
		return;
	a2_Play(state, sfx_g, sounds[wid], (pitch / 65536.0f - 60.0f)  / 12.0f,
			vol / 65536.0f);
}


void KOBO_sound::start_player_gun()
{
	if(!checksound(S_SHOT, "KOBO_sound::start_player_gun()"))
		return;
	gunhandle = a2_Start(state, sfx_g, sounds[S_SHOT], 0.0f,
			(prefs->cannonloud << 14) / 6553600.0f);
	if(gunhandle < 0)
	{
		log_printf(WLOG, "Couldn't start player gun sound! (%s)\n",
				a2_ErrorString((A2_errors)-gunhandle));
		gunhandle = 0;
	}
}


void KOBO_sound::g_player_fire()
{
	if(!state)
		return;
	if(!gunhandle)
		start_player_gun();
	if(gunhandle)
		a2_Send(state, gunhandle, 2);
}


void KOBO_sound::g_player_charge(float charge)
{
	if(!state)
		return;
	if(!gunhandle)
		start_player_gun();
	if(gunhandle)
		a2_Send(state, gunhandle, 3, charge);
}


void KOBO_sound::g_player_charged_fire(float charge)
{
	if(!state)
		return;
	if(!gunhandle)
		start_player_gun();
	if(gunhandle)
		a2_Send(state, gunhandle, 4, charge);
}


void KOBO_sound::g_player_damage(float level)
{
	if(!state)
		return;
	if(checksound(S_PLAYER_DAMAGE, "KOBO_sound::g_player_damage()"))
		a2_Play(state, sfx_g, sounds[S_PLAYER_DAMAGE], 0.0f, level);
}


void KOBO_sound::g_player_explo_start()
{
	g_player_damage();
	if(!state)
		return;
	if(checksound(S_EXPLO_PLAYER, "KOBO_sound::g_player_explo_start()"))
		a2_Play(state, sfx_g, sounds[S_EXPLO_PLAYER]);
}


void KOBO_sound::g_kill_all()
{
	if(!state)
		return;

	// Fade out...
	a2_Send(state, sfx_g, 2, 0);

	A2_timestamp t = a2_TimestampBump(state, a2_ms2Timestamp(state, 150));

	// Kill all sounds, and restore volume
	a2_KillSub(state, sfx_g);
	a2_Send(state, sfx_g, 2, pref2vol(prefs->sfx_vol));

	a2_TimestampSet(state, t);

	gunhandle = 0;	// This one gets killed too, so we'll need a new one!
}


/*--------------------------------------------------
	UI sound effects
--------------------------------------------------*/

void KOBO_sound::ui_music_title()
{
	music(S_TITLESONG);
}


void KOBO_sound::ui_noise(int h)
{
	if(!state)
		return;
	if(h == current_noise)
		return;
	if(h && !checksound(h, "KOBO_sound::ui_noise()"))
		return;
	if(noisehandle)
	{
		a2_Send(state, noisehandle, 1);
		a2_Release(state, noisehandle);
		noisehandle = 0;
	}
	if(h)
		noisehandle = a2_Start(state, ui_g, sounds[h]);
	current_noise = h;
}


void KOBO_sound::ui_countdown(int remain)
{
	ui_play(S_UI_CDTICK, 32768, (60 - remain)<<16);
}
