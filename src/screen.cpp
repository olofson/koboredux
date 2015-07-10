/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2005, 2007, 2009 David Olofson
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

#include <math.h>
#ifndef M_PI
# define M_PI 3.14159265358979323846	/* pi */
#endif

#include "kobo.h"
#include "screen.h"
#include "manage.h"
#include "options.h"
#include "enemies.h"
#include "myship.h"
#include "radar.h"
#include "scenes.h"
#include "config.h"
#include "random.h"

int _screen::scene_max;
int _screen::scene_num;
int _screen::region;
int _screen::level;
int _screen::bg_altitude;
int _screen::bg_backdrop;
int _screen::bg_clouds;
int _screen::restarts;
int _screen::generate_count;
_map _screen::map;
int _screen::show_title = 0;
int _screen::do_noise = 0;
float _screen::_fps = 40;
float _screen::scroller_speed = SCROLLER_SPEED;
float _screen::target_speed = SCROLLER_SPEED;
int _screen::noise_y = 0;
int _screen::noise_h = WMAIN_H;
int _screen::noise_source = B_NOISE;
float _screen::noise_fade = 0.0f;
float _screen::noise_bright = 0.0f;
float _screen::noise_depth = 0.0f;
int _screen::highlight_y = WMAIN_H / 2;
int _screen::highlight_h = 0;
int _screen::hi_sc[10];
int _screen::hi_st[10];
char _screen::hi_nm[10][20];
int _screen::nstars = 0;
int _screen::star_alt;
int _screen::star_psize;
int _screen::star_pscale;
KOBO_Star *_screen::stars = NULL;
Uint32 _screen::starcolors[STAR_COLORS];
int _screen::star_oxo = 0;
int _screen::star_oyo = 0;
radar_modes_t _screen::radar_mode = RM_OFF;


_screen::~_screen()
{
	free(stars);
}


void _screen::init_maps()
{
	scene_max = 0;
	while(scene[scene_max].ratio != -1)
		scene_max++;
}


void _screen::init_graphics()
{
	init_starfield_colors();
}


void _screen::render_title_plasma(int t, float fade, int y, int h)
{
#if 0
	s_sprite_t *s = gengine->get_sprite(B_FOCUSFX, 0);
	if(!s || !s->surface)
		return;
	SDL_Surface *fx = s->surface;
	SDL_Rect sr;
	sr.x = 0;
	sr.w = fx->w;
	sr.h = 1;
	for(int ty = 0; ty < h; ++ty)
	{
		float sy = (float)ty / (h - 1);
		float shape = sin(M_PI * sy);
		float plasma = 0.5f + 0.5f * sin(t * 0.004f +
				sin(t * 0.00017f) * sy * 18.0f);
		float plasma2 = 0.5f + 0.5f * sin(t * 0.001f +
				sin(t * 0.00013f) * sy * 12.0f);
		int i = (int)((fx->h - 1) * (0.3f * plasma + 0.7f * shape) * fade);
		int xo = (int)(8192 * plasma2 * gengine->xscale() / 256);
		xo -= (int)(xo / fx->w) * fx->w;
		int xmax = woverlay->phys_rect.x + woverlay->phys_rect.w;
		for(int x = -xo; x < xmax; x += fx->w)
		{
			sr.y = i;
			RGN_Blit(fx, &sr, x, y + ty);
		}
	}
#endif
}


void _screen::render_title_noise(float fade, int y, int h, int bank, int frame)
{
#if 0
	for(int ty = 0; ty < h; )
	{
		int xo = (int)(pubrand.get(16) * gengine->xscale()) / 256;
		s_sprite_t *s = gengine->get_sprite(bank, pubrand.get(4));
		if(!s || !s->surface)
			continue;
		SDL_Surface *fx = s->surface;
		xo -= (int)(xo / fx->w) * fx->w;
		int xmax = woverlay->phys_rect.x + woverlay->phys_rect.w;
		for(int x = -xo; x < xmax; x += fx->w)
			RGN_Blit(fx, NULL, x, y + ty);
		ty += fx->h;
	}
#endif
}


static int flashin(int t)
{
	if(t < 0)
		return 1;
	else if(t > 800)
		return 0;
	else
		return (t % 200) > 100;
}


void _screen::title(int t, float fade, int mode)
{
	s_bank_t *b = s_get_bank(gfxengine->get_gfx(), B_LOGO);
	if(!b)
		return;
	s_sprite_t *s = s_get_sprite_b(b, 0);
	if(!s || !s->texture)
		return;

#if 1
	float mf = (1.0f - fade);
	float ly = mf * mf * mf * WMAIN_H - 0.5f;
	if(ly < 0.0f)
		ly = 0.0f;
	woverlay->sprite_fxp(PIXEL2CS(woverlay->width() - 320) / 2,
			(int)((woverlay->height()  / 2 - (128 - 20) - ly) *
					256.0f),
			B_LOGO, 0);
#elif 1
	woverlay->alphamod(fade * fade * fade);
	woverlay->sprite_fxp(PIXEL2CS(woverlay->width() - 320) / 2,
			PIXEL2CS(woverlay->height() / 2 - (128 - 20)),
			B_LOGO, 0);
	woverlay->alphamod(1.0f);
#else
	woverlay->sprite_fxp_scale(PIXEL2CS(woverlay->width() - 320) / 2,
			PIXEL2CS(woverlay->height() / 2 - (128 - 20)),
			B_LOGO, 0, 1.0f, fade);
#endif
	// Version
	if(fade > 0.9)
	{
		woverlay->font(B_NORMAL_FONT);
		woverlay->center(195, KOBO_VERSION);
	}

	// Cheat mode warning
	if((prefs->cmd_cheat || prefs->cmd_pushmove) && (t % 1000 < 500))
	{
		woverlay->font(B_MEDIUM_FONT);
		woverlay->center(210, "CHEAT MODE");
	}
#if 1
	// WIP notice
	if(!flashin(t - 2000))
	{
		woverlay->font(B_MEDIUM_FONT);
		woverlay->center(230, "This is a Work in Progress!");
		woverlay->center(240, "Check http://koboredux.com");
	}
#endif
}


void _screen::init_highscores()
{
	for(unsigned int i = 0; i < 10; ++i)
		if(i < scorefile.highs)
		{
			hi_sc[i] = scorefile.high_tbl[i].score;
			hi_st[i] = scorefile.high_tbl[i].end_scene;
			strncpy(hi_nm[i], scorefile.high_tbl[i].name, 19);
			hi_nm[i][19] = 0;
		}
		else
		{
			hi_sc[i] = 0;
			hi_st[i] = 0;
			strcpy(hi_nm[i], "---");
		}

}


void _screen::highscores(int t, float fade)
{
	int i, y;
	float mf = (1.0f - fade);
	mf *= mf * mf * mf * mf;
	screen.set_highlight(0, 0);

	woverlay->font(B_BIG_FONT);
	y = (int)(PIXEL2CS(100) * mf) - 256;
	if(y < 0)
		y = 0;
	woverlay->center_fxp(PIXEL2CS(20) - y, "HALL OF FAME");
	int xo = (int)(50 * 256 * mf);
	woverlay->sprite_fxp(PIXEL2CS(32) - xo, PIXEL2CS(30),
			B_PLAYER, (t / 50) % 16);
	woverlay->sprite_fxp(PIXEL2CS(woverlay->width() - 32) + xo,
			PIXEL2CS(30), B_PLAYER, 15 - (t / 50) % 16);

	woverlay->font(B_NORMAL_FONT);
	y = (int)(PIXEL2CS(75) * mf - 0.5f) - 256;
	if(y < 0)
		y = 0;
	woverlay->center_fxp(PIXEL2CS(40) - y, "(Score/Stage/Name)");

	woverlay->font(B_MEDIUM_FONT);
	float yo = t * (11*18+100) * 256.0 / 12500.0 - PIXEL2CS(110);
	for(i = 0, y = 65; i < 10; ++i, y += 18)
	{
		static char s[20];
		float xo, cy;
		int real_y = PIXEL2CS(y) - (int)yo;
		if(real_y < PIXEL2CS(55) || real_y > PIXEL2CS(165))
			continue;
		cy = (PIXEL2CS(y) - yo) - PIXEL2CS(65 + 5*18/2);
		xo = cy*cy*cy*cy*cy * 1e-16;
		snprintf(s, 16, "%d", hi_sc[i]);
		woverlay->center_token_fxp(PIXEL2CS(70) + (int)xo, real_y, s);
		snprintf(s, 16, "%d", hi_st[i]);
		woverlay->center_token_fxp(PIXEL2CS((70 + 125) / 2) + (int)xo,
				real_y, s, -1);
		woverlay->string_fxp(PIXEL2CS(125) + (int)xo, real_y,
				hi_nm[i]);
	}
#if 0
	if(!flashin(t - 1000))
	{
		woverlay->font(B_MEDIUM_FONT);
		woverlay->center(170,
				"NOTE: Highscores for \"Classic\" only!");
		woverlay->center(180,
				"The new skill levels are experimental.");
	}
#endif
}


void _screen::credits(int t)
{
	if((t % 4000 < 3600) && (t / 4000 <= 1))
		screen.set_highlight(102, 70);
	else
		screen.set_highlight(0, 0);
	int t2 = 10 * (t % 4000) / 3700;
	if(t % 4000 < 3700)
		switch(t / 4000)
		{
		  case 0:
			woverlay->font(B_BIG_FONT);
			if(t2 > 0)
				woverlay->center(80, "DAVID OLOFSON");
			woverlay->font(B_NORMAL_FONT);
			if(t2 > 2)
				woverlay->center(105,
						"New Audio & GFX Engines");
			if(t2 > 3)
				woverlay->center(115,
						"Graphics, Sound & Music");
			break;
		  case 1:
			woverlay->font(B_BIG_FONT);
			if(t2 > 0)
				woverlay->center(80, "AKIRA HIGUCHI");
			woverlay->font(B_NORMAL_FONT);
			if(t2 > 2)
				woverlay->center(110,
						"XKobo - The Original Game");
			break;
		}
	if(!flashin(t - 7000))
	{
		woverlay->font(B_MEDIUM_FONT);
		woverlay->center(170, "Additional Credits & Thanks");
		woverlay->center(180, "in the scroller below");
	}
}


void _screen::help(int t)
{
	woverlay->font(B_BIG_FONT);
	woverlay->center(53, "HOW TO PLAY");

	// 0..3000: Control
	if(t < 200)
		screen.set_highlight(112, 80);
	else if(t < 2800)
	{
		if(t > 2700)
			screen.set_highlight(0, 0);
		woverlay->font(B_MEDIUM_FONT);
		woverlay->sprite(woverlay->width() / 2, 105, B_PLAYER,
				(t / 50) % 16);
		woverlay->center(125, "Use arrow keys or NumPad.");
		woverlay->center(135, "to control the ship.");
	}
	else if(t < 3000)
		return;

	// 3000..6000: Fire
	else if(t < 3200)
		screen.set_highlight(112, 80);
	else if(t < 5800)
	{
		if(t > 5700)
			screen.set_highlight(0, 0);
		woverlay->font(B_MEDIUM_FONT);
		int i;
		for(i = 0; i < 4; ++i)
		{
			int t2 = t + 153 * i;
			woverlay->sprite(woverlay->width() / 2 - (t2 / 5) % 120,
					105, B_BOLT, 8 + (t2 / 50) % 4);
			woverlay->sprite(
					woverlay->width() / 2 + (t2 / 5) % 120,
					105, B_BOLT, 8 + (t2 / 50) % 4);
		}
		woverlay->sprite(woverlay->width() / 2, 105, B_PLAYER, 4);
		woverlay->center(135,
				"Use any SHIFT or CONTROL keys to fire.");
	}
	else if(t < 6000)
		return;
	
	// 6000..9000: Bases
	else if(t < 6200)
		screen.set_highlight(112, 80);
	else if(t < 8800)
	{
		if(t > 8700)
			screen.set_highlight(0, 0);
		woverlay->font(B_MEDIUM_FONT);
// TODO: Short demo of how to destroy a base
		int i;
		for(i = B_R1_TILES; i <= B_R5_TILES; ++i)
		{
			int xo = 24 * i - (5 * 24 / 2) + 4;
			woverlay->sprite(woverlay->width() / 2 + xo, 105 - 8,
					i, 7);
		}
		woverlay->center(135,
				"Destroy bases by shooting their cores.");
	}
	else if(t < 9000)
		return;
	
	// 9000..14000: Shoot everything!
	else if(t < 9200)
		screen.set_highlight(112, 80);
	else if(t < 13800)
	{
		if(t > 13700)
			screen.set_highlight(0, 0);
		woverlay->font(B_MEDIUM_FONT);
// TODO: Short demo of intense battle
		woverlay->sprite(50, 110, B_RING, (t / 30) % 16);
		woverlay->sprite(65, 90, B_BOMB, (t / 40) % 16);
		woverlay->sprite(80, 110, B_BMR_GREEN, (t / 80) % 16);
		woverlay->sprite(95, 90, B_BMR_PURPLE, (t / 70) % 16);
		woverlay->sprite(110, 110, B_BMR_PINK, (t / 60) % 16);
		woverlay->sprite(125, 90, B_FIGHTER, (t / 50) % 16);
		woverlay->sprite(140, 110, B_MISSILE1, (t / 45) % 16);
		woverlay->sprite(155, 90, B_MISSILE2, (t / 40) % 16);
		woverlay->sprite(170, 110, B_MISSILE3, (t / 55) % 16);
		woverlay->center(125, "Shoot everything that moves,");
		woverlay->center(135, "but avoid getting hit!");
	}
	else if(t < 14000)
		return;
	
	// 14000..20000: Indestructible
	else if(t < 14200)
		screen.set_highlight(112, 80);
	else if(t < 19800)
	{
		if(t > 19700)
			screen.set_highlight(0, 0);
		woverlay->font(B_MEDIUM_FONT);
// TODO: Demo destroying a rock
		int i;
		for(i = B_R1_TILES; i <= B_R2_TILES; ++i)
		{
			int xo = 24 * i - (5 * 24 / 2) + 4;
			woverlay->sprite(woverlay->width() / 2 + xo, 82 - 8,
					i, 0);
		}
		woverlay->center(87, "Some objects are indestructible...");
		woverlay->sprite(65, 120, B_ROCK1, (t / 45) % 32);
		woverlay->sprite(90, 115, B_ROCK2, (t / 40) % 32);
		woverlay->sprite(115, 125, B_ROCK3, (t / 35) % 48);
		woverlay->sprite(155, 120, B_BIGSHIP, (t / 40) % 16);
		woverlay->center(135, "...or take many hits to destroy.");
	}
}


void _screen::scroller()
{
	if(do_noise)
		return;

	/*
	 * Adjust scroller speed according to
	 * frame rate, for readability.
	 */
	if(_fps < 30)
		target_speed = SCROLLER_SPEED / 2;
	else if(_fps > 40)
		target_speed = SCROLLER_SPEED;

	scroller_speed += (target_speed - scroller_speed) * 0.05;

	static const char scrolltext[] =
			"Welcome to KOBO REDUX, a revival of the old "
			"KOBO DELUXE, which was an enhanced version of "
			"Akira Higuchi's fabulous X-Window game XKOBO. "
			"     "
			"KOBO REDUX uses SDL 2, the Simple DirectMedia Layer "
			"(www.libsdl.org) for graphics and input, and "
			"Audiality 2 (audiality.org) for sound and music. "
			"     "
			"KOBO DELUXE has been known to hinder productivity on: "
			"     "
			"  -  "
			"Windows 95/98/ME  -  "
			"Windows 2000/XP  -  "
			"Mac OS X (PPC)  -  "
			"BeOS (x86, PPC)  -  "
			"AmigaOS (68k, PPC)  -  "
			"Solaris (x86, SPARC)  -  "
			"QNX (x86)  -  "
			"GNU/Linux (x86, x86_64, PPC, PPC64)  -  "
			"OpenBSD (x86, PPC, SPARC, SPARC64)  -  "
			"FreeBSD (x86)  -  "
			"NetBSD (x86)  -  "
			"IRIX  -  "
			"OS/2  -  "
			"PlayStation 2/PS2Linux -  "
			"Xbox  -  "
			"Syllable  -  "
			"ITOS (Nokia internet tablets)  -  "
			"     "
			"Any help in the Cause of Infiltrating Further "
			"Platforms is Greatly Appreciated! "
			"                        "
			"Additional Credits & Thanks to: "
			"      Torsten Giebl (Slackware)"
			"      David Andersson (Some Good Ideas)"
			"      Samuel Hart (Joystick Support)"
			"      Max Horn (Mac OS X)"
			"      Jeremy Sheeley (Player Profiles)"
			"      Tsuyoshi Iguchi (FreeBSD, NetBSD)"
			"      G. Low (Solaris)"
			"      Gerry Jo \"Trick\" Jellestad (Testing & Ideas)"
			"      \"Riki\" (Intel Compiler)"
			"      Andreas Spaangberg (Sun Compiler)"
			"      \"SixK\" (Amiga Port)"
			"      Joey Hess (Debian)"
			"      Martijn van Oosterhout (FPS limiter)"
			"      Antonio Messina (Stage 1601+, Always Fire)"
			"      Hans de Goede (Audio bug)"
			"      Marianne Ibbotson (\"Autopause\")"
			"      Sam Palmer (Windows testing)"
			"      Michael Sterrett (glSDL issues)"
			"      Sam Lantinga & Others (SDL)"
			"      Members of the SDL Mailing List"
			"                        "
			"Additional Thanks from Akira Higuchi Go To: "
			"      Bruce Cheng"
			"      Christoph Lameter"
			"      Davide Rossi"
			"      Eduard Martinescu"
			"      Elan Feingold"
			"      Helmut Hoenig"
			"      Jeff Epler"
			"      Joe Ramey"
			"      Joey Hess"
			"      Michael Sterrett"
			"      Mihail Iotov"
			"      Shoichi Nakayama"
			"      Thomas Marsh"
			"      Torsten Wolnik"
			"                        "
			"And as we said in the old days... "
			"                        "
			"         <WRAP>         ";
// FIXME: Nasty static state variables...
	static const char *stp = scrolltext;
	static int pos = PIXEL2CS(SCREEN_WIDTH);
	static int t = 0;
	int nt = (int)SDL_GetTicks();
	int dt = nt - t;
	t = nt;
	if(dt > 100)
		dt = 100;
	static int fdt = 0;
	fdt += ((dt<<8) - fdt) >> 3;
	pos -= (fdt * PIXEL2CS((int)scroller_speed) + 128000) / 256000;
	woverlay->font(B_BIG_FONT);
	woverlay->string_fxp(pos, PIXEL2CS(312), stp);

	/*
	 * Chop away characters at the left edge
	 */
	char buf[2] = {*stp, 0};
	int cw = woverlay->textwidth_fxp(buf);
	if(-pos > cw)
	{
		pos += cw;
		++stp;
		if(*stp == 0)
		{
			// Wrap!
			pos = PIXEL2CS(SCREEN_WIDTH);
			stp = scrolltext;
		}
	}
}


void _screen::init_scene(int sc)
{
	map.init();
	if(sc < 0)
	{
		/*
		 * Intro mode
		 */
		show_title = 1;
		myship.off();
		enemies.off();
		if(sc == INTRO_SCENE)
		{
			/*
			 * Plain intro - no map
			 */
			radar_mode = RM_OFF;	/* Clear radar */
			scene_num = 16;
			restarts = 0;
		}
		else
		{
			/*
			 * Map selection - show current map
			 */
			radar_mode = RM_SHOW;
			scene_num = -(sc + 1) % scene_max;
			restarts = -(sc + 1) / scene_max;
		}
		wplanet->colormod(wplanet->map_rgb(128, 128, 128));
		wmain->colormod(wmain->map_rgb(128, 128, 128));
	}
	else
	{
		/*
		 * In-game mode
		 */
		show_title = 0;
		scene_num = sc % scene_max;
		restarts = sc / scene_max;
		radar_mode = RM_RADAR;
		wplanet->colormod(wplanet->map_rgb(128, 128, 128));
		wmain->colormod(wmain->map_rgb(255, 255, 255));
	}
	region = scene_num / 10 % 5;
	level = scene_num % 10 + 1;
	init_background();
	gengine->period(game.speed);
	sound.period(game.speed);
	const _scene *s = &scene[scene_num];
	int i;
	for(i = 0; i < s->base_max; i++)
		map.make_maze(s->base[i].x, s->base[i].y, s->base[i].h,
				s->base[i].v);
	map.convert(s->ratio);
	generate_count = 0;
	wradar->mode(radar_mode);
}


int _screen::prepare()
{
	if(scene_num < 0)
		return 0;

	const _scene *s = &scene[scene_num];
	int i, j;
	int count_core = 0;
	int c = 0;

	int lc = restarts > 31 ? 31 : restarts;
	int interval_1 = (s->ek1_interval) >> lc;
	int interval_2 = (s->ek2_interval) >> lc;
	if(interval_1 < 4)
		interval_1 = 4;
	if(interval_2 < 4)
		interval_2 = 4;
	enemies.set_ekind_to_generate(s->ek1, interval_1, s->ek2,
			interval_2);

	for(i = 0; i < MAP_SIZEX; i++)
		for(j = 0; j < MAP_SIZEY; j++)
		{
			int m = MAP_BITS(map.pos(i, j));
			if(IS_SPACE(m))
				continue;
			if((m == U_MASK) || (m == R_MASK) || (m == D_MASK)
					|| (m == L_MASK))
			{
				enemies.make(&cannon, i * 16 + 8,
						j * 16 + 8);
				c++;
			}
			else if(m & CORE)
			{
				enemies.make(&core, i * 16 + 8,
						j * 16 + 8);
				count_core++;
				c++;
			}
		}

	myship.set_position(s->startx << 4, s->starty << 4);

	return count_core;
}


void _screen::generate_fixed_enemies()
{
	static int sint[16] =
			{ 0, 12, 23, 30, 32, 30, 23, 12, 0, -12, -23, -30,
				-32, -30, -23, -12 };
	static int cost[16] =
			{ 32, 30, 23, 12, 0, -12, -23, -30, -32, -30, -23,
				-12, 0, 12, 23, 30 };
	const _scene *s = &scene[scene_num];
	if(generate_count < s->enemy_max)
	{
		int j;
		for(j = 0; j < s->enemy[generate_count].num; j++)
		{
			int sp = s->enemy[generate_count].speed;
			int x, y, h, v, t;
			x = gamerand.get() % (WORLD_SIZEX - VIEWLIMIT * 2);
			y = gamerand.get() % (WORLD_SIZEY - VIEWLIMIT * 2);
			x -= (WORLD_SIZEX / 2 - VIEWLIMIT);
			y -= (WORLD_SIZEY / 2 - VIEWLIMIT);
			if(x < 0)
				x -= VIEWLIMIT;
			else
				x += VIEWLIMIT;
			if(y < 0)
				y -= VIEWLIMIT;
			else
				y += VIEWLIMIT;
			x += myship.get_x();
			y += myship.get_y();

			t = gamerand.get(4);
			h = PIXEL2CS(sp * sint[t]) / 64;
			v = PIXEL2CS(sp * cost[t]) / 64;
			enemies.make(s->enemy[generate_count].kind, x, y,
					h, v);
		}
		generate_count++;
	}
	if(generate_count >= s->enemy_max)
		generate_count = 0;
}


void _screen::set_map(int x, int y, int n)
{
	map.pos(x, y) = n;
	wradar->update(x, y);
}


void _screen::render_noise()
{
	if(!do_noise)
		return;
	if(noise_fade < 0.01f)
		return;
	int ymax = noise_y + noise_h;
	int np = pubrand.get(8);
	int dnp = pubrand.get(4) - 8;
	float ifade = 1.0f - noise_fade;
	float step = 1.0f + ifade*ifade*ifade * 5.0f;
	float rstep = (ifade + ifade*ifade*ifade * 10.0f) / 256.0f;
	if(step < 1.0f)
		step = 1.0f;
	if(rstep < 0.0f)
		rstep = 0.0f;
	for(float fy = noise_y + pubrand.get(8) * rstep; fy < ymax;
			fy += step + pubrand.get(8) * rstep)
	{
		int xo = PIXEL2CS(pubrand.get(NOISE_SIZEX_LOG2));
		int xmax = ((WMAIN_W + CS2PIXEL(xo)) >> NOISE_SIZEX_LOG2) + 1;
		dnp += pubrand.get(3) - 4 + pubrand.get(1);
		np += dnp;
		if(np > 255)
			np = 255, dnp = -dnp / 2;
		else if(np < 0)
			np = 0, dnp = -dnp / 2;
		float lv = np * noise_depth * (1.0f - noise_bright) / 255.0f +
				noise_bright;
		for(int x = 0; x < xmax; ++x)
			wmain->sprite_fxp(
					PIXEL2CS(x << NOISE_SIZEX_LOG2) - xo,
					PIXEL2CS((int)fy),
					noise_source, (int)(lv * 15.0f +
					(float)pubrand.get(8) / 256.0f));
	}
}


void _screen::set_noise(int source, float fade, float bright, float depth)
{
	noise_source = source;
	noise_y = 0;
	noise_h = WMAIN_H;
	noise_fade = fade;
	noise_bright = bright;
	noise_depth = depth;
}


void _screen::render_highlight()
{
	if(!highlight_h)
		highlight_y = wmain->height() / 2;

	static float ypos = -50;
	static float hf = 0;
	static int ot = 0;
	int t = (int)SDL_GetTicks();
	int dt = t - ot;
	ot = t;
	if(dt > 1000)
		dt = 100;

	//Spring + friction style velocity component
	if(dt < 500)
		ypos += ((float)highlight_y - ypos) * (float)dt * 0.005f;
	else
		ypos = (float)highlight_y;

	//Constant velocity component
	float v = (float)dt * 0.2f;
	if((float)highlight_y - ypos > v)
		ypos += v;
	else if((float)highlight_y - ypos < -v)
		ypos -= v;
	else
		ypos = (float)highlight_y;
	
	// Scale to new height
	hf += ((float)highlight_h - hf) * (float)dt * 0.01f;

	// Render!
	int y = (int)((ypos - hf / 2.0f) * 256.0f);
	int h = (int)(hf * 256.0f);
	if(h < 128)
		return;
	for(int ty = -256; ty <= h; ty += h + 256)
	{
		int xo = PIXEL2CS(pubrand.get(NOISE_SIZEX_LOG2));
		int xmax = ((WMAIN_W + CS2PIXEL(xo)) >> NOISE_SIZEX_LOG2) + 1;
		for(int x = 0; x < xmax; ++x)
			woverlay->sprite_fxp(PIXEL2CS(x << NOISE_SIZEX_LOG2) - xo,
					ty + y,
					B_NOISE, 6);
	}

	int x0 = wmain->phys_rect.x;
	s_bank_t *b = s_get_bank(gfxengine->get_gfx(), B_FOCUSFX);
	if(!b)
		return;
	s_sprite_t *s = s_get_sprite_b(b, 0);
	if(!s || !s->texture)
		return;
	SDL_Renderer *r = gengine->renderer();
	y = (int)((y * gengine->yscale() + 128) / 256) + wmain->phys_rect.y;
	h = (int)(hf * gengine->yscale());
	wmain->select();
	for(int ty = 0; ty < h; ++ty)
	{
		float sy = (float)ty / (h - 1);
		float scy = sy * hf;
		float shape = 1.0f - sin(M_PI * sy);
		float edges = shape * shape * shape;
		float plasma = 0.5f + 0.5f * sin(t * 0.004f +
				sin(t * 0.00017f) * scy * 0.18f);
		float plasma2 = 0.5f + 0.5f * sin(t * .003 +
				sin(t * 0.0001f) * scy * 0.12f);
		int i = (int)((b->h - 1) * ((.5f * plasma + .5f * plasma *
				shape) * (1.0f - edges) + edges));
		int xo = (int)((t * 10 + 8192 * plasma2) *
				gengine->xscale() / 256);
		xo -= (int)(xo / b->w) * b->w;
		int xmax = (int)((WMAIN_W * gengine->xscale() + xo) / b->w);
		for(int x = 0; x <= xmax; ++x)
		{
			SDL_Rect sr, dr;
			sr.x = 0;
			sr.y = i;
			dr.x = x0 + (int)(x * b->w) - xo;
			dr.y = y + ty;
			dr.w = sr.w = b->w;
			dr.h = sr.h = 1;
			SDL_RenderCopy(r, s->texture, &sr, &dr);
		}
	}
}


void _screen::set_highlight(int y, int h)
{
	highlight_y = y;
	highlight_h = h;
}


// This starfield implementation may seem a bit backwards. Instead of moving
// around in a 3D point cloud, projecting by dividing x and y by z etc, we're
// doing a plain orthogonal projection. That is, the z coordinates have no
// direct impact on rendering!
//    What actually creates the parallax effect is that we scroll the *stars*
// around at different speeds, depending on their distance from the screen,
// which effectively replaces the usual (x/z, y/z) operation when projecting.
//    The interesting part is that stars wrap around the edges automatically as
// a side effect of the integer arithmetics. Thus we can have a nice, dense
// "3D" starfield covering the whole window - no nearby stars way off screen,
// and no distant stars wrapping around in a small square in the middle of the
// screen. No clipping is needed, and thus, no cycles are wasted animating off-
// screen stars.
//    Of course, this design means we cannot move along the Z axis (well, not
// trivially, at least), but we don't really need that here anyway.
//
void _screen::init_starfield(int altitude, int psize)
{
	free(stars);
	nstars = prefs->stars;
	star_alt = altitude;
	star_psize = psize;
	stars = (KOBO_Star *)malloc(nstars * sizeof(KOBO_Star));
	if(!stars)
	{
		nstars = 0;
		return;		// Out of memory!!!
	}
	int pivot = star_alt << 8;
	if(star_alt < 64)
		star_pscale = 512;
	else if(star_alt > 192)
		star_pscale = 256;
	else
		star_pscale = 512 - ((star_alt - 64) << 1);
	for(int i = 0; i < nstars; ++i)
	{
		stars[i].x = pubrand.get();
		stars[i].y = pubrand.get();
		int zz = 255 * i / nstars;
		stars[i].z = 65025 - zz * zz;
		int z =  ((int)stars[i].z - pivot) * star_pscale >> 8;
	}
}

void _screen::init_starfield_colors()
{
	const char colors[] = {
		1,	52,	51,	2,
		47,	3,	46,	4
	};
	for(int i = 0; i < STAR_COLORS; ++i)
		starcolors[STAR_COLORS - i - 1] =
				wmain->map_rgb(gengine->palette(colors[i]));
}

void _screen::render_starfield(int xo, int yo)
{
	int i;
	int w = wmain->width() * 256;
	int h = wmain->height() * 256;
	int xc = w / 2;
	int yc = h / 2;
	int pivot = star_alt << 8;	// Rotation pivot z coordinate
	int pclip = star_psize * star_psize / 4; // Planet radius squared
	int pscale;

	if(nstars != prefs->stars)
		init_starfield(star_alt, star_psize);

	// Calculate delta from last position, dealing with map position wrap
	const int WSX = WORLD_SIZEX << 8;
	const int WSY = WORLD_SIZEY << 8;
	int dx = (xo - star_oxo) & (WSX - 1);
	star_oxo = xo;
	if(dx & (WSX >> 1))
		dx |= 1 - (WSX - 1);
	int dy = (yo - star_oyo) & (WSY - 1);
	star_oyo = yo;
	if(dy & (WSY >> 1))
		dy |= 1 - (WSY - 1);

	// Scale the deltas to compensate for window/starfield size mismatch
	// (Otherwise stars at zero distance won't sync up with the map as
	// intended!)
	dx = (dx << 16) / w;
	dy = (dy << 16) / h;

	wmain->select();
	for(i = 0; i < nstars; ++i)
	{
		int iz = (int)stars[i].z;
		int z =  (iz - pivot) * star_pscale >> 8;

		// Move star!
		stars[i].x += dx * z >> 16;
		stars[i].y += dy * z >> 16;

		// Scale to fixp "native" display coordinates
		int x = stars[i].x * (w >> 8) >> 8;
		int y = stars[i].y * (h >> 8) >> 8;

		// Skip stars occluded by the planet
		int xi = x >> 8;
		int yi = y >> 8;
		if((z > 0) && ((xi * xi + yi * yi) < pclip))
			continue;

		// Center
		x += xc;
		y += yc;

		// Plot!
		wmain->foreground(starcolors[iz * STAR_COLORS >> 16]);
		wmain->fillrect_fxp(x, y, 256, 256);
	}
}


void _screen::init_background()
{
	// Select layers, altitudes etc...
	bg_altitude = 0;
	bg_backdrop = 0;
	bg_clouds = 0;
	int planet = 0;
	int psize = 0;
	spinplanet_modes_t md = SPINPLANET_OFF;
	wplanet->track_speed(0.5f, 1.0f);
	wplanet->track_offset(0.5f, 0.5f);
	wplanet->set_texture_repeat(2);
	switch(level)
	{
	  case 1:
		// 32x32 planet
		psize = 32;
		planet = B_R1L1_PLANET;
		md = SPINPLANET_SPIN;
		bg_altitude = 192;
		break;
	  case 2:
		// 48x48 planet
		psize = 48;
		planet = B_R1L2_PLANET;
		md = SPINPLANET_SPIN;
		bg_altitude = 176;
		break;
	  case 3:
		// 64x64 planet
		psize = 64;
		planet = B_R1L3_PLANET;
		md = SPINPLANET_SPIN;
		bg_altitude = 160;
		break;
	  case 4:
		// 96x96 planet
		psize = 96;
		planet = B_R1L4_PLANET;
		md = SPINPLANET_SPIN;
		bg_altitude = 144;
		break;
	  case 5:
		// 128x128 planet
		psize = 128;
		planet = B_R1L5_PLANET;
		md = SPINPLANET_SPIN;
		bg_altitude = 128;
		break;
	  case 6:
		// 192x192 planet
		psize = 192;
		planet = B_R1L6_PLANET;
		md = SPINPLANET_SPIN;
		bg_altitude = 112;
		break;
	  case 7:
		// 256x256 planet
		psize = 256;
		planet = B_R1L7_PLANET;
		md = SPINPLANET_SPIN;
		bg_altitude = 96;
		break;
	  case 8:
		// Above clouds
		bg_clouds = 1;
		bg_backdrop = B_R1L8_GROUND/* + region*/;
		bg_altitude = 64;
		break;
	  case 9:
		// Below clouds
		bg_backdrop = B_R1L9_GROUND/* + region*/;
		bg_altitude = 32;
		break;
	  case 10:
		// Ground level
		bg_backdrop = B_R1L10_GROUND/* + region*/;
		bg_altitude = 0;
		break;
	}
	if(md == SPINPLANET_STATIC)
		wplanet->set_planet(planet/* + region*/, 0);
	else
		wplanet->set_map(planet/* + region*/, 0);
	wplanet->set_size(psize);
	wplanet->set_mode(md);
	init_starfield(bg_altitude, psize);
}


void _screen::render_background()
{
	if(do_noise && (noise_fade >= 1.0f))
		return;

	int vx = gengine->xoffs(LAYER_BASES) * TILE_SIZE / 16;
	int vy = gengine->yoffs(LAYER_BASES) * TILE_SIZE / 16;

	// Start exactly at the top-left corner of the tile visible in the top-
	// left corner of the display window.
	int xo = vx % PIXEL2CS(TILE_SIZE);
	int yo = vy % PIXEL2CS(TILE_SIZE);
	int mx = CS2PIXEL(vx / TILE_SIZE);
	int my = CS2PIXEL(vy / TILE_SIZE);
	int ymax = ((WMAIN_H + CS2PIXEL(yo)) / TILE_SIZE) + 1;
	int xmax = ((WMAIN_W + CS2PIXEL(xo)) / TILE_SIZE) + 1;

	// Render ground
	s_bank_t *b = s_get_bank(gengine->get_gfx(), bg_backdrop);
	if(bg_backdrop && b)
	{
		// Center background on map, and repeat the texture twice
		// vertically, because the map aspect ratio is 1:2.
		int bw = b->w << 8;
		int bh = b->h << 8;
		float nx = gengine->nxoffs(LAYER_BASES);
		float ny = gengine->nyoffs(LAYER_BASES);
		int xo = (1.0f - nx + 1.5f) * bw + (WMAIN_W << 7);
		int yo = ((1.0f - ny) * 2.0f + 1.5f) * bh + (WMAIN_H << 7);
		int x = xo % bw;
		int y = yo % bh;
		wmain->colormod(wmain->map_rgb(128, 128, 128));	// HalfBrite!
		wmain->sprite_fxp(x - bw, y - bh, bg_backdrop, 0);
		wmain->sprite_fxp(x, y - bh, bg_backdrop, 0);
		wmain->sprite_fxp(x - bw, y, bg_backdrop, 0);
		wmain->sprite_fxp(x, y, bg_backdrop, 0);
		if(!show_title)
			wmain->colormod(wmain->map_rgb(255, 255, 255));
	}

	// Render clouds
	if(bg_clouds)
	{
//TODO:
//		wmain->sprite_fxp(vx, vy, bg_clouds + region, 0);
	}

	// Render parallax starfield
	if(bg_altitude >= 96)
		render_starfield(vx, vy);

	// Render bases
	int tileset = B_R1_TILES + region;
	int frame = manage.game_time();
	for(int y = 0; y < ymax; ++y)
		for(int x = 0; x < xmax; ++x)
		{
			int n = map.pos(mx + x, my + y);
			if(IS_SPACE(n) && (MAP_TILE(n) == 0))
				continue;
			int tile;
			if(n & CORE)
			{
				if(n & (U_MASK | D_MASK))
					tile = 16;
				else
					tile = 24;
				tile += frame / 2 & 7;
			}
			else
				tile = MAP_TILE(n);
			wmain->sprite_fxp(PIXEL2CS(x * TILE_SIZE) - xo,
					PIXEL2CS(y * TILE_SIZE) - yo,
					tileset, tile);
		}
}


void _screen::render_fx()
{
	render_noise();
	render_highlight();
}


void _screen::fps(float f)
{
	_fps = f;
}


void _screen::noise(int on)
{
	do_noise = on;
}
