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

#ifndef _KOBO_THEMEPARSER_H_
#define _KOBO_THEMEPARSER_H_

#include "graphics.h"
#include "fire.h"
#include <math.h>

#define	KOBO_TP_MAXLEN	256


class KOBO_ThemeData
{
	friend class KOBO_ThemeParser;
	unsigned sizes[KOBO_D__COUNT];
	double *items[KOBO_D__COUNT];
	char *strings[KOBO_D__COUNT];
	KOBO_ParticleFXDef *pfxdefs[KOBO_PFX__COUNT];
	int pfxaliases[KOBO_PFX__COUNT];
	unsigned pfxflags[KOBO_PFX__COUNT];
  public:
	KOBO_ThemeData *next;

	KOBO_ThemeData();
	~KOBO_ThemeData();

	void clear(KOBO_TD_Items item)
	{
		sizes[item] = 0;
	}

	bool set(KOBO_TD_Items item, unsigned index, double value);
	bool set(KOBO_TD_Items item, const char *str);

	bool set(KOBO_TD_Items item, double value)
	{
		return set(item, 0, value);
	}

	double get(KOBO_TD_Items item, unsigned index = 0)
	{
		if(!sizes[item])
			return 0.0f;
		if(index >= sizes[item])
			index = sizes[item] - 1;
		return items[item][index];
	}

	const char *get_string(KOBO_TD_Items item, const char *fallback = 0)
	{
		return strings[item] ? strings[item] : fallback;
	}

	double lerp(KOBO_TD_Items item, double index)
	{
		int ii = floor(index);
		float w = fmod(index, 1.0f);
		return get(item, ii) * (1.0f - w) + get(item, ii + 1) * w;
	}

	int length(KOBO_TD_Items item)
	{
		return sizes[item];
	}

	bool defined(KOBO_TD_Items item, unsigned index = 0)
	{
		if(!sizes[item])
			return false;
		if(index >= sizes[item])
			return false;
		return true;
	}

	KOBO_ParticleFXDef *pfxdef(KOBO_ParticleFX item);
	void pfxalias(KOBO_ParticleFX item, KOBO_ParticleFX to);
};


enum KOBO_TP_Tokens
{
	KTK_ERROR = -1,
	KTK_EOF = 0,
	KTK_EOLN,

	KTK_BEGIN,	// '{'
	KTK_END,	// '}'

	// Symbols
	KTK_BANK,	// iv = bank index
	KTK_PALETTE,	// iv = palette index
	KTK_FLAG,	// iv = flag bit mask
	KTK_STRING,	// sv = nul terminated string
	KTK_NUMBER,	// rv = value
	KTK_HEXCOLOR,	// iv = value (NOTE: Unsigned!)
	KTK_THEMEDATA,	// iv = ThemeData item index
	KTK_PFXDEF,	// iv = ThemeData particle effect definition index

	// General
	KTK_KW_FALLBACK,
	KTK_KW_PATH,
	KTK_KW_MESSAGE,
	KTK_KW_SET,

	// Stages, maps, and levels
	KTK_KW_STAGEMESSAGE,

	// Graphics
	KTK_KW_IMAGE,
	KTK_KW_SPRITES,
	KTK_KW_SFONT,
	KTK_KW_PALETTE,
	KTK_KW_ALIAS,
	KTK_KW_PARTICLES,

	// Local: Particle system definition keywords
	KTK_KW_PFX_DEFAULT,
	KTK_KW_PFX_DELAY,
	KTK_KW_PFX_THRESHOLD,
	KTK_KW_PFX_COUNT,
	KTK_KW_PFX_XOFFS,
	KTK_KW_PFX_YOFFS,
	KTK_KW_PFX_RADIUS,
	KTK_KW_PFX_TWIST,
	KTK_KW_PFX_SPEED,
	KTK_KW_PFX_DRAG,
	KTK_KW_PFX_HEAT,
	KTK_KW_PFX_FADE,
	KTK_KW_PFX_CHAIN,
	KTK_KW_PFX_CHILD
};

struct TP_Keywords
{
	const char	*kw;
	KOBO_TP_Tokens	token;
	int		value;
};

class KOBO_ThemeParser
{
	KOBO_ThemeData *themedata;
	const char *buffer;
	int bufsize;
	TP_Keywords *local_keywords;
	int pos;
	int unlex_pos;
	int default_flags;
	bool silent;
	char sv[KOBO_TP_MAXLEN];
	double rv;
	int iv;
	char basepath[KOBO_TP_MAXLEN];
	char path[KOBO_TP_MAXLEN];
	const char *get_path(const char *p);
	const char *fullpath(const char *fp);
	int bufget()
	{
		if(pos < bufsize)
			return buffer[pos++];
		else
		{
			++pos;
			return 0;
		}
	}
	void bufunget()
	{
		--pos;
	}
	bool is_num(int c)
	{
		return (c >= '0') && (c <= '9');
	}
	bool is_alpha(int c)
	{
		return ((c >= 'a') && (c <= 'z')) ||
				((c >= 'A') && (c <= 'Z'));
	}
	bool is_white(int c, bool eoln = false)
	{
		if(eoln && (c == '\n'))
			return true;
		return (c == ' ') || (c == '\t') || (c == '\a');
	}
	const char *token_name(KOBO_TP_Tokens tk);
	void dump_line();
	void skip_white(bool skipeoln = false);
	void skip_to_eoln();
	KOBO_TP_Tokens lex_number();
	KOBO_TP_Tokens lex_hexcolor();
	KOBO_TP_Tokens lex_string();
	KOBO_TP_Tokens lex_symbol();
	KOBO_TP_Tokens lex(bool skipeoln = false);
	void unlex();
	bool expect(KOBO_TP_Tokens token, bool skipeoln = false);
	bool read_flags(int *flags, int allowed);
	void apply_flags(int flags, double scale);
	void warn_bank_used(int bank);
	KOBO_TP_Tokens handle_message();
	KOBO_TP_Tokens handle_stagemessage();
	KOBO_TP_Tokens handle_image();
	KOBO_TP_Tokens handle_sprites();
	KOBO_TP_Tokens handle_sfont();
	KOBO_TP_Tokens handle_palette();
	KOBO_TP_Tokens handle_palette_gpl(int pal);
	KOBO_TP_Tokens handle_palette_hex(int pal);
	KOBO_TP_Tokens handle_palette_index(int pal, int source);
	KOBO_TP_Tokens handle_fallback();
	KOBO_TP_Tokens handle_path();
	KOBO_TP_Tokens handle_alias_bank(int bank);
	KOBO_TP_Tokens handle_alias_pfx(int pfx);
	KOBO_TP_Tokens handle_alias();
	KOBO_TP_Tokens handle_set();
	KOBO_TP_Tokens parse_spec(KOBO_RangeSpec &rs);
	KOBO_TP_Tokens parse_spec(KOBO_RandSpec &rs);
	KOBO_TP_Tokens particles_line(KOBO_ParticleFXDef *pfxd);
	KOBO_TP_Tokens particles_body(KOBO_ParticleFXDef *pfxd);
	KOBO_TP_Tokens handle_particles();
	KOBO_TP_Tokens parse_line();
	void init(int flags);
	KOBO_TP_Tokens parse_theme(const char *scriptpath, int flags = 0);
  public:
	KOBO_ThemeParser(KOBO_ThemeData &td);
	bool parse(const char *theme, int flags = 0);
	bool load(const char *themepath, int flags = 0);
	bool examine(const char *themepath, int flags = 0);
};

#endif // _KOBO_THEMEPARSER_H_

