#ident	"@(#)ksh93:src/lib/libast/dir/dirsys.h	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 *
 * <dirent.h> for systems with opendir() and no <ndir.h>
 */

#ifndef _DIRENT_H
#define _DIRENT_H

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide closedir opendir readdir seekdir telldir
#else
#define closedir	______closedir
#define opendir		______opendir
#define readdir		______readdir
#define seekdir		______seekdir
#define telldir		______telldir
#endif

#include <sys/dir.h>

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide closedir opendir readdir seekdir telldir
#else
#undef	closedir
#undef	opendir
#undef	readdir
#undef	seekdir
#undef	telldir
#endif

#ifndef dirent
#define dirent	direct
#endif

#if !defined(d_fileno) && !defined(d_ino)
#define d_fileno	d_ino
#endif

#ifdef	rewinddir
#undef	rewinddir
#define rewinddir(p)	seekdir(p,0L)
#endif

extern DIR*		opendir(const char*);
extern void		closedir(DIR*);
extern struct dirent*	readdir(DIR*);
extern void		seekdir(DIR*, long);
extern long		telldir(DIR*);

#endif
