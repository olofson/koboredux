/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2007, 2009 David Olofson
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

#ifndef XKOBO_H_ENEMIES
#define XKOBO_H_ENEMIES

#include <stdio.h>
#include "kobo.h"
#include "config.h"
#include "myship.h"
#include "manage.h"
#include "sound.h"

#define	KOBO_ALLENEMYKINDS			\
	KOBO_DEFS(BULLET1, bullet1)		\
	KOBO_DEFS(BULLET2, bullet2)		\
	KOBO_DEFS(BULLET3, bullet3)		\
	KOBO_DEFS(RINGEXPL, ringexpl)		\
	KOBO_DEFS(GREENBLTEXPL, greenbltexpl)	\
	KOBO_DEFS(REDBLTEXPL, redbltexpl)	\
	KOBO_DEFS(BLUEBLTEXPL, bluebltexpl)	\
	KOBO_DEFS(BOLTEXPL, boltexpl)		\
	KOBO_DEFS(ROCKEXPL, rockexpl)		\
	KOBO_DEFS(ENEMY1, enemy1)		\
	KOBO_DEFS(ENEMY2, enemy2)		\
	KOBO_DEFS(ENEMY3, enemy3)		\
	KOBO_DEFS(ENEMY4, enemy4)		\
	KOBO_DEFS(ENEMY5, enemy5)		\
	KOBO_DEFS(ENEMY6, enemy6)		\
	KOBO_DEFS(ENEMY7, enemy7)		\
	KOBO_DEFS(BOMB1, bomb1)			\
	KOBO_DEFS(BOMB2, bomb2)			\
	KOBO_DEFS(CANNON, cannon)		\
	KOBO_DEFS(PIPEIN, pipein)		\
	KOBO_DEFS(CORE, core)			\
	KOBO_DEFS(PIPEOUT, pipeout)		\
	KOBO_DEFS(ROCK, rock)			\
	KOBO_DEFS(RING, ring)			\
	KOBO_DEFS(ENEMY_M1, enemy_m1)		\
	KOBO_DEFS(ENEMY_M2, enemy_m2)		\
	KOBO_DEFS(ENEMY_M3, enemy_m3)		\
	KOBO_DEFS(ENEMY_M4, enemy_m4)

// KOBO_enemy_kinds
#define	KOBO_DEFS(x, y)	KOBO_EK_##x,
enum KOBO_enemy_kinds
{
	KOBO_ALLENEMYKINDS
	KOBO_EK__COUNT
};
#undef	KOBO_DEFS

class KOBO_enemy;
class KOBO_enemies;
//---------------------------------------------------------------------------//
struct KOBO_enemy_kind
{
	KOBO_enemy_kinds eki;
	int		score;
	void (KOBO_enemy::*make) ();
	void (KOBO_enemy::*move) ();
	void (KOBO_enemy::*kill) ();
	int		hitsize;
	int		bank, frame;	// (Logical) bank, anim base frame
	int		layer;
	int		launchspeed;
	KOBO_sounds	sound;		// Continuous sound fx program
	KOBO_sounds	launchsound;	// Launch/fire/spawn
	KOBO_sounds	damagesound;	// Impact/damage
	KOBO_sounds	deathsound;	// Death/failure
	KOBO_ParticleFX	deathpfx;	// Death particle effect
	bool is_bullet() const { return layer == LAYER_BULLETS; }
};

#define KOBO_EK_FX(x)		\
	S_##x,			\
	S_##x##_LAUNCH,		\
	S_##x##_DAMAGE,		\
	S_##x##_DEATH,		\
	KOBO_PFX_##x

// KOBO_enemy_kind externs
#define	KOBO_DEFS(x, y)	extern const KOBO_enemy_kind y;
KOBO_ALLENEMYKINDS
#undef	KOBO_DEFS

struct KOBO_enemystats
{
	uint32_t	spawned;
	uint32_t	killed;
	uint32_t	health;
	uint32_t	damage;
};

//---------------------------------------------------------------------------//
class KOBO_enemy
{
	friend class KOBO_enemies;
	KOBO_enemy	*next;
	cs_obj_t	*object;	// For the gfxengine connection
	const KOBO_enemy_kind	*ek;	// NOTE: NULL if enemy is dead!
	int	x, y;			// Position
	int	h, v;			// Velocity
	int	contact;		// 0 or amount of overlap (24:8)
	int	di;			// Direction
	int	a, b, c;		// "AI" work variables
	int	soundhandle;		// Continuous positional sound fx
	int	soundtimer;		// Positional audio update timer
	int	logical_bank;		// Logical bank. (Use set_bank()!)
	int	actual_bank;		// Actual bank, after aliases etc.
	int	frame;			// Base frame for animations
	int	frames;			// Number of frames in sprite bank
	int	health;			// Current health
	int	damage;			// Damage dealt to player at contact
	int	splash_damage;		// Splash damage dealt on death
	bool	takes_splash_damage;	// Can receive splash damage
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
	void explode();
      public:
	KOBO_enemy();
	inline void init(const KOBO_enemy_kind *k,
			int px, int py, int h1, int v1, int dir = 0);
	inline void release();
	inline void die();
	void player_collision(int dx, int dy);
	inline void move();
	inline void move_intro();
	inline void put();
	inline void force_position();
	inline void set_bank(int new_bank);
	inline int is_pipe();
	void hit(int dmg);
	void detonate();
	inline bool can_hit_map(int px, int py);
	inline bool can_splash_damage()	{ return takes_splash_damage; }
	inline bool in_range(int px, int py, int range, int &dist);
	inline int erase_cannon(int px, int py);
	void render_hit_zone();

	inline void playsound(KOBO_sounds si);
	inline void startsound(KOBO_sounds si);
	inline void controlsound(unsigned ctrl, float val);
	inline void stopsound();
	void detachsound();
	void restartsound();

	void kill_default();	// Default explosion anim + sfx
	void kill_silent();	// No explosion, no sfx
	void kill_unused();	// Not to be called. (Log warning.)

	void make_bullet1();
	void make_bullet2();
	void make_bullet3();
	void move_bullet();
	void kill_bullet1();
	void kill_bullet2();
	void kill_bullet3();

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
	static KOBO_enemy *active;
	static KOBO_enemy *pool;
	static const KOBO_enemy_kind *ekind_to_generate_1;
	static const KOBO_enemy_kind *ekind_to_generate_2;
	static int e1_interval;
	static int e2_interval;
	static inline KOBO_enemy *next(KOBO_enemy *current)
	{
		if(!current)
			current = active;
		else
			current = current->next;
		while(current && !current->ek)
			current = current->next;
		return current;
	}
	static void clean();
      public:
	static int is_intro;
	static int sound_update_period;
	static KOBO_enemystats stats[KOBO_EK__COUNT];
	static const char *enemy_name(KOBO_enemy_kinds eki);
	static int init();
	static void off();
	static void move();
	static void move_intro();
	static void detach_sounds();
	static void restart_sounds();
	static void put();
	static void force_positions();
	static void render_hit_zones();
	static KOBO_enemy *make(const KOBO_enemy_kind *ek,
			int x, int y, int h = 0, int v = 0, int di = 0);
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


inline void KOBO_enemy::init(const KOBO_enemy_kind *k, int px, int py,
		int h1, int v1, int dir)
{
	ek = k;
	x = px;
	y = py;
	h = h1;
	v = v1;
	contact = 0;
	di = dir;
	a = b = c = 0;
	soundhandle = 0;
	health = 20;
	damage = 50;
	splash_damage = 0;
	takes_splash_damage = true;
	shootable = true;
	mapcollide = false;
	physics = true;
	detonate_on_contact = false;
	hitsize = ek->hitsize;
	set_bank(ek->bank);
	frame = ek->frame;
	if(ek->sound)
		startsound(ek->sound);
	(this->*(ek->make)) ();
	if(actual_bank >= 0)
	{
		object = gengine->get_obj(ek->layer);
		if(object)
		{
			object->point.v.x = x;
			object->point.v.y = y;
			cs_point_force(&object->point);
			cs_obj_show(object);
		}
	}
	else
		object = NULL;
}


inline void KOBO_enemy::release()
{
	stopsound();
	soundhandle = 0;
	if(object)
		gengine->free_obj(object);
	enemies.stats[ek->eki].killed++;
	ek = NULL;	// Mark as dead! (Can't safely remove from list here.)
}

inline void KOBO_enemy::die()
{
	playsound(ek->deathsound);
	release();
}

inline void KOBO_enemy::playsound(KOBO_sounds si)
{
	sound.g_play(si, CS2PIXEL(x), CS2PIXEL(y));
}

inline void KOBO_enemy::startsound(KOBO_sounds si)
{
	stopsound();
	if(enemies.is_intro)
		return;
	soundhandle = sound.g_start(si, CS2PIXEL(x), CS2PIXEL(y));
	soundtimer = enemies.sound_update_period;
}

inline void KOBO_enemy::controlsound(unsigned ctrl, float val)
{
	if(soundhandle > 0)
		sound.g_control(soundhandle, ctrl, val);
}

inline void KOBO_enemy::stopsound()
{
	if(soundhandle > 0)
	{
		sound.g_stop(soundhandle);
		soundhandle = 0;
	}
}

inline void KOBO_enemy::set_bank(int new_bank)
{
	logical_bank = new_bank;
	actual_bank = s_get_actual_bank(gengine->get_gfx(), logical_bank);
	s_bank_t *bnk = s_get_bank(gengine->get_gfx(), actual_bank);
	if(bnk)
		frames = bnk->max + 1;
	else
		frames = 8;
}

inline void KOBO_enemy::hit(int dmg)
{
	if(!dmg)
		return;

	if(!shootable)
		return;

	if(HEALTH_INDESTRUCTIBLE != health)
	{
		health -= dmg;
		enemies.stats[ek->eki].damage += dmg;
	}
	if(enemies.is_intro)
	{
		if(health <= 0)
		{
			explode();
			die();
		}
		return;
	}
	else if(health > 0)
	{
		// Damaged but not destroyed!
		playsound(ek->damagesound);
		controlsound(2, dmg * 0.01f);
		return;
	}

	// Enemy destroyed.
	if(splash_damage)
		enemies.splash_damage(x, y, splash_damage);
	manage.add_score(ek->score);
	(this->*(ek->kill))();
}

inline void KOBO_enemy::detonate()
{
	if(splash_damage)
		enemies.splash_damage(x, y, splash_damage);
	(this->*(ek->kill))();
}

inline bool KOBO_enemy::can_hit_map(int px, int py)
{
	if(!mapcollide)
		return false;
	return ((signed)(CS2PIXEL(x) & (WORLD_SIZEX - 1)) >> 4 == px) &&
			((signed)(CS2PIXEL(y) & (WORLD_SIZEY - 1)) >> 4 == py);
}

inline bool KOBO_enemy::in_range(int px, int py, int range, int &dist)
{
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
	// Need to update this for stationary objects as well, as the listener
	// is (usually) moving around at all times!
	if((soundhandle > 0) && (soundtimer-- <= 0))
	{
		sound.g_move(soundhandle, CS2PIXEL(x), CS2PIXEL(y));
		soundtimer = enemies.sound_update_period;
	}

	x += h;
	y += v;
	x &= PIXEL2CS(WORLD_SIZEX) - 1;
	y &= PIXEL2CS(WORLD_SIZEY) - 1;
	int dx = WRAPDISTXCS(x, myship.get_csx());
	int dy = WRAPDISTYCS(y, myship.get_csy());
	diffx = CS2PIXEL(dx);
	diffy = CS2PIXEL(dy);
	mindiff = MAX(labs(diffx), labs(diffy));

	(this->*(ek->move))();
	if(!ek)
		return;	// Killed by ek->move()!

	// Handle collisions with the player ship
	if(!mapcollide && myship.alive() && (hitsize >= 0) &&
			(mindiff < (hitsize + myship.get_hitsize())))
	{
		player_collision(dx, dy);
		if(!ek)
			return;	// Killed by player_collision()!
	}
	else
		contact = 0;

	// Handle collisions with player bolts (Player bolts kill themselves
	// when they hit something, so we don't need to hit them from here.)
	if(!shootable || (mindiff >= ((VIEWLIMIT >> 1) + 8)))
		return;

	int dmg = myship.hit_bolt(CS2PIXEL(x), CS2PIXEL(y),
				hitsize + HIT_BOLT, health);

	hit(dmg);	// Bolt damages object
}

inline void KOBO_enemy::move_intro()
{
	x += h;
	y += v;
	x &= PIXEL2CS(WORLD_SIZEX) - 1;
	y &= PIXEL2CS(WORLD_SIZEY) - 1;
	diffx = WRAPDISTX(CS2PIXEL(x), myship.get_x());
	diffy = WRAPDISTY(CS2PIXEL(y), myship.get_y());
	mindiff = MAX(labs(diffx), labs(diffy));
	(this->*(ek->move))();
}

inline void KOBO_enemy::put()
{
	if(!object)
		return;
	object->point.v.x = x;
	object->point.v.y = y;
	cs_obj_image(object, actual_bank, frame + di - 1);
}

inline void KOBO_enemy::force_position()
{
	if(object)
		cs_point_force(&object->point);
}

inline int KOBO_enemy::is_pipe()
{
	return ((ek == &pipein) || (ek == &pipeout));
}

#endif				// XKOBO_H_ENEMIES
