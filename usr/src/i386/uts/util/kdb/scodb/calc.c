#ident	"@(#)kern-i386:util/kdb/scodb/calc.c	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 * Modification History:
 *
 *	L000	scol!nadeem	31jul92
 *	- added support for static text symbols.  When the user enters an
 *	  expression at the debug prompt, and this evaluates to a static
 *	  symbol, display the result in global symbol terms as well.
 *	  This is useful in determining exactly which routine an
 *	  assembler static symbol appears in.
 */

/*
#include	"sys/types.h"
#include	"sys/trap.h"
#include	"sys/tss.h"
#include	"sys/conf.h"
#include	"sys/param.h"
*/
#include	"sys/reg.h"
#include	"sys/seg.h"
#include	"dbg.h"
#include	"histedit.h"
#include	"stunv.h"
#include	"val.h"
#include	"dis.h"
#include	"sent.h"

/*
*	special command: take all arguments, treat as an expression
*/
NOTSTATIC
c_calc(c, v)
	int c;
	register char **v;
{
	int r;
	long val;
	char *symname(), *s;
	char buf[33];
	struct value va;
	struct sent *se, *findsym();
	extern int cclen, ccobas, ccnewl, *REGP;
	extern char ccpres[], ccposs[];

	va.va_seg = REGP[T_DS];
	if (!valuev(v, &va, 1, 1)) {
		perr();
		r = DB_ERROR;
	}
	else {
		val = va.va_value;
		printf("%s", ccpres);
		switch (cclen) {
			case 0:
				goto doret;
			case MD_BYTE:
				val &= 0x0FF;
				break;
			case MD_SHORT:
				val &= 0x0FFFF;
				break;
		}
		if (ccobas == 8)
			printf("%o", val);
		else if (ccobas == 10)
			printf("%d", val);
		else if (ccobas == 2) {
			/* base 2 */
			s = buf + sizeof buf - 1;
			*--s = '\0';
			while (val) {
				*--s = (val & 1) ? '1' : '0';
				val >>= 1;
			}
			printf(s);
		}
		else {
			pn(buf, val);
			printf(buf);
		}
		if (ccposs[0])
			printf("%s", ccposs);
		else if (s = symname(val, 1)) {
			se = findsym(val);
			if (se->se_flags & SF_STATIC) {		/* L000 v */
				printf("\t%s", s);
				s = symname(val, 1 | SYM_GLOBAL);
				printf(" (%s)\t%s", s, se->se_flags & SF_TEXT ? "text" : "data");
			} else					/* L000 ^ */
				printf("\t%s\t%s", s, se->se_flags & SF_TEXT ? "text" : "data");
		}
		if (ccnewl)
			putchar('\n');
doret:		r = DB_CONTINUE;
	}
	return r;
}
