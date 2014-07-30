
Component File System instructions

GNU APL must be built with SQL support prior to using the component
file system.  See SQL_README.txt

The component file system utilizes the workspace named ComponentFiles.
This is contained in a file names ComponentFiles.apl This workspace
contains all of the functions you will need.  You may need to copy
this file into your workspaces directory.

In the instructions that follow, the following variables have the
specified meanings:

DBTYPE = 'sqlite' or 'postgresql'

DBSPEC (for sqlite) = '/path/to/myfiles.db'
  or
DBSPEC (for postgresql) = 'host=localhost user=myusername password=mypassword dbname=myfiles'

Note: In the case of sqlite, if myfiles.db does not exist, the system
will create it.  In the case of PostgreSQL, the database (like
myfiles) must already exist.

fh = component file handle
(Any number of component files can be in a single database.)

fhl = file handle list - vector of one or more file handles

fnm = file name, character vector

cid = component id, scalar integer

val = arbitrary data to be associated with key (may be nested array)

Z = (0 0⍴0) - part of the standard

--------------------

DBTYPE  CF_DBCREATE  DBSPEC	create a new APL component file system
				(only do this the first time)

DBTYPE  CF_DBCONNECT  DBSPEC	open an existing component file system

CF_DISCONNECT			close all open component files and
				close the connection to the underlying SQL DB


				
---------------------

Summary of the spec

FL←CF_LIST A			return a list of files (A='')

fh←CF_CREATE fnm		create a new file

z←CF_ERASE fnm			erase (delete) file

Z←new-fnm CF_RENAME old-fnm	rename file

fh←[fh] CF_OPEN fnm		open an existing file with [fh]
				if passed or a new fh

Z←CF_CLOSE fhl			close files associated to handles

fhl←CF_INUSE			return a list of active file handles

Z←val CF_WRITE fh,cid		write val to existing component cid

cid←val CF_APPEND fh		append a new component to the file

val←CF_READ fh,cid		read a component

cid←CF_NEXT fh			the next cid to be used

Note that you cannot rely on CF_NEXT in a multi-user environment
becasue the value may change immediatly after calling CF_NEXT by
another user.  CF_APPEND take this into account.





