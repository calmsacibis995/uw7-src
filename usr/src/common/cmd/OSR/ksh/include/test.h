#ident	"@(#)OSRcmds:ksh/include/test.h	1.1"
#pragma comment(exestr, "@(#) test.h 25.3 93/01/20 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1993.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*									*/
/*		   Copyright (C) AT&T, 1984-1992.			*/
/*			All Rights Reserved				*/
/*									*/
/*	  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.		*/
/*	    The copyright notice above does not evidence any		*/
/*	   actual or intended publication of such source code.		*/
/*									*/

/*
 * Modification History
 *
 *	L000	scol!markhe	11 Nov 92
 *	- changed type of message variables from 'const char' to MSG
 *	  (not diff marked)
 */

/*
 *	UNIX shell
 *
 *	David Korn
 *	AT&T Bell Laboratories
 *
 */

/*
 *  These are the valid test operators
 */

#define TEST_ARITH	020	/* arithmetic operators */
#define TEST_BINOP	0200	/* binary operator */
#define TEST_PATTERN	040	/* turn off bit for pattern compares */

#define TEST_NE		(TEST_ARITH|9)
#define TEST_EQ		(TEST_ARITH|4)
#define TEST_GE		(TEST_ARITH|5)
#define TEST_GT		(TEST_ARITH|6)
#define TEST_LE		(TEST_ARITH|7)
#define TEST_LT		(TEST_ARITH|8)
#define TEST_OR		(TEST_BINOP|1)
#define TEST_AND	(TEST_BINOP|2)
#define TEST_SNE	(TEST_PATTERN|1)
#define TEST_SEQ	(TEST_PATTERN|14)
#define TEST_PNE	1
#define TEST_PEQ	14
#define TEST_EF		3
#define TEST_NT		10
#define TEST_OT		12
#define TEST_SLT	15
#define TEST_SGT	16
#define TEST_END	8

extern MSG		test_unops;			/* L000 */
extern const struct sysnod	test_optable[];
extern MSG		e_test;				/* L000 */
extern MSG		e_bracket;			/* L000 */
extern MSG		e_paren;			/* L000 */
extern MSG		e_testop;			/* L000 */
