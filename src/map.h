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

#ifndef XKOBO_H_MAP
#define XKOBO_H_MAP

#include "config.h"
#include "scenes.h"

#define SITE_MAX       1024

#define WALL           1

// Tile state bits
#define U_MASK         (1<<0)
#define R_MASK         (1<<1)
#define D_MASK         (1<<2)
#define L_MASK         (1<<3)
#define CORE           (1<<4)
#define HIT_MASK       (CORE | U_MASK | R_MASK | D_MASK | L_MASK)
#define HARD           (1<<5)
#define SPACE          (1<<6)
#define	MAP_BITS(x)	((x) & 0xff)
#define	MAP_TILE(x)	((x) >> 8)

#define	IS_SPACE(x)	((x) & SPACE)
#define	IS_BASE(x)	(!IS_SPACE(x) && ((x) & HIT_MASK))

#define	COLL_HORIZONTAL 0x01
#define	COLL_VERTICAL	0x02
#define	COLL_STUCK	0x04

class _map
{
  public:
	void init(const _scene *s);
	inline unsigned short &pos(int x, int y)
	{
		x &= MAP_SIZEX - 1;
		y &= MAP_SIZEY - 1;
		return data[(y << MAP_SIZEX_LOG2) + x];
	}
	// Find first tile collision on the line (x1, y1)-(x3, y3). If there is
	// a solution, the return value indicates if a horizontal and/or
	// vertical wall was hit, the position of the collision is written
	// to (hx, hy), and the intersection point (effectively the point right
	// before collision) is written to (x2, y2). (24:8 world coordinates.)
	int test_line(int x1, int y1, int x3, int y3,
			int *x2, int *y2, int *hx, int *hy);

  protected:
	int sitex[SITE_MAX];
	int sitey[SITE_MAX];
	int site_max;
	unsigned short data[1 << (MAP_SIZEX_LOG2 + MAP_SIZEY_LOG2)];
	int maze_pop();
	void maze_push(int x, int y);
	void maze_move_and_push(int x, int y, int d);
	int maze_judge(int cx, int cy, int dx, int dy, int x, int y);
	void clear();
	void make_maze(int x, int y, int difx, int dify);
	void convert(unsigned ratio);	// ratio < 64
};

#endif	// XKOBO_H_MAP
