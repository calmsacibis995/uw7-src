#ident	"@(#)ksh93:src/cmd/ksh93/data/testops.c	1.1"
#pragma prototyped

/*
 * tables for the test builin [[...]] and [...]
 */

#include	"shtable.h"
#include	"test.h"

/*
 * This is the list of binary test and [[...]] operators
 */

const Shtable_t shtab_testops[] =
{
		"!=",		TEST_SNE,
		"-a",		TEST_AND,
		"-ef",		TEST_EF,
		"-eq",		TEST_EQ,
		"-ge",		TEST_GE,
		"-gt",		TEST_GT,
		"-le",		TEST_LE,
		"-lt",		TEST_LT,
		"-ne",		TEST_NE,
		"-nt",		TEST_NT,
		"-o",		TEST_OR,
		"-ot",		TEST_OT,
		"=",		TEST_SEQ,
		"==",		TEST_SEQ,
		"<",		TEST_SLT,
		">",		TEST_SGT,
		"]]",		TEST_END,
		"",		0
};

const char test_opchars[]	= "HLSVOGCaeohrwxdcbfugkpsnzt";
const char e_argument[]		= "argument expected";
const char e_argument_id[]	= ":9";
const char e_missing[]		= "%s missing";
const char e_missing_id[]	= ":202";
const char e_badop[]		= "%s: unknown operator";
const char e_badop_id[]		= ":203";
const char e_tstbegin[]		= "[[ ! ";
const char e_tstend[]		= " ]]\n";
