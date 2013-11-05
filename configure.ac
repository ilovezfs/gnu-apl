#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([GNU APL], [1.1], [bug-apl@gnu.org])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

AC_GNU_SOURCE
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h stdint.h stdlib.h \
                  string.h strings.h unistd.h execinfo.h])

# Checks for typedefs, structures, and compiler characteristics.
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
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([floor memset munmap pow sqrt strchr strerror])

# Checks for libraries.
AC_CHECK_LIB([lapack], [dgelsy_])
AC_CHECK_LIB([blas], [dcopy_])
#AC_CHECK_LIB([gfortran], [_gfortran_transpose])
AC_CHECK_LIB([execinfo], [backtrace])
AC_CHECK_LIB([intl], [libintl_gettext])
AC_CHECK_LIB([m], [acosh])
AC_CHECK_LIB([rt], [shm_open])
AC_CHECK_LIB([pthread], [sem_init])

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
            [AC_SUBST([LIBREADLINE], ["-lreadline -lncurses"])
             AC_DEFINE([HAVE_LIBREADLINE],
                      [1],
                      [Define if you have libreadline])
	    ],
	    [ if test "x$with_readline" != xcheck; then
		AC_MSG_FAILURE(
		    [--with-readline was given, but test for readline failed])
	      fi
	    ],
	    -lncurses)
	])

AC_CONFIG_FILES([Makefile po/Makefile.in po/Makefile
                 debian/Makefile
                 debian/source/Makefile
                 rpm/Makefile
		 rpm/apl.spec
                 src/Makefile
                 src/APs/Makefile
                 workspaces/Makefile
                 wslib1/Makefile
                 wslib2/Makefile])

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


# APL: print semaphore desired ?
AC_ARG_VAR(PRINT_SEMA_WANTED,
   [ enable print semaphore (see README-2-configure) default: no ])
if test "x$PRINT_SEMA_WANTED" = "xyes"; then
AC_DEFINE_UNQUOTED([PRINT_SEMA_WANTED], [yes],
                   [ define to enable the print semaphore ])
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


# APL: desired cellcount of short values ?
AC_ARG_VAR(SHORT_VALUE_LENGTH_WANTED,
    [ Cellcount of short values (see README-2-configure) default: 12) ])
if test "x$SHORT_VALUE_LENGTH_WANTED" = "x"; then SHORT_VALUE_LENGTH_WANTED=12; fi
AC_DEFINE_UNQUOTED([SHORT_VALUE_LENGTH_WANTED], [$SHORT_VALUE_LENGTH_WANTED],
                   [short value cellcount])

# APL: desired colors for CERR, CIN, and COUT ?
AC_ARG_VAR(CERR_COLOR_WANTED,
         [ CERR color (see README-2-configure) default: CSI 0;35;47m  ])
AC_ARG_VAR(CIN_COLOR_WANTED,
         [ CIN  color (see README-2-configure) default: CSI 0;30;47m ])
AC_ARG_VAR(COUT_COLOR_WANTED,
         [ COUT color (see README-2-configure) default: CSI 0;30;48m ])
AC_ARG_VAR(RESET_COLORS_WANTED,
         [ reset colors (see README-2-configure) default: CSI 0;38;48m ])
AC_ARG_VAR(CLEAR_EOL_WANTED,
         [ clear to end-of-line (see README-2-configure) default: CSI K ])

if test "x$CERR_COLOR_WANTED" = "x";
   then CERR_COLOR_WANTED="27, 91, '0', ';', '3', '5', ';', '4', '8', 'm'" ; fi
if test "x$CIN_COLOR_WANTED" = "x";
   then  CIN_COLOR_WANTED="27, 91, '0', ';', '3', '0', ';', '4', '7', 'm'" ; fi
if test "x$COUT_COLOR_WANTED" = "x";
   then COUT_COLOR_WANTED="27, 91, '0', ';', '3', '0', ';', '4', '8', 'm'" ; fi
if test "x$RESET_COLORS_WANTED" = "x";
   then RESET_COLORS_WANTED="27, 91, '0', ';', '3', '8', ';', '4', '8', 'm'"; fi
if test "x$CLEAR_EOL_WANTED" = "x";
   then CLEAR_EOL_WANTED="27, 91, 'K'" ; fi

AC_DEFINE_UNQUOTED([CERR_COLOR_WANTED], [$CERR_COLOR_WANTED], [CERR color])
AC_DEFINE_UNQUOTED([CIN_COLOR_WANTED],  [$CIN_COLOR_WANTED],  [CIN color])
AC_DEFINE_UNQUOTED([COUT_COLOR_WANTED], [$COUT_COLOR_WANTED], [COUT color])
AC_DEFINE_UNQUOTED([RESET_COLORS_WANTED],[$RESET_COLORS_WANTED],[default color])
AC_DEFINE_UNQUOTED([CLEAR_EOL_WANTED], [$CLEAR_EOL_WANTED], [clear to EOL])

AC_OUTPUT
