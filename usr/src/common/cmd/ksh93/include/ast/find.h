#ident	"@(#)ksh93:include/ast/find.h	1.1"
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
 * fast find interface definitions
 */

#ifndef _FIND_H
#if !defined(__PROTO__)
#include <prototyped.h>
#endif

#define _FIND_H

extern __MANGLE__ __V_*		findopen __PROTO__((const char*));
extern __MANGLE__ char*		findnext __PROTO__((__V_*));
extern __MANGLE__ void		findclose __PROTO__((__V_*));

#endif