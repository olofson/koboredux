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

#ifndef _KOBO_GRAPHICS_H_
#define _KOBO_GRAPHICS_H_

// Graphics banks
#define KOBO_ALLGFXBANKS			\
	KOBO_DEFS(NONE)				\
	KOBO_DEFS(R1_TILES)			\
	KOBO_DEFS(R2_TILES)			\
	KOBO_DEFS(R3_TILES)			\
	KOBO_DEFS(R4_TILES)			\
	KOBO_DEFS(R5_TILES)			\
\
	KOBO_DEFS(R1_TILES_SMALL_SPACE)		\
	KOBO_DEFS(R2_TILES_SMALL_SPACE)		\
	KOBO_DEFS(R3_TILES_SMALL_SPACE)		\
	KOBO_DEFS(R4_TILES_SMALL_SPACE)		\
	KOBO_DEFS(R5_TILES_SMALL_SPACE)		\
\
	KOBO_DEFS(R1_TILES_TINY_SPACE)		\
	KOBO_DEFS(R2_TILES_TINY_SPACE)		\
	KOBO_DEFS(R3_TILES_TINY_SPACE)		\
	KOBO_DEFS(R4_TILES_TINY_SPACE)		\
	KOBO_DEFS(R5_TILES_TINY_SPACE)		\
\
	KOBO_DEFS(R1_TILES_SMALL_GROUND)	\
	KOBO_DEFS(R2_TILES_SMALL_GROUND)	\
	KOBO_DEFS(R3_TILES_SMALL_GROUND)	\
	KOBO_DEFS(R4_TILES_SMALL_GROUND)	\
	KOBO_DEFS(R5_TILES_SMALL_GROUND)	\
\
	KOBO_DEFS(R1_TILES_TINY_GROUND)		\
	KOBO_DEFS(R2_TILES_TINY_GROUND)		\
	KOBO_DEFS(R3_TILES_TINY_GROUND)		\
	KOBO_DEFS(R4_TILES_TINY_GROUND)		\
	KOBO_DEFS(R5_TILES_TINY_GROUND)		\
\
	KOBO_DEFS(R1_PLANET)			\
	KOBO_DEFS(R2_PLANET)			\
	KOBO_DEFS(R3_PLANET)			\
	KOBO_DEFS(R4_PLANET)			\
	KOBO_DEFS(R5_PLANET)			\
\
	KOBO_DEFS(R1_CLOUDS)			\
	KOBO_DEFS(R2_CLOUDS)			\
	KOBO_DEFS(R3_CLOUDS)			\
	KOBO_DEFS(R4_CLOUDS)			\
	KOBO_DEFS(R5_CLOUDS)			\
\
	KOBO_DEFS(R1L8_GROUND)			\
	KOBO_DEFS(R2L8_GROUND)			\
	KOBO_DEFS(R3L8_GROUND)			\
	KOBO_DEFS(R4L8_GROUND)			\
	KOBO_DEFS(R5L8_GROUND)			\
\
	KOBO_DEFS(R1L9_GROUND)			\
	KOBO_DEFS(R2L9_GROUND)			\
	KOBO_DEFS(R3L9_GROUND)			\
	KOBO_DEFS(R4L9_GROUND)			\
	KOBO_DEFS(R5L9_GROUND)			\
\
	KOBO_DEFS(R1L10_GROUND)			\
	KOBO_DEFS(R2L10_GROUND)			\
	KOBO_DEFS(R3L10_GROUND)			\
	KOBO_DEFS(R4L10_GROUND)			\
	KOBO_DEFS(R5L10_GROUND)			\
\
	KOBO_DEFS(CROSSHAIR)			\
	KOBO_DEFS(PLAYER)			\
	KOBO_DEFS(BLT_GREEN)			\
	KOBO_DEFS(BLT_RED)			\
	KOBO_DEFS(BLT_BLUE)			\
	KOBO_DEFS(BLTX_GREEN)			\
	KOBO_DEFS(BLTX_RED)			\
	KOBO_DEFS(BLTX_BLUE)			\
	KOBO_DEFS(RING)				\
	KOBO_DEFS(RINGEXPL)			\
	KOBO_DEFS(BOMB)				\
	KOBO_DEFS(BOMBDETO)			\
	KOBO_DEFS(BOLT)				\
	KOBO_DEFS(EXPLO1)			\
	KOBO_DEFS(EXPLO3)			\
	KOBO_DEFS(EXPLO4)			\
	KOBO_DEFS(EXPLO5)			\
	KOBO_DEFS(ROCK1)			\
	KOBO_DEFS(ROCK2)			\
	KOBO_DEFS(ROCK3)			\
	KOBO_DEFS(ROCKEXPL)			\
	KOBO_DEFS(BMR_GREEN)			\
	KOBO_DEFS(BMR_PURPLE)			\
	KOBO_DEFS(BMR_PINK)			\
	KOBO_DEFS(FIGHTER)			\
	KOBO_DEFS(MISSILE1)			\
	KOBO_DEFS(MISSILE2)			\
	KOBO_DEFS(MISSILE3)			\
	KOBO_DEFS(BIGSHIP)			\
\
	KOBO_DEFS(NOISE)			\
	KOBO_DEFS(HITNOISE)			\
	KOBO_DEFS(FOCUSFX)			\
\
	KOBO_DEFS(SCREEN)			\
	KOBO_DEFS(HLEDS)			\
	KOBO_DEFS(VLEDS)			\
	KOBO_DEFS(BLEDS)			\
\
	KOBO_DEFS(LOGO)				\
\
	KOBO_DEFS(HIGH_BACK)			\
	KOBO_DEFS(SCORE_BACK)			\
	KOBO_DEFS(RADAR_BACK)			\
	KOBO_DEFS(SHIPS_BACK)			\
	KOBO_DEFS(STAGE_BACK)			\
\
	KOBO_DEFS(HEALTH_LID)			\
	KOBO_DEFS(TEMP_LID)			\
	KOBO_DEFS(TTEMP_LID)			\
\
	KOBO_DEFS(OALOGO)			\
	KOBO_DEFS(OAPLANET)			\
\
	KOBO_DEFS(LOADER_FONT)			\
	KOBO_DEFS(DEBUG_FONT)			\
	KOBO_DEFS(SMALL_FONT)			\
	KOBO_DEFS(NORMAL_FONT)			\
	KOBO_DEFS(MEDIUM_FONT)			\
	KOBO_DEFS(BIG_FONT)			\
	KOBO_DEFS(COUNTER_FONT)

#define	KOBO_DEFS(x)	B_##x,
enum KOBO_gfxbanks
{
	KOBO_ALLGFXBANKS
	B__COUNT
};
#undef	KOBO_DEFS

extern const char *kobo_gfxbanknames[];


typedef enum
{
	// Clamping/wrapping options for filters
	KOBO_CLAMP =		0x0001,	// Clamp to frame edge pixels
	KOBO_CLAMP_OPAQUE =	0x0002,	// Clamp to black; not transparent
	KOBO_WRAP =		0x0004,	// Wrap around frame edges

	// Scaling filter options
	KOBO_ABSSCALE =		0x0010,	// Scale factor is absolute
	KOBO_NEAREST =		0x0020,	// Force NEAREST scale mode
	KOBO_BILINEAR =		0x0040,	// Force BILINEAR scale mode
	KOBO_SCALE2X =		0x0080,	// Force Scale2X scale mode

	// Other options
	KOBO_NOALPHA =		0x0100,	// Disable alpha channel
	KOBO_CENTER =		0x0200,	// Center hotspot in frames
	KOBO_NOBRIGHT =		0x0400,	// Disable brightness/contrast filter
	KOBO_FALLBACK =		0x1000,	// Disable "in use" overwrite warning
	KOBO_FUTURE =		0x2000	// Allow alias to (still) empty banks
} KOBO_GfxDescFlags;


#define	KOBO_TP_MAXLEN	256

enum KOBO_TP_Tokens
{
	KTK_ERROR = -1,
	KTK_EOF = 0,
	KTK_EOLN,

	KTK_BANK,	// iv = bank
	KTK_FLAG,	// iv = flag bit mask
	KTK_STRING,	// sv = nul terminated string
	KTK_NUMBER,	// rv = value

	KTK_KW_MESSAGE,
	KTK_KW_IMAGE,
	KTK_KW_SPRITES,
	KTK_KW_SFONT,
	KTK_KW_PALETTE,
	KTK_KW_FALLBACK,
	KTK_KW_PATH,
	KTK_KW_ALIAS
};

class KOBO_ThemeParser
{
	char *buffer;
	int bufsize;
	int pos;
	int unlex_pos;
	int default_flags;
	char sv[KOBO_TP_MAXLEN];
	double rv;
	int iv;
	char basepath[KOBO_TP_MAXLEN];
	char path[KOBO_TP_MAXLEN];
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
	bool is_white(int c)
	{
		return (c == ' ') || (c == '\t') || (c == '\a');
	}
	const char *token_name(KOBO_TP_Tokens tk)
	{
		switch(tk)
		{
		  case KTK_ERROR:	return "ERROR";
		  case KTK_EOF:		return "EOF";
		  case KTK_EOLN:	return "EOLN";
		  case KTK_BANK:	return "BANK";
		  case KTK_FLAG:	return "FLAG";
		  case KTK_STRING:	return "STRING";
		  case KTK_NUMBER:	return "NUMBER";
		  case KTK_KW_MESSAGE:	return "KW_MESSAGE";
		  case KTK_KW_IMAGE:	return "KW_IMAGE";
		  case KTK_KW_SPRITES:	return "KW_SPRITES";
		  case KTK_KW_SFONT:	return "KW_SFONT";
		  case KTK_KW_PALETTE:	return "KW_PALETTE";
		  case KTK_KW_FALLBACK:	return "KW_FALLBACK";
		  case KTK_KW_PATH:	return "KW_PATH";
		  case KTK_KW_ALIAS:	return "KW_ALIAS";
		}
		return "<unknown>";
	}
	void dump_line();
	void skip_white();
	void skip_to_eoln();
	KOBO_TP_Tokens lex_number();
	KOBO_TP_Tokens lex_string();
	KOBO_TP_Tokens lex_symbol();
	KOBO_TP_Tokens lex();
	void unlex();
	bool expect(KOBO_TP_Tokens token);
	bool read_flags(int *flags, int allowed);
	void apply_flags(int flags, double scale);
	void warn_bank_used(int bank);
	KOBO_TP_Tokens handle_message();
	KOBO_TP_Tokens handle_image();
	KOBO_TP_Tokens handle_sprites();
	KOBO_TP_Tokens handle_sfont();
	KOBO_TP_Tokens handle_palette();
	KOBO_TP_Tokens handle_fallback();
	KOBO_TP_Tokens handle_path();
	KOBO_TP_Tokens handle_alias();
	KOBO_TP_Tokens parse_line();
	KOBO_TP_Tokens parse_theme(const char *scriptpath, int flags = 0);
  public:
	KOBO_ThemeParser();
	bool load_theme(const char *themepath, int flags = 0);
};

#endif // _KOBO_GRAPHICS_H_
