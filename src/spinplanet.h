/*(LGPLv2.1)
----------------------------------------------------------------------
	spinplanet.h - Spinning planet effect
----------------------------------------------------------------------
 * Copyright 2015 David Olofson (Kobo Redux)
 *
 * This library is free software;  you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation;  either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library  is  distributed  in  the hope that it will be useful,  but
 * WITHOUT   ANY   WARRANTY;   without   even   the   implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library;  if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef	_SPINPLANET_H_
#define	_SPINPLANET_H_

#include "window.h"
#include "config.h"

enum spinplanet_modes_t
{
	SPINPLANET_OFF,
	SPINPLANET_BLACK,
	SPINPLANET_STATIC,
	SPINPLANET_SPIN
};

class spinplanet_t : public stream_window_t
{
	int sbank[3], sframe[3];
	int tlayer;		// Engine scroll layer to track
	int psize;		// Actual planet size
	int msize;		// World map size
	int msizemask;		// Mask for msize wrapping
	float texrep;		// Texture repeat factor

	// Tracking wrap handling
	float xspeed, yspeed;	// Tracking speed scaling factors
	float trackox, trackoy;	// Tracking offset
	float lastnx, lastny;	// Last normalized offsets
	float wox, woy;		// Wrap offsets

#ifdef	KOBO_PLANET_DITHER
	unsigned ditherstate;	// Dither state
#else
	int lastx, lasty;	// Last rendered map position
#endif

	// Lens encoding:
	//	[i + 0]: target X (pixels)
	//	[i + 1]: target Y (pixels)
	//	[i + 2]: length (pixels; 0: end of data)
	//	[i + 3 + n]: source X offset (12:4)
	//	[i + 3 + n + 1]: source Y offset (12:4)
	int16_t *lens;

	spinplanet_modes_t mode;
	void refresh_static();
	void init_lens();
#ifdef	KOBO_PLANET_DITHER
	int dither()
	{
		ditherstate *= 1566083941UL;
		ditherstate++;
		return (int)(ditherstate * (ditherstate >> 16) >> 16);
	}
#endif
  public:
	spinplanet_t(gfxengine_t *e);
	virtual ~spinplanet_t();
	void clear();
	void set_source(int src, int bank, int frame);
	void set_planet(int bank, int frame)
	{
		set_source(0, bank, frame);
	}
	void set_map(int bank, int frame)
	{
		set_source(2, bank, frame);
	}
	void set_size(int size)			{ psize = size; }
	void set_mode(spinplanet_modes_t md);
	void set_texture_repeat(int txr)	{ texrep = txr; }
	void track_layer(int lr)		{ tlayer = lr; }
	void track_speed(float xs, float ys)
	{
		xspeed = xs;
		yspeed = ys;
	}
	void track_offset(float tox, float toy)
	{
		trackox = tox;
		trackoy = toy;
	}
	void refresh(SDL_Rect *r);
	void render(SDL_Rect *r);
};

#endif // _SPINPLANET_H_
