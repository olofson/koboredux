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
#include "game.h"
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
A2_handle KOBO_sound::master_g = 0;
A2_handle KOBO_sound::ui_g = 0;
A2_handle KOBO_sound::sfx_g = 0;
A2_handle KOBO_sound::music_g = 0;
A2_handle KOBO_sound::title_g = 0;
A2_handle KOBO_sound::noisehandle = 0;
A2_handle KOBO_sound::musichandle = 0;
A2_handle *KOBO_sound::modules = NULL;
A2_handle KOBO_sound::sounds[SOUND__COUNT];

static const char *kobo_a2sfiles[] =
{
	"SFX>>master.a2s",
	"SFX>>sfx.a2s",
	"SFX>>uisfx.a2s",
	"SFX>>music.a2s"
};

#define	A2SFILE__COUNT	((int)(sizeof(kobo_a2sfiles) / sizeof(char *)))

#define	KOBO_DEFS(x, y)	y,
static const char *kobo_soundnames[] =
{
	KOBO_ALLSOUNDS
};
#undef	KOBO_DEFS


KOBO_sound::KOBO_sound()
{
	modules = (A2_handle *)calloc(A2SFILE__COUNT, sizeof(A2_handle));
}


KOBO_sound::~KOBO_sound()
{
	free(modules);
	close();
}


A2_handle KOBO_sound::load_a2s(const char *path)
{
	A2_handle h;
	log_printf(ULOG, "Loading A2S bank \"%s\"\n", path);
	const char *p = fmap->get(path);
	if(!p)
	{
		log_printf(ELOG, "Couldn't find \"%s\"!\n", path);
		return -1;
	}
	if((h = a2_Load(state, p, 0)) < 0)
	{
		log_printf(ELOG, "Couldn't load\"%s\"! (%s)\n", path,
				a2_ErrorString((A2_errors)-h));
		return -1;
	}
	return h;
}


/*--------------------------------------------------
	Open/close
--------------------------------------------------*/


int KOBO_sound::load(int (*prog)(const char *msg), int force)
{
	for(int i = 0; i < A2SFILE__COUNT; ++i)
		modules[i] = load_a2s(kobo_a2sfiles[i]);

	for(int i = 0; i < SOUND__COUNT; ++i)
	{
		A2_handle h = -1;
		for(int j = 0; j < A2SFILE__COUNT; ++j)
		{
			int hh = a2_Get(state, modules[j], kobo_soundnames[i]);
			if(hh >= 0)
			{
				h = hh;
				break;
			}
		}
		if(h > 0)
		{
			sounds[i] = h;
#if 0
			printf("%s: %d\n", kobo_soundnames[i], sounds[i]);
#endif
		}
		else
		{
			log_printf(WLOG, "Couldn't find sound effect \"%s\"!"
					" (%s)\n", kobo_soundnames[i],
					a2_ErrorString((A2_errors)-h));
			sounds[i] = -1;
		}
	}

	init_mixdown();
	prefschange();

	return prog(NULL);
}


void KOBO_sound::init_mixdown()
{
	master_g = a2_Start(state, rootvoice, sounds[SOUND_G_MASTER]);
	if(master_g < 0)
	{
		log_printf(WLOG, "Couldn't create master mixer group!\n");
		master_g = a2_NewGroup(state, rootvoice);
	}

	ui_g = a2_Start(state, master_g, sounds[SOUND_G_UI]);
	if(ui_g < 0)
	{
		log_printf(WLOG, "Couldn't create UI mixer group!\n");
		ui_g = a2_NewGroup(state, master_g);
	}

	sfx_g = a2_Start(state, master_g, sounds[SOUND_G_SFX]);
	if(sfx_g < 0)
	{
		log_printf(WLOG, "Couldn't create SFX mixer group!\n");
		sfx_g = a2_NewGroup(state, master_g);
	}

	music_g = a2_Start(state, master_g, sounds[SOUND_G_MUSIC]);
	if(music_g < 0)
	{
		log_printf(WLOG, "Couldn't create music mixer group!\n");
		music_g = a2_NewGroup(state, master_g);
	}

	title_g = a2_Start(state, master_g, sounds[SOUND_G_TITLE]);
	if(title_g < 0)
	{
		log_printf(WLOG, "Couldn't create music mixer group!\n");
		title_g = a2_NewGroup(state, master_g);
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
	if(rootvoice < 0)
		log_printf(ELOG, "Couldn't find root voice!\n");

	g_wrap(WORLD_SIZEX, WORLD_SIZEY);
	g_scale(VIEWLIMIT * 3 / 2, VIEWLIMIT);

	time = SDL_GetTicks();
	return 0;
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
	master_g = 0;
	ui_g = 0;
	sfx_g = 0;
	music_g = 0;
	title_g = 0;
	noisehandle = 0;
	musichandle = 0;
	memset(modules, 0, sizeof(modules));
	memset(sounds, 0, sizeof(sounds));
}



/*--------------------------------------------------
	Main controls
--------------------------------------------------*/

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


void KOBO_sound::music(int sng, bool ingame)
{
	if(!state)
		return;
	if(musichandle)
	{
		a2_Send(state, musichandle, 1);
		a2_Release(state, musichandle);
		musichandle = 0;
	}
	if((sng >= 0) && sounds[sng])
		musichandle = a2_Start(state, ingame ? music_g : title_g,
				sounds[sng]);
}


void KOBO_sound::jingle(int sng)
{
	if(!state)
		return;
	if(sounds[sng])
		a2_Play(state, master_g, sounds[sng], 0.0f, 0.5f);
}


/*--------------------------------------------------
	In-game sound
--------------------------------------------------*/

void KOBO_sound::g_music(unsigned scene)
{
//FIXME
	music(SOUND_INGAMESONG, true);
//FIXME
}


void KOBO_sound::ui_play(unsigned wid, int vol, int pitch, int pan)
{
	if(!state || !sounds[wid])
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


void KOBO_sound::g_play(unsigned wid, int x, int y, int vol, int pitch)
{
	int volume, vx, vy, pan;

	if(!state || !sounds[wid])
		return;

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

	a2_Play(state, sfx_g, sounds[wid], (pitch / 65536.0f - 60.0f)  / 12.0f,
			volume / 65536.0f, pan / 65536.0f);
}


void KOBO_sound::g_play0(unsigned wid, int vol, int pitch)
{
	if(!state || !sounds[wid])
		return;
	a2_Play(state, sfx_g, sounds[wid], (pitch / 65536.0f - 60.0f)  / 12.0f,
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


void KOBO_sound::g_player_damage(float level)
{
	int p0 = (60<<16) + (pubrand.get(9) << 8);
	g_play(SOUND_DAMAGE, listener_x - 100, listener_y,
			65536 * level, p0 + 3000);
	g_play(SOUND_DAMAGE, listener_x + 100, listener_y,
			65536 * level, p0 - 3000);
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
	music(SOUND_TITLESONG);
}


void KOBO_sound::ui_noise(int n)
{
	if(!state || !sounds[SOUND_UI_NOISE])
		return;
	if(noisehandle)
	{
		a2_Send(state, noisehandle, 1);
		a2_Release(state, noisehandle);
		noisehandle = 0;
	}
	if(n)
		noisehandle = a2_Start(state, ui_g, sounds[
				n == 1 ? SOUND_UI_NOISE : SOUND_UI_LOADER]);
}


void KOBO_sound::ui_open()
{
	ui_play(SOUND_UI_OPEN);
}


void KOBO_sound::ui_ok()
{
	ui_play(SOUND_UI_OK);
}


void KOBO_sound::ui_cancel()
{
	ui_play(SOUND_UI_CANCEL);
}


void KOBO_sound::ui_move()
{
	ui_play(SOUND_UI_MOVE);
}


void KOBO_sound::ui_tick()
{
	ui_play(SOUND_UI_TICK);
}


void KOBO_sound::ui_error()
{
	ui_play(SOUND_UI_ERROR);
}


void KOBO_sound::ui_play()
{
	ui_play(SOUND_UI_PLAY);
}


void KOBO_sound::ui_pause()
{
	ui_play(SOUND_UI_PAUSE);
}


void KOBO_sound::ui_ready()
{
	ui_play(SOUND_UI_READY);
}


void KOBO_sound::ui_countdown(int remain)
{
	ui_play(SOUND_UI_CDTICK, 32768, (60 - remain)<<16);
}


void KOBO_sound::ui_gameover()
{
	ui_play(SOUND_UI_GAMEOVER);
}
