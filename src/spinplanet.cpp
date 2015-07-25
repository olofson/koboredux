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
	lens = NULL;
#ifdef	KOBO_PLANET_DITHER
	ditherstate = 16576;
#endif
	xspeed = yspeed = 1.0f;
	trackox = trackoy = 0.5f;
	texrep = 1.0f;
}


spinplanet_t::~spinplanet_t()
{
	free(lens);
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


void spinplanet_t::set_source(int src, int bank, int frame)
{
	if(src < 0 || src >= 3)
	{
		log_printf(ELOG, "spinplanet_t::set_source(): Tried to set "
				"undefined source %d!\n", src);
		return;
	}
	sbank[src] = bank;
	sframe[src] = frame;
	s_bank_t *b = s_get_bank(engine->get_gfx(), sbank[src]);
	if(!b)
	{
		log_printf(ELOG, "spinplanet_t::set_source(): Sprite bank %d "
				"not found!\n", sbank[src]);
		set_mode(SPINPLANET_OFF);
		return;
	}
	if(b->h != b->w)
	{
		log_printf(ELOG, "spinplanet_t::set_source(): The sprites of "
				"bank %d are %dx%d, but need to be square!\n",
				sbank[src], b->w, b->h);
		set_mode(SPINPLANET_OFF);
		return;
	}
	switch(src)
	{
	  case 0:
	  case 1:
		if(((int)b->w > width()) || ((int)b->h > height()))
		{
			log_printf(ELOG, "spinplanet_t::set_source(): The "
				"sprites of bank %d are larger than the "
				"window! (s: %dx%d, w: %dx%d)\n", sbank[src],
				b->w, b->h, width(), height());
			set_mode(SPINPLANET_OFF);
			return;
		}
		break;
	}
	switch(src)
	{
	  case 0:
		psize = b->w;
		free(lens);
		lens = NULL;
		break;
	  case 2:
		msize = b->w;
		msizemask = 1;
		while(msizemask != msize && msizemask)
			msizemask <<= 1;
		if(!msizemask)
		{
			log_printf(ELOG, "spinplanet_t::set_source(): The "
				"world map texture size needs to be a power "
				"of two! (actual: %dx%d)\n", b->w, b->h);
			set_mode(SPINPLANET_OFF);
		}
		--msizemask;
		break;
	}
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
#ifndef KOBO_PLANET_DITHER
	lastx = lasty = -1;
#endif
	lastnx = lastny = -1.0f;
	wox = woy = 0.0f;
	switch(mode)
	{
	  case SPINPLANET_OFF:
		autoinvalidate(0);
		break;
	  case SPINPLANET_STATIC:
		refresh_static();
		autoinvalidate(0);
		break;
	  case SPINPLANET_SPIN:
		init_lens();
		refresh(NULL);
		autoinvalidate(1);
		break;
	}
}


void spinplanet_t::refresh_static()
{
//TODO: Clipping! Or, just remove this mode altogether... Not used!

	// Source
	Uint32 *src;
	int srcp;
	s_bank_t *b = s_get_bank(engine->get_gfx(), sbank[0]);
	if(!b)
	{
		log_printf(ELOG, "spinplanet_t::refresh_static(): "
				"Sprite bank %d not found!\n", sbank[0]);
		set_mode(SPINPLANET_OFF);
		return;
	}
	s_sprite_t *s = s_get_sprite_b(b, sframe[0]);
	if(!s)
	{
		log_printf(ELOG, "spinplanet_t::refresh_static(): "
				"Sprite frame %d:%d not found!\n",
				sbank[0], sframe[0]);
		set_mode(SPINPLANET_OFF);
		return;
	}
	src = (Uint32 *)s->surface->pixels;
	srcp = s->surface->pitch / sizeof(Uint32);

	// Destination
	Uint32 *buffer;
	int pitch = lock(NULL, &buffer);
	if(!pitch)
	{
		log_printf(ELOG, "spinplanet_t::refresh_static() failed to "
				"lock buffer!\n");
		return;
	}

	// Positioning
	buffer = &buffer[pitch * (height() - psize) / 2 +
			(width() - psize) / 2];

	// Render!
	for(int y = 0; y < psize; ++y)
		for(int x = 0; x < psize; ++x)
			buffer[pitch * y + x] = src[srcp * y + x];

	unlock();
}


void spinplanet_t::refresh(SDL_Rect *r)
{
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
#ifndef	KOBO_PLANET_DITHER
	if((vx == lastx) && (vy == lasty))
		return;		// Position hasn't changed!
	lastx = vx;
	lasty = vy;
#endif

	// Source
	s_bank_t *b = s_get_bank(engine->get_gfx(), sbank[2]);
	if(!b)
	{
		log_printf(ELOG, "spinplanet_t::refresh(): Sprite "
				"bank %d not found!\n", sbank[2]);
		set_mode(SPINPLANET_OFF);
		return;
	}
	s_sprite_t *s = s_get_sprite_b(b, sframe[2]);
	if(!s)
	{
		log_printf(ELOG, "spinplanet_t::refresh(): Sprite "
				"frame %d:%d not found!\n",
				sbank[2], sframe[2]);
		set_mode(SPINPLANET_OFF);
		return;
	}
	Uint32 *src = (Uint32 *)s->surface->pixels;
	int srcpitch = s->surface->pitch / sizeof(Uint32);

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
		int len = lens[i++];
		if(!len)
			break;	// Terminator!
		Uint32 *d = &buffer[pitch * y + x];
		int16_t *l = &lens[i];
		for(int j = 0; j < len; ++j)
		{
#ifdef	KOBO_PLANET_DITHER
			int nx = dither();
			int ny = (nx >> 8) & 0xf;
			nx &= 0xf;
			int mx = ((vx + l[j * 2] + nx) >> 4) & msizemask;
			int my = ((vy + l[j * 2 + 1] + ny) >> 4) & msizemask;
#else
			int mx = ((vx + l[j * 2]) >> 4) & msizemask;
			int my = ((vy + l[j * 2 + 1]) >> 4) & msizemask;
#endif
			d[j] = src[srcpitch * my + mx];
		}
		i += len * 2;
	}
	unlock();
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
	  case SPINPLANET_STATIC:
	  case SPINPLANET_SPIN:
		stream_window_t::render(r);
		break;
	}
}
