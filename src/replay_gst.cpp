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

#include "kobolog.h"
#include "replay_gst.h"
#include "myship.h"
#include "manage.h"
#include "enemies.h"

KOBO_replay_gst::KOBO_replay_gst()
{
	next = NULL;
}


KOBO_replay_gst::~KOBO_replay_gst()
{
}


bool KOBO_replay_gst::record()
{
	frame = manage.game_time();
	score = manage.current_score();
	player.x = myship.get_csx();
	player.y = myship.get_csy();
	player.health = myship.health();
	player.charge = myship.charge();
	for(int i = 0; i < KOBO_EK__COUNT; ++i)
	{
		enemystats[i].spawned = enemies.stats[i].spawned;
		enemystats[i].killed = enemies.stats[i].killed;
		enemystats[i].health = enemies.stats[i].health;
		enemystats[i].damage = enemies.stats[i].damage;
	}
	return true;
}


void KOBO_replay_gst::verify(const char *desc, const char *desc2,
		uint32_t snap, uint32_t current)
{
	if(snap == current)
		return;
	status = false;
	log_printf(ELOG, "REPLAY DIFF in frame %d: %s%s is %u; "
			"should be %u!\n",
			frame, desc, desc2 ? desc2 : "", current, snap);
}


void KOBO_replay_gst::verify(const char *desc, const char *desc2,
		int32_t snap, int32_t current)
{
	if(snap == current)
		return;
	status = false;
	log_printf(ELOG, "REPLAY DIFF in frame %d: %s%s is %d; "
			"should be %d!\n",
			frame, desc, desc2 ? desc2 : "", current, snap);
}


bool KOBO_replay_gst::verify()
{
	if(frame != manage.game_time())
	{
		log_printf(ELOG, "KOBO_replay_gst::verify() called on wrong "
				"frame!\n");
		return false;
	}
	status = true;
	verify("score", "", score, manage.current_score());
	verify("player.x", "", player.x, myship.get_csx());
	verify("player.y", "", player.y, myship.get_csy());
	verify("player.health", "", player.health, myship.health());
	verify("player.charge", "", player.charge, myship.charge());
	for(int i = 0; i < KOBO_EK__COUNT; ++i)
	{
		switch((KOBO_enemy_kinds)i)
		{
		  case KOBO_EK_RINGEXPL:
		  case KOBO_EK_GREENBLTEXPL:
		  case KOBO_EK_REDBLTEXPL:
		  case KOBO_EK_BLUEBLTEXPL:
		  case KOBO_EK_BOLTEXPL:
		  case KOBO_EK_ROCKEXPL:
			// These enemies should not affect gameplay!
			continue;
		  default:
			break;
		}
		const char *ename = enemies.enemy_name((KOBO_enemy_kinds)i);
		verify(ename, ".spawned", enemystats[i].spawned,
				enemies.stats[i].spawned);
		verify(ename, ".killed", enemystats[i].killed,
				enemies.stats[i].killed);
		verify(ename, ".health", enemystats[i].health,
				enemies.stats[i].health);
		verify(ename, ".damage", enemystats[i].damage,
				enemies.stats[i].damage);
	}
	return status;
}


bool KOBO_replay_gst::save(pfile_t *pf)
{
	pf->chunk_write(KOBO_PF_GSTD_4CC, KOBO_PF_GSTD_VERSION);

	pf->write(frame);
	pf->write(score);
	pf->write(player.x);
	pf->write(player.y);
	pf->write(player.health);
	pf->write(player.charge);

	pf->write((uint32_t)KOBO_EK__COUNT);
	for(int i = 0; i < KOBO_EK__COUNT; ++i)
	{
		pf->write(enemystats[i].spawned);
		pf->write(enemystats[i].killed);
		pf->write(enemystats[i].health);
		pf->write(enemystats[i].damage);
	}

	pf->chunk_end();
	return !pf->status();
}


bool KOBO_replay_gst::load(pfile_t *pf)
{
	if(pf->chunk_version() != KOBO_PF_GSTD_VERSION)
		return false;

	pf->read(frame);
	pf->read(score);
	pf->read(player.x);
	pf->read(player.y);
	pf->read(player.health);
	pf->read(player.charge);

	uint32_t ek;
	pf->read(ek);
	if(ek != KOBO_EK__COUNT)
		return false;

	for(int i = 0; i < KOBO_EK__COUNT; ++i)
	{
		pf->read(enemystats[i].spawned);
		pf->read(enemystats[i].killed);
		pf->read(enemystats[i].health);
		pf->read(enemystats[i].damage);
	}

	pf->chunk_end();
	return !pf->status();
}
