#!/usr/local/bin/apl --script --

 ⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝
⍝ FILE_IO.apl 2014-07-29 15:40:42 (GMT+2)
⍝
 ⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝

⍝ This library contains APL wrapper functions the the native function
⍝ implemented in src/native/file_io.cc
⍝
⍝ The purpose is to give functions for file I/O meaningful names instead
⍝ of difficult-to-remember numbers. The function name is normally the name
⍝ of the C-function that is called by the native function.
⍝
⍝  Legend: d - table of dirent structs
⍝          e - error code
⍝          i - integer
⍝          h - file handle (integer)
⍝          n - names (nested vector of strings)
⍝          s - string
⍝          A1, A2, ...  nested vector with elements A1, A2, ...


      ⍝ share library file_io.so as FILE_IO
      ⍝
      ('F' ≠ ↑'lib_file_io.so' ⎕FX 'FILE_IO') / '⋆⋆⋆ ⎕FX FILE_IO failed'


∇Zi ← FIO∆errno
 ⍝⍝ errno (of last call)
 Zi ← FILE_IO[1] ''
∇

∇Zs ← FIO∆strerror Be
 ⍝⍝ strerror(Be)
 Zs ← FILE_IO[2] Be
∇

∇Zh ← As FIO∆fopen Bs
 ⍝⍝ fopen(Bs, As) filename Bs
 Zh ← FILE_IO[3] Bs
∇

∇Zh ← FIO∆fopen_ro Bs
 ⍝⍝ fopen(Bs, "r") filename Bs
 Zh ← FILE_IO[3] Bs
∇

∇Ze ← FIO∆fclose Bh
 ⍝⍝ fclose(Bh)
 Ze ← FILE_IO[4] Bh
∇

∇Ze ← FIO∆file_errno Bh
 ⍝⍝ errno (of last call on Bh)
 Ze ← FILE_IO[5] Bh
∇

∇Zi ← Ai FIO∆fread Bh
 ⍝⍝ fread(Zi, 1, Ai, Bh) 1 byte per Zi
 ⍝⍝ read (at most) 5000 items if Ai is not defined
 →(0≠⎕NC 'Ai')/↑⎕LC+1 ◊ Ai←5000
 Zi ← Ai FILE_IO[ 6] Bh
∇

∇Zi ← Ai FIO∆fwrite Bh
 ⍝⍝ fwrite(Ai, 1, ⍴Ai, Bh) 1 byte per Zi
 Zi ← Ai FILE_IO[7] Bh
∇

∇Zi ← Ai FIO∆fgets Bh
 ⍝⍝ fgets(Zi, Ai, Bh) 1 byte per Zi
 ⍝⍝ read (at most) 5000 items unless Ai is defined
 →(0≠⎕NC 'Ai')/↑⎕LC+1 ◊ Ai←5000
 Zi ← Ai FILE_IO[8] Bh
∇

∇Zi ← FIO∆fgetc Bh
 ⍝⍝ fgetc(Zi, Bh) 1 byte per Zi
 Zi ← FILE_IO[9] Bh
∇

∇Zi ← FIO∆feof Bh
 ⍝⍝ feof(Bh)
 Zi ← FILE_IO[10] Bh
∇

∇Zi ← FIO∆ferror Bh
 ⍝⍝ ferror(Bh)
 Zi ← FILE_IO[11] Bh
∇

∇Zi ← FIO∆ftell Bh
 ⍝⍝ ftell(Bh)
 Zi ← FILE_IO[12] Bh
∇

∇Zi ← Ai FIO∆fseek_cur Bh
 ⍝⍝ fseek(Bh, Ai, SEEK_SET)
 Zi ← Ai FILE_IO[13] Bh
∇

∇Zi ← Ai FIO∆fseek_set Bh
 ⍝⍝ fseek(Bh, Ai, SEEK_CUR)
 Zi ← Ai FILE_IO[14] Bh
∇

∇Zi ← Ai FIO∆fseek_end Bh
 ⍝⍝ fseek(Bh, Ai, SEEK_END)
 Zi ← Ai FILE_IO[15] Bh
∇

∇Zi ← FIO∆fflush Bh
 ⍝⍝ fflush(Bh)
 Zi ← FILE_IO[16] Bh
∇

∇Zi ← FIO∆fsync Bh
 ⍝⍝ fsync(Bh)
 Zi ← FILE_IO[17] Bh
∇

∇Zi ← FIO∆fstat Bh
 ⍝⍝ fstat(Bh)
 Zi ← FILE_IO[18] Bh
∇

∇Zi ← FIO∆unlink Bh
 ⍝⍝ unlink(Bc)
 Zi ← FILE_IO[19] Bh
∇

∇Zi ← FIO∆mkdir_777 Bh
 ⍝⍝ mkdir(Bc, 0777)
 Zi ← FILE_IO[20] Bh
∇

∇Zi ← Ai FIO∆mkdir Bh
 ⍝⍝ mkdir(Bc, Ai)
 Zi ← Ai FILE_IO[20] Bh
∇

∇Zi ← FIO∆rmdir Bh
 ⍝⍝ rmdir(Bc)
 Zi ← FILE_IO[21] Bh
∇

∇Zi ← FIO∆printf B
 ⍝⍝ printf(B1, B2...) format B1
 Zi ← B FILE_IO[22] 1
∇

∇Zi ← FIO∆fprintf_stderr B
 ⍝⍝ fprintf(stderr, B1, B2...) format B1
 Zi ← B  FILE_IO[22] 2
∇

∇Zi ← Ah FIO∆fprintf B
 ⍝⍝ fprintf(Ah, B1, B2...) format B1
 Zi ← Ah FILE_IO[22] B
∇

∇Zi ← Ac FIO∆fwrite Bh
 ⍝⍝ fwrite(Ac, 1, ⍴Ac, Bh) Unicode Ac Output UTF-8
 Zi ← Ac FILE_IO[23] Bh
∇

∇Zh ← As FIO∆popen Bs
 ⍝⍝ popen(Bs, As) command Bs mode As
 Zh ← As FILE_IO[24] Bs
∇

∇Zh ← FIO∆popen_ro Bs
 ⍝⍝ popen(Bs, "r") command Bs
 Zh ← FILE_IO[24] Bs
∇

∇Ze ← FIO∆pclose Bh
 ⍝⍝ pclose(Bh)
 Ze ← FILE_IO[25] Bh
∇

∇Zs ← FIO∆read_file Bs
 ⍝⍝ return entire file Bs as byte vector
 Zs ← FILE_IO[26] Bs
∇

∇Zs ← As FIO∆rename Bs
 ⍝⍝ rename file As to Bs
 Zs ← As FILE_IO[27] Bs
∇

∇Zd ← FIO∆read_directory Bs
 ⍝⍝ return content of directory Bs
 Zd ← FILE_IO[28] Bs
∇

∇Zn ← FIO∆files_in_directory Bs
 ⍝⍝ return file names in directory Bs
 Zn ← FILE_IO[29] Bs
∇

∇Zs ← FIO∆getcwd
 ⍝⍝ getcwd()
 Zs ← FILE_IO 30
∇

∇Zn ← FIO∆access Bs
 ⍝⍝ access(As, Bs) As ∈ 'RWXF'
 Zn ← As FILE_IO[31] Bs
∇


⍝ meta data for this library...
⍝

∇Z←FIO⍙Author
 Z←,⊂'Jürgen Sauermann'
∇

∇Z←FIO⍙BugEmail
 Z←,⊂'bug-apl@gnu.org'
∇

∇Z←FIO⍙Documentation
 Z←,⊂''
∇

∇Z←FIO⍙Download
 Z←,⊂'http://svn.savannah.gnu.org/viewvc/trunk/wslib5/Makefile.in?root=apl'
∇

∇Z←FIO⍙License
 Z←,⊂'LGPL'
∇

∇Z←FIO⍙Portability
 Z←,⊂'L3'
∇

∇Z←FIO⍙Provides
 Z←,⊂'file-io'
∇

∇Z←FIO⍙Requires
 Z←,⊂''
∇

∇Z←FIO⍙Version
 Z←,⊂'1.0'
∇

