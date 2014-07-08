
dist_noinst_DATA = preferences.in
dist_DATA = preferences
distdir = ${sysconfdir}/gnu-apl.d

sedcmd = "s|@GNU_APL_LIBS@|${pkglibdir}|"

# update preferences when eg. ./configure --prefix is executed
#
preferences: preferences.in Makefile
	echo ${sedcmd}
	cat $< | sed -e ${sedcmd} > $@

