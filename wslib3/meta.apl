#!/usr/local/bin/apl --script

 ⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝
⍝
⍝ /home/eedjsa/apl/apl-1.4/wslib3/meta.apl 2014-08-23 18:27:01 (GMT+2)
⍝
 ⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝⍝

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

∇Z←meta⍙metadata
 Z←0 2⍴⍬
 Z←Z⍪'Author'          'Jürgen Sauermann'
 Z←Z⍪'BugEmail'        'bug-apl@gnu.org'
 Z←Z⍪'Documentation'   ''
 Z←Z⍪'Download'        'http://svn.savannah.gnu.org/viewvc/*checkout*/trunk/wslib3/meta.apl?root=apl'
 Z←Z⍪'License'         'GPL'
 Z←Z⍪'Portability'     'L1'
 Z←Z⍪'Provides'        'metadata'
 Z←Z⍪'Requires'        ''
 Z←Z⍪'Version'         '1,0'
∇

LEN←1⍴17

⎕CT←1E¯13

⎕FC←6⍴(,⎕UCS 46 44 8902 48 95 175)

⎕IO←1

⎕L←0

⎕LX←' '
  ⎕LX←0⍴⎕LX

⎕PP←10

⎕PR←' '

⎕PS←0

⎕PW←80

⎕R←0

⎕RL←16807

⎕TZ←2

⎕X←0

