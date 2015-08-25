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
	type = tp;
	skill = sk;

	// Enemies
	rock_health = 255 * bolt_damage;
	rock_damage = 1000;

	// The overheat logic is no longer used in Kobo Redux!
	noseheatup = 0;
	nosecooling = 256;
	tailheatup = 0;
	tailcooling = 256;

	switch(skill)
	{
	  case SKILL_CLASSIC:
		speed = 30;
		lives = 5;
		bonus_first = 2000;
		bonus_every = 3000;
		health = 1;
		health_fade = 5;
		damage = 0;
		bolts = 10;
		bolt_damage = 20;
		noseloadtime = 1;
		altfire = 0;
		tailloadtime = 1;
		break;
	  case SKILL_NEWBIE:
		speed = 40;
		lives = 1;
		bonus_first = 0;
		bonus_every = 0;
		health = 100;
		damage = 100;
		bolts = MAX_BOLTS;
		noseloadtime = 1;
		tailloadtime = 0;
		altfire = 1;
		rock_health = 200;
		rock_damage = 50;
		break;
	  case SKILL_GAMER:
		speed = 30;
		lives = 1;
		bonus_first = 0;
		bonus_every = 0;
		health = 60;
		damage = 100;
		bolts = MAX_BOLTS;
		noseloadtime = 0;
		tailloadtime = 1;
		rock_health = 500;
		rock_damage = 50;
		break;
	  case SKILL_ELITE:
		speed = 27;
		lives = 1;
		bonus_first = 0;
		bonus_every = 0;
		health = 50;
		damage = 50;
		bolts = MAX_BOLTS;
		noseloadtime = 1;
		tailloadtime = 2;
		break;
	  case SKILL_GOD:
	  default:
		speed = 25;
		lives = 1;
		bonus_first = 0;
		bonus_every = 0;
		health = 40;
		damage = 30;
		bolts = MAX_BOLTS;
		noseloadtime = 1;
		tailloadtime = 2;
		break;
	}
}
