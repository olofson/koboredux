/*(LGPLv2.1)
----------------------------------------------------------------------
	Simple display for score, lives etc.
----------------------------------------------------------------------
 * Copyright (C) 2001, 2009 David Olofson
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

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "window.h"

class display_t : public window_t
{
	char	_caption[64];
	char	_text[64];
	int	_on;
	Uint32	_color;
	void render_caption();
	void render_text();
  public:
	display_t();
	void refresh(SDL_Rect *r);
	void color(Uint32 _cl);
	void caption(const char *cap);
	void text(const char *txt);
	void on();
	void off();
};

#endif /* _WINDOW_H_ */
