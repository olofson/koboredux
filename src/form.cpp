/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001, 2002, 2007, 2009 David Olofson
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

#define	DBG(x)

#include "kobolog.h"
#include "config.h"
#include "form.h"
#include "kobo.h"

kobo_form_t::kobo_form_t(gfxengine_t *e) : ct_form_t(e)
{
	ypos = 0;
	current_list = NULL;
	_big = 0;
	xoffs = 0.5;
	halign = ALIGN_DEFAULT;
}


kobo_form_t::~kobo_form_t()
{
}


void kobo_form_t::next()
{
	sound.ui_move();
	ct_form_t::next();
}


void kobo_form_t::prev()
{
	sound.ui_move();
	ct_form_t::prev();
}


/* virtual */ void kobo_form_t::change(int delta)
{
	ct_form_t::change(delta);
	if(selected()->user)
	{
		int *val = (int *)selected()->user;
		*val = selected()->value();
		DBG(log_printf(D2LOG, "Changed to %d\n", selected()->value());)
	}
}


/* virtual */void kobo_form_t::build()
{
}


void kobo_form_t::init_widget(ct_widget_t *w)
{
	current_widget = w;
	switch(_big)
	{
	  case 0:
		w->place(px(), py() + ypos, width(), LINE_H);
		w->font(B_NORMAL_FONT);
		ypos += LINE_H;
		break;
	  case 1:
		w->place(px(), py() + ypos, width(), BIG_LINE_H);
		w->font(B_BIG_FONT);
		ypos += BIG_LINE_H;
		break;
	  case 2:
		w->place(px(), py() + ypos, width(), BIG_LINE_H);
		w->font(B_MEDIUM_FONT);
		ypos += BIG_LINE_H;
		break;
	}
	w->halign(halign);
	add(w);
}


void kobo_form_t::big()
{
	_big = 1;
}


void kobo_form_t::medium()
{
	_big = 2;
}


void kobo_form_t::small()
{
	_big = 0;
}


void kobo_form_t::begin()
{
	clean();
	ypos = 15;
	current_list = NULL;
	small();
}


void kobo_form_t::label(const char *cap)
{
	ct_label_t *w = new ct_label_t(engine, cap);
	init_widget(w);
}


void kobo_form_t::yesno(const char *cap, int *var, int tag)
{
	current_list = new ct_list_t(engine, cap);
	current_list->offset(xoffs, 0);
	current_list->user = var;
	current_list->tag = tag;
	current_list->add("Yes", 1);
	current_list->add("No", 0);
	init_widget(current_list);
}


void kobo_form_t::onoff(const char *cap, int *var, int tag)
{
	current_list = new ct_list_t(engine, cap);
	current_list->offset(xoffs, 0);
	current_list->user = var;
	current_list->tag = tag;
	current_list->add("On", 1);
	current_list->add("Off", 0);
	init_widget(current_list);
}


void kobo_form_t::spin(const char *cap, int *var, int min, int max,
		const char *unit, int tag)
{
	ct_spin_t *w = new ct_spin_t(engine, cap, min, max, unit);
	w->offset(xoffs, 0);
	w->user = var;
	w->tag = tag;
	init_widget(w);
}


void kobo_form_t::list(const char *cap, int *var, int tag)
{
	ct_list_t *w = new ct_list_t(engine, cap);
	w->offset(xoffs, 0);
	w->user = var;
	w->tag = tag;
	current_list = w;
	init_widget(w);
}


void kobo_form_t::item(const char *cap, int value, int ind)
{
	if(current_list)
	{
		ct_item_t *i = new ct_item_t(cap, value);
		i->index(ind);
		current_list->add(i);
	}
	else
		log_printf(ELOG, "kobo_form_t::form_item(): No list!\n");
}


void kobo_form_t::perc_list(int first, int last, int step)
{
	char buf[50];
	for(int i = first; i <= last; i += step)
	{
		snprintf(buf, sizeof(buf), "%d%%", i);
		item(buf, i);
	}
}


void kobo_form_t::enum_list(int first, int last)
{
	char buf[50];
	for(int i = first; i <= last; i++)
	{
		snprintf(buf, sizeof(buf), "%d", i);
		item(buf, i);
	}
}


void kobo_form_t::button(const char *cap, int tag)
{
	ct_widget_t *w = new ct_button_t(engine, cap);
	w->offset(xoffs, 0);
	w->tag = tag;
	init_widget(w);
}


void kobo_form_t::data(config_parser_t *_d)
{
	_data = _d;
}


void kobo_form_t::editor(int handle, int tag)
{
	if(!_data)
		return;

	switch(_data->type(handle))
	{
	  default:
		log_printf(ELOG, "kobo_form_t: Unknown data type!\n");
		break;
	  case CFG_BOOL:
		onoff(_data->description(handle), (int *)NULL, tag);
		current_widget->user2 = handle;
		break;
	  case CFG_INT:
	  case CFG_FLOAT:
		spin(_data->description(handle), (int *)NULL,
				-100000, 100000, "", tag);
		current_widget->user2 = handle;
		break;
	  case CFG_STRING:
		log_printf(ELOG, "kobo_form_t: String editor not yet implemented!\n");
		break;
	}
}


void kobo_form_t::editor(const char *name, int tag)
{
	if(!_data)
		return;

	int h = _data->find(name);
	if(h < 0)
		return;
	editor(h, tag);
}


void kobo_form_t::space(int lines)
{
	if(_big)
		ypos += BIG_SPACE_SIZE * lines;
	else
		ypos += SPACE_SIZE * lines;
}


void kobo_form_t::end()
{
}


void kobo_form_t::build_all()
{
	xoffs = 0.5;
	begin();
	build();
	end();

	/*
	 * Initialize all widgets from prefs and/or config_parser.
	 */
	ct_widget_t *w = widgets;
	while(w)
	{
		if(w->user)
		{
			int *val = (int *)w->user;
			if(val)
			{
				w->value(*val);
				log_printf(D3LOG, "Initialized to %d from prefs\n",
						w->value());
			}
			else if(_data)
			{
				w->value(_data->get_i(w->user2));
				log_printf(D3LOG, "Initialized to %d from config_parser\n",
						w->value());
			}
		}
		w = w->next;
		if(w == widgets)
			break;	//Done!
	}
}

