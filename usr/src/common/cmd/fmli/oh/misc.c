/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *      All Rights Reserved
 */

#ident	"@(#)fmli:oh/misc.c	1.3.4.4"

#include	<stdio.h>
#include	<string.h>
#include	"wish.h"
#include	"moremacros.h"

/*
 * gets the next line that does not begin with '#' or '\n', and removes
 * the trailing '\n'.  Returns buf, or NULL if EOF is encountered.
 */
char *
get_skip(buf, size, fp)
char	*buf;
int	size;
FILE	*fp;
{
	register char	*p;

	while ((p = fgets(buf, size, fp)) && (buf[0] == '#' || buf[0] == '\n'))
		;
	if (p)
		p[(int)strlen(p) - 1] = '\0';
	return p;
}

/*
 * frees *dst, if already set, and sets it to the strsaved value of the
 * next tab delimited field.  Return value is ptr to char after the tab
 * (which is overwritten by a '\0').  If there is no field or src is
 * NULL, *dst remains unchanged and NULL is returned
 */
char *
tab_parse(dst, src)
char	**dst;
char	*src;
{
	register char	*p;
	char	*strchr();

	if (src == NULL)
		return NULL;
	while (*src == '\t')
		src++;
	if (*src == '\0')
		return NULL;
	if (*dst)
		free(*dst);
	if (p = strchr(src, '\t'))
		*p++ = '\0';
	*dst = strsave(src);
	src = p;
	return src;
}

long
tab_long(src, base)
char	**src;
{
	register char	*p;
	char	*strchr();
	long	strtol();

	if (*src == NULL || **src == '\0') {
		*src == NULL;
		return 0L;
	}
	while (**src == '\t')
		(*src)++;
	return strtol(*src, src, base);
}
