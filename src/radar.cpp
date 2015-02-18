/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2005-2007, 2009 David Olofson
 * Copyright 2015 David Olofson (Kobo Redux)
 * 
 * This program  is free software; you can redistribute it and/or modify it
 * under the terms  of  the GNU General Public License  as published by the
 * Free Software Foundation;  either version 2 of the License,  or (at your
 * option) any later version.
 *
 * This program is  distributed  in  the hope that  it will be useful,  but
 * WITHOUT   ANY   WARRANTY;   without   even   the   implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received  a copy of the GNU General Public License along
 * with this program; if not,  write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdlib.h>
#include <math.h>

#include "kobo.h"
#include "kobolog.h"
#include "screen.h"
#include "myship.h"
#include "radar.h"
#include "prefs.h"
#include "random.h"

/*---------------------------------------------------------------------
	radar_map_t
---------------------------------------------------------------------*/
radar_map_t::radar_map_t(gfxengine_t *e) : window_t(e)
{
	w = MAP_SIZEX;
	h = MAP_SIZEY;
	pixel_core = 0;
	pixel_launcher = 0;
	pixel_hard = 0;
	pixel_bg = 0;
}


void radar_map_t::update(int x, int y, int draw_space)
{
	int a = MAP_BITS(screen.get_map(x, y));
	if(IS_SPACE(a))
	{
		if(!draw_space)
			return;
		foreground(pixel_bg);
	}
	else if(a & CORE)
		foreground(pixel_core);
	else if((a == U_MASK) || (a == R_MASK) || (a == D_MASK) ||
			(a == L_MASK))
		foreground(pixel_launcher);
	else if(a & HIT_MASK)
		foreground(pixel_hard);
	point(x, y);
}


void radar_map_t::refresh(SDL_Rect *r)
{
	int draw_space = 1;
	SDL_Rect nr;
	if(!r)
	{
		nr.x = 0;
		nr.y = 0;
		nr.w = w;
		nr.h = h;
		r = &nr;
		clear(r);
		draw_space = 0;
	}
	int i, j;
	for(i = 0; i < r->w; i++)
		for(j = 0; j < r->h; j++)
			update(r->x + i, r->y + j, draw_space);
}


/*---------------------------------------------------------------------
	radar_window_t
---------------------------------------------------------------------*/
radar_window_t::radar_window_t(gfxengine_t *e) : window_t(e)
{
	_mode = RM_OFF;
	old_scrollradar = -1;
	xpos = -1;
	ypos = -1;
	xoffs = 0;
	yoffs = 0;
	time = 0;
}


void radar_window_t::update(int mx, int my)
{
	SDL_Rect r;
	r.x = mx & (MAP_SIZEX - 1);
	r.y = my & (MAP_SIZEY - 1);
	r.w = r.h = 1;
	wmap->invalidate(&r);
}


void radar_window_t::refresh(SDL_Rect *r)
{
	switch(_mode)
	{
	  case RM__REINIT:
	  case RM_OFF:
	  case RM_NOISE:
	  case RM_INFO:
		clear(r);
		break;
	  case RM_SHOW:
		blit(0, 0, wmap);
		break;
	  case RM_RADAR:
		if(prefs->scrollradar)
		{
			blit(-xoffs, MAP_SIZEY - yoffs, wmap);
			blit(-xoffs, -yoffs, wmap);
			blit(MAP_SIZEX - xoffs, MAP_SIZEY - yoffs, wmap);
			blit(MAP_SIZEX - xoffs, -yoffs, wmap);
		}
		else
			blit(0, 0, wmap);
		int t = SDL_GetTicks();
		foreground(map_rgb(engine->palette(28 + t / 100 % 8)));
		point((xpos - pxoffs) & (MAP_SIZEX - 1),
				(ypos - pyoffs) & (MAP_SIZEY - 1));
		break;
	}
}


void radar_window_t::mode(radar_modes_t newmode)
{
	if(newmode == RM__REINIT)
		newmode = _mode;
	wmap->pixel_core = map_rgb(engine->palette(29));
	wmap->pixel_hard = map_rgb(engine->palette(33));
	wmap->pixel_launcher = map_rgb(engine->palette(31));
	wmap->pixel_bg = map_rgb(engine->palette(0));
	wmap->background(wmap->pixel_bg);
	_mode = newmode;
	time = SDL_GetTicks();
	wmap->invalidate();
	invalidate();
}


void radar_window_t::set_scroll(int xs, int ys)
{
	xoffs = (xs + MAP_SIZEX / 2) & (MAP_SIZEX - 1);
	yoffs = (ys + MAP_SIZEY / 2) & (MAP_SIZEY - 1);
}


void radar_window_t::radar()
{
	int xpos_new = (myship.get_x() & (WORLD_SIZEX - 1)) >> 4;
	int ypos_new = (myship.get_y() & (WORLD_SIZEY - 1)) >> 4;
	if((xpos_new == xpos) && (ypos_new == ypos))
		return;
	if(prefs->scrollradar)
	{
		// Scrolling
		xpos = xpos_new;
		ypos = ypos_new;
		set_scroll(xpos, ypos);
		pxoffs = xoffs;
		pyoffs = yoffs;
	}
	else
	{
		// Fixed
		xpos = xpos_new;
		ypos = ypos_new;
		pxoffs = pyoffs = xoffs = yoffs = 0;
	}
}


void radar_window_t::frame()
{
	if(prefs->scrollradar != old_scrollradar)
	{
		old_scrollradar = prefs->scrollradar;
		time = SDL_GetTicks();
		pxoffs = pyoffs = xoffs = yoffs = 0;
	}
	switch(_mode)
	{
	  case RM__REINIT:
	  case RM_OFF:
		break;
	  case RM_RADAR:
		radar();
		break;
	  case RM_SHOW:
		break;
	  case RM_NOISE:
	  case RM_INFO:
		clear();
		break;
	}
}
