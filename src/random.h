/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2002, 2007, 2009 David Olofson
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

#ifndef _KOBO_RANDOM_H_
#define _KOBO_RANDOM_H_

#include "SDL.h"

class rand_num_t
{
	Uint32 seed;
  public:
	void init(Uint32 _seed = 0)
	{
		if(_seed)
			seed = _seed;
		else
			seed = SDL_GetTicks();
	}
	Uint32 get_seed()	{ return seed; }
	Uint32 get()
	{
		seed *= 1566083941UL;
		seed++;
		seed &= 0xffffffffUL;
		return seed;
	}
	Uint32 get(Uint32 bit)
	{
		seed *= 1566083941UL;
		seed++;
		seed &= 0xffffffffUL;
		return (seed >> (32 - bit));
	}
};

//This instance is *ONLY* for map generation and
//pure game AI stuff!!! Stealing numbers from here
//will screw up demo playback totally, as demos
//record only the seed used for each level; not
//every random number used.
extern rand_num_t gamerand;

//Use this "public" random number generator for
//other stuff, like explosion effects, and things
//that may pick different amounts of numbers
//depending on engine version, configuration,
//computer speed and the like.
extern rand_num_t pubrand;

#endif //_KOBO_RANDOM_H_
