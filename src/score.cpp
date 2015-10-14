/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 1995, 1996 Akira Higuchi
 * Copyright 2002 Jeremy Sheeley
 * Copyright 2001-2003, 2007, 2009 David Olofson
 * Copyright 2015 David Olofson (Kobo Redux)
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

#define	DBG(x)
#define	DBG2(x)
#define	DBG3(x)

#include "kobo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#if !defined(_WIN32) && !defined(MACOS)
#include <pwd.h>
#endif

#include "kobolog.h"
#include "score.h"

score_manager_t scorefile;


/*----------------------------------------------------------
	s_hiscore_t
----------------------------------------------------------*/

s_hiscore_t::s_hiscore_t()
{
	profile = NULL;
	name[0] = 0;
	clear();
}

void s_hiscore_t::clear()
{
	start_date = 0;
	end_date = 0;
	skill = -1;
	score = 0;
	start_scene = -1;
	end_scene = -1;
	end_lives = -1;
	end_health = -1;
	playtime = 0;
	saves = loads = -1;
	gametype = -1;
}

void s_hiscore_t::read(pfile_t &pf)
{
	pf.read(start_date);
	pf.read(end_date);
	pf.read(skill);
	pf.read(score);
	pf.read(start_scene);
	pf.read(end_scene);
	pf.read(end_lives);
	pf.read(end_health);
	pf.read(playtime);
	pf.read(saves);
	pf.read(loads);
	pf.read(gametype);
}

void s_hiscore_t::write(pfile_t &pf)
{
	pf.write(start_date);
	pf.write(end_date);
	pf.write(skill);
	pf.write(score);
	pf.write(start_scene);
	pf.write(end_scene);
	pf.write(end_lives);
	pf.write(end_health);
	pf.write(playtime);
	pf.write(saves);
	pf.write(loads);
	pf.write(gametype);
}


/*----------------------------------------------------------
	s_profile_t
----------------------------------------------------------*/

s_profile_t::s_profile_t()
{
	for(int i = 0; i < HISCORE_SAVE_MAX; ++i)
		hiscoretab[i].profile = this;
	filename = NULL;
}

s_profile_t::~s_profile_t()
{
	free(filename);
}

void s_profile_t::clear()
{
	best_score = 0;
	last_scene = 0;
	name[0] = 0;
	version = 0;
	skill = SKILL_NORMAL;
	handicap = 0;
	color1 = -1;
	color2 = -1;
	hiscores = 0;
	for(int i = 0; i < HISCORE_SAVE_MAX; ++i)
		hiscoretab[i].clear();
}

int s_profile_t::load(const char *fn)
{
	log_printf(D3LOG, "s_profile_t::load('%s')\n", fn);

	free(filename);
	filename = strdup(fn);

	FILE *f = fopen(filename, "rb");
	if(!f)
	{
		log_printf(ELOG, "Failed to open player profile '%s'!\n", fn);
		return -1;
	}

	pfile_t pf(f);

	clear();

	// Check if this is an old broken 73 byte file written by a
	// previous version for Win32. If it is, skip the first byte.
	fseek(f, 0, SEEK_END);
	if(ftell(f) == 73)
	{
		log_printf(ELOG, "Broken score file '%s' detected and fixed.\n", fn);
		fseek(f, 1, SEEK_SET);
	}
	else
		fseek(f, 0, SEEK_SET);

	pf.read(best_score);
	pf.read(last_scene);
	pf.read(name, sizeof(name));
	log_printf(DLOG, "name: %s\n", name);
	log_printf(DLOG, "  best_score: %u\n", best_score);
	log_printf(DLOG, "  last_scene: %u\n", last_scene);
	pf.buffer_close();

	if(pf.status() < 0)
	{
		log_printf(ELOG, "Error reading player profile '%s'!\n", fn);
		fclose(f);
		return -1;
	}

	//Kobo Deluxe profile file format 1+
	int chunks = 0;
	while(!feof(f))
	{
		if(pf.chunk_read() < 0)
		{
			pf.status();	// Just EOF - no big deal...
			break;
		}
		switch(pf.chunk_type())
		{
		  case MAKE_4CC('P', 'R', 'O', 'F'):
			log_printf(D2LOG, "Chunk 'PROF'; %d bytes.\n",
					pf.chunk_size());
			if(pf.chunk_size() >= 20)
			{
				pf.read(version);
				pf.read(skill);
				pf.read(handicap);
				pf.read(color1);
				pf.read(color2);
			}
			else
			{
				log_printf(WLOG, "WARNING: Truncated 'PROF' chunk"
						" in player profile '%s'!\n", fn);
			}
			break;
		  case MAKE_4CC('H', 'I', 'S', 'C'):
			log_printf(D2LOG, "Chunk 'HISC'; %d bytes.\n",
					pf.chunk_size());
			if(hiscores < HISCORE_SAVE_MAX)
				hiscoretab[hiscores++].read(pf);
			break;
		  default:
			{
				char tp[5];
				int t = pf.chunk_type();
				tp[0] = (t >> 24) & 0xff;
				tp[1] = (t >> 16) & 0xff;
				tp[2] = (t >> 8) & 0xff;
				tp[3] = t & 0xff;
				tp[4] = 0;
				log_printf(D2LOG, "Unknown chunk '%s'; %d bytes.\n",
						tp, pf.chunk_size());
			}
			break;
		}
		++chunks;
		pf.chunk_end();
	}

	if(!chunks)
	{
		log_printf(D2LOG, "Old scorefile.\n");
		//Construct a "fake" highscore entry from the old data.
		hiscores = 1;
		hiscoretab[0].skill = SKILL_UNKNOWN;	//Classic removed!
		hiscoretab[0].gametype = GAME_SINGLE;	//No others available.
		hiscoretab[0].score = best_score;
		hiscoretab[0].end_scene = last_scene;	//Likely, but...
		hiscoretab[0].saves = 0;		//Not implemented.
		hiscoretab[0].loads = 0;		//Not implemented.
	}

	fclose(f);
	return pf.status();
}


int s_profile_t::save()
{
	log_printf(D3LOG, "s_profile_t::save('%s')\n", filename);
#ifndef	_WIN32
	umask(022);
#endif
	if(!filename)
	{
		log_printf(ELOG, "Failed to save player profile - no file name!\n");
		return -1;
	}

//The "safe list"; platforms that do not have symlinks:
#if !defined(_WIN32)
# ifdef KOBO_HAVE_LSTAT
	// We will not write score files via symlinks!
	struct stat statbuf;
	if(lstat(filename, &statbuf) < 0)
	{
		log_printf(ELOG, "Cannot stat player profile '%s'! "
				"Could be a symlink - will NOT write.\n",
				filename);
		return -1;	
	}
	if( (statbuf.st_mode & S_IFLNK) == S_IFLNK )
	{
		log_printf(ELOG, "Player profile '%s' is a symlink! "
				"I will NOT write through symlinks.\n",
				filename);
		return -1;	
	}
# else
#  warning ================= SECURITY HAZARD =================
#  warning If this platform has symlinks or similar, please
#  warning add an appropriate test. If not, add your platform
#  warning to the "safe list". Either way, if you read this,
#  warning post a bug report to the Kobo Redux maintainer.
#  warning ================= SECURITY HAZARD =================
#  error (Remove this line to compile anyway.)
# endif	/* KOBO_HAVE_LSTAT */
#endif	/* "Safe list" */

	FILE *f = fopen(filename, "wb");
	if(!f)
	{
		log_printf(ELOG, "Failed to create player profile '%s'!\n",
				filename);
		return -1;
	}

	fseek(f, 0, SEEK_SET);

	pfile_t pf(f);

	// Write old XKobo stuff
	pf.write(best_score);
	pf.write(last_scene);
	pf.write(name, sizeof(name));
	pf.buffer_write();

	// Write profile header
	if(pf.chunk_write(MAKE_4CC('P', 'R', 'O', 'F')) < 0)
	{
		fclose(f);
		return pf.status();
	}
	pf.write((unsigned int)PROFILE_VERSION);
	pf.write(skill);
	pf.write(handicap);
	pf.write(color1);
	pf.write(color2);
	pf.chunk_end();

	// Write highscores
	for(unsigned int i = 0; i < hiscores; ++i)
	{
		if(pf.chunk_write(MAKE_4CC('H', 'I', 'S', 'C')) < 0)
		{
			fclose(f);
			return pf.status();
		}
		hiscoretab[i].write(pf);
		pf.chunk_end();
	}

	fclose(f);
	return pf.status();
}


s_hiscore_t *s_profile_t::best_hiscore()
{
	if(!hiscores)
		return NULL;

	s_hiscore_t *hs = hiscoretab;
	for(unsigned int i = 1; i < hiscores; ++i)
		if(hiscoretab[i].score > hs->score)
			hs = hiscoretab + i;

	return hs;
}


/*----------------------------------------------------------
	score_manager_t
----------------------------------------------------------*/

score_manager_t::score_manager_t()
{
}


score_manager_t::~score_manager_t()
{
}


void score_manager_t::init()
{
	const char *p;

	// The logic to this is:
	// 1.  Check to see if they have an old score file.  
	// If they do, rename it to follow the new naming format
	// which is X.UID.  On Win32 (which doesn't have a UID), the
	// UID is set to 42.
	// 2.  Load all of the X.UID scores into our array.
	// 3.  Determine what the last used file was.

	// Change as of 0.4-pre7: "score files" are upgraded
	// to "player profile files", which contain IFF style
	// chunks beyond the 74 XKobo compatible bytes.

#ifdef _WIN32
/*
FIXME: This should be properly implemented on Windoze - if we
FIXME: decide that it's actually much point. (Users, security etc
FIXME: are rather foreign matters to the average winuser...)
*/
	int nUID = 42;
#else
	uid_t nUID = getuid();
#endif
	// Now load all of the files.  Start with trying to get the old
	// file name first. That file's information will become games[0],
	// and then the old file name will be deleted.

	int numberSaveGamesFound = 0;
	for(int i = 0; i < MAX_PROFILES; i++)
		profiles[i].clear();

	char buf[1024];
	snprintf(buf, sizeof(buf), "SCORES>>%u", nUID);
	p = fmap->get(buf);
	if(p)
	{
		if(profiles[numberSaveGamesFound].load(p) < 0)
			log_printf(WLOG, "WARNING: Can't read score file!\n");
		else
		{
			numberSaveGamesFound = 1;
			log_printf(ULOG, "Old score file \"%s\" found"
					" and imported.\n", p);
		}
	}

	// Search through all registered score directories
	// for all of the files that end in .UID
	struct stat st_buffer;
	fmap->get_all("SCORES>>", FM_DIR);
	char UIDbuf[12];
	snprintf(UIDbuf, sizeof(UIDbuf), ".%u", nUID);
	while(1)
	{
		const char *path = fmap->get_next();
		if(!path)
			break;

		// if the last characters in the file name is the string
		// UIDbuf...
		char *tmpptr = strstr((char *)path, UIDbuf);
		if(tmpptr != NULL && (*(tmpptr + strlen(UIDbuf)) == '\0'))
		{
			if(stat(path, &st_buffer))
				continue;
			if(!S_ISREG(st_buffer.st_mode))
				continue;

			if(profiles[numberSaveGamesFound].load(path) < 0)
				continue;

			if((++numberSaveGamesFound) >= MAX_PROFILES)
				break;
		}
	}

	numProfiles = numberSaveGamesFound;

	// If there are no save files, introbase in states.cpp will take
	// them to the enter_name screen.

	gather_high_scores();
	print_high_scores();
}


int score_manager_t::add_player(const char *_name)
{
	log_printf(D3LOG, "creating profile %d\n", numProfiles);
	if(numProfiles == MAX_PROFILES)
		return -1;
	int ret = 0;
#ifdef _WIN32
	int userid = 42;
#else
	uid_t userid = getuid();
#endif
	profiles[numProfiles].clear();
	strncpy(profiles[numProfiles].name, _name, SCORE_NAME_LEN);

// TODO: Somewhere around here would be a nice place to check if the
// TODO: scoredir exists, and if not, try to create one.

	// This means that we always create new profiles in the first
	// scoredir registered. (Loaded score files are written back
	// where they were read from when changed.)
	char buf[1024];
	snprintf(buf, sizeof(buf), "SCORES>>%s.%u", _name, userid);
	const char *fn = fmap->get(buf, FM_FILE_CREATE);
	free(profiles[numProfiles].filename);
	if(fn)
		profiles[numProfiles].filename = strdup(fn);
	else
	{
		profiles[numProfiles].filename = NULL;
		log_printf(ELOG, "ERROR: Couldn't resolve player profile"
				" path '%s'!\n", buf);
		ret = -2;
	}

	log_printf(ULOG, "Created new player profile '%s' for player '%s'\n",
			fn ? fn : "<NO FILE!>", profiles[numProfiles].name);
	currentProfile = numProfiles;

	// Try to create the high score file.
	if(profiles[numProfiles].save() < 0)
		ret = -3;

	numProfiles++;
	return ret;
}


void score_manager_t::select_profile(int prof)
{
	log_printf(D3LOG, "score_manager_t::select_profile(%d)\n", prof);
	currentProfile = prof;
}


// Rules:
//	* Highest score gets highest priority.
//	* If the maximum number of entries is
//	  exceeded, the lowest score entry is
//	  overwritten.
//	* best_score and last_scene of the
//	  profile are bumped if exceeded.
void score_manager_t::record(s_hiscore_t *entry, int force)
{
	log_printf(D3LOG, "score_manager_t::record\n");
	s_profile_t *p = profiles + currentProfile;
	int write;
	if(force)
		write = 1;
	else
		write = 0;

	if(entry->score > p->best_score)
	{
		p->best_score = entry->score;
		write = 1;
	}

	if(entry->end_scene > p->last_scene)
	{
		p->last_scene = entry->end_scene;
		write = 1;
	}

	int si = -1;
	if(p->hiscores < HISCORE_SAVE_MAX)
	{
		// Grab next free slot
		si = p->hiscores++;
	}
	else
	{
		// Find worst score
//FIXME: Pick old or new when scores are equal?
		unsigned int worst_score = entry->score;
		for(int i = 0; i < HISCORE_SAVE_MAX; ++i)
			if(p->hiscoretab[i].score <= worst_score)
			{
				si = i;
				worst_score = p->hiscoretab[i].score;
			}
	}
	if(si >= 0)
	{
		// Copy and "fix" entry...
		log_printf(DLOG, "Saving in slot %d.\n", si);
		p->hiscoretab[si] = *entry;
		p->hiscoretab[si].profile = p;
		memcpy(p->hiscoretab[si].name, p->name, SCORE_NAME_LEN);
		write = 1;
	}
	else
		log_printf(DLOG, "Not saved.\n");

	if(write)
	{
		log_printf(DLOG, "Writing file %s for player %s...\n",
				p->filename, p->name);
		p->save();
		gather_high_scores();
		print_high_scores();
		log_printf(DLOG, "  Done!\n");
	}
}


static int s_table_cmp(const void *_a, const void *_b)
{
	s_hiscore_t *a = (s_hiscore_t *)_a;
	s_hiscore_t *b = (s_hiscore_t *)_b;
	if(a->score < b->score)
		return 1;
	else if(a->score > b->score)
		return -1;
	return 0;
}


void score_manager_t::gather_high_scores(int placeholders)
{
	const int sc[] = {
		10000, 9000, 8000, 7000, 6000,
		5000, 4000, 3000, 2000, 1000,
		0
	};
	const int st[] = {
		10, 9, 8, 7, 6,
		5, 4, 3, 2, 1
	};
	const char *nm[] = {
		"DAVID OLOFSON",
		"AKIRA HIGUCHI",
		"MASANAO IZUMO",
		"MAX HORN",
		"SAMUEL HART",
		"ANDREAS",
		"TRICK",
		"RIKI",
		"GEORGIE",
		"JEREMY"
	};

	for(int i = 0; i < MAX_HIGHSCORES; ++i)
		high_tbl[i].clear();

	s_profile_t p;
	highs = 0;
	fmap->get_all("SCORES>>", FM_DIR);
	while(highs < MAX_HIGHSCORES)
	{
		const char *path = fmap->get_next();
		if(!path)
			break;

		if(p.load(path) < 0)
			continue;

		s_hiscore_t *hs = p.best_hiscore();
		if(!hs)
			continue;

		high_tbl[highs] = *hs;
		memcpy(high_tbl[highs].name, p.name, SCORE_NAME_LEN);
		high_tbl[highs].profile = NULL;
		++highs;
	}

	if(placeholders)
	{
		//Throw some nice looking names in, if there's room. ;-)
		int i = 0;
		while(highs < MAX_HIGHSCORES)
		{
			if(!sc[i])
				break;
			memcpy(high_tbl[highs].name, nm[i], SCORE_NAME_LEN);
			high_tbl[highs].skill = SKILL_NORMAL;
			high_tbl[highs].gametype = GAME_SINGLE;
			high_tbl[highs].score = sc[i];
			high_tbl[highs].start_scene = 0;
			high_tbl[highs].end_scene = st[i];
			++i;
			++highs;
		}
	}

	qsort(high_tbl, highs, sizeof(high_tbl[0]), s_table_cmp);
}


int score_manager_t::highscore(int prof)
{
	if(-1 == prof)
		return profiles[currentProfile].last_scene;
	else if(-2 == prof)
		return high_tbl[0].score;
	else
		return profiles[prof].best_score;
}


void score_manager_t::print_high_scores()
{
	if(!highs)
	{
		log_printf(WLOG, "No hiscore entries found!\n");
		return;
	}

	log_printf(ULOG, "Name                     Score Start End\n");
	for(unsigned int j = 0; j < highs; j++)
	{
		log_printf(ULOG, "%-20s %9u %5d %3d\n", high_tbl[j].name,
				high_tbl[j].score,
				high_tbl[j].start_scene,
				high_tbl[j].end_scene);
	}
}
