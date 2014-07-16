
dist_doc_DATA = SQL.apl

pkglib_LTLIBRARIES = lib_sql.la
AM_CXXFLAGS =		-I .. -I ../..
AM_LDFLAGS =
lib_sql_la_SOURCES =	apl-sqlite.cc	apl-sqlite.hh		\
			Connection.cc	Connection.hh		\
					ArgListBuilder.hh	\
					Provider.hh

if SQLITE3

AM_CXXFLAGS += $(SQLITE3_CFLAGS)
AM_LDFLAGS += $(SQLITE3_LDFLAGS)
lib_sql_la_SOURCES +=	SqliteArgListBuilder.cc	SqliteArgListBuilder.hh	\
			SqliteConnection.cc	SqliteConnection.hh	\
			SqliteResultValue.cc	SqliteResultValue.hh	\
			SqliteProvider.cc	SqliteProvider.hh

endif

if POSTGRES

AM_CXXFLAGS += $(POSTGRESQL_CFLAGS)
AM_LDFLAGS += $(POSTGRESQL_LDFLAGS)
lib_sql_la_SOURCES +=	PostgresArgListBuilder.cc PostgresArgListBuilder.hh \
			PostgresConnection.cc     PostgresConnection.hh	\
			PostgresProvider.cc       PostgresProvider.hh

endif


