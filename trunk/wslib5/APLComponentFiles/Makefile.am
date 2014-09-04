
dist_apl5ACF_DATA =	AUTHORS			\
			LAST_GIT_PULL		\
			CF_README.txt		\
			LICENSE			\
			README_FIRST.txt	\
			SQL_README.txt		\
			ComponentFiles.apl	\
			README.md

apl5ACFdir = $(pkglibdir)/wslib5/APLComponentFiles

.PHONY: git-pull
git-pull:
	rm -Rf tmp
	svn export https://github.com/blakemcbride/APLComponentFiles/trunk tmp
	mv -f tmp/*apl tmp/*txt tmp/*md tmp/LICENSE .
	rm -Rf tmp
	@echo "Last GIT pull on `date '+%F %R'`" > LAST_GIT_PULL


