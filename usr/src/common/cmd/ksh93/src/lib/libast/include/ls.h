#ident	"@(#)ksh93:src/lib/libast/include/ls.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * ls formatter interface definitions
 */

#ifndef _LS_H
#define _LS_H

#include <ast_fs.h>
#include <ast_mode.h>

/*
 * some systems (could it beee AIX) pollute the std name space
 */

#undef	fileid
#define fileid	fileID

#if _mem_st_blocks_stat
#define iblocks(p)	(((p)->st_blocks+1)/2)
#else
#define iblocks(p)	_iblocks(p)
extern off_t		_iblocks(struct stat*);
#endif

#if _mem_st_rdev_stat
#define idevice(p)	((p)->st_rdev)
#define IDEVICE(p,v)	((p)->st_rdev=(v))
#else
#define idevice(p)	0
#define IDEVICE(p,v)
#endif

#define LS_ATIME	(1<<0)		/* list st_atime		*/
#define LS_BLOCKS	(1<<1)		/* list blocks used by file	*/
#define LS_CTIME	(1<<2)		/* list st_ctime		*/
#define LS_EXTERNAL	(1<<3)		/* st_mode is modex canonical	*/
#define LS_INUMBER	(1<<4)		/* list st_ino			*/
#define LS_LONG		(1<<5)		/* long listing			*/
#define LS_MARK		(1<<6)		/* append file name marks	*/
#define LS_NOGROUP	(1<<7)		/* omit group name for LS_LONG	*/
#define LS_NOUSER	(1<<8)		/* omit user name for LS_LONG	*/
#define LS_NUMBER	(1<<9)		/* number instead of name	*/

#define LS_USER		(1<<10)		/* first user flag bit		*/

#define LS_W_BLOCKS	6		/* LS_BLOCKS field width	*/
#define LS_W_INUMBER	7		/* LS_INUMBER field width	*/
#define LS_W_LONG	55		/* LS_LONG width (w/o names)	*/
#define LS_W_LINK	4		/* link text width (w/o names)	*/
#define LS_W_MARK	1		/* LS_MARK field width		*/
#define LS_W_NAME	9		/* group|user name field width	*/

#if defined(_AST_H) || defined(_POSIX_SOURCE)
#define _AST_mode_t	mode_t
#else
#define _AST_mode_t	int
#endif

extern char*		fmtls(char*, const char*, struct stat*, const char*, const char*, int);
extern int		chmod(const char*, _AST_mode_t);
#if !defined(_ver_fstat)
extern int		fstat(int, struct stat*);
#endif
#if !defined(_ver_lstat)
extern int		lstat(const char*, struct stat*);
#endif
extern int		mkdir(const char*, _AST_mode_t);
extern int		mkfifo(const char*, _AST_mode_t);
#if !defined(_ver_mknod)
extern int		mknod(const char*, _AST_mode_t, dev_t);
#endif
extern int		pathstat(const char*, struct stat*);
#if !defined(_ver_stat)
extern int		stat(const char*, struct stat*);
#endif
extern _AST_mode_t	umask(_AST_mode_t);

#undef	_AST_mode_t

#endif
