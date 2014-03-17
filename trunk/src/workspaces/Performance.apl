#!./apl --script --

 ⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝
⍝ Performance 2014-03-17 14:05:38 (GMT+1)
⍝
 ⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝

∇T←T0 DELTA EXPR;T2
 (MS CNT)←RUN_PERF EXPR ◊ T←MS÷CNT
 T←T2←0⌈T-T0 ◊ UNIT←' ms'
 →1+(T2≥1)⍴⎕LC ◊ T2←T2×1000 ◊  UNIT←' μs'
 →1+(T2≥1)⍴⎕LC ◊ T2←T2×1000 ◊  UNIT←' ns'
 TOTAL←TOTAL⍪((8 2⍕T2),UNIT) EXPR (8 0⍕CNT)
∇

∇TOTAL←TPP GO EXPRS;I_10_10;I_1000;R_1000; T0;T1
 ⍝ Arguments for benchmarked functions
 ⍝
 I_10_10 ←? 10 10⍴10
 I_1000  ←?  1000⍴10
 R_1000  ← I_1000+.1
 TOTAL←1 3⍴'    Time   ' 'Operation' '     CNT'
 TOTAL←TOTAL,[1] '-----------' '---------' '--------'
 T0←0.0 DELTA ''
 0⍴T0 DELTA¨EXPRS
 TOTAL←TOTAL,[1] '-----------' '---------' '--------'
∇

∇MAKE_FUT EXPR;F;TXT
 ⍝
 ⍝ ⎕FX a function that executes EXPR CNT times and returns
 ⍝ the number of milliseconds spent for doing so, and CNT
 ⍝
 TXT←     ⊂ 'MS_CNT←FUT CNT;N;Z;MS'
 TXT←TXT, ⊂ 'N←0 ◊ CNT←5×⌈CNT÷5 ◊ MS←⎕TS'
 TXT←TXT, ⊂ 'LOOP:'
 TXT←TXT, ⊂ (0<⍴,EXPR)/'Z←',EXPR
 TXT←TXT, ⊂ (0<⍴,EXPR)/'Z←',EXPR
 TXT←TXT, ⊂ (0<⍴,EXPR)/'Z←',EXPR
 TXT←TXT, ⊂ (0<⍴,EXPR)/'Z←',EXPR
 TXT←TXT, ⊂ (0<⍴,EXPR)/'Z←',EXPR
 TXT←TXT, ⊂ '→(CNT>N←N+5)⍴LOOP'
 TXT←TXT, ⊂ 'MS←1 12 30 24 60 60 1000⊥⎕TS-MS'
 TXT←TXT, ⊂ 'MS_CNT←MS,CNT'
 F←⎕FX TXT
∇

∇MS_CNT←RUN_PERF EXPR;MS;CNT;N1;N2
 ⍝
 ⍝ return the total time (in ms) to execute EXPR
 ⍝
 ⍝ 1. create FUT that runs and measures EXPR N times.
 ⍝
 MAKE_FUT EXPR
 ⍝
 ⍝ 2. compute N1 so that FUT N1 takes > 100 ms.
 ⍝
 N1←1
 N1←N1×3 ◊ (MS CNT)←FUT N1 ◊ →(MS < 100)⍴⎕LC
 ⍝
 ⍝ 3. compute N2 so that FUT N takes about TPP seconds
 ⍝
 N2←⌈TPP×CNT÷MS
 ⍝
 ⍝ 4. execute FUT N
 ⍝
 MS_CNT←(MS CNT)←FUT N2
 'Running  ''' (30↑EXPR) ''' ' (8 0⍕CNT) ' times: ' MS 'ms'
∇

∇LEN PLOT EXPR;Filename;Handle;LEN1;FORMAT;TIME
 'lib_file_io.so' ⎕FX 'FILE_IO'
 Filename←'performamce_data'
 Handle←'w' FILE_IO[3] Filename

 TPP←2000
LOOP: →(0=⍴LEN)⍴DONE
 LEN1←''⍴(↑LEN) ◊ LEN←1↓LEN
 VEC←⍳LEN1
 (MS CNT)←RUN_PERF EXPR
 FORMAT←"%d, %6.2f\n"
 TIME_uS←1000×MS÷CNT
 FORMAT LEN1 TIME_uS FILE_IO[22] Handle
 →LOOP
DONE: FILE_IO[4] Handle   ⍝ close plot data file 
∇

      LEN←1,100×⍳5
      LEN PLOT "VEC + VEC"
      )HOST gnuplot < workspaces/Performance.gnuplot
      )OFF

