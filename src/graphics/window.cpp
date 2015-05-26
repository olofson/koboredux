/*(LGPLv2.1)
---------------------------------------------------------------------------
	window.cpp - Generic Rendering Window
---------------------------------------------------------------------------
 * Copyright 2001-2003, 2006-2007, 2009 David Olofson
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

#include "logger.h"
#include "window.h"
#include "gfxengine.h"
#include "sofont.h"

#define	SELECT	if(engine->selected != this) select();


enum gfx_offscreen_mode_t
{
	OFFSCREEN_RENDER_TARGET =	1,
	OFFSCREEN_SOFTWARE =		2
};


  /////////////////////////////////////////////////////////////////////////////
 // Engine window base class
/////////////////////////////////////////////////////////////////////////////

windowbase_t::windowbase_t(gfxengine_t *e)
{
	next = NULL;
	prev = NULL;
	engine = NULL;
	renderer = NULL;
	_visible = 1;
	link(e);
	xs = engine->xs;
	ys = engine->ys;
	fgcolor = bgcolor = 0;
	_alphamod = 255;
	_colormod = 0xffffffff;
}


windowbase_t::~windowbase_t()
{
	if(engine && (engine->selected == this))
		engine->selected = NULL;
	unlink();
	if(renderer)
		SDL_DestroyRenderer(renderer);
}


void windowbase_t::link(gfxengine_t *e)
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
	if(!renderer)
		renderer = engine->renderer();
}


void windowbase_t::unlink(void)
{
	if(engine)
	{
		if(engine->windows == this)
			engine->windows = next;
		if(engine->renderer() == renderer)
			renderer = NULL;
	}
	if(next)
		next->prev = prev;
	if(prev)
		prev->next = next;
}


void windowbase_t::visible(int vis)
{
	_visible = vis;
}


void windowbase_t::place(int left, int top, int sizex, int sizey)
{
	int x2 = ((left + sizex) * xs + 128) >> 8;
	int y2 = ((top + sizey) * ys + 128) >> 8;
	phys_rect.x = (left * xs + 128) >> 8;
	phys_rect.y = (top * ys + 128) >> 8;
	phys_rect.w = x2 - phys_rect.x;
	phys_rect.h = y2 - phys_rect.y;
}


void windowbase_t::scale(float x, float y)
{
	if(x > 0)
		xs = (int)(x * 256.f);
	else
		xs = (int)((-x) * engine->xs);
	if(y > 0)
		ys = (int)(y * 256.f);
	else
		ys = (int)((-y) * engine->ys);
}


  /////////////////////////////////////////////////////////////////////////////
 // Normal or offscreen window
/////////////////////////////////////////////////////////////////////////////

window_t::window_t(gfxengine_t *e) : windowbase_t(e)
{
	phys_rect.x = phys_rect.y = 0;
	phys_rect.w = 320;
	phys_rect.h = 240;
	otexture = NULL;
	osurface = NULL;
	bg_bank = -1;
	bg_frame = -1;
	_font = 0;
	_visible = 1;
	_offscreen = 0;
}


window_t::~window_t()
{
	if(otexture)
		SDL_DestroyTexture(otexture);
	if(osurface)
		SDL_FreeSurface(osurface);
}


void window_t::visible(int vis)
{
	if(_offscreen)
		return;		// Cannot be visible!
	_visible = vis;
}


void window_t::place(int left, int top, int sizex, int sizey)
{
	if(_offscreen)
		left = top = 0;
	windowbase_t::place(left, top, sizex, sizey);
}


void window_t::select()
{
	if(!engine)
		return;
	if(!renderer)
		return;
	switch(_offscreen)
	{
	  case 0:
		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderSetClipRect(renderer, &phys_rect);
		break;
	  case OFFSCREEN_RENDER_TARGET:
		SDL_SetRenderTarget(renderer, otexture);
		SDL_RenderSetClipRect(renderer, NULL);
		break;
	  case OFFSCREEN_SOFTWARE:
		// Has its own renderer, so nothing needs to be done here!
		break;
	}
	engine->selected = this;
}


int window_t::offscreen()
{
	if(!engine)
		return -1;
	if(_offscreen)
		return 0;	// Already offscreen!
	visible(0);
	phys_rect.x = phys_rect.y = 0;
#if 0
	/*
	 * This has some serious performance issues for some reason, and
	 * shouldn't really be needed anyway.
	 */
	if(SDL_RenderTargetSupported(engine->renderer()))
	{
		// Texture used as render target
		otexture = SDL_CreateTexture(engine->renderer(),
				SDL_PIXELFORMAT_ARGB8888,
				SDL_TEXTUREACCESS_TARGET,
				phys_rect.w, phys_rect.h);
		if(!otexture)
			return -1;
		_offscreen = OFFSCREEN_RENDER_TARGET;
	}
	else
#endif
	{
		// Fallback: Texture + surface with software renderer
		Uint32 fmt = SDL_PIXELFORMAT_ARGB8888;
		int bpp;
		Uint32 Rmask, Gmask, Bmask, Amask;
		SDL_PixelFormatEnumToMasks(fmt,
				&bpp, &Rmask, &Gmask, &Bmask, &Amask);
		osurface = SDL_CreateRGBSurface(0,
				phys_rect.w, phys_rect.h,
				bpp, Rmask, Gmask, Bmask, Amask);
		if(!osurface)
			return -10;
		renderer = SDL_CreateSoftwareRenderer(osurface);
		if(!renderer)
			return -11;
		otexture = SDL_CreateTexture(engine->renderer(),
				fmt, SDL_TEXTUREACCESS_STREAMING,
				phys_rect.w, phys_rect.h);
		if(!otexture)
			return -12;
		_offscreen = OFFSCREEN_SOFTWARE;
	}
	return 0;
}


void window_t::offscreen_invalidate(SDL_Rect *r)
{
	switch(_offscreen)
	{
	  case OFFSCREEN_RENDER_TARGET:
		SDL_RenderPresent(renderer);
		break;
	  case OFFSCREEN_SOFTWARE:
		SDL_RenderPresent(renderer);
		SDL_Rect rr;
		if(!r)
		{
			rr.x = 0;
			rr.y = 0;
			rr.w = phys_rect.w;
			rr.h = phys_rect.h;
		}
		else
		{
			rr.x = (int)r->x * xs >> 8;
			rr.y = (int)r->y * ys >> 8;
			int x2 = (int)(r->x + r->w) * xs >> 8;
			int y2 = (int)(r->y + r->h) * ys >> 8;
			rr.w = x2 - rr.x;
			rr.h = y2 - rr.y;
		}
		// FIXME: Is this *actually* slower than locking the texture
		// and copying the pixels? Theoretically, SDL_UpdateTexture()
		// should have a chance of doing a better job.
		SDL_UpdateTexture(otexture, &rr,
				(Uint8 *)osurface->pixels +
				rr.y * osurface->pitch +
				rr.x * osurface->format->BytesPerPixel,
				osurface->pitch);
		break;
	}
}


void window_t::invalidate(SDL_Rect *r)
{
	if(!engine || !renderer || !_offscreen)
		return;
	SELECT
	refresh(r);
	offscreen_invalidate(r);
}


/*---------------------------------------------------------------
	Rendering API
---------------------------------------------------------------*/

void window_t::bgimage(int bank, int frame)
{
	bg_bank = bank;
	bg_frame = frame;
}


void window_t::colorkey(Uint32 color)
{
#if 0
	if(!engine)
		return;
	if(!engine->surface())
		return;
	if(!_offscreen)
		return;
	SDL_SetColorKey(surface, SDL_SRCCOLORKEY, color);
#endif
}

void window_t::colorkey()
{
#if 0
	if(!engine)
		return;
	if(!engine->surface())
		return;
	if(!_offscreen)
		return;
	SDL_SetColorKey(surface, 0, 0);
#endif
}

void window_t::alpha(float a)
{
#if 0
	if(!engine)
		return;
	if(!engine->surface())
		return;
	if(!_offscreen)
		return;
	SDL_SetAlpha(surface, SDL_SRCALPHA, (int)(a * 255.0));
#endif
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
	f->PutString(_x, _y, txt);
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
	f->PutString(_x, _y, txt);
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
	f->PutString(_cx, _y, txt);
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
	if(!engine || !renderer)
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
	s_sprite_t *s = NULL;
	if((-1 != bg_bank) && (-1 != bg_frame))
	{
		s = engine->get_sprite(bg_bank, bg_frame);
		if(s && !s->texture)
			s = NULL;
	}
	if(s)
	{
		SDL_RenderCopy(renderer, s->texture, &sr, &dr);
	}
	else
	{
		SDL_SetRenderDrawColor(renderer,
				get_r(bgcolor), get_g(bgcolor),
				get_b(bgcolor), get_a(bgcolor));
		SDL_RenderFillRect(renderer, &dr);
	}
}


void window_t::point(int _x, int _y)
{
	if(!engine || !renderer)
		return;
	if((_offscreen == OFFSCREEN_SOFTWARE) && (xs == 256) && (ys == 256))
	{
		Uint32 *p;
		if((_x < 0) || (_x >= osurface->w) || (_y < 0) ||
				(_y >= osurface->h))
			return;
		p = (Uint32 *)((char *)osurface->pixels + _y *
				osurface->pitch);
		p[_x] = fgcolor;
	}
	else
	{
		SELECT
		SDL_Rect r;
		int x2 = ((_x + 1) * xs + 128) >> 8;
		int y2 = ((_y + 1) * ys + 128) >> 8;
		_x = (_x * xs + 128) >> 8;
		_y = (_y * ys + 128) >> 8;
		r.x = phys_rect.x + _x;
		r.y = phys_rect.y + _y;
		r.w = x2 - _x;
		r.h = y2 - _y;
		SDL_SetRenderDrawColor(renderer, get_r(fgcolor),
				get_g(fgcolor), get_b(fgcolor),
				get_a(fgcolor));
		SDL_RenderFillRect(renderer, &r);
	}
}


void window_t::fillrect(int _x, int _y, int w, int h)
{
	int x2 = ((_x + w) * xs + 128) >> 8;
	int y2 = ((_y + h) * ys + 128) >> 8;
	_x = (_x * xs + 128) >> 8;
	_y = (_y * ys + 128) >> 8;
	if(!engine || !renderer)
		return;
	SELECT
	SDL_Rect r;
	r.x = phys_rect.x + _x;
	r.y = phys_rect.y + _y;
	r.w = x2 - _x;
	r.h = y2 - _y;
	SDL_SetRenderDrawColor(renderer, get_r(fgcolor), get_g(fgcolor),
			get_b(fgcolor), get_a(fgcolor));
	SDL_RenderFillRect(renderer, &r);
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
	if(!engine || !renderer)
		return;
	SELECT
	SDL_Rect r;
	r.x = phys_rect.x + xx;
	r.y = phys_rect.y + yy;
	r.w = w;
	r.h = h;
	SDL_SetRenderDrawColor(renderer, get_r(fgcolor), get_g(fgcolor),
			get_b(fgcolor), get_a(fgcolor));
	SDL_RenderFillRect(renderer, &r);
}


void window_t::sprite(int _x, int _y, int bank, int frame)
{
	sprite_fxp(PIXEL2CS(_x), PIXEL2CS(_y), bank, frame);
}


void window_t::sprite_fxp(int _x, int _y, int bank, int frame)
{
	if(!engine || !renderer)
		return;
	s_bank_t *b = s_get_bank(gfxengine->get_gfx(), bank);
	if(!b)
		return;
	s_sprite_t *s = s_get_sprite_b(b, frame);
	if(!s || !s->texture)
		return;
	_x = CS2PIXEL(((_x - (s->x << 8)) * xs + 128) >> 8);
	_y = CS2PIXEL(((_y - (s->y << 8)) * ys + 128) >> 8);
	SDL_Rect r;

	SELECT
	r.x = phys_rect.x + _x;
	r.y = phys_rect.y + _y;
	r.w = b->w;
	r.h = b->h;
	SDL_SetTextureAlphaMod(s->texture, _alphamod);
	SDL_SetTextureColorMod(s->texture,
			get_r(_colormod), get_g(_colormod), get_b(_colormod));
	SDL_RenderCopy(renderer, s->texture, NULL, &r);
}


void window_t::sprite_fxp_scale(int _x, int _y, int bank, int frame,
		float xscale, float yscale)
{
	if(!engine || !renderer)
		return;
	s_bank_t *b = s_get_bank(gfxengine->get_gfx(), bank);
	if(!b)
		return;
	s_sprite_t *s = s_get_sprite_b(b, frame);
	if(!s || !s->texture)
		return;
	_x = CS2PIXEL(((_x - (s->x << 8)) * xs + 128) >> 8);
	_y = CS2PIXEL(((_y - (s->y << 8)) * ys + 128) >> 8);
	SDL_Rect r;

	SELECT
	r.x = phys_rect.x + _x;
	r.y = phys_rect.y + _y;
	r.w = b->w * xscale;
	r.h = b->h * yscale;
	SDL_SetTextureAlphaMod(s->texture, _alphamod);
	SDL_SetTextureColorMod(s->texture,
			get_r(_colormod), get_g(_colormod), get_b(_colormod));
	SDL_RenderCopy(renderer, s->texture, NULL, &r);
}


void window_t::blit(int dx, int dy,
		int sx, int sy, int sw, int sh, window_t *src)
{
	if(!engine || !src || !renderer || !src->otexture)
		return;

	SELECT
	SDL_Rect src_rect;
	int sx2 = ((sx + sw) * src->xs + 128) >> 8;
	int sy2 = ((sy + sh) * src->ys + 128) >> 8;
	src_rect.x = (sx * src->xs + 128) >> 8;
	src_rect.y = (sy * src->ys + 128) >> 8;
	src_rect.w = sx2 - src_rect.x;
	src_rect.h = sy2 - src_rect.y;

	SDL_Rect dest_rect;
	dest_rect.x = phys_rect.x + ((dx * xs + 128) >> 8);
	dest_rect.y = phys_rect.y + ((dy * ys + 128) >> 8);
	dest_rect.w = src_rect.w * xs / src->xs;
	dest_rect.h = src_rect.h * ys / src->ys;

	SDL_RenderCopy(renderer, src->otexture, &src_rect, &dest_rect);
}


void window_t::blit(int dx, int dy, window_t *src)
{
	if(!engine || !src || !renderer || !src->otexture)
		return;

	SELECT
	SDL_Rect src_rect;
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.w = src->phys_rect.w;
	src_rect.h = src->phys_rect.h;

	SDL_Rect dest_rect;
	dest_rect.x = phys_rect.x + ((dx * xs + 128) >> 8);
	dest_rect.y = phys_rect.y + ((dy * ys + 128) >> 8);
	dest_rect.w = src_rect.w * xs / src->xs;
	dest_rect.h = src_rect.h * ys / src->ys;

	SDL_RenderCopy(renderer, src->otexture, &src_rect, &dest_rect);
}


  /////////////////////////////////////////////////////////////////////////////
 // Engine output window
/////////////////////////////////////////////////////////////////////////////

void engine_window_t::refresh(SDL_Rect *r)
{
	engine->wx = x();
	engine->wy = y();
	engine->pre_render();
	select();
	cs_engine_render(engine->csengine);
	engine->post_render();
}
