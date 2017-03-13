/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2002, 2007, 2009 David Olofson
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
	type = tp;
	skill = sk;

	// Master game parameters
	speed = 30;
	if(prefs && prefs->cheat_speed)
		speed /= prefs->cheat_speed;

	// Player ship: Health and damage
	top_speed = PIXEL2CS(4);
	cruise_speed = PIXEL2CS(2);
	max_health = 200;
	health = 100;
	regen_step = 20;
	health_fade = 10;

	// Player guns: Accumulator
	initial_charge = level_charge = charge = 1000;
	charge_rate = 10;

	// Player ship: Velocity dependent damage
	vdmg_minvel = PIXEL2CS(1);
	vdmg_maxvel = PIXEL2CS(5);
	vdmg_linear = 128;
	vdmg_quadratic = 128;

	// Player guns: Primary (Nose + Tail)
	bolt_speed = 12;
	bolt_range = (VIEWLIMIT >> 1) + 16 + 32;
	bolt_drain = 15;
	if(prefs && prefs->cheat_firepower)
		noseloadtime = tailloadtime = 0;
	else
		noseloadtime = tailloadtime = 1;

	// Player guns: Secondary (Charged Blast)
	charged_min = 200;
	charged_max = 500;
	charged_drain = 10;
	charged_spread = 3;
	charged_cooldown = 10;

	// Player guns: Tertiary (Fire Blossom)
	blossom_min = 750;
	blossom_max = 750;
	blossom_drain = 5;
	blossom_cooldown = 10;

	switch(skill)
	{
	  case SKILL_NEWBIE:
		// Master game parameters
		launch_speed = 160;	// 0.625
		bullet_speed = 192;	// 0.75
		core_destroyed_health_bonus = 50;
		stage_cleared_health_bonus = 50;
		splash_damage_multiplier = 1;

		// Player ship: Velocity dependent damage
		ram_damage = 100;
		crash_damage = 20;

		// Player guns
		bolt_damage = 50;

		// Enemies
		rock_health = 1000;
		rock_damage = 20;
		core_health = 1000;
		node_health = 40;
		enemy_m_health = 2000;
		bomb_delay = 12;
		break;

	  case SKILL_EASY:
		// Master game parameters
		launch_speed = 192;	// 0.75
		bullet_speed = 224;	// 0.875
		core_destroyed_health_bonus = 35;
		stage_cleared_health_bonus = 35;
		splash_damage_multiplier = 2;

		// Player ship: Velocity dependent damage
		ram_damage = 100;
		crash_damage = 25;

		// Player guns
		bolt_damage = 45;

		// Enemies
		rock_health = 2000;
		rock_damage = 30;
		core_health = 1000;
		node_health = 40;
		enemy_m_health = 2500;
		bomb_delay = 10;
		break;

	  case SKILL_NORMAL:
		// Master game parameters
		launch_speed = 256;	// 1.0
		bullet_speed = 256;	// 1.0
		core_destroyed_health_bonus = 25;
		stage_cleared_health_bonus = 25;
		splash_damage_multiplier = 2;

		// Player ship: Velocity dependent damage
		ram_damage = 100;
		crash_damage = 30;

		// Player guns
		bolt_damage = 40;

		// Enemies
		rock_health = 3000;
		rock_damage = 40;
		core_health = 1000;
		node_health = 40;
		enemy_m_health = 3000;
		bomb_delay = 8;
		break;

	  case SKILL_HARD:
		// Master game parameters
		launch_speed = 352;	// 1.375
		bullet_speed = 384;	// 1.5
		core_destroyed_health_bonus = 10;
		stage_cleared_health_bonus = 25;
		splash_damage_multiplier = 3;

		// Player ship: Velocity dependent damage
		ram_damage = 50;
		crash_damage = 40;

		// Player guns
		bolt_damage = 30;

		// Enemies
		rock_health = 5000;
		rock_damage = 60;
		core_health = 1500;
		node_health = 50;
		enemy_m_health = 4000;
		bomb_delay = 5;
		break;

	  case SKILL_INSANE:
	  default:
		// Master game parameters
		launch_speed = 384;	// 1.5
		bullet_speed = 448;	// 1.75
		core_destroyed_health_bonus = 10;
		stage_cleared_health_bonus = 10;
		splash_damage_multiplier = 5;

		// Player ship: Velocity dependent damage
		ram_damage = 30;
		crash_damage = 50;

		// Player guns
		bolt_damage = 25;

		// Enemies
		rock_health = HEALTH_INDESTRUCTIBLE;
		rock_damage = 80;
		core_health = 2000;
		node_health = 60;
		enemy_m_health = 5000;
		bomb_delay = 2;
		break;
	}
}


int game_t::scale_vel_damage(int vel, int dmg)
{
	vel = labs(vel);

	// Upper velocity limit
	if(vel > vdmg_maxvel)
		vel = vdmg_maxvel;

	// Lower velocity limit/truncation
	//	Note that this will "compress" the response, as we still
	//	normalize to the ship's nominal top speed!
	vel -= vdmg_minvel;
	if(vel <= 0)
		return 0;

	// Normalize to top speed
	int ds = (vel << 8) / top_speed;
	int ds2 = ds * ds >> 8;
	return dmg * (ds * vdmg_linear + ds2 * vdmg_quadratic) >> 16;
}
