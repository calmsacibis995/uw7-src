#ident	"@(#)"
/***	deflt.c - Default Reading Package, copied from libcmd:deflt.c
 *
 *      this package consists of the routines:
 *
 *              defopen()
 *              defread()
 *
 *      These routines allow one to conveniently pull data out of
 *      files in
 *                      /etc/uucp/default/
 *
 *      and thus make configuring a utility's defopens standard and
 *      convenient.
 *
 */

#include <stdio.h>
#include <deflt.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>

#define	FOREVER			for ( ; ; )
#define	TSTBITS(flags, mask)	(((flags) & (mask)) == (mask))

extern	int	errno,
		tolower();

extern 	char	*strcat();

static	void	strlower(), strnlower();

static	int	Dcflags;	/* [re-]initialized on each call to defopen() */


/*      defopen() - declare defopen filename
 *
 *      defopen(cp)
 *              char *cp
 *		this is a full path of the default file to open
 *
 *      see defread() for more details.
 *
 *      EXIT    returns FILE * if ok
 *              returns NULL if error
 */

FILE *
defopen(fn)
	char *fn;
{
	FILE	*dfile	= NULL;

	if ((dfile = fopen(fn, "r")) == (FILE *) NULL) {
		return (FILE *) NULL;
	}

	Dcflags = DC_STD;

	return dfile;
}


/*      defread() - read an entry from the defaults file
 *
 *      defread(fp, cp)
 *		FILE *fp
 *              char *cp
 *
 *      The file pointed to by fp must have been previously opened by
 *      defopen().  defread scans the data file looking for a line
 *      which begins with the string '*cp'.  If such a line is found,
 *      defread returns a pointer to the first character following
 *      the matched string (*cp).  If no line is found or no file
 *      is open, defread() returns NULL.
 *
 *      If *cp is NULL then defread returns the first line in the file
 *	so subsequent calls will sequentially read the file.
 */

char *
defread(fp, cp)
FILE *fp;
char *cp;
{
	static char buf[256];
	register int len;
	register int patlen;

	if (fp == (FILE *) NULL) {
		errno = EINVAL;
		return((char *) NULL);
	}

	if (cp) {

		patlen = strlen(cp);
		rewind(fp);
		while (fgets(buf, sizeof(buf), fp)) {
			len = strlen(buf);
			if (buf[len-1] == '\n')
				buf[len-1] = 0;
			else
				return((char *) NULL);		/* line too long */
			if (!TSTBITS(Dcflags, DC_CASE)) {
				strlower(cp, cp);
				strnlower(buf, buf, patlen);
			}
		
		if ((strncmp(cp, buf, patlen) == 0) && (buf[patlen] == '='))
				return(&buf[++patlen]);           /* found it */
		}
		return((char *) NULL);

	} else {
	
skip:
		if(fgets(buf, sizeof(buf), fp) == NULL)
			return((char *) NULL);
		else {
			if (buf[0] == '#')
				goto skip;
			len = strlen(buf);
			if (buf[len-1] == '\n')
				buf[len-1] = 0;
			else
				return((char *) NULL);		/* line too long */
			return(&buf[0]);
		}
	}

}

int
defclose(fp)
FILE *fp;
{
	return(fclose(fp));
}

/***	strlower -- convert upper-case to lower-case.
 *
 *	Like strcpy(3), but converts upper to lower case.
 *	All non-upper-case letters are copied as is.
 *
 *	ENTRY
 *	  from		'From' string.  ASCIZ.
 *	  to		'To' string.  Assumed to be large enough.
 *	EXIT
 *	  to		filled in.
 */

static	void
strlower(from, to)
register char  *from, *to;
{
	register int  ch;

	FOREVER {
		ch = *from++;
		if ((*to = tolower(ch)) == '\0')
			break;
		else 
			++to;

	}

	return;
}

/***	strnlower -- convert upper-case to lower-case.
 *
 *	Like strncpy(3), but converts upper to lower case.
 *	All non-upper-case letters are copied as is.
 *
 *	ENTRY
 *	  from		'from' string.  May be ASCIZ.
 *	  to		'to' string.
 *	  cnt		Max # of chars to copy.
 *	EXIT
 *	  to		Filled in.
 */

static	void
strnlower(from, to, cnt)
register char  *from, *to;
register int  cnt;
{
	register int  ch;

	while (cnt-- > 0) {
		ch = *from++;
		if ((*to = tolower(ch)) == '\0')
			break;
		else
			++to;
	}
}

