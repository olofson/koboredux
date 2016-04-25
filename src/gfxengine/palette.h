/*(LGPLv2.1)
----------------------------------------------------------------------
	palette.h - GIMP .gpl palette loader
----------------------------------------------------------------------
 * Copyright 2015, 2016 David Olofson (Kobo Redux)
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

#ifndef	_PALETTE_H_
#define	_PALETTE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct GFX_palette {
	unsigned	nentries;
	uint32_t	*entries;
} GFX_palette;

GFX_palette *gfx_palette_new(unsigned initsize);
GFX_palette *gfx_palette_parse(const char *data);
GFX_palette *gfx_palette_load(const char *path);

void gfx_palette_set(GFX_palette *p, unsigned i, uint32_t color);

static inline uint32_t gfx_palette_get(GFX_palette *p, unsigned i)
{
	if(i >= p->nentries)
		return p->entries[0];
	return p->entries[i];
}

void gfx_palette_free(GFX_palette *p);

#ifdef __cplusplus
};
#endif

#endif /* _PALETTE_H_ */
