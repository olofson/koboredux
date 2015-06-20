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
}


spinplanet_t::~spinplanet_t()
{
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
	if(src < 0 || src >= 2)
	{
		log_printf(ELOG, "spinplanet_t::set_source(): Tried to set "
				"undefined source %d!\n", src);
		return;
	}
	sbank[src] = bank;
	sframe[src] = frame;
}


void spinplanet_t::set_mode(spinplanet_modes_t md)
{
	clear();
	mode = md;
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
		refresh(NULL);
		autoinvalidate(1);
		break;
	}
}


void spinplanet_t::refresh_static()
{
	// Source
	Uint32 *src;
	int srcp;
	int w, h;
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
	w = b->w;
	h = b->h;

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
	if(w > width())
		w = width();
	if(h > height())
		h = height();
	buffer = &buffer[pitch * (height() - h) / 2 + (width() - w) / 2];

	// Render!
	for(int y = 0; y < h; ++y)
		for(int x = 0; x < w; ++x)
			buffer[pitch * y + x] = src[srcp * y + x];

	unlock();
}


void spinplanet_t::refresh(SDL_Rect *r)
{
	// Sources
	Uint32 *src[2];
	int srcp[2];
	int w, h;
	for(int i = 0; i < 2; ++i)
	{
		s_bank_t *b = s_get_bank(engine->get_gfx(), sbank[i]);
		if(!b)
		{
			log_printf(ELOG, "spinplanet_t::refresh(): Sprite "
					"bank %d not found!\n", sbank[0]);
			set_mode(SPINPLANET_OFF);
			return;
		}
		s_sprite_t *s = s_get_sprite_b(b, sframe[i]);
		if(!s)
		{
			log_printf(ELOG, "spinplanet_t::refresh(): Sprite "
					"frame %d:%d not found!\n",
					sbank[0], sframe[0]);
			set_mode(SPINPLANET_OFF);
			return;
		}
		src[i] = (Uint32 *)s->surface->pixels;
		srcp[i] = s->surface->pitch / sizeof(Uint32);
		w = b->w;
		h = b->h;
	}

	// Destination
	Uint32 *buffer;
	int pitch = lock(r, &buffer);
	if(!pitch)
	{
		log_printf(ELOG, "spinplanet_t::refresh_static() failed to "
				"lock buffer!\n");
		return;
	}

	// Positioning
	if(w > width())
		w = width();
	if(h > height())
		h = height();
	buffer = &buffer[pitch * (height() - h) / 2 + (width() - w) / 2];

	// Rotation
	int vx = engine->nxoffs(tlayer)/* * w*/;
	int vy = engine->nyoffs(tlayer)/* * h*/;
//printf("%d, %d\n", vx, vy);

	// Render!
	for(int y = 0; y < h; ++y)
		for(int x = 0; x < w; ++x)
			if((x > vx) || (y > vy))
				buffer[pitch * y + x] =
						src[1][srcp[1] * y + x];
			else
				buffer[pitch * y + x] =
						src[0][srcp[0] * y + x];

	unlock();
}


void spinplanet_t::render(SDL_Rect *r)
{
	switch(mode)
	{
	  case SPINPLANET_OFF:
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
