/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001-2003 David Olofson
 * Copyright 2002 Jeremy Sheeley
 * Copyright 2005-2007, 2009 David Olofson
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

#include "SDL.h"
#include <math.h>
#ifndef M_PI
# define M_PI 3.14159265358979323846	/* pi */
#endif

#include "kobolog.h"
#include "config.h"
#include "states.h"
#include "kobo.h"
#include "screen.h"
#include "manage.h"
#include "options.h"
#include "sound.h"
#include "radar.h"
#include "random.h"
#include "openurl.h"


gamestatemanager_t gsm;
int last_level = -1;


/*----------------------------------------------------------
	kobo_basestate_t
----------------------------------------------------------*/

kobo_basestate_t::kobo_basestate_t()
{
	name = "<unnamed>";
	info = NULL;
	song = -1;
}


void kobo_basestate_t::enter()
{
	if(!km.quitting() && (song >= 0))
		sound.music(song);
}


void kobo_basestate_t::reenter()
{
	if(!km.quitting() && (song >= 0))
		sound.music(song);
}


void kobo_basestate_t::pre_render()
{
	screen.render_background();
}


void kobo_basestate_t::post_render()
{
	// CRT noise, rewind/retry gray tint, lower curtains, GUI focus FX
	screen.render_fx();

	// Debug overlay
	if(prefs->debug)
	{
		int y = 1;

		// Game manager state
		woverlay->string(4, y, enumstr(manage.state()));
		y += woverlay->fontheight();

		// Arcade style title screen demo mode
		if(manage.demo())
			woverlay->string(4, y, "[DEMO]");
		y += woverlay->fontheight();

		// UI state stack
		gamestate_t *s = this;
		while(s)
		{
			if(s->info)
			{
				char buf[80];
				snprintf(buf, sizeof(buf), "%s - %s",
						s->name, s->info);
				woverlay->string(4, y, buf);
			}
			else
				woverlay->string(4, y, s->name);
			y += woverlay->fontheight();
			woverlay->font(B_SMALL_FONT);
			s = s->previous();
		}
	}
}

void kobo_basestate_t::transition_change(gamestate_t *to,
		KOBO_TransitionStyle trs)
{
	st_transition_delay.start_transition(trs);
	st_transition_delay.start_change(to);
}


void kobo_basestate_t::transition_push(gamestate_t *to,
		KOBO_TransitionStyle trs)
{
	st_transition_delay.start_transition(trs);
	st_transition_delay.start_push(to);
}


void kobo_basestate_t::transition_pop(KOBO_TransitionStyle trs)
{
	st_transition_delay.start_transition(trs);
	st_transition_delay.start_pop();
}


/*----------------------------------------------------------
	st_transition_delay_t
----------------------------------------------------------*/

st_transition_delay_t::st_transition_delay_t()
{
	name = "transition_delay";
}


void st_transition_delay_t::enter()
{
	timeout = SDL_GetTicks() + 10000;
}


void st_transition_delay_t::frame()
{
	bool tmo = SDL_TICKS_PASSED(SDL_GetTicks(), timeout);
	bool done = false;
	switch(trstyle)
	{
	  case KOBO_TRS_INSTANT:
	  case KOBO_TRS_FULLSCREEN_IN_ONLY:
	  case KOBO_TRS_GAME_NOISE:
		done = true;
		break;
	  case KOBO_TRS_FULLSCREEN_SLOW:
	  case KOBO_TRS_FULLSCREEN_FAST:
		done = !wdash->busy();
		break;
	  case KOBO_TRS_GAME_SLOW:
	  case KOBO_TRS_GAME_FAST:
		done = screen.curtains();
		break;
	}
	if(tmo || done)
	{
		if(tmo)
			log_printf(ELOG, "Timeout in transition_delay!\n");
		tail_pop();
		switch(trstyle)
		{
		  case KOBO_TRS_INSTANT:
		  case KOBO_TRS_GAME_NOISE:
		  case KOBO_TRS_FULLSCREEN_IN_ONLY:
		  case KOBO_TRS_FULLSCREEN_SLOW:
		  case KOBO_TRS_FULLSCREEN_FAST:
			break;
		  case KOBO_TRS_GAME_SLOW:
			screen.curtains(false,
					KOBO_ENTER_TITLE_FXTIME * 0.001f);
			break;
		  case KOBO_TRS_GAME_FAST:
			screen.curtains(false,
					KOBO_ENTER_STAGE_FXTIME * 0.001f);
			break;
		}
		if(to_state)
		{
			if(change_to)
				gsm.change(to_state);
			else
				gsm.push(to_state);
		}
		else
			gsm.pop();
	}
}


void st_transition_delay_t::pre_render()
{
	if(previous())
		previous()->pre_render();
	else
		kobo_basestate_t::pre_render();
}


void st_transition_delay_t::post_render()
{
	if(previous())
		previous()->post_render();
	else
		kobo_basestate_t::post_render();
}


void st_transition_delay_t::start_transition(KOBO_TransitionStyle trs)
{
	trstyle = trs;
	switch(trs)
	{
	  case KOBO_TRS_INSTANT:
		break;
	  case KOBO_TRS_FULLSCREEN_SLOW:
		break;
	  case KOBO_TRS_FULLSCREEN_FAST:
		break;
	  case KOBO_TRS_FULLSCREEN_IN_ONLY:
		break;
	  case KOBO_TRS_GAME_SLOW:
		screen.curtains(true, KOBO_ENTER_TITLE_FXTIME * 0.001f, true);
		break;
	  case KOBO_TRS_GAME_FAST:
		screen.curtains(true, KOBO_ENTER_STAGE_FXTIME * 0.001f, true);
		break;
	  case KOBO_TRS_GAME_NOISE:
		manage.noise_glitch();
		break;
	}
}


void st_transition_delay_t::start_change(gamestate_t *to)
{
	to_state = to;
	change_to = true;
	gsm.tail_push(this);
}


void st_transition_delay_t::start_push(gamestate_t *to)
{
	to_state = to;
	change_to = false;
	gsm.tail_push(this);
}


void st_transition_delay_t::start_pop()
{
	to_state = NULL;
	gsm.tail_push(this);
}


st_transition_delay_t st_transition_delay;


/*----------------------------------------------------------
	st_intro
----------------------------------------------------------*/

int kobo_intro_durations[KOBO_IP__COUNT] = 
{
	INTRO_TITLE_TIME,		// KOBO_IP_TITLE
	INTRO_INSTRUCTIONS_TIME,	// KOBO_IP_INSTRUCTIONS
	INTRO_TITLE2_TIME,		// KOBO_IP_TITLE2
	INTRO_CREDITS_TIME		// KOBO_IP_CREDITS
};


st_intro_t::st_intro_t()
{
	name = "intro";
	page = KOBO_IP_TITLE;
	duration = 0;
	timer = 0;
	song = S_TITLESONG;
	boot = true;
}


void st_intro_t::init_page()
{
	start_time = (int)SDL_GetTicks() + INTRO_BLANK_TIME;
	timer = 0;
	if(page >= KOBO_IP__COUNT)
		page = 0;
	else if(page < 0)
		page = KOBO_IP__COUNT - 1;
	duration = kobo_intro_durations[page];
}


void st_intro_t::enter()
{
	kobo_basestate_t::enter();
	// Instant transition to demo/title mode if we're already hidden
	if(screen.curtains() || wdash->closed() || boot)
	{
		manage.show_demo(true, true);
		boot = false;
	}
	else
		manage.show_demo();
	if(!km.quitting())
		sound.music(song);
	init_page();
	screen.set_highlight(0, 0);
}


void st_intro_t::yield()
{
	kobo_basestate_t::yield();
	screen.set_highlight(0, 0);
}


void st_intro_t::reenter()
{
	kobo_basestate_t::reenter();
	enter();
}


void st_intro_t::leave()
{
	kobo_basestate_t::leave();
	screen.set_highlight(0, 0);
}


void st_intro_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_MENU:
	  case BTN_CLOSE:
	  case BTN_PRIMARY:
	  case BTN_SECONDARY:
	  case BTN_TERTIARY:
	  case BTN_SELECT:
	  case BTN_LMB:
	  case BTN_MMB:
	  case BTN_RMB:
		gsm.push(&st_main_menu);
		break;
	  case BTN_UP:
	  case BTN_LEFT:
		--page;
		init_page();
		break;
	  case BTN_DOWN:
	  case BTN_RIGHT:
		++page;
		init_page();
		break;
	  default:
		break;
	}
}


void st_intro_t::frame()
{
	if(timer > duration)
	{
		++page;
		init_page();
	}
}


void st_intro_t::pre_render()
{
	kobo_basestate_t::pre_render();
	timer = (int)SDL_GetTicks() - start_time;
}


void st_intro_t::post_render()
{
	kobo_basestate_t::post_render();
	if((timer >= 0) && (timer < duration))
	{
		switch((KOBO_IntroPages)page)
		{
		  case KOBO_IP__COUNT:
		  case KOBO_IP_TITLE:
		  case KOBO_IP_TITLE2:
		  {
			float nt = (float)timer / duration;
			float snt = 1.0f - sin(nt * M_PI);
			snt = 1.0f - snt * snt * snt;
			screen.title(timer, snt);
			break;
		  }
		  case KOBO_IP_INSTRUCTIONS:
		  {
			screen.help(timer);
			break;
		  }
		  case KOBO_IP_CREDITS:
		  {
			screen.credits(timer);
			break;
		  }
		}
	}
	else
	{
		screen.set_highlight(0, 0);
		screen.set_fade(0.0f);
	}
}


st_intro_t st_intro;


/*----------------------------------------------------------
	st_long_credits
----------------------------------------------------------*/

st_long_credits_t::st_long_credits_t()
{
	name = "long_credits";
	timer = 0;
	song = S_EPILOGUE;
}


void st_long_credits_t::enter()
{
	kobo_basestate_t::enter();
	manage.show_stage(KOBO_CREDITS_BACKGROUND_LEVEL, GS_TITLE);
	start_time = (int)SDL_GetTicks() + INTRO_BLANK_TIME;
	timer = 0;
}


void st_long_credits_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_MENU:
	  case BTN_CLOSE:
	  case BTN_PRIMARY:
	  case BTN_SECONDARY:
	  case BTN_TERTIARY:
	  case BTN_SELECT:
	  case BTN_LMB:
	  case BTN_MMB:
	  case BTN_RMB:
		transition_pop(KOBO_TRS_GAME_SLOW);
		break;
	  default:
		break;
	}
}


void st_long_credits_t::pre_render()
{
	kobo_basestate_t::pre_render();
	timer = (int)SDL_GetTicks() - start_time;
}


void st_long_credits_t::post_render()
{
	kobo_basestate_t::post_render();
	screen.long_credits(timer);
}


st_long_credits_t st_long_credits;


/*----------------------------------------------------------
	st_game
----------------------------------------------------------*/

st_game_t::st_game_t()
{
	name = "game";
	g_slot = 0;
	g_skill = KOBO_DEFAULT_SKILL;
}


void st_game_t::enter()
{
	if(!manage.game_in_progress())
	{
		manage.start_new_game(g_slot, g_stage, g_skill);
	}
	else
	{
		g_slot = manage.current_slot();
		g_stage = manage.current_stage();
		g_skill = manage.current_skill();
	}
#ifdef KOBO_DEMO
	if(manage.current_stage() > KOBO_DEMO_LAST_STAGE)
	{
		gsm.change(&st_demo_over);
		return;
	}
#endif
	if(!manage.game_in_progress())
	{
		st_error.message("INTERNAL ERROR", "Could not start game!");
		gsm.change(&st_error);
		return;
	}
	if(prefs->mousecapture)
		if(!SDL_GetRelativeMouseMode())
			SDL_SetRelativeMouseMode(SDL_TRUE);
	manage.background(false);
	manage.pause(false);
	st_intro.set_page(KOBO_IP_TITLE);
}


void st_game_t::leave()
{
	if(SDL_GetRelativeMouseMode())
		SDL_SetRelativeMouseMode(SDL_FALSE);
	manage.background(true);
}


void st_game_t::yield()
{
	if(SDL_GetRelativeMouseMode())
		SDL_SetRelativeMouseMode(SDL_FALSE);
	manage.background(true);
}


void st_game_t::reenter()
{
	manage.pause(false);
	if(!manage.game_in_progress())
	{
		pop();	// Game aborted by st_ask_abort_game, most likely!
		return;
	}
	manage.background(false);
	if(prefs->mousecapture)
		if(!SDL_GetRelativeMouseMode())
			SDL_SetRelativeMouseMode(SDL_TRUE);
}


void st_game_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_SELECT:
	  case BTN_MENU:
		gsm.push(&st_main_menu);
		break;
	  case BTN_CLOSE:
		gsm.push(&st_ask_exit);
		break;
	  case BTN_PAUSE:
		sound.ui_play(S_UI_PAUSE);
		gsm.push(&st_pause_game);
		break;
	  default:
		break;
	}
}


void st_game_t::frame()
{
	// Check mouse capture state here, as video restarts will reset it!
	if(prefs->mousecapture)
		if(!SDL_GetRelativeMouseMode())
			SDL_SetRelativeMouseMode(SDL_TRUE);
	switch(manage.state())
	{
	  case GS_GETREADY:
		gsm.push(&st_get_ready);
		return;
	  case GS_GAMEOVER:
		st_intro.set_page(KOBO_IP_INSTRUCTIONS);
		gsm.change(&st_game_over);
		return;
	  case GS_NONE:
		transition_pop(KOBO_TRS_GAME_SLOW);
		return;
	  default:
		last_level = manage.current_stage();
		break;
	}
	if(km.quitting())
		pop();
}


void st_game_t::post_render()
{
	kobo_basestate_t::post_render();
	wradar->frame();
}


st_game_t st_game;


/*----------------------------------------------------------
	st_rewind
----------------------------------------------------------*/

st_rewind_t::st_rewind_t()
{
	name = "rewind";
}


void st_rewind_t::enter()
{
#ifdef KOBO_DEMO
	if(manage.current_stage() > KOBO_DEMO_LAST_STAGE)
	{
		gsm.change(&st_demo_over);
		return;
	}
#endif
	if(!manage.game_in_progress())
	{
		st_error.message("INTERNAL ERROR",
				"Entered st_rewind with no game in progress!");
		gsm.change(&st_error);
		return;
	}
	if(manage.state() == GS_GETREADY)
	{
		// Too short replay, probably! Switch directly to st_game.
		gsm.change(&st_game);
		return;
	}
	manage.background(false);
}


void st_rewind_t::leave()
{
	st_intro.set_page(KOBO_IP_TITLE);
	manage.background(true);
}


void st_rewind_t::yield()
{
	manage.background(true);
}


void st_rewind_t::reenter()
{
	if(!manage.game_in_progress())
	{
		pop();	// Game aborted by st_ask_abort_game, most likely!
		return;
	}
	manage.background(false);
}


void st_rewind_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_MENU:
	  case BTN_RMB:
	  case BTN_MMB:
		gsm.push(&st_main_menu);
		break;
	  case BTN_CLOSE:
		gsm.push(&st_ask_exit);
		break;
	  case BTN_SELECT:
	  case BTN_LMB:
	  case BTN_PAUSE:
		if(manage.paused())
			sound.ui_play(S_UI_PLAY);
		else
			sound.ui_play(S_UI_PAUSE);
		manage.pause(!manage.paused());
		break;
	  default:
		break;
	}
}


void st_rewind_t::frame()
{
	if((manage.replay_mode() != RPM_RETRY) &&
			(manage.state() == GS_PLAYING ||
			manage.state() == GS_GETREADY))
		gsm.change(&st_game);
	switch(manage.state())
	{
	  case GS_NONE:
		transition_pop(KOBO_TRS_GAME_FAST);
		return;
	  default:
		break;
	}
	if(km.quitting())
		pop();
}


void st_rewind_t::post_render()
{
	kobo_basestate_t::post_render();

	// "Timeout" progress bar
	woverlay->font(B_NORMAL_FONT);
	float p = manage.replay_progress();
	float px = p * p;
	px *= px;
	int p1 = (int)(px * 127.0f);
	int p2 = (int)(px * 192.0f);

	// Played
	woverlay->alphamod(64);
	woverlay->foreground(woverlay->map_rgb(128 + p1, 192 - p2, 192 - p2));
	woverlay->fillrect_fxp(0, PIXEL2CS(300),
			PIXEL2CS((int)(woverlay->width() * p) - 1),
			PIXEL2CS(woverlay->fontheight() + 1));

	// Remaining
	woverlay->foreground(woverlay->map_rgb(0, 0, 0));
	woverlay->fillrect_fxp(PIXEL2CS((int)(woverlay->width() * p)),
			PIXEL2CS(300),
			PIXEL2CS(woverlay->width()),
			PIXEL2CS(woverlay->fontheight() + 1));

	// "Bookmarks"
	woverlay->foreground(woverlay->map_rgb(128, 128, 128));
	for(int i = 0; ; ++i)
	{
		int t = manage.bookmark(i);
		if(t >= (int)manage.replay_duration())
			break;
		int x = woverlay->width() * t / manage.replay_duration();
		woverlay->fillrect_fxp(PIXEL2CS(x), PIXEL2CS(300),
				PIXEL2CS(1),
				PIXEL2CS(woverlay->fontheight() + 1));
	}

	// Position
	woverlay->alphamod(128);
	woverlay->foreground(woverlay->map_rgb(255, 255, 255));
	woverlay->fillrect_fxp(PIXEL2CS((int)(woverlay->width() * p) - 1),
			PIXEL2CS(300), PIXEL2CS(1),
			PIXEL2CS(woverlay->fontheight() + 1));

	// Label
	woverlay->alphamod(255);
	if(SDL_GetTicks() & 0x300)
		woverlay->center_fxp(PIXEL2CS(301), manage.paused() ?
				"[ REPLAY (PAUSED) ]" : "[ REPLAY ]");

	// "Take over" instructions
	woverlay->font(B_SMALL_FONT);
	woverlay->center_fxp(PIXEL2CS(315),
			"Stick controls speed.   Fire to take over!");

	wradar->frame();
}


st_rewind_t st_rewind;


/*----------------------------------------------------------
	st_replay
----------------------------------------------------------*/

st_replay_t::st_replay_t()
{
	name = "replay";
	rp_slot = 0;
	rp_stage = 1;
}


void st_replay_t::enter()
{
	if(!manage.start_replay(rp_slot, rp_stage))
	{
		sound.ui_play(S_UI_ERROR);
		st_error.message("Campaign Replay Problem!",
				"Could not load or start replay.");
		gsm.change(&st_error);
		return;
	}
	manage.background(false);
}


void st_replay_t::leave()
{
	st_intro.set_page(KOBO_IP_TITLE);
	manage.abort_game();
	manage.background(true);
}


void st_replay_t::yield()
{
	manage.background(true);
}


void st_replay_t::reenter()
{
	manage.background(false);
}


void st_replay_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_MENU:
	  case BTN_MMB:
	  case BTN_RMB:
	  case BTN_PRIMARY:
		gsm.push(&st_main_menu);
		break;
	  case BTN_CLOSE:
		gsm.push(&st_ask_exit);
		break;
	  case BTN_SELECT:
	  case BTN_PAUSE:
	  case BTN_SECONDARY:
	  case BTN_TERTIARY:
	  case BTN_LMB:
		if(manage.paused())
			sound.ui_play(S_UI_PLAY);
		else
			sound.ui_play(S_UI_PAUSE);
		manage.pause(!manage.paused());
		break;
	  default:
		break;
	}
}


void st_replay_t::frame()
{
	if(km.quitting())
		pop();
	switch(manage.state())
	{
	  case GS_GAMEOVER:
	  case GS_REPLAYEND:
		if(prefs->loopreplays)
			break;
		if(manage.time_remaining() > KOBO_ENTER_TITLE_FXTIME)
			return;
		// Fall-through
	  case GS_NONE:
		transition_pop(KOBO_TRS_GAME_SLOW);
		return;
	  default:
		break;
	}
}


void st_replay_t::post_render()
{
	kobo_basestate_t::post_render();

	// Thin progress bar
	woverlay->font(B_SMALL_FONT);
	float p = manage.replay_progress();
	woverlay->foreground(woverlay->map_rgb(64, 96, 96));
	woverlay->alphamod(64);
	woverlay->fillrect_fxp(0, PIXEL2CS(300),
			PIXEL2CS((int)(woverlay->width() * p)),
			PIXEL2CS(woverlay->fontheight() + 1));
	woverlay->alphamod(128);
	if(SDL_GetTicks() & 0x300)
	{
		char buf[128];
		snprintf(buf, sizeof(buf), "[ FULL REPLAY - %d/%d %s]",
				manage.current_stage(), manage.replay_stages(),
				manage.paused() ? "- (PAUSED) " : "");
		woverlay->center_fxp(PIXEL2CS(301), buf);
	}

	// Replay control instructions
	woverlay->center_fxp(PIXEL2CS(315),
			"Stick controls speed, and skips stages.");
	woverlay->alphamod(255);

	wradar->frame();
}


st_replay_t st_replay;


/*----------------------------------------------------------
	st_pause_game
----------------------------------------------------------*/

st_pause_game_t::st_pause_game_t()
{
	name = "pause_game";
}


void st_pause_game_t::enter()
{
	// We should only ever pause if there's an actual game in progress
	if((manage.state() != GS_PLAYING) ||
			(manage.replay_mode() != RPM_PLAY) || manage.demo())
		pop();
}


void st_pause_game_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_MENU:
	  case BTN_MMB:
	  case BTN_RMB:
		gsm.change(&st_main_menu);
		break;
	  default:
		sound.ui_play(S_UI_PLAY);
		pop();
		break;
	}
}


void st_pause_game_t::frame()
{
}


void st_pause_game_t::post_render()
{
	kobo_basestate_t::post_render();

	float ft = SDL_GetTicks() * 0.001;
	woverlay->font(B_BIG_FONT);
	int y = PIXEL2CS(75) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
	woverlay->center_fxp(y, "PAUSED");

	wradar->frame();
}


st_pause_game_t st_pause_game;



/*----------------------------------------------------------
	st_get_ready
----------------------------------------------------------*/

st_get_ready_t::st_get_ready_t()
{
	name = "get_ready";
}


void st_get_ready_t::enter()
{
	sound.ui_play(S_UI_READY);
	reenter();
}


void st_get_ready_t::reenter()
{
	start_time = (int)SDL_GetTicks();
	frame_time = 0;
	countdown = prefs->countdown;
}


void st_get_ready_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_MENU:
	  case BTN_RMB:
	  case BTN_NO:
		gsm.change(&st_main_menu);
		break;
	  case BTN_LEFT:
	  case BTN_RIGHT:
	  case BTN_UP:
	  case BTN_DOWN:
	  case BTN_UL:
	  case BTN_UR:
	  case BTN_DL:
	  case BTN_DR:
	  case BTN_PRIMARY:
	  case BTN_SECONDARY:
	  case BTN_TERTIARY:
	  case BTN_LMB:
	  case BTN_YES:
		if(frame_time < 500)
			break;
		sound.ui_play(S_UI_PLAY);
		manage.pause(false);
		manage.player_ready();
		pop();
		break;
	  case BTN_SELECT:
	  case BTN_MMB:
	  case BTN_PAUSE:
		sound.ui_play(S_UI_PAUSE);
		manage.player_ready();
		gsm.change(&st_pause_game);
		break;
	  default:
		break;
	}
}


void st_get_ready_t::frame()
{
	if(km.quitting() || !manage.game_in_progress())
	{
		pop();
		return;
	}

	frame_time = (int)SDL_GetTicks() - start_time;
	if(!prefs->countdown)
	{
		if(frame_time > 700)
		{
			sound.ui_play(S_UI_PLAY);
			manage.pause(false);
			manage.player_ready();
			pop();
		}
	}
	else if(prefs->countdown <= 9)
	{
		int prevcount = countdown;
		countdown = prefs->countdown - frame_time / 1000;
		if(prevcount != countdown)
			sound.ui_countdown(countdown);

		if(countdown < 1)
		{
			sound.ui_play(S_UI_PLAY);
			manage.pause(false);
			manage.player_ready();
			pop();
		}
	}
}


void st_get_ready_t::post_render()
{
	kobo_basestate_t::post_render();

	float ft = SDL_GetTicks() * 0.001;
	woverlay->font(B_BIG_FONT);
	int y = PIXEL2CS(70) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
	woverlay->center_fxp(y, "GET READY!");
	screen.render_countdown(y + PIXEL2CS(60),
			(int)SDL_GetTicks() - start_time,
			prefs->countdown, countdown);

	wradar->frame();
}


st_get_ready_t st_get_ready;


/*----------------------------------------------------------
	st_game_over
----------------------------------------------------------*/

st_game_over_t::st_game_over_t()
{
	name = "game_over";
}


void st_game_over_t::enter()
{
	sound.ui_play(S_UI_GAMEOVER);
	reenter();
}


void st_game_over_t::reenter()
{
	if(!manage.game_in_progress())
	{
		pop();
		return;
	}
	start_time = (int)SDL_GetTicks();
	frame_time = 0;
	if(prefs->autocontinue)
		countdown = prefs->autocontinue;
	else
		countdown = prefs->cont_countdown;
}


void st_game_over_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_MENU:
	  case BTN_MMB:
	  case BTN_RMB:
	  case BTN_NO:
		gsm.push(&st_main_menu);
		break;
	  case BTN_LEFT:
	  case BTN_RIGHT:
	  case BTN_UP:
	  case BTN_DOWN:
	  case BTN_UL:
	  case BTN_UR:
	  case BTN_DL:
	  case BTN_DR:
		// We don't want these in autocontinue mode.
		if(prefs->autocontinue)
			break;
	  case BTN_PRIMARY:
	  case BTN_SECONDARY:
	  case BTN_TERTIARY:
	  case BTN_SELECT:
	  case BTN_LMB:
	  case BTN_YES:
		if(prefs->autocontinue)
		{
			if(frame_time < prefs->autocontinue * 500)
				break;
		}
		else if(frame_time < 500)
			break;
		sound.ui_play(S_UI_OK);
		manage.rewind();
		gsm.change(&st_rewind);
	  default:
		break;
	}
}


void st_game_over_t::frame()
{
	frame_time = (int)SDL_GetTicks() - start_time;
	if(prefs->autocontinue)
	{
		if(prefs->autocontinue <= 9)
		{
			countdown = prefs->autocontinue - frame_time / 1000;
			if(countdown < 1)
			{
				sound.ui_play(S_UI_OK);
				manage.rewind();
				gsm.change(&st_rewind);
			}
		}
	}
	else if(!prefs->cont_countdown)
	{
		if(frame_time > 700)
		{
			sound.ui_play(S_UI_OK);
			manage.rewind();
			gsm.change(&st_rewind);
		}
	}
	else if(prefs->cont_countdown <= 9)
	{
		int prevcount = countdown;
		countdown = prefs->cont_countdown - frame_time / 1000;
		if(prevcount != countdown)
			sound.ui_countdown(countdown);
		if(countdown < 1)
		{
			sound.ui_play(S_UI_CANCEL);
			manage.abort_game();
			pop();
		}
	}
}


void st_game_over_t::post_render()
{
	kobo_basestate_t::post_render();

	if(!prefs->autocontinue)
	{
		float ft = SDL_GetTicks() * 0.001;
		woverlay->font(B_BIG_FONT);
		int y = PIXEL2CS(100) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
		woverlay->center_fxp(y, "CONTINUE?");
		screen.render_countdown(y + PIXEL2CS(60),
				(int)SDL_GetTicks() - start_time,
				prefs->cont_countdown, countdown);
	}

	wradar->frame();
}


st_game_over_t st_game_over;


/*----------------------------------------------------------
	Menu Base
----------------------------------------------------------*/

/*
 * menu_base_t
 */
void menu_base_t::open()
{
	place(woverlay->px(), woverlay->py(),
			woverlay->width(), woverlay->height());
	font(B_NORMAL_FONT);
	foreground(woverlay->map_rgb(0xffffff));
	background(woverlay->map_rgb(0x000000));
	build_all();
}


void menu_base_t::close()
{
	clean();
}


/*
 * st_menu_base_t
 */

st_menu_base_t::st_menu_base_t()
{
	name = "(menu_base derivate)";
	sounds = 1;
	form = NULL;
}


st_menu_base_t::~st_menu_base_t()
{
	delete form;
}


void st_menu_base_t::enter()
{
	form = open();
	if(sounds)
		sound.ui_play(S_UI_OPEN);

	// If the mouse cursor is visible when entering a menu, and it happens
	// to land on an interactive widget, we want that to be selected.
	if(mouse_visible)
		gsm.pos(mouse_x, mouse_y);
}


void st_menu_base_t::rebuild()
{
	form->rebuild();
}


void st_menu_base_t::leave()
{
	screen.set_highlight(0, 0);
	close();
	delete form;
	form = NULL;
}


void st_menu_base_t::frame()
{
}


void st_menu_base_t::post_render()
{
	kobo_basestate_t::post_render();
	if(form)
		form->render();
	if(manage.state() != GS_SHOW)
		wradar->frame();
}


int st_menu_base_t::translate(int tag, int button)
{
	switch(button)
	{
	  case BTN_INC:
	  case BTN_RIGHT:
	  case BTN_DEC:
	  case BTN_LEFT:
		return -1;
	  default:
		return tag ? tag : -1;
	}
}


void st_menu_base_t::pos(int x, int y)
{
	if(form)
		form->select(x, y);
}


void st_menu_base_t::press(gc_targets_t button)
{
	int selection;
	if(!form)
		return;

	do_default_action = 1;

	// Translate
	switch(button)
	{
	  case BTN_MENU:
	  case BTN_CLOSE:
		selection = 0;
		break;
	  case BTN_UP:
	  case BTN_DOWN:
	  case BTN_MMB:
		selection = -1;
		break;
	  case BTN_INC:
	  case BTN_RIGHT:
	  case BTN_DEC:
	  case BTN_LEFT:
	  case BTN_PRIMARY:
	  case BTN_SECONDARY:
	  case BTN_TERTIARY:
	  case BTN_SELECT:
	  case BTN_LMB:
		if(form->selected())
			selection = translate(form->selected()->tag, button);
		else
			selection = -2;
		break;
	  default:
		selection = -1;
		break;
	}

	// Default action
	if(do_default_action)
		switch(button)
		{
		  case BTN_MENU:
		  case BTN_RMB:
			escape();
			break;
		  case BTN_INC:
		  case BTN_RIGHT:
			form->change(1);
			break;
		  case BTN_DEC:
		  case BTN_LEFT:
		  case BTN_SECONDARY:
		  case BTN_TERTIARY:
			form->change(-1);
			break;
		  case BTN_PRIMARY:
		  case BTN_SELECT:
		  case BTN_LMB:
		  case BTN_MMB:
			form->change(0);
			break;
		  case BTN_UP:
			form->prev();
			break;
		  case BTN_DOWN:
			form->next();
			break;
		  default:
			break;
		}

	switch(selection)
	{
	  case -1:
		break;
	  case 0:
		if(sounds)
			sound.ui_play(S_UI_CANCEL);
		select(0);
		pop();
		break;
	  default:
		select(selection);
		break;
	}
}


/*----------------------------------------------------------
	st_error
----------------------------------------------------------*/

void st_error_t::message(const char *error, const char *hint)
{
	msg[0] = error;
	msg[1] = hint;
}


st_error_t::st_error_t()
{
	name = "error";
	msg[0] = "No error!";
	msg[1] = "(Why am I here...?)";
}


void st_error_t::enter()
{
	sound.ui_play(S_UI_ERROR);
	start_time = (int)SDL_GetTicks();
}


void st_error_t::press(gc_targets_t button)
{
	if(frame_time < 500)
		return;

	switch (button)
	{
	  case BTN_NO:
	  case BTN_LEFT:
	  case BTN_RIGHT:
	  case BTN_UP:
	  case BTN_DOWN:
	  case BTN_UL:
	  case BTN_UR:
	  case BTN_DL:
	  case BTN_DR:
	  case BTN_PRIMARY:
	  case BTN_SECONDARY:
	  case BTN_TERTIARY:
	  case BTN_SELECT:
	  case BTN_LMB:
	  case BTN_YES:
		sound.ui_play(S_UI_OK);
		pop();
		break;
	  default:
		break;
	}
}


void st_error_t::frame()
{
	frame_time = (int)SDL_GetTicks() - start_time;
#if 0
	if(frame_time % 1000 < 500)
		sound.ui_play(S_UI_ERROR);
#endif
}


void st_error_t::post_render()
{
	kobo_basestate_t::post_render();

	woverlay->font(B_BIG_FONT);
	woverlay->center(95, msg[0]);

	woverlay->font(B_NORMAL_FONT);
	woverlay->center(123, msg[1]);

	if(frame_time % 1000 < 500)
	{
		int w = woverlay->width();
		int h = woverlay->height();
		woverlay->foreground(woverlay->map_rgb(0xff0000));
		woverlay->rectangle(20, 80, w - 40, h - 160);
		woverlay->rectangle(22, 82, w - 44, h - 164);
		woverlay->rectangle(24, 84, w - 48, h - 168);
	}
}


st_error_t st_error;


/*----------------------------------------------------------
	st_main_menu
----------------------------------------------------------*/

void main_menu_t::buildStartLevel()
{
	char buf[50];
	int MaxStartLevel = 500;
	start_level = manage.current_stage();
	if(start_level > MaxStartLevel)
		start_level = MaxStartLevel;
	halign = ALIGN_CENTER;
	list("Start at Stage", &start_level, 5);
	halign = ALIGN_DEFAULT;
	for(int i = 1; i <= MaxStartLevel; ++i)
	{
		snprintf(buf, sizeof(buf), "%d", i);
		item(buf, i);
	}
}


void main_menu_t::build()
{
	int sc = manage.last_played_stage();
	if(sc < 1)
		sc = 1;
	if(prefs->cheat_startlevel && manage.ok_to_switch())
		manage.show_stage(sc, GS_SHOW);
	st_game.set_stage(sc);

	space(2);
	if(manage.game_in_progress())
		button("Return to Game", MENU_TAG_OK);
	else if((manage.replay_mode() == RPM_REPLAY) && !manage.demo())
		button("Return to Replay", MENU_TAG_OK);
	else
	{
		if(savemanager.exists(-1))
		{
			button("Continue Campaign", 51);
			space();
		}

		button("New Campaign", 1);
		if(prefs->cheat_startlevel)
			buildStartLevel();

		if(savemanager.exists(-1))
		{
			space();
			button("View Replay", 60);
		}
	}
#ifdef KOBO_DEMO
	space(2);
	button("Buy Kobo Redux", 40);
	label("https://olofson.itch.io/kobo-redux");
#endif
	space(2);
#if 0
	button("Showcase", 20);
#endif
	button("Options", 2);
	space(2);
	if(manage.game_in_progress())
		button("Abort Current Game", 101);
	else if((manage.replay_mode() == RPM_REPLAY) && !manage.demo())
		button("Stop Replay", 102);
	else
	{
		button("Credits & Thanks", 30);
		space(2);
		button("Return to Intro", MENU_TAG_OK);
	}
	button("Quit Kobo Redux", MENU_TAG_CANCEL);
}


void main_menu_t::rebuild()
{
	int sel = selected_index();
	build_all();
	select(sel);
}


kobo_form_t *st_main_menu_t::open()
{
	menu = new main_menu_t(gengine);
	menu->open();
	return menu;
}


void st_main_menu_t::reenter()
{
	menu->rebuild();
	st_menu_base_t::reenter();
}


void st_main_menu_t::press(gc_targets_t button)
{
	switch(button)
	{
	  case BTN_MENU:
	  case BTN_MMB:
	  case BTN_RMB:
	  case BTN_CLOSE:
		// We want to treat these the same as "Return to game"
		select(MENU_TAG_OK);
	  default:
		st_menu_base_t::press(button);
		return;
	}
}


// Custom translator to map inc/dec on certain widgets
int st_main_menu_t::translate(int tag, int button)
{
	switch(tag)
	{
	  case 5:
		return tag;
	  default:
		return st_menu_base_t::translate(tag, button);
	}
}


void st_main_menu_t::select(int tag)
{
	switch(tag)
	{
	  case 1:
		st_campaign_menu.setup("Select Save Slot for Campaign", true);
		gsm.change(&st_campaign_menu);
		break;
	  case 2:
		gsm.change(&st_options_main);
		break;
	  case 5:	// Start level: Inc/Dec
		sound.ui_play(S_UI_TICK);
		manage.show_stage(menu->start_level, GS_SHOW);
		st_game.set_stage(menu->start_level);
		break;
	  case 20:	// Showcase
		sound.ui_play(S_UI_ERROR);
		st_error.message("Showcase not implemented!",
				"Here, you'll find information on\n"
				"enemies and other objects of\n"
				"interest seen in the game.");
		gsm.push(&st_error);
		break;
	  case 30:	// Credits & Thanks
		transition_change(&st_long_credits, KOBO_TRS_GAME_SLOW);
		break;
	  case 40:	// Home site link
		kobo_OpenURL("https://olofson.itch.io/kobo-redux");
		break;
	  case 51:	// Continue Campaign
		st_campaign_menu.setup("Select Campaign to Continue", false);
		gsm.change(&st_campaign_menu);
		break;
	  case 60:	// View Replay
		st_campaign_menu.setup("Select Campaign to View", false, true);
		gsm.change(&st_campaign_menu);
		break;
	  case 101:
		gsm.change(&st_ask_abort_game);
		break;
	  case 102:
		pop();
		transition_pop(KOBO_TRS_GAME_SLOW);
		break;
	  case MENU_TAG_OK:
		gsm.change(&st_pause_game);
		break;
	  case MENU_TAG_CANCEL:
		gsm.change(&st_ask_exit);
		break;
	}
}


st_main_menu_t st_main_menu;


/*----------------------------------------------------------
	Campaign Selector
----------------------------------------------------------*/

campaign_menu_t::campaign_menu_t(gfxengine_t *e) : menu_base_t(e)
{
	header = NULL;
	newgame = false;
	view_replay = false;
	selected_slot = 1;
}


void campaign_menu_t::setup(const char *hdr, bool new_game, bool replay)
{
	header = hdr;
	newgame = new_game;
	view_replay = replay;
	selected_slot = 1;
	savemanager.load(-1);
	savemanager.analysis(-1, true);

	// Timestamp reset, because loading might take a few frames.
	// Or much longer than that, if debug log output is enabled!
	sound.timestamp_reset();
}


const char *campaign_menu_t::timedate(time_t *t)
{
	const struct tm *lt = localtime(t);
	if(!lt)
		return "<cannot decode timestamp>";
	struct tm tmp = *lt;
	if(!strftime(tdbuf, sizeof(tdbuf), "%a %b %d %H:%M:%S %Y", &tmp))
		return "<strftime() failed>";
	return tdbuf;
}


void campaign_menu_t::colonalign()
{
	current_widget->token(':');
	current_widget->halign(ALIGN_CENTER_TOKEN);
}


void campaign_menu_t::build()
{
	title(header);
	button("Return to Intro", MENU_TAG_OK);
	space(1);
	xoffs = 0.1;
	for(int i = 0; i < KOBO_MAX_CAMPAIGN_SLOTS; ++i)
	{
		if((view_replay || !newgame) && !savemanager.exists(i))
			continue;

		char buf[128];
		KOBO_campaign_info *ci = savemanager.analysis(i);
		if(ci)
			snprintf(buf, sizeof(buf), "%d: %s", i,
					timedate(&ci->starttime));
		else
			snprintf(buf, sizeof(buf), "%d: <empty slot>", i);
		button(buf, i + 10);
		colonalign();
		space(.25);
	}
	space(1);

	xoffs = 0.3;
	KOBO_campaign_info *ci = savemanager.analysis(selected_slot);
	if(ci)
	{
		char buf[128];

		snprintf(buf, sizeof(buf), "Start: %s",
				timedate(&ci->starttime));
		label(buf);
		colonalign();

		snprintf(buf, sizeof(buf), "End: %s",
				timedate(&ci->endtime));
		label(buf);
		colonalign();

		int s = ci->stage;
		snprintf(buf, sizeof(buf), "Progress: Region %d/%d "
				"Level %d/%d (Stage %d)",
				(s - 1) / 10 % 5 + 1, KOBO_REGIONS,
				(s - 1) % 10 + 1, KOBO_LEVELS_PER_REGION,
				s);
		label(buf);
		colonalign();

		snprintf(buf, sizeof(buf), "Score: %d", ci->score);
		label(buf);
		colonalign();

		snprintf(buf, sizeof(buf), "Health: %d", ci->health);
		label(buf);
		colonalign();

		snprintf(buf, sizeof(buf), "Charge: %d", ci->charge);
		label(buf);
		colonalign();
	}
}


void campaign_menu_t::rebuild()
{
	int sel = selected_index();
	build_all();
	select(sel);
}


bool campaign_menu_t::select(ct_widget_t *w)
{
	if(kobo_form_t::select(w))
	{
		if(!selected())
			return true;
		int slot = selected()->tag - 10;
		if((slot >= 0) && (slot < KOBO_MAX_CAMPAIGN_SLOTS) &&
				(slot != selected_slot))
		{
			selected_slot = slot;
			rebuild();
		}
		return true;
	}
	return false;
}


kobo_form_t *st_campaign_menu_t::open()
{
	menu = new campaign_menu_t(gengine);
	menu->open();
	menu->setup(header, newgame, view_replay);
	menu->rebuild();
	return menu;
}


void st_campaign_menu_t::setup(const char *hdr, bool new_game, bool replay)
{
	header = hdr;
	newgame = new_game;
	view_replay = replay;
}


void st_campaign_menu_t::press(gc_targets_t button)
{
	st_menu_base_t::press(button);
	switch (button)
	{
	  case BTN_UP:
	  case BTN_DOWN:
	  {
		int slot = menu->selected()->tag - 10;
		if((slot >= 0) && (slot < KOBO_MAX_CAMPAIGN_SLOTS))
		{
			menu->selected_slot = slot;
			menu->rebuild();
		}
		break;
	  }
	  case BTN_MMB:
	  case BTN_RMB:
		select(MENU_TAG_OK);
		break;
	  default:
		break;
	}
}


void st_campaign_menu_t::select(int tag)
{
	if(tag == MENU_TAG_OK)
	{
		sound.ui_play(S_UI_CANCEL);
		transition_pop(KOBO_TRS_GAME_FAST);
		return;
	}
	int slot = tag - 10;
	if((slot >= 0) && (slot < KOBO_MAX_CAMPAIGN_SLOTS))
	{
		menu->selected_slot = slot;
		if(view_replay)
		{
			st_replay.setup(slot, 1);
			transition_change(&st_replay, KOBO_TRS_GAME_FAST);
		}
		else if(newgame)
		{
			st_game.set_slot(slot);
			if(savemanager.exists(slot))
				gsm.change(&st_ask_overwrite_campaign);
			else
				gsm.change(&st_skill_menu);
		}
		else
		{
			if(manage.continue_game(slot))
				gsm.change(&st_rewind);
			else
			{
				sound.ui_play(S_UI_ERROR);
				st_error.message("Campaign Problem!",
					"Could not continue Campaign.");
				gsm.push(&st_error);
			}
		}
	}
}


st_campaign_menu_t st_campaign_menu;


/*----------------------------------------------------------
	st_skill_menu
----------------------------------------------------------*/

void skill_menu_t::build()
{
	title("Select Skill Level");
	button("Newbie",	SKILL_NEWBIE + 10);
	button("Easy",		SKILL_EASY + 10);
	button("Normal",	SKILL_NORMAL + 10);
	button("Hard",		SKILL_HARD + 10);
	button("Insane",	SKILL_INSANE + 10);
	space(2);
	switch(skill)
	{
	  case SKILL_NEWBIE:
		label("\"I'm afraid of the dark.\"");
		break;
	  case SKILL_EASY:
		label("\"Please, don't hurt me!\"");
		break;
	  case SKILL_NORMAL:
		label("\"Let's warm up a little first.\"");
		break;
	  case SKILL_HARD:
		label("\"Bah! Gimme some resistance here.\"");
		break;
	  case SKILL_INSANE:
		label("\"The dark is afraid of me.\"");
		break;
	  default:
		label("<Unimplemented skill level!>");
		break;
	}
}


void skill_menu_t::rebuild()
{
	int sel = selected_index();
	build_all();
	select(sel);
}


void st_skill_menu_t::enter()
{
#ifdef KOBO_DEMO
	st_game.set_stage(1);
	st_game.set_skill(KOBO_DEMO_SKILL);
	gsm.change(&st_game);
#else
	st_menu_base_t::enter();
#endif
}


kobo_form_t *st_skill_menu_t::open()
{
	menu = new skill_menu_t(gengine);
	menu->set_skill(manage.current_skill());
	menu->open();
	switch(manage.current_skill())
	{
	  case SKILL_NEWBIE:
		menu->select(1);
		break;
	  case SKILL_EASY:
		menu->select(2);
		break;
	  default:
	  case SKILL_NORMAL:
		menu->select(3);
		break;
	  case SKILL_HARD:
		menu->select(4);
		break;
	  case SKILL_INSANE:
		menu->select(5);
		break;
	}
	return menu;
}


void st_skill_menu_t::press(gc_targets_t button)
{
	st_menu_base_t::press(button);
	switch (button)
	{
	  case BTN_UP:
	  case BTN_DOWN:
		menu->set_skill(menu->selected()->tag - 10);
		menu->rebuild();
		break;
	  default:
		break;
	}
}


void st_skill_menu_t::select(int tag)
{
	if((tag >= 10) && (tag <= 20))
	{
		st_game.set_skill(menu->selected()->tag - 10);
		transition_change(&st_game, KOBO_TRS_GAME_SLOW);
	}
}


st_skill_menu_t st_skill_menu;


/*----------------------------------------------------------
	st_options_main
----------------------------------------------------------*/

void options_main_t::build()
{
	title("Options");
	button("Game", 1);
	button("Controls", 2);
	button("Interface", 3);
	space();
	button("Video", 10);
	button("Audio", 11);
	button("System", 12);
	space();
	font(B_NORMAL_FONT);
	button("Cheats", 20);
	button("Debug", 21);
	button("More", 22);
	font();
	space(2);
	button("DONE!", MENU_TAG_OK);
}


kobo_form_t *st_options_main_t::open()
{
	options_main_t *m = new options_main_t(gengine);
	m->open();
	if(manage.ok_to_switch() &&
			(manage.current_stage() !=
			KOBO_OPTIONS_BACKGROUND_LEVEL))
		manage.show_stage(KOBO_OPTIONS_BACKGROUND_LEVEL, GS_TITLE);
	return m;
}


void st_options_main_t::select(int tag)
{
	switch(tag)
	{
	  case 1:
		gsm.push(&st_options_game);
		break;
	  case 2:
		gsm.push(&st_options_controls);
		break;
	  case 3:
		gsm.push(&st_options_interface);
		break;

	  case 10:
		gsm.push(&st_options_video);
		break;
	  case 11:
		gsm.push(&st_options_audio);
		break;
	  case 12:
		gsm.push(&st_options_system);
		break;

#ifdef KOBO_DEMO
	  case 20:
		sound.ui_play(S_UI_ERROR);
		st_error.message("Not available in this demo!",
				"These options are not available\n"
				"in the Kobo Redux demo.\n"
				"\n"
				"Get the full version at:\n"
				"https://olofson.itch.io/kobo-redux");
		gsm.push(&st_error);
		break;
#else
	  case 20:
		gsm.push(&st_options_cheat);
		break;
#endif
	  case 21:
		gsm.push(&st_options_debug);
		break;
	  case 22:
		gsm.push(&st_options_more);
		break;

	  case MENU_TAG_OK:
		pop();
		break;
	}
}


st_options_main_t st_options_main;


/*----------------------------------------------------------
	st_options_base
----------------------------------------------------------*/

kobo_form_t *st_options_base_t::open()
{
	sounds = 0;
	cfg_form = oopen();
	cfg_form->open(prefs);
	return cfg_form;
}


void st_options_base_t::close()
{
	check_update();
	cfg_form->close();
	cfg_form = NULL;
}


void st_options_base_t::enter()
{
	sound.ui_play(S_UI_OPEN);
	st_menu_base_t::enter();
}


void st_options_base_t::check_update()
{
	// Handle changes that require only an update...
	if(cfg_form->status() & OS_UPDATE_AUDIO)
		sound.prefschange();
	if(cfg_form->status() & OS_UPDATE_ENGINE)
	{
		gengine->timefilter(prefs->timefilter * 0.01f);
		gengine->interpolation(prefs->filter);
		gengine->vsync(prefs->vsync);
	}
	if(cfg_form->status() & OS_UPDATE_SCREEN)
		screen.init_background();
	cfg_form->clearstatus(OS_UPDATE);
}


void st_options_base_t::select(int tag)
{
	if(cfg_form->status() & OS_CANCEL)
	{
		cfg_form->undo();
		check_update();
		pop();
	}
	else if(cfg_form->status() & OS_CLOSE)
	{
		if(cfg_form->status() & (OS_RESTART | OS_RELOAD))
			gengine->stop();
		pop();
	}
}


void st_options_base_t::press(gc_targets_t button)
{
	// NOTE:
	//	This may result in select() above being called, and that in
	//	turn, may result in close() being called before when we get
	//	back here! Thus, we need to check cfg_form.
	st_menu_base_t::press(button);

	if(cfg_form)
		check_update();
}


void st_options_base_t::escape()
{
	sound.ui_play(S_UI_CANCEL);
	cfg_form->undo();
	check_update();
}


/*----------------------------------------------------------
	Options...
----------------------------------------------------------*/

st_options_system_t st_options_system;
st_options_video_t st_options_video;
st_options_controls_t st_options_controls;
st_options_audio_t st_options_audio;
st_options_interface_t st_options_interface;
st_options_game_t st_options_game;
st_options_cheat_t st_options_cheat;
st_options_debug_t st_options_debug;


/*----------------------------------------------------------
	More Options
----------------------------------------------------------*/

void options_more_t::build()
{
	title("More Options");
	space();
	font(B_BIG_FONT); label("What!?"); font();
	space();
	label("What do you think this is?");
	label("Free/Open Source Software...?");
	space();
	label("Oh, wait.");
	space();
	button("It is. :-)", 1);
	space(2);
	button("ACCEPT", MENU_TAG_OK);
	button("CANCEL", MENU_TAG_CANCEL);
}


kobo_form_t *st_options_more_t::open()
{
	options_more_t *m = new options_more_t(gengine);
	m->open();
	return m;
}


void st_options_more_t::select(int tag)
{
	switch(tag)
	{
	  case 1:
		kobo_OpenURL("https://github.com/olofson/koboredux");
		break;
	  case MENU_TAG_OK:
	  case MENU_TAG_CANCEL:
		pop();
		break;
	}
}


st_options_more_t st_options_more;


/*----------------------------------------------------------
	st_demo_over_t
----------------------------------------------------------*/

#ifdef KOBO_DEMO

void demo_over_t::build()
{
	space(2);
	font(B_BIG_FONT); label("Thank You For Playing!"); font();
	space(2);
	label("You've reached the end of the");
	space();
	font(B_BIG_FONT); label("Kobo Redux"); font();
	space();
	label("demo.");
	space(2);
	label("To play the rest of the game...");
	space();
	button("Buy Kobo Redux", 40);
	label("https://olofson.itch.io/kobo-redux");
	space(2);
	button("Back To Title", MENU_TAG_OK);
}


kobo_form_t *st_demo_over_t::open()
{
	demo_over_t *m = new demo_over_t(gengine);
	m->open();
	return m;
}


void st_demo_over_t::leave()
{
	manage.abort_game();
}


void st_demo_over_t::select(int tag)
{
	switch(tag)
	{
	  case 40:
		kobo_OpenURL("https://olofson.itch.io/kobo-redux");
		break;
	  case MENU_TAG_OK:
	  case MENU_TAG_CANCEL:
		transition_pop(KOBO_TRS_GAME_SLOW);
		break;
	}
}


st_demo_over_t st_demo_over;
#endif


/*----------------------------------------------------------
	Requesters
----------------------------------------------------------*/

void yesno_menu_t::build()
{
	space(-130);
	button("YES", MENU_TAG_OK);
	button("NO", MENU_TAG_CANCEL);
}


void yesno_menu_t::rebuild()
{
	int sel = selected_index();
	build_all();
	select(sel);
}


/*----------------------------------------------------------
	st_yesno_base_t
----------------------------------------------------------*/

kobo_form_t *st_yesno_base_t::open()
{
	menu = new yesno_menu_t(gengine);
	menu->open();
	menu->select(1);
	return menu;
}


void st_yesno_base_t::reenter()
{
	menu->rebuild();
	menu->select(1);
	st_menu_base_t::reenter();
}


void st_yesno_base_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_NO:
		select(MENU_TAG_CANCEL);
		break;
	  case BTN_YES:
		select(MENU_TAG_OK);
		break;
	  default:
		st_menu_base_t::press(button);
	}
}


void st_yesno_base_t::frame()
{
}


void st_yesno_base_t::post_render()
{
	st_menu_base_t::post_render();

	float ft = SDL_GetTicks() * 0.001;
	woverlay->font(B_BIG_FONT);
	int y = PIXEL2CS(90) + (int)floor(PIXEL2CS(15) * sin(ft * 6));
	woverlay->center_fxp(y, msg);
}


/*----------------------------------------------------------
	st_exit
----------------------------------------------------------*/

void st_exit_t::enter()
{
	km.quit();
}


st_exit_t st_exit;


/*----------------------------------------------------------
	st_ask_exit
----------------------------------------------------------*/

st_ask_exit_t::st_ask_exit_t()
{
	name = "ask_exit";
	msg = "Quit Kobo Redux?";
}


void st_ask_exit_t::select(int tag)
{
	switch(tag)
	{
	  case MENU_TAG_OK:
		if(!(prefs->quickstart || prefs->cmd_warp))
		{
			sound.ui_play(S_UI_OK);
			wdash->mode(DASHBOARD_BLACK, DASHBOARD_FAST);
			transition_change(&st_exit, KOBO_TRS_FULLSCREEN_SLOW);
		}
		else
		{
			pop();
			km.quit();
		}
		break;
	  case MENU_TAG_CANCEL:
		sound.ui_play(S_UI_CANCEL);
		gsm.change(&st_pause_game);
		break;
	}
}


st_ask_exit_t st_ask_exit;


/*----------------------------------------------------------
	st_ask_abort_game
----------------------------------------------------------*/

st_ask_abort_game_t::st_ask_abort_game_t()
{
	name = "ask_abort_game";
	msg = "Abort Game?";
}


void st_ask_abort_game_t::leave()
{
	manage.abort_game();
}


void st_ask_abort_game_t::select(int tag)
{
	switch(tag)
	{
	  case MENU_TAG_OK:
		sound.ui_play(S_UI_OK);
		transition_pop(KOBO_TRS_GAME_SLOW);
		break;
	  case MENU_TAG_CANCEL:
		gsm.change(&st_pause_game);
		break;
	}
}


st_ask_abort_game_t st_ask_abort_game;


/*----------------------------------------------------------
	st_ask_overwrite_campaign
----------------------------------------------------------*/

st_ask_overwrite_campaign_t::st_ask_overwrite_campaign_t()
{
	name = "ask_overwrite_campaign";
	msg = "Overwrite Old Campaign?";
}


void st_ask_overwrite_campaign_t::select(int tag)
{
	switch(tag)
	{
	  case MENU_TAG_OK:
		sound.ui_play(S_UI_OK);
#ifdef KOBO_DEMO
		st_game.set_stage(1);
		st_game.set_skill(KOBO_DEMO_SKILL);
		gsm.change(&st_game);
#else
		gsm.change(&st_skill_menu);
#endif
		break;
	  case MENU_TAG_CANCEL:
		pop();
		break;
	}
}


st_ask_overwrite_campaign_t st_ask_overwrite_campaign;
