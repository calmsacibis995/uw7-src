#ident	"@(#)ksh93:src/lib/libast/preroot/getpreroot.c	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 * return the real absolute pathname of the preroot dir for cmd
 * if cmd==0 then current preroot path returned
 */

#include <ast.h>
#include <preroot.h>

#if FS_PREROOT

#include "dirlib.h"
#include <ls.h>
#include <error.h>
#include <stdio.h>

#ifndef ERANGE
#define ERANGE		E2BIG
#endif

#define ERROR(e)	{errno=e;goto error;}

char*
getpreroot(char* path, const char* cmd)
{
	register int	c;
	register FILE*	fp;
	register char*	p;
	char		buf[PATH_MAX];

	if (!path) path = buf;
	if (cmd)
	{
		sfsprintf(buf, sizeof(buf), "set x `%s= %s - </dev/null 2>&1`\nwhile :\ndo\nshift\ncase $# in\n[012]) break ;;\nesac\ncase \"$1 $2\" in\n\"+ %s\")	echo $3; exit ;;\nesac\ndone\necho\n", PR_SILENT, cmd, PR_COMMAND);
		if (!(fp = popen(buf, "rug"))) return(0);
		for (p = path; (c = getc(fp)) != EOF && c != '\n'; *p++ = c);
		*p = 0;
		pclose(fp);
		if (path == p) return(0);
		return(path == buf ? strdup(path) : path);
	}
	else
	{
		char*		d;
		DIR*		dirp = 0;
		int		namlen;
		int		euid;
		int		ruid;
		struct dirent*	entry;
		struct stat*	cur;
		struct stat*	par;
		struct stat*	tmp;
		struct stat	curst;
		struct stat	parst;
		struct stat	tstst;
		char		dots[PATH_MAX];

		cur = &curst;
		par = &parst;
		if ((ruid = getuid()) != (euid = geteuid())) setuid(ruid);
		if (stat(PR_REAL, cur) || stat("/", par) || cur->st_dev == par->st_dev && cur->st_ino == par->st_ino) ERROR(ENOTDIR);

		/*
		 * like getcwd() but starting at the preroot
		 */

		d = dots;
		*d++ = '/';
		p = path + PATH_MAX - 1;
		*p = 0;
		for (;;)
		{
			tmp = cur;
			cur = par;
			par = tmp;
			if ((d - dots) > (PATH_MAX - 4)) ERROR(ERANGE);
			*d++ = '.';
			*d++ = '.';
			*d = 0;
			if (!(dirp = opendir(dots))) ERROR(errno);
#if !_dir_ok || _mem_dd_fd_DIR
			if (fstat(dirp->dd_fd, par)) ERROR(errno);
#else
			if (stat(dots, par)) ERROR(errno);
#endif
			*d++ = '/';
			if (par->st_dev == cur->st_dev)
			{
				if (par->st_ino == cur->st_ino)
				{
					closedir(dirp);
					*--p = '/';
					if (ruid != euid) setuid(euid);
					if (path == buf) return(strdup(p));
					if (path != p)
					{
						d = path;
						while (*d++ = *p++);
					}
					return(path);
				}
#if _mem_d_fileno_dirent || _mem_d_ino_dirent
#if !_mem_d_fileno_dirent
#define d_fileno	d_ino
#endif
				while (entry = readdir(dirp))
					if (entry->d_fileno == cur->st_ino)
					{
#if _mem_d_namlen_dirent
						namlen = entry->d_namlen;
#else
						namlen = strlen(entry->d_name);
#endif
						goto found;
					}
	
				/*
				 * this fallthrough handles logical naming
				 */

				rewinddir(dirp);
#endif
			}
			do
			{
				if (!(entry = readdir(dirp))) ERROR(ENOENT);
#if _mem_d_namlen_dirent
				namlen = entry->d_namlen;
#else
				namlen = strlen(entry->d_name);
#endif
				if ((d - dots) > (PATH_MAX - 1 - namlen)) ERROR(ERANGE);
				memcpy(d, entry->d_name, namlen + 1);
				if (stat(dots, &tstst)) ERROR(errno);
			} while (tstst.st_ino != cur->st_ino || tstst.st_dev != cur->st_dev);
		found:
			if (*p) *--p = '/';
			if ((p -= namlen) <= (path + 1)) ERROR(ERANGE);
			memcpy(p, entry->d_name, namlen);
			closedir(dirp);
			dirp = 0;
		}
	error:
		if (dirp) closedir(dirp);
		if (ruid != euid) setuid(euid);
	}
	return(0);
}

#else

NoN(getpreroot)

#endif
