#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([GNU APL], [1.3], [bug-apl@gnu.org])
AC_USE_SYSTEM_EXTENSIONS
AM_INIT_AUTOMAKE
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

# Checks for header files.
#
AC_CHECK_HEADER([term.h], [], [], [
# include <curses.h> ])
AC_CHECK_HEADERS([	\
	curses.h	\
	execinfo.h	\
	fcntl.h		\
	inttypes.h	\
	omp.h		\
	stdint.h	\
	stdlib.h	\
        string.h	\
	strings.h	\
	unistd.h	\
	utmpx.h		])

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
AC_CHECK_LIB([curses],    [setupterm])

AC_CHECK_LIB([lapack],    [dgelsy_])
AC_CHECK_LIB([blas],      [dcopy_])
#AC_CHECK_LIB([gfortran], [_gfortran_transpose])

AC_CHECK_LIB([execinfo],  [backtrace])
AC_CHECK_LIB([gomp],      [omp_set_num_threads])
AC_CHECK_LIB([m],         [acosh])
AC_CHECK_LIB([rt],        [shm_open])
AC_CHECK_LIB([pthread],   [sem_init])
AC_CHECK_LIB([ncurses],   [setupterm])
AC_CHECK_LIB([dl],        [dlopen])

# check if rdtsc (read CPU cycle counter is available.
# This is expected only on Intel CPUs
AC_MSG_CHECKING([whether CPU has rdtsc (read cycle counter) opcode])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[]],
	[[unsigned int lo, hi;
          __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));]]
    )], [HAVE_rdtsc=yes], [HAVE_rdtsc=no])
AC_MSG_RESULT([$HAVE_rdtsc])
if test "x$HAVE_rdtsc" = "xyes"; then
   AC_DEFINE_UNQUOTED([HAVE_RDTSC], [1], [CPU has rdtsc opcode])
fi

# check if shm_open() is in libc (like in OSX) or in a library above
AC_MSG_CHECKING([whether shm_open() is in libc])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[extern int shm_open(const char *, int, int);]],
	[[return shm_open("", 0, 0);]]
    )], [HAVE_shm_open=yes], [HAVE_shm_open=no])
AC_MSG_RESULT([$HAVE_shm_open])
if test "x$HAVE_shm_open" = "xyes"; then
   AC_DEFINE_UNQUOTED([HAVE_shm_open], [1], [libc has shm_open()])
fi

# see if arg 3 of tputs is (*)(int) or (*)(char)
#
AC_MSG_CHECKING([type of arg3 in tputs(0, 0, (*)(arg3))])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[#include <term.h>]
                      [extern int arg3(int);]],
 	             [[tputs(0, 0, arg3);]])],
                     [TPUTS_arg3=int], [TPUTS_arg3=char])
AC_MSG_RESULT([$TPUTS_arg3])
AC_DEFINE_UNQUOTED([TPUTS_arg3], [$TPUTS_arg3],
                   [Define the type of arg3 in tputs(i, 0, (*)(arg3))])


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

AC_ARG_WITH( [android],
             [AS_HELP_STRING([--with-android],
             [Enable when cross-compiling for Android])]
             [],
             [with_android=yes])

echo "with_android = ,$with_android,"
if test "x$with_android" = xyes ; then
   AC_DEFINE([HAVE_ANDROID], [1], [Define if compiling for Android])
fi

AM_CONDITIONAL(HAVE_ANDROID, test "x$with_android" = xyes)

# see if the compiler knoes -rdynamic
#


# enable readline support if libreadline is present and the user
# has not disabled it.
#
AC_ARG_WITH( [readline],
             [AS_HELP_STRING([--with-readline],
             [support fancy command line editing @<:@default=check@:>@])],
             [],
             [with_readline=check])

LIBREADLINE=
    AS_IF(
	[test "x$with_readline" != xno],
	[AC_CHECK_LIB(
	    [readline],
	    [readline],
            [AC_SUBST([LIBREADLINE], ["-lreadline"])
             AC_DEFINE([HAVE_LIBREADLINE],
                      [1],
                      [Define if you have libreadline])
	    ],
	    [ if test "x$with_readline" != xcheck; then
		AC_MSG_FAILURE(
		    [--with-readline was given, but test for readline failed])
	      fi
	    ],
	    )
	])

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
                 src/testcases/Makefile
                 src/workspaces/Makefile
		 support-files/Makefile
                 tools/Makefile
                 workspaces/Makefile
                 wslib3/Makefile
                 wslib4/Makefile
                 wslib5/Makefile])

# APL: development configuration desired ? Set this before other xxx_WANTED
# options so that the implied xxx_WANTED options set here can be overridden
# further down
AC_ARG_VAR(DEVELOP_WANTED,
   [ set development options (see README-2-configure) default: no])
if test "x$DEVELOP_WANTED" = "xyes"; then
   AC_DEFINE_UNQUOTED([DEVELOP_WANTED], [yes],
                   [define to set options typical for software development])
   VALUE_CHECK_WANTED=yes
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
if test "x$CORE_COUNT_WANTED" = "xargv"; then CORE_COUNT_WANTED=-2; fi
if test "x$CORE_COUNT_WANTED" = "xsyl";  then CORE_COUNT_WANTED=-3; fi
AC_DEFINE_UNQUOTED([CORE_COUNT_WANTED], [$CORE_COUNT_WANTED],
                   [core count])

OPENMP=
AS_IF(	[test "x$CORE_COUNT_WANTED" != "x0"],
	[AC_SUBST([OPENMP], ["-fopenmp"])])

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
AM_CONDITIONAL([DEVELOP], [test x$DEVELOP_WANTED = xyes])

AC_OUTPUT

