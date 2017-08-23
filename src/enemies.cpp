/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
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

#include <stdio.h>
#include "enemies.h"
#include "random.h"
#include "radar.h"

KOBO_enemy KOBO_enemies::enemy[ENEMY_MAX];
KOBO_enemy *KOBO_enemies::enemy_max;
const KOBO_enemy_kind *KOBO_enemies::ekind_to_generate_1;
const KOBO_enemy_kind *KOBO_enemies::ekind_to_generate_2;
int KOBO_enemies::e1_interval;
int KOBO_enemies::e2_interval;
int KOBO_enemies::is_intro = 0;
int KOBO_enemies::sound_update_period = 3;
KOBO_enemystats KOBO_enemies::stats[KOBO_EK__COUNT];


//---------------------------------------------------------------------------//
KOBO_enemy::KOBO_enemy()
{
	object = NULL;
	_state = notuse;
}

void KOBO_enemy::state(KOBO_state s)
{
	switch(s)
	{
	  case notuse:
		soundhandle = 0;
		if(object)
		{
			gengine->free_obj(object);
			object = NULL;
		}
		break;
	  case reserved:
	  case moving:
		if(actual_bank >= 0)
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

void KOBO_enemy::detachsound()
{
	if(soundhandle > 0)
	{
		sound.g_release(soundhandle);
		soundhandle = 0;
	}
}

void KOBO_enemy::restartsound()
{
	stopsound();
	if(ek->sound)
		startsound(ek->sound);
}

//---------------------------------------------------------------------------//
#define	KOBO_DEFS(x, y)	case KOBO_EK_##x: return #y;
const char *KOBO_enemies::enemy_name(KOBO_enemy_kinds eki)
{
	switch(eki)
	{
	  KOBO_ALLENEMYKINDS
	  default:	return "<unknown enemy kind>";
	}
}
#undef	KOBO_DEFS

void KOBO_enemies::off()
{
	KOBO_enemy *enemyp;
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
		enemyp->state(notuse);
}

int KOBO_enemies::init()
{
	KOBO_enemy *enemyp;
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
		enemyp->init();
	enemy_max = enemy;
	ekind_to_generate_1 = NULL;
	ekind_to_generate_2 = NULL;
	e1_interval = 1;
	e2_interval = 1;
	is_intro = 0;
	sound_update_period = KOBO_SOUND_UPDATE_PERIOD / game.speed;
	memset(stats, 0, sizeof(stats));
	return 0;
}

void KOBO_enemies::move()
{
	KOBO_enemy *enemyp;
	/* realize reserved enemies */
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
	{
		if(enemyp->realize())
			enemy_max = enemyp;
	}
	for(enemyp = enemy; enemyp <= enemy_max; enemyp++)
		enemyp->move();
}

void KOBO_enemies::move_intro()
{
	KOBO_enemy *enemyp;
	/* realize reserved enemies */
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
	{
		if(enemyp->realize())
			enemy_max = enemyp;
	}
	for(enemyp = enemy; enemyp <= enemy_max; enemyp++)
		enemyp->move_intro();
}

void KOBO_enemies::detach_sounds()
{
	KOBO_enemy *enemyp;
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
		if(enemyp->in_use())
			enemyp->detachsound();
}

void KOBO_enemies::restart_sounds()
{
	KOBO_enemy *enemyp;
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
		if(enemyp->in_use())
			enemyp->restartsound();
}

void KOBO_enemies::put()
{
	KOBO_enemy *enemyp;
	for(enemyp = enemy; enemyp <= enemy_max; enemyp++)
		enemyp->put();
}

void KOBO_enemies::force_positions()
{
	KOBO_enemy *enemyp;
	for(enemyp = enemy; enemyp <= enemy_max; enemyp++)
		enemyp->force_position();
}

void KOBO_enemies::render_hit_zones()
{
	KOBO_enemy *enemyp;
	wmain->blendmode(GFX_BLENDMODE_ADD);
	for(enemyp = enemy; enemyp <= enemy_max; enemyp++)
		enemyp->render_hit_zone();
	wmain->blendmode();
}

int KOBO_enemies::make(const KOBO_enemy_kind * ek, int x, int y, int h, int v,
		int di)
{
	KOBO_enemy *enemyp;
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
		if(!enemyp->make(ek, x, y, h, v, di))
		{
			stats[ek->eki].spawned++;
			stats[ek->eki].health += enemyp->get_health();
			return 0;
		}
	return 1;
}

int KOBO_enemies::erase_cannon(int x, int y)
{
	int count = 0;
	KOBO_enemy *enemyp;
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
		count += enemyp->erase_cannon(x, y);
	if(count)
		wradar->update(x, y);
	return count;
}

void KOBO_enemies::hit_map(int x, int y, int dmg)
{
	if(!dmg)
		return;
	for(KOBO_enemy *enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
		if(enemyp->can_hit_map(x, y))
			enemyp->hit(dmg);
}

int KOBO_enemies::exist_pipe()
{
	int count = 0;
	KOBO_enemy *enemyp;
	for(enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
		if(enemyp->is_pipe())
			count++;
	return count;
}

void KOBO_enemies::set_ekind_to_generate(const KOBO_enemy_kind * e1, int i1,
		const KOBO_enemy_kind * e2, int i2)
{
	ekind_to_generate_1 = e1;
	ekind_to_generate_2 = e2;
	e1_interval = i1;
	e2_interval = i2;
}

void KOBO_enemies::splash_damage(int x, int y, int damage)
{
	int maxdist = PIXEL2CS(damage);
	int dist;

	// Enemies
	for(KOBO_enemy *enemyp = enemy; enemyp < enemy + ENEMY_MAX; enemyp++)
	{
		if(!enemyp->can_splash_damage())
			continue;
		if(!enemyp->in_range(x, y, maxdist, dist))
			continue;
		int dmg = damage * ENEMY_SPLASH_DAMAGE_MULTIPLIER *
				(maxdist - dist) / maxdist;
		if(dmg)
			enemyp->hit(dmg);
	}

	// Player
	if(myship.in_range(x, y, maxdist, dist))
	{
		int dmg = damage * game.splash_damage_multiplier *
				(maxdist - dist) / maxdist;
		if(dmg)
			myship.hit(dmg);
	}
}
