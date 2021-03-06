/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
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

#include "starfield.h"
#include "gfxengine.h"
#include "random.h"
#include "config.h"


KOBO_Starfield::KOBO_Starfield()
{
	target = NULL;
	nstars = 0;
	stars = NULL;
	oxo = 0;
	oyo = 0;
}


KOBO_Starfield::~KOBO_Starfield()
{
	free(stars);
}


void KOBO_Starfield::init_colors(unsigned pal)
{
	ncolors = target->get_engine()->palette_size(pal);
	if(ncolors > MAX_STAR_COLORS)
		ncolors = MAX_STAR_COLORS;
	for(unsigned i = 0; i < ncolors; ++i)
		colors[ncolors - i - 1] = target->map_rgb(
				target->get_engine()->palette(pal, i));
}


void KOBO_Starfield::set_target(window_t *_target, unsigned pal)
{
	target = _target;
	init_colors(pal);
}


bool KOBO_Starfield::init(int _nstars, int _altitude, int _planetsize,
		bool distant_only)
{
	altitude = _altitude;
	planetsize = _planetsize;
	if(distant_only)
	{
		planetscale = altitude / 2;
		pivot = 0;
	}
	else
	{
		if(altitude < 64)
			planetscale = 512;
		else if(altitude > 192)
			planetscale = 256;
		else
			planetscale = 512 - ((altitude - 64) << 1);
		pivot = altitude;
	}
	if(_nstars != nstars)
	{
		free(stars);
		nstars = _nstars;
		stars = (KOBO_Star *)malloc(nstars * sizeof(KOBO_Star));
		if(!stars)
		{
			nstars = 0;
			return false;		// Out of memory!!!
		}
	}
	for(int i = 0; i < nstars; ++i)
	{
		stars[i].x = pubrand.get();
		stars[i].y = pubrand.get();
		int zz = 255 * i / nstars;
		stars[i].z = 65025 - zz * zz;
	}
	return true;
}


void KOBO_Starfield::render(int xo, int yo)
{
	int w = target->width() * 256;
	int h = target->height() * 256;
	if(!w || !h)
		return;

	int xc = w / 2;
	int yc = h / 2;
	int pivot8 = pivot << 8;	// Rotation pivot z coordinate
	int pclip = planetsize * planetsize / 4; // Planet radius squared

	// Calculate delta from last position, dealing with map position wrap
	const int WSX = WORLD_SIZEX << 8;
	const int WSY = WORLD_SIZEY << 8;
	int dx = (xo - oxo) & (WSX - 1);
	oxo = xo;
	if(dx & (WSX >> 1))
		dx |= 1 - (WSX - 1);
	int dy = (yo - oyo) & (WSY - 1);
	oyo = yo;
	if(dy & (WSY >> 1))
		dy |= 1 - (WSY - 1);

	// Scale the deltas to compensate for window/starfield size mismatch
	// (Otherwise stars at zero distance won't sync up with the map as
	// intended!)
	dx = (dx << 16) / w;
	dy = (dy << 16) / h;

	target->select();
	for(int i = 0; i < nstars; ++i)
	{
		int iz = (int)stars[i].z;
		int z =  (iz - pivot8) * planetscale >> 8;

		// Move star!
		stars[i].x += dx * z >> 16;
		stars[i].y += dy * z >> 16;

		// Scale to fixp "native" display coordinates
		int x = stars[i].x * (w >> 8) >> 8;
		int y = stars[i].y * (h >> 8) >> 8;

		// Skip stars occluded by the planet
		int xi = x >> 8;
		int yi = y >> 8;
		if((z > 0) && ((xi * xi + yi * yi) < pclip))
			continue;

		// Center
		x += xc;
		y += yc;

		// Plot!
		target->foreground(colors[iz * ncolors >> 16]);
		target->fillrect_fxp(x, y, 256, 256);
	}
}
