#ident	"@(#)m4:common/m4.h	1.11"

#include	<ctype.h>
#include	<pfmt.h>

#ifdef NATIVE

#include	<widec.h>

#define WCHAR_T	wchar_t
#define	l_type(a)	L##a

#else	/* building for "bootstrapping" environment */

#undef EOSW
#define	EOSW	EOS

#define	WCHAR_T char

#undef	iswalpha
#undef	iswdigit
#undef	iswalnum
#undef	iswspace
#undef	iswgraph

#define iswalpha(c)	isalpha(c)
#define iswdigit(c)	isdigit(c)
#define iswalnum(c)	isalnum(c)
#define iswspace(c)	isspace(c)
#define iswgraph(c)	isgraph(c)

#undef	wscmp
#undef	wscpy
#undef	wslen

#define	wscmp(s1, s2)	strcmp(s1, s2)
#define	wscpy(s1, s2)	strcpy(s1, s2)
#define	wslen(s)	strlen(s)

#undef	fputws
#undef	fputwc
#undef	getwc

#define	fputws(str, stream)	fputs(str, stream)
#define	fputwc(chr, stream)	fputc(chr, stream)
#define	getwc(stream)		getc(stream)

#define	l_type(a)	a

#endif

#include	<locale.h>

#define EOS	'\0'

#ifndef EOSW
#define EOSW	L'\0'
#endif

#define LOW7	0177
#define LOW31	0x7FFFFFFF
#ifdef NATIVE
#define	LOWBITS	LOW31
#else	
#define	LOWBITS	LOW7
#endif

#define MAXSYM	5
#define PUSH	1
#define NOPUSH	0
#define OK	0
#define NOT_OK	1
#define WORKLEN	500

#define	stkchr(c)	(op < obuflm? (*op++ = (c)): error2(gettxt(":133",aofmsg),bufsize))
#define	putbak(c)	(ip < ibuflm? (*ip++ = (c)): error2(gettxt(":134",pbmsg),bufsize))

#ifdef NATIVE
#define sputchr(c,f)	(putwc(c,f)==L'\n'? lnsync(f): 0)
#define putchr(c)	(Cp?stkchr(c):cf?(sflag?sputchr(c,cf):putwc(c,cf)):0)
#else
#define sputchr(c,f)	(putc(c,f)=='\n'? lnsync(f): 0)
#define putchr(c)	(Cp?stkchr(c):cf?(sflag?sputchr(c,cf):putc(c,cf)):0)
#endif /* NATIVE */

struct bs {
	int	(*bfunc)();
	WCHAR_T	*bname;
};

struct	call {
	int	plev;
	WCHAR_T	**argp;
};

struct	nlist {
	WCHAR_T	*name;
	WCHAR_T	*def;
	char	tflag;
	struct	nlist *next;
};

extern FILE	*cf;
extern FILE	*ifile[];
extern FILE	*ofile[];
extern FILE	*xfopen();
extern WCHAR_T	**Ap;
extern WCHAR_T	**argstk;
extern WCHAR_T	*Wrapstr;
extern WCHAR_T	*astklm;
extern WCHAR_T	*inpmatch();
extern char	*chkbltin();
extern void	*xcalloc();
extern WCHAR_T	*copy();
extern char	*mktemp();
extern char	*strcpy();
extern char	*fnbufend;
extern char	*fname[];
extern WCHAR_T	*ibuf;
extern WCHAR_T	*ibuflm;
extern WCHAR_T	*ip;
extern WCHAR_T	*ipflr;
extern WCHAR_T	*ipstk[10];
extern WCHAR_T	*obuf;
extern WCHAR_T	*obuflm;
extern WCHAR_T	*op;
extern char	*procnam;
extern char	*tempfile;
extern WCHAR_T	*token;
extern WCHAR_T	*toklm;
extern int	C;
extern int	getchr();
extern char	aofmsg[];
extern char	astkof[];
extern char	badfile[];
extern char	fnbuf[];
extern WCHAR_T	lcom[];
extern WCHAR_T	lquote[];
extern char	nocore[];
extern WCHAR_T	nullstr[];
extern char	pbmsg[];
extern WCHAR_T	rcom[];
extern WCHAR_T	rquote[];
extern char	type[];
extern int	bufsize;
extern void	catchsig();
extern int	fline[];
extern int	hshsize;
extern int	hshval;
extern int	ifx;
extern int	nflag;
extern int	ofx;
extern int	sflag;
extern int	stksize;
extern int	sysrval;
extern int	toksize;
extern int	trace;
extern long	ctol();
extern struct bs	barray[];
extern struct call	*Cp;
extern struct call	*callst;
extern struct nlist	**hshtab;
extern struct nlist	*install();
extern struct nlist	*lookup();
