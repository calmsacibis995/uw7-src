#ident	"@(#)ksh93:src/lib/libast/string/fmtperm.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * return strperm() expression for perm
 */

#include <ast.h>
#include <ls.h>

char*
fmtperm(register int perm)
{
	register char*	s;
	register char*	p;
	char*		o;
	int		c;

	static char	buf[32];

	s = buf;

	/*
	 * ugo
	 */

	p = s;
	*s++ = c = '=';
	o = s;
	if (perm & S_ISVTX) *s++ = 't';
	if ((perm & (S_ISUID|S_ISGID)) == (S_ISUID|S_ISGID))
	{
		perm &= ~(S_ISUID|S_ISGID);
		*s++ = 's';
	}
	if ((perm & (S_IRUSR|S_IRGRP|S_IROTH)) == (S_IRUSR|S_IRGRP|S_IROTH))
	{
		perm &= ~(S_IRUSR|S_IRGRP|S_IROTH);
		*s++ = 'r';
	}
	if ((perm & (S_IWUSR|S_IWGRP|S_IWOTH)) == (S_IWUSR|S_IWGRP|S_IWOTH))
	{
		perm &= ~(S_IWUSR|S_IWGRP|S_IWOTH);
		*s++ = 'w';
	}
	if ((perm & (S_IXUSR|S_IXGRP|S_IXOTH)) == (S_IXUSR|S_IXGRP|S_IXOTH))
	{
		perm &= ~(S_IXUSR|S_IXGRP|S_IXOTH);
		*s++ = 'x';
	}
	if (s == o) s = p;
	else c = '+';

	/*
	 * ug
	 */

	p = s;
	if (s > buf) *s++ = ',';
	*s++ = 'u';
	*s++ = 'g';
	*s++ = c;
	o = s;
	if ((perm & (S_IRUSR|S_IRGRP)) == (S_IRUSR|S_IRGRP))
	{
		perm &= ~(S_IRUSR|S_IRGRP);
		*s++ = 'r';
	}
	if ((perm & (S_IWUSR|S_IWGRP)) == (S_IWUSR|S_IWGRP))
	{
		perm &= ~(S_IWUSR|S_IWGRP);
		*s++ = 'w';
	}
	if ((perm & (S_IXUSR|S_IXGRP)) == (S_IXUSR|S_IXGRP))
	{
		perm &= ~(S_IXUSR|S_IXGRP);
		*s++ = 'x';
	}
	if (s == o) s = p;
	else c = '+';

	/*
	 * u
	 */

	p = s;
	if (s > buf) *s++ = ',';
	*s++ = 'u';
	*s++ = c;
	o = s;
	if (perm & S_ISUID) *s++ = 's';
	if (perm & S_IRUSR) *s++ = 'r';
	if (perm & S_IWUSR) *s++ = 'w';
	if (perm & S_IXUSR) *s++ = 'x';
	if (s == o) s = p;
	else c = '+';

	/*
	 * go
	 */

	p = s;
	if (s > buf) *s++ = ',';
	*s++ = 'g';
	*s++ = 'o';
	*s++ = c;
	o = s;
	if ((perm & (S_IRGRP|S_IROTH)) == (S_IRGRP|S_IROTH))
	{
		perm &= ~(S_IRGRP|S_IROTH);
		*s++ = 'r';
	}
	if ((perm & (S_IWGRP|S_IWOTH)) == (S_IWGRP|S_IWOTH))
	{
		perm &= ~(S_IWGRP|S_IWOTH);
		*s++ = 'w';
	}
	if ((perm & (S_IXGRP|S_IXOTH)) == (S_IXGRP|S_IXOTH))
	{
		perm &= ~(S_IXGRP|S_IXOTH);
		*s++ = 'x';
	}
	if (s == o) s = p;
	else c = '+';

	/*
	 * g
	 */

	p = s;
	if (s > buf) *s++ = ',';
	*s++ = 'g';
	*s++ = c;
	o = s;
	if (perm & S_ISGID) *s++ = 's';
	if (perm & S_IRGRP) *s++ = 'r';
	if (perm & S_IWGRP) *s++ = 'w';
	if (perm & S_IXGRP) *s++ = 'x';
	if (s == o) s = p;
	else c = '+';

	/*
	 * o
	 */

	p = s;
	if (s > buf) *s++ = ',';
	*s++ = 'o';
	*s++ = '+';
	o = s;
	if (perm & S_IROTH) *s++ = 'r';
	if (perm & S_IWOTH) *s++ = 'w';
	if (perm & S_IXOTH) *s++ = 'x';
	if (s == o) s = p;
	if (s == buf) *s++ = '0';
	*s = 0;
	return(buf);
}

char *
fmtperm_pos(register int perm)
{
	static char	buf[32];
	char		*next = buf;

	*next++ = 'u';
	*next++ = '=';
	if (perm & S_IRUSR)
		*next++ = 'r';
	if (perm & S_IWUSR)
		*next++ = 'w';
	if (perm & S_IXUSR)
		*next++ = 'x';
	*next++ = ',';
	*next++ = 'g';
	*next++ = '=';
	if (perm & S_IRGRP)
		*next++ = 'r';
	if (perm & S_IWGRP)
		*next++ = 'w';
	if (perm & S_IXGRP)
		*next++ = 'x';
	*next++ = ',';
	*next++ = 'o';
	*next++ = '=';
	if (perm & S_IROTH)
		*next++ = 'r';
	if (perm & S_IWOTH)
		*next++ = 'w';
	if (perm & S_IXOTH)
		*next++ = 'x';
	*next++ = '\0';

	return(buf);
}
