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
	set(GAME_SINGLE, SKILL_CLASSIC);
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
	regen_step = 100;
	health_fade = 10;
	damage = 100;

	// Player guns
	bolts = MAX_BOLTS;
	bolt_damage = 20;
	bolt_range = (VIEWLIMIT >> 1) + 16 + 32;
	noseloadtime = 1;
	altfire = 0;
	tailloadtime = 1;

	// The overheat logic is no longer used in Kobo Redux!
	noseheatup = 0;
	nosecooling = 256;
	tailheatup = 0;
	tailcooling = 256;

	// Enemies
	rock_health = HEALTH_INDESTRUCTIBLE;
	rock_damage = 1000;
	core_health = 100;

	switch(skill)
	{
	  case SKILL_CLASSIC:
		lives = 5;
		bonus_first = 2000;
		bonus_every = 3000;
		health = 1;
		max_health = 1;
		damage = 0;
		bolts = 10;
		rock_health = 255 * bolt_damage;
		core_health = bolt_damage;
		core_destroyed_health_bonus = 0;
		stage_cleared_health_bonus = 0;
		break;
	  case SKILL_NEWBIE:
		speed = 40;
		tailloadtime = 0;
		altfire = 1;
		rock_health = 200;
		rock_damage = 50;
		core_health = 40;
		core_destroyed_health_bonus = 50;
		stage_cleared_health_bonus = 50;
		break;
	  case SKILL_GAMER:
		max_health = 175;
		regen_step = 40;
		noseloadtime = 0;
		rock_health = 500;
		rock_damage = 32;
		core_destroyed_health_bonus = 25;
		stage_cleared_health_bonus = 25;
		break;
	  case SKILL_ELITE:
		speed = 27;
		max_health = 150;
		regen_step = 20;
		damage = 50;
		rock_health = 1000;
		tailloadtime = 2;
		core_health = 200;
		core_destroyed_health_bonus = 10;
		stage_cleared_health_bonus = 25;
		break;
	  case SKILL_GOD:
	  default:
		speed = 25;
		max_health = 125;
		regen_step = 16;
		damage = 30;
		tailloadtime = 2;
		core_health = 300;
		core_destroyed_health_bonus = 10;
		stage_cleared_health_bonus = 10;
		break;
	}
}
