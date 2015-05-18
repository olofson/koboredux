/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2007, 2009 David Olofson
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

inline void _enemy::move_enemy_m(int quick, int maxspeed)
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

inline void _enemy::move_enemy_template(int quick, int maxspeed)
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

inline void _enemy::move_enemy_template_2(int quick, int maxspeed)
{
	h = -PIXEL2CS(diffy) / (1<<quick);
	v = PIXEL2CS(diffx) / (1<<quick);

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

inline void _enemy::move_enemy_template_3(int quick, int maxspeed)
{
	h = PIXEL2CS(diffy) / (1<<quick);
	v = -PIXEL2CS(diffx) / (1<<quick);

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

inline void _enemy::shot_template(const enemy_kind * ekp,
		int shift, int rnd, int maxspeed)
{
	int vx = -diffx;
	int vy = -diffy;
	if(enemies.is_intro)
		return;
	if(rnd)
	{
		vx += (gamerand.get() & (rnd - 1)) - (rnd >> 1);
		vy += (gamerand.get() & (rnd - 1)) - (rnd >> 1);
	}
	vx = PIXEL2CS(vx) / (1<<shift);
	vy = PIXEL2CS(vy) / (1<<shift);
	if(maxspeed > 0)
	{
		if(vx > maxspeed)
			vx = maxspeed;
		else if(vx < -maxspeed)
			vx = -maxspeed;
		if(vy > maxspeed)
			vy = maxspeed;
		else if(vy < -maxspeed)
			vy = -maxspeed;
	}
	enemies.make(ekp, CS2PIXEL(x + vx), CS2PIXEL(y + vy), vx,
			vy);
	if(&ring == ekp)
		sound.g_launch_ring(x, y);
	else if(&beam == ekp)
		sound.g_launch_beam(x, y);
	else if(&bomb1 == ekp || &bomb2 == ekp)
		sound.g_launch_bomb(x, y);
	else
		sound.g_launch(x, y);
}

void _enemy::shot_template_8_dir(const enemy_kind * ekp)
{
	static int vx[] = { 0, 200, 300, 200, 0, -200, -300, -200 };
	static int vy[] = { -300, -200, 0, 200, 300, 200, 0, -200 };
	int i;
	for(i = 0; i < 8; i++)
		enemies.make(ekp, CS2PIXEL(x), CS2PIXEL(y), vx[i],
				vy[i]);
	if(&ring == ekp)
		sound.g_m_launch_ring(x, y);
	else if(&beam == ekp)
		sound.g_m_launch_beam(x, y);
	else if(&bomb1 == ekp || &bomb2 == ekp)
		sound.g_m_launch_bomb(x, y);
	else
		sound.g_m_launch(x, y);
}

void _enemy::kill_default()
{
	enemies.make(enemies.randexp(), CS2PIXEL(x), CS2PIXEL(y),
			h >> 1, v >> 1);
	sound.g_enemy_explo(x, y);
	release();
}


/*
 * ===========================================================================
 *                                beam
 * ===========================================================================
 */

void _enemy::make_beam()
{
	di = 1 + pubrand.get(4);
	health = 1;
	shootable = 0;
	damage = 20;
}

void _enemy::move_beam()
{
	if(norm >= ((VIEWLIMIT >> 1) + 32))
		release();
	di += pubrand.get(1) + 1;
	if(di > 16)
		di = 1;
}

void _enemy::kill_beam()
{
	enemies.make(&beamexpl, CS2PIXEL(x), CS2PIXEL(y));
	release();
}

const enemy_kind beam = {
	0,
	&_enemy::make_beam,
	&_enemy::move_beam,
	&_enemy::kill_beam,
	2,
	B_BULLETS, 0,
	LAYER_BULLETS
};


/*
 * ===========================================================================
 *                                rock
 * ===========================================================================
 */

void _enemy::make_rock()
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

void _enemy::move_rock()
{
	if(bank == B_ROCK3)
	{
		di += a;
		if(di < 1)
			di += 48;
		else if(di > 48)
			di -= 48;
	}
	else
		di = ((di + a - 1) & 31) + 1;
}

void _enemy::kill_rock()
{
	enemies.make(&rockexpl, CS2PIXEL(x), CS2PIXEL(y), h >> 1, v >> 1);
	sound.g_rock_explo(x, y);
	release();
}

const enemy_kind rock = {
	10,
	&_enemy::make_rock,
	&_enemy::move_rock,
	&_enemy::kill_rock,
	4,
	B_ROCK1, 0,
	LAYER_ENEMIES
};


/*
 * ===========================================================================
 *                                ring
 * ===========================================================================
 */

void _enemy::make_ring()
{
	count = 500;
	health = 20;
	damage = 30;
	di = 1;
}

void _enemy::move_ring()
{
	di += 1 + pubrand.get(1);
	if(di > 16)
		di = 1;
}

void _enemy::kill_ring()
{
	enemies.make(&ringexpl, CS2PIXEL(x), CS2PIXEL(y), h >> 1, v >> 1);
	sound.g_ring_explo(x, y);
	release();
}

const enemy_kind ring = {
	1,
	&_enemy::make_ring,
	&_enemy::move_ring,
	&_enemy::kill_ring,
	4,
	B_RING, 0,
	LAYER_BULLETS
};


/*
 * ===========================================================================
 *                                bomb
 * ===========================================================================
 */

void _enemy::make_bomb()
{
	count = 500;
	health = 20;
	damage = 70;
	di = 1;
}

void _enemy::move_bomb1()
{
	int h1 = ABS(diffx);
	int v1 = ABS(diffy);
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
		enemies.make(&beam, CS2PIXEL(x), CS2PIXEL(y), vx2, vy2);
		enemies.make(&beam, CS2PIXEL(x), CS2PIXEL(y), vx3, vy3);
		enemies.make(&bombdeto, CS2PIXEL(x), CS2PIXEL(y),
				-vx1 >> 2, -vy1 >> 2);
		sound.g_bomb_deto(x, y);
		release();
	}
	if(++di > 16)
		di = 1;
}

const enemy_kind bomb1 = {
	5,
	&_enemy::make_bomb,
	&_enemy::move_bomb1,
	&_enemy::kill_default,
	5,
	B_BOMB, 0,
	LAYER_ENEMIES
};


/*
 * ===========================================================================
 *                                bomb2
 * ===========================================================================
 */

void _enemy::move_bomb2()
{
	int h1 = ABS(diffx);
	int v1 = ABS(diffy);
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
		enemies.make(&beam, CS2PIXEL(x), CS2PIXEL(y), vx1, vy1);
		enemies.make(&beam, CS2PIXEL(x), CS2PIXEL(y), vx2, vy2);
		enemies.make(&beam, CS2PIXEL(x), CS2PIXEL(y), vx3, vy3);
		enemies.make(&beam, CS2PIXEL(x), CS2PIXEL(y), vx4, vy4);
		enemies.make(&beam, CS2PIXEL(x), CS2PIXEL(y), vx5, vy5);
		enemies.make(&bombdeto, CS2PIXEL(x), CS2PIXEL(y),
				-vx1 >> 2, -vy1 >> 2);
		sound.g_bomb_deto(x, y);
		release();
	}
	if(--di < 1)
		di = 16;
}

const enemy_kind bomb2 = {
	20,
	&_enemy::make_bomb,
	&_enemy::move_bomb2,
	&_enemy::kill_default,
	5,
	B_BOMB, 0,
	LAYER_ENEMIES
};


/*
 * ===========================================================================
 *                                explosionX
 *                            Various explosions
 * ===========================================================================
 */

void _enemy::make_expl()
{
	health = 1;
	damage = 0;
	shootable = 0;
	switch(bank)
	{
	  case B_EXPLO1:
	  case B_BOMBDETO:
		a = 8;
		break;
	  case B_EXPLO3:
	  case B_EXPLO4:
	  case B_EXPLO5:
	  case B_ROCKEXPL:
		a = 12;
		break;
	  case B_RINGEXPL:
		frame = 8 * pubrand.get(1);
		a = 8;
		break;
	  case B_BULLETEXPL:
		a = 6;
		break;
	  case B_BOLT:
		frame = 32 + 8 * pubrand.get(2);
		di = pubrand.get(2);
		a = 8;
		break;
	}
}

void _enemy::move_expl()
{
	if(++di > a)
		release();
}

const enemy_kind explosion = {
	0,
	&_enemy::make_expl,
	&_enemy::move_expl,
	&_enemy::kill_default,
	-1,
	B_EXPLO1, 0,
	LAYER_FX
};

const enemy_kind explosion3 = {
	0,
	&_enemy::make_expl,
	&_enemy::move_expl,
	&_enemy::kill_default,
	-1,
	B_EXPLO3, 0,
	LAYER_FX
};

const enemy_kind explosion4 = {
	0,
	&_enemy::make_expl,
	&_enemy::move_expl,
	&_enemy::kill_default,
	-1,
	B_EXPLO4, 0,
	LAYER_FX
};

const enemy_kind explosion5 = {
	0,
	&_enemy::make_expl,
	&_enemy::move_expl,
	&_enemy::kill_default,
	-1,
	B_EXPLO5, 0,
	LAYER_FX
};


/*
 * ===========================================================================
 *                                 ringexpl
 *                           Ring dies in a flash
 * ===========================================================================
 */

const enemy_kind ringexpl = {
	0,
	&_enemy::make_expl,
	&_enemy::move_expl,
	&_enemy::kill_default,
	-1,
	B_RINGEXPL, 0,
	LAYER_FX
};


/*
 * ===========================================================================
 *                                 beamexpl
 *                         Enemy "bullet" discharges
 * ===========================================================================
 */

const enemy_kind beamexpl = {
	0,
	&_enemy::make_expl,
	&_enemy::move_expl,
	&_enemy::kill_default,
	-1,
	B_BULLETEXPL, 0,
	LAYER_FX
};


/*
 * ===========================================================================
 *                                 boltexpl
 *                         Player bolt discharges
 * ===========================================================================
 */

const enemy_kind boltexpl = {
	0,
	&_enemy::make_expl,
	&_enemy::move_expl,
	&_enemy::kill_default,
	-1,
	B_BOLT, 0,
	LAYER_FX
};


/*
 * ===========================================================================
 *                                 rockexpl
 *                          Rock *finally* explodes
 * ===========================================================================
 */

const enemy_kind rockexpl = {
	0,
	&_enemy::make_expl,
	&_enemy::move_expl,
	&_enemy::kill_default,
	-1,
	B_ROCKEXPL, 0,
	LAYER_FX
};


/*
 * ===========================================================================
 *                                 bombdeto
 *                  Bomb detonation (not really an explosion!)
 * ===========================================================================
 */

const enemy_kind bombdeto = {
	0,
	&_enemy::make_expl,
	&_enemy::move_expl,
	&_enemy::kill_default,
	-1,
	B_BOMBDETO, 0,
	LAYER_ENEMIES
};


/*
 * ===========================================================================
 *                                cannon
 * ===========================================================================
 */

void _enemy::make_cannon()
{
	count = 0;
	health = 20;
	damage = 75;
	b = enemies.eint1() - 1;
	a = gamerand.get() & b;
}

void _enemy::move_cannon()
{
	(count)++;
	(count) &= (b);
	if(count == a && norm < ((VIEWLIMIT >> 1) + 8))
	{
		int shift = (enemies.ek1() == &beam) ? 6 : 5;
		this->shot_template(enemies.ek1(), shift, 32, 0);
	}
}

void _enemy::kill_cannon()
{
	sound.g_base_node_explo(x, y);
	enemies.make(&pipein, CS2PIXEL(x), CS2PIXEL(y));
	enemies.make(enemies.randexp(), CS2PIXEL(x), CS2PIXEL(y));
	release();
}

const enemy_kind cannon = {
	10,
	&_enemy::make_cannon,
	&_enemy::move_cannon,
	&_enemy::kill_cannon,
	4,
	-1, 0,
	LAYER_BASES
};


/*
 * ===========================================================================
 *                                core
 * ===========================================================================
 */

void _enemy::make_core()
{
	count = 0;
	health = 20;
	damage = 150;
	b = enemies.eint2() - 1;
	a = gamerand.get() & b;
}

void _enemy::move_core()
{
	(count)++;
	(count) &= (b);
	if(count == a && norm < ((VIEWLIMIT >> 1) + 8))
	{
		int shift = (enemies.ek2() == &beam) ? 6 : 5;
		this->shot_template(enemies.ek2(), shift, 0, 0);
	}
}

void _enemy::kill_core()
{
	enemies.make(&pipeout, CS2PIXEL(x), CS2PIXEL(y), 0, 0, 3);
	enemies.make(&pipeout, CS2PIXEL(x), CS2PIXEL(y), 0, 0, 7);
	enemies.make(&pipeout, CS2PIXEL(x), CS2PIXEL(y), 0, 0, 1);
	enemies.make(&pipeout, CS2PIXEL(x), CS2PIXEL(y), 0, 0, 5);
	enemies.make(&explosion4, CS2PIXEL(x), CS2PIXEL(y));
	sound.g_base_core_explo(x, y);
	release();
	manage.destroyed_a_core();
}

const enemy_kind core = {
	200,
	&_enemy::make_core,
	&_enemy::move_core,
	&_enemy::kill_core,
	4,
	-1, 0,
	LAYER_BASES
};


/*
 * ===========================================================================
 *                                pipein
 *                    Exploding pipe from leaf nodes
 * ===========================================================================
 */

void _enemy::make_pipein()
{
	health = 1;
	damage = 0;
	shootable = 0;
	count = 0;
	a = 0;
}

void _enemy::move_pipein()
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
	if(norm < ((VIEWLIMIT >> 1) + 32))
	{
		sound.g_pipe_rumble(x, y);
		enemies.make(enemies.randexp(),
				CS2PIXEL(x) + pubrand.get(4) - 8,
				CS2PIXEL(y) + pubrand.get(4) - 8, 0, 0, 1);
	}
	screen.clean_scrap(x1, y1);
	screen.set_map(x1, y1, (scraptube << 8) | SPACE);
	x += x_next;
	y += y_next;
	a = a_next;
}

const enemy_kind pipein = {
	0,
	&_enemy::make_pipein,
	&_enemy::move_pipein,
	&_enemy::kill_default,
	-1,
	-1, 0,
	LAYER_BASES
};


/*
 * ===========================================================================
 *                                pipeout
 *      Exploding pipes from core, recursively following branches
 * ===========================================================================
 */

void _enemy::make_pipeout()
{
	int x1 = (CS2PIXEL(x) & (WORLD_SIZEX - 1)) >> 4;
	int y1 = (CS2PIXEL(y) & (WORLD_SIZEY - 1)) >> 4;
	screen.clean_scrap(x1, y1);
	screen.set_map(x1, y1, SPACE);
	health = 1;
	damage = 0;
	shootable = 0;
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

void _enemy::move_pipeout()
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
		sound.g_base_node_explo(x, y);
		if(norm < ((VIEWLIMIT >> 1) + 32))
			enemies.make(enemies.randexp(),
					CS2PIXEL(x), CS2PIXEL(y));
		return;
	}

	if((p ^ a) == HARD)
	{
		release();
		screen.set_map(x1, y1, SPACE);
		sound.g_pipe_rumble(x, y);
		if(norm < ((VIEWLIMIT >> 1) + 32))
			enemies.make(enemies.randexp(),
					CS2PIXEL(x), CS2PIXEL(y), 0, 0, 1);
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
			enemies.make(&pipeout, CS2PIXEL(x), CS2PIXEL(y),
					0, 0, 1);
		if(p & R_MASK)
			enemies.make(&pipeout, CS2PIXEL(x), CS2PIXEL(y),
					0, 0, 3);
		if(p & D_MASK)
			enemies.make(&pipeout, CS2PIXEL(x), CS2PIXEL(y),
					0, 0, 5);
		if(p & L_MASK)
			enemies.make(&pipeout, CS2PIXEL(x), CS2PIXEL(y),
					0, 0, 7);
		manage.add_score(10);
		release();
		return;
	}
	screen.set_map(x1, y1, (scraptube << 8) | SPACE);
	sound.g_pipe_rumble(x, y);
	if(norm < ((VIEWLIMIT >> 1) + 32))
		enemies.make(&explosion,
				CS2PIXEL(x) + pubrand.get(4) - 8,
				CS2PIXEL(y) + pubrand.get(4) - 8,
				0, 0, 1);
	x += x_next;
	y += y_next;
	a = a_next;
}

const enemy_kind pipeout = {
	0,
	&_enemy::make_pipeout,
	&_enemy::move_pipeout,
	&_enemy::kill_default,
	-1,
	-1, 0,
	LAYER_BASES
};


/*
 * ===========================================================================
 *                                enemy1
 *                           Gray Dumb Missile
 * ===========================================================================
 */

void _enemy::make_enemy1()
{
	di = 1;
	health = 20;
}

void _enemy::move_enemy1()
{
	this->move_enemy_template(2, 256);
}

const enemy_kind enemy1 = {
	2,
	&_enemy::make_enemy1,
	&_enemy::move_enemy1,
	&_enemy::kill_default,
	6,
	B_MISSILE1, 0,
	LAYER_ENEMIES
};


/*
 * ===========================================================================
 *                                enemy2
 *                          Teal Firing Fighter
 * ===========================================================================
 */

void _enemy::make_enemy2()
{
	di = 1;
	health = 20;
	count = gamerand.get() & 63;
}

void _enemy::move_enemy2()
{
	this->move_enemy_template(4, 192);
	if(--(count) <= 0)
	{
		if(norm < ((VIEWLIMIT >> 1) + 8))
		{
			this->shot_template(&beam, 5, 0, 0);
		}
		count = 32;
	}
}

const enemy_kind enemy2 = {
	10,
	&_enemy::make_enemy2,
	&_enemy::move_enemy2,
	&_enemy::kill_default,
	6,
	B_FIGHTER, 0,
	LAYER_ENEMIES
};


/*
 * ===========================================================================
 *                                enemy3
 *                         Maroon Homing Missile
 * ===========================================================================
 */

void _enemy::make_enemy3()
{
	di = 1;
	health = 20;
}

void _enemy::move_enemy3()
{
	this->move_enemy_template(32, 96);
}

const enemy_kind enemy3 = {
	1,
	&_enemy::make_enemy3,
	&_enemy::move_enemy3,
	&_enemy::kill_default,
	6,
	B_MISSILE2, 0,
	LAYER_ENEMIES
};


/*
 * ===========================================================================
 *                                enemy4
 *                          Blue Homing Missile
 * ===========================================================================
 */

void _enemy::make_enemy4()
{
	di = 1;
	health = 20;
}

void _enemy::move_enemy4()
{
	this->move_enemy_template(4, 96);
}

const enemy_kind enemy4 = {
	1,
	&_enemy::make_enemy4,
	&_enemy::move_enemy4,
	&_enemy::kill_default,
	6,
	B_MISSILE3, 0,
	LAYER_ENEMIES
};


/*
 * ===========================================================================
 *                                enemy5
 *                        Green Boomerang Fighter
 * ===========================================================================
 */

void _enemy::make_enemy()
{
	count = gamerand.get() & 127;
	di = 1;
	health = 20;
	a = 0;
}

void _enemy::move_enemy5()
{
	if(a == 0)
	{
		if(norm > ((VIEWLIMIT >> 1) - 32))
			this->move_enemy_template(6, 192);
		else
			a = 1;
	}
	else
	{
		if(norm < VIEWLIMIT)
			this->move_enemy_template_2(4, 192);
		else
			a = 0;
	}
	if((--count) <= 0)
	{
		count = 8;
		if(norm > ((VIEWLIMIT >> 1) - 32))
			this->shot_template(&beam, 6, 0, 0);
	}
}

const enemy_kind enemy5 = {
	5,
	&_enemy::make_enemy,
	&_enemy::move_enemy5,
	&_enemy::kill_default,
	6,
	B_BMR_GREEN, 0,
	LAYER_ENEMIES
};


/*
 * ===========================================================================
 *                                enemy6
 *                         Purple Boomerang Fighter
 * ===========================================================================
 */

void _enemy::move_enemy6()
{
	if(a == 0)
	{
		if(norm > ((VIEWLIMIT >> 1) - 0))
			this->move_enemy_template(6, 192);
		else
			a = 1;
	}
	else
	{
		if(norm < VIEWLIMIT)
			this->move_enemy_template_2(5, 192);
		else
			a = 0;
	}
	if((--count) <= 0)
	{
		count = 128;
		if(norm > ((VIEWLIMIT >> 1) - 32))
			this->shot_template(&beam, 6, 0, 0);
	}
}

const enemy_kind enemy6 = {
	2,
	&_enemy::make_enemy,
	&_enemy::move_enemy6,
	&_enemy::kill_default,
	6,
	B_BMR_PURPLE, 0,
	LAYER_ENEMIES
};


/*
 * ===========================================================================
 *                                enemy7
 *                        Pink Boomerang Fighter
 * ===========================================================================
 */

void _enemy::move_enemy7()
{
	if(a == 0)
	{
		if(norm > ((VIEWLIMIT >> 1) - 32))
			this->move_enemy_template(6, 192);
		else
			a = 1;
	}
	else
	{
		if(norm < VIEWLIMIT)
			this->move_enemy_template_3(4, 192);
		else
			a = 0;
	}
	if((--count) <= 0)
	{
		count = 8;
		if(norm > ((VIEWLIMIT >> 1) - 32))
			this->shot_template(&beam, 6, 0, 0);
	}
}

const enemy_kind enemy7 = {
	5,
	&_enemy::make_enemy,
	&_enemy::move_enemy7,
	&_enemy::kill_default,
	6,
	B_BMR_PINK, 0,
	LAYER_ENEMIES
};


/*
 * ===========================================================================
 *                                enemy_m1
 * ===========================================================================
 */

void _enemy::make_enemy_m1()
{
	di = 1;
	health = 20 * 26;
	count = gamerand.get() & 15;
}

void _enemy::move_enemy_m1()
{
	this->move_enemy_m(3, 128);
	if(++di > 16)
		di = 1;
	if((count--) <= 0)
	{
		count = 4;
		if(norm < ((VIEWLIMIT >> 1) - 16))
		{
			this->shot_template(&enemy1, 4, 0, 0);
		}
	}
	if(health < 200)
	{
		this->shot_template_8_dir(&enemy2);
		release();
	}
}

const enemy_kind enemy_m1 = {
	50,
	&_enemy::make_enemy_m1,
	&_enemy::move_enemy_m1,
	&_enemy::kill_default,
	12,
	B_BIGSHIP, 0,
	LAYER_ENEMIES
};


/*
 * ===========================================================================
 *                                enemy_m2
 * ===========================================================================
 */

void _enemy::make_enemy_m2()
{
	di = 1;
	health = 20 * 26;
	count = gamerand.get() & 15;
}

void _enemy::move_enemy_m2()
{
	this->move_enemy_m(3, 128);
	if(++di > 16)
		di = 1;
	if((count--) <= 0)
	{
		count = 8;
		if(norm < ((VIEWLIMIT >> 1) + 8))
		{
			this->shot_template(&enemy2, 4, 128, 192);
		}
	}
	if(health < 200)
	{
		this->shot_template_8_dir(&bomb2);
		release();
	}
}

const enemy_kind enemy_m2 = {
	50,
	&_enemy::make_enemy_m2,
	&_enemy::move_enemy_m2,
	&_enemy::kill_default,
	12,
	B_BIGSHIP, 0,
	LAYER_ENEMIES
};


/*
 * ===========================================================================
 *                                enemy_m3
 * ===========================================================================
 */

void _enemy::make_enemy_m3()
{
	di = 1;
	health = 20 * 26;
	count = gamerand.get() & 15;
}

void _enemy::move_enemy_m3()
{
	this->move_enemy_m(3, 128);
	if(--di < 1)
		di = 16;
	if((count--) <= 0)
	{
		count = 64;
		if(norm < ((VIEWLIMIT >> 1) + 8))
		{
			this->shot_template_8_dir(&bomb2);
		}
	}
	if(health < 200)
	{
		this->shot_template_8_dir(&rock);
		release();
	}
}

const enemy_kind enemy_m3 = {
	50,
	&_enemy::make_enemy_m3,
	&_enemy::move_enemy_m3,
	&_enemy::kill_default,
	12,
	B_BIGSHIP, 0,
	LAYER_ENEMIES
};


/*
 * ===========================================================================
 *                                enemy_m4
 * ===========================================================================
 */

void _enemy::make_enemy_m4()
{
	di = 1;
	health = 20 * 26;
	count = gamerand.get() & 15;
}

void _enemy::move_enemy_m4()
{
	this->move_enemy_m(2, 96);
	if(--di < 1)
		di = 16;
	static const enemy_kind *shot[8] = {
		&enemy1, &enemy2, &bomb2, &ring, &enemy1, &enemy2, &ring,
		&enemy1
	};
	if((count--) <= 0)
	{
		count = 64;
		if(norm < ((VIEWLIMIT >> 1) + 8))
		{
			this->shot_template_8_dir(shot[gamerand.get() & 7]);
		}
	}
	if(health < 200)
	{
		this->shot_template_8_dir(&rock);
		release();
	}
}

const enemy_kind enemy_m4 = {
	100,
	&_enemy::make_enemy_m4,
	&_enemy::move_enemy_m4,
	&_enemy::kill_default,
	12,
	B_BIGSHIP, 0,
	LAYER_ENEMIES
};


/*---------------------------------------------------------------------------*/
/*  void _enemy::make_xxxx(){}
 *  void _enemy::move_xxxx(){}
 *  void _enemy::die_xxxx(){}
 *  enemy_kind xxxxx = {
 *	score,
 *	make_xxxx, move_xxxx, die_xxx,
 *	hitsize,
 *	bank, frame, tilesize
 *  };
 */
