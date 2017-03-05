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
#include "replay_gst.h"
#include "gamectl.h"

// Replay header
#define	KOBO_PF_REPH_4CC	MAKE_4CC('R', 'E', 'P', 'H')
#define	KOBO_PF_REPH_VERSION	1

// Replay data
#define	KOBO_PF_REPD_4CC	MAKE_4CC('R', 'E', 'P', 'D')
#define	KOBO_PF_REPD_VERSION	1

// Minimal acceptable length of a replay. (Game logic frames) This is used to
// determine whether or not a replay is long enough to start the game in replay
// mode, or if we should just start in "Get Ready" state instead.
#define	KOBO_MIN_REPLAY_LENGTH	30

enum KOBO_replay_compat
{
	KOBO_RPCOM_NONE = 0,
	KOBO_RPCOM_LIMITED,
	KOBO_RPCOM_FULL
};

enum KOBO_replay_logdump
{
	KOBO_RLD_ALL,
	KOBO_RLD_HEADER,
	KOBO_RLD_REPLAY,
	KOBO_RLD_GAMESTATE
};

class KOBO_replay
{
	int32_t		version;	// Game logic version
	int32_t		gameversion;	// Game version that generated this
	uint32_t	config;		// Relevant config switches

	// Player input log
	int32_t		bufsize;	// Physical size of buffer
	int32_t		bufrecord;	// Number of frames recorded
	int32_t		bufplay;	// Current playback frame
	uint8_t		*buffer;

	KOBO_replay_gst	*gst_first;	// List head
	KOBO_replay_gst	*gst_last;	// List tail
	KOBO_replay_gst	*gst_current;	// Current item

	KOBO_replay_compat	compat;	// Compatibility status of current data

	bool		_modified;

	uint32_t get_config();
	void write(uint8_t b);
	void clear();
	bool load_reph(pfile_t *pf);
	bool load_repd(pfile_t *pf);
	bool load_gstd(pfile_t *pf);
  public:
	KOBO_replay();
	virtual ~KOBO_replay();

	bool modified()		{ return _modified; }
	void modified(bool m)	{ _modified = m; }

	// Game parameters
	int32_t		stage;
	int32_t		type;		// game_types_t
	int32_t		skill;		// skill_levels_t

	// Initial game state
	time_t		starttime;	// Level start time
	uint32_t	seed;		// Seed for gamerand
	int32_t		health;		// Player health
	int32_t		charge;		// Player secondary weapon accu charge
	int32_t		score;		// Player score

	// Final game state
	time_t		endtime;	// End/save time
	int32_t		end_health;
	int32_t		end_charge;
	int32_t		end_score;

	// Total number of deaths/rewinds on this stage during this campaign
	int32_t		deaths;

	// Record
	void write(KOBO_player_controls ctrl);	// Record + advance
	void compact();		// Drop preallocated buffer space to save RAM

	// Compatibility check
	KOBO_replay_compat compatibility()	{ return compat; }

	// Playback
	void rewind();				// Rewind to start
	KOBO_player_controls read();		// Read + advance

	// Punch-in (Start recording at the current playback position,
	// overwriting any previous data from this point on.)
	void punchin();

	void log_dump(int level, KOBO_replay_logdump rld = KOBO_RLD_ALL);

	int recorded()		{ return bufrecord; }
	float progress();

	// Game state snapshots
	bool record_state();	// Record snapshot of current game state
	bool verify_state();	// Verify game state against snapshot, if any

	// Write REPH, REPD, and (optionally) GSTD chucks to campaign file
	bool save(pfile_t *pf);

	// Read REPH, REPD, or GSTD chuck from campaign file
	// NOTE: This call expects chunk_read() to have been called first!
	bool load(pfile_t *pf);
};

#endif // _KOBO_REPLAY_H_
