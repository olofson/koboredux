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

#include "savemanager.h"
#include "random.h"


char *KOBO_campaign::construct_path(unsigned slot, const char *ext)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "SAVES>>campaign_slot_%d.%s", slot, ext);
	return strdup(buf);
}


KOBO_save_manager::KOBO_save_manager()
{
	for(int i = 0; i < KOBO_MAX_CAMPAIGN_SLOTS; ++i)
	{
		char dat[64];
		char bak[64];

		// Save slot
		snprintf(dat, sizeof(dat), "SAVES>>campaign_slot_%d.dat", i);
		snprintf(bak, sizeof(bak), "SAVES>>campaign_slot_%d.bak", i);
		slots[i].campaign = new KOBO_campaign(dat, bak);
		slots[i].cinfo = NULL;

		// Demo slot (read-only)
		snprintf(dat, sizeof(dat), "DEMOS>>demo_%d.dat", i);
		demos[i].campaign = new KOBO_campaign(dat, NULL);
		demos[i].cinfo = NULL;
	}
}


KOBO_save_manager::~KOBO_save_manager()
{
	for(int i = 0; i < KOBO_MAX_CAMPAIGN_SLOTS; ++i)
	{
		if(slots[i].campaign)
			delete slots[i].campaign;
		if(slots[i].cinfo)
			delete slots[i].cinfo;
		if(demos[i].campaign)
			delete demos[i].campaign;
		if(demos[i].cinfo)
			delete demos[i].cinfo;
	}
}


bool KOBO_save_manager::exists(int slot)
{
	if(slot < 0)
	{
		for(int i = 0; i < KOBO_MAX_CAMPAIGN_SLOTS; ++i)
			if(exists(i))
				return true;
		return false;
	}
	if(slot >= KOBO_MAX_CAMPAIGN_SLOTS)
		return false;

	return slots[slot].campaign->exists() ||
			!slots[slot].campaign->empty();
}


bool KOBO_save_manager::load(int slot)
{
	if(slot < 0)
	{
		bool loaded = false;
		for(int i = 0; i < KOBO_MAX_CAMPAIGN_SLOTS; ++i)
			if(load(i))
				loaded = true;
		return loaded;
	}
	if(slot >= KOBO_MAX_CAMPAIGN_SLOTS)
		return false;

	if(slots[slot].cinfo)
	{
		delete slots[slot].cinfo;
		slots[slot].cinfo = NULL;
	}
	return slots[slot].campaign->load();
}


KOBO_campaign_info *KOBO_save_manager::analysis(int slot, bool force)
{
	if(slot < 0)
	{
		for(int i = 0; i < KOBO_MAX_CAMPAIGN_SLOTS; ++i)
			analysis(i, force);
		return NULL;
	}

	if(slot >= KOBO_MAX_CAMPAIGN_SLOTS)
		return NULL;

	if(force && slots[slot].cinfo)
	{
		delete slots[slot].cinfo;
		slots[slot].cinfo = NULL;
	}
	if(!slots[slot].cinfo)
		slots[slot].cinfo = slots[slot].campaign->analyze();
	return slots[slot].cinfo;
}


bool KOBO_save_manager::load_demos()
{
	bool loaded = false;
	for(int i = 0; i < KOBO_MAX_CAMPAIGN_SLOTS; ++i)
	{
		if(demos[i].cinfo)
		{
			delete demos[i].cinfo;
			demos[i].cinfo = NULL;
		}
		if(demos[i].campaign->load())
		{
			loaded = true;
			demos[i].cinfo = demos[i].campaign->analyze();
		}
	}
	return loaded;
}


KOBO_campaign *KOBO_save_manager::campaign(unsigned slot)
{
	if(slot >= KOBO_MAX_CAMPAIGN_SLOTS)
		return NULL;

	return slots[slot].campaign;
}


KOBO_campaign *KOBO_save_manager::demo(int slot)
{
	if(slot < 0)
	{
		slot = pubrand.get() % KOBO_MAX_CAMPAIGN_SLOTS;
		for(int i = 0; i < KOBO_MAX_CAMPAIGN_SLOTS; ++i)
		{
			KOBO_campaign *k = demo((slot + i) %
					KOBO_MAX_CAMPAIGN_SLOTS);
			if(k && k->last_stage())
				return k;
		}
		return NULL;
	}
	if(slot >= KOBO_MAX_CAMPAIGN_SLOTS)
		return NULL;

	return demos[slot].campaign;
}


bool KOBO_save_manager::save(unsigned slot)
{
	if(slot >= KOBO_MAX_CAMPAIGN_SLOTS)
		return false;

	return slots[slot].campaign->save();
}


void KOBO_save_manager::resave_all()
{
	load(-1);
	for(int i = 0; i < KOBO_MAX_CAMPAIGN_SLOTS; ++i)
		if(!slots[i].campaign->empty())
			slots[i].campaign->save(true);
}
