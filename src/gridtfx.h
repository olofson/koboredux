/*(GPLv2)
------------------------------------------------------------
	gridtfx.h - Grid Transition Effects
------------------------------------------------------------
 * Copyright 2017 David Olofson (Kobo Redux)
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

#ifndef KOBO_GRIDTFX_H
#define KOBO_GRIDTFX_H

#include "window.h"

class KOBO_GridTFX
{
	window_t	*target;
	int		tilebank;
	int		levels;
	float		current_state;	// [0, 1]
	bool		target_state;
	Uint32		t0;		// Transition start time
	Uint32		duration;	// Transition duration
	Uint32		style;
	Uint32		tileset;
	float fxbias(float x, float y);
  public:
	KOBO_GridTFX();
	~KOBO_GridTFX();
	void Target(window_t *t);
	void Tiles(int bank, int tiles_per_set);
	void Render();
	void State(bool st, float dur = 1.0f);
	bool State();
	float Progress();
	bool Done();
};

#endif /* KOBO_GRIDTFX_H */
