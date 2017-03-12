/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996  Akira Higuchi
 * Copyright 2001, 2003, 2007, 2009 David Olofson
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

#ifndef XKOBO_H_MYSHIP
#define XKOBO_H_MYSHIP

#include "config.h"
#include "gfxengine.h"
#include "game.h"
#include "gamectl.h"

enum KOBO_myship_state
{
	SHIP_NORMAL,
	SHIP_SHIELD,
	SHIP_INVULNERABLE,
	SHIP_DEAD
};

struct KOBO_player_bolt
{
	int x, y;
	int dx, dy;
	int dir, state;
	cs_obj_t *object;
};

class KOBO_myship
{
	static KOBO_myship_state _state;
	static int shield_timer;
	static KOBO_player_controls ctrl;
	static int di;		// Direction (1: N, 2: NE, 3: W etc)
	static int fdi;		// Filtered direction (sprite frames, 24:8)
	static int dframes;	// Number of sprite rotation frames
	static int x, y;	// Position
	static int vx, vy;	// Velocity
	static int ax, ay;	// Acceleration
	static int hitsize;
	static int _health;
	static int _charge;	// Weapon boost capacitor charge
	static float charge_blipp_granularity;
	static int charge_cool;	// Charge cooldown timer
	static int health_time;
	static int explo_time;
	static int nose_reload_timer;
	static int tail_reload_timer;
	static KOBO_player_bolt bolts[MAX_BOLTS];

	// For the gfxengine connection
	static cs_obj_t *object;

	static void shot_single(float dir, int loffset, int hoffset,
			int speed = 65536);
	static void charged_fire(int dir);
	static void apply_position();
	static void explode();
	static void fire_control();
	static void handle_controls();
	static void update_position();
	static void kill_bolt(int bolt, bool impact);
  public:
	KOBO_myship();
	static KOBO_player_controls decode_input();
	static void control(KOBO_player_controls c)	{ ctrl = c; }
	static void state(KOBO_myship_state s);
	static int get_velx()		{ return vx; }
	static int get_vely()		{ return vy; }
	static int get_x()		{ return CS2PIXEL(x); }
	static int get_y()		{ return CS2PIXEL(y); }
	static int get_csx()		{ return x; }
	static int get_csy()		{ return y; }
	static int get_hitsize()	{ return hitsize; }
	static void impulse(int ix, int iy)
	{
		vx += ix;
		vy += iy;
	}
	static void init(bool newship);		// New game or new stage
	static void init(int rhealth, int rcharge);	// Replay init
	static void off();
	static void move();
	static int put();
	static void force_position();
	static void render();
	static void nose_fire();
	static void tail_fire();
	static int hit_bolt(int ex, int ey, int hitsize, int health);
	static void check_base_bolts();
	static void hit(int dmg);
	static int health()		{ return _health; }
	static void health_bonus(int h);
	static int regen_next()
	{
		return (_health + game.regen_step - 1) /
				game.regen_step * game.regen_step;
	}
	static int charge()		{ return _charge; }
	static void set_position(int px, int py);
	static int alive()		{ return _state != SHIP_DEAD; }
	static bool in_range(int px, int py, int range, int &dist);
	static inline int bolt_frame(int dir, int frame)
	{
		const char animtab[8] = { 0, 1, 2, 3, 2, 1, 2, 1 };
		if(frame < 4)
			return (dir - 1) * 4 + frame;
		else
			return 32 + ((dir - 1) & 3) * 8 +
					animtab[(frame - 4) & 7];
	}
};

extern KOBO_myship myship;

#endif // XKOBO_H_MYSHIP
