/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001-2003, 2007, 2009 David Olofson
 * Copyright 2005 Erik Auerswald
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

#ifndef	_KOBO_PREFS_H_
#define	_KOBO_PREFS_H_

#include "config.h"
#include "cfgparse.h"

class prefs_t : public config_parser_t
{
  public:
	// System options
	int	logfile;	//Log messages to log.txt/html
	int	logformat;	//0: text, 1: ANSI, 2: HTML
	int	logverbosity;
	int	quickstart;	//Skip jingle, loader noise effects etc

	// Input options
	int	joystick;
	int	joystick_index;		//which joystick to use
	int	mouse;
	int	mousemode;
	int	fire_tap_time;
	int	broken_numdia;
	int	dia_emphasis;
	int	mousecapture;

	// Game options
	int	filter;		//Use motion filtering
	int	timefilter;	//Delta time filter
	int	scrollradar;	//Scrolling radar
	int	countdown;	//"Get Ready" countdown
	int	stars;		//Number of parallax stars
	int	cannonloud;	//Cannon loudness

	// Sound: System
	cfg_string_t	audiodriver;	//Name/options for a2_NewDriver()
	int	sound;		//Enable sound
	int	music;		//Enable music
	int	samplerate;
	int	latency;	//Audio latency in ms
	int	audiobuffer;	//Custom audio buffer size (sample frames)
	int	tsdelay;	//Timestamp delay in ms

	// Sound: Mixer
	cfg_string_t	sfxtheme;	//Path to sound theme file
	int	volume;		//Digital master volume
	int	vol_boost;	//Master volume boost
	int	ui_vol;		//User interface volume
	int	sfx_vol;	//Sound effects volume
	int	music_vol;	//Music volume
	int	title_vol;	//Title music volume

	// Video settings
	int	fullscreen;	//Use fullscreen mode
	int	width;		//Screen/window width
	int	height;		//Screen/window height
	int	autoswap;	//Automatically swap dimensions if init fails
	int	maxfps;		//Maximum fps
	int	maxfps_strict;	//Strictly regulated fps limiter
	int	doublebuf;	//Use double buffering
	int	videomode;	//New video mode codes
	int	vsync;		//Vertical (retrace) sync

	// Graphics settings
	cfg_string_t	gfxtheme;	//Path to graphics theme file
	int	scalemode;	//Scaling filter mode
	int	alpha;		//Alpha blending
	int	brightness;	//Graphics brightness
	int	contrast;	//Graphics contrast
	int	planetdither;	//Spinning planet dither style

	// Debug features
	int	debug;
	int	show_fps;
	int	force_fallback_gfxtheme; // Force load fallback graphics theme
	int	force_fallback_sfxtheme; // Force load fallback sound theme
	int	show_map_border;
	int	show_coordinates;
	int	show_tiles;
	int	show_hit;
	int	tsdebug;	//Timestamp debugging
	int	soundtools;
	cfg_string_t	toolstheme;

	// Cheat/test features
	int	cheat_pushmove;
	int	cheat_freewheel;
	int	cheat_shield;
	int	cheat_invulnerability;
	int	cheat_firepower;
	int	cheat_startlevel;
	int	cheat_brokentrigger;
	float	cheat_speed;

	// Returns 'true' if any cheat/test features are enabled
	bool cheats()
	{
		return cheat_pushmove ||
				cheat_freewheel ||
				cheat_shield ||
				cheat_invulnerability ||
				cheat_firepower ||
				cheat_startlevel ||
				cheat_brokentrigger ||
				cheat_speed ||
				cmd_warp;
	}

	// File paths
	cfg_string_t	dir;		//Path to root of game directory
	cfg_string_t	gfxdir;		//Path to gfx/
	cfg_string_t	sfxdir;		//Path to sfx/
	cfg_string_t	scoredir;	//Path to scores/

	// "Hidden" stuff ("to remember until next startup")
	int	last_profile;		//Last used player profile
	int	number_of_joysticks;	//no of connected joysticks

	void init();
	void postload();

	// "Commands" - never written to config files
	int	cmd_showcfg;
	int	cmd_hiscores;
	int	cmd_override;
	int	cmd_autoshot;	//Take ingame screenshots
	int	cmd_help;	//Show help and exit
	int	cmd_options_man;//Output OPTIONS doc in Un*x man source format
	int	cmd_warp;
};

#endif	//_KOBO_PREFS_H_
