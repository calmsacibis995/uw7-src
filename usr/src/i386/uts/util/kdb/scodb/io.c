#ident	"@(#)kern-i386:util/kdb/scodb/io.c	1.1"
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
 *	L000	scol!nadeem & scol!hughd	23apr92
 *	- from scol!nadeem & scol!hughd 9jan92.
 *	- changed getrchr() simply to call getchar() with getchar_doecho
 *	  cleared: allows it to use standard getchar(), and so work on MPX
 *	  even if called on an auxiliary processor (before it would hang
 *	  because only base processor could do the I/O)
 *	- if one processor is doing a getchar() while another's
 *	  doing a getrchr(), then we're in trouble anyway!
 *	L001	scol!nadeem	21may92
 *	- changed C-common definition of getchar_doecho in this module to an
 *	  extern definition.  It is also defined in io/getput.c which causes
 *	  problems with the BTLD linker.  The result is two symbols called
 *	  "getchar_doecho" with two different addresses and double echoing
 *	  as a result.
 *	L002	nadeem@sco.com	13dec94
 *	- mods for user level scodb:
 *	- moved history initialisation code out of init.c into a new routine
 *	  in this module called db_hist_init().
 *	- #ifdef out getrchr() for user level code (defined in stdalone.c).
 *	- moved do_all() from bkp.c into here, because bkp.c is not compiled
 *	  into the user level code, but do_all() is referenced by other
 *	  modules.
 *	- change the prompt from "debug" to "scodb",
 *	- generic change: in anykeydel(), optimise the way that the
 *	  "Hit any key to continue ..." message is removed from the screen.
 *	  This used to be done character by character, but is now done by
 *	  moving to the beginning of the line and issuing a clear to end of
 *	  line escape sequence.
 */

#include	<util/kdb/kdebugger.h>
#include	"dbg.h"
#include	"histedit.h"

#ifdef STDALONE
# undef	putchar
# include		"stdio.h"
# define	FPRINTF(args)		fprintf args
#endif

int scodb_startio_flag = 0;

extern struct scodb_list scodb_history;				/* L002 */
extern uint_t db_lines_left, db_nlines;

#if !defined(STDALONE) && !defined(USER_LEVEL)			/* L002 */
# define	FPRINTF(args)		0

extern void _watchdog_hook(void);

/*
*	like getchar() but doesn't echo
*/
NOTSTATIC
getrchr() {
	int c;

	db_lines_left = db_nlines;
	kdb_output_aborted = B_FALSE;

	scodb_start_io();

	while ((c = DBG_GETCHAR()) == -1)
		_watchdog_hook();

	return(c);
}

#endif /* STDALONE */

/*
*	get an input string
*	do input processing for \b
*/
NOTSTATIC
getistr(ibuf, list)
	register char ibuf[];
	struct scodb_list *list;
{
	register char c;
	int inc, r;

	inc = 0;
	if (list->li_flag & LF_EDITING)
		goto edit;
	for (;;) {
		if (inc >= DBIBFL) {
			printf("\ninput line too long (%d chars max), forcing entry.\n", DBIBFL);
			inc = DBIBFL - 1;
			break;
		}
		c = getrchr();
		if (c == 033) {
			ibuf[inc] = '\0';
		edit:	if ((r = db_edit(ibuf, inc, list)) == ED_CANCEL)
				return 0;
			else
				return r;
		}
		if (c == 0177) {
			putchar('\n');
			return 0;
		}
		if (c == CTRL('D') || c == '\n' || c == '\r') {
			putchar('\n');
			break;
		}
		else if (c == '\b') {
			if (--inc < 0)
				inc = 0;
			else {	/* \b \b will erase characters... */
				backsp();
				putchar(' ');
				backsp();
			}
		}
		else if (c < ' ')
			dobell();
		else {
			ibuf[inc++] = c;
			putchar(c);
		}
	}
	ibuf[inc] = '\0';
	return 1;
}

/*
*	get an input string, with a prompt, allowing for editing
*/
NOTSTATIC
getistre(prompt, bf, empty)
	char *prompt, *bf;
{
	struct scodb_list list;

	list.li_flag	= empty ? 0 : LF_EDITING;
	list.li_mod	= 1;
	list.li_curnum	= 1;
	list.li_minum	= 1;
	list.li_mxnum	= 1;
	strcpy(list.li_prompt, prompt);
	list.li_pp	= list.li_prompt + strlen(list.li_prompt);
	/* don't need any bufps because only 1 line */

	printf(prompt);
	return getistr(bf, &list);
}

/*
*	return number of items in vector
*
*	allows grouping over whitespace using quotation (")
*/
NOTSTATIC
getivec(ac, av, list, bfp, il)
	register int *ac;
	char ***av, *bfp;
	struct scodb_list *list;
	struct ilin *il;
{
	int r;
	char q = 0, pq = 0;
	char *s, *o, **v;
	static char inbuf[DBIBFL];
	extern char *scodb_error;

	if (il)	/* just vectorize bfp, place in il */
		goto ilp;
	if (!bfp)
		bfp = inbuf;
	r = getistr(bfp, list);
	if (r == 0)
		return 0;
	/* don't process input if a return */
	if ((list->li_flag & LF_EDITING) && ((r & ED_OK) | ((r & OP) == OP_RT))) {
		return 0;
	}
	s = bfp;
	il = list->li_buffers[ntob(list, list->li_curnum)];
ilp:	o = il->il_ibuf;
	*av = v = il->il_ivec;
	*ac = 0;
	for (;;) {
		while (white(*s))
			++s;
		if (!*s)
			break;
		pq = *s;
		if (*s == '`')
			q = *s++;
		v[(*ac)++] = o;
		if (pq == '('/*)*/) {
			++s;
			*o++ = pq;
			pq = 1;
		}
		else
			pq = 0;
		for (;;) {
			if (!*s)
				break;
			if (pq) {
				if (*s == '('/*)*/)
					++pq;
				else if (*s == /*(*/')') {
					--pq;
					*o++ = *s++;
					if (pq != 0)
						continue;
				}
			}
			else if (q && *s == q) {
				q = 0;
				++s;
				continue;
			}
			if (*s == '`') {
				q = *s++;
				continue;
			}
			if (white(*s) && !q && !pq)
				break;
			*o++ = *s++;
		}
		if (*s) {
			*o++ = '\0';
			++s;
		}
	}
	*o = '\0';
	if (q || (pq && (q = '('/*)*/))) {
		printf("unmatched %c.\n", q);
		scodb_error = (char *)1;	/* ? */
		return 0;
	}
	v[*ac] = 0;
	il->il_narg = *ac;
	return *ac;
}

/*
*	prompt user for input, and return status:
*		0	user wants to quit
*		1	user wants to continue
*/
NOTSTATIC
anykeydel(s)
	char *s;
{
#define		PR	"Hit any key to continue%s%s or <DEL> to quit: "
#define		SZP	(sizeof(PR) - 1)
#define		XPR	" displaying "
#define		SZXP	(sizeof(XPR) - 1)
#define		SZX	4		/* %s%s + \0 */
	int i, l;
	char *ss;
	unsigned char c;

	if (s) {
		l = SZP - SZX + SZXP + strlen(s);
		ss = XPR;
	}
	else {
		l = SZP - SZX;
		s = ss = "";
	}
	printf(PR, ss, s);
	c = getrchr();
	putchar('\r');						/* L002 */
	clrtoeol();						/* L002 */
	return (!quit(c));	/* 0 - got a quit. 1: no quit */
}

/*
*	check a cursor motion:
*		if a command is active then do the command,
*		otherwise do the cursor motion
*/
#define		checkcm()	{				\
			if (operation) {			\
				cmd = operation;		\
				operation = 0;			\
				++repro;			\
				goto reproc;			\
			}					\
			db_cursor(hcurmo);			\
			loc += hcurmo;				\
		}

#define		SAVE(list, ib, nc, loc)	scodb_save(list, ib, nc, loc)

/*
*	save stuff for later undo.
*/

STATIC
scodb_save(list, ib, nc, loc)
	struct scodb_list *list;
	char *ib;
	int nc, loc;
{
	register struct lsbu *lu = &list->li_lsbuf;

	strcpy(lu->lb_save, ib);
	lu->lb_flag |= LB_SAVED;
	lu->lb_nc = nc;
	lu->lb_hc = loc;
	lu->lb_vc = list->li_curnum;
}

#define		UNDO(list, ib, nc, loc, tbf)			\
	{							\
		switch (undo(list, ib, nc, loc, tbf)) {		\
			case -1:				\
				goto err;			\
			case 0:					\
				cln = list->li_curnum;		\
				retv = ED_EDIT|LS(cln);	\
				goto retn;			\
			case 1:					\
				;				\
		}						\
	}

STATIC
undo(list, ib, nc, loc, tbf)
	struct scodb_list *list;
	char *ib, *tbf;
	int *nc, *loc;
{
	register struct lsbu *lu = &list->li_lsbuf;
	int tx;

	if ((lu->lb_flag & LB_SAVED) == 0)
		return -1;
	if ((lu->lb_vc != list->li_curnum) && ((list->li_flag & LF_EDITING) == 0))
		return -1;
	tx = *nc;
	*nc = lu->lb_nc;
	lu->lb_nc = *nc;
	strcpy(tbf, ib);
	strcpy(ib, lu->lb_save);
	strcpy(lu->lb_save, tbf);
	if (*nc && *loc >= *nc)
		*loc = *nc - 1;
	tx = list->li_curnum;
	list->li_curnum = lu->lb_vc;
	*loc = lu->lb_hc;
	if (tx != list->li_curnum)
		return 0;
	return 1;
}

#define		oper_is(c)	(operation == (c))
#define		CHANGE()	oper_is('c')
#define		DELETE()	oper_is('d')
#define		YANK()		oper_is('y')

/*
*	Edit a line or lines.
*	Commands missing that would be nice to have:
*		s	substitute
*
*	return  	if
*		1	user selects (<CR>)
*		0	user quits (<DEL>)
*/
STATIC
db_edit(ibf, loc, list)
	char ibf[];
	int loc;
	struct scodb_list *list;
{
	int cln;		/* current line #		*/
	int hcurmo;		/* horizontal cursor motion	*/
	int i, j, k;		/* loop vars			*/
	int keepn;		/* set if n should be kept	*/
	int lastl;		/* last line available		*/
	int n;			/* # repetitions for command	*/
	int ni;			/* # of chars from db_insert()	*/
	int numc;		/* # of chars in inbuf		*/
	int editprlen;		/* edit prompt length		*/
	int newe = 0;		/* is new edit (no cmds yet)	*/
	int r, retv;
	int starti;		/* started insert		*/
	register char cmd;	/* current input command	*/
	char operation = 0;	/* operation for hcurmo		*/
	char repro;		/* is a reproc?			*/
	char lf = 0;		/* last find (f,t,F,T)		*/
	char fc = 0;		/* find character used		*/
	char *s;		/* temp ptr for copies		*/
	char *use;		/* buffer to use for new line	*/
	struct ilin *oil = 0;

	char inbuf[DBIBFL];	/* input buffer			*/
	char tbf[DBIBFL];	/* temp buffer (for i,c,R,/)	*/
	char ls = 0;		/* last search (/,?)		*/
	char tbfS[DBIBFL];	/* temp buffer for searches	*/

	if (!ibf)
		cln = 0;
	else
		cln = list->li_curnum;
	lastl = list->li_mxnum;

	hcurmo = 0;
	keepn = 0;
	use = ibf;
	n = 1;
	repro = 0;
	if (list->li_flag & LF_EDITING) {
		switch (list->li_flag & OP) {
			case OP_CO:
				cmd = ':';
				goto colon;

			case OP_SF:
				cmd = '/';
				goto search;

			case OP_SR:
				cmd = '?';
				goto search;
		}
	}
	k = 1;
	goto newl;
	for (;;) {
		k = 0;
		if (!keepn)
			n = 1;
		else
			keepn = 0;
		cmd = getrchr();
		if (numer(cmd) && cmd != '0') {
			n = 0;
			do {
				n = n * 10 + (cmd - '0');
			} while (cmd = getrchr(), numer(cmd));
			k |= 0x01;
		}
/*
*	what we do:
*
*		by default all actions happen once (n == 1)
*		this can be changed by entering a number
*
*		some commands are cursor motions (h, w, etc)
*		but these can also be used along with other
*		commands that are given before them (c, d, etc).
*/
		repro = 0;
	reproc:	switch (cmd) {
			case 033:	/* ESC */
				keepn = 0;
				if (!operation)
					goto err;
				operation = 0;
				break;

		delout:	case 0177:	/* DEL */
				retv = ED_CANCEL;
				goto retn;


			colon:
			case ':':
				if (n != 1 || operation || ((list->li_flag & LF_EDITING) == 0))
					goto err;
				if ((list->li_flag & OP_CO) == 0) {
					/* "got a colon!" */
					retv = ED_EDIT|OP_CO|LS(cln);
					goto retn;
				}
				/* else we're at the right spot */
				goloc(0);
				clrtoeol();
				putchar(':');
				r = db_insert(&ni, tbf, 0, -1);
				if (r < 0)
					goto badi;
				if (ni == 0)
					goto badi;
				for (i = 0;i < ni && white(tbf[i]);i++)
					;
				for (j = i + 1;tbf[j] && white(tbf[j]);j++)
					;
				if (tbf[j] || tbf[i] != 'q') {
			badi:		dobell();
					retv = ED_EDIT|OP_RT|LS(cln);
				}
				else
					retv = ED_OK;
				goto retn;

			case CTRL('D'):		/* accept input line */
		ret:	case '\n':
			case '\r':
				if (operation)
					goto err;
				if (list->li_flag & LF_EDITING) {
					cmd = 'o';
					goto reproc;
				}
				if (ibf)
					strcpy(ibf, inbuf);
				retv = ED_OK;
				goto retn;

			search:
			case '/':		/* search forwards for line containing pat */
			case '?':		/* search backwards for line containing pat */
				if (operation)
					goto err;
				if (!repro) {
					if ((list->li_flag & LF_EDITING) && (list->li_flag & OP_SR) == 0) {
						/* "got a search!" */
						retv = ED_EDIT|LS(cln);
						if (cmd == '/')
							retv |= OP_SF;
						else
							retv |= OP_SR;
						goto retn;
					}
					ls = cmd;
					goloc(0);
					clrtoeol();
					putchar(cmd);
					r = db_insert(&ni, tbfS, 0, -1);
					if (r < 0) {
						if (list->li_flag & LF_EDITING)
							goto bads;
						else
							goto delout;
					}
					if (ni)	/* repeat last search */
						strcpy(tbfS, list->li_lsbuf.lb_search);
				}
				else if (list->li_flag & LF_EDITING)
					strcpy(tbfS, list->li_lsbuf.lb_search);
				if (cmd == '/') {
					for (i = cln + 1;i < lastl;i++) {
						db_cplb(tbf, i, list, 0);
						if (db_substr(tbfS, tbf))
							goto fst;
					}
					for (i = list->li_minum;i <= cln;i++) {
						db_cplb(tbf, i, list, 0);
						if (db_substr(tbfS, tbf))
							goto fst;
					}
				}
				else {
					for (i = cln - 1;i >= list->li_minum;i--) {
						db_cplb(tbf, i, list, 0);
						if (db_substr(tbfS, tbf))
							goto fst;
					}
					for (i = lastl - 1;i >= cln;i--) {
						db_cplb(tbf, i, list, 0);
						if (db_substr(tbfS, tbf))
							goto fst;
					}
				}
			bads:	dobell();
				i = cln;
			fst:	cln = i;
				/*
				*	save search string in lsbuf
				*/
				list->li_lsbuf.lb_flag |= LB_SEARCHED;
				strcpy(list->li_lsbuf.lb_search, tbfS);
				if (list->li_flag & LF_EDITING) {
					retv = ED_EDIT|OP_RT|LS(cln);
					goto retn;
				}
				goto newl;

			case '0':
				/* cursor motion to col 0 */
				hcurmo = -loc;
				checkcm();
				break;

			case '|':
				if (n < 1)
					n = 1;
				if (n > numc)
					n = numc;
/*DANGER*/			hcurmo = (n - 1) - loc;
				checkcm();
				break;

			case '^':
				i = 0;
				while (white(inbuf[i]))
					++i;
				hcurmo = i - loc;
				checkcm();
				break;

			eol:
			case '$':
				hcurmo = numc - loc;
				if ((!operation || CHANGE() || YANK()) && (numc > loc))
					--hcurmo;
				checkcm();
				break;
			
			case ';':
				if (!lf)
					goto err;
				++repro;
				cmd = lf;
				goto reproc;

			case ',':
				if (!lf)
					goto err;
				++repro;
				switch (lf) {
					case 'f': cmd = 'F'; break;
					case 'F': cmd = 'f'; break;
					case 't': cmd = 'T'; break;
					case 'T': cmd = 't'; break;
				}
				goto reproc;

			case 'a':
				SAVE(list, inbuf, numc, loc);
				if (loc != numc) {
					++loc;
					right();
				}
				++repro;
				cmd = 'i';
				goto reproc;

			case 'A':
				SAVE(list, inbuf, numc, loc);
				while (loc < numc) {
					right();
					++loc;
				}
				++repro;
				cmd = 'i';
				goto reproc;

			case 'b':
				k = loc;
				for (i = 0;i < n;i++) {
					j = k;
					if (k)
						--k;
					if (!k)
						break;
					while (k && white(inbuf[k]))
						--k;
					while (k > 0 && !white(inbuf[k-1]))
						--k;
				}
				if (white(inbuf[k]))
					while (k < j)
						++k;
				hcurmo = k - loc;
				checkcm();
				break;

			case 'C':
				SAVE(list, inbuf, numc, loc);
				operation = 'c';
				cmd = '$';
				goto reproc;

			case 'c':		/* change stuff */
				if (numc == 0)
					goto err;
				if (repro) {
					/* change hcurmo chars */
					if (hcurmo < 0) {
						putchar('$');
						backsp();
						db_cursor(hcurmo);
						loc += hcurmo;
						hcurmo = -hcurmo;
					}
					else {
						for (i = 0;i < hcurmo;i++)
							right();
						putchar('$');
						for (i = -1;i < hcurmo;i++)
							backsp();
					}
					j = hcurmo;
					k = db_insert(&ni, tbf, 1, hcurmo);
					if (k < 0)
						goto delout;
					s = inbuf + loc;
					j = ni - (hcurmo + 1);
					if (j < 0) {
						s = inbuf + loc;
						strcpy(s, tbf);
						j = -j;
						s += ni + j;
						while (1) {
							*(s-j) = *s;
							if (!*s)
								break;
							++s;
						}
						numc -= j;
						for (i = 0;i < j;i++)
							delch();
					}
					else {
						s = inbuf + numc + j;
						for (i = numc + j;i > loc;i--)
							inbuf[i] = inbuf[i - j];
						for (i = 0;i < ni;i++)
							inbuf[loc + i] = tbf[i];
						numc += j;
					}
					inbuf[numc] = '\0';
					loc += ni - 1;
					backsp();
					if (!k)
						goto ret;
					break;
				}
				if (CHANGE())
					goto eol;
				else if (operation)
					goto err;
				SAVE(list, inbuf, numc, loc);
				operation = 'c';
				++keepn;
				break;

			case 'D':
				SAVE(list, inbuf, numc, loc);
				operation = 'd';
				cmd = '$';
				goto reproc;

			case 'd':		/* delete stuff */
				if (repro) {
					/* delete hcurmo characters */
					if (hcurmo > 0) {
						n = hcurmo;
						while (n--)
							delch();
						s = inbuf + loc;
						while (1) {
							*s = *(s+hcurmo);
							if (!*s)
								break;
							++s;
						}
					}
					else if (hcurmo < 0) {
						hcurmo = -hcurmo;
						n = hcurmo;
						while (n--) {
							backsp();
							delch();
						}
						s = inbuf + loc;
						while (1) {
							*(s-hcurmo) = *s;
							if (!*s)
								break;
							++s;
						}
					}
					numc -= hcurmo;
					inbuf[numc] = '\0';
					if (numc && loc == numc) {
						--loc;
						backsp();
					}
					break;
				}
				switch (operation) {
					case 0:	/* none */
						break;
					case 'd':	/* delete line */
						retv = ED_EDIT|OP_DL|LS(cln);
						goto retn;
					default:	/* error */
						goto err;
				}
				SAVE(list, inbuf, numc, loc);
				operation = 'd';
				++keepn;
				break;

			case 'e':		/* go to end of word */
				k = loc;
				if (CHANGE() && (k == (numc - 1) || white(inbuf[k+1])))
					--n;
				for (i = 0;i < n;i++) {
					if (k == numc - 1)
						break;
					if (!white(inbuf[k]) && white(inbuf[k+1]))
						++k;
					while ((k != (numc -1)) && white(inbuf[k]))
						++k;
					while ((k != (numc - 1)) && !white(inbuf[k]))
						++k;
				}
				if (white(inbuf[k]) && (k-1 > 0) && !white(inbuf[k-1]))
					--k;
				hcurmo = k - loc;
				if (DELETE())
					++hcurmo;
				checkcm();
				break;

			case 't':
			case 'f':
				if (!repro) {
					fc = db_getechr();
					lf = cmd;
				}
				if (!fc)
					goto err;
				j = loc;
				k = j;
				for (i = 0;i < n;i++) {
					for (++j;inbuf[j] && inbuf[j] !=fc;j++)
						;
					if (!inbuf[j])
						break;
					k = j;
				}
				if (i != n)
					goto err;
				hcurmo = k - loc;
				if (cmd == 't')
					--hcurmo;
				if (DELETE())
					++hcurmo;
				checkcm();
				break;

			case 'T':
			case 'F':
				if (!repro) {
					fc = db_getechr();
					lf = cmd;
				}
				if (!fc)
					goto err;
				j = loc;
				k = j;
				for (i = 0;i < n;i++) {
					for (--j;j >= 0 && inbuf[j] != fc;j--)
						;
					if (j < 0)
						break;
					k = j;
				}
				if (i != n)
					goto err;
				hcurmo = k - loc;
				if (cmd == 'T')
					++hcurmo;
				checkcm();
				break;

			case 'G':
				if (operation)
					goto err;
				if (n == 1 && ((k & 0x01) == 0))
					n = list->li_mxnum - 1;
				i = lastl;
				if ((list->li_flag & LF_EDITING) && i != list->li_minum)
					--i;
				if (n > i || n < list->li_minum)
					goto err;
				if (cln == list->li_curnum)
					strcpy(ibf, inbuf);
				cln = n;
				goto newl;
			
			case 'H':
				if (operation)
					goto err;
				if (cln == list->li_curnum)
					strcpy(ibf, inbuf);
				cln = list->li_minum - 1 + n;
				i = lastl;
				if ((list->li_flag & LF_EDITING) && i != list->li_minum)
					--i;
				if (cln > i && cln != list->li_minum)
					--cln;
				goto newl;

			case CTRL('H'):		/* go left one space */
			case 'h':
				if (loc == 0)
					goto err;
				if (n > loc)
					n = loc;
				hcurmo = -n;
				if (CHANGE())
					--hcurmo;
				checkcm();
				break;

			case 'I':		/* insert at beginning of stuff */
				i = 0;
				while (white(inbuf[i]))
					++i;
				if (!inbuf[i])
					--i;
				j = loc;
				loc = i;
				for (;i < j;i++)
					backsp();
				cmd = 'i';
				goto reproc;

			case 'i':		/* insert */
				if (!repro)
					SAVE(list, inbuf, numc, loc);
				r = db_insert(&ni, tbf, 1, -1);
				if (r < 0)
					goto delout;
				i = ni * n;
				numc += i;
				s = inbuf + numc + i;
				while (s > (inbuf + loc)) {
					*s = *(s - i);
					--s;
				}
				s = inbuf + loc;
				starti = startinsert();
				for (i = 0;i < n;i++)
					for (j = 0;j < ni;j++) {
						if (i) {
							if (!starti)
								insch();
							putchar(tbf[j]);
						}
						*s++ = tbf[j];
					}
				endinsert();
				loc += ni * n;
				if (numc && loc == numc) {
					--loc;
					backsp();
				}
				inbuf[numc] = '\0';
				if (!r)
					goto ret;
				break;

			case 'j':		/* go down */
				if (operation)
					goto err;
				if (list->li_flag & LF_EDITING) {
					if (n != 1)
						goto err;
					/* k is set if from \n */
					if ((cln + n) >= (lastl+((k & 0x02) == 0x02)))
						goto err;
					cln += n;
				}
				else {
					if ((cln + n) > lastl)	/* ? >= */
						goto err;
					cln += n;
				}
				goto newl;

			case 'k':		/* go up */
				if (operation)
					goto err;
				if ((cln - n) < list->li_minum)
					goto err;
				if (cln == list->li_curnum)
					strcpy(ibf, inbuf);
				cln -= n;
			newl:
				if (cln == list->li_curnum)
					use = ibf;
				else
					use = 0;
				j = 0;
				if (list->li_flag & LF_EDITING) {
					if (list->li_flag & OP_AL) {
						/* first time through */
						list->li_flag &= ~OP_AL;
						++j;
					}
					else if (list->li_flag & OP_GL) {
						list->li_flag &= ~OP_GL;
					}
					else {
						retv = ED_EDIT|LS(cln);
						goto retn;
					}
				}
				if (newe++ || j)
					loc = 0;
				goloc(0);
				clrtoeol();
				if (use) {
					strcpy(inbuf, use);
					numc = strlen(inbuf);
				}
				else
					numc = db_cplb(inbuf, cln, list, 0);
				printf("[%d] %s", cln, inbuf);
				editprlen = db_ndi(cln, 10) + 3;
				if (loc >= numc && numc)
					loc = numc - 1;
				goloc(editprlen + loc);
				if (j) {
					cmd = 'a';
					goto reproc;
				}
				break;
			
			case 'L':
				if (operation)
					goto err;
				if (cln == list->li_curnum)
					strcpy(ibf, inbuf);
				i = lastl;
				if ((list->li_flag & LF_EDITING) && i != list->li_minum)
					--i;
				cln = i + 1 - n;
				if (cln < list->li_minum)
					cln = list->li_minum;
				goto newl;

			case CTRL('L'):		/* go right */
			case ' ':
			case 'l':
 				if (!CHANGE() && !DELETE() && loc >= (numc - 1))
					goto err;
				if (numc && (loc + n) > (numc - 1))
					n = (numc - 1) - loc;
				hcurmo = n;
 				if (CHANGE() && hcurmo)
					--hcurmo;
 				else if (DELETE() && !hcurmo)
 					hcurmo = 1;
				checkcm();
				break;
			
			case 'M':
				if (operation)
					goto err;
				if (cln == list->li_curnum)
					strcpy(ibf, inbuf);
				i = lastl;
				if ((list->li_flag & LF_EDITING) && i)
					--i;
				cln = (i + list->li_minum) / 2;
				goto newl;

			case 'n':
			case 'N':
				if (operation)
					goto err;
				if (list->li_flag & LF_EDITING) {
					if ((list->li_lsbuf.lb_flag & LB_SEARCHED) == 0)
						goto err;
				}
				else if (!ls)
					goto err;
				++repro;
				if (cmd == 'n')
					cmd = ls;
				else
					cmd = (ls == '/') ? '?' : '/';
				goto reproc;
			
			case 'O':
			case 'o':
				if (n != 1 || operation || ((list->li_flag & LF_EDITING) == 0))
					goto err;
if (
	(list->li_flag & LF_WRAPS) == 0
		&&
	(list->li_mxnum - list->li_minum + 1) == list->li_mod
   )
	goto err;
				n = cln + (cmd == 'o');
				retv = ED_EDIT|OP_AL|LS(n);
				goto retn;

			case 'r':
				if (operation)
					goto err;
				if ((loc + n) > numc)
					goto err;
				SAVE(list, inbuf, numc, loc);
				cmd = db_getechr();
				for (i = 0;i < n;i++) {
					putchar(cmd);
					inbuf[loc++] = cmd;
				}
				--loc;
				backsp();
				break;
			
			case 'R':
				if (operation)
					goto err;
				SAVE(list, inbuf, numc, loc);
				r = db_insert(&ni, tbf, 0, 0);
				if (r < 0)
					goto delout;
				for (i = 0;i < ni;i++)
					inbuf[loc++] = tbf[i];
				--loc;
				if (n == 1)
					backsp();
				i = ni * (n - 1);
				numc += i;
				s = inbuf + numc;
				while (s > (inbuf + loc)) {
					*s = *(s - i);
					--s;
				}
				if (n > 1) {
					starti = startinsert();
					for (i = 1;i < n;i++)
						for (j = 0;j < ni;j++) {
							inbuf[++loc] = tbf[j];
							if (!starti)
								insch();
							putchar(tbf[j]);
						}
					endinsert();
					backsp();
				}
				if (loc >= numc && numc)
					numc = loc + 1;
				inbuf[numc] = '\0';
				if (!r)
					goto ret;
				break;

			case CTRL('R'):
				if (operation)
					goto err;
				goloc(editprlen);
				clrtoeol();
				printf(inbuf);
				goloc(loc + editprlen);
				break;

			case 'S':
				if (numc == 0)
					goto err;
				if (operation)
					goto err;
				SAVE(list, inbuf, numc, loc);
				numc = 0;
				loc = 0;
				goloc(editprlen);
				clrtoeol();
				n = 1;
				cmd = 'i';
				++repro;	/* don't SAVE(list, inbuf, numc, loc) again */
				goto reproc;

			case 's':
				if (operation)
					goto err;
				SAVE(list, inbuf, numc, loc);
				operation = 'c';
				cmd = 'l';
				goto reproc;

			case 'u':
				if (operation)
					goto err;
				UNDO(list, inbuf, &numc, &loc, tbf);
				cmd = CTRL('R');
				goto reproc;

			case 'w':		/* go to next word */
				if (CHANGE() || YANK()) {	/* use e */
					cmd = 'e';
					goto reproc;
				}
				k = loc;
				for (i = 0;i < n;i++) {
					if (k == numc - 1)
						break;
					while ((k != (numc -1)) && !white(inbuf[k]))
						++k;
					while ((k != (numc - 1)) && white(inbuf[k]))
						++k;
				}
				hcurmo = k - loc;
				checkcm();
				break;

			case 'X':
			case 'x':
				if (operation)
					goto err;
				SAVE(list, inbuf, numc, loc);
				operation = 'd';
 				cmd = (cmd == 'x') ? 'l' : 'h';
				goto reproc;

		err:	default:
				dobell();
				break;
		}
	}
retn:	;
	if ((retv & ED_EDIT) == 0)
		putchar('\n');
	else {
		strcpy(ibf, inbuf);
	}
	if (oil) {	/* free it up */
		oil->il_narg = 0;
	}
	list->li_rflag = retv;
	return retv;
}

/*
*	get an edit character.
*	^V will `escape' a character.
*/
STATIC
db_getechr() {
	char cmd;

	cmd = getrchr();
	switch (cmd) {
		case CTRL('V'):
			cmd = db_getechr();
			break;
		case CTRL('M'):
			cmd = '\n';
			break;
		case '\t':
			cmd = ' ';
			break;
	}
	return cmd;
}

/*
*	handle insertion, place in `tbf'
*	`ni' holds number of characters inserted
*	if `isin' then characters are inserted, not replaced.
*	any number of characters after `after' are inserted.
*/
STATIC
db_insert(ni, tbf, isin, after)
	register int *ni;
	register char tbf[];
	int isin, after;
{
	char cmd;
	int j;
	int starti;

	j = 0;
	for (;;) {
		switch (cmd = db_getechr()) {
			case 0177:
				return -1;

			case 033:
				tbf[j] = '\0';
				*ni = j;
				return 1;

			case '\n':
				tbf[j] = '\0';
				*ni = j;
				return 0;

			case '\b':
				if (j == 0) {
					dobell();
					break;
				}
				--j;
				backsp();
				if (isin) {
					if (j == after) {
						putchar('$');
						backsp();
					}
					else if (j > after)
						delch();
				}
				break;

			default:
				if (cmd < ' ') {
					dobell();
					break;
				}
				tbf[j++] = cmd;
				starti = 0;
				if (isin && j > (after+1)) {
					if ((starti = startinsert()) == 0)
						insch();
				}
				putchar(cmd);
				if (starti)
					endinsert();
				break;
		}
	}
}

/*
*	number of digits, in `base', in val.
*/
NOTSTATIC
db_ndi(val, base)
	register val, base;
{
	register int nd = 0;

	if (val == 0)
		return 1;
	while (val) {
		++nd;
		val /= base;
	}
	return nd;
}

STATIC
db_substr(sub, instr)
	register char *sub, *instr;
{
	int l = strlen(sub) - 1;

	while (*instr) {
		if (*sub == *instr && (!l || !strncmp(sub + 1, instr + 1, l)))
			return 1;
		++instr;
	}
	return 0;
}

NOTSTATIC
db_cplb(bf, ln, list, ilb)
	register char *bf;
	int ln;
	struct scodb_list *list;
	struct ilin *ilb;
{
	int i;
	register char *is;
	char *bbf = bf;
	register struct ilin *il;

	if (!list)
		il = ilb;
	else
		il = list->li_buffers[ntob(list, ln)];
	if (il) {
		for (i = 0;i < il->il_narg;i++) {
			if (i)
				*bf++ = ' ';
			is = il->il_ivec[i];
			if (!is) {
				strcpy(bf, "{history vector corrupt}");
				return 0;
			}
			while (*is)
				*bf++ = *is++;
		}
	}
	*bf = '\0';
	return bf - bbf;
}

STATIC
db_cursor(n)
	register int n;
{
	if (n > 0) {
		while (n--)
			right();
	}
	else {
		while (n++)
			backsp();
	}
}

/*
*	print lines bl through el.
*	if bl > el, just go up...
*/
STATIC
plines(list, bl, el, bf)
	struct scodb_list *list;
	int bl, el;
	char bf[];
{
	int i;

	putchar('\r');
	if (bl > el) {
		for (i = bl;i > el;i--)
			up();
	}
	else {
		for (i = bl;i < el;i++) {
			db_cplb(bf, i, list, 0);
			printf("[%d] %s\n", i, bf);
		}
	}
}

/*
*	go up n lines
*/
STATIC
goup(n)
	int n;
{
	int i;

	for (i = 0;i < n;i++)
		up();
}

NOTSTATIC
list_edit(list, balloc, bfree)
	struct scodb_list *list;
	struct ilin *(*balloc)();
	int (*bfree)();
{
	int i, lnr, onl, r = 0, f = 0;
	int c;
	char **v;
	struct ilin *il;
/*-S-*/	char tbf[DBIBFL];
	struct ilin *ilb[128];	/* way more than ... */

	for (i = 0;i < NMEL(ilb);i++)
		ilb[i] = NULL;
	il = list->li_bufl;
	for (i = 0;il;i++) {
		ilb[i + list->li_minum] = il;
		il = il->il_next;
	}
	list->li_buffers = ilb;
	list->li_mxnum = list->li_minum + i;

	lnr = onl = list->li_minum;
	plines(list, onl, list->li_mxnum, tbf);
	goup(list->li_mxnum - onl);
	list->li_flag |= LF_EDITING;
	r = ED_EDIT|LS(onl);
	while (r & ED_EDIT) {
		lnr = LN(r);
		if ((r & OP) == 0)	/* no op yet - no buffer? add else go line */
			r |= list->li_buffers[lnr] ? OP_GL : OP_AL;
		switch (r & OP) {
			case OP_DL:
				/* delete line lnr: we're on lnr */
				if (lnr == list->li_minum && lnr == (list->li_mxnum - 1)) {
					/*
					*	trying to delete the only line:
					*	bitch a little,
					*	"free" the buffer, but don't give it up
					*	so that we'll reuse it.
					*/
					dobell();
					list->li_buffers[lnr]->il_narg = 0;
					r = ED_EDIT|LS(lnr);
					break;
				}
				/* no save if was on deleted line */
				if ((list->li_lsbuf.lb_flag & LB_SAVED) &&
						list->li_lsbuf.lb_vc == lnr)
					list->li_lsbuf.lb_flag &= ~LB_SAVED;
				(*bfree)(list->li_buffers[lnr]);
				for (i = lnr;i < list->li_mxnum;i++)
					list->li_buffers[i] = list->li_buffers[i + 1];
				--list->li_mxnum;
				if (list->li_mxnum)
#ifdef NOTDEF
 				if (list->li_mxnum != list->li_minum)
#endif
					delline();
				if (lnr == list->li_mxnum) {
					--lnr;
					--onl;
					up();
				}
				plines(list, onl, list->li_mxnum, tbf);
				goup(list->li_mxnum - onl);
				r = ED_EDIT|LS(onl);	/* go to the new line. */
				f = 0;
				break;

			case OP_AL:
				/* open a line above lnr: we're on lnr */
				if ((il = (*balloc)()) == NULL) {
					for (i = onl;i < list->li_mxnum;i++)
						down();
					printf("\rNo more line buffers [hit any key to continue]");
					getrchr();
					/* no lines, no buffers - just return */
					if (list->li_minum == list->li_mxnum) {
						putchar('\n');
						r = ED_OK;
					}
					else {
						--lnr;
						f = 1;
						r = ED_EDIT|OP_RT|LS(lnr);
					}
					break;
				}
				insline();
				for (i = list->li_mxnum;i >= lnr;i--)
					list->li_buffers[i + 1] = list->li_buffers[i];
				++list->li_mxnum;
				list->li_buffers[lnr] = il;
				plines(list, onl, list->li_mxnum, tbf);
				goup(list->li_mxnum - onl);
				r = ED_EDIT|LS(lnr);
				f = 0;
				break;

			case OP_CO:		/* colon		*/
			case OP_SF:		/* search forward	*/
			case OP_SR:		/* search backwards	*/
				for (i = onl;i < list->li_mxnum;i++)
					down();
				list->li_flag |= (r & OP);
				f = 1;
				goto iv;

			case OP_RT:
				if (f) {
					onl = lnr;
					delline();
					goup(list->li_mxnum - onl);
				}
				r = ED_EDIT|LS(lnr);
				f = 0;
				break;

			case OP_GL:
				/* we're on onl */
				f = 0;
				plines(list, onl, lnr, tbf);
				onl = lnr;
				db_cplb(tbf, lnr, list, 0);
				list->li_curnum = lnr;
				list->li_flag &= LF;
				if (list->li_buffers[onl]->il_narg)
					list->li_flag |= OP_GL;
				else
					list->li_flag |= OP_AL;
			iv:	getivec(&c, &v, list, tbf, 0);
#define		ZOK	(ED_OK|OP_CO|OP_SR)		/* no input ok */
				/* if no input and we needed input, delete the line */
				if ((c == 0) && ((list->li_rflag & ZOK) == 0))
					r = ED_EDIT|OP_DL|LS(onl);
				else
					r = list->li_rflag;
				break;

		}
	}
	if (list->li_buffers[lnr]->il_narg == 0) {
		/*
		*	if nothing on that line,
		*	delete it.
		*/
		(*bfree)(list->li_buffers[lnr]);
		for (i = lnr;i < list->li_mxnum;i++)
			list->li_buffers[i] = list->li_buffers[i + 1];
		--list->li_mxnum;
	}

	/*
	*	rebuild list.li_bufl
	*/
	il = NULL;
	for (i = list->li_mxnum - 1;i >= list->li_minum;i--) {
		list->li_buffers[i]->il_next = il;
		il = list->li_buffers[i];
	}
	list->li_bufl = il;

	return r;
}

db_hist_init()							/* L002v */
{
	if (scodb_history.li_mod == 0) {	/* not setup yet */
		int i;
		extern struct ilin *scodb_ibufp[];
		extern struct ilin scodb_ibufs[];
		extern int scodb_maxhist;

		scodb_history.li_minum = 1;	/* start at line 1 */
		scodb_history.li_mxnum = 1;
		scodb_history.li_mod = scodb_maxhist;
#ifdef USER_LEVEL
		strcpy(scodb_history.li_prompt, "scodb");
#else
		strcpy(scodb_history.li_prompt, "debug");
#endif
		scodb_history.li_buffers = scodb_ibufp;
		for (i = 0;i < scodb_maxhist;i++)
			scodb_ibufp[i] = &scodb_ibufs[i];
	}
}

NOTSTATIC
do_all(md, s)
	int md;
	char *s;
{
	int ret = -1;
	char ci;

	printf(s);
	clrtoeol();
	while (ret == -1) {
		ci = getrchr();
		switch (ci) {
			case 'y':
			case 'Y':
				printf("Yes");
				ret = DM_YES;
				break;

			case 'j':
			case 'n':
			case 'N':
				printf("No");
				ret = DM_NO;
				break;
				
			case '\b':
			case 'k':
			case 'l':
			case '-':
				if (md & DM_LAST) {
					printf("Last");
					ret = DM_LAST;
				}
				else
					goto derr;
				break;

			case 'q':
			case 'Q':
			case '\033':
			case '\177':
				if (md & DM_QUIT) {
					printf("Quit");
					ret = DM_QUIT;
				}
				else
					goto derr;
				break;

			derr:
			default:
				dobell();
		}
	}
	if ((md & DM_NONL) == 0)
		putchar('\n');
	return ret;
}								/* L002^ */

/*
 * Called when entering scodb to ensure that the console is ready for
 * use whilst in the debugger.
 */

scodb_start_io()
{
	if (!scodb_startio_flag) {
		kdb_start_io();
		scodb_startio_flag = 1;
	}
}

/*
 * Called when exiting scodb to return the console to general system use.
 */

scodb_end_io()
{
	if (scodb_startio_flag) {
		kdb_end_io();
		scodb_startio_flag = 0;
	}
}
