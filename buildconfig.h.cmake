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

#define	PACKAGE			"@PACKAGE@"
#define	KOBO_VERSION_STRING	"@VERSION_STRING@"
#define	KOBO_MAKE_VERSION(major, minor, micro, build)	\
		(((major) << 24) | ((minor) << 16) | ((micro) << 8) | (build))
#define	KOBO_MAJOR(ver)	(((ver) >> 24) & 0xff)
#define	KOBO_MINOR(ver)	(((ver) >> 16) & 0xff)
#define	KOBO_MICRO(ver)	(((ver) >> 8) & 0xff)
#define	KOBO_BUILD(ver)	((ver) & 0xff)
#define	KOBO_VERSION	KOBO_MAKE_VERSION(@VERSION_MAJOR@, @VERSION_MINOR@, @VERSION_PATCH@, @VERSION_BUILD@)

#cmakedefine	KOBO_DATADIR	"@KOBO_DATADIR@"
#cmakedefine	KOBO_USERDIR	"@KOBO_USERDIR@"
#cmakedefine	KOBO_EXEFILE	"@KOBO_EXEFILE@"
#cmakedefine	KOBO_CONFIGFILE	"@KOBO_CONFIGFILE@"

#cmakedefine	KOBO_SYSCONFDIR "@KOBO_SYSCONFDIR@"
