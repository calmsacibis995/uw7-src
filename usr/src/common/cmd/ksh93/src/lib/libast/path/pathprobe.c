#ident	"@(#)ksh93:src/lib/libast/path/pathprobe.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * return in path the full path name of the probe(1)
 * information for lang and tool using proc
 * if attr != 0 then path attribute assignments placed here
 *
 * if path==0 then the space is malloc'd
 *
 * op:
 *
 *	-1	return path name with no generation
 *	0	verbose probe
 *	1	silent probe
 *
 * 0 returned if the info does not exist and cannot be generated
 */

#include <ast.h>
#include <error.h>
#include <ls.h>
#include <proc.h>

#ifndef PROBE
#define PROBE		"probe"
#endif

char*
pathprobe(char* path, char* attr, const char* lang, const char* tool, const char* aproc, int op)
{
	char*		proc = (char*)aproc;
	register char*	p;
	register char*	k;
	register char**	ap;
	int		n;
	char*		e;
	char*		probe;
	char		buf[PATH_MAX];
	char		cmd[PATH_MAX];
	char		lib[PATH_MAX];
	char*		arg[6];
	unsigned long	ptime;
	struct stat	st;

	if (*proc != '/')
	{
		if (p = strchr(proc, ' '))
		{
			n = p - proc;
			proc = strncpy(buf, proc, n);
			*(proc + n) = 0;
		}
		if (!(proc = pathpath(cmd, proc, NiL, PATH_ABSOLUTE|PATH_REGULAR|PATH_EXECUTE)))
			proc = (char*)aproc;
		else if (p)
			strcpy(proc + strlen(proc), p);
	}
	if (!path) path = buf;
	probe = PROBE;
	p = strcopy(lib, "lib/");
	p = strcopy(p, probe);
	*p++ = '/';
	p = strcopy(k = p, lang);
	*p++ = '/';
	p = strcopy(p, tool);
	*p++ = '/';
	e = strcopy(p, probe);
	if (!pathpath(path, lib, "", PATH_ABSOLUTE|PATH_EXECUTE) || stat(path, &st)) return(0);
	ptime = st.st_mtime;
	pathkey(p, attr, lang, proc);
	p = path + strlen(path) - (e - k);
	strcpy(p, k);
	if (op >= 0 && !stat(path, &st))
	{
		if (ptime <= (unsigned long)st.st_mtime || ptime <= (unsigned long)st.st_ctime) op = -1;
		else if (st.st_mode & S_IWUSR)
		{
			if (op == 0) error(0, gettxt(":275","%s probe information for %s language processor %s must be manually regenerated"), tool, lang, proc);
			op = -1;
		}
	}
	if (op >= 0)
	{
		strcpy(p, probe);
		ap = arg;
		*ap++ = path;
		if (op > 0) *ap++ = "-s";
		*ap++ = (char*)lang;
		*ap++ = (char*)tool;
		*ap++ = proc;
		*ap = 0;
		if (procrun(path, arg)) return(0);
		strcpy(p, k);
		if (access(path, R_OK)) return(0);
	}
	return(path == buf ? strdup(path) : path);
}
