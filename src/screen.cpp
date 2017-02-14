/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2005, 2007, 2009 David Olofson
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

int KOBO_screen::stage;
int KOBO_screen::region;
int KOBO_screen::level;
const KOBO_scene *KOBO_screen::scene;
int KOBO_screen::bg_altitude;
int KOBO_screen::bg_clouds;
int KOBO_screen::restarts;
int KOBO_screen::generate_count;
KOBO_map KOBO_screen::map[KOBO_BG_MAP_LEVELS + 1];
int KOBO_screen::show_title = 0;
int KOBO_screen::do_noise = 0;
float KOBO_screen::_fps = 40;
float KOBO_screen::scroller_speed = SCROLLER_SPEED;
float KOBO_screen::target_speed = SCROLLER_SPEED;
int KOBO_screen::noise_y = 0;
int KOBO_screen::noise_h = 100;
int KOBO_screen::noise_source = B_NOISE;
float KOBO_screen::noise_fade = 0.0f;
float KOBO_screen::noise_bright = 0.0f;
float KOBO_screen::noise_depth = 0.0f;
int KOBO_screen::highlight_y = 0;
int KOBO_screen::highlight_h = 0;
KOBO_Starfield KOBO_screen::stars;
int KOBO_screen::long_credits_wrap = 0;


void KOBO_screen::init_graphics()
{
	stars.set_target(wmain, KOBO_P_MAIN_STARS);
	noise_h = DASHH(MAIN);
	highlight_y = DASHH(MAIN) / 2;
}


void KOBO_screen::render_title_plasma(int t, float fade, int y, int h)
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


void KOBO_screen::render_title_noise(float fade, int y, int h, int bank, int frame)
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


void KOBO_screen::title(int t, float fade, int mode)
{
	s_bank_t *b = s_get_bank(gfxengine->get_gfx(), B_LOGO);
	if(!b)
		return;
	s_sprite_t *s = s_get_sprite_b(b, 0);
	if(!s || !s->texture)
		return;

	int frame = 0;
	if(b->max >= 1)
	{
		frame = 1;
		if(fade > 0.9)
			frame = 0;
		else if(fade > 0.8 && fmod(fade, .04) > .03)
			frame = 0;
	}

	switch((KOBO_LogoEffects)themedata.get(KOBO_D_LOGO_EFFECT))
	{
	  case KOBO_LOGO_FX_SLIDE:
	  {
		float mf = (1.0f - fade);
		float ly = mf * mf * mf * DASHH(MAIN) - 0.5f;
		if(ly < 0.0f)
			ly = 0.0f;
		woverlay->sprite_fxp(PIXEL2CS(woverlay->width() - 320) / 2,
				(int)((woverlay->height()  / 2 -
				(128 - 20) - ly) * 256.0f), B_LOGO, frame);
		break;
	  }
	  case KOBO_LOGO_FX_FADE:
		woverlay->alphamod(fade * fade * fade * 255.0f);
		woverlay->sprite_fxp(PIXEL2CS(woverlay->width() - 320) / 2,
				PIXEL2CS(woverlay->height() / 2 - (128 - 20)),
				B_LOGO, frame);
		woverlay->alphamod(255);
		break;
	  case KOBO_LOGO_FX_ZOOM:
		woverlay->sprite_fxp_scale(
				PIXEL2CS(woverlay->width() - 320) / 2,
				PIXEL2CS(woverlay->height() / 2 - (128 - 20)),
				B_LOGO, frame, 1.0f, fade);
		break;
	}

	if(fade > 0.9)
	{
		// Version
		woverlay->font(B_NORMAL_FONT);
		woverlay->center(195, KOBO_VERSION);

		// Copyright
		woverlay->font(B_SMALL_FONT);
		woverlay->center(210, KOBO_COPYRIGHT);
	}

	// Cheat mode warning
	if((prefs->cheats()) && (t % 1000 < 500))
	{
		woverlay->font(B_MEDIUM_FONT);
		woverlay->center(230, "CHEAT MODE");
	}
#ifdef KOBO_DEMO
	if(t % 1000 < 500)
	{
		woverlay->font(B_BIG_FONT);
		woverlay->center(230, "DEMO");
	}
#endif
#if 1
	// WIP notice
	if(!flashin(t - 2000))
	{
		woverlay->font(B_MEDIUM_FONT);
		woverlay->center(260, "This is a Work in Progress!");
		woverlay->center(270, "Check out");
		woverlay->center(280, "https://olofson.itch.io/kobo-redux");
	}
#endif
}


void KOBO_screen::credits(int t)
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
#if 0
	if(!flashin(t - 7000))
	{
		woverlay->font(B_MEDIUM_FONT);
		woverlay->center(170, "Additional Credits & Thanks");
		woverlay->center(180, "in the scroller below");
	}
#endif
}


static const char *kobo_credits[] = {
	// '\\', 'i'(image)/'s'(sprite), bank, frame
	(const char []){ '\\', 'i', B_LOGO, 0 },
	"",
	"!W a s   C r e a t e d   B y",
	"",
	"Akira Higuchi",
	"!a n d",
	"David Olofson",
	"",
	"",
	(const char []){ '\\', 's', B_PLAYER, 0 },
	"",
	"",
	"!S p e c i a l   T h a n k s   T o",
	"",
	"Andreas Spaangberg",
	"Antonio Messina",
	"Bruce Cheng",
	"Christoph Lameter",
	"David Andersson",
	"Davide Rossi",
	"Eduard Martinescu",
	"Elan Feingold",
	"Erik Auerswald",
	"Florian Schulze",
	"G. Low",
	"Gerry Jo \"Trick\" Jellestad",
	"Hans de Goede",
	"Helmut Hoenig",
	"Jeff Epler",
	"Jeremy Sheeley",
	"Joe Ramey",
	"Joey Hess",
	"Karina Gomez",
	"Marianne Ibbotson",
	"Martijn van Oosterhout",
	"Masanao Izumo",
	"Max Horn",
	"Michael Sterrett",
	"Mihail Iotov",
	"Riki",
	"Robert Salender",
	"Robert Schuster",
	"Ryan C. Gordon",
	"Sam Lantinga",
	"Sam Palmer",
	"Samuel Hart",
	"Shoichi Nakayama",
	"Simon Peter",
	"SixK",
	"Stephanie Vivian",
	"Thomas Marsh",
	"Torsten Giebl",
	"Torsten Wolnik",
	"Tsuyoshi Iguchi",
	"",
	"",
	(const char []){ '\\', 's', B_PLAYER, 0 },
	"",
	"",
	"T h a n k   Y o u",
	"F o r",
	"P l a y i n g",
	"",
	"",
	(const char []){ '\\', 's', B_PLAYER, 0 },
	NULL
};


void KOBO_screen::long_credits(int t)
{
	int y = woverlay->height();
	t *= 10;
	if(long_credits_wrap)
		t %= long_credits_wrap;
	screen.set_highlight(0, 0);
	for(int i = 0; kobo_credits[i]; ++i)
	{
		const char *s = kobo_credits[i];
		if(s[0] == '!')
		{
			woverlay->font(B_NORMAL_FONT);
			++s;
		}
		else
			woverlay->font(B_BIG_FONT);

		int sy = PIXEL2CS(y - woverlay->fontheight() / 2) - t;

		if(s[0] == '\\')
		{
			int w = 0;
			int h = 0;
			s_bank_t *b = s_get_bank(gfxengine->get_gfx(), s[2]);
			if(b)
			{
				w = b->w;
				h = b->h;
			}
			int sx;
			if(s[1] == 'i')
			{
				// Center, assuming top-left hotspot
				sx = PIXEL2CS(woverlay->width() - w) / 2;
			}
			else
			{
				// Assume centered hotspot!
				sx = PIXEL2CS(woverlay->width()) / 2;
				sy += PIXEL2CS(h / 2);
			}
			woverlay->sprite_fxp(sx, sy, s[2], s[3]);
			y += h + 5;
		}
		else
		{
			woverlay->center_fxp(sy, s);
			y += 30;
		}
	}
	long_credits_wrap = PIXEL2CS(y + woverlay->height());
}


void KOBO_screen::render_anim(int x, int y, int bank, int first, int last,
			float speed, int t)
{
	int nf = gengine->get_nframes(bank);
	if(first < 0)
		first = nf + first;
	if(last < 0)
		last = nf + last;
	int reverse = first > last;
	if(reverse)
	{
		int tmp = first;
		first = last;
		last = tmp;
	}
	nf = last - first + 1;
	if(speed < 0)
		speed = -speed * nf;
	int frame = first + (int)(t * speed / 1000.0f) % (last - first + 1);
	if(reverse)
		frame = last - frame;
	woverlay->sprite(x, y, bank, frame);
}


void KOBO_screen::help(int t)
{
	int cx = woverlay->width() / 2;
	woverlay->font(B_BIG_FONT);
	woverlay->center(53, "HOW TO PLAY");

	// Control
	if(t < 0 + INST_TIME_IN)
		screen.set_highlight(112, 80);
	else if(t < INST_TIME_CONTROL - INST_TIME_OUT)
	{
		if(t > INST_TIME_CONTROL - INST_TIME_HL_OUT)
			screen.set_highlight(0, 0);
		woverlay->font(B_MEDIUM_FONT);
		render_anim(cx, 105, B_PLAYER, 0, -1, 30, t);
		woverlay->center(130, "Use arrow keys or NumPad.");
		woverlay->center(140, "to control the ship.");
	}
	else if(t < INST_TIME_CONTROL)
		return;

	// Fire
	else if(t < INST_TIME_CONTROL + INST_TIME_IN)
		screen.set_highlight(112, 100);
	else if(t < INST_TIME_FIRE - INST_TIME_OUT)
	{
		if(t > INST_TIME_FIRE - INST_TIME_HL_OUT)
			screen.set_highlight(0, 0);
		woverlay->font(B_MEDIUM_FONT);
		woverlay->center(80, "Primary Weapon: SHIFT or X");
		woverlay->sprite(cx, 105, B_PLAYER,
				gengine->get_nframes(B_PLAYER) / 4);
		int i;
		int txo = t * cx / 8 / 15 % (cx / 8);
		for(i = 0; i < 8; ++i)
		{
			// NOTE: GUN_*_DIR constants ignored here...
			int st = (cx * i / 8 + txo) / 5;
			woverlay->sprite(cx - GUN_NOSE_Y - cx * i / 8 - txo,
					105,
					B_BOLT, myship.bolt_frame(7, st));
			woverlay->sprite(cx + GUN_TAIL_Y + cx * i / 8 + txo,
					105,
					B_BOLT, myship.bolt_frame(3, st));
		}
		int cx2 = cx - 40;
		woverlay->sprite(cx2, 135, B_PLAYER,
				gengine->get_nframes(B_PLAYER) / 4);
		txo = t % 1000 / 3;
		for(i = 0; i < 40; ++i)
		{
			int st = (cx2 * i / 8 + txo) / 5;
			woverlay->sprite(cx2 + GUN_TAIL_Y + txo + i,
					135 + sin(i * i * 0.02f +
					txo * 0.05f) * 4,
					B_BOLT, myship.bolt_frame(3, st + i));
		}
		woverlay->center(150, "Secondary Weapon: CTRL or SPACE");
	}
	else if(t < INST_TIME_FIRE)
		return;

	// Bases
	else if(t < INST_TIME_FIRE + INST_TIME_IN)
		screen.set_highlight(112, 65);
	else if(t < INST_TIME_BASES - INST_TIME_OUT)
	{
		if(t > INST_TIME_BASES - INST_TIME_HL_OUT)
			screen.set_highlight(0, 0);
		woverlay->font(B_MEDIUM_FONT);
		int i;
		for(i = 0; i < 5; ++i)
		{
			int f0 = (i & 1) ? 24 : 16;
			render_anim(cx + 24 * i - ((4 * 24 + 16) / 2),
					100 - 8, B_R1_TILES + i,
					f0, f0 + 7, 20, t);
		}
		woverlay->center(125,
				"Destroy bases by shooting their cores.");
	}
	else if(t < INST_TIME_BASES)
		return;

	// Shoot everything!
	else if(t < INST_TIME_BASES + INST_TIME_IN)
		screen.set_highlight(112, 95);
	else if(t < INST_TIME_ENEMIES - INST_TIME_OUT)
	{
		if(t > INST_TIME_ENEMIES - INST_TIME_HL_OUT)
			screen.set_highlight(0, 0);
		woverlay->font(B_MEDIUM_FONT);
		render_anim(cx - 100, 95, B_RING,	0, -1, -.5, t);
		render_anim(cx - 85, 120, B_BOMB,	0, 32, -.5, t);
		render_anim(cx - 70, 90, B_BOMB,	32, 0, -.5, t);
		render_anim(cx - 50, 115, B_BMR_GREEN,	0, -1, -.75, t);
		render_anim(cx - 20, 95, B_BMR_PURPLE,	-1, 0, -.7, t);
		render_anim(cx + 10, 115, B_BMR_PINK,	0, -1, -.8, t);
		render_anim(cx + 40, 95, B_FIGHTER,	-1, 0, -1, t);
		render_anim(cx + 60, 115, B_MISSILE1,	0, -1, -.85, t);
		render_anim(cx + 80, 95, B_MISSILE2,	-1, 0, -.95, t);
		render_anim(cx + 100, 115, B_MISSILE3,	0, -1, -1.05, t);
		woverlay->center(135, "Shoot everything that moves,");
		woverlay->center(145, "but avoid getting hit!");
	}
	else if(t < INST_TIME_ENEMIES)
		return;

	// Tough or indestructible objects
	else if(t < INST_TIME_ENEMIES + INST_TIME_IN)
		screen.set_highlight(112, 110);
	else if(t < INST_TIME_TOUGH - INST_TIME_OUT)
	{
		if(t > INST_TIME_TOUGH - INST_TIME_HL_OUT)
			screen.set_highlight(0, 0);
		woverlay->font(B_MEDIUM_FONT);
		woverlay->center(75, "Some objects are indestructible...");
		int i;
		for(i = 0; i < 5; ++i)
			woverlay->sprite(cx + 24 * i - ((4 * 24 + 16) / 2),
					95 - 8, B_R1_TILES + i, 32 + i % 4);
		render_anim(cx - 70, 130, B_ROCK1,	0, -1, -.7, t);
		render_anim(cx - 30, 120, B_ROCK2,	-1, 0, -.9, t);
		render_anim(cx + 20, 130, B_BIGSHIP,	0, -1, -1, t);
		render_anim(cx + 70, 135, B_ROCK3,	-1, 0, -.8, t);
		woverlay->center(155, "...or take many hits to destroy.");
	}
}


void KOBO_screen::scroller()
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
	static int pos = PIXEL2CS((int)SCREEN_WIDTH);
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
			pos = PIXEL2CS((int)SCREEN_WIDTH);
			stp = scrolltext;
		}
	}
}


void KOBO_screen::init_stage(int st, bool ingame)
{
	wplanet->resetmod();
	wplanet->blendmode(GFX_BLENDMODE_ALPHA);
	int cm = 255.0f * themedata.get(KOBO_D_PLANET_COLORMOD, level - 1);
	wplanet->colormod(cm, cm, cm);

	if(!ingame)
	{
		show_title = 1;
		myship.off();
		enemies.off();
	}
	else
		show_title = 0;

	// Get scene data, region, level etc
	stage = st;
	scene = scene_manager.get(stage);
	if(!scene)
		return;
	restarts = stage / scene_manager.scene_count();
	region = scene_manager.region(stage);
	level = scene_manager.level(stage);

	// Initialize maps (current + parallax layers)
	map[0].init(scene);
	for(int i = 0; i < KOBO_BG_MAP_LEVELS; ++i)
	{
		const KOBO_scene *s = NULL;
		if(level + i <= KOBO_LEVELS_PER_REGION)
			s = scene_manager.get(stage + 1 + i);
		map[i + 1].init(s);
	}

	// Set up backdrop, planet, starfield, ground etc
	init_background();
	generate_count = 0;
}


int KOBO_screen::prepare()
{
	if(stage <= 0)
		return 0;

	int i, j;
	int count_core = 0;
	int c = 0;

	sound.g_position(scene->startx << 4, scene->starty << 4);

	int lc = restarts > 31 ? 31 : restarts;
	int interval_1 = (scene->ek1_interval) >> lc;
	int interval_2 = (scene->ek2_interval) >> lc;
	if(interval_1 < 4)
		interval_1 = 4;
	if(interval_2 < 4)
		interval_2 = 4;
	enemies.set_ekind_to_generate(scene->ek1, interval_1, scene->ek2,
			interval_2);

	for(i = 0; i < MAP_SIZEX; i++)
		for(j = 0; j < MAP_SIZEY; j++)
		{
			int m = MAP_BITS(map[0].pos(i, j));
			if(IS_SPACE(m))
				continue;
			if((m == U_MASK) || (m == R_MASK) || (m == D_MASK)
					|| (m == L_MASK))
			{
				enemies.make(&cannon, PIXEL2CS(i * 16 + 8),
						PIXEL2CS(j * 16 + 8));
				c++;
			}
			else if(m & CORE)
			{
				enemies.make(&core, PIXEL2CS(i * 16 + 8),
						PIXEL2CS(j * 16 + 8));
				count_core++;
				c++;
			}
		}

	myship.set_position(scene->startx << 4, scene->starty << 4);

	return count_core;
}


void KOBO_screen::generate_wave(const KOBO_enemy_set *wave)
{
	static int sint[16] =
			{ 0, 12, 23, 30, 32, 30, 23, 12, 0, -12, -23, -30,
				-32, -30, -23, -12 };
	static int cost[16] =
			{ 32, 30, 23, 12, 0, -12, -23, -30, -32, -30, -23,
				-12, 0, 12, 23, 30 };
	for(int j = 0; j < wave->num; j++)
	{
		int sp = wave->speed;
		if(wave->kind->is_bullet())
			sp *= game.bullet_speed;
		else
			sp *= game.launch_speed;
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
		h = sp * sint[t] / 64;
		v = sp * cost[t] / 64;
		enemies.make(wave->kind, PIXEL2CS(x), PIXEL2CS(y), h, v);
	}
}


void KOBO_screen::generate_fixed_enemies()
{
	if(scene->enemy_max < 0)
	{
		for(int i = 0; i < -scene->enemy_max; ++i)
			generate_wave(&scene->enemy[i]);
	}
	else
	{
		if(generate_count >= scene->enemy_max)
			generate_count = 0;
		generate_wave(&scene->enemy[generate_count]);
		if(generate_count < scene->enemy_max)
			generate_count++;
	}
}


void KOBO_screen::set_map(int x, int y, int n)
{
	map[0].pos(x, y) = n;
	wradar->update(x, y);
}


void KOBO_screen::render_noise()
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
		int xmax = (((int)DASHW(MAIN) + CS2PIXEL(xo)) >>
				NOISE_SIZEX_LOG2) + 1;
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


void KOBO_screen::set_noise(int source, float fade, float bright, float depth)
{
	noise_source = source;
	noise_y = 0;
	noise_h = DASHH(MAIN);
	noise_fade = fade;
	noise_bright = bright;
	noise_depth = depth;
}


void KOBO_screen::render_highlight()
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
	int x0 = wmain->phys_rect.x;
	s_bank_t *b = s_get_bank(gfxengine->get_gfx(), B_FOCUSFX);
	if(!b)
		return;
	s_sprite_t *s = s_get_sprite_b(b, 0);
	if(!s || !s->texture)
		return;
	SDL_Renderer *r = gengine->renderer();
	y = (int)((y * gengine->yscale() + 128) / 256) + wmain->phys_rect.y;
	int h = (int)(hf * gengine->yscale());
	wmain->select();
	int c = 240 - hf;
	if(c < 128)
		c = 128;
	else if(c > 255)
		c = 255;
	SDL_SetTextureColorMod(s->texture, c, c, c);
	int xc = (b->w - wmain->phys_rect.w / gengine->xscale()) / 2;
	for(int ty = 0; ty < h; ty += gengine->yscale())
	{
		int i = b->h * ty / h;
		float xo = fmod(t * 0.001f, 1.0f);
		SDL_SetTextureAlphaMod(s->texture, 255 * themedata.lerp(
				KOBO_D_FOCUSFX_ALPHA, (float)ty / h *
				(themedata.length(KOBO_D_FOCUSFX_ALPHA) - 1)));
		for(int x = 0; ; ++x)
		{
			SDL_Rect sr, dr;
			int xop = xo * (THD(FOCUSFX_GRID, 0) * (h - ty) / h +
					THD(FOCUSFX_GRID, 1) * ty / h);
			sr.x = 0;
			sr.y = i;
			sr.w = b->w;
			sr.h = 1;
			dr.x = x0 + (int)(x * b->w - xc + xop) *
					gengine->xscale();
			if(dr.x > x0 + wmain->phys_rect.w)
				break;
			dr.y = y + ty;
			dr.w = sr.w * gengine->xscale();
			dr.h = sr.h * gengine->yscale();
			SDL_RenderCopy(r, s->texture, &sr, &dr);
		}
	}
	SDL_SetTextureAlphaMod(s->texture, 255);
	SDL_SetTextureColorMod(s->texture, 255, 255, 255);
}


void KOBO_screen::set_highlight(int y, int h)
{
	highlight_y = y;
	highlight_h = h * 1.25f;
}


void KOBO_screen::init_background()
{
	// Select layers, altitudes etc...
	bg_altitude = 0;
	int backdrop = 0;
	bg_clouds = 0;
	int psize = 0;
	spinplanet_modes_t md = SPINPLANET_OFF;
	wplanet->track_speed(0.5f, 1.0f);
	wplanet->track_offset(0.5f, 0.5f);
	wplanet->set_texture_repeat(2);
	switch(level)
	{
	  case 1:
		md = SPINPLANET_SPIN;
		bg_altitude = 192;
		break;
	  case 2:
		md = SPINPLANET_SPIN;
		bg_altitude = 176;
		break;
	  case 3:
		md = SPINPLANET_SPIN;
		bg_altitude = 160;
		break;
	  case 4:
		md = SPINPLANET_SPIN;
		bg_altitude = 144;
		break;
	  case 5:
		md = SPINPLANET_SPIN;
		bg_altitude = 128;
		break;
	  case 6:
		md = SPINPLANET_SPIN;
		bg_altitude = 112;
		break;
	  case 7:
		md = SPINPLANET_SPIN;
		bg_altitude = 96;
		break;
	  case 8:
		// Above clouds
		bg_clouds = 1;
		backdrop = B_R1L8_GROUND + region;
		bg_altitude = 64;
		break;
	  case 9:
		// Below clouds
		backdrop = B_R1L9_GROUND + region;
		bg_altitude = 32;
		break;
	  case 10:
		// Ground level
		backdrop = B_R1L10_GROUND + region;
		bg_altitude = 0;
		break;
	}

	if(md == SPINPLANET_SPIN)
	{
		psize = 40 + level * 12 + level * level * 6;
		wplanet->set_size(psize);
		wplanet->set_source(B_R1_PLANET + region, 0);
		wplanet->set_palette(KOBO_P_PLANET_R1 + region);
		spinplanet_dither_t dth = (prefs->planetdither >= 0) ?
				(spinplanet_dither_t)prefs->planetdither :
				(spinplanet_dither_t)themedata.get(
				KOBO_D_PLANET_DITHERMODE);
		wplanet->set_dither(dth, themedata.get(
				KOBO_D_PLANET_BRIGHTNESS, level - 1),
				themedata.get(KOBO_D_PLANET_CONTRAST,
				level - 1));
	}

	wbackdrop->resetmod();
	int cm = 255.0f * themedata.get(KOBO_D_BACKDROP_COLORMOD, level - 1);
	wbackdrop->colormod(cm, cm, cm);
	if(backdrop)
	{
		wbackdrop->image(backdrop);
		wbackdrop->reverse(false);
	}
	else
	{
		wbackdrop->image(B_SPACE);
		wbackdrop->reverse(true);
	}

	wplanet->set_mode(md);
	stars.init(prefs->stars, bg_altitude, psize, true);
}


void KOBO_screen::render_bases(KOBO_map &map, int tileset, int vx, int vy)
{
	s_bank_t *b = s_get_bank(gengine->get_gfx(), tileset);
	if(!b)
		return;

	// Undo centering
	vx += PIXEL2CS((int)DASHW(MAIN) / 2);
	vy += PIXEL2CS((int)DASHH(MAIN) / 2);

	// Convert to the correct tile size
	vx = vx * b->w / TILE_SIZE;
	vy = vy * b->h / TILE_SIZE;

	// Redo centering
	vx -= PIXEL2CS((int)DASHW(MAIN) / 2);
	vy -= PIXEL2CS((int)DASHH(MAIN) / 2);

	// Re-wrap, because the code below can't handle negative values
	vx = (vx + PIXEL2CS(MAP_SIZEX * b->w)) % PIXEL2CS(MAP_SIZEX * b->w);
	vy = (vy + PIXEL2CS(MAP_SIZEY * b->h)) % PIXEL2CS(MAP_SIZEY * b->h);

	// Start exactly at the top-left corner of the tile visible in the top-
	// left corner of the display window.
	int mx = CS2PIXEL(vx / b->w);
	int my = CS2PIXEL(vy / b->h);
	int xo = (vx + PIXEL2CS(MAP_SIZEX * b->w)) % PIXEL2CS(b->w);
	int yo = (vy + PIXEL2CS(MAP_SIZEY * b->h)) % PIXEL2CS(b->h);
	int ymax = ((DASHH(MAIN) + CS2PIXEL(yo)) / b->w) + 1;
	int xmax = ((DASHW(MAIN) + CS2PIXEL(xo)) / b->h) + 1;
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
			wmain->sprite_fxp(PIXEL2CS(x * b->w) - xo,
					PIXEL2CS(y * b->h) - yo,
					tileset, tile);
		}
	if(prefs->show_map_border)
	{
		wmain->foreground(wmain->map_rgb(0, 100, 200));
		wmain->fillrect_fxp(PIXEL2CS(MAP_SIZEX * b->w) - vx,
				0, PIXEL2CS(1), PIXEL2CS((int)DASHH(MAIN)));
		wmain->fillrect_fxp(0, PIXEL2CS(MAP_SIZEY * b->h) - vy,
				PIXEL2CS((int)DASHW(MAIN)), PIXEL2CS(1));
	}
}


void KOBO_screen::render_background()
{
	int cm;

	if(do_noise && (noise_fade >= 1.0f))
		return;

	int vx = gengine->xoffs(LAYER_BASES);
	int vy = gengine->yoffs(LAYER_BASES);

	// Render clouds
	if(bg_clouds)
	{
//TODO:
		wmain->resetmod();
		wmain->alphamod(128);
//		wmain->sprite_fxp(vx, vy, bg_clouds + region, 0);
	}

	// Render parallax starfield
	if(bg_altitude >= 96)
	{
		wmain->resetmod();
		wmain->alphamod(128);
		stars.render(vx, vy);
	}

	// Render parallax level (upcoming) bases
	wmain->resetmod();
	for(int m = KOBO_BG_MAP_LEVELS - 1; m >= 0; --m)
	{
		if(level + m >= 10)
			continue;
		cm = 255.0f * themedata.get(KOBO_D_BASES_COLORMOD, m);
		wmain->colormod(cm, cm, cm);
		int tiles = region;
		if(bg_altitude > 100)
			switch(m)
			{
			  case 0: tiles += B_R1_TILES_SMALL_SPACE; break;
			  case 1: tiles += B_R1_TILES_TINY_SPACE; break;
			}
		else if(bg_altitude < 80)
			switch(m)
			{
			  case 0: tiles += B_R1_TILES_SMALL_GROUND; break;
			  case 1: tiles += B_R1_TILES_TINY_GROUND; break;
			}
		else
			switch(m)
			{
			  case 0: tiles += B_R1_TILES_SMALL_SPACE; break;
			  case 1: tiles += B_R1_TILES_TINY_INTERMEDIATE; break;
			}
		render_bases(map[m + 1], tiles, vx, vy);
	}

	// Render the bases of the current level
	cm = 255.0f * themedata.get(KOBO_D_BASES_COLORMOD, show_title ? 3 : 2);
	wmain->colormod(cm, cm, cm);
	render_bases(map[0], B_R1_TILES + region, vx, vy);
}


void KOBO_screen::render_fx()
{
	render_noise();
	render_highlight();
}


void KOBO_screen::fps(float f)
{
	_fps = f;
}


void KOBO_screen::noise(int on)
{
	do_noise = on;
}


void KOBO_screen::render_countdown(int y, float t, int timeout, int countdown)
{
	char counter[2] = "0";

	if(timeout == 10)
		t = -1;
	else if(timeout)
		t = timeout - t * 0.001f;
	else
		t = 1.0f - t / 700.0f;
	if((t > 0.0f) && (t < 1.0f))
	{
		float x = woverlay->width() / 2;
		woverlay->foreground(woverlay->map_rgb(
				255 - (int)(t * 255.0f),
				(int)(t * 255.0f),
				0));
		woverlay->fillrect_fxp(PIXEL2CS((int)(x - t * 50.0f)),
				y + PIXEL2CS(16),
				PIXEL2CS((int)(t * 100.0f)),
				PIXEL2CS(10));
	}

	woverlay->font(B_MEDIUM_FONT);
	if(timeout == 10)
		woverlay->center_fxp(y + PIXEL2CS(10), "(Press FIRE)");
	else if(timeout)
	{
		woverlay->center_fxp(y + PIXEL2CS(40), "(Press FIRE)");
		counter[0] = countdown + '0';
		woverlay->font(B_COUNTER_FONT);
		woverlay->center_fxp(y, counter);
	}
}
