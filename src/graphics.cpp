/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2016 David Olofson (Kobo Redux)
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

#include "graphics.h"


#define	KOBO_DEFS(x)	"B_" #x,
const char *kobo_gfxbanknames[] =
{
	KOBO_ALLGFXBANKS
};
#undef	KOBO_DEFS


#define	KOBO_DEFS(x)	"P_" #x,
const char *kobo_palettenames[] =
{
	KOBO_ALLPALETTES
};
#undef	KOBO_DEFS

#define	KOBO_DEFS(x)	"D_" #x,
const char *kobo_datanames[] =
{
	KOBO_ALLTDITEMS
};
#undef	KOBO_DEFS
