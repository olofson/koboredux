/*
---------------------------------------------------------------------------
@KOBOREDUX_AUTO_MESSAGE@
---------------------------------------------------------------------------
*/

#cmakedefine	KOBO_HAVE_SNPRINTF
#cmakedefine	KOBO_HAVE__SNPRINTF
#cmakedefine	KOBO_HAVE_VSNPRINTF
#cmakedefine	KOBO_HAVE__VSNPRINTF
#cmakedefine	KOBO_HAVE_STAT
#cmakedefine	KOBO_HAVE_LSTAT

#cmakedefine	KOBO_HAVE_GETEGID
#cmakedefine	KOBO_HAVE_SETGID

#cmakedefine	KOBO_ENABLE_TOUCHSCREEN

#define	RETSIGTYPE	@RETSIGTYPE@

#define	PACKAGE		"@PACKAGE@"
#define	KOBO_VERSION	"@VERSION_STRING@"

#define	KOBO_DATADIR	"@KOBO__DATADIR@"
#define	KOBO_SCOREDIR	"@KOBO__SCOREDIR@"
#define	KOBO_CONFIGDIR	"@KOBO__CONFIGDIR@"
#define	KOBO_CONFIGFILE	"@KOBO__CONFIGFILE@"
#define	KOBO_EXEFILE	"@KOBO__EXEFILE@"

#ifndef WIN32
#define	KOBO_SYSCONFDIR "@KOBO__SYSCONFDIR@"
#endif
