/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2003, 2007, 2009 David Olofson
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

#ifndef KOBO_DASHBOARD_H
#define KOBO_DASHBOARD_H

#include "window.h"

/* "Screen" window; takes care of the border, if any */
class screen_window_t : public window_t
{
	int	_top, _left, _right, _bottom;
  public:
	screen_window_t(gfxengine_t *e) : window_t(e) { }
	void border(int top, int left, int right, int bottom);
	void refresh(SDL_Rect *r);
};


enum dashboard_modes_t {
	DASHBOARD_OFF = 0,
	DASHBOARD_BLACK,
	DASHBOARD_GAME,
	DASHBOARD_LOADING
};

/* Dashboard window; dashboard or loading screen */
class dashboard_window_t : public window_t
{
	char			*_msg;
	float			_percent;
	dashboard_modes_t	_mode;
	int			progress_index;
	float			*progress_table;
	Uint32			progress_time;
	int			progress_bench;
	void render_progress();
  public:
	dashboard_window_t(gfxengine_t *e);
	~dashboard_window_t();
	void mode(dashboard_modes_t m);
	void doing(const char *msg);
	void progress_init(float *progtab);
	void progress();
	void progress_done();
	void refresh(SDL_Rect *r);
};

/* Labeled text display */
class display_t : public window_t
{
	char	_caption[64];
	char	_text[64];
	int	_on;
	Uint32	_color;
	void render_caption();
	void render_text();
  public:
	display_t(gfxengine_t *e);
	void refresh(SDL_Rect *r);
	void color(Uint32 _cl);
	void caption(const char *cap);
	void text(const char *txt);
	void on();
	void off();
};

/* Bar graph display */
class bargraph_t : public window_t
{
	float	_value;
	int	_redmax;
	int	_y;
	int	_enabled;
  public:
	bargraph_t(gfxengine_t *e);
	void value(float val);
	void refresh(SDL_Rect *r);
	void redmax(int rm)
	{
		_redmax = rm;
	}
	void enable(int ena);
};

#endif /* KOBO_DASHBOARD_H */
