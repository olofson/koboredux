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
*/

#ifndef __SOFONT_H
#define __SOFONT_H

#include "glSDL.h"

class SoFont
{
public:
	SoFont();
	~SoFont();

	bool load(SDL_Surface *FontSurface);

	// Blits a string to a surface
	//   Destination: the suface you want to blit to
	//   text: a string containing the text you want to blit.
	void PutString(SDL_Surface *Surface, int x, int y, const char *text, SDL_Rect *clip=NULL);
	void PutStringWithCursor(SDL_Surface *Surface, int x, int y, const char *text, int cursPos, SDL_Rect *clip=NULL, bool showCurs=true);

	// Returns the width of "text" in pixels
	int TextWidth(const char *text, int min=0, int max=255);

	int FontHeight()	{ return height; }

	// Blits a string to with centered x position
	void XCenteredString(SDL_Surface *Surface, int y, const char *text, SDL_Rect* clip=NULL);
	// Blits a string to with centered x & y position
	void CenteredString(SDL_Surface *Surface, const char *text, SDL_Rect* clip=NULL);
	// Blits a string to with centered around x & y positions
	void CenteredString(SDL_Surface *Surface, int x, int y, const char *text, SDL_Rect* clip=NULL);
	
	// This was specially developped for GUI
	void PutStringCleverCursor(SDL_Surface *Surface, const char *text, int cursPos, SDL_Rect *r, SDL_Rect* clip=NULL, bool showCurs=true);
	
	// Gives the cursor position given a x-axix point in the text
	int TextCursorAt(const char *text, int px);
	int CleverTextCursorAt(const char *text, int px, int cursPos, SDL_Rect *r);
	
#	define START_CHAR 33
	int getMinChar(){return START_CHAR;}
	int getMaxChar(){return max_i;}

	void ExtraSpace(int xs)
	{
		xspace = xs;
	}

protected:
	int height;
	SDL_Surface *picture;
	int *CharPos;
	int *CharOffset;
	int *Spacing;
	int xspace;
	
	int max_i, spacew, cursShift;
	Uint32 background;
	bool DoStartNewChar(Sint32 x);
	void CleanSurface();
};

#endif
