#! /usr/local/bin/apl --script

'Running ScalarBenchmark'

 ⍝ expressions to be benchmarked
 ⍝
EXPR←⎕INP 'END-OF-EXPR'
I+I
R+R
C+C
I-I
1○R
1○C
END-OF-EXPR

⍝ fix FILE_IO to get probes
⍝
'lib_file_io' ⎕FX 'FILE_IO'

⍝ setup some variables used in benchmark expressions:
⍝ I - Integer, F - Float, and C - Complex
⍝
LEN←20
I←10 - ?LEN⍴12
R←10 - (3÷○1)×?LEN⍴20
C←10 - (3÷○1)×?LEN⍴20
C←C +  (0J3÷○1)×10 - ?LEN⍴20

 ⍝ I
 ⍝ R
 ⍝ C

 ⍝ GNUPLOT the execution times of one expression
∇TITLE GNUPLOT TIMES
 'GNUPLOTTING ', TITLE
∇

⍝ benchmark one expression
⍝
∇RUN B;TIME;T1;Tn;AVE;DEV
 0⍴¯1 FILE_IO ¯3
 0⍴⍎B
 TIME←¯1 FILE_IO ¯3  ⍝ get execution times of several passes

 ⍝ the first time is always too high (cache ?) so we show that value separately
 ⍝ and the average and std. deviation of the rest.
 T1←↑TIME ◊ Tn←1↓TIME
 AVE←(+/Tn)÷⍴Tn
 DEV←Tn-AVE ◊ DEV←DEV×DEV ◊ DEV←(+/DEV)÷⍴Tn ◊ DEV←DEV⋆.5 ⍝ Std. deviation
 B, ':   1st= ', (6 0⍕T1), '   μ=' , (6 0⍕⌈AVE+.5) , '   σ=' , 8 2⍕DEV

 ⍝ start GNUplot
 ⍝
 B GNUPLOT TIME
∇

⍝ benchmark all expressions
⍝
RUN¨EXPR 

  ⍝ done
)OFF


