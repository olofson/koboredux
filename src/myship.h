/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996  Akira Higuchi
 * Copyright (C) 2001, 2003, 2007, 2009 David Olofson
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

#ifndef XKOBO_H_MYSHIP
#define XKOBO_H_MYSHIP

#include "config.h"
#include "gfxengine.h"
#include "game.h"

#define ABS(x)		(((x)>=0) ? (x) : (-(x)))
#define MAX(x,y)	(((x)>(y)) ? (x) : (y))

//---------------------------------------------------------------------------//
enum _myship_state
{
	normal,
	dead
};

class _myship
{
	static _myship_state _state;
	static int di;		// direction
	static int x, y;
	static int vx, vy;
	static int _health;
	static int health_time;
	static int explo_time;
	static int nose_reload_timer;
	static int nose_temperature;
	static int nose_alt;
	static int tail_reload_timer;
	static int tail_temperature;
	static int tail_alt;
	static int boltx[MAX_BOLTS], bolty[MAX_BOLTS];
	static int boltdx[MAX_BOLTS], boltdy[MAX_BOLTS];
	static int boltst[MAX_BOLTS];

	// For the gfxengine connection
	static cs_obj_t *object;
	static cs_obj_t *bolt_objects[MAX_BOLTS];
	static cs_obj_t *crosshair;

	static void state(_myship_state s);
	static void shot_single(int i, int dir, int offset);
	static void apply_position();
	static void explode();
	static void move_classic();
	static void move_redux();
  public:
	 _myship();
	static inline int get_x()
	{
		return CS2PIXEL(x);
	}
	static inline int get_y()
	{
		return CS2PIXEL(y);
	}
	static inline int get_virtx()
	{
		return x - PIXEL2CS(WMAIN_W / 2);
	}
	static inline int get_virty()
	{
		return y - PIXEL2CS(WMAIN_H / 2);
	}
	static inline int get_nose_temp()
	{
		return nose_temperature;
	}
	static inline int get_tail_temp()
	{
		return tail_temperature;
	}
	static int init();
	static void off();
	static int move();
	static int put();
	static void put_crosshair();
	static int xkobo_shot();
	static int nose_fire();
	static int tail_fire();
	static int hit_structure();
	static int hit_bolt(int ex, int ey, int hitsize, int health);
	static void hit(int dmg);
	static int health()		{ return _health; }
	static void health(int h)	{ _health = h; }
	static void health_bonus(int h);
	static void set_position(int px, int py);
	static int alive()		{ return _state == normal; }
};

extern _myship myship;

#endif // XKOBO_H_MYSHIP
