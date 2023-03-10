#ident	"@(#)ksh93:include/ast/keyprintf.h	1.1"
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
 * keyword printf definitions
 */

#ifndef _KEYPRINTF_H
#if !defined(__PROTO__)
#include <prototyped.h>
#endif

#define _KEYPRINTF_H

#include <ast.h>

typedef int (*Key_lookup_t) __PROTO__((__V_*, const char*, char**, long*, const char*, int));

typedef char* (*Key_convert_t) __PROTO__((__V_*, int, const char*, char*, long, int));

extern __MANGLE__ int	keyprintf __PROTO__((Sfio_t*, __V_*, const char*, Key_lookup_t, Key_convert_t));

#endif
