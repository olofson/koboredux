/*(GPLv2)
------------------------------------------------------------
   Kobo Redux - Save Manager
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

#ifndef	_KOBO_SAVEMANAGER_H_
#define	_KOBO_SAVEMANAGER_H_

#include "config.h"
#include "campaign.h"

struct KOBO_save_slot
{
	KOBO_campaign		*campaign;
	KOBO_campaign_info	*cinfo;
};

class KOBO_save_manager
{
	KOBO_save_slot	slots[KOBO_MAX_CAMPAIGN_SLOTS];
	KOBO_save_slot	demos[KOBO_MAX_CAMPAIGN_SLOTS];
  public:
	KOBO_save_manager();
	virtual ~KOBO_save_manager();

	// Load/analyze/save
	bool exists(int slot = -1);	// File exists or there's recorded data
	bool load(int slot = -1);	// (Re)load specified, or all
	KOBO_campaign *campaign(unsigned slot);
	bool save(unsigned slot);
	void resave_all();

	// Demos
	bool load_demos();
	KOBO_campaign *demo(int slot = -1);
	bool demo_exists(int slot = -1);

	// Campaign and demo analysis
	void analyze();
	KOBO_campaign_info *analysis(KOBO_campaign *campaign);
};

#endif // _KOBO_SAVEMANAGER_H_
