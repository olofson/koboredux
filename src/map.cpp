/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2002, 2007, 2009 David Olofson
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

#include "config.h"
#include "map.h"
#include "random.h"

void KOBO_map::init(const KOBO_scene *s)
{
	clear();
	for(int i = 0; i < s->base_max; i++)
		make_maze(s->base[i].x, s->base[i].y, s->base[i].h,
				s->base[i].v);
	convert(s->ratio);
}

void KOBO_map::clear()
{
	int i, j;
	for(i = 0; i < MAP_SIZEX; i++)
		for(j = 0; j < MAP_SIZEY; j++)
			pos(i, j) = SPACE;
}

void KOBO_map::make_maze(int x, int y, int difx, int dify)
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

int KOBO_map::maze_pop()
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

void KOBO_map::maze_push(int x, int y)
{
	sitex[site_max] = x;
	sitey[site_max++] = y;
	pos(x, y) = WALL;
}

void KOBO_map::maze_move_and_push(int x, int y, int d)
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

int KOBO_map::maze_judge(int cx, int cy, int dx, int dy, int x, int y)
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

void KOBO_map::convert(unsigned ratio)
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

int KOBO_map::test_line(int x1, int y1, int x3, int y3,
		int *x2, int *y2, int *hx, int *hy)
{
	// End position (map)
	int mx3 = CS2PIXEL(x3) >> TILE_SIZEX_LOG2;
	int my3 = CS2PIXEL(y3) >> TILE_SIZEY_LOG2;

	// Are we actually crossing any tile boundaries?
	if((WORLD2MAPX(CS2PIXEL(x1)) == mx3) &&
			(WORLD2MAPY(CS2PIXEL(y1)) == my3))
	{
		if(IS_BASE(pos(mx3, my3)))
			return COLL_STUCK;
		else
			return 0;
	}

	// Current position
	int x = x1;
	int y = y1;

	// Position right before current
	int px = x;
	int py = y;

	int next_test = COLL_STUCK;

	while(1)
	{
		// Check the current tile
		int mx = CS2PIXEL(x) >> TILE_SIZEX_LOG2;
		int my = CS2PIXEL(y) >> TILE_SIZEY_LOG2;
		if(IS_BASE(pos(mx, my)))
		{
			// Collision!
			*x2 = px;
			*y2 = py;
			*hx = x;
			*hy = y;
			return next_test;
		}

		// If we're clear, and in the end position tile, we're done!
		if((mx == mx3) && (my == my3))
			return 0;

		// Figure out which side of the current tile we're going to
		// exit through, so we can check the correct adjacent tile.

		// Next vertical collision
		int vcx = 0, vcy = 0, vd2;
		if(mx3 != mx)
		{
			if(x3 > x)
				vcx = PIXEL2CS((mx + 1) << TILE_SIZEX_LOG2);
			else
				vcx = PIXEL2CS(mx << TILE_SIZEX_LOG2) - 1;
			vcy = y1 + (y3 - y1) * (vcx - x1) / (x3 - x1);
			int dx = vcx - x;
			int dy = vcy - y;
			vd2 = dx*dx + dy*dy;
		}
		else
			vd2 = 0x7fffffff;

		// Next horizontal collision
		int hcx, hcy, hd2;
		if(my3 != my)
		{
			if(y3 > y)
				hcy = PIXEL2CS((my + 1) << TILE_SIZEY_LOG2);
			else
				hcy = PIXEL2CS(my << TILE_SIZEY_LOG2) - 1;
			hcx = x1 + (x3 - x1) * (hcy - y1) / (y3 - y1);
			int dx = hcx - x;
			int dy = hcy - y;
			hd2 = dx*dx + dy*dy;
		}
		else
			hd2 = 0x7fffffff;

		// Which one is closer?
		if(vd2 > hd2)
		{
			x = hcx;
			y = hcy;
			next_test = COLL_HORIZONTAL;
			px = x;
			py = y - (y3 > y1 ? 1 : -1);
		}
		else if(vd2 < hd2)
		{
			x = vcx;
			y = vcy;
			next_test = COLL_VERTICAL;
			px = x - (x3 > x1 ? 1 : -1);
			py = y;
		}
		else
		{
			// Both: Diagonal corner hit!
			x = vcx;
			y = vcy;
			next_test = COLL_VERTICAL | COLL_HORIZONTAL;
			px = x - (x3 > x1 ? 1 : -1);
			py = y - (y3 > y1 ? 1 : -1);
		}
	}
}
