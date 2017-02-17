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
	version = KOBO_REPLAY_VERSION;
	gameversion = KOBO_VERSION;
	config = get_config();
	starttime = time(NULL);

	bufsize = KOBO_REPLAY_BUFFER;
	bufrecord = bufplay = 0;
	buffer = (Uint8 *)malloc(bufsize);
	if(!buffer)
		log_printf(ELOG, "Out of memory! Limited save/replay "
				"capabilities.\n");

	compat = KOBO_RPCOM_FULL;
}


KOBO_replay::~KOBO_replay()
{
	free(buffer);
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


void KOBO_replay::log_dump(int level)
{
	log_printf(level, " .- Replay -----------------------\n");
	log_printf(level, " |     version: %d.%d.%d.%d\n",
			KOBO_MAJOR(version), KOBO_MINOR(version),
			KOBO_MICRO(version), KOBO_BUILD(version));
	log_printf(level, " | gameversion: %d.%d.%d.%d\n",
			KOBO_MAJOR(gameversion), KOBO_MINOR(gameversion),
			KOBO_MICRO(gameversion), KOBO_BUILD(gameversion));
	log_printf(level, " |      config: %8.8x\n", config);
	log_printf(level, " |   starttime: %s", ctime(&starttime));
	log_printf(level, " |    recorded: %d frames\n", bufrecord);
	log_printf(level, " |    position: %d frames\n", bufplay);
	log_printf(level, " |       stage: %d\n", stage);
	log_printf(level, " |        type: %d\n", type);
	log_printf(level, " |       skill: %d\n", skill);
	log_printf(level, " |        seed: %u\n", seed);
	log_printf(level, " |      health: %d\n", health);
	log_printf(level, " |      charge: %d\n", charge);
	log_printf(level, " |       score: %d\n", score);
	log_printf(level, " '--------------------------------\n");
}


float KOBO_replay::progress()
{
	if(!bufrecord)
		return 0.0f;
	return (float)bufplay / bufrecord;
}
