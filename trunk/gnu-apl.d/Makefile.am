
dist_noinst_DATA = preferences preferences.in

sedcmd = "s|@GNU_APL_LIBS@|${pkglibdir}|"

preferences: preferences.in
	echo ${sedcmd}
	cat $< | sed -e ${sedcmd} > $@

