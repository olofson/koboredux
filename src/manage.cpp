/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2002 Jeremy Sheeley
 * Copyright 2001-2003, 2007, 2009 David Olofson
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "kobo.h"
#include "kobolog.h"
#include "screen.h"
#include "manage.h"
#include "options.h"
#include "scenes.h"
#include "enemies.h"
#include "myship.h"
#include "radar.h"
#include "gamectl.h"
#include "states.h"
#include "random.h"

#define GIGA             1000000000

KOBO_gamestates _manage::gamestate = GS_NONE;
KOBO_replaymodes _manage::replaymode = RPM_PLAY;
bool _manage::is_paused = false;

KOBO_campaign *_manage::campaign = NULL;
KOBO_replay *_manage::replay = NULL;
bool _manage::owns_replay = false;
KOBO_player_controls _manage::lastctrl = KOBO_PC_FIRE;
unsigned _manage::ctrltimer = 0;

bool _manage::in_background = false;
bool _manage::show_bars = false;
float _manage::disp_health;
float _manage::disp_charge;
int _manage::flash_score_count = 0;
bool _manage::score_changed = true;
int _manage::intro_x = TILE_SIZEX * (64 - 18);
int _manage::intro_y = TILE_SIZEY * (64 - 7);

int _manage::game_seed;
int _manage::total_cores;
int _manage::remaining_cores;
int _manage::selected_slot = 0;
int _manage::selected_stage = -1;
int _manage::last_stage;	// HAX for the start stage selector
skill_levels_t _manage::selected_skill = KOBO_DEFAULT_SKILL;
unsigned _manage::highscore = 0;
unsigned _manage::score;
unsigned _manage::playtime;

bool _manage::scroll_jump = false;
int _manage::noise_flash = 500;
int _manage::noise_duration = 0;
int _manage::noise_timer = 0;
float _manage::noise_level = 0.0f;

int _manage::delayed_stage = -1;
KOBO_gamestates _manage::delayed_gamestate = GS_NONE;

int _manage::retry_skip = 0;

int _manage::cam_lead_x = 0;
int _manage::cam_lead_y = 0;
int _manage::cam_lead_xf = 0;
int _manage::cam_lead_yf = 0;

int _manage::shake_x = 0;
int _manage::shake_y = 0;
int _manage::shake_fade_x = 0;
int _manage::shake_fade_y = 0;

int _manage::delay_count;


const char *enumstr(KOBO_replaymodes rpm)
{
	switch(rpm)
	{
	  case RPM_PLAY:	return "RPM_PLAY";
	  case RPM_RETRY:	return "RPM_RETRY";
	  case RPM_REPLAY:	return "RPM_REPLAY";
	}
	return "<illegal KOBO_replaymodes value>";
}


const char *enumstr(KOBO_gamestates gst)
{
	switch(gst)
	{
	  case GS_NONE:		return "GS_NONE";
	  case GS_TITLE:	return "GS_TITLE";
	  case GS_SHOW:		return "GS_SHOW";
	  case GS_GETREADY:	return "GS_GETREADY";
	  case GS_PLAYING:	return "GS_PLAYING";
	  case GS_LEVELDONE:	return "GS_LEVELDONE";
	  case GS_GAMEOVER:	return "GS_GAMEOVER";
	  case GS_REPLAYEND:	return "GS_REPLAYEND";
	}
	return "<illegal KOBO_gamestates value>";
}


void _manage::set_bars()
{
	whealth->enable(show_bars);
	wcharge->enable(show_bars);
}


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
		screen.set_noise(B_NOISE, 1.0f - t,
				sin(a * 0.5f) * 0.3f, 0.3f + t * 0.5f);
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
	}
	noise_level += (noise - noise_level) * 0.3f;
	screen.set_noise(B_NOISE, noise_level, 0.0f, noise_level);
	screen.noise(noise_level > 0.1f);
	sound.ui_noise(noise ? S_UI_NOISE : 0);
}


void _manage::noise_glitch()
{
	noise_duration = noise_timer = 100;
	noise_level = 1.0f;
	noise_flash = 0;
	screen.set_noise(B_NOISE, 1.0f, 0.0f, 1.0f);
	screen.noise(1);
}


void _manage::noise_damage(float amt)
{
	if(!prefs->playerhitfx)
		return;
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


void _manage::state(KOBO_gamestates gst)
{
	if(prefs->debug)
		log_printf(ULOG, "Switched to %s.\n", enumstr(gst));
	gamestate = gst;
}


void _manage::select_slot(int sl)
{
	selected_slot = sl;
	if(selected_slot < 0)
	{
		campaign = NULL;
		return;
	}
	campaign = savemanager.campaign(selected_slot);
	log_printf(ULOG, "Selected%s campaign slot %d.\n",
			!campaign->exists() ? " empty" : "", selected_slot);
}


void _manage::select_stage(int stage, KOBO_gamestates gs)
{
	delayed_stage = -1;	// Cancel any pending stage change!
	selected_stage = stage;
	gamerand.init();
	screen.init_stage(stage, false);
	if(gs == GS_SHOW)
	{
		wradar->mode(RM_SHOW);
	}
	else if(gs == GS_TITLE)
	{
		wradar->mode(RM_OFF);
		enemies.init();
		enemies.is_intro = 1;
		myship.init(true);
		myship.off();
		screen.prepare();
		screen.generate_fixed_enemies();
		gengine->camfilter(0);
		gengine->force_scroll();
		show_bars = false;
		set_bars();
		pxtop->fx(PFX_SCAN, PCOLOR_CORE);
		pxbottom->fx(PFX_SCANREV, PCOLOR_CORE);
		pxleft->fx(PFX_SCANREV, PCOLOR_CORE);
		pxright->fx(PFX_SCAN, PCOLOR_CORE);
		wdash->fade(1.0f);
		wdash->mode(DASHBOARD_TITLE);
		stop_screenshake();
	}
	put_info();
	state(gs);
}


void _manage::show_stage(int stage, KOBO_gamestates gs)
{
	if(selected_stage < 0)
		select_stage(stage, gs);
	else if(stage != selected_stage)
	{
		delayed_stage = stage;
		delayed_gamestate = gs;
		screen.curtains(true, KOBO_ENTER_STAGE_FXTIME * 0.001f);
	}
}


void _manage::init_game(KOBO_replay *rp, bool newship)
{
	sound.timestamp_reset();
	sound.g_new_scene(100);
	sound.g_music(selected_stage);
	stop_screenshake();

	disp_health = 0.0f;
	disp_charge = 0.0f;
	flash_score_count = 0;
	score_changed = true;
	delay_count = 0;
	scroll_jump = true;
	show_bars = true;
	lastctrl = KOBO_PC_FIRE;
	retry_skip = 0;

	if(replay && owns_replay)
		delete replay;
	replay = NULL;
	owns_replay = false;

	if(rp)
	{
		// Start from replay!
		replay = rp;
		// Only use replay if compatible, and long enough!
		if((replay->recorded() >= KOBO_MIN_REPLAY_LENGTH) &&
				(replay->compatibility() == KOBO_RPCOM_FULL))
		{
			state(GS_PLAYING);
			if(replaymode == RPM_PLAY)
				replaymode = RPM_RETRY;
		}
		else
		{
			if(replaymode == RPM_REPLAY)
			{
				state(GS_REPLAYEND);
				delay_count = KOBO_REPLAYEND_TIMEOUT;
			}
			else
			{
				replaymode = RPM_PLAY;
				state(GS_GETREADY);
			}
		}
		log_printf(ULOG, "Starting at stage %d from replay!\n",
				replay->stage);
		if(prefs->debug)
			replay->log_dump(ULOG);
		selected_stage = replay->stage;
		game.set((game_types_t)replay->type,
				(skill_levels_t)replay->skill);
		gamerand.init(replay->seed);
		score = replay->score;
		replay->rewind();
	}
	else
	{
		// No replay! Start from parameters.
		state(GS_GETREADY);
		log_printf(ULOG, "Starting new level, stage %d!\n",
				selected_stage);
		game.set(GAME_SINGLE, selected_skill);
		replay = new KOBO_replay();
		replay->stage = selected_stage;
		replay->type = game.type;
		replay->skill = game.skill;
		replay->seed = game_seed = gamerand.get_seed();
		replay->score = score;
		replay->modified(true);
		if(campaign)
			campaign->add_replay(replay);
		else
			owns_replay = true;
	}

	playtime = 0;
	gengine->period(game.speed);
	screen.init_stage(selected_stage, true);
	last_stage = selected_stage;
	wradar->mode(RM_RADAR);
	enemies.init();
	if(rp)
	{
		// New ship with state from the replay data
		myship.init(replay->health, replay->charge);
	}
	else
	{
		// Old/new ship as specified; record state to replay!
		myship.init(newship);
		replay->health = myship.health();
		replay->charge = myship.charge();
		replay->modified(true);
	}
	total_cores = remaining_cores = screen.prepare();
	screen.generate_fixed_enemies();
	put_info();
	put_score();
	gengine->camfilter(KOBO_CAM_FILTER);
	set_bars();
	put_player_stats();
	pxtop->fx(PFX_OFF);
	pxbottom->fx(PFX_OFF);
	pxleft->fx(PFX_OFF);
	pxright->fx(PFX_OFF);
	wdash->fade(1.0f);
	wdash->mode(DASHBOARD_GAME);
	if(prefs->debug)
		replay->log_dump(ULOG);
}


void _manage::finalize_replay()
{
	if(replay && (replaymode == RPM_PLAY))
	{
		replay->end_health = myship.health();
		replay->end_charge = myship.charge();
		replay->end_score = score;
		replay->compact();
	}
}


void _manage::start_new_game()
{
	replaymode = RPM_PLAY;
	if(campaign)
		campaign->reset();
	score = 0;
	init_game(NULL, true);
	gamecontrol.clear();
}


bool _manage::continue_game()
{
	if(!campaign)
		return false;

	replay = campaign->get_replay(-1);
	if(!replay)
		return false;

	replaymode = RPM_PLAY;
	init_game(replay);
	advance(-KOBO_RETRY_REWIND);
	gamecontrol.clear();
	return true;
}


bool _manage::start_replay(int stage)
{
	if(!campaign)
		return false;

	replay = campaign->get_replay(stage);
	if(!replay)
		return false;

	replaymode = RPM_REPLAY;
	init_game(replay);
	gamecontrol.clear();
	return true;
}


void _manage::rewind()
{
	if(!replay)
	{
		log_printf(ELOG, "_manage::rewind() called with no replay!\n");
		return;
	}
	noise_glitch();
	init_game(replay);
	advance(-KOBO_RETRY_REWIND);
}


void _manage::advance(int frame)
{
	if(!replay)
	{
		log_printf(ELOG, "_manage::advance() with no replay!\n");
		return;
	}
	if(frame < 0)
	{
		frame = replay->recorded() + frame;
		if(frame < 0)
			frame = 0;
	}
	if(frame < (int)replay->position())
	{
		log_printf(ELOG, "_manage::advance(): Someone tried to rewind "
				"time from %d to %d!\n", replay->position(),
				frame);
		return;
	}
	if(frame == (int)replay->position())
		return;		// We're already there!

	KOBO_replaymodes replaymode_save = replaymode;
	replaymode = RPM_REPLAY;
	float volume_save = sound.g_volume();
	sound.g_volume(0.0f);
	while((int)replay->position() < frame)
	{
		KOBO_player_controls rpctrl = replay->read();
		if(rpctrl == KOBO_PC_END)
			rpctrl = KOBO_PC_NONE;
		myship.control(rpctrl);
		myship.move();
		enemies.move();
		myship.check_base_bolts();
		if(prefs->replaydebug)
			replay->verify_state();
		++playtime;
		sound.timestamp_reset();
	}
	kill_screenshake();
	scroll_jump = true;
	update();
	sound.timestamp_reset();
	sound.g_volume(volume_save);
	replaymode = replaymode_save;
}


void _manage::player_ready()
{
	if(gamestate == GS_GETREADY)
		state(GS_PLAYING);
}


void _manage::next_bookmark()
{
	if(!replay)
	{
		log_printf(ELOG, "_manage::next_bookmark() with no replay!\n");
		return;
	}
	sound.g_new_scene(100);
	advance(playtime + KOBO_RETRY_SKIP);
}


void _manage::prev_bookmark()
{
	if(!replay)
	{
		log_printf(ELOG, "_manage::prev_bookmark() with no replay!\n");
		return;
	}
	unsigned target = playtime > KOBO_RETRY_SKIP ?
			playtime - KOBO_RETRY_SKIP : 0;
	init_game(replay);
	advance(target);
}


void _manage::next_stage()
{
	if(screen.curtains())
		screen.curtains(false, KOBO_ENTER_STAGE_FXTIME * 0.001f);
	else
		noise_glitch();
	finalize_replay();
	selected_stage++;
	if(selected_stage >= GIGA - 1)
		selected_stage = GIGA - 2;
	switch(replaymode)
	{
	  case RPM_PLAY:
		init_game();
		if(campaign)
			campaign->save();
		if(selected_stage == km.smsg_stage)
		{
			sound.ui_play(S_UI_PAUSE);
			st_error.message(km.smsg_header, km.smsg_message);
			gsm.push(&st_error);
		}
#ifdef KOBO_DEMO
		if(selected_stage == (KOBO_DEMO_LAST_STAGE + 1))
			gsm.change(&st_demo_over);
#endif
		break;
	  case RPM_RETRY:
		// Wut? We're not supposed to get GS_LEVELDONE in this mode!
		log_printf(WLOG, "_manage::next_stage() during RPM_RETRY!\n");
		break;
	  case RPM_REPLAY:
		// Full replay! Move to next stage.
		replay = campaign->get_replay(selected_stage);
		if(replay)
			init_game(replay);
		else
		{
			state(GS_REPLAYEND);
			delay_count = KOBO_REPLAYEND_TIMEOUT;
		}
		break;
	}
}


void _manage::prev_stage()
{
	if(replaymode != RPM_REPLAY)
	{
		log_printf(WLOG, "_manage::prev_stage() used with other "
				"replay mode than RPM_REPLAY!\n");
		return;
	}
	if(selected_stage <= 1)
		return;
	noise_glitch();
	selected_stage--;
	replay = campaign->get_replay(selected_stage);
	if(replay)
		init_game(replay);
}


void _manage::put_player_stats()
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
	whealth->value((float)disp_health / game.health);
	whealth->marker((float)myship.regen_next() / game.health);

	h = myship.charge();
	if(h > disp_charge)
	{
		disp_charge += (float)game.charge * .05;
		if(disp_charge > h)
			disp_charge = h;
	}
	else if(h < disp_charge)
	{
		disp_charge -= (float)game.charge * .1;
		if(disp_charge < h)
			disp_charge = h;
	}
	wcharge->value((float)disp_charge / game.charge);
}


void _manage::put_info()
{
	static char s[16];

	snprintf(s, 16, "%d", highscore);
	dhigh->text(s);
	dhigh->on();

	snprintf(s, 16, "%d / %d", (selected_stage - 1) / 10 % 5 + 1,
			KOBO_REGIONS);
	dregion->text(s);
	dregion->on();

	snprintf(s, 16, "%d / %d", (selected_stage - 1) % 10 + 1,
			KOBO_LEVELS_PER_REGION);
	dlevel->text(s);
	dlevel->on();

	score_changed = true;
}


void _manage::put_score()
{
	if(score_changed)
	{
		static char s[32];
		snprintf(s, 16, "%d", score);
		dscore->text(s);
		dscore->on();
		if(score > highscore)
		{
			dhigh->text(s);
			dhigh->on();
		}
		score_changed = false;
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


void _manage::init()
{
	last_stage = selected_stage = -1;
	flash_score_count = 0;
	delay_count = 0;
	state(GS_NONE);
}


void _manage::screenshake(float mag_x, float mag_y, float fade)
{
	int nsx = mag_x * 256.0f;
	int nsy = mag_y * 256.0f;
	if((nsx > shake_x) || (nsy > shake_y))
		shake_fade_x = shake_fade_y = fade * 256.0f;
	if(nsx > shake_x)
		shake_x = nsx;
	if(nsy > shake_y)
		shake_y = nsy;
}


void _manage::stop_screenshake()
{
	shake_fade_x = shake_fade_y = 0.75 * 256.0f;
}


void _manage::kill_screenshake()
{
	shake_x = shake_y = 0;
}


void _manage::run_intro()
{
	intro_y -= 1;
	intro_x &= WORLD_SIZEX - 1;
	intro_y &= WORLD_SIZEY - 1;
	float w = intro_y * M_PI * 2.0f * 3.0f / WORLD_SIZEX;
	myship.set_position(intro_x + DASHW(MAIN) / 2 +
			(int)(DASHW(MAIN) * 0.3f * sin(w)),
			intro_y + DASHH(MAIN) / 2 +
			(int)(DASHH(MAIN) * 0.3f * cos(w)));
	enemies.move_intro();
	++playtime;
	enemies.put();

	gengine->scroll(PIXEL2CS(intro_x), PIXEL2CS(intro_y));
	if(scroll_jump)
	{
		enemies.force_positions();
		gengine->force_scroll();
		scroll_jump = false;
	}

	put_player_stats();
	put_score();

	run_noise();
	run_leds();
}


void _manage::update()
{
	// Update sprite positions
	myship.put();
	enemies.put();

	if(scroll_jump)
	{
		myship.force_position();
		enemies.force_positions();
	}

	// Render effects, displays, and LEDs
	run_noise();
	run_leds();
	put_player_stats();
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

	// Screen shake
	int ss = prefs->screenshake * prefs->screenshake;
	float ph = SDL_GetTicks() * 2.0f * M_PI * 0.001f;
	int shx = shake_x * ss * sin(ph * SCREEN_SHAKE_RATE_X);
	int shy = shake_y * ss * sin(ph * SCREEN_SHAKE_RATE_Y);
	shake_x = shake_x * shake_fade_x >> 8;
	shake_y = shake_y * shake_fade_y >> 8;

	// Apply scroll position
	gengine->scroll(myship.get_csx() + cam_lead_xf + shx -
			PIXEL2CS((int)DASHW(MAIN) / 2),
			myship.get_csy() + cam_lead_yf + shy -
			PIXEL2CS((int)DASHH(MAIN) / 2));
	if(scroll_jump)
	{
		gengine->force_scroll();
		scroll_jump = false;
	}
}


void _manage::run_game()
{
	KOBO_player_controls ctrlin = in_background ? KOBO_PC_NONE :
			myship.decode_input();
	KOBO_player_controls ctrl = KOBO_PC_NONE;
	switch(replaymode)
	{
	  case RPM_PLAY:
		ctrl = controls_play(ctrlin);
		break;
	  case RPM_RETRY:
		ctrl = controls_retry(ctrlin);
		break;
	  case RPM_REPLAY:
		ctrl = controls_replay(ctrlin);
		break;
	}
	myship.control(ctrl);
	myship.move();
	enemies.move();
	myship.check_base_bolts();
	if(replay && prefs->replaydebug)
		switch(replaymode)
		{
		  case RPM_PLAY:
			replay->record_state();
			break;
		  case RPM_RETRY:
		  case RPM_REPLAY:
			replay->verify_state();
			break;
		}
	update();
	++playtime;
	if(lastctrl == ctrlin)
		++ctrltimer;
	else
	{
		lastctrl = ctrlin;
		ctrltimer = 0;
	}
}


// Control input handling: Live + record
//
//	Use and record the control input! Stop recording the moment the player
//	dies, to keep the replay progress bar accurate.
//
KOBO_player_controls _manage::controls_play(KOBO_player_controls ctrl)
{
	if(replay && myship.alive())
		replay->write(ctrl);
	return ctrl;
}


// Control input handling: Rewind/retry; wait for player to take over
//
//	Control input is used for controlling the rewind/replay playback, and
//	also allows the player can take over and start playing at any point.
//
void _manage::controls_retry_skip(KOBO_player_controls ctrl)
{
	if((ctrl & KOBO_PC_DIR) != (lastctrl & KOBO_PC_DIR))
	{
		switch(ctrl & KOBO_PC_DIR)
		{
		  case 1:	// Up
			++retry_skip;
			if(retry_skip > 1)
				retry_skip = 1;
			break;
		  case 5:	// Down
			--retry_skip;
			if(retry_skip < -1)
				retry_skip = -1;
			break;
		}
		screen.curtains(retry_skip != 0,
				KOBO_RETRY_SKIP_FXTIME * 0.001f);
	}
	if(screen.curtains())
	{
		if(retry_skip > 0)
			next_bookmark();
		else if(retry_skip < 0)
			prev_bookmark();
		retry_skip = 0;
		screen.curtains(false, KOBO_RETRY_SKIP_FXTIME * 0.001f);
	}
}

KOBO_player_controls _manage::controls_retry(KOBO_player_controls ctrl)
{
	controls_retry_skip(ctrl);

	if((ctrl & KOBO_PC_FIRE) && !(lastctrl & KOBO_PC_FIRE))
	{
		// Player takes over control! Replay recording must be
		// resumed at exactly this frame, overwriting any
		// subsequent data.
		screen.curtains(false, KOBO_RETRY_SKIP_FXTIME * 0.001f);
		replay->punchin();
		replay->write(ctrl);
		replaymode = RPM_PLAY;
		sound.ui_countdown(0);
		gengine->period(game.speed);
		sound.g_volume(1.0f);
		sound.g_pitch(0.0f);
		return ctrl;
	}

	KOBO_player_controls rpctrl = replay->read();
	if(rpctrl == KOBO_PC_END)
	{
		rpctrl = KOBO_PC_NONE;
		if(myship.alive())
		{
			state(GS_REPLAYEND);
			delay_count = KOBO_REPLAYEND_TIMEOUT;
		}
		return rpctrl;
	}

	// Control the replay playback speed
	float rps = KOBO_REPLAY_NORMAL_RATE;
	float vol = KOBO_REPLAY_NORMAL_VOL;
	float pch = KOBO_REPLAY_NORMAL_PITCH;
	switch(ctrl & KOBO_PC_DIR)
	{
	  case 3:	// Right
		rps = KOBO_REPLAY_FAST_RATE;
		vol = KOBO_REPLAY_FAST_VOL;
		pch = KOBO_REPLAY_FAST_PITCH;
		break;
	  case 7:	// Left
		rps = KOBO_REPLAY_SLOW_RATE;
		vol = KOBO_REPLAY_SLOW_VOL;
		pch = KOBO_REPLAY_SLOW_PITCH;
		break;
	}
	gengine->period(game.speed * rps);
	sound.g_volume(vol);
	sound.g_pitch(pch);

	return rpctrl;
}


// Control input handling: Pure replay - no gameplay interaction possible
//
//	Control input is used only for controlling the replay speed, and to
//	skip back and forth between stages in the campaign.
//
KOBO_player_controls _manage::controls_replay(KOBO_player_controls ctrl)
{
	// Playback skip controls
	if((ctrl & KOBO_PC_DIR) != (lastctrl & KOBO_PC_DIR))
		switch(ctrl & KOBO_PC_DIR)
		{
		  case 1:	// Up
			if(campaign->get_replay(selected_stage + 1))
				next_stage();
			break;
		  case 5:	// Down
			prev_stage();
			break;
		}

	KOBO_player_controls rpctrl = replay ? replay->read() : KOBO_PC_END;
	if(rpctrl == KOBO_PC_END)
	{
		rpctrl = KOBO_PC_NONE;
		if(myship.alive())
		{
			state(GS_REPLAYEND);
			delay_count = KOBO_REPLAYEND_TIMEOUT;
		}
	}

	// Control the replay playback speed
	float rps = KOBO_REPLAY_NORMAL_RATE;
	float vol = KOBO_REPLAY_NORMAL_VOL;
	float pch = KOBO_REPLAY_NORMAL_PITCH;
	switch(ctrl & KOBO_PC_DIR)
	{
	  case 3:	// Right
		rps = KOBO_REPLAY_FAST_RATE;
		vol = KOBO_REPLAY_FAST_VOL;
		pch = KOBO_REPLAY_FAST_PITCH;
		break;
	  case 7:	// Left
		rps = KOBO_REPLAY_SLOW_RATE;
		vol = KOBO_REPLAY_SLOW_VOL;
		pch = KOBO_REPLAY_SLOW_PITCH;
		break;
	}
	gengine->period(game.speed * rps);
	sound.g_volume(vol);
	sound.g_pitch(pch);

	return rpctrl;
}


void _manage::run()
{
	// Delayed stage selection
	if((delayed_stage > 0) && screen.curtains())
	{
		select_stage(delayed_stage, delayed_gamestate);
		screen.curtains(false, KOBO_ENTER_STAGE_FXTIME * 0.001f);
		scroll_jump = true;
	}

	if(is_paused)
	{
		update();
		return;
	}
	switch(gamestate)
	{
	  case GS_NONE:
		break;
	  case GS_SHOW:
	  case GS_TITLE:
		run_intro();
		break;
	  case GS_GETREADY:
		update();
		break;
	  case GS_PLAYING:
		run_game();
		break;
	  case GS_GAMEOVER:
	  case GS_REPLAYEND:
		if(delay_count && !--delay_count)
			switch(replaymode)
			{
			  case RPM_REPLAY:
				state(GS_NONE);
				break;
			  case RPM_RETRY:
				rewind();
				break;
			  default:
				break;
			}
		if(gamestate == GS_GAMEOVER)
			run_game();
		else
		{
			if((replaymode == RPM_RETRY) && !in_background)
				controls_retry_skip(myship.decode_input());
			update();
		}
		break;
	  case GS_LEVELDONE:
		if(delay_count && !enemies.exist_pipe())
			delay_count--;
		if(delay_count == 1)
		{
			put_info();
			next_stage();
		}
		else
			run_game();
		break;
	}
}


float _manage::replay_progress()
{
	if(!replay)
		return 0.0f;

	return replay->progress();
}


int _manage::time_remaining()
{
	return delay_count * gengine->period();
}


int _manage::replay_stages()
{
	if(!campaign)
		return 0;

	return campaign->last_stage();
}


void _manage::abort_game()
{
	log_printf(ULOG, "Aborting game!\n");
	if(replaymode == RPM_PLAY)
		finalize_replay();
	sound.g_new_scene();
	wdash->fade(1.0f);
	wdash->mode(DASHBOARD_TITLE);
	if(campaign)
	{
		if(replaymode == RPM_PLAY)
			campaign->save();
		campaign = NULL;
	}
	if(replay && owns_replay)
		delete replay;
	replay = NULL;
	owns_replay = false;
	state(GS_NONE);
	replaymode = RPM_PLAY;
}


void _manage::pause(bool p)
{
	if(p == is_paused)
		return;

	is_paused = p;
	if(is_paused)
	{
		stop_screenshake();
		if(campaign && (replaymode == RPM_PLAY))
		{
			finalize_replay();
			campaign->save();
		}
	}
}


void _manage::background(bool bg)
{
	if(bg == in_background)
		return;

	in_background = bg;
	if(in_background)
	{
		if(replaymode == RPM_PLAY)
			switch(gamestate)
			{
			  case GS_GETREADY:
			  case GS_PLAYING:
			  case GS_LEVELDONE:
				pause(true);
				break;
			  default:
				break;
			}
	}
}


void _manage::lost_myship()
{
	state(GS_GAMEOVER);
	if(replaymode != RPM_PLAY)	// Handled by UI state when playing!
		delay_count = KOBO_GAMEOVER_TIMEOUT;
	log_printf(ULOG, "Player died at stage %d; score: %d, health: %d, "
			"charge: %d\n", selected_stage, score, myship.health(),
			myship.charge());
	if(replaymode == RPM_PLAY)
	{
		if(replay)
		{
			finalize_replay();
			++replay->deaths;
		}
		if(campaign)
			campaign->save();
	}
}


void _manage::destroyed_a_core()
{
	if(gamestate != GS_PLAYING)
		return;	// Can't win after death...

	// Award health bonus for destroyed core!
	myship.health_bonus(game.core_destroyed_health_bonus);
	remaining_cores--;
	if(remaining_cores == 0)
	{
		// Award extra health bonus for stage cleared!
		myship.health_bonus(game.stage_cleared_health_bonus);
		myship.state(SHIP_INVULNERABLE);

		// Don't try to leave if we're in rewind!
		if(replaymode != RPM_RETRY)
		{
			state(GS_LEVELDONE);
			delay_count = KOBO_LEVELDONE_TIMEOUT;
		}
	}
	screen.generate_fixed_enemies();
	screenshake(0.5f, 0.5f, 0.95f);
}


void _manage::add_score(int sc)
{
	score += sc;
	if(score >= GIGA)	//This *could* happen... Or maybe not. :-)
		score = GIGA - 1;
	else if(!prefs->cheats())
	{
		if(score >= highscore)
		{
			if(flash_score_count == 0)
				flash_score_count = 50;
		}
	}
	score_changed = true;
}


void _manage::reenter()
{
	screen.init_background();
	put_player_stats();
	put_info();
	put_score();
	set_bars();
}
