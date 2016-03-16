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

#ifndef _KOBO_THEMEPARSER_H_
#define _KOBO_THEMEPARSER_H_

#include "graphics.h"

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

#endif // _KOBO_THEMEPARSER_H_
