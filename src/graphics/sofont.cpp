/*(LGPLv2.1)
------------------------------------------------------------
	SoFont - SDL Object Font Library
------------------------------------------------------------
 * Copyright (C) ???? Karl Bartel
 * Copyright (C) ???? Luc-Olivier de Charriere
 * Copyright (C) 2009 David Olofson
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

SoFont::SoFont()
{
	picture = NULL;
	CharPos = NULL;
	CharOffset = NULL;
	Spacing = NULL;
	height = 0;
	max_i = 0;
	spacew = 0;
	cursShift = 0;
	background = 0;
	xspace = 1;
}

SoFont::~SoFont()
{
	if(picture)
		SDL_FreeSurface(picture);
	delete[]CharPos;
	delete[]CharOffset;
	delete[]Spacing;
}

namespace SoFontUtilities
{
	Uint32 SoFontGetPixel(SDL_Surface * Surface, Sint32 X, Sint32 Y)
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
			break;
		  case 2:
			return *((Uint16 *) Surface->pixels +
					Y * Surface->pitch / 2 + X);
			break;
		  case 3:
			// Format/endian independent
			Uint8 r, g, b;
			r = *((bits) + Surface->format->Rshift / 8);
			g = *((bits) + Surface->format->Gshift / 8);
			b = *((bits) + Surface->format->Bshift / 8);
			return SDL_MapRGB(Surface->format, r, g, b);
			break;
		  case 4:
			return *((Uint32 *) Surface->pixels +
					Y * Surface->pitch / 4 + X);
			break;
		}
		log_printf(ELOG, "SoFontGetPixel: Unsupported pixel format!\n");
		return 0;	// David (to get rid of warning)
	}
	void SoFontSetPixel(SDL_Surface * Surface, Sint32 X, Sint32 Y,
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
	void clipx(SDL_Rect * srcrect, SDL_Rect * dstrect, SDL_Rect * clip)
	{
		// Use if destination have the same size than source.
		int dx = clip->x - dstrect->x;

		int sw = srcrect->w;	// Because SDL_Rect.w are
					// unsigned.
		int dw = dstrect->w;

		if(dx > 0)
		{
			srcrect->x += dx;
			dstrect->x += dx;

			sw -= dx;
			dw -= dx;
		}

		int dwx = (dstrect->x + dstrect->w) - (clip->x + clip->w);

		if(dwx > 0)
		{
			sw -= dwx;
			dw -= dwx;
		}

		if(sw > 0)
			srcrect->w = sw;
		else
			srcrect->w = 0;

		if(dw > 0)
			dstrect->w = dw;
		else
			dstrect->w = 0;
	}
	void sdcRects(SDL_Rect * source, SDL_Rect * destination,
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

bool SoFont::DoStartNewChar(Sint32 x)
{
	if(!picture)
		return false;
	return SoFontGetPixel(picture, x, 0) ==
			SDL_MapRGB(picture->format, 255, 0, 255);
}

void SoFont::CleanSurface()
{
	if(!picture)
		return;

	int x = 0, y = 0;
	Uint32 pix = SDL_MapRGB(picture->format, 255, 0, 255);

	while(x < picture->w)
	{
		y = 0;
		if(SoFontGetPixel(picture, x, y) == pix)
			SoFontSetPixel(picture, x, y, background);
		x++;
	}
}


bool SoFont::load(SDL_Surface * FontSurface)
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
	if(picture)
		SDL_FreeSurface(picture);
	picture = FontSurface;
	height = picture->h - 1;
	while(x < picture->w)
	{
		if(DoStartNewChar(x))
		{
			if(i)
				_Spacing[i - 1] = x - s + xspace;
			int p = x;
			while((x < picture->w) && (DoStartNewChar(x)))
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
	_CharPos[i++] = picture->w;

	max_i = START_CHAR + i - 1;
	background = SoFontGetPixel(picture, 0, height);
// FIXME: Use this when there is no alpha channel?
//	SDL_SetColorKey(picture, SDL_SRCCOLORKEY, background);
	CleanSurface();

	delete[]CharPos;
	delete[]CharOffset;
	delete[]Spacing;
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
		spacew = TextWidth("i") * 3 / 2;
	if(!spacew)
		spacew = TextWidth("I") * 3 / 2;
	if(!spacew)
		spacew = TextWidth(".") * 3 / 2;
	if(!spacew)
		spacew = CharPos[1] - CharPos[0];

	// We search for a smart cursor position:
	int cursBegin = 0;
	int cursWidth = 0;
	cursShift = 0;
	if('|' > max_i)
		return true;	//No bar in this font!

	if(background == SoFontGetPixel(picture, CharPos['|' - START_CHAR],
				height / 2))
	{
		// Up to the first | color
		for(cursBegin = 0; cursBegin <= TextWidth("|");
				cursBegin++)
			if(background != SoFontGetPixel(picture,
						CharPos['|' - START_CHAR] +
						cursBegin, height / 2))
				break;
		// Up to the end of the | color
		for(cursWidth = 0; cursWidth <= TextWidth("|");
				cursWidth++)
			if(background == SoFontGetPixel(picture,
						CharPos['|' - START_CHAR] +
						cursBegin + cursWidth,
						height / 2))
				break;
	}
	else
	{
		// Up to the end of the | color
		for(cursWidth = 0; cursWidth <= TextWidth("|");
				cursWidth++)
			if(background == SoFontGetPixel(picture,
						CharPos['|' - START_CHAR] +
						cursWidth, height / 2))
				break;
	}
	cursShift = cursBegin + 1;	// cursWidth could be used if
					// image format changes.

	return true;
}

void SoFont::PutString(SDL_Surface *Surface, int x, int y,
		const char *text, SDL_Rect *clip)
{
	if((!picture) || (!Surface) || (!text))
		return;
	int ofs, i = 0;
	SDL_Rect srcrect, dstrect;
	while(text[i] != '\0')
	{
		if(text[i] == ' ')
		{
			x += spacew;
			i++;
		}
		else if((text[i] >= START_CHAR) && (text[i] <= max_i))
		{
			ofs = text[i] - START_CHAR;
			srcrect.w = dstrect.w =
					this->CharPos[ofs + 1] -
					this->CharPos[ofs];
			srcrect.h = dstrect.h = height;
			srcrect.x = this->CharPos[ofs];
			srcrect.y = 1;
			dstrect.x = x - CharOffset[ofs];
			dstrect.y = y;
			x += Spacing[ofs];
			if(clip)
				sdcRects(&srcrect, &dstrect, *clip);
			SDL_BlitSurface(picture, &srcrect, Surface,
					&dstrect);
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
			if(x > Surface->w)
				return;
		}
	}
}

void SoFont::PutStringWithCursor(SDL_Surface * Surface, int xs, int y,
		const char *text, int cursPos, SDL_Rect * clip,
		bool showCurs)
{
	if((!picture) || (!Surface) || (!text))
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
				x += spacew;
				i++;
			}
			else if((text[i] >= START_CHAR)
					&& (text[i] <= max_i))
			{
				ofs = text[i] - START_CHAR;
				x += Spacing[ofs];
				i++;
			}
			else
				i++;
		ofs = '|' - START_CHAR;

		srcrect.w = dstrect.w = CharPos[ofs + 1] - CharPos[ofs];
		srcrect.h = dstrect.h = height;
		srcrect.x = this->CharPos[ofs];
		srcrect.y = 1;
		dstrect.x = x - cursShift;
		dstrect.y = y;
		if(clip)
			sdcRects(&srcrect, &dstrect, *clip);
		SDL_BlitSurface(picture, &srcrect, Surface, &dstrect);
	}
	// Then the text:
	PutString(Surface, xs, y, text, clip);
}
int SoFont::TextWidth(const char *text, int min, int max)
{
	if(!picture)
		return 0;
	int ofs, x = 0, i = min;
	while((text[i] != '\0') && (i < max))
	{
		if(text[i] == ' ')
		{
			x += spacew;
			i++;
		}
		else if((text[i] >= START_CHAR) && (text[i] <= max_i))
		{
			ofs = text[i] - START_CHAR;
			x += Spacing[ofs];
			i++;
		}
		else
			i++;
	}
	return x;
}
void SoFont::XCenteredString(SDL_Surface * Surface, int y,
		const char *text, SDL_Rect * clip)
{
	if(!picture)
		return;
	PutString(Surface, Surface->w / 2 - TextWidth(text) / 2, y, text,
			clip);
}

void SoFont::CenteredString(SDL_Surface * Surface, int x, int y,
		const char *text, SDL_Rect * clip)
{
	if(!picture)
		return;
	PutString(Surface, x - TextWidth(text) / 2, y - height / 2, text,
			clip);
}

void SoFont::CenteredString(SDL_Surface * Surface, const char *text,
		SDL_Rect * clip)
{
	if(!picture)
		return;
	CenteredString(Surface, Surface->clip_rect.w / 2,
			Surface->clip_rect.h / 2, text, clip);
}

void SoFont::PutStringCleverCursor(SDL_Surface * Surface, const char *text,
		int cursPos, SDL_Rect * r, SDL_Rect * clip, bool showCurs)
{
	if((!picture) || (!text))
		return;

	int w1, w2;

	w1 = TextWidth(text, 0, cursPos);
	w2 = TextWidth(text);

	if((w2 < r->w) || (w1 < r->w / 2))
		PutStringWithCursor(Surface, r->x,
				r->y + (r->h - height) / 2, text, cursPos,
				clip, showCurs);
	else if(w1 < w2 - r->w / 2)
		PutStringWithCursor(Surface, r->x - w1 + r->w / 2,
				r->y + (r->h - height) / 2, text, cursPos,
				clip, showCurs);
	else
		PutStringWithCursor(Surface, r->x - w2 + r->w,
				r->y + (r->h - height) / 2, text, cursPos,
				clip, showCurs);
}

int SoFont::TextCursorAt(const char *text, int px)
{
	if(!picture)
		return 0;
	int ofs, x = 0, i = 0, ax = 0;

	if(px <= 0)
		return 0;

	while(text[i] != '\0')
	{
		if(text[i] == ' ')
		{
			x += spacew;
			i++;
		}
		else if((text[i] >= START_CHAR) && (text[i] <= max_i))
		{
			ofs = text[i] - START_CHAR;
			x += Spacing[ofs];
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
		SDL_Rect * r)
{
	if((!picture) || (!text))
		return 0;
	int w1, w2;
	w1 = TextWidth(text, 0, cursPos);
	w2 = TextWidth(text);
	if((w2 < r->w) || (w1 < r->w / 2))
		return TextCursorAt(text, px);
	else if(w1 < w2 - r->w / 2)
		return TextCursorAt(text, px + w1 - (r->w / 2));
	else
		return TextCursorAt(text, px + w2 - r->w);
}
