#ident	"@(#)kern-i386:util/kdb/scodb/dcl.c	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

#include	<syms.h>
#include	"stunv.h"
#include	"dbg.h"
#include	"dcl.h"
#include	"sent.h"
#include	"val.h"

#define		LPR		'('	/*)*/
#define		RPR	/*(*/	')'

#define		whites(s)	while (white(*(s))) ++(s)
#define		nwhites(s)	while (*(s) && !white(*(s))) ++(s)

#ifndef STDALONE
db_dcl_init() {
	register int i;
	extern int scodb_ndcvari;
	extern struct cvari scodb_dcvari[];

	for (i = 0;i < scodb_ndcvari;i++)
		_dcl_init(i);
}

static
_dcl_init(n) {
	extern struct cvari scodb_dcvari[];
	extern char scodb_dcvarinames[];

	*(scodb_dcvari[n].cv_names = scodb_dcvarinames + n * NAMEL) = '\0';
}

NOTSTATIC
c_dcl(c, v)
	int c;
	char **v;
{
	int i, n;
	char *s;
	struct value va;
	struct cvari *cvp;
	extern int scodb_ndcvari, nvari;
	extern struct cvari scodb_dcvari[], *cvari;
	static char *p_havedcl	= "Have declaration (%s) already - ";
	static char *p_ovwr	= "overwrite [yn]? ";

	cvp = &va.va_cvari;
	cvp->cv_names = va.va_name;
	if (c == 1) {
		n = 0;
		for (i = 0;i < scodb_ndcvari;i++) {
			if (scodb_dcvari[i].cv_names[0] == 0)
				continue;
			if (n && n % NLPP == 0 && !anykeydel("debugger commands"))
				break;
			printf("   %s\n", scodb_dcvari[i].cv_names);
			++n;
		}
		return DB_CONTINUE;
	}
	switch (dclv(++v, cvp)) {
		case D_BADINP:
		case D_NOCSARE:
		case D_NORPR:
		case D_SUNKNOWN:
		case D_TUNKNOWN:
			printf("Bad declaration.\n");
			return DB_ERROR;

		case D_OK:
			break;
	}
	/* check loaded ones */
	for (i = 0;i < nvari;i++)
		if (!strcmp(cvp->cv_names, cvari[i].cv_names)) {
			printf(p_havedcl, cvp->cv_names);
			if (do_all(0, p_ovwr)) {
				s = cvari[i].cv_names;
				cvari[i] = *cvp;	/* BAD */
				cvari[i].cv_names = s;
				strcpy(s, cvp->cv_names);
			}
			return DB_CONTINUE;
		}
	/* check temps */
	for (i = 0;i < scodb_ndcvari;i++)
		if (!strcmp(cvp->cv_names, scodb_dcvari[i].cv_names)) {
			printf(p_havedcl, cvp->cv_names);
			if (do_all(0, p_ovwr)) {
				s = scodb_dcvari[i].cv_names;
				scodb_dcvari[i] = *cvp;
				scodb_dcvari[i].cv_names = s;
				strcpy(s, cvp->cv_names);
			}
			return DB_CONTINUE;
		}
	/* find free slot */
	for (i = 0;i < scodb_ndcvari;i++)
		if (scodb_dcvari[i].cv_names[0] == '\0')
			break;
	if (i == scodb_ndcvari) {
		printf("No free declaration slots (%d total)\n", scodb_ndcvari);
		return DB_ERROR;
	}
	s = scodb_dcvari[i].cv_names;
	scodb_dcvari[i] = *cvp;
	scodb_dcvari[i].cv_names = s;
	strcpy(s, cvp->cv_names);
	return DB_CONTINUE;
}

NOTSTATIC
c_undcl(c, v)
	int c;
	char **v;
{
	int i, er = 0;
	extern int scodb_ndcvari;
	extern struct cvari scodb_dcvari[];

	while (*++v) {
		if (**v == '*' && !(*v)[1]) {
			if (do_all(0, "Clear all declarations? ")) {
				db_dcl_init();
				break;
			}
			else
				continue;
		}
		for (i = 0;i < scodb_ndcvari;i++)
			if (!strcmp(*v, scodb_dcvari[i].cv_names)) {
				_dcl_init(i);
				break;
			}
		if (i == scodb_ndcvari) {
			printf("Temporary declaration %s not found\n", *v);
			++er;
		}
	}
	return er ? DB_ERROR : DB_CONTINUE;
}
#endif	/* !STDALONE */

NOTSTATIC
dclv(v, cv)
	char **v;
	struct cvari *cv;
{
	char buf[256];
	register char *s = buf;

	/* concat all args */
	while (*v) {
		strcpy(s, *v);
		while (*s) s++;
		*s++ = ' ';
		++v;
	}
	*s = '\0';
	return dcl(buf, cv);
}

/*
*	take an input line, and find the appropriate C
*	declaration.
*/
NOTSTATIC
dcl(inp, cv)
	char *inp;
	struct cvari *cv;
{
	int dn = 0;

	cv->cv_type = 0;
	return _dcl(inp, cv, &dn);
}

/*
*	given input of a C decl, find the symbol name
*	and the type of the symbol.
*	Modifies input string
*
*	input:	*type should == 0
*		first == 1
*/
STATIC
_dcl(inp, cv, dn)
	char *inp;
	struct cvari *cv;
	int *dn;
{
	char *s, *syme;
	int i, t_basic, t_derived, p, r;
	char tbf[16];
	extern struct btype btype[];
	struct cstun *sstun, *findstun();
	extern struct cstun *stun;

	if (cv->cv_type == 0) {
		cv->cv_names[0] = '\0';
		whites(inp);
		if (!*inp)
			return D_BADINP;
		for (i = 0;i < sizeof(tbf) && alnum(inp[i]);i++)
			tbf[i] = inp[i];
		if (i == sizeof(tbf))
			return D_BADINP;
		tbf[i] = '\0';
		inp += i;
	}
	t_basic = cv->cv_type & N_BTMASK;
	t_derived = cv->cv_type >> N_BTSHFT;
	if (t_basic == 0) {
		for (t_basic = 0;btype[t_basic].bt_nm;t_basic++)
			if (!strcmp(btype[t_basic].bt_nm, tbf))
				break;
		if (btype[t_basic].bt_nm == 0)
			return D_TUNKNOWN;
#ifndef STDALONE
		if (t_basic == T_STRUCT || t_basic == T_UNION) {
			whites(inp);
			if (!*inp)
				return D_BADINP;
			for (i = 0;i < sizeof(tbf) && alnum(inp[i]);i++)
				tbf[i] = inp[i];
			if (i == sizeof(tbf))
				return D_BADINP;
			tbf[i] = '\0';
			inp += i;
			sstun = findstun(tbf);
			if (!sstun)
				return D_SUNKNOWN;
			cv->cv_index = sstun - stun;
		}
#endif
	}
	for (;;) {
		whites(inp);
		if (!*inp)
			break;
		if (*inp == '*') {
			++inp;
			t_derived <<= 2;
			t_derived |= DT_PTR;
		}
		else if (*inp == CSARB) {
			i = 0;
			++inp;
			whites(inp);
			while (hexdi(*inp)) {
				i *= 16;
				i += *inp - (numer(*inp) ? '0' : ((hexcl(*inp) ? 'a' : 'A') - 10));
				++inp;
			}
			whites(inp);
			if (*inp++ != CSARE)
				return D_NOCSARE;
			cv->cv_dim[(*dn)++] = i;
			t_derived <<= 2;
			t_derived |= DT_ARY;
		}
		else if (alpha(*inp)) {
			if (cv->cv_names[0])
				return D_BADINP;
			syme = cv->cv_names;
			while (alnum(*syme = *inp))
				++syme, ++inp;
			*syme = '\0';
		}
		else if (*inp == LPR) {
			++inp;
			whites(inp);
			if (*inp == RPR) {
				++inp;
				t_derived <<= 2;
				t_derived |= DT_FCN;
			}
			else {
				p = 1;
				s = inp;
				for (;;) {
					whites(s);
					if (!*s)
						return D_BADINP;
					if (*s == LPR)
						++p;
					if (*s == RPR)
						--p;
					if (!p)
						break;
					++s;
				}
				if (p)
					return D_NORPR;
				*s = '\0';
				cv->cv_type = t_basic | (t_derived << N_BTSHFT);
				r = _dcl(s + 1, cv, dn);
				if (r)
					return r;
				t_basic = cv->cv_type & N_BTMASK;
				t_derived = cv->cv_type >> N_BTSHFT;
			}
		}
		else
			return D_BADINP;
	}
	cv->cv_type = t_basic | (t_derived << N_BTSHFT);
	return 0;
}

#ifdef STDALONE

char *inps[] = {
	"int a",
	"int b[2]",
	"int *c[3]",
	"int d[2][3]",
	"int *e[2][3]",
	"int (*f)()",
	"int (*g)[4]",
	"int (*h[5])[6]",
	"int *",
	"int *[4]",
	"int (*)[6]",
	"int (*[3])[2]",
	"int (*)()",
	0
};

main() {
	char **v;
	struct cvari cvari;
	char cvarinamebuf[NAMEL];
	int r, i = 0;

	cvari.cv_names = cvarinamebuf;
	for (v = inps;*v;v++) {
		r = dcl(*v, &cvari);
		if (!cvari.cv_names[0])
			sprintf(cvari.cv_names, "v%d", i++);
		if (!r) {
			pcvari(0, &cvari);
			putchar('\n');
		}
		else
			printf("error: %d\n", r);
	}
}

int stunlist, varilist;

#endif
