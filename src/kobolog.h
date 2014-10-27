/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - Logger definitions
------------------------------------------------------------
 * Copyright (C) 2003, 2009 David Olofson
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

#ifndef	KOBOLOG_H
#define	KOBOLOG_H

#include "logger.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Critical error messages: "Huston, we have a problem." */
#define	CELOG	5

/* Debug messages; level 2: Detailed */
#define	D2LOG	6

/* Debug messages; level 3: Deep Hack Mode stuff */
#define	D3LOG	7

#ifdef __cplusplus
};
#endif

#endif	/* KOBOLOG_H */
