/*(LGPLv2.1)
------------------------------------------------------------
	SoFont - SDL Object Font Library
------------------------------------------------------------
 * Copyright ???? Karl Bartel
 * Copyright ???? Luc-Olivier de Charriere
 * Copyright 2009 David Olofson
 * Copyright 2015-2017 David Olofson (Kobo Redux)
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
	
	Originally from C Library:
		Karl Bartel

*    SFONT - SDL Font Library by Karl Bartel <karlb@gmx.net>            *
*                                                                       *
*  All functions are explained below. There are two versions of each    *
*  funtction. The first is the normal one, the function with the        *
*  2 at the end can be used when you want to handle more than one font  *
*  in your program.                                                     *
*                                                                       *
	
	Copied into a C++ object to allow multiple fonts by:
		Luc-Olivier de Charriere

	David Olofson <david@olofson.net>:
		* Strings changed to 'const char *'
		* Cursor tests first check if '|' is present.
		* Shadowed variables fixed.
		* Garbage data in spacing table fixed. (Thanks to
		  Andreas Sp�ngberg for discovering this one!)
		* Added ExtraSpace(). (Scaling support hack...)
		* Disabled colorkeying. (Ruins some RGBA fonts!)

	David Olofson 2015:
		* SDL 2 port
		* Glyph marker color is now anything more purple than #808080.
		* The top (marker) pixel row is now simply cleared after
		  glyphs have been extracted, completely disregarding contents.
		* Added scaling support.
		* Fixed some API naming inconsistencies.

	David Olofson 2016:
		* ExtraSpace() default changed to 0, to avoid ugly hacks when
		  scaling the source textures. Fonts need to include full
		  spacing in their encoding!
		* Added GetGlyphs() to allow the application to access SDL2
		  blending parameters.

	David Olofson 2017:
		* Added TabSize() and '\t' tab support.
*/

#ifndef __SOFONT_H
#define __SOFONT_H

#include "SDL.h"

#define START_CHAR 33

class SoFont
{
public:
	SoFont(SDL_Renderer *_target);
	~SoFont();

	bool Load(SDL_Surface *FontSurface);
	void SetScale(float xs, float ys)
	{
		xscale = (int)(xs * 256.0f);
		yscale = (int)(ys * 256.0f);
	}

	// Renders a string to the target
	void PutString(int x, int y, const char *text, SDL_Rect *clip = NULL);
	void PutStringWithCursor(int x, int y, const char *text, int cursPos,
			SDL_Rect *clip = NULL, bool showCurs = true);

	// Returns the width of "text" in pixels
	int TextWidth(const char *text, int min = 0, int max = 255);

	int FontHeight()	{ return height; }

	// Blits a string to with centered x position
	void XCenteredString(int y, const char *text, SDL_Rect* clip=NULL);
	// Blits a string to with centered x & y position
	void CenteredString(const char *text, SDL_Rect* clip=NULL);
	// Blits a string to with centered around x & y positions
	void CenteredString(int x, int y, const char *text,
			SDL_Rect* clip = NULL);
	
	// This was specially developed for GUI
	void PutStringCleverCursor(const char *text, int cursPos, SDL_Rect *r,
			SDL_Rect* clip = NULL, bool showCurs = true);
	
	// Gives the cursor position given a x-axix point in the text
	int TextCursorAt(const char *text, int px);
	int CleverTextCursorAt(const char *text, int px, int cursPos,
			SDL_Rect *r);

	int GetMinChar()	{ return START_CHAR; }
	int GetMaxChar()	{ return max_i; }

	void ExtraSpace(int xs)	{ xspace = xs; }
	void TabSize(int ts)	{ tabsize = ts; }

	SDL_Texture *GetGlyphs()	{ return glyphs; }

protected:
	SDL_Renderer *target;
	SDL_Texture *glyphs;
	int height;
	int *CharPos;
	int *CharOffset;
	int *Spacing;
	int xspace;
	int max_i, spacew, cursShift;
	int tabsize;
	Uint32 background;
	int xscale, yscale;
	bool DoStartNewChar(SDL_Surface *surface, Sint32 x);
	void CleanSurface(SDL_Surface *surface);
};

#endif
