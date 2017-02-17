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

/*
 * Save/replay versioning
 *
 *	KOBO_REPLAY_VERSION
 *		Integer 8:8:8:8 version code that is bumped to the current game
 *		version (KOBO_VERSION) whenever a compatibility breaking change
 *		(that is, basically any change at all) is made to the game
 *		logic code.
 *		   Some versions of the game may accept the initial state part
 *		from older version saves with some restrictions, but replay
 *		data will just not work, as it's totally dependent on the game
 *		logic.
 *
 *	KOBO_replay.version
 *		Initialized to KOBO_REPLAY_VERSION as a KOBO_replay is created.
 *
 *	KOBO_replay.gameversion
 *		Initialized to KOBO_VERSION. Intended for debugging purposes,
 *		and may be checked in special cases, if unintentional
 *		compatibility breaking changes are discovered.
 */

#ifndef	_KOBO_REPLAY_H_
#define	_KOBO_REPLAY_H_

#include "pfile.h"
#include <time.h>

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

enum KOBO_replay_compat
{
	KOBO_RPCOM_NONE = 0,
	KOBO_RPCOM_LIMITED,
	KOBO_RPCOM_FULL
};

class KOBO_replay
{
	int	version;	// Game logic version
	int	gameversion;	// Version of game binary that generated this
	Uint32	config;		// Compilation of relevant config options
	time_t	starttime;	// Level start time

	// Player input log
	int	bufsize;	// Physical size of buffer
	int	bufrecord;	// Number of frames recorded
	int	bufplay;	// Current playback frame
	Uint8	*buffer;

	KOBO_replay_compat	compat;	// Compatibility status of current data

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

	// Compatibility check
	KOBO_replay_compat compatibility()	{ return compat; };

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
