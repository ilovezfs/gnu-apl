
SQL interface for GNU APL
Elias Mårtenson <lokedhs@gmail.com>

The SQL package provides an SQL interface for GNU APL.  It supports
Sqlite and PostgreSQL.

It is now a part of GNU APL so that you do not need to load or build
this separately.  However, when building GNU APL you must be sure to
include the database development libraries.  Under LinuxMint or Ubunto
these are:


	sqlite3
	sqlite3-dev
	libpq_dev
	libpq5
	


