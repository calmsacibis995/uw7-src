#ident	"@(#)lprof:libprof/i386/mach_type.h	1.2"

#define	MACH_TYPE	M386
#define CANAME		"__coverage."
#define COV_PREFIX	"__coverage."
#define PC   		EIP

/*
*	NOTE: When you change MATCH_STR, also change pcrt1.s.
*	(See notes on "verify_match()" in file symintLoad.c.)
*/
#if defined(__STDC__)
#define	MATCH_NAME	_edata
#define	MATCH_STR	"_edata"
#else
#define	MATCH_NAME	edata
#define	MATCH_STR	"edata"
#endif


