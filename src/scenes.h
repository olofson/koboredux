/*(GPLv2)
 * XKOBO, a video-oriented game
 * Copyright (C) 1995, 1996 Akira Higuchi
 *     a-higuti@math.hokudai.ac.jp
 * Copyright (C) 2006, 2007, 2009 David Olofson
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

#ifndef XKOBO_H_SCENES
#define XKOBO_H_SCENES

#include "config.h"
#include "enemies.h"

#define SCENE_ENEMY_MAX   20
#define SCENE_BASE_MAX    40

struct enemy_set
{
	const enemy_kind *kind;
	int num;
	int speed;
};

struct  _base
{
	int x, y, h, v;
};

struct _scene
{
	int ratio;
	int startx;
	int starty;
	const enemy_kind *ek1;
	int ek1_interval;
	const enemy_kind *ek2;
	int ek2_interval;
	int enemy_max;
	enemy_set enemy[SCENE_ENEMY_MAX];
	int base_max;
	_base base[SCENE_BASE_MAX];
};

extern const _scene scene[];

#endif // XKOBO_H_SCENES
