/*		copyright	"%c%" 	*/

#ident	"@(#)namecmp.c	1.2"
#ident  "$Header$"

#include <stdio.h>
#include <string.h>
#include "idmap.h"

static int	mult();
static int	hasstar();
static int	hasbracket();
static int	hasquestion();
static int	regexpcmp();
static int	cmpmult();

/*
 * namecmp() compares names (broken into fields) by comparing fields one at
 * a time from highest number field to lowest until a definite order can
 * be determined.
 * If neither field contains a regular expression, then the fields are
 * simply compared alphabetically.
 * If exactly one field contains a regular expression, then the name
 * which contains that field is considered greater than the other name.
 * If both fields contain regular expressions, the following rules are
 * used:
 *	If one field contains '[' or '?' as well as '*', then this
 *	field is more specific than a field containing just '*'.
 *	Going from left to right, the field with the leftmost general
 *	expression is most general.
 *
 */

int
namecmp(n1fields, n2fields)
FIELD n1fields[MAXFIELDS];
FIELD n2fields[MAXFIELDS];
{
	int level = MAXFIELDS - 1;	/* field significance level */
	int mult1, mult2;		/* multiple match flags - i.e */
					/* string contains *, ?, etc. */
	int sr;				/* strcmp() return code */

	while(level >= 0) {
		/* assuming that both names had the same descriptor, then */
		/* the type of each field would be same for both names */
		if (n1fields[level].type == 'M') {

			mult1 = mult(n1fields[level].value);
			mult2 = mult(n2fields[level].value);

			if (!mult1)
				if (!mult2) {
					sr = strcmp(n1fields[level].value,
						    n2fields[level].value);
					if (sr != 0)
						return(sr);
				} else
					/* namecmp( name, REGEXP ) */
					return(-1);
			else
				if (!mult2) 
					/* namecmp( REGEXP, name ) */
					return(1);
				else {
					/* namecmp( REGEXP, REGEXP ) */
					sr = regexpcmp(n1fields[level].value,
						       n2fields[level].value);
					if (sr != 0)
						return(sr);
				} /* else */
		} /* if */
		level--;
	} /* while */
	/* the names were exactly the same */
	return(0);
}


#define	STARSTR		"*"
#define	BRACKETSSTR	"[]"
#define	QUESTIONSTR	"?"


/*
 * mult() returns 1 if the string parameter contains any characters
 * (at least one of [,],?,*) which can be used in a regular expression 
 * to match multiple strings.  Otherwise, mult() returns 0.
 *
 */

static int
mult(str)
char	*str;
{
	return( hasstar(str) || hasbracket(str) || hasquestion(str) );
}

static int
hasstar(str)
char	*str;
{
	return( strpbrk(str, STARSTR) != NULL );
}

static int
hasbracket(str)
char	*str;
{
	return( strpbrk(str, BRACKETSSTR) != NULL );
}

static int
hasquestion(str)
char	*str;
{
	return( strpbrk(str, QUESTIONSTR) != NULL );
}


static int
regexpcmp(f1, f2)
char	*f1, *f2;		/* fields containing regular expressions */
{
	int	hr1, hr2;	/* hasXXX() return code */

	hr1 = hasbracket(f1) || hasquestion(f1);
	hr2 = hasbracket(f2) || hasquestion(f2);

	if (!hr1 && hr2)
		/* regexpcmp( *, [?] ) */
		return(1);
	if (hr1 && !hr2)
		/* regexpcmp( [?], * ) */
		return(-1);

	/* regexpcmp( [?], [?] ) */
	/* regexpcmp( *, * ) */

	return(cmpmult(f1, f2));
}


static int
cmpmult(f1, f2)
char	*f1, *f2;		/* fields containing regular expressions */
{
	if (*f1 == '\0') {
		if (*f2 == '\0')
			return(0);
		if (*f2 == '*')
			return(-1);
		else
			return(1);

	}

	if (*f2 == '\0')
		if (*f1 == '*')
			return(1);
		else
			return(-1);

	if (*f1 == *f2)
		return(cmpmult(f1+1, f2+1));

	switch(*f1) {

	case '*':
		return(1);

	case '[':
	case '?':
		if (*f2 == '*')
			return(-1);
		else
			return(1);

	default:
		if ((*f2 == '[') || (*f2 == '?') || (*f2 == '*'))
			return(-1);
		else
			return(cmpmult(f1+1, f2+1));

	} /* case */
}
