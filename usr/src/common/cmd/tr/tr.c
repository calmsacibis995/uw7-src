/*		copyright	"%c%" 	*/

#ident	"@(#)tr:tr.c	1.4.3.1"
#ident "$Header$"

/* tr - transliterate data stream */

#include <stdarg.h>
#include "tr.h"

#define EOS	257	/* End of string marker */
#define MAXWCL	500	/* 
			 * Estimated maximum number of wide characters
			 * that will be returned by _wcl_* routines.
			 */

int mode;		/* Options */
int iseuc;		/* used only for cross codeset range rejection */
#ifdef DEBUG
static int debug;
#endif

static chs	*newchs(chs *, int , wint_t, unsigned long );
static void	addtostring(chs **, int , ...);
static chs	*BuildString(const char *, int);
static wint_t	nextc(const char **, wint_t *);
static void	changecase(chs *, chs *);
#ifdef DEBUG
static void	PrintString(chs *);
#endif  
static wint_t	next(chs **, int);
extern int	mtr(chs *, chs *);

static void
usage(int msgflag)
{
	if (msgflag)
		(void) pfmt(stderr, MM_ERROR, ":1396:Incorrect usage\n");
	(void) pfmt(stderr, MM_ACTION, ":1405:Usage:\n\ttr [-cs] string1 string2\n\ttr -s[-c] string1\n\ttr -d[-c] string1\n\ttr -ds[-c] string1 string2\n");
	exit(1);
}

/*
 * Print an message and exit if an error occurs when parsing a string 
 * argument.
 */
static void
badstring(const char *str, const char *fmt, ...) {
va_list ap;
	va_start(ap, fmt);
	if (str == NULL)
		(void) pfmt(stderr, MM_ERROR, ":1390:Bad string:\n");
	else
		(void) pfmt(stderr, MM_ERROR, ":1389:Bad string: %s:\n", str);
	(void) vpfmt(stderr, MM_ERROR, fmt, ap);
	exit(1);
}

main(int argc, char *argv[]) {
register i, c, d;
int j, errflg=0;
int *compl;
chs *string1, *string2;
int code[UCHAR_MAX+1];
int squeez[UCHAR_MAX+1];
int vect[UCHAR_MAX+1];
int save = EOS;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:tr");

	iseuc = (MB_CUR_MAX == 5); /* ugh...sorry about this */

#ifdef DEBUG
	while ((c=getopt(argc, argv, "Wxcds")) != EOF) {
#else
	while ((c=getopt(argc, argv, "cds")) != EOF) {
#endif
		switch(c) {
		case 'c':
			mode |= TR_COMPLEMENT;
			continue;
		case 'd':
			mode |= TR_DELETE;
			continue;
		case 's':
			mode |= TR_SQUEEZE;
			continue;
#ifdef DEBUG
		case 'W':
			debug |= 2;
			continue;
		case 'x':
			debug |= 1;
			continue;
#endif
		case '?':
			errflg++;
			break;
		}
	}

        /*
         *  As this command accepts stdin as default, we need to
         *  purge the standalone '-' option for further processing.
         */
        if ((argv[argc-1][0] == '-') && (argv[argc-1][1] == '\0')) {
            argv[argc-1][0] = '\0';
            argc--;
        }

	if (errflg) 
		usage(0);
	else {
		switch(argc - optind) {
		case 2:
			if (mode & TR_DELETE && !(mode & TR_SQUEEZE))
				usage(1);
			if ((mode&(TR_DELETE|TR_SQUEEZE))!=(TR_DELETE|TR_SQUEEZE))
				mode |= TR_MAP;
			string2 = BuildString(argv[optind+1], 2);
			string1 = BuildString(argv[optind], 1);
			break;
		case 1:
			if ((mode & TR_DELETE && mode & TR_SQUEEZE)
			  ||(!(mode & TR_DELETE) && !(mode & TR_SQUEEZE)))
				usage(1);

			string1 = BuildString(argv[optind], 1);
			string2 = BuildString("", 1);
			break;
		default:
			usage(1);
		}
	}

#ifdef DEBUG
	if (debug & 1) {
		PrintString(string1);
		PrintString(string2);
	}
#endif
	/* Deal with pseudo class upper/lower */
	if (mode & TR_MAP)
		changecase(string1, string2);
#ifdef DEBUG
	if (debug & 1) {
		PrintString(string1);
		PrintString(string2);
	}
#endif

#ifdef DEBUG
	if (MB_CUR_MAX > 1 || debug & 2)
#else
	if (MB_CUR_MAX > 1) 
#endif
		exit(mtr(string1, string2));

	(void)memset(vect, 0, sizeof(vect));
	if (mode & TR_COMPLEMENT) {
		while ((c = next(&string1,0)) != EOS)
			vect[c] = 1;
		j = 0;
		for (i=0; i<=UCHAR_MAX; i++)
			if (vect[i]==0) vect[j++] = i;
		vect[j] = EOS;
		compl = vect;
	}

	for (i=0; i<=UCHAR_MAX; i++)
		code[i] = EOS;
	
	for (;;){
		if (mode & TR_COMPLEMENT) 
			c = *compl++;
		else 
			c = next(&string1, 0);
		if (c==EOS) 
			break;
		d = next(&string2, 0);
		if (d==EOS) 
			d = c;
		code[c] = d;
		squeez[d] = 1;
	}

	while ((d = next(&string2, 1)) != EOS)
		squeez[d] = 1;
	squeez[0] = 1;
	for (i=0;i<=UCHAR_MAX;i++) {
		if (code[i]==EOS) 
			code[i] = i;
		else if (mode & TR_DELETE) 
			code[i] = EOS;
	}

	while ((c=getc(stdin)) != EOF ) {
		if ((c = code[c]) != EOS)
			if (!(mode & TR_SQUEEZE) || c!=save || !squeez[c])
				(void) putchar(save = c);
	}
	return 0;
}


/*
 * Allocate a new chs structure and set the structure members.
 * Place it at the end of the list and return a pointer to it.
 */
static chs *
newchs(chs *s, int type, wint_t ch, unsigned long n) {
chs *c;

  if ((c = malloc(sizeof(chs))) == NULL) {
	(void)pfmt(stderr,MM_ERROR, ":1394:Cannot malloc: %s\n", strerror(errno));
	exit(1);
  }
  c->next = NULL;
  c->flags = type;
  if (type == CHS_CHARS) {
	if ((c->chrs = malloc(NCHARS * sizeof(wint_t))) == NULL) {
		(void)pfmt(stderr,MM_ERROR, ":1394:Cannot malloc: %s\n", 
				strerror(errno));
		exit(1);
	}
	c->chrs[0] = ch;
  } else
	c->chr = ch;
	
  c->nch = n;
  c->i = 0;
  if (s != NULL)
	s->next = c;
  return c;
}

/*
 * Add the character, range, array etc. passed as arguments
 * to string *s.
 */
static void
addtostring(chs **s, int type, ...) {
va_list ap;
wint_t w1, w2, *p;
unsigned long n;
int i;

  if (type == CHS_UPPER || type == CHS_LOWER)
	*s = newchs(*s, type, L'\0', 1);
  else {
	va_start(ap, type);

	switch(type) {
		case CHS_RANGE:
				w1 = va_arg(ap, wint_t);
				w2 = va_arg(ap, wint_t);
				n = w2 - w1;
				*s = newchs(*s, type, w1, n+1);
				break;
		case CHS_REPEAT:
				w1 = va_arg(ap, wint_t);
				n = va_arg(ap, unsigned long);
				*s = newchs(*s, type, w1, n);
				break;
		case CHS_CHARS:
				i = va_arg(ap, int);
				while (i--) {
					w1 = va_arg(ap, wint_t);
					if (*s != NULL &&
					   (*s)->flags==type&&(*s)->nch<NCHARS)
						(*s)->chrs[(*s)->nch++] = w1;
					else {
						*s = newchs(*s, type, w1, 1);
					}
				}
				break;
		case CHS_ARRAY: p = va_arg(ap, wint_t *);
				n = va_arg(ap, unsigned long);
				*s = newchs(*s, type, L'\0', n);
				(*s)->flags = CHS_CHARS;
				(*s)->chrs = p;
				break;
	}
	va_end(ap);
  }
}

/*
 * Allocate size wide characters, with error checking
 */
static wchar_t *
allocwc(size_t size) {
wchar_t *w;
  if ((w = malloc(size * sizeof(wchar_t))) == NULL) {
	(void)pfmt(stderr,MM_ERROR, ":1394:Cannot malloc: %s\n", strerror(errno));
	exit(1);
  }
  return w;
}
	
/*
 * Parse a string passed as an operand to tr and compile it into
 * a list of chs's.
 */
static chs *
BuildString(const char *arg, int string){
chs *str, *eos;		/* the beginning and end of the compile string */
wint_t w1, w2, w3, w4;	/* Wide characters used in parsing string */
int f1, f2, f3;		/* Flags for the respective wide character */
wint_t prev = L'\0';	/* character left over from previous iteration */
const char *bad = arg;	/* save argument for error messages */
wchar_t *w;
size_t len;
char *save;		

  str = eos = newchs(NULL, CHS_CHARS, L'\0', 0);

  while ((w1 = prev) || (f1 = nextc(&arg, &w1)) != WEOF) {
	save=(char *) arg; /* So we can go back to the character after w1 */
	prev='\0';
	if (w1 == L'[') {
		/*
		 * Could be:
		 * [:class:], [=equiv=]	 locale information
		 * [c-c] SysV encoded value range.
		 * [c*n], where n is optional.
		 * incomplete [:, [=, [c- and [c* are treated as errors,
		 * [ on its own is treated as an ordinary character.
		 */
		if (nextc(&arg,&w2) == WEOF) {
			addtostring(&eos, CHS_CHARS, 1, w1);
			break;
		}
		if (w2 == L':') {
			/*
			 * Character class - if the class is "upper" or
			 * "lower", just make a note of the fact.
			 */
			char class[CHARCLASS_NAME_MAX+1];
			wctype_t type;
			int i = 0;


			/* Get class name - must be ASCII alphanumerics */
			if (setlocale(LC_CTYPE, "C") == NULL) {
				(void)pfmt(stderr, MM_ERROR, 
					":1393:Can't set LC_CTYPE to C locale");
				exit(1);
			}
			while (i < CHARCLASS_NAME_MAX && isalnum(*arg)) 
				class[i++] = *arg++;
			(void) setlocale(LC_CTYPE, "");
			class[i] = '\0';

			if (nextc(&arg, NULL)!= L':' ||nextc(&arg, NULL)!= L']')
				badstring(bad, ":1402:Missing :]\n");

			if ((type = wctype(class)) == 0)
				badstring(class, ":1399:Invalid class\n");

			if (mode & TR_MAP) {
				if (strcmp(class, "upper") == 0) {
					addtostring(&eos, CHS_UPPER);
					continue;
				} else if (strcmp(class, "lower") == 0) {
					addtostring(&eos, CHS_LOWER);
					continue;
				} else if (string == 2)
					badstring(bad, ":1406:[:class:] not allowed in string two\n");
			}
			w = allocwc(MAXWCL);
			if ((len = _wcl_class(w, MAXWCL, type)) == (size_t) -1)
				badstring(bad, ":1399:Invalid class\n");
			if (len > MAXWCL) {
				free(w);
				w = allocwc(len);
				if ((len = _wcl_class(w, len,type))==(size_t)-1)
					badstring(bad, ":1399:Invalid class\n");
			}
			addtostring(&eos, CHS_ARRAY, w, (unsigned long) len);
				
		} else if ( w2 == L'=') {
			/*
			 * All characters in the same equivalence class.
			 */

			if (string == 2 && mode&TR_MAP) 
				badstring(bad, ":1407:[:equiv:] not allowed in string two\n");

			if (nextc(&arg, &w3) == WEOF ||
			    nextc(&arg, NULL)!= L'=' ||nextc(&arg,NULL) != L']')
				badstring(bad, ":1403:Missing =]\n");

			w = allocwc(MAXWCL);
			if ((len = _wcl_equiv(w, MAXWCL, w3)) == (size_t) -1)
				badstring(bad, ":1400:Invalid equivalence class\n");
			if (len > MAXWCL) {
				free(w);
				w = allocwc(len);
				if ((len = _wcl_equiv(w, len, w3)) ==(size_t)-1)
					badstring(bad, ":1400:Invalid equivalence class\n");
			}
			addtostring(&eos, CHS_ARRAY, w, (unsigned long) len);
		} else {
			if (nextc(&arg, &w3) == WEOF) {
				addtostring(&eos, CHS_CHARS, 2, w1, w2);
				break;
			}
			if (w3 == L'-') {
				if (nextc(&arg, &w4) == WEOF || w4 < w2 
				 || (iseuc && (w2 >> 28) != (w4 >> 28)) /*ugh*/
			         || nextc(&arg, NULL) != L']')
					badstring(bad, ":1409:Bad range\n");
				addtostring(&eos, CHS_RANGE, w2, w4);
			} else if (w3 == L'*') {
				unsigned long n;
				char *tmp = (char *) arg;

				if (string == 1)
					badstring(bad, ":1408:[c*n] only allowed in string two\n");

				if (nextc(&arg, NULL) == L']')
					n = MAXWC; /* fill whole string */
				else {
					arg = tmp;
					errno = 0;
					n = strtoul(arg, (char **) &arg, 0);
					if (errno)
						badstring(bad, 
						   ":1392:Can't parse repeat number\n");
					if (nextc(&arg, NULL) != L']')
						badstring(bad, ":1404:Missing ] in [c*n]\n");
					if (n == 0L)
						n = MAXWC;
				}
				
				addtostring(&eos, CHS_REPEAT, w2, n);
			} else {
				arg=save; /* reset string to char after w1 */
				addtostring(&eos, CHS_CHARS, 1, w1);
			}
		}
	} else {
		/*
		 * Either a simple character or a range.
		 * If the range was specified using octal escape
		 * sequences, it is based on encoded character values
		 * (1003.2b), otherwise on LC_COLLATE ordering.
		 */
		if ((f2 = nextc(&arg,&w2)) == WEOF) {
			addtostring(&eos, CHS_CHARS, 1, w1);
			break;
		}
		if (w2 != L'-') {
			prev = w2; f1 = f2;
			addtostring(&eos, CHS_CHARS, 1, w1);
		} else {
			if ((f3 = nextc(&arg,&w3)) == WEOF) {
				addtostring(&eos, CHS_CHARS, 2, w1, w2);
				break;
			}
			if (w3 < w1 || (iseuc && (w1 >> 28) != (w3 >> 28))) /*ugh*/
				badstring(bad, ":1409:Bad range\n");
			if (f1 & CH_OCTAL || f3 & CH_OCTAL) {
				addtostring(&eos, CHS_RANGE, w1, w3);
			} else {
				w = allocwc(MAXWCL);
				if ((len=_wcl_range(w,MAXWCL,w1,w3))==(size_t)-1 || len == 0)
					badstring(bad, ":1401:Invalid range\n");
				if (len > MAXWCL) {
					free(w);
					w = allocwc(len);
					if ((len=_wcl_range(w,len,w1,w3))==(size_t)-1 || len==0)
						badstring(bad, ":1401:Invalid range\n");
				}
				addtostring(&eos, CHS_ARRAY, w, (unsigned long) len);
			}
		}
	}
  }
  return str;
}


/*
 * Return the next character from the compiled string.
 * if once is set, only return a repeated character once.
 */
static wint_t
next(chs **s, int once) {

  while (*s != NULL) {
	switch((*s)->flags) {
		case CHS_CHARS:	 if ((*s)->i++ < (*s)->nch)
					return (*s)->chrs[(*s)->i-1];
				 break;
		case CHS_RANGE:  if ((*s)->i++ < (*s)->nch)
					return (*s)->chr + (*s)->i-1;
				 break;
		case CHS_REPEAT: if ((*s)->i++ < (once ? once : (*s)->nch))
					return (*s)->chr;
				 break;
	}

	(*s)->i = 0;	/* Reset pointer */
	*s = (*s)->next;
  }
  return EOS;
  
}

/*
 * Return the next character from the string passed as argument.
 *
 * Perform escape processing on the string.
 *
 * Returns WEOF if the end of the string is reached,
 * CH_OCTAL if an octal number was specified,
 * the next character if the second argument is NULL,
 * 0 otherwise.
 *
 * stores the next character in the second argument if it is not NULL.
 *
 * increments the char * passed as the first argument to point to the 
 * following character.
 */

static wint_t
nextc(const char **s, wint_t *r){
wint_t w;
int l, n, i;
int flag = 0;

	if ((l = mbtowc(&w, *s, MB_CUR_MAX)) == 0)
		return WEOF;

	if (l < 0)
		badstring(*s, ":1398:Invalid character\n");
	*s += l;

	if (w == L'\\') {
		if ((l = mbtowc(&w, *s, MB_CUR_MAX)) == 0)
			return WEOF;
		if (l < 0)
			badstring(*s, ":1398:Invalid character\n");
		(*s) += l;
		
		switch(w) {
		case L'0':
		case L'1':
		case L'2':
		case L'3':
		case L'4':
		case L'5':
		case L'6':
		case L'7':
			i = n = 0;
			(*s)--;
			do {
				n = n * 8 + w - '0';
				i++;
				(*s)++;
			} while((i < 3) && ((w = **s) >= '0') && (w <= '7'));
			w = n;
			flag = CH_OCTAL;
			break;
		case L'a':
			w = '\a';
			break;
		case L'b':
			w = '\b';
			break;
		case L'f':
			w = '\f';
			break;
		case L'n':
			w = '\n';
			break;
		case L'r':
			w = '\r';
			break;
		case L't':
			w = '\t';
			break;
		case L'v':
			w = '\v';
			break;
		default:
			break;
		}  /* end switch(w) */
	}  /* end if (w == '\\') */

	if (r != NULL) {
		*r = w;
		return flag;
	} else
		return w;
}

/*
 * Scan through string one looking for placeholders for [:upper:] and
 * [:lower:].
 * If there is the corresponding placeholder in the same position in string 
 * two, then replace both with the toupper/tolower classes.  If not,
 * replace with upper/lower.
 *
 * After string one has been scanned, if there are any placeholders
 * in string two exit with an error as they are illegal.
 */
 
static void	
changecase(chs *s1, chs *s2){
unsigned long pos1 = 0;
unsigned long pos2 = 0;
unsigned int f;
wchar_t *w, *u, *v;
size_t len;
wctype_t type;
char *class;

  for (; s1 != NULL; s1 = s1->next) {
	pos1 += s1->nch;
	if (s1->flags == CHS_UPPER) {
		class = "upper";
		f = CHS_LOWER;
	} else if (s1->flags == CHS_LOWER) {
		class = "lower";
		f = CHS_UPPER;
	} else 
		continue;
	type = wctype(class);

	/* Look for corresponding placeholder in s2 */
	while (s2 != NULL) {
		pos2 += s2->nch;
		
		if (s2->flags == f) {
			if (pos2 == pos1) {
				/*
				 * A match - replace the palceholders
				 * with arrays of upper and lower case
				 * letters.
				 */
				w = allocwc(MAXWCL);
				if ((len=_wcl_class(w, MAXWCL, type))==(size_t) -1)
					badstring(NULL, ":1391:Can't expand class: %s\n", class);
				if (len > MAXWCL) {
					free(w);
					w = allocwc(len);
					if ((len = _wcl_class(w, len,type))==(size_t)-1)
						badstring(NULL, ":1391:Can't expand class: %s\n", class);
				}
				s1->flags = CHS_CHARS;
				s2->flags = CHS_CHARS;
				s1->chrs = v = w;
				s1->nch = len;
				s2->nch = len;
				u = w = allocwc(len);
				if (f == CHS_UPPER)
					while (len--)
						*(u++) = towupper(*(v++));
				else
					while (len--)
						*(u++) = towlower(*(v++));

				s2->chrs = w;
			} else
				badstring(NULL,
				   ":1406:[:class:] not allowed in string two\n");
		} else if (s2->flags == s1->flags)
				badstring(NULL, 
				   ":1406:[:class:] not allowed in string two\n");
		s2 = s2->next;
	}

	/*
	 * This placeholder wasn't matched, so expand it to an
	 * array of upper/lowercase letters.
	 */

	w = allocwc(MAXWCL);
	if ((len = _wcl_class(w, MAXWCL, type)) == (size_t) -1)
		badstring(NULL, ":1391:Can't expand class: %s\n", class);
	if (len > MAXWCL) {
		free(w);
		w = allocwc(len);
		if ((len = _wcl_class(w, len,type))==(size_t)-1)
			badstring(NULL, ":1391:Can't expand class: %s\n", class);
	}
	s1->flags = CHS_CHARS;
	s1->chrs = w;
	s1->nch = len;
  }

  while (s2 != NULL) {
	if (s2->flags == CHS_UPPER || s2->flags == CHS_LOWER)
		badstring(NULL, ":1406:[:class:] not allowed in string two\n");
	s2 = s2->next;
  }
	

}

#ifdef DEBUG
static void
PrintString(chs *str) {
chs *s;
int i;

  for (s = str; s != NULL; s=s->next) {
	switch((s)->flags) {
		case CHS_CHARS:	 for (i = 0; i< s->nch; i++)
					(void)putwchar(s->chrs[i]);
				 break;
		case CHS_RANGE:  (void)printf("%c - %c", s->chr, s->chr+s->nch-1);
				 break;
		case CHS_REPEAT: (void)printf("%c*%lu", s->chr, s->nch);
				 break;
		default:
				(void)printf("Type %d", (s)->flags);
	}
	(void)printf(" -> ");

  }
  (void)printf(" WEOF\n");
}
#endif

