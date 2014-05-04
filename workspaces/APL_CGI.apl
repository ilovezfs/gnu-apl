#!/usr/bin/apl --script --

 ⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝
⍝ APL_CGI 2014-05-04 14:13:17 (GMT+2)
⍝
 ⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝

∇xZ←xHREF A xB
 Assert 1 ≡ ≡xB    ◊ Assert 1 ≡ ''⍴⍴⍴xB
 Assert 1 ≡ ≡xHREF ◊ Assert 1 ≡ ''⍴⍴⍴xHREF
 xZ←,⊃"A" TX_B_E_1[_href xHREF] xB
 Assert 1 ≡ ≡xZ ◊ Assert 1 ≡ ''⍴⍴⍴xZ
∇

∇Assert B;COND;LOC;VAR
 →(1≡B)⍴0
 ' '
 COND←7↓,¯2 ⎕SI 4
 LOC←,¯2 ⎕SI 3
 '<pre>************************************************'
 ' '
 '*** Assertion (', COND, ') failed at ',LOC
 ''
 ⍝ show variable (assuming COND ends with the variable name)
 ⍝
 VCHAR←'∆⍙',⎕UCS ,64 96 ∘.+⍳26
 VAR←(⌽∧\⌽(COND∈VCHAR))/COND
 '</pre>'
 0 0⍴VAR debug ⍎VAR
 '<pre>'
 ⍝ show stack
 ⍝ 
 ' '
 'Stack:'
 7 ⎕CR ⊃¯1↓⎕SI 3
 ' '
 '************************************************</pre>'
 →
∇

∇yZ←Body[xX] yB
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 yZ←'BODY' TX_B_E[xX] yB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

∇yZ←Document
 yZ←    x2y HTML∆HTTP_header
 yZ←yZ, x2y '<!DOCTYPE html>'
 yZ←yZ, Html yBODY
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

∇yZ←H1[xX] xB
 yZ←"H1" TX_B_E_1[xX] xB
∇

∇yZ←H2[xX] xB
 yZ←"H2" TX_B_E_1[xX] xB
∇

∇yZ←H3[xX] xB
 yZ←"H3" TX_B_E_1[xX] xB
∇

∇yZ←H4[xX] xB
 yZ←"H4" TX_B_E_1[xX] xB
∇

∇xZ←HTML∆HTTP_header
 xZ←'' ◊ →(0=⍴,⎕ENV "GATEWAY_INTERFACE")⍴0   ⍝ nothing if not run as CGI script
 ⍝
 ⍝ The 'Content-type: ...' string ends with LF (and no CR)
 ⍝ ⎕UCS 13 10 13 emits CR LF CR.
 ⍝ Together this gives two lines, each ending with CR/LF
 ⍝ Most tolerate LF without CR, but the CGI standard wants CR/LF
 ⍝
 xZ←'Content-type: text/html; charset=UTF-8', ⎕UCS 13 10 13
∇

∇xZ←xA HTML∆attr xB
 Assert 1 ≡ ≡xA ◊ Assert 1 ≡ ''⍴⍴⍴xA
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 xZ←' ',xA,'="',xB,'"'
 Assert 1 ≡ ≡xZ ◊ Assert 1 ≡ ''⍴⍴⍴xZ
∇

∇yZ←yA HTML∆xbox yB;LenA;H
 Assert 2 ≡ ≡yA ◊ Assert 1 ≡ ''⍴⍴⍴yA
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 LenA←⌈/⍴¨yA
 H←↑(⍴yA)⌈⍴yB
 yA←H↑LenA↑¨yA
 yZ←yA ,¨H↑yB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

∇Z←HTML⍙Author
 Z←,⊂'Jürgen Sauermann'
∇

∇Z←HTML⍙BugEmail
 Z←,⊂'bug-apl@gnu.org'
∇

∇Z←HTML⍙Documentaion
 Z←,⊂''
∇

∇Z←HTML⍙Download
 Z←,⊂''
∇

∇Z←HTML⍙License
 Z←,⊂'LGPL (GNU Lesser General Public License)''
∇

∇Z←HTML⍙Portability
 Z←,⊂'L1'
∇

∇Z←HTML⍙Provides
 Z←,⊂'HTML encoding functions'
∇

∇Z←HTML⍙Requires
 Z←0⍴⊂''
∇

∇Z←HTML⍙Version
 Z←,⊂'1.0'
∇

∇yZ←Head;yB
 yB←Title xTITLE
 yB←yB,,⊂'META' T[(_http_eq 'Content-Type'), _content 'text/html; charset=UTF-8'] 1
 yB←yB,,⊂'META' T[(_name 'description'), _content xDESCRIPTION] 1
 yB←yB,Link (_rel  'stylesheet'),(_type 'text/css'), _href 'apl-home.css'
 yB←yB,Style[_type  'text/css'] ,⊂''
 yZ←'HEAD' TX_B_E yB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

∇yZ←Html[xX] yB
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 yZ←'HTML' TX_B_E Head, Body[xX] yB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

∇yZ←Img[xX] xB
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 Assert 1 ≡ ≡xX ◊ Assert 1 ≡ ''⍴⍴⍴xX
 yZ←,⊂'IMG' T[xX] xB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

∇xZ←Li xB
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 xZ←'<LI> ', xB
 Assert 1 ≡ ≡xZ ◊ Assert 1 ≡ ''⍴⍴⍴xZ
∇

∇yZ←Link xX
 Assert 1 ≡ ≡xX ◊ Assert 1 ≡ ''⍴⍴⍴xX
 yZ←,⊂'LINK' T[xX] 1
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

∇yZ←Ol[xX] yB
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 yZ←'OL' TX_B_E[xX] Li¨yB
∇

∇yZ←Style[xX] yB
 Assert 1 ≡ ≡xX ◊ Assert 1 ≡ ''⍴⍴⍴xX
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 yZ←'STYLE' TX_B_E[xX] yB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

∇xZ←xA T[xX] B
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 Assert 1 ≡ ≡xA ◊ Assert 1 ≡ ''⍴⍴⍴xA
 Assert 1 ≡ ≡xX ◊ Assert 1 ≡ ''⍴⍴⍴xX
 xZ←'<',((B=2)⍴'/'),xA,xX,((B=3)⍴'/'),'>'
 Assert 1 ≡ ≡xZ ◊ Assert 1 ≡ ''⍴⍴⍴xZ
∇

∇yZ←xA TX_B_1[xX] xB
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 Assert 1 ≡ ≡xA ◊ Assert 1 ≡ ''⍴⍴⍴xA
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 Assert 1 ≡ ≡xX ◊ Assert 1 ≡ ''⍴⍴⍴xX
 yZ←,⊂(xA T[xX] 1),xB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

∇yZ←xA TX_B_E[xX] yB
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 Assert 1 ≡ ≡xA ◊ Assert 1 ≡ ''⍴⍴⍴xA
 Assert 1 ≡ ≡xX ◊ Assert 1 ≡ ''⍴⍴⍴xX
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 yZ←,⊂xA T[xX] 1
 yZ←yZ,indent yB
 yZ←yZ,⊂xA T 2
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

∇yZ←xA TX_B_E_1[xX] xB
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 Assert 1 ≡ ≡xA ◊ Assert 1 ≡ ''⍴⍴⍴xA
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 Assert 1 ≡ ≡xX ◊ Assert 1 ≡ ''⍴⍴⍴xX
 yZ←,⊂(xA T[xX] 1),xB,xA T 2
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

∇yZ←Title xB
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 yZ←'TITLE' TX_B_E_1 xB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

∇yZ←Ul[xX] yB
 →1+(0≠⎕NC 'xX')⍴⎕LC ◊ xX←''
 yZ←'UL' TX_B_E[xX] Li¨yB
∇

∇xZ←_alt  xB
 xZ←"alt"        HTML∆attr xB
∇

∇xZ←_content xB
 xZ←"content"    HTML∆attr xB
∇

∇xZ←_h_w B
 xZ←(_height ↑B), _width 1↓B
∇

∇xZ←_height B
 xZ←"height"     HTML∆attr ⍕B
∇

∇xZ←_href xB
 xZ←"href"       HTML∆attr xB
∇

∇xZ←_http_eq xB
 xZ←"http-equiv" HTML∆attr xB
∇

∇xZ←_in
 xZ←"class"      HTML∆attr "apl_in"
∇

∇xZ←_name xB
 xZ←"name"       HTML∆attr xB
∇

∇xZ←_out
 xZ←"class"      HTML∆attr "apl_out"
∇

∇xZ←_rel  xB
 xZ←"rel"        HTML∆attr xB
∇

∇xZ←_src  xB
 xZ←"src"        HTML∆attr xB
∇

∇xZ←_type xB
 xZ←"type"       HTML∆attr xB
∇

∇xZ←_width B
 xZ←"width"      HTML∆attr ⍕B
∇

∇Z←A debug B;yPROP;yVAL
 yPROP←,⊂'<pre>At ',(,¯2 ⎕SI 3),': ' ◊ V←''
 →1+(0=⎕NC 'A')⍴⎕LC ◊  yPROP←'' ◊ V←A
 yPROP←yPROP,⊂'≡  ',V,': ',,⍕≡B
 yPROP←yPROP,⊂'⍴⍴ ',V,': ',,⍕⍴⍴B
 yPROP←yPROP,⊂'⍴  ',V,': ',,⍕⍴B
 yVAL←⊂[⎕IO+1]4 ⎕CR B
 ⊃yPROP HTML∆xbox yVAL
 '</pre>'
 Z←B
∇

∇emit yB
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 emit_1¨yB
∇

∇emit_1 yB
 ⊃yB
∇

∇yZ←indent yB
 Assert 2 ≡ ≡yB ◊ Assert 1 ≡ ''⍴⍴⍴yB
 yZ←indent_2 ¨,yB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

∇xZ←indent_2 xB
 xB←,xB
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 xZ←'  ',xB
 Assert 1 ≡ ≡xZ ◊ Assert 1 ≡ ''⍴⍴⍴xZ
∇

∇yZ←x2y xB
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 yZ←,⊂xB
 Assert 2 ≡ ≡yZ ◊ Assert 1 ≡ ''⍴⍴⍴yZ
∇

xDESCRIPTION←25⍴'<please-set-xDESCRIPTION>'

xTITLE←19⍴'<please-set-xTITLE>'

yBODY←' '
  yBODY←0⍴yBODY

⎕CT←1E¯13

⎕FC←6⍴(,⎕UCS 46 44 8902 48 95 175)

⎕IO←1

⎕L←0

⎕LX←' '
  ⎕LX←0⍴⎕LX

⎕NLT←1⍴'C'

⎕PP←10

⎕PR←1⍴' '

⎕PS←0

⎕PW←1000

⎕R←0

⎕RL←1

⎕TZ←2

