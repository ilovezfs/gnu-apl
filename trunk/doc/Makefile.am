
info_TEXINFOS = apl.texi

man1_MANS = apl.1

# apl.info in HTML format
#
apl.html:   apl.texi
	makeinfo --html apl.texi -o $@ --no-split

dist_doc_DATA =			\
	apl.1			\
	apl.html		\
	APL-on-Macintosh.pdf

