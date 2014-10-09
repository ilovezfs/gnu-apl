#! /usr/local/bin/apl --script

]log 25
]log 26

  ⍝ tunable parameters for this benchmark program
  ⍝
  DO_PLOT←0             ⍝ do/don't plot the results of start-up cost
  ILRC←1000             ⍝ repeat count for the inner loop of start-up cost
  LEN_PI←1000000        ⍝ vector length for measuring the per-item cost
  PROFILE←4000 2000 50  ⍝ fractions of Integer, Real, and Complex numbers
  CORES←3               ⍝ number of cores used for parallel execution
  TIME_LIMIT←2000       ⍝ time limit per pass (milliseconds)

)COPY 5 FILE_IO

 ⍝ expressions to be benchmarked
 ⍝
∇Z←MON_EXPR
  Z←⍬
  ⍝     A          OP    B          N CN          STAT
  Z←Z,⊂ ''         '+'   'Mix_IRC'  1 'PLUS_B'    35
  Z←Z,⊂ ''         '-'   'Mix_IRC'  1 'MINUS_B'   35
  Z←Z,⊂ ''         '×'   'Mix_IRC'  1 'TIMES_B'   35
  Z←Z,⊂ ''         '÷'   'Mix1_IRC' 1 'DIVIDE_B'  35
  Z←Z,⊂ ''         '∼'   'Bool'     1 'WITHOUT_B' 35
  Z←Z,⊂ ''         '⌈'   'Mix_IR'   1 'RND_UP_B'  35
  Z←Z,⊂ ''         '⌊'   'Mix_IR'   1 'RND_DN_B'  35
  Z←Z,⊂ ''         '!'   'Int2'     1 'BINOM_B'   35
  Z←Z,⊂ ''         '⋆'   'Mix_IRC'  1 'POWER_B'   35
  Z←Z,⊂ ''         '⍟'   'Mix1_IRC' 1 'LOGA_B'    35
  Z←Z,⊂ ''         '○'   'Mix_IRC'  1 'CIRCLE_B'  35
  Z←Z,⊂ ''         '∣'   'Mix_IR'   1 'STILE_B'   35
  Z←Z,⊂ ''         '?'   'Int2'     1 'ROLL_B'    35
∇

∇Z←DYA_EXPR
  Z←⍬
  ⍝     A          OP    B          N CN          STAT
  Z←Z,⊂ 'Mix_IRC'  '+'   'Mix1_IRC' 2 'PLUS_AB'    36
  Z←Z,⊂ 'Mix_IRC'  '-'   'Mix1_IRC' 2 'MINUS_AB'   36
  Z←Z,⊂ 'Mix_IRC'  '×'   'Mix1_IRC' 2 'TIMES_AB'   36
  Z←Z,⊂ 'Mix1_IRC' '÷'   'Mix1_IRC' 2 'DIVIDE_AB'  36
  Z←Z,⊂ 'Bool'     '∧'   'Bool1'    2 'AND_AB'     36
  Z←Z,⊂ 'Bool'     '∨'   'Bool1'    2 'OR_AB'      36
  Z←Z,⊂ 'Bool'     '⍲'   'Bool1'    2 'NAND_AB'    36
  Z←Z,⊂ 'Bool'     '⍱'   'Bool1'    2 'NOR_AB'     36
  Z←Z,⊂ 'Mix_IR'   '⌈'   'Mix_IR'   2 'RND_UP_AB'  36
  Z←Z,⊂ 'Mix_IR'   '⌊'   'Mix_IR'   2 'RND_DN_AB'  36
  Z←Z,⊂ 'Mix_IRC'  '!'   'Mix_IRC'  2 'BINOM_AB'   36
  Z←Z,⊂ 'Mix_IRC'  '⋆'   'Mix_IRC'  2 'POWER_AB'   36
  Z←Z,⊂ 'Mix1_IRC' '⍟'   'Mix1_IRC' 2 'LOGA_AB'    36
  Z←Z,⊂ 'Mix_IR '  '<'   'Mix_IR'   2 'LESS_AB'    36
  Z←Z,⊂ 'Mix_IR '  '≤'   'Mix_IR'   2 'LEQ_AB'     36
  Z←Z,⊂ 'Mix_IRC'  '='   'Mix_IRC'  2 'EQUAL_AB'   36
  Z←Z,⊂ 'Mix_IRC'  '≠'   'Mix_IRC'  2 'UNEQ_AB'    36
  Z←Z,⊂ 'Mix_IR'   '>'   'Mix_IR'   2 'GREATER_AB' 36
  Z←Z,⊂ 'Mix_IR'   '≥'   'Mix_IR'   2 'MEQ_AB'     36
  Z←Z,⊂ '1'        '○'   'Mix_IRC'  2 'CIRCLE_AB'  36
  Z←Z,⊂ 'Mix_IRC'  '∣'   'Mix_IRC'  2 'STILE_AB'   36
  Z←Z,⊂ '1 2 3'    '⋸'   'Int'      2 'FIND_AB'    36
  Z←Z,⊂ 'Mat1_IRC' '+.×' 'Mat1_IRC' 3 'IPROD_AB'   39
  Z←Z,⊂ 'Vec1_IRC' '∘.×' 'Vec1_IRC' 3 'OPROD_AB'   40
∇

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
  Mat1_IRC ← (2⍴⌈LEN⋆0.5)⍴Mix1_IRC
  Vec1_IRC ← (⌈LEN⋆0.5)⍴Mix1_IRC
∇

'libaplplot' ⎕FX  'PLOT'

∇EXPR PLOT_P DATA;PLOTARG
  ⍝⍝
  ⍝⍝ plot data if enabled by DO_PLOT
  ⍝⍝
 →DO_PLOT↓0
  PLOTARG←'xcol 0;'
  PLOTARG←PLOTARG,'xlabel "result length";'
  PLOTARG←PLOTARG,'ylabel "CPU cycles";'
  PLOTARG←PLOTARG,'draw l;'
  PLOTARG←PLOTARG,'plwindow ' , TITLE EXPR
  ⊣ PLOTARG PLOT DATA
  ⍞
∇

∇Z←Average[X] B
 ⍝⍝ return the average of B along axis X
 Z←(+/[X]B) ÷ (⍴B)[X]
∇

∇Z←TITLE EXPR;A;OP;B
  (A OP B)←3↑EXPR
  Z←OP, ' ', B
  →(0=⍴A)/0
  Z←A,' ',Z
∇

∇Z←TITLE1 EXPR;A;OP;B
  (A OP B)←3↑EXPR
  Z←OP, ' B'
  →(0=⍴A)/0
  Z←'A ',Z
∇

∇Z←X LSQRL Y;N;XY;XX;Zb;Za;SX;SXX;SY;SXY
 ⍝⍝ return the least square regression line (a line a + b×N with minimal
 ⍝⍝ distance from samples Y(X))
 N←⍴X
 XY←X×Y ◊ XX←X×X
 SX←+/X ◊ SY←+/Y ◊ SXY←+/XY ◊ SXX←+/XX
 Zb←( (N×SXY) - SX×SY ) ÷ ((N×SXX) - SX×SX)
 Za←(SY - Zb×SX) ÷ N
 Z←Za, Zb
∇

  ⍝ ----------------------------------------------------
  ⍝ Run one pass (one length), return average cycles
  ⍝
∇Z←CCNT ONE_PASS EXPR;OP;STAT;I;ZZ;TH1;TH2;CYCLES;T0;T1
  OP←⊃EXPR[2]
  STAT←EXPR[6]
  ⎕SYL[26;2] ← CCNT
  TH1← 1 FIO∆set_monadic_threshold OP
  TH2← 1 FIO∆set_dyadic_threshold  OP

  I←0
  ZZ←⍬
  T0←24 60 60 1000⊥¯4↑⎕TS
L:
  FIO∆clear_statistics STAT
  Q←⍎TITLE EXPR
  CYCLES←(FIO∆get_statistics STAT)[4]
  ZZ←ZZ,CYCLES
  T1←24 60 60 1000⊥¯4↑⎕TS
  →((I≥2) ∧ TIME_LIMIT<T1-T0)⍴DONE   ⍝ don't let it run too long
  →(ILRC≥I←I+1)/L
DONE:

  ⍝ restore thresholds
  ⊣ TH1 FIO∆set_monadic_threshold OP
  ⊣ TH2 FIO∆set_dyadic_threshold  OP

  ⍝ ignore the first 2 measurements as cache warm-up 
  ⍝
  ZZ←2↓ZZ
⍝ Z←(⍴,Q), ⌊ Average[1]ZZ
  Z←(⍴,Q), ⌊ ⌊⌿ZZ
∇

  ⍝ ----------------------------------------------------
  ⍝ figure start-up times for sequential and parallel execution.
  ⍝ We use small vector sizes for better precision
  ⍝
∇Z←FIGURE_A EXPR;LENGTHS;I;LEN;ZS;ZP;SA;SB;PA;PB;H1;H2;P;TXT
  TXT←78↑'  ===================== ', (TITLE EXPR), '  ', 80⍴ '='
  '' ◊ TXT ◊ ''
  Z←0 3⍴0
  LL←⍴LENGTHS←⌽⍳20 ⍝ outer loop vector lengths
  'Benchmarking start-up cost for ', (TITLE EXPR), ' ...'

  I←1 ◊ ZS←0 2⍴0
LS: INIT_DATA LEN←LENGTHS[I]
  ZS←ZS⍪0 ONE_PASS EXPR
  →(LL≥I←I+1)⍴LS

  I←1 ◊ ZP←0 2⍴0
LP: INIT_DATA LEN←LENGTHS[I]
  ZP←ZP⍪CORES ONE_PASS EXPR
  →(LL≥I←I+1)⍴LP

  (SA SB)←⌊ ZS[;1] LSQRL ZS[;2]
  (PA PB)←⌊ ZP[;1] LSQRL ZP[;2]

  ⍝ print and plot result
  ⍝
  P←ZS,ZP[;2]            ⍝ sequential and parallel cycles
  P←P,(SA+ZS[;1]×SB)     ⍝ sequential least square regression line
  P←P,(PA+ZP[;1]×PB)     ⍝ parallel least square regression line
  H1←'Length' '  Sequ Cycles' '  Para Cycles' '  Linear Sequ' 'Linear Para'
  H2←'======' '  ===========' '  ===========' '  ===========' '==========='
  H1⍪H2⍪P

  ''
  'regression line sequential:     ', (¯8↑⍕SA), ' + ', (⍕SB),'×N cycles'
  'regression line parallel:       ', (¯8↑⍕PA), ' + ', (⍕PB),'×N cycles'

  ⍝ xdomain of aplplot seems not to work for xy plots - create a dummy x=0 line
  P←(0, SA, PA, SA, PA)⍪P

  EXPR PLOT_P P
  Z←SA,PA
∇

  ⍝ ----------------------------------------------------
  ⍝ figure per-item times for sequential and parallel execution.
  ⍝ We use one LARGE vector
  ⍝
∇Z←SUP_A FIGURE_B EXPR;SOFF;POFF;SCYC;PCYC;LEN
  (SOFF POFF)←SUP_A
  'Benchmarking per-item cost for ', (TITLE EXPR), ' ...'
  SUMMARY←SUMMARY,⊂'-------------- ', (TITLE EXPR), ' -------------- '
  SUMMARY←SUMMARY,⊂'average sequential startup cost:', (¯8↑⍕⌈SOFF), ' cycles'
  SUMMARY←SUMMARY,⊂'average parallel startup cost:  ', (¯8↑⍕⌈POFF), ' cycles'

  INIT_DATA LEN_PI
  (LEN SCYC)←0     ONE_PASS EXPR
  (LEN PCYC)←CORES ONE_PASS EXPR
  Z←⊂TITLE EXPR
  Z←Z, ⌈ (SCYC - SOFF) ÷ LEN
  Z←Z, ⌈ (PCYC - POFF) ÷ LEN
  TS←'per item cost sequential:       ',(¯8↑⍕Z[2]), ' cycles'
  TP←'per item cost parallel:         ',(¯8↑⍕Z[3]), ' cycles'
  SUMMARY←SUMMARY,(⊂TS),(⊂TP)

  SUP_A BREAK_EVEN (⊂EXPR),Z
∇

∇SUP BREAK_EVEN PERI;EXPR;OP;ICS;ICP;SUPS;SUPP;T1;T2;BE;FPA
  (SUPS SUPP)←SUP   ⍝ start-up cost
  (EXPR OP ICS ICP)←PERI ⍝ per-item cost
  T1←'parallel break-even length:     '
  T2←'     not reached' ◊ T3←'0x7000000000000000ULL'
  →(ICP ≥ ICS)⍴1+↑⎕LC ◊ T2←¯8↑BE←⍕⌈ (SUPP - SUPS) ÷ ICS - ICP ◊ T3←21↑BE
  SUMMARY←SUMMARY,(⊂T1,T2),⊂''

  FPA←(⊂"perfo_%d(%s, \"%s\", %s)\n"), EXPR[4]   ⍝ %d
  FPA←FPA,(EXPR[5]),(⊂TITLE1 EXPR),⊂T3
  FPA FIO∆fprintf TH_FILE
∇

  ⍝ ----------------------------------------------------

∇GO;DYA_A;MON_A;SUMMARY;TH_FILE
  CORES←CORES ⌊ ⎕SYL[25;2]
  'Running ScalarBenchmark_2 with' CORES 'cores...'

  ⍝ check that the core count can be set
  ⍝
  ⎕SYL[26;2] ← CORES
  →(CORES = ⎕SYL[26;2])⍴CORES_OK
  '*** CPU core count could not be set!'
  '*** This is usually a configuration or platform problem.'
  '***'
  '***  try "make parallel1" in the top-level directory'
  '***'
  '*** the relevant ./configure options (used by make parallel1) are:'
  '***      PERFORMANCE_COUNTERS_WANTED=yes'
  '***      CORE_COUNT_WANTED=SYL'
  '***'
  '*** NOTE: parallel GNU APL currently requires linux and a recent Intel CPU'
  '***'
  →0
  
CORES_OK:
  ⎕SYL[26;2] ← 0

  ⍝ figure start-up costs
  ⍝
  MON_A←Average[1] ⊃ FIGURE_A ¨ MON_EXPR
  DYA_A←Average[1] ⊃ FIGURE_A ¨ DYA_EXPR

  ⍝ figure per-item costs. we can do that only after computing MON_A/DYA_A
  ⍝
  ''
  SUMMARY←0⍴''
  TH_FILE←"w" FIO∆fopen "parallel_thresholds"

  (⊂"\n") FIO∆fprintf TH_FILE

  ⊣ (⊂MON_A) FIGURE_B ¨ MON_EXPR

  (⊂"\n") FIO∆fprintf TH_FILE

  ⊣ (⊂DYA_A) FIGURE_B ¨ DYA_EXPR

  (⊂"\n") FIO∆fprintf TH_FILE
  (⊂"#undef perfo_1\n") FIO∆fprintf TH_FILE
  (⊂"#undef perfo_2\n") FIO∆fprintf TH_FILE
  (⊂"#undef perfo_3\n") FIO∆fprintf TH_FILE

   FIO∆fclose TH_FILE

 ''
 78↑' ============================  SUMMARY  ',80⍴'='
 ''
  ⊣ { ⎕←⍵ }¨SUMMARY
∇


  GO

  )OFF

