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
#define MIN(x,y)	(((x)<(y)) ? (x) : (y))
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
	static int di;		// Direction
	static int x, y;	// Position
	static int vx, vy;	// Velocity
	static int ax, ay;	// Acceleration
	static int _health;
	static int health_time;
	static int explo_time;
	static int nose_reload_timer;
	static int tail_reload_timer;
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
	static void handle_controls();
	static void update_position();
  public:
	 _myship();
	static int get_velx()		{ return vx; }
	static int get_vely()		{ return vy; }
	static int get_x()		{ return CS2PIXEL(x); }
	static int get_y()		{ return CS2PIXEL(y); }
	static int get_csx()		{ return x; }
	static int get_csy()		{ return y; }
	static int init();
	static void off();
	static void move();
	static int put();
	static void put_crosshair();
	static int nose_fire();
	static int tail_fire();
	static int hit_bolt(int ex, int ey, int hitsize, int health);
	static void check_base_bolts();
	static void hit(int dmg);
	static int health()		{ return _health; }
	static void health(int h)	{ _health = h; }
	static void health_bonus(int h);
	static inline int regen_next()
	{
		return (_health + game.regen_step - 1) /
				game.regen_step * game.regen_step;
	}
	static void set_position(int px, int py);
	static int alive()		{ return _state == normal; }
	static bool in_range(int px, int py, int range, int &dist);
};

extern _myship myship;

#endif // XKOBO_H_MYSHIP
