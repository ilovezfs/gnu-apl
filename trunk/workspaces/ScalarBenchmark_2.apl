#! /usr/local/bin/apl --script

'Running ScalarBenchmark_2'

)COPY 5 FILE_IO

'libaplplot' ⎕FX  'PLOT'

 ⍝ expressions to be benchmarked
 ⍝
EXPR←⎕INP 'END-OF-EXPR'
+ Int
+ Real
+ Comp
- Int
- Real
- Comp
× Int
× Real
× Comp
÷ Int1
÷ Real1
÷ Comp1
∼ Bool
! Int2
⌈ Real
⌊ Real
⋆ Real
⍟ Real
○ Real
∣ Real
? Int2
Int  + Int
Real + Real
Comp + Comp
Int  - Int
Real - Real
Comp - Comp
Int  × Int
Real × Real
Comp × Comp
Int1  ÷ Int1
Real1 ÷ Real1
Comp1 ÷ Comp1
Bool ∧ Bool1
Bool ∨ Bool1
Bool ⍲ Bool1
Bool ⍱ Bool1
Int  ⌈ Int
Real ⌈ Real
Int  ⌊ Int
Real ⌊ Real
Int  ! Int
Real ! Real
Comp ! Comp
Int  ⋆ Int
Real ⋆ Real
Comp ⋆ Comp
Int  ⍟ Int
Real ⍟ Real
Comp ⍟ Comp
Int  < Int
Real < Real
Int  ≤ Int
Real ≤ Real
Int  = Int
Real = Real
Comp = Comp
Int  ≠ Int
Real ≠ Real
Comp ≠ Comp
Int  > Int
Real > Real
Int  ≥ Int
Real ≥ Real
1    ○ Real
1    ○ Comp
Int  ∣ Int
Real ∣ Real
Comp ∣ Comp
1 2 3 ⋸ Int
END-OF-EXPR

⍝ setup the mix of Int, Real, and Complex
⍝
Ilen←4000
Rlen←2000
Clen←50

∇INIT_DATA
  ⍝ setup variables used in benchmark expressions:
  ⍝ Int:  ¯2 ... 9
  ⍝ Int1: nonzero Int
  ⍝ Real: ¯10 to 10 or so
  ⍝
  One  ← 1
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
∇

∇RESHAPE_DATA LEN
  Int  ← LEN ⍴ Int
  Int1 ← LEN ⍴ Int1
  Int2 ← LEN ⍴ Int2
  Bool ← LEN ⍴ Bool
  Bool1← LEN ⍴ Bool1
  Real ← LEN ⍴ Real
  Real1← LEN ⍴ Real1
  Real2← LEN ⍴ Real2
  Comp ← LEN ⍴ Comp
  Comp1← LEN ⍴ Comp1
∇

 ⍝ Int
 ⍝ Real
 ⍝ Comp
 ⍝ Comp1
 ⍝ Bool

ACHARS←' 0123456789', ⎕UCS (64 +⍳26), 96+⍳26  ⍝ chars left of function in EXPR

⍝ benchmark one expression
⍝
∇RUN B
⍝ SHOW B
 ⊣⍎B
∇

∇Z←IS_DYADIC EXP
 Z←EXP[1]∈ACHARS
∇

∇SHOW EXP;OP;NOP;NA;NB
 OP←(EXP∈ACHARS)⍳0
 NOP←EXP[OP]
 NA←'' ◊ →(OP=1)/⎕LC+1 ◊ NA←⍎¯1↓OP↑EXP
 NB←⍎OP↓EXP
 NA,' ',NOP,' ',NB
∇

∇Z←X LSQRL Y;N;XY;XX;b;a
 N←⍴X
 XY←X×Y ◊ XX←X×X
 SX←+/X ◊ SY←+/Y ◊ SXY←+/XY ◊ SXX←+/XX
 b←( (N×SXY) - SX×SY ) ÷ ((N×SXX) - SX×SX )
 a←(SY - b×SX) ÷ N
 Z←a,b
∇

  ⍝ figure start-up times. We use small sizes for better precision
  ⍝
∇Z←FIGURE_A EXP;RL;LENGTHS;IDX;LEN;ZZ;SA;SB;PA;PB;BE;ILL
  RL←⎕RL
  Z←0 3⍴0
  LENGTHS←2+⍳20 ⍝ outer loop vector lengths
  ILL←100       ⍝ inner loop repeat count

  IDX←⍴LENGTHS
L:→(1=IDX←IDX-1)/LX ◊ LEN←LENGTHS[IDX]
  ZZ←LEN, 0, 0
  STAT←34 + IS_DYADIC EXP
  OP←EXP[(EXP∈ACHARS)⍳0]

  ⍝ sequential
  ⍝
  ⎕SYL[26;2] ← 0
  ⎕RL←RL ◊ INIT_DATA ◊ RESHAPE_DATA LEN
  IDX1←0
  ZZZ←⍬
LS: →(ILL=IDX1←IDX1+1)/LSX
  FIO∆clear_statistics STAT
  RUN EXP
  ZZZ←ZZZ,(FIO∆get_statistics STAT)[4]
  →LS
LSX:
  ZZ[2]←⌊/2↓ZZZ

  ⍝ parallel
  ⍝
  ⎕SYL[26;2] ← 4
  ⎕RL←RL ◊ INIT_DATA ◊ RESHAPE_DATA LEN
  IDX1←0
  ZZZ←⍬
LP: →(ILL=IDX1←IDX1+1)/LPX
  FIO∆clear_statistics STAT
  ⊣ 1 FIO∆set_dyadic_threshold OP
  ⊣ 1 FIO∆set_monadic_threshold OP
  RUN EXP
  ZZZ←ZZZ,(FIO∆get_statistics STAT)[4]
  →LP
LPX:
  ZZ[3]←⌊/¯2↓ZZZ

  Z←ZZ⍪Z
  →L
LX:

  (SA SB)←⌊Z[;1] LSQRL Z[;2]
  (PA PB)←⌊Z[;1] LSQRL Z[;3]

⍝ plot it
P←Z[;2 3]
P←P,(SA+Z[;1]×SB)
P←P,(PA+Z[;1]×PB)
'------' ◊ (Z[;1],P) ◊ '------'
('tlabel "',EXP,'";draw l') PLOT P
⍞

  Z←SA,PA
  (20↑EXP),(¯6↑⍕Z[1]),¯6↑⍕Z[2]
∇

  ⍝ ----------------------------------------------------

  D←FIGURE_A ¨ EXPR
  ⌊ (+⌿D)÷↑⍴D

  )OFF

