/*		copyright	"%c%" 	*/

#ident	"@(#)trchan.c	1.2"
#ident  "$Header$"
/*
 *
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987.
 *	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 *
 *	MODIFICATION HISTORY
 *
 *	L000	02 Mar 88	scol!craig
 *	- Added double dead keys and reversible compose sequences to match
 *	  mapchan enhancements.
 *	- Added i/o/k options to select partial conversions.
 *	L001	04 Mar 88	scol!craig
 *	- Changes for increased buffer size.
 *	L002	31 Mar 88	scol!craig
 *	- Change for merged mapctrl - doesn't yet use nmap, currently ignores.
 */
static char sccsid[] = "@(#)trchan.c	22.1 89/11/14 ";
#include "defs.h"

static byte *buf, *buf2;				       /*L002*/
static byte comp_key;
static byte *inmap, *outmap, *p_dead, *p_comp, *p_seq, *p_str, *p_strbuf;
static long count;
static bool cont;
bool Dopt;
static bool isect;						       /*L000*/
static bool osect;						       /*L000*/
static bool ksect;						       /*L000*/

int convert();
char *malloc();

extern char *optarg;						       /*L000*/
extern int optind, opterr;					       /*L000*/

main(argc, argv)
int argc;
char **argv;
{
	int rc, i, c, d, state, reversed;			       /*L000*/
	byte *first, *last;
	byte savedc;
	byte *p;

	cont = isect = osect = ksect = FALSE;			/*begin *L000*/
	while ((c = getopt(argc, argv, "ciko")) != EOF) {
		switch (c) {
		case 'c':
			cont = TRUE;
			break;
		case 'i':
			isect = TRUE;
			break;
		case 'k':
			ksect = TRUE;
			break;
		case 'o':
			osect = TRUE;
			break;
		case '?':
			oops("usage: trchan [-ciko] mapfile\n");
		}
	}
	if (!(isect || ksect || osect))
		isect = ksect = osect = TRUE;
	if (optind != argc - 1)
		oops("usage: trchan [-ciko] mapfile\n");	/*end	*L000*/

	if ((buf = malloc(MAXBUFFER)) == NULL)			       /*L001*/
		oops("not enough memory\n");
	if ((buf2 = malloc(MAXBUFFER)) == NULL)			       /*L002*/
		oops("not enough memory\n");			       /*L002*/
	rc = convert(argv[optind], buf, buf2);			       /*L002*/
	if (rc == BAD_MAP_FILE)
		exit(1);
	else if (rc == NULL_MAP_FILE) {
		while ((c = getchar()) != EOF)
			putchar(c);
		exit(0);
	}
	inmap = buf;
	outmap = buf+256;
	comp_key = buf[512];
	p_dead = buf+512+1+1+(4*2);
	i = 512+1+1;
	p_comp = buf + GET(i);
	i += 2;
	p_seq = buf + GET(i);
	i += 2;
	p_str = buf + GET(i);
	i += 2;
	p_strbuf = buf + GET(i);
	state = 0;
	count = 0;
	while ((c = getchar()) != EOF) {
		++count;
		switch (state) {
		case 0:		/* not in a dead or compose sequence */
			if (isect && (d = inmap[c])) echo(d);	/*begin	*L000*/
			else if (c && ksect && !inmap[c]) {
				if (c == comp_key)
					state = 2;
				else {
					savedc = c;
					state = 1;
				}
			} else echo(c);				/*end	*L000*/
			break;
		case 1:		/* in a dead sequence, dead key is in savedc */
			if ((d = inmap[c]) && !isect) d = c;	/*begin	*L000*/
			if (!d) {
				if (comp_key && c == comp_key) {
					whoa("compose key found in dead key sequence\n");
					state = 2;
					continue;
				} else d = c;			/*end	*L000*/
			}
			p = p_dead;
			while (p < p_comp && *p < savedc)
				p += 2;
			if (*p != savedc) {
				/*
				 * the following should never happen
				 * if the mapchan file was correctly converted.
				 * ??should we bomb??
				 */
				whoa("unknown dead key: 0x%02x\n", savedc); 
				state = 0;
				continue;
			}
			first = p_seq + 2* (*(p+1));
			last = p_seq + 2* (*(p+3) -1 );
			p = first;
			while (p <= last && *p < d)
				p += 2;
			if (*p != d) {
				whoa("illegal dead key sequence: 0x%02x 0x%02x\n",
				     savedc, c);
				state = 0;
				continue;
			}
			echo(*(p+1));
			state = 0;
			break;
		case 2:		/* saw compose key */
			if ((d = inmap[c]) && !isect) d = c;	       /*L000*/
			if (!d) {
				if (c == comp_key) {
					whoa("compose key found in compose key sequence\n");
					state = 2;
					continue;
				} else
					d = c; /* dead key inside of compose */
			}
			savedc = d;
			state = 3;
			break;
		case 3:     /* in compose key sequence, 1st char is in savedc */
			if ((d = inmap[c]) && !isect) d = c;	       /*L000*/
			if (!d) {
				if (c == comp_key) {
					whoa("compose key found in compose key sequence\n");
					state = 2;
					continue;
				} else
					d = c; /* dead key inside of compose */
			}
			reversed = 0;				       /*L000*/
compscan:							       /*L000*/
			p = p_comp;
			while (p < p_seq-2 && *p < savedc)
				p += 2;
			if (*p != savedc) {
				if (!reversed) {		/*begin	*L000*/
					reversed = 1;
					i = savedc;
					savedc = d;
					d = i;
					goto compscan;
				}
				whoa("unknown 1st compose key: 0x%02x\n", savedc);
								/*end	*L000*/
				state = 0;
				continue;
			}
			first = p_seq + 2* (*(p+1));
			last = p_seq + 2* (*(p+3) - 1);
			p = first;
			while (p <= last && *p < d)
				p += 2;
			if (*p != d) {
				if (!reversed) {		/*begin	*L000*/
					reversed = 1;
					i = savedc;
					savedc = d;
					d = i;
					goto compscan;
				}
				whoa("unknown compose key sequence: 0x%02x 0x%02x\n",
				     d, savedc);		/*end	*L000*/
				state = 0;
				continue;
			}
			if (!*(p+1))				/*begin	*L000*/
				whoa("disallowed compose sequence: 0x%02x 0x%02x\n", savedc, d);
			else echo(*(p+1));			/*end	*L000*/
			state = 0;
			break;
		default:
			oops("unknown state\n");
		} /* end of switch */
	} /* end of while */
	exit(0);
}

echo(c)
byte c;
{
	byte d;
	byte *p, *first, *last;

    if (!osect) putchar(c);					       /*L000*/
    else {							       /*L000*/
	d = outmap[c];
	if (d || !c) {
		putchar(d);
		return;
	}
	p = p_str;
	while (p < p_strbuf-2 && *p < c)
		p += 2;
	if (*p != c)
		oops("unknown string character: 0x%02x\n", c);
	first = p_strbuf + *(p+1);
	last = p_strbuf + *(p+3) - 1;
	if (first > last)					       /*L002*/
		putchar(c);	/* Should mark start of no-map */      /*L002*/
	else							       /*L002*/
	for (p = first; p <= last; ++p)
		putchar(*p);
    }								       /*L000*/
}

whoa(fmt, args)
char *fmt;
int args;
{
	fprintf(stderr, "char %ld, ", count);
	vfprintf(stderr, fmt, &args);
	if (!cont)
		exit(1);
}
