#ident	"@(#)ksh93:src/lib/libast/dir/dirlib.h	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 *
 * directory stream access library private definitions
 * library routines should include this file rather than <dirent.h>
 */

#ifndef _DIRLIB_H
#define _DIRLIB_H

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide getdents
#else
#define getdents	______getdents
#endif

#include <ast.h>
#include <errno.h>

#if _lib_opendir && ( _hdr_dirent || _hdr_ndir || _sys_dir )

#define _dir_ok		1

#include <ls.h>
#ifndef _DIRENT_H
#if _hdr_dirent
#include <dirent.h>
#else
#if _hdr_ndir
#include <ndir.h>
#else
#include <sys/dir.h>
#endif
#ifndef dirent
#define dirent	direct
#endif
#endif
#endif

#else

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide DIR closedir dirent opendir readdir seekdir telldir
#else
#define DIR		______DIR
#define closedir	______closedir
#define dirent		______dirent
#define opendir		______opendir
#define readdir		______readdir
#define seekdir		______seekdir
#define telldir		______telldir
#endif

#include <ast_param.h>

#include <ls.h>
#include <limits.h>
#include <sys/dir.h>

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide DIR closedir dirent opendir readdir seekdir telldir
#else
#undef	DIR
#undef	closedir
#undef	dirent
#undef	opendir
#undef	readdir
#undef	seekdir
#undef	telldir
#endif

#define _DIR_PRIVATE_ \
	int		dd_loc;		/* offset in block		*/ \
	int		dd_size;	/* valid data in block		*/ \
	char*		dd_buf;		/* directory block		*/

#include "dirstd.h"

#ifndef	DIRBLKSIZ
#ifdef	DIRBLK
#define DIRBLKSIZ	DIRBLK
#else
#ifdef	DIRBUF
#define DIRBLKSIZ	DIRBUF
#else
#define DIRBLKSIZ	8192
#endif
#endif
#endif

#endif

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide getdents
#else
#undef	getdents
#endif

#ifndef errno
extern int	errno;
#endif

extern ssize_t		getdents(int, void*, size_t);

#endif
