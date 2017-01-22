/*(GPLv2)
 * XKOBO, a video-oriented game
 * Copyright 1995, 1996 Akira Higuchi
 *     a-higuti@math.hokudai.ac.jp
 * Copyright 2006, 2007, 2009 David Olofson
 * Copyright 2015-2017 David Olofson (Kobo Redux)
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

struct KOBO_enemy_set
{
	const KOBO_enemy_kind *kind;
	int num;
	int speed;
};

struct KOBO_base
{
	int x, y, h, v;
};

struct KOBO_scene
{
	int ratio;
	int startx;
	int starty;
	const KOBO_enemy_kind *ek1;
	int ek1_interval;
	const KOBO_enemy_kind *ek2;
	int ek2_interval;
	int enemy_max;
	KOBO_enemy_set enemy[SCENE_ENEMY_MAX];
	int base_max;
	KOBO_base base[SCENE_BASE_MAX];
};

class KOBO_scene_manager
{
	int			nscenes;
	const KOBO_scene	*scenes;
  public:
	KOBO_scene_manager();
	const KOBO_scene *get(int stage);
	int scene_count()	{ return nscenes; }
	int region(int stage)
	{
		return stage / KOBO_LEVELS_PER_REGION % KOBO_REGIONS;
	}
	int level(int stage)
	{
		return stage % KOBO_LEVELS_PER_REGION + 1;
	}
};

extern KOBO_scene_manager scene_manager;

#endif // XKOBO_H_SCENES
