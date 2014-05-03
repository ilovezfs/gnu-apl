⍝! apl --script
⍝
⍝ HTML.apl
⍝
⍝ This filee is a GNU APL L1 library containing functions for generating
⍝ frequently used HTML tags. It provides the following functions (meta
⍝ functions omitted):
⍝
⍝ HTML∆xbox	append text matrices A and B horizontally
⍝

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ append boxes A and B horizontally:
⍝
⍝ AABBB   AA      BBB
⍝ AABBB ← AA xbox BBB
⍝ AA      AA
⍝
∇yZ←yA HTML∆xbox yB;LenA;H
 Assert 2 ≡ ≡yA ◊ Assert 1 ≡ ''⍴⍴⍴yA
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 LenA←⌈/⍴¨yA
 H←↑(⍴yA)⌈⍴yB
 yA←H↑LenA↑¨yA
 yZ←yA ,¨H↑yB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

