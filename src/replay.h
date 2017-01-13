/*(GPLv2)
------------------------------------------------------------
   Kobo Redux - Replay/gamesave logger
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

#include "myship.h"

#ifndef	_KOBO_REPLAY_H_
#define	_KOBO_REPLAY_H_

enum KOBO_player_controls
{
	KOBO_PC_NONE =			0,

	KOBO_PC_DIR =			0x0f,	// Dirs 1..8; 0 is "neutral"

	KOBO_PC_PRIMARY_DOWN =		0x10,	// Primary fire just pressed
	KOBO_PC_PRIMARY =		0x20,	// Primary fire held down
	KOBO_PC_SECONDARY_DOWN =	0x40,	// Secondary fire just pressed
	KOBO_PC_SECONDARY =		0x80,	// Secondary fire held down
	KOBO_PC_FIRE =		KOBO_PC_PRIMARY_DOWN | KOBO_PC_PRIMARY |
				KOBO_PC_SECONDARY_DOWN | KOBO_PC_SECONDARY,

	KOBO_PC_END =			0xff00	// End-of-replay
};

class KOBO_replay
{
	int	version;	// Game logic version
	Uint32	config;		// Compilation of relevant config options

	// Player input log
	int	bufsize;	// Physical size of buffer
	int	bufrecord;	// Number of frames recorded
	int	bufplay;	// Current playback frame
	Uint8	*buffer;

	Uint32 get_config();
	void write(Uint8 b);
  public:
	KOBO_replay();
	virtual ~KOBO_replay();

	// Initial state description
	int	stage;
	int	type;		// game_types_t
	int	skill;		// skill_levels_t
	Uint32	seed;		// Seed for gamerand
	int	health;		// Player health
	int	charge;		// Player secondary weapon accumulator charge
	int	score;		// Player score

	// Record
	void write(KOBO_player_controls ctrl);	// Record + advance

	// Playback
	void rewind();				// Rewind to start
	KOBO_player_controls read();		// Read + advance

	// Punch-in (Start recording at the current playback position,
	// overwriting any previous data from this point on.)
	void punchin();

	void log_dump(int level);

	int recorded()		{ return bufrecord; }
	float progress();
};

#endif // _KOBO_REPLAY_H_
