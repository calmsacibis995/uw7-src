/*      copyright       "%c%"   */

#ident	"@(#)cut:cut.c	1.11.3.5"
#
#define _XOPEN_SOURCE 1
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <locale.h>	
#include <pfmt.h>
#include <limits.h>	/* INT_MAX, LINE_MAX */
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

typedef struct range {
	struct range	*next;
	unsigned long		low;
	unsigned long		high;
	} range_t;


static int	sflag;
static FILE    	*inptr;

static range_t	*firstp = NULL;
static range_t	*freep = NULL;

static char	*fldbuf = NULL;
static size_t	fldsize;

static char	*del = "\t";	/* default delimiter */
static int	delw = 1;


static wctype_t blank;


static char	msg_bcflist[] =
	":812:Bad list for b/c/f option\n";

static char	msg_nomem[] =
	":725:out of memory\n";

static char	msg_delimiter[] =
	":139:No delimiter\n";

static char	msg_longline[] =
	":141:Line too long\n";

static char	msg_usage[] =
	":811:Usage:\n\tcut -b list [-n] [file ...]\n"
	"\tcut -c list [file ...]\n"
	"\tcut -f list [-d char] [-s] [file ...]\n";

static char	msg_noflds[] =
	":140:No fields\n";


static void free_ranges( range_t *ptr );


static void
err_exit(int code, char *msg) {
	(void)pfmt(stderr, MM_ERROR, msg);
	free_ranges( firstp );
	free_ranges( freep );
	if( fldbuf != NULL )
		free( fldbuf );
	exit(code);
}

/*
 * Return a free range_t
 */

static
range_t *
range_alloc(void) {
range_t *r;
	if( freep == NULL ) {
		freep = (range_t*) malloc(sizeof(range_t));
		if( freep == NULL )
			err_exit(2,msg_nomem);
		freep->next = NULL;
	}
	r = freep;
	freep = freep->next;
	return r;
}

/*
** "low" is lower boundary or 1 if not given
** "high" is higher boundary or LONG_MAX if not given
*/
static void
new_range(unsigned long low, unsigned long high) {
	
	range_t	*lowp, *highp;
	range_t	*olowp, *ohighp;	/* old (previous) */

	
	/* Search for range overlapping lower boundary (in olowp)  */
	for(	olowp = NULL, lowp = firstp;
		(lowp != NULL) && (low-1 > lowp->high);
		olowp = lowp, lowp = lowp->next
		);

	/* If there is not range adjacent to lower boundary or */
	/* overlapping it then append new range "low-high" */
	if( lowp == NULL ) {
		range_t *r = range_alloc();
		
		r->low = low;
		r->high = high;
		r->next = NULL;

		if (olowp == NULL) {
			firstp = r;
		} else
			olowp->next = r;
		
		return;
	}

	/* Search for range overlapping higher boundary (in ohighp)  */
	for(	ohighp = NULL, highp = lowp;
		(highp != NULL) && (high+1 >= highp->low);
		ohighp = highp, highp = highp->next
		);

	/* If higher boundary of the new range is bigger than all already */
	/* selected ranges than extend high boundary of range overlapping */
	/* lower boundary. Release ranges covered by new one */
	if( highp == NULL ) {
		if( low < lowp->low )
			lowp->low = low;

		lowp->high = (high > ohighp->high)
				? (high) : ohighp->high;	

		if ( lowp->next != NULL ) {
			ohighp->next = freep;
			freep = lowp->next;
		}

		lowp->next = NULL;
		return;
	}

	/* If there is no overlapping or adjacent range than */
	/* insert new range and return */
	if( lowp == highp ) {
		range_t *r = range_alloc();
		
		r->low = low;
		r->high = high;
		r->next = highp;

		if (olowp == NULL) {
			firstp = r;
		} else
			olowp->next = r;
		
		return;
	}
		
	/* There is at least one overlapping range. Extend its boundary */
	/* and release ranges covered by new one (if any) */
	if( low < lowp->low )
		lowp->low = low;

	lowp->high = (high > ohighp->high)
		? high : ohighp->high;

	if( lowp != ohighp ) {
		if ( lowp->next != NULL ) {
			ohighp->next = freep;
			freep = lowp->next;
		}
	}
	lowp->next = highp;
	return;
}


static void
free_ranges(range_t *ptr) {
	range_t *nptr;	/* next ptr */

	while( ptr != NULL ) {
		nptr = ptr->next;
		free(ptr);
		ptr = nptr;
	}
}
		

static void
parse_list(char *list) {

	register char	*p;
	unsigned long		low, high;

	low=0; high=0;
	if ((list == NULL)||(*list=='\0'))	/* is there something to do? */
		err_exit(2,msg_bcflist);

	p = list;
	do {
		while (iswctype((int)*p,blank)) p++;	/* skip blanks */

		if( isdigit(*p) ) {		/* starts with digit */
			low = strtol(p,&list,10);
			p = list;
			if( *p != '-' ) {	/* number only */
				high = low;
			} else {		/* range */
				p++;
				if( isdigit(*p) ) {
                        		high = strtol( p, &list, 10);
					p = list;
				} else {
					high = LONG_MAX;
				}
			}
		} else if( *p == '-' ) {	/* starts with '-' */
			p++;
			if( isdigit(*p) ) {	/* range 1-... */
				low  = 1;
				high = strtol( p, &list, 10);
				p = list;
			} else {
				err_exit(2,msg_bcflist);
			}
		} else {
			/* if input list is a quoted empty string */
			if(*p == '\0')
				err_exit(2,msg_noflds);
			else
				err_exit(2,msg_bcflist);
			
		}

		/* Check if given range make sense. */
		if(low > high || !low || !high ) 
			err_exit(2,msg_bcflist);
	
		new_range(low, high);

		while (iswctype((int)*p,blank)) p++;
		if( *p == ',' ) {
			p++;
			continue;	/* another range should follow */
			}
	} while( *p != '\0' );
}


static void
b_opt(void) {
	int	c;
	unsigned long		poscnt;
	range_t		*rangep;
	int hi, lo;

	/* for all lines of a file */
	for(;;) {
		/* starting a new line */
		poscnt = 0;
		rangep = firstp;	
		hi = rangep->high;
		lo = rangep->low;

		/* for all chars of the line */
		while (((c = getc(inptr)) != '\n') && (c != EOF)) {
			poscnt++;
			if(poscnt > hi) {
				if((rangep = rangep->next) == NULL) 
					lo = hi = LONG_MAX;
				else {
					hi = rangep->high;
					lo = rangep->low;
				}
			}
			if(poscnt >= lo && poscnt <= hi)
				(void)putchar(c);
		}
		if(c == EOF)
			break;

		(void)putchar('\n');
	}
}



static void
bn_opt(void) {
	wint_t	wc;
	unsigned long		poscnt;
	range_t		*rangep;
	char 		*junk;
	int 		lo, hi;

	if((junk = malloc(MB_CUR_MAX)) == NULL) {
		pfmt(stderr,MM_ERROR,":864:malloc failure\n");
		exit(1);
	}
	/* for all lines of a file */
	for(;;) {
		/* starting a new line */
		poscnt = 0;
		rangep = firstp;	
		hi = rangep->high;
		lo = rangep->low;

		/* for all chars of the line */
		while (((wc = getwc(inptr)) != '\n') && (wc != WEOF)) {
			/* All we really want is the length of the character */
			poscnt += wctomb(junk,wc);
			if(poscnt > hi) {
				if((rangep = rangep->next) == NULL) 
					lo = hi = LONG_MAX;
				else {
					hi = rangep->high;
					lo = rangep->low;
				}
			}
			if(poscnt >= lo && poscnt <= hi)
				(void)putwchar( wc );
		}

		if(wc == WEOF)
			break;
		(void)putchar('\n');
	}
	(void) free(junk);
}



static void
c_opt(void)
{
	register wint_t	wc;
	unsigned long		poscnt;
	range_t		*rangep;
	int hi, lo;

	/* for all lines of a file */
	for(;;) {
		/* starting a new line */
		poscnt = 0;
		rangep = firstp;
		lo = rangep->low;
		hi = rangep->high;

		/* for all chars of the line */
		while (((wc = getwc(inptr)) != '\n') && (wc != WEOF)) {
			poscnt++;
			if(poscnt > hi) {
				if((rangep = rangep->next) == NULL) 
					lo = hi = LONG_MAX;
				else {
					hi = rangep->high;
					lo = rangep->low;
				}
			}
			if(poscnt >= lo && poscnt <= hi)
				(void)putwchar( wc );
		}
		if(wc == WEOF)
			break;

		(void)putchar('\n');
	
	} 
}



static void
f_opt(void)
{
	unsigned long		poscnt;
	range_t		*rangep;
	int		outcnt;
	wint_t 	wc;
	wchar_t		delimiter;
	wchar_t		*wbuf = NULL, *wbufptr;

	int 		wsize = 0;
	int 		wcnt = 0;
	int 		hi, lo;


	if(mbtowc(&delimiter,del,strlen(del)) != strlen(del)) {
		pfmt(stderr,MM_ERROR,msg_delimiter);
		exit(1);
	}
	/* for all lines of a file */
    for(;;) {   
		/* starting a new line */
		poscnt = 0;
		rangep = firstp;
		outcnt = 0;
		wbufptr = wbuf;
		lo = rangep->low;
		hi = rangep->high;

		for(;;) {
			wc = getwc(inptr);
			if(wbufptr + 1 > wbuf + wsize) {
				wcnt = wbufptr - wbuf;
				if((wbuf = (wchar_t *) realloc(wbuf,(wsize+LINE_MAX)*sizeof(wchar_t))) == NULL) {
					pfmt(stderr,MM_ERROR,":864:malloc failure\n");
					exit(1);
				}
				wbufptr = wbuf + wcnt;
				wsize +=LINE_MAX;
			}
			if(wc == delimiter || ((wc == '\n' || wc
			== WEOF) && poscnt > 0)) {
				poscnt++;
				if(poscnt > hi) {
					if((rangep = rangep->next) == NULL) 
						lo = hi = LONG_MAX;
					else {
						hi = rangep->high;
						lo = rangep->low;
					}
				}
				if(poscnt >= lo && poscnt <= hi) {
					*wbufptr = '\0';
					if(outcnt++ != 0)
						putwchar(delimiter);
					fputws(wbuf,stdout);
				}
				if(wc == '\n' || wc == WEOF)
					break;
				wbufptr = wbuf;
				continue;
			}
			else if(wc == '\n' || wc == WEOF)
				break;
			*wbufptr++ = wc;
		}

		if(poscnt == 0) {
			if(!sflag && wbufptr != NULL) {
				*wbufptr = '\0';
				fputws(wbuf,stdout);
			}
		}
		if(wc == WEOF)
			break;
		if(poscnt > 0 || !sflag)
			putchar('\n');
    } 
}

void
usage(int complain) {
	if (complain)
		pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
	pfmt(stderr, MM_ACTION, msg_usage);
	exit(2);
}


int 
main(int argc, char **argv) {

	char		*list = NULL;
	int		bflag,cflag,fflag,nflag;
	int		retcode = 0;
	int		filenr = 0;
	int		c;


	bflag = nflag = cflag = fflag = sflag = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:cut");

	blank = wctype("blank");

	while((c = getopt(argc, argv, "b:c:d:f:ns")) != EOF)
		switch( c ) {
			case 'b':
				if (cflag || fflag || sflag)
					usage(1);
				bflag = 1;
				list = optarg;
				break;

			case 'c':
				if (bflag || fflag || nflag || sflag)
					usage(1);
				cflag = 1;
				list = optarg;
				break;

			case 'd':
				if (bflag || cflag || nflag)
					usage(1);
				del = optarg;
				break;

			case 'f':
				if (bflag || cflag || nflag)
					usage(1);
				fflag = 1;
				list = optarg;
				break;

			case 'n':
				if (cflag || fflag || sflag)
					usage(1);
				nflag = 1;
				break;

			case 's':
				if (bflag || cflag || nflag)
					usage(1);
				sflag = 1;
				break;

			default:
				usage(0);
		}

	if( !(cflag || fflag || bflag) )
		err_exit(2,msg_bcflist);

	argv = &argv[optind];
	argc -= optind;

	parse_list( list );
	if( firstp == NULL)	/* should already be covered in parse_list() */
		err_exit(2,msg_bcflist);

	do {	/* for all input files */
		if( argc == 0 || strcmp(argv[filenr], "-") == 0 ) {
			inptr = stdin;
		} else {
			if( (inptr = fopen(argv[filenr],"r")) == NULL ) {
				(void)pfmt(stderr, MM_WARNING,
					":92:Cannot open %s: %s\n",
					argv[filenr], strerror(errno));
				retcode = 1;
				continue;
			}
		}

		if( bflag ) {
			if( nflag ) {
				bn_opt();
			} else {
				b_opt();
			}
		} else if( cflag ) {
			c_opt();
		} else if( fflag ) {
			f_opt();
		} else {	/* this should be already covered */
			usage(1);
		}

		if( inptr != stdin ) {
			(void) fclose( inptr );
		}
	} while( ++filenr < argc );

	free_ranges( freep );
	free_ranges( firstp );
	if( fldbuf != NULL ) {
		free( fldbuf );
	}

	return(retcode);
}


