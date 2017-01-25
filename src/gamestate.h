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

#ifndef	_GAMESTATEMANAGER_H_
#define	_GAMESTATEMANAGER_H_

#include "gamectl.h"

class gamestatemanager_t;

class gamestate_t
{
	friend class gamestatemanager_t;
  private:
	gamestate_t		*next;		//LIFO stack
	gamestatemanager_t	*manager;
  protected:
	const char		*name;
	void pop();

	virtual void press(gc_targets_t button);
	virtual void release(gc_targets_t button);
	virtual void pos(int x, int y);
	virtual void delta(int dx, int dy);

	virtual void enter();
	virtual void leave();
	virtual void yield();
	virtual void reenter();
	virtual void rebuild();

	virtual void frame();		//Control system stuff
	virtual void pre_render();	//Background rendering
	virtual void post_render();	//Foreground rendering
  public:
	gamestate_t();
	virtual ~gamestate_t();
};


class gamestatemanager_t
{
  private:
	gamestate_t	*top;		//Stack top
  public:
	gamestatemanager_t();
	~gamestatemanager_t();

	// Event router interface
	void press(SDL_Keysym sym);
	void release(SDL_Keysym sym);
	void pressbtn(gc_targets_t button);
	void releasebtn(gc_targets_t button);
	void pos(int x, int y);
	void delta(int dx, int dy);

	// CS frame and rendering callbacks
	void rebuild();
	void frame();
	void pre_render();
	void post_render();

	// State management
	void change(gamestate_t *gs);
	void push(gamestate_t *gs);
	void pop();
	gamestate_t *current();
	gamestate_t *previous();
};

#endif /*_GAMESTATEMANAGER_H_*/
