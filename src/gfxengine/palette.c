/*(LGPLv2.1)
----------------------------------------------------------------------
	palette.c - GIMP .gpl palette loader
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

#include "palette.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Find 'tk' in 's'. Return NULL if a NUL terminator is found instead. */
static const char *read_until(const char *s, char tk)
{
	while(*s != tk)
		if(!*s)
			return NULL;
		else
			++s;
	return s;
}


static const char *parse_header(GFX_palette *p, const char *s)
{
	const char *e;

	/* "GIMP Palette" */
	if(!(e = read_until(s, 10)))
		return NULL;
	if(strncmp(s, "GIMP Palette", e - s) != 0)
		return NULL;
	s = e + 1;

	/* Not interested in the metadata, so we skip the rest */
	while(*s != '#')
	{
		if(!(s = read_until(s, 10)))
			return NULL;
		++s;
	}

	if((s = read_until(s, 10)))
		++s;
	return s;
}

static int parse_entry(const char *s, uint32_t *color)
{
	int c;
	const char *e = s;
	*color = 0;
	for(c = 0; c < 3; ++c)
	{
		while(isblank(*s))
			++s;
		e = s;
		while(isdigit(*e))
			++e;
		if(!*e || (e == s))
			return 0;
		*color <<= 8;
		*color |= atoi(s) & 0xff;
		s = e;
	}
	return 1;
}

GFX_palette *gfx_palette_parse(const char *data)
{
	int i;
	const char *s;
	GFX_palette *p = (GFX_palette *)calloc(1, sizeof(GFX_palette));
	if(!p)
		return NULL;

	/* Count lines and allocate space for the entries */
	s = data;
	while((s = read_until(s, 10)))
	{
		++p->nentries;
		++s;
	}
	if(p->nentries <= 4)
	{
		free(p);
		return NULL;
	}
	p->nentries -= 4;
	p->entries = (uint32_t *)calloc(p->nentries, sizeof(uint32_t));
	if(!p->entries)
	{
		free(p);
		return NULL;
	}

	if(!(s = parse_header(p, data)))
	{
		fprintf(stderr, "Invalid .gpl palette header!\n");
		gfx_palette_free(p);
		return NULL;
	}

	for(i = 0; i < p->nentries; ++i)
	{
		if(!parse_entry(s, p->entries + i))
		{
			fprintf(stderr, "Invalid .gpl palette data!\n");
			gfx_palette_free(p);
			return NULL;
		}
		s = read_until(s, 10);
		if(!s)
		{
			fprintf(stderr, "Unexpected end of .gpl palette!\n");
			gfx_palette_free(p);
			return NULL;
		}
		++s;
	}

	return p;
}


GFX_palette *gfx_palette_load(const char *path)
{
	GFX_palette *p;
	FILE *f;
	size_t fsize;
	char *buf;
	if(!(f = fopen(path, "rb")))
		return NULL;
	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	if(!(buf = malloc(fsize + 1)))
	{
		fclose(f);
		return NULL;
	}
	buf[fsize] = 0;
	if(fread(buf, 1, fsize, f) != fsize)
	{
		fclose(f);
		free(buf);
		return NULL;
	}
	fclose(f);

	p = gfx_palette_parse(buf);

	free(buf);
	return p;
}


void gfx_palette_free(GFX_palette *p)
{
	free(p->entries);
	free(p);
}
