/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996 Akira Higuchi
 * Copyright (C) 2001-2003, 2007, 2009 David Olofson
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

#ifndef XKOBO_H_ENEMIES
#define XKOBO_H_ENEMIES

#include <stdio.h>
#include "kobo.h"
#include "config.h"
#include "myship.h"
#include "manage.h"
#include "sound.h"

#define ABS(x)   (((x)>=0) ? (x) : (-(x)))
#define MAX(x,y) (((x)>(y)) ? (x) : (y))

#define	HEALTH_INDESTRUCTIBLE	9000000

class _enemy;
class _enemies;
//---------------------------------------------------------------------------//
struct enemy_kind
{
	int	score;
	void (_enemy::*make) ();
	void (_enemy::*move) ();
	void (_enemy::*kill) ();
	int	hitsize;
	int	bank, frame;
	int	layer;
};


extern const enemy_kind beam;
extern const enemy_kind explosion;
extern const enemy_kind explosion3;
extern const enemy_kind explosion4;
extern const enemy_kind explosion5;
extern const enemy_kind ringexpl;
extern const enemy_kind beamexpl;
extern const enemy_kind boltexpl;
extern const enemy_kind rockexpl;
extern const enemy_kind enemy1;
extern const enemy_kind enemy2;
extern const enemy_kind enemy3;
extern const enemy_kind enemy4;
extern const enemy_kind enemy5;
extern const enemy_kind enemy6;
extern const enemy_kind enemy7;
extern const enemy_kind bomb1;
extern const enemy_kind bomb2;
extern const enemy_kind bombdeto;
extern const enemy_kind cannon;
extern const enemy_kind pipein;
extern const enemy_kind core;
extern const enemy_kind pipeout;
extern const enemy_kind rock;
extern const enemy_kind ring;
extern const enemy_kind enemy_m1;
extern const enemy_kind enemy_m2;
extern const enemy_kind enemy_m3;
extern const enemy_kind enemy_m4;


//---------------------------------------------------------------------------//
enum _state_t
{
	notuse,
	reserved,
	moving
};

class _enemy
{
	cs_obj_t	*object;	/* For the gfxengine connection */
	_state_t	_state;
	const enemy_kind *ek;
	int		x, y;
	int		h, v;
	int		di;
	int		a, b;
	int		count;
	int		health;
	int		damage;
	int		shootable;
	int		diffx, diffy;
	int		norm;
	int		hitsize;
	int		bank, frame;
	void hit(int dmg);
	void move_enemy_m(int quick, int maxspeed);
	void move_enemy_template(int quick, int maxspeed);
	void move_enemy_template_2(int quick, int maxspeed);
	void move_enemy_template_3(int quick, int maxspeed);
	void shot_template(const enemy_kind * ekp,
			int shift, int rand_num, int maxspeed);
	void shot_template_8_dir(const enemy_kind * ekp);
      public:
	 _enemy();
	inline void init();
	inline void release();
	void state(_state_t s);
	inline void move();
	inline void move_intro();
	inline void put();
	inline int make(const enemy_kind * k,
			int px, int py, int h1, int v1, int dir = 0);
	inline int realize();
	inline int is_pipe();
	inline int erase_cannon(int px, int py);

	void kill_default();

	void make_beam();
	void move_beam();
	void kill_beam();

	void make_rock();
	void move_rock();
	void kill_rock();

	void make_ring();
	void move_ring();
	void kill_ring();

	void make_bomb();
	void move_bomb1();
	void move_bomb2();

	void make_expl();
	void move_expl();

	void make_cannon();
	void move_cannon();
	void kill_cannon();

	void make_core();
	void move_core();
	void kill_core();

	void make_pipein();
	void move_pipein();

	void make_pipeout();
	void move_pipeout();

	void make_enemy1();
	void move_enemy1();

	void make_enemy2();
	void move_enemy2();

	void make_enemy3();
	void move_enemy3();

	void make_enemy4();
	void move_enemy4();

	void make_enemy();
	void move_enemy5();
	void move_enemy6();
	void move_enemy7();

	void make_enemy_m1();
	void move_enemy_m1();

	void make_enemy_m2();
	void move_enemy_m2();

	void make_enemy_m3();
	void move_enemy_m3();

	void make_enemy_m4();
	void move_enemy_m4();
};

//---------------------------------------------------------------------------//
class _enemies
{
	static _enemy enemy[ENEMY_MAX];
	static _enemy *enemy_max;
	static const enemy_kind *ekind_to_generate_1;
	static const enemy_kind *ekind_to_generate_2;
	static int e1_interval;
	static int e2_interval;
	static int explocount;
      public:
	static int is_intro;
	static int init();
	static void off();
	static void move();
	static void move_intro();
	static void put();
	static int make(const enemy_kind * ek,
			int x, int y, int h = 0, int v = 0, int di = 0);
	static const enemy_kind *randexp();
	static int erase_cannon(int x, int y);
	static int exist_pipe();
	static void set_ekind_to_generate(const enemy_kind * ek1, int i1,
			const enemy_kind * ek2, int i2);
	static inline const enemy_kind *ek1()
	{
		return ekind_to_generate_1;
	}
	static inline const enemy_kind *ek2()
	{
		return ekind_to_generate_2;
	}
	static inline int eint1()
	{
		return e1_interval;
	}
	static inline int eint2()
	{
		return e2_interval;
	}
};

extern _enemies enemies;


inline void _enemy::init()
{
	release();
}

inline void _enemy::release()
{
	state(notuse);
}

inline int _enemy::make(const enemy_kind * k, int px, int py,
		int h1, int v1, int dir)
{
	if(_state != notuse)
		return -1;

	ek = k;
	state(reserved);
	x = PIXEL2CS(px);
	y = PIXEL2CS(py);
	di = dir;
	h = h1;
	v = v1;
	a = 0;
	b = 0;
	count = 0;
	health = 20;
	damage = 50;
	shootable = 1;
	hitsize = ek->hitsize;
	bank = ek->bank;
	frame = ek->frame;
	(this->*(ek->make)) ();
	return 0;
}

inline void _enemy::hit(int dmg)
{
	if(HEALTH_INDESTRUCTIBLE != health)
		health -= dmg;
	if(enemies.is_intro)
	{
		if(health <= 0)
		{
			enemies.make(&explosion, CS2PIXEL(x), CS2PIXEL(y));
			release();
		}
		return;
	}
	else if(health > 0)
	{
		if(ek == &rock)
			sound.g_bolt_hit_rock(x, y);
		else
			sound.g_bolt_hit(x, y);
		return;
	}

	manage.add_score(ek->score);
	(this->*(ek->kill)) ();
}

inline int _enemy::erase_cannon(int px, int py)
{
	if( (_state != notuse) && (ek == &cannon)
			&& ((signed)(CS2PIXEL(x) & (WORLD_SIZEX - 1)) >> 4 == px)
			&& ((signed)(CS2PIXEL(y) & (WORLD_SIZEY - 1)) >> 4 == py)
	  )
	{
		release();
		return 1;
	}
	return 0;
}

inline void _enemy::move()
{
	if(_state != moving)
		return;
	x += h;
	y += v;
	diffx = CS2PIXEL(x) - myship.get_x();
	diffy = CS2PIXEL(y) - myship.get_y();

	if(diffx > (WORLD_SIZEX >> 1))
	{
		diffx -= WORLD_SIZEX;
		x -= PIXEL2CS(WORLD_SIZEX);
	}
	if(diffx < -(WORLD_SIZEX >> 1))
	{
		diffx += WORLD_SIZEX;
		x += PIXEL2CS(WORLD_SIZEX);
	}
	if(diffy > (WORLD_SIZEY >> 1))
	{
		diffy -= WORLD_SIZEY;
		y -= PIXEL2CS(WORLD_SIZEY);
	}
	if(diffy < -(WORLD_SIZEY >> 1))
	{
		diffy += WORLD_SIZEY;
		y += PIXEL2CS(WORLD_SIZEY);
	}

	norm = MAX(ABS(diffx), ABS(diffy));
	(this->*(ek->move)) ();

	// Handle collisions with the player ship
	if((hitsize >= 0) && (norm < (hitsize + HIT_MYSHIP)))
	{
		if(prefs->cmd_indicator)
			sound.g_player_damage();
		else if(myship.alive())
		{
			hit(game.damage);	// Ship damages object
			myship.hit(damage);	// Object damages ship
		}
	}

	// Handle collisions with player bolts (Player bolts kill themselves
	// when they hit something, so we don't need to hit them from here.)
	if(!shootable || (norm >= ((VIEWLIMIT >> 1) + 8)))
		return;

	int dmg = myship.hit_bolt(CS2PIXEL(x), CS2PIXEL(y),
				hitsize + HIT_BOLT, health);
	if(dmg)
	{
		if(prefs->cmd_indicator)
			sound.g_player_damage();
		else
			hit(dmg);	// Bolt damages object
	}
}

inline void _enemy::move_intro()
{
	if(_state != moving)
		return;
	x += h;
	y += v;
	diffx = CS2PIXEL(x) - myship.get_x();
	diffy = CS2PIXEL(y) - myship.get_y();

	if(diffx > (WORLD_SIZEX >> 1))
	{
		diffx -= WORLD_SIZEX;
		x -= PIXEL2CS(WORLD_SIZEX);
	}
	if(diffx < -(WORLD_SIZEX >> 1))
	{
		diffx += WORLD_SIZEX;
		x += PIXEL2CS(WORLD_SIZEX);
	}
	if(diffy > (WORLD_SIZEY >> 1))
	{
		diffy -= WORLD_SIZEY;
		y -= PIXEL2CS(WORLD_SIZEY);
	}
	if(diffy < -(WORLD_SIZEY >> 1))
	{
		diffy += WORLD_SIZEY;
		y += PIXEL2CS(WORLD_SIZEY);
	}

	norm = MAX(ABS(diffx), ABS(diffy));
	(this->*(ek->move)) ();
}

inline void _enemy::put()
{
	if(!object)
		return;
	object->point.v.x = x;
	object->point.v.y = y;
	cs_obj_image(object, bank, frame + di - 1);
}

inline int _enemy::realize()
{
	if(_state == reserved)
	{
		state(moving);
		if(object)
		{
			object->point.v.x = x;
			object->point.v.y = y;
			cs_point_force(&object->point);
		}
	}
	return (_state == moving);
}

inline int _enemy::is_pipe()
{
	return ((_state != notuse) && ((ek == &pipein) || (ek == &pipeout)));
}


#endif				// XKOBO_H_ENEMIES
