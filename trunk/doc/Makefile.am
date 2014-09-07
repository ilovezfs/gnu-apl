
info_TEXINFOS = apl.texi

man1_MANS = apl.1

# apl.info in HTML format
#
apl.html:   apl.texi
	if [ -x `which makeinfo` ];				\
		then makeinfo --html apl.texi -o $@ --no-split;	\
	else	touch $@; fi

dist_doc_DATA =			\
	apl.1			\
	apl.html		\
	APL-on-Macintosh.pdf

