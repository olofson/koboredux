/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2007, 2009 David Olofson
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

#ifndef XKOBO_H_ENEMIES
#define XKOBO_H_ENEMIES

#include <stdio.h>
#include "kobo.h"
#include "config.h"
#include "myship.h"
#include "manage.h"
#include "sound.h"


class KOBO_enemy;
class KOBO_enemies;
//---------------------------------------------------------------------------//
struct KOBO_enemy_kind
{
	int		score;
	void (KOBO_enemy::*make) ();
	void (KOBO_enemy::*move) ();
	void (KOBO_enemy::*kill) ();
	int		hitsize;
	int		bank, frame;
	int		layer;
	int		launchspeed;
	KOBO_sounds	sound;
	KOBO_sounds	launchsound;
	KOBO_sounds	impactsound;
	KOBO_sounds	deathsound;
};

extern const KOBO_enemy_kind greenbullet;
extern const KOBO_enemy_kind redbullet;
extern const KOBO_enemy_kind bluebullet;
extern const KOBO_enemy_kind explosion;
extern const KOBO_enemy_kind explosion3;
extern const KOBO_enemy_kind explosion4;
extern const KOBO_enemy_kind explosion5;
extern const KOBO_enemy_kind ringexpl;
extern const KOBO_enemy_kind greenbltexpl;
extern const KOBO_enemy_kind redbltexpl;
extern const KOBO_enemy_kind bluebltexpl;
extern const KOBO_enemy_kind boltexpl;
extern const KOBO_enemy_kind rockexpl;
extern const KOBO_enemy_kind enemy1;
extern const KOBO_enemy_kind enemy2;
extern const KOBO_enemy_kind enemy3;
extern const KOBO_enemy_kind enemy4;
extern const KOBO_enemy_kind enemy5;
extern const KOBO_enemy_kind enemy6;
extern const KOBO_enemy_kind enemy7;
extern const KOBO_enemy_kind bomb1;
extern const KOBO_enemy_kind bomb2;
extern const KOBO_enemy_kind bombdeto;
extern const KOBO_enemy_kind cannon;
extern const KOBO_enemy_kind pipein;
extern const KOBO_enemy_kind core;
extern const KOBO_enemy_kind pipeout;
extern const KOBO_enemy_kind rock;
extern const KOBO_enemy_kind ring;
extern const KOBO_enemy_kind enemy_m1;
extern const KOBO_enemy_kind enemy_m2;
extern const KOBO_enemy_kind enemy_m3;
extern const KOBO_enemy_kind enemy_m4;


//---------------------------------------------------------------------------//
enum KOBO_state
{
	notuse,
	reserved,
	moving
};

class KOBO_enemy
{
	cs_obj_t	*object;	// For the gfxengine connection
	KOBO_state	_state;
	const KOBO_enemy_kind *ek;
	int	x, y;			// Position
	int	h, v;			// Velocity
	int	contact;		// 0 or amount of overlap (24:8)
	int	di;			// Direction
	int	a, b, count;		// "AI" work variables
	int	soundhandle;		// Continuous positional sound fx
	int	soundtimer;		// Positional audio update timer
	int	bank, frame;		// Current sprite bank and frame
	int	health;			// Current health
	int	damage;			// Damage dealt to player at contact
	int	splash_damage;		// Splash damage dealt on death
	bool	takes_splash_damage;
	bool	shootable;		// Can be hit by player bolts
	bool	physics;		// Use physics collision responses
	bool	mapcollide;		// Tied to tile; collisions via map!
	bool	detonate_on_contact;	// Detonate on contact with player
	int	diffx, diffy, mindiff;	// Distance to player
	int	hitsize;		// Hit square/circle radius
	void move_enemy_m(int quick, int maxspeed);
	void move_enemy_template(int quick, int maxspeed);
	void move_enemy_template_2(int quick, int maxspeed);
	void move_enemy_template_3(int quick, int maxspeed);
	void launch(const KOBO_enemy_kind *ekp);
	void shot_template_8_dir(const KOBO_enemy_kind *ekp);
      public:
	 KOBO_enemy();
	inline void init();
	inline void release();
	inline void die();
	void state(KOBO_state s);
	void player_collision(int dx, int dy);
	inline void move();
	inline void move_intro();
	inline void put();
	inline int make(const KOBO_enemy_kind *k,
			int px, int py, int h1, int v1, int dir = 0);
	inline int realize();
	inline int is_pipe();
	void hit(int dmg);
	void detonate();
	inline bool can_hit_map(int px, int py);
	inline bool can_splash_damage()	{ return takes_splash_damage; }
	inline bool in_range(int px, int py, int range, int &dist);
	inline int erase_cannon(int px, int py);
	void render_hit_zone();

	void startsound();
	void playsound(KOBO_sounds si)
	{
		sound.g_play(si, CS2PIXEL(x), CS2PIXEL(y));
	}

	void kill_default();

	void make_bullet_green();
	void make_bullet_red();
	void make_bullet_blue();
	void move_bullet();
	void kill_bullet_green();
	void kill_bullet_red();
	void kill_bullet_blue();

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
	void destroy_cannon();
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
class KOBO_enemies
{
	static KOBO_enemy enemy[ENEMY_MAX];
	static KOBO_enemy *enemy_max;
	static const KOBO_enemy_kind *ekind_to_generate_1;
	static const KOBO_enemy_kind *ekind_to_generate_2;
	static int e1_interval;
	static int e2_interval;
	static int explocount;
      public:
	static int is_intro;
	static int sound_update_period;
	static int init();
	static void off();
	static void move();
	static void move_intro();
	static void restart_sounds();
	static void put();
	static void render_hit_zones();
	static int make(const KOBO_enemy_kind * ek,
			int x, int y, int h = 0, int v = 0, int di = 0);
	static const KOBO_enemy_kind *randexp();
	static int erase_cannon(int x, int y);
	static int exist_pipe();
	static void hit_map(int x, int y, int dmg);
	static void set_ekind_to_generate(const KOBO_enemy_kind * ek1, int i1,
			const KOBO_enemy_kind * ek2, int i2);
	static void splash_damage(int x, int y, int damage);
	static const KOBO_enemy_kind *ek1()
	{
		return ekind_to_generate_1;
	}
	static const KOBO_enemy_kind *ek2()
	{
		return ekind_to_generate_2;
	}
	static int eint1()
	{
		return e1_interval;
	}
	static int eint2()
	{
		return e2_interval;
	}
};

extern KOBO_enemies enemies;


inline void KOBO_enemy::init()
{
	state(notuse);
}

inline void KOBO_enemy::release()
{
	if(soundhandle)
		sound.g_stop(soundhandle);
	state(notuse);
}

inline void KOBO_enemy::die()
{
	playsound(ek->deathsound);
	release();
}

inline void KOBO_enemy::startsound()
{
	soundhandle = 0;
	if(enemies.is_intro || (_state == notuse) || !ek->sound)
		return;
	soundhandle = sound.g_start(ek->sound, CS2PIXEL(x), CS2PIXEL(y));
	soundtimer = enemies.sound_update_period;
}

inline int KOBO_enemy::make(const KOBO_enemy_kind *k, int px, int py,
		int h1, int v1, int dir)
{
	if(_state != notuse)
		return -1;

	ek = k;
	state(reserved);
	x = px;
	y = py;
	di = dir;
	h = h1;
	v = v1;
	contact = 0;
	a = 0;
	b = 0;
	count = 0;
	health = 20;
	damage = 50;
	splash_damage = 0;
	takes_splash_damage = true;
	shootable = true;
	mapcollide = false;
	physics = true;
	detonate_on_contact = false;
	hitsize = ek->hitsize;
	bank = ek->bank;
	frame = ek->frame;
	startsound();
	(this->*(ek->make)) ();
	bank = s_get_actual_bank(gengine->get_gfx(), bank);
	return 0;
}

inline void KOBO_enemy::hit(int dmg)
{
	if(HEALTH_INDESTRUCTIBLE != health)
		health -= dmg;
	if(enemies.is_intro)
	{
		if(health <= 0)
		{
			enemies.make(&explosion, x, y);
			die();
		}
		return;
	}
	else if(health > 0)
	{
		// Damaged but not destroyed!
		playsound(ek->impactsound);
		if(soundhandle)
			sound.g_control(soundhandle, 2, dmg * 0.01f);
		return;
	}

	// Enemy destroyed.
	if(splash_damage)
		enemies.splash_damage(x, y, splash_damage);
	manage.add_score(ek->score);
	(this->*(ek->kill)) ();
}

inline void KOBO_enemy::detonate()
{
	if(splash_damage)
		enemies.splash_damage(x, y, splash_damage);
	(this->*(ek->kill)) ();
}

inline bool KOBO_enemy::can_hit_map(int px, int py)
{
	if(_state == notuse)
		return false;
	if(!mapcollide)
		return false;
	return ((signed)(CS2PIXEL(x) & (WORLD_SIZEX - 1)) >> 4 == px) &&
			((signed)(CS2PIXEL(y) & (WORLD_SIZEY - 1)) >> 4 == py);
}

inline bool KOBO_enemy::in_range(int px, int py, int range, int &dist)
{
	if(_state == notuse)
		return false;
	if(mapcollide)
		return false;	// Non-fixed enemies only!
	int dx = labs(x - px);
	if(dx > PIXEL2CS(WORLD_SIZEX))
		dx = PIXEL2CS(WORLD_SIZEX) - dx;
	if(dx > range)
		return false;
	int dy = labs(y - py);
	if(dy > PIXEL2CS(WORLD_SIZEY))
		dy = PIXEL2CS(WORLD_SIZEY) - dy;
	if(dy > range)
		return false;
	dist = sqrt(dx*dx + dy*dy);
	return dist <= range;
}

inline int KOBO_enemy::erase_cannon(int px, int py)
{
	if( (ek == &cannon) && can_hit_map(px, py) )
	{
		destroy_cannon();
		return 1;
	}
	return 0;
}

inline void KOBO_enemy::move()
{
	if(_state == notuse)
		return;

	// Need to update this for stationary objects as well, as the listener
	// is (usually) moving around at all times!
	if((soundhandle > 0) && (soundtimer-- <= 0))
	{
		sound.g_move(soundhandle, CS2PIXEL(x), CS2PIXEL(y));
		soundtimer = enemies.sound_update_period;
	}

	if(_state != moving)
		return;

	x += h;
	y += v;
	x &= PIXEL2CS(WORLD_SIZEX) - 1;
	y &= PIXEL2CS(WORLD_SIZEY) - 1;
	int dx = WRAPDISTXCS(x, myship.get_csx());
	int dy = WRAPDISTYCS(y, myship.get_csy());
	diffx = CS2PIXEL(dx);
	diffy = CS2PIXEL(dy);
	mindiff = MAX(labs(diffx), labs(diffy));
	(this->*(ek->move)) ();

	// Handle collisions with the player ship
	if(!mapcollide && myship.alive() && (hitsize >= 0) &&
			(mindiff < (hitsize + myship.get_hitsize())))
		player_collision(dx, dy);
	else
		contact = 0;

	// Handle collisions with player bolts (Player bolts kill themselves
	// when they hit something, so we don't need to hit them from here.)
	if(!shootable || (mindiff >= ((VIEWLIMIT >> 1) + 8)))
		return;

	int dmg = myship.hit_bolt(CS2PIXEL(x), CS2PIXEL(y),
				hitsize + HIT_BOLT, health);
	if(dmg && shootable)
		hit(dmg);	// Bolt damages object
}

inline void KOBO_enemy::move_intro()
{
	if(_state != moving)
		return;
	x += h;
	y += v;
	x &= PIXEL2CS(WORLD_SIZEX) - 1;
	y &= PIXEL2CS(WORLD_SIZEY) - 1;
	diffx = WRAPDISTX(CS2PIXEL(x), myship.get_x());
	diffy = WRAPDISTY(CS2PIXEL(y), myship.get_y());
	mindiff = MAX(labs(diffx), labs(diffy));
	(this->*(ek->move)) ();
}

inline void KOBO_enemy::put()
{
	if(!object)
		return;
	object->point.v.x = x;
	object->point.v.y = y;
	cs_obj_image(object, bank, frame + di - 1);
}

inline int KOBO_enemy::realize()
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

inline int KOBO_enemy::is_pipe()
{
	return ((_state != notuse) && ((ek == &pipein) || (ek == &pipeout)));
}

#endif				// XKOBO_H_ENEMIES
