/*(LGPLv2.1)
----------------------------------------------------------------------
	Simple display for score, lives etc.
----------------------------------------------------------------------
 * Copyright (C) 2001-2003, 2007, 2009 David Olofson
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

#define	D_LINE_HEIGHT	9

#define	D_LINE1_POS	1
#define	D_LINE1_TXOFFS	0

#define	D_LINE2_POS	9
#define	D_LINE2_TXOFFS	0

#include <string.h>

#include "window.h"
#include "display.h"

display_t::display_t()
{
	_on = 0;
	caption("CAPTION");
	text("TEXT");
	on();
}


void display_t::color(Uint32 _cl)
{
	_color = _cl;
	if(_on)
		invalidate();
}


void display_t::caption(const char *cap)
{
	strncpy(_caption, cap, sizeof(_caption));
	if(_on)
		invalidate();
}


void display_t::text(const char *txt)
{
	strncpy(_text, txt, sizeof(_text));
	if(_on)
		invalidate();
}


void display_t::on()
{
	if(_on)
		return;

	_on = 1;
	invalidate();
}


void display_t::off()
{
	if(!_on)
		return;

	_on = 0;
	invalidate();
}


void display_t::render_caption()
{
	SDL_Rect r;
	r.x = 0;
	r.y = D_LINE1_POS;
	r.w = width();
	r.h = D_LINE_HEIGHT;
	background(_color);
	clear(&r);
	if(_on)
		center(D_LINE1_POS + D_LINE1_TXOFFS, _caption);
}


void display_t::render_text()
{
	SDL_Rect r;
	r.x = 0;
	r.y = D_LINE2_POS;
	r.w = width();
	r.h = D_LINE_HEIGHT;
	background(_color);
	clear(&r);
	if(_on)
		center(D_LINE2_POS + D_LINE2_TXOFFS, _text);
}


void display_t::refresh(SDL_Rect *r)
{
	render_caption();
	render_text();
}
