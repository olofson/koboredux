/*(LGPLv2.1)
------------------------------------------------------------
	SoFont - SDL Object Font Library
------------------------------------------------------------
 * Copyright ???? Karl Bartel
 * Copyright ???? Luc-Olivier de Charriere
 * Copyright 2009 David Olofson
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
#include "logger.h"
#include "stdlib.h"
#include "sofont.h"
#include "string.h"

SoFont::SoFont(SDL_Renderer *_target)
{
	target = _target;
	glyphs = NULL;
	CharPos = NULL;
	CharOffset = NULL;
	Spacing = NULL;
	height = 0;
	max_i = 0;
	spacew = 0;
	cursShift = 0;
	background = 0;
	xspace = 0;
	xscale = yscale = 256;
}

SoFont::~SoFont()
{
	if(glyphs)
		SDL_DestroyTexture(glyphs);
	delete [] CharPos;
	delete [] CharOffset;
	delete [] Spacing;
}

namespace SoFontUtilities
{
	static Uint32 SoFontGetPixel(SDL_Surface *Surface, Sint32 X, Sint32 Y)
	{
		Uint8 *bits;
		Uint32 Bpp;

		if(!Surface)
		{
			log_printf(ELOG, "SoFontGetPixel: No surface!\n");
			return 0;
		}
		if((X < 0) || (X >= Surface->w))
		{
			log_printf(ELOG, "SoFontGetPixel: X (%d)"
					" out of range!\n", X);
			return 0;
		}

		Bpp = Surface->format->BytesPerPixel;

		bits = ((Uint8 *) Surface->pixels) + Y * Surface->pitch +
				X * Bpp;

		switch (Bpp)
		{
		  case 1:
			return *((Uint8 *) Surface->pixels +
					Y * Surface->pitch + X);
		  case 2:
			return *((Uint16 *) Surface->pixels +
					Y * Surface->pitch / 2 + X);
		  case 3:
			// Format/endian independent
			Uint8 r, g, b;
			r = *((bits) + Surface->format->Rshift / 8);
			g = *((bits) + Surface->format->Gshift / 8);
			b = *((bits) + Surface->format->Bshift / 8);
			return SDL_MapRGB(Surface->format, r, g, b);
		  case 4:
			return *((Uint32 *) Surface->pixels +
					Y * Surface->pitch / 4 + X);
		}
		log_printf(ELOG, "SoFontGetPixel: Unsupported pixel format!\n");
		return 0;	// David (to get rid of warning)
	}
	static void SoFontSetPixel(SDL_Surface *Surface, Sint32 X, Sint32 Y,
			Uint32 c)
	{
		Uint8 *bits;
		Uint32 Bpp;

		if(!Surface)
		{
			log_printf(ELOG, "SoFontSetPixel: No surface!\n");
			return;
		}
		if((X < 0) || (X >= Surface->w))
		{
			log_printf(ELOG, "SoFontSetPixel: X (%d)"
					" out of range!\n", X);
			return;
		}

		Bpp = Surface->format->BytesPerPixel;

		bits = ((Uint8 *) Surface->pixels) + Y * Surface->pitch +
				X * Bpp;

		switch (Bpp)
		{
		  case 1:
			*((Uint8 *) Surface->pixels + Y * Surface->pitch +
					X) = (Uint8) c;
			break;
		  case 2:
			*((Uint16 *) Surface->pixels +
					Y * Surface->pitch / 2 + X) =
					(Uint16) c;
			break;
		  case 3:
			// Format/endian independent
			Uint8 r, g, b;
			SDL_GetRGB(c, Surface->format, &r, &g, &b);
			*((bits) + Surface->format->Rshift / 8) = r;
			*((bits) + Surface->format->Gshift / 8) = g;
			*((bits) + Surface->format->Bshift / 8) = b;
			break;
		  case 4:
			*((Uint32 *) Surface->pixels +
					Y * Surface->pitch / 4 + X) = c;
			break;
		}
	}
	static void sdcRects(SDL_Rect * source, SDL_Rect * destination,
			SDL_Rect clipping)
	{
		// Use if destination have the same size than source &
		// cliping on destination
		int dx = clipping.x - destination->x;
		int dy = clipping.y - destination->y;

		int sw = source->w;
		int sh = source->h;

		if(dx > 0)
		{
			source->x += dx;
			destination->x += dx;

			sw -= dx;
			destination->w -= dx;
		}
		if(dy > 0)
		{
			source->y += dy;
			destination->y += dy;

			sh -= dy;
			destination->h -= dy;
		}

		int dwx = (destination->x + destination->w) - (clipping.x +
				clipping.w);
		int dhy = (destination->y + destination->h) - (clipping.y +
				clipping.h);

		if(dwx > 0)
		{
			sw -= dwx;
			destination->w -= dwx;
		}
		if(dhy > 0)
		{
			sh -= dhy;
			destination->h -= dhy;
		}

		if(sw > 0)
			source->w = sw;
		else
			source->w = 0;

		if(sh > 0)
			source->h = sh;
		else
			source->h = 0;

	}

}
using namespace SoFontUtilities;

bool SoFont::DoStartNewChar(SDL_Surface *surface, Sint32 x)
{
	Uint8 r, g, b;
	SDL_GetRGB(SoFontGetPixel(surface, x, 0), surface->format, &r, &g, &b);
	return (r > 128) && (g < 128) && (b > 128);
}

void SoFont::CleanSurface(SDL_Surface *surface)
{
	int x = 0, y = 0;
	while(x < surface->w)
	{
		y = 0;
		SoFontSetPixel(surface, x, y, background);
		x++;
	}
}


bool SoFont::Load(SDL_Surface *FontSurface)
{
	int x = 0, i = 0, s = 0;

	int _CharPos[256];
	int _CharOffset[256];
	int _Spacing[256];

	if(!FontSurface)
	{
		log_printf(ELOG, "SoFont recieved a NULL SDL_Surface\n");
		return false;
	}
	if(glyphs)
		SDL_DestroyTexture(glyphs);
	glyphs = NULL;
	height = FontSurface->h - 1;
	while(x < FontSurface->w)
	{
		if(DoStartNewChar(FontSurface, x))
		{
			if(i)
				_Spacing[i - 1] = x - s;
			int p = x;
			while((x < FontSurface->w) &&
					(DoStartNewChar(FontSurface, x)))
				x++;
			s = x;
			_CharPos[i] = (x + p + 1) / 2;
			_CharOffset[i] = x - _CharPos[i];
			i++;
		}
		x++;
	}
	//Note that spacing is not needed for the last char,
	//as it's just used for the blit width calculation.
	if(i)
		_Spacing[i - 1] = x - s;
	_Spacing[i] = 0;
	_CharOffset[i] = 0;
	_CharPos[i++] = FontSurface->w;

	max_i = START_CHAR + i - 1;
	background = SoFontGetPixel(FontSurface, 0, height);
// FIXME: Use this when there is no alpha channel?
//	SDL_SetColorKey(picture, SDL_SRCCOLORKEY, background);
	CleanSurface(FontSurface);

	glyphs = SDL_CreateTextureFromSurface(target, FontSurface);
	if(!glyphs)
	{
		log_printf(ELOG, "SoFont could not create texture from "
				"surface\n");
		return false;
	}

	delete [] CharPos;
	delete [] CharOffset;
	delete [] Spacing;
	CharPos = new int[i];
	CharOffset = new int[i];
	Spacing = new int[i];
	memcpy(CharPos, _CharPos, i * sizeof(int));
	memcpy(CharOffset, _CharOffset, i * sizeof(int));
	memcpy(Spacing, _Spacing, i * sizeof(int));

	// We search for a smart space width:
	// Changed from "a", "A", "0" for Kobo Deluxe.
	// Spaces were *way* to wide! //David
	spacew = 0;
	if(!spacew)
		spacew = TextWidth("i") * 3 / 2 * 256 / xscale;
	if(!spacew)
		spacew = TextWidth("I") * 3 / 2 * 256 / xscale;
	if(!spacew)
		spacew = TextWidth(".") * 3 / 2 * 256 / xscale;
	if(!spacew)
		spacew = CharPos[1] - CharPos[0];

	// We search for a smart cursor position:
	int cursBegin = 0;
	int cursWidth = 0;
	cursShift = 0;
	if('|' > max_i)
		return true;	//No bar in this font!

	if(background == SoFontGetPixel(FontSurface, CharPos['|' - START_CHAR],
			height / 2))
	{
		// Up to the first | color
		for(cursBegin = 0; cursBegin <= TextWidth("|"); cursBegin++)
			if(background != SoFontGetPixel(FontSurface,
					CharPos['|' - START_CHAR] +
					cursBegin, height / 2))
				break;
		// Up to the end of the | color
		for(cursWidth = 0; cursWidth <= TextWidth("|"); cursWidth++)
			if(background == SoFontGetPixel(FontSurface,
					CharPos['|' - START_CHAR] +
					cursBegin + cursWidth, height / 2))
				break;
	}
	else
	{
		// Up to the end of the | color
		for(cursWidth = 0; cursWidth <= TextWidth("|"); cursWidth++)
			if(background == SoFontGetPixel(FontSurface,
					CharPos['|' - START_CHAR] +
					cursWidth, height / 2))
				break;
	}
	cursShift = cursBegin + 1;	// cursWidth could be used if
					// image format changes.

	return true;
}

void SoFont::PutString(int x, int y, const char *text, SDL_Rect *clip)
{
	if((!glyphs) || (!text))
		return;
	int x0 = x;
	int ofs, i = 0;
	SDL_Rect srcrect, dstrect;
	int targetw;
	if(!clip)
		SDL_GetRendererOutputSize(target, &targetw, NULL);
	while(text[i] != '\0')
	{
		if(text[i] == ' ')
		{
			x += spacew * xscale >> 8;
			i++;
		}
		else if(text[i] == '\n')
		{
			x = x0;
			y += height * yscale >> 8;
			i++;
		}
		else if(text[i] == '\r')
		{
			x = x0;
			y += height * yscale >> 9;
			i++;
		}
		else if((text[i] >= START_CHAR) && (text[i] <= max_i))
		{
			ofs = text[i] - START_CHAR;
			srcrect.w = CharPos[ofs + 1] - CharPos[ofs];
			srcrect.h = height;
			srcrect.x = CharPos[ofs];
			srcrect.y = 1;
			dstrect.x = x - (CharOffset[ofs] * xscale >> 8);
			dstrect.y = y;
			dstrect.w = (int)srcrect.w * xscale >> 8;
			dstrect.h = (int)srcrect.h * yscale >> 8;
			x += (Spacing[ofs] + xspace) * xscale >> 8;
			if(clip)
				sdcRects(&srcrect, &dstrect, *clip);
			SDL_RenderCopy(target, glyphs, &srcrect, &dstrect);
			i++;
		}
		else
			i++;	// other chars are ignored

		// Coarse clipping
		if(clip)
		{
			if(x > clip->x + clip->w)
				return;
		}
		else
		{
			if(x > targetw)
				return;
		}
	}
}

void SoFont::PutStringWithCursor(int xs, int y,
		const char *text, int cursPos, SDL_Rect * clip,
		bool showCurs)
{
	if((!glyphs) || (!text))
		return;
	if('|' > max_i)
		showCurs = false;
	int ofs, i = 0, x = xs;

	SDL_Rect srcrect, dstrect;

	// We want the cursor to appear under the main text.
	if(showCurs)
	{
		while(text[i] != '\0')
			if(i == cursPos)
				break;
			else if(text[i] == ' ')
			{
				x += spacew * xscale >> 8;
				i++;
			}
			else if(text[i] == '\n')
			{
				x = xs;
				y += height * yscale >> 8;
				i++;
			}
			else if(text[i] == '\r')
			{
				x = xs;
				y += height * yscale >> 9;
				i++;
			}
			else if((text[i] >= START_CHAR)
					&& (text[i] <= max_i))
			{
				ofs = text[i] - START_CHAR;
				x += (Spacing[ofs] + xspace) * xscale >> 8;
				i++;
			}
			else
				i++;
		ofs = '|' - START_CHAR;
		srcrect.w = CharPos[ofs + 1] - CharPos[ofs];
		srcrect.h = height;
		srcrect.x = CharPos[ofs];
		srcrect.y = 1;
		dstrect.x = x - (cursShift * xscale >> 8);
		dstrect.y = y;
		dstrect.w = (int)srcrect.w * xscale >> 8;
		dstrect.h = (int)srcrect.h * xscale >> 8;
		if(clip)
			sdcRects(&srcrect, &dstrect, *clip);
		SDL_RenderCopy(target, glyphs, &srcrect, &dstrect);
	}
	// Then the text:
	PutString(xs, y, text, clip);
}

int SoFont::TextWidth(const char *text, int min, int max)
{
	if(!text)
		return 0;
	int ofs, x = 0, i = min;
	int maxx = 0;
	while((text[i] != '\0') && (i < max))
	{
		if(text[i] == ' ')
		{
			x += spacew;
			i++;
		}
		else if((text[i] == '\n') || (text[i] == '\r'))
		{
			if(x >  maxx)
				maxx = x;
			x = 0;
			i++;
		}
		else if((text[i] >= START_CHAR) && (text[i] <= max_i))
		{
			ofs = text[i] - START_CHAR;
			x += Spacing[ofs] + xspace;
			i++;
		}
		else
			i++;
	}
	if(x >  maxx)
		maxx = x;
	return maxx * xscale >> 8;
}

void SoFont::XCenteredString(int y, const char *text, SDL_Rect *clip)
{
	int targetw;
	SDL_GetRendererOutputSize(target, &targetw, NULL);
	PutString(targetw / 2 - TextWidth(text) / 2, y, text, clip);
}

void SoFont::CenteredString(int x, int y, const char *text, SDL_Rect *clip)
{
	PutString(x - TextWidth(text) / 2, y - height / 2, text, clip);
}

void SoFont::CenteredString(const char *text, SDL_Rect *clip)
{
	int targetw, targeth;
	SDL_GetRendererOutputSize(target, &targetw, &targeth);
	CenteredString(targetw / 2, targeth / 2, text, clip);
}

void SoFont::PutStringCleverCursor(const char *text, int cursPos,
		SDL_Rect *r, SDL_Rect *clip, bool showCurs)
{
	int w1 = TextWidth(text, 0, cursPos);
	int w2 = TextWidth(text);

	if((w2 < r->w) || (w1 < r->w / 2))
		PutStringWithCursor(r->x,
				r->y + (r->h - height) / 2, text, cursPos,
				clip, showCurs);
	else if(w1 < w2 - r->w / 2)
		PutStringWithCursor(r->x - w1 + r->w / 2,
				r->y + (r->h - height) / 2, text, cursPos,
				clip, showCurs);
	else
		PutStringWithCursor(r->x - w2 + r->w,
				r->y + (r->h - height) / 2, text, cursPos,
				clip, showCurs);
}

int SoFont::TextCursorAt(const char *text, int px)
{
	int ofs, x = 0, i = 0, ax = 0;

	if(px <= 0)
		return 0;

	while(text[i] != '\0')
	{
		if(text[i] == ' ')
		{
			x += spacew * xscale >> 8;
			i++;
		}
		else if((text[i] >= START_CHAR) && (text[i] <= max_i))
		{
			ofs = text[i] - START_CHAR;
			x += (Spacing[ofs] + xspace) * xscale >> 8;
			i++;
		}
		else
			i++;

		if(px < (ax + x) / 2)
			return (i - 1);
		ax = x;
	}
	return i;
}

int SoFont::CleverTextCursorAt(const char *text, int px, int cursPos,
		SDL_Rect *r)
{
	int w1 = TextWidth(text, 0, cursPos);
	int w2 = TextWidth(text);
	if((w2 < r->w) || (w1 < r->w / 2))
		return TextCursorAt(text, px);
	else if(w1 < w2 - r->w / 2)
		return TextCursorAt(text, px + w1 - (r->w / 2));
	else
		return TextCursorAt(text, px + w2 - r->w);
}
