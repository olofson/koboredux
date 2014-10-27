/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright (C) 1995, 1996 Akira Higuchi
 * Copyright (C) 2002 Jeremy Sheeley
 * Copyright (C) 2001-2002, 2007, 2009 David Olofson
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

#ifndef	_KOBO_SCORE_H_
#define	_KOBO_SCORE_H_

#include "config.h"
#include "game.h"
#include "pfile.h"

#define MAX_PROFILES		100
#define MAX_HIGHSCORES		100
#define HISCORE_SAVE_MAX	100

#define	PROFILE_VERSION		1

//DO NOT CHANGE! (Will break the file format.)
#define	SCORE_NAME_LEN		64


/*----------------------------------------------------------
	s_*_t - Profile and score data wrappers
----------------------------------------------------------*/
// File format:
//	* The first 74 bytes are identical to those in old
//	  XKobo score files, for compatibility reasons.
//
//	* New sections are added in the form of RIFF style
//	  chunks, starting at offset 74 in the file. (See
//	  pfile.(h|cpp) for details.)
//
//	* Chunks of unknown types are IGNORED, and are
//	  therefore LOST if the file is overwritten by an
//	  older version of Kobo Deluxe.
//
//	* Extra data beyond the end of a chunk (ie data that
//	  is added by a later version of Kobo Deluxe) is
//	  IGNORED, and is therefore LOST if the file is
//	  overwritten by an older version of Kobo Deluxe.
//
//	* New versions may ADD data to chunks, but never
//	  remove data from chunks.
//
//	* New versions may add new chunk types.
//
//	* New versions may remove old chunk types, although
//	  this is STRONGLY DISCOURAGED, as it may render
//	  old Kobo Deluxe versions seeing a vertually empty
//	  profile file...
//
//	* Strings are stored as fixed length, null padded
//	  arrays of 64 bytes. All strings should have at
//	  least one terminating null byte.
//
//	* Signed and unsigned int are stored as little
//	  endian 32 bit integers.
//
// Version 1 chunks:
//
//	PROF:	//Player profile header
//		Sint32	version;
//		Sint32	skill;
//		Sint32	handicap;
//		Sint32	color1;
//		Sint32	color2;
//
//	HISC:	//Hiscore entry
//		Uint32	start_date;
//		Uint32	end_date;
//		Sint32	skill;
//		Uint32	score;
//		Sint32	start_scene;
//		Sint32	end_scene;
//		Sint32	end_lives;
//		Sint32	end_health;
//		Uint32	playtime;
//		Sint32	saves;
//		Sint32	loads;
//		Sint32	gametype;
//

struct s_profile_t;

// Stored as one "HISC" chunk for each entry.
// No "real" maximum number of entries per profile,
// but the current implementation uses an array of
// HISCORE_SAVE_MAX entries.
// FIXME: Should be dynamic, although there should
// FIXME: probably be some sensible limit regardless.
struct s_hiscore_t
{
	// For all of the below, -1 (signed) or 0 (unsigned) means "unknown".
	unsigned int	start_date;	//Seconds since 2000...?
	unsigned int	end_date;	//Seconds since 2000...?
	int		skill;		//Skill setting used
	int		gametype;	//Single/cooperative/deathmatch/...
	unsigned int	score;		//Score achieved
	int		start_scene;	//Scene player started on
	int		end_scene;	//Scene player quit or died on
	int		end_lives;	//# of lives when player quit
	int		end_health;	//Health when player quit
	unsigned int	playtime;	//Effective play time, logic frames
	int		saves;		//# of times game was saved
	int		loads;		//# of times game was aborted + loaded

	// The fields below this line are not saved to file.
	s_profile_t	*profile;	//Profile this entry belongs to,
					//if any. (May be NULL!)
	char		name[SCORE_NAME_LEN];	//(Kludge for highscore table.)

	s_hiscore_t();
	void clear();		//Reset and clear all fields.
	void read(pfile_t &f);	//Read this struct from pfile 'f'.
	void write(pfile_t &f);	//Write this struct to pfile 'f'.
};

// Stored as one "PROF" chunk.
struct s_profile_t
{
	//Old stuff (from XKobo)
	unsigned int	best_score;		//Best score
	int		last_scene;		//Last scene completed
	char		name[SCORE_NAME_LEN];	//Player name

	//New stuff (for Kobo Deluxe 0.4)
	unsigned int	version;	//Profile file format version
	int		skill;		//Player skill setting
	int		handicap;	//Player handicap. 0 == none.
	int		color1;		//Primary color. (-1 == default)
	int		color2;		//Secondary color. (-1 == default)

	s_hiscore_t	hiscoretab[HISCORE_SAVE_MAX];

	// The fields below this line are not saved to file.
	unsigned int	hiscores;	//# of hiscores stored
	char		*filename;

	s_profile_t();
	~s_profile_t();
	void clear();			//Reset and clear all fields.
	int load(const char *fn);	//Load from file 'fn'.
	int save();			//Save back to file.

	s_hiscore_t *best_hiscore();
};


/*----------------------------------------------------------
	score_manager_t
----------------------------------------------------------*/

class score_manager_t
{
	s_profile_t profiles[MAX_PROFILES];
	unsigned int currentProfile;
  public:
	unsigned int highs;
	s_hiscore_t high_tbl[MAX_HIGHSCORES];

	int numProfiles;
	score_manager_t();
	~score_manager_t();
	int addPlayer(const char *name);
	void init();
	s_profile_t *profile(int prof = -1)
	{
		if(prof < 0)
			return profiles + currentProfile;
		else
			return profiles + prof;
	}
	// NOTE: hiscore() with no argument (or -2) returns
	// the best known score of all found profiles; NOT
	// that of the current profile.
	int highscore(int prof = -2);
	int last_scene(int prof = -1)
	{
		if(prof < 0)
			return profiles[currentProfile].last_scene;
		else
			return profiles[prof].last_scene;
	}
	char *name(int prof = -1)
	{
		if(prof < 0)
			return profiles[currentProfile].name;
		else
			return profiles[prof].name;
	}
	void select_profile(int prof);
	int current_profile()	{ return currentProfile; }
	void record(s_hiscore_t *entry, int force = 0);
	void gather_high_scores(int placeholders = 0);
	void print_high_scores();
};

extern score_manager_t scorefile;

#endif	//_KOBO_SCORE_H_
