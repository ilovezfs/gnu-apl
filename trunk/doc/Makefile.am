
info_TEXINFOS = apl.texi

man1_MANS = apl.1

# apl.texi in HTML format
#
apl.html:   apl.texi
	$(MAKEINFOHTML) $< -o $@ --no-split || touch $@

# apl.texi in info format
#
apl.info:   apl.texi
	$(MAKEINFO) $< -o $@ --no-split || touch $@

# libapl.texi in HTML format
#
libapl.html: libapl.texi
	$(MAKEINFOHTML) $< -o $@ --no-split || touch $@

# libapl.texi in info format
#
libapl.info: libapl.texi
	$(MAKEINFO) $< -o $@ --no-split || touch $@


dist_doc_DATA =			\
	apl.1			\
	apl.texi		\
	apl.info		\
	apl.html		\
	APL-on-Macintosh.pdf	\
	libapl.texi		\
	libapl.info		\
	libapl.html

