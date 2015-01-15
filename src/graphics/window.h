/*(LGPLv2.1)
---------------------------------------------------------------------------
	window.h - Generic Rendering Window
---------------------------------------------------------------------------
 * Copyright 2001-2003, 2007, 2009 David Olofson
 * Copyright 2015 David Olofson (Kobo Redux)
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
 *	virtual void init(gfxengine_t *e);
 *		Call this to connect a window_t instance to
 *		the gfxengine_t instance that manages your
 *		screen. You *must* call this for pretty much
 *		any other calls to work.
 *
 *	void place(int left, int top, int sizex, int sizey);
 *		Position the window on the screen.
 *
 *	int offscreen();
 *		Make this an off-screen window, with a
 *		surface of it's own for rendering. An off-
 *		screen window will never be directly visible,
 *		but can be used as a source window for blit().
 *		May return -1 if there is an error.
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
 *		windows that aren't repainted every frame.
 *		After a call to invalidate(), refresh() will
 *		be called by the engine as necessary to
 *		produce a correct display. Note that in some
 *		cases, this means that refresh() will be
 *		called once for each screen flip, until all
 *		buffers (usually two) are up to date. Also
 *		note that you should do all rendering to the
 *		window either directly *or* in the refresh()
 *		function.
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
 *	void colorkey(Uint32 color);
 *	void colorkey();
 *		Set the colorkey for this window to 'color'
 *		and enable colorkeying. This affects blit()
 *		when this window is used as the source. The
 *		version without arguments disables
 *		colorkeying for this window.
 *		NOTE:	Colorkeying is only supported by
 *			offscreen windows.
 *
 *	void alpha(float a);
 *		Set the full surface alpha of this window.
 *		0.0 is fully transparent and 1.0 is opaque.
 *		NOTE:	Alpha is only supported by offscreen
 *			windows.
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

#include "SDL.h"

class gfxengine_t;

class window_t
{
	friend class gfxengine_t;
  public:
	window_t();
	window_t(gfxengine_t *e);
	virtual ~window_t();

	virtual void init(gfxengine_t *e);
	void place(int left, int top, int sizex, int sizey);
	int offscreen();
	void visible(int vis);
	int visible()	{ return _visible; }
	void select();
	void invalidate(SDL_Rect *r = NULL);
	virtual void refresh(SDL_Rect *r)	{ ; }

	/* Color tools */
	Uint32 mulrgb(Uint32 c1, Uint32 c2)
	{
		int r = ((c1 >> 16) & 0xff) * ((c2 >> 16) & 0xff) * 258 >> 16;
		int g = ((c1 >> 8) & 0xff) * ((c2 >> 8) & 0xff) * 258 >> 16;
		int b = (c1 & 0xff) * (c2 & 0xff) * 258 >> 16;
		return r << 16 | g << 8 | b;
	}
	Uint32 fadergb(Uint32 rgb, Uint8 fade)
	{
		int r = ((rgb >> 16) & 0xff) * fade * 258 >> 16;
		int g = ((rgb >> 8) & 0xff) * fade * 258 >> 16;
		int b = (rgb & 0xff) * fade * 258 >> 16;
		return r << 16 | g << 8 | b;
	}

	/* Rendering */
	Uint32 map_rgb(Uint8 r, Uint8 g, Uint8 b)
	{
		return 0xff000000 | (r << 16) | (g << 8) | b;
	}
	Uint32 map_rgb(Uint32 c)	{ return 0xff000000 | c; }
	Uint32 map_rgba(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
	{
		return (a << 24) | (r << 16) | (g << 8) | b;
	}
	Uint8 get_r(Uint32 c)		{ return c >> 16; }
	Uint8 get_g(Uint32 c)		{ return c >> 8; }
	Uint8 get_b(Uint32 c)		{ return c; }
	Uint8 get_a(Uint32 c)		{ return c >> 24; }
	void foreground(Uint32 color)	{ fgcolor = color; }
	void background(Uint32 color)	{ bgcolor = color; };

	void bgimage(int bank = -1, int frame = -1);
	void colorkey(Uint32 color);
	void colorkey();
	void alpha(float a);

	void clear(SDL_Rect *r = NULL);
	void font(int fnt);
	void string(int _x, int _y, const char *txt);
	void center(int _y, const char *txt);
	void center_token(int _x, int _y, const char *txt, signed char token = 0);
	void string_fxp(int _x, int _y, const char *txt);
	void center_fxp(int _y, const char *txt);
	void center_token_fxp(int _x, int _y, const char *txt, signed char token = 0);
	int textwidth(const char *txt, int min = 0, int max = 255);
	int textwidth_fxp(const char *txt, int min = 0, int max = 255);
	int fontheight();

	void point(int _x, int _y);
	void rectangle(int _x, int _y, int w, int h);
	void fillrect(int _x, int _y, int w, int h);
	void fillrect_fxp(int _x, int _y, int w, int h);

	void sprite(int _x, int _y, int bank, int frame, int inval = 1);
	void sprite_fxp(int _x, int _y, int bank, int frame, int inval = 1);

	void blit(int dx, int dy, int sx, int sy, int sw, int sh, window_t *src);
	void blit(int dx, int dy, window_t *src);

	int x()		{ return (phys_rect.x * 256 + 128) / xs; }
	int y()		{ return (phys_rect.y * 256 + 128) / ys; }
	int x2()	{ return ((phys_rect.x + phys_rect.w) * 256 + 128) / xs; }
	int y2()	{ return ((phys_rect.y + phys_rect.h) * 256 + 128) / ys; }
	int width()	{ return x2() - x(); }
	int height()	{ return y2() - y(); }

	SDL_Rect	phys_rect;

  protected:
	window_t	*next, *prev;

	gfxengine_t	*engine;

	SDL_Renderer	*renderer;	// Can be engine or local renderer!

	SDL_Texture	*otexture;	// Buffer for offscreen windows

	// Fallback for offscreen window, if there's no render target support
	SDL_Surface	*osurface;

	int		xs, ys;		// fixp 24:8
	Uint32		fgcolor, bgcolor;
	int		bg_bank, bg_frame;
	int		_font;
	int		_visible;
	int		_offscreen;

	void _select();			// Internal version
	void link(gfxengine_t *e);
	void unlink(void);

	void phys_refresh(SDL_Rect *r)
	{
		if(!r)
			refresh(NULL);
		else
		{
			SDL_Rect dr;
			dr.x = r->x - phys_rect.x;
			dr.y = r->y - phys_rect.y;
			int x2 = (int)(dr.x + r->w) * 256 / xs;
			int y2 = (int)(dr.y + r->h) * 256 / ys;
			dr.x = (int)dr.x * 256 / xs;
			dr.y = (int)dr.y * 256 / ys;
			dr.w = x2 - dr.x;
			dr.h = y2 - dr.y;
			refresh(&dr);
		}
	}

	void offscreen_invalidate(SDL_Rect *r);
};

#endif
