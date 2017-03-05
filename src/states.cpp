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


void kobo_basestate_t::frame()
{
}


void kobo_basestate_t::pre_render()
{
	screen.render_background();
}


void kobo_basestate_t::post_render()
{
	screen.render_fx();
	if(prefs->debug)
	{
		// State stack
		int y = 1;
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


/*----------------------------------------------------------
	st_introbase
----------------------------------------------------------*/

st_introbase_t::st_introbase_t()
{
	name = "intro";
	inext = NULL;
	duration = 0;
	timer = 0;
	song = S_TITLESONG;
}


void st_introbase_t::enter()
{
	kobo_basestate_t::enter();
	if(manage.state() != GS_INTRO)
		manage.start_intro();
	if(!km.quitting())
		sound.music(song);
	start_time = (int)SDL_GetTicks() + INTRO_BLANK_TIME;
	timer = 0;
}


void st_introbase_t::reenter()
{
	kobo_basestate_t::reenter();
	if(manage.state() != GS_INTRO)
		manage.start_intro();
	if(!km.quitting())
		sound.music(song);
	gsm.change(&st_intro_title);
}


void st_introbase_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_EXIT:
//		gsm.push(&st_ask_exit);
//		break;
	  case BTN_CLOSE:
		gsm.push(&st_main_menu);
		break;
	  case BTN_PRIMARY:
	  case BTN_SECONDARY:
	  case BTN_SELECT:
		gsm.push(&st_main_menu);
#if 0
		if(scorefile.numProfiles <= 0)
			gsm.push(&st_new_player);
#endif
		break;
	  case BTN_BACK:
	  case BTN_UP:
	  case BTN_LEFT:
		gsm.change(&st_intro_title);
		break;
	  case BTN_DOWN:
	  case BTN_RIGHT:
		if(inext)
			gsm.change(inext);
		break;
	  default:
		break;
	}
}


void st_introbase_t::frame()
{
	if((timer > duration) && inext)
		gsm.change(inext);
}


void st_introbase_t::pre_render()
{
	kobo_basestate_t::pre_render();
	timer = (int)SDL_GetTicks() - start_time;
}


void st_introbase_t::post_render()
{
	kobo_basestate_t::post_render();
#if 0
	screen.scroller();
#endif
}


/*----------------------------------------------------------
	st_intro_title
----------------------------------------------------------*/

st_intro_title_t::st_intro_title_t()
{
	name = "intro_title";
}

void st_intro_title_t::enter()
{
	st_introbase_t::enter();
	if(!duration)
		duration = INTRO_TITLE_TIME + 2000 - INTRO_BLANK_TIME;
	if(!inext)
		inext = &st_intro_instructions;
	screen.set_highlight(0, 0);
}

void st_intro_title_t::post_render()
{
	if(km.quitting())
		return;
	st_introbase_t::post_render();
	if((timer >= 0) && (timer < duration))
	{
		float nt = (float)timer / duration;
		float snt = 1.0f - sin(nt * M_PI);
		snt = 1.0f - snt * snt * snt;
		screen.title(timer, snt, mode);
	}
}

st_intro_title_t st_intro_title;


/*----------------------------------------------------------
	st_intro_instructions
----------------------------------------------------------*/

st_intro_instructions_t::st_intro_instructions_t()
{
	name = "intro_instructions";
}

void st_intro_instructions_t::enter()
{
	st_introbase_t::enter();
	duration = INTRO_INSTRUCTIONS_TIME;
	inext = &st_intro_title;
	st_intro_title.inext = &st_intro_credits;
	st_intro_title.duration = INTRO_TITLE2_TIME - INTRO_BLANK_TIME;
	st_intro_title.mode = pubrand.get(1) + 1;
}

void st_intro_instructions_t::post_render()
{
	if(km.quitting())
		return;
	st_introbase_t::post_render();
	if((timer >= 0) && (timer < duration))
		screen.help(timer);
	else
		screen.set_highlight(0, 0);
}

st_intro_instructions_t st_intro_instructions;


/*----------------------------------------------------------
	st_intro_credits
----------------------------------------------------------*/

st_intro_credits_t::st_intro_credits_t()
{
	name = "intro_credits";
}

void st_intro_credits_t::enter()
{
	st_introbase_t::enter();
	duration = INTRO_CREDITS_TIME;
	inext = &st_intro_title;
	st_intro_title.inext = &st_intro_instructions;
	st_intro_title.duration = INTRO_TITLE_TIME - INTRO_BLANK_TIME;
	st_intro_title.mode = 0;
}

void st_intro_credits_t::post_render()
{
	if(km.quitting())
		return;
	st_introbase_t::post_render();
	if((timer >= 0) && (timer < duration))
		screen.credits(timer);
	else
		screen.set_highlight(0, 0);
}

st_intro_credits_t st_intro_credits;


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
	if(!manage.game_in_progress())
		manage.select_scene(KOBO_CREDITS_BACKGROUND_LEVEL);
	start_time = (int)SDL_GetTicks() + INTRO_BLANK_TIME;
	timer = 0;
}


void st_long_credits_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_EXIT:
	  case BTN_CLOSE:
	  case BTN_PRIMARY:
	  case BTN_SECONDARY:
	  case BTN_SELECT:
	  case BTN_BACK:
		gsm.pop();
		break;
#if 0
	  case BTN_UP:
	  case BTN_LEFT:
		break;
	  case BTN_DOWN:
	  case BTN_RIGHT:
		break;
#endif
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
}


void st_game_t::enter()
{
	if(!manage.game_in_progress())
	{
		st_error.message("INTERNAL ERROR",
				"Entered st_game with no game in progress!");
		gsm.change(&st_error);
		return;
	}
	if(prefs->mousecapture)
		if(!SDL_GetRelativeMouseMode())
			SDL_SetRelativeMouseMode(SDL_TRUE);
}


void st_game_t::leave()
{
	if(SDL_GetRelativeMouseMode())
		SDL_SetRelativeMouseMode(SDL_FALSE);
	st_intro_title.inext = &st_intro_instructions;
	st_intro_title.duration = INTRO_TITLE_TIME + 2000;
	st_intro_title.mode = 0;
}


void st_game_t::yield()
{
	manage.pause(true);
	if(SDL_GetRelativeMouseMode())
		SDL_SetRelativeMouseMode(SDL_FALSE);
}


void st_game_t::reenter()
{
	if(prefs->mousecapture)
		if(!SDL_GetRelativeMouseMode())
			SDL_SetRelativeMouseMode(SDL_TRUE);
	manage.pause(false);
}


void st_game_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_EXIT:
		gsm.push(&st_main_menu);
		break;
	  case BTN_CLOSE:
		gsm.push(&st_ask_exit);
		break;
	  case BTN_SELECT:
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
	if(prefs->debug)
		info = manage.state_name(manage.state());
	switch(manage.state())
	{
	  case GS_GETREADY:
		gsm.push(&st_get_ready);
		return;
	  case GS_GAMEOVER:
		gsm.change(&st_game_over);
		return;
	  case GS_NONE:
		manage.abort_game();
		pop();
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
	if(!manage.game_in_progress())
	{
		st_error.message("INTERNAL ERROR",
				"Entered st_rewind with no game in progress!");
		gsm.change(&st_error);
		return;
	}
#ifdef KOBO_DEMO
	if(manage.current_stage() > KOBO_DEMO_LAST_STAGE)
		gsm.change(&st_demo_over);
#endif
}


void st_rewind_t::leave()
{
	st_intro_title.inext = &st_intro_instructions;
	st_intro_title.duration = INTRO_TITLE_TIME + 2000;
	st_intro_title.mode = 0;
}


void st_rewind_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_EXIT:
		gsm.push(&st_main_menu);
		break;
	  case BTN_CLOSE:
		gsm.push(&st_ask_exit);
		break;
	  case BTN_SELECT:
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
	if(prefs->debug)
		info = manage.state_name(manage.state());
	if(manage.replay_mode() != RPM_REWIND)
		gsm.change(&st_game);
	switch(manage.state())
	{
	  case GS_NONE:
		manage.abort_game();
		pop();
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

	// Gray overlay
	woverlay->foreground(woverlay->map_rgb(48, 48, 48));
	woverlay->alphamod(128);
	woverlay->fillrect(0, 0, woverlay->width(), woverlay->height());
	woverlay->alphamod(255);

	// "Timeout" progress bar
	woverlay->font(B_NORMAL_FONT);
	float p = manage.replay_progress();
	float px = p * p;
	px *= px;
	int p1 = (int)(px * 127.0f);
	int p2 = (int)(px * 192.0f);
	woverlay->foreground(woverlay->map_rgb(128 + p1, 192 - p2, 192 - p2));
	woverlay->alphamod(64);
	woverlay->fillrect_fxp(0, PIXEL2CS(300),
			PIXEL2CS((int)(woverlay->width() * p)),
			PIXEL2CS(woverlay->fontheight() + 1));
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
}


void st_replay_t::enter()
{
	if(!manage.game_in_progress())
	{
		st_error.message("INTERNAL ERROR",
				"Entered st_replay with no game in progress!");
		gsm.change(&st_error);
		return;
	}
}


void st_replay_t::leave()
{
	st_intro_title.inext = &st_intro_instructions;
	st_intro_title.duration = INTRO_TITLE_TIME + 2000;
	st_intro_title.mode = 0;
}


void st_replay_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_EXIT:
		gsm.push(&st_main_menu);
		break;
	  case BTN_CLOSE:
		gsm.push(&st_ask_exit);
		break;
	  case BTN_SELECT:
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


void st_replay_t::frame()
{
	if(prefs->debug)
		info = manage.state_name(manage.state());
	if(manage.replay_mode() != RPM_REPLAY)
		gsm.change(&st_game);
	switch(manage.state())
	{
	  case GS_NONE:
		manage.abort_game();
		pop();
		return;
	  default:
		break;
	}
	if(km.quitting())
		pop();
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
			(manage.replay_mode() != RPM_PLAY))
		pop();
}


void st_pause_game_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_EXIT:
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
	  case BTN_EXIT:
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
	  case BTN_YES:
		if(frame_time < 500)
			break;
		sound.ui_play(S_UI_PLAY);
		manage.pause(false);
		manage.player_ready();
		pop();
		break;
	  case BTN_SELECT:
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
	countdown = prefs->cont_countdown;
}


void st_game_over_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_EXIT:
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
	  case BTN_PRIMARY:
	  case BTN_SECONDARY:
	  case BTN_SELECT:
	  case BTN_YES:
		if(frame_time < 500)
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
	if(!prefs->cont_countdown)
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

	float ft = SDL_GetTicks() * 0.001;
	woverlay->font(B_BIG_FONT);
	int y = PIXEL2CS(100) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
	woverlay->center_fxp(y, "CONTINUE?");
	screen.render_countdown(y + PIXEL2CS(60),
			(int)SDL_GetTicks() - start_time,
			prefs->cont_countdown, countdown);

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
#if 0
	if(manage.game_stopped())
	{
		manage.init_resources_title();
		st_intro_title.inext = &st_intro_instructions;
		st_intro_title.duration = INTRO_TITLE_TIME + 2000;
		st_intro_title.mode = 0;
	}
#endif
}

void st_menu_base_t::frame()
{
}

void st_menu_base_t::post_render()
{
	kobo_basestate_t::post_render();
	if(form)
		form->render();
	if(manage.game_in_progress())
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

void st_menu_base_t::press(gc_targets_t button)
{
	int selection;
	if(!form)
		return;

	do_default_action = 1;

	// Translate
	switch(button)
	{
	  case BTN_EXIT:
	  case BTN_CLOSE:
		selection = 0;
		break;
	  case BTN_UP:
	  case BTN_DOWN:
		selection = -1;
		break;
	  case BTN_INC:
	  case BTN_RIGHT:
	  case BTN_DEC:
	  case BTN_LEFT:
	  case BTN_PRIMARY:
	  case BTN_SECONDARY:
	  case BTN_SELECT:
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
		  case BTN_EXIT:
			escape();
			break;
		  case BTN_INC:
		  case BTN_RIGHT:
			form->change(1);
			break;
		  case BTN_DEC:
		  case BTN_LEFT:
			form->change(-1);
			break;
		  case BTN_PRIMARY:
		  case BTN_SECONDARY:
		  case BTN_SELECT:
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
	  case BTN_EXIT:
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
	  case BTN_SELECT:
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
	if(!manage.game_in_progress())
	{
		int sc;
		if(prefs->cheat_startlevel)
		{
			sc = manage.last_played_stage();
			if(sc < 1)
				sc = 1;
		}
		else
			sc = 1;
		manage.select_scene(sc);
	}

	space(2);
	if(manage.game_in_progress())
	{
		if(manage.replay_mode() == RPM_REPLAY)
			button("Return to Replay", MENU_TAG_OK);
		else
			button("Return to Game", MENU_TAG_OK);
	}
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
	button("Showcase", 20);
	button("Options", 2);
	space(2);
	if(manage.game_in_progress())
	{
		if(manage.replay_mode() == RPM_REPLAY)
			button("Stop Replay", 102);
		else
			button("Abort Current Game", 101);
	}
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
	  case BTN_EXIT:
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
		gsm.push(&st_options_main);
		break;
	  case 5:	// Start level: Inc/Dec
		sound.ui_play(S_UI_TICK);
		manage.select_scene(menu->start_level, true);
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
		gsm.push(&st_long_credits);
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
		manage.abort_game();
		pop();
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
}


void campaign_menu_t::setup(const char *hdr, bool new_game, bool replay)
{
	header = hdr;
	newgame = new_game;
	view_replay = replay;
	savemanager.load(-1);
	savemanager.analysis(-1, true);

	// Timestamp reset, because loading might take a few frames.
	// Or much longer than that, if debug log output is enabled!
	sound.timestamp_reset();
}


const char *campaign_menu_t::timedate(time_t *t)
{
	struct tm *tmp = localtime(t);
	if(!tmp)
		return "<cannot decode timestamp>";
	strftime(tdbuf, sizeof(tdbuf), "%a %b %d %T %Y", tmp);
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
	xoffs = 0.1;
	for(int i = 0; i < KOBO_MAX_CAMPAIGN_SLOTS; ++i)
	{
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
	KOBO_campaign_info *ci = savemanager.analysis(manage.current_slot());
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
		manage.select_slot(menu->selected()->tag - 10);
		menu->rebuild();
		break;
	  default:
		break;
	}
}

void st_campaign_menu_t::select(int tag)
{
	int slot = tag - 10;
	if((slot >= 0) && (slot < KOBO_MAX_CAMPAIGN_SLOTS))
	{
		manage.select_slot(slot);
		if(view_replay)
		{
			if(manage.start_replay(1))
				gsm.change(&st_replay);
			else
			{
				sound.ui_play(S_UI_ERROR);
				st_error.message("Campaign Replay Problem!",
						"Could not load or start "
						"replay.");
				gsm.change(&st_error);
			}
		}
		else if(newgame)
		{
			if(savemanager.exists(manage.current_slot()))
				gsm.change(&st_ask_overwrite_campaign);
			else
				gsm.change(&st_skill_menu);
		}
		else
		{
			if(manage.continue_game())
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
	manage.select_skill(KOBO_DEMO_SKILL);
	manage.start_new_game();
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
		manage.select_skill(menu->selected()->tag - 10);
		manage.start_new_game();
		gsm.change(&st_game);
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
	if(!manage.game_in_progress())
		manage.select_scene(KOBO_OPTIONS_BACKGROUND_LEVEL);
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
	  case 21:
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
	  case 21:
		gsm.push(&st_options_debug);
		break;
#endif
	  case 22:
		gsm.push(&st_options_more);
		break;

	  case MENU_TAG_OK:
		gsm.pop();
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
		gsm.pop();
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
	font(B_BIG_FONT); label("Thank You For Playing!");; font();
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

void st_demo_over_t::select(int tag)
{
	switch(tag)
	{
	  case 40:
		kobo_OpenURL("https://olofson.itch.io/kobo-redux");
		break;
	  case MENU_TAG_OK:
	  case MENU_TAG_CANCEL:
		gsm.pop();
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
		sound.ui_play(S_UI_OK);
		km.quit();
		pop();
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

void st_ask_abort_game_t::select(int tag)
{
	switch(tag)
	{
	  case MENU_TAG_OK:
		sound.ui_play(S_UI_OK);
		manage.abort_game();
		pop();
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
		manage.select_skill(KOBO_DEMO_SKILL);
		manage.start_new_game();
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
