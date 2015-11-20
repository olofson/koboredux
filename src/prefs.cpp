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

#include "kobolog.h"
#include "prefs.h"
#include "gfxengine.h"
#include "gamectl.h"
#include "game.h"

void prefs_t::init()
{
	comment("--------------------------------------------");
	comment(" Kobo Redux "KOBO_VERSION" Configuration File");
	comment("--------------------------------------------");
	comment(" Switches - [no]<switch>");
	comment(" Values - <key> [<value>|\"<string>\"]");
	comment("--------------------------------------------");
	comment("--- System options --------------------------");
	yesno("logfile", logfile, 0); desc("Log To File");
	key("logformat", logformat, 0); desc("Log File Format");
	key("logverbosity", logverbosity, 2); desc("Log Verbosity Level");

	comment("--- Input options --------------------------");
	yesno("joystick", use_joystick, 0); desc("Use Joystick");
	key("joystick_no", joystick_no, 0); desc("Joystick Number");
	yesno("mouse", use_mouse, 0); desc("Use Mouse");
	key("mousemode", mousemode, MMD_CROSSHAIR); desc("Mouse Control Mode");
	yesno("broken_numdia", broken_numdia, 0); desc("Broken NumPad Diagonals");
	key("dia_emphasis", dia_emphasis, 1); desc("Diagonals Emphasis Filter");
	yesno("always_fire", always_fire, 0); desc("Always Fire");
	yesno("mousecapture", mousecapture, 1); desc("In-game Mouse Capture");

	comment("--- Game options ---------------------------");
	yesno("scrollradar", scrollradar, 1); desc("Scrolling Radar");
	yesno("filter", filter, 1); desc("Motion Interpolation");
	key("timefilter", timefilter, 50); desc("Time Filter");
	key("countdown", countdown, 5); desc("Get Ready Countdown");
	key("stars", stars, 500); desc("Number of Parallax Stars");
	key("cannonloud", cannonloud, 100); desc("Player Cannons Loudness");

	comment("--- Sound settings -------------------------");
	yesno("sound", use_sound, 1); desc("Enable Sound");
	yesno("music", use_music, 1); desc("Enable Music");
	yesno("cached_sounds", cached_sounds, 0); desc("Use Cached Sounds");
	key("samplerate", samplerate, 44100); desc("Sample Rate");
	key("latency", latency, 50); desc("Sound Latency");
	key("vol", volume, 100); desc("Master Volume");
	key("intro_vol", intro_vol, 100); desc("Intro Music Volume");
	key("sfx_vol", sfx_vol, 100); desc("Sound Effects Volume");
	key("music_vol", music_vol, 30); desc("In-Game Music Volume");
	key("reverb", reverb, 100); desc("Reverb Level");
	key("vol_boost", vol_boost, 0); desc("Volume Boost");

	comment("--- Video settings -------------------------");
	yesno("fullscreen", fullscreen, 1); desc("Fullscreen Display");
	key("width", width, 0); desc("Horizontal Resolution");
	key("height", height, 0); desc("Vertical Resolution");
	yesno("autoswap", autoswap, 0); desc("Automatically swap display dimension");
	key("aspect", aspect, 1000); desc("Pixel Aspect Ratio");
	key("maxfps", max_fps, 100); desc("Maximum fps");
	key("maxfpsstrict", max_fps_strict, 0); desc("Strictly Regulated fps");
	key("videomode", videomode, 1); desc("Video Mode");
	yesno("vsync", vsync, 1); desc("Enable Vertical Sync");

	comment("--- Graphics settings ----------------------");
	key("scalemode", scalemode, 0); desc("Scaling Filter Mode");
	yesno("alpha", alpha, 1); desc("Use Alpha Blending");
	key("brightness", brightness, 100); desc("Brightness");
	key("contrast", contrast, 100); desc("Contrast");
	key("planetdither", planetdither, 4); desc("Planet Dither Style");

	comment("--- File paths -----------------------------");
	key("files", dir, ""); desc("Game Root Path");
	key("gfx", gfxdir, ""); desc("Graphics Data Path");
	key("sfx", sfxdir, ""); desc("Sound Data Path");
	key("scores", scoredir, ""); desc("Score File Path");

	comment("--- Temporary variables --------------------");
	key("last_profile", last_profile, 0);
			desc("Last used player profile");
	key("number_of_joysticks", number_of_joysticks, 0);
			desc("Number of Connected Joysticks");

	// "Commands" - never written to config files
	command("showcfg", cmd_showcfg); desc("Show Configuration");
	command("hiscores", cmd_hiscores); desc("List High Scores");
	command("highscores", cmd_hiscores); desc("List High Scores");
	command("override", cmd_override); desc("Ignore Configuration File");
	command("debug", cmd_debug); desc("Enable Debug Features");
	command("fps", cmd_fps); desc("Show Frame Rate");
	command("cheat", cmd_cheat); desc("Enable Cheat Mode");
	command("indicator", cmd_indicator);
			desc("Enable Collision Indicator Mode");
	command("pushmove", cmd_pushmove); desc("Enable Push Move Mode");
	command("autoshot", cmd_autoshot); desc("Ingame screenshots/movie");
	command("help", cmd_help); desc("Print usage info and exit");
	command("options_man", cmd_options_man);
			desc("Print options for 'man'");
}


void prefs_t::postload()
{
}
