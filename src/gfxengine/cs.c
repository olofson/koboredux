/*(LGPLv2.1)
 * Project Spitfire sprites + scrolling engine for SDL
----------------------------------------------------------------------
	cs.c - Simplistic Control System
----------------------------------------------------------------------
 * Copyright 2001, 2003, 2007, 2009 David Olofson
 * Copyright 2015-2016 David Olofson (Kobo Redux)
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

#define	DBG(x)

#include "cs.h"
#include "logger.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

/*#define ABS(x)   (((x)>=0) ? (x) : (-(x)))*/
#define ABS(x)	labs(x)


static inline void __wrap_point(cs_point_t *p, int wx, int wy)
{
	if(wx)
	{
		if(p->gx < 0)
			p->gx += (-p->gx / wx + 1) * wx;
		p->gx %= wx;
	}
	if(wy)
	{
		if(p->gy < 0)
			p->gy += (-p->gy / wy + 1) * wy;
		p->gy %= wy;
	}
}


/*
 * Make one point do it's thing...
 */
static inline void __move_point(cs_point_t *p, int wx, int wy)
{
	/* 
	 * Push filter
	 */
	p->ox = p->v.x;
	p->oy = p->v.y;

	/* 
	 * Update position and speed
	 */
	p->v.x += p->v.xv;
	p->v.y += p->v.yv;
	p->v.xv += p->v.xa;
	p->v.yv += p->v.ya;

	__wrap_point(p, wx, wy);
}


/*
 * Update point by interpolating between the previous and current state, or by
 * extrapolating from the current state.
 */
static inline void update_point_ix(cs_point_t *p, int ff, int wx, int wy,
		int extrapolate)
{
	int dx, dy;
	int ogx = p->gx;
	int ogy = p->gy;

	/* Unwrap for correct interpolation/extrapolation */
	if(wx)
	{
		if((p->ox - p->v.x) > (wx >> 1))
			p->ox -= wx;
		else if((p->v.x - p->ox) > (wx >> 1))
			p->ox += wx;
	}
	if(wy)
	{
		if((p->oy - p->v.y) > (wy >> 1))
			p->oy -= wy;
		else if((p->v.y - p->oy) > (wy >> 1))
			p->oy += wy;
	}

	dx = (p->v.x - p->ox) * ff >> 8;
	dy = (p->v.y - p->oy) * ff >> 8;
	if(extrapolate)
	{
		p->gx = p->v.x + dx;
		p->gy = p->v.y + dy;
	}
	else
	{
		p->gx = p->ox + dx;
		p->gy = p->oy + dy;
	}

	/* 
	 * Wrap filtered interpolation/extrapolation
	 */
	if(wx)
	{
		while(p->gx < 0)
			p->gx += wx;
		p->gx %= wx;
	}
	if(wy)
	{
		while(p->gy < 0)
			p->gy += wy;
		p->gy %= wy;
	}

	/* 
	 * Generate "changed" flag mask.
	 */
	p->changed = (ogx ^ p->gx) | (ogy ^ p->gy);
}


/*
 * Update point using the current state.
 */
static inline void update_point(cs_point_t *p, int ff)
{
	int ogx = p->gx;
	int ogy = p->gy;

	/* 
	 * Interpolate output graphics coordinate
	 */
	p->gx = p->v.x;
	p->gy = p->v.y;

	/* 
	 * Generate "changed" flag mask.
	 */
	p->changed = (ogx ^ p->gx) | (ogy ^ p->gy);
}


/*
----------------------------------------------------------------------
 * cs_obj_t
----------------------------------------------------------------------
 */

/*
 * Final adjustment of interpolated graphics coords to deal
 * with the display window wrapping over the edge of the world.
 */
static inline void __fix_wrap(cs_engine_t *e, cs_obj_t *o)
{
	if(o->point.gx + (o->w << __CS_SHIFT) < 0)
		o->point.gx += e->wx;
	else if(o->point.gx > (e->w << __CS_SHIFT))
		o->point.gx -= e->wx;
	if(o->point.gy + (o->h << __CS_SHIFT) < 0)
		o->point.gy += e->wy;
	else if(o->point.gy > (e->h << __CS_SHIFT))
		o->point.gy -= e->wy;
}


static inline int __onscreen(cs_engine_t *e, cs_obj_t *o)
{
	int gx = CS2PIXEL(o->point.gx);
	int gy = CS2PIXEL(o->point.gy);
	if((gx + o->w < 0) || (gx > e->w))
		return 0;
	if((gy + o->h < 0) || (gy > e->h))
		return 0;
	return 1;
}


static inline void __obj_set_layer(cs_obj_t *o, unsigned int layer)
{
	if(o->layer < 0)
		o->layer = CS_DEFAULT_LAYER;
	if(layer >= CS_LAYERS)
		layer = CS_LAYERS - 1;
	o->layer = layer;
}

/*
 * Add object *first*, ie with lowest priority.
 * (Hmmm... Isn't that how some h/w sprite generators do it,
 * if you allocate in increasing channel order?)
 */
static inline void __obj_attach(cs_obj_t *o)
{
	if(o->head || o->next || o->prev)
	{
		log_printf(ELOG, "cs: HEEEELP! Someone's trying"
				" to short-circuit my guts!\n");
		return;
	}

	if(o->layer < 0)
		o->layer = CS_DEFAULT_LAYER;
	o->next = o->owner->objects[o->layer];
	o->head = &o->owner->objects[o->layer];
	if(o->next)
		o->next->prev = o;
	o->owner->objects[o->layer] = o;
}

/*
 * BEWARE! There's a nasty hack in here; these objects need to
 * keep track of the addresses of their list heads in order to
 * remove themselves properly!
 */
static inline void __obj_detach(cs_obj_t *o)
{
	/* "Bypass" */
	if(o->next)
		o->next->prev = o->prev;
	if(o->prev)
		o->prev->next = o->next;
	/* In case we're the first object in the list... */
	if(o->head)
		if(*(o->head) == o)
			*(o->head) = o->next;

	o->head = NULL;
	o->next = NULL;
	o->prev = NULL;
}


void cs_obj_layer(cs_obj_t *o, unsigned int layer)
{
	if(layer == o->layer)
		return;

	__obj_set_layer(o, layer);
	if(o->flags | CS_OBJ_ACTIVE)
	{
		__obj_detach(o);
		__obj_attach(o);
	}
}


void cs_obj_activate(cs_obj_t *o)
{
	if(o->flags | CS_OBJ_ACTIVE)
	{
		DBG(log_printf(DLOG, "cs: Tried to activate active object!\n");)
		return;
	}

	__obj_attach(o);
	o->flags |= CS_OBJ_ACTIVE;
}


void cs_obj_deactivate(cs_obj_t *o)
{
	if(!(o->flags | CS_OBJ_ACTIVE))
	{
		DBG(log_printf(DLOG, "cs: Tried to deactivate passive object!\n");)
		return;
	}

	__obj_detach(o);
	o->flags &= ~CS_OBJ_ACTIVE;
}


/* Basic methods */

void cs_obj_pos(cs_obj_t *o, int x, int y)
{
	o->point.v.x = PIXEL2CS(x);
	o->point.v.y = PIXEL2CS(y);
}


void cs_obj_vel(cs_obj_t *o, int xs, int ys)
{
	o->point.v.xv = xs;
	o->point.v.yv = ys;
}


/*Set movement acceleration*/
void cs_obj_acc(cs_obj_t *o, int xa, int ya)
{
	o->point.v.xa = xa;
	o->point.v.ya = ya;
}


/*Disable display*/
void cs_obj_hide(cs_obj_t *o)
{
	o->flags &= ~CS_OBJ_VISIBLE;
}


/*Enable display*/
void cs_obj_show(cs_obj_t *o)
{
	if(o->flags & CS_OBJ_VISIBLE)
		return;

	o->flags |= CS_OBJ_VISIBLE;
	/* 
	 * Kludge? I'm not sure this is really the right place...
	 */
	cs_point_force(&o->point);
}


/*
 * Remove and throw back in the pool
 */
void cs_obj_free(cs_obj_t *o)
{
	if(o->head == &o->owner->pool)
	{
		DBG(log_printf(DLOG, "cs: Tried to free free object %p!\n",
					o);)
		return;
	}
	cs_obj_deactivate(o);
	if(o->on_free)
		o->on_free(o);
	cs_obj_clear(o);

	/* Set up head pointer for error checking. */
	o->head = &o->owner->pool;

	/* Link to pool; singly only */
	o->next = o->owner->pool;
	o->owner->pool = o;

	++o->owner->pool_free;
}

void cs_obj_clear(cs_obj_t *o)
{
	o->flags = 0;
	o->layer = -1;
	o->fire.rate = 0;
	o->score = 0;
	o->health = 0;
}

void cs_obj_vector(cs_obj_t *o, cs_vector_t * v)
{
	if(v)
		o->point.v = *v;
	else
		memset(&(o->point.v), 0, sizeof(o->point.v));
}



/* Animation methods */
void cs_obj_anim(cs_obj_t *o, int bank,
		int first, int num, int start, int spd)
{
	o->anim.bank = bank;
	o->anim.frame = first;
	o->anim.fframe = first;
	o->anim.aframes = num;
	o->anim.cframe = start << 4;
	o->anim.aspeed = spd;
	o->flags |= CS_OBJ_DOANIM;
}


void cs_obj_image(cs_obj_t *o, int bank, int frame)
{
	o->anim.bank = bank;
	o->anim.frame = frame;
	o->flags &= ~CS_OBJ_DOANIM;
	if((bank >= 0) && (bank < o->owner->nimageinfo))
	{
		o->w = o->owner->imageinfo[bank].w;
		o->h = o->owner->imageinfo[bank].h;
	}
	else
		o->w = o->h = 1;
}


/*Start death sequence*/
void cs_obj_explode(cs_obj_t *o)
{
	o->flags |= CS_OBJ_DYING;
/*
FIXME: Hardcoded bank for explosion!!!
*/
	cs_obj_anim(o, 4, 0, 15, 0, 12);
	if(o->on_dying)
		o->on_dying(o);
}

void cs_obj_shoot(cs_obj_t *o, int xo, int yo, int xs,
		int ys, int rate, int delay, int ammo)
{
	o->fire.v.x = xo;
	o->fire.v.y = yo;
	o->fire.v.xv = xs;
	o->fire.v.yv = ys;
	o->fire.rate = rate;
	o->fire.timer = delay;
	o->fire.ammo = ammo;
}



/*
 * cs_engine_t
 */

static cs_obj_t *__new_obj(cs_engine_t *e)
{
	cs_obj_t *o = calloc(1, sizeof(cs_obj_t));
	if(!o)
		return NULL;

	o->owner = e;
	o->prev = NULL;
	o->next = NULL;
	cs_obj_clear(o);
	++e->pool_total;
	return o;
}

cs_engine_t *cs_engine_create(int w, int h, int objects)
{
	cs_engine_t *e = calloc(1, sizeof(cs_engine_t));
	if(!e)
		return NULL;

	cs_engine_set_size(e, w, h);
	cs_engine_set_wrap(e, 0, 0);

	/* 
	 * Create some objects and throw into the pool.
	 * Note that "pool_free" gets initialized by
	 * cs_obj_die(), objects are initialized and so
	 * on, this way - automatically!
	 */
	while(objects--)
	{
		cs_obj_t *o = __new_obj(e);
		if(o)
			cs_obj_free(o);
		else
		{
			/* Oops, no memory... */
			cs_engine_delete(e);
			return NULL;
		}
	}
	return e;
}


cs_obj_t *cs_engine_get_obj(cs_engine_t *e)
{
	cs_obj_t *o = e->pool;
	if(o)
	{
		e->pool = o->next;
		o->head = NULL;
		o->prev = NULL;
		o->next = NULL;
		--e->pool_free;
	}
	return o;
}


void cs_engine_set_size(cs_engine_t *e, int w, int h)
{
	e->w = w;
	e->h = h;
}


void cs_engine_set_image_size(cs_engine_t *e, int bank, int w, int h)
{
	if(bank < 0)
		return;

	if(bank >= e->nimageinfo)
	{
		int nnii = bank + 16;
		image_info_t *nii = realloc(e->imageinfo,
				sizeof(image_info_t) * nnii);
		if(!nii)
			return;

		e->nimageinfo = nnii;
		e->imageinfo = nii;
	}

	e->imageinfo[bank].w = w;
	e->imageinfo[bank].h = h;
}


void cs_engine_set_wrap(cs_engine_t *e, int x, int y)
{
	e->wx = PIXEL2CS(x);
	e->wy = PIXEL2CS(y);
}


void cs_engine_delete(cs_engine_t *e)
{
	cs_obj_t *o;
	/* First get all objects to the pool... */
	cs_engine_reset(e);
	/* ...then empty the pool. */
	while((o = cs_engine_get_obj(e)))
		free(o);
	free(e->imageinfo);
	free(e);
}


void cs_engine_reset(cs_engine_t *e)
{
	int i;
	for(i = 0; i < CS_LAYERS; ++i)
		while(e->objects[i])
			cs_obj_free(e->objects[i]);
}



/*
----------------------------------------------------------------------
 * The actual *engine* :-)
----------------------------------------------------------------------
 */

static void __enemy_fire(cs_obj_t *o)
{
	log_printf(DLOG, "fire!\n");
}

static void __make_move(cs_obj_t *o, int wx, int wy)
{
	/* 
	 * Movement
	 */
	__move_point(&o->point, wx, wy);

	/* 
	 * Animation
	 */
	if(o->flags & CS_OBJ_DOANIM)
	{
/*
FIXME: Dirty hack here... Should play *any* death animation one-shot and then die.
*/
		if((o->flags & CS_OBJ_DYING)
				&& (o->anim.cframe >> 4 == 14))
			cs_obj_free(o);

		o->anim.cframe += o->anim.aspeed;
		o->anim.cframe %= o->anim.aframes << 4;
		o->anim.frame = (o->anim.cframe >> 4) + o->anim.fframe;
	}

	/* 
	 * Fire / spawning
	 */
	if(!(o->flags & CS_OBJ_DYING) && o->fire.rate)
	{
		if(o->fire.timer > 0)
			--o->fire.timer;
		else
		{
			o->fire.timer = o->fire.rate;
			__enemy_fire(o);
			if(o->on_fire)
				o->on_fire(o);
		}
	}
}


static void __update_points(cs_engine_t *e, float frac_frame)
{
	cs_obj_t *o;
	int i;
	int ff = frac_frame * 256.0;

	if(ff < 0)
		ff = 0;
	else if(ff > 256)
		ff = 256;

	switch(e->filter)
	{
	  default:
	  case CS_FM_NONE:
		for(i = 0; i < CS_USER_POINTS; ++i)
			update_point(&e->points[i], ff);
		break;
	  case CS_FM_INTERPOLATE:
		for(i = 0; i < CS_USER_POINTS; ++i)
			update_point_ix(&e->points[i], ff, e->wx, e->wy, 0);
		break;
	  case CS_FM_EXTRAPOLATE:
		for(i = 0; i < CS_USER_POINTS; ++i)
			update_point_ix(&e->points[i], ff, e->wx, e->wy, 1);
		break;
	}

	for(i = 0; i < CS_LAYERS; ++i)
	{
		switch(e->filter)
		{
		  default:
		  case CS_FM_NONE:
			update_point(&e->offsets[i], ff);
			break;
		  case CS_FM_INTERPOLATE:
			update_point_ix(&e->offsets[i], ff, e->wx, e->wy, 0);
			break;
		  case CS_FM_EXTRAPOLATE:
			update_point_ix(&e->offsets[i], ff, e->wx, e->wy, 1);
			break;
		}
		e->changed[i] = 0;
		o = e->objects[i];
		while(o)
		{
			switch(e->filter)
			{
			  default:
			  case CS_FM_NONE:
				update_point(&o->point, ff);
				break;
			  case CS_FM_INTERPOLATE:
				update_point_ix(&o->point, ff, e->wx, e->wy,
						0);
				break;
			  case CS_FM_EXTRAPOLATE:
				update_point_ix(&o->point, ff, e->wx, e->wy,
						1);
				break;
			}
			o->point.gx -= e->offsets[i].gx;
			o->point.gy -= e->offsets[i].gy;
			e->changed[i] |= o->point.changed;
			__fix_wrap(e, o);
			o = o->next;
		}
	}
}


static void __run_all(cs_engine_t *e)
{
	int i;
	cs_obj_t *o;

	for(i = 0; i < CS_USER_POINTS; ++i)
		__move_point(&e->points[i], e->wx, e->wy);

	for(i = 0; i < CS_LAYERS; ++i)
	{
		__move_point(&e->offsets[i], e->wx, e->wy);
		o = e->objects[i];
		while(o)
		{
			__make_move(o, e->wx, e->wy);
			o = o->next;
		}
	}
}


static void __wrap_all(cs_engine_t *e)
{
	int i;
	cs_obj_t *o;

	for(i = 0; i < CS_USER_POINTS; ++i)
		__wrap_point(&e->points[i], e->wx, e->wy);

	for(i = 0; i < CS_LAYERS; ++i)
	{
		__wrap_point(&e->offsets[i], e->wx, e->wy);
		o = e->objects[i];
		while(o)
		{
			__wrap_point(&o->point, e->wx, e->wy);
			o = o->next;
		}
	}
}


void cs_engine_advance(cs_engine_t *e)
{
	__run_all(e);
	e->on_frame(e);
}


void cs_engine_tween(cs_engine_t *e, float fractional_frame)
{
	if(e->wx || e->wy)
		__wrap_all(e);
	__update_points(e, fractional_frame);
}


void cs_engine_render(cs_engine_t *e, int layer)
{
	cs_obj_t *o = e->objects[layer];
	while(o)
	{
		if(o->flags & CS_OBJ_VISIBLE)
			if(o->render)
				if(__onscreen(e, o))
					o->render(o);
		o = o->next;
	}
}
