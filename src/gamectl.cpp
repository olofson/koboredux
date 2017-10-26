/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2006, 2009, 2012 David Olofson
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

#include "manage.h"
#include "kobo.h"
#include "gamectl.h"
#include "myship.h"
#include "mathutil.h"


int gamecontrol_t::direction = 1;
int gamecontrol_t::new_direction = 0;
int gamecontrol_t::turret_dir = 0;
int gamecontrol_t::latch_timer = 0;
bool gamecontrol_t::movekey_pressed = false;
bool gamecontrol_t::key_sprint = false;
bool gamecontrol_t::mouse_sprint = false;
bool gamecontrol_t::mouse_muted = false;
unsigned gamecontrol_t::state[BTN__COUNT];
unsigned gamecontrol_t::_pressed[BTN__COUNT];
unsigned gamecontrol_t::_released[BTN__COUNT];


void gamecontrol_t::init()
{
	memset(state, 0, sizeof(state));
	memset(_pressed, 0, sizeof(_pressed));
	memset(_released, 0, sizeof(_released));
	movekey_pressed = key_sprint = mouse_sprint = mouse_muted = false;
	latch_timer = 0;
}


gamecontrol_t::gamecontrol_t()
{
	init();
}


void gamecontrol_t::clear()
{
	direction = 1;
	turret_dir = 0;
	movekey_pressed = key_sprint = mouse_sprint = mouse_muted = false;
}


void gamecontrol_t::mouse_mute(bool m)
{
	mouse_muted = m;
	if(prefs->mouse && mouse_muted)
	{
		// Only clear state if actually using mouse input, because this
		// also kills keyboard/joystick state, which is annoying when
		// navigating through a campaign replay.
		direction = 1;
		turret_dir = 0;
		mouse_sprint = false;
	}
}


gc_targets_t gamecontrol_t::mapsrc(SDL_Keysym sym, int &src)
{
	// Configured key bindings
	src = GC_SRC_KEY0;
	if(sym.scancode == prefs->keyboard_up)
		return BTN_UP;
	if(sym.scancode == prefs->keyboard_down)
		return BTN_DOWN;
	if(sym.scancode == prefs->keyboard_left)
		return BTN_LEFT;
	if(sym.scancode == prefs->keyboard_right)
		return BTN_RIGHT;
	if(sym.scancode == prefs->keyboard_primary)
		return BTN_PRIMARY;
	if(sym.scancode == prefs->keyboard_secondary)
		return BTN_SECONDARY;
	if(sym.scancode == prefs->keyboard_tertiary)
		return BTN_TERTIARY;

	// Hardwired key bindings
	++src;
	switch(sym.sym)
	{
	  // Directions
	  case SDLK_LEFT:
		return BTN_LEFT;
	  case SDLK_RIGHT:
		return BTN_RIGHT;
	  case SDLK_UP:
		return BTN_UP;
	  case SDLK_DOWN:
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

	  // Exit
	  case SDLK_BACKSPACE:
		++src;
	  case SDLK_ESCAPE:
		return BTN_EXIT;

	  // Pause
	  case SDLK_PAUSE:
		++src;
	  case SDLK_p:
		return BTN_PAUSE;

	  // Select
	  case SDLK_SPACE:
		++src;
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
#if 0
	if(mouse_muted && (s == GC_SRC_MOUSE))
		return;
#endif
	if(b < 0 || b >= BTN__COUNT)
		return;
	if(s < 0 || s > 31)
		return;
	if(state[b] & (1 << s))
		return;	// Filter out keyboard repeat!
	_pressed[b] |= 1 << s;
	state[b] |= 1 << s;
	gamecontrol_t::change();
}


void gamecontrol_t::releasebtn(gc_targets_t b, gc_sources_t s)
{
#if 0
	if(mouse_muted && (s == GC_SRC_MOUSE))
		return;
#endif
	if(b < 0 || b >= BTN__COUNT)
		return;
	if(s < 0 || s > 31)
		return;
	_released[b] |= 1 << s;
	state[b] &= ~(1 << s);
	gamecontrol_t::change();
}


void gamecontrol_t::mouse_position(int h, int v)
{
	if(mouse_muted)
		return;

	switch(prefs->mousemode)
	{
	  case MMD_SHIP:
	  {
		// Determine new heading for the ship
		int newdir = speed2dir(h, v, 8);

		// Determine whether or not we're "pushing". Note that we need
		// to "blip" movekey_pressed for every change regardless, as
		// the ship will not latch new directions otherwise!
		mouse_sprint = (v * v + h * h >=
				prefs->mouse_threshold *
				prefs->mouse_threshold ||
				newdir != direction);

		direction = newdir;
		turret_dir = (direction - 1) * AIM_RESOLUTION / 8;
		break;
	  }
	  case MMD_TURRET:
	  {
		// Determine direction of the turret
		int newdir = speed2dir(h, v, AIM_RESOLUTION);
		if(newdir)
			turret_dir = newdir - 1;
		break;
	  }
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

	if(new_direction)
		movekey_pressed = true;

	key_sprint = (new_direction != 0);
}


bool gamecontrol_t::dir_push()
{
	return movekey_pressed || key_sprint ||
			(prefs->mouse && (!prefs->mouse_threshold ||
			mouse_sprint));
}


void gamecontrol_t::reset_flanks()
{
	memset(_pressed, 0, sizeof(_pressed));
	memset(_released, 0, sizeof(_released));
}


void gamecontrol_t::frame()
{
	if(latch_timer)
		if(!--latch_timer)
			direction = new_direction;
	movekey_pressed = false;
	reset_flanks();
}
