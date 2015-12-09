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
int gamecontrol_t::direction = 1;
int gamecontrol_t::new_direction = 0;
int gamecontrol_t::latch_timer = 0;
int gamecontrol_t::movekey_pressed;
unsigned gamecontrol_t::state[BTN__COUNT];


void gamecontrol_t::init(int always_fire)
{
	afire = always_fire;
	memset(state, 0, sizeof(state));
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


gc_targets_t gamecontrol_t::mapsrc(SDL_Keysym sym, int &src)
{
//FIXME: This should be replaced by a configurable mapping system.
	src = GC_SRC_KEY0;
	switch(sym.sym)
	{
	  // Directions
	  case SDLK_KP_4:
		++src;
	  case SDLK_LEFT:
		++src;
	  case SDLK_a:		// Qwerty + Dvorak
		return BTN_LEFT;
	  case SDLK_KP_6:
		++src;
	  case SDLK_RIGHT:
		++src;
	  case SDLK_d:		// Qwerty
		++src;
	  case SDLK_e:		// Dvorak
		return BTN_RIGHT;
	  case SDLK_KP_8:
		++src;
	  case SDLK_UP:
		++src;
	  case SDLK_w:		// Qwerty
		++src;
	  case SDLK_COMMA:	// Some Swedish Dvorak variants
		++src;
	  case SDLK_LESS:	// US Dvorak?
		return BTN_UP;
	  case SDLK_KP_2:
		++src;
	  case SDLK_DOWN:
		++src;
	  case SDLK_s:		// Qwerty
		++src;
	  case SDLK_o:		// Dvorak
		return BTN_DOWN;

	  // Keypad diagonals
	  case SDLK_KP_1:
		return BTN_DL;
	  case SDLK_KP_3:
		return BTN_DR;
	  case SDLK_KP_7:
		return BTN_UL;
	  case SDLK_KP_9:
		return BTN_UR;

	  // Keypad diagonals with some broken keymaps...
	  case SDLK_PAGEUP:
		++src;
		if(prefs->broken_numdia)
			return BTN_UR;
		else
			return BTN_NEXT;
	  case SDLK_PAGEDOWN:
		++src;
		if(prefs->broken_numdia)
			return BTN_DR;
		else
			return BTN_PREV;
	  case SDLK_HOME:
		++src;
		if(prefs->broken_numdia)
			return BTN_UL;
		else
			return BTN_NONE;
	  case SDLK_END:
		++src;
		if(prefs->broken_numdia)
			return BTN_DL;
		else
			return BTN_NONE;

	  // Fire
	  case SDLK_LSHIFT:
		++src;
	  case SDLK_RSHIFT:
		++src;
	  case SDLK_LCTRL:
		++src;
	  case SDLK_RCTRL:
		return BTN_FIRE;

	  // Exit
	  case SDLK_ESCAPE:
		return BTN_EXIT;

	  // Pause
	  case SDLK_PAUSE:
		++src;
	  case SDLK_p:
		return BTN_PAUSE;

	  // Start
	  case SDLK_SPACE:
		return BTN_START;

	  // Select
	  case SDLK_KP_ENTER:
		++src;
	  case SDLK_RETURN:
		return BTN_SELECT;

	  // GUI navigation and editing
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

	  // Unmapped!
	  default:
		return BTN_NONE;
	}
}


void gamecontrol_t::press(SDL_Keysym sym)
{
	int src;
	gc_targets_t tgt = mapsrc(sym, src);
	if(tgt != BTN_NONE)
		pressbtn(tgt, (gc_sources_t)src);
}


void gamecontrol_t::release(SDL_Keysym sym)
{
	int src;
	gc_targets_t tgt = mapsrc(sym, src);
	if(tgt != BTN_NONE)
		releasebtn(tgt, (gc_sources_t)src);
}


void gamecontrol_t::pressbtn(gc_targets_t b, gc_sources_t s)
{
	if(b < 0 || b >= BTN__COUNT)
		return;
	if(s < 0 || s > 31)
		return;
	state[b] |= 1 << s;
	gamecontrol_t::change();
}


void gamecontrol_t::releasebtn(gc_targets_t b, gc_sources_t s)
{
	if(b < 0 || b >= BTN__COUNT)
		return;
	if(s < 0 || s > 31)
		return;
	state[b] &= ~(1 << s);
	gamecontrol_t::change();
}


void gamecontrol_t::mouse_press(int n)
{
	if(n == 1)
		pressbtn(BTN_FIRE, GC_SRC_MOUSE);
#if 0
	else if(n == 3)
		manage.key_down(KEY_START);
#endif
}


void gamecontrol_t::mouse_release(int n)
{
	if(n == 1)
		releasebtn(BTN_FIRE, GC_SRC_MOUSE);
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
		// Insert delta pos sensitivity filter here
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
	int left = state[BTN_LEFT] != 0;
	int up = state[BTN_UP] != 0;
	int down = state[BTN_DOWN] != 0;
	int right = state[BTN_RIGHT] != 0;
	int ul = state[BTN_UL] != 0;
	int ur = state[BTN_UR] != 0;
	int dl = state[BTN_DL] != 0;
	int dr = state[BTN_DR] != 0;
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
