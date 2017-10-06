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
#include "replay_gst.h"

// Initial buffer size (enough for about 65 minutes at 30 ms/frame)
#define	KOBO_REPLAY_BUFFER	131072


KOBO_replay::KOBO_replay()
{
	buffer = NULL;
	gst_first = gst_last = gst_current = NULL;
	clear();
}


KOBO_replay::~KOBO_replay()
{
	reset();
}


void KOBO_replay::reset()
{
	free(buffer);
	buffer = NULL;
	bufsize = 0;
	bufrecord = bufplay = 0;

	while(gst_first)
	{
		KOBO_replay_gst *gst = gst_first;
		gst_first = gst->next;
		delete gst;
	}
	gst_last = gst_current = NULL;

	version = KOBO_REPLAY_VERSION;
	gameversion = KOBO_VERSION;
	config = get_config();
	endtime = starttime = time(NULL);
	compat = KOBO_RPCOM_FULL;
	_modified = true;
}


void KOBO_replay::clear()
{
	reset();

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
	if(prefs)
	{
		rp_get_bool(prefs->cheat_pushmove, c);
		rp_get_bool(prefs->cheat_freewheel, c);
		rp_get_bool(prefs->cheat_shield, c);
		rp_get_bool(prefs->cheat_invulnerability, c);
		rp_get_bool(prefs->cheat_ceasefire, c);
		rp_get_bool(prefs->cheat_firepower, c);
		rp_get_bool(prefs->cheat_brokentrigger, c);
	}
	return c;
}


void KOBO_replay::write(KOBO_player_controls ctrl)
{
	if(bufrecord >= bufsize)
	{
		int nbs = bufsize * 3 / 2;
		if(nbs < KOBO_REPLAY_BUFFER)
			nbs = KOBO_REPLAY_BUFFER;
		Uint8 *nb = (Uint8 *)realloc(buffer, nbs);
		if(!nb)
		{
			log_printf(ELOG, "OOM in KOBO_replay::write()!\n");
			return;	// OOM...!? Not cool!
		}
		buffer = nb;
		bufsize = nbs;
	}
	buffer[bufrecord++] = ctrl;
	_modified = true;
}


void KOBO_replay::compact()
{
	Uint8 *nb = (Uint8 *)realloc(buffer, bufrecord);
	if(!nb && bufrecord)
	{
		log_printf(ELOG, "OOM in KOBO_replay::compact()!\n");
		return;	// Wut? OOM when releasing memory...?
	}
	buffer = nb;
	bufsize = bufrecord;
}


void KOBO_replay::rewind()
{
	bufplay = 0;
	gst_current = gst_first;
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
	unsigned punchframe = manage.game_time();

	if(punchframe > bufrecord)
	{
		log_printf(ELOG, "KOBO_replay::punchin() beyond end of replay "
				"data! This will break replay.\n");
		return;
	}

	if(prefs->debug)
		log_printf(ULOG, "KOBO_replay::punchin() at frame %d\n",
				punchframe);

	// Discard any game state snapshots for this frame and on
	KOBO_replay_gst *p = NULL;
	gst_last = gst_first;
	while(gst_last)
	{
		if(gst_last->frame >= punchframe)
		{
			KOBO_replay_gst *d = gst_last;
			gst_last = gst_current = p;
			if(p)
				p->next = NULL;
			else
				gst_first = NULL;
			while(d)
			{
				p = d;
				d = d->next;
				delete p;
			}
			break;
		}
		p = gst_last;
		gst_last = gst_last->next;
	}

	// Truncate replay data, and make sure we have a properly sized buffer
	bufrecord = punchframe;
	if(bufsize < KOBO_REPLAY_BUFFER)
	{
		Uint8 *nb = (Uint8 *)realloc(buffer, KOBO_REPLAY_BUFFER);
		if(!nb)
		{
			log_printf(ELOG, "OOM in KOBO_replay::punchin()!\n");
			return;
		}
		buffer = nb;
		bufsize = KOBO_REPLAY_BUFFER;
	}
}


void KOBO_replay::log_dump(int level, KOBO_replay_logdump rld)
{
	bool header = false;
	bool replay = false;
	bool gstd = false;
	switch(rld)
	{
	  case KOBO_RLD_ALL:
		header = replay = gstd = true;
		log_printf(level, " .- Replay -----------------------\n");
		break;
	  case KOBO_RLD_HEADER:
		header = true;
		log_printf(level, " .- Replay Header ----------------\n");
		break;
	  case KOBO_RLD_REPLAY:
		replay = true;
		log_printf(level, " .- Replay Data ------------------\n");
		break;
	  case KOBO_RLD_GAMESTATE:
		gstd = true;
		log_printf(level, " .- Game State Data --------------\n");
		break;
	}

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

	if(header && (replay || gstd))
		log_printf(level, " |- Data -------------------------\n");

	if(replay)
		log_printf(level, " |    recorded: %d frames\n", bufrecord);

	if(gstd)
	{
		int n = 0;
		for(KOBO_replay_gst *gst = gst_first; gst; gst = gst->next)
			++n;
		log_printf(level, " |  Game State: %d snapshots\n", n);
	}

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


bool KOBO_replay::record_state()
{
	KOBO_replay_gst *gst = new KOBO_replay_gst;
	if(gst_last)
	{
		gst_last->next = gst;
		gst_last = gst;
	}
	else
		gst_last = gst_first = gst_current = gst;
	return gst->record();
}


bool KOBO_replay::verify_state()
{
	while(gst_current && (manage.game_time() > gst_current->frame))
		gst_current = gst_current->next;
	if(!gst_current)
		return true;
	if(manage.game_time() != gst_current->frame)
{
log_printf(ULOG, "(((frame %d missing!)))\n", manage.game_time());
		return true;
}
	return gst_current->verify();
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
		pf->chunk_end();
		if(prefs->debug)
			log_dump(ULOG);
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
		log_dump(ULOG, KOBO_RLD_HEADER);

	return !pf->status();
}


bool KOBO_replay::load_repd(pfile_t *pf)
{
	pf->read(bufrecord);
	if(!bufrecord)
	{
		log_printf(WLOG, "REPD chunk with no data!\n");
		pf->chunk_end();
		return true;
	}
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
		log_dump(ULOG, KOBO_RLD_REPLAY);

	return !pf->status();
}


bool KOBO_replay::load_gstd(pfile_t *pf)
{
	KOBO_replay_gst *gst = new KOBO_replay_gst;
	if(gst_last)
	{
		gst_last->next = gst;
		gst_last = gst;
	}
	else
		gst_last = gst_first = gst_current = gst;
	return gst->load(pf);
}


bool KOBO_replay::load(pfile_t *pf)
{
	switch(pf->chunk_type())
	{
	  case KOBO_PF_REPH_4CC:
		return load_reph(pf);
	  case KOBO_PF_REPD_4CC:
		return load_repd(pf);
	  case KOBO_PF_GSTD_4CC:
		if(prefs->replaydebug)
			return load_gstd(pf);
		else
		{
			// Feature disabled - skip these chunks!
			pf->chunk_end();
			return true;
		}
	  default:
		return false;
	}
}


bool KOBO_replay::save(pfile_t *pf)
{
	endtime = time(NULL);

	if(prefs->debug)
		log_dump(ULOG);

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

	//
	// GSTD (debug/verification data)
	//
	if(prefs->replaydebug)
		for(KOBO_replay_gst *gst = gst_first; gst; gst = gst->next)
			gst->save(pf);

	return !pf->status();
}
