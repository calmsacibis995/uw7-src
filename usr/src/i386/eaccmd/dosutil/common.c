/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/common.c	1.2.1.2"
#ident  "$Header$"
/*	@(#) common.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

/*
 * Copyright (C) Microsoft Corporation, 1983.
 *
 * common.c - common routines used to access MS-DOS filesystems
 *
 *	MODIFICATION HISTORY
 *	M004	Dec 7, 1984	ericc			16 bit FAT Support
 *	- Rewrote the programs, splitting common.c into many different
 *	  files.
 */


#include	<stdio.h>
#include	<ctype.h>
#include	"dosutil.h"



/*	decompose()  --  break a string up into the Unix pathname of the
 *		device and the DOS path to the DOS file.  The DOS path thus
 *		gleaned is stripped of any trailing DIRSEPs.
 *
 *		string:  input string, typically "/dev/hd04:/a/b/dosfile.ext"
 *		px  :  pointer to Unix pathname
 *		pdos  :  pointer to DOS path
 */

decompose(string,px,pdos)
char	*string, **px, **pdos;
{
	extern char *defread();

	static int defflag = FALSE;
	char *dospath, *tmp, *def = "A=";

	if (defflag == FALSE)
		defflag = (defopen(DEFAULT) == (int) NULL);

	if ((dospath = strchr(string,':')) == NULL)
		dospath = strdup("");
	else{
		*(dospath++) = '\0';
		tmp = dospath + strlen(dospath) - 1;
		while (*tmp == DIRSEP)			/* strip all trailing */
			*(tmp--) = '\0';		/*     DIRSEPs */

		if (defflag && (string[1] == '\0')){
			*def = toupper(*string);
			if ((tmp = defread(def)) != NULL)
				string = strdup(tmp);
		}
	}
	*px = string;
	*pdos = dospath;
}


/*	fatal()  --  complaint routine.  If code is nonzero, exit; if code
 *		is zero, carry on, but change the exit status to 1.
 *		WARNING: Excessively long strings in errbuf[] cause chaos!
 */

fatal(msg,code)
char *msg;
int  code;
{
	extern int exitcode;
	extern char *f_name;

	fprintf(stderr,"%s: %s\n",f_name,msg);
	if (code)
		exit(code);
	exitcode = 1;
	return;
}

/*
 *	fill(string,character,n)  --  write n characters into string.
 */

fill(string,c,n)
char *string, c;
unsigned n;
{
	for (; n > 0; n--)
		*(string++) = c;
}


/*	movchar(destination,source,nchar)  --  our own version of strncpy()
 *				which doesn't stop at the NULL character.
 */

movchar(dest,src,nchar)
char *dest, *src;
unsigned nchar;
{
	for(; nchar > 0; nchar--)
		*(dest++) = *(src++);
}


/*
 *	upshift(string,nchar)  --  convert string to uppercase completely.
 */

upshift(string,nchar)
char *string;
unsigned nchar;
{
	for (; nchar > 0; nchar--)
		*(string++) = toupper(*string);
}
