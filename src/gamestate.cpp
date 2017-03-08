/*(LGPLv2.1)
------------------------------------------------------------
	Game State Manager
------------------------------------------------------------
 * Copyright 2001-2003, 2009 David Olofson
 * Copyright 2015, 2017 David Olofson (Kobo Redux)
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

#include <stdio.h>
#include "logger.h"
#include "gamestate.h"

/*----------------------------------------------------------
	gamestate_t
----------------------------------------------------------*/

gamestate_t::gamestate_t()
{
	prev = NULL;
	manager = NULL;
	name = "<unnamed gamestate>";
	info = NULL;
	pushed = false;
}


gamestate_t::~gamestate_t()			{}


void gamestate_t::pop()
{
	gamestatemanager_t *mgr = manager;
	tail_pop();
	if(mgr->current())
		mgr->current()->reenter();
}


void gamestate_t::tail_pop()
{
	if(!pushed)
	{
		log_printf(ELOG, "GameStateManager: Tried to pop state '%s', "
				"which is not on the stack!\n", name);
		return;
	}
	if(manager->current() != this)
	{
		log_printf(ELOG, "GameStateManager: Tried to pop state '%s', "
				"which is not on top of the stack!\n", name);
		return;
	}
	manager->tail_pop();
}


void gamestate_t::enter()			{}
void gamestate_t::leave()			{}
void gamestate_t::yield()			{}
void gamestate_t::reenter()			{}
void gamestate_t::rebuild()			{}
void gamestate_t::press(gc_targets_t button)	{}
void gamestate_t::release(gc_targets_t button)	{}
void gamestate_t::pos(int x, int y)		{}
void gamestate_t::delta(int dx, int dy)		{}
void gamestate_t::frame()			{}
void gamestate_t::pre_render()			{}
void gamestate_t::post_render()			{}



/*----------------------------------------------------------
	gamestatemanager_t
----------------------------------------------------------*/

gamestatemanager_t::gamestatemanager_t()
{
	top = NULL;
}


gamestatemanager_t::~gamestatemanager_t()
{
}


void gamestatemanager_t::change(gamestate_t *gs)
{
	if(gs->pushed)
	{
		log_printf(ELOG, "GameStateManager: Tried to change to state "
				"'%s', which is already on the stack!\n",
				gs->name);
		return;
	}

	gs->manager = this;	// NOTE: May well be NULL if 'this' is static!
	gs->pushed = true;

	gamestate_t *oldtop = top;
	if(top)
		top = top->prev;
	gs->prev = top;
	top = gs;
	log_printf(DLOG, "GameStateManager: Switched to '%s'\n", gs->name);

	if(oldtop)
	{
		oldtop->leave();
		oldtop->manager = NULL;
		oldtop->pushed = false;
	}
	if(top)
		top->enter();
}


void gamestatemanager_t::tail_push(gamestate_t *gs)
{
	if(gs->pushed)
	{
		log_printf(ELOG, "GameStateManager: Tried to push state '%s' "
				"more than once!\n", gs->name);
		return;
	}

	gs->manager = this;
	gs->pushed = true;

	gs->prev = top;
	top = gs;
	log_printf(DLOG, "GameStateManager: Tail pushed state '%s'\n",
			gs->name);

	if(top)
		top->enter();
}

void gamestatemanager_t::push(gamestate_t *gs)
{
	if(gs->pushed)
	{
		log_printf(ELOG, "GameStateManager: Tried to push state '%s' "
				"more than once!\n", gs->name);
		return;
	}

	gs->manager = this;
	gs->pushed = true;

	gamestate_t *oldtop = top;
	gs->prev = top;
	top = gs;
	log_printf(DLOG, "GameStateManager: Pushed state '%s'\n", gs->name);

	if(oldtop)
		oldtop->yield();
	if(top)
		top->enter();
}


void gamestatemanager_t::tail_pop()
{
	gamestate_t *oldtop = top;
	if(top)
		top = top->prev;
	if(oldtop)
	{
		oldtop->leave();
		oldtop->manager = NULL;
		oldtop->pushed = false;
		log_printf(DLOG, "GameStateManager: Popped state '%s'\n",
				oldtop->name);
	}
}


void gamestatemanager_t::pop()
{
	tail_pop();
	if(top)
		top->reenter();
}


gamestate_t *gamestatemanager_t::current()
{
	return top;
}


gamestate_t *gamestatemanager_t::previous()
{
	if(top)
		return top->prev;
	else
		return NULL;
}


void gamestatemanager_t::press(SDL_Keysym sym)
{
	if(top)
		top->press(gamecontrol.map(sym));
}


void gamestatemanager_t::release(SDL_Keysym sym)
{
	if(top)
		top->release(gamecontrol.map(sym));
}


void gamestatemanager_t::pressbtn(gc_targets_t button)
{
	if(top)
		top->press(button);
}


void gamestatemanager_t::releasebtn(gc_targets_t button)
{
	if(top)
		top->release(button);
}


void gamestatemanager_t::pos(int x, int y)
{
	if(top)
		top->pos(x, y);
}


void gamestatemanager_t::delta(int dx, int dy)
{
	if(top)
		top->delta(dx, dy);
}


void gamestatemanager_t::rebuild()
{
	if(top)
		top->rebuild();
}


void gamestatemanager_t::frame()
{
	if(top)
		top->frame();
}


void gamestatemanager_t::pre_render()
{
	if(top)
		top->pre_render();
}


void gamestatemanager_t::post_render()
{
	if(top)
		top->post_render();
}
