#ident	"@(#)m4:common/m4macs.c	1.18"

#include	<limits.h>
#include	<stdio.h>
#include	<sys/param.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	"m4.h"

#define arg(n)	(c<(n)? nullstr: ap[n])

dochcom(ap,c)
WCHAR_T	**ap;
{
	register WCHAR_T	*l = arg(1);
	register WCHAR_T	*r = arg(2);

	if (wslen(l)>MAXSYM || wslen(r)>MAXSYM)
		error2(gettxt(":127","comment marker longer than %d chars"),MAXSYM);
	wscpy(lcom,l);
	if (*r)	
		wscpy(rcom, r);
	else 
		wscpy(rcom, l_type("\n"));
}

docq(ap,c)
register WCHAR_T 	**ap;
{
	register WCHAR_T	*l = arg(1);
	register WCHAR_T	*r = arg(2);

	if (wslen(l)>MAXSYM || wslen(r)>MAXSYM)
		error2(gettxt(":128","quote marker longer than %d chars"), MAXSYM);

	if (c<=1 && !*l) {
		l = l_type("`");
		r = l_type("'");
	} else if (c==1) {
		r = l;
	}

	wscpy(lquote,l);
	wscpy(rquote,r);
}

dodecr(ap,c)
WCHAR_T 	**ap;
{
#ifdef NATIVE
	pbnum(wstol(arg(1))-1);
#else
	pbnum(ctol(arg(1))-1);
#endif
}

dodef(ap,c)
WCHAR_T	**ap;
{
	def(ap,c,NOPUSH);
}

def(ap,c,mode)
register WCHAR_T 	**ap;
{
	register WCHAR_T	*s;

	if (c<1)
		return;

	s = ap[1];

#ifdef	NATIVE
	if(iswalpha(*s) || ((unsigned int)*s > 0x7f && iswgraph(*s)))
	     while(iswalnum(*s) || ((unsigned int)*s > 0x7f && iswgraph(*s)))
		    ++s;
#else
	if (isalpha(*s))
		while (isalnum(*++s))
			;
#endif

	if (*s || s==ap[1])
		error(gettxt(":129","bad macro name"));

	if (wscmp(ap[1],ap[2])==0)
		error(gettxt(":130","macro defined as itself"));
	install(ap[1],arg(2),mode);
}

dodefn(ap,c)
register WCHAR_T	**ap;
register c;
{
	register WCHAR_T *d;

	while (c > 0)
		if ((d = lookup(ap[c--])->def) != NULL) {
			putbak(*rquote);
			while (*d)
				putbak(*d++);
			putbak(*lquote);
		}
}

dodiv(ap,c)
register WCHAR_T **ap;
{
	register long f;

#ifdef NATIVE
	f = wstol(arg(1));
#else
	f = atoi(arg(1));
#endif
	if (f>=10 || f<0) {
		cf = NULL;
		ofx = f;
		return;
	}
	tempfile[7] = 'a'+f;
	if (ofile[f] || (ofile[f]=xfopen(tempfile,"w"))) {
		ofx = f;
		cf = ofile[f];
	}
}

/* ARGSUSED */
dodivnum(ap,c)
{
	pbnum((long) ofx);
}

/* ARGSUSED */
dodnl(ap,c)
WCHAR_T 	*ap;
{
	register t;

	while ((t=getchr())!=l_type('\n') && t!=EOF)
		;
}

dodump(ap,c)
WCHAR_T 	**ap;
{
	register struct nlist *np;
	register	i;

	if (c > 0)
		while (c--) {
			if ((np = lookup(*++ap))->name != NULL)
				dump(np->name,np->def);
		}
	else
		for (i=0; i<hshsize; i++)
			for (np=hshtab[i]; np!=NULL; np=np->next)
				dump(np->name,np->def);
}

dump(name,defnn)
register WCHAR_T	*name,
			*defnn;
{
	register WCHAR_T	*s = defnn;
	register	i,cnt;

	fprintf(stderr,"%S:\t",name);

	while (*s++)
		;
	--s;

	while (s > defnn)
		if (*--s&~LOWBITS)
			fprintf(stderr,"<%S>",barray[*s&LOWBITS].bname);
		else
			fputwc(*s,stderr);
	fputc('\n',stderr);
}

doerrp(ap,c)
WCHAR_T 	**ap;
{
	if (c > 0)
		fprintf(stderr,"%S",ap[1]);
}

long	evalval;	/* return value from yacc stuff */
char	*pe;	/* used by grammar */
doeval(ap,c)
WCHAR_T 	**ap;
{
	register int	base ;
	register int	pad ;
	char	*bufptr;
#ifdef NATIVE
	char	buf[WORKLEN];

	(void)wcstombs(buf, arg(1), WORKLEN-1);
	buf[WORKLEN-1] = EOS;
	bufptr = buf;
	base = (int)wstol(arg(2));
	pad = (int)wstol(arg(3));
#else 
	bufptr = arg(1);
	base = atoi(arg(2));
	pad = atoi(arg(3));
#endif
	evalval = 0;
	if (c > 0) {
		pe = bufptr;
		if (yyparse()!=0)
			error(gettxt(":131","invalid expression"));
	}
	pbnbr(evalval, base>0?base:10, pad>0?pad:1);
}

doexit(ap,c)
WCHAR_T	**ap;
{
#ifdef NATIVE
	delexit((int)wstol(arg(1)));
#else
	delexit((int)atoi(arg(1)));
#endif
}

doif(ap,c)
register WCHAR_T **ap;
{

	if (c < 3)
		return;
	while (c >= 3) {
		if (wscmp(ap[1],ap[2])==0) {
			pbstr(ap[3]);
			return;
		}
		c -= 3;
		ap += 3;
	}
	if (c > 0)
		pbstr(ap[1]);
}

doifdef(ap,c)
WCHAR_T 	**ap;
{

	if (c < 2)
		return;

	while (c >= 2) {
		if (lookup(ap[1])->name != NULL) {
			pbstr(ap[2]);
			return;
		}
		c -= 2;
		ap += 2;
	}

	if (c > 0)
		pbstr(ap[1]);
}

doincl(ap,c)
WCHAR_T	**ap;
{
	incl(ap,c,1);
}

incl(ap,c,noisy)
register WCHAR_T 	**ap;
{
	char 	*bufptr;
#ifdef	NATIVE
	char	buf[PATH_MAX+1];

	(void)wcstombs(buf, ap[1], PATH_MAX);
	buf[PATH_MAX] = EOS;
	bufptr = buf;
#else
	bufptr = (char *)ap[1];
#endif

	if (c>0 && strlen(bufptr)>0) {
		if (ifx >= 9)
			error(gettxt(":132","input file nesting too deep (9)"));
		if ((ifile[++ifx]=fopen(bufptr,"r"))==NULL){
			--ifx;
			if (noisy)
				error(gettxt(":120",badfile));
		} else {
			ipstk[ifx] = ipflr = ip;
			setfname(bufptr);
		}
	}
}

doincr(ap,c)
WCHAR_T 	**ap;
{
#ifdef NATIVE
	pbnum(wstol(arg(1))+1);
#else
	pbnum(ctol(arg(1))+1);
#endif
}

doindex(ap,c)
WCHAR_T	**ap;
{
	register WCHAR_T	*subj = arg(1);
	register WCHAR_T	*obj  = arg(2);
	register	i;

	for (i=0; *subj; ++i)
		if (leftmatch(subj++,obj)) {
			pbnum( (long) i );
			return;
		}

	pbnum( (long) -1 );
}

leftmatch(str,substr)
register WCHAR_T	*str;
register WCHAR_T	*substr;
{
	while(*substr)
		if (*str++ != *substr++)
			return (0);

	return (1);
}

dolen(ap,c)
WCHAR_T 	**ap;
{
	pbnum(wslen(arg(1)));
}


domake(ap,c)
WCHAR_T 	**ap;
{
#ifdef NATIVE
	char	buf[WORKLEN];
	char	*filebuf;
	WCHAR_T	wfilebuf[WORKLEN/sizeof(WCHAR_T)];

	(void)wcstombs(buf, ap[1], WORKLEN-1);
	buf[WORKLEN-1] = EOS;
	if (c > 0)
	{
		filebuf=mktemp(buf);
		(void)mbstowcs(wfilebuf,filebuf,WORKLEN/sizeof(WCHAR_T)-1);
		wfilebuf[WORKLEN/sizeof(WCHAR_T)-1] = 0;
		pbstr(wfilebuf);
	}
#else
	if (c > 0)
		pbstr(mktemp(ap[1]));
#endif
}

dopopdef(ap,c)
WCHAR_T	**ap;
{
	register	i;

	for (i=1; i<=c; ++i)
		undef(ap[i]);
}

dopushdef(ap,c)
WCHAR_T	**ap;
{
	def(ap,c,PUSH);
}

doshift(ap,c)
register WCHAR_T	**ap;
register c;
{
	if (c <= 1)
		return;

	for (;;) {
		pbstr(rquote);
		pbstr(ap[c--]);
		pbstr(lquote);

		if (c <= 1)
			break;

		pbstr(l_type(","));
	}
}

dosincl(ap,c)
WCHAR_T	**ap;
{
	incl(ap,c,0);
}

dosubstr(ap,c)
register WCHAR_T 	**ap;
{
	WCHAR_T	*str;
	char    *p;
	int	inlen, outlen;
	register long	offset, ix;

	inlen = wslen(str=arg(1));
#ifdef NATIVE
	offset = wstol(arg(2));
#else
	offset = atoi(arg(2));
#endif

	if (offset<0 || offset>=inlen)
		return;

#ifdef NATIVE
	outlen = c>=3? wstol(ap[3]): inlen;
#else
	outlen = c>=3? atoi(ap[3]): inlen;
#endif
	ix = min(offset+outlen,inlen);

	while (ix > offset)
		putbak(str[--ix]);
}

dosyscmd(ap,c)
WCHAR_T 	**ap;
{
	char	*bufptr;
#ifdef NATIVE
	char	buf[WORKLEN];

	(void)wcstombs(buf, ap[1], WORKLEN-1);
	buf[WORKLEN-1] = EOS;
	bufptr = buf;
#else
	bufptr = (char *)ap[1];
#endif

	sysrval = 0;
	if (c > 0) {
		fflush(stdout);
		sysrval = system(bufptr);
	}
}

/* ARGSUSED */
dosysval(ap,c)
WCHAR_T	**ap;
{
	pbnum((long) (sysrval < 0 ? sysrval :
		(sysrval >> 8) & ((1 << 8) - 1)) |
		((sysrval & ((1 << 8) - 1)) << 8));
}

dotransl(ap,c)
WCHAR_T 	**ap;
{
	WCHAR_T	*sink, *fr, *sto;
	register WCHAR_T	*source, *to;

	if (c<1)
		return;

	sink = ap[1];
	fr = arg(2);
	sto = arg(3);

	for (source = ap[1]; *source; source++) {
		register WCHAR_T	*i;
		to = sto;
		for (i = fr; *i; ++i) {
			if (*source==*i)
				break;
			if (*to)
				++to;
		}
		if (*i) {
			if (*to)
				*sink++ = *to;
		} else
			*sink++ = *source;
	}
	*sink = EOSW;
	pbstr(ap[1]);
}

dotroff(ap,c)
register WCHAR_T	**ap;
{
	register struct nlist	*np;

	trace = 0;

	while (c > 0)
		if ((np=lookup(ap[c--]))->name)
			np->tflag = 0;
}

dotron(ap,c)
register WCHAR_T	**ap;
{
	register struct nlist	*np;

	trace = !*arg(1);

	while (c > 0)
		if ((np=lookup(ap[c--]))->name)
			np->tflag = 1;
}

doundef(ap,c)
WCHAR_T	**ap;
{
	register	i;

	for (i=1; i<=c; ++i)
		while (undef(ap[i]))
			;
}

undef(nam)
WCHAR_T	*nam;
{
	register struct	nlist *np, *tnp;

	if ((np=lookup(nam))->name==NULL)
		return 0;
	tnp = hshtab[hshval];	/* lookup sets hshval */
	if (tnp==np)	/* it's in first place */
		hshtab[hshval] = tnp->next;
	else {
		while (tnp->next != np)
			tnp = tnp->next;

		tnp->next = np->next;
	}
	cfree((char *)np->name);
	cfree((char *)np->def);
	cfree((char *) np);
	return 1;
}

doundiv(ap,c)
register WCHAR_T 	**ap;
{
	register int i;

	if (c<=0)
		for (i=1; i<10; i++)
			undiv(i,OK);
	else
		while (--c >= 0)
		{
#ifdef NATIVE
			undiv((int)wstol(*ap),OK);
#else
			undiv(atoi(*ap),OK);
#endif
			++ap;
		}
}

dowrap(ap,c)
WCHAR_T	**ap;
{
	register WCHAR_T	*a = arg(1);

	if (Wrapstr)
		cfree((char *)Wrapstr);

	Wrapstr = (WCHAR_T *)xcalloc(wslen(a)+1,sizeof(WCHAR_T));
	wscpy(Wrapstr,a);
}

struct bs	barray[] = {
	dochcom,	l_type("changecom"),
	docq,		l_type("changequote"),
	dodecr,		l_type("decr"),
	dodef,		l_type("define"),
	dodefn,		l_type("defn"),
	dodiv,		l_type("divert"),
	dodivnum,	l_type("divnum"),
	dodnl,		l_type("dnl"),
	dodump,		l_type("dumpdef"),
	doerrp,		l_type("errprint"),
	doeval,		l_type("eval"),
	doexit,		l_type("m4exit"),
	doif,		l_type("ifelse"),
	doifdef,	l_type("ifdef"),
	doincl,		l_type("include"),
	doincr,		l_type("incr"),
	doindex,	l_type("index"),
	dolen,		l_type("len"),
	domake,		l_type("maketemp"),
	dopopdef,	l_type("popdef"),
	dopushdef,	l_type("pushdef"),
	doshift,	l_type("shift"),
	dosincl,	l_type("sinclude"),
	dosubstr,	l_type("substr"),
	dosyscmd,	l_type("syscmd"),
	dosysval,	l_type("sysval"),
	dotransl,	l_type("translit"),
	dotroff,	l_type("traceoff"),
	dotron,		l_type("traceon"),
	doundef,	l_type("undefine"),
	doundiv,	l_type("undivert"),
	dowrap,		l_type("m4wrap"),
	0,		0
};
