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
 Zi ← FILE_IO[1] ''             ⍝ errno (of last call)
∇

∇Zs ← FIO∆strerror Be
 Zs ← FILE_IO[2] Be             ⍝ strerror(Be)
∇

∇Zh ← As FIO∆fopen Bs
 Zh ← FILE_IO[3] Bs             ⍝ fopen(Bs, As) filename Bs
∇

∇Zh ← FIO∆fopen_ro Bs
 Zh ← FILE_IO[3] Bs             ⍝ fopen(Bs, "r") filename Bs
∇

∇Ze ← FIO∆fclose Bh
 Ze ← FILE_IO[4] Bh             ⍝ fclose(Bh)
∇

∇Ze ← FIO∆file_errno Bh
 Ze ← FILE_IO[5] Bh             ⍝ errno (of last call on Bh)
∇

∇Zi ← Ai FIO∆fread Bh
 →(0≠⎕NC 'Ai')/↑⎕LC+1 ◊ Ai←5000 ⍝ read 5000 items unless Ai is defined
 Zi ← Ai FILE_IO[ 6] Bh         ⍝ fread(Zi, 1, Ai, Bh) 1 byte per Zi
∇

∇Zi ← Ai FIO∆fwrite Bh
 Zi ← Ai FILE_IO[7] Bh          ⍝ fwrite(Ai, 1, ⍴Ai, Bh) 1 byte per Zi
∇

∇Zi ← Ai FIO∆fgets Bh
 →(0≠⎕NC 'Ai')/↑⎕LC+1 ◊ Ai←5000 ⍝ read 5000 items unless Ai is defined
 Zi ← Ai FILE_IO[8] Bh          ⍝ fgets(Zi, Ai, Bh) 1 byte per Zi
∇

∇Zi ← FIO∆fgetc Bh
 Zi ← FILE_IO[9] Bh             ⍝ fgetc(Zi, Bh) 1 byte per Zi
∇

∇Zi ← FIO∆feof Bh
 Zi ← FILE_IO[10] Bh            ⍝ feof(Bh)
∇

∇Zi ← FIO∆ferror Bh
 Zi ← FILE_IO[11] Bh            ⍝ ferror(Bh)
∇

∇Zi ← FIO∆ftell Bh
 Zi ← FILE_IO[12] Bh            ⍝ ftell(Bh)
∇

∇Zi ← Ai FIO∆fseek_cur Bh
 Zi ← Ai FILE_IO[13] Bh         ⍝ fseek(Bh, Ai, SEEK_SET)
∇

∇Zi ← Ai FIO∆fseek_set Bh
 Zi ← Ai FILE_IO[14] Bh         ⍝ fseek(Bh, Ai, SEEK_CUR)
∇

∇Zi ← Ai FIO∆fseek_end Bh
 Zi ← Ai FILE_IO[15] Bh         ⍝ fseek(Bh, Ai, SEEK_END)
∇

∇Zi ← FIO∆fflush Bh
 Zi ← FILE_IO[16] Bh            ⍝ fflush(Bh)
∇

∇Zi ← FIO∆fsync Bh
 Zi ← FILE_IO[17] Bh            ⍝ fsync(Bh)
∇

∇Zi ← FIO∆fstat Bh
 Zi ← FILE_IO[18] Bh            ⍝ fstat(Bh)
∇

∇Zi ← FIO∆unlink Bh
 Zi ← FILE_IO[19] Bh            ⍝ unlink(Bc)
∇

∇Zi ← FIO∆mkdir_777 Bh
 Zi ← FILE_IO[20] Bh           ⍝ mkdir(Bc, 0777)
∇

∇Zi ← Ai FIO∆mkdir Bh
 Zi ← Ai FILE_IO[20] Bh         ⍝ mkdir(Bc, Ai)
∇

∇Zi ← FIO∆rmdir Bh
 Zi ← FILE_IO[21] Bh            ⍝ rmdir(Bc)
∇

∇Zi ← FIO∆printf B
 Zi ← B FILE_IO[22] 1           ⍝ printf(A1, A2...) format A1
∇

∇Zi ← A  FIO∆fprintf_stderr B
 Zi ← B  FILE_IO[22] 2          ⍝ fprintf(stderr, A1, A2...) format A1
∇

∇Zi ← A  FIO∆fprintf Bh
 Zi ← A  FILE_IO[22] Bh         ⍝ fprintf(Bh, A1, A2...) format A1
∇

∇Zi ← Ac FIO∆fwrite Bh
 Zi ← Ac FILE_IO[23] Bh         ⍝ fwrite(Ac, 1, ⍴Ac, Bh) Unicode Ac Output UTF-8
∇

∇Zh ← As FIO∆popen Bs
 Zh ← As FILE_IO[24] Bs         ⍝ popen(Bs, As) command Bs mode As
∇

∇Zh ← FIO∆popen_ro Bs
 Zh ← FILE_IO[24] Bs            ⍝ popen(Bs, "r") command Bs
∇

∇Ze ← FIO∆pclose Bh
 Ze ← FILE_IO[25] Bh            ⍝ pclose(Bh)
∇

∇Zs ← FIO∆read_file Bs
 Zs ← FILE_IO[26] Bs            ⍝ return entire file Bs as byte vector
∇

∇Zs ← As FIO∆rename Bs
 Zs ← As FILE_IO[27] Bs         ⍝ rename file As to Bs
∇

∇Zd ← FIO∆read_directory Bs
 Zd ← FILE_IO[28] Bs            ⍝ return content of directory Bs
∇

∇Zn ← FIO∆files_in_directory Bs
 Zn ← FILE_IO[29] Bs            ⍝ return file names in directory Bs
∇

∇Zs ← FIO∆getcwd
 Zs ← FILE_IO 30                ⍝ getcwd()
∇

∇Zn ← FIO∆access Bs
 Zn ← As FILE_IO[31] Bs         ⍝ access(As, Bs) As ∈ 'RWXF'
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

