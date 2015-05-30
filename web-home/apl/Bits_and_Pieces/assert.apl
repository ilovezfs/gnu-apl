<!-- #!apl --script -->
<html><head>
<meta http-equiv="content-type" content="text/html; charset=UTF-8">
</head><body><pre>
⍝
⍝ Author:       Jürgen Sauermann
⍝ Date:         May 31, 2015
⍝ Copyright:    Copyright (C) 2015 by Jürgen Sauermann
⍝ License:      LGPL, see http://www.gnu.org/licenses/lgpl.html
⍝ email:        bug-apl@gnu.org
⍝ Portability:  L3
⍝
⍝ Purpose:
⍝
⍝ Check a condition and complain if it is not true
⍝
⍝ Description:
⍝ 
⍝ this file contains a function 'assert' that can be used for debugging APL
⍝ programs. assert checks a condition and if the condition is false
⍝ (i.e. not 1) then it prints the failed condition and the )SI stack to
⍝ locate the failed assertion. If the condition ends with a variable name
⍝ then the value of the variable is also printed.
⍝
⍝ )COPY assert or cut-and paste 'assert'
⍝
⍝ Then try
⍝
⍝ Q←5
⍝ Assert 4 = 2 + 2 ⍝ OK
⍝ Assert 3 = 2 + 2 ⍝ not OK: print failed condition and SI stack
⍝ Assert 3 = Q     ⍝ not OK: print failed condition, value of Q, and SI stack
⍝


⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝ assert: check a condition
⍝
∇assert B;COND;LOC;VAR;VCHAR
 →(1≡B)⍴0
 ' '
 COND←7↓,¯2 ⎕SI 4 ◊ LOC←,¯2 ⎕SI 3
 '************************************************'
 '*** Assertion (', COND, ') failed at ',LOC
 ⍝ maybe show variable (assuming COND ends with the variable name)
 VCHAR←'∆⍙',⎕UCS ,64 96 ∘.+⍳26
 VAR←(⌽∧\⌽(COND∈VCHAR))/COND
 →((⎕NC VAR)∈2 6)↓1+↑⎕LC ◊ 'Variable ',VAR,':' ◊ 4 ⎕CR ⍎VAR
 ⍝ show stack
 'SI Stack:' ◊ 7 ⎕CR ⊃¯1↓⎕SI 3
 '************************************************'
 →
∇

⍝
⍝ EOF &lt;/pre&gt;&lt;/body&gt;&lt;/html&gt;
