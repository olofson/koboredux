/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001, 2002, 2007, 2009 David Olofson
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

#define	DBG(x)

#include "kobolog.h"
#include "config.h"
#include "form.h"
#include "kobo.h"


/*----------------------------------------------------------
	kobo_keybinding_t::
----------------------------------------------------------*/

kobo_keybinding_t::kobo_keybinding_t(gfxengine_t *e, const char *cap) :
		ct_string_t(e, cap)
{
	scanning = false;
}


void kobo_keybinding_t::value(double val)
{
	// NOTE:
	//	We're displaying the names from the current keyboard layout,
	//	which is technically wrong, but probably less confusing than
	//	scancodes. (And SDL doesn't have name strings for scancodes.)
	//	However, the configuration stores scancodes, as it should.
	//
	int v = (int)val;
	scanning = false;
	rawcapture(false);
	ct_string_t::value(v);
	SDL_Keycode key = SDL_GetKeyFromScancode((SDL_Scancode)v);
	const char *kn = SDL_GetKeyName(key);
	if(kn[0] && textwidth(kn))
		ct_string_t::value(kn);
	else
	{
		char buf[16];
		snprintf(buf, sizeof(buf), "0x%X", v);
		ct_string_t::value(buf);
	}
}


void kobo_keybinding_t::change(int delta)
{
	if(delta)
	{
		scanning = false;
		ct_string_t::change(delta);
	}
	else
		scanning = true;
	rawcapture(scanning);
}


bool kobo_keybinding_t::rawevent(SDL_Event *ev)
{
	switch (ev->type)
	{
	  case SDL_KEYDOWN:
		switch(ev->key.keysym.sym)
		{
		  case SDLK_ESCAPE:
			scanning = false;
			rawcapture(false);
			return true;
		  default:
			value(ev->key.keysym.scancode);
			parent->apply_change(this);
			return true;
		}
		break;
	  case SDL_WINDOWEVENT:
		switch(ev->window.event)
		{
		  case SDL_WINDOWEVENT_CLOSE:
		  case SDL_WINDOWEVENT_HIDDEN:
		  case SDL_WINDOWEVENT_MINIMIZED:
		  case SDL_WINDOWEVENT_FOCUS_LOST:
			scanning = false;
			rawcapture(false);
			return false;
		}
		break;
	  case SDL_QUIT:
		scanning = false;
		rawcapture(false);
		return false;
	}
	return false;
}


void kobo_keybinding_t::render()
{
	if(scanning)
	{
		char buf[128];
		ct_widget_t::render();
		snprintf(buf, sizeof(buf), "%s: %s", caption(),
				SDL_GetTicks() & 0x100 ? "Press key..." : "");
		render_text_aligned(buf);
	}
	else
		ct_string_t::render();
}


/*----------------------------------------------------------
	kobo_form_t::
----------------------------------------------------------*/

kobo_form_t::kobo_form_t(gfxengine_t *e) : ct_form_t(e)
{
	ypos = 0;
	current_list = NULL;
	help_bar = NULL;
	_font = 0;
	xoffs = 0.5;
	halign = ALIGN_DEFAULT;
}


kobo_form_t::~kobo_form_t()
{
}


void kobo_form_t::clean()
{
	current_widget = NULL;
	current_list = NULL;
	help_bar = NULL;
	ct_form_t::clean();
}


void kobo_form_t::update_help()
{
	if(!help_bar || !selected())
		return;

	if(!selected()->user)
	{
		help_bar->caption("");
		return;
	}

	int handle = prefs->get(selected()->user);
	if(handle < 0)
	{
		help_bar->caption("");
		return;
	}

	help_bar->caption(prefs->description(handle));
}


bool kobo_form_t::select(ct_widget_t *w)
{
	if(ct_form_t::select(w))
	{
		update_help();
		return true;
	}
	return false;
}


void kobo_form_t::apply_change(ct_widget_t *w)
{
	if(!w->user || !selected())
		return;

	int handle = prefs->get(selected()->user);
	if(handle < 0)
	{
		// Then it's not wired to a config, and it must be an int, as
		// thats the only type we support in that case!
		*((int *)w->user) = (int)w->value();
		return;
	}

	switch(prefs->type(handle))
	{
	  case CFG_BOOL:
	  case CFG_INT:
		prefs->set(handle, (int)w->value());
		DBG(log_printf(D2LOG, "Changed to %d\n", (int)w->value());)
		break;
	  case CFG_FLOAT:
		prefs->set(handle, (float)w->value());
		DBG(log_printf(D2LOG, "Changed to %f\n", w->value());)
		break;
	  case CFG_STRING:
		prefs->set(handle, w->stringvalue());
		DBG(log_printf(D2LOG, "Changed to \"%s\"\n",
				w->stringvalue());)
		break;
	  default:
		break;
	}
}


void kobo_form_t::build()
{
}


void kobo_form_t::rebuild()
{
	int sel = selected_index();
	build_all();
	select(sel);
}


void kobo_form_t::init_widget(ct_widget_t *w, int def_font)
{
	current_widget = w;
	w->font(get_font(def_font));
	w->place(px(), py() + ypos, width(), w->fontheight());
	ypos += w->fontheight();
	w->halign(halign);
	add(w);
}


void kobo_form_t::font(int bank)
{
	_font = bank;
}


int kobo_form_t::get_font(int def)
{
	if(_font)
		return _font;
	else
		return def;
}


void kobo_form_t::begin()
{
	clean();
	ypos = 15;
	current_list = NULL;
	font();
}


void kobo_form_t::title(const char *cap)
{
	ct_label_t *w = new ct_label_t(engine, cap);
	w->offset(xoffs, 0);
	init_widget(w, B_NORMAL_FONT);
	space(2);
}


void kobo_form_t::label(const char *cap)
{
	ct_label_t *w = new ct_label_t(engine, cap);
	w->offset(xoffs, 0);
	init_widget(w, B_MEDIUM_FONT);
}


void kobo_form_t::yesno(const char *cap, int *var, int tag)
{
	current_list = new ct_list_t(engine, cap);
	current_list->offset(xoffs, 0);
	current_list->user = var;
	current_list->tag = tag;
	current_list->add("Yes", 1);
	current_list->add("No", 0);
	init_widget(current_list, B_NORMAL_FONT);
}


void kobo_form_t::onoff(const char *cap, int *var, int tag)
{
	current_list = new ct_list_t(engine, cap);
	current_list->offset(xoffs, 0);
	current_list->user = var;
	current_list->tag = tag;
	current_list->add("On", 1);
	current_list->add("Off", 0);
	init_widget(current_list, B_NORMAL_FONT);
}


void kobo_form_t::spin(const char *cap, int *var, int min, int max,
		const char *unit, int tag)
{
	ct_spin_t *w = new ct_spin_t(engine, cap, min, max, unit);
	w->offset(xoffs, 0);
	w->user = var;
	w->tag = tag;
	init_widget(w, B_NORMAL_FONT);
}


void kobo_form_t::list(const char *cap, void *var, int tag)
{
	ct_list_t *w = new ct_list_t(engine, cap);
	w->offset(xoffs, 0);
	w->user = var;
	w->tag = tag;
	current_list = w;
	init_widget(w, B_NORMAL_FONT);
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


void kobo_form_t::item(const char *cap, float value, int ind)
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


void kobo_form_t::item(const char *cap, const char *value, int ind)
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


void kobo_form_t::keybinding(const char *cap, int *var, int tag)
{
	kobo_keybinding_t *k = new kobo_keybinding_t(engine, cap);
	k->offset(xoffs, 0);
	k->user = var;
	k->tag = tag;
	init_widget(k, B_NORMAL_FONT);
}


void kobo_form_t::button(const char *cap, int tag)
{
	ct_widget_t *w = new ct_button_t(engine, cap);
	w->offset(xoffs, 0);
	w->tag = tag;
	init_widget(w, B_BIG_FONT);
}


void kobo_form_t::space(float lines)
{
	if(lines < 0)
		ypos -= lines;
	else
		ypos += engine->screen()->fontheight(B_NORMAL_FONT) * lines;
}


void kobo_form_t::help()
{
	if(help_bar)
	{
		log_printf(ELOG, "kobo_form_t: Tried to create more than one "
				"help bar!\n");
		return;
	}
	help_bar = new ct_label_t(engine, "");
	help_bar->offset(0.5, 0);
	init_widget(help_bar, B_SMALL_FONT);
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

	// Initialize all widgets from prefs and/or config_parser.
	for(ct_widget_t *w = widgets; w;
			w = w->next == widgets ? NULL : w->next)
	{
		if(!w->user)
			continue;
		int handle = prefs->get(w->user);
		if(handle < 0)
		{
			w->value(*((int *)w->user));
			continue;
		}
		switch(prefs->type(handle))
		{
		  case CFG_BOOL:
		  case CFG_INT:
			w->value(prefs->get_i(handle));
			break;
		  case CFG_FLOAT:
			w->value(prefs->get_f(handle));
			break;
		  case CFG_STRING:
			w->value(prefs->get_s(handle));
			break;
		  default:
			log_printf(ELOG, "kobo_form_t: Configuration key '%s' "
					"has an unsupported data type!\n",
					prefs->name(handle));
			break;
		}
	}
	update_help();
}
