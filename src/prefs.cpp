/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 2001-2003, 2007, 2009 David Olofson
 * Copyright (C) 2005 Erik Auerswald
 * Copyright (C) 2008 Robert Schuster
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
#include "a_types.h"
#include "game.h"

void prefs_t::init()
{
	comment("--------------------------------------------");
	comment(" Kobo Deluxe "VERSION" Configuration File");
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
	key("dia_emphasis", dia_emphasis, 0); desc("Diagonals Emphasis Filter");
	yesno("always_fire", always_fire, 0); desc("Always Fire");
	yesno("mousecapture", mousecapture, 1); desc("In-game Mouse Capture");

	comment("--- Game options ---------------------------");
	key("scrollradar", scrollradar, 2); desc("Radar Scroll Mode");
	yesno("filter", filter, 1); desc("Motion Interpolation");
	key("timefilter", timefilter, 50); desc("Time Filter");
	key("countdown", countdown, 5); desc("Get Ready Countdown");
	key("starfield", starfield, 2); desc("Starfield Style");
	key("stars", stars, 500); desc("Number of Parallax Stars");
	key("overheatloud", overheatloud, 0); desc("Overheat Warning Loudness");
	key("cannonloud", cannonloud, 100); desc("Player Cannons Loudness");

	comment("--- Sound settings -------------------------");
	yesno("sound", use_sound, 1); desc("Enable Sound");
	yesno("music", use_music, 1); desc("Enable Music");
	yesno("cached_sounds", cached_sounds, 0); desc("Use Cached Sounds");
#ifdef HAVE_OSS
	yesno("oss", use_oss, 0);
#else
	yesno("oss", use_oss, 0, 0);	//Don't write!
#endif
			 desc("Use OSS Sound Driver");
	key("samplerate", samplerate, 44100); desc("Sample Rate");
	key("latency", latency, 50); desc("Sound Latency");
	key("mixquality", mixquality, AQ_HIGH); desc("Mixing Quality");
	key("vol", volume, 100); desc("Master Volume");
	key("intro_vol", intro_vol, 100); desc("Intro Music Volume");
	key("sfx_vol", sfx_vol, 100); desc("Sound Effects Volume");
	key("music_vol", music_vol, 30); desc("In-Game Music Volume");
	key("reverb", reverb, 100); desc("Reverb Level");
	key("vol_boost", vol_boost, 0); desc("Volume Boost");

	comment("--- Video settings -------------------------");
	yesno("fullscreen", fullscreen, 0); desc("Fullscreen Display");
	key("videodriver", videodriver, GFX_DRIVER_SDL2D);
			desc("Display Driver");
	key("width", width, 640); desc("Horizontal Resolution");
	key("height", height, 480); desc("Vertical Resolution");
	yesno("autoswap", autoswap, 0); desc("Automatically swap display dimension");
	key("aspect", aspect, 1000); desc("Pixel Aspect Ratio");
	key("depth", depth, 0); desc("Display Depth");
	key("maxfps", max_fps, 100); desc("Maximum fps");
	key("maxfpsstrict", max_fps_strict, 0); desc("Strictly Regulated fps");
	key("buffer", doublebuf, 1); desc("Display Buffer Mode");
	yesno("shadow", shadow, 1); desc("Use Software Shadow Buffer");
	key("videomode", videomode, 0x04330); desc("Video Mode");
	yesno("vsync", vsync, 1); desc("Enable Vertical Sync");
	key("videopages", pages, -1); desc("Number of Video Pages");

	comment("--- Graphics settings ----------------------");
	key("scalemode", scalemode, 1); desc("Scaling Filter Mode");
	yesno("dither", use_dither, 1); desc("Use Dithering");
	key("dither_type", dither_type, 0); desc("Dither Type");
	yesno("broken_rgba8", broken_rgba8, 0); desc("Broken RGBA (OpenGL)");
	yesno("alpha", alpha, 1); desc("Use Alpha Blending");
	key("brightness", brightness, 100); desc("Brightness");
	key("contrast", contrast, 100); desc("Contrast");

	comment("--- File paths -----------------------------");
	key("files", dir, ""); desc("Game Root Path");
	key("gfx", gfxdir, ""); desc("Graphics Data Path");
	key("sfx", sfxdir, ""); desc("Sound Data Path");
	key("scores", scoredir, ""); desc("Score File Path");

	// Obsolete stuff (not written into new files)
	key("size", o_size, 0, 0); desc("Screen Size (Obsolete)");
	key("wait", o_wait_msec, 30); desc("Game Speed (Obsolete)");
	key("bgm", o_bgm_indexfile, "", 0);
			desc("Background Music File (Obsolete)");
	key("threshold", o_threshold, 200, 0); desc("Limiter Gain");
	key("release", o_release, 50, 0); desc("Limiter Speed");
	key("internalres", o_internalres, 1, 0); desc("Texture Resolution");

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
	command("noframe", cmd_noframe); desc("Remove Window Frame");
	command("midi", cmd_midi); desc("Enable MIDI Input");
	command("cheat", cmd_cheat); desc("Enable Cheat Mode");
	command("indicator", cmd_indicator);
			desc("Enable Collision Indicator Mode");
	command("pushmove", cmd_pushmove); desc("Enable Push Move Mode");
	command("noparachute", cmd_noparachute); desc("Disable SDL Parachute");
	command("pollaudio", cmd_pollaudio); desc("Use Polling Audio Output");
	command("autoshot", cmd_autoshot); desc("Ingame screenshots/movie");
	command("help", cmd_help); desc("Print usage info and exit");
	command("options_man", cmd_options_man);
			desc("Print options for 'man'");
}


void prefs_t::postload()
{
	if(redefined(o_size))
	{
		//For pre 20011007 compatibility
		width = 320 * o_size;
		height = 240 * o_size;
		o_size = 0;
		accept(o_size);
	}

	if((o_wait_msec != 30) && !cmd_cheat)
	{
		log_printf(ELOG, "'wait' is only avaliable in cheat mode!\n");
		o_wait_msec = 30;
	}

	if(scrollradar == 1)
	{
		log_printf(ELOG, "Radar sweep mode broken and disabled!\n");
		scrollradar = 2;
	}

	// Some unimplemented enums were removed in 0.4.1
	if(videodriver != GFX_DRIVER_SDL2D)
		videodriver = GFX_DRIVER_GLSDL;
}
