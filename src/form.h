/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001, 2007, 2009 David Olofson
 * Copyright 2005 Erik Auerswald
 * Copyright 2015 David Olofson (Kobo Redux)
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

#define	LINE_H		9
#define	SPACE_SIZE	7

#define	BIG_LINE_H	18
#define	BIG_SPACE_SIZE	14

class kobo_form_t : public ct_form_t
{
	config_parser_t	*_data;
	//Called by build_all().
	void begin();
	void end();
  protected:
	ct_widget_t	*current_widget;
	ct_list_t	*current_list;
	int		ypos;
	int		_big;
	float		xoffs;
	ct_align_t	halign;
	virtual void init_widget(ct_widget_t *wg);

	//Toolkit
	void big();
	void medium();
	void small();
	void label(const char *cap);
	void yesno(const char *cap, int *var, int tag = 0);
	void onoff(const char *cap, int *var, int tag = 0);
	void spin(const char *cap, int *var, int min, int max,
			const char *unit, int tag = 0);
	void button(const char *cap, int tag = 0);
	void space(int lines = 1);

	//config_parser_t aware version! :-)
	void data(config_parser_t *_d);
	void editor(int handle, int tag = 0);
	void editor(const char *name, int tag = 0);

	//List tools
	void list(const char *cap, int *var, int tag = 0);
	void item(const char *cap, int value, int ind = 0);
	void perc_list(int first, int last, int step);
	void enum_list(int first, int last);

	//Internal wrapper for build()
	void build_all();
  public:
	kobo_form_t(gfxengine_t *e);
	virtual ~kobo_form_t();
	void next();
	void prev();
	virtual void change(int delta);
	virtual void build();
};

#endif
