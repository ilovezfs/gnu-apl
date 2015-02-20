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
 Zh ← As FILE_IO[3] Bs
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

∇Zi ← Ac FIO∆fwrite_utf8 Bh
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

∇Zh ← FIO∆socket Bi
 ⍝⍝ socket(Bi). Bi is domain, type, protocol, e.g.
 ⍝⍝ Zh ← FIO∆socket 2 1 0  for an IPv4 TCP socket
 Zh ← FILE_IO[32] Bi
∇

∇Zi ← Ai FIO∆bind Bh
 ⍝⍝ bind(Bh, Ai). Ai is domain, local IPv4-address, local port, e.g.
 ⍝⍝ 2 (256⊥127 0 0 1) 80 bind Bh binds socket Bh to port 80 on
 ⍝⍝ localhost 127.0.0.1 (web server)
 Zi ← Ai FILE_IO[33] Bh
∇

∇Zi ← Ai FIO∆listen Bh
 ⍝⍝ listen(Bh, Ai).
 Zi ← Ai FILE_IO[34] Bh
∇

∇Zh ← FIO∆accept Bh
 ⍝⍝ accept(Bh).
 ⍝⍝ Return errno or 4 integers: handle, domain, remote IPv4-address, remote port
 Zh ← FILE_IO[35] Bh
∇

∇Zh ← Aa FIO∆connect Bh
 ⍝⍝ connect(Bh, Aa). Aa is domain, remote IPv4-address, remote port, e.g.
 ⍝⍝ 2 (256⊥127 0 0 1) 80 connects to port 80 on localhost 127.0.0.1 (web server)
 Zh ← Aa FILE_IO[36] Bh
∇

∇Zi ← Ai FIO∆recv Bh
 ⍝⍝ return (at most) Ai bytes from socket Bh
 Zi←Ai FILE_IO[37] Bh
∇

∇Zi ← Ai FIO∆send Bh
 ⍝⍝ send bytes Ai on socket Bh
 Zi←Ai FILE_IO[38] Bh
∇

∇Zi ← Ai FIO∆send_utf8 Bh
 ⍝⍝ send Unicode characters Ai on socket Bh
 Zi←Ai FILE_IO[39] Bh
∇

∇Z ← FIO∆select B
 ⍝⍝ perform select() on handles in B
 ⍝⍝ B = Br [Bw [Be [Bt]]] is a (nested) vector of 1, 2, 3, or 4 items:
 ⍝⍝ Bt: timeout in milliseconds
 ⍝⍝ Be: handles with exceptions
 ⍝⍝ Bw: handles ready for writing
 ⍝⍝ Br: handles ready for reading
 ⍝⍝
 ⍝⍝ Z = Zc Zr Zw Ze Zt is a (nested) 5-item vector:
 ⍝⍝ Zc: number of handles in Zr, Zw, and Ze, or -errno
 ⍝⍝ Zr, Zw, Ze: handles ready for reading, writing, and exceptions respectively
 ⍝⍝ Zt: timeout remaining
 Z←FILE_IO[40] B
∇

∇Zi ← Ai FIO∆read Bh
 ⍝⍝ read (at most) Ai bytes from file descriptor Bh
 Zi←Ai FILE_IO[41] Bh
∇

∇Zi ← Ai FIO∆write Bh
 ⍝⍝ write bytes Ai on file descriptor Bh
 Zi←Ai FILE_IO[42] Bh
∇

∇Zi ← Ai FIO∆write_utf8 Bh
 ⍝⍝ write Unicode characters Ai on file descriptor Bh
 Zi←Ai FILE_IO[43] Bh
∇

∇Za ← FIO∆getsockname Bh
 ⍝⍝ get socket address (address family, ip address, port)
 Zi←FILE_IO[44] Bh
∇

∇Za ← FIO∆getpeername Bh
 ⍝⍝ get socket address (address family, ip address, port) of peer
 Zi←FILE_IO[45] Bh
∇

∇Zi ← Ai FIO∆getsockopt Bh
 ⍝⍝ get socket option from socket Bh
 ⍝⍝ Ai is socket-level, option-name
 Zi←Ai FILE_IO[46] Bh
∇

∇Zi ← Ai FIO∆setsockopt Bh
 ⍝⍝ set socket option from socket Bh
 ⍝⍝ Ai is socket-level, option-name, option-value
 Zi←Ai FILE_IO[47] Bh
∇

∇FIO∆clear_statistics Bi
 ⍝⍝ clear performance statistics with ID Bi
 Zn ← FILE_IO[200] Bi
∇

∇Z ← FIO∆get_statistics Bi
 ⍝⍝ read performance statistics with ID Bi
 ⍝⍝ Z[1] statistics type
 ⍝⍝ Z[2] statistics name
 ⍝⍝ Z[3 4 5] first pass (or only statistics)
 ⍝⍝ Z[6 7 8] optional subsequent pass statistics
 Z ← FILE_IO[201] Bi
∇

∇Z ← FIO∆get_monadic_threshold Bc
 ⍝⍝ read the monadic parallel execution threshold for primitive Bc
 Z ← FILE_IO[202] Bc
∇

∇Z ← Ai FIO∆set_monadic_threshold Bc
 ⍝⍝ set the monadic parallel execution threshold for primitive Bc
 Z ← Ai FILE_IO[202] Bc
∇

∇Z ← FIO∆get_dyadic_threshold Bc
 ⍝⍝ read the dyadic parallel execution threshold for primitive Bc
 Z ← FILE_IO[203] Bc
∇

∇Z ← Ai FIO∆set_dyadic_threshold Bc
 ⍝⍝ set the dyadic parallel execution threshold for primitive Bc
 Z ← Ai FILE_IO[203] Bc
∇

⍝ meta data for this library...
⍝
∇Z←FIO⍙metadata
 Z←0 2⍴⍬
 Z←Z⍪'Author'      'Jürgen Sauermann'
 Z←Z⍪'BugEmail'    'bug-apl@gnu.org'
 Z←Z⍪'Documentation' ''
 Z←Z⍪'Download'      'http://svn.savannah.gnu.org/viewvc/trunk/wslib5/Makefile.in?root=apl'
 Z←Z⍪'License'       'LGPL'
 Z←Z⍪'Portability'   'L3'
 Z←Z⍪'Provides'      'file-io'
 Z←Z⍪'Requires'      ''
 Z←Z⍪'Version'       '1.0'
∇

