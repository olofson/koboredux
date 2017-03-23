/*(LGPLv2.1)
---------------------------------------------------------------------------
	logger.c - Simple logger with redirection
---------------------------------------------------------------------------
 * Copyright 2003, 2009 David Olofson
 * Copyright 2015, 2017 David Olofson (Kobo Redux)
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

#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "config.h"
#include "logger.h"
#include "SDL.h"

#define	LOG_BUFFER	1024

static int i__last;
#define	for_one_or_all(iterator, index, items)		\
	if(index < LOG_TARGETS)				\
	{						\
		if(index >= 0)				\
			iterator = i__last = index;	\
		else					\
		{					\
			iterator = 0;			\
			i__last = items - 1;		\
		}					\
	}						\
	else						\
	{						\
		iterator = 0;				\
		i__last = -10;				\
	}						\
	--iterator;					\
	while(++iterator <= i__last)


typedef struct
{
	/* Stream output */
	int		use_stream;
	FILE		*stream;

	/* Callback output */
	void		*handle;
	int (*callback)(void *handle, const char *data);

	/* Formatting control */
	unsigned	flags;
	int		written;
} LOG_target;


typedef struct
{
	/* Output */
	unsigned	targets;

	/* Attributes */
	unsigned	attr;
} LOG_level;


static LOG_level *l_levels = NULL;
static LOG_target *l_targets = NULL;
static char *l_buffer = NULL;

Uint32 start_time = 0;


/*--------------------------------------------------------------------
	Low level internal stuff
--------------------------------------------------------------------*/

static inline int target_is_active(int target)
{
	return l_targets[target].use_stream || l_targets[target].callback;
}


static inline int level_is_active(int level)
{
	int t;
	if(!l_levels[level].targets)
		return 0;
	for(t = 0; t < LOG_TARGETS; ++t)
		if(target_is_active(t))
			return 1;
	return 0;
}


/* Write stuff without any translation */
static inline int log_write_raw(int target, const char *text)
{
	if(l_targets[target].use_stream)
		return fputs(text, l_targets[target].stream);
	else if(l_targets[target].callback)
		return l_targets[target].callback(
				l_targets[target].handle, text);
	else
		return 0;
}


static void write_html_header(int target)
{
	log_write_raw(target, "<!DOCTYPE HTML PUBLIC "
			"\"-//W3C//DTD HTML 4.01 Transitional//EN\" "
			"\"http://www.w3.org/TR/REC-html40/loose.dtd\">\n");
	log_write_raw(target, "<HTML>\n\t<HEAD>\n");
	log_write_raw(target, "\t\t<TITLE>Log File</TITLE>\n");
	log_write_raw(target, "\t\t<meta http-equiv=\"Content-Type\" "
			"content=\"text/html; charset=ISO-8859-1\">\n");
	log_write_raw(target, "\t</HEAD>\n");
	log_write_raw(target, "\t<BODY BGCOLOR=#000000 TEXT=#999999>\n");
}


static void write_html_footer(int target)
{
	log_write_raw(target, "\t</BODY>\n</HTML>\n");
}


/* Write log file header if applicable. */
static inline void check_header(int target)
{
	if(l_targets[target].written)
		return;

	if(l_targets[target].flags & LOG_HTML)
		write_html_header(target);
	l_targets[target].written = 1;
}


/* Write log file footer if applicable. */
static inline void check_footer(int target)
{
	if(!l_targets[target].written)
		return;

	if(l_targets[target].flags & LOG_HTML)
		write_html_footer(target);
}


/* Write converting/escaping any reserved characters as needed */
static inline int log_write(int target, const char *text)
{
	int i;
	if(l_targets[target].flags & LOG_HTML)
	{
/*
FIXME: Optimize this a little, maybe. :-)
*/
		char buf[2] = "\0\0";
		int last = -1;
		for(i = 0; text[i]; ++i)
		{
			buf[0] = text[i];
			switch(text[i])
			{
			  case '\n':
				log_write_raw(target, "<br>\n");
				break;
			  case ' ':
				if(' ' == last)
					log_write_raw(target, "&nbsp;");
				else
					log_write_raw(target, " ");
				break;
			  case '<':
				log_write_raw(target, "&lt;");
				break;
			  case '>':
				log_write_raw(target, "&gt;");
				break;
			  case '&':
				log_write_raw(target, "&amp;");
				break;
			  default:
				log_write_raw(target, buf);
				break;
			}
			last = text[i];
		}
		return i;
	}
	else
		return log_write_raw(target, text);
}


static inline int put_timestamp(int target)
{
	char buf[16];
	if(!(l_targets[target].flags & LOG_TIMESTAMP))
		return 0;
	snprintf(buf, sizeof(buf) - 1, "[%d] ", SDL_GetTicks() - start_time);
	return log_write_raw(target, buf);
}


#define	ANSI_SUPPORTED_FLAGS	(LOG_COLORS | LOG_BRIGHT | LOG_STRONG | LOG_BLINK)
#define	HTML_SUPPORTED_FLAGS	(LOG_COLORS | LOG_BRIGHT | LOG_STRONG | LOG_BLINK)

static inline void begin_ansi_attr(unsigned a, int target)
{
	int pos = 0;
	char buf[24];
	if(!(a & ANSI_SUPPORTED_FLAGS))
		return;

#define	SEP	if('[' != buf[pos - 1]) buf[pos++] = ';'
	buf[pos++] = '\033';
	buf[pos++] = '[';
	if(a & LOG_COLORS)
	{
		buf[pos++] = '3';
		buf[pos++] = '0' + (a & LOG_COLORS) - 1;
	}
	if(a & LOG_BRIGHT)
	{
		SEP;
		buf[pos++] = '1';
	}
	if(a & LOG_STRONG)
	{
		SEP;
		buf[pos++] = '4';
	}
	if(a & LOG_BLINK)
	{
		SEP;
		buf[pos++] = '5';
	}
	buf[pos++] = 'm';
	buf[pos++] = '\0';
	log_write_raw(target, buf);
#undef SEP
}


static inline void end_ansi_attr(unsigned a, int target)
{
	if(a & ANSI_SUPPORTED_FLAGS)
		log_write_raw(target, "\033[0m");
}


static inline void begin_html_attr(unsigned a, int target)
{
	if(a & LOG_COLORS)
	{
		const char *sel;
		if(a & LOG_BRIGHT)
			switch( (a & LOG_COLORS) - 1)
			{
			  case 0: sel = "<FONT COLOR=#333333>"; break;
			  case 1: sel = "<FONT COLOR=#990000>"; break;
			  case 2: sel = "<FONT COLOR=#009900>"; break;
			  case 3: sel = "<FONT COLOR=#999900>"; break;
			  case 4: sel = "<FONT COLOR=#000099>"; break;
			  case 5: sel = "<FONT COLOR=#990099>"; break;
			  case 6: sel = "<FONT COLOR=#009999>"; break;
			  case 7:
			  default:
				sel = "<FONT COLOR=#999999>"; break;
			}
		else
			switch( (a & LOG_COLORS) - 1)
			{
			  case 0: sel = "<FONT COLOR=#666666>"; break;
			  case 1: sel = "<FONT COLOR=#ff0000>"; break;
			  case 2: sel = "<FONT COLOR=#00ff00>"; break;
			  case 3: sel = "<FONT COLOR=#ffff00>"; break;
			  case 4: sel = "<FONT COLOR=#0000ff>"; break;
			  case 5: sel = "<FONT COLOR=#ff00ff>"; break;
			  case 6: sel = "<FONT COLOR=#00ffff>"; break;
			  case 7:
			  default:
				sel = "<FONT COLOR=#ffffff>"; break;
			}
		log_write_raw(target, sel);
	}
	if(a & LOG_STRONG)
		log_write_raw(target, "<STRONG>");
	if(a & LOG_BLINK)
		log_write_raw(target, "<BLINK>");
}


static inline void end_html_attr(unsigned a, int target)
{
	if(a & LOG_BLINK)
		log_write_raw(target, "</BLINK>");
	if(a & LOG_STRONG)
		log_write_raw(target, "</STRONG>");
	if(a & LOG_COLORS)
		log_write_raw(target, "</FONT>");
}


static inline void set_attr(int level, int target)
{
	if(l_levels[level].targets & (1 << target))
	{
		if(l_targets[target].flags & LOG_ANSI)
			begin_ansi_attr(l_levels[level].attr, target);
		else if(l_targets[target].flags & LOG_HTML)
			begin_html_attr(l_levels[level].attr, target);
	}
}


static inline void reset_attr(int level, int target)
{
	if(l_levels[level].targets & (1 << target))
	{
		if(l_targets[target].flags & LOG_ANSI)
			end_ansi_attr(l_levels[level].attr, target);
		else if(l_targets[target].flags & LOG_HTML)
			end_html_attr(l_levels[level].attr, target);
	}
}


/*--------------------------------------------------------------------
	API entry points
--------------------------------------------------------------------*/

int log_open(int flags)
{
	if(l_levels)
		return 0;

	l_levels = calloc(LOG_LEVELS, sizeof(LOG_level));
	if(!l_levels)
		return -1;

	l_targets = calloc(LOG_TARGETS, sizeof(LOG_target));
	if(!l_targets)
	{
		log_close();
		return -2;
	}
	l_buffer = calloc(1, LOG_BUFFER);
	if(!l_buffer)
	{
		log_close();
		return -3;
	}

	log_set_target_stream(-1, stdout);
	log_set_target_flags(-1, 0);

	log_enable_level_target(-1, 0);
	log_set_level_attr(-1, LOG_NOCOLOR);

	if(flags & LOG_RESET_TIME)
		start_time = SDL_GetTicks();

	return 0;
}


#define	CHECK_INIT	(l_levels ? 0 : -1)


void log_close(void)
{
	int t;
	if(CHECK_INIT < 0)
		return;
	for(t = 0; t < LOG_TARGETS; ++t)
		check_footer(t);
	free(l_targets);
	l_targets = NULL;
	free(l_levels);
	l_levels = NULL;
	free(l_buffer);
	l_buffer = NULL;
}


void log_set_target_stream(int target, FILE *stream)
{
	int i;
	if(CHECK_INIT < 0)
		return;

	for_one_or_all(i, target, LOG_TARGETS)
	{
		l_targets[i].callback = NULL;
		l_targets[i].stream = stream;
		l_targets[i].use_stream = (stream != NULL);
	}
}


void log_set_target_callback(int target,
		int (*callback)(void *handle, const char *data), void *handle)
{
	int i;
	if(CHECK_INIT < 0)
		return;

	for_one_or_all(i, target, LOG_TARGETS)
	{
		l_targets[i].use_stream = 0;
		l_targets[i].callback = callback;
		l_targets[i].handle = handle;
	}
}


void log_set_target_flags(int target, unsigned flags)
{
	int i;
	if(CHECK_INIT < 0)
		return;

	for_one_or_all(i, target, LOG_TARGETS)
		l_targets[i].flags = flags;
}


void log_enable_level_target(int level, int target)
{
	int i;
	if(CHECK_INIT < 0)
		return;

	for_one_or_all(i, level, LOG_LEVELS)
		if(target == -1)
			l_levels[i].targets = 0xffffffff;
		else
			l_levels[i].targets |= 1 << target;
}


void log_disable_level_target(int level, int target)
{
	int i;
	if(CHECK_INIT < 0)
		return;

	for_one_or_all(i, level, LOG_LEVELS)
		if(target == -1)
			l_levels[i].targets = 0;
		else
			l_levels[i].targets &= ~(1 << target);
}


void log_set_level_attr(int level, unsigned attr)
{
	int i;
	if(CHECK_INIT < 0)
		return;

	for_one_or_all(i, level, LOG_LEVELS)
		l_levels[i].attr = attr;
}


static inline int log_print(int level, const char *text)
{
	int t;
	int result = 0;

	if(CHECK_INIT < 0)
		return -1;

	if(level < 0 || level >= LOG_LEVELS)
		return -2;

	if(!l_levels[level].targets)
		return 0;

	for(t = 0; t < LOG_TARGETS; ++t)
	{
		int r;
		if(!(l_levels[level].targets & (1 << t)))
			continue;

		if(!target_is_active(t))
			continue;

		check_header(t);
		set_attr(level, t);
		put_timestamp(t);
		r = log_write(t, text);
		if(r > 0)
			result += r;
		reset_attr(level, t);
	}
	return result;
}


int log_puts(int level, const char *text)
{
	if(CHECK_INIT < 0)
	{
		fputs(text, stderr);
		fputs("\n[Logging not yet initialized!]\n", stderr);
		return -1;
	}
	snprintf(l_buffer, LOG_BUFFER - 1, "%s\n", text);
	return log_print(level, l_buffer);
}


int log_printf(int level, const char *format, ...)
{
	va_list args;
	int result;

	if(CHECK_INIT < 0)
	{
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
		fputs("[Logging not yet initialized!]\n", stderr);
		return -1;
	}

	if(level < 0 || level >= LOG_LEVELS)
		return -2;

	if(!level_is_active(level))
		return 0;

	va_start(args, format);
	result = vsnprintf(l_buffer, LOG_BUFFER - 1, format, args);
	va_end(args);
	if(result > 0)
		return log_print(level, l_buffer);
	else
		return 0;
}
