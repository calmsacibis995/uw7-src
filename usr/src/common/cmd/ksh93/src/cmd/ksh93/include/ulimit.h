#ident	"@(#)ksh93:src/cmd/ksh93/include/ulimit.h	1.1"
#pragma prototyped
#ifndef _ULIMIT_H
#define _ULIMIT_H 1
/*
 * This is for the ulimit built-in command
 */

#include	"shtable.h"
#include	"FEATURE/rlimits"
#if defined(_sys_resource) && defined(_lib_getrlimit)
#   include	"FEATURE/time"
#   include	<sys/resource.h>
#   if !defined(RLIMIT_FSIZE) && defined(_sys_vlimit)
	/* This handles hp/ux problem */ 
#	include	<sys/vlimit.h>
#	define RLIMIT_FSIZE	(LIM_FSIZE-1)
#	define RLIMIT_DATA	(LIM_DATA-1)
#	define RLIMIT_STACK	(LIM_STACK-1)
#	define RLIMIT_CORE	(LIM_CORE-1)
#	define RLIMIT_CPU	(LIM_CPU-1)
#	ifdef LIM_MAXRSS
#		define RLIMIT_RSS       (LIM_MAXRSS-1)
#	endif /* LIM_MAXRSS */
#   endif
#   undef _lib_ulimit
#else
#   ifdef _sys_vlimit
#	include	<sys/vlimit.h>
#	undef _lib_ulimit
#	define RLIMIT_FSIZE	LIM_FSIZE
#	define RLIMIT_DATA	LIM_DATA
#	define RLIMIT_STACK	LIM_STACK
#	define RLIMIT_CORE	LIM_CORE
#	define RLIMIT_CPU	LIM_CPU
#	ifdef LIM_MAXRSS
#		define RLIMIT_RSS       LIM_MAXRSS
#	endif /* LIM_MAXRSS */
#   else
#	ifdef _lib_ulimit
#	    define vlimit ulimit
#	endif /* _lib_ulimit */
#   endif /* _lib_vlimit */
#endif

#ifdef RLIM_INFINITY
#   define INFINITY	RLIM_INFINITY
#else
#   ifndef INFINITY
#	define INFINITY	-1L
#   endif /* INFINITY */
#endif /* RLIM_INFINITY */

#if defined(_lib_getrlimit) || defined(_lib_vlimit) || defined(_lib_ulimit)
#   ifndef RLIMIT_CPU
#	define RLIMIT_CPU	0
#   endif /* !RLIMIT_CPU */
#   ifndef RLIMIT_DATA
#	define RLIMIT_DATA	0
#   endif /* !RLIMIT_DATA */
#   ifndef RLIMIT_RSS
#	define RLIMIT_RSS	0
#   endif /* !RLIMIT_RSS */
#   ifndef RLIMIT_STACK
#	define RLIMIT_STACK	0
#   endif /* !RLIMIT_STACK */
#   ifndef RLIMIT_CORE
#	define RLIMIT_CORE	0
#   endif /* !RLIMIT_CORE */
#   ifndef RLIMIT_VMEM
#	define RLIMIT_VMEM	0
#   endif /* !RLIMIT_VMEM */
#   ifndef RLIMIT_NOFILE
#	define RLIMIT_NOFILE	0
#   endif /* !RLIMIT_NOFILE */
#else
#   define _no_ulimit
#endif
extern const char		e_unlimited[];

#endif /* _ULIMIT_H */
