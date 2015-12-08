/*(LGPLv2.1)
------------------------------------------------------------
	Game State Manager
------------------------------------------------------------
 * Copyright 2001-2003, 2009 David Olofson
 * Copyright 2015 David Olofson (Kobo Redux)
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

#define	DBG(x)

#include <stdio.h>
#include "logger.h"
#include "gamestate.h"

/*----------------------------------------------------------
	gamestate_t
----------------------------------------------------------*/

gamestate_t::gamestate_t()
{
	next = 0;
	manager = 0;
}

gamestate_t::~gamestate_t()			{}

void gamestate_t::pop()
{
	if(!manager)
		return;
	//This is far from fool-proof, but it should
	//handle the most common cases of "trying to
	//pop 'this' more than once"...
	if(manager->current() == this)
		manager->pop();
	else
		log_printf(WLOG, "GameStateManager: "
				"WARNING: Tried to pop state more than once!\n");
}

void gamestate_t::enter()			{}
void gamestate_t::leave()			{}
void gamestate_t::yield()			{}
void gamestate_t::reenter()			{}
void gamestate_t::press(buttons_t button)	{}
void gamestate_t::release(buttons_t button)	{}
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
	top = 0;
}


gamestatemanager_t::~gamestatemanager_t()
{
}


void gamestatemanager_t::change(gamestate_t *gs)
{
	gs->manager = this;

	gamestate_t *oldtop = top;
	if(top)
		top = top->next;
	gs->next = top;
	top = gs;

	if(oldtop)
	{
		oldtop->leave();
		oldtop->manager = 0;
	}
	if(top)
		top->enter();

	log_printf(DLOG, "GameStateManager: Switched to '%s'\n", gs->name);
}


void gamestatemanager_t::push(gamestate_t *gs)
{
	gs->manager = this;

	gamestate_t *oldtop = top;
	gs->next = top;
	top = gs;

	if(oldtop)
		oldtop->yield();
	if(top)
		top->enter();

	log_printf(DLOG, "GameStateManager: Pushed state '%s'\n", gs->name);
}


void gamestatemanager_t::pop()
{
	gamestate_t *oldtop = top;
	if(top)
		top = top->next;
	if(oldtop)
	{
		oldtop->leave();
		oldtop->manager = 0;
		log_printf(DLOG, "GameStateManager: Popped state '%s'\n", oldtop->name);
	}
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
		return top->next;
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


void gamestatemanager_t::pressbtn(buttons_t button)
{
	if(top)
		top->press(button);
}


void gamestatemanager_t::releasebtn(buttons_t button)
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
