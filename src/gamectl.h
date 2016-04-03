/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001, 2002, 2006, 2009 David Olofson
 * Copyright 2015-2016 David Olofson (Kobo Redux)
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

#ifndef _KOBO_GAMECTL_H_
#define _KOBO_GAMECTL_H_

#include "SDL.h"
#include "config.h"


enum gc_targets_t
{
	BTN_NONE = -1,

	BTN_UP = 0,		//Joystick, arrows, numpad etc
	BTN_DOWN,
	BTN_LEFT,
	BTN_RIGHT,
	BTN_UL,
	BTN_UR,
	BTN_DL,
	BTN_DR,

	BTN_INC,	// +
	BTN_DEC,	// -

	BTN_NEXT,	// PageUp
	BTN_PREV,	// PageDn

	BTN_YES,	// 'Y'
	BTN_NO,		// 'N'

	BTN_FIRE,	//ctrl, fire button, mouse button,...
	BTN_START,	//Space
	BTN_SELECT,	//Return, enter,...
	BTN_EXIT,	//ESC
	BTN_PAUSE,	//Pause or P; also used as an internal event.
	BTN_CLOSE,	//Window close button, ALT-F4,...
	BTN_BACK,	//Backspace

	BTN__COUNT
};


enum gc_mousemodes_t
{
	MMD_OFF = 0,
	MMD_CROSSHAIR,
	MMD_RELATIVE
};


enum gc_sources_t
{
	GC_SRC_JOYSTICK = 0,
	GC_SRC_MOUSE,
	GC_SRC_KEY0,
	GC_SRC_KEY1,
	GC_SRC_KEY2,
	GC_SRC_KEY3,
	GC_SRC__COUNT
};


class gamecontrol_t
{
	static unsigned state[BTN__COUNT];	// Current state
	static unsigned _pressed[BTN__COUNT];	// Pressed during this frame
	static unsigned _released[BTN__COUNT];	// Released during this frame
	static int direction, new_direction;
	static int latch_timer;
	static int movekey_pressed;
	static void change();
	static gc_targets_t mapsrc(SDL_Keysym sym, int &src);
  public:
	gamecontrol_t();
	static void init();
	static void clear();
	static gc_targets_t map(SDL_Keysym sym)
	{
		int src;
		return mapsrc(sym, src);
	}
	static void pressbtn(gc_targets_t b, gc_sources_t s);
	static void releasebtn(gc_targets_t b, gc_sources_t s);
	static void press(SDL_Keysym sym);
	static void release(SDL_Keysym sym);
	static void mouse_press(int n);
	static void mouse_release(int n);
	static void mouse_position(int h, int v);
	static void post_process(); // Call after processing input!
	static int dir()			{ return direction; }
	static int dir_push()			{ return movekey_pressed; }
	static bool down(gc_targets_t b)	{ return state[b]; }
	static bool pressed(gc_targets_t b)	{ return _pressed[b]; }
	static bool released(gc_targets_t b)	{ return _released[b]; }
	static int fire()
	{
		return down(BTN_FIRE) || pressed(BTN_FIRE);
	}
	static void reset();	// Reset the pressed()/released() state
};

extern gamecontrol_t gamecontrol;

#endif //_KOBO_KEY_H_
