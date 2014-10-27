/*(LGPLv2.1)
 * Project Spitfire sprites + scrolling engine for SDL
----------------------------------------------------------------------
	cs.h - Simplistic Control System
----------------------------------------------------------------------
 * Copyright (C) 2001, 2003, 2007, 2009 David Olofson
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

/*
 * Note on coordinates:
 *	Normally, cs assumes that the world continues
 *	indefinitely in every direction. (Actually,
 *	it wraps at 4 Gpixel intervals... :-)
 *
 *	However, by calling cs_engine_set_wrap(x, y),
 *	it's possible to make the engine wrap
 *	automatically, to nicely support old style
 *	endless scrolling games. The movement system
 *	will automatically wrap around edges, and
 *	most importantly, *interpolation* will work
 *	correctly in conjunction with wrapping. This
 *	applies to all points in the system, user
 *	points included.
 *
 *	Note that cs doesn't have an internal notion
 *	of layers when it comes to wrapping, which
 *	means that it's not trivial to implement
 *	wrapping for parallax scrolling correctly.
 *	All parallax layers have to be of the same
 *	size, as cs cannot tell which points belong
 *	to which layer when performing the internal
 *	calculations.
 *
 *	Call cs_engine_set_wrap(0, 0); to disable
 *	wrapping. Wrapping can be set or disabled
 *	individually for each axis.
 *
 * Note on graphics coordinates:
 *	Graphics coordinates don't always wrap at the
 *	same time as their corresponding object
 *	coordinates! This is because the engine must
 *	consider the view rectangle and the bounding
 *	rectangles of objects, and adjust the wrapping
 *	so that all objects that should be visible
 *	are.
 *
 * Note on wrapping (with wrapping enabled only):
 *	When a layer is scrolled so that it's limits
 *	end up inside the display window, the engine
 *	automatically makes objects wrap around from
 *	the far edge, so that continuous looping is
 *	possible.
 *	However, note that this breaks if objects can
 *	be seen at more than one edge at a time! That
 *	is, (wrap size - display size) must be bigger
 *	than the size of the largest object.
 *
 * TODO:
 *	* Scrap the animation system and throw
 *	  something nicer in. A primitive VM based
 *	  "scripting" system with <frame, duration>
 *	  commands and switches based on speed,
 *	  direction, state etc would be incredibly
 *	  powerful, and still not too complicated.
 *
 *	* Implement collision detection. There should
 *	  be flags to enable/disable it on per-layer
 *	  as well as per-object basis.
 *
 *	* Group all per-layer stuff into a struct,
 *	  and then use an array of structs.
 *
 *	* Split each layer into two lists; one for
 *	  visible objects and one for hidden objects,
 *	  so they can be handled more efficiently.
 *	  (Hidden objects are skipped one by one in
 *	  the render loop now, for example - it will
 *	  become worse over time, I think.)
 *
 *	* Implement some sort of zone based collision
 *	  detection engine that scales better than
 *	  "check everything against everything".
 */
 /*
 *
 * BUGS:
 *	* There is a restriction on object movement
 *	  speed in a wrapping world: No object must
 *	  move more than half of the extent of the
 *	  world on either axis in one engine cycle.
 *		STATUS: Who cares?
 *
 *	* Objects that are not free or active (they
 *	  do *not* have to be visible!) are "missing
 *	  in action" as far as the engine is concerned.
 *	  It won't find them unless you free them,
 *	  or make them visible.
 *		STATUS: Can and should be fixed.
 */

#ifndef _CS_H_
#define _CS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>


#define	__CS_SHIFT	8
#define	CS2PIXEL(x)	( (x) >> __CS_SHIFT )
#define	PIXEL2CS(x)	( (x) << __CS_SHIFT )

#define	CS_LAYERS		8
#define	CS_DEFAULT_LAYER	1	/*0 is normally for overlays*/
#define	CS_USER_POINTS		16

/*
FIXME: This fixpoint crap should probably be changed to float...
FIXME: Would break the "changed" mask bonus feature described
FIXME: below, though.
*/

typedef struct
{
	/* 24:8 fixpoint values */
	int		x, y;		/* logic position */
	int		xv, yv;
	int		xa, ya;
} cs_vector_t;


/*
 * Some handy vector construction macros...
 *
 * NOTE: position (x,y) in integer format, not fixpoint!
 */

/* Point */
static inline cs_vector_t cs_vector_p(int x, int y)
{
	cs_vector_t v;
	v.x = PIXEL2CS(x);
	v.y = PIXEL2CS(y);
	v.xv = v.yv = v.xa = v.ya = 0;
	return v;
}

/* Point + velocity */
static inline cs_vector_t cs_vector_pv(int x, int y, int xv, int yv)
{
	cs_vector_t v;
	v.x = PIXEL2CS(x);
	v.y = PIXEL2CS(y);
	v.xv = xv;
	v.yv = yv;
	v.xa = v.ya = 0;
	return v;
}

/* Point + velocity + acceleration */
static inline cs_vector_t cs_vector_pva(int x, int y, int xv, int yv, int xa, int ya)
{
	cs_vector_t v;
	v.x = PIXEL2CS(x);
	v.y = PIXEL2CS(y);
	v.xv = xv;
	v.yv = yv;
	v.xa = xa;
	v.ya = ya;
	return v;
}


/*
 * Generic coordinate + speed and acceleration type. Position filtering
 * support is built-in as well, for smooth animation regardless of
 * control system / video frame rate ratio.
FIXME: Interpolation can break detailed animations involving multiple
FIXME: sprites that move and animate (changing images) at the same
FIXME: time. Provide a "CS_OBJ_NO_INTERPOLATION" flag?
 */
typedef struct
{
	/* 24:8 fixpoint values here as well */
	cs_vector_t	v;

	int		ox, oy;		/* last position (for filtering) */
	int		gx, gy;		/* graphic position */

	/* (changed != 0) if gx or gy changed during the last update.
	 * This is a bit mask with the same format as the coords,
	 * which means that you can mask some low bits off to set
	 * a coarse movement sensitivity threshold.
	 *   Ex. (changed & 0xffffff00) == "gx or gy moved to
	 *                                  another pixel posn."
	 */
	int		changed;
} cs_point_t;


/*
 * Force the interpolation filter of a point to the current
 * position of the vector. (This is required when activating
 * new objects.)
 */
static inline void cs_point_force(cs_point_t *p)
{
	p->ox = p->gx = p->v.x;
	p->oy = p->gy = p->v.y;
}


/*
 * This stuff is translated from Object Pascal to "Object
 * Oriented Style C" with my new naming conventions...
 * I don't consider this code needing advanced OO, so I
 * don't want to drag C++ in.
 */

struct cs_engine_t;
struct cs_obj_t;

/*
----------------------------------------------------------------------
 * cs_obj_t (used to be object + sprite)
----------------------------------------------------------------------
 * 
 *	"The Basic Move Object.
 *
 *	This object reperesents a moving object on the screen.
 *	You just tell it in what direction to go, and the Game Graphics
 *	system will do the job."
 *
 * What to do in the callbacks:
 *	collision() 	Check accurately if two objects really collide.
 *			(The control system will ask when bounding rects
 *			are overlapping and the collision masks indicate
 *			that a collision would affect one or both objects.)
 */
typedef struct cs_obj_t
{
	struct cs_engine_t	*owner;
	struct cs_obj_t		**head;
	struct cs_obj_t		*next, *prev;

	cs_point_t	point;		/* Position, speed, acceleration */

	int		w, h;		/* Sprite bounding rect size (pixels) */

	int		flags;
	int		layer;

	/* Collision filter (new) */
	int		group_mask;	/* bit set = "belongs to group" */
	int		hit_mask;	/* bit set = "can hit group" */

	struct
	{
		int	bank;		/* Image bank */
		int	frame;		/* Image frame */
		int	fframe;		/* First frame (integer) */
		int	aframes;	/* # of frames; from fframe (integer) */
		int	cframe;		/* Current frame; from fframe (28:4 fixp) */
		int	aspeed;		/* Frames / engine cycle (28:4 fixp) */
	} anim;

	struct
	{
		cs_vector_t	v;	/* weapon offset + vel + acc */
		int		rate;	/* frames/shot */
		int		timer;
		int		ammo;	/* bullet type */
	} fire;

	int		score;		/*Score for destroy*/
	int		health;		/*Health points*/

	/*
	 * Event callbacks
	 */
	int (*on_hit)(struct cs_obj_t *o);	/* Ouch! */
	int (*on_dying)(struct cs_obj_t *o);	/* Destructing... */
	int (*on_fire)(struct cs_obj_t *o);	/* BOOM! */

	/*
	 * Collision detection callbacks
	 *	Called when bounding rect intersection is detected.
	 *	Should return 1 if there actually is a collision.
	 */
	int (*collision)(struct cs_obj_t *o1, struct cs_obj_t *o2);

	/*
	 * Rendering callback
	 *	NOTE: Use (gx,gy) when rendering, *NOT* (x,y)!
	 */
	void (*render)(struct cs_obj_t *o);

	/*
	 * User destructor; Object is freed after you return.
	 */
	int (*on_free)(struct cs_obj_t *o);

	/* User data */
	int		tag;		/* user ID */
	void		*user;		/* user data */
} cs_obj_t;


/* cs_obj_t flags */

/*
 * True when an object is "in the system"; ie when it's in one of
 * the sprite lists, as opposed to in the free pool, or unattached.
 * ("Unattached" is the state an object is in when you've just got
 * it from cs_engine_get_obj().)
 */
#define	CS_OBJ_ACTIVE		0x00000001

/*
 * Note that an object with a 0 visible flag is still *active*! It's
 * coordinates are updated, animation, collisions, spawning/shooting
 * etc runs - it's just not visible. (Well, whatever is spawned is.)
 */
#define	CS_OBJ_VISIBLE		0x00000002

/*
 * Set by the collision detection system whenever any part of the
 * object's bounding rectangle is inside the bounding rectangle
 * of the screen.
 */
#define	CS_OBJ_ONSCREN		0x00000004

/*
 * Set whenever the cs animation system is to be used to generate
 * image frame numbers for this object.
 */
#define	CS_OBJ_DOANIM		0x00000008

/*
 * Set while the object is marked as dying. Normally, you would
 * run some animation and perhaps spawn some explosion objects,
 * and then free the object.
 */
#define	CS_OBJ_DYING		0x00000010


/* Basic methods */
/*
 * Note: You only really need cs_obj_layer(), cs_obj_show()
 *       and cs_obj_free(). The others are just for more fine
 *       grained life cycle control.
 */
void cs_obj_layer(cs_obj_t *o, unsigned int layer);
void cs_obj_activate(cs_obj_t *o);	/* Add to cs */
void cs_obj_show(cs_obj_t *o);		/* Enable display */
void cs_obj_hide(cs_obj_t *o);		/* Disable display */
void cs_obj_deactivate(cs_obj_t *o);	/* Disengage from cs */
void cs_obj_free(cs_obj_t *o);	/* Remove and throw back in the pool */

void cs_obj_pos(cs_obj_t *o, int x, int y);	/* Set new position */
void cs_obj_vel(cs_obj_t *o, int xs, int ys);	/* Set movement speed */
void cs_obj_acc(cs_obj_t *o, int xa, int ya);	/* Set movement acceleration */
void cs_obj_vector(cs_obj_t *o, cs_vector_t *v);/* Set all */

void cs_obj_clear(cs_obj_t *o);

/* Animation methods */
void cs_obj_image(cs_obj_t *o,  int bank, int frame);
void cs_obj_anim(cs_obj_t *o, int bank,
			int first, int num,
			int start, int spd);
void cs_obj_explode(cs_obj_t *o);	/*Start death sequence*/

/* Fire/spawning control */
/* NOTE:
	In the original version, spawned object speed was
	absolute, while here, it's relative to object 'o'.
*/
void cs_obj_shoot(cs_obj_t *o, int xo, int yo, int xs,
			int ys, int rate, int delay, int ammo);


typedef struct
{
	int	w, h;
} image_info_t;

/*
----------------------------------------------------------------------
 * cs_engine_t (new)
----------------------------------------------------------------------
 */
typedef struct cs_engine_t
{
	/* Display window size (pixels) */
	int		w, h;

	/* Wrapping limits (pixels) */
	int		wx, wy;

	/* Motion filtering */
	int		filter;	/* 0: newest,
				 * 1: linear interpolation
				 */

	/* Current time in frames */
	double		time;

	/*
	 * Callback that's called once per Control System
	 * frame from within cs_engine_advance().
	 */
	void (*on_frame)(struct cs_engine_t *e);

	/* Active objects */
	cs_obj_t	*objects[CS_LAYERS];

	/* Layer position/offset control */
	cs_point_t	offsets[CS_LAYERS];

	/* Position control for custom stuff */
	cs_point_t	points[CS_USER_POINTS];

	/*
	 * Combined "changed" flag masks for the object groups above.
	 */
	int		changed[CS_LAYERS];

	/* Pool of unused objects */
	cs_obj_t	*pool;
	int		pool_free;	/* # of objects in here */
	int		pool_total;	/* # of objects in system */

	/* Image info table */
	image_info_t	*imageinfo;
	int		nimageinfo;
} cs_engine_t;


cs_engine_t *cs_engine_create(int w, int h, int objects);
void cs_engine_set_size(cs_engine_t *e, int w, int h);
void cs_engine_set_image_size(cs_engine_t *e, int bank, int w, int h);
void cs_engine_set_wrap(cs_engine_t *e, int x, int y);
void cs_engine_delete(cs_engine_t *e);

void cs_engine_reset(cs_engine_t *e);
void cs_engine_advance(cs_engine_t *e, double to_frame);	/* 0 == reset timer*/
void cs_engine_render(cs_engine_t *e);

cs_obj_t *cs_engine_get_obj(cs_engine_t *e);

#ifdef __cplusplus
};
#endif

#endif /* _CS_H_ */
