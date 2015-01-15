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
	list("Maximum Frame Rate", &prf->max_fps, OS_REBUILD);
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
	if(prf->max_fps)
		yesno("Strictly Regulated", &prf->max_fps_strict, 0);

	space();
	xoffs = 0.6;
	list("Time Filter", &prf->timefilter, OS_UPDATE_ENGINE);
		item("Off", 100);
		item("Fast", 75);
		item("Normal", 50);
		item("Slow", 10);
		item("Logic/Video Lock", 0);
	onoff("Motion Interpolation", &prf->filter, OS_UPDATE_ENGINE);

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

	if(firstbuild)
	{
		vmm_Init(VMM_ALL, 0);
		VMM_Mode *vm = vmm_FindMode(prf->videomode);
		if(vm)
		{
			if(vm->flags & VMM_PC)
				showmodes = VMM_PC;
			else if(vm->flags & VMM_TV)
				showmodes = VMM_TV;
			else if(vm->flags & VMM_4_3)
				showmodes = VMM_4_3;
			else if(vm->flags & VMM_3_2)
				showmodes = VMM_3_2;
			else if(vm->flags & VMM_5_4)
				showmodes = VMM_5_4;
			else if(vm->flags & VMM_16_10)
				showmodes = VMM_16_10;
			else if(vm->flags & VMM_16_9)
				showmodes = VMM_16_9;
			else
				showmodes = VMM_ALL;
			if(vm->flags & VMM_LORES)
				showlow = 1;
		}
		else
		{
			showmodes = VMM_PC;
			showlow = 0;
			prf->videomode = -1;
		}
		firstbuild = 0;
	}

	xoffs = 0.35;
	list("Show Modes" , &showmodes, OS_REBUILD);
		item("Show All", VMM_ALL);
		item("Common PC/Mac Modes", VMM_PC);
		item("Common TV Modes", VMM_TV);
		item("4:3 (Standard CRT)", VMM_4_3);
		item("3:2 (NTSC TV)", VMM_3_2);
		item("5:4 (Some TFTs)", VMM_5_4);
		item("16:10 (PC/Mac Widescreen)", VMM_16_10);
		item("16:9 (TV Widescreen)", VMM_16_9);
	xoffs = 0.7;
	yesno("Show Odd Low Resolutions" , &showlow, OS_REBUILD);
	xoffs = 0.5;
	list("Display Mode", &prf->videomode, OS_RESTART_VIDEO | OS_REBUILD);
		vmm_Init(showmodes, showlow ? 0 : VMM_LORES);
		VMM_Mode *vm = vmm_First();
		char buf[256];
		buf[sizeof(buf) - 1] = 0;
		while(vm)
		{
			if(vm->name)
				snprintf(buf, sizeof(buf) - 1, "%dx%d (%s)",
						vm->width, vm->height, vm->name);
			else
				snprintf(buf, sizeof(buf) - 1, "%dx%d",
						vm->width, vm->height);
			item(buf, vm->id);
			vm = vmm_Next(vm);
		}
		item("Custom", -1);
	if(prf->videomode < 0)
	{
		spin("Width", &prf->width, 32, 4096, "pixels",
				OS_RESTART_VIDEO | OS_REBUILD);
		spin("Height", &prf->height, 32, 4096, "pixels",
				OS_RESTART_VIDEO | OS_REBUILD);
	}
	space();
	yesno("Fullscreen Display", &prf->fullscreen, OS_RESTART_VIDEO);
	list("Display Depth", &prf->depth, OS_RESTART_VIDEO);
		item("Default", 0);
		item("8 bits", 8);
		item("15 bits", 15);
		item("16 bits", 16);
		item("24 bits", 24);
		item("32 bits", 32);
#if defined(HAVE_OPENGL) && !defined(GLSDL_OFF)
	list("Display Driver", &prf->videodriver, OS_RESTART_VIDEO |
			OS_REBUILD);
		item("SDL 2D", GFX_DRIVER_SDL2D);
		item("OpenGL/glSDL", GFX_DRIVER_GLSDL);
#else
	prf->videodriver = GFX_DRIVER_SDL2D;
#endif
	space();
	xoffs = 0.55;
	switch(prf->videodriver)
	{
	  case GFX_DRIVER_SDL2D:
		list("Display Buffering Mode", &prf->doublebuf, OS_RESTART_VIDEO);
			item("Single", 0);
			item("Double", 1);
		yesno("Software Shadow Buffer", &prf->shadow, OS_RESTART_VIDEO);
		break;
	  case GFX_DRIVER_GLSDL:
		yesno("Vertical Retrace Sync", &prf->vsync, OS_RESTART_VIDEO);
		break;
	}
	list("Display Buffer Pages", &prf->pages, OS_UPDATE_ENGINE);
		item("Assume Standard", -1);
		item("Always Repaint", 0);
		item("Single", 1);
		item("Double", 2);
		item("Triple", 3);
	big();
	space();
	xoffs = 0.5;
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL);
}


void graphics_options_t::build()
{
	medium();
	label("Graphics Options");
	space();
	small();
	xoffs = 0.6;
	list("Scale Mode", &prf->scalemode, OS_RELOAD_GRAPHICS);
		item("Nearest", GFX_SCALE_NEAREST);
		item("Bilinear", GFX_SCALE_BILINEAR);
// FIXME: Bilinear+Oversampling is unreliable and doesn't help much anyway
//		item("Bilinear+Oversampling", GFX_SCALE_BILIN_OVER);

// FIXME: We need to specify a fallback chain for when filters do not support
// FIXME: ratios. So, we have quality and image specific requirements, and we
// FIXME: want a list of filters/modes in order of preference.

//	if((gengine->xscale() == 2.0f) && (gengine->yscale() == 2.0f))
//	{
		item("Scale2x", GFX_SCALE_SCALE2X);
		item("Diamond2x", GFX_SCALE_DIAMOND);
//	}
//	else if(prf->scalemode >= GFX_SCALE_SCALE2X)
//		prf->scalemode = GFX_SCALE_BILINEAR;
	space(1);
	yesno("Dithering", &prf->use_dither, OS_RELOAD_GRAPHICS);
	list("Dither Type", &prf->dither_type, OS_RELOAD_GRAPHICS);
		item("2x2 Filter", 0);
		item("4x4 Filter", 1);
		item("Random", 2);
	switch(prf->videodriver)
	{
	  case GFX_DRIVER_GLSDL:
		yesno("Broken RGBA8 (OpenGL)", &prf->broken_rgba8,
				OS_RELOAD_GRAPHICS);
		break;
	}
	space();
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
	yesno("Enable Sound", &prf->use_sound, OS_RESTART_AUDIO | OS_REBUILD);
	if(prf->use_sound)
	{
		yesno("Enable Music", &prf->use_music, OS_RESTART_AUDIO | OS_REBUILD);
		yesno("Cached Sounds", &prf->cached_sounds, OS_RELOAD_AUDIO_CACHE);

		//System
		space();
		list("Sample Rate", &prf->samplerate, OS_RESTART_AUDIO);
			item("8 kHz", 8000);
			item("11025 Hz", 11025);
			item("16 kHz", 16000);
			item("22050", 22050);
			item("32 kHz", 32000);
			item("44.1 kHz", 44100);
			item("48 kHz", 48000);
			item("88.2 kHz", 88200);
			item("96 kHz", 96000);
			item("176.4 kHz", 176400);
			item("192 kHz", 192000);
#if 0
		// Audiality 2 only has 3 quality levels, set at build time!
		list("Mixing Quality", &prf->mixquality, OS_UPDATE_AUDIO);
			item("Very Low", AQ_VERY_LOW);
			item("Low", AQ_LOW);
			item("Normal", AQ_NORMAL);
			item("High", AQ_HIGH);
			item("Very High", AQ_VERY_HIGH);
#endif
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
#ifdef HAVE_OSS
		yesno("Use OSS Sound Driver", &prf->use_oss, OS_RESTART_AUDIO);
#else
		// We're running out of space here... :-/
		space();
#endif
		//Mixer
		list("Sound Effects Level", &prf->sfx_vol, OS_UPDATE_AUDIO);
			item("OFF", 0);
			perc_list(10, 90, 10);
			perc_list(100, 200, 25);
		if(prf->use_music)
		{
			list("Intro Music Level", &prf->intro_vol, OS_UPDATE_AUDIO);
				item("OFF", 0);
				perc_list(10, 90, 10);
				perc_list(100, 200, 25);
			list("In-Game Music Level", &prf->music_vol, OS_UPDATE_AUDIO);
				item("OFF", 0);
				perc_list(10, 90, 10);
				perc_list(100, 200, 25);
		}

		//Master effects
		list("Ambience", &prf->reverb, OS_UPDATE_AUDIO);
			item("OFF", 0);
			item("Low", 50);
			item("Normal", 100);
			item("High", 175);
			item("Extreme", 250);
		list("Volume Boost", &prf->vol_boost, OS_UPDATE_AUDIO);
			item("OFF", 0);
			item("Low", 1);
			item("Moderate", 2);
			item("High", 3);
			item("Extreme", 4);
	}
	space();
	big();
	xoffs = 0.5;
	button("ACCEPT", OS_CLOSE);
	button("CANCEL", OS_CANCEL | OS_UPDATE_AUDIO);
}

void audio_options_t::undo_hook()
{
	clearstatus(OS_RELOAD | OS_RESTART | OS_UPDATE);
	setstatus(OS_UPDATE_AUDIO);
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
		yesno("Use Joystick", &prf->use_joystick,
				OS_RESTART_INPUT | OS_REBUILD);
		if(prf->use_joystick)
		{
			if(prf->number_of_joysticks)
			{
				list("Joystick Number", &prf->joystick_no,
						OS_RESTART_INPUT);
					enum_list(0, prf->number_of_joysticks - 1);
			}
			else
				label("No Joysticks Found!");
		}
	}
	space();

	yesno("Use Mouse", &prf->use_mouse, OS_RESTART_INPUT | OS_REBUILD);
	if(prf->use_mouse)
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
	list("Radar Scroll Mode", &prf->scrollradar, 0);
		item("Off", 0);
// FIXME: Radar sweep mode is broken!
//		item("Sweep", 1);
		item("Scroll", 2);
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
	list("Starfield Mode", &prf->starfield, OS_REBUILD);
		item("None", STARFIELD_NONE);
		item("Old (Flat)", STARFIELD_OLD);
		item("Parallax", STARFIELD_PARALLAX);
	if(prf->starfield == STARFIELD_PARALLAX)
	{
		list("Starfield Density", &prf->stars, 0);
			item("Minimal", 50);
			item("Low", 250);
			item("Normal", 500);
			item("High", 1000);
			item("Massive", 2500);
			item("Insane", 8000);
	}
	space();
	xoffs = 0.7;
	list("Cannon Overheat Warning", &prf->overheatloud, 0);
		item("Off", 0);
		item("Soft", 50);
		item("Normal", 100);
		item("Loud", 200);
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

void game_options_t::undo_hook()
{
	clearstatus(OS_RELOAD | OS_RESTART | OS_UPDATE);
	setstatus(OS_UPDATE_ENGINE);
}
