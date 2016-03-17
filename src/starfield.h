/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
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

/*-----------------------------------------------------------------------------

This starfield implementation may seem a bit backwards. Instead of moving
around in a 3D point cloud, projecting by dividing x and y by z etc, we're
doing a plain orthogonal projection. That is, the z coordinates have no direct
impact on rendering!

What actually creates the parallax effect is that we scroll the *stars* around
at different speeds, depending on their distance from the screen, which
effectively replaces the usual (x/z, y/z) operation when projecting.

The interesting part is that stars wrap around the edges automatically as a
side effect of the integer arithmetics. Thus we can have a nice, dense "3D"
starfield covering the whole window - no nearby stars way off screen, and no
distant stars wrapping around in a small square in the middle of the screen. No
clipping is needed, and thus, no cycles are wasted animating off-screen stars.

Of course, this design means we cannot move along the Z axis (well, not
trivially, at least), but we don't really need that here anyway.

-----------------------------------------------------------------------------*/

#ifndef KOBO_STARFIELD_H
#define KOBO_STARFIELD_H

#include "window.h"

#define	STAR_COLORS	8

struct KOBO_Star
{
	short		x;
	short		y;
	unsigned short	z;
};

class KOBO_Starfield
{
	window_t *target;
	int altitude;
	int planetsize;
	int planetscale;
	int pivot;
	int nstars;
	KOBO_Star *stars;
	Uint32 colors[STAR_COLORS];
	int oxo, oyo;
	void init_colors(unsigned pal);
  public:
	KOBO_Starfield();
	~KOBO_Starfield();
	void set_target(window_t *_target, unsigned pal);
	bool init(int _nstars, int _altitude, int _planetsize,
			bool distant_only = false);
	void render(int xo, int yo);
};

#endif /* KOBO_STARFIELD_H */
