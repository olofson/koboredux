/*(LGPLv2.1)
----------------------------------------------------------------------
	fire.h - Fire and smoke engine
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

/*
	Coordinate systems explained
	----------------------------
	This little engine is designed primarily for 2D scrolling games where
	the map wraps when scrolling past the edges. In order to handle this
	transparently, the particle systems are aware of the full map, and
	implement coordinate wrapping and off-screen culling accordingly.

	However, the rendering part (typically) uses a smaller buffer, just
	slightly larger than the on-screen viewport, with its own wrapping
	intervals. Rendering wraps around the edges of this buffer, so that
	(limited) asynchronous scrolling can be implemented without actually
	moving contents in the buffer.

	That is, it's possible to smooth scroll the viewport at any frame rate,
	as long as there is enough buffer margin outside the viewport that one
	doesn't outrun the engine updates.


	Particle system culling
	-----------------------
	The viewport scroll margin, set via SetViewMargin(), should be selected
	so that it is at least wide enough to contain any particle system as it
	reaches its maximum size. This allows the engine to cull off-screen
	particle systems, rather than individual particles.
*/

#ifndef	KOBO_FIRE_H
#define	KOBO_FIRE_H

#include "window.h"
#include "mathutil.h"

// Maximum number of particles in one particle system
#define	FIRE_MAX_PARTICLES	1024

// World size granularity
// (TODO: Tile subdivision for rendering optimizations)
#define	FIRE_TILE_SIZE		32

// Maximum colors supported, including transparency
#define	FIRE_MAX_COLORS		16

// Minimum heat threshold for particles. (16:16)
//	1.0 (65536) would kill particles as soon as they translate to color
//	entry 0. However, since particle rendering is additive, particles below
//	that level can still affect the final output, and thus, the threshold
//	is preferably set well below that.
#define	FIRE_HEAT_THRESHOLD	32768

// Particle effect definition
struct KOBO_RangeSpec
{
	int	min, max;	// Min/max limits (16:16 fixp)

	void Set(float _min, float _max)
	{
		min = _min * 65536.0f;
		max = _max * 65536.0f;
	}
};

struct KOBO_RandSpec
{
	// Per system base value range (16:16 fixp)
	//	For each new particle system, a single base value will be
	//	randomized in the range [bmin, bmax].
	KOBO_RangeSpec	base;

	// Per particle relative variation range (16:16 fixp)
	//	For each particle issued, the corresponding value is randomly
	//	selected in the range [rmin * base, rmax * base].
	KOBO_RangeSpec	relative;

	// Per particle absolute variation range (16:16 fixp)
	//	For each particle issued, an random value in the absolute range
	//	[0, noise] is added to the value calculated as described above.
	int	noise;

	void Set(float basemin, float basemax,
			float relmin = 1.0f, float relmax = 1.0f,
			float absnoise = 0.0f)
	{
		base.Set(basemin, basemax);
		relative.Set(relmin, relmax);
		noise = absnoise * 65536.0f;
	}
};

class KOBO_ParticleFXDef
{
	friend class KOBO_Fire;
	KOBO_ParticleFXDef	*next;
  public:

	// General parameters
	int		threshold;	// Death "heat" level

	// Initial state
	int		init_count;	// Number of initial particles
	KOBO_RangeSpec	xoffs;		// Horizontal offset (pixels)
	KOBO_RangeSpec	yoffs;		// Vertical offset (pixels)
	KOBO_RandSpec	radius;		// Initial radius (pixels)

	// Behavior of initial particles
	KOBO_RandSpec	twist;		// Init pos/speed vector deflection
	KOBO_RandSpec	speed;		// Speed range (pixels/frame)
	KOBO_RandSpec	drag;		// Drag coefficient (16:16 fixp coeff)
	KOBO_RandSpec	heat;		// Initial heat range (16:16 fixp)
	KOBO_RandSpec	fade;		// Fade rate range (16:16 fixp coeff)

	// Particle spawning
	// TODO

	KOBO_ParticleFXDef();
	~KOBO_ParticleFXDef();

	// Add new particle system definition to cluster
	KOBO_ParticleFXDef *Add()
	{
		KOBO_ParticleFXDef *d = this;
		while(d->next)
			d = d->next;
		d->next = new KOBO_ParticleFXDef;
		return d->next;
	}
};

// Single particle
struct KOBO_Particle
{
	int	x, y, z;	// Position and heat (16:16)
	int	dx, dy;		// Velocity (16:16)
	int	zc;		// Cool-down ratio (20:12)
	int	drag;		// Drag coefficient (20:12)
};

// Particle system
struct KOBO_ParticleSystem
{
	KOBO_ParticleSystem	*next;
	int			x, y;
	// TODO: <parameters for issuing new particles>
	int			threshold;
	int			nparticles;
	KOBO_Particle		particles[FIRE_MAX_PARTICLES];
};

// Particle systems + "fire effect" engine
class KOBO_Fire : public stream_window_t
{
	// World map size
	unsigned	worldw, worldh;

	// Minimum viewport/buffer scroll margin
	unsigned	viewmargin;		// Requested
	unsigned	xmargin, ymargin;	// Actual

	// Current culling window, top/left corner
	int		cxmin, cymin;

	// Work buffer (16:16 fixed point "heat" values)
	unsigned	bufw, bufh, current_buffer;
	Uint32		*buffers[2];
	bool		need_refresh;	// Buffer needs refresh to texture

	void UpdateViewSize();

	// Colors
	unsigned	ncolors;
	Uint32		colors[FIRE_MAX_COLORS];
	gfx_dither_t	dither;
	int		fade;		// Fade-out coefficient (24:8)
	int		threshold;	// Minimum particle heat threshold

	// Particles
	KOBO_ParticleSystem	*psystems;	// Active particle systems
	KOBO_ParticleSystem	*psystempool;	// Particle system free pool
	int pscount, pcount;
	bool RunPSystem(KOBO_ParticleSystem *ps);
	void RunParticles();
	bool RunPSystemNR(KOBO_ParticleSystem *ps);
	void RunParticlesNR();

	// Particle systems
	KOBO_ParticleSystem *NewPSystem(int x, int y, int vx, int vy,
			const KOBO_ParticleFXDef *fxd);

	// RNG
	unsigned noisestate;
	inline int Noise()
	{
		noisestate *= 1566083941UL;
		noisestate++;
		return (int)(noisestate * (noisestate >> 16) >> 16);
	}
	inline int RandRange(int min, int max)
	{
		return min + ((int64_t)Noise() * (max - min) >> 16);
	}
	inline int RandRange(const KOBO_RangeSpec &rs)
	{
		return RandRange(rs.min, rs.max);
	}

	void RRPrepare(const KOBO_RandSpec &rs, int &min, int &max)
	{
		int base = RandRange(rs.base);
		min = (int64_t)rs.relative.min * base >> 16;
		max = ((int64_t)rs.relative.max * base >> 16) + rs.noise;
	}

	bool IsOnScreen(KOBO_ParticleSystem *ps)
	{
		if(mod(ps->x - cxmin, worldw) >= (int)bufw)
			return false;
		if(mod(ps->y - cymin, worldh) >= (int)bufh)
			return false;
		return true;
	}

  public:
	KOBO_Fire(gfxengine_t *e);
	~KOBO_Fire();

	// NOTE: Call before place(). (Simple hack to avoid reallocations...)
	void SetViewMargin(int vm)		{ viewmargin = vm; }

	void place(int left, int top, int sizex, int sizey);
	void scroll(int x, int y, bool wrap);

	void SetPalette(unsigned pal);
	void SetDither(gfx_dither_t dth)	{ dither = dth; }
	void SetFade(float coeff)		{ fade = coeff * 256.0f; }

	// Wold map size (set to 0 to disable wrapping)
	void SetWorldSize(int w, int h);

	void update();
	void update_norender();
	void refresh(SDL_Rect *r);

	// Spawn new particle system as specified by 'fxd', at (x, y), with an
	// initial velocity bias of (vx, vy). Coordinates are 24:8 fixp.
	KOBO_ParticleSystem *Spawn(int x, int y, int vx, int vy,
			const KOBO_ParticleFXDef *fxd);

	bool PSVisible(KOBO_ParticleSystem *ps)
	{
		if(!ps->nparticles)
			return false;
		return IsOnScreen(ps);
	}
	void Clear(bool buffer = true, bool particles = false);

	int StatPSystems()	{ return pscount; }
	int StatParticles()	{ return pcount; }
};

#endif // KOBO_FIRE_H
