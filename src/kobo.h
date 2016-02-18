/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2001-2003, 2005, 2007, 2009 David Olofson
 * Copyright 2008 Robert Schuster
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

#ifndef _KOBO_H_
#define _KOBO_H_

#include "config.h"

#include "gfxengine.h"
#include "window.h"
#include "filemap.h"
#include "prefs.h"
#include "radar.h"
#include "dashboard.h"
#include "sound.h"
#include "spinplanet.h"
#include "graphics.h"


  /////////////////////////////////////////////////////////////////////////////
 //	Singletons
/////////////////////////////////////////////////////////////////////////////
extern KOBO_sound	sound;


  /////////////////////////////////////////////////////////////////////////////
 //	Globals
/////////////////////////////////////////////////////////////////////////////

class kobo_gfxengine_t : public gfxengine_t
{
#ifdef ENABLE_TOUCHSCREEN
	bool pointer_margin_used;
	int pointer_margin_width_min;
	int pointer_margin_width_max;
	int pointer_margin_height_min;
	int pointer_margin_height_max;
#endif
	void pre_loop();
	void pre_advance(float fractional_frame);
	void frame();
	void pre_render();
	void post_render();
	void post_loop();
	void safe_video_mode();
	void fullscreen_toggle();
	float timestamp_delay();
  public:
#ifdef ENABLE_TOUCHSCREEN
	void setup_pointer_margin(int, int);
#endif
};


class backdrop_t : public window_t
{
  protected:
	bool	_reverse;
	int	_image;
  public:
	backdrop_t(gfxengine_t *e);
	void reverse(bool rv)		{ _reverse = rv; }
	void image(int bank)		{ _image = bank; }
	void refresh(SDL_Rect *r);
};


extern kobo_gfxengine_t		*gengine;
extern filemapper_t		*fmap;
extern prefs_t			*prefs;

extern screen_window_t		*wscreen;
extern dashboard_window_t	*wdash;
extern shieldbar_t		*wshield;
extern KOBO_radar_map		*wmap;
extern KOBO_radar_window	*wradar;

extern backdrop_t		*wbackdrop;
extern spinplanet_t		*wplanet;
extern engine_window_t		*wmain;
extern window_t			*woverlay;

extern display_t		*dhigh;
extern display_t		*dscore;
extern display_t		*dstage;
extern display_t		*dregion;
extern display_t		*dlevel;
extern hledbar_t		*pxtop;
extern hledbar_t		*pxbottom;
extern vledbar_t		*pxleft;
extern vledbar_t		*pxright;

extern int mouse_x, mouse_y;
extern int mouse_left, mouse_middle, mouse_right;

extern int exit_game;


  /////////////////////////////////////////////////////////////////////////////
 //	Constants
/////////////////////////////////////////////////////////////////////////////

// Sprite priority levels
#define	LAYER_OVERLAY	0	// Mouse crosshair
#define	LAYER_BULLETS	1	// Bullets - most important!
#define	LAYER_FX	2	// Explosions and similar effects
#define	LAYER_PLAYER	3	// Player and fire bolts
#define	LAYER_ENEMIES	4	// Enemies
#define	LAYER_BASES	5	// Bases and stationary enemies

// NOTE: Needs to be a layer with 1:1 scroll ratio!
#define	LAYER_PLANET	LAYER_BASES	// Spinning planet control

#define	NOALPHA_THRESHOLD	64

#define	INTRO_SCENE	-100000

typedef enum
{
	STARFIELD_NONE = 0,
	STARFIELD_OLD,
	STARFIELD_PARALLAX
} KOBO_StarfieldModes;

#endif // _KOBO_H_
