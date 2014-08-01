#!apl -f

⍝  This file contains helper definititions to make the SQL API more easy
⍝  to use. Example use:
⍝
⍝  db ← 'sqlite' SQL∆Connect '/path/to/db/file'
⍝
⍝    Connects to the database of type X with connect arguments Y.
⍝    This call returns the ID of the connection.
⍝
⍝  results ← 'select * from foo where a > ?' SQL∆Select[db] 100
⍝
⍝    Executes the query given in X with the bind parameters in Y.
⍝
⍝  SQL∆Disconnect db
⍝
⍝    Disconnects from the database connection given as X.

∇Z←type SQL∆Connect arg
⍝⍝ Connect to database of type L using connection arguments R.
⍝⍝
⍝⍝ L must be a string indicating the database type. Current supported
⍝⍝ values are 'postgresql' and 'sqlite'.
⍝⍝
⍝⍝ R is the connection parameters which depends on the type of
⍝⍝ database:
⍝⍝
⍝⍝   - For type≡'sqlite': the argument is string pointing to the
⍝⍝     database file.
⍝⍝
⍝⍝   - For type≡'postgresql', the argument is a standard connect string
⍝⍝     as described in the PostgreSQL documentation.
⍝⍝
⍝⍝ This function returns a database handle that should be used when
⍝⍝ using other SQL functions. This value should be seen as an opaque
⍝⍝ handle. It is, however, guaranteed that the handle is a scalar
⍝⍝ value.
  Z←type SQL[1] arg
∇

∇Z←SQL∆Disconnect db
⍝⍝ Disconnect from database R.
⍝⍝
⍝⍝ R is the database handle that should be disconnected. After this
⍝⍝ function has been called, no further operations are to be performed
⍝⍝ on this handle. Future calls to SQL∆Connect may reuse previously
⍝⍝ disconnected handles.
  Z←SQL[2] db
∇

∇Z←statement SQL∆Select[db] args
⍝⍝ Execute a select statement and return the result table.
⍝⍝
⍝⍝ The axis parameter indicates the database handle.
⍝⍝
⍝⍝ L is a select statement to be executed. Positional parameters can be
⍝⍝ supplied by specifying a question mark "?" in the statemement.
⍝⍝
⍝⍝ R is an array containing the values for the positional parameters.
⍝⍝ If the array is of rank 2, the statement will be executed multiple
⍝⍝ times with each row being the values for each call.
⍝⍝
⍝⍝ The return value is a rank-2 array representing the result of the
⍝⍝ select statement. Null values are returned as ⍬ and empty strings
⍝⍝ are returned as ''.
  Z←statement SQL[3,db] args
∇

∇Z←statement SQL∆Exec[db] args
⍝⍝ Execute an SQL statement that does not return a result.
⍝⍝
⍝⍝ This function is identical to SQL∆Select with the exception that it
⍝⍝ is used on statements which do not return a result table.
  Z←statement SQL[4,db] args
∇

∇Z←SQL∆Begin db
⍝⍝ Begin a transaction.
  Z←SQL[5] db
∇

∇Z←SQL∆Commit db
⍝⍝ Commit a transaction.
  Z←SQL[6] db
∇

∇Z←SQL∆Rollback db
⍝⍝ Rolls back the current transaction.
  Z←SQL[7] db
∇

∇Z←SQL∆Tables db
⍝⍝ Return an array containing the name of all tables.
  Z←SQL[8] db
∇

∇Z←db SQL∆Columns table
⍝⍝ Return an array containing information about the columns in the
⍝⍝ given table. Currently, the column layout is as follows:
⍝⍝
⍝⍝   Name
⍝⍝   Type
⍝⍝
⍝⍝ More columns containing extra information may be added in a future
⍝⍝ release.
  Z←db SQL[9] table
∇

∇Z←X (F SQL∆WithTransaction FINDDB) Y;result
  SQL∆Begin FINDDB

  →(0≠⎕NC 'X')/dyadic
  result ← '→rollback' ⎕EA 'F Y'
  →commit

dyadic:
  result ← '→rollback' ⎕EA 'X F Y'

commit:
  SQL∆Commit FINDDB
  Z ← result
  →end

rollback:
  SQL∆Rollback FINDDB
  ⎕ES "Transaction rolled back"
end:
∇

⍝
⍝  Metadata for library
⍝

∇Z←SQL⍙Author
  Z ← ,⊂'Elias Mårtenson'
∇

∇Z←SQL⍙BugEmail
  Z ← ,⊂'bug-apl@gnu.org'
∇

∇Z←SQL⍙Documentation
  Z ← ,⊂''
∇

∇Z←SQL⍙Download
  Z ← ,⊂'https://github.com/lokedhs/apl-sqlite'
∇

∇Z←SQL⍙License
  Z←,⊂'LGPL'
∇

∇Z←SQL⍙Portability
  Z←,⊂'L3'
∇

∇Z←SQL⍙Provides
  Z←,⊂'sql'
∇

∇Z←SQL⍙Requires
  Z←,⊂''
∇

∇Z←SQL⍙Version
  Z←,⊂'1.0'
∇

⍝
⍝  Load the native library
⍝

∇sql⍙load_library;result
  →(0≠⎕NC 'SQL')/skip
  result ← 'lib_sql' ⎕FX 'SQL'
  →('SQL'≡result)/skip
  ⎕ES 'Error loading native library'
skip:
∇

sql⍙load_library
)erase sql⍙load_library

⎕←'SQL lib loaded'
