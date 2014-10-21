
noinst_PROGRAMS =	APL_keyboard		\
			APL_keyboard_learn	\
			APL_keyboard_show	\
			phrase_gen

if HAVE_RDTSC
if HAVE_AFFINITY_NP
noinst_PROGRAMS += memory_benchmark
memory_benchmark_SOURCES = memory_benchmark.cc
memory_benchmark_LDADD = -lpthread
endif
endif

dist_noinst_SCRIPTS =	check_src

LIBS =
APL_keyboard_show_SOURCES  = APL_keyboard_show.cc
APL_keyboard_learn_SOURCES = APL_keyboard_learn.cc  APL_keyboard.orig.def
APL_keyboard_SOURCES       = APL_keyboard.cc        APL_keyboard.def

phrase_gen_SOURCES = phrase_gen.cc phrase_gen.def

gen:	phrase_gen
	./phrase_gen > ../src/Prefix.def

