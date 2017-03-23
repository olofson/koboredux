/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001-2003, 2006-2007, 2009 David Olofson
 * Copyright 2005 Erik Auerswald
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


#include "config.h"
#include "options.h"
#include "gfxengine.h"
#include "gamectl.h"
#include "kobo.h"


void system_options_t::build()
{
	title("System Options");

	xoffs = 0.55;
	yesno("Quick Startup", &prf->quickstart, 0);

	space();
	yesno("Log Messages to File", &prf->logfile, OS_RESTART_LOGGER);
	yesno("Log Messages to Console", &prf->logconsole, OS_RESTART_LOGGER);
	list("Log File Output Format", &prf->logformat, OS_RESTART_LOGGER);
		item("Plain Text", 0);
		item("ANSI Coloring", 1);
		item("HTML", 2);
	list("Console Log Output Format", &prf->conlogformat,
			OS_RESTART_LOGGER);
		item("Plain Text", 0);
		item("ANSI Coloring", 1);
	list("Log Verbosity", &prf->logverbosity, OS_RESTART_LOGGER);
		item("Critical Errors Only", 0);
		item("Errors Only", 1);
		item("Errors & Warnings", 2);
		item("Some Debug", 3);
		item("More Debug", 4);
		item("Everything", 5);

	space();
	yesno("Loop Campaign Replays", &prf->loopreplays, 0);

	xoffs = 0.5;
	space(2);
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL);
	space(2);
	help();
}


void video_options_t::build()
{
	title("Video Options");

	vmm_Init();
	if(firstbuild)
	{
		VMM_Mode *vm = vmm_GetMode(prf->videomode);
		if(vm)
		{
			// Set up filter based on current mode
			if(vm->flags & VMM__RECOMMENDED)
				showmodes = VMM__RECOMMENDED;
			else if(vm->flags & VMM__COMMON)
				showmodes = VMM__COMMON;
			else if(vm->flags & VMM__WIDESCREEN)
				showmodes = VMM__WIDESCREEN;
			else if(vm->flags & VMM__NONWIDESCREEN)
				showmodes = VMM__NONWIDESCREEN;
			else
				showmodes = VMM_ALL;
			if(vm->flags & VMM_LORES)
				showlow = 1;
		}
		else
		{
			// Show recommended modes by default!
			showmodes = VMM_DESKTOP | VMM_16_9;
			showlow = 0;
			prf->videomode = -1;
		}
		firstbuild = 0;
	}

	xoffs = 0.55;
	list("Show Modes" , &showmodes, OS_REBUILD);
		item("Show All", (int)VMM_ALL);
		item("Recommended Modes", (int)VMM__RECOMMENDED);
		item("Common Modes", (int)VMM__COMMON);
		item("Widescreen Modes", (int)VMM__WIDESCREEN);
		item("Non Widescreen Modes", (int)VMM__NONWIDESCREEN);
	yesno("Show Odd Low Resolutions" , &showlow, OS_REBUILD);
	list("Display Mode", &prf->videomode, OS_RESTART_VIDEO | OS_REBUILD);
	{
		char buf[256];
		buf[sizeof(buf) - 1] = 0;
		for(VMM_Mode *vm = NULL; ; )
		{
			if(!(vm = vmm_FindNext(vm,
					showmodes,
					showlow ? 0 : VMM_LORES,
					0, 0)))
				break;

			// Skipping this for now, as it doesn't seem to work
			// properly.
			if(vm->id == VMID_FULLWINDOW)
				continue;

			if(vm->flags & VMM_DESKTOP)
				snprintf(buf, sizeof(buf) - 1, "%s (%dx%d)",
						vm->name,
						vm->width, vm->height);
			else if(vm->name)
				snprintf(buf, sizeof(buf) - 1, "%dx%d (%s)",
						vm->width, vm->height,
						vm->name);
			else
				snprintf(buf, sizeof(buf) - 1, "%dx%d",
						vm->width, vm->height);
			item(buf, vm->id);
		}
		item("Custom", -1);
	}
	if(prf->videomode < 0)
	{
		if(prf->width <= 32)
			prf->width = SCREEN_WIDTH * 2;
		if(prf->height <= 32)
			prf->height = SCREEN_HEIGHT * 2;
		spin("Width", &prf->width, 32, 4096, "pixels",
				OS_RESTART_VIDEO | OS_REBUILD);
		spin("Height", &prf->height, 32, 4096, "pixels",
				OS_RESTART_VIDEO | OS_REBUILD);
	}
	if((prf->videomode != VMID_DESKTOP) &&
			(prf->videomode != VMID_FULLWINDOW))
	{
		space();
		yesno("Fullscreen Display", &prf->fullscreen,
				OS_RESTART_VIDEO);
	}
	space();
	yesno("Vertical Retrace Sync", &prf->vsync, OS_RESTART_VIDEO);
	space();
	list("Maximum Frame Rate", &prf->maxfps, OS_REBUILD);
	{
		char buf[10];
		item("OFF", 0);
		for(int i = 1; i < 25; i += 1)
		{
			snprintf((char *)&buf, sizeof(buf),
					"%d Hz", i);
			item(buf, i);
		}
		for(int i = 25; i < 100; i += 5)
		{
			snprintf((char *)&buf, sizeof(buf),
					"%d Hz", i);
			item(buf, i);
		}
		for(int i = 100; i <= 200; i += 10)
		{
			snprintf((char *)&buf, sizeof(buf),
					"%d Hz", i);
			item(buf, i);
		}
	}
	if(prf->maxfps)
		yesno("Strictly Regulated", &prf->maxfps_strict, 0);

	space();
	list("Time Filter", &prf->timefilter, OS_UPDATE_ENGINE);
		item("Off", 100);
		item("Fast", 75);
		item("Normal", 50);
		item("Slow", 10);
		item("Logic/Video Lock", 0);
	list("Motion Filter", &prf->filter, OS_UPDATE_ENGINE);
		item("None", 0);
		item("Interpolation", 1);
		item("Extrapolation", 2);

	xoffs = 0.5;
	space(2);
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL);
	space(2);
	help();
}

void video_options_t::close()
{
	vmm_Close();
	config_form_t::close();
}


void controls_options_t::build()
{
	title("Controls");

	xoffs = 0.57;
	yesno("Use Mouse", &prf->mouse, OS_RESTART_INPUT | OS_REBUILD);
	if(prf->mouse)
	{
		list("Mouse Control Mode", &prf->mousemode, OS_RESTART_INPUT);
			item("Disabled", MMD_OFF);
			item("Crosshair", MMD_CROSSHAIR);
	}
	space();
	yesno("In-game Mouse Capture", &prf->mousecapture, 0);
	space();
	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
		label("Could not initialize joysticks!");
	else
	{
		prf->number_of_joysticks = SDL_NumJoysticks();
		yesno("Use Joystick", &prf->joystick,
				OS_RESTART_INPUT | OS_REBUILD);
		if(prf->joystick)
		{
			if(prf->number_of_joysticks)
			{
				list("Joystick Number", &prf->joystick_index,
						OS_RESTART_INPUT);
					enum_list(0, prf->number_of_joysticks
							- 1);
			}
			else
				label("No Joysticks Found!");
		}
	}
	space();
	yesno("Broken NumPad Diagonals", &prf->broken_numdia, 0);
	list("Diagonals Emphasis Filter", &prf->dia_emphasis, 0);
		item("OFF", 0);
		item("1 frame", 1);
		item("2 frames", 2);
		item("3 frames", 3);
		item("4 frames", 4);
		item("5 frames", 5);

	xoffs = 0.5;
	space(2);
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL);
	space(2);
	help();
}


void audio_options_t::build()
{
	title("Sound Options");

	xoffs = 0.55;
	yesno("Enable Sound", &prf->sound, OS_RESTART_AUDIO | OS_REBUILD);
	if(prf->sound)
	{
		yesno("Enable Music", &prf->music,
				OS_UPDATE_AUDIO | OS_REBUILD);

		//System

		space();
		driver = -1;
		memset(drivers, 0, sizeof(drivers));
		const char *comma;
		int len;
		if((comma = strchr(prf->audiodriver, ',')))
			len = comma - prf->audiodriver;
		else
			len = strlen(prf->audiodriver);
		list("Driver", &driver, OS_RESTART_AUDIO);
		A2_regdriver *rd = NULL;
		for(int i = 0; i < MAX_AUDIO_DRIVERS; ++i)
		{
			rd = a2_FindDriver(A2_AUDIODRIVER, rd);
			if(!rd)
				break;
			drivers[i] = a2_DriverName(rd);
			if(strncmp(drivers[i], prf->audiodriver, len) == 0)
				driver = i;
			item(drivers[i], i);
		}

		list("Sample Rate", &prf->samplerate, OS_RESTART_AUDIO);
			item("8 kHz", 8000);
			item("16 kHz", 16000);
			item("22.05 kHz", 22050);
			item("32 kHz", 32000);
			item("44.1 kHz", 44100);
			item("48 kHz", 48000);
			item("96 kHz", 96000);
			item("88.2 kHz", 88200);
			item("192 kHz", 192000);
		list("Sound Latency", &prf->latency, OS_RESTART_AUDIO);
		{
			char buf[10];
			for(int i = 1; i < 20; i += 1)
			{
				snprintf((char *)&buf, sizeof(buf),
						"%d ms", i);
				item(buf, i);
			}
			for(int i = 20; i < 50; i += 5)
			{
				snprintf((char *)&buf, sizeof(buf),
						"%d ms", i);
				item(buf, i);
			}
			for(int i = 50; i < 100; i += 10)
			{
				snprintf((char *)&buf, sizeof(buf),
						"%d ms", i);
				item(buf, i);
			}
			for(int i = 100; i <= 500; i += 25)
			{
				snprintf((char *)&buf, sizeof(buf),
						"%d ms", i);
				item(buf, i);
			}
		}
		yesno("Use Timestamps", &prf->audiots, OS_RESTART_AUDIO);

		space();

		//Master section
		list("Master Volume", &prf->volume, OS_UPDATE_AUDIO);
			item("OFF", 0);
			perc_list(10, 90, 10);
			perc_list(100, 200, 25);
		list("Volume Boost", &prf->vol_boost, OS_UPDATE_AUDIO);
			item("OFF", 0);
			item("Low", 1);
			item("Moderate", 2);
			item("High", 3);
			item("Extreme", 4);

		space();

		//Mixer
		list("Sound Effects Volume", &prf->sfx_vol, OS_UPDATE_AUDIO);
			item("OFF", 0);
			perc_list(10, 90, 10);
			perc_list(100, 200, 25);
		list("User Interface Volume", &prf->ui_vol, OS_UPDATE_AUDIO);
			item("OFF", 0);
			perc_list(10, 90, 10);
			perc_list(100, 200, 25);
		if(prf->music)
		{
			list("Title Music Volume", &prf->title_vol,
					OS_UPDATE_AUDIO);
				item("OFF", 0);
				perc_list(10, 90, 10);
				perc_list(100, 200, 25);
			list("In-Game Music Volume", &prf->music_vol,
					OS_UPDATE_AUDIO);
				item("OFF", 0);
				perc_list(10, 90, 10);
				perc_list(100, 200, 25);
		}
	}

	xoffs = 0.5;
	space(2);
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL | OS_UPDATE_AUDIO);
	space(2);
	help();
}

void audio_options_t::prepare_to_apply()
{
	if(driver >= 0)
		strcpy(prf->audiodriver, drivers[driver]);
}


void interface_options_t::add_theme_items(const char *type)
{
	KOBO_ThemeData *td = NULL;
	item("Default", "");
	while((td = km.get_next_theme(type, td)))
	{
		const char *lab = td->get_string(KOBO_D_THEMELABEL);
		if(!lab)
			continue;	// Can't use one without a valid label!
		item(td->get_string(KOBO_D_THEMENAME, lab), lab);
	}
}


void interface_options_t::build()
{
	title("Interface");

	xoffs = 0.55;
#ifdef KOBO_DEMO
	label("Graphics Theme: Kobo Redux Demo");
	label("Sound Theme: Kobo Redux Demo");
#else
	list("Graphics Theme", &prf->gfxtheme, OS_RESTART_VIDEO);
	add_theme_items("gamegfx");
	list("Sound Theme", &prf->sfxtheme, OS_RELOAD_SOUNDS);
	add_theme_items("gamesfx");
#endif
	space();
	yesno("Scrolling Radar", &prf->scrollradar, 0);
	space();
	yesno("Visual Player Hit FX", &prf->playerhitfx, 0);
	list("Screen Shake", &prf->screenshake, 0);
		item("OFF", 0);
		item("Subtle", 1);
		item("Low", 2);
		item("Moderate", 3);
		item("Extreme", 4);
		item("Ridiculous", 5);
	space();
	list("Starfield Density", &prf->stars, 0);
		item("Minimal", 50);
		item("Low", 250);
		item("Normal", 500);
		item("High", 1000);
		item("Massive", 2500);
		item("Insane", 8000);
	list("Planet Dither Style", &prf->planetdither, OS_UPDATE_SCREEN);
		item("Theme Default", -1);
		item("None", SPINPLANET_DITHER_NONE);
		item("Raw", SPINPLANET_DITHER_RAW);
		item("Random", SPINPLANET_DITHER_RANDOM);
		item("Ordered", SPINPLANET_DITHER_ORDERED);
		item("Skewed", SPINPLANET_DITHER_SKEWED);
		item("Temporal, Noise", SPINPLANET_DITHER_NOISE);
		item("Temporal, 2 Frames", SPINPLANET_DITHER_TEMPORAL2);
		item("Temporal, 4 Frames", SPINPLANET_DITHER_TEMPORAL4);
		item("TrueColor", SPINPLANET_DITHER_TRUECOLOR);
#if 0
	space(1);
	list("Scale Mode", &prf->scalemode, OS_RELOAD_GRAPHICS);
		item("Nearest", GFX_SCALE_NEAREST);
		item("Bilinear", GFX_SCALE_BILINEAR);
		item("Scale2x", GFX_SCALE_SCALE2X);
		item("Diamond2x", GFX_SCALE_DIAMOND);
	yesno("Alpha Blending", &prf->alpha, OS_RELOAD_GRAPHICS);
	list("Brightness", &prf->brightness, OS_RELOAD_GRAPHICS);
		perc_list(50, 150, 10);
	list("Contrast", &prf->contrast, OS_RELOAD_GRAPHICS);
		perc_list(50, 150, 10);
#endif
	space();
	list("Cannon Sound Suppression", &prf->cannonloud, OS_UPDATE_AUDIO);
		item("Off", 100);
		item("Low", 50);
		item("High", 25);
		item("Silent", 0);

	xoffs = 0.5;
	space(2);
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL | OS_UPDATE_ENGINE);
	space(2);
	help();
}


void game_options_t::build()
{
	title("Game Options");

	xoffs = 0.55;
	list("\"Get Ready!\" Countdown", &prf->countdown, 0);
		item("Quick Start", 0);
		item("1 second", 1);
		item("2 seconds", 2);
		item("3 seconds", 3);
		item("4 seconds", 4);
		item("5 seconds", 5);
		item("6 seconds", 6);
		item("7 seconds", 7);
		item("8 seconds", 8);
		item("9 seconds", 9);
		item("Wait Forever", 10);
	list("\"Continue?\" Countdown", &prf->cont_countdown, 0);
		item("Quick Continue", 0);
		item("1 second", 1);
		item("2 seconds", 2);
		item("3 seconds", 3);
		item("4 seconds", 4);
		item("5 seconds", 5);
		item("6 seconds", 6);
		item("7 seconds", 7);
		item("8 seconds", 8);
		item("9 seconds", 9);
		item("Wait Forever", 10);

	xoffs = 0.5;
	space(2);
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL | OS_UPDATE_ENGINE);
	space(2);
	help();
}


void cheat_options_t::build()
{
	title("Cheat Options");

	xoffs = 0.6;
	yesno("Push Move Mode", &prf->cheat_pushmove, 0);
	yesno("Freewheel Mode", &prf->cheat_freewheel, 0);
	yesno("Shield", &prf->cheat_shield, 0);
	yesno("Invulnerability", &prf->cheat_invulnerability, 0);
	yesno("Enemies Will Not Fire", &prf->cheat_ceasefire, 0);
	yesno("Extra Firepower", &prf->cheat_firepower, 0);
	yesno("Start Level Selector", &prf->cheat_startlevel, 0);
	yesno("Broken Triggers", &prf->cheat_brokentrigger, 0);
	space();
	list("Game Speed", &prf->cheat_speed, 0);
		item("Normal", 0.0f);
		item("10%", 0.1f);
		item("20%", 0.2f);
		item("25%", 0.25f);
		item("33%", 0.33f);
		item("50%", 0.5f);
		item("75%", 0.75f);
		item("125%", 1.25f);
		item("133%", 1.33f);
		item("150%", 1.5f);
		item("175%", 1.75f);
		item("200%", 2.0f);

	xoffs = 0.5;
	space(2);
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL);
	space(2);
	help();
}


void debug_options_t::build()
{
	title("Debug Options");

	xoffs = 0.6;
	yesno("General Debug Features", &prf->debug, 0);
	yesno("Show FPS Counter", &prf->show_fps, 0);
#ifndef KOBO_DEMO
	yesno("Show Map Border", &prf->show_map_border, 0);
	yesno("Show Coordinates", &prf->show_coordinates, 0);
	yesno("Show Tile/Sprite Edges", &prf->show_tiles, OS_RELOAD_GRAPHICS);
	yesno("Show Hit Zones", &prf->show_hit, 0);
#endif
	yesno("Timestamp Debug Output", &prf->tsdebug, 0);
#ifndef KOBO_DEMO
	yesno("Sound Design Tools", &prf->soundtools, 0);
#endif
	yesno("Replay Debug Data (Huge Saves!)", &prf->replaydebug, 0);
#ifndef KOBO_DEMO
	yesno("Force Fallback Graphics theme", &prf->force_fallback_gfxtheme,
			OS_RELOAD_GRAPHICS);
	yesno("Force Fallback Sound Theme", &prf->force_fallback_sfxtheme,
			OS_RELOAD_SOUNDS);
#endif

	xoffs = 0.5;
	space(2);
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL);
	space(2);
	help();
}
