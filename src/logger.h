/*(LGPLv2.1)
---------------------------------------------------------------------------
	logger.h - Simple logger with redirection
---------------------------------------------------------------------------
 * Copyright (C) 2003, 2009 David Olofson
 *
 * This library is free software;  you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation;  either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library  is  distributed  in  the hope that it will be useful,  but
 * WITHOUT   ANY   WARRANTY;   without   even   the   implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library;  if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef	DO_LOGGER_H
#define	DO_LOGGER_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
TODO: Levels sending to multiple targets.
TODO: Printing to multiple levels.
*/

#define	LOG_TARGETS	8
#define	LOG_LEVELS	32

/*
 * Standard log levels (so libs and stuff can share your logger! :-)
 */
#define	ULOG	0	/* User:	Messages meant for normal users */
#define	WLOG	1	/* Warning:	Warning messages */
#define	ELOG	2	/* Error:	Error messages */
#define	VLOG	3	/* Verbose:	Messages for advanced users */
#define	DLOG	4	/* Debug:	Debug messages */


typedef enum
{
	LOG_ANSI =	0x00000001,	/* Use ANSI escape codes */
	LOG_HTML =	0x00000002,	/* Use HTML formatting tags */
	LOG_TIMESTAMP =	0x00000100	/* Timestamp messages */
} LOG_flags;


typedef enum
{
	LOG_COLORS =	0x000f,		/* Mask for color bits */
	LOG_NOCOLOR = 0,		/* Don't try to change! */
	LOG_BLACK,
	LOG_RED,
	LOG_GREEN,
	LOG_YELLOW,
	LOG_BLUE,
	LOG_MAGENTA,
	LOG_CYAN,
	LOG_WHITE,
	LOG_BRIGHT =	0x0100,		/* High intensity colors */
	LOG_STRONG =	0x0200,		/* Bold or underlined */
	LOG_BLINK =	0x0400		/* Blinking, if possible */
} LOG_attributes;


/*
 * The usual open/close calls...
 *
 * Note that log_close() may write footers and stuff to files,
 * so you may get slightly broken log files if you exit
 * without calling it!
 *
 * log_open() returns a negative value in case of failure.
 */
int log_open(void);
void log_close(void);


/*
 * Send output through 'target' to 'stream'. 'stream' must be
 * a valid stdio stream opened for output.
 *
 * If 'target' is -1, all targets are changed.
 */
void log_set_target_stream(int target, FILE *stream);


/*
 * Send output through 'target' to 'callback'. The callback
 * will receive the value passed as 'handle' as it's first
 * argument, followed by the data to output. (NULL terminated
 * string.) The callback should return the number of bytes
 * successfully handled, or, in case of failure, a negative
 * value.
 *
 * If 'target' is -1, all targets are changed.
 */
void log_set_target_callback(int target,
		int (*callback)(int handle, const char *data), int handle);


/*
 * Set formatting flags for 'target'.
 *
 * If 'target' is -1, all targets are changed.
 */
void log_set_target_flags(int target, unsigned flags);


/*
 * Send output through 'level' to 'target'. 'levels' is a bit
 * mask with one bit for each log level.
 *
 * If 'level' is -1, all levels are changed.
 */
void log_set_level_target(int level, int target);


/*
 * Set attributes for 'level'.
 *
 * If 'level' is -1, all levels are changed.
 */
void log_set_level_attr(int level, unsigned attr);


/*
 * Print 'text' to 'level'. 'level' is the index of the log
 * level to print to. No formatting will be done, and the
 * output will be followed by a "newline".
 *
 * Returns a negative value in case of failure.
 */
int log_puts(int level, const char *text);


/*
 * Format and print to 'level'. 'level' is the index of the
 * log level to print to.
 *
 * Returns a negative value in case of failure.
 */
int log_printf(int level, const char *format, ...);

#ifdef __cplusplus
};
#endif

#endif	/* DO_LOGGER_H */
