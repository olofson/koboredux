/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996 Akira Higuchi
 * Copyright (C) 2001-2003, 2005-2007, 2009 David Olofson
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

#ifndef KOBO_RADAR_H
#define KOBO_RADAR_H

#include "window.h"

enum radar_modes_t
{
	RM__REINIT,	// Reinitialize (lost windows etc)
	RM_OFF,		// Gray, empty
	RM_RADAR,	// Radar display
	RM_SHOW,	// Show map (no player ship cursor)
	RM_NOISE,	// Radar interference
	RM_INFO		// Scrolling info display
};

// The off-screen map window
class radar_map_t : public window_t
{
  public:
	int w, h;			//Map size (tiles)
	Uint32 pixel_core;		//Pixel colors
	Uint32 pixel_launcher;
	Uint32 pixel_hard;
	Uint32 pixel_bg;
	radar_map_t();
	void refresh(SDL_Rect *r);
	void update(int x, int y, int force);	//Update one tile
};

// The on-screen radar window
class radar_window_t : public window_t
{
	radar_modes_t _mode;
	int old_scrollradar;		//To detect prefs change
	int xpos, ypos;			//Player position (tiles)
	int xoffs, yoffs;		//Scroll offset (tiles)
	int pxoffs, pyoffs;		//Scroll offset for player marker
	int platched;			//p*offset latched yet?
	int time;			//for delta time calc
	int refresh_pos;		//Sweeping refresh posn
	void sweep();			//Incremental sweeping refresh
	void radar();			//Drive any actual radar mode
	void noise();			//Render noise effect
	void set_scroll(int xs, int ys);//Set map scroll offset
  public:
	radar_window_t();
	void refresh(SDL_Rect *r);
	void mode(radar_modes_t newmode);//Set radar mode
	void update(int mx, int my);	//Update map + radar
	void update_player(int px, int py);//Update player cursor
	void frame();			//Track player, drive logic etc...
};

#endif // KOBO_RADAR_H
