#ident	"@(#)localedef:get.c	1.1"
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include "_colldata.h"
#include "_localedef.h"



/* UTF-8 multibyte to wide character representation */
int
_umbtowc(wchar, s, n)
wchar_t *wchar;
const char *s;
size_t n;
{
	unsigned char c0 = *s;
	unsigned long wc;
	int length, tl;

	if(c0 < 0xc0) {
		goto _ubad;
	}

	if(c0 < 0xe0) {
		length = 2;
		wc = c0 & 0x1f;
	}else
	if(c0 < 0xf0) {
		length = 3;
		wc = c0 & 0xf;
	} else
	if(c0 < 0xf8) {
		length = 4;
		wc = c0 & 0x7;
	} else
	if(c0 < 0xfc) {
		length = 5;
		wc = c0 & 0x3;
	} else {
		length = 6;
		wc = c0 & 0x1;
	}

	if(length > n) {
		goto _ubad;
	}
	tl = length;
	s++;
	while(--length) {
		c0 = *s++ ^ 0x80;
		if(c0 & 0xc0) {
			goto _ubad;
		}
		wc = (wc << 6) | c0;
	}
	if(wchar != 0)
		*wchar = wc;
	return(tl);
_ubad:
/*
	errno = EILSEQ;
*/
	return -1;
}


/* UTF-8 version of wide character to multibyte representation */
int
_uwctomb(s, wchar)
register unsigned char *s;
register CODE wchar;
{
	unsigned char mask;
	int length,tl;

	if(wchar <= 0x7f) {
		*s = wchar;
		return(1);
	}
	if(wchar <= 0x7ff) {
		length = 1;
		mask = 0xc0;
	} else
	if(wchar <= 0xffff) {
		length = 2;
		mask = 0xe0;
	} else
	if(wchar <= 0x1ffff) {
		length = 3;
		mask = 0xf0;
	} else
	if(wchar <= 0x3ffff) {
		length = 4;
		mask = 0xf8;
	} else {
		length = 5;
		mask = 0xfc;
	}
	tl = length;
	while(length) {
		*(s+length) = 0x80 | (wchar & 0x3f);
		wchar >>= 6;
		length--;
	}
	*s= wchar | mask;
	return(tl+1);
}

/* multibyte to wide character and wide character to multibyte from libc 
   - copy needed here so references locale being built by localedef rather 
   than locale in environment or C locale 
*/
int
mymbtowc(CODE *p, const unsigned char *s, size_t n)
{
	register int ch;
	register const unsigned char *us;
	register int len;
	register wchar_t wc;
	int retval;
	wchar_t mask;

	if ((us = (const unsigned char *)s) == 0)
		return 0;	/* stateless encoding */
	if (n == 0)
		return -1;	/* don't set errno for too few bytes */
	if ((ch = *us) < 0x80 || !MULTIBYTE)
	{
	onebyte:;
		if (p != 0)
			*p = ch;
		return ch != '\0';
	}
	if(ENCODING == MBENC_UTF8)
		return(_umbtowc(p,s,n));

	if (ch < 0xa0)	/* C1 (i.e., metacontrol) byte */
	{
		if (ch == SS2)
		{
			if ((len = EUCW2) == 0)
				goto onebyte;
			mask = P01;
		}
		else if (ch == SS3)
		{
			if ((len = EUCW3) == 0)
				goto onebyte;
			mask = P10;
		}
		else
			goto onebyte;
		wc = 0;
		retval = len + 1;
	}
	else
	{
		if ((len = EUCW1) == 0)
			goto onebyte;
		mask = P11;
		wc = ch & 0x7f;
		retval = len;
		if (--len == 0)
			goto out;
	}
	if (retval > n)
		return -1;	/* don't set errno for too few bytes */
	do
	{
		if ((ch = *++us) < 0x80)
		{
		/*	errno = EILSEQ; */
			return -1;
		}
		wc <<= 7;
		wc |= ch & 0x7f;
	} while (--len != 0);
out:;
	if (p != 0)
		*p = wc | mask;
	return retval;
}

int
mywctomb(unsigned char *s, register CODE wc)
{
	register unsigned char *us;
	register int n, len, cs1;

	if ((us = (unsigned char *)s) == 0)
		return 0;	/* stateless encoding */
	if(ENCODING == MBENC_UTF8)
		return(_uwctomb(s,wc));
	switch ((wc >> EUCDOWN) & DOWNMSK)
	{
	default:
	bad:;
		/* errno = EILSEQ; */
		return -1;
	case DOWNP00:
		if ((wc & ~(wchar_t)0xff) != 0
			|| wc >= 0xa0 && MULTIBYTE && EUCW1 != 0)
		{
			goto bad;
		}
		*us = wc;
		return 1;
	case DOWNP11:
		n = EUCW1;
		cs1 = 1;
		break;
	case DOWNP01:
		n = EUCW2;
		*us++ = SS2;
		cs1 = 0;
		break;
	case DOWNP10:
		n = EUCW3;
		*us++ = SS3;
		cs1 = 0;
		break;
	}
	switch (len = n)	/* fill in backwards */
	{
	case 0:
		goto bad;
#if MLEN_MAX > 5
	default:
		us += n;
		do
		{
#if UCHAR_MAX == 0xff
			*--us = wc | 0x80;
#else
			*--us = (wc | 0x80) & 0xff;
#endif
			wc >>= 7;
		} while (--n != 0);
		break;
#endif /*MLEN_MAX > 5*/
	case 4:
#if UCHAR_MAX == 0xff
		us[3] = wc | 0x80;
#else
		us[3] = (wc | 0x80) & 0xff;
#endif
		wc >>= 7;
		/*FALLTHROUGH*/
	case 3:
#if UCHAR_MAX == 0xff
		us[2] = wc | 0x80;
#else
		us[2] = (wc | 0x80) & 0xff;
#endif
		wc >>= 7;
		/*FALLTHROUGH*/
	case 2:
#if UCHAR_MAX == 0xff
		us[1] = wc | 0x80;
#else
		us[1] = (wc | 0x80) & 0xff;
#endif
		wc >>= 7;
		/*FALLTHROUGH*/
	case 1:
#if UCHAR_MAX == 0xff
		us[0] = wc | 0x80;
#else
		us[0] = (wc | 0x80) & 0xff;
#endif
		break;
	}
	if (!cs1)
		len++;
	else if (*us < 0xa0)	/* C1 cannot be first byte */
		goto bad;
	return len;
}

/* get a symbolic character i.e., <name> */
unsigned char *
getsymchar(unsigned char **line_remain)
{
	unsigned char *line, *tline, *dest;

	line = *line_remain;
	line = skipspace(line);
	if(*line != '<')
		return(NULL);
	tline = ++line;
	dest = tline;
	/* escaped characters are themselves  in symbolic character names */
	while(isgraph(*tline) && *tline != '>') {
		if(*tline == escape_char) {
				tline++;
		}
		*dest++ = *tline++;
	}
	if(*tline != '>')
		return(NULL);
	*dest = '\0';
	*line_remain = tline+1;
	return(line);
}


/* look for a keyword starting at the beginning of the buffer given 
   and return a pointer to the structure associated with it - used by parse
*/
struct keyword_func *
getkeyword(unsigned char **line_remain, struct kf_hdr *kfh)
{
	unsigned char *tmp, schar;
	int i, c;
	struct keyword_func *kf;

	tmp = *line_remain;
	while(isgraph(*tmp))
		tmp++;
	if(tmp == *line_remain) 
			return(NULL);

	schar = *tmp;
	*tmp = '\0';
	i = kfh->kfh_numkf-1;
	kf= &(kfh->kfh_arraykf[i]);
	for(; i >= 0;kf--,i--) {
		if((c = strcmp((char *) *line_remain, kf->kf_keyword)) == 0) {
			*line_remain = tmp;
			break;
		}
		if(c > 0) {
			kf = NULL;
			break;
		}
	}
	if(i < 0)
		kf = NULL;
	*tmp = schar;
	return(kf);
}


	
/* Process a numerical representation of a character as a sequence of bytes.
   The interpreatation of the bytes depends on the encoding specified. */
CODE
getnum(unsigned char **line_remain)
{
	CODE num = WEOF, tnum = WEOF;
	unsigned int n;
	unsigned char *line = *line_remain;
	char *format;
	unsigned char buf[MB_LEN_MAX];
	unsigned char *bufptr;
	mboolean_t uflag = FALSE;

	bufptr = buf;

	while(!EOL(line)) {
		if(*line != escape_char) {
			break;
		}
		switch(*++line) {

			case 'D':
			case 'd':
				format = "%d%n";
				break;
			default:
				line--;
				format = "%o%n";
				break;
			case 'u':
			case 'U':
				uflag = TRUE;
			case 'X':
			case 'x':
				format = "%x%n";
				break;
		}

		/* will still accept \x0x23 for example; do we care? NO */
		if(sscanf((char *)++line, format, &tnum, &n) != 1) {
			line = line - 2;
			break;
		}
		if(tnum < 0)
			return(WEOF);

		if(uflag) {
			*line_remain = line +n;
			return(tnum);
		}
			
		/* cannot specify more than one byte per number */
		if(tnum > 0xff) 
			return(WEOF);
	
		line += n;
		/* strip off leading zeros */
		if(tnum == 0 && bufptr == buf) 
			continue;

		if(bufptr - buf == MB_LEN_MAX) {
			diag(LIMITS,TRUE,":97:Multibyte character longer than %d bytes\n",MB_LEN_MAX);
			return(WEOF);
		}
		*bufptr++ = (unsigned char) tnum;
	}
	if(bufptr != buf) {
		n = bufptr - buf;
		if(n > 1) {
			/* taken as multibyte representation of character */
			if(mymbtowc(&num,buf, n) != n) {
				return(WEOF);
			}
		}
		else {
			num = *buf;
		}
		*line_remain = line;
	}
	else if(tnum == 0)
		return(0);
	return(num);
}



			
unsigned char *
skipspace(unsigned char *x)
{
	while(isspace(*x))
		x++;
	return(x);
}


struct codent *
getcolchar(unsigned char **line_remain, unsigned char specchar, unsigned char **symname)
{
		unsigned char *name;
		CODE code;
		mboolean_t errret;
		struct syment *tsym;
		struct codent *tcode;

		if(symname != NULL)
			*symname = NULL;
		if((name = getsymchar(line_remain)) != NULL) {
			if((tsym = findsym(name)) == NULL) {
				/* name is only returned if valid symbolic name but not found */
				if(symname != NULL)
					*symname = name;
				return(NULL);
			} else
				return(tsym->sy_codent);
		}
		else if((code = (CODE) getnum(line_remain)) != WEOF) {
			return(findcode(code));
		}
		name = *line_remain;
		/* This is somewhat more liberal than the std requires.  There it says
		   that inside strings >, ", and the escape character and outside
		   strings comma, ;, <, and, > must be escaped to be interpreted as 
		   themselves. */
		if(*name == specchar)
			return(NULL);
		if(*name == escape_char) {
			name++;
			switch(*name) {
				case 'n':
					*name = '\n';
					break;
				case 't':
					*name = '\t';
					break;
				case 'r':
					*name = '\r';
					break;
				case 'a':
					*name = '\a';
					break;
				case 'b':
					*name = '\b';
					break;
				case 'f':
					*name = '\f';
					break;
				case 'v':
					*name = '\v';
					break;

			}
		}
		if(EOL(name))
			return(NULL);
		if((tcode = findcode((CODE)*name)) != NULL) {
			*line_remain = ++name;
			return(tcode);
		}
		return(NULL);
}


/* Get a character, but do not include names associated with special
   collation constructs.
*/
struct codent *
getlocchar(unsigned char **line_remain, unsigned char specchar, unsigned char **symname)
{

	struct codent *tcode;

	if((tcode = getcolchar(line_remain,specchar,symname)) == NULL ||
		tcode->cd_flags & (MCCE_COLL|SYM_COLL))
			return(NULL);
	return(tcode);
}
			

/* service routine to get a string of characters surrounded by quotes 
   numallowed == 0 means any number of characters allowed */
unsigned char *
getstring(unsigned char **line, unsigned char *dest, unsigned int numallowed,
	mboolean_t mballowed)
{
	unsigned char *tline = *line;
	unsigned int n, count = 0;
	struct codent *tcode;

	tline = skipspace(tline);
	if(*tline++ != '"')
		goto badstr;

	while((tcode = getlocchar(&tline, '"',NULL)) != NULL) {

		/* keep track of how many characters there are vs how many allowed */
		if(numallowed != 0 && ++count > numallowed) {
			diag(LIMITS, TRUE,":113:Only %d charaters allowed in string\n",numallowed);
			return(NULL);
		}

		if(tcode->cd_code <= 0x7f) {
			*dest++ = tcode->cd_code;
		}
		else if(ENCODING == MBENC_NONE && tcode->cd_code <= 0xff) {
				*dest++ = tcode->cd_code;
		}
		else {
			/* If multibyte characters are not allowed, complain */
			if(!mballowed) {
				diag(LIMITS,TRUE,":98:Multibyte characters may not be specified.\n");
				return(NULL);
			}
			if((n = mywctomb(dest,tcode->cd_code)) == -1) {
				diag(ERROR, TRUE, ":83:Invalid wide character 0x%x\n",tcode->cd_code);
				return(NULL);
			}
			dest+=n;
		}
	}
	if(*tline++ == '"') {
		*line = tline;
		return(dest);
	}

badstr:
	diag(ERROR, TRUE, ":138:Unknown character in string or Malformed string\n");
	return(NULL);
}
