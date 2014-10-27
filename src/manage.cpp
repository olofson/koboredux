/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996 Akira Higuchi
 * Copyright (C) 2002 Jeremy Sheeley
 * Copyright (C) 2001-2003, 2007, 2009 David Olofson
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

// Define to get time and ships lost printed out per finished stage
// NOTE: Also disables bonus ships!
#undef	PLAYSTATS

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#ifndef M_PI
# define M_PI 3.14159265358979323846	/* pi */
#endif

#include "kobo.h"
#include "screen.h"
#include "manage.h"
#include "options.h"
#include "scenes.h"
#include "score.h"
#include "enemies.h"
#include "myship.h"
#include "radar.h"
#include "gamectl.h"
#include "states.h"
#include "audio.h"
#include "random.h"

#define GIGA             1000000000

int _manage::blank = 0;
int _manage::next_state_out;
int _manage::next_state_next;
int _manage::game_seed;
int _manage::scene_num;
int _manage::ships;
int _manage::score;
float _manage::disp_health;
float _manage::disp_temp;
int _manage::score_changed = 1;
int _manage::ships_changed = 1;
int _manage::bonus_next;
int _manage::flush_score_count = 0;
int _manage::flush_ships_count = 0;
int _manage::scroll_jump = 0;
int _manage::delay_count;
int _manage::rest_cores;
int _manage::introtime = 0;
int _manage::exit_manage = 0;
int _manage::playing = 0;
int _manage::_get_ready = 0;
int _manage::_game_over = 0;
s_hiscore_t _manage::hi;
int _manage::noise_duration = 0;
int _manage::noise_timer = 0;
int _manage::noise_flash = 500;
float _manage::noise_level = 0.0f;
int _manage::intro_x = CHIP_SIZEX * (64-18);
int _manage::intro_y = CHIP_SIZEY * (64-7);
int _manage::show_bars = 0;

#ifdef PLAYSTATS
static Uint32 start_time = 0;
static int start_ships = 0;
#endif


void _manage::set_bars()
{
	whealth->enable(show_bars);
	wtemp->enable(show_bars);
	wttemp->enable(show_bars);
}


// NOTE:
//	After a noise out has faded out the sound effects, it's the
//	noise/interfecence effect when the playfield is reinitialized
//	that "accidentally" restores volume!
//
void _manage::run_noise()
{
	if(noise_flash == -2)
	{
		// Damage flash
		noise_timer -= game.speed;
		if(noise_timer <= 0)
			noise_timer = 0;
		screen.noise(noise_timer);
		float t = (float)noise_timer / noise_duration;
		wmain->background(wmain->map_rgb(wmain->fadergb(0xff0000,
				(int)(t * noise_level * 64))));
		screen.set_noise(B_HITNOISE, t * noise_level,
				t * noise_level, 0.0f);
		return;
	}
	else if(noise_flash == -1)
	{
		// Noise-out
		noise_timer -= game.speed;
		if(noise_timer <= 0)
			noise_timer = 0;
		float t = (float)noise_timer / noise_duration;
		float a = t * 2.0f * M_PI;
		wmain->background(wmain->map_rgb(wmain->fadergb(0x00ff99,
				(int)(sin(a * 0.5f) * 0.3f * 64))));
		screen.set_noise(B_NOISE, 1.0f - t,
				sin(a * 0.5f) * 0.3f,
				0.3f + t * 0.5f);
		sound.sfx_volume(t);
		return;
	}

	wmain->background(wmain->map_rgb(0x00000));

	// Noise/interference
	int noise;
	if(noise_timer > noise_flash)
	{
		noise_timer -= game.speed;
		noise = 1;
	}
	else if(noise_timer > 0)
	{
		unsigned prob = noise_flash - noise_timer;
		prob <<= 8;
		prob /= noise_flash;
		noise_timer -= game.speed;
		noise = pubrand.get(8) > prob;
	}
	else
	{
		noise_timer = 0;
		noise = 0;
	}

	noise_level += (noise - noise_level) * 0.3f;
	screen.set_noise(B_NOISE, noise_level, 0.0f, noise_level);
	screen.noise(noise_level > 0.1f);

	sound.ui_noise(noise);
}


void _manage::noise(int duration, int flash)
{
	noise_flash = flash;
	noise_duration = noise_timer = duration;
	sound.ui_noise(-1);
	screen.set_noise(B_NOISE, 1.0f, 0.0f, 1.0f);
	noise_level = 1.0f;
}


void _manage::noise_out(int duration)
{
	noise_flash = -1;
	noise_duration = noise_timer = duration;
	screen.noise(1);
}


void _manage::noise_damage(float amt)
{
	noise_flash = -2;
	noise_duration = noise_timer = (int)(300.0f + 1500.0f * amt);
	noise_level = 0.3f + 0.3f * amt;
}


void _manage::game_start()
{
	_game_over = 0;
	hi.clear();

	game.set(GAME_SINGLE, (skill_levels_t)scorefile.profile()->skill);

#ifdef PLAYSTATS
	ships = 100;
#else
	ships = game.lives;
#endif
	disp_health = 0;
	disp_temp = 0;
	score = 0;
	flush_score_count = 0;
	bonus_next = game.bonus_first;
	gengine->period(game.speed);
	sound.period(game.speed);
	screen.init_scene(scene_num);
	init_resources_to_play(1);

	gamecontrol.clear();
	gamecontrol.repeat(0, 0);

	playing = 1;
	_get_ready = 1;

	hi.skill = game.skill;
	hi.playtime = 0;
	hi.gametype = game.type;
#if 1
	// The new skill levels are under development!
	// We want to mark highscores so they're not mistaken
	// for official scores from a future finalized version.
	if(game.skill != SKILL_CLASSIC)
		hi.gametype |= GAME_EXPERIMENTAL;
#endif
	hi.saves = 0;
	hi.loads = 0;
	hi.start_scene = scene_num;
	hi.end_lives = ships;
	sound.g_music(SOUND_BGM);

#ifdef PLAYSTATS
	start_time = SDL_GetTicks();
	start_ships = ships;
#endif
}


void _manage::game_stop()
{
	if(!prefs->cmd_cheat && !prefs->cmd_pushmove)
	{
		hi.score = score;
		hi.end_scene = scene_num;
		hi.end_health = myship.health();

		scorefile.record(&hi);
	}
	ships = 0;
	ships_changed = 1;
	audio_channel_stop(0, -1);
	playing = 0;
}

void _manage::next_scene()
{
#ifdef PLAYSTATS
	printf("stage %d:\n", scene_num);
	Uint32 nst = SDL_GetTicks();
	printf("    time:   %d s\n", (nst - start_time) / 1000);
	printf("  deaths:   %d\n", start_ships - ships);
	start_time = nst;
	start_ships = ships;
#endif
	scene_num++;
	if(scene_num >= GIGA - 1)
		scene_num = GIGA - 2;
	screen.init_scene(scene_num);
	scroll_jump = 1;
	_get_ready = 1;
}


void _manage::retry()
{
	if(!prefs->cmd_cheat)
	{
		ships--;
		ships_changed = 1;
	}
	if(ships <= 0)
	{
		if(!_game_over)
		{
			game_stop();
			_game_over = 1;
		}
	}
	else
		_get_ready = 1;
	gamecontrol.clear();
}


void _manage::init_resources_title()
{
	noise(1000, 800);
	screen.init_scene(INTRO_SCENE);
	gengine->period(30);
	sound.period(30);
	gamerand.init();
	enemies.init();
	myship.init();
	myship.off();
	screen.prepare();
	screen.generate_fixed_enemies();
	put_info();
	put_score();
	put_ships();
	put_health(1);
	put_temp(1);
	run_intro();
	gengine->force_scroll();
	gamecontrol.repeat(KOBO_KEY_DELAY, KOBO_KEY_REPEAT);
	show_bars = 0;
	set_bars();
}


void _manage::init_resources_to_play(int newship)
{
	noise(400, 300);
	delay_count = 0;
	flush_score_count = (flush_score_count) ? -1 : 0;
	flush_ships_count = 0;
	score_changed = 0;
	next_state_out = 0;
	next_state_next = 0;

	gamerand.init();
	game_seed = gamerand.get_seed();
	enemies.init();
	myship.init();
	if(newship)
		myship.health(game.health);
	rest_cores = screen.prepare();
	scroll_jump = 1;
	screen.generate_fixed_enemies();
	put_info();
	put_score();
	put_ships();
	switch(game.skill)
	{
	  case SKILL_CLASSIC:
		show_bars = 0;
		break;
	  case SKILL_NEWBIE:
	  case SKILL_GAMER:
	  case SKILL_ELITE:
	  case SKILL_GOD:
	  default:
		show_bars = 1;
		break;
	}
	set_bars();
	put_health(1);
	put_temp(1);
	myship.put();
	gengine->scroll(PIXEL2CS(myship.get_virtx()),
			PIXEL2CS(myship.get_virty()));
	gengine->force_scroll();
}


void _manage::put_health(int force)
{
	int h = myship.health();
	if(h > disp_health)
	{
		disp_health += (float)game.health * .05;
		if(disp_health > h)
			disp_health = h;
//		whealth->background(whealth->map_rgb(0, 100, 0));
	}
	else if(h < disp_health)
	{
		disp_health -= (float)game.health * .1;
		if(disp_health < h)
			disp_health = h;
//		whealth->background(whealth->map_rgb(128, 0, 0));
	}
//	else
//		whealth->background(whealth->map_rgb(0, 0, 0));
	whealth->value((float)disp_health / game.health);
	if(force)
		whealth->invalidate();
}


void _manage::put_temp(int force)
{
	wtemp->value(myship.get_nose_temp() / 256.0f);
	wttemp->value(myship.get_tail_temp() / 256.0f);
	if(force)
	{
		wtemp->invalidate();
		wttemp->invalidate();
	}
}


void _manage::put_info()
{
	static char s[16];

	snprintf(s, 16, "%010d", scorefile.highscore());
	dhigh->text(s);
	dhigh->on();

	snprintf(s, 16, "%03d", scene_num + 1);
	dstage->text(s);
	dstage->on();

	score_changed = 1;
	ships_changed = 1;
}


void _manage::put_score()
{
	if(score_changed)
	{
		static char s[32];
		snprintf(s, 16, "%010d", score);
		dscore->text(s);
		dscore->on();
		if(score > scorefile.highscore())
		{
			dhigh->text(s);
			dhigh->on();
		}
		score_changed = 0;
	}
	if(flush_score_count > 0)
		flush_score();
}


void _manage::put_ships()
{
	if(ships_changed)
	{
		static char s[32];
		if(!prefs->cmd_cheat)
			snprintf(s, 16, "%03d", ships);
		else
			snprintf(s, 16, "999");
		dships->text(s);
		dships->on();
		ships_changed = 0;
	}
	if(flush_ships_count > 0)
		flush_ships();
}


void _manage::flush_score()
{
	flush_score_count--;
	if(flush_score_count & 1)
		return;

	if(flush_score_count & 2)
		dscore->off();
	else
		dscore->on();
	if(flush_score_count == 0)
		flush_score_count = -1;
}


void _manage::flush_ships()
{
	flush_ships_count--;
	if(flush_ships_count & 1)
		return;

	if(flush_ships_count & 2)
		dships->off();
	else
		dships->on();
	if(flush_ships_count == 0)
		flush_ships_count = -1;
}


/****************************************************************************/
void _manage::init()
{
	scorefile.init();
	ships = 0;
	exit_manage = 0;
	scene_num = -1;
	flush_ships_count = 0;
	flush_score_count = 0;
	delay_count = 0;
	screen.init();
	init_resources_title();
}


void _manage::run_intro()
{
	double t = SDL_GetTicks() * 0.001f;
	gengine->scroll(PIXEL2CS(intro_x), PIXEL2CS(intro_y));
	intro_y -= 3;
	intro_x &= MAP_SIZEX*CHIP_SIZEX-1;
	intro_y &= MAP_SIZEY*CHIP_SIZEY-1;
	sound.g_position(intro_x + (WSIZE >> 1), intro_y + (WSIZE >> 1));
	myship.set_position(intro_x + (WSIZE >> 1) + (int)(WSIZE * 0.3f * sin(t)),
			intro_y + (WSIZE >> 1) + (int)(WSIZE * 0.3f * sin(t * 1.73)));
	enemies.move_intro();
	enemies.put();

	put_health();
	put_temp();
	put_score();
	put_ships();

	run_noise();
}


void _manage::update()
{
	myship.put();
	enemies.put();
	put_score();
	put_ships();
	gengine->scroll(PIXEL2CS(myship.get_virtx()),
			PIXEL2CS(myship.get_virty()));
	if(scroll_jump)
	{
		gengine->force_scroll();
		scroll_jump = 0;
	}
	run_noise();
}


void _manage::run_pause()
{
	put_health();
	put_temp();
	update();
}


void _manage::run_game()
{
	put_health();
	put_temp();

	if(delay_count)
		delay_count--;

	if(delay_count == 1)
	{
		if(enemies.exist_pipe())
			delay_count = 2;
		else
		{
			int newship = next_state_out;
			put_info();
			if(next_state_out)
			{
				retry();
				if(ships <= 0)
					return;
			}
			if(next_state_next)
				next_scene();
			init_resources_to_play(newship);
			return;
		}
	}

	myship.move();
	enemies.move();
	myship.hit_structure();
	update();
	++hi.playtime;
}


void _manage::lost_myship()
{
	if(delay_count == 0)
		delay_count = 100;
	next_state_out = 1;
}


void _manage::destroyed_a_core()
{
	// Award 10% health bonus for destroyed core!
	myship.health_bonus(10);
	rest_cores--;
	if(rest_cores == 0)
	{
		next_state_next = 1;
		delay_count = 50;
		// Award another 25% health bonus for stage cleared!
		myship.health_bonus(25);
	}
	screen.generate_fixed_enemies();
}


void _manage::add_score(int sc)
{
	score += sc;
	if(score >= GIGA)	//This *could* happen... Or maybe not. :-)
		score = GIGA - 1;
	else if(!prefs->cmd_cheat)
	{
#ifndef	PLAYSTATS
		if(score >= bonus_next)
		{
			bonus_next += game.bonus_every;
			ships++;
			ships_changed = 1;
			flush_ships_count = 50;
			sound.ui_oneup();
		}
#endif
		if(score >= scorefile.highscore())
		{
			if(flush_score_count == 0)
				flush_score_count = 50;
		}
	}
	score_changed = 1;
}


void _manage::select_next(int redraw_map)
{
	if((scene_num < scorefile.last_scene()) || prefs->cmd_cheat)
	{
		sound.ui_tick();
		scene_num++;
	}
	else
		sound.ui_error();
	select_scene(scene_num);
}


void _manage::select_prev(int redraw_map)
{
	scene_num--;
	if(scene_num < 0)
	{
		sound.ui_error();
		scene_num = 0;
	}
	else
		sound.ui_tick();
	select_scene(scene_num);
}


void _manage::regenerate()
{
	sound.ui_tick();
	select_scene(scene_num, 1);
}


void _manage::select_scene(int scene, int redraw_map)
{
        scene_num = scene;
	put_info();
	if(redraw_map)
		screen.init_scene(-scene_num - 1);
	noise(150, 0);
}


void _manage::abort()
{
	if(!exit_manage)
	{
		game_stop();
		exit_manage = 1;
	}
}


void _manage::freeze_abort()
{
	exit_manage = 1;
}


void _manage::reenter()
{
	exit_manage = 0;
	put_health(1);
	put_temp(1);
	put_info();
	put_score();
	put_ships();
}


int _manage::get_ready()
{
	if(_get_ready)
	{
		_get_ready = 0;
		return 1;
	}
	return 0;
}


int _manage::game_over()
{
	return _game_over;
}
