/*(LGPLv2.1)
----------------------------------------------------------------------
	mathutil.h - Various handy integer/fixp math functions
----------------------------------------------------------------------
 * Copyright 2017 David Olofson (Kobo Redux)
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


#ifndef KOBO_MATHUTIL_H
#define KOBO_MATHUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Integer modulo */
static inline int mod(int a, int b)
{
	int r = a % b;
	return r < 0 ? r + b : r;
}

/*
 * Fast integer atan() approximation. Input is 24:8 fixed point.
 * Output is 0..64 for 0..45 deg, accurate down to LSB.
 *	In:	q = 256 * minv / maxv	(0..256 <==> 0..1)
 *	Out:	atan(q / 256) * 256	(0..64 <==> 0..45 deg)
 *
 * atan() approximations for 0..90 deg:
 *	a = 83 * q / 256 - q*q / 2844				(+/- 2.1%)
 *	a = 82 * q / 256 - q*q / 8500 - q*q*q / 1600000		(+/- 0.6%)
 *
 * FIXME: Better version?
 *	a = 84 * q / 256 - 95 * q*q / 524288 - 8 * q*q*q / 16777216
 */
static inline int fastatan(int q)
{
	int q2 = q * q;
	int q3 = q2 * q;
	return (82 * q >> 8) - (62 * q2 >> 19) - (10 * q3 >> 24);
}

static inline int speed2dir(int sx, int sy, int frames)
{
	if(!sx && !sy)
		return 0;

	int at2;
	if(sx > 0)
	{
		// Right
		if(sy < 0)
		{
			// Top-right quadrant
			sy = -sy;
			if(sy > sx)
				at2 = fastatan(sx * 256 / sy);
			else
				at2 = 128 - fastatan(sy * 256 / sx);
		}
		else
		{
			// Bottom-right quadrant
			if(sx > sy)
				at2 = 128 + fastatan(sy * 256 / sx);
			else
				at2 = 256 - fastatan(sx * 256 / sy);
		}
	}
	else
	{
		// Left
		sx = -sx;
		if(sy > 0)
		{
			// Bottom-left quadrant
			if(sy > sx)
				at2 = 256 + fastatan(sx * 256 / sy);
			else
				at2 = 384 - fastatan(sy * 256 / sx);
		}
		else
		{
			// Top-left quadrant
			sy = -sy;
			if(sx > sy)
				at2 = 384 + fastatan(sy * 256 / sx);
			else
				at2 = 512 - fastatan(sx * 256 / sy);
		}
	}
	at2 = (at2 * frames + 256) >> 9;
	return at2 > frames - 1 ? 1 : at2 + 1;
}

#ifdef __cplusplus
};
#endif

#endif	/* KOBO_MATHUTIL_H */
