/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001-2003, 2007, 2009 David Olofson
 * Copyright 2005 Erik Auerswald
 * Copyright 2008 Robert Schuster
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

#ifndef	_KOBO_PREFS_H_
#define	_KOBO_PREFS_H_

#include "config.h"
#include "cfgparse.h"

class prefs_t : public config_parser_t
{
  public:
	//System options
	int	logfile;	//Log messages to log.txt/html
	int	logformat;	//0: text, 1: ANSI, 2: HTML
	int	logverbosity;

	//Input options
	int	use_joystick;
	int	joystick_no;		//which joystick to use
	int	use_mouse;
	int	mousemode;
	int	broken_numdia;
	int	dia_emphasis;
	int	always_fire;
	int	mousecapture;

	//Game options
	int	filter;		//Use motion filtering
	int	timefilter;	//Delta time filter
	int	scrollradar;	//Scrolling radar
	int	countdown;	//"Get Ready" countdown
	int	stars;		//Number of parallax stars
	int	cannonloud;	//Cannon loudness

	//Sound: System
	int	use_sound;	//Enable sound
	int	use_music;	//Enable music
	int	cached_sounds;	//Use prerendered waveforms
	int	samplerate;
	int	latency;	//Audio latency in ms
	int	volume;		//Digital master volume
	//Sound: Mixer
	int	intro_vol;	//Intro music volume
	int	sfx_vol;	//Sound effects volume
	int	music_vol;	//In-game music volume
	//Sound: Master Effects
	int	reverb;		//Master reverb level
	int	vol_boost;	//Master volume boost

	//Video settings
	int	fullscreen;	//Use fullscreen mode
	int	width;		//Screen/window width
	int	height;		//Screen/window height
	int	autoswap;	//Automatically swap dimensions if init fails
	int	aspect;		//Pixel aspect ratio * 1000
	int	max_fps;	//Maximum fps
	int	max_fps_strict;	//Strictly regulated fps limiter
	int	doublebuf;	//Use double buffering
	int	videomode;	//New video mode codes
	int	vsync;		//Vertical (retrace) sync

	//Graphics settings
	int	scalemode;	//Scaling filter mode
	int	alpha;		//Alpha blending
	int	brightness;	//Graphics brightness
	int	contrast;	//Graphics contrast

	//File paths
	cfg_string_t	dir;		//Path to kobo-deluxe/
	cfg_string_t	gfxdir;		//Path to gfx/
	cfg_string_t	sfxdir;		//Path to sfx/
	cfg_string_t	scoredir;	//Path to scores/

	//"Hidden" stuff ("to remember until next startup")
	int	last_profile;		//Last used player profile
	int	number_of_joysticks;	//no of connected joysticks

	void init();
	void postload();

	/* "Commands" - never written to config files */
	int cmd_showcfg;
	int cmd_hiscores;
	int cmd_override;
	int cmd_debug;
	int cmd_fps;
	int cmd_cheat;		//Unlimited lives; select any starting stage
	int cmd_indicator;	//Enable collision testing mode
	int cmd_pushmove;	//Stop when not holding any direction down
	int cmd_autoshot;	//Take ingame screenshots
	int cmd_help;		//Show help and exit
	int cmd_options_man;	//Output OPTIONS doc in Un*x man source format
};

#endif	//_KOBO_PREFS_H_
