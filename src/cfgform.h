/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001, 2003, 2009 David Olofson
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

#ifndef	_CONFIG_FORM_H_
#define	_CONFIG_FORM_H_

#include "form.h"
#include "prefs.h"

enum
{
	OS_CLOSE =		0x0001,
	OS_CANCEL =		0x0002,
	OS_REBUILD =		0x0004,
	OS_ACTION =		0x000f,

	OS_RELOAD_GRAPHICS =	0x0010,
	OS_RELOAD =		0x0070,

	OS_RESTART_VIDEO =	0x0100,
	OS_RESTART_AUDIO =	0x0200,
	OS_RESTART_INPUT =	0x0800,
	OS_RESTART_LOGGER =	0x0080,
	OS_RESTART =		0x0f80,

	OS_UPDATE_AUDIO =	0x1000,
	OS_UPDATE_ENGINE =	0x2000,
	OS_UPDATE_SCREEN =	0x4000,
	OS_UPDATE =		0xf000
};


class config_form_t : public kobo_form_t
{
  protected:
	prefs_t		*prf;
	prefs_t		prfbak;
	int		stat;
	int		undostat;
  public:
	config_form_t(gfxengine_t *e);
	void open(prefs_t *p);
	virtual void build();		// <-- Override this!
	//...and this, if you need. Normally,
	//it just sets the status to 0.
	void close();
	int status();
	void setstatus(int mask);
	void clearstatus(int mask);
	void undo();
	void change(int delta);
};

extern int global_status;

#endif 	//_CONFIG_FORM_H_
