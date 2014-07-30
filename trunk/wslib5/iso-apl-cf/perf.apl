⍝! -*- mode: gnu-apl -*-
⍝ Component File performance tests
⍝ David B. Lamkins <david@lamkins.net>

)copy iso-apl-cf/iso_cf

'This test takes several minutes to run.'
'The first step takes the longest. Please be patient.'

⍝ Expect times on the order of 100 ms per transaction.

⍝ Definitions:
cf_file_path←'ptest.cf'

⍝ Timing functions:
cf_now←{ ⎕ai[⎕io+2] }
cf_time←{ 'time: 5,550.50 ms per component'⍕⍵÷⍨cf_now-⍺ }

⍝ Prepare the environment.
CF_ERASE cf_file_path

⍝ Create a component file.
cf_tn←CF_CREATE cf_file_path

⍝ Create the test data.
cf_data←'test' (⍳10) (4 3⍴'thecatwasfat')

⍝ Append 500 components.
'append    500; no transaction'
cf_start←cf_now
0 0⍴ { cf_data CF_APPEND cf_tn }¨⍳500
cf_start cf_time 500

⍝ Append another 500 components in a transaction wrapper.
'append    500; w/ transaction'
cf_start←cf_now
cf_transaction←CF_TRANSACTION_BEGIN cf_tn
0 0⍴ { cf_data CF_APPEND cf_tn }¨⍳500
CF_TRANSACTION_COMMIT cf_transaction
cf_start cf_time 500

⍝ Replace 50 random components.
'replace    50; no transaction'
cf_start←cf_now
0 0⍴ { ⍵ CF_WRITE cf_tn,⍵ }¨50?500
cf_start cf_time 50

⍝ Replace another 50 random components in a transaction wrapper.
'replace    50; w/ transaction'
cf_start←cf_now
cf_transaction←CF_TRANSACTION_BEGIN cf_tn
0 0⍴ { ⍵ CF_WRITE cf_tn,⍵ }¨500+50?500
CF_TRANSACTION_COMMIT cf_transaction
cf_start cf_time 50

⍝ Drop 50 random components.
'drop       50; no transaction'
cf_start←cf_now
0 0⍴ { CF_DROP cf_tn,⍵ }¨50?500
cf_start cf_time 50

⍝ Drop another 50 random components in a transaction wrapper.
'drop       50; w/ transaction'
cf_start←cf_now
cf_transaction←CF_TRANSACTION_BEGIN cf_tn
0 0⍴ { CF_DROP cf_tn,⍵ }¨500+50?500
CF_TRANSACTION_COMMIT cf_transaction
cf_start cf_time 50

⍝ Here's why we put transactions under control of the application.
⍝ Look what happens if we batch 10,000 new components under just
⍝ one transaction.
'append 10,000; w/ transaction'

cf_start←cf_now
cf_transaction←CF_TRANSACTION_BEGIN cf_tn
0 0⍴ { (⍵ cf_data) CF_APPEND cf_tn }¨(1-⎕io)+⍳10000
CF_TRANSACTION_COMMIT cf_transaction
cf_start cf_time 10000

⍝ Lesson learned: inserts are fast; transactions are slow.
⍝ Don't believe it...? Look:
(~∧/cf_iv={↑CF_READ cf_tn,⍵}¨1000+cf_iv←(1-⎕io)+⍳¯1000+¯1+CF_NEXT cf_tn)/'mismatch'

⍝ Close the file.
CF_CLOSE cf_tn

'Finished all tests'
