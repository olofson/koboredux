/*(LGPLv2.1)
----------------------------------------------------------------------
	gfxengine.h - Graphics Engine
----------------------------------------------------------------------
 * Copyright 2001-2003, 2007, 2009 David Olofson
 * Copyright 2008 Robert Schuster
 * Copyright 2015-2016 David Olofson (Kobo Redux)
 *
 * This library is free software;  you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation;  either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library  is  distributed  in  the hope that it will be useful,  but
 * WITHOUT   ANY   WARRANTY;   without   even   the   implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library;  if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef	_GFXENGINE_H_
#define	_GFXENGINE_H_

#define GFX_BANKS	256

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sprite.h"
#include "cs.h"
#include "palette.h"
#include "vidmodes.h"

#define MIN(x,y)	(((x)<(y)) ? (x) : (y))
#define MAX(x,y)	(((x)>(y)) ? (x) : (y))

enum gfx_scalemodes_t
{
	GFX_SCALE_NEAREST =	0,
	GFX_SCALE_BILINEAR =	1,
	GFX_SCALE_BILIN_OVER =	2,
	GFX_SCALE_SCALE2X =	3,
	GFX_SCALE_DIAMOND =	4
};

class windowbase_t;
class window_t;
class SoFont;

class gfxengine_t
{
	friend class window_t;
	friend class windowbase_t;
	friend class engine_window_t;
	friend class stream_window_t;
	int video_flags();
  public:
	gfxengine_t();
	virtual ~gfxengine_t();

	window_t *screen()	{ return fullwin; }
	window_t *target()	{ return _target; }

	void messagebox(const char *message);

	//
	// Initialization
	//
	void mode(VMM_ModeID modeid, int fullscreen);
	void size(int w, int h);
	void centered(int c);
	void scale(float x, float y);
	void camfilter(float cf);

	// 1: Enable vsync, if available
	void vsync(int use);

	void interpolation(int inter);

	void period(float frameduration);
	float period()	{	return ticks_per_frame; }

	// 0 to disable timing, running one logic frame per rendered frame.
	// 1 to disable filtering, using raw delta times for timing.
	void timefilter(float coeff);

	void wrap(int x, int y);
	int get_wrapx()	{	return wrapx; }
	int get_wrapy()	{	return wrapy; }

	void scroll_ratio(int layer, float xr, float yr);

	// Engine open/close
	int open(int objects = 1024, int extraflags = 0);
	void close();

	// Data management (use while engine is open)
	void reset_filters();
	void filterflags(int fgs);
	void scalemode(gfx_scalemodes_t sm, int clamping = 0);
	void source_scale(float x, float y);	// 0.0f ==> no scaling!
	void absolute_scale(float x, float y);
	void colorkey(Uint8 r, Uint8 g, Uint8 b);
	void clampcolor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	void dither(int type = 0);
	void noalpha(int threshold = 128);
	void brightness(float bright, float contr);
	int load_palette(const char *path);
	uint32_t palette(unsigned ind);

	int loadimage(int bank, const char *name);
	int loadtiles(int bank, int w, int h, const char *name);
	int loadfont(int bank, const char *name, float srcscale);
	int copyrect(int bank, int sbank, int sframe, SDL_Rect *r);
	void draw_scale(int bank, float _xs, float _ys);
	s_bank_t *alias_bank(int bank, int orig);

	int is_loaded(int bank);
	void reload();
	void unload(int bank = -1);

	// Settings (use while engine is open)
	void title(const char *win, const char *icon);

	// Display show/hide
	bool check_mode_autoswap(int *, int *);
	int show();
	void hide();

	// Main loop take-over
	void run();

	//
	// Override these;
	//
	//	pre_loop() is called right before entering the engine main
	//		loop.
	//
	//	pre_advance() is called right before advancing the game logic
	//		to the time of the upcoming video frame.
	//
	//	frame() is called once per control system frame, after the
	//		control system has executed.
	//
	//	pre_render() is called after the engine has advanced to the
	//		state for the current video frame (interpolated state
	//		is calculated), before the engine renders all graphics.
	//
	//	post_render() is called after the engine has rendered all
	//		sprites, but before video the sync/flip/update
	//		operation.
	//
	//	post_loop() is called when the engine leaves the main loop.
	//
	virtual void pre_loop();
	virtual void pre_advance(float fractional_frame);
	virtual void frame();
	virtual void pre_render();
	virtual void post_render();
	virtual void post_loop();

	////////////////////////////////////////////
	// The members below can safely be called
	// from within the frame() handler.
	////////////////////////////////////////////

	// Rendering
	void clear(Uint32 _color = 0x000000);

	// Screenshots
	void screenshot();

	// Control
	SDL_Renderer *renderer()	{ return sdlrenderer; }
	cs_engine_t *cs()		{ return csengine; }
	void present();		// Render all visible windows to display
	void stop();
	cs_obj_t *get_obj(int layer);
	void free_obj(cs_obj_t *obj);
	void cursor(int csr);

	s_container_t *get_gfx()	{ return gfx; }
	s_sprite_t *get_sprite(unsigned bank, int _frame)
	{
		return s_get_sprite(gfx, bank, _frame);
	}
	SoFont *get_font(unsigned bank);
	int set_hotspot(unsigned bank, int _frame, int x, int y)
	{
		return s_set_hotspot(gfx, bank, _frame, x, y);
	}

	void scroll(int xs, int ys);
	void force_scroll();

	// Scroll offsets
	int xoffs(int layer);
	int yoffs(int layer);

	// Normalized scroll offsets (only valid with wrapping!)
	float nxoffs(int layer);
	float nyoffs(int layer);

	double frame_delta_time()	{ return _frame_delta_time; }

	// Info
	int objects_in_use();

	int width()		{ return _width; }
	int height()		{ return _height; }
	float xscale()		{ return xs * (1.0f / 256.0f); }
	float yscale()		{ return ys * (1.0f / 256.0f); }

  protected:
	gfx_scalemodes_t	_scalemode;
	int			_clamping;
	SDL_Window	*sdlwindow;
	SDL_Renderer	*sdlrenderer;
	window_t	*fullwin;
	window_t	*_target;
	windowbase_t	*windows;	// Linked list
	windowbase_t	*selected;	// Currently selected for rendering
	int		xs, ys;		// Display scale (fixp 24:8)
	int		sxs, sys;	// Load scale (fixp 24:8)
	s_filter_t	*sf1, *sf2;	// Scaling filter plugins
	s_filter_t	*acf;		// Alpha cleaning plugin
	s_filter_t	*bcf;		// Brightness/contrast plugin
	s_filter_t	*dsf;		// Display format plugin

	// Scrolling and wrapping
	int		_camfilter;		// Camera filter, or 0 (24:8)
	int		xtarget, ytarget;	// Scroll target position
	int		xscroll, yscroll;	// Current scroll position
	int		wrapx, wrapy;		// World wrap, or 0
	float		xratio[CS_LAYERS];	// Layer scroll ratios
	float		yratio[CS_LAYERS];

	s_container_t	*gfx;
	GFX_palette	*_palette;
	cs_engine_t	*csengine;
	VMM_ModeID	_modeid;
	int		_fullscreen;
	int		xflags;
	int		_pages;
	int		_vsync;
	int		_centered;
	cs_filtermode_t	motion_filter_mode;
	int		_width, _height;
	const char	*_title;
	const char	*_icontitle;
	int		_cursor;
	int		_dither;
	int		_dither_type;
	int		alpha_threshold;	//For noalpha()
	float		_brightness;
	float		_contrast;

	int		last_tick;
	float		ticks_per_frame;
	float		_timefilter;
	double		_frame_delta_time;

	int		is_showing;
	int		is_running;
	int		is_open;

	int		screenshot_count;

	static void on_frame(cs_engine_t *e);

	void start_engine();
	void stop_engine();
	static void render_sprite(cs_obj_t *o);
	void apply_scroll();
};


extern gfxengine_t *gfxengine;

#endif
