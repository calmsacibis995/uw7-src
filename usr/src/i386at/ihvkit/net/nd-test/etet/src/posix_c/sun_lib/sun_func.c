/*
 * Copyright 1992 SunSoft, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SunSoft, Inc. not be used in 
 * advertising or publicity pertaining to distribution of the software 
 * without specific, written prior permission.  SunSoft, Inc. makes
 * no representations about the suitability of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
 *
 * SunSoft, Inc. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO 
 * EVENT SHALL SunSoft, Inc. BE LIABLE FOR ANY SPECIAL, INDIRECT OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* Common functions for test development using TET API */

#include <stdio.h>
#include <errno.h>
#include <tet_api.h>	/* tet include file */
#include "sun_tet.h"	/* sun include file */

/* 
 * Function: exit_fatal
 * Argument: a message
 * Prints test case, test purpose, and a fatal error message 
 * then aborts test 
 */ 
void 
exit_fatal(msg)		
char *msg;			
{
	fprintf(stderr,
		"FATAL ERROR*** TEST: %s, TEST PURPOSE: %d, %s\n", 
			tet_pname, tet_thistest, msg);
	tet_result(FATAL);
}


/*
 * Function: send_tetinfo
 * Argument: a message
 * Prints test case, test pupose, and a message to the screen 
 * and the journal file 
 */ 
void 
send_tetinfo(msg)
char *msg;              
{
	static char mybuf[512];

	sprintf(mybuf,"TEST: %s, TEST PURPOSE: %d, %s\n",
	    tet_pname, tet_thistest, msg);
	printf(mybuf);
	tet_infoline(mybuf);
}


/*
 * Function: cmp_errno
 * Argument: a message
 * Return: NOT_EQUAL or EQUAL
 * Prints test case, test pupose, and a message to the screen 
 * and the journal file 
 */ 
int
cmp_errno(expected_errno) 
int expected_errno;           
{
	static char mybuf[512];

	if (errno != expected_errno) {	
		sprintf(mybuf,
			"TEST: %s, TEST PURPOSE: %d, expected errno %d, got %d\n",
		        tet_pname, tet_thistest, expected_errno, errno);
		send_tetinfo(mybuf);
		return NOT_EQUAL;
   	}
   	else                  /* compare OK */
		return EQUAL;
}


/* 
 * Function: test_done
 * Indicates final test status before exiting.
 * If no error has been recorded, exit PASS, otherwise FAIL.
 * This function must be called as the last thing in the clean up 
 * procedure in the test program.
 */
void
test_done()
{
	if (err_cnt !=0)
		exit(FAIL);
	else
		exit(PASS);
}
