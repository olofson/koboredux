/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2001-2003 David Olofson
 * Copyright 2002 Jeremy Sheeley
 * Copyright 2007, 2009 David Olofson
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

#ifndef	_KOBO_STATES_H_
#define	_KOBO_STATES_H_

#include "gamestate.h"
#include "gamectl.h"
#include "form.h"
#include "kobo.h"
#include "options.h"
#include "campaign.h"

#define	MENU_TAG_OK	99
#define	MENU_TAG_CANCEL	100


extern gamestatemanager_t gsm;


class kobo_basestate_t : public gamestate_t
{
  protected:
	int		song;
public:
	kobo_basestate_t();
	void enter();
	void reenter();
	void pre_render();
	void post_render();

	// Smooth transition alternatives to change(), push(), and pop().
	// These will initiate a visual transition effect, and then push a
	// delay state, which forwards rendering calls to the current state, so
	// that it keeps running behind the transition effect layer. When the
	// transition is "closed" (screen covered), the actual change(),
	// push(), or pop() is performed, and then the "opening" part of the
	// transition runs on top of the new state.
	void transition_change(gamestate_t *to, KOBO_TransitionStyle trs);
	void transition_push(gamestate_t *to, KOBO_TransitionStyle trs);
	void transition_pop(KOBO_TransitionStyle trs);
};


/*----------------------------------------------------------
	Transition Delay (for the transition_*() methods)
----------------------------------------------------------*/

class st_transition_delay_t : public kobo_basestate_t
{
	Uint32		timeout;
	gamestate_t	*to_state;	// Where to go when "faded" out
	bool		change_to;	// true: change(); false: push()
	KOBO_TransitionStyle trstyle;
  public:
	st_transition_delay_t();
	void enter();
	void frame();
	void pre_render();
	void post_render();
	void start_transition(KOBO_TransitionStyle trs);
	void start_change(gamestate_t *to);
	void start_push(gamestate_t *to);
	void start_pop();
};

extern st_transition_delay_t st_transition_delay;


/*----------------------------------------------------------
	Intro
----------------------------------------------------------*/

enum KOBO_IntroPages
{
	KOBO_IP_TITLE = 0,
	KOBO_IP_INSTRUCTIONS,
	KOBO_IP_TITLE2,
	KOBO_IP_CREDITS,
	KOBO_IP__COUNT
};

extern int kobo_intro_durations[KOBO_IP__COUNT];

class st_intro_t : public kobo_basestate_t
{
	int	page;
	int	timer, start_time;
	int	duration;
	void init_page();
  public:
	st_intro_t();
	void set_page(KOBO_IntroPages pg)	{ page = pg; }
	void enter();
	void yield();
	void reenter();
	void leave();
	void press(gc_targets_t button);
	void frame();
	void pre_render();
	void post_render();
};

extern st_intro_t st_intro;


/*----------------------------------------------------------
	Long Credits
----------------------------------------------------------*/

class st_long_credits_t : public kobo_basestate_t
{
  protected:
	int		timer, start_time;
  public:
	st_long_credits_t();
	void enter();
	void leave();
	void press(gc_targets_t button);
	void pre_render();
	void post_render();
};

extern st_long_credits_t st_long_credits;


/*----------------------------------------------------------
	In-game
----------------------------------------------------------*/

class st_game_t : public kobo_basestate_t
{
	int	g_slot;
	int	g_stage;
	int	g_skill;
  public:
	st_game_t();
	void set_slot(int slot)		{ g_slot = slot; }
	void set_stage(int stage)	{ g_stage = stage; }
	void set_skill(int skill)	{ g_skill = skill; }
	void enter();
	void leave();
	void yield();
	void reenter();
	void press(gc_targets_t button);
	void frame();
	void post_render();
};

extern st_game_t st_game;


class st_rewind_t : public kobo_basestate_t
{
  public:
	st_rewind_t();
	void enter();
	void leave();
	void yield();
	void reenter();
	void press(gc_targets_t button);
	void frame();
	void post_render();
};

extern st_rewind_t st_rewind;


class st_replay_t : public kobo_basestate_t
{
	int	rp_slot;
	int	rp_stage;
  public:
	st_replay_t();
	void setup(int slot, int stage)
	{
		rp_slot = slot;
		rp_stage = stage;
	}
	void enter();
	void leave();
	void yield();
	void reenter();
	void press(gc_targets_t button);
	void frame();
	void post_render();
};

extern st_replay_t st_replay;


class st_pause_game_t : public kobo_basestate_t
{
  public:
	st_pause_game_t();
	void enter();
	void press(gc_targets_t button);
	void frame();
	void post_render();
};

extern st_pause_game_t st_pause_game;


class st_get_ready_t : public kobo_basestate_t
{
	int	start_time, frame_time;
	int	countdown;
  public:
	st_get_ready_t();
	void enter();
	void reenter();
	void press(gc_targets_t button);
	void frame();
	void post_render();
};

extern st_get_ready_t st_get_ready;


class st_game_over_t : public kobo_basestate_t
{
	int	start_time, frame_time;
	int	countdown;
  public:
	st_game_over_t();
	void enter();
	void reenter();
	void press(gc_targets_t button);
	void frame();
	void post_render();
};

extern st_game_over_t st_game_over;


/*----------------------------------------------------------
	Menu Base
----------------------------------------------------------*/

/*
 * Alt 1:
 *   Derive from this and override build()
 *
 *   The tags of the widgets registered by build() will be
 *   passed to select() of your st_menu_base_t derivate when
 *   the widgets are activated.
 *
 *   BTN_EXIT and BTN_CLOSE will both have the same effect
 *   as activating a widget with tags 0 - which means that
 *   tags 0 is effectively the universal "exit code". Handle
 *   this event if you like, but keep in mind that the menu
 *   will be closed and destroyed, and the state popped
 *   automatically afterwards.
 *
 * Alt 2: (For advanced custom forms.)
 *   Ignore menu_base_t and plug in any other kobo_form_t
 *   derivate instead.
 *
 *   Your custom form is expected to handle input via the
 *   normal select()/change() API of ct_form_t, and widget
 *   tags are passed to st_menu_base_t::select() whenever a
 *   widget is activated.
 *
 *   If your custom form doesn't use tags the way
 *   st_menu_base_t expects, override translate() to translate
 *   according to these rules:
 *	No action:		-1
 *	Exit/close/cancel:	0
 *	Any other functions:	>0
 *
 * Either way:
 *   Extra communication between the st_menu_base_t::select()
 *   handler and your custom menu will have to be implemented
 *   in custom ways. See the options forms for examples.
 *
 *   Of course, other virtual methods may still be overridden
 *   just as with the normal Kobo forms and states this
 *   wrapper inherits from - just keep in mind to call the
 *   inherited versions to keep things working!
 *
 *   If you want to control event sounds yourself, set the
 *   'sounds' field to 0. (Inside open() is soon enough.)
 */
class menu_base_t : public kobo_form_t
{
  public:
	menu_base_t(gfxengine_t *e) : kobo_form_t(e) { }
	void open();
	void close();
};

/*
 * Derive from this and override open(), close() and select().
 *
 * open() should instantiate, open and return your
 * kobo_form_t derivate.
 *
 * close() should clean up any extra stuff. (The form will be
 * deleted in st_menu_base_t's destructor.)
 *
 * select() is your "widget activated" event handler.
 *
 * translate() is optional, and is used for translating
 * widget a tag depending on, for example, what key is used to
 * "activate" the widget. translate() is plugged in before
 * select(), and is expected to return "new" tag codes that
 * are passed on to select().
 *
 * Default actions:
 *	st_menu_base_t implements default widget actions for
 *	some keys. These actions are performed after
 *	translate() is called, but translate() may temporarily
 *	disable this feature by setting the do_default_action
 *	member to 0.
 *
 * Override escape() if you want to do something special when
 * the menu wants to exit the "oops, forget about this" way.
 * Note that this is *not* called if your select() handler
 * explicitly makes the menu exit. Also note that select()
 * will still be called with a 0 argument after escape() has
 * been called.
 */
class st_menu_base_t : public kobo_basestate_t
{
  protected:
	kobo_form_t	*form;
	int		sounds;
	int		do_default_action;
  public:
	st_menu_base_t();
	~st_menu_base_t();
	void enter();
	void rebuild();
	void leave();
	void pos(int x, int y);
	void press(gc_targets_t button);
	void frame();
	void post_render();

	virtual kobo_form_t *open() = 0;
	virtual void close()	{ ; }
	virtual int translate(int tag, int button);
	virtual void select(int tag) = 0;
	virtual void escape()	{ ; }
};


/*----------------------------------------------------------
	Main Menu
----------------------------------------------------------*/

class main_menu_t : public menu_base_t
{
  public:
	int	start_level;
	main_menu_t(gfxengine_t *e) : menu_base_t(e) { }
	virtual void buildStartLevel();
	void build();
	void rebuild();
};

class st_main_menu_t : public st_menu_base_t
{
	main_menu_t	*menu;
  public:
	st_main_menu_t()	{	name = "main_menu"; }
	kobo_form_t *open();
	void reenter();
	void press(gc_targets_t button);
	int translate(int tag, int button);
	void select(int tag);
};

extern st_main_menu_t st_main_menu;


/*----------------------------------------------------------
	Campaign Selector
----------------------------------------------------------*/

class campaign_menu_t : public menu_base_t
{
	const char		*header;
	bool			newgame;
	bool			view_replay;
	char			tdbuf[128];
  public:
	campaign_menu_t(gfxengine_t *e);
	const char *timedate(time_t *t);
	void colonalign();
	void setup(const char *hdr, bool new_game, bool view);
	void build();
	void rebuild();
};

class st_campaign_menu_t : public st_menu_base_t
{
	campaign_menu_t	*menu;
	const char	*header;
	bool		newgame;
	bool		view_replay;
  public:
	st_campaign_menu_t()	{	name = "campaign_selector"; }
	kobo_form_t *open();
	void setup(const char *header, bool new_game, bool view = false);
	void press(gc_targets_t button);
	void select(int tag);
};

extern st_campaign_menu_t st_campaign_menu;


/*----------------------------------------------------------
	Skill Menu
----------------------------------------------------------*/

class skill_menu_t : public menu_base_t
{
	int skill;
  public:
	skill_menu_t(gfxengine_t *e) : menu_base_t(e) { }
	void set_skill(int sk)
	{
		skill = sk;
	};
	void build();
	void rebuild();
};

class st_skill_menu_t : public st_menu_base_t
{
	skill_menu_t	*menu;
  public:
	st_skill_menu_t()	{	name = "skill_menu"; }
	kobo_form_t *open();
	void enter();
	void press(gc_targets_t button);
	void select(int tag);
};

extern st_skill_menu_t st_skill_menu;


/*----------------------------------------------------------
	Options Main
----------------------------------------------------------*/
class options_main_t : public menu_base_t
{
  public:
	options_main_t(gfxengine_t *e) : menu_base_t(e) { }
	void build();
};

class st_options_main_t : public st_menu_base_t
{
  public:
	st_options_main_t()	{ name = "options_main"; }
	kobo_form_t *open();
	void select(int tag);
};

extern st_options_main_t st_options_main;


/*----------------------------------------------------------
	Options Base
----------------------------------------------------------*/
class st_options_base_t : public st_menu_base_t
{
	config_form_t	*cfg_form;
	void check_update();
  public:
	st_options_base_t()
	{
		cfg_form = NULL;
	}
	virtual config_form_t *oopen() = 0;
	kobo_form_t *open();
	void enter();
	void close();
	void select(int tag);
	void press(gc_targets_t button);
	void escape();
};


/*----------------------------------------------------------
	Options Submenus
----------------------------------------------------------*/
class st_options_system_t : public st_options_base_t
{
  public:
	st_options_system_t()	{ name = "options_system"; }
	config_form_t *oopen()	{ return new system_options_t(gengine); }
};

extern st_options_system_t st_options_system;


class st_options_video_t : public st_options_base_t
{
  public:
	st_options_video_t()	{ name = "options_video"; }
	config_form_t *oopen()	{ return new video_options_t(gengine); }
};

extern st_options_video_t st_options_video;


class st_options_controls_t : public st_options_base_t
{
  public:
	st_options_controls_t()	{ name = "options_controls"; }
	config_form_t *oopen()	{ return new controls_options_t(gengine); }
};

extern st_options_controls_t st_options_controls;


// NOTE: The "Sound" category is covered by the Audio menu
class st_options_audio_t : public st_options_base_t
{
  public:
	st_options_audio_t()	{ name = "options_audio"; }
	config_form_t *oopen()	{ return new audio_options_t(gengine); }
};

extern st_options_audio_t st_options_audio;


class st_options_interface_t : public st_options_base_t
{
  public:
	st_options_interface_t()	{ name = "options_interface"; }
	config_form_t *oopen()	{ return new interface_options_t(gengine); }
};

extern st_options_interface_t st_options_interface;


class st_options_game_t : public st_options_base_t
{
  public:
	st_options_game_t()	{ name = "options_game"; }
	config_form_t *oopen()	{ return new game_options_t(gengine); }
};

extern st_options_game_t st_options_game;


class st_options_cheat_t : public st_options_base_t
{
  public:
	st_options_cheat_t()	{ name = "options_cheat"; }
	config_form_t *oopen()	{ return new cheat_options_t(gengine); }
};

extern st_options_cheat_t st_options_cheat;


class st_options_debug_t : public st_options_base_t
{
  public:
	st_options_debug_t()	{ name = "options_debug"; }
	config_form_t *oopen()	{ return new debug_options_t(gengine); }
};

extern st_options_debug_t st_options_debug;


/*----------------------------------------------------------
	More Options
----------------------------------------------------------*/
class options_more_t : public menu_base_t
{
  public:
	options_more_t(gfxengine_t *e) : menu_base_t(e) { }
	void build();
};

class st_options_more_t : public st_menu_base_t
{
  public:
	st_options_more_t()	{ name = "options_more"; }
	kobo_form_t *open();
	void select(int tag);
};

extern st_options_more_t st_options_more;


/*----------------------------------------------------------
	Demo Over Page
----------------------------------------------------------*/

#ifdef KOBO_DEMO

class demo_over_t : public menu_base_t
{
  public:
	demo_over_t(gfxengine_t *e) : menu_base_t(e) { }
	void build();
};

class st_demo_over_t : public st_menu_base_t
{
  public:
	st_demo_over_t()	{ name = "demo_over"; }
	kobo_form_t *open();
	void leave();
	void select(int tag);
};

extern st_demo_over_t st_demo_over;

#endif


/*----------------------------------------------------------
	Requesters
----------------------------------------------------------*/

class yesno_menu_t : public menu_base_t
{
  public:
	yesno_menu_t(gfxengine_t *e) : menu_base_t(e) { }
	void build();
	void rebuild();
};

class st_yesno_base_t : public st_menu_base_t
{
	yesno_menu_t	*menu;
  protected:
	const char	*msg;
  public:
	kobo_form_t *open();
	void reenter();
	void press(gc_targets_t button);
	void frame();
	void post_render();
};


class st_exit_t : public kobo_basestate_t
{
  public:
	st_exit_t() { };
	void enter();
};

extern st_exit_t st_exit;


class st_ask_exit_t : public st_yesno_base_t
{
  public:
	st_ask_exit_t();
	void select(int tag);
};

extern st_ask_exit_t st_ask_exit;


class st_ask_abort_game_t : public st_yesno_base_t
{
  public:
	st_ask_abort_game_t();
	void leave();
	void select(int tag);
};

extern st_ask_abort_game_t st_ask_abort_game;


class st_ask_overwrite_campaign_t : public st_yesno_base_t
{
  public:
	st_ask_overwrite_campaign_t();
	void select(int tag);
};

extern st_ask_overwrite_campaign_t st_ask_overwrite_campaign;


/*----------------------------------------------------------
	Critical Error Screen
----------------------------------------------------------*/

class st_error_t : public kobo_basestate_t
{
	int	start_time, frame_time;
	const char	*msg[2];
  public:
	st_error_t();
	void message(const char *error, const char *hint);
	void enter();
	void press(gc_targets_t button);
	void frame();
	void post_render();
};

extern st_error_t st_error;

#endif	//_KOBO_STATES_H_
