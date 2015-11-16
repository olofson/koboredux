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
#include "game.h"
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
	_msg = NULL;
	_percent = 0.0f;
	_mode = DASHBOARD_BLACK;
	_fade = 1.0f;
	progress_index = 0;
	progress_table = NULL;
}


dashboard_window_t::~dashboard_window_t()
{
	free(_msg);
}


void dashboard_window_t::mode(dashboard_modes_t m)
{
	const int psize = 288;
	int main = 0;
	int score = 0;
	int ingame = 0;
	switch(m)
	{
	  case DASHBOARD_TITLE:
		main = score = 1;
		break;
	  case DASHBOARD_GAME:
		main = ingame = score = 1;
		break;
	  default:
		break;
	}
	// NOTE: wplanet is actually rendered in LOADING and JINGLE modes, but
	//       due to the rendering order there, it's done manually by
	//       dashboard_window_t::refresh()!
	wplanet->visible(main);
	wmain->visible(main);
	wshield->visible(main);
	wradar->visible(main);
	dhigh->visible(score);
	dscore->visible(score);
	dstage->visible(ingame);
	dregion->visible(ingame);
	dlevel->visible(ingame);
	pxtop->visible(main);
	pxbottom->visible(main);
	pxleft->visible(main);
	pxright->visible(main);

	switch(m)
	{
	  case DASHBOARD_JINGLE:
		// Don't reinit when switching from LOADING to JINGLE!
		if(_mode == DASHBOARD_LOADING)
			break;
	  case DASHBOARD_LOADING:
		wplanet->colormod(wplanet->map_rgb(128, 128, 128));
		wplanet->track_speed(1.0f, 1.0f);
		wplanet->track_offset(0.0f, 0.0f);
		wplanet->set_texture_repeat(2);
		wplanet->set_size(psize);
		wplanet->set_source(B_OAPLANET, 0);
		wplanet->set_dither(SPINPLANET_DITHER_RAW, 0, 0);
		wplanet->set_mode(SPINPLANET_SPIN);
		jingelstars.set_target(this);
		jingelstars.init(1000, 100, psize);
		break;
	  default:
		break;
	}

	_mode = m;
	switch(m)
	{
	  case DASHBOARD_OFF:
		break;
	  default:
		gengine->present();
		break;
	}
}


void dashboard_window_t::doing(const char *msg)
{
	free(_msg);
	_msg = strdup(msg);
	gengine->present();
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
		_percent = 0.0f;
	gengine->present();
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
	else
	{
		_percent = 100.0f;
		gengine->present();
	}
	progress_table = NULL;
}


void dashboard_window_t::render_progress()
{
	int x, y, w, h;
	Uint32 colors[3];
	const char entries[] = {
		52,	51,	50
	};
	for(int i = 0; i < 3; ++i)
		colors[i] = map_rgb(get_engine()->palette(entries[i]));

	x = 0;
	w = (int)(_percent * 0.01f * width() + 0.5f);
	if(w < 4)
		w = 4;
	else if(w > width())
		w = width();

	h = 5;
	y = height() - h;

	foreground(colors[0]);
	rectangle(x, y, w, h);

	++x;
	++y;
	w -= 2;
	h -= 2;
	foreground(colors[1]);
	rectangle(x, y, w, h);

	++x;
	++y;
	w -= 2;
	h -= 2;
	foreground(colors[2]);
	fillrect(x, y, w, h);
	
	if(_msg)
	{
		font(B_NORMAL_FONT);
		center(height() - 40, _msg);
	}
}


void dashboard_window_t::refresh(SDL_Rect *r)
{
	double t = SDL_GetTicks() * 15;
	switch(_mode)
	{
	  case DASHBOARD_OFF:
		break;
	  case DASHBOARD_BLACK:
		background(map_rgb(0x000000));
		clear();
		break;
	  case DASHBOARD_NOISE:
	  {
		const char entries[] = {
			0,	1,	35,	53,
			33,	54,	32,	55
		};
		int c = entries[0];
		for(int y = 0; y < height(); ++y)
		{
			if(!pubrand.get(2))
			{
				int ci = pubrand.get(3) * _fade + 0.5f;
				c = entries[ci];
			}
			foreground(map_rgb(get_engine()->palette(c)));
			fillrect(0, y, width(), 1);
		}
		break;
	  }
	  case DASHBOARD_TITLE:
	  case DASHBOARD_GAME:
		colormod(map_gray(_fade));
		sprite(0, 0, B_SCREEN, 0);
		break;
	  case DASHBOARD_LOADING:
	  case DASHBOARD_JINGLE:
		background(map_rgb(0x000000));
		clear();
		wplanet->colormod(wplanet->map_gray(_fade * 128.0f));
		wplanet->track_offset(t * 0.0000015f + 0.2f,
				t * 0.0000015f + 0.6f);
		wplanet->render(NULL);
		colormod(map_gray(_fade));
		jingelstars.render(t * 2.0f, t * 2.0f);
		sprite(width() / 2, height() / 2, B_OALOGO, 0);
		if(_mode == DASHBOARD_LOADING)
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
	Bar graph display (base)
----------------------------------------------------------*/

bargraph_t::bargraph_t(gfxengine_t *e) : window_t(e)
{
	_value = 0.0f;
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


/*----------------------------------------------------------
	Plain bar graph display
----------------------------------------------------------*/

plainbar_t::plainbar_t(gfxengine_t *e) : bargraph_t(e)
{
	_redmax = 1;
}


void plainbar_t::refresh(SDL_Rect *r)
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
	Health/shield LED bar display with overcharge
----------------------------------------------------------*/

enum shield_colors_t {
	SCOLORS_OFF = 0,	// Off
	SCOLORS_RED = 1,	// Almost off to full red gradient
	SCOLORS_YELLOW = 5,	// Almost off to full yellow gradient
	SCOLORS_GREEN = 9,	// Almost off to full green gradient
	SCOLORS_BLUE = 13,	// Green to bright blue gradient
};


shieldbar_t::shieldbar_t(gfxengine_t *e) : bargraph_t(e)
{
	fvalue = 0.0f;
	led_bank = 0;
	_marker = 0.0f;
}


void shieldbar_t::refresh(SDL_Rect *r)
{
	s_bank_t *b = s_get_bank(engine->get_gfx(), led_bank);
	if(!b)
		return;

	int led_height = b->h;
	int leds = height() / led_height;
	if(!_enabled)
	{
		for(int i = 0; i < leds; ++i)
			sprite(0, i * led_height, led_bank, SCOLORS_OFF);
		fvalue = _value;
		return;
	}

	int off, c0;

	// Filtering
	fvalue += (_value - fvalue) * SHIELD_FILTER_COEFF *
			engine->frame_delta_time();
	float v = fvalue;

#ifdef	SHIELD_DITHER
	// Dithering
	v += (pubrand.get(4) - 7.5f) / 16.0f / (leds * SHIELD_GRADIENT_SIZE);
#endif

	// Normal/overcharge
	int marker_pos;
	if(v <= 1.0f)
	{
		// Normal
		off = SCOLORS_OFF;
		if(v < 0.25f)
			c0 = SCOLORS_RED;
		else if(v < 0.5f)
			c0 = SCOLORS_YELLOW;
		else
			c0 = SCOLORS_GREEN;
		if(_marker)
		{
			marker_pos = _marker * leds - 0.5f;
			if(marker_pos > leds)
				marker_pos = leds - 1;
		}
		else
			marker_pos = -1;
	}
	else
	{
		// Overcharge
		off = SCOLORS_GREEN + SHIELD_GRADIENT_SIZE - 1;
		c0 = SCOLORS_BLUE;
		v -= 1.0f;
		marker_pos = -1;		// No marker!
	}

	// Rounding
	v += 0.5f / (leds * SHIELD_GRADIENT_SIZE);

	// Render!
	int led = v * leds;
	int frame = fmod(v * leds, 1.0f) * SHIELD_GRADIENT_SIZE;
	for(int i = 0; i < leds; ++i)
	{
		int c;
		if(i < led)
			c = c0 + SHIELD_GRADIENT_SIZE - 1;
		else if(i > led || !frame)
		{
			if(i == marker_pos && SDL_GetTicks() % 300 > 200)
				c = SCOLORS_GREEN;
			else
				c = off;
		}
		else
			c = c0 + frame - 1;
		sprite(0, (leds - 1 - i) * led_height, led_bank, c);
	}
}


/*----------------------------------------------------------
	Horizontal LED bar display
----------------------------------------------------------*/

hledbar_t::hledbar_t(gfxengine_t *e) : window_t(e)
{
	memset(leds, 0, sizeof(leds));
	fxtype = PFX_OFF;
	fxcolor = PCOLOR_OFF;
	fxstate = fxpos = 0;
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
		log_printf(DLOG, "hledbar_t::set(): Unrecognized "
				"'color' %d!\n", color);
		return;
	}
	if((intensity < 0.0f) || (intensity > 1.0f))
	{
		log_printf(DLOG, "hledbar_t::set(): 'intensity' out "
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
	  case PFX_BLINK:
		fxstate = PROXY_FLASH_PERIOD;
		break;
	  case PFX_RUN:
		fxstate = 1;
		fxpos = 0;
		break;
	  case PFX_RUNREV:
		fxstate = -1;
		fxpos = PROXY_LEDS - PROXY_SCAN_WIDTH - 1;
		break;
	  case PFX_SCAN:
		fxstate = 1;
		fxpos = -PROXY_SCAN_WIDTH;
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
	  case PFX_BLINK:
		if(!fxstate)
			fxstate = PROXY_FLASH_PERIOD;
		// Fall through!
	  case PFX_FLASH:
		reset();
		if(fxstate > PROXY_FLASH_PERIOD - PROXY_FLASH_DURATION)
		{
			float fi = fxstate - (PROXY_FLASH_PERIOD -
					PROXY_FLASH_DURATION);
			fi /= PROXY_FLASH_DURATION;
			for(int i = 0; i < PROXY_LEDS; ++i)
				set(i, fxcolor, fi);
		}
		if(fxstate)
			--fxstate;
		break;
	  case PFX_RUN:
	  case PFX_RUNREV:
		reset();
		for(int i = 0; i < PROXY_SCAN_WIDTH; ++i)
			set((fxpos + i) % PROXY_LEDS, fxcolor, 1.0f);
		fxpos += fxstate;
		if(fxpos < 0)
			fxpos += PROXY_LEDS;
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
			leds[i].intensity += PROXY_LIGHTSPEED;
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
		sprite(i * PROXY_LED_SIZE, 0, B_HLEDS, leds[i].frame);
}


/*----------------------------------------------------------
	Vertical LED bar display
----------------------------------------------------------*/

void vledbar_t::refresh(SDL_Rect *r)
{
	for(int i = 0; i < PROXY_LEDS; ++i)
		sprite(0, i * PROXY_LED_SIZE, B_VLEDS, leds[i].frame);
}
