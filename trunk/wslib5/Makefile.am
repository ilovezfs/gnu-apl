
SUBDIRS = APLComponentFiles iso-apl-cf

dist_apl5_DATA =	HTML.apl	\
			FILE_IO.apl	\
			SQL.apl

apl5dir = $(pkglibdir)/wslib5

.PHONY: git-pull
git-pull:
	make -C APLComponentFiles $@
	make -C iso-apl-cf $@

