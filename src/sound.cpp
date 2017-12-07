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

#include "sound.h"
#include "kobo.h"
#include "game.h"
#include "kobolog.h"
#include "random.h"
#include "enemies.h"

int KOBO_sound::tsdcounter = 0;

int KOBO_sound::listener_x = 0;
int KOBO_sound::listener_y = 0;
int KOBO_sound::wrap_x = 0;
int KOBO_sound::wrap_y = 0;
int KOBO_sound::scale = 65536 / 1000;
int KOBO_sound::panscale = 65536 / 700;
unsigned KOBO_sound::rumble = 0;
float KOBO_sound::volscale = 1.0f;
float KOBO_sound::pitchshift = 0.0f;

A2_interface *KOBO_sound::iface = NULL;
A2_handle KOBO_sound::rootvoice = 0;
A2_handle KOBO_sound::groups[KOBO_MG__COUNT] = { 0, 0, 0, 0, 0 };
int KOBO_sound::current_noise = 0;
A2_handle KOBO_sound::noisehandle = 0;
A2_handle KOBO_sound::musichandle = 0;
A2_handle KOBO_sound::gunhandle = 0;
A2_handle KOBO_sound::banks[KOBO_SOUND_BANKS];
A2_handle KOBO_sound::sounds[S__COUNT];
unsigned KOBO_sound::sbank[S__COUNT];
float KOBO_sound::buffer_latency = 0.0f;

bool KOBO_sound::shield_enabled = false;
A2_handle KOBO_sound::shieldhandle = 0;

int KOBO_sound::current_song = 0;
bool KOBO_sound::music_is_ingame = false;

// S_* sounds normally running on the respective groups
static const int group_programs[KOBO_MG__COUNT] = {
	S_G_MASTER,
	S_G_UI,
	S_G_SFX,
	S_G_MUSIC,
	S_G_TITLE
};

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
	if(!iface)
	{
		log_printf(WLOG, "KOBO_sound::load(): Audio engine not open! "
				"Operation ignored.\n");
		return false;
	}

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
	if((banks[bank] = a2_Load(iface, p, 0)) < 0)
	{
		log_printf(ELOG, "Couldn't load\"%s\"! (%s)\n", path,
				a2_ErrorString((A2_errors)-banks[bank]));
		banks[bank] = 0;
		return false;
	}

	for(int i = 1; i < S__COUNT; ++i)
	{
		int h = a2_Get(iface, banks[bank], kobo_soundnames[i]);
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
			a2_Kill(iface, musichandle);
			musichandle = 0;
		}
		if(i == current_noise)
		{
			current_noise = 0;
			a2_Kill(iface, noisehandle);
			noisehandle = 0;
		}
		switch(i)
		{
		  case S_PLAYER_GUN:
			a2_Kill(iface, gunhandle);
			gunhandle = 0;
			break;
		  case S_PLAYER_SHIELD:
			a2_Kill(iface, shieldhandle);
			shieldhandle = 0;
			break;
		}
		for(int j = 0; j < KOBO_MG__COUNT; ++j)
			if(i == group_programs[j])
				if(groups[j] > 0)
				{
					a2_Kill(iface, groups[j]);
					groups[j] = 0;
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

	init_mixer_group(KOBO_MG_ALL);
	prefschange();

	if(prog)
		prog(NULL);
	return true;
}


void KOBO_sound::unload(int bank)
{
	if(!iface)
		return;

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

	if(!iface)
		return;

	if(!banks[bank])
		return;

	// Unload bank! (This will kill any playing sounds from this bank.)
	a2_Release(iface, banks[bank]);
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
			  case S_PLAYER_GUN:
				gunhandle = 0;
				break;
			  case S_PLAYER_SHIELD:
				shieldhandle = 0;
				break;
			}
			for(int j = 0; j < KOBO_MG__COUNT; ++j)
				if(i == group_programs[j])
					if(groups[j] > 0)
						groups[j] = 0;
			sounds[i] = 0;
		}
}


void KOBO_sound::init_mixer_group(KOBO_mixer_group grp)
{
	A2_handle parent;
	if(!iface)
		return;

	if(grp == KOBO_MG_ALL)
	{
		for(int j = 0; j < KOBO_MG__COUNT; ++j)
			init_mixer_group((KOBO_mixer_group)j);
		return;
	}

	if(groups[grp] > 0)
		return;		// Already up!

	if(grp == KOBO_MG_MASTER)
		parent = rootvoice;
	else
		parent = groups[KOBO_MG_MASTER];
	if(parent <= 0)
		return;		// No parent group!

	groups[grp] = a2_Start(iface, parent, sounds[group_programs[grp]]);
	if(groups[grp] < 0)
	{
		log_printf(WLOG, "Couldn't create mixer group %d! (%s)\n",
				grp, a2_ErrorString((A2_errors)-groups[grp]));
		groups[grp] = a2_NewGroup(iface, parent);
	}
}


void KOBO_sound::logsetup()
{
	if(!iface)
		return;

	// Set up log filter
	int ll = A2_LOGM_CRITICAL;
	if(prefs->debug || prefs->soundtools)
		ll |= A2_LOGM_NORMAL;
	if(prefs->soundtools)
		ll |= A2_LOGM_DEBUG;
	a2_SetStateProperty(iface, A2_PLOGLEVELS, ll);
}


void KOBO_sound::prefschange()
{
	if(!iface)
		return;

	logsetup();

	a2_Send(iface, groups[KOBO_MG_MASTER], 3, (float)prefs->vol_boost);
	a2_Send(iface, groups[KOBO_MG_MASTER], 2, pref2vol(prefs->volume));

	a2_Send(iface, groups[KOBO_MG_SFX], 2,
			pref2vol(prefs->sfx_vol) * volscale);

	a2_Send(iface, groups[KOBO_MG_UI], 2, pref2vol(prefs->ui_vol));

	a2_Send(iface, groups[KOBO_MG_MUSIC], 2, pref2vol(prefs->music_vol));
	a2_Send(iface, groups[KOBO_MG_TITLE], 2, pref2vol(prefs->title_vol));
	update_music(false);

	if(gunhandle)
		start_player_gun();
	if(shield_enabled)
		g_player_shield(true);
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
			A2_REALTIME | A2_AUTOCLOSE |
			(prefs->audiots ? A2_TIMESTAMP : 0));
	if(!cfg)
	{
		log_printf(ELOG, "Couldn't create audio configuration;"
				" disabling sound effects.\n");
		return -1;
	}

	if(prefs->audiodriver[0] && a2_AddDriver(cfg,
			a2_NewDriver(A2_AUDIODRIVER, prefs->audiodriver)))
		log_printf(WLOG, "Couldn't add audio driver \"%s\"; "
				"trying default.\n", prefs->audiodriver);

	if(!(iface = a2_Open(cfg)))
	{
		log_printf(ELOG, "Couldn't create audio engine iface;"
				" disabling sound effects.\n");
		return -3;
	}

	logsetup();

	log_printf(ULOG, "  Actual sample rate: %d Hz\n", cfg->samplerate);
	log_printf(ULOG, "         Buffer size: %d frames\n", cfg->buffer);

	buffer_latency = cfg->buffer * 1000.0f / cfg->samplerate;
	log_printf(ULOG, "  Calculated latency: %f ms\n", buffer_latency);

	// We don't use this! We calculate our own margin.
	a2_SetStateProperty(iface, A2_PTIMESTAMPMARGIN, 0);

	rootvoice = a2_RootVoice(iface);
	if(rootvoice < 0)
		log_printf(ELOG, "Couldn't find root voice!\n");

	g_wrap(WORLD_SIZEX, WORLD_SIZEY);
	g_scale(VIEWLIMIT, VIEWLIMIT);
	g_volume(1.0f);
	g_pitch(0.0f);
	return 0;
}


void KOBO_sound::close()
{
	for(int i = 0; i < KOBO_SOUND_BANKS; ++i)
		unload(i);
	if(iface)
	{
		a2_Close(iface);
		iface = NULL;
	}
	rootvoice = 0;
	for(int j = 0; j < KOBO_MG__COUNT; ++j)
		groups[j] = 0;
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


const char *KOBO_sound::grpname(unsigned grp)
{
	switch((KOBO_mixer_group)grp)
	{
	  case KOBO_MG_ALL:	return "ALL";
	  case KOBO_MG_MASTER:	return "MASTER";
	  case KOBO_MG_UI:	return "UI";
	  case KOBO_MG_SFX:	return "SFX";
	  case KOBO_MG_MUSIC:	return "MUSIC";
	  case KOBO_MG_TITLE:	return "TITLE";
	  case KOBO_MG__COUNT:
		break;
	}
	return "<OUT OF RANGE>";
}


/*--------------------------------------------------
	Main controls
--------------------------------------------------*/

void KOBO_sound::timestamp_reset()
{
	if(iface)
		a2_TimestampReset(iface);
}


void KOBO_sound::timestamp_nudge(float ms)
{
	if(!iface)
		return;
	a2_TimestampNudge(iface, a2_ms2Timestamp(iface, ms), 0.001f);
}


void KOBO_sound::timestamp_bump(float ms)
{
	if(!iface)
		return;
	int min = 0;
	if(prefs->tsdebug && (++tsdcounter >= 10))
	{
		int avg, max;
		tsdcounter = 0;
		a2_GetStateProperty(iface, A2_PTSMARGINMIN, &min);
		a2_GetStateProperty(iface, A2_PTSMARGINAVG, &avg);
		a2_GetStateProperty(iface, A2_PTSMARGINMAX, &max);
		printf("%d\t%d\t%d/%d/%d\n",
				(int)(buffer_latency + 0.5f), prefs->maxfps,
				(int)(a2_Timestamp2ms(iface, min) + 0.5f),
				(int)(a2_Timestamp2ms(iface, avg) + 0.5f),
				(int)(a2_Timestamp2ms(iface, max) + 0.5f));
	}
	else
		a2_GetStateProperty(iface, A2_PTSMARGINMIN, &min);
	a2_SetStateProperty(iface, A2_PTSMARGINAVG, 0);
	if(min < 0)
	{
		if(prefs->soundtools)
			log_printf(WLOG, "Late audio API messages. (Up to %f "
					"ms.) Timestamp bumped 1 ms.\n",
					-a2_Timestamp2ms(iface, min));
		ms += 1.0f;
	}
	a2_TimestampBump(iface, a2_ms2Timestamp(iface, ms));
}


void KOBO_sound::frame()
{
	// Various sound control logic
	rumble = 0;	// Only one per logic frame!
	if(iface)
		a2_PumpMessages(iface);
}


void KOBO_sound::update_music(bool newsong)
{
	if(!iface)
		return;

	// Stop any playing song, if new song, or music is disabled in prefs
	if(musichandle && (newsong || !prefs->music))
	{
		a2_Send(iface, musichandle, 1);
		a2_Release(iface, musichandle);
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
		musichandle = a2_Start(iface, groups[music_is_ingame ?
				KOBO_MG_MUSIC : KOBO_MG_TITLE],
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
	if(!iface)
		return;
	if(checksound(sng, "KOBO_sound::jingle()"))
		a2_Play(iface, groups[KOBO_MG_MASTER],
				sounds[sng], 0.0f, 0.5f);
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
		if(prefs->soundtools && prefs->st_missing)
			log_printf(WLOG, "%s: Sound %s (%d) not loaded!\n",
					where, kobo_soundnames[wid], wid);
		return false;
	}
	return true;
}


void KOBO_sound::g_music(unsigned scene)
{
	music(S_INGAMESONG1 + scene / 10 % 5, true);
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
	if((vx | vy) & 0xffff0000 || !volscale)
	{
		*vol = *pan = 0.0f;
		return false;
	}

	vx = (65536 - vx) >> 1;
	vy = (65536 - vy) >> 1;
	int v = vx * vy >> 14;
	v = ((v >> 2) * v >> 14) * (v >> 2) >> 14;

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
	if(!iface)
		return;
	if(!checksound(wid, "KOBO_sound::g_play()"))
		return;

	float vol, pan;
	if(!eval_pos(x, y, &vol, &pan))
	{
		return;	// We don't start "short" sounds that are out of range!
	}

	a2_Play(iface, groups[KOBO_MG_SFX], sounds[wid],
			pitchshift, vol, pan);
	if(prefs->soundtools && prefs->st_played)
		log_printf(ULOG, "Playing %s (%d)\n", kobo_soundnames[wid],
				wid);
}


int KOBO_sound::g_start(unsigned wid, int x, int y)
{
	if(!iface)
		return -1;
	if(!checksound(wid, "KOBO_sound::g_start()"))
		return -1;
	if(!volscale)
		return -1;

	float vol, pan;
	eval_pos(x, y, &vol, &pan);
	A2_handle h = a2_Start(iface, groups[KOBO_MG_SFX], sounds[wid],
			pitchshift, vol, pan);
	if(h < 0)
		log_printf(WLOG, "Couldn't start %s (%d)! (%s)\n",
				kobo_soundnames[wid], wid,
				a2_ErrorString((A2_errors)-h));
	else if(prefs->soundtools && prefs->st_played)
		log_printf(ULOG, "Started %s (%d); handle: %d\n",
				kobo_soundnames[wid], wid, h);
	return h;
}


void KOBO_sound::g_move(int h, int x, int y)
{
	if(!iface || h <= 0)
		return;
	float vol, pan;
	eval_pos(x, y, &vol, &pan);
	a2_Send(iface, h, 3, pitchshift, vol, pan, KOBO_SOUND_UPDATE_PERIOD);
}


void KOBO_sound::g_control(int h, int c, float v)
{
	if(!iface || h <= 0)
		return;
	a2_Send(iface, h, c, v);
}


void KOBO_sound::g_stop(int h)
{
	if(!iface || h <= 0)
		return;
	a2_Send(iface, h, 1);
	a2_Release(iface, h);
}


void KOBO_sound::g_release(int h)
{
	if(!iface || h <= 0)
		return;
	a2_Release(iface, h);
}


void KOBO_sound::start_player_gun()
{
	if(!checksound(S_PLAYER_GUN, "KOBO_sound::start_player_gun()"))
		return;
	if(!iface)
		return;
	if(gunhandle)
	{
		a2_Send(iface, gunhandle, 1);
		a2_Release(iface, gunhandle);
		gunhandle = 0;
	}
	if(!prefs->cannonloud)
		return;
	gunhandle = a2_Start(iface, groups[KOBO_MG_SFX], sounds[S_PLAYER_GUN],
			0.0f, (prefs->cannonloud << 14) / 6553600.0f);
	if(gunhandle < 0)
	{
		log_printf(WLOG, "Couldn't start player gun sound! (%s)\n",
				a2_ErrorString((A2_errors)-gunhandle));
		gunhandle = 0;
	}
	else if(prefs->soundtools && prefs->st_played)
		log_printf(ULOG, "Started player gun SFX; handle: %d\n",
				gunhandle);
}


void KOBO_sound::g_player_fire()
{
	if(!iface)
		return;
	if(!volscale)
		return;
	if(!gunhandle)
		start_player_gun();
	if(gunhandle)
		a2_Send(iface, gunhandle, 2);
}


void KOBO_sound::g_player_fire_denied()
{
	if(!iface)
		return;
	if(!volscale)
		return;
	if(checksound(S_PLAYER_FIRE_DENIED,
			"KOBO_sound::g_player_fire_denied()"))
	{
		a2_Play(iface, groups[KOBO_MG_UI],
				sounds[S_PLAYER_FIRE_DENIED]);
		if(prefs->soundtools && prefs->st_played)
			log_printf(ULOG, "Playing %s (%d)\n",
				kobo_soundnames[S_PLAYER_FIRE_DENIED],
				S_PLAYER_FIRE_DENIED);
	}
}


void KOBO_sound::g_player_charge(float charge)
{
	if(!iface)
		return;
	if(!volscale)
		return;
	if(!gunhandle)
		start_player_gun();
	if(gunhandle)
		a2_Send(iface, gunhandle, 3, charge);
}


void KOBO_sound::g_player_charged_fire(float charge)
{
	if(!iface)
		return;
	if(!volscale)
		return;
	if(!gunhandle)
		start_player_gun();
	if(gunhandle)
		a2_Send(iface, gunhandle, 4, charge);
}


void KOBO_sound::g_player_damage(float level)
{
	if(!iface)
		return;
	if(!volscale)
		return;
	if(shield_enabled)
		g_control(shieldhandle, 2, level);
	else if(checksound(S_PLAYER_DAMAGE, "KOBO_sound::g_player_damage()"))
	{
		a2_Play(iface, groups[KOBO_MG_SFX], sounds[S_PLAYER_DAMAGE],
				pitchshift, level);
		if(prefs->soundtools && prefs->st_played)
			log_printf(ULOG, "Playing %s (%d)\n",
				kobo_soundnames[S_PLAYER_DAMAGE],
				S_PLAYER_DAMAGE);
	}
}


void KOBO_sound::g_player_explo_start()
{
	if(!volscale)
		return;
	g_player_damage();
	if(!iface)
		return;
	if(checksound(S_PLAYER_DEATH, "KOBO_sound::g_player_explo_start()"))
	{
		a2_Play(iface, groups[KOBO_MG_SFX], sounds[S_PLAYER_DEATH],
				pitchshift);
		if(prefs->soundtools && prefs->st_played)
			log_printf(ULOG, "Playing %s (%d)\n",
				kobo_soundnames[S_PLAYER_DEATH],
				S_PLAYER_DEATH);
	}
}


void KOBO_sound::g_player_shield(bool enable)
{
	if(!iface)
		return;
	if(shieldhandle && !enable)
	{
		a2_Send(iface, shieldhandle, 1);
		a2_Release(iface, shieldhandle);
		shieldhandle = 0;
	}
	else if(enable && !shieldhandle)
	{
		if(!checksound(S_PLAYER_SHIELD,
				"KOBO_sound::g_player_shield()"))
			return;
		shieldhandle = a2_Start(iface, groups[KOBO_MG_SFX],
				sounds[S_PLAYER_SHIELD],
				0.0f, (prefs->shieldloud << 14) / 6553600.0f);
		if(shieldhandle < 0)
		{
			log_printf(WLOG, "Couldn't start player shield sound! "
					" (%s)\n",
					a2_ErrorString(
					(A2_errors)-shieldhandle));
			shieldhandle = 0;
		}
		else if(prefs->soundtools && prefs->st_played)
			log_printf(ULOG, "Started shield SFX; handle: %d\n",
					shieldhandle);
	}
	shield_enabled = enable;
}


void KOBO_sound::g_new_scene(int fadetime)
{
	if(!iface)
		return;
	if(!fadetime)
		fadetime = KOBO_SFX_XFADE_TIME;

	if(prefs->soundtools)
		log_printf(ULOG, "--- g_new_scene() ---\n");

	// Detach all continuous sounds, as they're now invalid
	enemies.detach_sounds();
	gunhandle = 0;
	shieldhandle = 0;

	// Fade out, then kill the current SFX group
	a2_Send(iface, groups[KOBO_MG_SFX], 2, 0, (float)fadetime);
	A2_timestamp t = a2_TimestampBump(iface, a2_ms2Timestamp(iface,
			fadetime));
	a2_Kill(iface, groups[KOBO_MG_SFX]);
	a2_TimestampSet(iface, t);
	groups[KOBO_MG_SFX] = 0;

	// Replace the SFX group with a new instance, and fade that in
	init_mixer_group(KOBO_MG_SFX);
	a2_Send(iface, groups[KOBO_MG_SFX], 2, 0, 0);
	a2_Send(iface, groups[KOBO_MG_SFX], 2,
			pref2vol(prefs->sfx_vol) * volscale, (float)fadetime);
}


void KOBO_sound::g_volume(float volume)
{
	if(prefs->soundtools)
		log_printf(ULOG, "--- g_volume(%f) ---\n", volume);
	if(volume && !volscale)
	{
		// This is a bit ugly... Only the game logic can "restart"
		// sounds that were dropped or killed when sound was muted.
		volscale = volume;
		enemies.restart_sounds();
	}
	else
		volscale = volume;
	if(!iface)
		return;
	a2_Send(iface, groups[KOBO_MG_SFX], 2,
			pref2vol(prefs->sfx_vol) * volscale,
			KOBO_SOUND_UPDATE_PERIOD);
}


void KOBO_sound::g_pitch(float pitch)
{
	// That's all! The game logic calls g_move() to update all continuous
	// sounds anyway, which takes care of pitch changes as well.
	pitchshift = pitch;
}


/*--------------------------------------------------
	UI sound effects
--------------------------------------------------*/

void KOBO_sound::ui_play(unsigned wid, int vol, int pitch, int pan)
{
	if(!iface)
		return;
	if(wid < 0 || wid >= S__COUNT)
	{
		log_printf(ELOG, "KOBO_sound::up_play(): Sound index %d is "
				"out of range!\n", wid);
		return;
	}
	if(!sounds[wid])
		return;
	a2_Play(iface, groups[KOBO_MG_UI],
			sounds[wid], (pitch / 65536.0f - 60.0f)  / 12.0f,
			vol / 65536.0f, pan / 65536.0f);
}


void KOBO_sound::ui_noise(int h)
{
	if(!iface)
		return;
	if(h == current_noise)
		return;
	if(h && !checksound(h, "KOBO_sound::ui_noise()"))
		return;
	if(noisehandle)
	{
		a2_Send(iface, noisehandle, 1);
		a2_Release(iface, noisehandle);
		noisehandle = 0;
	}
	if(h)
		noisehandle = a2_Start(iface, groups[KOBO_MG_UI], sounds[h]);
	current_noise = h;
}


void KOBO_sound::ui_countdown(int remain)
{
	ui_play(S_UI_CDTICK, 32768, (60 - remain)<<16);
}


/*--------------------------------------------------
	Low level API
--------------------------------------------------*/

void KOBO_sound::play(int grp, unsigned wid)
{
	if(!iface)
		return;
	if(grp < 0 || grp >= KOBO_MG__COUNT)
	{
		log_printf(ELOG, "Invalid sound group %d!\n", grp);
		return;
	}
	if(!checksound(wid, "KOBO_sound::play()"))
		return;
	a2_Play(iface, groups[grp], sounds[wid], 0.0f, 1.0f, 0.0f);
}


int KOBO_sound::start(int grp, unsigned wid)
{
	if(!iface)
		return -1;
	if(grp < 0 || grp >= KOBO_MG__COUNT)
	{
		log_printf(ELOG, "Invalid sound group %d!\n", grp);
		return -1;
	}
	if(!checksound(wid, "KOBO_sound::start()"))
		return -1;
	return a2_Start(iface, groups[grp], sounds[wid], 0.0f, 1.0f, 0.0f);
}


void KOBO_sound::kill_all(int grp)
{
	if(!iface)
		return;

	if(grp == KOBO_MG_MASTER)
	{
		a2_KillSub(iface, rootvoice);
		memset(groups, 0, sizeof(groups));
		init_mixer_group(KOBO_MG_ALL);
		prefschange();
	}
	else
		a2_KillSub(iface, groups[grp]);
}
