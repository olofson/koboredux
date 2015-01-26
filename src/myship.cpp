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

#define	WING_GUN_OFFSET	4

_myship_state _myship::_state;
int _myship::di;
int _myship::virtx;
int _myship::virty;
int _myship::x;
int _myship::y;
int _myship::_health;
int _myship::health_time;
int _myship::explo_time;
int _myship::nose_reload_timer;
int _myship::nose_temperature;
int _myship::nose_alt = 0;
int _myship::tail_reload_timer;
int _myship::tail_temperature;
int _myship::tail_alt = WING_GUN_OFFSET;
int _myship::lapx;
int _myship::lapy;
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
	nose_temperature = 0;
	tail_reload_timer = 0;
	tail_temperature = 0;
	x = WORLD_SIZEX >> 1;
	y = (WORLD_SIZEY >> 2) * 3;
	virtx = x - WMAIN_W / 2;
	virty = y - WMAIN_H / 2;
	lapx = 0;
	lapy = 0;
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


void _myship::explode(int x, int y)
{
	if(explo_time == 56)
		manage.noise_out(1000);
	else if(explo_time > 64)
		return;

	int d = 4096 - (64 - explo_time)*(64 - explo_time);
	int i;
	for(i = 0; i < 2; ++i)
	{
		int dx = (int)(pubrand.get(6)) - 32;
		int dy = (int)(pubrand.get(6)) - 32;
		int vx = dx * (4096 - d) >> 8;
		int vy = dy * (4096 - d) >> 8;
		dx = dx * d >> 12;
		dy = dy * d >> 12;
		enemies.make(enemies.randexp(), x + dx, y + dy, vx, vy,
				explo_time >> 4);
	}
	++explo_time;
}


#define BEAMV1           12
#define BEAMV2           (BEAMV1*2/3)

int _myship::move()
{
	int i;
	di = gamecontrol.dir();

	if(++health_time >= game.health_fade)
	{
		health_time = 0;
		if(_health > game.health)
			--_health;
	}

	virtx = x - WMAIN_W / 2;
	virty = y - WMAIN_H / 2;
	if(_state == normal)
	{
		int vd, vo;
		if(!prefs->cmd_pushmove)
		{
			vd = 2;
			vo = 3;
		}
		else if(gamecontrol.dir_push())
			{
				vd = 1;
				vo = 1;
			}
			else
			{
				vd = 0;
				vo = 0;
			}
		switch (di)
		{
		  case 1:
			virty -= vo;
			break;
		  case 2:
			virty -= vd;
			virtx += vd;
			break;
		  case 3:
			virtx += vo;
			break;
		  case 4:
			virtx += vd;
			virty += vd;
			break;
		  case 5:
			virty += vo;
			break;
		  case 6:
			virty += vd;
			virtx -= vd;
			break;
		  case 7:
			virtx -= vo;
			break;
		  case 8:
			virtx -= vd;
			virty -= vd;
			break;
		}
		explo_time = 0;
	}
	else if(_state == dead)
		explode(x, y);

	lapx = 0;
	lapy = 0;
	if(virtx < 0)
	{
		virtx += WORLD_SIZEX;
		lapx = WORLD_SIZEX;
	}
	if(virtx >= WORLD_SIZEX)
	{
		virtx -= WORLD_SIZEX;
		lapx = -WORLD_SIZEX;
	}
	if(virty < 0)
	{
		virty += WORLD_SIZEY;
		lapy = WORLD_SIZEY;
	}
	if(virty >= WORLD_SIZEY)
	{
		virty -= WORLD_SIZEY;
		lapy = -WORLD_SIZEY;
	}
	x = virtx + WMAIN_W / 2;
	y = virty + WMAIN_H / 2;

	nose_temperature -= game.nosecooling;
	if(nose_temperature < 0)
		nose_temperature = 0;
	tail_temperature -= game.tailcooling;
	if(tail_temperature < 0)
		tail_temperature = 0;

	if((_state == normal) && gamecontrol.get_shot())
	{
		if(game.skill == SKILL_CLASSIC)
		{
			if(nose_reload_timer > 0)
				--nose_reload_timer;
			else
			{
				_myship::xkobo_shot();
				nose_reload_timer = game.noseloadtime - 1;
			}
		}
		else
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
#if 0
			if(fired == 1)
				sound.g_player_fire();
			else if(fired == 2)
				sound.g_player_overheat();
#else
			if(fired)
				sound.g_player_fire();
#endif
		}
	}
	else
	{
		if(!gamecontrol.get_shot())
		{
			nose_alt = 1;
			sound.g_player_fire_off();
		}
		if(nose_reload_timer > 0)
			--nose_reload_timer;
		if(tail_reload_timer > 0)
			--tail_reload_timer;
	}

	const char animtab[8] = { 3, 2, 1, 0, 1, 2, 1, 2 };
	for(i = 0; i < MAX_BOLTS; i++)
	{
		if(!boltst[i])
			continue;
		++boltst[i];
		boltx[i] += lapx;
		bolty[i] += lapy;
		boltx[i] += boltdx[i];
		bolty[i] += boltdy[i];
		if((ABS(boltx[i] - x) >= (VIEWLIMIT >> 1) + 16) ||
				(ABS(bolty[i] - y) >= (VIEWLIMIT >> 1) + 16))
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
	return 0;
}


void _myship::hit(int dmg)
{
	if(_state != normal)
		return;

	if(!dmg)
		dmg = 1000;

	if(_health && (dmg < _health))
		manage.noise_damage((float)dmg / _health);
	else
		manage.noise_damage(1.0f);

	_health -= dmg;
	if(_health > 0)
		sound.g_player_damage();
	else
	{
		_health = 0;
		manage.lost_myship();
		state(dead);
		sound.g_player_explo_start();
	}
}


void _myship::health_bonus(int h)
{
	if(game.skill == SKILL_CLASSIC)
		return;
	_health += game.health * h / 100;
	health_time = 0;
	if(_health > game.health * 2)
		_health = game.health * 2;
}


int _myship::hit_structure()
{
	int x1, y1;
	int i, ch;

	// Check bolts/objects
	for(i = 0; i < MAX_BOLTS; i++)
	{
		if(!boltst[i])
			continue;
		x1 = (boltx[i] & (WORLD_SIZEX - 1)) >> 4;
		y1 = (bolty[i] & (WORLD_SIZEY - 1)) >> 4;
		ch = screen.get_map(x1, y1);
		if(!IS_SPACE(ch) && (ch & HIT_MASK))
		{
			sound.g_bolt_hit(x1<<12, y1<<12);
			enemies.make(&boltexpl, boltx[i], bolty[i]);
			boltst[i] = 0;
			if(bolt_objects[i])
				gengine->free_obj(bolt_objects[i]);
			bolt_objects[i] = NULL;
		}
	}

	// Check player/bases
	x1 = (x & (WORLD_SIZEX - 1)) >> 4;
	y1 = (y & (WORLD_SIZEY - 1)) >> 4;
	ch = screen.get_map(x1, y1);
	if(!IS_SPACE(ch) && (ch & HIT_MASK))
	{
		if(prefs->cmd_indicator)
			sound.g_player_damage();
		else
			_myship::hit(1000);
	}
	return 0;
}


int _myship::hit_bolt(int ex, int ey, int hitsize, int health)
{
	int dmg = 0;
	int i;
	for(i = 0; i < MAX_BOLTS; i++)
	{
		if(boltst[i] == 0)
			continue;
		if(ABS(ex - boltx[i]) >= hitsize)
			continue;
		if(ABS(ey - bolty[i]) >= hitsize)
			continue;
		if(!prefs->cmd_cheat)
		{
			boltst[i] = 0;
			if(bolt_objects[i])
				gengine->free_obj(bolt_objects[i]);
			bolt_objects[i] = NULL;
		}
		enemies.make(&boltexpl, boltx[i], bolty[i]);
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
	/* Player */
	apply_position();
	if(object)
		cs_obj_image(object, B_PLAYER, (di - 1) * 2);

	/* Bullets */
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
	boltx[i] = x;
	bolty[i] = y;
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


int _myship::xkobo_shot()
{
	int i, j;
	for(i = 0; i < game.bolts && boltst[i]; i++)
		;
	for(j = i + 1; j < game.bolts && boltst[j]; j++)
		;
	if(j >= game.bolts)
	{
#if 0
		sound.g_player_overheat(1);
#endif
		return 1;
	}
	shot_single(i, di, 0);
	shot_single(j, (di > 4) ? (di - 4) : (di + 4), 0);
	sound.g_player_fire();
	return 0;
}


int _myship::nose_fire()
{
	int i, j;
	// Logic: If firing would cause overheat, wait!
	if(255 - nose_temperature < game.noseheatup)
		return 1;
	for(i = 0; i < game.bolts && boltst[i]; i++)
		;
	if(!game.altfire)
	{
		if(i >= game.bolts)
			return 1;
		shot_single(i, di, 0);
		nose_temperature += game.noseheatup;
		nose_reload_timer = game.noseloadtime;
		return 0;
	}
	for(j = i + 1; j < game.bolts && boltst[j]; j++)
		;
	if(j >= game.bolts)
		return 1;
	if(++nose_alt > 2)
		nose_alt = 0;
	shot_single(i, di, (nose_alt - 1) * WING_GUN_OFFSET);
	if(++nose_alt > 2)
		nose_alt = 0;
	shot_single(j, di, (nose_alt - 1) * WING_GUN_OFFSET);
	nose_temperature += game.noseheatup;
	nose_reload_timer = game.noseloadtime;
	return 0;
}


int _myship::tail_fire()
{
	int i;
	if(255 - tail_temperature < game.tailheatup)
		return 1;
	for(i = 0; i < game.bolts && boltst[i]; i++)
		;
	if(i >= game.bolts)
		return 1;
	if(game.altfire)
	{
		tail_alt = -tail_alt;
		shot_single(i, (di > 4) ? (di - 4) : (di + 4), tail_alt);
	}
	else
		shot_single(i, (di > 4) ? (di - 4) : (di + 4), 0);
	tail_temperature += game.tailheatup;
	tail_reload_timer = game.tailloadtime;
	return 0;
}


void _myship::set_position(int px, int py)
{
	x = px;
	y = py;
	virtx = x - WMAIN_W / 2;
	virty = y - WMAIN_H / 2;

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
		object->point.v.x = PIXEL2CS(x);
		object->point.v.y = PIXEL2CS(y);
	}
	sound.g_position(x, y);
}
