/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001-2003, 2006-2007, 2009 David Olofson
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

#include "kobolog.h"
#include "config.h"
#include "cfgform.h"
#include "kobo.h"
#include "sound.h"

int global_status = 0;


config_form_t::config_form_t(gfxengine_t *e) : kobo_form_t(e)
{
	prf = NULL;
	stat = 0;
	undostat = 0;
}


void config_form_t::open(prefs_t *p)
{
	place(wmain->px(), wmain->py(), wmain->width(), wmain->height());
	font(B_NORMAL_FONT);
	foreground(wmain->map_rgb(0xffffff));
	background(wmain->map_rgb(0x000000));
	prf = p;
	prfbak = *p;
	stat = global_status & (OS_RELOAD | OS_RESTART | OS_UPDATE);
	build_all();
}


/* virtual */void config_form_t::build()
{
}


void config_form_t::close()
{
	clean();
}


void config_form_t::undo()
{
	*prf = prfbak;
	stat = undostat;
}


int config_form_t::status()
{
	return stat;
}


void config_form_t::clearstatus(int mask)
{
	stat &= ~mask;
	global_status = stat & (OS_RELOAD | OS_RESTART | OS_UPDATE);
}


void config_form_t::setstatus(int mask)
{
	stat |= mask;
	undostat |= mask;
	global_status = stat & (OS_RELOAD | OS_RESTART | OS_UPDATE);
}

/*virtual*/ void config_form_t::change(int delta)
{
	kobo_form_t::change(delta);

	if(!selected())
		return;

	if(selected()->tag & OS_CLOSE)
	{
		if(delta == 0)
		{
			sound.ui_play(SOUND_UI_OK);
			setstatus(OS_CLOSE);
		}
	}
	else if(selected()->tag & OS_CANCEL)
	{
		if(delta == 0)
		{
			sound.ui_play(SOUND_UI_CANCEL);
			undo();
			setstatus(OS_CLOSE);
		}
	}
	else
		sound.ui_play(SOUND_UI_TICK);

	if(selected()->user)
		prf->changed = 1;

	if(selected()->tag & OS_CLOSE)
		prepare_to_apply();

	setstatus(selected()->tag & (OS_RELOAD | OS_RESTART | OS_UPDATE));

	if(selected()->tag & OS_REBUILD)
	{
		int sel = selected_index();
		build_all();
		select(sel);
	}
}

/* virtual */void config_form_t::prepare_to_apply()
{
}
