/*		copyright	"%c%" 	*/

#ident	"@(#)check.c	1.2"
#ident  "$Header$"

#include <stdio.h>
#include <pwd.h>
#include "idmap.h"

extern	int	strlen();
extern	char	*strdup();
extern	int	strcmp();

extern	int	breakname();
extern	int	namecmp();

/*
 *	check_descr()
 *
 *		input: descr - NULL terminated string, such as "M1@M2"
 *		       the leading '!' is NOT part of the descriptor
 *		return value: 0 for OK, -1 for invalid descriptor
 */

int
check_descr(descr)
char	*descr;		/* remote name/attribute field descriptor */
{
	int	descrlen;
	int	i;
	int	fieldused[MAXFIELDS];

	/*
	 *	check descriptor length --
	 *	should be 3 * x + 2
	 */

	descrlen = strlen(descr);
	if ((descrlen % 3) != 2)
		return(-1);

	for (i = 0; i < MAXFIELDS; i++)
		fieldused[i] = 0;

	for (i = 0; i < descrlen; i += 3) {

		/*
		 *	field type must be M
		 */

		if (descr[i] != 'M')
			return(-1);

		/*
		 *	field number must be between 0 and 9
		 */

		if ((descr[i+1] < '0') || (descr[i+1] > '9'))
			return(-1);

		/*
		 *	field number must be unused
		 */

		if (fieldused[descr[i+1] - '0'])
			return(-1);
		else
			fieldused[descr[i+1] - '0']++;
	}

	/*
	 *	all passed
	 */

	return(0);
}


/*
 *	check_user()
 *
 *	input: entry - NULL terminated string which represents the
 *	       line entry being checked.
 *	       allowmacros - flag signifying whether %x macros are allowed
 *	return value: 0 for OK, non-0 for error
 *				IE_NOUSER	user not in passwd file
 */

int
check_user(entry, allowmacros)
char	*entry;		/* mapping file entry line with correct syntax */
int	allowmacros;	/* 0 == do not allow %x, 1 == allow %x */
{
	char	remote[MAXLINE];	/* local name */
	char	local[MAXLINE];		/* remote name */
	char	fake[MAXLINE];		/* fake place holder */

	if (sscanf(entry, "%s %s %s", remote, local, fake) != 2)
		return(IE_SYNTAX);

	/* is it a macro? */
	if (local[0] == '%')
		/* are macros allowed? */
		if (allowmacros)
			/* OK, but then we can't check it now */
			return(0);
		/* maybe the user name starts with % */

	if (getpwnam(local) == (struct passwd *) NULL)
		return(IE_NOUSER);

	return(0);
}


/*
 *	check_entry()
 *
 *	input: descr - NULL terminated string, such as "!M1@M2\n",
 *	       the leading '!' is part of the descriptor,
 *	       the trailing \n is also part of the descriptor.
 *	       prev_entry, entry - NULL terminated strings, which
 *	       represent the entry being checked and the entry
 *	       preceeding it.  If prev_entry pointer is NULL,
 *	       duplication and order are not checked.
 *	return value: 0 for OK, non-0 for error as follows:
 *				IE_SYNTAX	syntax error
 *				IE_MANDATORY	mandatory field missing
 *				IE_DUPLICATE	entries are duplicates
 *				IE_ORDER	entries are out of order
 *				IE_NOFIELD	%n field not present
 *	no user in passwd file checks are performed since this routine
 *	is also used by attradmin which may not be mapping login names.
 */

int
check_entry(descr, prev_entry, entry)
char	*descr;		/* remote name/attribute field descriptor */
char	*prev_entry;	/* previous entry */
char	*entry;		/* current entry being checked */
{
	FIELD	fields1[MAXFIELDS];	/* fields of entry */
	FIELD	fields2[MAXFIELDS];	/* fields of prev_entry */
	char	remote[MAXLINE];	/* local name */
	char	local[MAXLINE];		/* remote name */
	char	fake[MAXLINE];		/* fake place holder */
	char	prev_remote[MAXLINE];	/* previous local name */
	char	prev_local[MAXLINE];	/* previous remote name */

	/*
	 *	check syntax
	 */

	if (sscanf(entry, "%s %s %s", remote, local, fake) != 2)
		return(IE_SYNTAX);

	/*
	 *	check mandatory fields
	 */

	if (breakname(strdup(remote), descr, fields1) < 0)
			return(IE_MANDATORY);

	/*
	 *	check macro for presence in remote name
	 */

	if ((local[0] == '%') && (local[1] != 'i')) {
		if (strcmp(fields1[local[1] - '0'].value, "") == 0)
			return(IE_NOFIELD);
	}

	if (prev_entry != NULL) {

		(void) sscanf(prev_entry, "%s %s", prev_remote, prev_local);

		/*
		 *	check if duplicate
		 */

		if (strcmp(remote, prev_remote) == 0)
			return(IE_DUPLICATE);

		/*
		 *	check order
		 */

		(void) breakname(strdup(prev_remote), descr, fields2);

		if (namecmp(fields1, fields2) < 0)
			return(IE_ORDER);
	}

	/*
	 *	all checks passed
	 */

	return(0);
}


/*
 *	check_macro()
 *
 *	input: descr - NULL terminated string such as "M1@M2\n",
 *	       without a leading '!'.
 *	       macchar - macro character
 *	return value: 0 for OK, -1 for error
 *
 */

int
check_macro(descr, macchar)
char	*descr;		/* field descriptor */
char	macchar;	/* macro character */
{
	if (macchar == 'i')
		return(0);

	if ((macchar < '0') || (macchar > '9'))
		return(-1);

	/* descriptor has to be at least "M1" */

	do {
		if (*(descr + 1) == macchar)
			return(0);
		descr += 3;
	} while(*(descr - 1) != '\n');

	return(-1);
}
