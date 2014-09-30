#! /usr/local/bin/apl --script

  ⍝ tunable parameters for this benchmark program
  ⍝
  DO_PLOT←1             ⍝ plot the results of start-up cost ?
  ILRC←1000             ⍝ inner loop (of start-up cost) repeat count
  LEN_PI←100000         ⍝ vector length for measuring the per-item cost
  PROFILE←4000 2000 50  ⍝ fractions of Integer, Real, and Complex numbers
  CORES←3               ⍝ number of cores used for parallel execution

'Running ScalarBenchmark_2'

)COPY 5 FILE_IO

'libaplplot' ⎕FX  'PLOT'

∇EXPR PLOT_P DATA;PLOTARG
 →DO_PLOT↓0
 PLOTARG←'xcol 0;draw l;plwindow ' , EXPR
  ⊣ PLOTARG PLOT DATA
  ⍞
∇

 ⍝ expressions to be benchmarked
 ⍝
MON_EXPR←⎕INP 'END-OF-EXPR'
+ Mix_IRC:PLUS_B
- Mix_IRC:MINUS_B
× Mix_IRC:TIMES_B
÷ Mix1_IRC:DIVIDE_B
∼ Bool:WITHOUT_B
⌈ Mix_IR:RND_UP_B
⌊ Mix_IR:RND_DN_B
! Int2:BINOM_B
⋆ Mix_IRC:POWER_B
⍟ Mix1_IRC:LOGA_B
○ Mix_IRC:CIRCLE_B
∣ Mix_IR:STILE_B
? Int2:ROLL_B
END-OF-EXPR

DYA_EXPR←⎕INP 'END-OF-EXPR'
Mix_IRC + Mix_IRC:PLUS_AB
Mix_IRC - Mix_IRC:MINUS_AB
Mix_IRC × Mix_IRC:TIMES_AB
Mix1_IRC ÷ Mix1_IRC:DIVIDE_AB
Bool ∧ Bool1:AND_AB
Bool ∨ Bool1:OR_AB
Bool ⍲ Bool1:NAND_AB
Bool ⍱ Bool1:NOR_AB
Mix_IR ⌈ Mix_IR:RND_UP_AB
Mix_IR ⌊ Mix_IR:RND_DN_AB
Mix_IRC ! Mix_IRC:BINOM_AB
Mix_IRC ⋆ Mix_IRC:POWER_AB
Mix1_IRC ⍟ Mix1_IRC:LOGA_AB
Mix_IR < Mix_IR:LESS_AB
Mix_IR ≤ Mix_IR:LEQ_AB
Mix_IRC = Mix_IRC:EQUAL_AB
Mix_IRC ≠ Mix_IRC:UNEQ_AB
Mix_IR > Mix_IR:GREATER_AB
Mix_IR ≥ Mix_IR:MEQ_AB
1 ○ Mix_IRC:CIRCLE_AB
Mix_IRC  ∣ Mix_IRC:STILE_AB
1 2 3 ⋸ Int:FIND_AB
END-OF-EXPR

∇INIT_DATA LEN;N;Ilen;Rlen;Clen
  ⍝⍝
  ⍝⍝ setup variables used in benchmark expressions:
  ⍝⍝ Int:  ¯2 ... 9
  ⍝⍝ Int1: nonzero Int
  ⍝⍝ Real: ¯10 to 10 or so
  ⍝⍝
  (Ilen Rlen Clen)←PROFILE
  Int  ← 10 - ? Ilen ⍴ 12
  Int1 ← Ilen ⍴ (Int≠0)/Int
  Int2 ← Ilen ⍴ (Int>0) / Int
  Bool ← 2 ∣ Int
  Bool1← 1 ⌽ Bool
  Real ← Rlen ⍴ Int + 3 ÷ ○1
  Real1← Rlen ⍴ (Real≠0)/Real
  Real2← Rlen ⍴ (Real>0)/Real
  Comp ← Clen ⍴ Real + 0J1×1⌽Real
  Comp1← Clen ⍴ (Comp≠0)/Comp

  Mix_IR   ←Int,Real         ◊ Mix_IR   [N?N←⍴Mix_IR  ] ← Mix_IR
  Mix_IRC  ←Int,Real,Comp    ◊ Mix_IRC  [N?N←⍴Mix_IRC ] ← Mix_IRC
  Mix1_IRC ←Int1,Real1,Comp1 ◊ Mix1_IRC [N?N←⍴Mix1_IRC] ← Mix1_IRC

  Int      ← LEN ⍴ Int
  Int1     ← LEN ⍴ Int1
  Int2     ← LEN ⍴ Int2
  Bool     ← LEN ⍴ Bool
  Bool1    ← LEN ⍴ Bool1
  Real     ← LEN ⍴ Real
  Real1    ← LEN ⍴ Real1
  Real2    ← LEN ⍴ Real2
  Comp     ← LEN ⍴ Comp
  Comp1    ← LEN ⍴ Comp1
  Mix_IR   ← LEN ⍴ Mix_IR
  Mix_IRC  ← LEN ⍴ Mix_IRC
  Mix1_IRC ← LEN ⍴ Mix1_IRC
∇

 ⍝ Int
 ⍝ Real
 ⍝ Comp
 ⍝ Comp1
 ⍝ Bool

∇Z←ACHARS EXPR
 Z←EXPR∈' 0123456789_∆⍙', ⎕UCS (64+⍳26), 96+⍳26
∇

∇Z←Average[X] B
 Z←(+/[X]B) ÷ (⍴B)[X]
∇

∇Z←IS_DYADIC EXPR
 Z←ACHARS EXPR[1]
∇

∇Z←X LSQRL Y;N;XY;XX;b;a;SX;SXX;SY;SXY
 N←⍴X
 XY←X×Y ◊ XX←X×X
 SX←+/X ◊ SY←+/Y ◊ SXY←+/XY ◊ SXX←+/XX
 b←( (N×SXY) - SX×SY ) ÷ ((N×SXX) - SX×SX )
 a←(SY - b×SX) ÷ N
 Z←a,b
∇

  ⍝ ----------------------------------------------------
  ⍝ Run one pass (one length), return average cycles
  ⍝
∇Z←A ONE_PASS EXPR;RL;STAT;IDX;ZZ;OP;TH1;TH2;CYCLES;CCNT
  (CCNT RL LEN)←A
  ⎕SYL[26;2] ← CCNT
  STAT←35 + IS_DYADIC EXPR
  ⎕RL←RL ◊ INIT_DATA LEN
  OP←EXPR[(ACHARS EXPR)⍳0]
  TH1← 1 FIO∆set_monadic_threshold OP
  TH2← 1 FIO∆set_dyadic_threshold OP

  IDX←0
  ZZ←⍬
L:
  FIO∆clear_statistics STAT
  ⊣⍎EXPR
  CYCLES←(FIO∆get_statistics STAT)[4]
  ZZ←ZZ,CYCLES
  →(ILRC≥IDX←IDX+1)/L

  ⊣ TH1 FIO∆set_monadic_threshold OP
  ⊣ TH2 FIO∆set_dyadic_threshold OP

  ⍝ ignore the first 2 measurements as cache warm-up 
  ⍝
  ZZ←2↓ZZ
  Z←⌊Average[1]ZZ
⍝ Z←⌊⌊⌿ZZ
∇

  ⍝ ----------------------------------------------------
  ⍝ figure start-up times for sequential and parallel execution.
  ⍝ We use small vector sizes for better precision
  ⍝
∇Z←FIGURE_A EXPR;RL;LENGTHS;IDX;LEN;ZZ;SA;SB;PA;PB;TT;TITLE;H1;H2;P;OP
  TT←EXPR⍳':' ◊ TITLE←TT↓EXPR ◊ EXPR←(TT-1)↑EXPR
''
78↑'  ===================== Benchmarking ', EXPR , '  ', 80⍴ '='
''
  RL←⎕RL
  Z←0 3⍴0
  LENGTHS←⌽2+⍳20 ⍝ outer loop vector lengths

  IDX←1
L:LEN←LENGTHS[IDX]
  ZZ←LEN, 0, 0
  OP←EXPR[(ACHARS EXPR)⍳0]

  ZZ[2]← (0,     RL, LEN) ONE_PASS EXPR
  ZZ[3]← (CORES, RL, LEN) ONE_PASS EXPR

  Z←ZZ⍪Z
  IDX←IDX+1 ◊ →(IDX≤⍴LENGTHS)⍴L

  (SA SB)←⌊ Z[;1] LSQRL Z[;2]
  (PA PB)←⌊ Z[;1] LSQRL Z[;3]

  ⍝ print and plot result
  ⍝
  P←Z                    ⍝ sequential and parallel cycles
  P←P,(SA+Z[;1]×SB)      ⍝ sequential least square regression line
  P←P,(PA+Z[;1]×PB)      ⍝ parallel least square regression line
  H1←'Length' '  Sequ Cycles' '  Para Cycles' '  Linear Sequ' 'Linear Para'
  H2←'======' '  ===========' '  ===========' '  ===========' '==========='
  H1⍪H2⍪P
  EXPR PLOT_P P

  ''
  'Regression lines:    sequential: ' SA '+' SB 'N,    parallel: ' PA '+' PB 'N'
  ''
  Z←SA,PA
∇

  ⍝ ----------------------------------------------------
  ⍝ figure per-item times for sequential and parallel execution.
  ⍝ We use one LARGE vector
  ⍝
∇Z←A FIGURE_B EXPR;RL;TT;LEN;SOFF;POFF;L
  (LEN SOFF POFF)←A
  TT←EXPR⍳':' ◊ EXPR←(TT-1)↑EXPR
  RL←⎕RL

  L←'per item cycles for:  ', 20↑EXPR
  Z←⊂EXPR
  Z←Z, ⌈ (((0,     RL, LEN) ONE_PASS EXPR) - SOFF) ÷ LEN
  Z←Z, ⌈ (((CORES, RL, LEN) ONE_PASS EXPR) - POFF) ÷ LEN
  L 'sequential' (¯5↑⍕ Z[2]) '  parallel' (¯5↑⍕ Z[3])
∇

∇A BREAK_EVEN B;OP;ICS;ICP;SUPS;SUPP;BE;T1
  (OP ICS ICP)←A
  (SUPS SUPP)←B
  T1←43↑'Break-even length for:  ',OP,': '
  →(ICP ≥ ICS)⍴1+↑⎕LC ◊ T1 'not reached' ◊ →0
  T1 (⌈ (SUPP - SUPS) ÷ ICS - ICP)
∇
  ⍝ ----------------------------------------------------

∇GO;DYA_A;DYA_B;MON_A;MON_B
  ⍝ measure start-up costs
  ⍝
  DYA_A ← Average[1] ⊃ FIGURE_A ¨ DYA_EXPR
  MON_A ← Average[1] ⊃ FIGURE_A ¨ MON_EXPR

  ⍝ measure per-item costs
  ⍝
  DYA_B ← (⊂LEN_PI,DYA_A) FIGURE_B ¨ DYA_EXPR
  MON_B ← (⊂LEN_PI,MON_A) FIGURE_B ¨ MON_EXPR

''
78↑80⍴'='
78↑' ====================  SUMMARY  ',80⍴'='
78↑80⍴'='
''
'dyadic  extra startup cost for parallel: ' , (⍕⌊ DYA_A[2] - DYA_A[1]) , ' CPU cycles'
'dyadic  cost per element: '
⊃DYA_B
''
'monadic extra startup cost for parallel: ' , (⍕⌊ MON_A[2] - MON_A[1]) , ' CPU cycles'
'monadic cost per element: '
''
⊃MON_B
''
78↑' ====================  BREAK-EVEN POINTS ',80⍴'='
''

  DYA_B BREAK_EVEN ¨ ⊂DYA_A
  MON_B BREAK_EVEN ¨ ⊂MON_A
∇


  GO
  )OFF

