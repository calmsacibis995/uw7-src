/*		copyright	"%c%" 	*/

#ident	"@(#)display.c	1.2"
#ident  "$Header$"
/*
 *
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987, 1988.
 *	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
/*
 *	MODIFICATION HISTORY
 *	L000	31 Mar 88	scol!craig
 *	- Added mapctrl functionality per Dougs request.
 */
/*
 * it is hard to say much about the displaying of the data
 * in the buffer that hasn't already been said in the design document.
 * read that first and then this code should be transparent.
 */
#include "defs.h"
#include "nmap.h"
#include <ctype.h>

static int base;
static bool beepflag;
static byte comp_key;
static byte *p_dead, *p_comp, *p_seq, *p_str, *p_strbuf;
static byte *inmap, *outmap;

display(buf, buf2, abase)					       /*L000*/
byte *buf, *buf2;						       /*L000*/
int abase;
{
	int i;

	base = abase;
	inmap = buf;
	outmap = buf+256;
	comp_key = buf[512];
	beepflag = buf[513];
	p_dead = buf+512+1+1+(4*2);
	i = 512+1+1;
	p_comp = buf + GET(i);
	i += 2;
	p_seq = buf + GET(i);
	i += 2;
	p_str = buf + GET(i);
	i += 2;
	p_strbuf = buf + GET(i);
	input();
	output();
	dead();
	compose();
	beep();
	control(buf2);						       /*L000*/
}

static
input()
{
	int i;

	printf("input\n");
	for (i = 0; i < 256; ++i)
		if (inmap[i]) {
			if (inmap[i] != i) {
				show_byte(i, SP);
				show_byte(inmap[i], NL);
			}
		}
	putchar('\n');
}

show_byte(i, c)
int i;
char c;
{
	if (isalnum(i))
		printf("'%c'", i);
	else
		printf((base == 8 )? "0%o":
		       (base == 10)? "%d":
		       (base == 16)? "0x%x":
				     "%d", i);
	putchar(c);
}

static
output()
{
	int i, j, first, last;
	byte *p;

	printf("output\n");
	for (i = 0; i < 256; ++i) {
		if (outmap[i] == i)
			continue;
		if (outmap[i]) {
			show_byte(i, SP);
			show_byte(outmap[i], NL);
		} else {
			p = p_str;
			/*
			 * the -2 below is necessary because
			 * of the extra terminating entry at the end.
			 */
			while (p < p_strbuf-2 && *p != i)
				p += 2;
			if (*p != i)
				oops("error in buf: %d not in strings\n", i);
			first = *(p+1);
			last = *(p+3) - 1;
			if (first > last)			       /*L000*/
				continue;	/* Doug's hack */      /*L000*/
			/*
			 * note the -1 above
			 * the ending index of the current key
			 * is one less than the index of the next one.
			 */
			show_byte(i, SP);
			for (j = first; j < last; ++j)
				show_byte(p_strbuf[j], SP);
			show_byte(p_strbuf[last], NL);
		}
	}
	putchar('\n');
}

dead()
{
	byte *p, *q, *last;

	p = p_dead;
	while (p < p_comp) {
		printf("dead ");
		show_byte(*p, NL);
		q = p_seq + 2*(*(p+1));
		last = p_seq + 2*(*(p+3) - 1);
		/*
		 * note the -1 above
		 * the ending index of the current key
		 * is one less than the index of the next one.
		 */
		while (q <= last) {
			show_byte(*q, SP);
			show_byte(*(q+1), NL);
			q += 2;
		}
		p += 2;
		putchar('\n');
	}
}

compose()
{
	byte *p, *q, *last;

	if (!comp_key)
		return;
	printf("compose ");
	show_byte(comp_key, NL);
	p = p_comp;
	while (p < p_seq-2) {			/* note the -2 */
		q = p_seq + 2*(*(p+1));
		last = p_seq + 2*(*(p+3) - 1);	/* note the -1 */
		while (q <= last) {
			show_byte(*p, SP);
			show_byte(*q, SP);
			show_byte(*(q+1), NL);
			q += 2;
		}
		p += 2;
	}
	putchar('\n');
}

beep()
{
	if (beepflag)
		printf("beep\n");
}

/*--------------------------------------------------------------begin	*L000*/

/*
 *	Display the contents of a mapctrl buffer in a form that may
 *	be used as input to mapctrl to recreate the map.
 */
static
control(buf)
char *buf;
{
	struct nmseq *seqp;
	register i;
	register byte *cp;
	struct nmtab *tablep = (struct nmtab *)buf;

	if (tablep->n_aseqs == 0) return;

	printf("\nCONTROL\n\ninput\n");
	for (i = 0; i < tablep->n_aseqs; i++) {
		if (i == tablep->n_iseqs) printf("\noutput\n");
		seqp = (struct nmseq *)(buf + tablep->n_seqidx[i]);
		cp = seqp->n_nmseq;
		while (*cp) {
			switch (*cp) {
			case 0:
			case 0200:
			case 033:
			case '\b':
			case '\f':
			case '\n':
			case '\r':
			case '\t':
			case '^':
			case '\\':
			case '#':
				putchar('\\');
				switch (*cp) {
				case 0:
				case 0200: putchar('0'); break;
				case 033: putchar('E'); break;
				case '\b': putchar('b'); break;
				case '\f': putchar('f'); break;
				case '\n': putchar('n'); break;
				case '\r': putchar('r'); break;
				case '\t': putchar('t'); break;
				case '^': putchar('^'); break;
				case '\\': putchar('\\'); break;
				case '#': putchar('#'); break;
				}
				break;
			default:
				if (!isprint(*cp)) {
					if (*cp < 32) printf("^%c", '@' + *cp);
					else printf("\\%o", *cp);
				} else if (isspace(*cp)) printf("\\%c", *cp);
				else putchar(*cp);
			}
			cp++;
		}
		printf("\t%d\n", seqp->n_nmcnt);
	}
}

/*--------------------------------------------------------------end	*L000*/
