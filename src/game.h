/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 2002, 2007, 2009 David Olofson
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

#ifndef	_KOBO_GAME_H_
#define	_KOBO_GAME_H_

// Max number of player bolts flying
#define MAX_BOLTS		40

// Special value for game.health
#define	HEALTH_INDESTRUCTIBLE	9000000

enum game_types_t
{
	GAME_UNKNOWN = -1,
	GAME_SINGLE,
	GAME_SINGLE_NEW,
	GAME_COOPERATIVE,
	GAME_DEATHMATCH,
	GAME_EXPERIMENTAL = 0x10000000	// Testing! No official highscores.
};


enum skill_levels_t
{
	SKILL_UNKNOWN = -1,
	SKILL_NEWBIE,	// Reserved!
	SKILL_EASY,	// Reserved!
	SKILL_NORMAL,
	SKILL_HARD,
	SKILL_INSANE
};

class game_t
{
  public:
	// Master game parameters
	int	type;
	int	skill;
	int	speed;		// ms per logic frame
	int	core_destroyed_health_bonus;
	int	stage_cleared_health_bonus;

	// Player ship health and damage
	int	max_health;	// Maximum health (boost)
	int	health;		// Initial health
	int	regen_step;	// Health regeneration step
	int	health_fade;	// Health fade period (logic frames/unit)
	int	ram_damage;	// Damage player inflicts when colliding with
				// another object

	// Player guns
	int	bolt_damage;	// Damage inflicted by player fire bolt
	int	bolt_range;	// Max distance player bolts can travel
	int	noseloadtime;	// logic frames per nose shot
	int	tailloadtime;	// logic frames per tail shot

	// Enemies
	int	rock_health;
	int	rock_damage;
	int	core_health;

	game_t();
	void reset();
	void set(game_types_t tp, skill_levels_t sk);
};

extern game_t game;

#endif /*_KOBO_GAME_H_*/
