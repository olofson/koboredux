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

#define	STAR_COLORS	16
#define	STAR_ZBITS	10
#define	STAR_Z0		256

class window_t;

struct KOBO_Star
{
	short		x;
	short		y;
	unsigned short	z;
};

class _screen
{
  protected:
	static int scene_num;
	static int level;
	static int generate_count;
	static _map map;
	static int scene_max;
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
	static int nstars;
	static KOBO_Star *stars;
	static Uint32 starcolors[STAR_COLORS];
	static int star_oxo;
	static int star_oyo;
	static void render_noise(window_t *win);
	static void render_highlight(window_t *win);
	static void render_title_plasma(int t, float fade, int y, int h);
	static void render_title_noise(float fade, int y, int h, int bank, int frame);
	static void render_starfield(window_t *win, int xo, int yo);
  public:
	~_screen();
	static radar_modes_t radar_mode;	// Last set radar mode
	static void init();
	static void init_scene(int sc);
	static int prepare();
	static void generate_fixed_enemies();
	static inline int get_map(int x, int y)
	{
		return map.pos(x, y);
	}
	static void set_map(int x, int y, int n);
	static void clean_scrap(int x, int y);
	static void set_highlight(int y, int h);
	static void set_noise(int source, float fade, float bright, float depth);
	static void render_background(window_t * win);
	static void render_fx(window_t * win);
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
