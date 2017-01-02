/*(GPLv2)
------------------------------------------------------------
   Kobo Deluxe - An enhanced SDL port of XKobo
------------------------------------------------------------
 * Copyright 2016-2017 David Olofson
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

#include "openurl.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <stdio.h>
#include <stdlib.h>
#endif

int kobo_OpenURL(const char *url)
{
#ifdef _WIN32
	HINSTANCE res;
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
			COINIT_DISABLE_OLE1DDE);
	res = ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
	CoUninitialize();
	return (res > (HINSTANCE)32);
#else
	char buf[1024];
	snprintf(buf, sizeof(buf), "xdg-open %s", url);
	return (system(buf) != -1);
#endif
}
