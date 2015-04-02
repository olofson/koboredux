/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996, Akira Higuchi
 * Copyright 2001-2003, 2009 David Olofson
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

#ifndef _KOBO_CONFIG_H_
#define _KOBO_CONFIG_H_

#include "buildconfig.h"

#ifndef DEBUG
#	undef	DBG
#	undef	DBG2
#	undef	DBG3
#	undef	DBG4
#	define	DBG(x)
#	define	DBG2(x)
#	define	DBG3(x)
#	define	DBG4(x)
#endif

#ifndef KOBO_HAVE_SNPRINTF
#ifndef KOBO_HAVE__SNPRINTF
#error	Must have snprintf() or _snprintf!
#endif
#define snprintf _snprintf
#endif

#ifndef KOBO_HAVE_VSNPRINTF
#ifndef KOBO_HAVE__VSNPRINTF
#error	Must have vsnprintf() or _vsnprintf!
#endif
#define vsnprintf _vsnprintf
#endif

/* Key/button repeat timing */
#define	KOBO_KEY_DELAY		250
#define	KOBO_KEY_REPEAT		40

/*
 * This was originally 1024, but was changed in Kobo Deluxe 0.4.1
 * to avoid the bug where we run out of enemies when destroying a
 * base, and thus leave parts of it behind.
FIXME: Is 2048 actually enough with the new effects in 0.5.x+...?
 */
#define ENEMY_MAX		2048

/*
 * Fraction of the screen size in which clicks are not considered
 * clicks but movements in that direction (as regarded from the
 * center of the screen) or other special things (pause & exit).
 *
 * Used only in touchscreen mode.
 */
#define POINTER_MARGIN_PERCENT	10

/*
 * Fraction of the screen size in which clicks are not considered
 * clicks but movements in that direction (as regarded from the
 * center of the screen) or other special things (pause & exit).
 *
 * Used only in touchscreen mode.
 */
#define POINTER_MARGIN_PERCENT	10

/*
 * In XKobo, WSIZE was used where VIEWLIMIT is used now; in the game logic
 * code. The original value was 224.
 *
 * Kobo Redux replaces WSIZE with WMAIN_W and WMAIN_H, but VIEWLIMIT is still
 * what that determines what the game logic considers "in view!" The value is
 * changed to 336, to match the new view size.
 */
#define VIEWLIMIT		336

// Player ship hit rect size
#define HIT_MYSHIP		5

// Player bolt hit rect size
#define HIT_BOLT		5

/* Various size info (DO NOT EDIT!) */
#define CHIP_SIZEX_LOG2		4
#define CHIP_SIZEY_LOG2		4
#define MAP_SIZEX_LOG2		6
#define MAP_SIZEY_LOG2		7
#define WORLD_SIZEX_LOG2	(MAP_SIZEX_LOG2 + CHIP_SIZEX_LOG2)
#define WORLD_SIZEY_LOG2	(MAP_SIZEY_LOG2 + CHIP_SIZEY_LOG2)
#define NOISE_SIZEX_LOG2	8
#define CHIP_SIZEX		(1 << CHIP_SIZEX_LOG2)
#define CHIP_SIZEY		(1 << CHIP_SIZEY_LOG2)
#define MAP_SIZEX		(1 << MAP_SIZEX_LOG2)
#define MAP_SIZEY		(1 << MAP_SIZEY_LOG2)
#define WORLD_SIZEX		(1 << WORLD_SIZEX_LOG2)
#define WORLD_SIZEY		(1 << WORLD_SIZEY_LOG2)
#define NOISE_SIZEX		(1 << NOISE_SIZEX_LOG2)

/* Text scroller speed (pixels/second) */
#define	SCROLLER_SPEED		120

/* Intro loop timing */
#define	INTRO_BLANK_TIME	1000	/* Inter-page blanking */
#define	INTRO_TITLE_TIME	7000	/* "Real" title show time */
#define	INTRO_TITLE2_TIME	5500	/* Intermediate title show time */
#define	INTRO_INSTRUCTIONS_TIME	19700
#define	INTRO_HIGHSCORE_TIME	11700
#define	INTRO_CREDITS_TIME	13700

/*
 * This was originally 320x240. Kobo Redux changes the "native" resolution to
 * 640x360, while keeping the 16x16 tile size, so the view size is now 50%
 * larger than in XKobo and Kobo Deluxe.
 */
#define	SCREEN_WIDTH		640
#define	SCREEN_HEIGHT		360

/*
 * Map tile size in "native" 640x360 pixels. Until Kobo Redux, this was 32, but
 * indirectly derived from CHIP_SIZE*, rather than defined explicitly.
 */
#define	TILE_SIZE		16

/* Number of proximity warning LEDs along each edge of the main view window. */
#define	PROXY_LEDS		42

/* Proximity warning LED fade speed (units per logic frame; 16:16 fixp) */
#define	PROXY_LIGHTSPEED	16384
#define	PROXY_FADESPEED		8192

/* Proximity warning LED effects configuration */
#define	PROXY_SCAN_WIDTH	7
#define	PROXY_FLASH_PERIOD	30
#define	PROXY_FLASH_DURATION	20

/* Labeled text display */
#define	D_LINE_HEIGHT		9
#define	D_LINE1_POS		1
#define	D_LINE1_TXOFFS		0
#define	D_LINE2_POS		9
#define	D_LINE2_TXOFFS		0

/*
 * Dashboard window layout
 */
#define WMAIN_X			152
#define WMAIN_Y			12
#define WMAIN_W			336
#define WMAIN_H			336

#define WCONSOLE_X		8
#define WCONSOLE_Y		52
#define WCONSOLE_W		128
#define WCONSOLE_H		256

#define WRADAR_X		504
#define WRADAR_Y		52
#define WRADAR_W		128
#define WRADAR_H		256

#define TOPLEDS_X		152
#define TOPLEDS_Y		4
#define TOPLEDS_W		336
#define TOPLEDS_H		5

#define BOTTOMLEDS_X		152
#define BOTTOMLEDS_Y		351
#define BOTTOMLEDS_W		336
#define BOTTOMLEDS_H		5

#define LEFTLEDS_X		144
#define LEFTLEDS_Y		12
#define LEFTLEDS_W		5
#define LEFTLEDS_H		336

#define RIGHTLEDS_X		491
#define RIGHTLEDS_Y		12
#define RIGHTLEDS_W		5
#define RIGHTLEDS_H		336

#endif	/*_KOBO_CONFIG_H_*/
