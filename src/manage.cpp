/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2002 Jeremy Sheeley
 * Copyright 2001-2003, 2007, 2009 David Olofson
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

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
#include "random.h"

#define GIGA             1000000000

KOBO_gamestates _manage::gamestate = GS_NONE;
bool _manage::paused = false;
int _manage::game_seed;
int _manage::scene_num;
int _manage::score;
float _manage::disp_health;
int _manage::score_changed = 1;
int _manage::flash_score_count = 0;
int _manage::scroll_jump = 0;
int _manage::delay_count;
int _manage::rest_cores;
s_hiscore_t _manage::hi;
int _manage::noise_duration = 0;
int _manage::noise_timer = 0;
int _manage::noise_flash = 500;
float _manage::noise_level = 0.0f;
int _manage::intro_x = TILE_SIZEX * (64 - 18);
int _manage::intro_y = TILE_SIZEY * (64 - 7);
int _manage::show_bars = 0;
int _manage::cam_lead_x = 0;
int _manage::cam_lead_y = 0;
int _manage::cam_lead_xf = 0;
int _manage::cam_lead_yf = 0;


void _manage::set_bars()
{
	wshield->enable(show_bars);
}


// NOTE:
//	After a noise out has faded out the sound effects, it's the
//	noise/interference effect when the playfield is reinitialized
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
	else
	{
		noise_timer = 0;
		noise = 0;
		sound.ui_noise(0);
	}

	noise_level += (noise - noise_level) * 0.3f;
	screen.set_noise(B_NOISE, noise_level, 0.0f, noise_level);
	screen.noise(noise_level > 0.1f);
}


void _manage::noise(int duration, int flash)
{
	noise_flash = flash;
	noise_duration = noise_timer = duration;
	screen.set_noise(B_NOISE, 1.0f, 0.0f, 1.0f);
	noise_level = 1.0f;
	sound.ui_noise(1);
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


void _manage::run_leds()
{
	pxtop->frame();
	pxbottom->frame();
	pxleft->frame();
	pxright->frame();
}


void _manage::start_intro()
{
	init_resources_title();
	gamestate = GS_INTRO;
}


void _manage::select_scene(int scene)
{
	scene_num = scene;
	put_info();
	screen.init_scene(-scene_num - 1);
	noise(150, 0);
	gamestate = GS_SELECT;
}


void _manage::start_game()
{
	hi.clear();

	game.set(GAME_SINGLE, (skill_levels_t)scorefile.profile()->skill);

	disp_health = 0;
	score = 0;
	flash_score_count = 0;
	gengine->period(game.speed);
	screen.init_scene(scene_num);
	init_resources_to_play(true);

	gamecontrol.clear();
	gamecontrol.repeat(0, 0);

	hi.skill = game.skill;
	hi.playtime = 0;
	hi.gametype = game.type;
#if 1
	// The new skill levels are under development!
	// We want to mark highscores so they're not mistaken
	// for official scores from a future finalized version.
	hi.gametype |= GAME_EXPERIMENTAL;
#endif
	hi.saves = 0;
	hi.loads = 0;
	hi.start_scene = scene_num;
	hi.end_lives = 0;
	sound.g_music(scene_num);

	gamestate = GS_GETREADY;
}


void _manage::player_ready()
{
	if(gamestate == GS_GETREADY)
		gamestate = GS_PLAYING;
}


void _manage::next_scene()
{
	scene_num++;
	if(scene_num >= GIGA - 1)
		scene_num = GIGA - 2;
	screen.init_scene(scene_num);
	scroll_jump = 1;
	gamestate = GS_GETREADY;
}


void _manage::init_resources_title()
{
	noise(1000, 800);
	screen.init_scene(INTRO_SCENE);
	gengine->period(30);
	gamerand.init();
	enemies.init();
	enemies.is_intro = 1;
	myship.init();
	myship.off();
	screen.prepare();
	screen.generate_fixed_enemies();
	put_info();
	put_score();
	put_health(1);
	run_intro();
	gengine->camfilter(0);
	gengine->force_scroll();
	gamecontrol.repeat(KOBO_KEY_DELAY, KOBO_KEY_REPEAT);
	show_bars = 0;
	set_bars();
	pxtop->fx(PFX_SCAN, PCOLOR_CORE);
	pxbottom->fx(PFX_SCANREV, PCOLOR_CORE);
	pxleft->fx(PFX_SCANREV, PCOLOR_CORE);
	pxright->fx(PFX_SCAN, PCOLOR_CORE);
	wdash->fade(1.0f);
	wdash->mode(DASHBOARD_TITLE);
}


void _manage::init_resources_to_play(bool newship)
{
	noise(400, 300);
	delay_count = 0;
	flash_score_count = (flash_score_count) ? -1 : 0;
	score_changed = 0;

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
	gengine->camfilter(KOBO_CAM_FILTER);
	show_bars = 1;
	set_bars();
	put_health(1);
	myship.put();
	gengine->scroll(myship.get_csx() - PIXEL2CS(WMAIN_W / 2),
			myship.get_csy() - PIXEL2CS(WMAIN_H / 2));
	gengine->force_scroll();
	pxtop->fx(PFX_OFF);
	pxbottom->fx(PFX_OFF);
	pxleft->fx(PFX_OFF);
	pxright->fx(PFX_OFF);
	wdash->fade(1.0f);
	wdash->mode(DASHBOARD_GAME);

	gamecontrol.clear();
}


void _manage::put_health(int force)
{
	int h = myship.health();
	if(h > disp_health)
	{
		disp_health += (float)game.health * .05;
		if(disp_health > h)
			disp_health = h;
	}
	else if(h < disp_health)
	{
		disp_health -= (float)game.health * .1;
		if(disp_health < h)
			disp_health = h;
	}
	wshield->value((float)disp_health / game.health);
	wshield->marker((float)myship.regen_next() / game.health);
	if(force)
		wshield->invalidate();
}


void _manage::put_info()
{
	static char s[16];

	snprintf(s, 16, "%d", scorefile.highscore());
	dhigh->text(s);
	dhigh->on();

	snprintf(s, 16, "%d", scene_num + 1);
	dstage->text(s);
	dstage->on();

	snprintf(s, 16, "%d", scene_num / 10 % 5 + 1);
	dregion->text(s);
	dregion->on();

	snprintf(s, 16, "%2d", scene_num % 10 + 1);
	dlevel->text(s);
	dlevel->on();

	score_changed = 1;
}


void _manage::put_score()
{
	if(score_changed)
	{
		static char s[32];
		snprintf(s, 16, "%d", score);
		dscore->text(s);
		dscore->on();
		if(score > scorefile.highscore())
		{
			dhigh->text(s);
			dhigh->on();
		}
		score_changed = 0;
	}
	if(flash_score_count > 0)
		flash_score();
}


void _manage::flash_score()
{
	flash_score_count--;
	if(flash_score_count & 1)
		return;

	if(flash_score_count & 2)
		dscore->off();
	else
		dscore->on();
	if(flash_score_count == 0)
		flash_score_count = -1;
}


/****************************************************************************/
void _manage::init()
{
	scorefile.init();
	scene_num = -1;
	flash_score_count = 0;
	delay_count = 0;
	screen.init_maps();
	gamestate = GS_NONE;
}


void _manage::run_intro()
{
	gengine->scroll(PIXEL2CS(intro_x), PIXEL2CS(intro_y));
	intro_y -= 1;
	intro_x &= WORLD_SIZEX - 1;
	intro_y &= WORLD_SIZEY - 1;
	float w = intro_y * M_PI * 2.0f * 3.0f / WORLD_SIZEX;
	myship.set_position(intro_x + WMAIN_W / 2 +
			(int)(WMAIN_W * 0.3f * sin(w)),
			intro_y + WMAIN_H / 2 +
			(int)(WMAIN_H * 0.3f * cos(w)));
	enemies.move_intro();
	++hi.playtime;
	enemies.put();

	put_health();
	put_score();

	run_noise();
	run_leds();
}


void _manage::update()
{
	myship.put();
	enemies.put();
	put_score();

	// Constant speed chase + IIR filtered camera lead
	int tlx = myship.get_velx() * KOBO_CAM_LEAD;
	int tly = myship.get_vely() * KOBO_CAM_LEAD;
	if(cam_lead_x < tlx)
	{
		cam_lead_x += KOBO_CAM_LEAD_SPEED;
		if(cam_lead_x > tlx)
			cam_lead_x = tlx;
	}
	else if(cam_lead_x > tlx)
	{
		cam_lead_x -= KOBO_CAM_LEAD_SPEED;
		if(cam_lead_x < tlx)
			cam_lead_x = tlx;
	}
	if(cam_lead_y < tly)
	{
		cam_lead_y += KOBO_CAM_LEAD_SPEED;
		if(cam_lead_y > tly)
			cam_lead_y = tly;
	}
	else if(cam_lead_y > tly)
	{
		cam_lead_y -= KOBO_CAM_LEAD_SPEED;
		if(cam_lead_y < tly)
			cam_lead_y = tly;
	}
	cam_lead_xf += (cam_lead_x - cam_lead_xf) * KOBO_CAM_LEAD_FILTER >> 8;
	cam_lead_yf += (cam_lead_y - cam_lead_yf) * KOBO_CAM_LEAD_FILTER >> 8;

	gengine->scroll(myship.get_csx() + cam_lead_xf -
			PIXEL2CS(WMAIN_W / 2),
			myship.get_csy() + cam_lead_yf -
			PIXEL2CS(WMAIN_H / 2));
	if(scroll_jump)
	{
		gengine->force_scroll();
		scroll_jump = 0;
	}
	run_noise();
	run_leds();
}


void _manage::run_game()
{
	put_health();
	myship.move();
	enemies.move();
	myship.check_base_bolts();
	update();
	++hi.playtime;
}


void _manage::run()
{
	if(paused)
	{
		put_health();
		update();
		return;
	}
	switch(gamestate)
	{
	  case GS_NONE:
		break;
	  case GS_INTRO:
	  case GS_SELECT:
		run_intro();
		break;
	  case GS_GETREADY:
		put_health();
		update();
		break;
	  case GS_PLAYING:
	  case GS_GAMEOVER:
		run_game();
		break;
	  case GS_LEVELDONE:
		if(delay_count && !enemies.exist_pipe())
			delay_count--;
		if(delay_count == 1)
		{
			put_info();
			next_scene();
			init_resources_to_play(true);
		}
		else
			run_game();
		break;
	}
}


void _manage::abort_game()
{
	gamestate = GS_NONE;
	sound.g_kill_all();
	wdash->fade(1.0f);
	wdash->mode(DASHBOARD_TITLE);
}


void _manage::pause(bool p)
{
	if(p == paused)
		return;
	paused = p;
	if(paused)
	{
		sound.g_player_fire(false);
		sound.ui_play(SOUND_UI_PAUSE);
	}
	else
		sound.ui_play(SOUND_UI_PLAY);
}


void _manage::lost_myship()
{
	if(!prefs->cheat && !prefs->pushmove)
	{
		hi.score = score;
		hi.end_scene = scene_num;
		hi.end_health = myship.health();
		scorefile.record(&hi);
	}
	gamestate = GS_GAMEOVER;
}


void _manage::destroyed_a_core()
{
	if(gamestate != GS_PLAYING)
		return;	// Can't win after death...

	// Award health bonus for destroyed core!
	myship.health_bonus(game.core_destroyed_health_bonus);
	rest_cores--;
	if(rest_cores == 0)
	{
		delay_count = 50;
		// Award extra health bonus for stage cleared!
		myship.health_bonus(game.stage_cleared_health_bonus);
		myship.state(SHIP_INVULNERABLE);
		gamestate = GS_LEVELDONE;
	}
	screen.generate_fixed_enemies();
}


void _manage::add_score(int sc)
{
	score += sc;
	if(score >= GIGA)	//This *could* happen... Or maybe not. :-)
		score = GIGA - 1;
	else if(!prefs->cheat)
	{
		if(score >= scorefile.highscore())
		{
			if(flash_score_count == 0)
				flash_score_count = 50;
		}
	}
	score_changed = 1;
}


void _manage::reenter()
{
	screen.init_background();
	put_health(1);
	put_info();
	put_score();
	set_bars();
}
