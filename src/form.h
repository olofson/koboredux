/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001, 2007, 2009 David Olofson
 * Copyright 2005 Erik Auerswald
 * Copyright 2015-2017 David Olofson (Kobo Redux)
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

#ifndef	_KOBO_FORM_H_
#define	_KOBO_FORM_H_

#include "toolkit.h"
#include "cfgparse.h"

class kobo_keybinding_t : public ct_string_t
{
  protected:
	bool scanning;
	void render();
  public:
	kobo_keybinding_t(gfxengine_t *e, const char *cap);
	void value(double val);
	void change(int delta);
	bool rawevent(SDL_Event *ev);
};

class kobo_form_t : public ct_form_t
{
	//Called by build_all().
	void begin();
	void end();
  protected:
	ct_widget_t	*current_widget;
	ct_list_t	*current_list;
	ct_label_t	*help_bar;
	int		ypos;
	int		_font;
	float		xoffs;
	ct_align_t	halign;
	virtual void init_widget(ct_widget_t *wg, int def_font);
	void update_help();
	int get_font(int def);

	//Toolkit
	void font(int bank = 0);	// Override widget default font
	void title(const char *cap);
	void label(const char *cap);
	void yesno(const char *cap, int *var, int tag = 0);
	void onoff(const char *cap, int *var, int tag = 0);
	void spin(const char *cap, int *var, int min, int max,
			const char *unit, int tag = 0);
	void keybinding(const char *cap, int *var, int tag = 0);
	void button(const char *cap, int tag = 0);
	void space(float lines = 1.0f);	// Negative values are "virtual" pixels
	void help();

	//List tools
	void list(const char *cap, void *var, int tag = 0);
	void item(const char *cap, int value, int ind = 0);
	void item(const char *cap, float value, int ind = 0);
	void item(const char *cap, const char *value, int ind = 0);
	void perc_list(int first, int last, int step);
	void enum_list(int first, int last);

	//Internal wrapper for build()
	void build_all();
  public:
	kobo_form_t(gfxengine_t *e);
	virtual ~kobo_form_t();
	void next();
	void prev();
	void rebuild();
	void apply_change(ct_widget_t *w);
	virtual void build();
};

#endif
