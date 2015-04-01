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

#include "config.h"
#include "kobo.h"
#include "random.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>

#define MAX_PROGRESS	1000


/*----------------------------------------------------------
	Screen
----------------------------------------------------------*/

void screen_window_t::border(int top, int left, int right, int bottom)
{
	_top = top;
	_left = left;
	_right = right;
	_bottom = bottom;
}


void screen_window_t::refresh(SDL_Rect *r)
{
	int x, y, w, h;
	foreground(map_rgb(0x000000));
	x = 0;
	y = 0;
	w = width();
	h = _top;
	fillrect(x, y, w, h);

	y = height() - _bottom;
	h = _bottom;
	fillrect(x, y, w, h);

	y = _top;
	w = _left;
	h = height() - _top - _bottom;
	fillrect(x, y, w, h);

	x = width() - _right;
	w = _right;
	fillrect(x, y, w, h);
}


/*----------------------------------------------------------
	Dashboard
----------------------------------------------------------*/

dashboard_window_t::dashboard_window_t(gfxengine_t *e) : window_t(e)
{
	_mode = DASHBOARD_BLACK;
	_percent = 0.0f;
	_msg = NULL;
}


dashboard_window_t::~dashboard_window_t()
{
	free(_msg);
}


void dashboard_window_t::mode(dashboard_modes_t m)
{
	int vis;
	_mode = m;
	switch(_mode)
	{
	  case DASHBOARD_GAME:
		vis = 1;
		break;
	  default:
		vis = 0;
		break;
	}
	wmain->visible(vis);
#if 0
	whealth->visible(vis);
	wtemp->visible(vis);
	wttemp->visible(vis);
#else
	whealth->visible(0);
	wtemp->visible(0);
	wttemp->visible(0);
#endif
	wradar->visible(vis);
	dhigh->visible(vis);
	dscore->visible(vis);
	dstage->visible(vis);
	dships->visible(vis);
	pxtop->visible(vis);
	pxbottom->visible(vis);
	pxleft->visible(vis);
	pxright->visible(vis);

	switch(_mode)
	{
	  case DASHBOARD_OFF:
		break;
	  default:
		gengine->flip();
		break;
	}
}


void dashboard_window_t::doing(const char *msg)
{
	free(_msg);
	_msg = strdup(msg);
	gengine->flip();
}


void dashboard_window_t::progress_init(float *progtab)
{
	if(!progtab)
	{
		progress_table = (float *)malloc(MAX_PROGRESS * sizeof(float));
		progress_bench = (progress_table != NULL);
	}
	else
	{
		progress_table = progtab;
		progress_bench = 0;
	}
	progress_index = 0;
}


void dashboard_window_t::progress()
{
	if(progress_table)
	{
		if(progress_bench)
		{
			progress_table[progress_index++] = (float)SDL_GetTicks();
			_percent = 0.0f;
		}
		else
			_percent = progress_table[progress_index++];
	}
	else
		_percent = 50.0f;
	gengine->flip();
}


void dashboard_window_t::progress_done()
{
	if(progress_bench)
	{
		int i;
		int total = SDL_GetTicks() - (int)progress_table[0];
		printf("Progress percentages:\n");
		printf("---------------------\n");
		for(i = 0; i < progress_index; ++i)
			printf("\t%f,\n", (progress_table[i] -
					progress_table[0]) * 100.0f / total);
		printf("---------------------\n");
		free(progress_table);
		progress_bench = 0;
	}
	progress_table = NULL;
}


void dashboard_window_t::render_progress()
{
	SDL_Rect r;
	int x, y, w, h;

	x = 0;
	w = (int)(_percent * 0.01f * width() + 0.5f);
	if(w < 4)
		w = 4;
	else if(w > width())
		w = width();

	h = 16;
	y = height() - h;

	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;

	foreground(map_rgb(0x000099));
	rectangle(x, y, w, h);

	++x;
	++y;
	w -= 2;
	h -= 2;
	foreground(map_rgb(0x0000cc));
	rectangle(x, y, w, h);

	++x;
	++y;
	w -= 2;
	h -= 2;
	foreground(map_rgb(0x0000ff));
	fillrect(x, y, w, h);
	
	r.x = 0;
	r.y = height() - 40;
	r.w = width();
	r.h = 12;
	foreground(map_rgb(0x000000));
	fillrect(r.x, r.y, r.w, r.h);
	if(_msg)
	{
		font(B_NORMAL_FONT);
		center(height() - 40, _msg);
	}
}


void dashboard_window_t::refresh(SDL_Rect *r)
{
	switch(_mode)
	{
	  case DASHBOARD_OFF:
		break;
	  case DASHBOARD_BLACK:
		background(map_rgb(0x000000));
		clear();
		break;
	  case DASHBOARD_GAME:
		sprite(0, 0, B_SCREEN, 0);
		break;
	  case DASHBOARD_LOADING:
		background(map_rgb(0x000000));
		clear();
		sprite(width() / 2, height() / 3, B_LOADING, 0);
		render_progress();
		break;
	}
}


/*----------------------------------------------------------
	Labeled text display
----------------------------------------------------------*/

display_t::display_t(gfxengine_t *e) : window_t(e)
{
	_color = map_rgb(0);
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


/*----------------------------------------------------------
	Bar graph display
----------------------------------------------------------*/

bargraph_t::bargraph_t(gfxengine_t *e) : window_t(e)
{
	_value = 0.0f;
	_redmax = 1;
	_y = -1000;
	_enabled = 1;
}


void bargraph_t::value(float val)
{
	if(val < 0.0f)
		_value = 0.0f;
	else
		_value = val;
	if(!_enabled)
		return;
	int y = (int)((height() - 2) * _value);
	if(y != _y)
		invalidate();
}


void bargraph_t::enable(int ena)
{
	_enabled = ena;
	invalidate();
}


void bargraph_t::refresh(SDL_Rect *r)
{
	if(!_enabled)
	{
		clear();
		return;
	}
	float v = _value;
	int red, green, blue;
	if(v > 1.0f)
	{
		blue = 50 + (v - 1.0f) * 512;
		if(_redmax)
		{
			red = 255;
			green = blue / 2;
		}
		else
		{
			green = 255;
			red = blue / 2;
		}
		v = 1.0f;
	}
	else
	{
		if(_redmax)
		{
			red = (int)(v * 300.0);
			green = (int)((1.0 - v) * 400.0);
		}
		else
		{
			red = (int)((1.0 - v) * 300.0);
			green = (int)(v * 400.0);
		}
		blue = 50;
	}
	if(green > 180)
		green = 180;
	if(red > 230)
		red = 230;
	if(blue > 255)
		blue = 255;
	_y = (int)((height() - 2) * (1.0f - v));
	foreground(bgcolor);
	fillrect(0, 0, width(), height());
	foreground(map_rgb(red, green, blue));
	fillrect(1, _y + 1, width() - 2, height() - _y - 2);
}


/*----------------------------------------------------------
	Horizontal LED bar display
----------------------------------------------------------*/

hledbar_t::hledbar_t(gfxengine_t *e) : window_t(e)
{
	memset(leds, 0, sizeof(leds));
}


void hledbar_t::reset()
{
	for(int i = 0; i < PROXY_LEDS; ++i)
	{
		leds[i].tcolor = PCOLOR_OFF;
		leds[i].tintensity = 0;
	}
}


void hledbar_t::set(int pos, proxy_colors_t color, float intensity)
{
	if((pos < 0) || (pos >= PROXY_LEDS))
		return;
#ifdef DEBUG
	if((color < 0) || (color > PCOLOR_HAZARD))
	{
		log_printf(DLOG, stderr, "hledbar_t::set(): Unrecognized "
				"'color' %d!\n", color);
		return;
	}
	if((intensity < 0.0f) || (intensity > 1.0f))
	{
		log_printf(DLOG, stderr, "hledbar_t::set(): 'intensity' out "
				"of range! (%f)\n", intensity);
		return;
	}
#endif
	int ii = (int)(intensity * 65536.0f);
	if((color >= leds[pos].tcolor) && (ii > leds[pos].tintensity))
	{
		leds[pos].tcolor = color;
		leds[pos].tintensity = ii;
	}
}


void hledbar_t::fx(proxy_fxtypes_t fxt, proxy_colors_t color)
{
	reset();
	fxtype = fxt;
	fxcolor = color;
	switch(fxtype)
	{
	  case PFX_OFF:
		break;
	  case PFX_FLASH:
		break;
	  case PFX_BLINK:
		break;
	  case PFX_RUN:
		break;
	  case PFX_RUNREV:
		break;
	  case PFX_SCAN:
		fxstate = 1;
		fxpos = 0;
		break;
	  case PFX_SCANREV:
		fxstate = -1;
		fxpos = PROXY_LEDS;
		break;
	}
}


void hledbar_t::frame()
{
	// Run effects, if any
	switch(fxtype)
	{
	  case PFX_OFF:
		break;
	  case PFX_FLASH:
		break;
	  case PFX_BLINK:
		break;
	  case PFX_RUN:
		break;
	  case PFX_RUNREV:
		break;
	  case PFX_SCAN:
	  case PFX_SCANREV:
		reset();
		for(int i = 0; i < PROXY_SCAN_WIDTH; ++i)
			set(fxpos + i, fxcolor, 1.0f);
		fxpos += fxstate;
		if(fxpos <= -PROXY_SCAN_WIDTH)
		{
			fxstate = -fxstate;
			fxpos = -PROXY_SCAN_WIDTH;
		}
		else if(fxpos >= PROXY_LEDS)
		{
			fxstate = -fxstate;
			fxpos = PROXY_LEDS;
		}
		break;
	}

	// Update LEDs!
	for(int i = 0; i < PROXY_LEDS; ++i)
	{
		if(leds[i].color != leds[i].tcolor)
		{
			// Fade out to switch color
			leds[i].intensity -= PROXY_FADESPEED;
			if(leds[i].intensity <= 0)
			{
				leds[i].intensity = 0;
				leds[i].color = leds[i].tcolor;
			}
		}
		else if(leds[i].intensity < leds[i].tintensity)
		{
			// Fade up to target level
			leds[i].intensity += PROXY_FADESPEED;
			if(leds[i].intensity > leds[i].tintensity)
				leds[i].intensity = leds[i].tintensity;
		}
		else if(leds[i].intensity > leds[i].tintensity)
		{
			// Fade down to target level
			leds[i].intensity -= PROXY_FADESPEED;
			if(leds[i].intensity < leds[i].tintensity)
				leds[i].intensity = leds[i].tintensity;
		}
		else
			continue;	// Done! No change.

		// Calculate graphics frame to render
		if(leds[i].intensity < 3 * 65536 / 8)
		{
			leds[i].frame = 0;	// Dark
			continue;
		}
		switch(leds[i].color)
		{
		  case PCOLOR_OFF:
			leds[i].frame = 0;	// Dark
			continue;
		  case PCOLOR_PICKUP:
			leds[i].frame = 4;	// Green
			break;
		  case PCOLOR_CORE:
			leds[i].frame = 7;	// Blue
			break;
		  case PCOLOR_BOSS:
			leds[i].frame = 10;	// Yellow
			break;
		  case PCOLOR_HAZARD:
			leds[i].frame = 1;	// Red
			break;
		}
		leds[i].frame += (leds[i].intensity - 3 * 65536 / 8) /
				(65536 / 4);
	}
}


void hledbar_t::refresh(SDL_Rect *r)
{
	for(int i = 0; i < PROXY_LEDS; ++i)
		sprite(i * 8, 0, B_HLEDS, leds[i].frame);
}


/*----------------------------------------------------------
	Vertical LED bar display
----------------------------------------------------------*/

void vledbar_t::refresh(SDL_Rect *r)
{
	for(int i = 0; i < PROXY_LEDS; ++i)
		sprite(0, i * 8, B_VLEDS, leds[i].frame);
}
