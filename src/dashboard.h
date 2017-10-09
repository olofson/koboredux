/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2003, 2007, 2009 David Olofson
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

#ifndef KOBO_DASHBOARD_H
#define KOBO_DASHBOARD_H

#include "window.h"
#include "starfield.h"
#include "gridtfx.h"


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

enum dashboard_transitions_t {
	DASHBOARD_INSTANT = 0,
	DASHBOARD_SLOW,
	DASHBOARD_FAST,
	DASHBOARD_IN_ONLY
};

const char *enumstr(dashboard_modes_t dbm);
const char *enumstr(dashboard_transitions_t dbt);

// Dashboard window; dashboard or loading screen
class dashboard_window_t : public window_t
{
	char			*_msg;
	float			_percent;
	dashboard_modes_t	_mode;
	dashboard_modes_t	new_mode;
	dashboard_transitions_t trmode;
	bool			transitioning;
	float			_fade;
	KOBO_Starfield		jingelstars;
	KOBO_GridTFX		gridtfx;
	Uint32			last_update;
	void render_progress();
	void update(bool force);
  public:
	dashboard_window_t(gfxengine_t *e);
	~dashboard_window_t();
	void transition(dashboard_transitions_t tr);
	void mode(dashboard_modes_t m,
			dashboard_transitions_t tr = DASHBOARD_INSTANT);
	dashboard_modes_t mode()	{ return _mode; }
	bool busy(bool trans = false);
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
	void render_final();
};


// Label
class label_t : public window_t
{
  protected:
	char	_caption[1024];
	int	_on;
	Uint32	_color;
  public:
	label_t(gfxengine_t *e);
	void refresh(SDL_Rect *r);
	void color(Uint32 _cl);
	void caption(const char *cap);
	void on();
	void off();
};


// Labeled text display
class display_t : public label_t
{
  protected:
	char	_text[64];
  public:
	display_t(gfxengine_t *e);
	void refresh(SDL_Rect *r);
	void text(const char *txt);
};


// Bar graph display (base)
class bargraph_t : public window_t
{
  protected:
	float	_value;
	float	fvalue;
	int	_y;
	int	_enabled;
	int	led_bank;
	float	_warn_level;
	float	_ok_level;
  public:
	bargraph_t(gfxengine_t *e);
	void refresh(SDL_Rect *r);
	void value(float val);
	void enable(int ena);
	void set_leds(int _bank)	{ led_bank = _bank; }
	void warn_level(float l)	{ _warn_level = l; }
	void ok_level(float l)		{ _ok_level = l; }
	int led_count();	// Returns number of LEDs with current theme
};


// Health/shield LED bar display with overcharge
class shieldbar_t : public bargraph_t
{
  protected:
	float	_marker;
	float	_timer;
  public:
	shieldbar_t(gfxengine_t *e);
	void refresh(SDL_Rect *r);
	void marker(float m)		{ _marker = m; }
	void timer(float t)		{ _timer = t; }
};


// Health/shield LED bar display with overcharge
class chargebar_t : public bargraph_t
{
  public:
	chargebar_t(gfxengine_t *e);
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
	int		led_bank;
	proxy_led_t	leds[PROXY_LEDS];
	proxy_fxtypes_t	fxtype;
	proxy_colors_t	fxcolor;
	int		fxstate;
	int		fxpos;
  public:
	hledbar_t(gfxengine_t *e);
	void set_leds(int _bank)	{ led_bank = _bank; }
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


// Weapon slot
class weaponslot_t : public window_t
{
  protected:
	int	_slot;
  public:
	weaponslot_t(gfxengine_t *e);
	void refresh(SDL_Rect *r);
	void slot(int s)	{ _slot = s; }
};

#endif /* KOBO_DASHBOARD_H */
