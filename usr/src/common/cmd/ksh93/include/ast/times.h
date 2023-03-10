#ident	"@(#)ksh93:include/ast/times.h	1.1"
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

/* : : generated by proto : : */
                  
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * -last clock_t,time_t interface definitions plus
 *
 *	<ast.h>
 *	<time.h>
 *	<sys/time.h>
 *	<sys/times.h>
 */

#ifndef _TIMES_H
#if !defined(__PROTO__)
#include <prototyped.h>
#endif

#define _TIMES_H

#if defined(__STDPP__directive) && defined(__STDPP__note)
#if !noticed(clock_t)
__STDPP__directive pragma pp:note clock_t
#endif
#if !noticed(time_t)
__STDPP__directive pragma pp:note time_t
#endif
#endif

#include <ast.h>

#undef	_TIMES_H
#include <ast_time.h>
#ifndef _TIMES_H
#define _TIMES_H
#endif

#if defined(__STDPP__directive) && defined(__STDPP__note)
#if !noticed(clock_t)
typedef unsigned long clock_t;
#endif
#if !noticed(time_t)
typedef unsigned long time_t;
#endif
#endif

extern __MANGLE__ int		touch __PROTO__((const char*, time_t, time_t, int));

#endif
