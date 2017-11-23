/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2007, 2009 David Olofson
 * Copyright 2015-2017 David Olofson (Kobo Redux)
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
#include "gridtfx.h"

class window_t;

class KOBO_screen
{
  protected:
	static window_t *target;
	static int stage;
	static int region;
	static int level;
	static const KOBO_scene *scene;
	static int bg_altitude;
	static int bg_backdrop;
	static int bg_clouds;
	static int bg_planet;
	static int restarts;
	static int generate_count;
	static KOBO_map map[KOBO_BG_MAP_LEVELS + 1];
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

	static float fade_level;
	static float fade_target;
	static Uint32 fade_time;

	static int highlight_y;
	static int highlight_h;
	static KOBO_ParticleFXDef highlight_fxd;
	static Uint32 highlight_time;

	static int hi_sc[10];
	static int hi_st[10];
	static char hi_nm[10][20];
	static int long_credits_wrap;
	static KOBO_Starfield stars;

	static KOBO_GridTFX gridtfx;
	static bool curtains_below;

	static void render_noise();
	static void render_highlight();
	static void render_bases(KOBO_map &map, int tileset, int vx, int vy);
	static void clean_scrap_tile(int x, int y)
	{
		if((map[0].pos(x, y) & SPACE) && (MAP_TILE(map[0].pos(x, y))))
			set_map(x, y, SPACE);
	}
	static void render_anim(int x, int y, int bank, int first, int last,
			float speed, int t);
  public:
	static void init_graphics();
	static void init_background();
	static void init_stage(int st, bool ingame);
	static int prepare();
	static void generate_wave(const KOBO_enemy_set *wave);
	static void generate_fixed_enemies();
	static inline int get_map(int x, int y)
	{
		return map[0].pos(x, y);
	}
	static inline int test_line(int x1, int y1, int x3, int y3,
			int *x2, int *y2, int *hx, int *hy)
	{
		return map[0].test_line(x1, y1, x3, y3, x2, y2, hx, hy);
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
	static void set_highlight();
	static void set_noise(int source, float fade, float bright,
			float depth);
	static void set_fade(float ft)	{ fade_target = ft; }
	static void render_background();
	static void render_fx();
	static void title(int t, float fade);
	static void credits(int t);
	static void long_credits(int t);
	static void help(int t);
	static void fps(float f);
	static float fps()	{ return _fps; }
	static void noise(int on);
	static void curtains(bool st, float dur = 1.0f, bool on_top = false);
	static bool curtains();
	static void render_curtains();
	static void render_countdown(int y, float t, int timeout,
			int countdown);
};

extern KOBO_screen screen;

#endif	//_KOBO_SCREEN_H_
