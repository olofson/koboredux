/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001-2003, 2007, 2009 David Olofson
 * Copyright 2005 Erik Auerswald
 * Copyright 2008 Robert Schuster
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

#include "kobolog.h"
#include "prefs.h"
#include "gfxengine.h"
#include "gamectl.h"
#include "game.h"

void prefs_t::init()
{
	comment("------------------------------------------------");
	comment(" Kobo Redux " KOBO_VERSION_STRING " Configuration File");
	comment("------------------------------------------------");
	comment(" Switches - [no]<switch> or <switch> [<value>]");
	comment(" Values - <key> [<value>|\"<string>\"]");
	comment("------------------------------------------------");
	key("version", version, 0); desc("Config File Version");

	section("System");
	yesno("logfile", logfile, 0); desc("Log To File");
	key("logformat", logformat, 0); desc("Log File Format");
	yesno("logconsole", logconsole, 1); desc("Log To Console");
	key("conlogformat", conlogformat, 0); desc("Console Log Format");
	key("logverbosity", logverbosity, 2); desc("Log Verbosity Level");
	yesno("quickstart", quickstart, 0); desc("Quick Startup");
	yesno("loopreplays", loopreplays, 0); desc("Loop Campaign Replays");

	section("Video");
	yesno("fullscreen", fullscreen, 1); desc("Fullscreen Display");
	key("width", width, 0); desc("Horizontal Resolution");
	key("height", height, 0); desc("Vertical Resolution");
	yesno("autoswap", autoswap, 0);
			desc("Automatically swap display dimension");
	key("maxfps", maxfps, 100); desc("Maximum fps");
	key("maxfps_strict", maxfps_strict, 0); desc("Strictly Regulated fps");
	key("videomode", videomode, 1); desc("Video Mode");
	yesno("vsync", vsync, 1); desc("Enable Vertical Sync");
	key("filter", filter, 2); desc("Logic-to-Video Motion Filter Mode");
	key("timefilter", timefilter, 50); desc("Time Filter");
	key("mouse_hidetime", mouse_hidetime, 2000);
			desc("Mouse Hide Timeout");

	section("Controls");
	yesno("joystick", joystick, 0); desc("Enable Joystick");
	key("joystick_index", joystick_index, 0); desc("Joystick Number");
	key("js_hat", js_hat, 0); desc("Joystick: Move Hat");
	key("js_horizontal", js_horizontal, 0);
			desc("Joystick: Horizontal Move Axis");
	key("js_vertical", js_vertical, 1);
			desc("Joystick: Vertical Move Axis");
	key("js_deadzone", js_deadzone, 10);
			desc("Joystick: Stick Deadzone");
	key("js_sprint", js_sprint, 50);
			desc("Joystick: Stick Sprint Threshold");
	key("js_primary", js_primary, 0);
			desc("Joystick: Primary Fire Button");
	key("js_secondary", js_secondary, 1);
			desc("Joystick: Secondary Fire Button");
	key("js_tertiary", js_tertiary, 3);
			desc("Joystick: Tertiary Fire Button");
	key("js_pause", js_pause, 2); desc("Joystick: Pause Button");
	yesno("mouse", mouse, 0); desc("Enable Mouse");
	key("mousemode", mousemode, MMD_CROSSHAIR); desc("Mouse Control Mode");
	key("mouse_threshold", mouse_threshold, 32);
			desc("Mouse Push Threshold");
	yesno("broken_numdia", broken_numdia, 0);
			desc("Broken NumPad Diagonals");
	key("dia_emphasis", dia_emphasis, 1);
			desc("Diagonals Emphasis Filter");
	yesno("mousecapture", mousecapture, 1); desc("In-game Mouse Capture");
	yesno("tertiary_button", tertiary_button, 1);
			desc("Use Tertiary Fire Button");

	section("Keyboard bindings");
	key("keyboard_up", keyboard_up, SDL_SCANCODE_UP);
			desc("Keyboard Up");
	key("keyboard_down", keyboard_down, SDL_SCANCODE_DOWN);
			desc("Keyboard Down");
	key("keyboard_left", keyboard_left, SDL_SCANCODE_LEFT);
			desc("Keyboard Left");
	key("keyboard_right", keyboard_right, SDL_SCANCODE_RIGHT);
			desc("Keyboard Right");
	key("keyboard_primary", keyboard_primary, SDL_SCANCODE_C);
			desc("Keyboard Primary Fire");
	key("keyboard_secondary", keyboard_secondary, SDL_SCANCODE_X);
			desc("Keyboard Secondary Fire");
	key("keyboard_tertiary", keyboard_tertiary, SDL_SCANCODE_Z);
			desc("Keyboard Tertiary Fire");

	section("Audio");
	yesno("sound", sound, 1); desc("Enable Sound");
	yesno("music", music, 1); desc("Enable Music");
	key("audiodriver", audiodriver, ""); desc("Audio Driver");
	key("samplerate", samplerate, 48000); desc("Sample Rate");
	key("latency", latency, 20); desc("Sound Latency");
	key("audiobuffer", audiobuffer, 0); desc("Custom Audio Buffer Size");
	yesno("audiots", audiots, 1); desc("Timestamp Audio Events");
	key("tsdelay", tsdelay, 5); desc("Timestamp Delay");

	section("Sound");
	key("sfxtheme", sfxtheme, ""); desc("Sound Theme");
	key("volume", volume, 100); desc("Master Volume");
	key("vol_boost", vol_boost, 2); desc("Volume Boost");
	key("ui_vol", ui_vol, 50); desc("User Interface Volume");
	key("sfx_vol", sfx_vol, 70); desc("Sound Effects Volume");
	key("music_vol", music_vol, 50); desc("Music Volume");
	key("title_vol", title_vol, 70); desc("Title Music Volume");

	section("Interface");
	key("gfxtheme", gfxtheme, ""); desc("Graphics Theme");
	yesno("scrollradar", scrollradar, 1); desc("Scrolling Radar");
	key("stars", stars, 500); desc("Number of Parallax Stars");
	key("cannonloud", cannonloud, 50); desc("Player Cannons Loudness");
	key("shieldloud", shieldloud, 50); desc("Player Shield Loudness");
	key("scalemode", scalemode, 0); desc("Scaling Filter Mode");
	yesno("alpha", alpha, 1); desc("Use Alpha Blending");
	key("brightness", brightness, 100); desc("Brightness");
	key("contrast", contrast, 100); desc("Contrast");
	key("planetdither", planetdither, -1); desc("Planet Dither Style");
	key("firedither", firedither, -1); desc("Fire Effect Dither Style");
	yesno("playerhitfx", playerhitfx, 0);
			desc("Use visual player hit effects");
	key("screenshake", screenshake, 3); desc("Screen Shake");
	yesno("titledemos", titledemos, 1); desc("Play Demos On Title Screen");

	section("Game");
	key("countdown", countdown, 5); desc("Get Ready Countdown");
	key("cont_countdown", cont_countdown, 9); desc("Continue Countdown");
	key("autocontinue", autocontinue, 5);
			desc("Automatic Continue Timeout");

	section("Debug");
	yesno("debug", debug, 0); desc("Enable Debug Features");
	yesno("show_fps", show_fps, 0); desc("Show Frame Rate");
	yesno("show_map_border", show_map_border, 0); desc("Show Map Border");
	yesno("show_coordinates", show_coordinates, 0);
			desc("Show Object Coordinates");
	yesno("show_tiles", show_tiles, 0); desc("Show Tile/Sprite Edges");
	yesno("show_hit", show_hit, 0); desc("Show Hit Zones");
	yesno("tsdebug", tsdebug, 0); desc("Timestamp Debug Output");
	yesno("soundtools", soundtools, 0); desc("Sound Design Tools");
	yesno("st_missing", st_missing, 0); desc("Log Missing Sound Effects");
	yesno("st_played", st_played, 0); desc("Log Played Sound Effects");
	yesno("replaydebug", replaydebug, 0); desc("Replay Debug Data");
	key("toolstheme", toolstheme, ""); desc("Authoring Tools Theme");
	yesno("force_fallback_gfxtheme", force_fallback_gfxtheme, 0);
			desc("Force Load Fallback Graphics Theme");
	yesno("force_fallback_sfxtheme", force_fallback_sfxtheme, 0);
			desc("Force Load Fallback Sound Theme");
	yesno("firebench", firebench, 0); desc("Fire FX Benchmarking");

	section("Cheat/test");
	yesno("cheat_pushmove", cheat_pushmove, 0);
			desc("Cheat: Enable Push Move Mode");
	yesno("cheat_freewheel", cheat_freewheel, 0);
			desc("Cheat: Free Physics When Releasing Controls");
	yesno("cheat_shield", cheat_shield, 0);
			desc("Cheat: Shield Always Up");
	yesno("cheat_invulnerability", cheat_invulnerability, 0);
			desc("Cheat: Player Invulnerable");
	yesno("cheat_ceasefire", cheat_ceasefire, 0);
			desc("Cheat: Enemies Will Not Fire");
	yesno("cheat_firepower", cheat_firepower, 0);
			desc("Cheat: Extra Firepower");
	yesno("cheat_startlevel", cheat_startlevel, 0);
			desc("Cheat: Unlimited Start Level Selector");
	yesno("cheat_brokentrigger", cheat_brokentrigger, 0);
			desc("Cheat: Bombs Trigger But Can't Detonate");
	key("cheat_speed", cheat_speed, 0.0f); desc("Cheat: Game Speed");

	section("Paths");
	key("data", data, ""); desc("Alternate Data Path");

	section("Temporary variables");
	key("number_of_joysticks", number_of_joysticks, 0);
			desc("Number of Connected Joysticks");

	section("Commands");
	comment(" (Never written to config files!)");
	command("showcfg", cmd_showcfg); desc("Show Configuration");
	command("override", cmd_override); desc("Ignore Configuration File");
	command("savecfg", cmd_savecfg, 1); desc("(Don't) Save Configuration");
	command("help", cmd_help); desc("Print usage info and exit");
	command("options_man", cmd_options_man);
			desc("Print options for 'man'");
	key("autoshot", cmd_autoshot, 0, false);
			desc("Ingame screenshots/movie");
	key("warp", cmd_warp, 0, false); desc("Warp To Stage");
	key("skill", cmd_skill, SKILL_NORMAL, false); desc("Warp Skill Level");
	command("resaveall", cmd_resaveall, 0);
			desc("Resave Config and Saves");
}


void prefs_t::postload()
{
	if(cheat_speed == 1.0f)
		cheat_speed = 0.0f;

#ifdef KOBO_DEMO
	// This is a bit ugly... We silently ignore any of these switches by
	// just resetting them after loading the cfg and parsing options.

	gfxtheme[0] = 0;
	sfxtheme[0] = 0;
	toolstheme[0] = 0;

	force_fallback_gfxtheme = 0;
	force_fallback_sfxtheme = 0;
	show_map_border = 0;
	show_coordinates = 0;
	show_tiles = 0;
	show_hit = 0;
	soundtools = 0;

	cheat_pushmove = 0;
	cheat_freewheel = 0;
	cheat_shield = 0;
	cheat_invulnerability = 0;
	cheat_ceasefire = 0;
	cheat_firepower = 0;
	cheat_startlevel = 0;
	cheat_brokentrigger = 0;
	cheat_speed = 0.0f;

	cmd_warp = 0;
#endif
}


void prefs_t::presave()
{
	version = KOBO_VERSION;
}
