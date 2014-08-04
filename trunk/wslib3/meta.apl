⍝! apl
⍝
⍝ meta: a library for creating and checking meta functions in a library
⍝
⍝ intended usage:
⍝
⍝ )COPY meta
⍝ meta∆make_function         create the function for your library
⍝ ... answer questions
⍝ meta∆remove_meta_helpers   remove functions provided by this library
⍝

⍝ return the meta functions to be provided by Package
⍝
∇Z←meta∆tags
 Z←  ⊂ 'Author'
 Z←Z,⊂ 'BugEmail'
 Z←Z,⊂ 'Documentation'
 Z←Z,⊂ 'Download'
 Z←Z,⊂ 'License'
 Z←Z,⊂ 'Portability'
 Z←Z,⊂ 'Provides'
 Z←Z,⊂ 'Requires'
 Z←Z,⊂ 'Version'
∇

⍝ create the meta function for Package
⍝
∇meta∆make_function Package;Tags;Vals;Fun;Qu
 Qu←{ ((1=⍴⍵)/'(,') , '''' , ⍵ , '''' , (1=⍴⍵)/')' }
 Tags ← meta∆tags ◊ LEN←4 + ⌈⌿⊃⍴¨Tags
 Vals ← { (LEN↓⍞ ⊣ ⍞← LEN↑(⊃⍵),':') } ¨ Tags   ⍝ read tag values

 Fun ← ⊂'Z←',Package,'⍙metadata' 
 Fun ← Fun , ⊂'Z←0 2⍴⍬'
 ⊣ Tags { Fun ← Fun , ⊂'Z←Z⍪', (LEN ↑ Qu ⍺), ' ', Qu ⍵ } ¨ Vals
 ⊣ ⎕FX Fun
∇

∇meta∆remove_meta_helpers
⊣ ⎕EX 'meta∆functions'
⊣ ⎕EX 'meta∆make_function'
⊣ ⎕EX 'meta∆remove_meta_helpers'
∇

      )FNS

