/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 2002, 2007, 2009 David Olofson
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

#ifndef	_KOBO_GAME_H_
#define	_KOBO_GAME_H_


  ////////////////////////////////////////////////////
 // Constant game parameters
////////////////////////////////////////////////////

#define	ENEMY_SPLASH_DAMAGE_MULTIPLIER	2


// This was originally 1024, but was changed in Kobo Deluxe 0.4.1 to avoid the
// bug where we run out of enemies when destroying a base, and thus leave
// parts of it behind.
// FIXME: Is 2048 actually enough with the new effects in 0.5.x+...?
#define ENEMY_MAX		2048

// Max number of player bolts in... space at once
#define MAX_BOLTS		40

// In XKobo, WSIZE was used where VIEWLIMIT is used now; in the game logic
// code. The original value was 224.
//
// Kobo Redux replaces WSIZE with WMAIN_W and WMAIN_H, but VIEWLIMIT is still
// what that determines what the game logic considers "in view!" The value is
// changed to 336, to match the new view size.
#define VIEWLIMIT		336

// Player ship hit rect size
#define HIT_MYSHIP		5

// Player bolt hit rect size
#define HIT_BOLT		5

// Minimum speed for impact damage (fraction of game.top_speed)
#define	KOBO_MIN_DAMAGE_SPEED	0.5f

// Define to have the player ship bounce off bases, rather than cling to them
#define	BASE_BOUNCE

// Launch speed for enemies with launchspeed == 0
#define	DEFAULT_LAUNCH_SPEED	4

// Gameplay debug tools
#undef	INVULNERABLE	// Player ship cannot take damage
#undef	NOSPEEDLIMIT	// Player ship controlled via acceleration; no drag
#undef	NOENEMYFIRE	// Enemies don't fire bullets at the player


  ////////////////////////////////////////////////////
 // Skill level dependent game parameters
////////////////////////////////////////////////////

// Special values for game.health
#define	HEALTH_INDESTRUCTIBLE	9000000
#define	HEALTH_DESTROY		(HEALTH_INDESTRUCTIBLE - 1)

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
	int	splash_damage_multiplier;

	// Player ship health and damage
	int	top_speed;	// Maximum speed when pushing stick
	int	cruise_speed;	// Speed with stick in neutral
	int	max_health;	// Maximum health (boost)
	int	health;		// Initial health
	int	regen_step;	// Health regeneration step
	int	health_fade;	// Health fade period (logic frames/unit)
	int	ram_damage;	// Damage player inflicts when colliding with
				// another object
	int	crash_damage;	// Damage inflicted on ship when hitting a base
				// at full speed

	// Player guns
	int	bolt_speed;	// Nominal bolt speed
	int	bolt_damage;	// Damage inflicted by player fire bolt
	int	bolt_range;	// Max distance player bolts can travel
	int	noseloadtime;	// logic frames per nose shot
	int	tailloadtime;	// logic frames per tail shot

	// Enemies
	int	rock_health;
	int	rock_damage;
	int	core_health;
	int	node_health;

	game_t();
	void reset();
	void set(game_types_t tp, skill_levels_t sk);

	int scale_vel_damage(int vel, int dmg);
};

extern game_t game;

#endif /*_KOBO_GAME_H_*/
