/*(LGPLv2.1)
---------------------------------------------------------------------------
	window.cpp - Generic Rendering Window
---------------------------------------------------------------------------
 * Copyright (C) 2001-2003, 2006-2007, 2009 David Olofson
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

#include "logger.h"
#include "window.h"
#include "gfxengine.h"
#include "sofont.h"

#define	SELECT	if(selected != this) _select();

window_t *window_t::selected = NULL;

window_t::window_t()
{
	engine = NULL;
	surface = NULL;
	next = NULL;
	prev = NULL;
	phys_rect.x = phys_rect.y = 0;
	phys_rect.w = 320;
	phys_rect.h = 240;
	fgcolor = bgcolor = 0;
	selected = 0;
	xs = ys = 256;
	bg_bank = -1;
	bg_frame = -1;
	_visible = 1;
	_offscreen = 0;
}


window_t::~window_t()
{
	if(_offscreen && surface)
		SDL_FreeSurface(surface);
	if(selected == this)
		selected = NULL;
	unlink();
}


void window_t::init(gfxengine_t *e)
{
	link(e);
	xs = engine->xs;
	ys = engine->ys;
	if(_offscreen && surface)
	{
		SDL_FreeSurface(surface);
		surface = NULL;
	}
	_offscreen = 0;
	surface = engine->surface();
}


void window_t::visible(int vis)
{
	if(_offscreen)
		return;		// Cannot be visible!
	if(_visible == vis)
		return;
	_visible = vis;
	invalidate();
}


void window_t::link(gfxengine_t *e)
{
	unlink();
	engine = e;
	next = NULL;
	prev = engine->windows;
	if(prev)
	{
		while(prev->next)
			prev = prev->next;
		prev->next = this;
	}
	else
		engine->windows = this;
}


void window_t::unlink(void)
{
	if(engine)
	{
		if(engine->windows == this)
			engine->windows = next;
		if(engine->surface() == surface)
			surface = NULL;
	}
	if(next)
		next->prev = prev;
	if(prev)
		prev->next = next;
}


void window_t::place(int left, int top, int sizex, int sizey)
{
	int x2 = ((left + sizex) * xs + 128) >> 8;
	int y2 = ((top + sizey) * ys + 128) >> 8;
	phys_rect.x = (left * xs + 128) >> 8;
	phys_rect.y = (top * ys + 128) >> 8;
	phys_rect.w = x2 - phys_rect.x;
	phys_rect.h = y2 - phys_rect.y;
}


int window_t::offscreen()
{
	if(!engine)
		return -1;
	if(!engine->surface())
		return -1;
	if(_offscreen)
		return 0;	// Already offscreen!
	visible(0);
	_offscreen = 1;
	SDL_Surface *s = SDL_CreateRGBSurface(SDL_SWSURFACE,
			phys_rect.w, phys_rect.h,
			32, 0xff000000, 0x00ff0000,
			0x0000ff00, 0x000000ff);
	if(!s)
		return -1;
	surface = SDL_DisplayFormat(s);
	SDL_FreeSurface(s);
	if(!surface)
		return -1;
	return 0;
}


void window_t::select()
{
	if(engine)
		_select();
}


void window_t::_select()
{
	SDL_Rect r = phys_rect;
	selected = this;
	if(surface)
		SDL_SetClipRect(surface, &r);
}


void window_t::invalidate(SDL_Rect *r)
{
	if(!engine)
		return;
	if(!engine->surface())
		return;

	SELECT

	if(_offscreen)
	{
		if(!r)
		{
			SDL_Rect rr;
			rr.x = 0;
			rr.y = 0;
			rr.w = phys_rect.w;
			rr.h = phys_rect.h;
			phys_refresh(&rr);
			glSDL_Invalidate(surface, &rr);
		}
		else
		{
			phys_refresh(r);
			glSDL_Invalidate(surface, r);
		}
		return;
	}

	if(!r)
		engine->invalidate(&phys_rect, this);
	else
	{
		/* Translate to screen coordinates */
		SDL_Rect dr = *r;
		dr.x = (dr.x * xs + 128) >> 8;
		dr.y = (dr.y * ys + 128) >> 8;
		dr.w = (((dr.w + dr.x) * xs + 128) >> 8) - dr.x;
		dr.h = (((dr.h + dr.y) * ys + 128) >> 8) - dr.y;
		dr.x += phys_rect.x;
		dr.y += phys_rect.y;

		/* Clip to window (stolen from SDL_surface.c) */
		int Amin, Amax, Bmin, Bmax;

		/* Horizontal intersection */
		Amin = dr.x;
		Amax = Amin + dr.w;
		Bmin = phys_rect.x;
		Bmax = Bmin + phys_rect.w;
		if(Bmin > Amin)
			Amin = Bmin;
		dr.x = Amin;
		if(Bmax < Amax)
			Amax = Bmax;
		dr.w = Amax - Amin > 0 ? Amax - Amin : 0;

		/* Vertical intersection */
		Amin = dr.y;
		Amax = Amin + dr.h;
		Bmin = phys_rect.y;
		Bmax = Bmin + phys_rect.h;
		if(Bmin > Amin)
			Amin = Bmin;
		dr.y = Amin;
		if(Bmax < Amax)
			Amax = Bmax;
		dr.h = Amax - Amin > 0 ? Amax - Amin : 0;

		if(dr.w && dr.h)
			engine->invalidate(&dr, this);
	}
}


/*---------------------------------------------------------------
	Rendering API
---------------------------------------------------------------*/

Uint32 window_t::map_rgb(Uint8 r, Uint8 g, Uint8 b)
{
	if(!engine)
		return 0;

	if(surface)
		return SDL_MapRGB(surface->format, r, g, b);
	else
		return 0xffffff;
}

Uint32 window_t::map_rgb(Uint32 rgb)
{
	if(!engine)
		return 0;

	Uint8 r = (rgb >> 16) & 0xff;
	Uint8 g = (rgb >> 8) & 0xff;
	Uint8 b = rgb & 0xff;

	if(surface)
		return SDL_MapRGB(surface->format, r, g, b);
	else
		return 0xffffff;
}

void window_t::bgimage(int bank, int frame)
{
	bg_bank = bank;
	bg_frame = frame;
}


void window_t::colorkey(Uint32 color)
{
	if(!engine)
		return;
	if(!engine->surface())
		return;
	if(!_offscreen)
		return;
	SDL_SetColorKey(surface, SDL_SRCCOLORKEY, color);
}

void window_t::colorkey()
{
	if(!engine)
		return;
	if(!engine->surface())
		return;
	if(!_offscreen)
		return;
	SDL_SetColorKey(surface, 0, 0);
}

void window_t::alpha(float a)
{
	if(!engine)
		return;
	if(!engine->surface())
		return;
	if(!_offscreen)
		return;
	SDL_SetAlpha(surface, SDL_SRCALPHA, (int)(a * 255.0));
}


void window_t::font(int fnt)
{
	_font = fnt;
}


void window_t::string(int _x, int _y, const char *txt)
{
	string_fxp(PIXEL2CS(_x), PIXEL2CS(_y), txt);
}


void window_t::center(int _y, const char *txt)
{
	center_fxp(PIXEL2CS(_y), txt);
}


void window_t::center_token(int _x, int _y, const char *txt,
		signed char token)
{
	center_token_fxp(PIXEL2CS(_x), PIXEL2CS(_y), txt, token);
}


void window_t::string_fxp(int _x, int _y, const char *txt)
{
	if(!engine)
		return;

	_x = CS2PIXEL((_x * xs + 128) >> 8);
	_y = CS2PIXEL((_y * ys + 128) >> 8);
	SoFont *f = engine->get_font(_font);
	if(!f)
		return;
	SELECT
	_x += phys_rect.x;
	_y += phys_rect.y;
	if(surface)
		f->PutString(surface, _x, _y, txt);
}


void window_t::center_fxp(int _y, const char *txt)
{
	_y = CS2PIXEL((_y * ys + 128) >> 8);

	if(!engine)
		return;

	SoFont *f = engine->get_font(_font);
	if(!f)
		return;
	SELECT
	int _x = (phys_rect.w - f->TextWidth(txt) + 1) / 2;
	_x += phys_rect.x;
	_y += phys_rect.y;
	if(surface)
		f->PutString(surface, _x, _y, txt);
}


void window_t::center_token_fxp(int _x, int _y, const char *txt,
		signed char token)
{
	_x = CS2PIXEL((_x * xs + 128) >> 8);
	_y = CS2PIXEL((_y * ys + 128) >> 8);

	if(!engine)
		return;

	SoFont *f = engine->get_font(_font);
	if(!f)
		return;
	SELECT
	int _cx;
	if(-1 == token)
		_cx = _x - f->TextWidth(txt)/2;
	else
	{
		int tokpos;
		/*
		 * My docs won't say if strchr(???, 0) is legal
		 * or even defined, so I'm not taking any chances...
		 */
		if(token)
		{
			const char *tok = strchr(txt, token);
			if(tok)
				tokpos = tok-txt;
			else
				tokpos = 255;
		}
		else
			tokpos = 255;
		_cx = _x - f->TextWidth(txt, 0, tokpos);
	}
	_cx += phys_rect.x;
	_y += phys_rect.y;
	if(surface)
		f->PutString(surface, _cx, _y, txt);
}


int window_t::textwidth_fxp(const char *txt, int min, int max)
{
	if(!engine)
		return strlen(txt);

	SoFont *f = engine->get_font(_font);
	if(!f)
		return strlen(txt);

	return (f->TextWidth(txt, min, max) << 16) / xs;
}


int window_t::textwidth(const char *txt, int min, int max)
{
	if(!engine)
		return strlen(txt);

	SoFont *f = engine->get_font(_font);
	if(!f)
		return strlen(txt);

	return (f->TextWidth(txt, min, max) << 8) / xs;
}


int window_t::fontheight()
{
	if(!engine)
		return 1;

	SoFont *f = engine->get_font(_font);
	if(!f)
		return 1;

	return (f->FontHeight() * 256) / ys;
}


void window_t::clear(SDL_Rect *r)
{
	SDL_Rect sr, dr;
	if(!engine)
		return;
	if(!surface)
		return;
	SELECT
	if(!r)
	{
		sr = phys_rect;
		sr.x = 0;
		sr.y = 0;
		dr = phys_rect;
	}
	else
	{
		sr.x = ((int)r->x * xs + 128) >> 8;
		sr.y = ((int)r->y * ys + 128) >> 8;
		sr.w = (((int)(r->x + r->w) * xs + 128) >> 8) - sr.x;
		sr.h = (((int)(r->y + r->h) * ys + 128) >> 8) - sr.y;
		dr = sr;
		dr.x += phys_rect.x;
		dr.y += phys_rect.y;
	}
	if((-1 == bg_bank) && (-1 == bg_frame))
		SDL_FillRect(surface, &dr, bgcolor);
	else
	{
		s_sprite_t *s = engine->get_sprite(bg_bank, bg_frame);
		if(!s || !s->surface)
		{
			SDL_FillRect(surface, &dr, bgcolor);
			return;
		}
		SDL_BlitSurface(s->surface, &sr, surface, &dr);
	}
}


void window_t::point(int _x, int _y)
{
	int x2 = ((_x + 1) * xs + 128) >> 8;
	int y2 = ((_y + 1) * ys + 128) >> 8;
	_x = (_x * xs + 128) >> 8;
	_y = (_y * ys + 128) >> 8;

	if(!engine)
		return;
	SELECT
	/* Quick hack; slow */
	SDL_Rect r;
	r.x = phys_rect.x + _x;
	r.y = phys_rect.y + _y;
	r.w = x2 - _x;
	r.h = y2 - _y;
	if(surface)
		SDL_FillRect(surface, &r, fgcolor);
}


void window_t::fillrect(int _x, int _y, int w, int h)
{
	int x2 = ((_x + w) * xs + 128) >> 8;
	int y2 = ((_y + h) * ys + 128) >> 8;
	_x = (_x * xs + 128) >> 8;
	_y = (_y * ys + 128) >> 8;

	if(!engine)
		return;
	SELECT
	SDL_Rect r;
	r.x = phys_rect.x + _x;
	r.y = phys_rect.y + _y;
	r.w = x2 - _x;
	r.h = y2 - _y;
	if(surface)
		SDL_FillRect(surface, &r, fgcolor);
}


void window_t::rectangle(int _x, int _y, int w, int h)
{
	fillrect(_x, _y, w, 1);
	fillrect(_x, _y + h - 1, w, 1);
	fillrect(_x, _y + 1, 1, h - 2);
	fillrect(_x + w - 1, _y + 1, 1, h - 2);
}


void window_t::fillrect_fxp(int _x, int _y, int w, int h)
{
	int xx = CS2PIXEL((_x * xs + 128) >> 8);
	int yy = CS2PIXEL((_y * ys + 128) >> 8);
	w = CS2PIXEL(((w + _x) * xs + 128) >> 8) - xx;
	h = CS2PIXEL(((h + _y) * ys + 128) >> 8) - yy;

	if(!engine)
		return;
	SELECT
	SDL_Rect r;
	r.x = phys_rect.x + xx;
	r.y = phys_rect.y + yy;
	r.w = w;
	r.h = h;
	if(surface)
		SDL_FillRect(surface, &r, fgcolor);
}


void window_t::sprite(int _x, int _y, int bank, int frame, int inval)
{
	sprite_fxp(PIXEL2CS(_x), PIXEL2CS(_y), bank, frame, inval);
}


void window_t::sprite_fxp(int _x, int _y, int bank, int frame, int inval)
{
	if(!engine)
		return;
	s_sprite_t *s = engine->get_sprite(bank, frame);
	if(!s || !s->surface)
		return;
	_x = CS2PIXEL(((_x - (s->x << 8)) * xs + 128) >> 8);
	_y = CS2PIXEL(((_y - (s->y << 8)) * ys + 128) >> 8);
	SDL_Rect dest_rect;

	SELECT
	dest_rect.x = phys_rect.x + _x;
	dest_rect.y = phys_rect.y + _y;
	if(surface)
		SDL_BlitSurface(s->surface, NULL, surface, &dest_rect);

	if(inval && !engine->autoinvalidate())
	{
		dest_rect.w = s->surface->w;
		dest_rect.h = s->surface->h;
		engine->invalidate(&dest_rect, this);
	}
}


void window_t::blit(int dx, int dy,
		int sx, int sy, int sw, int sh, window_t *src)
{
	if(!engine)
		return;
	if(!src)
		return;
	if(!surface)
		return;
	if(!src->surface)
		return;
	SELECT

	SDL_Rect src_rect;
	int sx2 = ((sx + sw) * xs + 128) >> 8;
	int sy2 = ((sy + sh) * ys + 128) >> 8;
	src_rect.x = (sx * xs + 128) >> 8;
	src_rect.y = (sy * ys + 128) >> 8;
	src_rect.w = sx2 - src_rect.x;
	src_rect.h = sy2 - src_rect.y;

	SDL_Rect dest_rect;
	dest_rect.x = phys_rect.x + ((dx * xs + 128) >> 8);
	dest_rect.y = phys_rect.y + ((dy * ys + 128) >> 8);

	SDL_BlitSurface(src->surface, &src_rect, surface, &dest_rect);
}


void window_t::blit(int dx, int dy, window_t *src)
{
	if(!engine)
		return;
	if(!src)
		return;
	if(!surface)
		return;
	if(!src->surface)
		return;

	SELECT
	dx = (dx * xs + 128) >> 8;
	dy = (dy * ys + 128) >> 8;

	SDL_Rect src_rect;
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.w = src->surface->w;
	src_rect.h = src->surface->h;

	SDL_Rect dest_rect;
	dest_rect.x = phys_rect.x + dx;
	dest_rect.y = phys_rect.y + dy;

	SDL_BlitSurface(src->surface, &src_rect, surface, &dest_rect);
}
