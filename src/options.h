/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001, 2003, 2007, 2009 David Olofson
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

#ifndef	_KOBO_OPTIONS_H_
#define	_KOBO_OPTIONS_H_

#include "cfgform.h"
#include "vidmodes.h"

#define	MAX_AUDIO_DRIVERS	20

class system_options_t : public config_form_t
{
  public:
	system_options_t(gfxengine_t *e) : config_form_t(e) { }
	void build();
};

class video_options_t : public config_form_t
{
	int showmodes;
	int showlow;
	int firstbuild;
  public:
	video_options_t(gfxengine_t *e) : config_form_t(e)
	{
		showmodes = VMM_PC;
		showlow = 0;
		firstbuild = 1;
	}
	void build();
	void close();
};

class controls_options_t : public config_form_t
{
  public:
	controls_options_t(gfxengine_t *e) : config_form_t(e) { }
	void build();
};

class audio_options_t : public config_form_t
{
	int driver;
	const char *drivers[MAX_AUDIO_DRIVERS];
  public:
	audio_options_t(gfxengine_t *e) : config_form_t(e) { }
	void build();
	void prepare_to_apply();
};

class interface_options_t : public config_form_t
{
  public:
	interface_options_t(gfxengine_t *e) : config_form_t(e) { }
	void build();
};

class game_options_t : public config_form_t
{
  public:
	game_options_t(gfxengine_t *e) : config_form_t(e) { }
	void build();
};

class cheat_options_t : public config_form_t
{
  public:
	cheat_options_t(gfxengine_t *e) : config_form_t(e) { }
	void build();
};

class debug_options_t : public config_form_t
{
  public:
	debug_options_t(gfxengine_t *e) : config_form_t(e) { }
	void build();
};

#endif 	//_KOBO_OPTIONS_H_
