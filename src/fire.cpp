/*(LGPLv2.1)
----------------------------------------------------------------------
	fire.cpp - Fire and smoke engine
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

#include "fire.h"
#include "gfxengine.h"
#include "kobo.h"
#include "logger.h"

// Debug: Define to disable "fire effect" filter, to see raw particles
#undef	FIRE_NOFILTER

// Debug: Define to render subtle noise around the edges of the buffer
#undef	FIRE_SHOW_EDGE


KOBO_ParticleFXDef::KOBO_ParticleFXDef()
{
	next = NULL;
	child = NULL;
	Reset();
}


KOBO_ParticleFXDef::~KOBO_ParticleFXDef()
{
	Reset();
}


void KOBO_ParticleFXDef::Reset()
{
	while(next)
	{
		KOBO_ParticleFXDef *nd = next;
		next = nd->next;
		nd->next = NULL;
		delete nd;
	}
	delete child;
	child = NULL;
	delay.Set(0.0f, 0.0f);
	threshold = 0;
	init_count = 256;
	xoffs.Set(0.0f, 0.0f);
	yoffs.Set(0.0f, 0.0f);
	radius.Set(0.0f, 0.0f);
	twist.Set(0.0f, 0.0f);
	speed.Set(0.0f, 0.0f);
	drag.Set(0.0f, 0.0f);
	heat.Set(0.0f, 0.0f);
	fade.Set(0.0f, 0.0f);
}


void KOBO_ParticleFXDef::Default()
{
	delay.Set(0.0f, 0.0f);
	threshold = 0;
	init_count = 256;
	xoffs.Set(0.0f, 0.0f);
	yoffs.Set(0.0f, 0.0f);
	radius.Set(3.0f, 8.0f, 0.0f);
	twist.Set(-0.25f, 0.25f, 0.5f);
	speed.Set(3.0f, 5.0f, 0.0f);
	drag.Set(0.85f, 0.85f, 0.9f);
	heat.Set(1.5f, 3.0f, 0.75f);
	fade.Set(0.8f, 0.9f, 0.9f);
}


KOBO_ParticleFXDef *KOBO_ParticleFXDef::Add()
{
	KOBO_ParticleFXDef *d = this;
	while(d->next)
		d = d->next;
	d->next = new KOBO_ParticleFXDef;
	return d->next;
}


KOBO_ParticleFXDef *KOBO_ParticleFXDef::Child()
{
	if(!child)
		child = new KOBO_ParticleFXDef;
	return child;
}


KOBO_Fire::KOBO_Fire(gfxengine_t *e) : stream_window_t(e)
{
	worldw = worldh = 0;
	viewmargin = 0;
	cxmin = cymin = 0;
	xmargin = ymargin = 0;
	bufw = bufh = 0;
	ncolors = 0;
	threshold = 0;
	buffers[0] = buffers[1] = NULL;
	current_buffer = 0;
	noisestate = 16576;
	blendmode(GFX_BLENDMODE_ALPHA);
	SetDither(GFX_DITHER_NOISE);
	SetFade(0.6f);
	need_refresh = true;
	autoinvalidate(true);
	psystems = NULL;
	psystempool = NULL;
	pscount = pcount = 0;
	standby_timer = FIRE_STANDBY_DELAY;
}


KOBO_Fire::~KOBO_Fire()
{
	free(buffers[0]);
	free(buffers[1]);
	while(psystems)
	{
		KOBO_ParticleSystem *ps = psystems;
		psystems = ps->next;
		delete ps;
	}
	while(psystempool)
	{
		KOBO_ParticleSystem *ps = psystempool;
		psystempool = ps->next;
		delete ps;
	}
}


void KOBO_Fire::SetPalette(unsigned pal)
{
	ncolors = engine->palette_size(pal) + 1;
	if(ncolors > FIRE_MAX_COLORS)
		ncolors = FIRE_MAX_COLORS;
	colors[0] = map_rgba(0, 0, 0, 0);
	unsigned i = 1;
	for( ; i < ncolors; ++i)
		colors[i] = map_rgb(engine->palette(pal, i - 1));
	for( ; i < FIRE_MAX_COLORS; ++i)
		colors[i] = map_rgb(0xff00ff);	// Error: Should never be seen!
	threshold = FIRE_HEAT_THRESHOLD / ncolors;
}


void KOBO_Fire::SetWorldSize(int w, int h)
{
	worldw = w;
	worldh = h;
}


void KOBO_Fire::UpdateViewSize()
{
	// Calculate tile granularity, power-of-two buffer size with margin
	if(width() && height())
	{
		unsigned w = width() + viewmargin;
		unsigned h = height() + viewmargin;
		bufw = FIRE_TILE_SIZE;
		while(bufw < w)
			bufw <<= 1;
		bufh = FIRE_TILE_SIZE;
		while(bufh < h)
			bufh <<= 1;
	}
	else
		bufw = bufh = 0;	// Invisible window ==> disable!

	xmargin = bufw - width();
	ymargin = bufh - height();

	// Update texture size
	buffersize(bufw, bufh);

	// Update internal rendering buffer size
	unsigned size = bufw * bufh * sizeof(Uint32);
	if(size)
	{
		Uint32 *nb = (Uint32 *)realloc(buffers[0], size);
		if(nb)
		{
			buffers[0] = nb;
			nb = (Uint32 *)realloc(buffers[1], size);
			if(nb)
				buffers[1] = nb;
			else
				size = 0;
		}
		else
			size = 0;
	}
	if(size && buffers[0] && buffers[1])
		Clear(true, false);
	else
	{
		free(buffers[0]);
		free(buffers[1]);
		bufw = bufh = 0;
		buffers[0] = buffers[1] = NULL;
	}
}


void KOBO_Fire::place(int left, int top, int sizex, int sizey)
{
	stream_window_t::place(left, top, sizex, sizey);
	UpdateViewSize();
}


void KOBO_Fire::scroll(int x, int y, bool wrap)
{
	stream_window_t::scroll(x, y, wrap);
	cxmin = mod(CS2PIXEL(scrollx) - xmargin / 2, worldw);
	cymin = mod(CS2PIXEL(scrolly) - ymargin / 2, worldh);
}


void KOBO_Fire::Clear(bool buffer, bool particles)
{
	if(buffer)
	{
		unsigned size = bufw * bufh * sizeof(Uint32);
		if(size)
		{
			memset(buffers[0], 0, size);
			memset(buffers[1], 0, size);
		}
		need_refresh = true;
	}

	if(particles)
		while(psystems)
		{
			KOBO_ParticleSystem *ps = psystems;
			psystems = ps->next;
			ps->next = psystempool;
			psystempool = ps;
		}

	standby_timer = 0;
}


bool KOBO_Fire::RunPSystem(KOBO_ParticleSystem *ps)
{
	int xmask = bufw - 1;
	int ymask = bufh - 1;
	Uint32 *dst = buffers[current_buffer];
	for(int i = 0; i < ps->nparticles; ++i)
	{
		KOBO_Particle *p = &ps->particles[i];

		// Update
		p->z = p->z * p->zc >> 12;
		if(p->z < ps->threshold)
		{
			// Dead! Remove.
			--ps->nparticles;
			ps->particles[i] = ps->particles[ps->nparticles];
			--i;
			continue;
		}
		p->x += p->dx;
		p->y += p->dy;
		p->dx = (p->dx >> 2) * p->drag >> 10;
		p->dy = (p->dy >> 2) * p->drag >> 10;

		// Render
		dst[bufw * ((p->y >> 16) & ymask) +
				((p->x >> 16) & xmask)] += p->z;
	}
	return ps->nparticles > 0;
}


void KOBO_Fire::RunParticles()
{
	pscount = pcount = 0;
	KOBO_ParticleSystem *ps = psystems;
	KOBO_ParticleSystem *pps = NULL;
	while(ps)
	{
		if(ps->delay)
		{
			--ps->delay;
			pps = ps;
			ps = ps->next;
		}
		else
		{
			++pscount;
			if(IsOnScreen(ps) && RunPSystem(ps))
			{
				pcount += ps->nparticles;
				// Next...
				pps = ps;
				ps = ps->next;
			}
			else
			{
				// Done! Remove particle system.
				KOBO_ParticleSystem *nps = ps->next;
				if(pps)
					pps->next = nps;
				else
					psystems = nps;
				ps->next = psystempool;
				psystempool = ps;
				ps = nps;
			}
		}
	}
}


bool KOBO_Fire::RunPSystemNR(KOBO_ParticleSystem *ps)
{
	for(int i = 0; i < ps->nparticles; ++i)
	{
		KOBO_Particle *p = &ps->particles[i];

		// Update
		p->z = p->z * p->zc >> 12;
		if(p->z < ps->threshold)
		{
			// Dead! Remove.
			--ps->nparticles;
			ps->particles[i] = ps->particles[ps->nparticles];
			--i;
			continue;
		}
		p->x += p->dx;
		p->y += p->dy;
		p->dx = (p->dx >> 2) * p->drag >> 10;
		p->dy = (p->dy >> 2) * p->drag >> 10;
	}
	return ps->nparticles > 0;
}


void KOBO_Fire::RunParticlesNR()
{
	pscount = pcount = 0;
	KOBO_ParticleSystem *ps = psystems;
	KOBO_ParticleSystem *pps = NULL;
	while(ps)
	{
		if(ps->delay)
		{
			--ps->delay;
			pps = ps;
			ps = ps->next;
		}
		else
		{
			++pscount;
			if(IsOnScreen(ps) && RunPSystemNR(ps))
			{
				pcount += ps->nparticles;
				// Next...
				pps = ps;
				ps = ps->next;
			}
			else
			{
				// Done! Remove particle system.
				KOBO_ParticleSystem *nps = ps->next;
				if(pps)
					pps->next = nps;
				else
					psystems = nps;
				ps->next = psystempool;
				psystempool = ps;
				ps = nps;
			}
		}
	}
}


KOBO_ParticleSystem *KOBO_Fire::NewPSystem(int x, int y, int vx, int vy,
		const KOBO_ParticleFXDef *fxd, int delay)
{
	KOBO_ParticleSystem *ps = NULL;

	int nparticles = fxd->init_count;
	if(nparticles > FIRE_MAX_PARTICLES)
	{
		log_printf(WLOG, "KOBO_Fire::Spawn() too many particles! "
				"(%d - max is %d)\n", nparticles,
				FIRE_MAX_PARTICLES);
		nparticles = FIRE_MAX_PARTICLES;
	}

	delay += RandRange(fxd->delay) >> 16;

	if(!fxd->child)
	{
		// Not nested - create actual PS, and generate particles
		if(psystempool)
		{
			ps = psystempool;
			psystempool = ps->next;
		}
		else
			ps = new KOBO_ParticleSystem;
		ps->next = psystems;
		psystems = ps;

		ps->delay = delay;
		ps->x = x >> 8;
		ps->y = y >> 8;

		if(ncolors)
			ps->threshold = fxd->threshold / ncolors;
		else
			ps->threshold = 0;
		if(ps->threshold < threshold)
			ps->threshold = threshold;
		ps->nparticles = nparticles;
	}

	// Convert position and velocity, and apply offset randomization
	x <<= 8;
	y <<= 8;
	x += RandRange(fxd->xoffs);
	y += RandRange(fxd->yoffs);
	vx <<= 8;
	vy <<= 8;

	// Initialize randomization ranges for all per-particle parameters
	int radius_min, radius_max;
	int twist_min, twist_max;
	int speed_min, speed_max;
	int drag_min, drag_max;
	int heat_min, heat_max;
	int fade_min, fade_max;
	RRPrepare(fxd->radius, radius_min, radius_max);
	RRPrepare(fxd->twist, twist_min, twist_max);
	RRPrepare(fxd->speed, speed_min, speed_max);
	RRPrepare(fxd->drag, drag_min, drag_max);
	RRPrepare(fxd->heat, heat_min, heat_max);
	RRPrepare(fxd->fade, fade_min, fade_max);

	// These are actually 20:12 internally, for headroom...
	drag_min >>= 4;
	drag_max >>= 4;
	fade_min >>= 4;
	fade_max >>= 4;

	for(int i = 0; i < nparticles; ++i)
	{
		float a = Noise() * M_PI / 32768.0f;
		float r = RandRange(radius_min, radius_max);
		float v = RandRange(speed_min, speed_max);
		int px = x + sinf(a) * r;
		int py = y + cosf(a) * r;
		a += RandRange(twist_min, twist_max) * M_PI / 32768.0f;
		int pvx = sinf(a) * v + vx;
		int pvy = cosf(a) * v + vy;
		if(ps)
		{
			KOBO_Particle *p = &ps->particles[i];
			p->x = px;
			p->y = py;
			p->dx = pvx;
			p->dy = pvy;
			p->drag = RandRange(drag_min, drag_max);
			p->z = RandRange(heat_min, heat_max);
			p->zc = RandRange(fade_min, fade_max);
		}
		else
			Spawn(px >> 8, py >> 8, pvx >> 8, pvy >> 8,
					fxd->child, delay);
	}

	// NOTE: We don't return a PS for nested effects! There is no actual
	// instance for the parent PS, so there's nothing sensible to return.
	return ps;
}


KOBO_ParticleSystem *KOBO_Fire::Spawn(int x, int y, int vx, int vy,
		const KOBO_ParticleFXDef *fxd, int delay)
{
	KOBO_ParticleSystem *first = NewPSystem(x, y, vx, vy, fxd, delay);
	for(fxd = fxd->next; fxd; fxd = fxd->next)
		NewPSystem(x, y, vx, vy, fxd, delay);
	return first;
}


static inline void fire_update(Uint32 *src, Uint32 *dst,
		int width, int height,
		int xmin, int ymin, int xmax, int ymax,
		bool wrap, int fade)
{
	int xmask = wrap ? width - 1 : -1;
	int ymask = height - 1;
	fade /= 10;	// Weighted filter window multiplies by 10
	for(int y = ymin; y <= ymax; ++y)
	{
		Uint32 *srow0 = &src[width * ((y - 1) & ymask)];
		Uint32 *srow1 = &src[width * y];
		Uint32 *srow2 = &src[width * ((y + 1) & ymask)];
		Uint32 *drow = &dst[width * y];
		for(int x = xmin; x <= xmax; ++x)
		{
#ifndef FIRE_NOFILTER
			int x0 = (x - 1) & xmask;
			int x2 = (x + 1) & xmask;
			Uint32 s = srow0[x];
			Uint32 sc = srow0[x0] + srow0[x2];
			s += srow1[x0] + (srow1[x] << 3) + srow1[x2];
			s += srow2[x];
			sc += srow2[x0] + srow2[x2];
			s += sc >> 1;
			s = s * fade >> 8;
			drow[x] = s;
#else
			drow[x] = srow1[x];
#endif
		}
	}
}

void KOBO_Fire::update()
{
	if(!bufw || !bufh)
		return;

	if(prefs->firebench)
		particle_prof.SampleBegin();

	// Run particle systems
#ifdef FIRE_NOFILTER
	memset(buffers[current_buffer], 0, bufw * bufh * sizeof(Uint32));
#endif
	RunParticles();

	if(prefs->firebench)
		particle_prof.SampleEnd();

	if(pcount)
		standby_timer = FIRE_STANDBY_DELAY;
	else
	{
		if(!standby_timer)
			return;
		--standby_timer;
	}

	if(prefs->firebench)
		filter_prof.SampleBegin();

	Uint32 *src = buffers[current_buffer];
	Uint32 *dst = buffers[!current_buffer];

#ifdef	FIRE_SHOW_EDGE
	for(unsigned x = 0; x < bufw; ++x)
		src[x] = src[(bufh - 1) * bufw + x] = 0;
	for(unsigned y = 1; y < bufh - 1; ++y)
		src[y * bufw] = src[y * bufw + bufw - 1] = 0;
#endif

	// Update the "fire filter" (overwrite previous buffer)
	fire_update(src, dst, bufw, bufh,
			1, 0,		bufw - 2, bufh - 1,
			false, fade);
	fire_update(src, dst, bufw, bufh,
			0, 0,		0, bufh - 1,
			true, fade);
	fire_update(src, dst, bufw, bufh,
			bufw - 1, 0,	bufw - 1, bufh - 1,
			true, fade);

#ifdef	FIRE_SHOW_EDGE
	for(unsigned x = 0; x < bufw; ++x)
	{
		dst[x] = Noise() >> 1;
		dst[(bufh - 1) * bufw + x] = Noise() >> 2;
	}
	for(unsigned y = 1; y < bufh - 1; ++y)
	{
		dst[y * bufw] = Noise() >> 1;
		dst[y * bufw + bufw - 1] = Noise() >> 2;
	}
#endif

	// Swap buffers
	current_buffer = !current_buffer;

	need_refresh = true;

	if(prefs->firebench)
		filter_prof.SampleEnd();
}


void KOBO_Fire::update_norender()
{
	if(!bufw || !bufh)
		return;

	// Run particle systems
	RunParticlesNR();
}


void KOBO_Fire::refresh(SDL_Rect *r)
{
	if(!need_refresh || !bufw || !bufh || !ncolors)
		return;

	// Source
	Uint32 *srcbuf = buffers[current_buffer];

	// Destination
	Uint32 *dstbuf;
	int pitch = lock(r, &dstbuf);
	if(!pitch)
	{
		log_printf(ELOG, "KOBO_Fire::refresh() failed to lock "
				"buffer!\n");
		return;
	}

	if(prefs->firebench)
		render_prof.SampleBegin();

	// Render!
	for(unsigned y = 0; y < bufh; ++y)
	{
		Uint32 *src = &srcbuf[bufw * y];
		Uint32 *dst = &dstbuf[pitch * y];
		int sx = 0;
		switch(dither)
		{
		  default:
		  case GFX_DITHER_NONE:
			for(unsigned x = 0; x < bufw; ++x)
			{
				unsigned n = src[x] * ncolors >> 16;
				if(n >= ncolors)
					n = ncolors - 1;
				dst[x] = colors[n];
			}
			break;
		  case GFX_DITHER_2X2:
			for(unsigned x = 0; x < bufw; ++x)
			{
				unsigned n = src[x] * ncolors;
				n >>= 12;
				n += (((x + sx) ^ y) & 1) << 3;
				n >>= 4;
				if(n >= ncolors)
					n = ncolors - 1;
				dst[x] = colors[n];
			}
			break;
		  case GFX_DITHER_SKEWED:
			sx = (y & 2) >> 1;
			// Fall-through!
		  case GFX_DITHER_ORDERED:
			for(unsigned x = 0; x < bufw; ++x)
			{
				unsigned n = src[x] * ncolors;
				n >>= 12;
				n += (((((x + sx) ^ y) & 1) << 1) + (y & 1))
						<< 2;
				n >>= 4;
				if(n >= ncolors)
					n = ncolors - 1;
				dst[x] = colors[n];
			}
			break;
		  case GFX_DITHER_NOISE:
			for(unsigned x = 0; x < bufw; ++x)
			{
				unsigned n = src[x] * ncolors;
				n >>= 14;
				n += Noise() & 3;
				n >>= 2;
				if(n >= ncolors)
					n = ncolors - 1;
				dst[x] = colors[n];
			}
			break;
		}
	}

	if(prefs->firebench)
		render_prof.SampleEnd();

	unlock();

	need_refresh = false;
}
