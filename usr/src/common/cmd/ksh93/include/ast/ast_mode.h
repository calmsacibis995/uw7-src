#ident	"@(#)ksh93:include/ast/ast_mode.h	1.1"
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
/* : : generated from features/mode.c by iffe version 05/09/95 : : */
#ifndef _def_mode_ast
#define _def_mode_ast	1
#define S_ITYPE(m)	((m)&S_IFMT)
#define S_ISLNK(m)	(S_ITYPE(m)==S_IFLNK)
#define S_ISSOCK(m)	(S_ITYPE(m)==S_IFSOCK)

#define S_IPERM		(S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO)

#define _S_IDPERM	1
#define _S_IDTYPE	1

#define BUFFERSIZE	8192

#endif
