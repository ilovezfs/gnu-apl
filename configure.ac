#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([GNU APL], [1.4], [bug-apl@gnu.org])
AC_PREFIX_DEFAULT( /usr/local )
AC_USE_SYSTEM_EXTENSIONS

# always enable dependency tracking regardless of the user's choice.
# This avoids stale .o files after SVN updates. enable_dependency_tracking
# needs to be set early in order to be recognised!
#
# enable_dependency_tracking=no

AM_INIT_AUTOMAKE([subdir-objects])
AM_MAINTAINER_MODE([disable])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

# use g++ for tests
#
AC_LANG(C++)

LT_INIT([dlopen])

# Checks for programs.
#
AC_PROG_CXX
AC_PROG_CC

AC_GNU_SOURCE

# check if compiler knows -rdynamic. Some compilers accept it but issue a
# warning if it is used. We therefore check explicitly if the compiler is g++
# rather than testing if the compiler accepts -rdynamic
#
AC_MSG_CHECKING([whether compiler is g++ (and then honors -rdynamic)])
if test "x$CXX" = "xg++"; then
   CAN_rdynamic=yes
   AC_SUBST([CXX_RDYNAMIC], ["-rdynamic"])
else
   CAN_rdynamic=no
   AC_SUBST([CXX_RDYNAMIC], [""])
fi
AC_MSG_RESULT([$CAN_rdynamic])

##############################################################################
# Checks for header files.
#

# check for ncurses.h first and for curses.h if ncurses.h was not found

AC_CHECK_HEADER(
	[ncurses.h],
	[AC_DEFINE_UNQUOTED(	[HAVE_NCURSES_H], [1],
				[Define to 1 if <ncurses.h> is present])],
	[AC_CHECK_HEADER(
	[curses.h],
	[AC_DEFINE_UNQUOTED(	[HAVE_CURSES_H], [1],
				[Define to 1 if <curses.h> is present])])
	])

AC_CHECK_HEADER(
	[term.h],
	[AC_DEFINE_UNQUOTED(	[HAVE_TERM_H], [1],
				[Define to 1 if <term.h> is present])],
	[],
	[	[ # if HAVE_NCURSES_H   ]
		[ # include <ncurses.h> ]
		[ # elif HAVE_CURSES_H  ]
		[ # include <curses.h>  ]
		[ # endif ]
	])

AC_CHECK_HEADERS([	\
	execinfo.h	\
	fcntl.h		\
	inttypes.h	\
	stdint.h	\
	stdlib.h	\
        string.h	\
	strings.h	\
	unistd.h	\
	sys/un.h	\
	ext/atomicity.h	\
	utmpx.h])

# Checks for typedefs, structures, and compiler characteristics.
#
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MMAP
AC_CHECK_FUNCS([floor memset munmap pow sqrt strchr strerror])

# Checks for libraries, less relevant first (to be last on linker command)
#
AC_CHECK_LIB([socket],    [socket])   # for Solaris
AC_CHECK_LIB([nsl],       [yp_all])   # for Solaris

# check for libncurses first and for libcurses if libncurses was not found
#
AC_CHECK_LIB(
	[ncurses], [setupterm],
	[	AC_DEFINE_UNQUOTED([HAVE_LIBCURSES], [1],
				   [Define to 1 if lib(n)curses is available])
	 	LIBS="-lncurses $LIBS"],
	[
    AC_CHECK_LIB(
	[curses], [setupterm],
	[	AC_DEFINE_UNQUOTED([HAVE_LIBCURSES], [1],
				   [Define to 1 if lib(n)curses is available])
		LIBS="-lcurses $LIBS"])
	])

AC_CHECK_LIB([execinfo],  [backtrace])
AC_CHECK_LIB([m],         [acosh])
AC_CHECK_LIB([pthread],   [sem_init])
AC_CHECK_LIB([dl],        [dlopen])

# check if SQLite3 is installed
m4_include([m4/ax_lib_sqlite3.m4])            AX_LIB_SQLITE3([])

# check if PostgreSQL is installed
m4_include([m4/ax_lib_postgresql.m4])         AX_LIB_POSTGRESQL([])

# check if PostgreSQL is usable (if present)
if test "x$found_postgresql" = "xyes" ; then
	AC_MSG_CHECKING([for PostgreSQL usability])
	saved_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS $POSTGRESQL_CFLAGS"
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([[@%:@include <libpq-fe.h>]],
		[[PGconn *db = PQconnectdbParams( 0, 0, 1 );]])],
		[USABLE_PostgreSQL=yes],
		[USABLE_PostgreSQL=no])

CPPFLAGS="$saved_CPPFLAGS"
AC_MSG_RESULT(["$USABLE_PostgreSQL"])
if test "x$USABLE_PostgreSQL" = "xyes"; then
   AC_DEFINE_UNQUOTED([USABLE_PostgreSQL], [1], [PostgreSQL code compiles])
fi
fi

# check if rdtsc (read CPU cycle counter is available.
# This is expected only on Intel CPUs
AC_MSG_CHECKING([whether CPU has rdtsc (read CPU cycle counter) opcode])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[]],
	[[unsigned int lo, hi;
          __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));]]
    )], [HAVE_rdtsc=yes], [HAVE_rdtsc=no])
if test "x$HAVE_rdtsc" = "xyes"; then
   AC_DEFINE_UNQUOTED([HAVE_RDTSC], [1], [CPU has rdtsc opcode])
fi
AM_CONDITIONAL(HAVE_RDTSC, test "x$HAVE_rdtsc" = xyes)
AC_MSG_RESULT([$HAVE_rdtsc])

# see if pthread_getaffinity_np() and pthread_setaffinity_np are available
#
AC_MSG_CHECKING([whether pthread_setaffinity_np is available])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([ [#include <pthread.h>]
                       [#include <ext/atomicity.h>]
                     ],
 	             [ [cpu_set_t cpuset;]
                       [pthread_setaffinity_np(0, sizeof(cpuset), &cpuset);]
                       [pthread_getaffinity_np(0, sizeof(cpuset), &cpuset);]
                       [_Atomic_word count = 0;]
                       [__gnu_cxx::__exchange_and_add_dispatch(&count, 1);]
                       [__gnu_cxx::__atomic_add_dispatch(&count, 1);]
                     ])],
    [HAVE_AFFINITY_NP=yes
     AC_DEFINE_UNQUOTED([HAVE_AFFINITY_NP], [1],
                        [Define to 1 if pthread_setaffinity_np() is available])
    ],
    [HAVE_AFFINITY_NP=no])
AM_CONDITIONAL(HAVE_AFFINITY_NP, test "x$HAVE_AFFINITY_NP" = xyes)
AC_MSG_RESULT([$HAVE_AFFINITY_NP])

# check for alternatives for ext/atomicity.h on OS/X 
#
AC_MSG_CHECKING([whether OSAtomicAdd32Barrier() is available])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([ [#include <libkern/OSAtomic.h>] ],
 	             [ [OSAtomicAdd32Barrier(0, 0);]   ])],
    [HAVE_OSX_ATOMIC=yes
     AC_DEFINE_UNQUOTED([HAVE_OSX_ATOMIC], [1],
                          [Define to 1 if OSAtomicAdd32Barrier() is available])
    ],
    [HAVE_OSX_ATOMIC=no])
AC_MSG_RESULT([$HAVE_OSX_ATOMIC])


# check for alternatives for ext/atomicity.h on Solaris
#
AC_MSG_CHECKING([whether atomic_add_32_nv() is available])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([ [#include <atomic.h>] ],
 	             [ [atomic_add_32_nv(0, 0);]
 	               [atomic_add_32   (0, 0);]
                     ])],
    [HAVE_SOLARIS_ATOMIC=yes
     AC_DEFINE_UNQUOTED([HAVE_SOLARIS_ATOMIC], [1],
                          [Define to 1 if atomic_add_32_nv() is available])
    ],
    [HAVE_SOLARIS_ATOMIC=no])
AC_MSG_RESULT([$HAVE_SOLARIS_ATOMIC])


# see if the compiler supports dynamic arrays
#
AC_MSG_CHECKING([if compiler supports dynamic arrays])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[extern int foo();]],
   [[int len = foo();][int array[len];][int a,r;]
    [for (a=0; a<len; ++a) array[a]=a;]
    [for (a=0; a<len; ++a) r+=array[a];] [ return r;]])],
    [DynArrays=yes], [DynArrays=no])
AC_MSG_RESULT([$DynArrays])
if test "x$DynArrays" = "xyes"; then
   AC_DEFINE_UNQUOTED([HAVE_DYNAMIC_ARRAYS], [1],
                      [Define to 1 if the compiler supports dynamic arrays])
fi

# check if the user compiles for ANDROID
#
AC_MSG_CHECKING([if we are compiling for Android])
user_wants_android=no
AC_ARG_WITH( [android],
             [AS_HELP_STRING([--with-android],
                             [enable when compiling for Android])],
             [user_wants_android=yes])

if test "x$user_wants_android" = xyes ; then
   AC_DEFINE([WANT_ANDROID], [1], [Define if compiling for Android])
fi
AM_CONDITIONAL(WANT_ANDROID, test "x$user_wants_android" = xyes)
AC_MSG_RESULT([$user_wants_android])


# check if the user wants libapl.so
#
AC_MSG_CHECKING([if we want to build libapl.so])
user_wants_libapl=no
AC_ARG_WITH( [libapl],
             [AS_HELP_STRING([--with-libapl],
                             [enable to build libapl.so])],
             [user_wants_libapl=yes])

if test "x$user_wants_libapl" = xyes ; then
   AC_DEFINE([WANT_LIBAPL], [1], [Define if building libapl.so])
fi

AM_CONDITIONAL(WANT_LIBAPL, test "x$user_wants_libapl" = xyes)
AC_MSG_RESULT([$user_wants_libapl])


AC_CONFIG_FILES([Makefile
                 debian/Makefile
                 debian/source/Makefile
                 doc/Makefile
                 gnu-apl.d/Makefile
                 rpm/Makefile
		 rpm/apl.spec
                 src/Makefile
                 src/APs/Makefile
                 src/emacs_mode/Makefile
                 src/native/Makefile
                 src/sql/Makefile
                 src/testcases/Makefile
                 src/workspaces/Makefile
		 support-files/Makefile
		 support-files/Dyalog-Keyboard/Makefile
		 support-files/old-Keyboard/Makefile
		 support-files/OS-X-Keyboard/Makefile
		 support-files/Unicomp-Keyboard/Makefile
		 support-files/WASD-Keyboard/Makefile
		 support-files/Dirk/Makefile
                 tools/Makefile
                 workspaces/Makefile
                 wslib3/Makefile
                 wslib4/Makefile
                 wslib5/Makefile
                 wslib5/APLComponentFiles/Makefile
                 wslib5/iso-apl-cf/Makefile
                ])

# APL: development configuration desired ? Set this before other xxx_WANTED
# options so that the implied xxx_WANTED options set here can be overridden
# further down
AC_ARG_VAR(DEVELOP_WANTED,
   [ set development options (see README-2-configure) default: no])
if test "x$DEVELOP_WANTED" = "xyes"; then
   AC_DEFINE_UNQUOTED([DEVELOP_WANTED], [yes],
                   [define to set options typical for software development])
   VALUE_CHECK_WANTED=yes
   PERFORMANCE_COUNTERS_WANTED=yes
   VALUE_HISTORY_WANTED=yes
   DYNAMIC_LOG_WANTED=yes
   ASSERT_LEVEL_WANTED=2
fi

# APL: max. rank ?
AC_ARG_VAR(MAX_RANK_WANTED,
   [ the max. Rank of APL values (see README-2-configure) default: 8) ])
if test "x$MAX_RANK_WANTED" = "x"; then MAX_RANK_WANTED=8; fi
AC_DEFINE_UNQUOTED([MAX_RANK_WANTED], [$MAX_RANK_WANTED],
                   [max. rank of APL values ])


# APL: CHECK() macro desired ?
AC_ARG_VAR(VALUE_CHECK_WANTED,
   [ enable CHECK() macro (see README-2-configure) default: no ])
if test "x$VALUE_CHECK_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([VALUE_CHECK_WANTED], [yes],
                   [ define to enable CHECK macro ])
fi


# APL: performance counters desired ?
AC_ARG_VAR(PERFORMANCE_COUNTERS_WANTED,
   [ enable performance counters (see README-2-configure) default: no ])
if test "x$PERFORMANCE_COUNTERS_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([PERFORMANCE_COUNTERS_WANTED], [yes],
                   [ define to enable performance counters ])
fi


# APL: ADD_EVENT() macro and friends desired ?
AC_ARG_VAR(VALUE_HISTORY_WANTED,
   [ enable CHECK() macro (see README-2-configure) default: no ])
if test "x$VALUE_HISTORY_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([VALUE_HISTORY_WANTED], [yes],
                   [ define to enable CHECK macro ])
fi


# APL: gprof profiling ?
AC_ARG_VAR(GPROF_WANTED,
   [ enable gprof (-pg) (see README-2-configure) default: no ])
if test "x$GPROF_WANTED" = "xyes"; then
   GPROF_WANTED=-pg
else
   GPROF_WANTED=
fi

AC_DEFINE_UNQUOTED([GPROF_WANTED], [-pg],
                   [ define to enable gprof profiling ])


# APL: dynamic logging desired ?
AC_ARG_VAR(DYNAMIC_LOG_WANTED,
   [ enable dynamic (see README-2-configure) default: no ])
if test "x$DYNAMIC_LOG_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([DYNAMIC_LOG_WANTED], [yes],
                   [ define to enable dynamic logging ])
fi


# APL: tracing of value flags (produces lots of messages) desired ?
AC_ARG_VAR(VF_TRACING_WANTED,
   [ enable tracing of value flags (see README-2-configure) default: no ])
if test "x$VF_TRACING_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([VF_TRACING_WANTED], [yes],
                   [ define to enable tracing of value flags ])
fi


# APL: desired assert level ?
AC_ARG_VAR(ASSERT_LEVEL_WANTED,
   [ Assert level (see README-2-configure) default: 1) ])
if test "x$ASSERT_LEVEL_WANTED" = "x"; then ASSERT_LEVEL_WANTED=1; fi
AC_DEFINE_UNQUOTED([ASSERT_LEVEL_WANTED], [$ASSERT_LEVEL_WANTED],
                   [assert level])

# APL: number of cores ?
AC_ARG_VAR(CORE_COUNT_WANTED,
   [ Core count (see README-2-configure) default: 0) ])
if test "x$CORE_COUNT_WANTED" = "x";     then CORE_COUNT_WANTED=0;  fi
if test "x$CORE_COUNT_WANTED" = "xall";  then CORE_COUNT_WANTED=-1; fi
if test "x$CORE_COUNT_WANTED" = "xALL";  then CORE_COUNT_WANTED=-1; fi
if test "x$CORE_COUNT_WANTED" = "xargv"; then CORE_COUNT_WANTED=-2; fi
if test "x$CORE_COUNT_WANTED" = "xARGV"; then CORE_COUNT_WANTED=-2; fi
if test "x$CORE_COUNT_WANTED" = "xsyl";  then CORE_COUNT_WANTED=-3; fi
if test "x$CORE_COUNT_WANTED" = "xSYL";  then CORE_COUNT_WANTED=-3; fi
AC_DEFINE_UNQUOTED([CORE_COUNT_WANTED], [$CORE_COUNT_WANTED],
                   [core count])

# APL: APserver transport ?
AC_ARG_VAR(APSERVER_TRANSPORT,
   [ APserver transport (see README-2-configure) default: TCP) ])
if test "x$APSERVER_TRANSPORT" = "x";        then APSERVER_TRANSPORT=0;  fi
if test "x$APSERVER_TRANSPORT" = "xTCP";     then APSERVER_TRANSPORT=0;  fi
if test "x$APSERVER_TRANSPORT" = "xLINUX";   then APSERVER_TRANSPORT=1;  fi
if test "x$APSERVER_TRANSPORT" = "xUNIX";    then APSERVER_TRANSPORT=2;  fi
AC_DEFINE_UNQUOTED([APSERVER_TRANSPORT], [$APSERVER_TRANSPORT],
                   [APserver transport: TCP=0, LINUX=1, UNIX=2])

# APL: APserver TCP port ?
AC_ARG_VAR(APSERVER_PORT,
   [ APserver transport (see README-2-configure) default: TCP) ])
if test "x$APSERVER_PORT" = "x";   then APSERVER_PORT=16366;  fi
AC_DEFINE_UNQUOTED([APSERVER_PORT], [$APSERVER_PORT],
                   [APserver TCP listen port])

# APL: APserver port name ?
AC_ARG_VAR(APSERVER_PATH,
   [ APserver transport (see README-2-configure) default: TCP) ])
if test "x$APSERVER_PATH" = "x";   then APSERVER_PATH="/tmp/GNU-APL/APserver";  fi
AC_DEFINE_UNQUOTED([APSERVER_PATH], ["$APSERVER_PATH"],
                   [APserver listen port name])

# APL: desired cellcount of short values ?
AC_ARG_VAR(SHORT_VALUE_LENGTH_WANTED,
    [ Cellcount of short values (see README-2-configure) default: 12) ])
if test "x$SHORT_VALUE_LENGTH_WANTED" = "x"; then SHORT_VALUE_LENGTH_WANTED=12; fi
AC_DEFINE_UNQUOTED([SHORT_VALUE_LENGTH_WANTED], [$SHORT_VALUE_LENGTH_WANTED],
                   [short value cellcount])


# APL: desired colors for CERR, CIN, and COUT ?
AC_ARG_VAR(CERR_COLOR_WANTED,   [ obsolete option ])
AC_ARG_VAR(CIN_COLOR_WANTED,    [ obsolete option ])
AC_ARG_VAR(COUT_COLOR_WANTED,   [ obsolete option ])
AC_ARG_VAR(RESET_COLORS_WANTED, [ obsolete option ])
AC_ARG_VAR(CLEAR_EOL_WANTED,    [ obsolete option ])

# APL: visible markers (debug only option) ?
AC_ARG_VAR(VISIBLE_MARKERS_WANTED,
   [ enable CHECK() macro (see README-2-configure) default: no ])
if test "x$VISIBLE_MARKERS_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([VISIBLE_MARKERS_WANTED], [yes],
                   [ define to have visible markers (DONT!)])
fi
# check if we are on i686 (to fix a strange rpmbuild problem
AM_CONDITIONAL([ON_i686], [test `uname -m` = i686])

# check if we want development options
AM_CONDITIONAL([DEVELOP],  [test x$DEVELOP_WANTED    = xyes])
AM_CONDITIONAL([SQLITE3],  [test x$found_sqlite      = xyes])
AM_CONDITIONAL([POSTGRES], [test x$USABLE_PostgreSQL = xyes])

AC_OUTPUT

