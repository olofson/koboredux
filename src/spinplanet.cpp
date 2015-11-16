/*(LGPLv2.1)
----------------------------------------------------------------------
	spinplanet.cpp - Spinning planet effect
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

#include "spinplanet.h"
#include "gfxengine.h"
#include "sprite.h"
#include "logger.h"


spinplanet_t::spinplanet_t(gfxengine_t *e) : stream_window_t(e)
{
	mode = SPINPLANET_OFF;
	dither = SPINPLANET_DITHER_RAW;
	lens = NULL;
	source = NULL;
	free_source = false;
	sourcepitch = 0;
	ditherstate = 16576;
	dither_brightness = 0;
	dither_contrast = 0;
	xspeed = yspeed = 1.0f;
	trackox = trackoy = 0.5f;
	texrep = 1.0f;
	needs_prepare = true;
}


spinplanet_t::~spinplanet_t()
{
	free(lens);
	if(free_source)
		free(source);
}


void spinplanet_t::clear()
{
	Uint32 *buffer;
	int pitch = lock(NULL, &buffer);
	if(!pitch)
	{
		log_printf(ELOG, "spinplanet_t::clear() failed to lock "
				"buffer!\n");
		return;
	}
	for(int y = 0; y < height(); ++y)
		memset(&buffer[pitch * y], 0, width() * sizeof(Uint32));
	unlock();
}


void spinplanet_t::set_msize(int size)
{
	msize = size;
	msizemask = 1;
	while(msizemask != msize && msizemask)
		msizemask <<= 1;
	if(!msizemask)
		set_mode(SPINPLANET_OFF);
	else
		--msizemask;
}


void spinplanet_t::set_source(int bank, int frame)
{
	sbank = bank;
	sframe = frame;
	s_bank_t *b = s_get_bank(engine->get_gfx(), sbank);
	if(!b)
	{
		log_printf(ELOG, "spinplanet_t::set_source(): Sprite bank %d "
				"not found!\n", sbank);
		set_mode(SPINPLANET_OFF);
		return;
	}
	if(b->h != b->w)
	{
		log_printf(ELOG, "spinplanet_t::set_source(): The sprites of "
				"bank %d are %dx%d, but need to be square!\n",
				sbank, b->w, b->h);
		set_mode(SPINPLANET_OFF);
		return;
	}
	set_msize(b->w);
	if(!msizemask)
		log_printf(ELOG, "spinplanet_t::set_source(): The "
			"world map texture size needs to be a power "
			"of two! (actual: %dx%d)\n", b->w, b->h);
	needs_prepare = true;
}


void spinplanet_t::init_lens()
{
	free(lens);
	lens = (int16_t *)malloc(psize * psize * 2 * sizeof(int16_t));
	int xcenter = width() / 2;
	int ycenter = height() / 2;
	int r = psize / 2;
	float norm90 = 2.0f / M_PI;	// 90° ==> 1.0
	float a2o = msize / 4.0f * texrep;
	int i = 0;
	for(int y = 0; y < height(); ++y)
	{
		float cy = y - ycenter + .5f;

		// Vertical clipping
		if(r*r <= cy*cy)
			continue;

		// Calculate run-length
		int cx = sqrt(r*r - cy*cy) + .5f;
		int xmin = xcenter - cx;
		int xmax = xcenter + cx;

		// Horizontal clipping
		if(xmin < 0)
			xmin = 0;
		if(xmax > width())
			xmax = width();

		// Run header
		lens[i++] = xmin;		// Target X
		lens[i++] = y;			// Target Y
		lens[i++] = xmax - xmin;	// Length

		for(int x = xmin; x < xmax; ++x)
		{
			float dx = x - xcenter + .5f;
			float dy = y - ycenter + .5f;
			float d = sqrt(dx*dx + dy*dy);
			if(d > r)
				d = r;	// Clamp to avoid math errors!
			float mr = asin(d / r) * norm90;
			float xa, ya;
			if(d >= 1.0f)
			{
				xa = mr * dx / d;
				ya = mr * dy / d;
			}
			else
			{
				xa = mr * dx;
				ya = mr * dy;
			}
			lens[i++] = xa * a2o * 16.0f;	// Source X offset
			lens[i++] = ya * a2o * 16.0f;	// Source Y offset
		}
	}
	lens[i++] = 0;	// Target X
	lens[i++] = 0;	// Target Y
	lens[i++] = 0;	// Length (0 == terminator)
	lens = (int16_t *)realloc(lens, i * sizeof(int16_t));
}

void spinplanet_t::set_mode(spinplanet_modes_t md)
{
	clear();
	mode = md;
	lastx = lasty = -1;
	lastnx = lastny = -1.0f;
	wox = woy = 0.0f;
#ifdef DEBUG
	const char *modenames[SPINPLANET_SPIN + 1] = {
		"SPINPLANET_OFF",
		"SPINPLANET_BLACK",
		"SPINPLANET_SPIN"
	};
	log_printf(DLOG, "spinplanet_t::set_mode(%s) (visible: %d)\n",
			modenames[md], visible());
#endif
	switch(mode)
	{
	  case SPINPLANET_OFF:
		autoinvalidate(0);
		break;
	  case SPINPLANET_BLACK:
		break;
	  case SPINPLANET_SPIN:
		autoinvalidate(1);
		break;
	}
}


void spinplanet_t::set_colors(const unsigned char *c, int nc)
{
	if(nc > SPINPLANET_MAX_COLORS)
		nc = SPINPLANET_MAX_COLORS;
	for(int i = 0; i < nc; ++i)
		colors[i] = map_rgb(engine->palette(c[i]));
	for(int i = nc; i <= SPINPLANET_MAX_COLORS; ++i)
		colors[i] = map_rgb(engine->palette(c[nc - 1]));
	needs_prepare = true;
}


void spinplanet_t::set_dither(spinplanet_dither_t dth, int brightness,
		int contrast)
{
	dither = dth;
	dither_brightness = brightness;
	dither_contrast = contrast;
	needs_prepare = true;
}


uint8_t *spinplanet_t::grayscale_convert(uint32_t *src, int sp,
		int w, int h, int brightness, int contrast)
{
	uint8_t *d = (uint8_t *)malloc(w * h);
	if(!d)
		return NULL;
	int i = 0;
	for(int y = 0; y < h; ++y, src += sp)
		for(int x = 0; x < w; ++x)
		{
			int c = (((src[x] >> 8) & 0xff00) +
					(src[x] & 0xff00) +
					((src[x] << 8) & 0xff00)) / 3;
			c += (brightness << 8);
			c = ((c - 0x8000) * (256 + contrast) >> 8) + 0x8000;
			if(c < 0)
				c = 0;
			else if(c > 0xffff)
				c = 0xffff;
			d[i++] = (c + 128) >> 8;
		}
	return d;
}


uint32_t *spinplanet_t::palette_remap(uint8_t *src, int sp, int w, int h,
			uint32_t *palette, int palettesize)
{
	uint32_t *d = (uint32_t *)malloc(w * h * sizeof(uint32_t));
	if(!d)
		return NULL;
	int i = 0;
	for(int y = 0; y < h; ++y, src += sp)
		for(int x = 0; x < w; ++x)
		{
			int c = (src[x] * palettesize + 128) >> 8;
			if(c > palettesize - 1)
				c = palettesize - 1;
			d[i++] = palette[c];
		}
	return d;
}


uint8_t *spinplanet_t::downscale_8bpp(uint8_t *src, int sp, int w, int h,
		int n)
{
	int n2 = n * n;
	uint8_t *d = (uint8_t *)malloc(w * h / n2);
	if(!d)
		return NULL;
	int i = 0;
	for(int y = 0; y < h; y += n, src += sp * n)
		for(int x = 0; x < w; x += n)
		{
			int c = 0;
			uint8_t *s2 = src;
			for(int sy = 0; sy < n; ++sy, s2 += sp)
				for(int sx = 0; sx < n; ++sx)
					c += src[x + sx];
			d[i++] = c / n2;
		}
	return d;
}


uint32_t *spinplanet_t::downscale_32bpp(uint32_t *src, int sp, int w, int h,
		int n)
{
	int n2 = n * n;
	uint32_t *d = (uint32_t *)malloc(w * h / n2 * sizeof(uint32_t));
	if(!d)
		return NULL;
	int i = 0;
	for(int y = 0; y < h; y += n, src += sp * n)
		for(int x = 0; x < w; x += n)
		{
			int r = 0;
			int g = 0;
			int b = 0;
			uint32_t *s2 = src;
			for(int sy = 0; sy < n; ++sy, s2 += sp)
				for(int sx = 0; sx < n; ++sx)
				{
					int c = src[x + sx];
					r += (c >> 16) & 0xff;
					g += (c >> 8) & 0xff;
					b += c & 0xff;
				}
			d[i++] = (r / n2 << 16) | (g / n2 << 8) | b / n2;
		}
	return d;
}


void spinplanet_t::scale_texture()
{
	void *newsource = NULL;
	int newmsize = msize;
	while(newmsize > psize * 6)
		newmsize >>= 1;
	if(newmsize == msize)
		return;

	switch(dither)
	{
	  case SPINPLANET_DITHER_RAW:
		// We never scale in raw mode!
		return;
	  case SPINPLANET_DITHER_NONE:
	  case SPINPLANET_DITHER_TRUECOLOR:
		newsource = downscale_32bpp((uint32_t *)source, sourcepitch,
				msize, msize, msize / newmsize);
		break;
	  case SPINPLANET_DITHER_RANDOM:
	  case SPINPLANET_DITHER_ORDERED:
	  case SPINPLANET_DITHER_SKEWED:
	  case SPINPLANET_DITHER_NOISE:
	  case SPINPLANET_DITHER_SEMIINTERLACE:
	  case SPINPLANET_DITHER_INTERLACE:
		newsource = downscale_8bpp((uint8_t *)source, sourcepitch,
				msize, msize, msize / newmsize);
		break;
	}
	if(!newsource)
		return;

	if(free_source)
	{
		free(source);
		free_source = false;
	}
	source = newsource;
	sourcepitch = newmsize;
	free_source = true;
	set_msize(newmsize);
}


void spinplanet_t::dth_prepare()
{
	if(free_source)
	{
		free(source);
		source = 0;
		sourcepitch = 0;
		free_source = false;
	}

	s_bank_t *bank = s_get_bank(engine->get_gfx(), sbank);
	if(!bank)
	{
		log_printf(ELOG, "spinplanet_t::refresh(): Sprite "
				"bank %d not found!\n", sbank);
		set_mode(SPINPLANET_OFF);
		return;
	}
	s_sprite_t *s = s_get_sprite_b(bank, sframe);
	if(!s)
	{
		log_printf(ELOG, "spinplanet_t::refresh(): Sprite "
				"frame %d:%d not found!\n",
				sbank, sframe);
		set_mode(SPINPLANET_OFF);
		return;
	}
	uint32_t *src = (uint32_t *)s->surface->pixels;
	int sp = s->surface->pitch / sizeof(uint32_t);

	switch(dither)
	{
	  case SPINPLANET_DITHER_RAW:
		source = src;
		sourcepitch = sp;
		break;
	  case SPINPLANET_DITHER_NONE:
	  case SPINPLANET_DITHER_TRUECOLOR:
	  {
		uint32_t *clrs = colors;
		int nclrs = SPINPLANET_MAX_COLORS;
		uint32_t gradient[256];
		uint8_t *tmp = grayscale_convert(src, sp,
				s->surface->w, s->surface->h,
				dither_brightness, dither_contrast);
		if(!tmp)
			return;
		if(dither == SPINPLANET_DITHER_TRUECOLOR)
		{
			for(int i = 0; i < 256; ++i)
			{
				int i1 = i * SPINPLANET_MAX_COLORS >> 8;
				int i2 = i > 255 ? i1 : i1 + 1;
				int f = i % SPINPLANET_MAX_COLORS;
				int rf = SPINPLANET_MAX_COLORS - f;
				int c1 = colors[i1];
				int c2 = colors[i2];
				int r = (((c1 >> 16) & 0xff) * rf +
						((c2 >> 16) & 0xff) * f) /
						SPINPLANET_MAX_COLORS;
				int g = (((c1 >> 8) & 0xff) * rf +
						((c2 >> 8) & 0xff) * f) /
						SPINPLANET_MAX_COLORS;
				int b = ((c1 & 0xff) * rf +
						(c2 & 0xff) * f) /
						SPINPLANET_MAX_COLORS;
				gradient[i] = (r << 16) | (g << 8) | b;
			}
			clrs = gradient;
			nclrs = 256;
		}
		source = palette_remap(tmp, s->surface->w,
				s->surface->w, s->surface->h, clrs, nclrs);
		free(tmp);
		if(!source)
		{
			set_mode(SPINPLANET_OFF);
			return;
		}
		sourcepitch = s->surface->w;
		free_source = true;
		break;
	  }
	  case SPINPLANET_DITHER_RANDOM:
	  case SPINPLANET_DITHER_ORDERED:
	  case SPINPLANET_DITHER_SKEWED:
	  case SPINPLANET_DITHER_NOISE:
	  case SPINPLANET_DITHER_SEMIINTERLACE:
	  case SPINPLANET_DITHER_INTERLACE:
		if(!(source = grayscale_convert(src, sp,
				s->surface->w, s->surface->h,
				dither_brightness, dither_contrast)))
		{
			set_mode(SPINPLANET_OFF);
			return;
		}
		sourcepitch = s->surface->w;
		free_source = true;
		break;
	}
	scale_texture();
}


inline void spinplanet_t::dth_raw(uint32_t *s, int sp, Uint32 *d,
		int16_t *l, int len, int x, int y, int vx, int vy)
{
	for(int j = 0; j < len; ++j)
	{
		int mx = ((vx + l[j * 2]) >> 4) & msizemask;
		int my = ((vy + l[j * 2 + 1]) >> 4) & msizemask;
		d[j] = s[sp * my + mx];
	}
}

inline void spinplanet_t::dth_random(uint8_t *s, int sp, Uint32 *d,
		int16_t *l, int len, int x, int y, int vx, int vy)
{
	for(int j = 0; j < len; ++j)
	{
		int mx = ((vx + l[j * 2]) >> 4) & msizemask;
		int my = ((vy + l[j * 2 + 1]) >> 4) & msizemask;
		int c = s[sp * my + mx];
		c = (c + (noise() & 0xf)) >> 4;
		d[j] = colors[c];
	}
}

inline void spinplanet_t::dth_ordered(uint8_t *s, int sp, Uint32 *d,
		int16_t *l, int len, int x, int y, int vx, int vy)
{
	for(int j = 0; j < len; ++j)
	{
		int mx = ((vx + l[j * 2]) >> 4) & msizemask;
		int my = ((vy + l[j * 2 + 1]) >> 4) & msizemask;
		int c = s[sp * my + mx];
		int dth = (((((x + j) ^ y) & 1) << 1) + (y & 1)) << 2;
		c = (c + dth) >> 4;
		d[j] = colors[c];
	}
}

void spinplanet_t::refresh(SDL_Rect *r)
{
	if(needs_prepare)
	{
		dth_prepare();
		init_lens();
		needs_prepare = false;
	}

	// "Rotation"
	float nx = engine->nxoffs(tlayer) + trackox;
	float ny = engine->nyoffs(tlayer) + trackoy;
	if(fabs(nx - lastnx) > 0.5f)
		wox += nx < lastnx ? 1.0f : -1.0f;
	if(fabs(ny - lastny) > 0.5f)
		woy += ny < lastny ? 1.0f : -1.0f;
	lastnx = nx;
	lastny = ny;
	// + 0.5f to get (0, 0) in the center of the texture
	int vx = ((nx + wox) * xspeed + 0.5f) * msize * 16.0f;
	int vy = ((ny + woy) * yspeed + 0.5f) * msize * 16.0f;

	switch(dither)
	{
	  case SPINPLANET_DITHER_RAW:
	  case SPINPLANET_DITHER_NONE:
	  case SPINPLANET_DITHER_RANDOM:
	  case SPINPLANET_DITHER_ORDERED:
	  case SPINPLANET_DITHER_SKEWED:
	  case SPINPLANET_DITHER_TRUECOLOR:
		ditherstate = 16576;	// For random dither
		if((vx == lastx) && (vy == lasty))
			return;		// Position hasn't changed!
		lastx = vx;
		lasty = vy;
		break;
	  case SPINPLANET_DITHER_NOISE:
	  case SPINPLANET_DITHER_SEMIINTERLACE:
	  case SPINPLANET_DITHER_INTERLACE:
		// Temporal dithering always at full frame rate!
		break;
	}

	// Source
	if(!source)
		return;

	// Destination
	Uint32 *buffer;
	int pitch = lock(r, &buffer);
	if(!pitch)
	{
		log_printf(ELOG, "spinplanet_t::refresh_static() failed to "
				"lock buffer!\n");
		return;
	}

	// Render!
	int i = 0;
	while(1)
	{
		int x = lens[i++];
		int y = lens[i++];
		int ldlen = lens[i++];
		if(!ldlen)
			break;	// Terminator!
		Uint32 *dst = &buffer[pitch * y + x];
		int16_t *ld = &lens[i];
		switch(dither)
		{
		  case SPINPLANET_DITHER_RAW:
		  case SPINPLANET_DITHER_NONE:
		  case SPINPLANET_DITHER_TRUECOLOR:
			dth_raw((uint32_t *)source, sourcepitch,
					dst, ld, ldlen, x, y, vx, vy);
			break;
		  case SPINPLANET_DITHER_RANDOM:
			dth_random((uint8_t *)source, sourcepitch,
					dst, ld, ldlen, x, y, vx, vy);
			break;
		  case SPINPLANET_DITHER_ORDERED:
			dth_ordered((uint8_t *)source, sourcepitch,
					dst, ld, ldlen, x, y, vx, vy);
			break;
		  case SPINPLANET_DITHER_SKEWED:
			x += (y & 2) >> 1;
			dth_ordered((uint8_t *)source, sourcepitch,
					dst, ld, ldlen, x, y, vx, vy);
			break;
		  case SPINPLANET_DITHER_NOISE:
			dth_random((uint8_t *)source, sourcepitch,
					dst, ld, ldlen, x, y, vx, vy);
			break;
		  case SPINPLANET_DITHER_SEMIINTERLACE:
			x += ditherstate;
			y += ditherstate;
			dth_ordered((uint8_t *)source, sourcepitch,
					dst, ld, ldlen, x, y, vx, vy);
			break;
		  case SPINPLANET_DITHER_INTERLACE:
			x += ditherstate >> 1;
			y += ditherstate;
			dth_ordered((uint8_t *)source, sourcepitch,
					dst, ld, ldlen, x, y, vx, vy);
			break;
		}
		i += ldlen * 2;
	}
	unlock();
	++ditherstate;
}


void spinplanet_t::render(SDL_Rect *r)
{
	switch(mode)
	{
	  case SPINPLANET_OFF:
		break;
	  case SPINPLANET_BLACK:
		select();
		SDL_SetRenderDrawColor(renderer,
				get_r(bgcolor), get_g(bgcolor),
				get_b(bgcolor), get_a(bgcolor));
		SDL_RenderFillRect(renderer, r);
		break;
	  case SPINPLANET_SPIN:
		stream_window_t::render(r);
		break;
	}
}
