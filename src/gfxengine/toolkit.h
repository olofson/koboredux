/*(LGPLv2.1)
---------------------------------------------------------------------------
	toolkit.h - Simple "GUI" toolkit for config screens.
---------------------------------------------------------------------------
 * Copyright 2001, 2009 David Olofson
 * Copyright 2015-2017 David Olofson (Kobo Redux)
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

/*
 * Note: change(0) means "apply", and would be the normal
 *       result of pressing enter while a widget is selected.
 */

#ifndef _TOOLKIT_H_
#define _TOOLKIT_H_

#include "window.h"

enum ct_align_t
{
	ALIGN_DEFAULT = -1,
	ALIGN_NONE = 0,
	ALIGN_LEFT,
	ALIGN_TOP = ALIGN_LEFT,
	ALIGN_RIGHT,
	ALIGN_BOTTOM = ALIGN_RIGHT,
	ALIGN_CENTER,
	ALIGN_CENTER_TOKEN
};

class ct_form_t;

class ct_widget_t : public window_t
{
  protected:
	friend class ct_form_t;
	int		interactive;
	int		transparent;
	int		highlighted;
	Uint32		_color;
	double		_value;
	char		*_string;
	int		_widget_index;
	ct_align_t	_halign;
	ct_align_t	_valign;
	char		_token;
	float		xo, yo;	//[0,1] maps to [0,width()]!
	virtual void render();
	void render_text_aligned(const char *buf);
  public:
	ct_form_t	*parent;
	ct_widget_t	*next, *prev;	//*CIRCULAR* list!
	void		*user;	//user stuff
	int		user2;	//user stuff
	int		tag;	//user stuff
	ct_widget_t(gfxengine_t *e);
	~ct_widget_t();
	void transparency(int t);
	void highlight(int hl);
	void color(Uint32 _cl);
	virtual void change(int delta);
	virtual void value(double val);
	virtual void value(const char *str);
	virtual double value();
	virtual const char *stringvalue();

	// Raw event capture
	void rawcapture(bool on);
	virtual bool rawevent(SDL_Event *ev);

	//Contents alignment modes.
	virtual void halign(ct_align_t ha);
	virtual void valign(ct_align_t va);
	ct_align_t halign()			{ return _halign; }
	ct_align_t valign()			{ return _valign; }

	//Token and offsets- used by some alignment modes.
	virtual void token(char tok)		{ _token = tok; }
	char token()				{ return _token; }
	virtual void offset(float _x, float _y)	{ xo = _x; yo = _y; }
	int xoffs()		{ return (int)(xo * width()); }
	int yoffs()		{ return (int)(yo * height()); }
	float rxoffs()		{ return xo; }
	float ryoffs()		{ return yo; }
};


class ct_engine_t
{
  protected:
	friend class ct_widget_t;
	ct_widget_t *rawcapture;
  public:
	ct_engine_t();
	void (*render_highlight)(ct_widget_t *wg);
	bool rawevent(SDL_Event *ev);
};

extern ct_engine_t ct_engine;


class ct_label_t : public ct_widget_t
{
  protected:
	char	_caption[64];
	void render();
  public:
	ct_label_t(gfxengine_t *e, const char *cap = NULL);
	void caption(const char *cap);
	const char *caption()	{ return _caption; }
	void halign(ct_align_t ha);
};


class ct_item_t
{
  protected:
	char	_caption[64];
	int	_index;
	double	_value;
	char	*_string;
  public:
	ct_item_t	*next, *prev;	//*CIRCULAR* list!
	ct_item_t(const char *cap = NULL, double val = 0.0f);
	ct_item_t(const char *ca, const char *str);
	void caption(const char *cap);
	const char *caption()	{ return _caption; }
	void value(double val)	{ _value = val; }
	void value(const char *str)
	{
		free(_string);
		_string = strdup(str);
	}
	double value()			{ return _value; }
	const char *stringvalue()	{ return _string; }
	void index(int i)		{ _index = i; }
	int index()			{ return _index; }
};

class ct_list_t : public ct_label_t
{
  protected:
	ct_item_t	*items;
	ct_item_t	*_selected;
	void render();
  public:
	ct_list_t(gfxengine_t *e, const char *cap = NULL);
	virtual ~ct_list_t();
	void add(ct_item_t *item);
	void add(const char *cap, int val);
	void clean();
	void select(ct_item_t *item);
	void select(int ind);
	ct_item_t *selected();
	void value(double val);
	void value(const char *val);
	double value();
	const char *stringvalue();
	void change(int delta);
	void halign(ct_align_t ha);
};


class ct_spin_t : public ct_label_t
{
  protected:
	int	min;
	int	max;
	char	_unit[32];
	void render();
  public:
	ct_spin_t(gfxengine_t *e, const char *cap = NULL, int _min = 0,
			int _max = 99999, const char *__unit = NULL);
	void value(double val);
	void unit(const char *txt);
	const char *unit()	{ return _unit; }
	void halign(ct_align_t ha);
};


class ct_string_t : public ct_label_t
{
  protected:
	void render();
  public:
	ct_string_t(gfxengine_t *e, const char *cap = NULL,
			const char *__str = "");
	void halign(ct_align_t ha);
};


class ct_button_t : public ct_label_t
{
  protected:
	void render();
  public:
	ct_button_t(gfxengine_t *e, const char *cap = NULL);
};


class ct_form_t : public window_t
{
  protected:
	ct_widget_t	*widgets;
	ct_widget_t	*_selected;
  public:
	ct_form_t(gfxengine_t *e);
	virtual ~ct_form_t();
	void add(ct_widget_t *w);
	void clean();
	virtual void select(ct_widget_t *w);
	virtual void select(int ind);
	virtual void select(int x, int y);
	virtual void next();
	virtual void prev();
	ct_widget_t *selected();
	int selected_index();
	void render();
	void render_nontransparent();
	virtual void change(int delta);
	virtual void apply_change(ct_widget_t *w);
};

#endif /* _TOOLKIT_H_ */
