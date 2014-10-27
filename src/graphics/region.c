/*(LGPLv2.1)
----------------------------------------------------------------------
	region.c - Graphics Engine
----------------------------------------------------------------------
 * Copyright (C) 2007, 2009 David Olofson
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

#include "region.h"
#include <stdlib.h>

static struct
{
	RGN_region	*region;
	int		x, y;
	SDL_Surface	*target;
} s;


static Uint32 gp1(SDL_Surface *surface, int x, int y)
{
	return *((Uint8 *)surface->pixels + y * surface->pitch + x);
}


static Uint32 gp2(SDL_Surface *surface, int x, int y)
{
	return *((Uint16 *)surface->pixels + y * surface->pitch / 2 + x);
}


static Uint32 gp3(SDL_Surface *surface, int x, int y)
{
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	return p[0] << 16 | p[1] << 8 | p[2];
#else
	return p[0] | p[1] << 8 | p[2] << 16;
#endif
}


static Uint32 gp4(SDL_Surface *surface, int x, int y)
{
	return *((Uint32 *)surface->pixels + y * surface->pitch / 4 + x);
}


RGN_region *RGN_ScanMask(SDL_Surface *src, Uint32 key)
{
	RGN_region *rgn = (RGN_region *)calloc(1, sizeof(RGN_region));
	Uint32 (*gp)(SDL_Surface *surface, int x, int y);
	int x, y;
	if(!rgn)
		return NULL;
	rgn->rows = src->h;
	rgn->spans = (Uint16 **)calloc(1, sizeof(Uint16 *) * src->h);
	if(!rgn->spans)
	{
		RGN_FreeRegion(rgn);
		return NULL;
	}
	switch(src->format->BytesPerPixel)
	{
	  case 1:
		gp = gp1;
		break;
	  case 2:
		gp = gp2;
		break;
	  case 3:
		gp = gp3;
		break;
	  case 4:
		gp = gp4;
		break;
	  default:
		RGN_FreeRegion(rgn);
		return NULL;
	}
	SDL_LockSurface(src);
	for(y = 0; y < src->h; ++y)
	{
		int i = 0;
		int drawing = 0;
		Uint16 *sp = (Uint16 *)malloc(sizeof(Uint16) * 2 * (src->w + 1));
		if(!sp)
		{
			SDL_UnlockSurface(src);
			RGN_FreeRegion(rgn);
			return NULL;
		}
		for(x = 0; x < src->w; ++x)
		{
			int nd = (gp(src, x, y) == key);
			if(nd != drawing)
			{
				sp[i++] = x;
				drawing = nd;
			}
		}
		if(drawing)
			sp[i++] = src->w;
		sp[i++] = 0;
		sp[i++] = 0;
		rgn->spans[y] = (Uint16 *)realloc(sp, sizeof(Uint16) * i);
		if(!rgn->spans[y])
			rgn->spans[y] = sp;
	}
	SDL_UnlockSurface(src);
	return rgn;
}


void RGN_FreeRegion(RGN_region *rgn)
{
	if(!rgn)
		return;
	if(rgn->spans)
	{
		int i;
		for(i = rgn->rows - 1; i >= 0; --i)
			free(rgn->spans[i]);
		free(rgn->spans);
	}
	free(rgn);
}


void RGN_Target(SDL_Surface *tgt)
{
	s.target = tgt;
}


void RGN_SetRegion(RGN_region *rgn, int xpos, int ypos)
{
	s.region = rgn;
	s.x = xpos;
	s.y = ypos;
}


int RGN_Blit(SDL_Surface *src, SDL_Rect *sr, int x, int y)
{
// FIXME: Clipping is only half done here...
	int yy0, yy, yy1;
	int sxmin, sxmax, symin, symax;
	SDL_Rect sr2;
	if(!s.region)
	{
		SDL_Rect dr;
		dr.x = x;
		dr.y = y;
		return SDL_BlitSurface(src, sr, s.target, &dr);
	}
	if(sr)
	{
		sxmin = sr->x;
		symin = sr->y;
		sxmax = sxmin + sr->w;
		symax = symin + sr->h;
	}
	else
	{
		sxmin = 0;
		symin = 0;
		sxmax = src->w;
		symax = src->h;
	}
	yy0 = s.y;
	if(yy0 >= y + (symax - symin))
		return 0;
	if(yy0 < y)
		yy0 = y;
	yy1 = s.y + s.region->rows;
	if(y >= yy1)
		return 0;
	if(yy1 > y + (symax - symin))
		yy1 = y + (symax - symin);
	sr2.h = 1;
	for(yy = yy0; yy < yy1; ++yy)
	{
		SDL_Rect dr;
		Uint16 *spans = s.region->spans[yy - s.y];
		sr2.y = symin + y - yy0;
		dr.y = yy;
		for( ; spans[0] && spans[1] ; spans += 2)
		{
			int x0 = s.x + spans[0];
			int x1 = s.x + spans[1];
			dr.x = x0;
			sr2.x = sxmin + x0 - x;
			sr2.w = x1 - x0;
			SDL_BlitSurface(src, &sr2, s.target, &dr);
		}
	}
	return 0;
}
