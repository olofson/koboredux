/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2007, 2009 David Olofson
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

#ifndef _KOBO_SCREEN_H_
#define _KOBO_SCREEN_H_

#include "map.h"
#include "radar.h"
#include "starfield.h"

class window_t;

class _screen
{
  protected:
	static window_t *target;
	static int scene_max;
	static int scene_num;
	static int region;
	static int level;
	static int bg_altitude;
	static int bg_backdrop;
	static int bg_clouds;
	static int bg_planet;
	static int restarts;
	static int generate_count;
	static _map map;
	static _map bg_map[KOBO_BG_MAP_LEVELS];
	static int show_title;
	static int do_noise;
	static float _fps;
	static float scroller_speed;
	static float target_speed;
	static int noise_y;
	static int noise_h;
	static int noise_source;
	static float noise_fade;
	static float noise_bright;
	static float noise_depth;
	static int highlight_y;
	static int highlight_h;
	static int hi_sc[10];
	static int hi_st[10];
	static char hi_nm[10][20];

	static KOBO_Starfield stars;

	static void render_noise();
	static void render_highlight();
	static void render_title_plasma(int t, float fade, int y, int h);
	static void render_title_noise(float fade, int y, int h, int bank, int frame);
	static void init_starfield(int altitude, int psize);
	static void init_starfield_colors();
	static void init_background();
	static void render_starfield(int xo, int yo);
	static void render_bases(_map &map, int tileset, int vx, int vy);
	static void clean_scrap_tile(int x, int y)
	{
		if((map.pos(x, y) & SPACE) && (MAP_TILE(map.pos(x, y))))
			set_map(x, y, SPACE);
	}
  public:
	~_screen();
	static radar_modes_t radar_mode;	// Last set radar mode
	static void init_maps();
	static void init_graphics();
	static void init_scene(int sc);
	static int prepare();
	static void generate_fixed_enemies();
	static inline int get_map(int x, int y)
	{
		return map.pos(x, y);
	}
	static void set_map(int x, int y, int n);
	static void clean_scrap(int x, int y)
	{
		clean_scrap_tile(x + 1, y);
		clean_scrap_tile(x - 1, y);
		clean_scrap_tile(x, y + 1);
		clean_scrap_tile(x, y - 1);
		clean_scrap_tile(x, y);
	}
	static void set_highlight(int y, int h);
	static void set_noise(int source, float fade, float bright, float depth);
	static void render_background();
	static void render_fx();
	static void title(int t, float fade, int mode);
	static void init_highscores();
	static void highscores(int t, float fade);
	static void credits(int t);
	static void help(int t);
	static void scroller();
	static void fps(float f);
	static float fps()	{ return _fps; }
	static void noise(int on);
};

extern _screen screen;

#endif	//_KOBO_SCREEN_H_
