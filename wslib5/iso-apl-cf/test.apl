⍝! -*- mode: gnu-apl -*-
⍝ Component File test cases
⍝ David B. Lamkins <david@lamkins.net>

)copy iso-apl-cf/iso_cf

⍝ Definitions:
cf_file_path_1←'test1.cf'
cf_file_path_2←'test2.cf'

'Starting tests'

cf_test_number←0

∇a cf_test_assert b
  cf_test_number←cf_test_number+1
  ⍎(~a≡b)/'''Test:    '' cf_test_number ◊ ''Expected:'' a ◊ ''Found:   '' b'
∇

⍝ Prepare the environment.
(0 0⍴0) cf_test_assert CF_ERASE cf_file_path_1
(0 0⍴0) cf_test_assert CF_ERASE cf_file_path_2

⍝ Create a component file.
0 cf_test_assert cf_tn1←CF_CREATE cf_file_path_1

⍝ Check the version.
1 cf_test_assert CF_VERSION cf_tn1

⍝ Confirm that the file is empty.
1 cf_test_assert CF_NEXT cf_tn1

⍝ Append a component.
1 cf_test_assert (cf_test_tn1c1←3 3⍴⍳9) CF_APPEND cf_tn1

⍝ Append another component.
2 cf_test_assert (cf_test_tn1c2←'Let''s save a string.') CF_APPEND cf_tn1

⍝ Yet another...
3 cf_test_assert (cf_test_tn1c3←1 (⍳5) (2 3⍴⍳6)) CF_APPEND cf_tn1

⍝ At svn 217 the 2⎕tf behavior caused data binding to fail for
⍝ cf∆append and cf∆replace in the case that the final element of
⍝ a nested array is a character array of rank greater than 1.
⍝ Let's make sure that this doesn't regress.
4 cf_test_assert (cf_test_tn1c4←'svn' 218 (2 4⍴'testcase')) CF_APPEND cf_tn1

⍝ Check the next component number.
5 cf_test_assert CF_NEXT cf_tn1

⍝ Close the file.
(0 0⍴0) cf_test_assert CF_CLOSE cf_tn1

⍝ Reopen the file.
0 cf_test_assert cf_tn1←CF_OPEN cf_file_path_1

⍝ Next component number should be unchanged.
5 cf_test_assert CF_NEXT cf_tn1

⍝ Read the 2nd component.
cf_test_tn1c2 cf_test_assert CF_READ cf_tn1,2

⍝ Now the 1st component.
cf_test_tn1c1 cf_test_assert CF_READ cf_tn1,1

⍝ Now the 4th...
cf_test_tn1c4 cf_test_assert CF_READ cf_tn1,4

⍝ And the 3rd...
cf_test_tn1c3 cf_test_assert CF_READ cf_tn1,3

⍝ Check component presence.
1 1 1 1 0 cf_test_assert { CF_EXISTS cf_tn1,⍵ }¨(1-⎕io)+⍳5

⍝ Let's create another component file.
1 cf_test_assert cf_tn2←CF_CREATE cf_file_path_2

⍝ The new file is empty.
1 cf_test_assert CF_NEXT cf_tn2

⍝ The first file hasn't changed.
5 cf_test_assert CF_NEXT cf_tn1

⍝ Add a component to the second file.
1 cf_test_assert 'test' CF_APPEND cf_tn2

⍝ Add a component to the second file using an explicit transaction.
1 cf_test_assert cf_transaction←CF_TRANSACTION_BEGIN cf_tn2
2 cf_test_assert ('now' 'is' 'the' 'time' 0) CF_APPEND cf_tn2
(0 0⍴0) cf_test_assert CF_TRANSACTION_COMMIT cf_transaction

⍝ The first component of the two files differs.
0 cf_test_assert (CF_READ cf_tn1,1) ≡ CF_READ cf_tn2,1

⍝ Confirm the next component number of the second file.
3 cf_test_assert CF_NEXT cf_tn2

⍝ Let's replace a component.
(0 0⍴0) cf_test_assert 777 CF_WRITE cf_tn1,2

⍝ Confirm that the first file's next component number is unaltered.
5 cf_test_assert CF_NEXT cf_tn1

⍝ Add a few more components.
5 6 7 cf_test_assert { (⍵⍴0) CF_APPEND cf_tn1 }¨⍳3

⍝ The next component number should track.
8 cf_test_assert CF_NEXT cf_tn1

⍝ Check the presence of a component.
1 cf_test_assert CF_EXISTS cf_tn1,5

⍝ Let's drop that component.
(0 0⍴0) cf_test_assert CF_DROP cf_tn1,5

⍝ Now it doesn't exist.
0 cf_test_assert CF_EXISTS cf_tn1,5

⍝ That won't have changed the next component number.
8 cf_test_assert CF_NEXT cf_tn1

⍝ Even when we drop trailing components, the maximum
⍝ component number doesn't revert. Component numbers
⍝ never get reused. This is by design: it preserves
⍝ the identity of a component even across deletion.
(2⍴⊂0 0⍴0) cf_test_assert { CF_DROP cf_tn1,⍵ }¨6 7
8 cf_test_assert CF_NEXT cf_tn1
8 cf_test_assert 'I''m component eight!' CF_APPEND cf_tn1

⍝ Write at an index beyond the highest component number.
(0 0⍴0) cf_test_assert 'whee!' CF_WRITE cf_tn1,100

⍝ The next component number remains beyond the highest written component.
101 cf_test_assert CF_NEXT cf_tn1

⍝ Now write a component into the "gap" created by the prior write.
(0 0⍴0) cf_test_assert (10⍴⊂'Where am I?') CF_WRITE cf_tn1,50

⍝ The next component number is unchanged by the write to the "gap".
101 cf_test_assert CF_NEXT cf_tn1

⍝ The last component hasn't been dropped, therefore it exists.
1 cf_test_assert CF_EXISTS cf_tn1,100

⍝ The component before that was never written, so it doesn't exist.
0 cf_test_assert CF_EXISTS cf_tn1,99

⍝ Now let's replace the last component.
(0 0⍴0) cf_test_assert (○1) CF_WRITE cf_tn1,100

⍝ The next component number remains unchanged.
101 cf_test_assert CF_NEXT cf_tn1

⍝ It's an error to attempt to read a nonexistent component.
5 4 cf_test_assert (1+⎕io)⊃⎕ec 'CF_READ cf_tn1,200'

⍝ Let's write a large component.
(0 0⍴0) cf_test_assert (cf_m←1000 1000⍴1000000?1000000) CF_WRITE cf_tn1,80

⍝ Confirm that we can read the large component.
1 cf_test_assert cf_m≡CF_READ cf_tn1,80

⍝ Confirm the list of open handles.
1 cf_test_assert ∧/(cf_tn1,cf_tn2)∊CF_INUSE

⍝ That's enough for now; let's close our files.
(0 0⍴0) cf_test_assert CF_CLOSE cf_tn1
(0 0⍴0) cf_test_assert CF_CLOSE cf_tn2

⍝ Confirm that the list of open handles is now empty.
⍬ cf_test_assert CF_INUSE

⍝ Make sure that we can list the component files in the filesystem.
1 cf_test_assert ∨/((¯1↑⍴cf_m)↑cf_file_path_1)∧.= ⍉cf_m←CF_LIST ''
1 cf_test_assert ∨/((¯1↑⍴cf_m)↑cf_file_path_2)∧.= ⍉cf_m←CF_LIST ''

⍝ Erase a component file.
(0 0⍴0) cf_test_assert CF_ERASE cf_file_path_1

⍝ Rename a component file. Note that the new name is on the left.
(0 0⍴0) cf_test_assert cf_file_path_1 CF_RENAME cf_file_path_2

⍝ Make sure that the rename had the expected effect.
1 cf_test_assert ∨/((¯1↑⍴cf_m)↑cf_file_path_1)∧.= ⍉cf_m←CF_LIST ''
0 cf_test_assert ∨/((¯1↑⍴cf_m)↑cf_file_path_2)∧.= ⍉cf_m←CF_LIST ''

⍝ Verify that we fail to create over an existing file.
5 4 cf_test_assert (1+⎕io)⊃⎕ec 'CF_CREATE cf_file_path_1'

⍝ Verify that we fail to open a nonexistent file.
5 4 cf_test_assert (1+⎕io)⊃⎕ec 'CF_OPEN cf_file_path_2'

⍝ Verify that it's OK to erase a nonexistent file.
(0 0⍴0) cf_test_assert CF_ERASE cf_file_path_2

'Finished all tests'
