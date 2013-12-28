
noinst_LTLIBRARIES = lib_file_io.la	\
                  lib_template_F0.la	\
                  lib_template_F12.la	\
                  lib_template_OP1.la	\
                  lib_template_OP2.la	

lib_template_F0_la_SOURCES = template_F0.cc template.hh
lib_template_F0_la_LD_FLAGS = -module -avoid-version

lib_template_F12_la_SOURCES = template_F12.cc template.hh
lib_template_F12_la_LD_FLAGS = -module -avoid-version

lib_template_OP1_la_SOURCES = template_OP1.cc template.hh
lib_template_OP1_la_LD_FLAGS = -module -avoid-version

lib_template_OP2_la_SOURCES = template_OP2.cc template.hh
lib_template_OP2_la_LD_FLAGS = -module -avoid-version

lib_file_io_la_SOURCES = file_io.cc
lib_file_io_la_LD_FLAGS = -module -avoid-version

all:
	@echo
	@echo "On success the .so files reside in the .libs subdirectory"
	@echo
