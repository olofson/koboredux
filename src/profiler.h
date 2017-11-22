/*(LGPLv2.1)
----------------------------------------------------------------------
	profiler.h - Performance counter based profiling tool
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

#ifndef	KOBO_PROFILER_H
#define	KOBO_PROFILER_H

#include "SDL.h"

class KOBO_Profiler
{
  protected:
	Uint64	f;			// Counter frequency (Hz)
	int	samples;		// Samples taken
	Uint64	period_start;
	Uint64	sample_start;		// Sample start
	Uint64	total;			// Total cycles spent

	// Result
	int	r_samples;
	double	r_average_time;		// ms per sample
  public:
	KOBO_Profiler()
	{
		f = SDL_GetPerformanceFrequency();
		r_samples = samples = 0;
		period_start = 0;
		sample_start = 0;
		total = 0;
		r_average_time = 0.0f;
		Period();
	}

	void SampleBegin()
	{
		sample_start = SDL_GetPerformanceCounter();
	}

	void SampleEnd()
	{
		Uint64 t = SDL_GetPerformanceCounter();
		total += t - sample_start;
		++samples;
	}

	void Period()
	{
		Uint64 t = SDL_GetPerformanceCounter();
		r_samples = samples;
		if(r_samples)
		{
			r_average_time = (double)total / r_samples / f *
					1000.0f;
		}
		else
		{
			r_average_time = 0.0f;
		}
		period_start = t;
	}

	float PeriodDuration()
	{
		Uint64 t = SDL_GetPerformanceCounter();
		return (double)(t - period_start) / f;
	}

	int Samples()
	{
		return r_samples;
	}

	float AverageTime()
	{
		return r_average_time;
	}
};

#endif // KOBO_PROFILER_H
