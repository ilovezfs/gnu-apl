#! /usr/local/bin/apl --script

'Running ScalarBenchmark_1'

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

⍝ setup the mix of Int, real, and Complex
⍝
Ilen←4000
Rlen←2000
Clen←50

⍝ setup some variables used in benchmark expressions:
⍝ Int:  ¯2 ... 9
⍝ Int1: nonzero Int
⍝ Real: ¯10 to 10 or so
⍝
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

 ⍝ Int
 ⍝ Real
 ⍝ Comp
 ⍝ Comp1
 ⍝ Bool

NAME←' ', ⎕UCS (64 +⍳26), 96+⍳26
⍝ benchmark one expression
⍝
∇RUN B
⍝ SHOW B
 ⊣⍎B
∇

∇SHOW B;OP;NOP;NA;NB
 OP←(B∈NAME)⍳0
 NOP←B[OP]
 NA←'' ◊ →(OP=1)/⎕LC+1 ◊ NA←⍎¯1↓OP↑B
 NB←⍎OP↓B
 NA,' ',NOP,' ',NB
∇

  ⍝ benchmark all expressions. We first do a warm-up run to preload the caches
  ⍝ a little and then measure the second run
  ⍝
  RUN¨EXPR 
  ]PSTAT CLEAR
  RUN¨EXPR 

  ⍝ done
  ⍝
  ]PSTAT SAVE
⍝ ]PSTAT 13
  ]PSTAT
  )OFF

