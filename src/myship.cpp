/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2007, 2009 David Olofson
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
#include "kobolog.h"
#include "screen.h"
#include "myship.h"
#include "enemies.h"
#include "gamectl.h"
#include "manage.h"
#include "random.h"
#include "sound.h"

KOBO_myship_state KOBO_myship::_state;
int KOBO_myship::shield_timer = 0;
KOBO_player_controls KOBO_myship::ctrl;
int KOBO_myship::di;
int KOBO_myship::fdi;
int KOBO_myship::turret_di;
int KOBO_myship::dframes;
int KOBO_myship::tframes;
int KOBO_myship::x;
int KOBO_myship::y;
int KOBO_myship::vx;
int KOBO_myship::vy;
int KOBO_myship::ax;
int KOBO_myship::ay;
int KOBO_myship::hitsize;
int KOBO_myship::_health;
int KOBO_myship::_charge;
float KOBO_myship::charge_blipp_granularity;
int KOBO_myship::charged_cooltimer;
int KOBO_myship::blossom_cooltimer;
int KOBO_myship::health_time;
int KOBO_myship::nose_reload_timer;
int KOBO_myship::tail_reload_timer;
KOBO_player_bolt KOBO_myship::bolts[MAX_BOLTS];
cs_obj_t *KOBO_myship::object;


KOBO_myship::KOBO_myship()
{
	memset(bolts, 0, sizeof(bolts));
	object = NULL;
	
	init(true);
}

void KOBO_myship::state(KOBO_myship_state s)
{
	if(prefs->cheat_shield && (s == SHIP_NORMAL))
		s = SHIP_INVULNERABLE;
	switch (s)
	{
	  case SHIP_DEAD:
		if(object)
			gengine->free_obj(object);
		object = NULL;
		shield_timer = 0;
		break;
	  case SHIP_SHIELD:
		shield_timer += MYSHIP_SHIELD_DURATION;
		// Fall-through
	  case SHIP_INVULNERABLE:
	  case SHIP_NORMAL:
		if(!object)
			object = gengine->get_obj(LAYER_PLAYER);
		// HACK: We don't use this for rendering in the normal fashion,
		// because we want to filter rotation at the rendering frame
		// rate. The engine should provide custom rendering callbacks
		// for stuff like this... (Same deal with the bolts!)
		if(object)
		{
			cs_obj_show(object);
			cs_obj_hide(object);
		}
		break;
	}
	switch (s)
	{
	  case SHIP_DEAD:
	  case SHIP_NORMAL:
		sound.g_player_shield(false);
		break;
	  case SHIP_SHIELD:
	  case SHIP_INVULNERABLE:
		sound.g_player_shield(true);
		break;
	}
	_state = s;
}


int KOBO_myship::shield_time()
{
	if(_state == SHIP_INVULNERABLE)
		return MYSHIP_SHIELD_DURATION;
	else return shield_timer;
}


void KOBO_myship::off()
{
	state(SHIP_DEAD);
	int i;
	for(i = 0; i < MAX_BOLTS; i++)
		if(bolts[i].object)
			gengine->free_obj(bolts[i].object);
	memset(bolts, 0, sizeof(bolts));
}


void KOBO_myship::init(bool newship)
{
	ctrl = KOBO_PC_NONE;
	shield_timer = 0;

	// Ship orientation and number of rotation frames
	di = 1;
	if(s_bank_t *b = s_get_bank(gengine->get_gfx(), B_PLAYER))
		dframes = b->max + 1;
	else
		dframes = 8;
	fdi = ((di - 1) * dframes << 8) / 8;

	// Turret orientation and number of rotation frames
	turret_di = 0;
	if(s_bank_t *b = s_get_bank(gengine->get_gfx(), B_PTURRET1))
		tframes = b->max + 1;
	else
		tframes = 8;

	x = PIXEL2CS(WORLD_SIZEX >> 1);
	y = PIXEL2CS((WORLD_SIZEY >> 2) * 3);
	vx = vy = 0;
	ax = ay = 0;

	hitsize = HIT_MYSHIP_NORMAL;

	if(newship)
	{
		_charge = game.initial_charge;
		_health = game.health;
	}
	else
	{
		if(game.level_charge >= 0)
			_charge = game.level_charge;
	}
	charge_blipp_granularity = (float)wcharge->led_count() / game.charge;

	health_time = charged_cooltimer = blossom_cooltimer = 0;
	nose_reload_timer = tail_reload_timer = 0;

	int i;
	for(i = 0; i < MAX_BOLTS; i++)
		if(bolts[i].object)
			gengine->free_obj(bolts[i].object);
	memset(bolts, 0, sizeof(bolts));

	// Now we're ready to dive in and play!
	state(SHIP_NORMAL);
	apply_position();
}


void KOBO_myship::init(int rhealth, int rcharge)
{
	init(false);
	_health = rhealth;
	_charge = rcharge;
}


void KOBO_myship::explode()
{
	KOBO_ParticleFXDef *pfxd = themedata.pfxdef(KOBO_PFX_PLAYER_DEATH);
	if(pfxd)
		wfire->Spawn(x, y, 0, 0, pfxd);
}


KOBO_player_controls KOBO_myship::decode_input()
{
	int c = 0;
	if(gamecontrol.dir_push())
		c |= gamecontrol.dir();
	c |= gamecontrol.aim() << 8;
	if(gamecontrol.pressed(BTN_PRIMARY) || gamecontrol.down(BTN_PRIMARY))
		c |= KOBO_PC_PRIMARY;
	if(prefs->tertiary_button)
	{
		// Triple fire button mode: Separate secondary/tertiary
		if(gamecontrol.pressed(BTN_SECONDARY))
			c |= KOBO_PC_SECONDARY;
		if(gamecontrol.pressed(BTN_TERTIARY))
			c |= KOBO_PC_TERTIARY;
	}
	else
	{
		// Dual fire button mode: Release stick to fire tertiary
		if(gamecontrol.pressed(BTN_SECONDARY) ||
				gamecontrol.pressed(BTN_TERTIARY))
		{
			if(gamecontrol.dir_push())
				c |= KOBO_PC_SECONDARY;
			else
				c |= KOBO_PC_TERTIARY;
		}
	}
	return (KOBO_player_controls)c;
}


void KOBO_myship::handle_controls()
{
	int v;
	if(prefs->cheat_pushmove)
		v = KOBO_PC_DIR(ctrl) ? PIXEL2CS(4) : 0;
	else if(KOBO_PC_DIR(ctrl))
		v = game.top_speed;
	else
		v = game.cruise_speed;
	if(KOBO_PC_DIR(ctrl))
		di = KOBO_PC_DIR(ctrl);
	turret_di = KOBO_PC_AIM(ctrl);
	int tvx = v * sin(M_PI * (di - 1) / 4);
	int tvy = -v * cos(M_PI * (di - 1) / 4);
	if(KOBO_PC_DIR(ctrl) || !prefs->cheat_freewheel)
	{
		if(prefs->cheat_pushmove)
		{
			ax = (tvx - vx) >> 3;
			ay = (tvy - vy) >> 3;
		}
		else
		{
			ax = (tvx - vx) >> 2;
			ay = (tvy - vy) >> 2;
			if(!tvx)
			{
				// Kill remainder creep caused by truncation
				if((vx > -8) && (vx < 8))
					ax = -vx;
			}
			if(!tvy)
			{
				if((vy > -8) && (vy < 8))
					ay = -vy;
			}
		}
	}
	else
		ax = ay = 0;
}


void KOBO_myship::fire_control()
{
	// Weapon charge capacitor charging
	float prevc = _charge;
	_charge += game.charge_rate;
	if(_charge > game.charge)
		_charge = game.charge;
	if(charged_cooltimer)
		--charged_cooltimer;
	if(blossom_cooltimer)
		--blossom_cooltimer;

	// Primary fire logic
	if(ctrl & KOBO_PC_PRIMARY)
	{
		int fired = 0;
		if(tail_reload_timer > 0)
			--tail_reload_timer;
		else
		{
			tail_fire();
			fired = 1;
		}
		if(nose_reload_timer > 0)
			--nose_reload_timer;
		else
		{
			nose_fire();
			fired = 1;
		}
		if(fired)
		{
			_charge -= game.bolt_drain;
			if(_charge < 0)
				_charge = 0;
			sound.g_player_fire();
		}
	}
	else
	{
		if(nose_reload_timer > 0)
			--nose_reload_timer;
		if(tail_reload_timer > 0)
			--tail_reload_timer;
	}

	// Secondary fire (no autofire!)
	if(ctrl & KOBO_PC_SECONDARY)
		charged_fire(di);

	// Tertiary fire (no autofire!)
	if(ctrl & KOBO_PC_TERTIARY)
		fire_blossom();

	// Charge bar blipps
	if(ceil(_charge * charge_blipp_granularity) >
			ceil(prevc * charge_blipp_granularity))
		sound.g_player_charge((float)_charge / game.charge);
}


void KOBO_myship::move()
{
	// Health regeneration/overcharge fade
	if(++health_time >= game.health_fade)
	{
		health_time = 0;
		if(_health > game.health)
		{
			// Fade when boosted above 100%
			--_health;
		}
		else if((_health > 0) && (_health < game.health))
		{
			// Regeneration to next threshold
			int rn = regen_next();
			if((_health != rn) && (_health < rn))
				++_health;
		}
	}

	// Shield timing
	if(_state == SHIP_SHIELD)
		if(--shield_timer <= 0)
			state(SHIP_NORMAL);

	// Hitrect/bounding circle size
	switch(_state)
	{
	  case SHIP_DEAD:
	  case SHIP_NORMAL:
		if(hitsize > HIT_MYSHIP_NORMAL)
			--hitsize;
		break;
	  case SHIP_SHIELD:
	  case SHIP_INVULNERABLE:
		if(hitsize < HIT_MYSHIP_SHIELD)
			++hitsize;
		break;
	}

	// Movement and collisions
	switch(_state)
	{
	  case SHIP_NORMAL:
	  case SHIP_SHIELD:
	  case SHIP_INVULNERABLE:
		handle_controls();
		update_position();
		break;
	  case SHIP_DEAD:
		_charge = 0;
		break;
	}

	// Wrapping
	x &= PIXEL2CS(WORLD_SIZEX) - 1;
	y &= PIXEL2CS(WORLD_SIZEY) - 1;

	// Fire control
	if(_state != SHIP_DEAD)
		fire_control();

	// Bolts
	for(int i = 0; i < MAX_BOLTS; i++)
	{
		if(!bolts[i].state)
			continue;
		if(bolts[i].state <= 2)
		{
			bolts[i].x += vx;
			bolts[i].y += vy;
			bolts[i].x += (bolts[i].dx - vx) *
					(bolts[i].state - 1) >> 1;
			bolts[i].y += (bolts[i].dy - vy) *
					(bolts[i].state - 1) >> 1;
		}
		else
		{
			bolts[i].x += bolts[i].dx;
			bolts[i].y += bolts[i].dy;
		}
		++bolts[i].state;
		bolts[i].x &= PIXEL2CS(WORLD_SIZEX) - 1;
		bolts[i].y &= PIXEL2CS(WORLD_SIZEY) - 1;
		int dx = labs(WRAPDISTX(CS2PIXEL(bolts[i].x), CS2PIXEL(x)));
		int dy = labs(WRAPDISTY(CS2PIXEL(bolts[i].y), CS2PIXEL(y)));
		if(dx >= game.bolt_range || dy >= game.bolt_range)
			kill_bolt(i, false);
	}
	// NOTE: We can't check bolt/base collisions here, as that would kill
	//       bolts before core (enemy) collisions are checked!
}


void KOBO_myship::hit(int dmg)
{
	if(!dmg)
		return;

	// Always trigger damage sfx; sound layer handles damage/shield
	if(dmg < game.health / 2)
		sound.g_player_damage((float)dmg / game.health * 2.0f);
	else
		sound.g_player_damage(1.0f);

	switch(_state)
	{
	  case SHIP_SHIELD:
	  case SHIP_INVULNERABLE:
		return;
	  default:
		break;
	}

	if(!prefs->cheat_invulnerability)
	{
		if(_health && (dmg < _health))
			manage.noise_damage((float)dmg / _health);
		else
			manage.noise_damage(1.0f);

		_health -= dmg;

		// Be nice if we land right on a regeneration threshold level.
		if(_health == regen_next())
			++_health;
	}
	else
		log_printf(ULOG, "INVULNERABLE: Ignored %d damage to "
				"player.\n", dmg);

	if(_health <= 0)
	{
		_health = 0;
		manage.screenshake(2.0f, 2.0f, 0.99f);
		manage.lost_myship();
		state(SHIP_DEAD);
		explode();
		sound.g_player_explo_start();
	}
	else
	{
		float sh = (float)dmg / (_health + dmg);
		manage.screenshake(sh, sh, 0.8f);
	}
}


void KOBO_myship::health_bonus(int h)
{
	if(_state == SHIP_DEAD)
		return;

	_health += game.health * h / 100;
	health_time = 0;
	if(_health > game.max_health)
		_health = game.max_health;
}


void KOBO_myship::kill_bolt(int bolt, bool impact)
{
	if(impact)
		enemies.make(&boltexpl, bolts[bolt].x, bolts[bolt].y);
	bolts[bolt].state = 0;
	if(bolts[bolt].object)
	{
		gengine->free_obj(bolts[bolt].object);
		bolts[bolt].object = NULL;
	}
}


// Check and handle base/bolt collisions
void KOBO_myship::check_base_bolts()
{
	for(int i = 0; i < MAX_BOLTS; i++)
	{
		if(!bolts[i].state)
			continue;

		int x1 = WORLD2MAPX(CS2PIXEL(bolts[i].x));
		int y1 = WORLD2MAPY(CS2PIXEL(bolts[i].y));
		if(IS_BASE(screen.get_map(x1, y1)))
		{
			sound.g_play(S_NO_DAMAGE, x1 << 4, y1 << 4);
			kill_bolt(i, true);
		}
	}
}


// Calculate bounce
static inline void calc_bounce(int p2, int *p3, int *v)
{
	*p3 = p2 + ((KOBO_BASE_BOUNCE) * (p2 - *p3) >> 8);
	*v = -((KOBO_BASE_BOUNCE) * *v >> 8);
}


// Check and handle base/ship collisions
void KOBO_myship::update_position()
{
	// Approximation: Apply half of the acceleration before collision
	// handling, and half after, to avoid continuous acceleration fun.
	vx += ax / 2;
	vy += ay / 2;

	// Calculate target for the "no collisions" case
	int x3 = x + vx;
	int y3 = y + vy;

	bool contact = 0;	// 'true' in case of collisions or contact
	int impact = 0;		// Collision velocity
	while(1)
	{
		// Find first collision, if any
		int x2, y2, hx, hy;
		int cd = screen.test_line(x, y, x3, y3, &x2, &y2, &hx, &hy);
		if(!cd)
		{
			// No (more) collisions!
			x = x3;
			y = y3;
			vx += ax / 2;
			vy += ay / 2;
			break;
		}

		contact = true;

		if(cd & COLL_STUCK)
			break;	// Stuck inside tile!

		// With bounce enabled:
		//	On collision, mirror the target position over the
		//	collision line, move the current position to the point
		//	of collision, and try again!
		//
		// With bounce disabled:
		//	Set the current and target positions to tho point of
		//	collision, and stop!
		//
		int imp = 0;
		if(cd & COLL_VERTICAL)
		{
			imp += labs(vx);
			calc_bounce(x2, &x3, &vx);
		}
		if(cd & COLL_HORIZONTAL)
		{
			imp += labs(vy);
			calc_bounce(y2, &y3, &vy);
		}

		// Deal velocity based damage to any fixed enemies attached to
		// the tile we just hit.
		enemies.hit_map(WORLD2MAPX(CS2PIXEL(hx)),
				WORLD2MAPY(CS2PIXEL(hy)),
				game.scale_vel_damage(imp, game.ram_damage));
		impact += imp;
		x = x2;
		y = y2;
	}
#ifdef DEBUG
	if(IS_BASE(screen.get_map(WORLD2MAPX(CS2PIXEL(x)),
			WORLD2MAPY(CS2PIXEL(y)))))
		log_printf(WLOG, "Bounce calculation failed at (%x, %x)\n",
				x, y);
#endif

	if(contact)
	{
#if 0
		// TODO: Grinding noise.
		float grind = sqrt(vx*vx + vy*vy) / game.top_speed;
#endif
	}

	// Deal velocity based damage to the ship
	hit(game.scale_vel_damage(impact, game.crash_damage));
}


int KOBO_myship::hit_bolt(int ex, int ey, int hitsize, int health)
{
	int dmg = 0;
	int i;
	for(i = 0; i < MAX_BOLTS; i++)
	{
		if(bolts[i].state == 0)
			continue;
		if(labs(WRAPDISTX(ex, CS2PIXEL(bolts[i].x))) >= hitsize)
			continue;
		if(labs(WRAPDISTY(ey, CS2PIXEL(bolts[i].y))) >= hitsize)
			continue;
		kill_bolt(i, true);
		dmg += game.bolt_damage;
		if(dmg >= health)
			break;
	}
	return dmg;
}


int KOBO_myship::put()
{
	// Player
	apply_position();

	// Bullets
	int i;
	for(i = 0; i < MAX_BOLTS; i++)
	{
		if(!bolts[i].object)
			continue;
		if(bolts[i].state)
		{
			bolts[i].object->point.v.x = bolts[i].x;
			bolts[i].object->point.v.y = bolts[i].y;
		}
	}
	return 0;
}


void KOBO_myship::force_position()
{
	// Player
	if(object)
	{
		apply_position();
		cs_point_force(&object->point);
	}

	// Bullets
	for(int i = 0; i < MAX_BOLTS; i++)
	{
		if(!bolts[i].object)
			continue;
		if(bolts[i].state)
		{
			bolts[i].object->point.v.x = bolts[i].x;
			bolts[i].object->point.v.y = bolts[i].y;
			cs_point_force(&bolts[i].object->point);
		}
	}
}


void KOBO_myship::render()
{
	if(!myship.object)
		return;
	
	// Render player ship
	int maxd = dframes << 8;
	int tdi = ((di - 1) * dframes << 8) / 8 + 127;
	int ddi = tdi - fdi;
	if(ddi < 0)
		ddi = maxd - (-ddi) % maxd;
	else
		ddi %= maxd;
	if(ddi > maxd / 2)
		ddi -= maxd;
	fdi += ddi * gengine->frame_delta_time() * 0.02f;
	fdi = (fdi + maxd) % maxd;
	wmain->sprite_fxp(object->point.gx, object->point.gy,
			B_PLAYER, fdi >> 8);

	// Render bolts
	int i;
	for(i = 0; i < MAX_BOLTS; i++)
	{
		if(!bolts[i].state || !bolts[i].object)
			continue;

		wmain->sprite_fxp(bolts[i].object->point.gx,
				bolts[i].object->point.gy, B_BOLT,
				bolt_frame(bolts[i].dir, bolts[i].state - 2));
	}

	// Render turret
	int tframe = turret_di * tframes / AIM_RESOLUTION;
	if(nose_reload_timer <= 0)
	{
#if 0
		// Recoil!
		// calculate the angle's vector
		float tempAng = (float)shipTurr.angle;
		float tempX = sin(tempAng);
		float tempY = cos(tempAng);
#endif
		wmain->sprite_fxp_scale(object->point.gx, object->point.gy,
				B_PTURRET1, tframe, 1.0f, 1.0f);
	}
	else
	{
		wmain->sprite_fxp_scale(object->point.gx, object->point.gy,
				B_PTURRET1, tframe, 1.0f, 1.0f);
	}

	// Render shield
	switch(_state)
	{
	  case SHIP_SHIELD:
		if(shield_timer < MYSHIP_SHIELD_WARNING)
			if(shield_timer & 2)
				break;
		// Fall-through
	  case SHIP_INVULNERABLE:
		wmain->sprite_fxp(object->point.gx, object->point.gy,
				B_SHIELDFX, manage.game_time() % 8);
	  default:
		break;
	}

	if(prefs->show_hit)
	{
		int r = PIXEL2CS(hitsize);
		wmain->foreground(wmain->map_rgb(128, 0, 128));
		wmain->blendmode(GFX_BLENDMODE_ADD);
		wmain->hairrect_fxp(object->point.gx - r,
				object->point.gy - r, 2 * r, 2 * r);
		wmain->circle_fxp(object->point.gx, object->point.gy, r);
		wmain->blendmode();
	}
}


void KOBO_myship::shot_single(float doffset, int loffset, int hoffset,
		float boltdir, int boltspeed)
{
	int i;
	for(i = 0; i < MAX_BOLTS && bolts[i].state; i++)
		;
	if(i >= MAX_BOLTS)
	{
		log_printf(WLOG, "Out of player bolts!\n");
		return;
	}
	bolts[i].state = 1;

	// Bolt spawn position
	int sdi = sin(M_PI * (doffset - 1) / 4) * 256.0f;
	int cdi = cos(M_PI * (doffset - 1) / 4) * 256.0f;
	bolts[i].x = x - vx + loffset * sdi + hoffset * cdi;
	bolts[i].y = y - vy - loffset * cdi + hoffset * sdi;
	int sp = game.bolt_speed * boltspeed;

	// Bolt direction and velocity
	if(boltdir < 0.0f)
		boltdir = doffset;
	bolts[i].dir = boltdir + .5f;
	sdi = sin(M_PI * (boltdir - 1) / 4) * 256.0f;
	cdi = cos(M_PI * (boltdir - 1) / 4) * 256.0f;
	bolts[i].dx = vx + (sp * sdi >> 16);
	bolts[i].dy = vy - (sp * cdi >> 16);
	if(!bolts[i].object)
		bolts[i].object = gengine->get_obj(LAYER_PLAYER);
	if(bolts[i].object)
	{
		bolts[i].object->point.v.x = bolts[i].x;
		bolts[i].object->point.v.y = bolts[i].y;
		cs_obj_show(bolts[i].object);
		cs_obj_hide(bolts[i].object);
	}
}


void KOBO_myship::nose_fire()
{
	shot_single(di + GUN_NOSE_DIR - 1, GUN_NOSE_Y, GUN_NOSE_X,
			(float)turret_di * 8 / AIM_RESOLUTION + 1);

	// reload timer
	nose_reload_timer = game.noseloadtime;
}


void KOBO_myship::tail_fire()
{
	shot_single((di + GUN_TAIL_DIR - 1) % 8 + 1, GUN_TAIL_Y, GUN_TAIL_X);
	tail_reload_timer = game.tailloadtime;
}


bool KOBO_myship::secondary_available()
{
	return !charged_cooltimer && (_charge >= game.charged_min);
}


bool KOBO_myship::tertiary_available()
{
	return !blossom_cooltimer && (_charge >= game.blossom_min);
}


void KOBO_myship::charged_fire(int dir)
{
	if(!secondary_available())
	{
		sound.g_player_fire_denied();
		return;
	}

	int power = _charge > game.charged_max ? game.charged_max : _charge;
	int maxbolts = game.charged_max / game.charged_drain;
	sound.g_player_charged_fire((float)power / game.charged_max);
	for(int b = 0; power >= game.charged_drain; ++b)
	{
		power -= game.charged_drain;
		_charge -= game.charged_drain;
		float spread = gamerand.get(16) * (8.0f / 65536.0f) - 4.0f;
		spread *= game.charged_spread * (1.0f / 360.0f);
		int cx = gamerand.get(3) - 4;
		shot_single(dir + spread, GUN_NOSE_Y - labs(cx), cx,
				-1.0f, (b + maxbolts * 2) * 16384 / maxbolts);
	}
	charged_cooltimer = game.charged_cooldown;
}


void KOBO_myship::fire_blossom()
{
	if(!tertiary_available())
	{
		sound.g_player_fire_denied();
		return;
	}

	int power = _charge > game.blossom_max ? game.blossom_max : _charge;
	sound.g_player_charged_fire((float)power / game.blossom_max);
	int n = power / game.blossom_drain;
	if(!n)
	{
		log_printf(WLOG, "fire_blossom() with no bolts! "
				"Game parameter bug?\n");
		return;
	}
	float dir = 1.0f;
	float spread = 8.0f / n;
	for(int b = 0; b < n; ++b, dir += spread)
		shot_single(dir, GUN_BLOSSOM_R, 0,
				-1.0f, (gamerand.get(3) + 8) << 12);
	_charge -= game.blossom_drain * n;
	blossom_cooltimer = game.blossom_cooldown;
}


void KOBO_myship::set_position(int px, int py)
{
	x = PIXEL2CS(px);
	y = PIXEL2CS(py);
	if(object)
	{
		apply_position();
		cs_point_force(&object->point);
	}
}


void KOBO_myship::apply_position()
{
	if(object)
	{
		object->point.v.x = x;
		object->point.v.y = y;
	}
}


bool KOBO_myship::in_range(int px, int py, int range, int &dist)
{
	int dx = labs(WRAPDISTXCS(x, px));
	if(dx > range)
		return false;
	int dy = labs(WRAPDISTYCS(y, py));
	if(dy > range)
		return false;
	dist = sqrt(dx*dx + dy*dy);
	return dist <= range;
}
