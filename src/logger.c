/*(LGPLv2.1)
---------------------------------------------------------------------------
	logger.c - Simple logger with redirection
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

#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "config.h"
#include "logger.h"
#include "glSDL.h"

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
	int		handle;
	int (*callback)(int handle, const char *data);

	/* Formatting control */
	unsigned	flags;
	int		written;
} LOG_target;


typedef struct
{
	/* Output */
	int		target;

	/* Atributes */
	unsigned	attr;
} LOG_level;


static LOG_level *l_levels = NULL;
static LOG_target *l_targets = NULL;
static char *l_buffer = NULL;

Uint32 start_time;


/*--------------------------------------------------------------------
	Low level internal stuff
--------------------------------------------------------------------*/

/* Write stuff without any translation */
static inline int log_write_raw(int target, const char *text)
{
	if(l_targets[target].use_stream)
		return fputs(text, l_targets[target].stream);
	else if(l_targets[target].callback)
		return l_targets[target].callback(
				l_targets[target].handle, text);
	else
	{
		/* We should never get here. */
		assert(0);
	}
}


/* Write log file header if applicable. */
static inline void check_header(int target)
{
	if(l_targets[target].written)
		return;

	if(l_targets[target].flags & LOG_HTML)
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
	l_targets[target].written = 1;
}


/* Write log file footer if applicable. */
static inline void check_footer(int target)
{
	if(!l_targets[target].written)
		return;

	if(l_targets[target].flags & LOG_HTML)
		log_write_raw(target, "\t</BODY>\n</HTML>\n");
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


static inline void put_timestamp(int level)
{
	if(l_targets[l_levels[level].target].flags & LOG_TIMESTAMP)
	{
		char buf[16];
		snprintf(buf, sizeof(buf)-1, "[%d] ", SDL_GetTicks() - start_time);
		log_write_raw(l_levels[level].target, buf);
	}
}


#define	ANSI_SUPPORTED_FLAGS	(LOG_COLORS | LOG_BRIGHT | LOG_STRONG | LOG_BLINK)
#define	HTML_SUPPORTED_FLAGS	(LOG_COLORS | LOG_BRIGHT | LOG_STRONG | LOG_BLINK)

static inline void set_attr(int level)
{
	unsigned a = l_levels[level].attr;
	if(l_targets[l_levels[level].target].flags & LOG_ANSI)
	{
#define	SEP	if('[' != buf[pos - 1]) buf[pos++] = ';'
		int pos = 0;
		char buf[24];
		if(!(a & ANSI_SUPPORTED_FLAGS))
			return;

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
		log_write_raw(l_levels[level].target, buf);
#undef SEP
	}
	else if(l_targets[l_levels[level].target].flags & LOG_HTML)
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
			log_write_raw(l_levels[level].target, sel);
		}
		if(a & LOG_STRONG)
			log_write_raw(l_levels[level].target, "<STRONG>");
		if(a & LOG_BLINK)
			log_write_raw(l_levels[level].target, "<BLINK>");
	}
}


static inline void reset_attr(int level)
{
	unsigned a = l_levels[level].attr;
	if(l_targets[l_levels[level].target].flags & LOG_ANSI)
	{
		if(!(a & ANSI_SUPPORTED_FLAGS))
			return;

		log_write_raw(l_levels[level].target, "\033[0m");
	}
	else if(l_targets[l_levels[level].target].flags & LOG_HTML)
	{
		if(a & LOG_BLINK)
			log_write_raw(l_levels[level].target, "</BLINK>");
		if(a & LOG_STRONG)
			log_write_raw(l_levels[level].target, "</STRONG>");
		if(a & LOG_COLORS)
			log_write_raw(l_levels[level].target, "</FONT>");
	}
}


static inline int is_active(int level)
{
	LOG_target *t = &l_targets[l_levels[level].target];
	return (t->use_stream || t->callback);
}


/*--------------------------------------------------------------------
	API entry points
--------------------------------------------------------------------*/

int log_open(void)
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

	log_set_level_target(-1, 0);
	log_set_level_attr(-1, LOG_NOCOLOR);

	start_time = SDL_GetTicks();
	return 0;
}


#define	CHECK_INIT	(l_levels ? 0 : -1)


void log_close(void)
{
	int i;
	if(CHECK_INIT < 0)
		return;
	for(i = 0; i < LOG_TARGETS; ++i)
		check_footer(i);
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
		int (*callback)(int handle, const char *data), int handle)
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


void log_set_level_target(int level, int target)
{
	int i;
	if(CHECK_INIT < 0)
		return;

	for_one_or_all(i, level, LOG_LEVELS)
		l_levels[i].target = target;
}


void log_set_level_attr(int level, unsigned attr)
{
	int i;
	if(CHECK_INIT < 0)
		return;

	for_one_or_all(i, level, LOG_LEVELS)
		l_levels[i].attr = attr;
}


int log_puts(int level, const char *text)
{
	int result, result2;

	if(CHECK_INIT < 0)
		return -1;

	if(level < 0 || level >= LOG_LEVELS)
		return -2;

	if(!is_active(level))
		return 0;

	check_header(l_levels[level].target);
	set_attr(level);
	put_timestamp(level);

	result = log_write(l_levels[level].target, text);
	if(result >= 0)
	{
		result2 = log_write_raw(l_levels[level].target, "\n");
		if(result2 >= 0)
			result += result2;
	}
	reset_attr(level);
	return result;
}


int log_printf(int level, const char *format, ...)
{
	va_list args;
	int result;

	if(CHECK_INIT < 0)
		return -1;

	if(level < 0 || level >= LOG_LEVELS)
		return -2;

	if(!is_active(level))
		return 0;

	check_header(l_levels[level].target);
	set_attr(level);
	put_timestamp(level);

	va_start(args, format);
	result = vsnprintf(l_buffer, LOG_BUFFER-1, format, args);
	va_end(args);
	if(result >= 0)
		result = log_write(l_levels[level].target, l_buffer);
	reset_attr(level);
	return result;
}
