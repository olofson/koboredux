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

#include "kobo.h"
#include "kobolog.h"
#include "manage.h"
#include "random.h"
#include "replay.h"

// Initial buffer size (enough for about an hour at 30 ms/frame)
#define	KOBO_REPLAY_BUFFER	256	//131072


KOBO_replay::KOBO_replay()
{
	clear();

	version = KOBO_REPLAY_VERSION;
	gameversion = KOBO_VERSION;
	config = get_config();
	endtime = starttime = time(NULL);

	bufsize = KOBO_REPLAY_BUFFER;
	bufrecord = bufplay = 0;
	buffer = (Uint8 *)malloc(bufsize);
	if(!buffer)
		log_printf(ELOG, "Out of memory! Limited save/replay "
				"capabilities.\n");

	compat = KOBO_RPCOM_FULL;
	_modified = true;
}


KOBO_replay::~KOBO_replay()
{
	free(buffer);
}


void KOBO_replay::clear()
{
	version = gameversion = 0;
	config = 0;
	starttime = endtime = 0;
	stage = type = skill = 0;
	seed = 0;
	health = charge = score = 0;
	end_health = end_charge = end_score = deaths = 0;
}


static inline void rp_get_bool(int v, Uint32 &c)
{
	c <<= 1;
	if(v)
		c |= 1;
}


Uint32 KOBO_replay::get_config()
{
	Uint32 c = 0;
	rp_get_bool(prefs->cheat_pushmove, c);
	rp_get_bool(prefs->cheat_freewheel, c);
	rp_get_bool(prefs->cheat_shield, c);
	rp_get_bool(prefs->cheat_invulnerability, c);
	rp_get_bool(prefs->cheat_ceasefire, c);
	rp_get_bool(prefs->cheat_firepower, c);
	rp_get_bool(prefs->cheat_brokentrigger, c);
	return c;
}


void KOBO_replay::write(KOBO_player_controls ctrl)
{
	// TODO:
	//	RLE? Although hardly worth the effort, and if we're saving or
	//	transmitting this data, we'll probably compress it anyway!
	//
	if(!buffer)
		return;

	if(bufrecord >= bufsize)
	{
		int nbs = bufsize * 3 / 2;
		Uint8 *nb = (Uint8 *)realloc(buffer, nbs);
		if(!nb)
			return;	// OOM...!? Not cool!

		buffer = nb;
		bufsize = nbs;
	}
	buffer[bufrecord++] = ctrl;
	_modified = true;
}


void KOBO_replay::rewind()
{
	bufplay = 0;
}


KOBO_player_controls KOBO_replay::read()
{
	if(!buffer || (bufplay >= bufrecord))
	{
		++bufplay;
		return KOBO_PC_END;
	}

	return (KOBO_player_controls)buffer[bufplay++];
}


void KOBO_replay::punchin()
{
	if(bufplay > bufrecord)
	{
		log_printf(ELOG, "WARNING: KOBO_replay::punchin() beyond "
				"end of replay data!\n");
		return;
	}
	bufrecord = bufplay;
}


void KOBO_replay::log_dump(int level, bool header, bool data)
{
	if(header && data)
		log_printf(level, " .- Replay -----------------------\n");
	else if(header)
		log_printf(level, " .- Replay Header ----------------\n");
	else
		log_printf(level, " .- Replay Data ------------------\n");
	if(header)
	{
		log_printf(level, " |     version: %d.%d.%d.%d\n",
				KOBO_MAJOR(version), KOBO_MINOR(version),
				KOBO_MICRO(version), KOBO_BUILD(version));
		log_printf(level, " | gameversion: %d.%d.%d.%d\n",
				KOBO_MAJOR(gameversion),
				KOBO_MINOR(gameversion),
				KOBO_MICRO(gameversion),
				KOBO_BUILD(gameversion));
		log_printf(level, " |      config: %8.8x\n", config);
		log_printf(level, " |   starttime: %s", ctime(&starttime));
		log_printf(level, " |       stage: %d\n", stage);
		log_printf(level, " |        type: %d\n", type);
		log_printf(level, " |       skill: %d\n", skill);
		log_printf(level, " |        seed: %u\n", seed);
		log_printf(level, " |      health: %d\n", health);
		log_printf(level, " |      charge: %d\n", charge);
		log_printf(level, " |       score: %d\n", score);
		log_printf(level, " |  end_health: %d\n", end_health);
		log_printf(level, " |  end_charge: %d\n", end_charge);
		log_printf(level, " |   end_score: %d\n", end_score);
		log_printf(level, " |      deaths: %d\n", deaths);
		const char *cmp = "<unknown>";
		switch(compat)
		{
		  case KOBO_RPCOM_NONE:		cmp = "NONE";		break;
		  case KOBO_RPCOM_LIMITED:	cmp = "LIMITED";	break;
		  case KOBO_RPCOM_FULL:		cmp = "FULL";		break;
		}
		log_printf(level, " |      compat: %s\n", cmp);
	}

	if(header && data)
		log_printf(level, " |- Data -------------------------\n");

	if(data)
		log_printf(level, " |    recorded: %d frames\n", bufrecord);

	log_printf(level, " '--------------------------------\n");
}


float KOBO_replay::progress()
{
	if(!bufrecord)
		return 0.0f;
	if(bufplay >= bufrecord)
		return 1.0f;
	return (float)bufplay / bufrecord;
}


bool KOBO_replay::load_reph(pfile_t *pf)
{
	// Clear modified flag unconditionally, as we'll either end up with
	// data just loaded from file, or corrupt data we don't want to save.
	clear();
	_modified = false;

	// Game versioning
	pf->read(version);
	pf->read(gameversion);

	if(version == KOBO_REPLAY_VERSION)
		compat = KOBO_RPCOM_FULL;
	else if(version > KOBO_REPLAY_VERSION)
	{
		// This is from a newer version, so we can't safely load this!
		compat = KOBO_RPCOM_NONE;
		if(prefs->debug)
			log_dump(ULOG);
		pf->chunk_end();
		return false;
	}
	else
	{
		// TODO: Add logic to upgrade the initial parameters parts from
		// older game versions, so one can at least continue a campaign
		// across updates, even if it won't result in a usable full
		// game replay.
		compat = KOBO_RPCOM_LIMITED;
	}

	pf->read(config);
	if(config != get_config())
	{
		// Incompatible configuration! We can load this just fine, but
		// it's cheating, and the replay data most likely won't work!
		compat = KOBO_RPCOM_LIMITED;
	}

	// Timestamps
	struct tm t;
	pf->read(t);
	starttime = timegm(&t);
	pf->read(t);
	endtime	= timegm(&t);

	// Game parameters
	pf->read(stage);
	pf->read(type);
	pf->read(skill);

	// Initial game state
	pf->read(seed);
	pf->read(health);
	pf->read(charge);
	pf->read(score);

	// Final game state
	pf->read(end_health);
	pf->read(end_charge);
	pf->read(end_score);

	// Total number of deaths/rewinds on this stage during this campaign
	pf->read(deaths);

	pf->chunk_end();

	if(prefs->debug)
		log_dump(ULOG, true, false);

	return !pf->status();
}


bool KOBO_replay::load_repd(pfile_t *pf)
{
	pf->read(bufrecord);
	if(bufrecord > bufsize)
	{
		Uint8 *nb = (Uint8 *)realloc(buffer, bufrecord);
		if(!nb)
		{
			// Ugly handling of an extremely unlikely situation...
			free(buffer);
			buffer = NULL;
			bufsize = bufrecord = bufplay = 0;
			pf->chunk_end();
			return false;
		}
		buffer = nb;
		bufsize = bufrecord;
	}
	if(buffer)
		pf->read(buffer, bufrecord);

	pf->chunk_end();

	if(prefs->debug)
		log_dump(ULOG, false, true);

	return !pf->status();
}


bool KOBO_replay::load(pfile_t *pf)
{
	switch(pf->chunk_type())
	{
	  case KOBO_PF_REPH_4CC:
		return load_reph(pf);
	  case KOBO_PF_REPD_4CC:
		return load_repd(pf);
	  default:
		return false;
	}
}


bool KOBO_replay::save(pfile_t *pf)
{
	endtime = time(NULL);

	//
	// REPH (header)
	//
	pf->chunk_write(KOBO_PF_REPH_4CC, KOBO_PF_REPH_VERSION);

	// Game versioning
	pf->write(version);
	pf->write(gameversion);
	pf->write(config);

	// Timestamps
	pf->write(gmtime(&starttime));
	pf->write(gmtime(&endtime));

	// Game parameters
	pf->write(stage);
	pf->write(type);
	pf->write(skill);

	// Initial game state
	pf->write(seed);
	pf->write(health);
	pf->write(charge);
	pf->write(score);

	// Final game state
	pf->write(end_health);
	pf->write(end_charge);
	pf->write(end_score);

	// Total number of deaths/rewinds on this stage during this campaign
	pf->write(deaths);

	pf->chunk_end();

	//
	// REPD (data)
	//
	if(bufrecord)
	{
		pf->chunk_write(KOBO_PF_REPD_4CC, KOBO_PF_REPD_VERSION);

		pf->write(bufrecord);
		pf->write(buffer, bufrecord);

		pf->chunk_end();
	}

	return !pf->status();
}
