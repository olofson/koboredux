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
	return true;
}


void KOBO_replay_gst::verify(const char *desc, uint32_t snap, uint32_t current)
{
	if(snap == current)
		return;
	status = false;
	log_printf(ELOG, "REPLAY DIFF in frame %d: %s is %u; should be %u!\n",
			frame, desc, current, snap);
}


void KOBO_replay_gst::verify(const char *desc, int32_t snap, int32_t current)
{
	if(snap == current)
		return;
	status = false;
	log_printf(ELOG, "REPLAY DIFF in frame %d: %s is %d; should be %d!\n",
			frame, desc, current, snap);
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
	verify("score",		score,		manage.current_score());
	verify("player.x",	player.x,	myship.get_csx());
	verify("player.y",	player.y,	myship.get_csy());
	verify("player.health",	player.health,	myship.health());
	verify("player.charge",	player.charge,	myship.charge());
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
	pf->chunk_end();
	return !pf->status();
}
