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
struct KOBO_RandSpec {
	// Per system base value range (16:16 fixp)
	//	For each new particle system, a single base value will be
	//	randomized in the range [bmin, bmax].
	int	bmin, bmax;

	// Per particle relative variation range (16:16 fixp)
	//	For each particle issued, the corresponding value is randomly
	//	selected in the range [rmin * base, rmax * base].
	int	rmin, rmax;

	// Per particle absolute variation range (16:16 fixp)
	//	For each particle issued, an random value in the absolute range
	//	[0, noise] is added to the value calculated as described above.
	int	noise;

	void Set(float basemin, float basemax,
			float relmin = 1.0f, float relmax = 1.0f,
			float absnoise = 0.0f)
	{
		bmin = basemin * 65536.0f;
		bmax = basemax * 65536.0f;
		rmin = relmin * 65536.0f;
		rmax = relmax * 65536.0f;
		noise = absnoise * 65536.0f;
	}
};

class KOBO_ParticleFXDef {
  public:

	// General parameters
	int		threshold;	// Death "heat" level

	// Initial state
	int		init_count;	// Number of initial particles
	KOBO_RandSpec	radius;		// Initial radius (pixels)

	// Behavior of initial particles
	KOBO_RandSpec	twist;		// Init pos/speed vector deflection
	KOBO_RandSpec	speed;		// Speed range (pixels/frame)
	KOBO_RandSpec	drag;		// Drag coefficient (16:16 fixp coeff)
	KOBO_RandSpec	heat;		// Initial heat range (16:16 fixp)
	KOBO_RandSpec	fade;		// Fade rate range (16:16 fixp coeff)

	// Particle spawning
	// TODO

	KOBO_ParticleFXDef()
	{
		threshold = 0;
		init_count = 256;
		radius.Set(3.0f, 8.0f, 0.0f);
		twist.Set(-0.25f, 0.25f, 0.5f);
		speed.Set(3.0f, 5.0f, 0.0f);
		drag.Set(0.85f, 0.85f, 0.9f);
		heat.Set(1.5f, 3.0f, 0.75f);
		fade.Set(0.8f, 0.9f, 0.9f);
	}
};

// Single particle
struct KOBO_Particle {
	int	x, y, z;	// Position and heat (16:16)
	int	dx, dy;		// Velocity (16:16)
	int	zc;		// Cool-down ratio (20:12)
	int	drag;		// Drag coefficient (20:12)
};

// Particle system
struct KOBO_ParticleSystem {
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

	void RRPrepare(const KOBO_RandSpec &rs, int &min, int &max)
	{
		int base = RandRange(rs.bmin, rs.bmax);
		min = (int64_t)rs.rmin * base >> 16;
		max = ((int64_t)rs.rmax * base >> 16) + rs.noise;
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
	void refresh(SDL_Rect *r);

	// Spawn new particle system as specified by 'fxd', at (x, y), with an
	// initial velocity bias of (vx, vy). Coordinates are 24:8 fixp.
	KOBO_ParticleSystem *NewPSystem(int x, int y, int vx, int vy,
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
