/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*		copyright	"%c%" 	*/

#ident	"@(#)psfile.c	1.2"
#ident	"$Header$"

#include <fcntl.h>

#define	PSCOM	"%!"
#define	WIN_PSCOM	"%!" /* This format has leading CNTL-D */

#if defined(__STDC__)
psfile(char * fname)
#else
psfile(fname)
char	*fname;
#endif
{
	int		fd;
	register int	ret = 0;
	char		buf[sizeof(PSCOM)-1];

	if ((fd = open(fname, O_RDONLY)) >= 0 &&
    	    read(fd, buf, sizeof(buf)) == sizeof(buf) &&
    	    (strncmp(buf, PSCOM, sizeof(buf)) == 0 ||
	    strncmp(buf, WIN_PSCOM, sizeof(buf)) == 0))
			ret++;
	(void)close(fd);
	return(ret);
}
