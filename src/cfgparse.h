/*(LGPLv2.1)
-------------------------------------------------------------------
	cfgparse.h - Generic Config File and Argument Parser
-------------------------------------------------------------------
 * Copyright (C) 2001, 2007, 2009 David Olofson
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

#ifndef	CFGPARSE_H
#define	CFGPARSE_H

#include <stdio.h>

enum cfg_types_t
{
	CFG_NONE,
	CFG_BOOL,
	CFG_INT,
	CFG_FLOAT,
	CFG_STRING
};


#define	CFG_STRING_LENGTH	256

typedef char cfg_string_t[CFG_STRING_LENGTH];


class cfg_key_t
{
  public:
	cfg_key_t	*next;
	int		save;
	int		redefined;

	cfg_types_t	typecode;
	const char	*name;
	char		*description;
	cfg_key_t();
	virtual ~cfg_key_t();
	virtual int read(int argc, char *argv[]);
	virtual int test(const char *arg);
	virtual int copy(cfg_key_t *from) = 0;
	virtual void write(FILE *f) = 0;
	virtual void set_default() = 0;
	virtual int is_your_var(void *var) = 0;
};


class cfg_comment_t : public cfg_key_t
{
  public:
	cfg_comment_t(const char *text)		{ name = text; }
	int read(int argc, char *argv[])	{ return 0; }
	int copy(cfg_key_t *from);
	void write(FILE *f);
	void set_default()			{ ; }
	int is_your_var(void *var)		{ return 0; }
};


class cfg_switch_t : public cfg_key_t
{
  public:
	int	*value;
	int	default_value;

	cfg_switch_t(const char *_name, int &var, int def, int _save);
	int copy(cfg_key_t *from);
	virtual int test(const char *arg);
	int read(int argc, char *argv[]);
	void write(FILE *f);
	void set_default();
	int is_your_var(void *var);
};


class cfg_key_int_t : public cfg_key_t
{
  public:
	int	*value;
	int	default_value;

	cfg_key_int_t(const char *_name, int &var, int def, int _save);
	int copy(cfg_key_t *from);
	int read(int argc, char *argv[]);
	void write(FILE *f);
	void set_default();
	int is_your_var(void *var);
};


class cfg_key_float_t : public cfg_key_t
{
  public:
	float	*value;
	float	default_value;

	cfg_key_float_t(const char *_name, float &var, float def, int _save);
	int copy(cfg_key_t *from);
	int read(int argc, char *argv[]);
	void write(FILE *f);
	void set_default();
	int is_your_var(void *var);
};


class cfg_key_string_t : public cfg_key_t
{
  public:
	cfg_string_t	*value;
	cfg_string_t	default_value;

	cfg_key_string_t(const char *_name, cfg_string_t &var,
			const cfg_string_t def, int _save);
	int copy(cfg_key_t *from);
	int read(int argc, char *argv[]);
	void write(FILE *f);
	void set_default();
	int is_your_var(void *var);
};


class config_parser_t
{
	cfg_key_t	*keys, *last_key;
	int		initialized;
	int		nkeys;		// For symbol table API
	cfg_key_t	**table;	// For symbol table API
	cfg_string_t	retbuf;		// For symbol table API
	void initialize();
	void print_switch(FILE *f, const char *base, int flag);
	int read_config(char ***cv, FILE *f);
	void add(cfg_key_t *_key);
	int _redefined(void *var);
	void _accept(void *var);
	void build_table();
	int check_symbol(int symbol);
  protected:
	//Key registration: For use in inherited init()
	void comment(const char *text);
	void yesno(const char *name, int &var, int def, int save = 1);
	void command(const char *name, int &var);
	void key(const char *name, int &var, int def, int save = 1);
	void key(const char *name, float &var, float def, int save = 1);
	void key(const char *name, cfg_string_t &var, const cfg_string_t def,
			int save = 1);
	void desc(const char *text);

	//Plug key registration and custom stuff in here!
	virtual void init() = 0;
	virtual void postload()		{ ; }
	virtual void presave()		{ ; }
  public:
	int	changed;	//Set when *edited* by the user
	config_parser_t();
	virtual ~config_parser_t();
	config_parser_t &operator = (config_parser_t &from);
	void set_defaults();
	int parse(int argc, char *argv[]);
	int read(FILE *f);
	int write(FILE *f);

	//Changed since last accept() call?
	int redefined(int &var)			{ return _redefined(&var); }
	int redefined(float &var)		{ return _redefined(&var); }
	int redefined(cfg_string_t &var)	{ return _redefined(&var); }

	//Reset redefined() state to 0.
	void accept(int &var)			{ _accept(&var); }
	void accept(float &var)			{ _accept(&var); }
	void accept(cfg_string_t &var)		{ _accept(&var); }

	/* Generic Symbol Table Style API */
	int find(const char *name);
	int find_next(int symbol = -1);
	cfg_types_t type(int symbol);
	int do_save(int symbol);
	const char *name(int symbol);
	const char *description(int symbol);
	void set(int symbol, int value);
	void set(int symbol, float value);
	void set(int symbol, const char *text);
	int get_i(int symbol);
	int get_default_i(int symbol);
	float get_f(int symbol);
	float get_default_f(int symbol);
	/*
	 * WARNING: These ones sometimes return pointers to non-constant
	 *          buffers! Therefore, you need to copy the results
	 *          before you call these functions again.
	 */
	const char *get_s(int symbol);
	const char *get_default_s(int symbol);
};


#endif	//CFGPARSE_H
