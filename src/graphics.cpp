/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2016 David Olofson (Kobo Redux)
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

#include "graphics.h"
#include "kobolog.h"
#include "kobo.h"

#include <stdlib.h>
#include <string.h>


#define	KOBO_DEFS(x)	"B_" #x,
const char *kobo_gfxbanknames[] =
{
	KOBO_ALLGFXBANKS
};
#undef	KOBO_DEFS



KOBO_ThemeParser::KOBO_ThemeParser()
{
}


const char *KOBO_ThemeParser::fullpath(const char *fp)
{
	if((fp[0] == '~') || (fp[0] == '/') || strstr(fp, FM_DEREF_TOKEN))
	{
		// Absolute or special path!
		return fmap->get(fp);
	}
	snprintf(tmp, sizeof(tmp), "%s/%s", basepath, fp);
	return fmap->get(tmp);
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
		if(is_white(c) || (c == '\n'))
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
		if(c == '"')
			break;
		else if(c == '\n')
		{
			dump_line();
			log_printf(ELOG, "[Theme Loader] Unexpected newline "
					"inside string literal!\n");
			return KTK_ERROR;
		}
		sv[p++] = c;
	}
	sv[p++] = 0;
	return KTK_STRING;
}


struct TP_keywords
{
	const char	*kw;
	KOBO_TP_Tokens	token;
	int		value;
};


static TP_keywords tp_keywords[] =
{
	{ "message",	KTK_KW_MESSAGE,		0	},
	{ "image",	KTK_KW_IMAGE,		0	},
	{ "sprites",	KTK_KW_SPRITES,		0	},
	{ "sfont",	KTK_KW_SFONT,		0	},
	{ "palette",	KTK_KW_PALETTE,		0	},
	{ "fallback",	KTK_KW_FALLBACK,	0	},

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

	{ NULL, KTK_EOF, 0 }
};


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
		if(is_white(c) || (c == '\n'))
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
			iv = tp_keywords[i].value;
			return tp_keywords[i].token;
		}

	// Bank indexes
	for(int i = 0; i < B__COUNT; ++i)
		if(strcmp(kobo_gfxbanknames[i], sv) == 0)
		{
			iv = i;
			return KTK_BANK;
		}

	dump_line();
	log_printf(ELOG, "[Theme Loader] Unknown symbol '%s'!\n", sv);
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
		log_printf(ELOG, "[Theme Loader] INTERNAL ERROR: unlex() with"
				" no previous lex()!\n");
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
	wdash->progress((float)pos / bufsize);
	log_printf(ULOG, "[Theme Loader] === %s\n", sv);
	return KTK_KW_MESSAGE;
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
	if(s_get_bank(gfxengine->get_gfx(), bank))
	{
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
		log_printf(ELOG, "[Theme Loader] Couldn't find image "
				"\"%s\"!\n", sv);
		return KTK_ERROR;
	}

	double scale = 0.0f;
	if(lex() == KTK_NUMBER)
		scale = rv;
	else
		unlex();

	int flags = 0;
	if(!read_flags(&flags, KOBO_CLAMP | KOBO_CLAMP_OPAQUE | KOBO_WRAP |
			KOBO_NEAREST | KOBO_BILINEAR | KOBO_SCALE2X |
			KOBO_NOBRIGHT | KOBO_NOALPHA | KOBO_ABSSCALE |
			KOBO_CENTER))
		return KTK_ERROR;

	log_printf(ULOG, "[Theme Loader] image %s \"%s\" %f 0x%4.4x\n",
			kobo_gfxbanknames[bank], fn, scale, flags);

	// Set up graphics loading pipeline
	apply_flags(flags, scale);

	// Load!
	if(gengine->loadimage(bank, fn) < 0)
	{
		log_printf(ELOG, "[Theme Loader] Couldn't load image "
				"\"%s\"!\n", fn);
		return KTK_ERROR;
	}

	// Set draw (real time) scale
	if(!scale)
		gengine->draw_scale(bank, gengine->xscale(),
				gengine->yscale());

	// Hotspot
	if(flags & KOBO_CENTER)
	{
		s_bank_t *b = s_get_bank(gfxengine->get_gfx(), bank);
		if(b)
			gengine->set_hotspot(bank, -1, b->w / 2, b->h / 2);
	}

	wdash->progress((float)pos / bufsize);
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
		log_printf(ELOG, "[Theme Loader] Couldn't find sprites "
				"\"%s\"!\n", sv);
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

	int flags = 0;
	if(!read_flags(&flags, KOBO_CLAMP | KOBO_CLAMP_OPAQUE | KOBO_WRAP |
			KOBO_NEAREST | KOBO_BILINEAR | KOBO_SCALE2X |
			KOBO_NOBRIGHT | KOBO_NOALPHA | KOBO_ABSSCALE |
			KOBO_CENTER))
		return KTK_ERROR;

	log_printf(ULOG, "[Theme Loader] sprites %s \"%s\" %d %d %f 0x%4.4x\n",
			kobo_gfxbanknames[bank], fn, fw, fh, scale, flags);

	// Set up graphics loading pipeline
	apply_flags(flags, scale);

	// Load!
	if(gengine->loadtiles(bank, fw, fh, fn) < 0)
	{
		log_printf(ELOG, "[Theme Loader] Couldn't load sprites "
				"\"%s\"!\n", fn);
		return KTK_ERROR;
	}

	// Set draw (real time) scale
	if(!scale)
		gengine->draw_scale(bank, gengine->xscale(),
				gengine->yscale());

	// Hotspot
	if(flags & KOBO_CENTER)
	{
		s_bank_t *b = s_get_bank(gfxengine->get_gfx(), bank);
		if(b)
			gengine->set_hotspot(bank, -1, b->w / 2, b->h / 2);
	}

	wdash->progress((float)pos / bufsize);
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
		log_printf(ELOG, "[Theme Loader] Couldn't find SFont "
				"\"%s\"!\n", sv);
		return KTK_ERROR;
	}

	double scale = 0.0f;
	if(lex() == KTK_NUMBER)
		scale = rv;
	else
		unlex();

	int flags = 0;
	if(!read_flags(&flags, KOBO_CLAMP | KOBO_CLAMP_OPAQUE | KOBO_WRAP |
			KOBO_NEAREST | KOBO_BILINEAR | KOBO_SCALE2X |
			KOBO_NOBRIGHT | KOBO_NOALPHA | KOBO_ABSSCALE))
		return KTK_ERROR;

	log_printf(ULOG, "[Theme Loader] sfont %s \"%s\" %f 0x%4.4x\n",
			kobo_gfxbanknames[bank], fn, scale, flags);

	// Set up graphics loading pipeline
	apply_flags(flags, scale);

	// Load!
	if(gengine->loadfont(bank, fn, scale) < 0)
	{
		log_printf(ELOG, "[Theme Loader] Couldn't load SFont "
				"\"%s\"!\n", fn);
		return KTK_ERROR;
	}

	// Set draw (real time) scale
	if(!scale)
		gengine->draw_scale(bank, gengine->xscale(),
				gengine->yscale());

	wdash->progress((float)pos / bufsize);
	return KTK_KW_SFONT;
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_palette()
{
	if(!expect(KTK_STRING))
		return KTK_ERROR;
	const char *fn;
	if(!(fn = fullpath(sv)))
	{
		log_printf(ELOG, "[Theme Loader] Couldn't find palette "
				"\"%s\"!\n", sv);
		return KTK_ERROR;
	}

	log_printf(ULOG, "[Theme Loader] palette \"%s\"\n", fn);

	if(!gengine->load_palette(fn))
	{
		log_printf(ELOG, "[Theme Loader] Couldn't load palette "
				"\"%s\"!\n", sv);
		return KTK_ERROR;
	}

	wdash->progress((float)pos / bufsize);
	return KTK_KW_PALETTE;
}


KOBO_TP_Tokens KOBO_ThemeParser::handle_fallback()
{
	if(!expect(KTK_STRING))
		return KTK_ERROR;
	const char *fn;
	if(!(fn = fullpath(sv)))
	{
		log_printf(WLOG, "[Theme Loader] Couldn't find fallback "
				"graphics theme \"%s\"!\n", sv);
		return KTK_EOLN;
	}

	log_printf(ULOG, "[Theme Loader] fallback \"%s\"\n", fn);

	KOBO_ThemeParser tp;
	if(!tp.load_theme(fn))
		log_printf(WLOG, "[Theme Loader] Couldn't load fallback "
				"graphics theme \"%s\"!\n", sv);

	wdash->progress((float)pos / bufsize);
	return KTK_KW_FALLBACK;
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
	  default:
		dump_line();
		log_printf(ELOG, "[Theme Loader] Unexpected token '%s'!\n",
				token_name(tk));
		return KTK_ERROR;
	}
}


bool KOBO_ThemeParser::load_theme(const char *path)
{
	// Because 'path' is sometimes a temporary string may be clobbered by
	// further extensive use of fmap->get()...
	char *p = strdup(path);

	pos = 0;
	sv[0] = 0;
	rv = 0.0f;
	unlex_pos = -1;

	log_printf(ULOG, "[Theme Loader] Loading \"%s\"...\n", path);

	// Load theme file
	FILE *f = fopen(path, "r");
	if(!f)
	{
		log_printf(ELOG, "[Theme Loader] Could not open \"%s\"!\n",
				path);
		return false;
	}

	fseek(f, 0, SEEK_END);
	bufsize = ftell(f);
	buffer = (char *)malloc(bufsize);
	if(!buffer)
	{
		log_printf(ELOG, "[Theme Loader] OOM loading \"%s\"!\n",
				path);
		fclose(f);
		return false;
	}
	fseek(f, 0, SEEK_SET);
	size_t sz = fread(buffer, bufsize, 1, f);
	if(sz < 0)
	{
		log_printf(ELOG, "[Theme Loader] Could not read \"%s\"!\n",
				path);
		fclose(f);
		free(buffer);
		return false;
	}
	fclose(f);

	// Grab base path for relative file paths
	strncpy(basepath, path, sizeof(basepath));
	char *s = strrchr(basepath, '/');
	if(!s)
		s = strrchr(basepath, '\\');
	if(s)
		*s = 0;
	strncpy(basepath, fmap->sys2unix(basepath), sizeof(basepath));
	log_printf(ULOG, "[Theme Loader] Base path: \"%s\"\n", basepath);

	// Parse theme file
	KOBO_TP_Tokens res;
	int loadergfx = 0;
	while((res = parse_line()) > KTK_EOF)
	{
		// Reinit loading screen once its assets are loaded!
		if(!loadergfx && s_get_bank(gfxengine->get_gfx(), B_OAPLANET))
		{
			wdash->show_progress();
			loadergfx = 1;
		}
	}

	free(buffer);

	if(res != KTK_ERROR)
	{
		log_printf(ULOG, "[Theme Loader] \"%s\" loaded!\n", p);
		free(p);
		return true;
	}
	else
	{
		log_printf(ULOG, "[Theme Loader] \"%s\" failed to load!\n", p);
		free(p);
		return false;
	}
}
