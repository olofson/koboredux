/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2007, 2009 David Olofson
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

#include "kobo.h"
#include "screen.h"
#include "myship.h"
#include "enemies.h"
#include "gamectl.h"
#include "manage.h"
#include "random.h"
#include "sound.h"

_myship_state _myship::_state;
int _myship::di;
int _myship::x;
int _myship::y;
int _myship::vx;
int _myship::vy;
int _myship::ax;
int _myship::ay;
int _myship::_health;
int _myship::health_time;
int _myship::explo_time;
int _myship::nose_reload_timer;
int _myship::tail_reload_timer;
int _myship::boltx[MAX_BOLTS];
int _myship::bolty[MAX_BOLTS];
int _myship::boltdx[MAX_BOLTS];
int _myship::boltdy[MAX_BOLTS];
int _myship::boltst[MAX_BOLTS];
cs_obj_t *_myship::object;
cs_obj_t *_myship::bolt_objects[MAX_BOLTS];
cs_obj_t *_myship::crosshair;


_myship::_myship()
{
	object = NULL;
	memset(bolt_objects, 0, sizeof(bolt_objects));
	crosshair = NULL;
}


void _myship::state(_myship_state s)
{
	switch (s)
	{
	  case dead:
		if(object)
			gengine->free_obj(object);
		object = NULL;
		if(crosshair)
			gengine->free_obj(crosshair);
		crosshair = NULL;
		break;
	  case normal:
		if(!object)
			object = gengine->get_obj(LAYER_PLAYER);
		if(object)
			cs_obj_show(object);
		break;
	}
	_state = s;
}


void _myship::off()
{
	state(dead);
	int i;
	for(i = 0; i < MAX_BOLTS; i++)
		if(bolt_objects[i])
		{
			gengine->free_obj(bolt_objects[i]);
			bolt_objects[i] = NULL;
		}
}


int _myship::init()
{
	nose_reload_timer = 0;
	tail_reload_timer = 0;
	x = PIXEL2CS(WORLD_SIZEX >> 1);
	y = PIXEL2CS((WORLD_SIZEY >> 2) * 3);
	vx = vy = 0;
	ax = ay = 0;
	di = 1;
	state(normal);

	apply_position();

	int i;
	for(i = 0; i < MAX_BOLTS; i++)
	{
		boltx[i] = 0;
		bolty[i] = 0;
		boltdx[i] = 0;
		boltdy[i] = 0;
		boltst[i] = 0;
		if(bolt_objects[i])
			gengine->free_obj(bolt_objects[i]);
		bolt_objects[i] = NULL;
	}
	return 0;
}


void _myship::explode()
{
	if(explo_time > 64)
		return;

	int d = 4096 - (64 - explo_time)*(64 - explo_time);
	int i;
	for(i = 0; i < 2; ++i)
	{
		int dx = (int)(pubrand.get(6)) - 32;
		int dy = (int)(pubrand.get(6)) - 32;
		int vx = dx * (4096 - d) >> 8;
		int vy = dy * (4096 - d) >> 8;
		dx = PIXEL2CS(dx * d >> 12);
		dy = PIXEL2CS(dy * d >> 12);
		enemies.make(enemies.randexp(), x + dx, y + dy, vx, vy,
				explo_time >> 4);
	}
	++explo_time;
}


#define BEAMV1	12
#define BEAMV2	(BEAMV1 * 2 / 3)

void _myship::handle_controls()
{
	int v;
	if(prefs->cmd_pushmove)
		v = gamecontrol.dir_push() ? PIXEL2CS(1) : 0;
	else if(gamecontrol.dir_push())
		v = PIXEL2CS(game.top_speed);
	else
#ifdef NOSPEEDLIMIT
		v = 0;
#else
		v = PIXEL2CS(game.cruise_speed);
#endif

	int tvx = v * sin(M_PI * (di - 1) / 4);
	int tvy = -v * cos(M_PI * (di - 1) / 4);
#ifdef NOSPEEDLIMIT
	ax = tvx >> 3;
	ay = tvy >> 3;
	if(gamecontrol.get_shot())
	{
		ax = ay = vx = vy = 0;
	}
#else
	ax = (tvx - vx) >> 2;
	ay = (tvy - vy) >> 2;
	if(!tvx)
	{
		// Kill remainder creep speed caused by truncation
		if((vx > -4) && (vx < 4))
			ax = -vx;
	}
	if(!tvy)
	{
		if((vy > -4) && (vy < 4))
			ay = -vy;
	}
#endif
}

void _myship::move()
{
	int i;
	di = gamecontrol.dir();

	if(++health_time >= game.health_fade)
	{
		health_time = 0;
		if(_health > game.health)
		{
			// Fade when boosted above 100%
			--_health;
		}
		else if((_health > 0) && (_health < game.health))
		{
			// Regeneration to next threshold
			int rn = regen_next();
			if((_health != rn) && (_health < rn))
				++_health;
		}
	}

	// Movement and collisions
	if(_state == normal)
	{
		handle_controls();
		update_position();
		explo_time = 0;
	}
	else if(_state == dead)
		explode();

	// Wrapping
	x &= PIXEL2CS(WORLD_SIZEX) - 1;
	y &= PIXEL2CS(WORLD_SIZEY) - 1;

	// Fire control
	if((_state == normal) && gamecontrol.get_shot())
	{
		int fired = 0;
		if(tail_reload_timer > 0)
			--tail_reload_timer;
		else if(!_myship::tail_fire())
			fired = 1;	// Ok!
		else
			fired = 2;	// Overheat!
		if(nose_reload_timer > 0)
			--nose_reload_timer;
		else if(!_myship::nose_fire())
			fired = 1;	// Ok!
		else
			fired = 2;	// Overheat!
		if(fired)
			sound.g_player_fire();
	}
	else
	{
		if(!gamecontrol.get_shot())
			sound.g_player_fire_off();
		if(nose_reload_timer > 0)
			--nose_reload_timer;
		if(tail_reload_timer > 0)
			--tail_reload_timer;
	}

	// Bolts
	const char animtab[8] = { 3, 2, 1, 0, 1, 2, 1, 2 };
	for(i = 0; i < MAX_BOLTS; i++)
	{
		if(!boltst[i])
			continue;
		++boltst[i];
		boltx[i] += boltdx[i];
		bolty[i] += boltdy[i];
		boltx[i] &= WORLD_SIZEX - 1;
		bolty[i] &= WORLD_SIZEY - 1;
		int dx = labs(wrapdist(boltx[i], CS2PIXEL(x), WORLD_SIZEX));
		int dy = labs(wrapdist(bolty[i], CS2PIXEL(y), WORLD_SIZEY));
		if(dx >= game.bolt_range || dy >= game.bolt_range)
		{
			boltst[i] = 0;
			if(bolt_objects[i])
				gengine->free_obj(bolt_objects[i]);
			bolt_objects[i] = NULL;
		}
		else if(bolt_objects[i])
			cs_obj_image(bolt_objects[i], B_BOLT,
					(bolt_objects[i]->anim.frame &
					0xfffffff8) + animtab[boltst[i] & 7]);
	}
	// NOTE: We can't check bolt/base collisions here, as that would kill
	//       bolts before core (enemy) collisions are checked!
}


void _myship::hit(int dmg)
{
	if(!dmg)
		return;

	if(_state != normal)
		return;

#ifdef	INVULNERABLE
	printf("INVULNERABLE: Ignored %d damage to player ship.\n", dmg);
#else
	if(_health && (dmg < _health))
		manage.noise_damage((float)dmg / _health);
	else
		manage.noise_damage(1.0f);

	_health -= dmg;

	// Be nice if we land right on a regeneration threshold level.
	if(_health == regen_next())
		++_health;
#endif

	if(dmg < game.health / 2)
		sound.g_player_damage((float)dmg / game.health * 2.0f);
	else
		sound.g_player_damage(1.0f);

	if(_health <= 0)
	{
		_health = 0;
		manage.lost_myship();
		state(dead);
		sound.g_player_explo_start();
	}
}


void _myship::health_bonus(int h)
{
	if(_state != normal)
		return;

	_health += game.health * h / 100;
	health_time = 0;
	if(_health > game.max_health)
		_health = game.max_health;
}


// Check and handle base/bolt collisions
void _myship::check_base_bolts()
{
	for(int i = 0; i < MAX_BOLTS; i++)
	{
		if(!boltst[i])
			continue;

		int x1 = WORLD2MAPX(boltx[i]);
		int y1 = WORLD2MAPY(bolty[i]);
		if(IS_BASE(screen.get_map(x1, y1)))
		{
			sound.g_play(SOUND_METALLIC, x1 << 4, y1 << 4);
			enemies.make(&boltexpl, PIXEL2CS(boltx[i]),
					PIXEL2CS(bolty[i]));
			boltst[i] = 0;
			if(bolt_objects[i])
				gengine->free_obj(bolt_objects[i]);
			bolt_objects[i] = NULL;
		}
	}
}


// Calculate bounce
static inline void calc_bounce(int p2, int *p3, int *v)
{
#ifdef BASE_BOUNCE
	*p3 = 2 * p2 - *p3;
	*v = -*v;
#else
	*p3 = p2;
	*v = 0;
#endif
}

// Check and handle base/ship collisions
void _myship::update_position()
{
	if(prefs->cmd_indicator)
	{
		// No collision response; just update and test.
		x += vx;
		y += vy;
		if(IS_BASE(screen.get_map(WORLD2MAPX(CS2PIXEL(x)),
				WORLD2MAPY(CS2PIXEL(y)))))
			sound.g_player_damage();
		return;
	}

	// Approximation: Apply half of the acceleration before collision
	// handling, and half after, to avoid continuous acceleration fun.
	vx += ax / 2;
	vy += ay / 2;

	// Calculate target for the "no collisions" case
	int x3 = x + vx;
	int y3 = y + vy;

	bool contact = 0;	// 'true' in case of collisions or contact
	int impact = 0;		// Collision velocity
	while(1)
	{
		// Find first collision, if any
		int x2, y2, hx, hy;
		int cd = screen.test_line(x, y, x3, y3, &x2, &y2, &hx, &hy);
		if(!cd)
		{
			// No (more) collisions!
			x = x3;
			y = y3;
			vx += ax / 2;
			vy += ay / 2;
			break;
		}

		contact = true;

		if(cd & COLL_STUCK)
			break;	// Stuck inside tile!

		// With bounce enabled:
		//	On collision, mirror the target position over the
		//	collision line, move the current position to the point
		//	of collision, and try again!
		//
		// With bounce disabled:
		//	Set the current and target positions to tho point of
		//	collision, and stop!
		//
		int imp = 0;
		if(cd & COLL_VERTICAL)
		{
			imp += labs(vx);
			calc_bounce(x2, &x3, &vx);
		}
		if(cd & COLL_HORIZONTAL)
		{
			imp += labs(vy);
			calc_bounce(y2, &y3, &vy);
		}

		// Deal velocity based damage to any fixed enemies attached to
		// the tile we just hit.
		enemies.hit_map(WORLD2MAPX(CS2PIXEL(hx)),
				WORLD2MAPY(CS2PIXEL(hy)),
				game.scale_vel_damage(imp, game.ram_damage));
		impact += imp;
		x = x2;
		y = y2;
	}
#ifdef DEBUG
	if(IS_BASE(screen.get_map(WORLD2MAPX(CS2PIXEL(x)),
			WORLD2MAPY(CS2PIXEL(y)))))
		printf("!!! Bounce calculation failed at (%x, %x)\n", x, y);
#endif

	if(contact)
	{
#if 0
		// TODO: Grinding noise.
		float grind = sqrt(vx*vx + vy*vy) / PIXEL2CS(game.top_speed);
#endif
	}

	// Deal velocity based damage to the ship
	hit(game.scale_vel_damage(impact, game.crash_damage));
}


int _myship::hit_bolt(int ex, int ey, int hitsize, int health)
{
	int dmg = 0;
	int i;
	for(i = 0; i < MAX_BOLTS; i++)
	{
		if(boltst[i] == 0)
			continue;
		if(labs(wrapdist(ex, boltx[i], WORLD_SIZEX)) >= hitsize)
			continue;
		if(labs(wrapdist(ey, bolty[i], WORLD_SIZEY)) >= hitsize)
			continue;
		if(!prefs->cmd_cheat)
		{
			boltst[i] = 0;
			if(bolt_objects[i])
				gengine->free_obj(bolt_objects[i]);
			bolt_objects[i] = NULL;
		}
		enemies.make(&boltexpl,
				PIXEL2CS(boltx[i]), PIXEL2CS(bolty[i]));
		dmg += game.bolt_damage;
		if(dmg >= health)
			break;
	}
	return dmg;
}


void _myship::put_crosshair()
{
	if(_state != normal)
		return;
	if(!crosshair)
	{
		crosshair = gengine->get_obj(LAYER_OVERLAY);
		if(crosshair)
			cs_obj_show(crosshair);
	}
	if(crosshair)
	{
		cs_obj_image(crosshair, B_CROSSHAIR, 0);
		crosshair->point.v.x = PIXEL2CS(mouse_x - WMAIN_X);
		crosshair->point.v.y = PIXEL2CS(mouse_y - WMAIN_H);
	}
}


int _myship::put()
{
	// Player
	apply_position();
	if(object)
		cs_obj_image(object, B_PLAYER, (di - 1) * 2);

	// Bullets
	int i;
	for(i = 0; i < MAX_BOLTS; i++)
	{
		if(!bolt_objects[i])
			continue;
		if(boltst[i])
		{
			bolt_objects[i]->point.v.x = PIXEL2CS(boltx[i]);
			bolt_objects[i]->point.v.y = PIXEL2CS(bolty[i]);
		}
	}
	return 0;
}


void _myship::shot_single(int i, int dir, int offset)
{
	int doffset = (offset * 7071 + (offset > 0 ? 5000 : -5000)) / 10000;
	boltst[i] = 1;
	boltx[i] = CS2PIXEL(x);
	bolty[i] = CS2PIXEL(y);
	switch((dir + 1) % 8 + 1)
	{
	  case 1:
		bolty[i] -= offset;
		break;
	  case 2:
		bolty[i] -= doffset;
		boltx[i] += doffset;
		break;
	  case 3:
		boltx[i] += offset;
		break;
	  case 4:
		boltx[i] += doffset;
		bolty[i] += doffset;
		break;
	  case 5:
		bolty[i] += offset;
		break;
	  case 6:
		bolty[i] += doffset;
		boltx[i] -= doffset;
		break;
	  case 7:
		boltx[i] -= offset;
		break;
	  case 8:
		boltx[i] -= doffset;
		bolty[i] -= doffset;
		break;
	}
	switch(dir)
	{
	  case 1:
		boltdx[i] = 0;
		boltdy[i] = -BEAMV1;
		break;
	  case 2:
		boltdy[i] = -BEAMV2;
		boltdx[i] = BEAMV2;
		break;
	  case 3:
		boltdx[i] = BEAMV1;
		boltdy[i] = 0;
		break;
	  case 4:
		boltdx[i] = BEAMV2;
		boltdy[i] = BEAMV2;
		break;
	  case 5:
		boltdx[i] = 0;
		boltdy[i] = BEAMV1;
		break;
	  case 6:
		boltdy[i] = BEAMV2;
		boltdx[i] = -BEAMV2;
		break;
	  case 7:
		boltdx[i] = -BEAMV1;
		boltdy[i] = 0;
		break;
	  case 8:
		boltdx[i] = -BEAMV2;
		boltdy[i] = -BEAMV2;
		break;
	}
	if(!bolt_objects[i])
		bolt_objects[i] = gengine->get_obj(LAYER_PLAYER);
	if(bolt_objects[i])
	{
		bolt_objects[i]->point.v.x = PIXEL2CS(boltx[i]);
		bolt_objects[i]->point.v.y = PIXEL2CS(bolty[i]);
		cs_obj_image(bolt_objects[i], B_BOLT,
				(((dir - 1) & 0x3) << 3) + 3);
		cs_obj_show(bolt_objects[i]);
	}
}


int _myship::nose_fire()
{
	int i;
	for(i = 0; i < MAX_BOLTS && boltst[i]; i++)
		;
	if(i >= MAX_BOLTS)
		return 1;
	shot_single(i, di, 0);
	nose_reload_timer = game.noseloadtime;
	return 0;
}


int _myship::tail_fire()
{
	int i;
	for(i = 0; i < MAX_BOLTS && boltst[i]; i++)
		;
	if(i >= MAX_BOLTS)
		return 1;
	shot_single(i, (di > 4) ? (di - 4) : (di + 4), 0);
	tail_reload_timer = game.tailloadtime;
	return 0;
}


void _myship::set_position(int px, int py)
{
	x = PIXEL2CS(px);
	y = PIXEL2CS(py);

	if(object)
	{
		apply_position();
		cs_point_force(&object->point);
	}
}


void _myship::apply_position()
{
	if(object)
	{
		object->point.v.x = x;
		object->point.v.y = y;
	}
	sound.g_position(CS2PIXEL(x), CS2PIXEL(y));
}


bool _myship::in_range(int px, int py, int range, int &dist)
{
	int dx = labs(wrapdist(x, px, PIXEL2CS(WORLD_SIZEX)));
	if(dx > range)
		return false;
	int dy = labs(wrapdist(y, py, PIXEL2CS(WORLD_SIZEY)));
	if(dy > range)
		return false;
	dist = sqrt(dx*dx + dy*dy);
	return dist <= range;
}
