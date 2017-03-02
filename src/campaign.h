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

#ifndef	_KOBO_CAMPAIGN_H_
#define	_KOBO_CAMPAIGN_H_

#include "replay.h"
#include <vector>

// Campaign header
#define	KOBO_PF_CPGN_4CC	MAKE_4CC('C', 'P', 'G', 'N')
#define	KOBO_PF_CPGN_VERSION	1

class KOBO_campaign_info
{
  public:
	KOBO_campaign_info();
	~KOBO_campaign_info();

	time_t		starttime;	// Campaign start time
	time_t		endtime;	// Campaign end time
	char		*dat_path;	// Campaign file path (system)

	int		stage;
	int		type;		// game_types_t
	int		skill;		// skill_levels_t

	int		health;		// Player health
	int		charge;		// Player secondary weapon accu charge
	int		score;		// Player score
};

class KOBO_campaign
{
	int32_t		version;	// Game logic version
	int32_t		gameversion;	// Game version that generated this

	time_t		starttime;	// Campaign start time
	std::vector<KOBO_replay *> replays;
	char		*dat_path;
	char		*bak_path;
	bool		_modified;

	KOBO_replay_compat	compat;	// Full campaign compatibility status

	void clear();
	char *construct_path(unsigned slot, const char *ext);
	bool backup();
	bool modified();
	bool load_header(pfile_t *pf);
  public:
	KOBO_campaign(unsigned slot);
	virtual ~KOBO_campaign();

	// Load/analyze/save
	bool load(bool quiet = false);
	KOBO_campaign_info *analyze();
	bool save(bool force = false);

	// Compatibility check
	KOBO_replay_compat compatibility()	{ return compat; }

	KOBO_replay *get_replay(int stage);
	void add_replay(KOBO_replay *replay);
	int last_stage();
};

#endif // _KOBO_CAMPAIGN_H_
