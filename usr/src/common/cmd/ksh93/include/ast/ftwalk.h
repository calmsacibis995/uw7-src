#ident	"@(#)ksh93:include/ast/ftwalk.h	1.1"
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
 * K. P. Vo
 * AT&T Bell Laboratories
 *
 * ftwalk(3) interface definitions
 */

#ifndef _FTWALK_H
#if !defined(__PROTO__)
#include <prototyped.h>
#endif

#define _FTWALK_H

#include <ls.h>

/*
 * ftwalk() argument flags
 */

#define FTW_CHILDREN	(1<<0)	/* call user function on child dirs too	*/
#define FTW_DELAY	(1<<1)	/* do child stat after parent preorder	*/
#define FTW_DOT		(1<<2)	/* don't chdir(2) to subdirectories	*/
#define FTW_MULTIPLE	(1<<3)	/* path arg is 0 terminated char** argv	*/
#define FTW_PHYSICAL	(1<<4)	/* physical rather than logical walk	*/
#define FTW_POST	(1<<5)	/* visit descendants before parent	*/
#define FTW_TWICE	(1<<6)	/* visit parent before&after descendants*/

#define FTW_META	(1<<7)	/* follow top dir symlinks even if phys	*/

#define FTW_USER	(1<<10)	/* first user flag bit			*/

typedef struct FTW Ftw_t;

struct FTW			/* user function arg			*/
{
	Ftw_t*		left;	/* left child in cycle check tree	*/
	Ftw_t*		right;	/* right child in cycle check tree	*/
	Ftw_t*		link;	/* identical to this elt on search path */
	Ftw_t*		parent;	/* parent in current search path	*/
	union
	{
	long		number;	/* local number				*/
	char*		pointer;/* local pointer			*/
	}		local;	/* local user data			*/
	struct stat	statb;	/* stat buffer of this object		*/
	char*		path;	/* full pathname			*/
	short		pathlen;/* strlen(path)				*/
	unsigned short	info;	/* FTW_* type bits			*/
	unsigned short	status;	/* user function entry/return status	*/
	short		level;	/* current tree depth			*/
	short		namelen;/* strlen(name)				*/
	char		name[sizeof(int)];	/* file base name	*/
};

/*
 * individual Ftw_t size 
 */

#define FTWSIZE(f)	((f)->namelen+1+sizeof(Ftw_t)-sizeof(int))

/*
 * Ftw_t.info type bits
 */

#define	FTW_NS		(1<<0)	/* stat failed - unknown		*/
#define	FTW_F		(1<<1)	/* file - not directory or symbolic link*/
#define FTW_SL		(1<<2)	/* symbolic link			*/
#define	FTW_D		(1<<3)	/* directory - pre-order visit		*/

#define FTW_C		(1<<4)	/* causes cycle				*/
#define FTW_NR		(1<<5)	/* cannot read				*/
#define FTW_NX		(1<<6)	/* cannot search			*/
#define FTW_P		(1<<7)	/* post-order visit			*/

#define FTW_DC	(FTW_D|FTW_C)	/* directory - would cause cycle	*/
#define	FTW_DNR	(FTW_D|FTW_NR)	/* directory - no read permission	*/
#define	FTW_DNX	(FTW_D|FTW_NX)	/* directory - no search permission	*/
#define	FTW_DP	(FTW_D|FTW_P)	/* directory - post-order visit		*/

/*
 * Ftw_t.status entry values
 */

#define FTW_NAME	(1<<0)	/* use Ftw_t.name instead of Ftw_t.path	*/
#define FTW_PATH	(1<<1)	/* use Ftw_t.path instead of Ftw_t.name	*/

/*
 * Ftw_t.status return values
 */

#define FTW_AGAIN	(1<<2)	/* process entry again			*/
#define FTW_FOLLOW	(1<<3)	/* follow FTW_SL symlink		*/
#define FTW_NOPOST	(1<<4)	/* skip post order visit		*/
#define FTW_SKIP	(1<<5)	/* skip FTW_D directory			*/
#define FTW_STAT	(1<<6)	/* userf did stat			*/

extern __MANGLE__ int	ftwalk __PROTO__((const char*, int(*)(Ftw_t*), int, int(*)(Ftw_t*, Ftw_t*)));
extern __MANGLE__ int	ftwflags __PROTO__((void));

#endif