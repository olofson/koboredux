/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2006, 2009, 2012 David Olofson
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

#include "manage.h"
#include "kobo.h"
#include "gamectl.h"
#include "myship.h"


int gamecontrol_t::afire;
int gamecontrol_t::r_delay = 250;
int gamecontrol_t::r_interval = 40;
int gamecontrol_t::space;
int gamecontrol_t::left;
int gamecontrol_t::up;
int gamecontrol_t::down;
int gamecontrol_t::right;
int gamecontrol_t::dl;
int gamecontrol_t::dr;
int gamecontrol_t::ul;
int gamecontrol_t::ur;
int gamecontrol_t::shot;
int gamecontrol_t::direction = 1;
int gamecontrol_t::new_direction = 0;
int gamecontrol_t::latch_timer = 0;
int gamecontrol_t::movekey_pressed;


void gamecontrol_t::init(int always_fire)
{
	afire = always_fire;
	left = 0;
	right = 0;
	up = 0;
	down = 0;
	shot = 0;
	movekey_pressed = 0;
#if 0
	SDL_EnableKeyRepeat(r_delay, r_interval);
#endif
}


gamecontrol_t::gamecontrol_t()
{
	init(0);
}


void gamecontrol_t::repeat(int delay, int interval)
{
	r_delay = delay;
	r_interval = interval;
#if 0
	//Temporary kludge - should apply repeat to
	//all switch inputs, not just the keyboard!
	SDL_EnableKeyRepeat(delay, interval);
#endif
}


void gamecontrol_t::clear()
{
	direction = 1;
}


int gamecontrol_t::map(SDL_Keycode sym)
{
/*
FIXME: This should be replaced by a configurable mapping system.
*/
	switch(sym)
	{
	  case KEY_KP_LEFT:
	  case SDLK_LEFT:
	  case SDLK_a:		// Qwerty + Dvorak
		return BTN_LEFT;
	  case KEY_KP_RIGHT:
	  case SDLK_RIGHT:
	  case SDLK_d:		// Qwerty
	  case SDLK_e:		// Dvorak
		return BTN_RIGHT;
	  case KEY_KP_UP:
	  case SDLK_UP:
	  case SDLK_w:		// Qwerty
	  case SDLK_COMMA:	// Some Swedish Dvorak variants
	  case SDLK_LESS:	// US Dvorak?
		return BTN_UP;
	  case KEY_KP_DOWN:
	  case SDLK_DOWN:
	  case SDLK_s:		// Qwerty
	  case SDLK_o:		// Dvorak
		return BTN_DOWN;
	  case KEY_KP_DL:
		return BTN_DL;
	  case KEY_KP_DR:
		return BTN_DR;
	  case KEY_KP_UL:
		return BTN_UL;
	  case KEY_KP_UR:
		return BTN_UR;

	  case SDLK_PAGEUP:
		if(prefs->broken_numdia)
			return BTN_UR;
		else
			return BTN_NEXT;
	  case SDLK_PAGEDOWN:
		if(prefs->broken_numdia)
			return BTN_DR;
		else
			return BTN_PREV;
	  case SDLK_HOME:
		if(prefs->broken_numdia)
			return BTN_UL;
		else
			return -1;
	  case SDLK_END:
		if(prefs->broken_numdia)
			return BTN_UL;
		else
			return -1;

	  case SDLK_LSHIFT:
	  case SDLK_RSHIFT:
	  case SDLK_LCTRL:
	  case SDLK_RCTRL:
		return BTN_FIRE;
	  case SDLK_ESCAPE:
		return BTN_EXIT;
	  case SDLK_PAUSE:
	  case SDLK_p:
		return BTN_PAUSE;
	  case SDLK_SPACE:
		return BTN_START;
	  case SDLK_KP_ENTER:
	  case SDLK_RETURN:
		return BTN_SELECT;
	  case SDLK_KP_PLUS:
		return BTN_INC;
	  case SDLK_KP_MINUS:
		return BTN_DEC;
	  case SDLK_y:
		return BTN_YES;
	  case SDLK_n:
		return BTN_NO;
	  case SDLK_BACKSPACE:
		return BTN_BACK;
	  case SDLK_F1:
		return BTN_F1;
	  case SDLK_F2:
		return BTN_F2;
	  case SDLK_F3:
		return BTN_F3;
	  case SDLK_F4:
		return BTN_F4;
	  case SDLK_F5:
		return BTN_F5;
	  case SDLK_F6:
		return BTN_F6;
	  case SDLK_F7:
		return BTN_F7;
	  case SDLK_F8:
		return BTN_F8;
	  case SDLK_F9:
		return BTN_F9;
	  case SDLK_F10:
		return BTN_F10;
	  case SDLK_F11:
		return BTN_F11;
	  case SDLK_F12:
		return BTN_F12;
	  default:
		return -1;
	}
}


void gamecontrol_t::press(int k)
{
	switch(k)
	{
	  case BTN_LEFT:
		left = 1;
		break;
	  case BTN_RIGHT:
		right = 1;
		break;
	  case BTN_UP:
		up = 1;
		break;
	  case BTN_DOWN:
		down = 1;
		break;
	  case BTN_UL:
		ul = 1;
		break;
	  case BTN_UR:
		ur = 1;
		break;
	  case BTN_DL:
		dl = 1;
		break;
	  case BTN_DR:
		dr = 1;
		break;
	  case BTN_FIRE:
		shot = 1;
		break;
	}
	gamecontrol_t::change();
}

void gamecontrol_t::release(int k)
{
	switch(k)
	{
	  case BTN_LEFT:
		left = 0;
		break;
	  case BTN_RIGHT:
		right = 0;
		break;
	  case BTN_UP:
		up = 0;
		break;
	  case BTN_DOWN:
		down = 0;
		break;
	  case BTN_UL:
		ul = 0;
		break;
	  case BTN_UR:
		ur = 0;
		break;
	  case BTN_DL:
		dl = 0;
		break;
	  case BTN_DR:
		dr = 0;
		break;
	  case BTN_FIRE:
		shot = 0;
		break;
	}
	gamecontrol_t::change();
}


void gamecontrol_t::mouse_press(int n)
{
	if(n == 1)
		shot = 1;
//	else if(n == 3)
//		manage.key_down(KEY_START);
	gamecontrol_t::change();
}


void gamecontrol_t::mouse_release(int n)
{
	if(n == 1)
		shot = 0;
	gamecontrol_t::change();
}


void gamecontrol_t::mouse_position(int h, int v)
{
	switch(prefs->mousemode)
	{
	  case MMD_OFF:
		return;
	  case MMD_CROSSHAIR:
		myship.put_crosshair();
		break;
	  case MMD_RELATIVE:
		/* Insert delta pos sensitivity filter here */
		break;
	}
	if(h > 0)
	{
		if(v > 0)
		{
			if(h > (v << 1))
				direction = 3;
			else if(v > (h << 1))
				direction = 5;
			else
				direction = 4;
		}
		else if(v <= 0)
		{
			if(h > ((-v) << 1))
				direction = 3;
			else if((-v) > (h << 1))
				direction = 1;
			else
				direction = 2;
		}
		else
			direction = 3;
	}
	else if(h <= 0)
	{
		if(v > 0)
		{
			if((-h) > (v << 1))
				direction = 7;
			else if(v > ((-h) << 1))
				direction = 5;
			else
				direction = 6;
		}
		else if(v <= 0)
		{
			if((-h) > ((-v) << 1))
				direction = 7;
			else if((-v) > ((-h) << 1))
				direction = 1;
			else
				direction = 8;
		}
		else
			direction = 7;
	}
}


void gamecontrol_t::change()
{
	int lr = left - right + ul - ur + dl - dr;
	int ud = up - down + ul + ur - dl - dr;
	if(lr > 0)
	{
		if(ud > 0)
			new_direction = 8;
		else if(ud < 0)
			new_direction = 6;
		else
			new_direction = 7;
	}
	else if(lr < 0)
	{
		if(ud > 0)
			new_direction = 2;
		else if(ud < 0)
			new_direction = 4;
		else
			new_direction = 3;
	}
	else
	{
		if(ud > 0)
			new_direction = 1;
		else if(ud < 0)
			new_direction = 5;
		else
			new_direction = 0;
	}

	if(prefs->dia_emphasis)
	{
		if(!new_direction)
		{
			//Change to neutral. Cancel delayed latch!
			latch_timer = 0;
		}
		else if(!movekey_pressed)
		{
			//Change from neutral - latch immediately.
			direction = new_direction;
			latch_timer = 0;
		}
		else if(new_direction & 1)
		{
			//Change from diagonal!
			latch_timer = prefs->dia_emphasis;
		}
		else
		{
			//Change to diagonal - latch immediately.
			direction = new_direction;
			latch_timer = 0;
		}
	}
	else if(new_direction)
		direction = new_direction;

	movekey_pressed = (new_direction != 0);
	mouse_x = 0;
	mouse_y = 0;
}


void gamecontrol_t::process()
{
	if(latch_timer)
		if(!--latch_timer)
			direction = new_direction;
}
