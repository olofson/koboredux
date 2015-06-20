/*(LGPLv2.1)
----------------------------------------------------------------------
	spinplanet.h - Spinning planet effect
----------------------------------------------------------------------
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

#ifndef	_SPINPLANET_H_
#define	_SPINPLANET_H_

#include "window.h"

enum spinplanet_modes_t
{
	SPINPLANET_OFF,
	SPINPLANET_STATIC,
	SPINPLANET_SPIN
};

class spinplanet_t : public stream_window_t
{
	int sbank[2], sframe[2];
	int tlayer;	// Engine scroll layer to track
	spinplanet_modes_t mode;
	void refresh_static();
  public:
	spinplanet_t(gfxengine_t *e);
	virtual ~spinplanet_t();
	void clear();
	void set_source(int src, int bank, int frame);
	void set_mode(spinplanet_modes_t md);
	void track_layer(int lr)	{ tlayer = lr; }
	void refresh(SDL_Rect *r);
	void render(SDL_Rect *r);
};

#endif // _SPINPLANET_H_
