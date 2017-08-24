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

KOBO_enemy *KOBO_enemies::active = NULL;
KOBO_enemy *KOBO_enemies::pool = NULL;
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
	while(active)
	{
		KOBO_enemy *e = active;
		active = active->next;
		if(e->ek)
			e->release();
		e->next = pool;
		pool = e;
	}
}

int KOBO_enemies::init()
{
	off();
	ekind_to_generate_1 = NULL;
	ekind_to_generate_2 = NULL;
	e1_interval = 1;
	e2_interval = 1;
	is_intro = 0;
	sound_update_period = KOBO_SOUND_UPDATE_PERIOD / game.speed;
	memset(stats, 0, sizeof(stats));
	return 0;
}

void KOBO_enemies::clean()
{
	KOBO_enemy *p = NULL;
	KOBO_enemy *e = active;
	while(e)
	{
		KOBO_enemy *de = e;
		e = e->next;
		if(!de->ek)
		{
			if(p)
				p->next = e;
			else
				active = e;
			de->next = pool;
			pool = de;
		}
		else
			p = de;
	}
}

void KOBO_enemies::move()
{
	clean();
	for(KOBO_enemy *e = NULL; (e = next(e)); )
		e->move();
}

void KOBO_enemies::move_intro()
{
	clean();
	for(KOBO_enemy *e = NULL; (e = next(e)); )
		e->move_intro();
}

void KOBO_enemies::detach_sounds()
{
	for(KOBO_enemy *e = NULL; (e = next(e)); )
		e->detachsound();
}

void KOBO_enemies::restart_sounds()
{
	for(KOBO_enemy *e = NULL; (e = next(e)); )
		e->restartsound();
}

void KOBO_enemies::put()
{
	for(KOBO_enemy *e = NULL; (e = next(e)); )
		e->put();
}

void KOBO_enemies::force_positions()
{
	for(KOBO_enemy *e = NULL; (e = next(e)); )
		e->force_position();
}

void KOBO_enemies::render_hit_zones()
{
	wmain->blendmode(GFX_BLENDMODE_ADD);
	for(KOBO_enemy *e = NULL; (e = next(e)); )
		e->render_hit_zone();
	wmain->blendmode();
}

KOBO_enemy *KOBO_enemies::make(const KOBO_enemy_kind *ek,
		int x, int y, int h, int v, int di)
{
	KOBO_enemy *e = pool;
	if(e)
		pool = e->next;
	else
		e = new KOBO_enemy;
	e->init(ek, x, y, h, v, di);
	stats[ek->eki].spawned++;
	stats[ek->eki].health += e->health;
	e->next = active;
	active = e;
	return e;
}

int KOBO_enemies::erase_cannon(int x, int y)
{
	int count = 0;
	for(KOBO_enemy *e = NULL; (e = next(e)); )
		count += e->erase_cannon(x, y);
	if(count)
		wradar->update(x, y);
	return count;
}

void KOBO_enemies::hit_map(int x, int y, int dmg)
{
	if(!dmg)
		return;
	for(KOBO_enemy *e = NULL; (e = next(e)); )
		if(e->can_hit_map(x, y))
			e->hit(dmg);
}

int KOBO_enemies::exist_pipe()
{
	int count = 0;
	for(KOBO_enemy *e = NULL; (e = next(e)); )
		if(e->is_pipe())
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
	for(KOBO_enemy *e = NULL; (e = next(e)); )
	{
		if(!e->can_splash_damage())
			continue;
		if(!e->in_range(x, y, maxdist, dist))
			continue;
		int dmg = damage * ENEMY_SPLASH_DAMAGE_MULTIPLIER *
				(maxdist - dist) / maxdist;
		if(dmg)
			e->hit(dmg);
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
