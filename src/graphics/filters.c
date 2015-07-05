/*(LGPLv2.1)
----------------------------------------------------------------------
	filters.c - Filters for the sprite manager
----------------------------------------------------------------------
 * Copyright 2001, 2003, 2006, 2007, 2009 David Olofson
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

#include <string.h>
#include <math.h>
#include "config.h"
#include "logger.h"
#include "SDL.h"
#include "SDL_image.h"
#include "sprite.h"
#include "filters.h"


/*--------------------------------------------------
	RNG
--------------------------------------------------*/

static unsigned int rndstate = 16576;

/* Resets the noise generator */
static void noise_reset(int seed)
{
	rndstate = 16576 + seed;
}

/* Returns a pseudo random number in the range [0, 65535] */
static inline int noise(void)
{
	rndstate *= 1566083941UL;
	rndstate++;
	rndstate &= 0xffffffffUL;	/* NOP on 32 bit machines */
	return (int)(rndstate * (rndstate >> 16) >> 16);
}


/*--------------------------------------------------
	Pixel access ops for 32 bit RGBA
--------------------------------------------------*/

static pix_t getpix32_empty = {0x7f, 0x7f, 0x7f, 0x7f};

static inline pix_t getpix32(SDL_Surface *s, int x, int y)
{
	pix_t	*p;
	if((x < 0) || (x >= s->w) || (y < 0) || (y >= s->h))
		return getpix32_empty;

	p = (pix_t *)((char *)s->pixels + y * s->pitch);
	return p[x];
}


/* Clamping version */
static inline pix_t getpix32c(SDL_Surface *s, int x, int y)
{
	pix_t	*p;
	if(x < 0)
		x = 0;
	else if(x >= s->w)
		x = s->w - 1;
	if(y < 0)
		y = 0;
	else if(y >= s->h)
		y = s->h - 1;
	p = (pix_t *)((char *)s->pixels + y * s->pitch);
	return p[x];
}

/* Wrapping version */
static inline pix_t getpix32w(SDL_Surface *s, int x, int y)
{
	pix_t	*p;
	if(x < 0)
		x += s->w;
	else if(x >= s->w)
		x -= s->w;
	if(y < 0)
		y += s->h;
	else if(y >= s->h)
		y -= s->h;
	p = (pix_t *)((char *)s->pixels + y * s->pitch);
	return p[x];
}

#define	GETPIXI(gp)			\
{					\
	int c0x, c0y, c1x, c1y;		\
	int c[4];			\
	pix_t e = {0, 0, 0, 0};		\
	pix_t p[4], r;			\
	getpix32_empty = e;		\
					\
	/* Calculate filter core */	\
	x -= 8;				\
	y -= 8;				\
	c1x = x & 0xf;			\
	c1y = y & 0xf;			\
	c0x = 16 - c1x;			\
	c0y = 16 - c1y;			\
	c[0] = c0x * c0y;		\
	c[1] = c1x * c0y;		\
	c[2] = c0x * c1y;		\
	c[3] = c1x * c1y;		\
					\
	/* Grab input pixels */		\
	x >>= 4;			\
	y >>= 4;			\
	p[0] = gp(s, x, y);		\
	p[1] = gp(s, x+1, y);		\
	p[2] = gp(s, x, y+1);		\
	p[3] = gp(s, x+1, y+1);		\
					\
	/* Interpolate... */		\
	r.r = (p[0].r*c[0] + p[1].r*c[1] + p[2].r*c[2] + p[3].r*c[3])>>8; \
	r.g = (p[0].g*c[0] + p[1].g*c[1] + p[2].g*c[2] + p[3].g*c[3])>>8; \
	r.b = (p[0].b*c[0] + p[1].b*c[1] + p[2].b*c[2] + p[3].b*c[3])>>8; \
	r.a = (p[0].a*c[0] + p[1].a*c[1] + p[2].a*c[2] + p[3].a*c[3])>>8; \
	return r;			\
}

/* Interpolated; 28:4 fixed point coords. */
static inline pix_t getpix32i(SDL_Surface *s, int x, int y)
{
	GETPIXI(getpix32)
}


/* Clamping version */
static inline pix_t getpix32ic(SDL_Surface *s, int x, int y)
{
	GETPIXI(getpix32c)
}


/* Wrapping version */
static inline pix_t getpix32iw(SDL_Surface *s, int x, int y)
{
	GETPIXI(getpix32w)
}


static inline pix_t *pix32(SDL_Surface *s, int x, int y)
{
	static pix_t dummy = {0x7f, 0x7f, 0x7f, 0x7f};
	pix_t *p;
	if((x < 0) || (x >= s->w) || (y < 0) || (y >= s->h))
		return &dummy;
	p = (pix_t *)((char *)s->pixels + y * s->pitch);
	return p + x;
}


static inline void setpix32(SDL_Surface *s, int x, int y, pix_t pix)
{
	pix_t *p;
	if((x < 0) || (x >= s->w) || (y < 0) || (y >= s->h))
		return;
	p = (pix_t *)((char *)s->pixels + y * s->pitch);
	p[x] = pix;
}


/*--------------------------------------------------
	Non-clipping versions of everything
	(No clamping here, obviously...!)
--------------------------------------------------*/

static inline pix_t getpix32_nc(SDL_Surface *s, int x, int y)
{
	pix_t *p = (pix_t *)((char *)s->pixels + y * s->pitch);
#ifdef DEBUG
	if((x < 0) || (x >= s->w) || (y < 0) || (y >= s->h))
	{
		log_printf(ELOG, "Clip in getpix32_nc()!"
				" x = %d, y = %d\n", x, y);
*((char *)NULL) = 0;
		return getpix32_empty;
	}
#endif
	return p[x];
}


/* Interpolated; 28:4 fixed point coords. */
static inline pix_t getpix32i_nc(SDL_Surface *s, int x, int y)
{
#ifdef DEBUG
	if((x < 0) || (x >= s->w << 4) || (y < 0) || (y >= s->h << 4))
	{
		log_printf(ELOG, "Clip in getpix32i_nc()!"
				" x = %d, y = %d\n", x>>4, y>>4);
*((char *)NULL) = 0;
//		return getpix32_empty;
	}
	GETPIXI(getpix32)
#else
	GETPIXI(getpix32_nc)
#endif
}
#undef GETPIXI

static inline void setpix32_nc(SDL_Surface *s, int x, int y, pix_t pix)
{
	pix_t *p = (pix_t *)((char *)s->pixels + y * s->pitch);
#ifdef DEBUG
	if((x < 0) || (x >= s->w) || (y < 0) || (y >= s->h))
	{
		log_printf(ELOG, "Clip in setpix32_nc()!"
				" x = %d, y = %d\n", x, y);
*((char *)NULL) = 0;
//		return;
	}
#endif
	p[x] = pix;
}


static inline pix_t *pix32_nc(SDL_Surface *s, int x, int y)
{
	pix_t *p = (pix_t *)((char *)s->pixels + y * s->pitch);
#ifdef DEBUG
//	static pix_t trash;
	if((x < 0) || (x >= s->w) || (y < 0) || (y >= s->h))
	{
		log_printf(ELOG, "Clip in pix32_nc()!"
				" x = %d, y = %d\n", x, y);
*((char *)NULL) = 0;
//		return &trash;
	}
#endif
	return p + x;
}



/*--------------------------------------------------
	The filters
--------------------------------------------------*/

int s_filter_rgba8(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args)
{
	unsigned i;
	for(i = 0; i < frames; ++i)
	{
		SDL_Surface *tmp;
		s_sprite_t *s = s_get_sprite_b(b, first+i);
		if(!s)
			continue;
		tmp = SDL_ConvertSurfaceFormat(s->surface, KOBO_PIXELFORMAT,
				SDL_SWSURFACE);
		if(!tmp)
			return -1;

		SDL_FreeSurface(s->surface);
		s->surface = tmp;
	}
	return 0;
}


int s_filter_dither(s_bank_t *bank, unsigned first, unsigned frames,
		s_filter_args_t *args)
{
	int x, y;
	unsigned i;
	int ar, ag, ab;
	if(!args->x && !args->r && !args->g && !args->b)
		return 0;
	noise_reset(first);
	for(i = 0; i < frames; ++i)
	{
		s_sprite_t *s = s_get_sprite_b(bank, first+i);
		if(!s)
			continue;
		ar = args->r;
		ag = args->g;
		ab = args->b;
		if(args->x)
			switch (s_blitmode)
			{
			  case S_BLITMODE_AUTO:
				if(s->surface->format->Amask)
					ar = ag = ab = 8;
				break;
			  case S_BLITMODE_OPAQUE:
				break;
			  case S_BLITMODE_COLORKEY:
			  case S_BLITMODE_ALPHA:
				ar = ag = ab = 8;
				break;
			}
		switch(args->y)
		{
		  default:
		  case 0:
		  case 1:
			ar >>= 1;
			ag >>= 1;
			ab >>= 1;
		  case 2:
			break;
		}
		for(y = (args->flags & SF_CLAMP_SFONT) ? 1 : 0; y < bank->h;
				++y)
		{
			pix_t *p = (pix_t *)((char *)s->surface->pixels +
					y * s->surface->pitch);
			switch(args->y)
			{
			  default:
			  case 0:
				/* 2x2 filter */
				for(x = 0; x < bank->w; ++x)
				{
					int r, g, b;
					pix_t *pix = p + x;
					if((x^y)&1)
					{
						r = pix->r + ar;
						g = pix->g + ag;
						b = pix->b + ab;
						if(r>255)
							r = 255;
						if(g>255)
							g = 255;
						if(b>255)
							b = 255;
					}
					else
					{
						r = pix->r - ar;
						g = pix->g - ag;
						b = pix->b - ab;
						if(r<0)
							r = 0;
						if(g<0)
							g = 0;
						if(b<0)
							b = 0;
					}
					pix->r = r;
					pix->g = g;
					pix->b = b;
				}
				break;
			  case 1:
				/* 4x4 filter */
				for(x = 0; x < bank->w; ++x)
				{
					int dr, dg, db;
					int r, g, b;
					pix_t *pix = p + x;
					r = pix->r;
					g = pix->g;
					b = pix->b;
					if((x^y) & 1)
					{
						dr = ar;
						dg = ag;
						db = ab;
					}
					else
					{
						dr = -ar;
						dg = -ag;
						db = -ab;
					}
					if(y & 1)
					{
						dr += ar >> 1;
						dg += ag >> 1;
						db += ab >> 1;
					}
					else
					{
						dr -= ar >> 1;
						dg -= ag >> 1;
						db -= ab >> 1;
					}
					r += dr;
					g += dg;
					b += db;

					if(r>255)
						r = 255;
					else if(r<0)
						r = 0;

					if(g>255)
						g = 255;
					else if(g<0)
						g = 0;

					if(b>255)
						b = 255;
					else if(b<0)
						b = 0;

					pix->r = r;
					pix->g = g;
					pix->b = b;
				}
				break;
			  case 2:
				/* Random */
				for(x = 0; x < bank->w; ++x)
				{
					int r, g, b, z;
					pix_t *pix = p + x;
					z = noise();
					r = pix->r - ar + (((ar << 1) - 1) & z);
					g = pix->g - ag + (((ag << 1) - 1) & z);
					b = pix->b - ab + (((ab << 1) - 1) & z);

					if(r>255)
						r = 255;
					else if(r<0)
						r = 0;

					if(g>255)
						g = 255;
					else if(g<0)
						g = 0;

					if(b>255)
						b = 255;
					else if(b<0)
						b = 0;

					pix->r = r;
					pix->g = g;
					pix->b = b;
				}
				break;
			}
		}
	}
	return 0;
}

#if 0
/*
 * Ugly hack to get around the problems caused by
 * SDL_SetColorKey() ignoring the alpha channel.
 * Basically just sets the blue LSB of any pixels
 * that match the colorkey, but have non-transparent
 * alpha.
 */
static void tweak_ck(SDL_Surface *s)
{
	int x, y;
	if(s->format->BitsPerPixel != 32)
		return;
	for(y = 0; y < s->h; ++y)
	{
		pix_t *p = (pix_t *)((char *)s->pixels + y * s->pitch);
		for(x = 0; x < s->w; ++x)
		{
			pix_t *pix = p + x;
			if(!pix->r && !pix->g && !pix->b)
				if(pix->a)
					pix->b |= 1;
		}
	}
}
#endif

int s_filter_displayformat(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args)
{
	unsigned i;
	if(!args->data)
		return 0;	/* No renderer! */
	for(i = 0; i < frames; ++i)
	{
#if 0
		SDL_Surface *tmp;
#endif
		s_sprite_t *s = s_get_sprite_b(b, first + i);
		if(!s)
			continue;
		s->texture = SDL_CreateTextureFromSurface(
				(SDL_Renderer *)args->data,
				s->surface);
#if 0
		SDL_FreeSurface(s->surface);
		s->surface = NULL;
#endif
#if 0
		switch(s_blitmode)
		{
		  case S_BLITMODE_AUTO:
			if(s->surface->format->Amask)
				SDL_SetAlpha(s->surface,
						SDL_SRCALPHA |
						SDL_RLEACCEL,
						SDL_ALPHA_OPAQUE);
			else
				SDL_SetColorKey(s->surface, SDL_TRUE, 0);
			break;
		  case S_BLITMODE_OPAQUE:
			SDL_SetColorKey(s->surface, SDL_RLEACCEL, 0);
			break;
		  case S_BLITMODE_COLORKEY:
			SDL_SetColorKey(s->surface,
					SDL_SRCCOLORKEY | SDL_RLEACCEL,
					SDL_MapRGB(s->surface->format,
						s_colorkey.r,
						s_colorkey.g,
						s_colorkey.b));
			break;
		  case S_BLITMODE_ALPHA:
			SDL_SetAlpha(s->surface,
					SDL_SRCALPHA | SDL_RLEACCEL,
					s_alpha);
			break;
		}

		if(args->x)
		{
			if(s->surface->format->Amask)
				tweak_ck(s->surface);
			tmp = SDL_DisplayFormat(s->surface);
			if(s->surface->format->Amask)
				SDL_SetColorKey(tmp,
						SDL_SRCCOLORKEY | SDL_RLEACCEL,
						SDL_MapRGB(tmp->format, 0,0,0));
		}
		else
		{
			if(s->surface->format->Amask)
				tmp = SDL_DisplayFormatAlpha(s->surface);
			else
				tmp = SDL_DisplayFormat(s->surface);
		}
		if(!tmp)
			return -1;

		SDL_FreeSurface(s->surface);
		s->surface = tmp;
#endif
	}
	return 0;
}


typedef struct
{
	SDL_Surface *src;	/* Source surface */
	SDL_Surface *dst;	/* Destination surface */
	int scx, scy;		/* Scale (fixp 16:16) */
	int start_sy, start_sx;	/* Src start pos (fixp 16:16) */
	int start_x, start_y;	/* Dst start pos (pixels) */
	int max_x, max_y;	/* Dst end pos (pixels) */
} scale_params_t;


static inline void select_rect(scale_params_t *p, scale_params_t *orig,
		int x, int y, int w, int h)
{
	p->start_sx = orig->start_sx + x * orig->scx;
	p->start_sy = orig->start_sy + y * orig->scy;
	p->start_x = orig->start_x + x;
	p->start_y = orig->start_y + y;
	p->max_x = p->start_x + w;
	p->max_y = p->start_y + h;
}

/* Optimization tool; use the slow clipping funcs only around the edges! */
static inline void do_scale(scale_params_t *p,
		void (*border)(scale_params_t *p),
		void (*body)(scale_params_t *p))
{
	/* Copy the parameters */
	scale_params_t lp = *p;
	int w = p->max_x - p->start_x;
	int h = p->max_y - p->start_y;
	int bx = (65535 + p->scx) / p->scx;
	int by = (65535 + p->scy) / p->scy;
	if(bx * 2 >= w || by * 2 >= h)
	{
		/* Area too small - body size would be zero or negative! */
		select_rect(&lp, p, 0, 0, w, h);
		border(&lp);
		return;
	}

	/* Top border */
	select_rect(&lp, p, 0, 0, w, by);
	border(&lp);

	/* Bottom border */
	select_rect(&lp, p, 0, h - by, w, by);
	border(&lp);

	/* Left border */
	select_rect(&lp, p, 0, by, bx, h - 2 * by);
	border(&lp);

	/* Right border */
	select_rect(&lp, p, w - bx, by, bx, h - 2 * by);
	border(&lp);

	/* Body */
	select_rect(&lp, p, bx, by, w - 2 * bx, h - 2 * by);
	body(&lp);
}


/*
 * Ugly but fast "nearest pixel" scaling
 */

static void scale_nearest(scale_params_t *p)
{
	int x, y, sx, sy;
	for(y = p->start_y, sy = p->start_sy + p->scy / 2 - 1;
			y < p->max_y; ++y, sy += p->scy)
	{
		pix_t *pix = (pix_t *)((char *)p->dst->pixels +
				y * p->dst->pitch);
		for(x = p->start_x, sx = p->start_sx + p->scx / 2 - 1;
				x < p->max_x; ++x, sx += p->scx)
			pix[x] = getpix32_nc(p->src, sx >> 16, sy >> 16);
	}
}


/*
 * Bilinear scaling
 */

static void do_scale_bilinear_noclip(scale_params_t *p)
{
	int x, y, sx, sy;
	for(y = p->start_y, sy = p->start_sy; y < p->max_y; ++y, sy += p->scy)
	{
		pix_t *pix = (pix_t *)((char *)p->dst->pixels +
				y * p->dst->pitch);
		for(x = p->start_x, sx = p->start_sx; x < p->max_x;
				++x, sx += p->scx)
			pix[x] = getpix32i_nc(p->src, sx >> 12, sy >> 12);
	}
}

static void do_scale_bilinear_clip(scale_params_t *p)
{
	int x, y, sx, sy;
	for(y = p->start_y, sy = p->start_sy; y < p->max_y; ++y, sy += p->scy)
	{
		pix_t *pix = (pix_t *)((char *)p->dst->pixels +
				y * p->dst->pitch);
		for(x = p->start_x, sx = p->start_sx; x < p->max_x;
				++x, sx += p->scx)
			pix[x] = getpix32i(p->src, sx >> 12, sy >> 12);
	}
}

static void do_scale_bilinear_clamp(scale_params_t *p)
{
	int x, y, sx, sy;
	for(y = p->start_y, sy = p->start_sy; y < p->max_y; ++y, sy += p->scy)
	{
		pix_t *pix = (pix_t *)((char *)p->dst->pixels +
				y * p->dst->pitch);
		for(x = p->start_x, sx = p->start_sx; x < p->max_x;
				++x, sx += p->scx)
			pix[x] = getpix32ic(p->src, sx >> 12, sy >> 12);
	}
}

static void do_scale_bilinear_wrap(scale_params_t *p)
{
	int x, y, sx, sy;
	for(y = p->start_y, sy = p->start_sy; y < p->max_y; ++y, sy += p->scy)
	{
		pix_t *pix = (pix_t *)((char *)p->dst->pixels +
				y * p->dst->pitch);
		for(x = p->start_x, sx = p->start_sx; x < p->max_x;
				++x, sx += p->scx)
			pix[x] = getpix32iw(p->src, sx >> 12, sy >> 12);
	}
}

static void scale_bilinear(scale_params_t *p)
{
	do_scale(p, do_scale_bilinear_clip, do_scale_bilinear_noclip);
}

static void scale_bilinear_clamp(scale_params_t *p)
{
	do_scale(p, do_scale_bilinear_clamp, do_scale_bilinear_noclip);
}

static void scale_bilinear_wrap(scale_params_t *p)
{
	do_scale(p, do_scale_bilinear_wrap, do_scale_bilinear_noclip);
}


/*
 * Scale2x scaling
 *
 * This one's cool for 16 color graphics and the like, but
 * even with the fuzziness factor, it doesn't work well
 * with 24 bit data, or even 256 color graphics.
 */
#define	CDIFF(x,y) (((x).r^(y).r) | ((x).g^(y).g) | ((x).b^(y).b) | ((x).a^(y).a))
#define	SCALE2X(gp, sp)			\
{					\
	int DB, BF, DH, FH;		\
	pix_t	   B   ;		\
	pix_t	D, E, F;		\
	pix_t	   H   ;		\
	pix_t	E0, E1;			\
	pix_t	E2, E3;			\
	B = gp(p->src, sx, sy-1);	\
	D = gp(p->src, sx-1, sy);	\
	E = gp(p->src, sx, sy);		\
	F = gp(p->src, sx+1, sy);	\
	H = gp(p->src, sx, sy+1);	\
	DB = CDIFF(D, B) & 0xc0;	\
	BF = CDIFF(B, F) & 0xc0;	\
	DH = CDIFF(D, H) & 0xc0;	\
	FH = CDIFF(F, H) & 0xc0;	\
	E0 = (!DB && BF && DH) ? D : E;	\
	E1 = (!BF && DB && FH) ? F : E;	\
	E2 = (!DH && DB && FH) ? D : E;	\
	E3 = (!FH && DH && BF) ? F : E;	\
	sp(p->dst, x, y, E0);		\
	sp(p->dst, x+1, y, E1);		\
	sp(p->dst, x, y+1, E2);		\
	sp(p->dst, x+1, y+1, E3);	\
}
static void do_scale_2x_noclip(scale_params_t *p)
{
	int x, y, sx, sy;
	for(y = p->start_y, sy = p->start_sy >> 16; y < p->max_y; y += 2, ++sy)
		for(x = p->start_x, sx = p->start_sx >> 16; x < p->max_x;
				x += 2, ++sx)
			SCALE2X(getpix32_nc, setpix32_nc);
}

static void do_scale_2x_clip(scale_params_t *p)
{
	int x, y, sx, sy;
	for(y = p->start_y, sy = p->start_sy >> 16; y < p->max_y; y += 2, ++sy)
		for(x = p->start_x, sx = p->start_sx >> 16; x < p->max_x;
				x += 2, ++sx)
			SCALE2X(getpix32, setpix32);
}

static void do_scale_2x_clamp(scale_params_t *p)
{
	int x, y, sx, sy;
	for(y = p->start_y, sy = p->start_sy >> 16; y < p->max_y; y += 2, ++sy)
		for(x = p->start_x, sx = p->start_sx >> 16; x < p->max_x;
				x += 2, ++sx)
			SCALE2X(getpix32c, setpix32);
}

static void do_scale_2x_wrap(scale_params_t *p)
{
	int x, y, sx, sy;
	for(y = p->start_y, sy = p->start_sy >> 16; y < p->max_y; y += 2, ++sy)
		for(x = p->start_x, sx = p->start_sx >> 16; x < p->max_x;
				x += 2, ++sx)
			SCALE2X(getpix32w, setpix32);
}
#undef	CDIFF
#undef	SCALE2X

static void scale_2x(scale_params_t *p)
{
	do_scale(p, do_scale_2x_clip, do_scale_2x_noclip);
}

static void scale_2x_clamp(scale_params_t *p)
{
	do_scale(p, do_scale_2x_clamp, do_scale_2x_noclip);
}

static void scale_2x_wrap(scale_params_t *p)
{
	do_scale(p, do_scale_2x_wrap, do_scale_2x_noclip);
}


/*
 * Diamond2x scaling
 *
 * This works *great* though; kicks the butt of
 * Linear/Oversampled real hard - and at an order
 * of magnitude higher speed! :-)
 */
#define MIX(t,x,y,z)	(t).r = ((x).r+(x).r+(y).r+(z).r)>>2;	\
			(t).g = ((x).g+(x).g+(y).g+(z).g)>>2;	\
			(t).b = ((x).b+(x).b+(y).b+(z).b)>>2;	\
			(t).a = ((x).a+(x).a+(y).a+(z).a)>>2;
#define	DIAMOND(gp, sp)			\
{					\
	pix_t	   B;			\
	pix_t	D, E, F;		\
	pix_t	   H;			\
	pix_t	E0, E1;			\
	pix_t	E2, E3;			\
	B = gp(p->src, sx, sy-1);	\
	D = gp(p->src, sx-1, sy);	\
	E = gp(p->src, sx, sy);		\
	F = gp(p->src, sx+1, sy);	\
	H = gp(p->src, sx, sy+1);	\
	MIX(E0, E, B, D);		\
	MIX(E1, E, B, F);		\
	MIX(E2, E, H, D);		\
	MIX(E3, E, H, F);		\
	sp(p->dst, x, y, E0);		\
	sp(p->dst, x+1, y, E1);		\
	sp(p->dst, x, y+1, E2);		\
	sp(p->dst, x+1, y+1, E3);	\
}
static void do_scale_diamond2x_noclip(scale_params_t *p)
{
	int x, y, sx, sy;
	for(y = p->start_y, sy = p->start_sy >> 16; y < p->max_y; y += 2, ++sy)
		for(x = p->start_x, sx = p->start_sx >> 16; x < p->max_x;
				x += 2, ++sx)
			DIAMOND(getpix32_nc, setpix32_nc);
}

static void do_scale_diamond2x_clip(scale_params_t *p)
{
	int x, y, sx, sy;
	for(y = p->start_y, sy = p->start_sy >> 16; y < p->max_y; y += 2, ++sy)
		for(x = p->start_x, sx = p->start_sx >> 16; x < p->max_x;
				x += 2, ++sx)
			DIAMOND(getpix32, setpix32);
}

static void do_scale_diamond2x_clamp(scale_params_t *p)
{
	int x, y, sx, sy;
	for(y = p->start_y, sy = p->start_sy >> 16; y < p->max_y; y += 2, ++sy)
		for(x = p->start_x, sx = p->start_sx >> 16; x < p->max_x;
				x += 2, ++sx)
			DIAMOND(getpix32c, setpix32);
}

static void do_scale_diamond2x_wrap(scale_params_t *p)
{
	int x, y, sx, sy;
	for(y = p->start_y, sy = p->start_sy >> 16; y < p->max_y; y += 2, ++sy)
		for(x = p->start_x, sx = p->start_sx >> 16; x < p->max_x;
				x += 2, ++sx)
			DIAMOND(getpix32w, setpix32);
}
#undef	MIX
#undef	DIAMOND

static void scale_diamond2x(scale_params_t *p)
{
	do_scale(p, do_scale_diamond2x_clip, do_scale_diamond2x_noclip);
}

static void scale_diamond2x_clamp(scale_params_t *p)
{
	do_scale(p, do_scale_diamond2x_clamp, do_scale_diamond2x_noclip);
}

static void scale_diamond2x_wrap(scale_params_t *p)
{
	do_scale(p, do_scale_diamond2x_wrap, do_scale_diamond2x_noclip);
}


int s_filter_scale(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args)
{
	scale_params_t params;
	static void (*scaler)(scale_params_t *);
	int fmode;
	int smode = args->x;
	unsigned i;
	if((args->fx <= 0.0f) && (args->fy <= 0.0f))
		return 0;
	if((args->fx == 1.0f) && (args->fy <= 1.0f))
		return 0;

	switch(smode)
	{
	  case SF_SCALE_NEAREST:
	  case SF_SCALE_BILINEAR:
		break;
	  case SF_SCALE_SCALE2X:
	  case SF_SCALE_DIAMOND:
		if((args->fx != 2.0) || (args->fy != 2.0))
		{
			log_printf(WLOG, "Scale2x and Diamond2x scaling "
					"works only with a scaling factor "
					"of exactly 2.0!\n");
			smode = SF_SCALE_BILINEAR;
		}
		break;
	}

	getpix32_empty = s_clampcolor;

	params.max_x = ceil(b->w * args->fx);
	params.scx = (int)(65536.0 / args->fx);
	params.scy = (int)(65536.0 / args->fy);
	params.start_x = 0;
	params.start_sx = 0;

	/* Check for special SFont mode */
	if(args->flags & SF_CLAMP_SFONT)
	{
		params.start_y = 1;
		params.start_sy = 65536;
		params.max_y = (int)((b->h - 1) * args->fy + 1 + 0.5f);
	}
	else
	{
		params.start_y = 0;
		params.start_sy = 0;
		params.max_y = (int)(b->h * args->fy + 0.5f);
	}

	/* Decode filter type + SF_CLAMP_EXTEND flag */
	if(args->flags & SF_CLAMP_EXTEND)
		fmode = 1;
	else if(args->flags & SF_WRAP)
		fmode = 2;
	else
		fmode = 0;
	switch(smode)
	{
	  case SF_SCALE_NEAREST:
		scaler = scale_nearest;
		break;
	  case SF_SCALE_BILINEAR:
		switch(fmode)
		{
		  case 0: scaler = scale_bilinear;		break;
		  case 1: scaler = scale_bilinear_clamp;	break;
		  case 2: scaler = scale_bilinear_wrap;		break;
		}
		/* Offset by half a pixel */
		params.start_sx += params.scx / 2;
		params.start_sy += params.scy / 2;
		break;
	  case SF_SCALE_SCALE2X:
		switch(fmode)
		{
		  case 0: scaler = scale_2x;		break;
		  case 1: scaler = scale_2x_clamp;	break;
		  case 2: scaler = scale_2x_wrap;	break;
		}
		break;
	  case SF_SCALE_DIAMOND:
		switch(fmode)
		{
		  case 0: scaler = scale_diamond2x;		break;
		  case 1: scaler = scale_diamond2x_clamp;	break;
		  case 2: scaler = scale_diamond2x_wrap;	break;
		}
		break;
	}

	for(i = 0; i < frames; ++i)
	{
		int bpp;
		Uint32 Rmask, Gmask, Bmask, Amask;
		s_sprite_t *s = s_get_sprite_b(b, first+i);
		if(!s)
			continue;
		params.src = s->surface;
		SDL_PixelFormatEnumToMasks(KOBO_PIXELFORMAT, &bpp,
				&Rmask, &Gmask, &Bmask, &Amask);
		params.dst = SDL_CreateRGBSurface(SDL_SWSURFACE,
				params.max_x, params.max_y, bpp,
				Rmask, Gmask, Bmask, Amask);
		if(!params.dst)
			return -1;
#ifdef DEBUG
		SDL_FillRect(params.dst, NULL,
				SDL_MapRGB(params.dst->format, 255, 128, 0));
#endif

		/* Deal with SFont marker row */
		if(args->flags & SF_CLAMP_SFONT)
		{
			int x, sx;
			pix_t clear = {0, 0, 0, 0};
			/* Scale marker row with "nearest" filter */
			for(x = 0, sx = params.start_sx + params.scx / 2 - 1;
					x < params.max_x;
					++x, sx += params.scx)
				setpix32_nc(params.dst, x, 0,
						getpix32(params.src,
						sx >> 16, 0));
			/* Remove marker row from source, to avoid leakage */
			for(x = 0; x < b->w; ++x)
				setpix32_nc(params.src, x, 0, clear);
		}

		/* Do the scaling! */
		scaler(&params);

		SDL_FreeSurface(s->surface);
		s->surface = params.dst;
	}
	b->w = params.max_x;
	b->h = params.max_y;
	return 0;
}


int s_filter_cleanalpha(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args)
{
	int min = args->min;
	int max = args->max;
	int x, y;
	unsigned i;
	int contrast = (int)(args->fx * 256);
	int offset = args->x + 128;	/* 128 for the bias! */
	if(!contrast)
		contrast = 256;	/*Default = 1.0*/
	for(i = 0; i < frames; ++i)
	{
		s_sprite_t *s = s_get_sprite_b(b, first+i);
		if(!s)
			continue;
		for(y = (args->flags & SF_CLAMP_SFONT) ? 1 : 0; y < b->h; ++y)
		{
			pix_t *p = (pix_t *)((char *)s->surface->pixels +
					y * s->surface->pitch);
			for(x = 0; x < b->w; ++x)
			{
				pix_t *pix = p + x;
				int alpha = pix->a;
				alpha = (alpha-128) * contrast >> 8;
				alpha += offset;
				if(alpha <= min)
					pix->a = pix->r = pix->g = pix->b = 0;
				else if(alpha > max)
					pix->a = 255;
				else
					pix->a = alpha;
			}
		}
	}
	return 0;
}


int s_filter_brightness(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args)
{
	int min = args->min;
	int max = args->max;
	int x, y;
	unsigned i;
	int contrast = (int)(args->fy * 256);
	int offset = (int)(args->fx * 256 - 256) + 128; /* bias! */
	if((256 == contrast) && (128 == offset) && (min <= 0) && (max >= 255))
		return 0;	/* No effect! --> */

	for(i = 0; i < frames; ++i)
	{
		s_sprite_t *s = s_get_sprite_b(b, first + i);
		if(!s)
			continue;
		for(y = (args->flags & SF_CLAMP_SFONT) ? 1 : 0; y < b->h; ++y)
		{
			pix_t *p = (pix_t *)((char *)s->surface->pixels +
					y * s->surface->pitch);
			for(x = 0; x < b->w; ++x)
			{
				int v;
				pix_t *pix = p + x;
				v = ((pix->r - 128) * contrast >> 8) + offset;
				if(v < min)
					v = min;
				else if(v > max)
					v = max;
				pix->r = v;
				v = ((pix->g - 128) * contrast >> 8) + offset;
				if(v < min)
					v = min;
				else if(v > max)
					v = max;
				pix->g = v;
				v = ((pix->b - 128) * contrast >> 8) + offset;
				if(v < min)
					v = min;
				else if(v > max)
					v = max;
				pix->b = v;
			}
		}
	}
	return 0;
}


static void construct_limits(int *min, int *max, int maxdev)
{
	*min -= maxdev;
	if(*min < 0)
		*min = 0;
	*max += maxdev;
	if(*max > 255)
		*max = 255;
}


static inline int check_limits(int min, int max, int value)
{
	if(value < min)
		return 0;
	if(value > max)
		return 0;
	return 1;
}


int s_filter_key2alpha(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args)
{
	int x, y, krmin, krmax, kgmin, kgmax, kbmin, kbmax;
	unsigned i;
	krmin = krmax = s_colorkey.r;
	kgmin = kgmax = s_colorkey.g;
	kbmin = kbmax = s_colorkey.b;
	construct_limits(&krmin, &krmax, args->max);
	construct_limits(&kgmin, &kgmax, args->max);
	construct_limits(&kbmin, &kbmax, args->max);
	for(i = 0; i < frames; ++i)
	{
		s_sprite_t *s = s_get_sprite_b(b, first+i);
		if(!s)
			continue;
		for(y = (args->flags & SF_CLAMP_SFONT) ? 1 : 0; y < b->h; ++y)
		{
			pix_t *p = (pix_t *)((char *)s->surface->pixels +
					y * s->surface->pitch);
			for(x = 0; x < b->h; ++x)
			{
				pix_t *pix = p + x;
				if(check_limits(krmin, krmax, pix->r)
						&& check_limits(kgmin, kgmax, pix->g)
						&& check_limits(kbmin, kbmax, pix->b))
					pix->r = pix->g = pix->b = pix->a = 0;
			}
		}
	}
	return 0;
}


int s_filter_noise(s_bank_t *bank, unsigned first, unsigned frames,
		s_filter_args_t *args)
{
	int x, y;
	unsigned i;
	int br, bg, bb;	/* bias */
	int ar, ag, ab;	/* amplitude */
	i = args->min;
	br = args->r * i >> 8;
	bg = args->g * i >> 8;
	bb = args->b * i >> 8;
	i = args->max - args->min;
	ar = args->r * i >> 8;
	ag = args->g * i >> 8;
	ab = args->b * i >> 8;
	noise_reset(args->x);
	for(i = 0; i < frames; ++i)
	{
		s_sprite_t *s = s_get_sprite_b(bank, first+i);
		if(!s)
			continue;
		for(y = (args->flags & SF_CLAMP_SFONT) ? 1 : 0; y < bank->h;
				++y)
		{
			pix_t *p = (pix_t *)((char *)s->surface->pixels +
					y * s->surface->pitch);
			for(x = 0; x < bank->w; ++x)
			{
				int r, g, b;
				pix_t *pix = p + x;
				int rnd = noise();
				r = rnd * ar + br + pix->r;
				g = rnd * ag + bg + pix->g;
				b = rnd * ab + bb + pix->b;
				if(r>255)
					r = 255;
				if(g>255)
					g = 255;
				if(b>255)
					b = 255;
				pix->r = r;
				pix->g = g;
				pix->b = b;
			}
		}
	}
	return 0;
}
