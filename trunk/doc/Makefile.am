
info_TEXINFOS = apl.texi

man1_MANS = apl.1

# apl.info in HTML format
#
apl.html:   apl.texi
	if [ x`which makeinfo` != x ];				\
		then makeinfo --html apl.texi -o $@ --no-split;	\
	else	touch $@; fi

apl.info:   apl.texi
	if [ x`which makeinfo` != x ];				\
		then makeinfo apl.texi -o $@ --no-split;	\
	else	touch $@; fi

dist_doc_DATA =			\
	apl.1			\
	apl.info		\
	apl.html		\
	APL-on-Macintosh.pdf

