
dist_apl5iac_DATA =     AUTHORS		\
			LAST_GIT_PULL	\
			README.md	\
			_control_.apl	\
			_metadata_	\
			iso_cf.apl	\
			perf.apl	\
			test.apl

apl5iacdir = $(pkglibdir)/wslib5/iso-apl-cf

.PHONY: git-pull
git-pull:
	rm -Rf tmp
	svn export https://github.com/TieDyedDevil/iso-apl-cf/trunk tmp
	mv -f tmp/*apl tmp/*md tmp/_metadata_ .
	rm -Rf tmp
	@echo "Last GIT pull on `date '+%F %R'`" > LAST_GIT_PULL

