
		Test Report and Metrics for message store

We found 50 bugs in 5100 lines of C source.
82% statement coverage was obtained.

50 bugs were found.
165 changes were made.
5100 lines of C source code were developed.
8500 lines of test code (7500 sh, 1000 C) were developed.
148 test cases were developed.
82% statement coverage was achieved using fully automated testing.
10 bugs/KLOC (bugs per thousand lines of code) were discovered.
33 changes/KLOC (changes per thousand lines of code) were made.

2 weeks were spent on the design.
1 weeks were spent to implement the design.
6 weeks were spent in testing (developing the suite and fixing the bugs found).

The resultant test suite can be executed in 20 minutes on a P5/90.

A significant amount of rework was needed during the testing phase.
This is visible from the calendar time to test.
This is indicative of a rushed design and implementation cycle.
This agrees with our actual experience as it was a bit rushed.
The problem was also quite complex.

How these results were obtained:

Statement coverage percentage is based on the number of instrumented lines
v.s. the number of lines executed.  Not all lines in a source file are
instrumented (only about 3000 lines in this case were instrumented).
We used lprof and cc -ql to instrument our program.

We diffed a partially tested version with the final version.
(source files scoms1.c and scomsc1.c were used).

Every difference was counted as well as logical bugs.  There was one delta
where the source file was reformatted but no substantive differences were made
so we ignored this particular diff listing.

The KLOC number is too low, as some of the test suite
had been developed by the time we deltaed the first source snapshot.
We learnt from this that "Code complete" needs it's own special delta
to preserve history as we lost some information here.

We spent a second week on implementation in the so-called test phase
since we discovered unimplemented features early in the test cycle.

We were unable to get lprof to work during test suite development.
The coverage numbers were obtained later (BL8) when I learned how
to make lprof work.  I think we could get the numbers up a bit with
some more test suite development work.  It would be nice to get 100%.
