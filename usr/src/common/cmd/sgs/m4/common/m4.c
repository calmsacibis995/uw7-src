#ident	"@(#)m4:common/m4.c	1.24"

#include	<stdio.h>
#include	<signal.h>
#include	<stdlib.h>
#include	"m4.h"

#include 	"sgs.h"

#define match(c,s)	(c==*s && (!s[1] || inpmatch(s+1)))
#define TOOL	"m4"
#define OPTIONS "-esV? -Bint -Hint -Sint -Tint -Dname[=val] -Uname [ files ]"

static const char moremsg[] = "more than %d chars in word";

main(argc,argv)
char 	**argv;
{
	register t;
	char     label[256];

#ifdef unix
	{
	static	sigs[] = {SIGHUP, SIGINT, SIGPIPE, 0};
	for (t=0; sigs[t]; ++t)
		if (signal(sigs[t], SIG_IGN) != SIG_IGN)
			(void) signal(sigs[t],catchsig);
	}

	tempfile = mktemp("/tmp/m4aXXXXX");
	close(creat(tempfile,0));
#endif

/* #ifdef NATIVE */
	(void)sprintf(label,"UX:%sm4", SGS);
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcplu");
	(void)setlabel(label);
/* #endif */

/* WARNING: the following depends on the implementation of the ctype macros! */
#if defined(__STDC__)
	(__ctype+1)['_'] |= _L;	/* makes '_' a real live letter */
#else
	(_ctype+1)['_'] |= _L;	/* makes '_' a real live letter */
#endif
/* end of warning */

	procnam = argv[0];
	getflags(&argc,&argv);
	initalloc();

	setfname("-");
	if (argc>1) {
		--argc;
		++argv;
		if (strcmp(argv[0],"-")) {
			ifile[ifx] = xfopen(argv[0],"r");
			setfname(argv[0]);
		}
	}
	

	for (;;) {
		token[0] = t = getchr();
		token[1] = EOSW;
		

		if (t==EOF) {
			if (ifx > 0) {
				fclose(ifile[ifx]);
				ipflr = ipstk[--ifx];
				continue;
			}

			getflags(&argc,&argv);

			if (argc<=1)
				if (Wrapstr) {
					pbstr(Wrapstr);
					Wrapstr = NULL;
					continue;
				} else
					break;

			--argc;
			++argv;

			if (ifile[ifx]!=stdin)
				fclose(ifile[ifx]);

			if (*argv[0]=='-')
				ifile[ifx] = stdin;
			else
				ifile[ifx] = xfopen(argv[0],"r");

			setfname(argv[0]);
			continue;
		}

#ifndef NATIVE
		if (isalpha(t)) {
			register char	*tp = token+1;
			register	tlim = toksize;
			struct nlist	*macadd;  /* temp variable */
			int	temp;

			while (isalnum(temp = getchr())){
				*tp++ = temp;
				if (--tlim<=0)
					error2(gettxt(":118",moremsg), toksize);
			}

			if (temp != EOF)
				putbak(temp);
			*tp = EOS;

			macadd = lookup(token);
			*Ap = (char *) macadd;
			if (macadd->def) {
				if ((char *) (++Ap) >= astklm) {
					--Ap;
					error2(gettxt(":125",astkof),stksize);
				}

				if (Cp++==NULL)
					Cp = callst;

				Cp->argp = Ap;
				*Ap++ = op;
				puttok(token);
				stkchr(EOS);
				if (temp == EOF)
					pbstr("()");
				else
				{
					t = getchr();
					putbak(t);

					if (t!='(')
						pbstr("()");
					else	/* try to fix arg count */
						*Ap++ = op;
				}
				Cp->plev = 0;
			} else {
				puttok(token);
			}
		} else 
#endif
		if(match(t,lquote)) {
			register	qlev = 1;

			for (;;) {
				token[0] = t = getchr();
				token[1] = EOSW;

				if (match(t,rquote)) {
					if (--qlev > 0)
						puttok(token);
					else
						break;
				} else if (match(t,lquote)) {
					++qlev;
					puttok(token);
				} else {
					if (t==EOF)
						error(gettxt(":117","EOF in quote"));

					putchr(t);
				}
			}
		} else if (match(t,lcom)) {
			puttok(token);

			for (;;) {
				token[0] = t = getchr();
				token[1] = EOSW;

				if (match(t,rcom)) {
					puttok(token);
					break;
				} else {
					if (t==EOF)
						error(gettxt(":74","EOF inside comment"));
					putchr(t);
				}
			}
		} else if ((t == l_type('(')) && (Cp != NULL)) {
			if (Cp->plev)
				stkchr(t);
			else {
				/* skip white before arg */
				while (iswspace(t=getchr()))
					;

				if (t != EOF)
					putbak(t);
			}

			++Cp->plev ;
		} else if ((t == l_type(')')) && (Cp != NULL)) {
			--Cp->plev;

			if (Cp->plev==0) {
				stkchr(EOSW);
				expand(Cp->argp,Ap-Cp->argp-1);
				op = *Cp->argp;
				Ap = Cp->argp-1;

				if (--Cp < callst)
					Cp = NULL;
			} else
				stkchr(t);
		} else if (t==l_type(',') && Cp->plev<=1 && Cp != NULL) {
			stkchr(EOSW);
			*Ap = op;

			if ((WCHAR_T *) (++Ap) >= astklm) {
				--Ap;
				error2(gettxt(":125",astkof),stksize);
			}

			while (iswspace(t=getchr()))
				;

			if (t != EOF)
				putbak(t);
		}else if(iswalpha(t) || 
		        ((unsigned int)t > 0x7f && iswgraph(t))){
			register WCHAR_T	*tp = token+1;
			register	tlim = toksize;
			struct nlist	*macadd;   /*temp variable */
			long temp;

	     		while(iswalnum(temp=getchr()) ||
	     		     ((unsigned int)temp > 0x7f && iswgraph(temp)))
			{
				if (--tlim<=0)
					error2(gettxt(":118",moremsg), toksize);
				if((match(temp,lquote)) ||
				   (match(temp,lcom) )  ||
				   (temp==l_type('('))     ||
				   (temp==l_type(')'))     ||
				   (iswspace(temp))     ||
				   (temp==l_type(','))) 
					break;
				else
					*tp++ = temp;
			}

			if (temp != EOF)
				putbak(temp);
			*tp = EOSW;

			macadd = lookup(token);
			*Ap = (WCHAR_T *) macadd;
			if (macadd->def) {
				if ((WCHAR_T *) (++Ap) >= astklm) {
					--Ap;
					error2(gettxt(":125",astkof),stksize);
				}

				if (Cp++==NULL)
					Cp = callst;

				Cp->argp = Ap;
				*Ap++ = op;
				puttok(token);
				stkchr(EOSW);
				if (temp == EOF)
					pbstr(l_type("()"));
				else
				{
					t = getchr();
					putbak(t);

					if (t!=l_type('('))
						pbstr(l_type("()"));
					else	/*	 try to fix arg count	*/
						*Ap++ = op;
				}
				Cp->plev = 0;
			} else
				puttok(token);
		} else if (Cp==NULL) {
			putchr(t);
		}else
			stkchr(t);
	}
	if (Cp!=NULL)
		error(gettxt(":126","EOF in argument list"));

	delexit(OK);
}

WCHAR_T	*inpmatch(s)
register WCHAR_T	*s;
{
	register WCHAR_T	*tp = token+1;
	long temp;

	while (*s) {
		temp = getchr();

		if (temp != *s++) {
			*tp = EOSW;
			pbstr((token+1));
			return 0;
		}
		*tp++ = temp;
	}

	*tp = EOSW;
	return token;
}

getflags(xargc,xargv)
register int	*xargc;
register char 	***xargv;
{
	int errflag = 0;
#ifdef NATIVE
	wchar_t *ws[3];
	size_t len;
#endif

	while (*xargc > 1) {
		register char	*arg = (*xargv)[1];

		if (arg[0]!='-' || arg[1]==EOS)
			break;

		switch (arg[1]) {
		case 'B':
			bufsize = atoi(&arg[2]);
			break;
		case 'D':
			{
			register char *t;
			char *s[2];

			initalloc();

			for (t = s[0] = &arg[2]; *t; t++)
				if (*t=='=') {
					*t++ = EOS;
					break;
				}

			s[1] = t;
#ifdef NATIVE
			len = 1 + mbstowcs((wchar_t *)0, s[0], ~0ul);
			ws[1] = malloc(len * sizeof(wchar_t));
			(void) mbstowcs(ws[1], s[0], len);
			len = 1 + mbstowcs((wchar_t *)0, s[1], ~0ul);
			ws[2] = malloc(len * sizeof(wchar_t));
			(void) mbstowcs(ws[2], s[1], len);
			dodef(ws,2);
#else
			dodef(&s[-1],2);
#endif
			break;
			}
		case 'H':
			hshsize = atoi(&arg[2]);
			break;
		case 'S':
			stksize = atoi(&arg[2]);
			break;
		case 'T':
			toksize = atoi(&arg[2]);
			break;
		case 'U':
			{
			char *s[1];

			initalloc();
			s[0] = &arg[2];
#ifdef	NATIVE
			len = 1 + mbstowcs((wchar_t *)0, s[0], ~0ul);
			ws[1] = malloc(len * sizeof(wchar_t));
			(void) mbstowcs(ws[1], s[0], len);
			doundef(ws, 1);
#else
			doundef(&s[-1],1);
#endif
			break;
			}
		case 'V':
			pfmt(stderr,MM_INFO,":114: %s %s\n" ,SGU_PKG,SGU_REL);
			break;
		case 'e':
#ifdef unix
			setbuf(stdout,(char *) NULL);
			signal(SIGINT,SIG_IGN);
#endif
			break;
		case 's':
			/* turn on line sync */
			sflag = 1;
			break;
		case '?':
			errflag++;
			break;
		default:
			pfmt(stderr,MM_ERROR,":115: bad option: %s\n",
                                arg);
			errflag++;
			break;
		}

		(*xargv)++;
		--(*xargc);
	}
	if (errflag) {
		pfmt(stderr,MM_ACTION, ":116: Usage: m4 [%s]\n"
                                        ,OPTIONS);
		delexit(NOT_OK);
	}
	return;
}

initalloc()
{
	static	done = 0;
	register	t;
	static char ux[]="unix";

	if (done++)
		return;

	hshtab = (struct nlist **) xcalloc(hshsize,sizeof(struct nlist *));
	callst = (struct call *) xcalloc(stksize/3+1,sizeof(struct call));
	Ap = argstk = (WCHAR_T **) xcalloc(stksize+3,sizeof(WCHAR_T *));
	ipstk[0] = ipflr = ip = ibuf = (WCHAR_T *)xcalloc(bufsize+1,sizeof(WCHAR_T));
	op = obuf = (WCHAR_T *)xcalloc(bufsize+1,sizeof(WCHAR_T));
	token = (WCHAR_T *)xcalloc(toksize+1,sizeof(WCHAR_T));

	astklm = (WCHAR_T *) (&argstk[stksize]);
	ibuflm = &ibuf[bufsize];
	obuflm = &obuf[bufsize];
	toklm = &token[toksize];

	for (t=0; barray[t].bname; ++t) {
		static WCHAR_T	p[2] = {0, EOSW};

		p[0] = t|~LOWBITS;
		install(barray[t].bname,p,NOPUSH);
	}

#ifdef unix
	install(l_type("unix"),nullstr,NOPUSH);
#endif

}

struct nlist	*
install(nam,val,mode)
WCHAR_T	*nam;
register WCHAR_T	*val;
{
	register struct nlist *np;
	register WCHAR_T	*cp;
	int		l;

	if (mode==PUSH)
		lookup(nam);	/* lookup sets hshval */
	else
		while (undef(nam))	/* undef calls lookup */
			;

	np = (struct nlist *) xcalloc(1,sizeof(*np));
	np->name = copy(nam);
	np->next = hshtab[hshval];
	hshtab[hshval] = np;

	cp = (WCHAR_T *)xcalloc((l=wslen(val))+1,sizeof(*val));
	np->def = cp;
	cp = &cp[l];

	while (*val)
		*--cp = *val++;
}

struct nlist	*
lookup(str)
WCHAR_T 	*str;
{
	register WCHAR_T		*s1;
	register struct nlist	*np;
	static struct nlist	nodef;

	s1 = str;

	for (hshval = 0; *s1; )
		hshval += *s1++;

	hshval %= hshsize;

	for (np = hshtab[hshval]; np!=NULL; np = np->next) {
		if (!wscmp(str, np->name))
			return(np);
	}

	return(&nodef);
}

expand(a1,c)
WCHAR_T	**a1;
{
	register WCHAR_T	*dp;
	register struct nlist	*sp;

	sp = (struct nlist *) a1[-1];

	if (sp->tflag || trace) {
		int	i;
		fprintf(stderr,"Trace(%d): %s",Cp-callst,a1[0]);

		if (c > 0) {
			fprintf(stderr,"(%s",chkbltin(a1[1]));
			for (i=2; i<=c; ++i)
				fprintf(stderr,",%s",chkbltin(a1[i]));
			fprintf(stderr,")");
		}

		fprintf(stderr,"\n");
	}

	dp = sp->def;

	for (; *dp; ++dp) {
		if (*dp&~LOWBITS ) {
			(*barray[*dp&LOWBITS].bfunc)(a1,c);
		} else if (dp[1]==l_type('$')) {
			if (iswdigit(*dp)) {
				register	n;
				if ((n = *dp-l_type('0')) <= c)
					pbstr(a1[n]);
				++dp;
			} else if (*dp==l_type('#')) {
				pbnum((long) c);
				++dp;
			} else if (*dp==l_type('*') || *dp==l_type('@')) {
				register i = c;
				WCHAR_T **a = a1;

				if (i > 0)
					for (;;) {
						if (*dp==l_type('@'))
							pbstr(rquote);

						pbstr(a[i--]);

						if (*dp==l_type('@'))
							pbstr(lquote);

						if (i <= 0)
							break;

						pbstr(l_type(","));
					}
				++dp;
			} else{ 
				putbak(*dp);
			}
		} else{
			putbak(*dp);
		}
	}
}

setfname(s)
register char 	*s;
{
	char *next = fname[ifx]+strlen(s)+1;

	if (next >= fnbufend) {
		ifx--;
		error(gettxt(":119","filenames too long"));
	}
	strcpy(fname[ifx],s);
	fname[ifx+1] = next;
	fline[ifx] = 1;
	nflag = 1;
	lnsync(stdout);
}

lnsync(iop)
register FILE	*iop;
{
	static int cline = 0;
	static int cfile = 0;

	if (!sflag || iop!=stdout)
		return;

	if (nflag || ifx!=cfile) {
		nflag = 0;
		cfile = ifx;
		fprintf(iop,"#line %d \"",cline = fline[ifx]);
		fpath(iop);
		fprintf(iop,"\"\n");
	} else if (++cline != fline[ifx])
		fprintf(iop,"#line %d\n",cline = fline[ifx]);
}

fpath(iop)
register FILE	*iop;
{
	register	i;

	fprintf(iop,"%s",fname[0]);

	for (i=1; i<=ifx; ++i)
		fprintf(iop,":%s",fname[i]);
}

/* ARGUSED */
void catchsig(i)
int i;
{
	i = 0;
#ifdef unix
	(void) signal(SIGHUP,SIG_IGN);
	(void) signal(SIGINT,SIG_IGN);
#endif

	delexit(NOT_OK);
}

delexit(code)
{
	register i;

	cf = stdout;

/*	if (ofx != 0) {	/* quitting in middle of diversion */
/*		ofx = 0;
/*		code = NOT_OK;
/*	}
*/
	ofx = 0;	/* ensure that everything comes out */
	for (i=1; i<10; i++)
		undiv(i,code);

	tempfile[7] = 'a';
	unlink(tempfile);

	if (code==OK)
		exit(code);

	_exit(code);
}

puttok(tp)
register WCHAR_T *tp;
{
	if (Cp) {
		while (*tp)
			stkchr(*tp++);
	} else if (cf) {
		while (*tp)
			sputchr(*tp++,cf);
	}
}

pbstr(str)
register WCHAR_T *str;
{
	register WCHAR_T *p;

	for (p = str + wslen(str); --p >= str; )
		putbak(*p);
}

undiv(i,code)
register	i;
{
	register FILE *fp;
	register	c;

	if (i<1 || i>9 || i==ofx || !ofile[i])
		return;

	fclose(ofile[i]);
	tempfile[7] = 'a'+i;

	if (code==OK && cf) {
		fp = xfopen(tempfile,"r");

		while ((c=getc(fp)) != EOF)
			sputchr(c,cf);

		fclose(fp);
	}

	unlink(tempfile);
	ofile[i] = NULL;
}

WCHAR_T 	*copy(s)
register WCHAR_T *s;
{
	register WCHAR_T *p;

	p = (WCHAR_T *)xcalloc(wslen(s)+1,sizeof(WCHAR_T *));
	wscpy(p, s);
	return(p);
}

pbnum(num)
long num;
{
	pbnbr(num,10,1);
}

pbnbr(nbr,base,len)
long	nbr;
register	base, len;
{
	register	neg = 0;

	if (base<=0)
		return;

	if (nbr<0)
		neg = 1;
	else
		nbr = -nbr;

	while (nbr<0) {
		register int	i;
		if (base>1) {
			i = nbr%base;
			nbr /= base;
#		if (-3 % 2) != -1
			while (i > 0) {
				i -= base;
				++nbr;
			}
#		endif
			i = -i;
		} else {
			i = 1;
			++nbr;
		}
		putbak(itochr(i));
		--len;
	}

	while (--len >= 0)
		putbak(l_type('0'));

	if (neg)
		putbak(l_type('-'));
}

itochr(i)
register	i;
{
	if (i>9)
		return i-10+l_type('A');
	else
		return i+l_type('0');
}

long wstol(str)
register WCHAR_T *str;
{
	register sign;
	long num;

	while (iswspace(*str))
		++str;
	num = 0;
	if (*str==l_type('-')) {
		sign = -1;
		++str;
	}
	else
		sign = 1;
	while (iswdigit(*str))
		num = num*10 + *str++ - l_type('0');
	return(sign * num);
}

min(a,b)
{
	if (a>b)
		return(b);
	return(a);
}

FILE	*
xfopen(name,mode)
char	*name,
	*mode;
{
	FILE	*fp;

	if ((fp=fopen(name,mode))==NULL)
		error(gettxt(":120",badfile));

	return fp;
}

void	*xcalloc(nbr,size)
{
	register void	*ptr;

	if ((ptr=calloc((unsigned) nbr,(unsigned) size)) == NULL)
		error(gettxt(":121",nocore));

	return ptr;
}

error2(str,num)
	char *str;
	int num;
{
	char buf[500];

	snprintf(buf,sizeof(buf),str,num);
	error(buf);
}

error(str)
	char *str;
{
	register	i;

	pfmt(stderr,MM_ERROR,":122:%s",fname[0]);

	for (i=1; i<=ifx; ++i)
		pfmt(stderr,MM_NOSTD,":123::%s",fname[i]);

	pfmt(stderr,MM_NOSTD,":124::%d %s\n",fline[ifx],str);
	if (Cp) {
		register struct call	*mptr;

		/* fix limit */
		*op = EOSW;
		(Cp+1)->argp = Ap+1;

		for (mptr=callst; mptr<=Cp; ++mptr) {
			register WCHAR_T	**aptr, **lim;

			aptr = mptr->argp;
			lim = (mptr+1)->argp-1;
			if (mptr==callst)
				fputws(*aptr,stderr);
			++aptr;
			fprintf(stderr,"(");
			if (aptr < lim)
				for (;;) {
					fputws(*aptr++,stderr);
					if (aptr >= lim)
						break;
					fprintf(stderr,",");
				}
		}
		while (--mptr >= callst)
			fprintf(stderr,")");

		fputs("\n",stderr);
	}
	delexit(NOT_OK);
}

char	*chkbltin(s)
char	*s;
{
	static char	buf[24];

	if (*s&~LOWBITS){
		sprintf(buf,"<%S>",barray[*s&LOWBITS].bname);
		return buf;
	}

	return s;
}

int	
getchr()
{
	if (ip > ipflr)
		return (*--ip);
	C = feof(ifile[ifx]) ? EOF : getwc(ifile[ifx]);
	if (C ==l_type('\n'))
		fline[ifx]++;
	return (C);
}

long 
ctol(str)
register char *str;
{
	register sign;
	long num;

	while (isspace(*str))
		++str;
	num = 0;
	if (*str=='-') {
		sign = -1;
		++str;
	}
	else
		sign = 1;
	while (isdigit(*str))
		num = num*10 + *str++ - '0';
	return(sign * num);
}
