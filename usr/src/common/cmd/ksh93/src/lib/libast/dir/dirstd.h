#ident	"@(#)ksh93:src/lib/libast/dir/dirstd.h	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 *
 * <dirent.h> for systems with no opendir()
 */

#ifndef _DIRENT_H
#define _DIRENT_H

typedef struct
{
	int		dd_fd;		/* file descriptor		*/
#ifdef _DIR_PRIVATE_
	_DIR_PRIVATE_
#endif
} DIR;

struct dirent
{
	long		d_fileno;	/* entry serial number		*/
	int		d_reclen;	/* entry length			*/
	int		d_namlen;	/* entry name length		*/
	char		d_name[1];	/* entry name			*/
};

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
