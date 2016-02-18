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

#include "kobo.h"
#include "screen.h"
#include "myship.h"
#include "enemies.h"
#include "gamectl.h"
#include "manage.h"
#include "random.h"
#include "sound.h"

KOBO_myship_state KOBO_myship::_state;
int KOBO_myship::di;
int KOBO_myship::fdi;
int KOBO_myship::dframes;
int KOBO_myship::x;
int KOBO_myship::y;
int KOBO_myship::vx;
int KOBO_myship::vy;
int KOBO_myship::ax;
int KOBO_myship::ay;
int KOBO_myship::_health;
int KOBO_myship::health_time;
int KOBO_myship::explo_time;
int KOBO_myship::nose_reload_timer;
int KOBO_myship::tail_reload_timer;
KOBO_player_bolt KOBO_myship::bolts[MAX_BOLTS];
cs_obj_t *KOBO_myship::object;
cs_obj_t *KOBO_myship::crosshair;


KOBO_myship::KOBO_myship()
{
	memset(bolts, 0, sizeof(bolts));
	object = NULL;
	crosshair = NULL;
}


void KOBO_myship::state(KOBO_myship_state s)
{
	if(prefs->cheat_shield && (s == SHIP_NORMAL))
		s = SHIP_INVULNERABLE;
	switch (s)
	{
	  case SHIP_DEAD:
		sound.g_player_fire(false);
		if(object)
			gengine->free_obj(object);
		object = NULL;
		if(crosshair)
			gengine->free_obj(crosshair);
		crosshair = NULL;
		break;
	  case SHIP_INVULNERABLE:
	  case SHIP_NORMAL:
		if(!object)
			object = gengine->get_obj(LAYER_PLAYER);
		// HACK: We don't use this for rendering in the normal fashion,
		// because we want to filter rotation at the rendering frame
		// rate. The engine should provide custom rendering callbacks
		// for stuff like this... (Same deal with the bolts!)
		if(object)
		{
			cs_obj_show(object);
			cs_obj_hide(object);
		}
		break;
	}
	_state = s;
}


void KOBO_myship::off()
{
	state(SHIP_DEAD);
	int i;
	for(i = 0; i < MAX_BOLTS; i++)
		if(bolts[i].object)
		{
			gengine->free_obj(bolts[i].object);
			bolts[i].object = NULL;
		}
}


int KOBO_myship::init()
{
	nose_reload_timer = 0;
	tail_reload_timer = 0;
	x = PIXEL2CS(WORLD_SIZEX >> 1);
	y = PIXEL2CS((WORLD_SIZEY >> 2) * 3);
	vx = vy = 0;
	ax = ay = 0;
	di = 1;
	state(SHIP_NORMAL);

	if(s_bank_t *b = s_get_bank(gengine->get_gfx(), B_PLAYER))
		dframes = b->max + 1;
	else
		dframes = 8;
	fdi = ((di - 1) * dframes << 8) / 8;

	apply_position();

	int i;
	for(i = 0; i < MAX_BOLTS; i++)
		if(bolts[i].object)
			gengine->free_obj(bolts[i].object);
	memset(bolts, 0, sizeof(bolts));
	return 0;
}


void KOBO_myship::explode()
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


void KOBO_myship::handle_controls()
{
	int v;
	if(prefs->cheat_pushmove)
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
		if((vx > -8) && (vx < 8))
			ax = -vx;
	}
	if(!tvy)
	{
		if((vy > -8) && (vy < 8))
			ay = -vy;
	}
#endif
}


void KOBO_myship::move()
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
	switch(_state)
	{
	  case SHIP_NORMAL:
	  case SHIP_INVULNERABLE:
		handle_controls();
		update_position();
		explo_time = 0;
		break;
	  case SHIP_DEAD:
		explode();
		break;
	}

	// Wrapping
	x &= PIXEL2CS(WORLD_SIZEX) - 1;
	y &= PIXEL2CS(WORLD_SIZEY) - 1;

	// Fire control
	if((_state != SHIP_DEAD) && gamecontrol.get_shot())
	{
		int fired = 0;
		if(tail_reload_timer > 0)
			--tail_reload_timer;
		else if(!tail_fire())
			fired = 1;	// Ok!
		else
			fired = 2;	// Overheat!
		if(nose_reload_timer > 0)
			--nose_reload_timer;
		else if(!nose_fire())
			fired = 1;	// Ok!
		else
			fired = 2;	// Overheat!
		if(fired)
			sound.g_player_fire(true);
	}
	else
	{
		if(!gamecontrol.get_shot())
			sound.g_player_fire(false);
		if(nose_reload_timer > 0)
			--nose_reload_timer;
		if(tail_reload_timer > 0)
			--tail_reload_timer;
	}

	// Bolts
	for(i = 0; i < MAX_BOLTS; i++)
	{
		if(!bolts[i].state)
			continue;
		if(bolts[i].state <= 2)
		{
			bolts[i].x += bolts[i].dx * (bolts[i].state - 1) >> 1;
			bolts[i].y += bolts[i].dy * (bolts[i].state - 1) >> 1;
		}
		else
		{
			bolts[i].x += bolts[i].dx;
			bolts[i].y += bolts[i].dy;
		}
		++bolts[i].state;
		bolts[i].x &= PIXEL2CS(WORLD_SIZEX) - 1;
		bolts[i].y &= PIXEL2CS(WORLD_SIZEY) - 1;
		int dx = labs(WRAPDISTX(CS2PIXEL(bolts[i].x), CS2PIXEL(x)));
		int dy = labs(WRAPDISTY(CS2PIXEL(bolts[i].y), CS2PIXEL(y)));
		if(dx >= game.bolt_range || dy >= game.bolt_range)
		{
			bolts[i].state = 0;
			if(bolts[i].object)
				gengine->free_obj(bolts[i].object);
			bolts[i].object = NULL;
		}
	}
	// NOTE: We can't check bolt/base collisions here, as that would kill
	//       bolts before core (enemy) collisions are checked!
}


void KOBO_myship::hit(int dmg)
{
	if(!dmg)
		return;

	if(_state != SHIP_NORMAL)
	{
		if(_state == SHIP_INVULNERABLE)
			sound.g_player_damage(0.5f);
		return;
	}

	if(!prefs->cheat_invulnerability)
	{
		if(_health && (dmg < _health))
			manage.noise_damage((float)dmg / _health);
		else
			manage.noise_damage(1.0f);

		_health -= dmg;

		// Be nice if we land right on a regeneration threshold level.
		if(_health == regen_next())
			++_health;
	}
	else
		printf("INVULNERABLE: Ignored %d damage to player.\n", dmg);

	if(dmg < game.health / 2)
		sound.g_player_damage((float)dmg / game.health * 2.0f);
	else
		sound.g_player_damage(1.0f);

	if(_health <= 0)
	{
		_health = 0;
		manage.lost_myship();
		state(SHIP_DEAD);
		sound.g_player_explo_start();
	}
}


void KOBO_myship::health_bonus(int h)
{
	if(_state == SHIP_DEAD)
		return;

	_health += game.health * h / 100;
	health_time = 0;
	if(_health > game.max_health)
		_health = game.max_health;
}


// Check and handle base/bolt collisions
void KOBO_myship::check_base_bolts()
{
	for(int i = 0; i < MAX_BOLTS; i++)
	{
		if(!bolts[i].state)
			continue;

		int x1 = WORLD2MAPX(CS2PIXEL(bolts[i].x));
		int y1 = WORLD2MAPY(CS2PIXEL(bolts[i].y));
		if(IS_BASE(screen.get_map(x1, y1)))
		{
			sound.g_play(SOUND_METALLIC, x1 << 4, y1 << 4);
			enemies.make(&boltexpl, bolts[i].x, bolts[i].y);
			bolts[i].state = 0;
			if(bolts[i].object)
				gengine->free_obj(bolts[i].object);
			bolts[i].object = NULL;
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
void KOBO_myship::update_position()
{
	if(prefs->indicator)
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


int KOBO_myship::hit_bolt(int ex, int ey, int hitsize, int health)
{
	int dmg = 0;
	int i;
	for(i = 0; i < MAX_BOLTS; i++)
	{
		if(bolts[i].state == 0)
			continue;
		if(labs(WRAPDISTX(ex, CS2PIXEL(bolts[i].x))) >= hitsize)
			continue;
		if(labs(WRAPDISTY(ey, CS2PIXEL(bolts[i].y))) >= hitsize)
			continue;
		enemies.make(&boltexpl, bolts[i].x, bolts[i].y);
		dmg += game.bolt_damage;
		if(dmg >= health)
			break;
	}
	return dmg;
}


void KOBO_myship::put_crosshair()
{
	if(_state == SHIP_DEAD)
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


int KOBO_myship::put()
{
	// Player
	apply_position();

	// Bullets
	int i;
	for(i = 0; i < MAX_BOLTS; i++)
	{
		if(!bolts[i].object)
			continue;
		if(bolts[i].state)
		{
			bolts[i].object->point.v.x = bolts[i].x;
			bolts[i].object->point.v.y = bolts[i].y;
		}
	}
	return 0;
}


void KOBO_myship::render()
{
	if(!myship.object)
		return;

	// Render player ship
	int maxd = dframes << 8;
	int tdi = ((di - 1) * dframes << 8) / 8 + 127;
	int ddi = tdi - fdi;
	if(ddi < 0)
		ddi = maxd - (-ddi) % maxd;
	else
		ddi %= maxd;
	if(ddi > maxd / 2)
		ddi -= maxd;
	fdi += ddi * gengine->frame_delta_time() * 0.02f;
	fdi = (fdi + maxd) % maxd;
	wmain->sprite_fxp(object->point.gx, object->point.gy,
			B_PLAYER, fdi >> 8);

	// Render bolts
	int i;
	for(i = 0; i < MAX_BOLTS; i++)
	{
		if(!bolts[i].state || !bolts[i].object)
			continue;
		wmain->sprite_fxp(bolts[i].object->point.gx,
				bolts[i].object->point.gy, B_BOLT,
				bolt_frame(bolts[i].dir, bolts[i].state - 2));
	}

	// Render shield
	if(_state == SHIP_INVULNERABLE)
		wmain->sprite_fxp(object->point.gx, object->point.gy,
				B_SHIELDFX, manage.game_time() % 8);
}


int KOBO_myship::shot_single(int dir, int loffset, int hoffset)
{
	int i;
	for(i = 0; i < MAX_BOLTS && bolts[i].state; i++)
		;
	if(i >= MAX_BOLTS)
		return 1;
	bolts[i].state = 1;
	bolts[i].dir = dir;
	int sdi = sin(M_PI * (dir - 1) / 4) * 256.0f;
	int cdi = cos(M_PI * (dir - 1) / 4) * 256.0f;
	bolts[i].x = x - vx + loffset * sdi + hoffset * cdi;
	bolts[i].y = y - vy - loffset * cdi + hoffset * sdi;
	bolts[i].dx = vx + game.bolt_speed * sdi;
	bolts[i].dy = vy - game.bolt_speed * cdi;
	if(!bolts[i].object)
		bolts[i].object = gengine->get_obj(LAYER_PLAYER);
	if(bolts[i].object)
	{
		bolts[i].object->point.v.x = bolts[i].x;
		bolts[i].object->point.v.y = bolts[i].y;
		cs_obj_show(bolts[i].object);
		cs_obj_hide(bolts[i].object);
	}
	return 0;
}


int KOBO_myship::nose_fire()
{
	if(shot_single(di, 15, 0))
		return 1;
	nose_reload_timer = game.noseloadtime;
	return 0;
}


int KOBO_myship::tail_fire()
{
	if(shot_single((di + 3) % 8 + 1, 11, 0))
		return 1;
	tail_reload_timer = game.tailloadtime;
	return 0;
}


void KOBO_myship::set_position(int px, int py)
{
	x = PIXEL2CS(px);
	y = PIXEL2CS(py);

	if(object)
	{
		apply_position();
		cs_point_force(&object->point);
	}
}


void KOBO_myship::apply_position()
{
	if(object)
	{
		object->point.v.x = x;
		object->point.v.y = y;
	}
}


bool KOBO_myship::in_range(int px, int py, int range, int &dist)
{
	int dx = labs(WRAPDISTXCS(x, px));
	if(dx > range)
		return false;
	int dy = labs(WRAPDISTYCS(y, py));
	if(dy > range)
		return false;
	dist = sqrt(dx*dx + dy*dy);
	return dist <= range;
}
