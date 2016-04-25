/*(LGPLv2.1)
----------------------------------------------------------------------
	spinplanet.h - Spinning planet effect
----------------------------------------------------------------------
 * Copyright 2015-2016 David Olofson (Kobo Redux)
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

#define	SPINPLANET_MAX_COLORS	16

enum spinplanet_modes_t
{
	SPINPLANET_OFF,
	SPINPLANET_BLACK,
	SPINPLANET_SPIN
};

enum spinplanet_dither_t
{
	SPINPLANET_DITHER_RAW,		// Use source texture pixels as is
	SPINPLANET_DITHER_NONE,		// Map to nearest palette entry
	SPINPLANET_DITHER_RANDOM,	// Random dither pattern
	SPINPLANET_DITHER_ORDERED,	// 4x4 ordered pattern
	SPINPLANET_DITHER_SKEWED,	// 4x4 skewed pattern
	SPINPLANET_DITHER_NOISE,	// Temporal noise dither
	SPINPLANET_DITHER_TEMPORAL2,	// 4x4 pattern, two frames
	SPINPLANET_DITHER_TEMPORAL4,	// 4x4 pattern, four frames
	SPINPLANET_DITHER_TRUECOLOR	// Interpolate between palette entries
};

class spinplanet_t : public stream_window_t
{
	int sbank, sframe;
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

	unsigned ditherstate;	// Dither state
	int dither_brightness;
	int dither_contrast;
	Uint32 colors[SPINPLANET_MAX_COLORS + 1];	// Dither palette
	int lastx, lasty;	// Last rendered map position

	bool needs_prepare;

	// Lens encoding:
	//	[i + 0]: target X (pixels)
	//	[i + 1]: target Y (pixels)
	//	[i + 2]: length (pixels; 0: end of data)
	//	[i + 3 + n]: source X offset (12:4)
	//	[i + 3 + n + 1]: source Y offset (12:4)
	int16_t *lens;

	// Source texture; either 32 bpp xRGB or 8 bpp grayscale
	void *source;		// uint8_t grayscale, or uint32_t xRGB
	int sourcepitch;	// Pixels
	bool free_source;

	spinplanet_modes_t mode;
	spinplanet_dither_t dither;

	void set_msize(int size);
	void init_lens();
	uint8_t *grayscale_convert(uint32_t *src, int sp, int w, int h,
			int brightness, int contrast);
	uint32_t *palette_remap(uint8_t *src, int sp, int w, int h,
			uint32_t *palette, int palettesize);
	uint8_t *downscale_8bpp(uint8_t *src, int sp, int w, int h, int n);
	uint32_t *downscale_32bpp(uint32_t *src, int sp, int w, int h, int n);
	void scale_texture();
	void dth_prepare();

	inline int noise()
	{
		ditherstate *= 1566083941UL;
		ditherstate++;
		return (int)(ditherstate * (ditherstate >> 16) >> 16);
	}
	inline void dth_raw(uint32_t *s, int sp, Uint32 *d,
			int16_t *l, int len, int x, int y, int vx, int vy);
	inline void dth_random(uint8_t *s, int sp, Uint32 *d,
			int16_t *l, int len, int x, int y, int vx, int vy);
	inline void dth_ordered(uint8_t *s, int sp, Uint32 *d,
			int16_t *l, int len, int x, int y, int vx, int vy);
  public:
	spinplanet_t(gfxengine_t *e);
	virtual ~spinplanet_t();
	void clear();
	void set_source(int bank, int frame);
	void set_palette(unsigned pal);
	void set_size(int size)			{ psize = size; }
	void set_mode(spinplanet_modes_t md);
	void set_dither(spinplanet_dither_t dth, int brightness, int contrast);
	void set_texture_repeat(int txr)	{ texrep = txr; }
	void track_layer(int lr)		{ tlayer = lr; }
	void track_speed(float _xspeed, float _yspeed)
	{
		xspeed = _xspeed;
		yspeed = _yspeed;
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
