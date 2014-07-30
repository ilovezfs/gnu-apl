⍝!

⍝ Package:    ISO APL Component Files
⍝ Author:     David B. Lamkins
⍝ Email:      david@lamkins.net
⍝ Date:       2014-07-06
⍝ License:    GPLv3
⍝ Platform:   GNU APL
⍝ Reference:  http://www.math.uwaterloo.ca/~ljdickey/apl-rep/docs/is13751.pdf
⍝ +           Programming Language APL, Extended; Annex A: Component Files
⍝ Bug report: https://lists.gnu.org/mailman/listinfo/bug-apl
⍝ Prefix:     CF
⍝ API:        CF_LIST, CF_CREATE, CF_ERASE, CF_RENAME, CF_OPEN, CF_CLOSE,
⍝ +           CF_INUSE, CF_WRITE, CF_APPEND, CF_READ, CF_NEXT
⍝ Extensions: CF_DROP, CF_EXISTS, CF_TRANSACTION_BEGIN, CF_TRANSACTION_COMMIT,
⍝ +           CF_TRANSACTION_ABORT, CF_VERSION, CF_DEFAULT_LIBRARY

⍝ ================================================================
⍝ PUBLIC API - ISO 13751 COMPONENT FILES
⍝ ================================================================

∇z←CF_LIST lib
  ⍝ Return a character matrix containing the names of component files
  ⍝ in lib. The lib must be empty to denote the default library;
  ⍝ non-empty values are reserved for future expansion. If no files are
  ⍝ present in lib, return an empty character matrix.
  ⎕es(0≠⍴,lib)/'NONCE ERROR'
  CF⍙ensure_native_libraries
  z←CF⍙extension CF⍙ls CF⍙default_library
∇

∇z←CF_CREATE name;path
  ⍝ Return a handle to a newly-created component file of given name.
  ⍝ The name must have a .cf extension. Signal an error if the file
  ⍝ exists or can not be created.
  CF⍙ensure_native_libraries
  CF⍙check_name name
  path←CF⍙path name
  ⎕es(CF⍙file_exists path)/5 4
  z←CF⍙register_handle CF⍙initialize CF⍙tie path
∇

∇z←CF_ERASE name
  ⍝ Erase the named component file and return an empty numeric matrix.
  ⍝ The name must have a .cf extension. Signal an error if the
  ⍝ component file can not be erased.
  CF⍙ensure_native_libraries
  CF⍙check_name name
  ⊣ CF⍙unlink CF⍙path name
  z←0 0⍴0
∇

∇z←new_name CF_RENAME name
  ⍝ Rename the the named component file and return an empty numeric
  ⍝ matrix. Signal an error if the component file can not be renamed.
  CF⍙ensure_native_libraries
  CF⍙check_name new_name
  CF⍙check_name name
  ⎕es(~CF⍙file_exists name)/5 4
  ⎕es(CF⍙file_exists new_name)/5 4
  ⊣ (CF⍙path name) CF⍙rename (CF⍙path new_name)
  z←0 0⍴0
∇

∇z←handle CF_OPEN name
  ⍝ Open a component file and return a handle. The file must exist.
  ⍝ The name must have a .cf extension. Signal an error if the file
  ⍝ can not be opened. The left function argument is reserved to
  ⍝ specify the desired handle; this is reserved for future expansion.
  ⎕es(0≠⎕nc 'handle')/'NONCE ERROR'
  CF⍙ensure_native_libraries
  ⎕es(~CF⍙file_exists CF⍙path name)/5 4
  ⊣ CF⍙register_handle z←CF⍙tie CF⍙path name
∇

∇z←CF_CLOSE handle_list
  ⍝ Given a list of file handles, close each file and return an empty
  ⍝ numeric matrix. Signal an error if any file is unable to be closed.
  CF⍙ensure_native_libraries
  ⊣ CF⍙close¨∊handle_list
  z←0 0⍴0
∇

∇z←CF_INUSE
  ⍝ Return a list of currently open file handles.
  z←CF⍙handles
∇

∇z←data CF_WRITE handle_and_component;h;c;stmt
  ⍝ The right argument is a two-element vector of handle and component
  ⍝ ID. Write data to the specified component and return an empty
  ⍝ numeric matrix. Signal an error if the write is unable to be
  ⍝ completed.
  CF⍙ensure_native_libraries
  (h c)←handle_and_component
  stmt←'replace into component ( oid, vers, data ) values ( ?, ?, ? )'
  ⊣ stmt CF¯SQL[4,h] (c) (CF_VERSION h) (CF⍙encode data)
  ⊣ h CF⍙update_max_cn c
  z←0 0⍴0
∇

∇z←data CF_APPEND handle
  ⍝ Append data to the file and return its component ID. Signal an
  ⍝ error if the append is unsuccessful.
  ⊣ data CF_WRITE handle,z←handle CF⍙update_max_cn CF_NEXT handle
∇

∇z←CF_READ handle_and_component;h;c;raw
  ⍝ The right argument is a two-element vector of handle and component
  ⍝ ID. Return the data of the specified component. Signal an error if
  ⍝ the read is unable to be completed.
  CF⍙ensure_native_libraries
  (h c)←handle_and_component
  raw←,⊃'select data from component where oid = ?' CF¯SQL[3,h] c
  ⎕es(⍬≡raw)/5 4
  z←CF⍙decode raw
∇

∇z←CF_NEXT handle
  ⍝ Return the next component ID of the file. For an empty file, return
  ⍝ 1. For a non-empty file, return one greater than the largest
  ⍝ component ID ever returned by an append or specified in a write for
  ⍝ this file. Signal an error if the next component ID cannot be
  ⍝ returned.
  CF⍙ensure_native_libraries
  z←↑'select 1 + current from max_cn where oid = 1' CF¯SQL[4,handle] ⍬
∇

⍝ ================================================================
⍝ PUBLIC API - EXTENSIONS
⍝ ================================================================

∇z←CF_DROP handle_and_component;h;c
  ⍝ The right argument is a two-element vector of handle and component
  ⍝ ID. Drop the specified component. Attempting to read a dropped
  ⍝ component will cause the read to signal an error. Signal an error
  ⍝ if the component can not be dropped.
  CF⍙ensure_native_libraries
  (h c)←handle_and_component
  ⊣ 'delete from component where oid = ?' CF¯SQL[4,h] c
  z←0 0⍴0
∇

∇z←CF_EXISTS handle_and_component;h;c
  ⍝ The right argument is a two-element vector of handle and component
  ⍝ ID. Return true if the specified component exists; otherwise
  ⍝ return false. Signal an error if the presence of the component can
  ⍝ not be determined.
  CF⍙ensure_native_libraries
  (h c)←handle_and_component
  z←↑,'select 1 from component where oid = ?' CF¯SQL[3,h] c
∇

∇z←CF_TRANSACTION_BEGIN handle
  ⍝ Begin a transaction on the given file and return a transaction ID.
  ⍝ A transaction begin must be paired with a commit or abort.
  ⍝ Transactions must not be nested. Signal an error if the transaction
  ⍝ can not be started.
  ⍝
  ⍝ Transactions are useful to batch operations which must be committed
  ⍝ as a whole for consistency.
  ⍝
  ⍝ Transactions are also useful to batch operations for performance
  ⍝ reasons. Every component file operation is executed in an implicit
  ⍝ transaction if no explicity transaction has begun; this guarantees
  ⍝ that the file is never left in an inconsistent state. Transactions
  ⍝ are very slow relative to file operations; performance scales with
  ⍝ the number of operations combined within a transaction.
  CF⍙ensure_native_libraries
  ⊣ CF¯SQL[5] handle
  z←handle
∇

∇z←CF_TRANSACTION_COMMIT transaction
  ⍝ Commit the given transaction. Return an empty numeric matrix.
  ⍝ Signal an error if the transaction can not be committed.
  CF⍙ensure_native_libraries
  ⊣ CF¯SQL[6] transaction
  z←0 0⍴0
∇

∇z←CF_TRANSACTION_ABORT transaction
  ⍝ Abort the given transaction. Return an empty numeric matrix.
  ⍝ Signal an error if the transaction can not be aborted.
  CF⍙ensure_native_libraries
  ⊣ CF¯SQL[7] transaction
  z←0 0⍴0
∇

∇z←CF_VERSION handle
  ⍝ Return the version number of the component file implementation
  ⍝ under which the given file was created.
  CF⍙ensure_native_libraries
  z←↑'select current from version where oid = 1' CF¯SQL[3,handle] ⍬
∇

∇z←CF_DEFAULT_LIBRARY path
  ⍝ Return the current path to the default library. If given path is
  ⍝ not empty, change the default library. The given path, if not
  ⍝ empty, must be an absolute path. Note that although the initial
  ⍝ default path is the current working directory - expressed as `.` -
  ⍝ this function will not allow setting the default path to `.`.
  z←CF⍙default_library
  →(0=⍴path)/0
  ⎕es(CF⍙path_separator≠↑path)/5 4
  CF⍙default_library←path
∇

⍝ ================================================================
⍝ PRIVATE IMPLEMENTATION
⍝ ================================================================

∇CF⍙ensure_native_libraries
  ⍝ Ensure that the SQL library is loaded.
  →(0≠⎕nc 'CF¯SQL')/file_io
  ⊣ 'lib_sql' ⎕fx 'CF¯SQL'
file_io:
  ⍝ Ensure that the file I/O library is loaded.
  →(0≠⎕nc 'CF¯FILEIO')/0
  ⊣ 'lib_file_io' ⎕fx 'CF¯FILEIO'
∇

⍝ Vector of open component file handles.
CF⍙handles←⍬

∇z←CF⍙register_handle handle
  ⍝ Register a handle on the list of open component file handles.
  z←handle
  →(handle∊CF⍙handles)/0
  CF⍙handles←CF⍙handles,handle
∇

∇z←CF⍙unregister_handle handle
  ⍝ Remove a handle from the list of open component file handles.
  CF⍙handles←CF⍙handles~handle
  z←handle
∇

∇z←CF⍙decode data
  ⍝ Decode from the database textual form to APL data.
  z←⍎data
∇

∇z←CF⍙encode data
  ⍝ Encode APL data into a textual form for the database.
  z←5↓2⎕tf 'data'
∇

⍝ Default library path.
CF⍙default_library←'.'

⍝ Path separator.
CF⍙path_separator←'/'

⍝ Component file extension.
CF⍙extension←'.cf'

∇z←CF⍙path name
  ⍝ Return a full path to the named component file.
  ⎕es(~CF⍙extension≡¯3↑name)/5 4
  z←CF⍙default_library,CF⍙path_separator,name
∇

∇z←suffix CF⍙ls path;dir
  ⍝ Return a character matrix of directory entries. Left argument,
  ⍝ if present, filters entries by suffix.
  z←0 0⍴''
  dir←CF¯FILEIO[28] path
  dir←(⍳↑⍴dir) (4+⎕io)⌷dir
  ⍎(0≠⎕nc 'suffix')/'dir←((⊂,suffix)≡¨(↑-⍴,suffix)↑¨dir)/dir'
  →(0=⍴dir)/0
  dir←⊃dir
  z←dir[(⎕ucs ⎕io-⍨⍳256)⍋dir;]
∇

∇z←old CF⍙rename new
  ⍝ Rename a file.
  ⊣ old CF¯FILEIO[27] new
  z←new
∇

∇z←CF⍙file_exists path;h
  ⍝ Return true if path exists.
  z←0
  h←CF¯FILEIO[3] path
  →(h<0)/0
  ⊣ CF¯FILEIO[4] h
  z←1
∇

∇z←CF⍙tie path
  ⍝ Get a SQLite handle for the given path.
  z←'sqlite' CF¯SQL[1] path
∇

∇z←CF⍙initialize handle
  ⍝ Initialize the SQLite database for a component file.
  ⊣ 'create table component ( data text, vers int )' CF¯SQL[4,handle] ⍬
  ⊣ 'create table version ( current int )' CF¯SQL[4,handle] ⍬
  ⊣ 'create table max_cn ( current int )' CF¯SQL[4,handle] ⍬
  ⊣ 'insert into max_cn ( oid, current ) values ( 1, ? )' CF¯SQL[4,handle] (0)
  ⊣ 'insert into version ( oid, current ) values ( 1, ? )' CF¯SQL[4,handle] (1)
  z←handle
∇

∇z←handle CF⍙update_max_cn component;next_cn
  ⍝ Record the new maximum component number for a component file.
  next_cn←CF_NEXT handle
  z←component⌈next_cn
  →(next_cn>component)/0
  ⊣ 'replace into max_cn ( oid, current ) values ( 1, ? )' CF¯SQL[4,handle] (z)
∇

∇z←CF⍙untie handle
  ⍝ Close the referenced component file.
  ⎕es(~handle∊CF⍙handles)/5 4
  ⊣ CF¯SQL[2] handle
  z←handle
∇

∇z←CF⍙close handle
  ⍝ Close and unregister one file handle.
  z←CF⍙unregister_handle CF⍙untie handle
∇

∇z←CF⍙unlink path
  ⍝ Delete the file on path.
  z←CF¯FILEIO[19] path
∇

∇CF⍙check_name name
  ⍝ Check that the given name ends with the proper extension and has
  ⍝ no path separator.
  ⎕es(~CF⍙extension≡¯3↑name)/5 4
  ⎕es(CF⍙path_separator∊name)/5 4
∇

⍝ Set the attributes such that none of the CF functions or operators
⍝ can be interrupted or suspended. See pg. 256, section A.3 of the ISO
⍝ 13751 spec. Note that we don't match ¯ as a break character
⍝ following the CF prefix. CF¯ is reserved to name native function
⍝ bindings; we don't attempt to set attributes on native functions.
⊣ (⊂0 1 1 0)⎕fx¨⊃¨⎕cr¨⊂[1+⎕io](('CF'⎕nl 3 4)[;2+⎕io]∊'_∆⍙')⌿'CF'⎕nl 3 4
