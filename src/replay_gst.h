/*(GPLv2)
------------------------------------------------------------
   Kobo Redux - Game state logging/verification
------------------------------------------------------------
 * Copyright 2017 David Olofson
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

#ifndef	_KOBO_REPLAY_GST_H_
#define	_KOBO_REPLAY_GST_H_

#include "pfile.h"
#include "enemies.h"

// Game state data (debug/test/verification)
#define	KOBO_PF_GSTD_4CC	MAKE_4CC('G', 'S', 'T', 'D')
#define	KOBO_PF_GSTD_VERSION	2

class KOBO_replay_gst
{
	bool	status;
	void verify(const char *desc, const char *desc2,
			uint32_t snap, uint32_t current);
	void verify(const char *desc, const char *desc2,
			int32_t snap, int32_t current);
  public:
	KOBO_replay_gst();
	~KOBO_replay_gst();

	bool record();
	bool verify();

	bool save(pfile_t *pf);
	bool load(pfile_t *pf);

	KOBO_replay_gst	*next;

	uint32_t	frame;
	uint32_t	score;
	struct {
		int32_t		x, y;
		int16_t		health;
		int16_t		charge;
	} player;
	KOBO_enemystats	enemystats[KOBO_EK__COUNT];
};

#endif /* _KOBO_REPLAY_GST_H_ */
