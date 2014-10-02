#! /usr/local/bin/apl --script

  ⍝ tunable parameters for this benchmark program
  ⍝
  DO_PLOT←0             ⍝ do/don't plot the results of start-up cost
  ILRC←100              ⍝ repeat count for the inner loop of start-up cost
  LEN_PI←10000          ⍝ vector length for measuring the per-item cost
  PROFILE←4000 2000 50  ⍝ fractions of Integer, Real, and Complex numbers
  CORES←3               ⍝ number of cores used for parallel execution

'Running ScalarBenchmark_2'

]log 25
]log 26

)COPY 5 FILE_IO

'libaplplot' ⎕FX  'PLOT'

∇EXPR PLOT_P DATA;PLOTARG
  ⍝⍝
  ⍝⍝ plot data if enabled by DO_PLOT
  ⍝⍝
 →DO_PLOT↓0
 PLOTARG←'xcol 0;draw l;plwindow ' , EXPR
  ⊣ PLOTARG PLOT DATA
  ⍞
∇

 ⍝ expressions to be benchmarked
 ⍝
∇Z←MON_EXPR
  Z←⍬
⍝ Z←Z,⊂ '' '+' , 'Mix_IRC'  'PLUS_B'    35
⍝ Z←Z,⊂ '' '-' , 'Mix_IRC'  'MINUS_B'   35
⍝ Z←Z,⊂ '' '×' , 'Mix_IRC'  'TIMES_B'   35
⍝ Z←Z,⊂ '' '÷' , 'Mix1_IRC' 'DIVIDE_B'  35
⍝ Z←Z,⊂ '' '∼' , 'Bool'     'WITHOUT_B' 35
⍝ Z←Z,⊂ '' '⌈' , 'Mix_IR'   'RND_UP_B'  35
⍝ Z←Z,⊂ '' '⌊' , 'Mix_IR'   'RND_DN_B'  35
⍝ Z←Z,⊂ '' '!' , 'Int2'     'BINOM_B'   35
⍝ Z←Z,⊂ '' '⋆' , 'Mix_IRC'  'POWER_B'   35
⍝ Z←Z,⊂ '' '⍟' , 'Mix1_IRC' 'LOGA_B'    35
⍝ Z←Z,⊂ '' '○' , 'Mix_IRC'  'CIRCLE_B'  35
⍝ Z←Z,⊂ '' '∣' , 'Mix_IR'   'STILE_B'   35
⍝ Z←Z,⊂ '' '?' , 'Int2'     'ROLL_B'    35
∇

∇Z←DYA_EXPR
  Z←⍬
  Z←Z,⊂ 'Mix_IRC'  '+'   'Mix_IRC'  'PLUS_AB'    36
  Z←Z,⊂ 'Mix_IRC'  '+'   'Mix1_IRC' 'PLUS_AB'    36
⍝ Z←Z,⊂ 'Mix_IRC'  '-'   'Mix_IRC'  'MINUS_AB'   36
⍝ Z←Z,⊂ 'Mix_IRC'  '×'   'Mix_IRC'  'TIMES_AB'   36
⍝ Z←Z,⊂ 'Mix1_IRC' '÷'   'Mix1_IRC' 'DIVIDE_AB'  36
⍝ Z←Z,⊂ 'Bool'     '∧'   'Bool1'    'AND_AB'     36
⍝ Z←Z,⊂ 'Bool'     '∨'   'Bool1'    'OR_AB'      36
⍝ Z←Z,⊂ 'Bool'     '⍲'   'Bool1'    'NAND_AB'    36
⍝ Z←Z,⊂ 'Bool'     '⍱'   'Bool1'    'NOR_AB'     36
⍝ Z←Z,⊂ 'Mix_IR'   '⌈'   'Mix_IR'   'RND_UP_AB'  36
⍝ Z←Z,⊂ 'Mix_IR'   '⌊'   'Mix_IR'   'RND_DN_AB'  36
⍝ Z←Z,⊂ 'Mix_IRC'  '!'   'Mix_IRC'  'BINOM_AB'   36
⍝ Z←Z,⊂ 'Mix_IRC'  '⋆'   'Mix_IRC'  'POWER_AB'   36
⍝ Z←Z,⊂ 'Mix1_IRC' '⍟'   'Mix1_IRC' 'LOGA_AB'    36
⍝ Z←Z,⊂ 'Mix_IR '  '<'   'Mix_IR'   'LESS_AB'    36
⍝ Z←Z,⊂ 'Mix_IR '  '≤'   'Mix_IR'   'LEQ_AB'     36
⍝ Z←Z,⊂ 'Mix_IRC'  '='   'Mix_IRC'  'EQUAL_AB'   36
⍝ Z←Z,⊂ 'Mix_IRC'  '≠'   'Mix_IRC'  'UNEQ_AB'    36
⍝ Z←Z,⊂ 'Mix_IR'   '>'   'Mix_IR'   'GREATER_AB' 36
⍝ Z←Z,⊂ 'Mix_IR'   '≥'   'Mix_IR'   'MEQ_AB'     36
⍝ Z←Z,⊂ '1'        '○'   'Mix_IRC'  'CIRCLE_AB'  36
⍝ Z←Z,⊂ 'Mix_IRC'  '∣'   'Mix_IRC'  'STILE_AB'   36
⍝ Z←Z,⊂ '1 2 3'    '⋸'   'Int'      'FIND_AB'    36
⍝ Z←Z,⊂ 'Mat_IRC'  '+.×' 'Mat_IRC'  'IPROD_AB'   39
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
  Mat_IRC  ← (2⍴⌈LEN⋆0.5)⍴Mix1_IRC
∇

∇Z←Average[X] B
 ⍝⍝ return the average of B along axis X
 Z←(+/[X]B) ÷ (⍴B)[X]
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
∇Z←CCNT ONE_PASS EXPR;AA;OP;BB;CNAM;STAT;IDX;ZZ;TH1;TH2;CYCLES
  (AA OP BB CNAM STAT)←EXPR
  ⎕SYL[26;2] ← CCNT
  TH1← 1 FIO∆set_monadic_threshold OP
  TH2← 1 FIO∆set_dyadic_threshold  OP

  IDX←0
  ZZ←⍬
L:
  FIO∆clear_statistics STAT
  Q←⍎AA,' ',OP,' ',BB
  CYCLES←(FIO∆get_statistics STAT)[4]
  ZZ←ZZ,CYCLES
  →(ILRC≥IDX←IDX+1)/L

  ⊣ TH1 FIO∆set_monadic_threshold OP
  ⊣ TH2 FIO∆set_dyadic_threshold  OP

  ⍝ ignore the first 2 measurements as cache warm-up 
  ⍝
  ZZ←2↓ZZ
  Z←⌊ Average[1]ZZ
⍝ Z←⌊ ⌊⌿ZZ
  Z←Z,⍴,Q
∇

  ⍝ ----------------------------------------------------
  ⍝ figure start-up times for sequential and parallel execution.
  ⍝ We use small vector sizes for better precision
  ⍝
∇Z←FIGURE_A EXPR;AA;OP;BB;CNAM;STAT;LENGTHS;IDX;LEN;ZZ;SA;SB;PA;PB;H1;H2;P;TXT
  (AA OP BB CNAM STAT)←EXPR
  TXT←78↑'  ===================== ', AA, ' ', OP, ' ', BB , '  ', 80⍴ '='
  '' ◊ TXT ◊ ''
  Z←0 3⍴0
  LENGTHS←⌽2+⍳20 ⍝ outer loop vector lengths

  IDX←1
L:LEN←LENGTHS[IDX]
  INIT_DATA LEN
  ZZ←LEN, 0, 0

  ZZ[2 1]← 0     ONE_PASS EXPR
  ZZ[3 1]← CORES ONE_PASS EXPR

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

  Z←SA,PA

  ''
  'regression line sequential:     ', (¯8↑⍕SA), ' + ', (⍕SB),'×N cycles'
  'regression line parallel:       ', (¯8↑⍕PA), ' + ', (⍕PB),'×N cycles'
∇

  ⍝ ----------------------------------------------------
  ⍝ figure per-item times for sequential and parallel execution.
  ⍝ We use one LARGE vector
  ⍝
∇Z←SUP_A FIGURE_B EXPR;AA;OP;BB;CNAM;STAT;TITLE;SOFF;POFF;LS;LP;SCYC;PCYC;LEN
  (SOFF POFF)←SUP_A
  (AA OP BB CNAM STAT)←EXPR
  TITLE←AA, ' ', OP, ' ', BB
  SUMMARY←SUMMARY,⊂'-------------- ', TITLE, ' -------------- '
  SUMMARY←SUMMARY,⊂'average sequential startup cost:', (¯8↑⍕⌈SOFF), ' cycles'
  SUMMARY←SUMMARY,⊂'average parallel startup cost:  ', (¯8↑⍕⌈POFF), ' cycles'

  INIT_DATA LEN_PI
  (SCYC LEN)←0     ONE_PASS EXPR
  (PCYC LEN)←CORES ONE_PASS EXPR
  Z←⊂TITLE
  Z←Z, ⌈ (SCYC - SOFF) ÷ LEN
  Z←Z, ⌈ (PCYC - POFF) ÷ LEN
  TS←'per item cost sequential:       ',(¯8↑⍕Z[2]), ' cycles'
  TP←'per item cost parallel:         ',(¯8↑⍕Z[3]), ' cycles'
  SUMMARY←SUMMARY,(⊂TS),(⊂TP)

  MON_A BREAK_EVEN Z
∇

∇DYA_A BREAK_EVEN DYA_B;OP;ICS;ICP;SUPS;SUPP;T1;T2
  (SUPS SUPP)←DYA_A
  (OP ICS ICP)←DYA_B
  T1←'parallel break-even length:     '
  T2←'     not reached'
  →(ICP ≥ ICS)⍴1+↑⎕LC ◊ T2←¯8↑⍕⌈ (SUPP - SUPS) ÷ ICS - ICP
  SUMMARY←SUMMARY,(⊂T1,T2),⊂''
∇

  ⍝ ----------------------------------------------------

∇GO;DYA_A;DYA_B;MON_A;SUMMARY
  MON_A←Average[1] ⊃ FIGURE_A ¨ MON_EXPR
  DYA_A←Average[1] ⊃ FIGURE_A ¨ DYA_EXPR

  SUMMARY←0⍴''
  ⊣ (⊂MON_A) FIGURE_B ¨ MON_EXPR
  ⊣ (⊂DYA_A) FIGURE_B ¨ DYA_EXPR

 ''
 78↑' ============================  SUMMARY  ',80⍴'='
 ''
  ⊣ { ⎕←⍵ }¨SUMMARY
∇


  GO

  )OFF

