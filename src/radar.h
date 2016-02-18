/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2005-2007, 2009 David Olofson
 * Copyright 2015-2016 David Olofson (Kobo Redux)
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

enum KOBO_radar_modes
{
	RM__REINIT,	// Reinitialize (lost windows etc)
	RM_OFF,		// Gray, empty
	RM_RADAR,	// Radar display
	RM_SHOW,	// Show map (no player ship cursor)
	RM_NOISE,	// Radar interference
	RM_INFO		// Scrolling info display
};

// The off-screen map window
class KOBO_radar_map : public window_t
{
  public:
	int w, h;			//Map size (tiles)
	Uint32 pixel_core;		//Pixel colors
	Uint32 pixel_launcher;
	Uint32 pixel_hard;
	Uint32 pixel_bg;
	KOBO_radar_map(gfxengine_t *e);
	void refresh(SDL_Rect *r);
	void update(int x, int y, int draw_space);	//Update one tile
};

// The on-screen radar window
class KOBO_radar_window : public window_t
{
	KOBO_radar_modes _mode;
	int old_scrollradar;		//To detect prefs change
	int xpos, ypos;			//Player position (tiles)
	int xoffs, yoffs;		//Scroll offset (tiles)
	int pxoffs, pyoffs;		//Scroll offset for player marker
	int platched;			//p*offset latched yet?
	int time;			//for delta time calc
	void radar();			//Drive any actual radar mode
	void noise();			//Render noise effect
	void set_scroll(int xscroll, int yscroll);	//Set map scroll offset
  public:
	KOBO_radar_window(gfxengine_t *e);
	void refresh(SDL_Rect *r);
	void mode(KOBO_radar_modes newmode);//Set radar mode
	void update(int mx, int my);	//Update map + radar
	void frame();			//Track player, drive logic etc...
};

#endif // KOBO_RADAR_H
