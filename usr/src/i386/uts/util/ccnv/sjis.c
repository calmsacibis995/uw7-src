#ident	"@(#)kern-i386:util/ccnv/sjis.c	1.5.1.1"
#include <svc/time.h>
#include <svc/errno.h>
#include <fs/dosfs/direntry.h>
#include <util/mod/moddefs.h> 
#include <svc/errno.h>

extern unsigned char tounix[];
extern unsigned char fromunix[];

#ifndef NULL
#define NULL 0
#endif
/*
 *  Cheezy macros to do case detection and conversion
 *  for the ascii character set.  DOESN'T work for ebcdic.
 */
#define	_toupper(c)	((c >= 'a'  &&  c <= 'z') ? (c - 'a' + 'A'): c)
#define	_tolower(c)	((c >= 'A'  &&  c <= 'Z') ? (c - 'A' + 'a'): c)

#define C1	0x80	/* Start of Control 1 set */
#define	CS1	0xa1	/* Start of EUC CODE SET1 */
#define	SS2	0x8e	/* EUC CODE SET2 */
#define	SS3	0x8f	/* EUC CODE SET3 */

/*
 * This macro returns SS2 or SS3 if the character is one of those two
 * values. If the character is in the CS1 character range, it returns
 * CS1. Otherwise, if the character is C1, return C1. Finally, if the
 * character is ASCII or C0, it returns 0.
 */
#define codeset(char)	((char == SS2 || char == SS3) ? char : \
				((char >= CS1 && char != 0xff) ? CS1 : \
					(char < C1 ? 0 : C1)))

#define SUB_CHAR_LEN	2

/*
 * Substitute character to use for codes that are legal in EUC but
 * not present in SJIS. (There are no codes that are legal in SJIS but
 * not present in EUC.)
 */
static unsigned char sub_char[SUB_CHAR_LEN] = { 0x81, 0xa0 }; /* A box */

static unsigned short int u2s(unsigned short int c);
static unsigned short int s2u(unsigned short int c);


	

unsigned long
euc_to_sjis(const unsigned char *from, unsigned long bytesleft, unsigned char **outbuf,
		int *outbytesleft, int charcase)
{
	unsigned char *to = (unsigned char *) *outbuf;
	unsigned long spaceleft = *outbytesleft;
	unsigned char inchar, inchar2;
	unsigned short int input, output;
	int errno = 0;
	int i;

	while (bytesleft && spaceleft) {

		inchar = *from;

		switch (codeset(inchar)) {
		case SS3:	/* Unsupported so output a substitute char */
			if (bytesleft < 3) {
				errno = EINVAL; 
				goto errret;
			}

			inchar = *(from+1);
			inchar2 = *(from+2);

			if (inchar < 0x80 || inchar2 < 0x80) {
				errno = EILSEQ;
				goto errret;
			}
			if (spaceleft < SUB_CHAR_LEN) {
				errno=E2BIG;
				goto errret;
			}

			for (i=0; i < SUB_CHAR_LEN; i++) {
				*to++ = sub_char[i];
			}
			spaceleft -= SUB_CHAR_LEN;

			from += 3;
			bytesleft -= 3;
			break;
		case SS2:	/* KATAKANA */
			if (bytesleft < 2) {
				errno = EINVAL;
				goto errret;
			}
			/*
			 * Since we only output one character, we know that
			 * there is enough space in the output buffer.
			 */
			inchar = *(from+1);
			if (inchar >= CS1 && inchar <= 0xdf) {
				from += 2;
				bytesleft -= 2;

				*to++ = inchar;
				spaceleft--;
			} else {
				errno = EILSEQ;
				goto errret;
			}
			break;
		case CS1:	/* KANJI */
			if (bytesleft < 2) {
				errno = EINVAL;
				goto errret;
			}

			inchar2 = *(from+1);

			/*
			 * Note that EILSEQ is always given priority over E2BIG
			 */
			if (inchar2 < CS1 || inchar2 == 0xff) {
				errno = EILSEQ;
				goto errret;
			} else if (spaceleft < 2) {
				errno=E2BIG;
				goto errret;
			} else {
				from += 2;
				bytesleft -= 2;

				input = (inchar << 8) | inchar2;
				output = u2s(input);
				*to++ = output >> 8;
				*to++ = (unsigned char) output;
				spaceleft -= 2;
			}
			break;
		case C1:	/* Doesn't translate, so output substitute */
			if (spaceleft < SUB_CHAR_LEN) {
				errno=E2BIG;
				goto errret;
			}

			for (i=0; i < SUB_CHAR_LEN; i++) {
				*to++ = sub_char[i];
			}
			spaceleft -= SUB_CHAR_LEN;

			from++;
			bytesleft--;
			break;
		default:	/* ASCII */
			if(charcase)
				*to++ = _toupper(*from);
			else
				*to++ = *from;
			from++;
			bytesleft--;
			spaceleft--;
			break;
		}
	}

	if (bytesleft != 0) {		/* if everything was not converted */
		errno = E2BIG;
	}
errret:
	*outbytesleft = spaceleft;
	*outbuf = to;
	return errno;
}

unsigned short int
u2s(unsigned short int c)
{
	unsigned short int hi, lo;

	hi = (c >> 8) & 0x7f;
	lo = c & 0x7f;

	if ( hi & 1) {
		lo += 0x1f;
	} else {
		lo += 0x7d;
	}

	hi = ( (hi - 0x21) >> 1 ) + 0x81;

	if (lo >= 0x7f) {
		lo++;
	}

	if (hi > 0x9f) {
		hi += 0x40;
	}

	return ((hi << 8) | lo);
}

unsigned long
sjis_to_euc(const unsigned char *from, unsigned long bytesleft, unsigned char **outbuf,
		int *outbytesleft, int charcase)
{
	unsigned char *to = (unsigned char *) *outbuf;
	unsigned long spaceleft = *outbytesleft;
	unsigned char inchar, inchar2;
	unsigned short int input, output;
	int errno = 0;

	while(bytesleft && spaceleft) {

		inchar = *from;

		if (inchar < 0x80) {				/* ASCII */
			from++;
			bytesleft--;

			if(charcase)
				*to++ = _tolower(inchar);
			else
				*to++ = inchar;
			spaceleft--;

		} else if (inchar > 0xa0 && inchar < 0xe0) { /* KATAKANA */

			if (spaceleft < 2) {
				errno=E2BIG;
				goto errret2;
			}
			from++;
			bytesleft--;

			*to++ = SS2;
			*to++ = inchar;
			spaceleft -= 2;

		} else if (inchar == 0x80 || inchar == 0xa0 ||	/* ILLEGAL */
				inchar >= 0xf0) {
			errno=EILSEQ;
			goto errret2;

		} else {					/* 2 BYTE */

			if (bytesleft < 2) {
				errno = EINVAL;
				goto errret2;
			}

			inchar2 = *(from+1);

			if (inchar2 < 0x40 || inchar2 == 0x7f ||
				inchar2 > 0xfc) {		/* ILLEGAL */

				errno=EILSEQ;
				goto errret2;
			} else {				/* KANJI */
				if (spaceleft < 2) {
					errno=E2BIG;
					goto errret2;
				}
				from += 2;
				bytesleft -= 2;

				input = (inchar << 8) | inchar2;
				output = s2u(input);
				*to++ = (unsigned char) (output >> 8);
				*to++ = (unsigned char) output;
				spaceleft -= 2;
			}
		}
	}

	if (bytesleft != 0) {		/* if everything was not converted */
		errno = E2BIG;
	}
errret2:
	*outbytesleft = spaceleft;
	*outbuf = to;
	return errno;
}

unsigned short int
s2u(unsigned short int c)
{
	unsigned short int hi, lo;

	hi = ( c >> 8 ) & 0xff;
	lo = c & 0xff;
	hi -= ( hi <= 0x9f) ? 0x71 : 0xb1;
	hi = hi * 2 + 1;

	if ( lo > 0x7f) {
		lo--;
	}

	if ( lo >= 0x9e) {
		lo -= 0x7d;
		hi++;
	} else {
		lo -= 0x1f;
	}

	return (((hi << 8) | lo) | 0x8080);
}

int
ccnv_dummy()
{
	return 0;
}

MOD_MISC_WRAPPER(ccnv, ccnv_dummy, ccnv_dummy, "eucJP/SJIS Codeset Conversion Module");

int
dos2unixfn(unsigned char *dn, unsigned char *un, int *len)
{
	int i;
	int ni;
	int ei;
	int thislong = 0;
	unsigned char c;
	int error;

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
	i = 24;		/* probable bug in dosfs->readdir; could overwrite
				buffer */
	if(error =  sjis_to_euc(dn,ni+1,&un,&i,1))
		return(error);
	thislong += 24 -i;

	/*
	 *  Now, if there is an extension then put in a period
	 *  and copy in the extension.
	 */
	if (ei >= 8) {
		*un++ = '.';
		thislong++;
		for (i = 8; i <= ei; i++) {
			c = dn[i];
			*un++ = _tolower(c);
			thislong++;
		}
	}
	*un++ = 0;


	*len = thislong;
	return 0;
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
	unsigned char *saveun;
	int saveunlen;
	int error;

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

	saveun = un;
	saveunlen = unlen;
	while (unlen  &&  (c = *un)  &&  c != '.') {
		un++;
		unlen--;
	}
	saveunlen = saveunlen - unlen;

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
		dn[i] = _toupper(c);
		un++;
		unlen--;
	}
	i = 8;
	error = euc_to_sjis(saveun,saveunlen,&dn,&i,1);
	/* OK to silently truncate too long file names */
	if(error == E2BIG)
		error = 0;
	/*
	 *  If the first char of the filename is 0xe5, then translate
	 *  it to 0x05.  This is because 0xe5 is the marker for a
	 *  deleted directory slot.  I guess this means you can't
	 *  have filenames that start with 0x05.  I suppose we should
	 *  check for this and doing something about it.
	 */
	if(dn[0] == 0xe5)
		dn[0] = 0x05;

	return(error);


}
unsigned long
ccnv_unix2dos(char **from, char *to, int length, int dosnamespace)
{
	unsigned long cc_errno;
	char *oto = to;

	length--;
	if((cc_errno = euc_to_sjis((unsigned char *)*from,strlen(*from),
			(unsigned char**)&to,&length,dosnamespace)) != 0)
		return(cc_errno);
	*to = '\0';
	*from = oto;
	return(0);
}
unsigned long
ccnv_dos2unix(char **from, char *to, int length,int dosnamespace)
{
	unsigned long cc_errno;
	char *oto = to;

	length--;
	if((cc_errno = sjis_to_euc((unsigned char *)*from,strlen(*from),
			(unsigned char**)&to,&length,dosnamespace)) != 0)
		return(cc_errno);
	*to = '\0';
	*from = oto;
	return(0);
}
					
	


/*
 *  Get rid of these macros before someone discovers
 *  we are using such hideous things.
 */
#undef	_toupper
#undef	_tolower
