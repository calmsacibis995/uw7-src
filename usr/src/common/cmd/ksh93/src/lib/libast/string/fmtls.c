#ident	"@(#)ksh93:src/lib/libast/string/fmtls.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * ls formatter
 */

#include <ast.h>
#include <ls.h>
#include <tm.h>

#ifndef LS_W_MAX
#define LS_W_MAX	128
#endif

/*
 * ls formatter
 *
 *	buf	results placed here
 *	name	file name
 *	st	file stat buffer
 *	info	optional info
 *	link	link text if != 0
 *	flags	LS_* flags
 *
 *	return	end of formatted buf
 */

char*
fmtls(char* buf, const char* name, register struct stat* st, const char* info, const char* link, register int flags)
{
	register char*		s;
	time_t			tm;

	s = buf;
	if (flags & LS_INUMBER)
		s += sfsprintf(s, LS_W_MAX, "%*lu ", LS_W_INUMBER - 1, (unsigned long)st->st_ino);
	if (flags & LS_BLOCKS)
		s += sfsprintf(s, LS_W_MAX, "%*lu ", LS_W_BLOCKS - 1, (unsigned long)iblocks(st));
	if (flags & LS_LONG)
	{
		s += sfsprintf(s, LS_W_MAX, "%s%3u", fmtmode(st->st_mode, flags & LS_EXTERNAL), (unsigned int)st->st_nlink);
		if (!(flags & LS_NOUSER))
		{
			if (flags & LS_NUMBER) s += sfsprintf(s, LS_W_MAX, " %-*d", LS_W_NAME - 1, st->st_uid);
			else s += sfsprintf(s, LS_W_MAX, " %-*s", LS_W_NAME - 1, fmtuid(st->st_uid));
		}
		if (!(flags & LS_NOGROUP))
		{
			if (flags & LS_NUMBER) s += sfsprintf(s, LS_W_MAX, " %-*d", LS_W_NAME - 1, st->st_gid);
			else s += sfsprintf(s, LS_W_MAX, " %-*s", LS_W_NAME - 1, fmtgid(st->st_gid));
		}
		if (S_ISBLK(st->st_mode) || S_ISCHR(st->st_mode)) s += sfsprintf(s, LS_W_MAX, "%8s ", fmtdev(st));
		else s += sfsprintf(s, LS_W_MAX, "%8lu ", (unsigned long)st->st_size);
		tm = (flags & LS_ATIME) ? st->st_atime : (flags & LS_CTIME) ? st->st_ctime : st->st_mtime;
		s = tmform(s, "%?%l", &tm);
		*s++ = ' ';
	}
	if (info)
	{
		while (*s = *info++) s++;
		*s++ = ' ';
	}
	while (*s = *name++) s++;
	if (flags & LS_MARK)
	{
		if (S_ISDIR(st->st_mode)) *s++ = '/';
#ifdef S_ISLNK
		else if (S_ISLNK(st->st_mode)) *s++ = '@';
#endif
		else if (st->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) *s++ = '*';
	}
	if (link)
	{
		s += sfsprintf(s, LS_W_MAX, " %s %s",
#ifdef S_ISLNK
			S_ISLNK(st->st_mode) ? "->" :
#endif
				"==", link);
	}
	*s = 0;
	return(s);
}
