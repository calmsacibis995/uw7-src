The mail test package include the following Word Perfect 6.1 formated documents:

unixmail.d5  (Revision 5 of the UNIX Mail Test Specification)
unixmail.d51 (Revision 5 Part 1 of the Unix Mail Test Specification)
unixmail.d52 (Revision 5 Part 2 of the Unix Mail Test Specification)
unixmail.d53 (Revision 5 Part 3 of the Unix Mail Test Specification)
binmail.wpd  (/bin/mail Front End Test Specification)
mailx.wpd    (/bin/mailx Front End Test Specification)
metamail.wpd (The Metamail Test Specification)
mailr.pwd    (The MailR Test Specification)

Note the UNIX mail test specification is split into 4 Word Perfect Documents.
The file unixmail.d5 is the master document.  To bring up the whole document,
execute following steps:

1. Bring up Word Perfect 6.1.
2. Open up the file unixmail.d5
3. From the File menu select Master Document then select Expand Master.

From here you can print the whole document and/or edit it.

The UNIX Mail Test Spec is the master document for all mail testing.  It 
references all other test specs dealing with UNIX mail (but not MHS or dtmail).
This is the docuent you start with to test mail.  It will direct the tester 
when to consult other mail component test specs.  Most test are interactive.
All automated tests are installed with the mail test package (mtest). Mtest is
localted in the /PRVTESTS/UTA_Tests/pkgs directory on this system.  Mtest is
in pkgadd format and requires UTA and ETET installed on the system. Mtest
has automated tests for the following components:

mailx
mailR
Mail Stress and Reliablility Tests (domail)
mail security
/bin/mail (This is still under construction. Only 20% of the tests are 
           automated.)

The UNIX Mail Test Specification describes how to setup and run the manual
and automated tests, or guides the tester to the correct mail component spec 
for information.



