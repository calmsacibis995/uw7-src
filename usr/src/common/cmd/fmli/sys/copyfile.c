/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */

#ident	"@(#)fmli:sys/copyfile.c	1.5.3.3"

#include	<stdio.h>
#include	"inc.types.h"	/* abs s14 */
#include	<sys/stat.h>
#include	"wish.h"
#include	<termio.h>
#define        _SYS_TERMIO_H
#include	"exception.h"

/*
 * copy a file
 */
FILE *
cpfile(from, to)
char	*from;
char	*to;
{
	register int	c;
	register FILE	*src;
	register FILE	*dst;

	if ((src = fopen(from, "r")) == NULL)
		return NULL;
	if ((dst = fopen(to, "w+")) == NULL) {
		fclose(src);
		return NULL;
	}
	while ((c = getc(src)) != EOF)
		putc(c, dst);
	if (ferror(src)) {
		fclose(src);
		fclose(dst);
		unlink(to);
		return NULL;
	}
	fclose(src);
	return dst;
}

copyfile(from, to)
char *from;
char *to;
{
	FILE *fp;

	if (fp = cpfile(from, to)) {
		fclose(fp);
		return(0);
	}
	return(-1);
}

movefile(source, target)
register char *source, *target;
{
	char	*dirname();
	struct	stat s1;
	struct	utimbuf	{
		time_t	actime;
		time_t	modtime;
		};
	struct utimbuf times;

#ifdef _DEBUG
	_debug(stderr, "IN MOVEFILE(%s, %s)\n", source, target);
#endif
	if (link(source, target) < 0) {
		if (access(target, 00) != -1)
			return(-1);
		if (stat(source, &s1) < 0) 
			return(-1);
		if (copyfile(source, target) != 0) 
			return(-1);
		times.actime = s1.st_atime;
		times.modtime = s1.st_mtime;
		utime(target, &times);
		chmod(target, s1.st_mode);
		chown(target, geteuid(), getegid());
	}
	if (unlink(source) < 0) 
		return(-1);
	return(0);
}
