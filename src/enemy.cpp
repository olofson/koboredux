/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2007, 2009 David Olofson
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

#include "screen.h"
#include "manage.h"
#include "enemies.h"
#include "myship.h"
#include "random.h"
#include <math.h>


///////////////////////////////////////////////////////////////////////////////
// Fast integer atan() approximation. Input is 24:8 fixed point.
// Output is 0..64 for 0..45 deg, accurate down to LSB.
//	In:	q = 256 * minv / maxv	(0..256 <==> 0..1)
//	Out:	atan(q / 256) * 256	(0..64 <==> 0..45 deg)
///////////////////////////////////////////////////////////////////////////////
// atan() approximations for 0..90 deg:
//	a = 83 * q / 256 - q*q / 2844				(+/- 2.1%)
//	a = 82 * q / 256 - q*q / 8500 - q*q*q / 1600000		(+/- 0.6%)
//
// FIXME: Better version?
//	a = 84 * q / 256 - 95 * q*q / 524288 - 8 * q*q*q / 16777216
///////////////////////////////////////////////////////////////////////////////
static inline int fastatan(int q)
{
	int q2 = q * q;
	int q3 = q2 * q;
	return (82 * q >> 8) - (62 * q2 >> 19) - (10 * q3 >> 24);
}

static int speed2dir(int sx, int sy, int frames)
{
	if(!sx && !sy)
		return 0;
	int at2;
	if(sx > 0)
	{
		// Right
		if(sy < 0)
		{
			// Top-right quadrant
			sy = -sy;
			if(sy > sx)
				at2 = fastatan(sx * 256 / sy);
			else
				at2 = 128 - fastatan(sy * 256 / sx);
		}
		else
		{
			// Bottom-right quadrant
			if(sx > sy)
				at2 = 128 + fastatan(sy * 256 / sx);
			else
				at2 = 256 - fastatan(sx * 256 / sy);
		}
	}
	else
	{
		// Left
		sx = -sx;
		if(sy > 0)
		{
			// Bottom-left quadrant
			if(sy > sx)
				at2 = 256 + fastatan(sx * 256 / sy);
			else
				at2 = 384 - fastatan(sy * 256 / sx);
		}
		else
		{
			// Top-left quadrant
			sy = -sy;
			if(sx > sy)
				at2 = 384 + fastatan(sy * 256 / sx);
			else
				at2 = 512 - fastatan(sx * 256 / sy);
		}
	}
	at2 = (at2 * frames + 256) >> 9;
	return at2 > frames - 1 ? 1 : at2 + 1;
}


/*
 * ===========================================================================
 *                                (template)
 * ===========================================================================
 */

inline void KOBO_enemy::move_enemy_m(int quick, int maxspeed)
{
	if(diffx > 0)
	{
		if(h > -maxspeed)
			h -= quick;
	}
	else if(diffx < 0)
	{
		if(h < maxspeed)
			h += quick;
	}
	if(diffy > 0)
	{
		if(v > -maxspeed)
			v -= quick;
	}
	else if(diffy < 0)
	{
		if(v < maxspeed)
			v += quick;
	}
}

inline void KOBO_enemy::move_enemy_template(int quick, int maxspeed)
{
	if(diffx > 0)
	{
		if(h > -maxspeed)
			h -= quick;
	}
	else if(diffx < 0)
	{
		if(h < maxspeed)
			h += quick;
	}
	if(diffy > 0)
	{
		if(v > -maxspeed)
			v -= quick;
	}
	else if(diffy < 0)
	{
		if(v < maxspeed)
			v += quick;
	}
	int ndi = speed2dir(h, v, 16);
	if(ndi > 0)
		di = ndi;
}

inline void KOBO_enemy::move_enemy_template_2(int quick, int maxspeed)
{
	h = -PIXEL2CS(diffy) >> quick;
	v = PIXEL2CS(diffx) >> quick;

	if(diffx > 0)
	{
		if(h > -maxspeed)
			h -= quick;
	}
	else if(diffx < 0)
	{
		if(h < maxspeed)
			h += quick;
	}
	if(diffy > 0)
	{
		if(v > -maxspeed)
			v -= quick;
	}
	else if(diffy < 0)
	{
		if(v < maxspeed)
			v += quick;
	}
	int ndi = speed2dir(h, v, 16);
	if(ndi > 0)
		di = ndi;
}

inline void KOBO_enemy::move_enemy_template_3(int quick, int maxspeed)
{
	h = PIXEL2CS(diffy) >> quick;
	v = -PIXEL2CS(diffx) >> quick;

	if(diffx > 0)
	{
		if(h > -maxspeed)
			h -= quick;
	}
	else if(diffx < 0)
	{
		if(h < maxspeed)
			h += quick;
	}
	if(diffy > 0)
	{
		if(v > -maxspeed)
			v -= quick;
	}
	else if(diffy < 0)
	{
		if(v < maxspeed)
			v += quick;
	}
	int ndi = speed2dir(h, v, 16);
	if(ndi > 0)
		di = ndi;
}

inline void KOBO_enemy::launch(const KOBO_enemy_kind *ekp)
{
#ifndef NOENEMYFIRE
	if(enemies.is_intro)
#endif
		return;
	int v0 = PIXEL2CS(ekp->launchspeed ? ekp->launchspeed :
			DEFAULT_LAUNCH_SPEED);
	int norm = (int)sqrt(diffx * diffx + diffy * diffy);
	if(!norm)
		return;
	int vx = v0 * -diffx / norm;
	int vy = v0 * -diffy / norm;
	enemies.make(ekp, x + vx, y + vy, vx, vy);
	playsound(ekp->launchsound);
}

void KOBO_enemy::shot_template_8_dir(const KOBO_enemy_kind *ekp)
{
	static int vx[] = { 0, 200, 300, 200, 0, -200, -300, -200 };
	static int vy[] = { -300, -200, 0, 200, 300, 200, 0, -200 };
	int i;
	for(i = 0; i < 8; i++)
		enemies.make(ekp, x, y, vx[i], vy[i]);
	playsound(SOUND_ENEMYM);
	playsound(ekp->launchsound);
}

void KOBO_enemy::kill_default()
{
	enemies.make(enemies.randexp(), x, y, h >> 1, v >> 1);
	release();
}

void KOBO_enemy::player_collision(int dx, int dy)
{
	if(!physics)
	{
		if(detonate_on_contact)
			detonate();		// Object detonates!
		else
			hit(game.ram_damage);	// Ship damages object
		myship.hit(damage);		// Object damages ship
		return;
	}

	// "Physics enabled" objects have bounding circles!
	int d = sqrt(dx * dx + dy * dy);
	contact = PIXEL2CS(hitsize + myship.get_hitsize()) - d;
	if(contact <= 0)
	{
		contact = 0;
		return;		// No intersection!
	}

	// Detonate-on-contact needs no physics. (Unless we knock things around
	// when dealing splash damage, but that doesn't belong here anyway.)
	if(detonate_on_contact)
	{
		detonate();		// Object detonates!
		myship.hit(damage);	// Object delivers full damage to ship
		return;
	}

	// Relative velocity
	int dvx = h - myship.get_velx();
	int dvy = v - myship.get_vely();

	// Only apply impulses if the objects are moving towards each other!
	if(dvx * dx + dvy * dy < 0)
	{
		// Calculate impulse based on contact point and velocity
		//
		// TODO:
		//	Do away with some trig functions - not because it's
		//	likely worth the effort performance wise, but because
		//	it would be nice to keep the game logic all integer and
		//	fully deterministic!
		//
		float da = atan2f(dy, dx);
		float dva = atan2f(dvy, dvx);
		float ra = da - dva;
		float cosra = cos(ra);
		float sinra = sin(ra);
		int ix = (dvx * cosra - dvy * sinra) * cosra;
		int iy = (dvx * sinra + dvy * cosra) * cosra;

		// Velocity dependent damage!
		int vel = sqrt(ix * ix + iy * iy);
		hit(game.scale_vel_damage(vel, game.ram_damage));
		myship.hit(game.scale_vel_damage(vel, damage));

		// Scale impulse for desired bounciness
		ix = (256 + (KOBO_ENEMY_BOUNCE)) * ix >> 9;
		iy = (256 + (KOBO_ENEMY_BOUNCE)) * iy >> 9;

		// Apply impulse! (All objects have identical mass...)
		h -= ix;
		v -= iy;
		myship.impulse(ix, iy);
	}

	// Since initial intersection and impulse rounding errors can allow
	// objects to creep into each other, we nudge them back to right at the
	// point of contact here.
	x += contact * dx / d;
	y += contact * dy / d;
}

void KOBO_enemy::render_hit_zone()
{
	if(!object)
		return;
	int r = PIXEL2CS(hitsize);
	if(physics)
	{
		if(contact)
			wmain->foreground(wmain->map_rgb(255, 128, 0));
		else
			wmain->foreground(wmain->map_rgb(128, 0, 128));
		wmain->circle_fxp(object->point.gx, object->point.gy, r);
	}
	else
	{
		wmain->foreground(wmain->map_rgb(128, 0, 128));
		wmain->hairrect_fxp(object->point.gx - r, object->point.gy - r,
				2 * r, 2 * r);
	}
}


/*
 * ===========================================================================
 *                                bullets
 * ===========================================================================
 */

void KOBO_enemy::make_bullet_green()
{
	di = 1 + pubrand.get(3);
	health = 1;
	shootable = false;
	physics = false;
	damage = 20;
	takes_splash_damage = false;
	detonate_on_contact = true;
}

void KOBO_enemy::make_bullet_red()
{
	make_bullet_green();
	damage = 40;
}

void KOBO_enemy::make_bullet_blue()
{
	make_bullet_green();
	damage = 10;
}

void KOBO_enemy::move_bullet()
{
	if(mindiff >= ((VIEWLIMIT >> 1) + 32))
		release();
	di += pubrand.get(1) + 1;
	if(di > 8)
		di = 1;
}

void KOBO_enemy::kill_bullet_green()
{
	enemies.make(&greenbltexpl, x, y);
	release();
}

void KOBO_enemy::kill_bullet_red()
{
	enemies.make(&redbltexpl, x, y);
	release();
}

void KOBO_enemy::kill_bullet_blue()
{
	enemies.make(&bluebltexpl, x, y);
	release();
}

const KOBO_enemy_kind greenbullet = {
	0,
	&KOBO_enemy::make_bullet_green,
	&KOBO_enemy::move_bullet,
	&KOBO_enemy::kill_bullet_green,
	2,
	B_BLT_GREEN, 0,
	LAYER_BULLETS,
	3,
	SOUND_LAUNCH_BULLET,
	SOUND_NONE,
	SOUND_NONE
};

const KOBO_enemy_kind redbullet = {
	0,
	&KOBO_enemy::make_bullet_red,
	&KOBO_enemy::move_bullet,
	&KOBO_enemy::kill_bullet_red,
	2,
	B_BLT_RED, 0,
	LAYER_BULLETS,
	2,
	SOUND_LAUNCH_BULLET,
	SOUND_NONE,
	SOUND_NONE
};

const KOBO_enemy_kind bluebullet = {
	0,
	&KOBO_enemy::make_bullet_blue,
	&KOBO_enemy::move_bullet,
	&KOBO_enemy::kill_bullet_blue,
	2,
	B_BLT_BLUE, 0,
	LAYER_BULLETS,
	5,
	SOUND_LAUNCH_BULLET,
	SOUND_NONE,
	SOUND_NONE
};


/*
 * ===========================================================================
 *                                rock
 * ===========================================================================
 */

void KOBO_enemy::make_rock()
{
	count = 500;
	health = game.rock_health;
	damage = game.rock_damage;
	di = gamerand.get(5) + 1;
	a = gamerand.get(1) ? 1 : -1;
	switch(gamerand.get() % 3)
	{
	  case 0:
		bank = B_ROCK1;
		break;
	  case 1:
		bank = B_ROCK2;
		break;
	  case 2:
		bank = B_ROCK3;
		break;
	}
}

void KOBO_enemy::move_rock()
{
#if 0
	if(bank == B_ROCK3)
	{
		di += a;
		if(di < 1)
			di += 48;
		else if(di > 48)
			di -= 48;
	}
	else
#endif
	di = ((di + a - 1) & 31) + 1;
}

void KOBO_enemy::kill_rock()
{
	enemies.make(&rockexpl, x, y, h >> 1, v >> 1);
	release();
}

const KOBO_enemy_kind rock = {
	10,
	&KOBO_enemy::make_rock,
	&KOBO_enemy::move_rock,
	&KOBO_enemy::kill_rock,
	12,
	B_ROCK1, 0,
	LAYER_ENEMIES,
	0,
	SOUND_LAUNCH,
	SOUND_HIT_ROCK,
	SOUND_EXPLO_ROCK
};


/*
 * ===========================================================================
 *                                ring
 * ===========================================================================
 */

void KOBO_enemy::make_ring()
{
	physics = false;
	detonate_on_contact = true;
	count = 500;
	health = 20;
	damage = 30;
	di = 1;
}

void KOBO_enemy::move_ring()
{
	di += 1 + pubrand.get(1);
	if(di > 16)
		di = 1;
}

void KOBO_enemy::kill_ring()
{
	enemies.make(&ringexpl, x, y, h >> 1, v >> 1);
	release();
}

const KOBO_enemy_kind ring = {
	1,
	&KOBO_enemy::make_ring,
	&KOBO_enemy::move_ring,
	&KOBO_enemy::kill_ring,
	4,
	B_RING, 0,
	LAYER_BULLETS,
	0,
	SOUND_LAUNCH_RING,
	SOUND_NONE,
	SOUND_EXPLO_RING
};


/*
 * ===========================================================================
 *                                bomb
 * ===========================================================================
 */

void KOBO_enemy::make_bomb()
{
	physics = false;
	count = 500;
	health = 20;
	damage = 70;
	di = 1;
}

void KOBO_enemy::move_bomb1()
{
	int h1 = labs(diffx);
	int v1 = labs(diffy);
	if(((h1 < 100) && (v1 < 30)) || ((h1 < 30) && (v1 < 100)))
	{
		int vx1 = PIXEL2CS(-diffx) / (3*8);
		int vy1 = PIXEL2CS(-diffy) / (3*8);
		int vx2 = vx1, vx3 = vx1;
		int vy2 = vy1, vy3 = vy1;
		int i;
		for(i = 0; i < 4; i++)
		{
			int tmp = vx2;
			vx2 += (vy2 >> 4);
			vy2 -= (tmp >> 4);
			tmp = vx3;
			vx3 -= (vy3 >> 4);
			vy3 += (tmp >> 4);
		}
		enemies.make(&redbullet, x, y, vx2, vy2);
		enemies.make(&redbullet, x, y, vx3, vy3);
		enemies.make(&bombdeto, x, y, -vx1 >> 2, -vy1 >> 2);
		playsound(SOUND_BOMB_DETO);
		release();
	}
	if(++di > 16)
		di = 1;
}

const KOBO_enemy_kind bomb1 = {
	5,
	&KOBO_enemy::make_bomb,
	&KOBO_enemy::move_bomb1,
	&KOBO_enemy::kill_default,
	5,
	B_GREENBOMB, 0,
	LAYER_ENEMIES,
	0,
	SOUND_LAUNCH_BOMB,
	SOUND_DAMAGE,
	SOUND_EXPLO_ENEMY
};


/*
 * ===========================================================================
 *                                bomb2
 * ===========================================================================
 */

void KOBO_enemy::move_bomb2()
{
	int h1 = labs(diffx);
	int v1 = labs(diffy);
	if(((h1 < 100) && (v1 < 20)) || ((h1 < 20) && (v1 < 100)))
	{
		int vx1 = PIXEL2CS(-diffx) / (3*8);
		int vy1 = PIXEL2CS(-diffy) / (3*8);
		int vx2 = vx1, vx3 = vx1;
		int vy2 = vy1, vy3 = vy1;
		int i;
		for(i = 0; i < 6; i++)
		{
			int tmp = vx2;
			vx2 += (vy2 >> 4);
			vy2 -= (tmp >> 4);
			tmp = vx3;
			vx3 -= (vy3 >> 4);
			vy3 += (tmp >> 4);
		}
		int vx4 = vx2, vx5 = vx3;
		int vy4 = vy2, vy5 = vy3;
		for(i = 0; i < 6; i++)
		{
			int tmp = vx2;
			vx2 += (vy2 >> 4);
			vy2 -= (tmp >> 4);
			tmp = vx3;
			vx3 -= (vy3 >> 4);
			vy3 += (tmp >> 4);
		}
		enemies.make(&redbullet, x, y, vx1, vy1);
		enemies.make(&redbullet, x, y, vx2, vy2);
		enemies.make(&redbullet, x, y, vx3, vy3);
		enemies.make(&redbullet, x, y, vx4, vy4);
		enemies.make(&redbullet, x, y, vx5, vy5);
		enemies.make(&bombdeto, x, y, -vx1 >> 2, -vy1 >> 2);
		playsound(SOUND_BOMB_DETO);
		release();
	}
	if(--di < 1)
		di = 16;
}

const KOBO_enemy_kind bomb2 = {
	20,
	&KOBO_enemy::make_bomb,
	&KOBO_enemy::move_bomb2,
	&KOBO_enemy::kill_default,
	5,
	B_REDBOMB, 0,
	LAYER_ENEMIES,
	0,
	SOUND_LAUNCH_BOMB,
	SOUND_DAMAGE,
	SOUND_EXPLO_ENEMY
};


/*
 * ===========================================================================
 *                                explosionX
 *                            Various explosions
 * ===========================================================================
 */

void KOBO_enemy::make_expl()
{
	health = 1;
	damage = 0;
	shootable = false;
	physics = false;
	switch(bank)
	{
	  case B_EXPLO1:
	  case B_EXPLO3:
	  case B_EXPLO4:
	  case B_EXPLO5:
		a = 16;
		break;
	  case B_BOMBDETO:
		a = 8;
		break;
	  case B_ROCKEXPL:
		a = 12;
		break;
	  case B_RINGEXPL:
		frame = 8 * pubrand.get(1);
		a = 8;
		break;
	  case B_BLTX_GREEN:
	  case B_BLTX_RED:
		a = 6;
		break;
	  case B_BOLT:
		frame = 80 + 8 * pubrand.get(2);
		di = pubrand.get(2);
		a = 8;
		break;
	}
}

void KOBO_enemy::move_expl()
{
	if(++di > a)
		release();
}

const KOBO_enemy_kind explosion = {
	0,
	&KOBO_enemy::make_expl,
	&KOBO_enemy::move_expl,
	&KOBO_enemy::kill_default,
	-1,
	B_EXPLO1, 0,
	LAYER_FX,
	0,
	SOUND_NONE,
	SOUND_NONE,
	SOUND_NONE
};

const KOBO_enemy_kind explosion3 = {
	0,
	&KOBO_enemy::make_expl,
	&KOBO_enemy::move_expl,
	&KOBO_enemy::kill_default,
	-1,
	B_EXPLO3, 0,
	LAYER_FX,
	0,
	SOUND_NONE,
	SOUND_NONE,
	SOUND_NONE
};

const KOBO_enemy_kind explosion4 = {
	0,
	&KOBO_enemy::make_expl,
	&KOBO_enemy::move_expl,
	&KOBO_enemy::kill_default,
	-1,
	B_EXPLO4, 0,
	LAYER_FX,
	0,
	SOUND_NONE,
	SOUND_NONE,
	SOUND_NONE
};

const KOBO_enemy_kind explosion5 = {
	0,
	&KOBO_enemy::make_expl,
	&KOBO_enemy::move_expl,
	&KOBO_enemy::kill_default,
	-1,
	B_EXPLO5, 0,
	LAYER_FX,
	0,
	SOUND_NONE,
	SOUND_NONE,
	SOUND_NONE
};


/*
 * ===========================================================================
 *                                 ringexpl
 *                           Ring dies in a flash
 * ===========================================================================
 */

const KOBO_enemy_kind ringexpl = {
	0,
	&KOBO_enemy::make_expl,
	&KOBO_enemy::move_expl,
	&KOBO_enemy::kill_default,
	-1,
	B_RINGEXPL, 0,
	LAYER_FX,
	0,
	SOUND_NONE,
	SOUND_NONE,
	SOUND_NONE
};


/*
 * ===========================================================================
 *                                 bltexpl
 *                         Enemy "bullet" discharges
 * ===========================================================================
 */

const KOBO_enemy_kind greenbltexpl = {
	0,
	&KOBO_enemy::make_expl,
	&KOBO_enemy::move_expl,
	&KOBO_enemy::kill_default,
	-1,
	B_BLTX_GREEN, 0,
	LAYER_FX,
	0,
	SOUND_NONE,
	SOUND_NONE,
	SOUND_NONE
};

const KOBO_enemy_kind redbltexpl = {
	0,
	&KOBO_enemy::make_expl,
	&KOBO_enemy::move_expl,
	&KOBO_enemy::kill_default,
	-1,
	B_BLTX_RED, 0,
	LAYER_FX,
	0,
	SOUND_NONE,
	SOUND_NONE,
	SOUND_NONE
};

const KOBO_enemy_kind bluebltexpl = {
	0,
	&KOBO_enemy::make_expl,
	&KOBO_enemy::move_expl,
	&KOBO_enemy::kill_default,
	-1,
	B_BLTX_BLUE, 0,
	LAYER_FX,
	0,
	SOUND_NONE,
	SOUND_NONE,
	SOUND_NONE
};


/*
 * ===========================================================================
 *                                 boltexpl
 *                         Player bolt discharges
 * ===========================================================================
 */

const KOBO_enemy_kind boltexpl = {
	0,
	&KOBO_enemy::make_expl,
	&KOBO_enemy::move_expl,
	&KOBO_enemy::kill_default,
	-1,
	B_BOLT, 0,
	LAYER_FX,
	0,
	SOUND_NONE,
	SOUND_NONE,
	SOUND_NONE
};


/*
 * ===========================================================================
 *                                 rockexpl
 *                          Rock *finally* explodes
 * ===========================================================================
 */

const KOBO_enemy_kind rockexpl = {
	0,
	&KOBO_enemy::make_expl,
	&KOBO_enemy::move_expl,
	&KOBO_enemy::kill_default,
	-1,
	B_ROCKEXPL, 0,
	LAYER_FX,
	0,
	SOUND_NONE,
	SOUND_NONE,
	SOUND_NONE
};


/*
 * ===========================================================================
 *                                 bombdeto
 *                  Bomb detonation (not really an explosion!)
 * ===========================================================================
 */

const KOBO_enemy_kind bombdeto = {
	0,
	&KOBO_enemy::make_expl,
	&KOBO_enemy::move_expl,
	&KOBO_enemy::kill_default,
	-1,
	B_BOMBDETO, 0,
	LAYER_ENEMIES,
	0,
	SOUND_NONE,
	SOUND_NONE,
	SOUND_NONE
};


/*
 * ===========================================================================
 *                                cannon
 * ===========================================================================
 */

void KOBO_enemy::make_cannon()
{
	physics = false;
	mapcollide = true;
	count = 0;
	health = game.node_health;
	damage = 0;
	splash_damage = 25;
	b = enemies.eint1() - 1;
	a = gamerand.get() & b;
}

void KOBO_enemy::move_cannon()
{
	count++;
	count &= b;
	if(count == a && mindiff < ((VIEWLIMIT >> 1) + 8))
		this->launch(enemies.ek1());
}

// For destruction via core chain reaction
void KOBO_enemy::destroy_cannon()
{
	enemies.make(enemies.randexp(), x, y);
	release();
}

// For destruction via normal damage
void KOBO_enemy::kill_cannon()
{
	enemies.make(&pipein, x, y);
	destroy_cannon();
}

const KOBO_enemy_kind cannon = {
	10,
	&KOBO_enemy::make_cannon,
	&KOBO_enemy::move_cannon,
	&KOBO_enemy::kill_cannon,
	4,
	-1, 0,
	LAYER_BASES,
	0,
	SOUND_NONE,
	SOUND_DAMAGE,
	SOUND_EXPLO_NODE
};


/*
 * ===========================================================================
 *                                core
 * ===========================================================================
 */

void KOBO_enemy::make_core()
{
	physics = false;
	mapcollide = true;
	count = 0;
	health = game.core_health;
	damage = 0;
	splash_damage = 50;
	b = enemies.eint2() - 1;
	a = gamerand.get() & b;
	if(!enemies.is_intro)
		soundhandle = sound.g_start(SOUND_CORE,
				CS2PIXEL(x), CS2PIXEL(y));
}

void KOBO_enemy::move_core()
{
	count++;
	count &= b;
	if(count == a && mindiff < ((VIEWLIMIT >> 1) + 8))
		this->launch(enemies.ek2());
}

void KOBO_enemy::kill_core()
{
	sound.g_stop(soundhandle);
	enemies.make(&pipeout, x, y, 0, 0, 3);
	enemies.make(&pipeout, x, y, 0, 0, 7);
	enemies.make(&pipeout, x, y, 0, 0, 1);
	enemies.make(&pipeout, x, y, 0, 0, 5);
	enemies.make(&explosion4, x, y);
	release();
	manage.destroyed_a_core();
}

const KOBO_enemy_kind core = {
	200,
	&KOBO_enemy::make_core,
	&KOBO_enemy::move_core,
	&KOBO_enemy::kill_core,
	4,
	-1, 0,
	LAYER_BASES,
	0,
	SOUND_NONE,
	SOUND_NONE,
	SOUND_NONE
};


/*
 * ===========================================================================
 *                                pipein
 *                    Exploding pipe from leaf nodes
 * ===========================================================================
 */

void KOBO_enemy::make_pipein()
{
	health = 1;
	damage = 0;
	shootable = false;
	physics = false;
	count = 0;
	a = 0;
}

void KOBO_enemy::move_pipein()
{
	if(--count > 0)
		return;
	count = 2 + gamerand.get(2);

	int x1 = (CS2PIXEL(x) & (WORLD_SIZEX - 1)) >> 4;
	int y1 = (CS2PIXEL(y) & (WORLD_SIZEY - 1)) >> 4;
	int a_next = 0;
	int x_next = 0;
	int y_next = 0;
	int m = screen.get_map(x1, y1);
	int p = MAP_BITS(m);
	if(IS_SPACE(p))
	{
		// Clean scrap here too, in case we were set off by a core
		// detonation chain reaction!
		screen.clean_scrap(x1, y1);
		release();
		return;
	}

	int scraptube = 48 + pubrand.get(1);
	switch(p ^ a)
	{
	  case U_MASK:
		a_next = D_MASK;
		y_next = -PIXEL2CS(16);
		break;
	  case R_MASK:
		a_next = L_MASK;
		x_next = PIXEL2CS(16);
		scraptube += 2;
		break;
	  case D_MASK:
		a_next = U_MASK;
		y_next = PIXEL2CS(16);
		scraptube += 4;
		break;
	  case L_MASK:
		a_next = R_MASK;
		x_next = -PIXEL2CS(16);
		scraptube += 6;
		break;
	  default:
		if(!(p & CORE))
			screen.set_map(x1, y1, m ^ a);
		release();
		return;
	}
	if(mindiff < ((VIEWLIMIT >> 1) + 32))
	{
		playsound(SOUND_RUMBLE);
		enemies.make(enemies.randexp(),
				x + PIXEL2CS(pubrand.get(4) - 8),
				y + PIXEL2CS(pubrand.get(4) - 8), 0, 0, 1);
	}
	screen.clean_scrap(x1, y1);
	screen.set_map(x1, y1, (scraptube << 8) | SPACE);
	x += x_next;
	y += y_next;
	a = a_next;
}

const KOBO_enemy_kind pipein = {
	0,
	&KOBO_enemy::make_pipein,
	&KOBO_enemy::move_pipein,
	&KOBO_enemy::kill_default,
	-1,
	-1, 0,
	LAYER_BASES,
	0,
	SOUND_NONE,
	SOUND_NONE,
	SOUND_NONE
};


/*
 * ===========================================================================
 *                                pipeout
 *      Exploding pipes from core, recursively following branches
 * ===========================================================================
 */

void KOBO_enemy::make_pipeout()
{
	int x1 = (CS2PIXEL(x) & (WORLD_SIZEX - 1)) >> 4;
	int y1 = (CS2PIXEL(y) & (WORLD_SIZEY - 1)) >> 4;
	screen.clean_scrap(x1, y1);
	screen.set_map(x1, y1, SPACE);
	health = 1;
	damage = 0;
	shootable = false;
	physics = false;
	count = 0;
	switch (di)
	{
	  case 1:
		a = D_MASK;
		y -= PIXEL2CS(16);
		break;
	  case 3:
		a = L_MASK;
		x += PIXEL2CS(16);
		break;
	  case 5:
		a = U_MASK;
		y += PIXEL2CS(16);
		break;
	  case 7:
		a = R_MASK;
		x -= PIXEL2CS(16);
		break;
	}
}

void KOBO_enemy::move_pipeout()
{
	if(--count > 0)
		return;

	count = 2 + gamerand.get(2);

	int x1 = (CS2PIXEL(x) & (WORLD_SIZEX - 1)) >> 4;
	int y1 = (CS2PIXEL(y) & (WORLD_SIZEY - 1)) >> 4;
	int a_next = 0;
	int x_next = 0;
	int y_next = 0;
	int p = MAP_BITS(screen.get_map(x1, y1));

	screen.clean_scrap(x1, y1);

	if(IS_SPACE(p))
	{
		release();
		return;
	}

	if((p ^ a) == 0)
	{
		manage.add_score(30);
		release();
		enemies.erase_cannon(x1, y1);
		screen.set_map(x1, y1, SPACE);
		return;
	}

	if((p ^ a) == HARD)
	{
		release();
		screen.set_map(x1, y1, SPACE);
		playsound(SOUND_RUMBLE);
		if(mindiff < ((VIEWLIMIT >> 1) + 32))
			enemies.make(enemies.randexp(), x, y, 0, 0, 1);
		return;
	}

	int scraptube = 48 + pubrand.get(1);
	switch(p ^ a)
	{
	  case U_MASK:
		a_next = D_MASK;
		y_next = -PIXEL2CS(16);
		break;
	  case R_MASK:
		a_next = L_MASK;
		x_next = PIXEL2CS(16);
		scraptube += 2;
		break;
	  case D_MASK:
		a_next = U_MASK;
		y_next = PIXEL2CS(16);
		scraptube += 4;
		break;
	  case L_MASK:
		a_next = R_MASK;
		x_next = -PIXEL2CS(16);
		scraptube += 6;
		break;
	  default:
		p ^= a;
		if(p & U_MASK)
			enemies.make(&pipeout, x, y, 0, 0, 1);
		if(p & R_MASK)
			enemies.make(&pipeout, x, y, 0, 0, 3);
		if(p & D_MASK)
			enemies.make(&pipeout, x, y, 0, 0, 5);
		if(p & L_MASK)
			enemies.make(&pipeout, x, y, 0, 0, 7);
		manage.add_score(10);
		release();
		return;
	}
	screen.set_map(x1, y1, (scraptube << 8) | SPACE);
	playsound(SOUND_RUMBLE);
	if(mindiff < ((VIEWLIMIT >> 1) + 32))
		enemies.make(&explosion,
				x + PIXEL2CS(pubrand.get(4) - 8),
				y + PIXEL2CS(pubrand.get(4) - 8),
				0, 0, 1);
	x += x_next;
	y += y_next;
	a = a_next;
}

const KOBO_enemy_kind pipeout = {
	0,
	&KOBO_enemy::make_pipeout,
	&KOBO_enemy::move_pipeout,
	&KOBO_enemy::kill_default,
	-1,
	-1, 0,
	LAYER_BASES,
	0,
	SOUND_NONE,
	SOUND_NONE,
	SOUND_NONE
};


/*
 * ===========================================================================
 *                                enemy1
 *                           Gray Dumb Missile
 * ===========================================================================
 */

void KOBO_enemy::make_enemy1()
{
	physics = false;
	detonate_on_contact = true;
	di = 1;
	health = 20;
}

void KOBO_enemy::move_enemy1()
{
	this->move_enemy_template(2, 256);
}

const KOBO_enemy_kind enemy1 = {
	2,
	&KOBO_enemy::make_enemy1,
	&KOBO_enemy::move_enemy1,
	&KOBO_enemy::kill_default,
	6,
	B_MISSILE1, 0,
	LAYER_ENEMIES,
	0,
	SOUND_LAUNCH,
	SOUND_DAMAGE,
	SOUND_EXPLO_ENEMY
};


/*
 * ===========================================================================
 *                                enemy2
 *                          Teal Firing Fighter
 * ===========================================================================
 */

void KOBO_enemy::make_enemy2()
{
	di = 1;
	health = 80;
	count = gamerand.get() & 63;
}

void KOBO_enemy::move_enemy2()
{
	this->move_enemy_template(4, 192);
	if(--(count) <= 0)
	{
		if(mindiff < ((VIEWLIMIT >> 1) + 8))
			this->launch(&redbullet);
		count = 32;
	}
}

const KOBO_enemy_kind enemy2 = {
	10,
	&KOBO_enemy::make_enemy2,
	&KOBO_enemy::move_enemy2,
	&KOBO_enemy::kill_default,
	6,
	B_FIGHTER, 0,
	LAYER_ENEMIES,
	0,
	SOUND_LAUNCH,
	SOUND_DAMAGE,
	SOUND_EXPLO_ENEMY
};


/*
 * ===========================================================================
 *                                enemy3
 *                         Maroon Homing Missile
 * ===========================================================================
 */

void KOBO_enemy::make_enemy3()
{
	physics = false;
	detonate_on_contact = true;
	di = 1;
	health = 40;
}

void KOBO_enemy::move_enemy3()
{
	this->move_enemy_template(32, 96);
}

const KOBO_enemy_kind enemy3 = {
	1,
	&KOBO_enemy::make_enemy3,
	&KOBO_enemy::move_enemy3,
	&KOBO_enemy::kill_default,
	6,
	B_MISSILE2, 0,
	LAYER_ENEMIES,
	0,
	SOUND_LAUNCH,
	SOUND_DAMAGE,
	SOUND_EXPLO_ENEMY
};


/*
 * ===========================================================================
 *                                enemy4
 *                          Blue Homing Missile
 * ===========================================================================
 */

void KOBO_enemy::make_enemy4()
{
	physics = false;
	detonate_on_contact = true;
	di = 1;
	health = 40;
}

void KOBO_enemy::move_enemy4()
{
	this->move_enemy_template(4, 96);
}

const KOBO_enemy_kind enemy4 = {
	1,
	&KOBO_enemy::make_enemy4,
	&KOBO_enemy::move_enemy4,
	&KOBO_enemy::kill_default,
	6,
	B_MISSILE3, 0,
	LAYER_ENEMIES,
	0,
	SOUND_LAUNCH,
	SOUND_DAMAGE,
	SOUND_EXPLO_ENEMY
};


/*
 * ===========================================================================
 *                                enemy5
 *                        Green Boomerang Fighter
 * ===========================================================================
 */

void KOBO_enemy::make_enemy()
{
	count = gamerand.get() & 127;
	di = 1;
	health = 60;
	a = 0;
}

void KOBO_enemy::move_enemy5()
{
	if(a == 0)
	{
		if(mindiff > ((VIEWLIMIT >> 1) - 32))
			this->move_enemy_template(6, 192);
		else
			a = 1;
	}
	else
	{
		if(mindiff < VIEWLIMIT)
			this->move_enemy_template_2(4, 192);
		else
			a = 0;
	}
	if((--count) <= 0)
	{
		count = 8;
		if(mindiff > ((VIEWLIMIT >> 1) - 32))
			this->launch(&greenbullet);
	}
}

const KOBO_enemy_kind enemy5 = {
	5,
	&KOBO_enemy::make_enemy,
	&KOBO_enemy::move_enemy5,
	&KOBO_enemy::kill_default,
	6,
	B_BMR_GREEN, 0,
	LAYER_ENEMIES,
	0,
	SOUND_LAUNCH,
	SOUND_DAMAGE,
	SOUND_EXPLO_ENEMY
};


/*
 * ===========================================================================
 *                                enemy6
 *                         Purple Boomerang Fighter
 * ===========================================================================
 */

void KOBO_enemy::move_enemy6()
{
	if(a == 0)
	{
		if(mindiff > ((VIEWLIMIT >> 1) - 0))
			this->move_enemy_template(6, 192);
		else
			a = 1;
	}
	else
	{
		if(mindiff < VIEWLIMIT)
			this->move_enemy_template_2(5, 192);
		else
			a = 0;
	}
	if((--count) <= 0)
	{
		count = 128;
		if(mindiff > ((VIEWLIMIT >> 1) - 32))
			this->launch(&redbullet);
	}
}

const KOBO_enemy_kind enemy6 = {
	2,
	&KOBO_enemy::make_enemy,
	&KOBO_enemy::move_enemy6,
	&KOBO_enemy::kill_default,
	6,
	B_BMR_PURPLE, 0,
	LAYER_ENEMIES,
	0,
	SOUND_LAUNCH,
	SOUND_DAMAGE,
	SOUND_EXPLO_ENEMY
};


/*
 * ===========================================================================
 *                                enemy7
 *                        Pink Boomerang Fighter
 * ===========================================================================
 */

void KOBO_enemy::move_enemy7()
{
	if(a == 0)
	{
		if(mindiff > ((VIEWLIMIT >> 1) - 32))
			this->move_enemy_template(6, 192);
		else
			a = 1;
	}
	else
	{
		if(mindiff < VIEWLIMIT)
			this->move_enemy_template_3(4, 192);
		else
			a = 0;
	}
	if((--count) <= 0)
	{
		count = 4;
		if(mindiff > ((VIEWLIMIT >> 1) - 32))
			this->launch(&bluebullet);
	}
}

const KOBO_enemy_kind enemy7 = {
	5,
	&KOBO_enemy::make_enemy,
	&KOBO_enemy::move_enemy7,
	&KOBO_enemy::kill_default,
	6,
	B_BMR_PINK, 0,
	LAYER_ENEMIES,
	0,
	SOUND_LAUNCH,
	SOUND_DAMAGE,
	SOUND_EXPLO_ENEMY
};


/*
 * ===========================================================================
 *                                enemy_m1
 * ===========================================================================
 */

void KOBO_enemy::make_enemy_m1()
{
	di = 1;
	health = 1000;		// Originally 26 hits
	count = gamerand.get() & 15;
}

void KOBO_enemy::move_enemy_m1()
{
	this->move_enemy_m(3, 128);
	if(++di > 16)
		di = 1;
	if((count--) <= 0)
	{
		count = 4;
		if(mindiff < ((VIEWLIMIT >> 1) - 16))
		{
			this->launch(&enemy1);
		}
	}
	if(health < 200)
	{
		this->shot_template_8_dir(&enemy2);
		release();
	}
}

const KOBO_enemy_kind enemy_m1 = {
	50,
	&KOBO_enemy::make_enemy_m1,
	&KOBO_enemy::move_enemy_m1,
	&KOBO_enemy::kill_default,
	12,
	B_BIGSHIP, 0,
	LAYER_ENEMIES,
	0,
	SOUND_NONE,
	SOUND_DAMAGE_M1,
	SOUND_EXPLO_M1
};


/*
 * ===========================================================================
 *                                enemy_m2
 * ===========================================================================
 */

void KOBO_enemy::make_enemy_m2()
{
	di = 1;
	health = 1000;
	count = gamerand.get() & 15;
}

void KOBO_enemy::move_enemy_m2()
{
	this->move_enemy_m(3, 128);
	if(++di > 16)
		di = 1;
	if((count--) <= 0)
	{
		count = 8;
		if(mindiff < ((VIEWLIMIT >> 1) + 8))
			this->launch(&enemy2);
	}
	if(health < 200)
	{
		this->shot_template_8_dir(&bomb2);
		release();
	}
}

const KOBO_enemy_kind enemy_m2 = {
	50,
	&KOBO_enemy::make_enemy_m2,
	&KOBO_enemy::move_enemy_m2,
	&KOBO_enemy::kill_default,
	12,
	B_BIGSHIP, 0,
	LAYER_ENEMIES,
	0,
	SOUND_NONE,
	SOUND_DAMAGE_M2,
	SOUND_EXPLO_M2
};


/*
 * ===========================================================================
 *                                enemy_m3
 * ===========================================================================
 */

void KOBO_enemy::make_enemy_m3()
{
	di = 1;
	health = 1000;
	count = gamerand.get() & 15;
}

void KOBO_enemy::move_enemy_m3()
{
	this->move_enemy_m(3, 128);
	if(--di < 1)
		di = 16;
	if((count--) <= 0)
	{
		count = 64;
		if(mindiff < ((VIEWLIMIT >> 1) + 8))
			this->shot_template_8_dir(&bomb2);
	}
	if(health < 200)
	{
		this->shot_template_8_dir(&rock);
		release();
	}
}

const KOBO_enemy_kind enemy_m3 = {
	50,
	&KOBO_enemy::make_enemy_m3,
	&KOBO_enemy::move_enemy_m3,
	&KOBO_enemy::kill_default,
	12,
	B_BIGSHIP, 0,
	LAYER_ENEMIES,
	0,
	SOUND_NONE,
	SOUND_DAMAGE_M3,
	SOUND_EXPLO_M3
};


/*
 * ===========================================================================
 *                                enemy_m4
 * ===========================================================================
 */

void KOBO_enemy::make_enemy_m4()
{
	di = 1;
	health = 1000;
	count = gamerand.get() & 15;
}

void KOBO_enemy::move_enemy_m4()
{
	this->move_enemy_m(2, 96);
	if(--di < 1)
		di = 16;
	static const KOBO_enemy_kind *shot[8] = {
		&enemy1, &enemy2, &bomb2, &ring, &enemy1, &enemy2, &ring,
		&enemy1
	};
	if((count--) <= 0)
	{
		count = 64;
		if(mindiff < ((VIEWLIMIT >> 1) + 8))
			this->shot_template_8_dir(shot[gamerand.get() & 7]);
	}
	if(health < 200)
	{
		this->shot_template_8_dir(&rock);
		release();
	}
}

const KOBO_enemy_kind enemy_m4 = {
	100,
	&KOBO_enemy::make_enemy_m4,
	&KOBO_enemy::move_enemy_m4,
	&KOBO_enemy::kill_default,
	12,
	B_BIGSHIP, 0,
	LAYER_ENEMIES,
	0,
	SOUND_NONE,
	SOUND_DAMAGE_M4,
	SOUND_EXPLO_M4
};
