#ident	"@(#)kern-i386:util/subr_f.c	1.11.5.1"
#ident	"$Header$"

/*
 * Miscellaneous family-specific kernel subroutines.
 */

#include <proc/iobitmap.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/seg.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/dl.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

/*
 * C-library string functions.
 * Assembler versions of others are in util/string.s.
 */

/*
 * char *
 * strncpy(char *, const char *, size_t)
 *
 *	Copy argument 2 to argument 1, truncating or null-padding
 *	to always copy argument 3 number of bytes.
 *
 * Calling/Exit State:
 *
 *	Return the first argument.
 */
char *
strncpy(char *s1, const char *s2, size_t n)
{
	char *os1 = s1;

	n++;
	while (--n != 0 && (*s1++ = *s2++) != '\0')
		continue;
	if (n != 0) {
		while (--n != 0)
			*s1++ = '\0';
	}
	return os1;
}

/*
 * int
 * strncmp(const char *, const char *, size_t)
 *
 *	Compare strings.
 *
 * Calling/Exit State:
 *
 *	Compare at most the third argument number of bytes.
 *	Return *s1-*s2 for the last characters in s1 and s2 which
 *	were compared.
 */
int
strncmp(const char *s1, const char *s2, size_t n)
{
	if (s1 == s2)
		return 0;
	n++;
	while (--n != 0 && *s1 == *s2++) {
		if (*s1++ == '\0')
			return 0;
	}
	return (n == 0) ? 0 : *s1 - *--s2;
}

/*
 * char *
 * strpbrk(const char *string, const char *brkset)
 *
 * Calling/Exit State:
 *	Return ptr to first occurance of any character from `brkset'
 *	in the character string `string'; NULL if none exists.
 */
char *
strpbrk(const char *string, const char *brkset)
{
	char *p;

	do {
		for (p = (char *)brkset; *p != '\0' && *p != *string; ++p)
			;
		if (*p != '\0')
			return (char *)string;
	} while (*string++);

	return (char *)NULL;
}

/*
 * char *
 * strchr(const char *s, int c)
 *	Find first occurrence of character, c, in string, s.
 *
 * Calling/Exit State:
 *	Returns pointer into s, or NULL if no occurrence found.
 */
char *
strchr(const char *s, int c)
{
	while (*s) {
		if (*s == c)
			return (char *)s;
		++s;
	}
	return NULL;
}

/*
 * unsigned long
 * strtoul(const char *str, char **ep, int base)
 *	Parse an ASCII number from the front of a string.
 *
 * Calling/Exit State:
 *	Returns the value of the number at str, in the specified base
 *	(2-36; 0 to depend on string prefix); if ep is non-NULL,
 *	*ep is set to point after the number.
 *
 *	This code was derived more-or-less mechanically from the 01/06/97
 *	libc version of strtoul (libc-port:str/strtoul.c 1.22).
 */

#define ISDIGIT(c)	('0' <= (c) && (c) <= '9')
#define ISLOWER(c)	('a' <= (c) && (c) <= 'z')
#define ISUPPER(c)	('A' <= (c) && (c) <= 'Z')
#define MAXBASE		(1 + 'z' - 'a' + 10)

unsigned long
strtoul(const char *str, char **ep, int base)
{
	const unsigned char *sp = (const unsigned char *)str;
	register int ch;
	unsigned long oflow;		/* used to catch overflows */
	register unsigned long val;	/* accumulates value here */
	int negate;			/* result value to be negated */
	char *nul;			/* used as pocket for null ep */

	if (ep == NULL)
		ep = &nul;
	*ep = (char *)sp;
	/*
	* Skip optional leading white space.
	*/
	ch = *sp;
	while (strchr(" \t\r\n\v\f", ch) != NULL)
		ch = *++sp;
	/*
	* Note optional leading sign.
	*/
	negate = 0;
	if (ch == '-')
	{
		negate = 1;
		goto skipsign;
	}
	if (ch == '+')
	{
	skipsign:;
		ch = *++sp;
	}
	/*
	* Should be at the initial digit/letter.
	* Verify that the initial character is the start of a number.
	*/
	if (!ISDIGIT(ch))
	{
		if (base <= 1 || base > MAXBASE)
			goto reterr;
		if (ISLOWER(ch))
			ch -= 'a' - 10;
		else if (ISUPPER(ch))
			ch -= 'A' - 10;
		else
			goto reterr;
		if (ch >= base)
			goto reterr;
		goto gotdig;
	}
	/*
	* ch is 0-9 and is the initial digit.  0 is special.
	*/
	if (ch == '0')
	{
		/*
		* 0x can be a prefix or part of a number for certain bases.
		*/
		if ((ch = *++sp) == 'x' || ch == 'X')
		{
			if (base == 0)
				base = 16;
			else if (base <= 1 || base > MAXBASE)
				goto reterr;
			else if (base != 16)
			{
				if ('x' - 'a' + 10 >= base)
				{
				retzero:;
					*ep = (char *)sp;
					return 0;
				}
				ch = 'x' - 'a' + 10;
				goto gotdig;
			}
			/*
			* base is 16.  Only accept 0x as a prefix if it is
			* followed by a valid hexadecimal digit.
			*/
			ch = *++sp;
			if (ISDIGIT(ch))
				ch -= '0';
			else if (ISLOWER(ch))
				ch -= 'a' - 10;
			else if (ISUPPER(ch))
				ch -= 'A' - 10;
			else
			{
			retbackzero:;
				*ep = (char *)sp - 1;
				return 0;
			}
			if (ch >= 16)
				goto retbackzero;
			goto gotdig;
		}
		if (ch == 'b' || ch == 'B') /* similarly for base 2 */
		{
			if (base == 0)
				base = 2;
			else if (base <= 1 || base > MAXBASE)
				goto reterr;
			else if (base != 2)
			{
				if ('b' - 'a' + 10 >= base)
					goto retzero;
				ch = 'b' - 'a' + 10;
				goto gotdig;
			}
			/*
			* base is 2.  Only accept 0b as a prefix
			* if it is followed by a 0 or 1.
			*/
			if ((ch = *++sp) != '0' && ch != '1')
				goto retbackzero;
			ch -= '0';
			goto gotdig;
		}
		if (base == 0)
			base = 8;
		else if (base <= 1 || base > MAXBASE)
			goto reterr;
		if (ISDIGIT(ch))
			ch -= '0';
		else if (ISLOWER(ch))
			ch -= 'a' - 10;
		else if (ISUPPER(ch))
			ch -= 'A' - 10;
		else
			goto retzero;
		if (ch >= base)
			goto retzero;
		goto gotdig;
	}
	/*
	* ch is 1-9.
	*/
	ch -= '0';
	if (base == 0)
		base = 10;
	else if (base <= 1 || base > MAXBASE || ch >= base)
		goto reterr;
gotdig:;
	/*
	* ch holds the value of the first (other than 0) digit
	* of a valid number.  sp points to that character.
	* Accumulate digits (and the resulting value) until:
	*  - the character isn't 0-9a-zA-Z, or
	*  - the digit/letter is not valid for the base, or
	*  - the value overflows an unsigned long.
	*/
	val = ch;
	oflow = ULONG_MAX / base;
	for (;;)
	{
		ch = *++sp;
		if (ISDIGIT(ch))
			ch -= '0';
		else if (ISLOWER(ch))
			ch -= 'a' - 10;
		else if (ISUPPER(ch))
			ch -= 'A' - 10;
		else
			break;
		if (ch >= base)
			break;
		if (val > oflow)
			goto overflow;
		val *= base;
		if ((val += ch) < ch)
			goto overflow;
	}
	*ep = (char *)sp;
	if (negate)
		return -val;
	return val;
reterr:;
	return 0;
overflow:;
	if (ep != &nul)	/* still need to locate the end of the number */
	{
		for (;;)
		{
			ch = *++sp;
			if (ISDIGIT(ch))
				ch -= '0';
			else if (ISLOWER(ch))
				ch -= 'a' - 10;
			else if (ISUPPER(ch))
				ch -= 'A' - 10;
			else
				break;
			if (ch >= base)
				break;
		}
		*ep = (char *)sp;
	}
	return ULONG_MAX;
}

#undef ISDIGIT
#undef ISLOWER
#undef ISUPPER
#undef MAXBASE

/*
 * int
 * stoi(char **)
 *
 *	Returns the integer value of the string of decimal numeric
 *	chars beginning at **str.
 *
 * Calling/Exit State:
 *
 *	Does no overflow checking.
 *	Note: updates *str to point at the last character examined.
 */
int
stoi(char **str)
{
	return (int)strtoul(*str, str, 10);
}

/*
 * void
 * numtos(ulong_t, char *)
 *
 *	Simple-minded conversion of an unsigned long into a
 *	null-terminated character string, using base 10.
 *
 * Calling/Exit State:
 *
 *	Caller must ensure there's enough space
 *	to hold the result.
 */
void
numtos(ulong_t num, char *s)
{
	int i = 0;
	ulong_t mm = 1000000000;
	int t;

	if (num < 10) {
		*s++ = num + '0';
		*s = '\0';
	} else while (mm) {
		t = num / mm;
		if (i || t) {
			i++;
			*s++ = t + '0';
			num -= t * mm;
		}
		mm = mm / 10;
	}
	*s = '\0';
}


/*
 * Function for efficient conversion of time units: from the number of
 * ticks (stored in a double long word) to seconds and nanoseconds, for
 * representing in a timestruc_t.
 */

/*
 * void
 * ticks_to_timestruc(timestruc_t *tp, dl_t *nticksp)
 *
 *	Convert a dl_t number of "ticks" into a timestruc_t.
 *	These dl_t things are sure a pain to manipulate.
 *	It takes forever to operate on them too.
 *
 * Calling/Exit State:
 *
 *	This is purely a conversion utility, so no lock protection is needed.
 */
void
ticks_to_timestruc(timestruc_t *tp, dl_t *nticksp)
{

#define	BILLION		1000000000
#define TWO_EXP_31	0x80000000


	static	boolean_t	first_time = B_TRUE;
	static	dl_t		billion, tres;
	static	uint_t		a, b;
	static	uint_t		c, d;
	
	
	if (first_time) {
		a = (BILLION / timer_resolution);
		b = (BILLION % timer_resolution);
		c = (TWO_EXP_31 / a);
		d = (TWO_EXP_31 % a);
		tres.dl_hop = 0;
		tres.dl_lop = timer_resolution;
		billion.dl_hop = 0;
		billion.dl_lop = BILLION;
		first_time = B_FALSE;
	}

	/*
	 * PERF: 
	 *	The nticks computed first need to be converted into 
	 * 	units of nanoseconds. From this nanosecond count, a
	 *	count of the number of seconds and a count of the leftover 
	 *	nanoseconds need to derived, for the fields
	 *		tp->tv_sec and 
	 *		tp->tv_nsec 
	 *	respectively.
 	 *
	 *	Ordinarily, this could be done by the following sequence
	 *	of equivalent double-long computations: 
	 *		nanosecs = (nticks * timer_resolution);
	 *		q = (nanosecs / billion);
	 *		r = (nanosecs % billion);
	 *	and then the tv_sec and tv_nsec fields of tp
	 *	can be set to q.dl_lop and r.dl_lop respectively.
	 *	
	 *	However, since the above double long computations can
	 *	be very expensive, we perform the following single long
	 *	computations to arrive at the desired results.
	 *
	 *	I. if timer_resolution < billion
	 *	--------------------------------
	 *		let a = (billion / timer_resolution);
	 *		let b = (billion % timer_resolution);
	 *	then 
	 *		SECONDS = ((nticks * timer_resolution) / billion);
	 *	can be computed first as
	 *		SECONDS = (nticks / a);
	 *	and if (b > 0) can then be corrected as:
	 *		SECONDS -= ((SECONDS * b) / billion)
	 *
	 *	and the nanosecond remainder, 
	 *		NANOSECONDS = ((nticks * timer_resolution) % billion);
	 *	can be computed first as
	 *		NANOSECONDS = (timer_resolution * (nticks % a)); 
	 *	and if (b > 0) can then be corrected as:
	 *		NANOSECONDS -= (SECONDS * b);
	 *
	 *	II. if timer_resolution >= billion
	 *	----------------------------------
	 *		This is probably atypical, and we can compute the
	 *	counts directly with the kernel double long functions that
	 *	are available.
	 */
	if (timer_resolution < BILLION) {

		uint_t  seconds_count;
		uint_t	nanosec_count;
	
		seconds_count = nticksp->dl_lop / a;

		if (nticksp->dl_hop != 0) {
			if (nticksp->dl_hop < a) {
				seconds_count += ((nticksp->dl_hop << 1) * c);
				/*
				 * Compute the approximate nanosecond part,
				 * 	timer_resolution * (nticks % a)
				 */
				nanosec_count =  ((nticksp->dl_hop << 1) * d);
				nanosec_count += (nticksp->dl_lop % a);
				nanosec_count %= a;

			} else {
				seconds_count = INT_MAX;
				nanosec_count = 0;
			}
		} else {
			nanosec_count = (nticksp->dl_lop % a);
			nanosec_count *= timer_resolution;
		}

		/*
		 * and then correct both the second and the nanosecond
		 * counts for any errors due to b.
		 */
		if (b > 0) {
			seconds_count -= ((seconds_count * b) / BILLION);
			nanosec_count -= (seconds_count * b);
		}

		tp->tv_sec = seconds_count;
		tp->tv_nsec = nanosec_count;

	} else {
		dl_t nanosecs, q, r;

		nanosecs = lmul((*nticksp), tres);
	        q = ldivide(nanosecs, billion);
		r = lmod(nanosecs, billion);
       		tp->tv_sec = q.dl_lop;
		tp->tv_nsec = r.dl_lop;
	}

}
