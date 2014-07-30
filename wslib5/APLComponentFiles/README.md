
A Component File System for GNU APL
===================================
Blake McBride (blake@arahant.com)

This package provides a component file system for GNU APL that
conforms to the ISO Standard.  Components are numbered with integers.
The data associated can be any APL scalar, array, or nested array of
any types.  All files are stored in an Sqlite or PostgreSQL SQL
database.  Any number of component files may be created within a
single SQL database.  All required functions are provided.

This package supports multiple simultaneous users.

Prior to using the component file system, but after all of the
required modules are loaded, a database connection must be made.
Facilities for this are provided.


This package depends upon the following:

GNU APL by Juergen Sauermann <juergen.sauermann@t-online.de>
http://www.gnu.org/software/apl/apl.html
Subversion:  http://svn.savannah.gnu.org/svn/apl/trunk


SQL Interface by  Elias Mårtenson <lokedhs@gmail.com>
This library provides an SQL interface to Sqlite & PostgreSQL
It is now a part of GNU APL so that you do not
need to load or build this separately.
