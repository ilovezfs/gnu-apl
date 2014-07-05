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

∇Z←X SQL∆Connect Y
 Z←X SQL[1] Y
∇

∇Z←SQL∆Disconnect Y
 Z←SQL[2] Y
∇

∇Z←X SQL∆Select[db] Y
 Z←X SQL[3,db] Y
∇

∇Z←X SQL∆Exec[db] Y
 Z←X SQL[4,db] Y
∇

∇Z←SQL∆Begin Y
 Z←SQL[5] Y
∇

∇Z←SQL∆Commit Y
 Z←SQL[6] Y
∇

∇Z←SQL∆Rollback Y
 Z←SQL[7] Y
∇

∇Z←SQL∆Tables Y
 Z←SQL[8] Y
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
⍝ Z←L (OP trycall) R
⍝
⍝ Call OP for each value in ARG until the function returns
⍝ true and return the index to that value. If none of the
⍝ arguments returned true, return ⎕IO+(⍴L)⌈⍴R
⍝
⍝ The retsult of this function is similar to (L OP¨ R)⍳1
⍝ except that the function will only be called until the first
⍝ found element.
⍝
∇Z←L (OP trycall) R;result;i
  i ← 0
next:
    
∇

∇Z←SYMBOL sql∆∆tryload NAME
  →(0≠⎕NC SYMBOL)/success


success:
  Z←1
∇
  

∇sql∆∆load_library;result
  →(0≠⎕NC 'SQL')/skip
  result ← 'lib_sql.so' ⎕FX 'SQL'
  →('SQL'≡result)/skip
  ⎕ES 'Error loading native library'
skip:
∇

sql∆∆load_library
)erase sql∆∆load_library

⎕←'SQL lib loaded'
