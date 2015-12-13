/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001-2003 David Olofson
 * Copyright 2002 Jeremy Sheeley
 * Copyright 2005-2007, 2009 David Olofson
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


gamestatemanager_t gsm;
int run_intro = 0;
int last_level = -1;


kobo_basestate_t::kobo_basestate_t()
{
	name = "<unnamed>";
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
	if(prefs->cmd_debug)
	{
		char buf[32];
		woverlay->font(B_NORMAL_FONT);
		woverlay->string(4, 1, name);
		snprintf(buf, sizeof(buf), "(%d, %d)",
				CS2PIXEL(gengine->xoffs(LAYER_BASES)),
				CS2PIXEL(gengine->yoffs(LAYER_BASES)));
		woverlay->string(4, WMAIN_H - 10, buf);
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
}


void st_introbase_t::enter()
{
	if(!run_intro)
	{
		manage.init_resources_title();
		if(prefs->use_music)
			sound.ui_music_title();
		run_intro = 1;
	}
	start_time = (int)SDL_GetTicks() + INTRO_BLANK_TIME;
	timer = 0;
}


void st_introbase_t::reenter()
{
	if(!run_intro)
	{
		manage.init_resources_title();
		if(prefs->use_music)
			sound.ui_music_title();
		run_intro = 1;
	}
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
	  case BTN_START:
	  case BTN_FIRE:
	  case BTN_SELECT:
		run_intro = 0;
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
	manage.run_intro();
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
	screen.scroller();
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
	if(exit_game)
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
	st_intro_title.inext = &st_intro_highscores;
	st_intro_title.duration = INTRO_TITLE2_TIME - INTRO_BLANK_TIME;
	st_intro_title.mode = pubrand.get(1) + 1;
}

void st_intro_instructions_t::post_render()
{
	if(exit_game)
		return;
	st_introbase_t::post_render();
	if((timer >= 0) && (timer < duration))
		screen.help(timer);
	else
		screen.set_highlight(0, 0);
}

st_intro_instructions_t st_intro_instructions;


/*----------------------------------------------------------
	st_intro_highshores
----------------------------------------------------------*/

st_intro_highscores_t::st_intro_highscores_t()
{
	name = "intro_highscores";
}

void st_intro_highscores_t::enter()
{
	scorefile.gather_high_scores(1);
	screen.init_highscores();
	st_introbase_t::enter();
	duration = INTRO_HIGHSCORE_TIME;
	inext = &st_intro_title;
	st_intro_title.inext = &st_intro_credits;
	st_intro_title.duration = INTRO_TITLE2_TIME - INTRO_BLANK_TIME;
	st_intro_title.mode = pubrand.get(1) + 1;
	screen.set_highlight(0, 0);
}

void st_intro_highscores_t::post_render()
{
	if(exit_game)
		return;
	st_introbase_t::post_render();
	if((timer >= 0) && (timer < duration))
	{
		float nt = (float)timer / duration;
		float snt = 1.0f - sin(nt * M_PI);
		snt = 1.0f - snt * snt * snt;
		screen.highscores(timer, snt);
	}
}

st_intro_highscores_t st_intro_highscores;


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
	if(exit_game)
		return;
	st_introbase_t::post_render();
	if((timer >= 0) && (timer < duration))
		screen.credits(timer);
	else
		screen.set_highlight(0, 0);
}

st_intro_credits_t st_intro_credits;


/*----------------------------------------------------------
	st_game
----------------------------------------------------------*/

st_game_t::st_game_t()
{
	name = "game";
}


void st_game_t::enter()
{
#if 0
	audio_channel_stop(0, -1);	//Stop any music
#endif
	run_intro = 0;
	manage.game_start();
	if(exit_game || manage.game_stopped())
	{
		st_error.message("Could not start game!",
				"Please, check your installation.");
		gsm.change(&st_error);
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
	if(SDL_GetRelativeMouseMode())
		SDL_SetRelativeMouseMode(SDL_FALSE);
}


void st_game_t::reenter()
{
	if(prefs->mousecapture)
		if(!SDL_GetRelativeMouseMode())
			SDL_SetRelativeMouseMode(SDL_TRUE);
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
	  case BTN_START:
	  case BTN_PAUSE:
		gsm.push(&st_pause_game);
		break;
	  default:
		break;
	}
}


void st_game_t::frame()
{
	if(manage.get_ready())
	{
		gsm.push(&st_get_ready);
		return;
	}
	manage.run_game();
	last_level = manage.current_scene();
	if(manage.game_over())
		gsm.change(&st_game_over);
	else if(exit_game || manage.game_stopped())
		pop();
}


void st_game_t::post_render()
{
	kobo_basestate_t::post_render();
	wradar->frame();
}


st_game_t st_game;



/*----------------------------------------------------------
	st_pause_game
----------------------------------------------------------*/

st_pause_game_t::st_pause_game_t()
{
	name = "pause_game";
}


void st_pause_game_t::enter()
{
	sound.ui_play(SOUND_UI_PAUSE);
}


void st_pause_game_t::press(gc_targets_t button)
{
	switch (button)
	{
	  case BTN_EXIT:
		gsm.change(&st_main_menu);
		break;
	  default:
		sound.ui_play(SOUND_UI_PLAY);
		pop();
		break;
	}
}


void st_pause_game_t::frame()
{
	manage.run_pause();
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
	manage.update();
	sound.ui_play(SOUND_UI_READY);
	start_time = (int)SDL_GetTicks();
	frame_time = 0;
	countdown = prefs->countdown;
}


void st_get_ready_t::press(gc_targets_t button)
{
	if(frame_time < 500)
		return;

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
	  case BTN_FIRE:
	  case BTN_YES:
		sound.ui_play(SOUND_UI_PLAY);
		pop();
		break;
	  case BTN_SELECT:
	  case BTN_START:
	  case BTN_PAUSE:
		gsm.change(&st_pause_game);
		break;
	  default:
		break;
	}
}


void st_get_ready_t::frame()
{
	manage.run_pause();

	if(exit_game || manage.game_stopped())
	{
		pop();
		return;
	}

	frame_time = (int)SDL_GetTicks() - start_time;
	if(0 == prefs->countdown)
	{
		if(frame_time > 700)
		{
			sound.ui_play(SOUND_UI_PLAY);
			pop();
		}
	}
	else if(prefs->countdown <= 9)
	{
		int prevcount = countdown;
		countdown = prefs->countdown - frame_time/1000;
		if(prevcount != countdown)
			sound.ui_countdown(countdown);

		if(countdown < 1)
		{
			sound.ui_play(SOUND_UI_PLAY);
			pop();
		}
	}
}


void st_get_ready_t::post_render()
{
	kobo_basestate_t::post_render();

	float ft = SDL_GetTicks() * 0.001;
	char counter[2] = "0";
	woverlay->font(B_BIG_FONT);
	int y = PIXEL2CS(70) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
	woverlay->center_fxp(y, "GET READY!");

	float z = (float)((int)SDL_GetTicks() - start_time);
	if(10 == prefs->countdown)
		z = -1;
	else if(prefs->countdown)
		z = prefs->countdown - z * 0.001;
	else
		z = 1.0 - z / 700.0;
	if((z > 0.0) && (z < 1.0))
	{
		float x = woverlay->width() / 2;
		woverlay->foreground(woverlay->map_rgb(
				255 - (int)(z * 255.0),
				(int)(z * 255.0),
				0));
		woverlay->fillrect_fxp(PIXEL2CS((int)(x - z * 50.0)),
				y + PIXEL2CS(76),
				PIXEL2CS((int)(z * 100.0)),
				PIXEL2CS(10));
	}

	woverlay->font(B_MEDIUM_FONT);
	if(10 == prefs->countdown)
		woverlay->center_fxp(y + PIXEL2CS(70), "(Press FIRE)");
	else if(prefs->countdown)
	{
		woverlay->center_fxp(y + PIXEL2CS(100), "(Press FIRE)");
		counter[0] = countdown + '0';
		woverlay->font(B_COUNTER_FONT);
		woverlay->center_fxp(y + PIXEL2CS(60), counter);
	}

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
	sound.ui_play(SOUND_UI_GAMEOVER);
	manage.update();
	start_time = (int)SDL_GetTicks();
}


void st_game_over_t::press(gc_targets_t button)
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
	  case BTN_FIRE:
	  case BTN_START:
	  case BTN_SELECT:
	  case BTN_YES:
		sound.ui_play(SOUND_UI_OK);
		pop();
		break;
	  default:
		break;
	}
}


void st_game_over_t::frame()
{
	manage.run_game();

	frame_time = (int)SDL_GetTicks() - start_time;
	if(frame_time > 5000)
		pop();
}


void st_game_over_t::post_render()
{
	kobo_basestate_t::post_render();

	float ft = SDL_GetTicks() * 0.001;
	woverlay->font(B_BIG_FONT);
	int y = PIXEL2CS(100) + (int)floor(PIXEL2CS(15)*sin(ft * 6));
	woverlay->center_fxp(y, "GAME OVER");

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
	if(manage.game_stopped())
		run_intro = 1;
	if(sounds)
		sound.ui_play(SOUND_UI_OPEN);
}

// Because we may get back here after changing the configuration!
void st_menu_base_t::reenter()
{
	if(global_status & OS_RESTART_VIDEO)
		pop();
}

void st_menu_base_t::leave()
{
	screen.set_highlight(0, 0);
	close();
	delete form;
	form = NULL;
	if(manage.game_stopped())
	{
		manage.init_resources_title();
		st_intro_title.inext = &st_intro_instructions;
		st_intro_title.duration = INTRO_TITLE_TIME + 2000;
		st_intro_title.mode = 0;
	}
}

void st_menu_base_t::frame()
{
	if(manage.game_stopped())
		manage.run_intro();
	//(Game is paused when a menu is active.)
}

void st_menu_base_t::post_render()
{
	kobo_basestate_t::post_render();
	if(form)
		form->render();
	if(!manage.game_stopped())
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
		return tag;
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
	  case BTN_FIRE:
	  case BTN_START:
	  case BTN_SELECT:
		if(form->selected())
			selection = translate(form->selected()->tag,
					button);
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
		  case BTN_FIRE:
		  case BTN_START:
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
			sound.ui_play(SOUND_UI_CANCEL);
		select(0);
		pop();
		break;
	  default:
		select(selection);
		break;
	}
}



/*----------------------------------------------------------
	st_new_player
----------------------------------------------------------*/

void new_player_t::open()
{
	place(woverlay->px(), woverlay->py(),
			woverlay->width(), woverlay->height());
	font(B_NORMAL_FONT);
	foreground(woverlay->map_rgb(255, 255, 255));
	background(woverlay->map_rgb(0, 0, 0));
	memset(name, 0, sizeof(name));
	name[0] = 'A';
	currentIndex = 0;
	editing = 1;
	build_all();
#if 0
//FIXME:
	SDL_EnableUNICODE(1);
#endif
}

void new_player_t::close()
{
#if 0
//FIXME:
	SDL_EnableUNICODE(0);
#endif
	clean();
}

void new_player_t::change(int delta)
{
	kobo_form_t::change(delta);

	if(!selected())
		return;

	selection = selected()->tag;
}

void new_player_t::build()
{
	medium();
	space(2);
	label("Use arrows, joystick or keyboard");
	label("to enter name");

	big();
	space();
	button(name, 1);
	space();

	button("Ok", MENU_TAG_OK);
	button("Cancel", MENU_TAG_CANCEL);
}

void new_player_t::rebuild()
{
	int sel = selected_index();
	build_all();
	select(sel);
}


st_new_player_t::st_new_player_t()
{
	name = "new_player";
}

kobo_form_t *st_new_player_t::open()
{
	menu = new new_player_t(gengine);
	menu->open();
	return menu;
}

void st_new_player_t::frame()
{
	manage.run_intro();
}

void st_new_player_t::enter()
{
	menu->open();
	run_intro = 0;
	sound.ui_play(SOUND_UI_OPEN);
}

void st_new_player_t::leave()
{
	menu->close();
}

void st_new_player_t::post_render()
{
	kobo_basestate_t::post_render();
	menu->render();
}

void st_new_player_t::press(gc_targets_t button)
{
	if(menu->editing)
	{
		switch(button)
		{
		  case BTN_EXIT:
			sound.ui_play(SOUND_UI_OK);
			menu->editing = 0;
			menu->next();	// Select the CANCEL option.
			menu->next();
			break;

		  case BTN_FIRE:
			if(!prefs->use_joystick)
				break;
		  case BTN_START:
		  case BTN_SELECT:
			sound.ui_play(SOUND_UI_OK);
			menu->editing = 0;
			menu->next();	// Select the OK option.
			break;

		  case BTN_UP:
		  case BTN_INC:
			if(!menu->name[menu->currentIndex])
				menu->name[menu->currentIndex] = 'A';
			else if(menu->name[menu->currentIndex] == 'Z')
				menu->name[menu->currentIndex] = 'a';
			else if(menu->name[menu->currentIndex] == 'z')
				menu->name[menu->currentIndex] = 'A';
			else
				menu->name[menu->currentIndex]++;
			sound.ui_play(SOUND_UI_TICK);
			break;

		  case BTN_DEC:
		  case BTN_DOWN:
			if(!menu->name[menu->currentIndex])
				menu->name[menu->currentIndex] = 'A';
			else if(menu->name[menu->currentIndex] == 'A')
				menu->name[menu->currentIndex] = 'z';
			else if(menu->name[menu->currentIndex] == 'a')
				menu->name[menu->currentIndex] = 'Z';
			else
				menu->name[menu->currentIndex]--;
			sound.ui_play(SOUND_UI_TICK);
			break;

		  case BTN_RIGHT:
			if(menu->currentIndex < sizeof(menu->name) - 2)
			{
				menu->currentIndex++;
				sound.ui_play(SOUND_UI_TICK);
			}
			else
			{
				sound.ui_play(SOUND_UI_ERROR);
				break;
			}
			if(menu->name[menu->currentIndex] == '\0')
				menu->name[menu->currentIndex] = 'A';
			break;

		  case BTN_LEFT:
		  case BTN_BACK:
			if(menu->currentIndex > 0)
			{
				menu->name[menu->currentIndex] = '\0';
				menu->currentIndex--;
				sound.ui_play(SOUND_UI_TICK);
			}
			else
				sound.ui_play(SOUND_UI_ERROR);
			break;

		  default:
#if 0
			if(((unicode >= 'a') && (unicode <= 'z')) ||
				((unicode >= 'A') && (unicode <= 'Z')))
			{
				menu->name[menu->currentIndex] = (char)unicode;
				if(menu->currentIndex < sizeof(menu->name) - 2)
				{
					menu->currentIndex++;
					sound.ui_play(SOUND_UI_TICK);
				}
				else
					sound.ui_play(SOUND_UI_ERROR);
			}
			else
				sound.ui_play(SOUND_UI_ERROR);
#endif
			break;
		}
		menu->rebuild();
	}
	else
	{
		menu->selection = -1;

		switch(button)
		{
		  case BTN_EXIT:
			menu->selection = MENU_TAG_CANCEL;
			break;

		  case BTN_CLOSE:
			menu->selection = MENU_TAG_OK;
			break;

		  case BTN_FIRE:
		  case BTN_START:
		  case BTN_SELECT:
			menu->change(0);
			break;

		  case BTN_INC:
		  case BTN_UP:
			menu->prev();
			break;

		  case BTN_DEC:
		  case BTN_DOWN:
			menu->next();
			break;

		  default:
			break;
		}

		switch(menu->selection)
		{
		  case 1:
			if(button == BTN_START
					|| button == BTN_SELECT
					|| button == BTN_FIRE)
			{
				sound.ui_play(SOUND_UI_OK);
				menu->editing = 1;
			}
			break;

		  case MENU_TAG_OK:
			switch(scorefile.add_player(menu->name))
			{
			  case 0:
				sound.ui_play(SOUND_UI_OK);
				prefs->last_profile = scorefile.current_profile();
				prefs->changed = 1;
				pop();
				break;
			  case -1:
				sound.ui_play(SOUND_UI_ERROR);
				st_error.message("Cannot create Player Profile!",
						"Too many profiles!");
				gsm.change(&st_error);
				break;
			  case -2:
			  case -3:
				prefs->last_profile = scorefile.current_profile();
				sound.ui_play(SOUND_UI_ERROR);
				st_error.message("Cannot save Player Profile!",
						"Please, check your installation.");
				gsm.change(&st_error);
				break;
			  default:
				sound.ui_play(SOUND_UI_ERROR);
				st_error.message("Cannot create Player Profile!",
						"Bug or internal error.");
				gsm.change(&st_error);
				break;
			}
			break;

		  case MENU_TAG_CANCEL:
			sound.ui_play(SOUND_UI_CANCEL);
			strcpy(menu->name, "A");
			pop();
			break;
		}
	}
}

st_new_player_t st_new_player;


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
	sound.ui_play(SOUND_UI_ERROR);
	manage.update();
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
	  case BTN_FIRE:
	  case BTN_START:
	  case BTN_SELECT:
	  case BTN_YES:
		sound.ui_play(SOUND_UI_OK);
		pop();
		break;
	  default:
		break;
	}
}


void st_error_t::frame()
{
	manage.run_intro();

	frame_time = (int)SDL_GetTicks() - start_time;
#if 0
	if(frame_time % 1000 < 500)
		sound.ui_play(SOUND_UI_ERROR);
#endif
}


void st_error_t::post_render()
{
	kobo_basestate_t::post_render();

	woverlay->font(B_MEDIUM_FONT);
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

void main_menu_t::buildStartLevel(int profNum)
{
	char buf[50];
	int MaxStartLevel = profNum >= 0 ? scorefile.last_scene(profNum) : 500;
	start_level = manage.current_scene();
	if(start_level > MaxStartLevel)
		start_level = MaxStartLevel;
	list("Start at Stage", &start_level, 5);
	for(int i = 0; i <= MaxStartLevel; ++i)
	{
		snprintf(buf, sizeof(buf), "%d", i + 1);
		item(buf, i);
	}
}

void main_menu_t::build()
{
	if(manage.game_stopped())
	{
		prefs->last_profile = scorefile.current_profile();
		if(last_level < 0)
			manage.select_scene(scorefile.last_scene());
		else
			manage.select_scene(last_level);
	}

	halign = ALIGN_CENTER;
	xoffs = 0.5;
	big();
	if(manage.game_stopped())
	{
		space();
#if 0
		if(scorefile.numProfiles > 0)
		{
			button("Start Game!", 1);
			space();
			list("Player", &prefs->last_profile, 4);
			for(int i = 0; i < scorefile.numProfiles; ++i)
				item(scorefile.name(i), i);
			small();
			buildStartLevel(prefs->last_profile);
			big();
		}
		else
			space(2);
		button("New Player...", 3);
#else
		log_printf(WLOG, "Player profiles are disabled!\n");
		button("Start Game!", 1);
		small();
		buildStartLevel(-1);
		big();
#endif
	}
	else
	{
		space(2);
		button("Return to Game", 0);
	}
	space();
	button("Options", 2);
	space();
	if(manage.game_stopped())
		button("Return to Intro", 0);
	else
		button("Abort Current Game", 101);
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
	if(manage.game_stopped())
	{
		// Moved here, as we want to do it as late as
		// possible, but *not* as a result of rebuild().
		if(prefs->last_profile >= scorefile.numProfiles)
		{
			prefs->last_profile = 0;
			prefs->changed = 1;
		}
		scorefile.select_profile(prefs->last_profile);
	}

	menu = new main_menu_t(gengine);
	menu->open();
	return menu;
}


void st_main_menu_t::reenter()
{
	menu->rebuild();
	st_menu_base_t::reenter();
	if(manage.game_stopped())
	    manage.select_scene(menu->start_level);
}


// Custom translator to map inc/dec on certain widgets
int st_main_menu_t::translate(int tag, int button)
{
	switch(tag)
	{
	  case 4:
		// The default translate() filters out the
		// inc/dec events, and performs the default
		// action for fire/start/select...
		switch(button)
		{
		  case BTN_FIRE:
		  case BTN_START:
		  case BTN_SELECT:
			do_default_action = 0;
			return tag + 10;
		  default:
			return tag;
		}
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
		gsm.change(&st_skill_menu);
		break;
	  case 2:
		gsm.push(&st_options_main);
		break;
	  case 3:
		gsm.push(&st_new_player);
		break;
	  case 4:	// Player: Inc/Dec
		sound.ui_play(SOUND_UI_TICK);
		prefs->changed = 1;
		scorefile.select_profile(prefs->last_profile);
		menu->rebuild();
		break;
	  case 14:	// Player: Select
		// Edit player profile!
//		menu->rebuild();
		break;
	  case 5:	// Start level: Inc/Dec
		sound.ui_play(SOUND_UI_TICK);
		manage.select_scene(menu->start_level);
		break;
	  case MENU_TAG_CANCEL:
		gsm.change(&st_ask_exit);
		break;
	  case 101:
		gsm.change(&st_ask_abort_game);
		break;
	  case 0:
		if(!manage.game_stopped())
			gsm.change(&st_pause_game);
		break;
	}
}

st_main_menu_t st_main_menu;


/*----------------------------------------------------------
	st_skill_menu
----------------------------------------------------------*/

void skill_menu_t::build()
{
	halign = ALIGN_CENTER;
	xoffs = 0.5;
	medium();
	label("Select Skill Level");
	space(1);

	big();
	button("Normal", SKILL_NORMAL + 10);
	button("Hard", SKILL_HARD + 10);
	button("Insane", SKILL_INSANE + 10);
	space();
	small();
	switch(skill)
	{
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


kobo_form_t *st_skill_menu_t::open()
{
	menu = new skill_menu_t(gengine);
	menu->set_skill(scorefile.profile()->skill);
	menu->open();
	switch(scorefile.profile()->skill)
	{
	  default:
	  case SKILL_NORMAL:
		menu->select(1);
		break;
	  case SKILL_HARD:
		menu->select(2);
		break;
	  case SKILL_INSANE:
		menu->select(3);
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
		scorefile.profile()->skill = menu->selected()->tag - 10;
		gsm.change(&st_game);
	}
}

st_skill_menu_t st_skill_menu;


/*----------------------------------------------------------
	st_options_main
----------------------------------------------------------*/

void options_main_t::build()
{
	medium();
	label("Options");
	space();

	big();
	button("Game", 4);
	button("Controls", 3);
	button("Video", 1);
	button("Graphics", 6);
	button("Audio", 2);
	button("System", 5);
	space();

	button("DONE!", 0);
}

kobo_form_t *st_options_main_t::open()
{
	options_main_t *m = new options_main_t(gengine);
	m->open();
	if(manage.game_stopped())
	    manage.select_scene(KOBO_OPTIONS_BACKGROUND_LEVEL);
	return m;
}

void st_options_main_t::select(int tag)
{
	switch(tag)
	{
	  case 1:
		gsm.push(&st_options_video);
		break;
	  case 2:
		gsm.push(&st_options_audio);
		break;
	  case 3:
		gsm.push(&st_options_control);
		break;
	  case 4:
		gsm.push(&st_options_game);
		break;
	  case 5:
		gsm.push(&st_options_system);
		break;
	  case 6:
		gsm.push(&st_options_graphics);
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
	sound.ui_play(SOUND_UI_OPEN);
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
	}
	else if(cfg_form->status() & OS_CLOSE)
	{
		if(cfg_form->status() & (OS_RESTART | OS_RELOAD))
		{
			exit_game = 0;
			manage.freeze_abort();
		}
	}
	if(cfg_form->status() & (OS_CANCEL | OS_CLOSE))
		pop();
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
	sound.ui_play(SOUND_UI_CANCEL);
	cfg_form->undo();
	check_update();
}



/*----------------------------------------------------------
	Options...
----------------------------------------------------------*/
st_options_system_t st_options_system;
st_options_video_t st_options_video;
st_options_graphics_t st_options_graphics;
st_options_audio_t st_options_audio;
st_options_control_t st_options_control;
st_options_game_t st_options_game;



/*----------------------------------------------------------
	Requesters
----------------------------------------------------------*/
void yesno_menu_t::build()
{
	halign = ALIGN_CENTER;
	xoffs = 0.5;
	big();
	space(8);
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
	if(manage.game_stopped())
		manage.run_intro();
	else
		manage.run_pause();
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
#if 0
		audio_channel_stop(0, -1);	//Stop any music
#endif
		sound.ui_play(SOUND_UI_OK);
		exit_game = 1;
		pop();
		break;
	  case MENU_TAG_CANCEL:
		sound.ui_play(SOUND_UI_CANCEL);
		if(manage.game_stopped())
			pop();
		else
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
		sound.ui_play(SOUND_UI_OK);
		manage.abort();
		pop();
		break;
	  case MENU_TAG_CANCEL:
		gsm.change(&st_pause_game);
		break;
	}
}

st_ask_abort_game_t st_ask_abort_game;
