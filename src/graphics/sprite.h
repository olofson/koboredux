/*(LGPLv2.1)
----------------------------------------------------------------------
	sprite.h - Sprite engine for use with cs.h
----------------------------------------------------------------------
 * Copyright 2001, 2003, 2007, 2009 David Olofson
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

#ifndef _SPRITE_H_
#define _SPRITE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "SDL.h"

typedef enum
{
	S_BLITMODE_AUTO = 0,	/* Use source alpha if present */
	S_BLITMODE_OPAQUE,	/* Alpha: blend over black during conv. */
	S_BLITMODE_COLORKEY,	/* Like OPAQUE, but apply colorkey to the result */
	S_BLITMODE_ALPHA	/* Apply source alpha during conv,
				 * then use full surface alpha */
} s_blitmodes_t;

struct s_container_t;

/* sprite image */
typedef struct
{
	int		x, y;		/* kern or hot-spot*/
	SDL_Texture	*texture;
	SDL_Surface	*surface;
/*TODO:	SDL_BlendMode	blendmode;	*/
} s_sprite_t;

/* Bank of sprite images */
typedef struct
{
	unsigned	w, h;	/* Same size for all sprites in bank! */
	unsigned	last;
	unsigned	max;
	s_sprite_t	**sprites;
} s_bank_t;

/* Two dimensionally indexed sprite container */
typedef struct s_container_t
{
	unsigned	max;
	s_bank_t	**banks;
} s_container_t;


/*
 * Basic Container Management
 */
s_container_t *s_new_container(unsigned banks);
void s_delete_container(s_container_t *c);

s_bank_t *s_new_bank(s_container_t *c, unsigned bank, unsigned frames,
				unsigned w, unsigned h);
void s_delete_bank(s_container_t *c, unsigned bank);
void s_delete_all_banks(s_container_t *c);

s_sprite_t *s_new_sprite(s_container_t *c, unsigned bank, unsigned frame);
void s_delete_sprite(s_container_t *c, unsigned bank, unsigned frame);

/*
 * Getting data
 */
s_sprite_t *s_get_sprite(s_container_t *c, unsigned bank, unsigned frame);
/* Detach sprite image from the manager. */
void s_detach_sprite(s_container_t *c, unsigned bank, unsigned frame);

/*
 * File Import Tools
 * (Will run plugins as files are loaded!)
 */
int s_load_image(s_container_t *c, unsigned bank, const char *name);
int s_load_bank(s_container_t *c, unsigned bank, unsigned w, unsigned h,
		const char *name);
int s_load_sprite(s_container_t *c, unsigned bank, unsigned frame,
		const char *name);

/*
 * Rendering Tools
 */
int s_generate_bank(s_container_t *c, unsigned bank, unsigned w, unsigned h,
		const char *name);

/*
 * Sprite metadata
 */
/* Set sprite hotspot. Pass -1 for 'frame' to set hotspot for all frames. */
int s_set_hotspot(s_container_t *c, unsigned bank, int frame, int x, int y);


/*
 * Internal data manipulation API
 * (Will not automatically run plugins.)
 */
int s_copy_rect(s_container_t *c, unsigned bank,
		unsigned frombank, unsigned fromframe, SDL_Rect *from);

/*
 * Lower level interfaces
 */
s_bank_t *s_get_bank(s_container_t *c, unsigned bank);
s_sprite_t *s_get_sprite_b(s_bank_t *b, unsigned frame);
s_sprite_t *s_new_sprite_b(s_bank_t *b, unsigned frame);
void s_delete_sprite_b(s_bank_t *b, unsigned frame);


/*
 * Filter Plugin Interface
 *
 * Rules:
 *	* Plugins are called to process one bank at a time,
 *	  after new frames have been loaded.
 *
 *	* Plugins are called in the order they were
 *	  registered.
 *
 *	* Plugins may replace the surfaces of sprites. If
 *	  they do so, they're responsible for disposing of
 *	  the old surfaces.
 *
 *	* It is recommended that plugins operate only on
 *	  the specified range of frames in a bank.
 *
 *	* The surface size of all sprites in a bank should
 *	  preferably match that of the bank. A chain of
 *	  plugins may break this rule temporarily, as long
 *	  as "order is restored" (ie surfaces are resized, or
 *	  bank size values is adjusted) before the chain ends.
 *
 * Cool plugin ideas:
 *	* Rotator:
 *	  Takes a bank and generates N rotated versions of
 *	  each sprite.
 *
 *	* Bumpmapper:
 *	  Takes a bank and applies light, shadows, highlights
 *	  and stuff to frames that have bumpmap channels.
 *	  (Q: Where do we put bumpmaps? Extend the bank to
 *	  twice the # of frames, using the last half for BMs?
 *	  Use two banks? I think we need an API change that
 *	  make plugins operators; X = A <op> B, where the
 *	  banks X, A and B are arguments.)
 *
 *	* Mixer:
 *	  A plugin that takes two banks and performs various
 *	  operations between them to produce the output.
 *
 *	* Colorizer:
 *	  Converts images to HSV and then replaces hue with
 *	  a fixed color.
 *
 *	* Map Colorizer:
 *	  Like Colorizer, but uses a (wrapping) source image
 *	  as a map to determine the new hue for each pixel.
 *
 *	* Tiled Stretch:
 *	  Given a list of tile interdependencies, generate
 *	  all tiles required for seamless rendering of the
 *	  map for which the interdependency list was generated.
 */
typedef struct s_filter_args_t
{
	int		x, y, z;
	float		fx, fy, fz;
	int		min, max;
	short		r, g, b;
	int		flags;
	unsigned	bank;
	void		*data;
} s_filter_args_t;

typedef int (*s_filter_cb_t)(s_bank_t *b, unsigned first, unsigned frames,
				s_filter_args_t *args);

typedef struct s_filter_t
{
	struct s_filter_t	*next;
	s_filter_cb_t		callback;
	s_filter_args_t		args;
} s_filter_t;

s_filter_t *s_insert_filter(s_filter_cb_t callback);
s_filter_t *s_add_filter(s_filter_cb_t callback);
/* callback == NULL means "remove all plugins" */
void s_remove_filter(s_filter_t *filter);

/* RGBA pixel type (used internally by most filters as well) */
typedef struct
{
	Uint8	r;
	Uint8	g;
	Uint8	b;
	Uint8	a;
} pix_t;

/*
 * UURGH!!! This should be OO like the rest...
 */
extern s_blitmodes_t	s_blitmode;
extern pix_t		s_colorkey;	/* NOTE: Alpha is ignored! */
extern pix_t		s_clampcolor;
extern unsigned char	s_alpha;
extern int		s_filter_flags;	/* Global flags; applied to all plugins. */

#ifdef __cplusplus
};
#endif

#endif /* _SPRITE_H_ */
