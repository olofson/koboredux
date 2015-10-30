/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
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
#include "map.h"
#include "random.h"

void _map::init(const _scene *s)
{
	clear();
	for(int i = 0; i < s->base_max; i++)
		make_maze(s->base[i].x, s->base[i].y, s->base[i].h,
				s->base[i].v);
	convert(s->ratio);
}

void _map::clear()
{
	int i, j;
	for(i = 0; i < MAP_SIZEX; i++)
		for(j = 0; j < MAP_SIZEY; j++)
			pos(i, j) = SPACE;
}

void _map::make_maze(int x, int y, int difx, int dify)
{
	int i, j;
	int vx, vy;

	/* clear */
	for(i = x - difx; i <= x + difx; i++)
		for(j = y - dify; j <= y + dify; j++)
			pos(i, j) = SPACE;

	/* push initial sites */
	site_max = 0;
	if(gamerand.get(8) < 128)
	{
		pos(x, y) = CORE | R_MASK | L_MASK;
		maze_push(x - 1, y);
		maze_push(x + 1, y);
	}
	else
	{
		pos(x, y) = CORE | U_MASK | D_MASK;
		maze_push(x, y - 1);
		maze_push(x, y + 1);
	}

	for(;;)
	{
		/* get one */
		if(maze_pop())
			break;
		vx = sitex[site_max];
		vy = sitey[site_max];

		int dirs[4];
		for(i = 0; i < 4; i++)
			dirs[i] = 0;
		int dirs_max = 0;
		if(maze_judge(x, y, difx, dify, vx + 2, vy + 0))
			dirs[dirs_max++] = 1;
		if(maze_judge(x, y, difx, dify, vx + 0, vy + 2))
			dirs[dirs_max++] = 2;
		if(maze_judge(x, y, difx, dify, vx - 2, vy + 0))
			dirs[dirs_max++] = 3;
		if(maze_judge(x, y, difx, dify, vx + 0, vy - 2))
			dirs[dirs_max++] = 4;
		if(dirs_max == 0)
			continue;	/* there are no places to go */
		i = gamerand.get() % dirs_max;
		maze_move_and_push(vx, vy, dirs[i]);
		maze_push(vx, vy);
	}
}

int _map::maze_pop()
{
	if(site_max == 0)
		return 1;
	int i = gamerand.get() % site_max;
	site_max--;
	if(i != site_max)
	{
		int tmpx = sitex[site_max];
		int tmpy = sitey[site_max];
		sitex[site_max] = sitex[i];
		sitey[site_max] = sitey[i];
		sitex[i] = tmpx;
		sitey[i] = tmpy;
	}
	return 0;
}

void _map::maze_push(int x, int y)
{
	sitex[site_max] = x;
	sitey[site_max++] = y;
	pos(x, y) = WALL;
}

void _map::maze_move_and_push(int x, int y, int d)
{
	int x1 = x;
	int y1 = y;
	switch (d)
	{
	  case 1:
		x1 += 2;
		break;
	  case 2:
		y1 += 2;
		break;
	  case 3:
		x1 -= 2;
		break;
	  case 4:
		y1 -= 2;
		break;
	}
	maze_push(x1, y1);
	pos((x + x1) / 2, (y + y1) / 2) = WALL;
}

int _map::maze_judge(int cx, int cy, int dx, int dy, int x, int y)
{
	if((x < cx - dx) || (x > cx + dx) || (y < cy - dy)
			|| (y > cy + dy))
		return 0;
	if(pos(x, y) == WALL)
		return 0;
	return 1;
}

static inline int bits2tile(int n)
{
	if(n & CORE)		// Core
		return n & (U_MASK | D_MASK) ? 16 : 24;
	else if(n & HARD)	// One of the 4 indestructible end nodes
		switch(n & 0xf)
		{
		  case U_MASK:	return 32;
		  case R_MASK:	return 33;
		  case D_MASK:	return 34;
		  case L_MASK:	return 35;
		}
	else if(n == 5)		// Vertical pipe
		switch(pubrand.get(3))
		{
		  case 0:	return 40;
		  case 1:	return 42;
		  case 2:	return 44;
		  default:	return 5;
		}
	else if(n == 10)	// Horizontal pipe
		switch(pubrand.get(3))
		{
		  case 0:	return 41;
		  case 1:	return 43;
		  case 2:	return 45;
		  default:	return 10;
		}
	else if(n == 15)
	{
		if(pubrand.get(1))
			return 0;
		else
			return 15;
	}
	return n;	// Other pipe parts or normal end nodes
}

void _map::convert(unsigned ratio)
{
	int i, j;
	int p = 0;
	for(i = 0; i < MAP_SIZEX; i++)
		for(j = 0; j < MAP_SIZEY; j++)
		{
			p = pos(i, j) & CORE;
			if(IS_SPACE(pos(i, j)))
			{
				pos(i, j) = SPACE;
				continue;
			}
			if((j > 0) && !IS_SPACE(pos(i, j - 1)))
				p |= U_MASK;
			if((i < MAP_SIZEX - 1) && !IS_SPACE(pos(i + 1, j)))
				p |= R_MASK;
			if((j < MAP_SIZEY - 1) && !IS_SPACE(pos(i, j + 1)))
				p |= D_MASK;
			if((i > 0) && !IS_SPACE(pos(i - 1, j)))
				p |= L_MASK;
			if((p == U_MASK) || (p == R_MASK) || (p == D_MASK)
					|| (p == L_MASK))
			{
				if(gamerand.get(8) < ratio)
					p |= HARD;
			}
			pos(i, j) = (bits2tile(p) << 8) | p;
		}
}

int _map::test_line(int x1, int y1, int x3, int y3,
		int *x2, int *y2, int *hx, int *hy)
{
	// Brute force implementation
	float dx = x3 - x1;
	float dy = y3 - y1;

	// World unit steps
	int steps = ABS(dx) > ABS(dy) ? ABS(dx) : ABS(dy);
	if(steps)
	{
		dx /= steps;
		dy /= steps;
	}

	// Find first collision
	float x = x1;
	float y = y1;
	float px, py;
	int mx, my, pmx, pmy;
	for(int z = 0; ; ++z)
	{
		if(z >= steps)
		{
			if(z == steps)
			{
				// We must never ever leave with an undetected
				// collision at (x3, y3)! So, we test that pos
				// last, to circumvent rounding errors.
				x = x3;
				y = y3;
			}
			else
				return 0;	// No collisions!
		}

		mx = WORLD2MAPX(CS2PIXEL((int)x));
		my = WORLD2MAPY(CS2PIXEL((int)y));
		if(IS_BASE(pos(mx, my)))
		{
			if(!z)
			{
				// This is an error condition that only
				// occurs when starting inside a tile!
				return COLL_STUCK;
			}
			break;
		}
		px = x;
		py = y;
		pmy = my;
		pmx = mx;
		x += dx;
		y += dy;
	}

	// Step back and evaluate the situation right before the collision
	int res = 0;
	*x2 = (int)px;
	*y2 = (int)py;
	*hx = (int)x;
	*hy = (int)y;
	if(mx != pmx)
		for(int z = MIN(my, pmy); z <= MAX(my, pmy); ++z)
			if(IS_BASE(pos(mx, z)))
				res |= COLL_VERTICAL;
	if(my != pmy)
		for(int z = MIN(mx, pmx); z <= MAX(mx, pmx); ++z)
			if(IS_BASE(pos(z, my)))
				res |= COLL_HORIZONTAL;
	return res;
}
