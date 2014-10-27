/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996 Akira Higuchi
 * Copyright (C) 2001, 2003, 2007, 2009 David Olofson
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

#include <stdio.h>
#include "enemies.h"
#include "random.h"
#include "radar.h"

_enemy _enemies::enemy[ENEMY_MAX];
_enemy *_enemies::enemy_max;
const enemy_kind *_enemies::ekind_to_generate_1;
const enemy_kind *_enemies::ekind_to_generate_2;
int _enemies::e1_interval;
int _enemies::e2_interval;
int _enemies::explocount = 0;
int _enemies::is_intro = 0;

_enemy::_enemy()
{
	object = NULL;
	_state = notuse;
}

void _enemy::state(_state_t s)
{
	switch(s)
	{
	  case notuse:
		if(object)
		{
			gengine->free_obj(object);
			object = NULL;
		}
		break;
	  case reserved:
	  case moving:
		if(bank >= 0)
		{
			if(!object)
				object = gengine->get_obj(ek->layer);
			if(!object)
				break;
			if(s == moving)
				cs_obj_show(object);
			else
				cs_obj_hide(object);
		}
		else if(object)
		{
			gengine->free_obj(object);
			object = NULL;
		}
		break;
	}
	_state = s;
}



void _enemies::off()
{
	_enemy *enemyp;
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
		enemyp->release();
}

int _enemies::init()
{
	_enemy *enemyp;
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
		enemyp->init();
	enemy_max = enemy;
	ekind_to_generate_1 = NULL;
	ekind_to_generate_2 = NULL;
	e1_interval = 1;
	e2_interval = 1;
	return 0;
}

void _enemies::move()
{
	_enemy *enemyp;
	/* realize reserved enemies */
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
	{
		if(enemyp->realize())
			enemy_max = enemyp;
	}
	for(enemyp = enemy; enemyp <= enemy_max; enemyp++)
		enemyp->move();
}

void _enemies::move_intro()
{
	_enemy *enemyp;
	is_intro = 1;
	/* realize reserved enemies */
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
	{
		if(enemyp->realize())
			enemy_max = enemyp;
	}
	for(enemyp = enemy; enemyp <= enemy_max; enemyp++)
		enemyp->move_intro();
	is_intro = 0;
}

void _enemies::put()
{
	_enemy *enemyp;
	for(enemyp = enemy; enemyp <= enemy_max; enemyp++)
		enemyp->put();
}

int _enemies::make(const enemy_kind * ek, int x, int y, int h, int v,
		int di)
{
	_enemy *enemyp;
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
		if(!enemyp->make(ek, x, y, h, v, di))
			return 0;
	return 1;
}

const enemy_kind *_enemies::randexp()
{
	explocount += 1 + pubrand.get(1);
	switch(explocount & 3)
	{
	  case 0:
		return &explosion;
	  case 1:
		return &explosion3;
	  case 2:
		return &explosion4;
	  default:
		return &explosion5;
	}
}

int _enemies::erase_cannon(int x, int y)
{
	int count = 0;
	_enemy *enemyp;
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
		count += enemyp->erase_cannon(x, y);
	if(count)
		wradar->update(x, y);
	return count;
}

int _enemies::exist_pipe()
{
	int count = 0;
	_enemy *enemyp;
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
		if(enemyp->is_pipe())
			count++;
	return count;
}

void _enemies::set_ekind_to_generate(const enemy_kind * e1, int i1,
		const enemy_kind * e2, int i2)
{
	ekind_to_generate_1 = e1;
	ekind_to_generate_2 = e2;
	e1_interval = i1;
	e2_interval = i2;
}
