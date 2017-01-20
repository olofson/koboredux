/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2016-2017 David Olofson (Kobo Redux)
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

#include "themeparser.h"

#include "kobolog.h"
#include "kobo.h"

#include <stdlib.h>
#include <string.h>


  /////////////////////////////////////////////////////////////////////////////
 //	Keywords and constants
/////////////////////////////////////////////////////////////////////////////

struct TP_keywords
{
	const char	*kw;
	KOBO_TP_Tokens	token;
	int		value;
};


static TP_keywords tp_keywords[] =
{
	// General
	{ "fallback",		KTK_KW_FALLBACK,	0	},
	{ "path",		KTK_KW_PATH,		0	},
	{ "message",		KTK_KW_MESSAGE,		0	},
	{ "set",		KTK_KW_SET,		0	},

	// Stages, maps, and levels
	{ "stagemessage",	KTK_KW_STAGEMESSAGE,	0	},

	// Graphics
	{ "image",		KTK_KW_IMAGE,		0	},
	{ "sprites",		KTK_KW_SPRITES,		0	},
	{ "sfont",		KTK_KW_SFONT,		0	},
	{ "palette",		KTK_KW_PALETTE,		0	},
	{ "alias",		KTK_KW_ALIAS,		0	},

	// Flag constants
	{ "CLAMP",		KTK_FLAG,	KOBO_CLAMP		},
	{ "CLAMP_OPAQUE",	KTK_FLAG,	KOBO_CLAMP_OPAQUE	},
	{ "WRAP",		KTK_FLAG,	KOBO_WRAP		},
	{ "ABSSCALE",		KTK_FLAG,	KOBO_ABSSCALE		},
	{ "NEAREST",		KTK_FLAG,	KOBO_NEAREST		},
	{ "BILINEAR",		KTK_FLAG,	KOBO_BILINEAR		},
	{ "SCALE2X",		KTK_FLAG,	KOBO_SCALE2X		},
	{ "NOALPHA",		KTK_FLAG,	KOBO_NOALPHA		},
	{ "CENTER",		KTK_FLAG,	KOBO_CENTER		},
	{ "NOBRIGHT",		KTK_FLAG,	KOBO_NOBRIGHT		},
	{ "FALLBACK",		KTK_FLAG,	KOBO_FALLBACK		},
	{ "FUTURE",		KTK_FLAG,	KOBO_FUTURE		},

	// Numeric constants
	{ "RAW",	KTK_NUMBER,	SPINPLANET_DITHER_RAW		},
	{ "NONE",	KTK_NUMBER,	SPINPLANET_DITHER_NONE		},
	{ "RANDOM",	KTK_NUMBER,	SPINPLANET_DITHER_RANDOM	},
	{ "ORDERED",	KTK_NUMBER,	SPINPLANET_DITHER_ORDERED	},
	{ "SKEWED",	KTK_NUMBER,	SPINPLANET_DITHER_SKEWED	},
	{ "NOISE",	KTK_NUMBER,	SPINPLANET_DITHER_NOISE		},
	{ "TEMPORAL2",	KTK_NUMBER,	SPINPLANET_DITHER_TEMPORAL2	},
	{ "TEMPORAL4",	KTK_NUMBER,	SPINPLANET_DITHER_TEMPORAL4	},
	{ "TRUECOLOR",	KTK_NUMBER,	SPINPLANET_DITHER_TRUECOLOR	},

	{ "LOGO_FX_SLIDE",	KTK_NUMBER,	KOBO_LOGO_FX_SLIDE	},
	{ "LOGO_FX_FADE",	KTK_NUMBER,	KOBO_LOGO_FX_FADE	},
	{ "LOGO_FX_ZOOM",	KTK_NUMBER,	KOBO_LOGO_FX_ZOOM	},

	{ NULL, KTK_EOF, 0 }
};


  /////////////////////////////////////////////////////////////////////////////
 //	KOBO_ThemeData
/////////////////////////////////////////////////////////////////////////////


KOBO_ThemeData::KOBO_ThemeData()
{
	next = NULL;
	memset(sizes, 0, sizeof(sizes));
	memset(items, 0, sizeof(items));
	memset(strings, 0, sizeof(strings));
}


KOBO_ThemeData::~KOBO_ThemeData()
{
	for(int i = 0; i < KOBO_D__COUNT; ++i)
	{
		free(items[i]);
		free(strings[i]);
	}
}


bool KOBO_ThemeData::set(KOBO_TD_Items item, unsigned index, double value)
{
	if(index >= sizes[item])
	{
		double *ni = (double *)realloc(items[item],
				(index + 1) * sizeof(double));
		if(!ni)
			return false;
		items[item] = ni;
		for(unsigned i = sizes[item]; i < index; ++i)
			items[item][i] = 0.0f;
		sizes[item] = index + 1;
	}
	items[item][index] = value;
	return true;
}


bool KOBO_ThemeData::set(KOBO_TD_Items item, const char *str)
{
	free(strings[item]);
	if(!str)
	{
		strings[item] = NULL;
		return true;
	}
	strings[item] = strdup(str);
	return (strings[item] != NULL);
}



  /////////////////////////////////////////////////////////////////////////////
 //	KOBO_ThemeParser
/////////////////////////////////////////////////////////////////////////////

KOBO_ThemeParser::KOBO_ThemeParser(KOBO_ThemeData &td)
{
	themedata = &td;
}


const char *KOBO_ThemeParser::get_path(const char *p)
{
	return fmap->get(p, FM_FILE, "GFX>>");
}


const char *KOBO_ThemeParser::fullpath(const char *fp)
{
	// Use absolute paths as is
	if(!fmap->is_relative(fp))
		return get_path(fp);

	// If 'path' is absolute, that overrides 'basepath'. Otherwise, we
	// prepend "basepath/path/" to 'fp'.
	char tmp[KOBO_TP_MAXLEN];
	if(!fmap->is_relative(path))
		snprintf(tmp, sizeof(tmp), "%s/%s", path, fp);
	else if(path[0])
		snprintf(tmp, sizeof(tmp), "%s/%s/%s", basepath, path, fp);
	else
		snprintf(tmp, sizeof(tmp), "%s/%s", basepath, fp);
	return get_path(tmp);
}


void KOBO_ThemeParser::dump_line()
{
	// Figure out and print out where we are
	int line = 1;
	int col = 1;
	int ls = 0;
	for(int i = 0; i < pos; ++i)
		switch(buffer[i])
		{
		  case '\n':
			col = 1;
			++line;
			ls = i + 1;
			break;
		  case 't':
			col = (col + 1) % 8;
			break;
		  default:
			++col;
			break;
		}
	log_printf(ELOG, "[Theme Loader] Line %d, column %d:\n", line, col);

	// Dump the offending line
	int le = ls;
	while(buffer[le] && (buffer[le] != '\n'))
		++le;
	strncpy(sv, buffer + ls, le - ls);
	sv[le - ls] = 0;
	log_printf(ELOG, "[Theme Loader] %s\n", sv);

	// Mark position
	for(int i = 0; sv[i]; ++i)
	{
		switch(sv[i])
		{
		  case '\t':
		  case ' ':
			break;
		  default:;
			sv[i] = ' ';
			break;
		}
		if(ls + i == pos)
		{
			sv[i] = '^';
			sv[i + 1] = 0;
			break;
		}
	}
	log_printf(ELOG, "[Theme Loader] %s\n", sv);
}


void KOBO_ThemeParser::skip_to_eoln()
{
	while(bufget() != '\n')
		;
}


void KOBO_ThemeParser::skip_white()
{
	while(is_white(bufget()))
		;
	bufunget();
}


KOBO_TP_Tokens KOBO_ThemeParser::lex_number()
{
	int p = 0;
	while(1)
	{
		if(p >= KOBO_TP_MAXLEN)
		{
			dump_line();
			log_printf(ELOG, "[Theme Loader] "
					"Too long numeric literal!\n");
			return KTK_ERROR;
		}
		int c = bufget();
		if(is_white(c) || (c == '\n') || !c)
		{
			bufunget();
			break;
		}
		if(is_num(c) || (c == '.') || (c == '-'))
		{
			sv[p++] = c;
			continue;
		}
		dump_line();
		log_printf(ELOG, "[Theme Loader] Unexpected token '%c'\n", c);
		return KTK_ERROR;
	}
	sv[p++] = 0;
	rv = atof(sv);
	return KTK_NUMBER;
}


KOBO_TP_Tokens KOBO_ThemeParser::lex_hexcolor()
{
	int i = 0;
	iv = 0;
	while(1)
	{
		int c = bufget();
		if(is_white(c) || (c == '\n') || !c)
		{
			bufunget();
			break;
		}
		if(is_num(c))
		{
			iv <<= 4;
			iv += c - '0';
			++i;
			continue;
		}
		if((c >= 'a') && (c <= 'f'))
		{
			iv <<= 4;
			iv += c - 'a' + 10;
			++i;
			continue;
		}
		if((c >= 'A') && (c <= 'F'))
		{
			iv <<= 4;
			iv += c - 'A' + 10;
			++i;
			continue;
		}
		dump_line();
		log_printf(ELOG, "[Theme Loader] Unexpected token '%c'\n", c);
		return KTK_ERROR;
	}
	switch(i)
	{
	  case 3:
	  case 4:
	  {
		// #RGB, #ARGB - Convert from 4 bit/channel to 8 bit/channel.
		int a = ((iv >> 12) & 0xf) * 0x11;
		int r = ((iv >> 8) & 0xf) * 0x11;
		int g = ((iv >> 4) & 0xf) * 0x11;
		int b = (iv & 0xf) * 0x11;
		iv = (a << 24) | (r << 16) | (g << 8) | b;
		break;
	  }
	  case 6:
	  case 8:
		// #RRGGBB, #AARRGGBB - No conversion needed!
		break;
	  default:
		dump_line();
		log_printf(ELOG, "[Theme Loader] Invalid hex color code! "
				"Expected 3, 4, 6, or 8 digits.\n");
		return KTK_ERROR;
	}
	return KTK_HEXCOLOR;
}


KOBO_TP_Tokens KOBO_ThemeParser::lex_string()
{
	int p = 0;
	while(1)
	{
		if(p >= KOBO_TP_MAXLEN)
		{
			dump_line();
			log_printf(ELOG, "[Theme Loader] "
					"Too long string literal!\n");
			return KTK_ERROR;
		}
		int c = bufget();
		switch(c)
		{
		  case '"':
			sv[p++] = 0;
			return KTK_STRING;
		  case '\\':
			switch((c = bufget()))
			{
			  case 'n':
				sv[p++] = '\n';
				break;
			  case 'r':
				sv[p++] = '\r';
				break;
			  case 't':
				sv[p++] = '\t';
				break;
			  case '\n':
				// Line continuation! Just skip and read on.
				break;
			  case '\\':
			  default:
				sv[p++] = c;
				break;
			}
			break;
		  case '\n':
			dump_line();
			log_printf(ELOG, "[Theme Loader] Unexpected newline "
					"inside string literal!\n");
			return KTK_ERROR;
		  default:
			sv[p++] = c;
			break;
		}
	}
}


KOBO_TP_Tokens KOBO_ThemeParser::lex_symbol()
{
	int p = 0;
	while(1)
	{
		if(p >= KOBO_TP_MAXLEN)
		{
			dump_line();
			log_printf(ELOG, "[Theme Loader] "
					"Too long symbol name!\n");
			return KTK_ERROR;
		}
		int c = bufget();
		if(is_white(c) || (c == '\n') || !c)
		{
			bufunget();
			break;
		}
		if(is_alpha(c) || is_num(c) || (c == '_'))
		{
			sv[p++] = c;
			continue;
		}
		dump_line();
		if(c >= ' ')
			log_printf(ELOG, "[Theme Loader] Unexpected character"
					" '%c'\n", c);
		else
			log_printf(ELOG, "[Theme Loader] Unexpected character"
					" 0x%2.2x\n", c);
		return KTK_ERROR;
	}
	sv[p++] = 0;

	// Keywords and flags
	for(int i = 0; tp_keywords[i].kw; ++i)
		if(strcmp(tp_keywords[i].kw, sv) == 0)
		{
			rv = iv = tp_keywords[i].value;
			return tp_keywords[i].token;
		}

	// Bank indexes
	for(int i = 0; i < B__COUNT; ++i)
		if(strcmp(kobo_gfxbanknames[i], sv) == 0)
		{
			iv = i;
			return KTK_BANK;
		}

	// Palettes
	for(int i = 0; i < KOBO_P__COUNT; ++i)
		if(strcmp(kobo_palettenames[i], sv) == 0)
		{
			iv = i;
			return KTK_PALETTE;
		}

	// ThemeData items (values/arrays)
	for(int i = 0; i < KOBO_D__COUNT; ++i)
		if(strcmp(kobo_datanames[i], sv) == 0)
		{
			iv = i;
			return KTK_THEMEDATA;
		}

	char *s = strdup(sv);
	dump_line();
	log_printf(ELOG, "[Theme Loader] Unknown symbol '%s'!\n", s);
	free(s);
	return KTK_ERROR;
}


KOBO_TP_Tokens KOBO_ThemeParser::lex()
{
	unlex_pos = pos;
	skip_white();
	switch(int c = bufget())
	{
	  case 0:
		return KTK_EOF;
	  case '\n':
		return KTK_EOLN;
	  case '#':
		return lex_hexcolor();
	  case '/':
		c = bufget(); 
		if(c != '/')
		{
			log_printf(ELOG, "[Theme Loader] Expected \"//\" - "
					" not \"/%c\"\n", c);
			return KTK_ERROR;
		}
		skip_to_eoln();
		return KTK_EOLN;
	  case '.':
	  case '-':
		bufunget();
		return lex_number();
	  case '"':
		return lex_string();
	  default:
		bufunget();
		if(is_num(c))
			return lex_number();
		else
			return lex_symbol();
	}
}


void KOBO_ThemeParser::unlex()
{
	if(unlex_pos >= 0)
		pos = unlex_pos;
	else
	{
		dump_line();
		log_printf(ELOG, "[Theme Loader] INTERNAL ERROR: unlex() with"
				" no previous lex()!\n");
	}
}


bool KOBO_ThemeParser::expect(KOBO_TP_Tokens token)
{
	KOBO_TP_Tokens tk = lex();
	if(tk != token)
	{
		dump_line();
		log_printf(ELOG, "[Theme Loader] Expected %s token; not %s!\n",
				token_name(token), token_name(tk));
		return false;
	}
	return true;
}


bool KOBO_ThemeParser::read_flags(int *flags, int allowed)
{
	KOBO_TP_Tokens tk;
	while((tk = lex()) == KTK_FLAG)
	{
		if(!(iv & allowed))
		{
			dump_line();
			log_printf(ELOG, "[Theme Loader] Flag not supported in"
					" in this context!\n");
			return false;
		}
		*flags |= iv;
	}
	switch(tk)
	{
	  case KTK_EOLN:
	  case KTK_EOF:
		return true;
	  default:
		dump_line();
		log_printf(ELOG, "[Theme Loader] Expected FLAG, EOF, or EOLN; "
				"not %s!\n", token_name(tk));
		return false;
	}
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_message()
{
	if(!expect(KTK_STRING))
		return KTK_ERROR;
	wdash->doing(sv);
	log_printf(ULOG, "[Theme Loader] === %s\n", sv);
	return KTK_KW_MESSAGE;
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_stagemessage()
{
	if(!expect(KTK_NUMBER))
		return KTK_ERROR;
	int stage = (int)rv;
	if(!expect(KTK_STRING))
		return KTK_ERROR;
	char *header = strdup(sv);
	if(!expect(KTK_STRING))
		return KTK_ERROR;
	if(prefs->debug)
		log_printf(ULOG, "[Theme Loader] === stagemessage "
				"%d \"%s\" \"%s\"\n", stage, header, sv);
	km.set_stagemessage(stage, header, sv);
	free(header);
	return KTK_KW_STAGEMESSAGE;
}


void KOBO_ThemeParser::apply_flags(int flags, double scale)
{
	// Set scale (filter) mode and clamping
	int clamping;
	gfx_scalemodes_t sm;
	if(flags & KOBO_CLAMP)
	{
		gengine->clampcolor(0, 0, 0, 0);
		clamping = 1;
	}
	else if(flags & KOBO_CLAMP_OPAQUE)
	{
		gengine->clampcolor(0, 0, 0, 255);
		clamping = 1;
	}
	else if(flags & KOBO_WRAP)
		clamping = 3;
	else
		clamping = 0;
	if(flags & KOBO_NEAREST)
		sm = GFX_SCALE_NEAREST;
	else if(flags & KOBO_BILINEAR)
		sm = GFX_SCALE_BILINEAR;
	else if(flags & KOBO_SCALE2X)
		sm = GFX_SCALE_SCALE2X;
	else
		sm = (gfx_scalemodes_t) prefs->scalemode;
	gengine->scalemode(sm, clamping);

	// Disable brightness filter?
	if(flags & KOBO_NOBRIGHT)
		gengine->brightness(1.0f, 1.0f);
	else
		gengine->brightness(0.01f * prefs->brightness,
				0.01f * prefs->contrast);

	// Alpha channels
	if(flags & KOBO_NOALPHA || !prefs->alpha)
		gengine->noalpha(NOALPHA_THRESHOLD);
	else
		gengine->noalpha(0);

	// Source image scale factor
	if(flags & KOBO_ABSSCALE)
		gengine->absolute_scale(scale, scale);
	else if(scale == 0.0f)
		gengine->absolute_scale(1.0f, 1.0f);
	else
		gengine->source_scale(scale, scale);
}


void KOBO_ThemeParser::warn_bank_used(int bank)
{
	if(s_bank_t *b = s_get_bank_raw(gfxengine->get_gfx(), bank))
	{
		if(b->userflags & KOBO_FALLBACK)
			return;
		dump_line();
		log_printf(WLOG, "[Theme Loader] WARNING: Bank %s(%d) already"
				" in use!\n", kobo_gfxbanknames[bank], bank);
	}
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_image()
{
	if(!expect(KTK_BANK))
		return KTK_ERROR;
	int bank = iv;
	warn_bank_used(bank);

	if(!expect(KTK_STRING))
		return KTK_ERROR;
	const char *fn;
	if(!(fn = fullpath(sv)))
	{
		char *s = strdup(sv);
		dump_line();
		log_printf(ELOG, "[Theme Loader] Couldn't find image "
				"\"%s\"!\n", s);
		free(s);
		return KTK_ERROR;
	}

	double scale = 0.0f;
	if(lex() == KTK_NUMBER)
		scale = rv;
	else
		unlex();

	int flags = default_flags;
	if(!read_flags(&flags, KOBO_CLAMP | KOBO_CLAMP_OPAQUE | KOBO_WRAP |
			KOBO_NEAREST | KOBO_BILINEAR | KOBO_SCALE2X |
			KOBO_NOBRIGHT | KOBO_NOALPHA | KOBO_ABSSCALE |
			KOBO_CENTER | KOBO_FALLBACK))
		return KTK_ERROR;

	log_printf(ULOG, "[Theme Loader] image %s \"%s\" %f 0x%4.4x\n",
			kobo_gfxbanknames[bank], fn, scale, flags);

	// Set up graphics loading pipeline
	apply_flags(flags, scale);

	// Load!
	if(gengine->loadimage(bank, fn) < 0)
	{
		dump_line();
		log_printf(ELOG, "[Theme Loader] Couldn't load image "
				"\"%s\"!\n", fn);
		return KTK_ERROR;
	}

	// Set draw (real time) scale
	if(!scale)
		gengine->draw_scale(bank, gengine->xscale(),
				gengine->yscale());

	s_bank_t *b = s_get_bank(gfxengine->get_gfx(), bank);
	if(!b)
	{
		dump_line();
		log_printf(ELOG, "[Theme Loader] INTERNAL ERROR: Could not get"
				" bank \"%s\", which should exist!\n",
				kobo_gfxbanknames[bank]);
		return KTK_ERROR;
	}

	b->userflags = flags;

	// Hotspot
	if(flags & KOBO_CENTER)
		gengine->set_hotspot(bank, -1, b->w / 2, b->h / 2);

	return KTK_KW_IMAGE;
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_sprites()
{
	if(!expect(KTK_BANK))
		return KTK_ERROR;
	int bank = iv;
	warn_bank_used(bank);

	if(!expect(KTK_STRING))
		return KTK_ERROR;
	const char *fn;
	if(!(fn = fullpath(sv)))
	{
		char *s = strdup(sv);
		dump_line();
		log_printf(ELOG, "[Theme Loader] Couldn't find sprite sheet "
				"\"%s\"!\n", s);
		free(s);
		return KTK_ERROR;
	}

	if(!expect(KTK_NUMBER))
		return KTK_ERROR;
	int fw = floor(rv);
	if(!expect(KTK_NUMBER))
		return KTK_ERROR;
	int fh = floor(rv);

	double scale = 0.0f;
	if(lex() == KTK_NUMBER)
		scale = rv;
	else
		unlex();

	int flags = default_flags;
	if(!read_flags(&flags, KOBO_CLAMP | KOBO_CLAMP_OPAQUE | KOBO_WRAP |
			KOBO_NEAREST | KOBO_BILINEAR | KOBO_SCALE2X |
			KOBO_NOBRIGHT | KOBO_NOALPHA | KOBO_ABSSCALE |
			KOBO_CENTER | KOBO_FALLBACK))
		return KTK_ERROR;

	log_printf(ULOG, "[Theme Loader] sprites %s \"%s\" %d %d %f 0x%4.4x\n",
			kobo_gfxbanknames[bank], fn, fw, fh, scale, flags);

	// Set up graphics loading pipeline
	apply_flags(flags, scale);

	// Load!
	if(gengine->loadtiles(bank, fw, fh, fn) < 0)
	{
		dump_line();
		log_printf(ELOG, "[Theme Loader] Couldn't load sprite sheet "
				"\"%s\"!\n", fn);
		return KTK_ERROR;
	}

	// Set draw (real time) scale
	if(!scale)
		gengine->draw_scale(bank, gengine->xscale(),
				gengine->yscale());

	s_bank_t *b = s_get_bank(gfxengine->get_gfx(), bank);
	if(!b)
	{
		dump_line();
		log_printf(ELOG, "[Theme Loader] INTERNAL ERROR: Could not get"
				" bank \"%s\", which should exist!\n",
				kobo_gfxbanknames[bank]);
		return KTK_ERROR;
	}

	b->userflags = flags;

	// Hotspot
	if(flags & KOBO_CENTER)
		gengine->set_hotspot(bank, -1, b->w / 2, b->h / 2);

	return KTK_KW_SPRITES;
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_sfont()
{
	if(!expect(KTK_BANK))
		return KTK_ERROR;
	int bank = iv;
	warn_bank_used(bank);

	if(!expect(KTK_STRING))
		return KTK_ERROR;
	const char *fn;
	if(!(fn = fullpath(sv)))
	{
		char *s = strdup(sv);
		dump_line();
		log_printf(ELOG, "[Theme Loader] Couldn't find SFont "
				"\"%s\"!\n", s);
		free(s);
		return KTK_ERROR;
	}

	double scale = 0.0f;
	if(lex() == KTK_NUMBER)
		scale = rv;
	else
		unlex();

	int flags = default_flags;
	if(!read_flags(&flags, KOBO_CLAMP | KOBO_CLAMP_OPAQUE | KOBO_WRAP |
			KOBO_NEAREST | KOBO_BILINEAR | KOBO_SCALE2X |
			KOBO_NOBRIGHT | KOBO_NOALPHA | KOBO_ABSSCALE |
			KOBO_FALLBACK))
		return KTK_ERROR;

	log_printf(ULOG, "[Theme Loader] sfont %s \"%s\" %f 0x%4.4x\n",
			kobo_gfxbanknames[bank], fn, scale, flags);

	// Set up graphics loading pipeline
	apply_flags(flags, scale);

	// Load!
	if(gengine->loadfont(bank, fn) < 0)
	{
		dump_line();
		log_printf(ELOG, "[Theme Loader] Couldn't load SFont "
				"\"%s\"!\n", fn);
		return KTK_ERROR;
	}

	// Set draw (real time) scale
	if(!scale)
		gengine->draw_scale(bank, gengine->xscale(),
				gengine->yscale());

	s_bank_t *b = s_get_bank(gfxengine->get_gfx(), bank);
	if(!b)
	{
		dump_line();
		log_printf(ELOG, "[Theme Loader] INTERNAL ERROR: Could not get"
				" bank \"%s\", which should exist!\n",
				kobo_gfxbanknames[bank]);
		return KTK_ERROR;
	}

	b->userflags = flags;

	return KTK_KW_SFONT;
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_palette_gpl(int pal)
{
	const char *fn;
	if(!(fn = fullpath(sv)))
	{
		char *s = strdup(sv);
		dump_line();
		log_printf(ELOG, "[Theme Loader] Couldn't find palette "
				"\"%s\"!\n", s);
		free(s);
		return KTK_ERROR;
	}

	log_printf(ULOG, "[Theme Loader] palette %d \"%s\"\n", pal, fn);

	if(!gengine->load_palette(pal, fn))
	{
		char *s = strdup(sv);
		dump_line();
		log_printf(ELOG, "[Theme Loader] Couldn't load palette "
				"\"%s\"!\n", s);
		free(s);
		return KTK_ERROR;
	}

	return KTK_KW_PALETTE;
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_palette_hex(int pal)
{
	log_printf(ULOG, "[Theme Loader] palette %d #...\n", pal);
	for(int i = 0; ; ++i)
	{
		KOBO_TP_Tokens tk = lex();
		switch(tk)
		{
		  case KTK_EOLN:
		  case KTK_EOF:
			return KTK_KW_PALETTE;
		  case KTK_HEXCOLOR:
			gengine->set_palette(pal, i, iv);
			break;
		  default:
			dump_line();
			log_printf(ELOG, "[Theme Loader] Expected hex color "
					" code - not '%s'!\n", token_name(tk));
			return KTK_ERROR;
		}
	}
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_palette_index(int pal, int source)
{
	log_printf(ULOG, "[Theme Loader] palette %d %d ...\n", pal, source);
	for(int i = 0; ; ++i)
	{
		KOBO_TP_Tokens tk = lex();
		switch(tk)
		{
		  case KTK_EOLN:
		  case KTK_EOF:
			if(i)
				return KTK_KW_PALETTE;
			// No entries! Make a full clone of 'source'.
			for(i = gengine->palette_size(source) - 1; i >= 0; --i)
				gengine->set_palette(pal, i, gengine->palette(
						source, i));
			return KTK_KW_PALETTE;
		  case KTK_NUMBER:
			gengine->set_palette(pal, i, gengine->palette(source,
					(uint32_t)rv));
			break;
		  default:
			dump_line();
			log_printf(ELOG, "[Theme Loader] Expected color index "
					"- not '%s'!\n", token_name(tk));
			return KTK_ERROR;
		}
	}
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_palette()
{
	if(!expect(KTK_PALETTE))
		return KTK_ERROR;
	int pal = iv;

	switch(KOBO_TP_Tokens tk = lex())
	{
	  case KTK_STRING:
		return handle_palette_gpl(pal);
	  case KTK_HEXCOLOR:
		unlex();
		return handle_palette_hex(pal);
	  case KTK_PALETTE:
		return handle_palette_index(pal, iv);
	  default:
		dump_line();
		log_printf(ELOG, "[Theme Loader] Expected palette file path, "
				"hex color codes, or another palette - not "
				"token '%s'!\n", token_name(tk));
		return KTK_ERROR;
	}
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_fallback()
{
	if(!expect(KTK_STRING))
		return KTK_ERROR;
	char *s = strdup(sv);

	log_printf(ULOG, "[Theme Loader] fallback \"%s\"\n", s);

	KOBO_ThemeParser tp(*themedata);
	if(!tp.load(s, KOBO_FALLBACK))
	{
		dump_line();
		log_printf(WLOG, "[Theme Loader] Couldn't load fallback "
				"graphics theme \"%s\"!\n", s);
	}

	free(s);
	return KTK_KW_FALLBACK;
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_path()
{
	if(!expect(KTK_STRING))
		return KTK_ERROR;
	strncpy(path, sv, sizeof(path));
	log_printf(ULOG, "[Theme Loader] path \"%s\"\n", path);
	return KTK_KW_PATH;
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_alias()
{
	if(!expect(KTK_BANK))
		return KTK_ERROR;
	int bank = iv;
	warn_bank_used(bank);

	if(!expect(KTK_BANK))
		return KTK_ERROR;
	int orig = iv;

	int flags = default_flags;
	if(!read_flags(&flags, KOBO_FALLBACK | KOBO_FUTURE))
		return KTK_ERROR;

	log_printf(ULOG, "[Theme Loader] alias %s %s 0x%x\n",
			kobo_gfxbanknames[bank], kobo_gfxbanknames[orig],
			flags);

	if(!(flags & KOBO_FUTURE) && !s_get_bank(gengine->get_gfx(), orig))
	{
		dump_line();
		log_printf(WLOG, "[Theme Loader] Failed to alias %s to "
				"empty bank %s! (Intentional? Use FUTURE!)\n",
				kobo_gfxbanknames[bank],
				kobo_gfxbanknames[orig]);
	}

	s_bank_t *b = gengine->alias_bank(bank, orig);
	if(!b)
	{
		dump_line();
		log_printf(WLOG, "[Theme Loader] Couldn't create alias %s for "
				"%s!\n", kobo_gfxbanknames[bank],
				kobo_gfxbanknames[orig]);
	}
	b->userflags = flags;

	return KTK_KW_PATH;
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_set()
{
	if(!expect(KTK_THEMEDATA))
		return KTK_ERROR;
	int td = iv;
	if(!(default_flags & KOBO_SILENT))
		log_printf(ULOG, "[Theme Loader] set %s ...\n",
				kobo_datanames[td]);
	for(int i = 0; ; ++i)
	{
		KOBO_TP_Tokens tk = lex();
		switch(tk)
		{
		  case KTK_EOLN:
		  case KTK_EOF:
			if(!i)
			{
				dump_line();
				log_printf(ELOG, "[Theme Loader] Expected at"
						" least one value!\n");
				return KTK_ERROR;
			}
			return KTK_KW_SET;
		  case KTK_NUMBER:
			themedata->set((KOBO_TD_Items)td, i, rv);
			break;
		  case KTK_STRING:
			if(i != 0)
			{
				log_printf(ELOG, "[Theme Loader] String arrays"
						" not supported!\n");
				return KTK_ERROR;
			}
			themedata->set((KOBO_TD_Items)td, sv);
			break;
		  case KTK_BANK:
		  case KTK_PALETTE:
		  case KTK_FLAG:
		  case KTK_HEXCOLOR:
		  case KTK_THEMEDATA:
			themedata->set((KOBO_TD_Items)td, i, iv);
			break;
		  default:
			dump_line();
			log_printf(ELOG, "[Theme Loader] Expected value "
					"- not '%s'!\n", token_name(tk));
			return KTK_ERROR;
		}
	}
	return KTK_KW_SET;
}


KOBO_TP_Tokens KOBO_ThemeParser::parse_line()
{
	switch(KOBO_TP_Tokens tk = lex())
	{
	  case KTK_EOF:
	  case KTK_ERROR:
	  case KTK_EOLN:
		return tk;
	  case KTK_KW_MESSAGE:
		return handle_message();
	  case KTK_KW_STAGEMESSAGE:
		return handle_stagemessage();
	  case KTK_KW_IMAGE:
		return handle_image();
	  case KTK_KW_SPRITES:
		return handle_sprites();
	  case KTK_KW_SFONT:
		return handle_sfont();
	  case KTK_KW_PALETTE:
		return handle_palette();
	  case KTK_KW_FALLBACK:
		return handle_fallback();
	  case KTK_KW_PATH:
		return handle_path();
	  case KTK_KW_ALIAS:
		return handle_alias();
	  case KTK_KW_SET:
		return handle_set();
	  default:
		dump_line();
		log_printf(ELOG, "[Theme Loader] Unexpected token '%s'!\n",
				token_name(tk));
		return KTK_ERROR;
	}
}


void KOBO_ThemeParser::init(int flags)
{
	basepath[0] = 0;
	path[0] = 0;
	pos = 0;
	sv[0] = 0;
	rv = 0.0f;
	unlex_pos = -1;
	default_flags = flags;
	silent = false;
}


KOBO_TP_Tokens KOBO_ThemeParser::parse_theme(const char *scriptpath, int flags)
{
	if(!(flags & KOBO_SILENT))
		log_printf(ULOG, "[Theme Loader] Loading \"%s\"...\n",
				scriptpath);

	init(flags);

	// Grab base path for relative file paths
	strncpy(basepath, scriptpath, sizeof(basepath));
	char *s = strrchr(basepath, '/');
	if(s)
		*s = 0;
	if(!(flags & KOBO_SILENT))
		log_printf(ULOG, "[Theme Loader] Base path: \"%s\"\n",
				basepath);

	// See if we can actually find this theme...
	char *sp = (char *)get_path(scriptpath);
	if(!sp)
	{
		log_printf(ELOG, "[Theme Loader] Couldn't find \"%s\"!\n",
				scriptpath);
		return KTK_ERROR;
	}
	if(!(flags & KOBO_SILENT))
		log_printf(ULOG, "[Theme Loader] Theme file: \"%s\"\n", sp);

	// Because this might be a temporary string that may be clobbered by
	// further extensive use of get_path()...
	sp = strdup(sp);

	// Load theme file
	FILE *f = fopen(sp, "r");
	if(!f)
	{
		log_printf(ELOG, "[Theme Loader] Couldn't open \"%s\"!\n", sp);
		free(sp);
		return KTK_ERROR;
	}

	fseek(f, 0, SEEK_END);
	bufsize = ftell(f);
	buffer = (char *)malloc(bufsize);
	if(!buffer)
	{
		log_printf(ELOG, "[Theme Loader] OOM loading \"%s\"!\n", sp);
		fclose(f);
		free(sp);
		return KTK_ERROR;
	}
	fseek(f, 0, SEEK_SET);
	size_t sz = fread((char *)buffer, bufsize, 1, f);
	if(sz < 0)
	{
		log_printf(ELOG, "[Theme Loader] Couldn't read \"%s\"!\n", sp);
		fclose(f);
		free((char *)buffer);
		free(sp);
		return KTK_ERROR;
	}
	fclose(f);

	// Parse theme file
	KOBO_TP_Tokens res;
	while((res = parse_line()) > KTK_EOF)
		if(wdash)
			wdash->progress((float)pos / bufsize);

	free((char *)buffer);
	free(sp);
	return KTK_EOF;
}


bool KOBO_ThemeParser::parse(const char *theme, int flags)
{
	char buf[128];
	snprintf(buf, sizeof(buf), "<string %p>", theme);
	themedata->set(KOBO_D_THEMEPATH, buf);
	themedata->set(KOBO_D_THEMELABEL, "<none>");
	init(flags);
	buffer = theme;
	bufsize = strlen(theme);
	log_printf(ULOG, "[Theme Loader] Parsing string \"%.40s\"...\n",
			theme);
	KOBO_TP_Tokens res;
	while((res = parse_line()) > KTK_EOF)
		wdash->progress((float)pos / bufsize);
	if(res != KTK_ERROR)
		log_printf(ULOG, "[Theme Loader] String \"%.40s\" parsed!\n",
				theme);
	else
		log_printf(ULOG, "[Theme Loader] String parse of \"%.40s\" "
				"failed!\n", theme);
	return (res != KTK_ERROR);
}


bool KOBO_ThemeParser::load(const char *themepath, int flags)
{
	char p[KOBO_TP_MAXLEN];
	char *tp = strdup(themepath);
	themedata->set(KOBO_D_THEMEPATH, tp);
	themedata->set(KOBO_D_THEMELABEL, fmap->get_name(tp));
	snprintf(p, sizeof(p), "%s/main.theme", tp);
	KOBO_TP_Tokens res = parse_theme(p, flags);
	if(res != KTK_ERROR)
		log_printf(ULOG, "[Theme Loader] \"%s\" loaded!\n", p);
	else
		log_printf(ULOG, "[Theme Loader] \"%s\" failed to load!\n", p);
	free(tp);
	return (res != KTK_ERROR);
}


bool KOBO_ThemeParser::examine(const char *themepath, int flags)
{
	char p[KOBO_TP_MAXLEN];
	char *tp = strdup(themepath);
	themedata->set(KOBO_D_THEMEPATH, tp);
	themedata->set(KOBO_D_THEMELABEL, fmap->get_name(tp));
	snprintf(p, sizeof(p), "%s/header.theme", tp);
	KOBO_TP_Tokens res = parse_theme(p, flags | KOBO_SILENT);
	free(tp);
	return (res != KTK_ERROR);
}
