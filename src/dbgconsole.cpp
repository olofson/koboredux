/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2017 David Olofson (Kobo Redux)
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

#include "config.h"
#include "dbgconsole.h"
#include <stdio.h>
#ifdef WIN32
# include <windows.h>
# include <fcntl.h>
#endif

bool debug_console_open = false;


void open_debug_console()
{
	if(debug_console_open)
		return;

#ifdef WIN32
	// Open or attach to console
	AllocConsole();
	SetConsoleTitle("Kobo Redux " KOBO_VERSION_STRING);

	// Redirect stdout
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	int fd = _open_osfhandle((intptr_t)h, _O_TEXT);
	FILE *fp = _fdopen(fd, "w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);

	// Set up a proper scrollback buffer
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if(GetConsoleScreenBufferInfo(h, &csbi))
	{
		COORD bs;
		bs.X = csbi.dwSize.X;
		bs.Y = 10000;
		SetConsoleScreenBufferSize(h, bs);
	}

	// Redirect stderr
	h = GetStdHandle(STD_ERROR_HANDLE);
	fd = _open_osfhandle((intptr_t)h, _O_TEXT);
	fp = _fdopen(fd, "w");
	*stderr = *fp;
	setvbuf(stderr, NULL, _IONBF, 0);
#else
	printf("open_debug_console() not implemented on this platform.\n");
#endif

	debug_console_open = true;
}
