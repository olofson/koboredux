/*(GPLv2)
------------------------------------------------------------
   Kobo Redux - Campaign
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
#include "campaign.h"
#include "replay_gst.h"
#include <algorithm>


KOBO_campaign_info::KOBO_campaign_info()
{
	starttime = endtime = 0;
	dat_path = strdup("<no path>");
	stage = type = skill = 0;
	health = charge = score = 0;
}


KOBO_campaign_info::~KOBO_campaign_info()
{
	free(dat_path);
}


KOBO_campaign::KOBO_campaign(const char *dat, const char *bak)
{
	clear(false);
	dat_path = strdup(dat);
	bak_path = bak ? strdup(bak) : NULL;
}


KOBO_campaign::~KOBO_campaign()
{
	clear(true);
	free(dat_path);
	free(bak_path);
}


void KOBO_campaign::clear(bool to_load)
{
	for(unsigned i = 0; i < replays.size(); ++i)
		if(replays[i])
			delete replays[i];
	replays.clear();
	if(to_load)
	{
		version = gameversion = 0;
		starttime = 0;
	}
	else
	{
		version = KOBO_REPLAY_VERSION;
		gameversion = KOBO_VERSION;
		starttime = time(NULL);
	}
	_modified = false;
	_empty = true;
}


bool KOBO_campaign::modified()
{
	if(_modified)
		return true;

	for(unsigned i = 0; i < replays.size(); ++i)
		if(replays[i] && replays[i]->modified())
			return true;

	return false;
}


bool KOBO_campaign::empty()
{
	return _empty && (replays.size() == 0);
}


bool KOBO_campaign::exists()
{
	return fmap->get(dat_path);
}


void KOBO_campaign::reset()
{
	clear(false);
}


bool KOBO_campaign::load_header(pfile_t *pf)
{
	_modified = true;
	_empty = false;
	pf->read(version);
	pf->read(gameversion);

	if(version == KOBO_REPLAY_VERSION)
		compat = KOBO_RPCOM_FULL;
	else if(version > KOBO_REPLAY_VERSION)
	{
		// This is from a newer version, so we can't safely load this!
		compat = KOBO_RPCOM_NONE;
		pf->chunk_end();
		return false;
	}
	else
		compat = KOBO_RPCOM_LIMITED;

	struct tm t;
	pf->read(t);
	starttime = timegm(&t);

	pf->chunk_end();
	return true;
}


bool KOBO_campaign::load(bool quiet)
{
	const char *syspath = NULL;
	FILE *f = fmap->fopen(dat_path, "rb", &syspath);
	if(!f)
	{
		if(prefs->debug && !quiet)
			log_printf(ELOG, "Could not open campaign file \"%s\" "
					"(\"%s\") for reading!\n",
					dat_path,
					syspath ? syspath : "<unresolved>");
		return false;
	}

	clear(true);

	if(!quiet)
		log_printf(ULOG, "Loading campaign \"%s\"...\n", dat_path);

	pfile_t *pf = new pfile_t(f);
	KOBO_replay *r = NULL;
	int gstd_count = 0;

	while(pf->chunk_read() >= 0)
	{
		int ct = pf->chunk_type();

		if(ct == KOBO_PF_GSTD_4CC)
			++gstd_count;
		else if(gstd_count)
		{
			if(prefs->debug && !quiet)
				log_printf(ULOG, "      x %d\n", gstd_count);
			gstd_count = 0;
		}

		if(prefs->debug && !quiet &&
				((ct != KOBO_PF_GSTD_4CC) ||
				(gstd_count == 1)))
			log_printf(ULOG, "  [%sv%d, %d bytes]\n",
					pf->fourcc2string(ct),
					pf->chunk_version(),
					pf->chunk_size());

		switch(ct)
		{
		  case KOBO_PF_CPGN_4CC:
			load_header(pf);
			break;
		  case KOBO_PF_REPH_4CC:
			r = new KOBO_replay;
			if(!r->load(pf))
			{
				if(!quiet)
					log_printf(ELOG,
							"KOBO_campaign::load()"
							" failed to load %s "
							"chunk! Skipping.\n",
							pf->fourcc2string(ct));
				delete r;
				r = NULL;
				break;
			}
			add_replay(r);
			_modified = true;
			_empty = false;
			break;
		  case KOBO_PF_REPD_4CC:
		  case KOBO_PF_GSTD_4CC:
			if(!r)
			{
				if(!quiet)
					log_printf(ELOG,
							"KOBO_campaign::load()"
							"skipping orphan %s "
							"chunk.\n",
							pf->fourcc2string(ct));
				break;
			}
			if(!r->load(pf) && !quiet)
				log_printf(ELOG, "KOBO_campaign::load() failed"
						" to load %s chunk! "
						"Ignoring.\n",
						pf->fourcc2string(ct));
			break;
		  default:
			if(!quiet)
				log_printf(WLOG, "KOBO_campaign::load() "
						"skipping unknown chunk %s.\n",
						pf->fourcc2string(ct));
			pf->chunk_end();
			break;
		}
	}
	if(gstd_count && prefs->debug && !quiet)
		log_printf(ULOG, "      x %d\n", gstd_count);

	delete pf;
	fclose(f);
	if(!quiet)
		log_printf(ULOG, "Campaign loaded.\n");

	return _modified;
}


KOBO_campaign_info *KOBO_campaign::analyze()
{
	if(_empty)
		if(!load(true))
			return NULL;

	KOBO_campaign_info *ci = new KOBO_campaign_info;
	ci->starttime = starttime;
	if(dat_path)
		ci->dat_path = strdup(dat_path);
	KOBO_replay *rp = get_replay(-1);
	if(rp)
	{
		ci->stage = rp->stage;
		ci->type = rp->type;
		ci->skill = rp->skill;

		if((rp->compatibility() == KOBO_RPCOM_FULL) && rp->end_health)
		{
			// Player still alive at the end of the last replay, so
			// we grab the end_* stats!
			ci->endtime = rp->endtime;
			ci->health = rp->end_health;
			ci->charge = rp->end_charge;
			ci->score = rp->end_score;
		}
		else
		{
			// Incompatible replay, or player dead, so let's forget
			// about the end_* fields...
			ci->endtime = rp->starttime;
			ci->health = rp->health;
			ci->charge = rp->charge;
			ci->score = rp->score;
		}
	}
	return ci;
}


bool KOBO_campaign::backup()
{
	const char *syspath = NULL;
	FILE *fi = fmap->fopen(dat_path, "rb", &syspath);
	if(!fi)
	{
		if(prefs->debug)
			log_printf(ELOG, "Could not open campaign \"%s\" "
					"(\"%s\") for backup!\n", dat_path,
					syspath ? syspath : "<unresolved>");
		return false;
	}

	fseek(fi, 0, SEEK_END);
	int fsize = ftell(fi);
	fseek(fi, 0, SEEK_SET);
	if(!fsize)
	{
		log_printf(ELOG, "Campaign file \"%s\" is empty! No backup.\n",
				dat_path);
		fclose(fi);
		return false;
	}

	char *buf = (char *)malloc(fsize);
	if(!buf)
	{
		log_printf(ELOG, "Out of memory while trying to backup "
				"campaign \"%s\" (\"%s\")!\n",
				bak_path, syspath ? syspath : "<unresolved>");
		fclose(fi);
		return false;
	}

	if(fread(buf, fsize, 1, fi) != 1)
	{
		log_printf(ELOG, "Could not read campaign file \"%s\" for "
				"backup!\n", dat_path);
		free(buf);
		fclose(fi);
		return false;
	}
	fclose(fi);

	syspath = NULL;
	FILE *fo = fmap->fopen(bak_path, "wb", &syspath);
	if(!fo)
	{
		free(buf);
		log_printf(ELOG, "Could not open campaign backup \"%s\" "
				"(\"%s\") for writing!\n",
				bak_path, syspath ? syspath : "<unresolved>");
		return false;
	}

	if(fwrite(buf, fsize, 1, fo) != 1)
	{
		log_printf(ELOG, "Could not write campaign backup \"%s\" "
				"(\"%s\")!\n",
				bak_path, syspath ? syspath : "<unresolved>");
		free(buf);
		fclose(fo);
		return false;
	}

	fflush(fo);
	fclose(fo);
	free(buf);
	if(prefs->debug)
		log_printf(ULOG, "Backed up campaign to \"%s\" (\"%s\"); "
				"%d bytes.\n", bak_path,
				syspath ? syspath : "<unresolved>", fsize);
	return true;
}


bool KOBO_campaign::save(bool force)
{
	// Only save if forced, or if there are unsaved changes
	if(!force && (empty() || !modified()))
		return true;

	backup();

	log_printf(ULOG, "Saving campaign \"%s\"...\n", dat_path);

	const char *syspath = NULL;
	FILE *f = fmap->fopen(dat_path, "wb", &syspath);
	if(!f)
	{
		log_printf(ELOG, "Could not open campaign file \"%s\" "
				"(\"%s\") for writing!\n",
				dat_path, syspath ? syspath : "<unresolved>");
		return false;
	}

	pfile_t *pf = new pfile_t(f);

	// Campaign header chunk
	if(prefs->debug)
		log_printf(ULOG, "  0: CPGN header\n");

	pf->chunk_write(KOBO_PF_CPGN_4CC, KOBO_PF_CPGN_VERSION);

	pf->write(version);
	pf->write(gameversion);

	pf->write(gmtime(&starttime));
	pf->chunk_end();

	// Replays
	for(unsigned i = 0; i < replays.size(); ++i)
	{
		if(replays[i])
		{
			if(prefs->debug)
				log_printf(ULOG, "  %d: Stage %d\n", i + 1,
						replays[i]->stage);
			replays[i]->save(pf);
		}
		else if(prefs->debug)
			log_printf(ULOG, "  %d: <no replay>\n", i + 1);
	}

	delete pf;
	fflush(f);
	fclose(f);
	log_printf(ULOG, "Campaign saved.\n");

	_modified = false;
	for(unsigned i = 0; i < replays.size(); ++i)
		if(replays[i])
			replays[i]->modified(false);
	return true;
}


KOBO_replay *KOBO_campaign::get_replay(int stage)
{
	if(!stage || !replays.size())
		return NULL;
	if(stage < 1)
		stage = replays.size() + stage + 1;
	if((stage < 1) || (stage > (int)replays.size()))
		return NULL;
	return replays[stage - 1];
}


void KOBO_campaign::add_replay(KOBO_replay *replay)
{
	std::vector<KOBO_replay *>::iterator it = std::find(
			replays.begin(), replays.end(), replay);
	if(it != replays.end())
	{
		log_printf(WLOG, "KOBO_campaign::add_replay() tried to re-add "
				"replay! Moving it instead.\n");
		replays[it - replays.begin()] = NULL;
	}
	KOBO_replay *old = get_replay(replay->stage);
	if(old)
	{
		log_printf(WLOG, "KOBO_campaign::add_replay() replacing "
				"existing stage %d replay!\n", replay->stage);
		delete old;
		replays[replay->stage - 1] = replay;
	}
	else
	{
		while((int)replays.size() < replay->stage - 1)
			replays.push_back(NULL);
		replays.push_back(replay);
	}
	_modified = true;
}


int KOBO_campaign::last_stage()
{
	return replays.size();
}
