/*(LGPLv2.1)
------------------------------------------------------------
	filemap.h - Simple Portable File Path Mapper
------------------------------------------------------------
 * Copyright 2001, 2009 David Olofson
 * Copyright 2015-2017 David Olofson (Kobo Redux)
 *
 * This library is free software;  you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation;  either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library  is  distributed  in  the hope that it will be useful,  but
 * WITHOUT   ANY   WARRANTY;   without   even   the   implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library;  if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/*
 * Path syntax:
 *	All filemapper_t functions but exepath(<exepath>)
 *	and sys2unix() (obviously!) expect paths in the
 *	native system format.
 *
 *	   ALL OTHER PATHS SHOULD BE IN UNIX FORMAT.
 *
 *	All arguments that are expected to be in system
 *	format are named along the lines of "syspath".
 *
 * Classes:
 *	The 'class>>' construct expands to one of the
 *	paths registered for 'class', or makes the mapping
 *	fail (NULL is returned), if the object doesn't
 *	exist or cannot be created as applicable.
 *
 *	Class references are recursive, and are resolved
 *	late, at mapping time. (That is, they are resolved
 *	when someone asks for it, not when registering
 *	paths.)
 *
 *	Class "HOME>>" is built-in, and resolves to the
 *	root of the user home directory. (Currently
 *	looks only at the environment variable "HOME",
 *	which may not be defined on all platforms.)
 *
 *	Class "EXE>>" is another built-in, and refers to
 *	the path extracted from exepath().
 */

#ifndef	_FILEMAP_H_
#define	_FILEMAP_H_

#include "config.h"

#define	FM_DEREF_TOKEN	">>"

#include <stdio.h>
#include <dirent.h>

#define	FM_BUFFERS	16
#define	FM_BUFFER_SIZE	512


struct fm_key_t
{
	fm_key_t	*next;
	char		key[10];
	char		*path;
};


enum
{
	FM_ERROR = 0,
	FM_FILE,
	FM_DIR,
	FM_ANY,
	FM_FILE_CREATE,
	FM_DIR_CREATE
};


// Not a great name - but fm_path_t would just be confusing,
// and an fm_object_t can actually be either a dir or a file.
struct fm_object_t
{
	fm_object_t	*next;
	char		*path;
	int		kind;
	~fm_object_t();
};


class filemapper_t
{
	// For get_(first|next)()
	fm_object_t	*objects;
	fm_object_t	*current_obj;
	DIR		*current_dir;

	// File mapper keys
	fm_key_t	*keys;

	// Application executable path
	char		*app_path;

	// Silly/safe/string alloc :-)
	char		buffers[FM_BUFFERS][FM_BUFFER_SIZE];
	int		next_buffer;
	char *salloc();

	// Various private funcs
	void no_double_slashes(char *buf);
	void fm_format(char *buf);
	void sys_format(char *buf);
	int probe(const char *syspath);
	int test_file_create(const char *syspath);
	int test_dir_create(const char *syspath);
	int test_file_dir_any(const char *syspath, int kind);
	int try_get(const char *path, int kind);
	void add_object(const char *path, int kind);
	int recurse_get(char *result, const char *ref, int kind,
			int level, int build);
  public:
	filemapper_t();
	~filemapper_t();

	// Set/get path to exe file.
	//	Note that exepath(<appname>) expects a
	//	path in the *native system* format,
	//	while exepath() returns a Unix path.
	void exepath(const char *syspath);
	const char *exepath()	{ return app_path; }

	// Add 'path' to class 'key'.
	//	Key should be given *without* the '::'.
	void addpath(const char *key, const char *path, int first = 0);

	// key == NULL, ref == NULL or "*":
	//	Get first key.
	// key == NULL, ref == <some class name>:
	//	Get first key of class 'ref'.
	// key == <some key>, ref == NULL:
	//	Get next key of same class as 'key'.
	// key == <some key>, ref == <some class name>:
	//	Get next key of class 'ref'.
	//	The "*" wildcard for 'ref' is allowed.
	// Returns NULL if no key was found.
	fm_key_t *getkey(fm_key_t *key = NULL, const char *ref = NULL);

	// Get object path (returns path in system format!)
	const char *get(const char *ref, int kind = FM_FILE,
			const char *defprefix = NULL);

	// Initialize path scan, by creating a list of all paths matching 'ref'
	// and 'kind'. Use list_next() to get the paths.
	void list_begin(const char *ref, int kind = FM_FILE);

	// Get first/next path, based on the results of list_begin(). Depending
	// on 'filter', this will return only files, only directory, or both.
	// If 'kind' is passed, it's set to the kind of the return object.
	// Returns path in system format, or NULL, in which case 'kind' (if
	// used) is undefined.
	const char *list_next(int filter = FM_ANY, int *kind = NULL);

	// Open/create file/dir.
	FILE *fopen(const char *ref, const char *mode,
			const char **pth = NULL);
	int mkdir(const char *ref);

	// Print out all registered paths of class 'ref'
	// to stream 'f'. (ref == '*') ==> All classes.
	void print(FILE *f, const char *ref);

	// Determine if a path (fm Unix-like!) is relative.
	bool is_relative(const char *path);

	// Get the name of the file/directory only, without the full path.
	const char *get_name(const char *path);

	// Translate to and from the internal Unix-like path format
	char *sys2fm(const char *syspath);
	char *fm2sys(const char *path);
};

#endif
