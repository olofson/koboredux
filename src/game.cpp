/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2002, 2007, 2009 David Olofson
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

#include "config.h"
#include "game.h"

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
	bonus_first = 0;
	bonus_every = 0;

	// Player ship health and damage
	lives = 1;
	health = 100;
	max_health = 200;
	regen_step = 20;
	health_fade = 10;

	// Player guns
	bolts = MAX_BOLTS;
	bolt_range = (VIEWLIMIT >> 1) + 16 + 32;
	noseloadtime = 1;
	altfire = 0;
	tailloadtime = 1;

	// The overheat logic is no longer used in Kobo Redux!
	noseheatup = 0;
	nosecooling = 256;
	tailheatup = 0;
	tailcooling = 256;

	switch(skill)
	{
	  case SKILL_NORMAL:
		damage = 100;
		bolt_damage = 40;
		rock_health = 500;
		rock_damage = 40;
		core_health = 100;
		core_destroyed_health_bonus = 25;
		stage_cleared_health_bonus = 25;
		break;
	  case SKILL_HARD:
		damage = 50;
		bolt_damage = 30;
		rock_health = 1000;
		rock_damage = 100;
		core_health = 200;
		core_destroyed_health_bonus = 10;
		stage_cleared_health_bonus = 25;
		break;
	  case SKILL_INSANE:
	  default:
		damage = 30;
		bolt_damage = 20;
		rock_health = HEALTH_INDESTRUCTIBLE;
		rock_damage = 1000;
		core_health = 300;
		core_destroyed_health_bonus = 10;
		stage_cleared_health_bonus = 10;
		break;
	}
}
