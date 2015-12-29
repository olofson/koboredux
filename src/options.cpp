/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001-2003, 2006-2007, 2009 David Olofson
 * Copyright 2005 Erik Auerswald
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


#include "config.h"
#include "options.h"
#include "gfxengine.h"
#include "gamectl.h"
#include "kobo.h"


void system_options_t::build()
{
	medium();
	label("System Options");

	space();
	small();
	xoffs = 0.6;
	yesno("Quick Startup", &prf->quickstart, 0);

	space();
	xoffs = 0.6;
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
	xoffs = 0.6;
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

	space();
	xoffs = 0.6;
	yesno("Log Messages to File", &prf->logfile, OS_RESTART_LOGGER);
	list("Log Output Format", &prf->logformat, 0);
		item("Plain Text", 0);
		item("ANSI Coloring", 1);
		item("HTML", 2);
	xoffs = 0.45;
	list("Log Verbosity", &prf->logverbosity, 0);
		item("Critical Errors Only", 0);
		item("Errors Only", 1);
		item("Errors & Warnings", 2);
		item("Some Debug", 3);
		item("More Debug", 4);
		item("Everything", 5);

	big();
	space();
	xoffs = 0.5;
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL);
}


void video_options_t::build()
{
	medium();
	label("Video Options");
	space();
	small();

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

	xoffs = 0.35;
	list("Show Modes" , &showmodes, OS_REBUILD);
		item("Show All", VMM_ALL);
		item("Recommended Modes", VMM__RECOMMENDED);
		item("Common Modes", VMM__COMMON);
		item("Widescreen Modes", VMM__WIDESCREEN);
		item("Non Widescreen Modes", VMM__NONWIDESCREEN);
	xoffs = 0.7;
	yesno("Show Odd Low Resolutions" , &showlow, OS_REBUILD);
	xoffs = 0.5;
	list("Display Mode", &prf->videomode, OS_RESTART_VIDEO | OS_REBUILD);
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
	xoffs = 0.55;
	yesno("Vertical Retrace Sync", &prf->vsync, OS_RESTART_VIDEO);
	big();
	space();
	xoffs = 0.5;
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL);
}

void video_options_t::close()
{
	vmm_Close();
	config_form_t::close();
}


void graphics_options_t::build()
{
	medium();
	label("Graphics Options");
	space();
	small();
	xoffs = 0.6;
	list("Planet Dither Style", &prf->planetdither, OS_UPDATE_SCREEN);
		item("None", SPINPLANET_DITHER_NONE);
		item("Random", SPINPLANET_DITHER_RANDOM);
		item("Ordered", SPINPLANET_DITHER_ORDERED);
		item("Skewed", SPINPLANET_DITHER_SKEWED);
		item("Noise", SPINPLANET_DITHER_NOISE);
		item("Semi Interlace", SPINPLANET_DITHER_SEMIINTERLACE);
		item("Interlace", SPINPLANET_DITHER_INTERLACE);
		item("TrueColor", SPINPLANET_DITHER_TRUECOLOR);
	list("Scale Mode", &prf->scalemode, OS_RELOAD_GRAPHICS);
		item("Nearest", GFX_SCALE_NEAREST);
		item("Bilinear", GFX_SCALE_BILINEAR);
		item("Scale2x", GFX_SCALE_SCALE2X);
		item("Diamond2x", GFX_SCALE_DIAMOND);
	space(1);
	yesno("Alpha Blending", &prf->alpha, OS_RELOAD_GRAPHICS);
	space();
	list("Brightness", &prf->brightness, OS_RELOAD_GRAPHICS);
		perc_list(50, 150, 10);
	list("Contrast", &prf->contrast, OS_RELOAD_GRAPHICS);
		perc_list(50, 150, 10);
	big();
	space();
	xoffs = 0.5;
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL);
}


void audio_options_t::build()
{
	medium();
	label("Sound Options");
	small();
	space();
	xoffs = 0.57;
	yesno("Enable Sound", &prf->enable_sound,
			OS_RESTART_AUDIO | OS_REBUILD);
	if(prf->enable_sound)
	{
		yesno("Enable Music", &prf->enable_music,
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
			item("22050 Hz", 22050);
			item("44.1 kHz", 44100);
			item("48 kHz", 48000);
			item("96 kHz", 96000);
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
		if(prf->enable_music)
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
	space();
	big();
	xoffs = 0.5;
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL | OS_UPDATE_AUDIO);
}

void audio_options_t::prepare_to_apply()
{
	if(driver >= 0)
		strcpy(prf->audiodriver, drivers[driver]);
}


void control_options_t::build()
{
	medium();
	label("Control Options");
	space();
	small();
	xoffs = 0.67;
	yesno("Always Fire", &prf->always_fire, OS_RESTART_INPUT);
	space();
	yesno("Broken NumPad Diagonals", &prf->broken_numdia, 0);
	list("Diagonals Emphasis Filter", &prf->dia_emphasis, 0);
		item("OFF", 0);
		item("1 frame", 1);
		item("2 frames", 2);
		item("3 frames", 3);
		item("4 frames", 4);
		item("5 frames", 5);
	space();

	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
		label("Could not initialize joysticks!");
	else
	{
		prf->number_of_joysticks = SDL_NumJoysticks();
		yesno("Use Joystick", &prf->enable_joystick,
				OS_RESTART_INPUT | OS_REBUILD);
		if(prf->enable_joystick)
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

	yesno("Use Mouse", &prf->enable_mouse, OS_RESTART_INPUT | OS_REBUILD);
	if(prf->enable_mouse)
	{
		list("Mouse Control Mode", &prf->mousemode, OS_RESTART_INPUT);
			item("Disabled", MMD_OFF);
			item("Crosshair", MMD_CROSSHAIR);
	}
	space();
	yesno("In-game Mouse Capture", &prf->mousecapture, 0);
	big();
	space();
	xoffs = 0.5;
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL);
}


void game_options_t::build()
{
	medium();
	label("Game Options");
	space();
	small();
	xoffs = 0.6;
	yesno("Scrolling Radar", &prf->scrollradar, 0);
	space();
	list("Get Ready Countdown", &prf->countdown, 0);
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
	space();
	list("Starfield Density", &prf->stars, 0);
		item("Minimal", 50);
		item("Low", 250);
		item("Normal", 500);
		item("High", 1000);
		item("Massive", 2500);
		item("Insane", 8000);
	space();
	xoffs = 0.7;
	list("Cannon Sound Suppression", &prf->cannonloud, 0);
		item("Off", 200);
		item("Low", 100);
		item("High", 50);
	big();
	space();
	xoffs = 0.5;
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL | OS_UPDATE_ENGINE);
}
