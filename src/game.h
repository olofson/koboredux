/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 2002, 2007, 2009 David Olofson
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
#define MAX_BOLTS		150

// In XKobo, WSIZE was used where VIEWLIMIT is used now; in the game logic
// code. The original value was 224.
//
// Kobo Redux replaces WSIZE with WMAIN_W and WMAIN_H, but VIEWLIMIT is still
// what that determines what the game logic considers "in view!" The value is
// changed to 336, to match the new view size.
#define VIEWLIMIT		336

// Player ship hit rect/circle size
#define HIT_MYSHIP_NORMAL	5
#define HIT_MYSHIP_SHIELD	14

// Player shield timings (logic frames)
#define	MYSHIP_SHIELD_DURATION	100
#define	MYSHIP_SHIELD_WARNING	30

// Player bolt hit rect size
#define HIT_BOLT		5

// Player ship vs base bounciness (24:8; 256 ==> 100% energy preserved)
#define	KOBO_BASE_BOUNCE	256

// Player ship vs enemy ship bounciness (24:8; 256 ==> 100% energy preserved)
#define	KOBO_ENEMY_BOUNCE	256

// Relative impact velocity at which nominal ram damage is given
#define	NOMINAL_DAMAGE_SPEED	4

// Launch speed for enemies with launchspeed == 0
#define	DEFAULT_LAUNCH_SPEED	4

// Gameplay debug tools
#undef	INVULNERABLE	// Player ship cannot take damage
#undef	NOSPEEDLIMIT	// Player ship controlled via acceleration; no drag

// Screen shake frequencies (Hz)
#define	SCREEN_SHAKE_RATE_X	9
#define	SCREEN_SHAKE_RATE_Y	11


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
	SKILL_NEWBIE,
	SKILL_EASY,
	SKILL_NORMAL,
	SKILL_HARD,
	SKILL_INSANE
};

#define	KOBO_DEFAULT_SKILL	SKILL_NORMAL


class game_t
{
  public:
	int	type;		// game_types_t
	int	skill;		// skill_levels_t

	// Master game parameters
	int	speed;		// ms per logic frame
	int	launch_speed;	// Enemy launch speed scale factor (24:8)
	int	bullet_speed;	// Enemy bullet speed scale factor (24:8)
	int	core_destroyed_health_bonus;
	int	stage_cleared_health_bonus;
	int	splash_damage_multiplier;

	// Player ship: Health and damage
	int	top_speed;	// Maximum speed when pushing stick (24:8)
	int	cruise_speed;	// Speed with stick in neutral (24:8)
	int	max_health;	// Maximum health (boost)
	int	health;		// Initial/full health
	int	regen_step;	// Health regeneration step
	int	health_fade;	// Health fade period (logic frames/unit)

	// Player ship: Velocity dependent damage
	int	vdmg_minvel;	// Minimum velocity for crash/ram damage (24:8)
	int	vdmg_maxvel;	// Velocity cap for crash/ram damage (24:8)
	int	vdmg_linear;	// Linear velocity->damage scaling (24:8)
	int	vdmg_quadratic;	// Quadratic (true) vel-dmg scaling (24:8)
	int	ram_damage;	// Damage player inflicts when colliding with
				// another object
	int	crash_damage;	// Damage inflicted on ship when hitting a base
				// at full speed

	// Player guns: Accumulator
	int	initial_charge;	// Boost capacitor charge on a new ship
	int	level_charge;	// Charge when entering new level (-1 to leave
				// the charge unchanged)
	int	charge;		// Initial/full charge
	int	charge_rate;	// Boost capacitor charge rate

	// Player guns: Primary
	int	bolt_speed;	// Nominal bolt speed
	int	bolt_damage;	// Damage inflicted by player fire bolt
	int	bolt_drain;	// Normal bolt capacitor drain
	int	bolt_range;	// Max distance player bolts can travel
	int	noseloadtime;	// logic frames per nose shot
	int	tailloadtime;	// logic frames per tail shot

	// Player guns: Secondary
	int	charge_min;	// Minium power of one charged shot
	int	charge_max;	// Maximum power of one charged shot
	int	charge_drain;	// Charge fire capacitor drain (per bolt)
	int	charge_spread;	// Charge fire spread (degrees)
	int	charge_cooldown;// Charge fire cooldown time (logic frames)

	// Enemies
	int	rock_health;
	int	rock_damage;
	int	core_health;
	int	node_health;
	int	enemy_m_health;
	int	bomb_delay;	// Bomb trigger-to-detonation delay

	game_t();
	void reset();
	void set(game_types_t tp, skill_levels_t sk);

	int scale_vel_damage(int vel, int dmg);
};

extern game_t game;

#endif /*_KOBO_GAME_H_*/
