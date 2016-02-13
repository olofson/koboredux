/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2002, 2007, 2009 David Olofson
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
#include "game.h"
#include "cs.h"
#include <math.h>

game_t game;


game_t::game_t()
{
	reset();
}


void game_t::reset()
{
	set(GAME_SINGLE, SKILL_NORMAL);
}


void game_t::set(game_types_t tp, skill_levels_t sk)
{
	// Master game parameters
	type = tp;
	skill = sk;
	speed = 30;
	if(prefs && prefs->cheat_speed)
		speed /= prefs->cheat_speed;

	// Player ship health and damage
	health = 100;
	max_health = 200;
	regen_step = 20;
	health_fade = 10;
	top_speed = 4;
	cruise_speed = 2;

	// Player guns
	bolt_range = (VIEWLIMIT >> 1) + 16 + 32;
	if(prefs && prefs->cheat_firepower)
		noseloadtime = tailloadtime = 0;
	else
		noseloadtime = tailloadtime = 1;

	switch(skill)
	{
	  case SKILL_NORMAL:
		ram_damage = 100;
		bolt_damage = 40;
		rock_health = 500;
		rock_damage = 40;
		crash_damage = 30;
		splash_damage_multiplier = 2;
		core_health = 100;
		node_health = 40;
		core_destroyed_health_bonus = 25;
		stage_cleared_health_bonus = 25;
		break;
	  case SKILL_HARD:
		ram_damage = 50;
		bolt_damage = 30;
		rock_health = 1000;
		rock_damage = 60;
		crash_damage = 40;
		splash_damage_multiplier = 3;
		core_health = 150;
		node_health = 50;
		core_destroyed_health_bonus = 10;
		stage_cleared_health_bonus = 25;
		break;
	  case SKILL_INSANE:
	  default:
		ram_damage = 30;
		bolt_damage = 25;
		rock_health = HEALTH_INDESTRUCTIBLE;
		rock_damage = 80;
		crash_damage = 50;
		splash_damage_multiplier = 5;
		core_health = 200;
		node_health = 60;
		core_destroyed_health_bonus = 10;
		stage_cleared_health_bonus = 10;
		break;
	}
}


int game_t::scale_vel_damage(int vel, int dmg)
{
	float ds = (float)vel / PIXEL2CS(top_speed);
	if(fabs(ds) < KOBO_MIN_DAMAGE_SPEED)
		return 0;
	return dmg * ds*ds;
}
