
# targets that can be made without running ./configure (and that typically
# run ./configure with various options
#
.PHONY: help develop develop_lib parallel parallel1
help develop develop_lib parallel parallel1:
	make -f Makefile.incl $@

# all other targets should run ./configure first
#
%:
	rm -f Makefile
	./configure
	make $@

