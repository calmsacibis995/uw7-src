#pragma comment(exestr, "@(#) atfields.c 25.2 92/09/11 ")
/*
 *	Copyright (C) 1991-1992 The Santa Cruz Operation, Inc.
 *		All Rights Reserved.
 *	The information in this file is provided for the exclusive use of
 *	the licensees of The Santa Cruz Operation, Inc.  Such users have the
 *	right to use, modify, and incorporate this code into other products
 *	for purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */
/****************************************************************************/
/*
 *	Modification History
 *
 *	L000	11aug92	scol!lawrence
 *		- Added support for ORTSFL. Removed support for CRTSFL.
 *
 *	25 Mar 96	scol!rroscoe				L001
 *	- Use termios.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#define toint(c)     ((int)((c)-'0')) /* not in ctype */
#define isodigit(x) ((isdigit(x)) && toint(x) < 8)

#define	TRUE	1
#define FALSE	0

#define WRDLENGTH	20	/* Maximum word length */ 

#define	SUCCESS	0

extern struct termios term;					/* L001 */

struct Symbols {
	char		*s_symbol;	/* Name of symbol */
	unsigned	s_value	;	/* Value of symbol */
};

/*	The following four symbols define the "SANE" state.		*/

#define	ISANE	(BRKINT|IGNPAR|ICRNL|IXON)	
#define	OSANE	(OPOST|ONLCR)
#define	CSANE	(CREAD)		
#define	LSANE	(ISIG|ICANON|ECHO|ECHOK)

/*	Modes set with the TCSETAW ioctl command.			*/

struct Symbols imodes[] = {
	"IGNBRK",	IGNBRK,
	"BRKINT",	BRKINT,
	"IGNPAR",	IGNPAR,
	"PARMRK",	PARMRK,
	"INPCK",	INPCK,
	"ISTRIP",	ISTRIP,
	"INLCR",	INLCR,
	"IGNCR",	IGNCR,
	"ICRNL",	ICRNL,
	"IUCLC",	IUCLC,
	"IXON",	IXON,
	"IXANY",	IXANY,
	"IXOFF",	IXOFF,
	NULL,	0
};

struct Symbols omodes[] = {
	"OPOST",	OPOST,
	"OLCUC",	OLCUC,
	"ONLCR",	ONLCR,
	"OCRNL",	OCRNL,
	"ONOCR",	ONOCR,
	"ONLRET",	ONLRET,
	"OFILL",	OFILL,
	"OFDEL",	OFDEL,
	"NLDLY",	NLDLY,
	"NL0",	NL0,
	"NL1",	NL1,
	"CRDLY",	CRDLY,
	"CR0",	CR0,
	"CR1",	CR1,
	"CR2",	CR2,
	"CR3",	CR3,
	"TABDLY",	TABDLY,
	"TAB0",	TAB0,
	"TAB1",	TAB1,
	"TAB2",	TAB2,
	"TAB3",	TAB3,
	"BSDLY",	BSDLY,
	"BS0",	BS0,
	"BS1",	BS1,
	"VTDLY",	VTDLY,
	"VT0",	VT0,
	"VT1",	VT1,
	"FFDLY",	FFDLY,
	"FF0",	FF0,
	"FF1",	FF1,
	NULL,	0
};

struct Symbols cmodes[] = {
	"B0",		B0,
	"B50",	B50,
	"B75", 	B75,
	"B110",	B110,
	"B134",	B134,
	"B150",	B150,
	"B200",	B200,
	"B300",	B300,
	"B600",	B600,
	"B1200",	B1200,
	"B1800",	B1800,
	"B2400",	B2400,
	"B4800",	B4800,
	"B9600",	B9600,
	"B19200",	B19200,
	"B38400",	B38400,
	"EXTA",		EXTA,	/* same as B19200 */
	"EXTB",		EXTB,	/* same as B38400 */
	"CS5",	CS5,
	"CS6",	CS6,
	"CS7",	CS7,
	"CS8",	CS8,
	"CSTOPB",	CSTOPB,
	"CREAD",	CREAD,
	"PARENB",	PARENB,
	"PARODD",	PARODD,
	"HUPCL",	HUPCL,
	"CLOCAL",	CLOCAL,
/*
	"CTSFLOW",	CTSFLOW,
	"RTSFLOW",	RTSFLOW,
	"ORTSFL",	ORTSFL,	
*/
	NULL,	0
};

struct Symbols lmodes[] = {
	"ISIG",	ISIG,
	"ICANON",	ICANON,
	"XCASE",	XCASE,
	"ECHO",	ECHO,
	"ECHOE",	ECHOE,
	"ECHOK",	ECHOK,
	"ECHONL",	ECHONL,
	"NOFLSH",	NOFLSH,
	NULL,	0
};


/*	
 *	"fields" picks up the stty settings from ptr and converts all	
 *	recognized words into the proper mask and puts it in the target
 *	field.							
 */
fields(ptr)
register char *ptr;
{
	struct Symbols *search();
	register struct Symbols *symbol;
	char *word,*getword();
	int size;
	int negate;

	while (*ptr != '\0') {

		/* Pick up next word in the sequence. */

		word = getword(ptr,&size); 

		/* If there is a word, scan the two mode tables for it. */

		if (*word != '\0') {
				/* 
				 * If the word begins with a hyphen, 
				 * then the flag is negated. Otherwise it
				 * is asserted.
				 */
			if (*word == '-') {
				negate = TRUE;
				word++;
			} else	negate = FALSE;

				/* 
				 * If the word is the special word "SANE", 
				 * put in all the flags 
				 * that are needed for SANE tty behavior. 
				 * There is no such a thing as -SANE (kz)
				 */

			if (strcmp(word,"SANE") == 0) {
				term.c_iflag |= ISANE;
				term.c_oflag |= OSANE;
				term.c_cflag |= CSANE;
				term.c_lflag |= LSANE;
			} else if ((symbol = search(word,imodes)) != NULL)

				if (negate)
					term.c_iflag &= ~symbol->s_value;
				else	term.c_iflag |=  symbol->s_value;
			else if ((symbol = search(word,omodes)) != NULL)
				if (negate)
					term.c_oflag &= ~symbol->s_value;
				else	term.c_oflag |=  symbol->s_value;
			else if ((symbol = search(word,cmodes)) != NULL)
				if (negate)
					term.c_cflag &= ~symbol->s_value;
				else	term.c_cflag |=  symbol->s_value;
			else if ((symbol = search(word,lmodes)) != NULL)
				if (negate)
					term.c_lflag &= ~symbol->s_value;
				else	term.c_lflag |=  symbol->s_value;
		}
		ptr += size;		/* Advance pointer to after the word. */
	}
}

char *
getword(ptr,size)
register char *ptr;
int *size;
{
	register char *optr,c;
	char quoted();
	static char word[WRDLENGTH+1];
	int qsize;

	/* 
	 * Skip over all white spaces including quoted spaces and tabs.
	 */

	for (*size=0; isspace(*ptr) || *ptr == '\\'; ) {
		if (*ptr == '\\') {
			c = quoted(ptr,&qsize);
			(*size) += qsize;
			ptr += qsize+1;

			/* 
			 * If this quoted character is not a space or a tab or 
			 * a newline then break. 
			 */

			if (isspace(c) == 0) 
				break;
		} 
		else {
			(*size)++;
			ptr++;
		}
	}

	/* 
	 * Put all characters from here to next white space or '\0'
	 * into the word, up to the size of the word.
	 */

	for (optr= word,*optr='\0'; isspace(*ptr) == 0 &&
	    *ptr != '\0'; ptr++,(*size)++) {

		/* If the character is quoted, analyze it. */

		if (*ptr == '\\') {
			c = quoted(ptr,&qsize);
			(*size) += qsize;
			ptr += qsize;
		} else 
			c = *ptr;

		/* If there is room, add this character to the word. */

		if (optr < &word[WRDLENGTH+1] ) 
			*optr++ = c;
	}

	/* Make sure the line is null terminated. */

	*optr++ = '\0';
	return(word);
}
/*	
 * "search" scans through a table of Symbols trying to find a
 * match for the supplied string.  If it does, it returns the
 * pointer to the Symbols structure, otherwise it returns NULL.
 */

struct Symbols *
search(target,symbols)
register char *target;
register struct Symbols *symbols;
{

/* 
 * Each symbol array terminates with a null pointer for an "s_symbol".  Scan 
 * until a match is found, or the null pointer is reached. 
 */
	for ( ; symbols->s_symbol != NULL; symbols++)
		if (strcmp(target,symbols->s_symbol) == 0) 
			return(symbols);
	return(NULL);
}

/*	
 * "quoted" takes a quoted character, starting at the quote
 *	character, and returns a single character plus the size of
 *	the quote string.  "quoted" recognizes the following as	
 *	special, \n,\r,\v,\t,\b,\f as well as the \nnn notation.
 */

char 
quoted(ptr,qsize)
char *ptr;
int *qsize;
{
	register char c,*rptr;
	register int i;

	rptr = ptr;
	switch(*++rptr) {
	case 'n':
		c = '\n';
		break;
	case 'r':
		c = '\r';
		break;
	case 'v':
		c = '\013';
		break;
	case 'b':
		c = '\b';
		break;
	case 't':
		c = '\t';
		break;
	case 'f':
		c = '\f';
		break;
	default:

		/*
		 * If this is a numeric string, take up to three characters of
		 * it as the value of the quoted character.
		 */

		if (isodigit(*rptr)) {
			for (i=0,c=0; i < 3;i++) {
				c = c*8 + toint(*rptr++);
				if (!isodigit(*rptr)) break;
			}
			rptr--;

		} 
		else if (*rptr == '\0') {

			/* 
			 * If the character following the '\\' is a NULL, back 
			 * up the ptr so that the NULL won't be missed.  The 
			 * sequence backslash null is essentially illegal.
			 */

			c = '\0';
			rptr--;

		} 
		else 
			c = *rptr;
			/* In all other cases the quoting does nothing. */
		break;
	}

	/* Compute the size of the quoted character. */

	(*qsize) = rptr - ptr + 1;
	return(c);
}

