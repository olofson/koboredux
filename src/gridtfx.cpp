/*(GPLv2)
------------------------------------------------------------
	gridtfx.cpp - Grid Transition Effects
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

#include "kobo.h"
#include "gridtfx.h"
#include "random.h"

KOBO_GridTFX::KOBO_GridTFX()
{
	target = NULL;
	tilebank = -1;
	levels = 16;
	current_state = 0.0f;
	target_state = false;
	t0 = SDL_GetTicks();
	duration = 0;
	style = pubrand.get();
	tileset = pubrand.get();
}


KOBO_GridTFX::~KOBO_GridTFX()
{
}


void KOBO_GridTFX::Target(window_t *t)
{
	target = t;
}


void KOBO_GridTFX::Tiles(int bank, int tiles_per_set)
{
	tilebank = bank;
	levels = tiles_per_set;
}


enum KOBO_GridTFXStyles
{
	KOBO_GRIDTFX_STYLE_0 = 0,
	KOBO_GRIDTFX_STYLE_1,
	KOBO_GRIDTFX_STYLE_2,
	KOBO_GRIDTFX_STYLE_3,
	KOBO_GRIDTFX_STYLE_4,

	KOBO_GRIDTFX_STYLES
};


float KOBO_GridTFX::fxbias(float x, float y)
{
	float z;
	switch(style % KOBO_GRIDTFX_STYLES)
	{
	  default:
	  case KOBO_GRIDTFX_STYLE_0:
		z = 0.0f;
		break;
	  case KOBO_GRIDTFX_STYLE_1:
		z = (x + y) * 0.5f;
		break;
	  case KOBO_GRIDTFX_STYLE_2:
		z = ((1.0f - x) + y) * 0.5f;
		break;
	  case KOBO_GRIDTFX_STYLE_3:
		z = 1.0f - (x + y) * 0.5f;
		break;
	  case KOBO_GRIDTFX_STYLE_4:
		z = (x + (1.0f - y)) * 0.5f;
		break;
	}
	return z * z;
}


void KOBO_GridTFX::Render()
{
	float p = Progress();
	if(target_state)
		current_state = p;
	else
		current_state = 1.0f - p;
	if(current_state == 0.0f)
		return;

	int tilesets = 1;
	int tw = 16;
	int th = 16;
	int tile0 = 0;
	s_bank_t *b = s_get_bank(gengine->get_gfx(), tilebank);
	if(b && levels)
		tilesets = (b->max + 1) / levels;
	if(b && tilesets)
	{
		tile0 = tileset % tilesets;
		tw = b->w;
		th = b->h;
	}
	else
		target->foreground(0x666666);

	float zoffs = 2.0f * current_state - 1.0f;
	int w = target->width();
	int h = target->height();
	for(int y = 0; y < h; y += th)
		for(int x = 0; x < w; x += tw)
		{
			float z = fxbias((float)x / w, (float)y / h) + zoffs;
			if(z < 0.0f)
				z = 0.0f;
			else if(z > 1.0f)
				z = 1.0f;
			if(b)
			{
				int sc = levels * (1.0f - z) + 0.5f;
				if(sc < levels)
					target->sprite(x, y, tilebank,
							tile0 + sc);
			}
			else
			{
				int scx = tw * z * 0.5f + 0.5f;
				int scy = th * z * 0.5f + 0.5f;
				target->fillrect(x + (tw / 2) - scx,
						y + (th / 2) - scy,
						scx * 2, scy * 2);
			}
		}
}


void KOBO_GridTFX::State(bool st, float dur)
{
	float skip = 0.0f;
	if(Done())
	{
		style = pubrand.get();
		if(!target_state)
			tileset = pubrand.get();
		if(target_state == st)
			return;
	}
	else
	{
		if(st)
			skip = current_state;
		else
			skip = 1.0f - current_state;
	}
	target_state = st;
	duration = dur * 1000.0f;
	t0 = SDL_GetTicks() - duration * skip;
	if(!duration)
		current_state = target_state ? 1.0f : 0.0f;
}


bool KOBO_GridTFX::State()
{
	return target_state;
}


float KOBO_GridTFX::Progress()
{
	if(!duration)
		return 1.0f;
	Uint32 t = SDL_GetTicks();
	if(SDL_TICKS_PASSED(t, t0 + duration))
		return 1.0f;
	return ((float)(t - t0)) / duration;
}


bool KOBO_GridTFX::Done()
{
	return Progress() == 1.0f;
}
