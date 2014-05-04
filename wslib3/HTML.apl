⍝! apl --script
⍝
⍝ HTML.apl
⍝
⍝ This file is a GNU APL L1 library containing functions for generating some
⍝ frequently used HTML tags.
⍝
⍝ The library is used by means of the )COPY command:
⍝
⍝ )COPY 3 HTML
⍝
⍝
⍝ The library  provides the following functions (meta functions omitted):
⍝
⍝ HTML∆xbox		append text matrices A and B horizontally
⍝ HTML∆HTTP_header	CGI header (if started from a web server)
⍝ HTML∆attr		HTML attribute xA with value xB
⍝
⍝
⍝ Variable name conventions:
⍝
⍝ Variables starting with x, e.g. xB, are strings (simple vectors of
⍝ characters), i.e. 1≡ ≡xB and 1≡''⍴⍴⍴xB
⍝
⍝ Variables starting with y are vectors of character strings,
⍝ i.e. 2≡ ≡yB and 1≡''⍴⍴⍴yB
⍝
⍝ Certain characters in function names have the following meaning:
⍝
⍝ T - start tag
⍝ E - end tag
⍝ X - attributes

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ HTML agnostic helper functions
⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝

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

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ generic HTML related helper functions
⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ emit a CGI header (if started from a web server)
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

⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ HTML attribute xA with value xB
⍝ xA="xB"
⍝
∇xZ←xA HTML∆attr xB
 Assert 1 ≡ ≡xA ◊ Assert 1 ≡ ''⍴⍴⍴xA
 Assert 1 ≡ ≡xB ◊ Assert 1 ≡ ''⍴⍴⍴xB
 xZ←' ',xA,'="',xB,'"'
 Assert 1 ≡ ≡xZ ◊ Assert 1 ≡ ''⍴⍴⍴xZ
∇

  ⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
 ⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝⍝                                                                     ⍝⍝
⍝⍝		library meta information...                            ⍝⍝
⍝⍝                                                                     ⍝⍝
 ⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
  ⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝

∇Z←HTML⍙Author
 Z←,⊂'Jürgen Sauermann'
∇

∇Z←HTML⍙Version
 Z←,⊂'1.0'
∇

∇Z←HTML⍙Portability
 Z←,⊂'L1'
∇

∇Z←HTML⍙License
 Z←,⊂'LGPL (GNU Lesser General Public License)''
∇

∇Z←HTML⍙BugEmail
 Z←,⊂'bug-apl@gnu.org'
∇

∇Z←HTML⍙Requires
 Z←0⍴⊂''
∇

∇Z←HTML⍙Provides
 Z←,⊂'HTML encoding functions'
∇

∇Z←HTML⍙Download
 Z←,⊂''
∇

∇Z←HTML⍙Documentaion
 Z←,⊂''
∇

