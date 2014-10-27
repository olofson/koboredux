/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996, Akira Higuchi
 * Copyright (C) 2001-2003, 2009 David Olofson
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

#ifndef _KOBO_CONFIG_H_
#define _KOBO_CONFIG_H_

#include <aconfig.h>

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

#ifndef HAVE_SNPRINTF
#ifndef HAVE__SNPRINTF
#error	Must have snprintf() or _snprintf!
#endif
#define snprintf _snprintf
#endif

#ifndef HAVE_VSNPRINTF
#ifndef HAVE__VSNPRINTF
#error	Must have vsnprintf() or _vsnprintf!
#endif
#define vsnprintf _vsnprintf
#endif

/*
 * On some platforms, assignments of these "opaque objects" are
 * illegal - while on others, the macro to handle that can be
 * missing... Oh, and this is really only in C99, of course. :-/
 */
#include <stdarg.h>
#ifndef va_copy
#	ifdef __va_copy
#		define va_copy(to, from) __va_copy(to, from)
#	else
#		define va_copy(to, from) (to) = (from)
#	endif
#endif

/* Key/button repeat timing */
#define	KOBO_KEY_DELAY	250
#define	KOBO_KEY_REPEAT	40

/*
 * This was originally 1024, but was changed in Kobo Deluxe 0.4.1
 * to avoid the bug where we run out of enemies when destroying a
 * base, and thus leave parts of it behind.
FIXME: Is 2048 actually enough with the new effects in 0.5.x+...?
 */
#define ENEMY_MAX	2048

/*
 * Game display size.
 *
 * In XKobo, there was only WSIZE - VIEWLIMIT was added
 * for Kobo Deluxe, and WSIZE was set to 230, to add some
 * extra pixels, most of which were more or less covered
 * by the rounded framework.
 *
 * As of Kobo Deluxe 0.4.1, WSIZE is restored to the
 * original 224.
 *
 * Note that if WSIZE is larger than VIEWLIMIT, some
 * objects might be created or deleted in view. Finding
 * out whether or not it can really happen calls for
 * closer analysis of the code.
 */
#define WSIZE	224
#define MARGIN	8

/*
 * Fraction of the screen size in which clicks are not considered
 * clicks but movements in that direction (as regarded from the
 * center of the screen) or other special things (pause & exit).
 *
 * Used only in touchscreen mode.
 */
#define POINTER_MARGIN_PERCENT 10

/*
 * Fraction of the screen size in which clicks are not considered
 * clicks but movements in that direction (as regarded from the
 * center of the screen) or other special things (pause & exit).
 *
 * Used only in touchscreen mode.
 */
#define POINTER_MARGIN_PERCENT 10

/*
 * (In XKobo, WSIZE was used where this is
 * used now; in the game logic code.)
 *
 * NOTE:  I *DON'T* want to change the view
 *        range as the XKobo engine knows it,
 *        as that would make the game play
 *        slightly differently. (Probably not
 *        so that anyone would notice, but
 *        let's not take chances... This is
 *        NOT "Kobo II".)
 */
#define VIEWLIMIT	224

// Player ship hit rect size
#define HIT_MYSHIP	5

// Player bolt hit rect size
#define HIT_BOLT	5

/* Actually, this is not the *full* size in windowed mode any more. */
#define	SCREEN_WIDTH	320
#define	SCREEN_HEIGHT	240

/* Various size info (DO NOT EDIT!) */
#define CHIP_SIZEX_LOG2   4
#define CHIP_SIZEY_LOG2   4
#define MAP_SIZEX_LOG2    6
#define MAP_SIZEY_LOG2    7
#define WORLD_SIZEX_LOG2 (MAP_SIZEX_LOG2+CHIP_SIZEX_LOG2)
#define WORLD_SIZEY_LOG2 (MAP_SIZEY_LOG2+CHIP_SIZEY_LOG2)
#define NOISE_SIZEX_LOG2   8

#define CHIP_SIZEX        (1<<CHIP_SIZEX_LOG2)
#define CHIP_SIZEY        (1<<CHIP_SIZEY_LOG2)
#define MAP_SIZEX         (1<<MAP_SIZEX_LOG2)
#define MAP_SIZEY         (1<<MAP_SIZEY_LOG2)
#define WORLD_SIZEX      (1<<WORLD_SIZEX_LOG2)
#define WORLD_SIZEY      (1<<WORLD_SIZEY_LOG2)
#define NOISE_SIZEX        (1<<NOISE_SIZEX_LOG2)

/* Text scroller speed (pixels/second) */
#define	SCROLLER_SPEED	120

/* Intro loop timing */
#define	INTRO_BLANK_TIME	1000	/* Inter-page blanking */
#define	INTRO_TITLE_TIME	7000	/* "Real" title show time */
#define	INTRO_TITLE2_TIME	5500	/* Intermediate title show time */
#define	INTRO_INSTRUCTIONS_TIME	19700
#define	INTRO_HIGHSCORE_TIME	11700
#define	INTRO_CREDITS_TIME	13700

#endif	/*_KOBO_CONFIG_H_*/
