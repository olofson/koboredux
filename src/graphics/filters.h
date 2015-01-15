/*(LGPLv2.1)
----------------------------------------------------------------------
	filters.h - Filters for the sprite manager
----------------------------------------------------------------------
 * Copyright 2001, 2003, 2009 David Olofson
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

#ifndef	SPRITE_FILTERS_H
#define	SPRITE_FILTERS_H

#include "sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Flags (args.flags) that apply to most plugins:
 */
#define	SF_CLAMP_EXTEND		0x00000001
#define	SF_CLAMP_SFONT		0x00000002


/*
 * Convert bank to RGBA 8:8:8:8 format.
 *
 * (No arguments.)
 */
int s_filter_rgba8(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);

/*
 * Dither R, G and B channels to the specified bit depth.
 *
 * args.x = 1 activates 16 bit RGBA fix (assume +/-16 dither depth for RGBA)
 * args.y = dither type (0 = 2x2 filter, 1 = random)
 *
 * args.r = red dither depth
 * args.g = green dither depth
 * args.b = blue dither depth
 *
 * args.flags;
 *	SF_CLAMP_SFONT		SFont mode; first row is ignored.
 *
 * Note: This plugin does *not* dither the alpha channel!
 */
int s_filter_dither(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);

/*
 * Optionally convert bank surfaces into textures.
 *
 * args.data = SDL renderer
 */
int s_filter_displayformat(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);

/*
 * Convert colorkey to alpha channel.
 *
 * The global colorkey variable is used for base colorkey value.
 * args.max = colorkey detection fuzziness factor
 * args.flags;
 *	SF_CLAMP_SFONT		SFont mode; first row is ignored.
 */
int s_filter_key2alpha(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);

/*
 * Adjust alpha level and contrast, and then clean out almost opaque
 * and almost transparent pixels.
 *
 * args.fx = alpha contrast (1.0 = unchanged; 2.0 = double; scales around 128)
 * args.x = alpha offset (0 = unchanged; -255 = all transp.; 255 = all opaque)
 *
 * args.min = min alpha passed (lower ==> 0)
 * args.max = max alpha passed (higher ==> 255)
 *
 * args.flags;
 *	SF_CLAMP_SFONT		SFont mode; first row is ignored.
 */
int s_filter_cleanalpha(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);

/*
 * Adjust brightness and contrast.
 *
 * args.fx = brightness (1.0 = unchanged; 0.0 = black; 2.0 = white)
 * args.fy = contrast (1.0 = unchanged; 0.5 = 50%; 2.0 = 200%)
 *
 * args.min = min value (clamping)
 * args.max = max value (clamping)
 *
 * args.flags;
 *	SF_CLAMP_SFONT		SFont mode; first row is ignored.
 */
int s_filter_brightness(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);

/*
 * Scale images by the specified factor, using the specified method.
 *
 * args.x = scaling mode;
 *	SF_SCALE_NEAREST	Nearest
 *	SF_SCALE_BILINEAR	Bilinear interpolation
 *	SF_SCALE_SCALE2X	Scale2x (Based on an algo from AdvanceMAME)
 *	SF_SCALE_DIAMOND	Diamond (Weighted diamond shaped core)
 *
 * args.fx = horizontal scale factor (1.0 ==> 1:1)
 * args.fy = vertical scale factor (1.0 ==> 1:1)
 *
 * args.flags;
 *	SF_CLAMP_EXTEND		Clamp at edges; image edge is extended outwards
 *	SF_CLAMP_SFONT		SFont mode; first row is treated specially.
 *
 */
#define	SF_SCALE_NEAREST	0
#define	SF_SCALE_BILINEAR	1
#define	SF_SCALE_SCALE2X	2
#define	SF_SCALE_DIAMOND	3
int s_filter_scale(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);

#if 0
/*
TODO:
 * Add shaded light to images.
 *
 * args.fx = Highlight X position ([0,1])
 * args.fy = Highlight Y position ([0,1])
 * args.fz = Highlight "half intensity" radius
 *
 * args.x = Gradient type:
 *	SF_LIGHT_LINEAR		Linear gradient
 *	SF_LIGHT_CIRCULAR	Circular spotlight
 *
 * args.z = Gradient rotation (65536 units/rotation)
 *
 * args.min = "Ambient light"
 * args.max = Max brightness
 *
 * args.r = Light color, red component
 * args.g = Light color, green component
 * args.b = Light color, blue component
 *
 * args.flags;
 *	SF_CLAMP_SFONT		SFont mode; first row is ignored.
 */
#define	SF_LIGHT_LINEAR		0
#define	SF_LIGHT_CIRCULAR	1
int s_filter_light(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);
#endif

/*
 * Add single colored noise to images.
 *
 * args.x = Pseudo-RNG seed for first frame
 *
 * args.min = Min brightness ([0,255])
 * args.max = Max brightness ([0,255])
 *
 * args.r = Noise color, red component ([0,255])
 * args.g = Noise color, green component ([0,255])
 * args.b = Noise color, blue component ([0,255])
 *
 * args.flags;
 *	SF_CLAMP_SFONT		SFont mode; first row is ignored.
 */
int s_filter_noise(s_bank_t *b, unsigned first, unsigned frames,
		s_filter_args_t *args);

#ifdef __cplusplus
}
#endif

#endif	/* SPRITE_FILTERS_H */
