#ident	"@(#)kern-i386:util/ccnv/sb.c	1.5.1.1"
#include <util/cmn_err.h>
#include <svc/time.h>
#include <svc/errno.h>
#include <fs/dosfs/direntry.h>
#include <util/mod/moddefs.h>

extern unsigned char nocase_tounix[];
extern unsigned char nocase_fromunix[];
extern unsigned char tounix[];
extern unsigned char fromunix[];
extern int no_ccnv;

#ifndef NULL
#define NULL 0
#endif

int
ccnv_dummy()
{
	return 0;
}

MOD_MISC_WRAPPER(ccnv, ccnv_dummy, ccnv_dummy, "8859-1 Code Conversion Module");

int
dos2unixfn(unsigned char *dn, unsigned char *un, int *len)
{
	int i;
	int ni;
	int ei;
	int thislong = 0;
	unsigned char c;

	/*
	 *  Find the last character in the name portion
	 *  of the dos filename.
	 */
	for (ni = 7; ni >= 0; ni--)
		if (dn[ni] != ' ') break;

	/*
	 *  Find the last character in the extension
	 *  portion of the filename.
	 */
	for (ei = 10; ei >= 8; ei--)
		if (dn[ei] != ' ') break;

	/*
	 *  If first char of the filename is SLOT_E5 (0x05), then
	 *  the real first char of the filename should be 0xe5.
	 *  But, they couldn't just have a 0xe5 mean 0xe5 because
	 *  that is used to mean a freed directory slot.
	 *  Another dos quirk.
	 */
	if (dn[0] == SLOT_E5)
		dn[0] = 0xe5;

	/*
	 *  Copy the name portion into the unix filename
	 *  string.
	 *  NOTE: DOS filenames are usually kept in upper
	 *  case.  To make it more unixy we convert all
	 *  DOS filenames to lower case.  Some may like
	 *  this, some may not.
	 */
	for (i = 0; i <= ni; i++) {
		c = dn[i];
		*un++ = tounix[c-1];
		thislong++;
	}

	/*
	 *  Now, if there is an extension then put in a period
	 *  and copy in the extension.
	 */
	if (ei >= 8) {
		*un++ = '.';
		thislong++;
		for (i = 8; i <= ei; i++) {
			c = dn[i];
			*un++ = tounix[c-1];
			thislong++;
		}
	}
	*un++ = 0;

	*len =  thislong;
	return(0);
}

/*
 *  Convert a unix filename to a DOS filename.
 */
int
unix2dosfn(unsigned char *un, unsigned char *dn, int unlen)
{
	int i;
	unsigned char c;
	int invalfound = 0;
	int dotsfound = 0;

	/*
	 *  Fill the dos filename string with blanks.
	 *  These are DOS's pad characters.
	 */
	for (i = 0; i <= 10; i++)
		dn[i] = ' ';

	/*
	 *  The filenames "." and ".." are handled specially,
	 *  since they don't follow dos filename rules.
	 */
	if (un[0] == '.'  &&  un[1] == '\0') {
		dn[0] = '.';
		return (0);
	}
	if (un[0] == '.'  &&  un[1] == '.'  &&  un[2] == '\0') {
		dn[0] = '.';
		dn[1] = '.';
		return (0);
	}

	/*
	 * Parse the name string looking for an invalid character.
	 */
	for (i = 0; i < unlen; i++) {
		switch (un[i]) {
		case '"':
		case '\\':
		case '/':
		case '[':
		case ']':
		case ':':
		case '*':
		case '<':
		case '>':
		case '|':
		case '+':
		case '=':
		case ';':
		case '\'':
		case '?':
			invalfound++;
			break;
		case '.':
			if (i == 0)
				invalfound++;
			dotsfound++;
			break;
		}
		if (dotsfound > 1  || invalfound)
			return EINVAL;
	}

	/*
	 *  Copy the unix filename into the dos filename string
	 *  upto the end of string, a '.', or 8 characters.
	 *  Whichever happens first stops us.
	 *  This forms the name portion of the dos filename.
	 *  Fold to upper case.
	 */
	for (i = 0; i <= 7  &&  unlen  &&  (c = *un)  &&  c != '.'; i++) {
		dn[i] = fromunix[c-1];
		un++;
		unlen--;
	}

	/*
	 *  If the first char of the filename is 0xe5, then translate
	 *  it to 0x05.  This is because 0xe5 is the marker for a
	 *  deleted directory slot.  I guess this means you can't
	 *  have filenames that start with 0x05.  I suppose we should
	 *  check for this and doing something about it.
	 */
	if (dn[0] == SLOT_DELETED)
		dn[0] = SLOT_E5;

	/*
	 *  Strip any further characters up to a '.' or the
	 *  end of the string.
	 */
	while (unlen  &&  (c = *un)  &&  c != '.') {
		un++;
		unlen--;
	}

	/*
	 *  If we stopped on a '.', then get past it.
	 */
	if (c == '.') {
		un++;
		unlen--;
	}

	/*
	 *  Copy in the extension part of the name, if any.
	 *  Force to upper case.
	 *  Note that the extension is allowed to contain '.'s.
	 *  Filenames in this form are probably inaccessable
	 *  under dos.
	 */
	for (i = 8; i <= 10  &&  unlen  &&  (c = *un); i++) {
		dn[i] = fromunix[c-1];
		un++;
		unlen--;
	}
	return(0);
}

unsigned long
ccnv_unix2dos(unsigned char **from, unsigned char *to, int length, int dosnamespace)
{
	int i;
	if(no_ccnv && !dosnamespace)
		return(0);
	if(strlen(*from) >= length)
		return(E2BIG);
	if(dosnamespace) {
		for(i=0;i<strlen(*from);i++)
			*(to+i) = fromunix[*((*from)+i)-1];
	}
	else {
		for(i=0;i<strlen(*from);i++)
			*(to+i) = nocase_fromunix[*((*from)+i)-1];
	}
	*(to+i) = '\0';
	*from = to;
	return(0);
}

unsigned long
ccnv_dos2unix(unsigned char **from, unsigned char *to, int length, int dosnamespace)
{
	int i;
	if(no_ccnv && !dosnamespace)
		return(0);
	if(strlen(*from) >= length)
		return(E2BIG);
	if(dosnamespace) {
		for(i=0;i<strlen(*from);i++)
			*(to+i) = tounix[*((*from)+i)-1];
	}
	else {
		for(i=0;i<strlen(*from);i++)
			*(to+i) = nocase_tounix[*((*from)+i)-1];
	}
	*(to+i) = '\0';
	*from = to;
	return(0);
}
