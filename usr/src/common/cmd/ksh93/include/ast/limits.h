#ident	"@(#)ksh93:include/ast/limits.h	1.1"
/***************************************************************
*                                                              *
*                      AT&T - PROPRIETARY                      *
*                                                              *
*         THIS IS PROPRIETARY SOURCE CODE LICENSED BY          *
*                          AT&T CORP.                          *
*                                                              *
*                Copyright (c) 1995 AT&T Corp.                 *
*                     All Rights Reserved                      *
*                                                              *
*           This software is licensed by AT&T Corp.            *
*       under the terms and conditions of the license in       *
*       http://www.research.att.com/orgs/ssr/book/reuse        *
*                                                              *
*               This software was created by the               *
*           Software Engineering Research Department           *
*                    AT&T Bell Laboratories                    *
*                                                              *
*               For further information contact                *
*                     gsf@research.att.com                     *
*                                                              *
***************************************************************/
/* : : generated from features/limits.c by iffe version 05/09/95 : : */
#ifndef _def_limits_ast
#define _def_limits_ast	1

#undef	CHAR_BIT
#define CHAR_BIT	8
#undef	MB_LEN_MAX
#define MB_LEN_MAX	6
#undef	UCHAR_MAX
#if defined(__STDC__)
#define UCHAR_MAX	255U
#else
#define UCHAR_MAX	255
#endif
#undef	SCHAR_MIN
#define SCHAR_MIN	-128
#undef	SCHAR_MAX
#define SCHAR_MAX	127
#undef	CHAR_MIN
#define CHAR_MIN	SCHAR_MIN
#undef	CHAR_MAX
#define CHAR_MAX	SCHAR_MAX
#undef	USHRT_MAX
#if defined(__STDC__)
#define USHRT_MAX	65535U
#else
#define USHRT_MAX	65535
#endif
#undef	SHRT_MIN
#define SHRT_MIN	-32768
#undef	SHRT_MAX
#define SHRT_MAX	32767
#undef	UINT_MAX
#if defined(__STDC__)
#define UINT_MAX	4294967295U
#else
#define UINT_MAX	4294967295
#endif
#undef	INT_MIN
#define INT_MIN		(-2147483647-1)
#undef	INT_MAX
#define INT_MAX		2147483647
#undef	ULONG_MAX
#define ULONG_MAX	UINT_MAX
#undef	LONG_MIN
#define LONG_MIN	INT_MIN
#undef	LONG_MAX
#define LONG_MAX	INT_MAX

#define AIO_LISTIO_MAX		2
#define _POSIX_AIO_LISTIO_MAX	2
#define AIO_MAX		1
#define _POSIX_AIO_MAX	1
#define ARG_MAX		306907
#define _POSIX_ARG_MAX	4096
#define ATEXIT_MAX		32
#define _XOPEN_ATEXIT_MAX	32
#define BC_BASE_MAX		2147483647
#define _POSIX2_BC_BASE_MAX	99
#define BC_DIM_MAX		2048
#define _POSIX2_BC_DIM_MAX	2048
#define BC_SCALE_MAX		99
#define _POSIX2_BC_SCALE_MAX	99
#define BC_STRING_MAX		1000
#define _POSIX2_BC_STRING_MAX	1000
#define CHARCLASS_NAME_MAX		100
#define CHILD_MAX		200
#define _POSIX_CHILD_MAX	6
#define _AST_CLK_TCK	60
#define _POSIX_CLOCKRES_MIN	1
#define COLL_WEIGHTS_MAX		14
#define _POSIX2_COLL_WEIGHTS_MAX	2
#define DELAYTIMER_MAX		32
#define _POSIX_DELAYTIMER_MAX	32
#define EXPR_NEST_MAX		32
#define _POSIX2_EXPR_NEST_MAX	32
#define FCHR_MAX		2147483647
#define _SVID_FCHR_MAX	2147483647
#define IOV_MAX		16
#define _XOPEN_IOV_MAX	16
#define LINE_MAX		2048
#define _POSIX2_LINE_MAX	2048
#define LINK_MAX		1000
#define _POSIX_LINK_MAX	8
#define _SVID_LOGNAME_MAX	8
#define LONG_BIT		32
#define MAX_CANON		256
#define _POSIX_MAX_CANON	255
#define MAX_INPUT		5120
#define _POSIX_MAX_INPUT	255
#define MQ_OPEN_MAX		8
#define _POSIX_MQ_OPEN_MAX	8
#define MQ_PRIO_MAX		32
#define _POSIX_MQ_PRIO_MAX	32
#define NAME_MAX		255
#define _POSIX_NAME_MAX	14
#define NGROUPS_MAX		16
#define _POSIX_NGROUPS_MAX	0
#define NL_ARGMAX		9
#define NL_LANGMAX		14
#define NL_MSGMAX		32767
#define NL_SETMAX		255
#define NL_TEXTMAX		2048
#define NZERO		20
#define OPEN_MAX		128
#define _POSIX_OPEN_MAX	16
#define OPEN_MAX_CEIL		128
#define PAGESIZE		4096
#define PAGE_SIZE		4096
#define _SVID_PASS_MAX	8
#define _POSIX_PATH	"/bin:/usr/bin"
#define PATH_MAX		1024
#define _POSIX_PATH_MAX	255
#define PID_MAX		30000
#define _SVID_PID_MAX	30000
#define PIPE_BUF		16384
#define _POSIX_PIPE_BUF	512
#define RE_DUP_MAX		5100
#define _POSIX2_RE_DUP_MAX	255
#define RTSIG_MAX		8
#define _POSIX_RTSIG_MAX	8
#define SEM_NSEMS_MAX		256
#define _POSIX_SEM_NSEMS_MAX	256
#define SEM_VALUE_MAX		32767
#define _POSIX_SEM_VALUE_MAX	32767
#define _AST_SHELL	"/bin/sh"
#define SIGQUEUE_MAX		32
#define _POSIX_SIGQUEUE_MAX	32
#define SSIZE_MAX		2147483647
#define _POSIX_SSIZE_MAX	32767
#define STD_BLK		1024
#define _SVID_STD_BLK	1024
#define STREAM_MAX		128
#define _POSIX_STREAM_MAX	8
#define SYMLINK_MAX		1023
#define _POSIX_SYMLINK_MAX	255
#define SYMLOOP_MAX		8
#define _POSIX_SYMLOOP_MAX	8
#define SYSPID_MAX		1
#define _SVID_SYSPID_MAX	2
#define TIMER_MAX		32
#define _POSIX_TIMER_MAX	32
#define _AST_TMP	"/tmp"
#define TZNAME_MAX		8
#define _POSIX_TZNAME_MAX	3
#define UID_MAX		60002
#define _SVID_UID_MAX	60002
#define WORD_BIT		32

/*
 * pollution control
 */

#if defined(__STDPP__directive) && defined(__STDPP__ignore)
__STDPP__directive pragma pp:ignore "limits.h"
#else
#endif

#endif
