/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/itoa.c	1.2.4.3"

char *
itoa(n, base)
long	n;			/* abs k16 */
int	base;
{
	register char	*p;
	register int	minus;
	static char	buf[36];

	p = &buf[36];
	*--p = '\0';
	if (n < 0) {
		minus = 1;
		n = -n;
	}
	else
		minus = 0;
	if (n == 0)
		*--p = '0';
	else
		while (n > 0) {
			*--p = "0123456789abcdef"[n % base];
			n /= base;
		}
	if (minus)
		*--p = '-';
	return p;
}
