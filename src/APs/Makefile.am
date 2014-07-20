
bin_PROGRAMS = AP100 AP210 APserver

common_files =	APmain.cc APmain.hh			\
                ../Backtrace.cc ../Backtrace.hh		\
		../CDR_string.hh ../Svar_signals.hh	\
		../Svar_DB.cc ../Svar_DB.hh		\
		../Svar_record.cc ../Svar_record.hh

AP100_SOURCES = AP100.cc $(common_files)
AP210_SOURCES = AP210.cc $(common_files)

APserver_SOURCES = APserver.cc				\
		Svar_DB_server.cc Svar_DB_server.hh	\
		../Svar_record.cc ../Svar_record.hh

AP100_CXXFLAGS    = -I .. -g -O2 -DAP_NUM=100
AP210_CXXFLAGS    = -I .. -g -O2 -DAP_NUM=210
APserver_CXXFLAGS = -I .. -g -O2

AM_CXXFLAGS = $(CXX_RDYNAMIC)

if DEVELOP
   AM_CXXFLAGS += -Werror -Wall
   AM_CXXFLAGS += -Wno-sign-compare -Wno-strict-aliasing
endif

AM_MAKEFLAGS = -j 8

