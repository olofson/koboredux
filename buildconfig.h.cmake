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

#cmakedefine	KOBO_DATADIR	"@KOBO_DATADIR@"
#cmakedefine	KOBO_USERDIR	"@KOBO_USERDIR@"
#cmakedefine	KOBO_EXEFILE	"@KOBO_EXEFILE@"
#cmakedefine	KOBO_CONFIGFILE	"@KOBO_CONFIGFILE@"

#cmakedefine	KOBO_SYSCONFDIR "@KOBO_SYSCONFDIR@"
