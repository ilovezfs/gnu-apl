
info_TEXINFOS = apl.texi

man1_MANS = apl.1

# info_TEXINFOS = fails for more than one .texi file. We therefore install
# libapl.info explicitly
#
install-info-am:
	@echo " $(MKDIR_P) $(DESTDIR)$(infodir)"
	@       $(MKDIR_P) "$(DESTDIR)$(infodir)" || exit 1
	@echo " $(INSTALL_DATA) apl.info  libapl.info $(DESTDIR)$(infodir)"
	@       $(INSTALL_DATA) apl.info  libapl.info $(DESTDIR)$(infodir)

uninstall-info-am:
	 @echo " rm -f $(DESTDIR)$(infodir)/apl.info"
	 @       rm -f $(DESTDIR)$(infodir)/apl.info
	 @echo " rm -f $(DESTDIR)$(infodir)/libapl.info"
	 @       rm -f $(DESTDIR)$(infodir)/libapl.info

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

