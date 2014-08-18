
dist_noinst_DATA = preferences.in keyboard1.txt
dist_DATA = preferences keyboard1.txt
distdir = ${sysconfdir}/gnu-apl.d

sedcmd1 = "s|@GNU_APL_LIBS@|${pkglibdir}|"
sedcmd2 = "s|@GNU_APL_ETC@|${sysconfdir}|"

# update preferences when eg. ./configure --prefix is executed
#
preferences: preferences.in Makefile
	echo ${sedcmd}
	cat $< | sed -e ${sedcmd1} | sed -e ${sedcmd2} > $@

