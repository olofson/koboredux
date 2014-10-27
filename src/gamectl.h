/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996 Akira Higuchi
 * Copyright (C) 2001, 2002, 2006, 2009 David Olofson
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

#include "glSDL.h"
#include "config.h"

#define KEY_KP_DOWN		SDLK_KP2
#define KEY_KP_LEFT		SDLK_KP4
#define KEY_KP_UP		SDLK_KP8
#define KEY_KP_RIGHT		SDLK_KP6
#define KEY_KP_DL		SDLK_KP1
#define KEY_KP_DR		SDLK_KP3
#define KEY_KP_UL		SDLK_KP7
#define KEY_KP_UR		SDLK_KP9


enum buttons_t
{
	BTN_UP,		//Joystick, arrows, numpad etc
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

	BTN_F1,		//Function keys
	BTN_F2,
	BTN_F3,
	BTN_F4,

	BTN_F5,
	BTN_F6,
	BTN_F7,
	BTN_F8,

	BTN_F9,
	BTN_F10,
	BTN_F11,
	BTN_F12
};


enum mousemodes_t
{
	MMD_OFF = 0,
	MMD_CROSSHAIR,
	MMD_RELATIVE
};


class gamecontrol_t
{
	static int r_delay, r_interval;
	static int afire;
	static int space;
	static int left, up, down, right, ul, ur, dl, dr;
	static int shot;
	static int direction, new_direction;
	static int latch_timer;
	static int movekey_pressed;
	static void change();
  public:
	gamecontrol_t();
	static void init(int always_fire);
	static void repeat(int delay, int interval);
	static void clear();
	static int map(SDLKey sym);
	static void process();	// Call every frame!
	static void press(int k);
	static void release(int k);
	static void mouse_press(int n);
	static void mouse_release(int n);
	static void mouse_position(int h, int v);
	static inline int dir()		{ return direction; }
	static inline int get_shot()	{ return shot || afire; }
	static inline int dir_push()	{ return movekey_pressed; }
};

extern gamecontrol_t gamecontrol;

#endif //_KOBO_KEY_H_
