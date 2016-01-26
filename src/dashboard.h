/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2003, 2007, 2009 David Olofson
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

#ifndef KOBO_DASHBOARD_H
#define KOBO_DASHBOARD_H

#include "window.h"
#include "starfield.h"


// "Screen" window; takes care of the border, if any
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
	DASHBOARD_NOISE,
	DASHBOARD_TITLE,
	DASHBOARD_GAME,
	DASHBOARD_LOADING,
	DASHBOARD_JINGLE
};


// Dashboard window; dashboard or loading screen
class dashboard_window_t : public window_t
{
	char			*_msg;
	float			_percent;
	dashboard_modes_t	_mode;
	float			_fade;
	KOBO_Starfield		jingelstars;
	void render_progress();
  public:
	dashboard_window_t(gfxengine_t *e);
	~dashboard_window_t();
	void mode(dashboard_modes_t m);
	void fade(float f)
	{
		if(f <= 0.0f)
			_fade = 0.0f;
		else if(f >= 1.0f)
			_fade = 1.0f;
		else
			_fade = f;
	}
	void doing(const char *msg);
	void show_progress();
	void progress_init(float *progtab);
	void progress(float done);
	void progress_done();
	void refresh(SDL_Rect *r);
};


// Labeled text display
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


// Bar graph display (base)
class bargraph_t : public window_t
{
  protected:
	float	_value;
	int	_y;
	int	_enabled;
  public:
	bargraph_t(gfxengine_t *e);
	void value(float val);
	void enable(int ena);
};


// Plain bar graph display
class plainbar_t : public bargraph_t
{
	int	_redmax;
  public:
	plainbar_t(gfxengine_t *e);
	void refresh(SDL_Rect *r);
	void redmax(int rm)
	{
		_redmax = rm;
	}
};


// Health/shield LED bar display with overcharge
class shieldbar_t : public bargraph_t
{
	float	fvalue;
	int	led_bank;
	float	_marker;
  public:
	shieldbar_t(gfxengine_t *e);
	void set_leds(int _bank)	{ led_bank = _bank; }
	void marker(float m)		{ _marker = m; }
	void refresh(SDL_Rect *r);
};


// NOTE: These are in order of increasing priority!
enum proxy_colors_t {
	PCOLOR_OFF,
	PCOLOR_PICKUP,	// Powerups, bonuses etc
	PCOLOR_CORE,	// Primary targets (remaining base cores)
	PCOLOR_BOSS,	// Assassins, bosses and the like
	PCOLOR_HAZARD	// Close or fast approaching enemies or other threats
};

enum proxy_fxtypes_t {
	PFX_OFF,	// Normal operation via reset()/set()
	PFX_FLASH,	// Flash all LEDs once
	PFX_BLINK,	// Repeatedly flash all LEDs
	PFX_RUN,	// Running lights, forward (right/down)
	PFX_RUNREV,	// Running lights, reverse (left/up)
	PFX_SCAN,	// Single LED bidirectional scanner
	PFX_SCANREV	// Single LED bidirectional scanner, reverse phase
};

struct proxy_led_t
{
	uint8_t	tcolor;		// Target color
	uint8_t	color;		// Current color
	int	tintensity;	// Target intensity (16:16 fixp)
	int	intensity;	// Current intensity (16:16 fixp)
	uint8_t	frame;		// Current gfx frame
};

// Horizontal LED bar display
class hledbar_t : public window_t
{
  protected:
	proxy_led_t	leds[PROXY_LEDS];
	proxy_fxtypes_t	fxtype;
	proxy_colors_t	fxcolor;
	int		fxstate;
	int		fxpos;
  public:
	hledbar_t(gfxengine_t *e);
	void reset();
	void set(int pos, proxy_colors_t color, float intensity);
	void fx(proxy_fxtypes_t fxt, proxy_colors_t color = PCOLOR_HAZARD);
	void refresh(SDL_Rect *r);
	void frame(void);
};


// Vertical LED bar display
class vledbar_t : public hledbar_t
{
  public:
	vledbar_t(gfxengine_t *e) : hledbar_t(e) { };
	void refresh(SDL_Rect *r);
};

#endif /* KOBO_DASHBOARD_H */
