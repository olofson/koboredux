/*(LGPLv2.1)
---------------------------------------------------------------------------
	window.h - Generic Rendering Window
---------------------------------------------------------------------------
 * Copyright 2001-2003, 2007, 2009 David Olofson
 * Copyright 2015-2017 David Olofson (Kobo Redux)
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

/*
 * Programmer's Documentation:
 *	The window_t class represents a simple window of the
 *	screen surface. It provides some high level rendering
 *	functions that operate inside the window's bounding
 *	rectangle. All operations are clipped. However,
 *	window occlusion is *not* handled, so you can't use
 *	this class for "real" windows.
 *
 *	void place(int left, int top, int sizex, int sizey);
 *		Position the window on the screen.
 *
 *	int offscreen();
 *		Make this an off-screen window, with a
 *		surface of it's own for rendering. An off-
 *		screen window will never be directly visible,
 *		but can be used as a source window for blit().
 *		Offscreen windows support rendering outside
 *		refresh(), and do not need to implement
 *		refresh(). May return -1 if there is an error.
 *
 *	void select();
 *		Makes this window active, and sets up SDL's
 *		clipping appropriately. You should not use
 *		this call normally, but it's provided to
 *		allow this engine to interface with other
 *		SDL code.
 *
 *	void invalidate(SDL_Rect *r = NULL);
 *		Invalidate the specified area, or the whole
 *		window if r is NULL. This will ensure that
 *		any changes in the invalidated area are made
 *		visible on the screen. Note that this call
 *		may also indirectly cause one or more calls
 *		to refresh().
 *
 *	virtual void refresh(SDL_Rect *r);
 *		Virtual refresh call to be implemented by
 *		ALL windows that are not offscreen.
 *
 *	Uint32 map_rgb(Uint8 r, Uint8 g, Uint8 b);
 *	Uint32 map_rgb(Uint32 rgb);
 *		Two different ways of translating a color in
 *		standard RGB format (separate components or
 *		HTML style hex code, respectively) into a
 *		pixel value compatible with the screen.
 *
 *	void foreground(Uint32 color);
 *	void background(Uint32 color);
 *		Change the foreground and background colors
 *		used by subsequent rendering operations.
 *		Note that the arguments have to be in screen
 *		pixel format - the easiest way is to use the
 *		map_rgb() calls to generate the values.
 *
 *	void bgimage(int bank, int frame);
 *		Select a background image, to use instead of
 *		a background color. Setting bank and frame to
 *		-1 (default) disables the image, so that the
 *		background color is used instead.
 *
 *	void clear(SDL_Rect *r = NULL);
 *		Fill the specified rectangle with the current
 *		background color or background image. If r is
 *		NULL (default), clear the whole window.
 *
 *	void font(int fnt);
 *		Select the font to be used for subsequent
 *		text rendering operations. The argument is a
 *		gfxengine_t bank index, and must refer to a
 *		bank that has been loaded with
 *		gfxengine_t::loadfont().
 *
 *	void string(int _x, int _y, const char *txt);
 *		Put text string 'txt' starting at (_x, _y).
 *
 *	void center(int _y, const char *txt);
 *		Put text string 'txt' centered horizontally
 *		inside the window, using '_y' as the y coord.
 *
 *	void center_token(int _x, int _y, const char *txt, char token = 0);
 *		Print string 'txt', positioning it so that
 *		the first occurrence of character 'token'
 *		is located at (_x, _y). Omitting the 'token'
 *		argument, or passing 0 aligns the end of the
 *		string with _x. Setting 'token' to -1 results
 *		in the graphical center of the string being
 *		located at (_x, _y) - basically an advanced
 *		version of center().
 *
 *	void string_fxp(int _x, int _y, const char *txt);
 *	void center_fxp(int _y, const char *txt);
 *	void center_token_fxp(int _x, int _y, const char *txt, char token = 0);
 *		gfxengine_t fixed point format versions of
 *		the corresponding non-fxp calls. (For sub-
 *		pixel accurate rendering and/or to make use
 *		of the higher resolutions in scaled modes.)
 *
 *	int textwidth(const char *txt, int min = 0, int max = 255);
 *		Calculates the graphical width of 'txt' if it
 *		was to be printed with the current font.
 *		Calculation starts at position 'min' in the
 *		string, and ends at 'max', which makes it
 *		possible to use this call for various
 *		advanced alignment calculations.
 *
 *	int fontheight();
 *		Returns the height of the currently selected
 *		font.
 *
 *	void point(int _x, int _y);
 *		Plot a pixel with the current foreground
 *		color at (_x, _y). Note that the pixel will
 *		become a quadratic block if the screen is
 *		scaled to a higher resolution.
 *
 *	void rectangle(int _x, int _y, int w, int h);
 *		Draw a hollow rectangle of w x h pixels at
 *		(_x, _y). The line thickness is scaled along
 *		with screen resolution, as with point().
 *
 *	void fillrect(int _x, int _y, int w, int h);
 *		Draw a solid rectangle of w x h pixels at
 *		(_x, _y).
 *
 *	void fillrect_fxp(int _x, int _y, int w, int h);
 *		As fillrect(), but with pixel or sub-pixel
 *		accuracy, depending on scaling and video
 *		driver.
 *
 *	void sprite(int _x, int _y, int bank, int frame, int inval = 1);
 *		Render sprite 'bank':'frame' at (_x, _y). If
 *		inval is passed and set to 0, the affected
 *		area will *not* be invalidated automatically.
 *
 *	void sprite_fxp(int _x, int _y, int bank, int frame, int inval = 1);
 *		As sprite(), but with pixel or sub-pixel
 *		accuracy, depending on scaling and video
 *		driver.
 *
 *	void blit(int dx, int dy, window_t *src);
 *		Blit from window 'src' into this window, placing
 *		the top left corner of 'src' at (dx, dy).
 *
 *	void blit(int dx, int dy, int sx, int sy, int sw, int sh, window_t *src);
 *		Blit rectangle (sx, sy, sw, sh) from window 'src'
 *		into this window, placing the top left corner of
 *		'src' at (dx, dy).
 *
 *	int x()		{ return rect.x / xsc; }
 *	int y()		{ return rect.y / ysc; }
 *	int width()	{ return rect.w / xsc; }
 *	int height()	{ return rect.h / ysc; }
 *		Get the current screen position of the
 *		window's top-left corner, and it's size.
 *
 *	int x2()	{ return (rect.x + rect.w) / xsc; }
 *	int y2()	{ return (rect.y + rect.h) / ysc; }
 *		Get the screen position of the bottom-right
 *		corner of the window.
 */

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "gfxengine.h"
#include "SDL.h"

class gfxengine_t;

enum blendmodes_t
{
	GFX_BLENDMODE_COPY =	SDL_BLENDMODE_NONE,
	GFX_BLENDMODE_ALPHA =	SDL_BLENDMODE_BLEND,
	GFX_BLENDMODE_ADD =	SDL_BLENDMODE_ADD,
	GFX_BLENDMODE_MOD =	SDL_BLENDMODE_MOD
};

#define	GFX_DEFAULT_BLENDMODE	GFX_BLENDMODE_COPY
#define	GFX_DEFAULT_COLORMOD	0xffffffff
#define	GFX_DEFAULT_ALPHAMOD	255

enum gfx_offscreen_mode_t
{
	OFFSCREEN_DISABLED =		0,
	OFFSCREEN_RENDER_TARGET =	1,
	OFFSCREEN_SOFTWARE =		2
};

// Not actually used by the engine itself (yet); it's only here because we need
// a unified enum for this for themes and extensions.
enum gfx_dither_t
{
	GFX_DITHER_RAW,		// Use source texture pixels as is
	GFX_DITHER_NONE,	// Map to nearest palette entry
	GFX_DITHER_RANDOM,	// Random dither pattern
	GFX_DITHER_ORDERED,	// 4x4 ordered pattern
	GFX_DITHER_SKEWED,	// 4x4 skewed pattern
	GFX_DITHER_NOISE,	// Temporal noise dither
	GFX_DITHER_TEMPORAL2,	// 4x4 pattern, two frames
	GFX_DITHER_TEMPORAL4,	// 4x4 pattern, four frames
	GFX_DITHER_TRUECOLOR	// Interpolate between palette entries
};


  /////////////////////////////////////////////////////////////////////////////
 // Engine window base class
/////////////////////////////////////////////////////////////////////////////
class windowbase_t
{
	friend class gfxengine_t;
  public:
	windowbase_t(gfxengine_t *e);
	virtual ~windowbase_t();

	gfxengine_t *get_engine()	{ return engine; }

	virtual void place(int left, int top, int sizex, int sizey);
	virtual void scale(float x, float y);

	virtual void visible(bool vis);
	bool visible()	{ return _visible; }

	void autoinvalidate(bool ai)	{ _autoinvalidate = ai; }
	bool autoinvalidate()		{ return _autoinvalidate; }

	virtual void select();
	void check_select()
	{
		if(engine->selected != this)
			select();
	}

	void translate_rect(int _x, int _y, int w, int h, SDL_Rect &r)
	{
		int x1 = (_x * xs + 128) >> 8;
		int y1 = (_y * ys + 128) >> 8;
		int x2 = ((_x + w) * xs + 128) >> 8;
		int y2 = ((_y + h) * ys + 128) >> 8;
		r.x = phys_rect.x + x1;
		r.y = phys_rect.y + y1;
		r.w = x2 - x1;
		r.h = y2 - y1;
	}

	void translate_rect_fxp(int _x, int _y, int w, int h, SDL_Rect &r)
	{
		int xx = CS2PIXEL((_x * xs + 128) >> 8);
		int yy = CS2PIXEL((_y * ys + 128) >> 8);
		w = CS2PIXEL(((w + _x) * xs + 128) >> 8) - xx;
		h = CS2PIXEL(((h + _y) * ys + 128) >> 8) - yy;
		r.x = phys_rect.x + xx;
		r.y = phys_rect.y + yy;
		r.w = w;
		r.h = h;
	}

	virtual void invalidate(SDL_Rect *r = NULL)	{ ; }
	virtual void refresh(SDL_Rect *r)		{ ; }

	void foreground(Uint32 color)	{ fgcolor = color; }
	Uint32 foreground()	{ return fgcolor; }

	void background(Uint32 color)	{ bgcolor = color; }
	Uint32 background()	{ return bgcolor; }

	void colormod(Uint32 color = GFX_DEFAULT_COLORMOD)
	{
		_colormod = color;
	}
	void colormod(Uint8 r, Uint8 g, Uint8 b)
	{
		_colormod = map_rgb(r, g, b);
	}
	void alphamod(Uint8 am = GFX_DEFAULT_ALPHAMOD)
	{
		_alphamod = am;
	}
	void blendmode(blendmodes_t bm = GFX_DEFAULT_BLENDMODE)
	{
		_blendmode = bm;
	}

	void resetmod()
	{
		colormod();
		alphamod();
		blendmode();
	}

	void set_texture_params(SDL_Texture *tx)
	{
		if(_blendmode != GFX_DEFAULT_BLENDMODE)
			SDL_SetTextureBlendMode(tx,
					(SDL_BlendMode)_blendmode);
		if(_alphamod != GFX_DEFAULT_ALPHAMOD)
			SDL_SetTextureAlphaMod(tx, _alphamod);
		if(_colormod != GFX_DEFAULT_COLORMOD)
			SDL_SetTextureColorMod(tx, get_r(_colormod),
				get_g(_colormod), get_b(_colormod));
	}
	void restore_texture_params(SDL_Texture *tx)
	{
		if(_blendmode != GFX_DEFAULT_BLENDMODE)
			SDL_SetTextureBlendMode(tx,
					(SDL_BlendMode)GFX_DEFAULT_BLENDMODE);
		if(_alphamod != GFX_DEFAULT_ALPHAMOD)
			SDL_SetTextureAlphaMod(tx, GFX_DEFAULT_ALPHAMOD);
		if(_colormod != GFX_DEFAULT_COLORMOD)
			SDL_SetTextureColorMod(tx, get_r(GFX_DEFAULT_COLORMOD),
					get_g(GFX_DEFAULT_COLORMOD),
					get_b(GFX_DEFAULT_COLORMOD));
	}

	void set_render_params(SDL_Renderer *rn, Uint32 color)
	{
		Uint32 bc = mulrgba(color, (_colormod & 0xffffff) |
					(_alphamod << 24));
		if((_alphamod != 255) && (_blendmode == GFX_BLENDMODE_COPY))
			SDL_SetRenderDrawBlendMode(rn, SDL_BLENDMODE_BLEND);
		else
			SDL_SetRenderDrawBlendMode(rn,
					(SDL_BlendMode)_blendmode);
		SDL_SetRenderDrawColor(rn, get_r(bc), get_g(bc), get_b(bc),
				get_a(bc));
	}

	// Color tools
	Uint32 mulrgb(Uint32 c1, Uint32 c2)
	{
		int r = ((c1 >> 16) & 0xff) * ((c2 >> 16) & 0xff) * 258 >> 16;
		int g = ((c1 >> 8) & 0xff) * ((c2 >> 8) & 0xff) * 258 >> 16;
		int b = (c1 & 0xff) * (c2 & 0xff) * 258 >> 16;
		return r << 16 | g << 8 | b;
	}
	Uint32 mulrgba(Uint32 c1, Uint32 c2)
	{
		int a = ((c1 >> 24) & 0xff) * ((c2 >> 24) & 0xff) * 258 >> 16;
		int r = ((c1 >> 16) & 0xff) * ((c2 >> 16) & 0xff) * 258 >> 16;
		int g = ((c1 >> 8) & 0xff) * ((c2 >> 8) & 0xff) * 258 >> 16;
		int b = (c1 & 0xff) * (c2 & 0xff) * 258 >> 16;
		return a << 24 | r << 16 | g << 8 | b;
	}
	Uint32 fadergb(Uint32 rgb, Uint8 fade)
	{
		int r = ((rgb >> 16) & 0xff) * fade * 258 >> 16;
		int g = ((rgb >> 8) & 0xff) * fade * 258 >> 16;
		int b = (rgb & 0xff) * fade * 258 >> 16;
		return r << 16 | g << 8 | b;
	}

	Uint32 map_rgb(Uint8 r, Uint8 g, Uint8 b)
	{
		return 0xff000000 | (r << 16) | (g << 8) | b;
	}
	Uint32 map_rgb(Uint32 c)	{ return 0xff000000 | c; }
	Uint32 map_gray(float i)
	{
		Uint8 ii = i * 255.0f;
		return 0xff000000 | (ii << 16) | (ii << 8) | ii;
	}
	Uint32 map_rgba(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
	{
		return (a << 24) | (r << 16) | (g << 8) | b;
	}
	Uint8 get_r(Uint32 c)	{ return c >> 16; }
	Uint8 get_g(Uint32 c)	{ return c >> 8; }
	Uint8 get_b(Uint32 c)	{ return c; }
	Uint8 get_a(Uint32 c)	{ return c >> 24; }

	int px()	{ return (phys_rect.x * 256 + 128) / xs; }
	int py()	{ return (phys_rect.y * 256 + 128) / ys; }
	int px2()
	{
		return ((phys_rect.x + phys_rect.w) * 256 + 128) / xs;
	}
	int py2()
	{
		return ((phys_rect.y + phys_rect.h) * 256 + 128) / ys;
	}
	int width()	{ return px2() - px(); }
	int height()	{ return py2() - py(); }

	SDL_Rect	phys_rect;

  protected:
	windowbase_t	*next, *prev;
	gfxengine_t	*engine;
	SDL_Renderer	*renderer;	// Can be engine or local renderer!
	bool		_visible;
	bool		_autoinvalidate;// Always invalidate before rendering
	int		xs, ys;		// fixp 24:8
	blendmodes_t	_blendmode;
	Uint32		_colormod, _alphamod;
	Uint32		fgcolor, bgcolor;

	void link(gfxengine_t *e);
	void unlink(void);

	virtual void render(SDL_Rect *r);
};


  /////////////////////////////////////////////////////////////////////////////
 // Streaming window
/////////////////////////////////////////////////////////////////////////////
class stream_window_t : public windowbase_t
{
	friend class gfxengine_t;
	friend class windowbase_t;
  public:
	stream_window_t(gfxengine_t *e);
	virtual ~stream_window_t();

	void place(int left, int top, int sizex, int sizey);

	// Lock area for updating. 'pixels' is pointed at a write-only buffer
	// of 32 bit pixels of the same format as used by the windowbase_t
	// color tools API. Returns the pitch (in Uint32 pixels) of the target
	// buffer, or if the operation fails, 0.
	int lock(SDL_Rect *r, Uint32 **pixels);

	// Unlock update area previously locked with lock(). This will upload
	// the changes to the GPU, or whatever is needed to apply the changes.
	void unlock();

	// Update the specified area with data from the specified buffer. Note
	// that 'pitch' is in Uint32 pixels!
	void update(SDL_Rect *r, Uint32 *pixels, int pitch);

	void invalidate(SDL_Rect *r = NULL);

  protected:
	SDL_Texture	*texture;	// Hardware/API texture

	void render(SDL_Rect *r);
};


  /////////////////////////////////////////////////////////////////////////////
 // Normal or offscreen window
/////////////////////////////////////////////////////////////////////////////
// TODO: Offscreen should be a separate class!
class window_t : public windowbase_t
{
	friend class gfxengine_t;
	friend class windowbase_t;
  public:
	window_t(gfxengine_t *e);
	virtual ~window_t();

	void place(int left, int top, int sizex, int sizey);
	void visible(bool vis);

	void select();
	void invalidate(SDL_Rect *r = NULL);

	int offscreen();

	// Rendering
	void bgimage(int bank = -1, int frame = -1);

	void clear(SDL_Rect *r = NULL);
	void font(int fnt);
	void string(int _x, int _y, const char *txt);
	void center(int _y, const char *txt);
	void center_token(int _x, int _y, const char *txt,
			signed char token = 0);
	void string_fxp(int _x, int _y, const char *txt);
	void center_fxp(int _y, const char *txt);
	void center_token_fxp(int _x, int _y, const char *txt,
			signed char token = 0);
	int textwidth(const char *txt, int min = 0, int max = 255);
	int textwidth_fxp(const char *txt, int min = 0, int max = 255);
	int fontheight(int fnt = -1);

	void point(int _x, int _y);
	void rectangle(int _x, int _y, int w, int h);
	void rectangle_fxp(int _x, int _y, int w, int h);
	void hairrect_fxp(int _x, int _y, int w, int h);
	void fillrect(int _x, int _y, int w, int h);
	void fillrect_fxp(int _x, int _y, int w, int h);
	void circle_fxp(int _x, int _y, int r);

	void sprite(int _x, int _y, int bank, int frame);
	void sprite_fxp(int _x, int _y, int bank, int frame);
	void sprite_fxp_scale(int _x, int _y, int bank, int frame,
			float xscale, float yscale);

	void blit(int dx, int dy, int sx, int sy, int sw, int sh,
			window_t *src);
	void blit(int dx, int dy, window_t *src);

  protected:
	SDL_Texture	*otexture;	// Buffer for offscreen windows

	// Fallback for offscreen window, if there's no render target support
	SDL_Surface	*osurface;

	int		bg_bank, bg_frame;
	int		_font;

	gfx_offscreen_mode_t	_offscreen;

	void offscreen_invalidate(SDL_Rect *r);
};


  /////////////////////////////////////////////////////////////////////////////
 // Engine output window
/////////////////////////////////////////////////////////////////////////////
class engine_window_t : public window_t
{
  public:
	engine_window_t(gfxengine_t *e);
	void refresh(SDL_Rect *r);
};

#endif
