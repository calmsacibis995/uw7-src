#ident	"@(#)ksh93:src/cmd/ksh93/sh/init.c	1.2.1.1"
#pragma prototyped
/*
 *
 * Shell initialization
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 2B-102
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *   research!dgk
 *
 */

#include        "defs.h"
#include        <stak.h>
#include        <ctype.h>
#include        "variables.h"
#include        "path.h"
#include        "fault.h"
#include        "name.h"
#include	"jobs.h"
#include	"io.h"
#include	"FEATURE/time"
#include	"FEATURE/dynamic"
#include	"lexstates.h"
#include	"FEATURE/locale"
#include	"national.h"

#if _hdr_wchar && _lib_wctype && _lib_iswctype
#   include <wchar.h>
#   include <wctype.h>
#   undef  isalpha
#   define isalpha(x)      iswalpha(x)
#   undef  isblank
#   define isblank(x)      iswblank(x)
#   if !_lib_iswblank

#     ifndef iswblank /*because we DO have one!*/

	static int
	iswblank(wint_t wc)
	{
		static int      initialized;
		static wctype_t wt;

		if (!initialized)
		{
			initialized = 1;
			wt = wctype("blank");
		}
		return(iswctype(wc, wt));
	}
#     endif
#   endif
#else
#   undef  _lib_wctype
#   ifndef isblank
#	define isblank(x)      ((x)==' '||(x)=='\t')
#   endif
#endif

#ifdef _hdr_locale
#   include	<locale.h>
#   ifndef LC_MESSAGES
#	define LC_MESSAGES	LC_ALL
#   endif /* LC_MESSAGES */
#endif /* _hdr_locale */

#define RANDMASK	0x7fff
#ifndef CLK_TCK
#   define CLK_TCK	60
#endif /* CLK_TCK */

extern char	**environ;
#ifdef MTRACE
    extern int Mt_certify;
#endif /* MTRACE */

extern char	*gettxt();

static void		env_init(void);
static void		nv_init(void);
static Hashtab_t	*inittree(const struct shtable2*);

static const char	rsh_pattern[] = "@(rk|kr|r)sh";
static int		rand_shift;
static Namval_t		*ifsnp;


/*
 * Invalidate all path name bindings
 */
static void rehash(register Namval_t *np)
{
	nv_onattr(np,NV_NOALIAS);
}

/*
 * out of memory routine for stak routines
 */
static char *nospace(int unused)
{
	NOT_USED(unused);
	error(ERROR_exit(3),gettxt(e_nospace_id,e_nospace));
	return(NIL(char*));
}

#ifdef SHOPT_MULTIBYTE
#   include	"edit.h"
    /* Trap for CSWIDTH variable */
    static void put_cswidth(Namval_t* np,const char *val,int flags,Namfun_t *fp)
    {
	register char *cp;
	register char *name = nv_name(np);
	if(ed_setwidth(val?val:"") && !(flags&NV_IMPORT))
		error(ERROR_exit(1),gettxt(e_format_id,e_format),nv_name(np));
	nv_putv(np, val, flags, fp);
    }
#endif /* SHOPT_MULTIBYTE */

/* Trap for VISUAL and EDITOR variables */
static void put_ed(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	register const char *cp, *name=nv_name(np);
	if(*name=='E' && nv_getval(nv_scoped(VISINOD)))
		goto done;
	sh_offoption(SH_VI|SH_EMACS|SH_GMACS);
	if(!(cp=val) && (*name=='E' || !(cp=nv_getval(nv_scoped(EDITNOD)))))
		goto done;
	/* turn on vi or emacs option if editor name is either*/
	cp = path_basename(cp);
	if(strmatch(cp,"*vi"))
		sh_onoption(SH_VI);
	if(strmatch(cp,"*macs"))
	{
		if(*cp=='g')
			sh_onoption(SH_GMACS);
		else
			sh_onoption(SH_EMACS);
	}
done:
	nv_putv(np, val, flags, fp);
}

/* Trap for OPTINDEX */
static void put_optindex(Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	sh.st.opterror = sh.st.optchar = 0;
	nv_putv(np, val, flags, fp);
}

/* Trap for restricted variables PATH, SHELL, ENV */
static void put_restricted(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	if(!(flags&NV_RDONLY) && sh_isoption(SH_RESTRICTED))
		error(ERROR_exit(1),gettxt(e_restricted_id,e_restricted),nv_name(np));
	if(nv_name(np)==nv_name(PATHNOD))			
	{
		sh.lastpath = 0;
		nv_scan(sh.track_tree,rehash,NV_TAGGED,NV_TAGGED);
	}
	nv_putv(np, val, flags, fp);
}

#ifdef apollo
    /* Trap for SYSTYPE variable */
    static void put_systype(Namval_t* np,const char *val,int flags,Namfun_t *fp)
    {
	sh.lastpath = 0;
	nv_scan(sh.track_tree,rehash,NV_TAGGED,NV_TAGGED);
	nv_putv(np, val, flags, fp);
    }
#endif

#ifdef _hdr_locale
    /*
     * This function needs to be modified to handle international
     * error message translations
     */
    static char *msg_translate(const char *message,int type)
    {
	NOT_USED(type);
	return((char*)message);
    }
#   ifdef SHOPT_MULTIBYTE
	void charsize_init(void)
	{
		static char fc[3] = { 0301,  ESS2, ESS3};
		char buff[8];
		register int i,n;
		wchar_t wc;
		memset(buff,0301,MB_CUR_MAX);
		for(i=0; i<=2; i++)
		{
			buff[0] = fc[i];
			if((n=mbtowc(&wc,buff,MB_CUR_MAX))>0)
			{
				if((int_charsize[i+1] = n-(i>0))>0)
					int_charsize[i+5] = wcwidth(wc);
				else
					int_charsize[i+5] = 0;
			}
		}
	}
#   endif /* SHOPT_MULTIBYTE */

    /* Trap for LC_ALL, LC_TYPE, LC_MESSAGES, LC_COLLATE and LANG */
    static void put_lang(Namval_t* np,const char *val,int flags,Namfun_t *fp)
    {
	int type;
	char *lc_all = nv_getval(LCALLNOD);
	if(nv_name(np)==nv_name(LCTYPENOD))
		type = LC_CTYPE;
	else if(nv_name(np)==nv_name(LCMSGNOD))
		type = LC_MESSAGES;
	else if(nv_name(np)==nv_name(LCCOLLNOD))
		type = LC_COLLATE;
	else if(nv_name(np)==nv_name(LCNUMNOD))
		type = LC_NUMERIC;
	else if(nv_name(np)==nv_name(LANGNOD) && !lc_all)
		type= -1;
	else
		type = LC_ALL;
	if(type>=0 && type!=LC_ALL && !lc_all)
		type= -1;
	if(type==LC_ALL || type==LC_CTYPE)
	{
		if(sh_lexstates[ST_BEGIN]!=sh_lexrstates[ST_BEGIN])
			free((void*)sh_lexstates[ST_BEGIN]);
		if(ast.locale.set&LC_SET_CTYPE)
		{
			register int c;
			char *state[4];
			sh_lexstates[ST_BEGIN] = state[0] = (char*)malloc(4*(1<<CHAR_BIT));
			memcpy(state[0],sh_lexrstates[ST_BEGIN],(1<<CHAR_BIT));
			sh_lexstates[ST_NAME] = state[1] = state[0] + (1<<CHAR_BIT);
			memcpy(state[1],sh_lexrstates[ST_NAME],(1<<CHAR_BIT));
			sh_lexstates[ST_DOL] = state[2] = state[1] + (1<<CHAR_BIT);
			memcpy(state[2],sh_lexrstates[ST_DOL],(1<<CHAR_BIT));
			sh_lexstates[ST_BRACE] = state[3] = state[2] + (1<<CHAR_BIT);
			memcpy(state[3],sh_lexrstates[ST_BRACE],(1<<CHAR_BIT));
			for(c=0; c<(1<<CHAR_BIT); c++)
			{
				if(isblank(c))
				{
					state[0][c]=0;
					state[1][c]=S_BREAK;
					state[2][c]=S_BREAK;
					continue;
				}
				if(!isalpha(c))
					continue;
				if(state[0][c]==S_REG)
					state[0][c]=S_NAME;
				if(state[1][c]==S_REG)
					state[1][c]=0;
				if(state[2][c]==S_ERR)
					state[2][c]=S_ALP;
				if(state[3][c]==S_ERR)
					state[3][c]=0;
			}
		}
		else
		{
			sh_lexstates[ST_BEGIN]=(char*)sh_lexrstates[ST_BEGIN];
			sh_lexstates[ST_NAME]=(char*)sh_lexrstates[ST_NAME];
			sh_lexstates[ST_DOL]=(char*)sh_lexrstates[ST_DOL];
			sh_lexstates[ST_BRACE]=(char*)sh_lexrstates[ST_BRACE];
		}
#ifdef SHOPT_MULTIBYTE
        	charsize_init();
#endif /* SHOPT_MULTIBYTE */
	}
	if(type==LC_ALL || type==LC_MESSAGES)
		error_info.translate = msg_translate;
	nv_putv(np, val, flags, fp);
    }
#endif /* _hdr_locale */

/* Trap for IFS assignment and invalidates state table */
static void put_ifs(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	ifsnp = 0;
	nv_putv(np, val, flags, fp);
}

/*
 * This is the lookup function for IFS
 * It keeps the sh.ifstable up to date
 */
static char* get_ifs(register Namval_t* np, Namfun_t *fp)
{
	register char *cp, *value;
	register int c,n;
	value = nv_getv(np,fp);
	if(np!=ifsnp)
	{
		ifsnp = np;
		memset(sh.ifstable,0,(1<<CHAR_BIT));
		if(cp=value)
		{
#ifdef SHOPT_MULTIBYTE
			while(n=mbtowc(0,cp,MB_CUR_MAX),c= *(unsigned char*)cp)
#else
			while(c= *(unsigned char*)cp++)
#endif /* SHOPT_MULTIBYTE */
			{
#ifdef SHOPT_MULTIBYTE
				cp+=n;
				if(n>1)
				{
					sh.ifstable[c] = S_MBYTE;
					continue;
				}
#endif /* SHOPT_MULTIBYTE */
				n = S_DELIM;
				if(c== *cp)
					cp++;
				else if(isspace(c))
					n = S_SPACE;
				sh.ifstable[c] = n;
			}
		}
		else
		{
			sh.ifstable[' '] = sh.ifstable['\t'] = S_SPACE;
			sh.ifstable['\n'] = S_NL;
		}
	}
	return(value);
}

/*
 * these functions are used to get and set the SECONDS variable
 */
#ifdef _lib_gettimeofday
#   define dtime(tp) ((double)((tp)->tv_sec)+1e-6*((double)((tp)->tv_usec)))
#   define tms	timeval
#else
#   define dtime(tp)	(((double)times(tp))/sh.lim.clk_tck)
#   define gettimeofday(a,b)
#endif

static void put_seconds(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	static double sec_offset;
	double d;
	struct tms tp;
	NOT_USED(fp);
	if(!val)
	{
		nv_stack(np, NIL(Namfun_t*));
		nv_unset(np);
		return;
	}
	if(flags&NV_INTEGER)
		d = *(double*)val;
	else
		d = sh_arith(val);
	gettimeofday(&tp,NIL(struct timezone*));
	sec_offset = dtime(&tp)-d;
	if(!np->nvalue.dp)
	{
		nv_setsize(np,3);
		np->nvalue.dp = &sec_offset;
	}
}

static char* get_seconds(register Namval_t* np, Namfun_t *fp)
{
	struct tms tp;
	double d;
	NOT_USED(fp);
	gettimeofday(&tp,NIL(struct timezone*));
	d = dtime(&tp)- *np->nvalue.dp;
	return(sh_ftos(d,nv_size(np)));
}

static double nget_seconds(register Namval_t* np, Namfun_t *fp)
{
	struct tms tp;
	NOT_USED(fp);
	gettimeofday(&tp,NIL(struct timezone*));
	return(dtime(&tp)- *np->nvalue.dp);
}

/*
 * These three functions are used to get and set the RANDOM variable
 */
static void put_rand(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	static long rand_last;
	register long n;
	NOT_USED(fp);
	if(!val)
	{
		nv_stack(np, NIL(Namfun_t*));
		nv_unset(np);
		return;
	}
	if(flags&NV_INTEGER)
		n = *(double*)val;
	else
		n = sh_arith(val);
	srand((int)(n&RANDMASK));
	rand_last = -1;
	if(!np->nvalue.lp)
		np->nvalue.lp = &rand_last;
}

/*
 * get random number in range of 0 - 2**15
 * never pick same number twice in a row
 */
static double nget_rand(register Namval_t* np, Namfun_t *fp)
{
	register long cur, last= *np->nvalue.lp;
	NOT_USED(fp);
	do
		cur = (rand()>>rand_shift)&RANDMASK;
	while(cur==last);
	*np->nvalue.lp = cur;
	return((double)cur);
}

static char* get_rand(register Namval_t* np, Namfun_t *fp)
{
	register long n = nget_rand(np,fp);
	return(fmtbase(n, 10, 0));
}

/*
 * These three routines are for LINENO
 */
static double nget_lineno(Namval_t* np, Namfun_t *fp)
{
	double d=1;
	if(error_info.line >0)
		d = error_info.line;
	else if(error_info.context && error_info.context->line>0)
		d = error_info.context->line;
	NOT_USED(np);
	NOT_USED(fp);
	return(d);
}

static void put_lineno(Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	register long n;
	NOT_USED(fp);
	if(!val)
	{
		nv_stack(np, NIL(Namfun_t*));
		nv_unset(np);
		return;
	}
	if(flags&NV_INTEGER)
		n = *(double*)val;
	else
		n = sh_arith(val);
	sh.st.firstline += nget_lineno(np,fp)+1-n;
}

static char* get_lineno(register Namval_t* np, Namfun_t *fp)
{
	register long n = nget_lineno(np,fp);
	return(fmtbase(n, 10, 0));
}

static char* get_lastarg(Namval_t* np, Namfun_t *fp)
{
	NOT_USED(np);
	NOT_USED(fp);
	return(sh.lastarg);
}

static void put_lastarg(Namval_t* np,const char *val,int flags,Namfun_t *fp)
{
	NOT_USED(fp);
	if(flags&NV_INTEGER)
		val = sh_etos(*((double*)val),12);
	if(sh.lastarg && !nv_isattr(np,NV_NOFREE))
		free((void*)sh.lastarg);
	else
		nv_offattr(np,NV_NOFREE);
	if(val)
		sh.lastarg = strdup(val);
	else
		sh.lastarg = 0;
}

#ifdef SHOPT_FS_3D
    /*
     * set or unset the mappings given a colon separated list of directories
     */
    static void vpath_set(char *str, int mode)
    {
	register char *lastp, *oldp=str, *newp=strchr(oldp,':');
	if(!sh.lim.fs3d)
		return;
	while(newp)
	{
		*newp++ = 0;
		if(lastp=strchr(newp,':'))
			*lastp = 0;
		mount((mode?newp:""),oldp,FS3D_VIEW,0);
		newp[-1] = ':';
		oldp = newp;
		newp=lastp;
	}
    }

    /* catch vpath assignments */
    static void put_vpath(register Namval_t* np,const char *val,int flags,Namfun_t *fp)
    {
	register char *cp;
	if(cp = nv_getval(np))
		vpath_set(cp,0);
	if(val)
		vpath_set((char*)val,1);
	nv_putv(np,val,flags,fp);
    }
    static const Namdisc_t VPATH_disc	= { 0, put_vpath  };
    static Namfun_t VPATH_init	= { &VPATH_disc  };
#endif /* SHOPT_FS_3D */

static const Namdisc_t IFS_disc		= {  0, put_ifs, get_ifs };
static const Namdisc_t RESTRICTED_disc	= {  0, put_restricted };
static const Namdisc_t EDITOR_disc	= {  0, put_ed };
static const Namdisc_t OPTINDEX_disc	= {  0, put_optindex };
static const Namdisc_t SECONDS_disc	= {  0, put_seconds, get_seconds, nget_seconds };
static const Namdisc_t RAND_disc	= {  0, put_rand, get_rand, nget_rand };
static const Namdisc_t LINENO_disc	= {  0, put_lineno, get_lineno, nget_lineno };
static const Namdisc_t L_ARG_disc	= {  0, put_lastarg, get_lastarg };
static Namfun_t IFS_init	= {  &IFS_disc};
static Namfun_t PATH_init	= {  &RESTRICTED_disc};
static Namfun_t SHELL_init	= {  &RESTRICTED_disc};
static Namfun_t ENV_init	= {  &RESTRICTED_disc};
static Namfun_t VISUAL_init	= {  &EDITOR_disc};
static Namfun_t EDITOR_init	= {  &EDITOR_disc};
static Namfun_t OPTINDEX_init	= {  &OPTINDEX_disc};
static Namfun_t SECONDS_init	= {  &SECONDS_disc};
static Namfun_t RAND_init	= {  &RAND_disc};
static Namfun_t LINENO_init	= {  &LINENO_disc};
static Namfun_t L_ARG_init	= {  &L_ARG_disc};
#ifdef _hdr_locale
    static const Namdisc_t LC_disc	= {  0, put_lang };
    static Namfun_t LC_TYPE_init	= {  &LC_disc};
    static Namfun_t LC_NUM_init		= {  &LC_disc};
    static Namfun_t LC_COLL_init	= {  &LC_disc};
    static Namfun_t LC_MSG_init		= {  &LC_disc};
    static Namfun_t LC_ALL_init		= {  &LC_disc};
    static Namfun_t LANG_init		= {  &LC_disc};
#endif /* _hdr_locale */
#ifdef apollo
    static const Namdisc_t SYSTYPE_disc	= {  0, put_systype };
    static Namfun_t SYSTYPE_init	= {  &SYSTYPEdisc};
#endif /* apollo */
#ifdef SHOPT_MULTIBYTE
    static const Namdisc_t CSWIDTH_disc	= {  0, put_cswidth };
    static Namfun_t CSWIDTH_init	= {  &CSWIDTH_disc};
#endif /* SHOPT_MULTIBYTE */

/*
 * This function will get called whenever a configuration parameter changes
 */
static int newconf(const char *name, const char *path, const char *value)
{
	register char *arg;
	if(strcmp(name,"UNIVERSE")==0 && strcmp(astconf(name,0,0),value))
	{
		sh.universe = 0;
		/* set directory in new universe */
		if(*(arg = path_pwd(0))=='/')
			chdir(arg);
		/* clear out old tracked alias */
		stakseek(0);
		stakputs(nv_getval(PATHNOD));
		stakputc(0);
		nv_putval(PATHNOD,stakseek(0),NV_RDONLY);
	}
	return(1);
}

/*
 * initialize the shell
 */
int sh_init(register int argc,register char *argv[])
{
	register char *name;
	register int n,prof;
#ifdef MTRACE
	Mt_certify = 1;
#endif /* MTRACE */
	memcpy(sh_lexstates,sh_lexrstates,ST_NONE*sizeof(char*));
	sh_onstate(SH_INIT);
	error_info.exit = sh_exit;
	error_info.id = path_basename(argv[0]);
	sh.cpipe[0] = -1;
	sh.coutpipe = -1;
	sh.userid=getuid();
	sh.euserid=geteuid();
	sh.groupid=getgid();
	sh.egroupid=getegid();
	for(n=0;n < 10; n++)
	{
		/* don't use lower bits when rand() generates large numbers */
		if(rand() > RANDMASK)
		{
			rand_shift = 3;
			break;
		}
	}
	sh.lim.clk_tck = sysconf(_SC_CLK_TCK);
	sh.lim.open_max = sysconf(_SC_OPEN_MAX);
	sh.lim.child_max = sysconf(_SC_CHILD_MAX);
	sh.lim.ngroups_max = sysconf(_SC_NGROUPS_MAX);
	sh.lim.posix_version = sysconf(_SC_VERSION);
	sh.lim.posix_jobcontrol = sysconf(_SC_JOB_CONTROL);
	if(sh.lim.child_max <=0)
		sh.lim.child_max = CHILD_MAX;
	if(sh.lim.open_max <0)
		sh.lim.open_max = OPEN_MAX;
	if(sh.lim.clk_tck <=0)
		sh.lim.clk_tck = CLK_TCK;
#ifdef SHOPT_FS_3D
	if(mount(".", NIL(char*),FS3D_GET|FS3D_VERSION,0) >= 0)
		sh.lim.fs3d = 1;
#endif /* SHOPT_FS_3D */
	sh_ioinit();
	stakinstall(NIL(Stak_t*),nospace);
	/* set up memory for name-value pairs */
	nv_init();
	/* read the environment */
#ifdef SHOPT_MULTIBYTE
       	charsize_init();
#endif /* SHOPT_MULTIBYTE */
	env_init();
	nv_putval(IFSNOD,(char*)e_sptbnl,NV_RDONLY);
#ifdef SHOPT_FS_3D
	nv_stack(VPATHNOD, &VPATH_init);
#endif /* SHOPT_FS_3D */
	astconfdisc(newconf);
	(void) setlocale(LC_ALL, "");
	(void) setcat("ksh93");
	/* initialize signal handling */
	sh_siginit();
#ifdef SHOPT_TIMEOUT
	sh.st.tmout = SHOPT_TIMEOUT;
#endif /* SHOPT_TIMEOUT */
	/* initialize jobs table */
	job_clear();
	if(argc>0)
	{
		name = path_basename(*argv);
		if(*name=='-')
		{
			name++;
			sh.login_sh = 2;
		}
		/* check for restricted shell */
		if(argc>0 && strmatch(name,rsh_pattern))
			sh_onoption(SH_RESTRICTED);
		/* look for options */
		/* sh.st.dolc is $#	*/
		if((sh.st.dolc = sh_argopts(-argc,argv)) < 0)
		{
			sh.exitval = 2;
			sh_done(0);
		}
		sh.st.dolv=argv+(argc-1)-sh.st.dolc;
		sh.st.dolv[0] = argv[0];
		if(sh.st.dolc < 1)
			sh_onoption(SH_SFLAG);
		if(!sh_isoption(SH_SFLAG))
		{
			sh.st.dolc--;
			sh.st.dolv++;
		}
	}
	/* set[ug]id scripts require the -p flag */
	prof = !sh_isoption(SH_PRIVILEGED);
	if(sh.userid!=sh.euserid || sh.groupid!=sh.egroupid)
	{
#ifdef SHOPT_P_SUID
		/* require sh -p to run setuid and/or setgid */
		if(!sh_isoption(SH_PRIVILEGED) && sh.euserid < SHOPT_P_SUID)
		{
			setuid(sh.euserid=sh.userid);
			setgid(sh.egroupid=sh.groupid);
		}
		else
#else
		{
			sh_onoption(SH_PRIVILEGED);
			prof = 0;
		}
#endif /* SHOPT_P_SUID */
#ifdef SHELLMAGIC
		/* careful of #! setuid scripts with name beginning with - */
		if(sh.login_sh && strcmp(argv[0],argv[1])==0)
			error(ERROR_exit(1),gettxt(e_prohibited_id,e_prohibited));
#endif /*SHELLMAGIC*/
	}
	else
		sh_offoption(SH_PRIVILEGED);
	sh.shname = strdup(sh.st.dolv[0]); /* shname for $0 in profiles */
	/*
	 * return here for shell script execution
	 * but not for parenthesis subshells
	 */
	error_info.id = strdup(sh.st.dolv[0]); /* error_info.id is $0 */
	sh_offstate(SH_INIT);
	return(prof);
}

/*
 * reinitialize before executing a script
 */
void sh_reinit(char *argv[])
{
	hashfree(sh.fun_tree);
	sh.fun_tree = hashalloc(sh.bltin_tree,HASH_set,HASH_SCOPE|HASH_ALLOCATE,0);
	hashfree(sh.alias_tree);
	sh.alias_tree = inittree(shtab_aliases);
	/* set up new args */
	sh.arglist = sh_argcreate(argv);
}

/*
 * set when creating a local variable of this name
 */
Namfun_t *nv_cover(register Namval_t *np)
{
	register Namfun_t *nfp=0;
	if(np==IFSNOD)
		nfp = &IFS_init;
	else if(np==PATHNOD)
		nfp = &PATH_init;
	else if(np==SHELLNOD)
		nfp = &SHELL_init;
	return(nfp);
}

/*
 * Initialize the shell name and alias table
 */
static void nv_init(void)
{
	double d=0;
	sh.var_base = sh.var_tree = inittree(shtab_variables);
	nv_stack(IFSNOD, &IFS_init);
	nv_stack(PATHNOD, &PATH_init);
	nv_stack(SHELLNOD, &SHELL_init);
	nv_stack(ENVNOD, &ENV_init);
	nv_stack(VISINOD, &VISUAL_init);
	nv_stack(EDITNOD, &EDITOR_init);
	nv_stack(OPTINDNOD, &OPTINDEX_init);
	nv_stack(SECONDS, &SECONDS_init);
	nv_stack(L_ARGNOD, &L_ARG_init);
	nv_putval(SECONDS, (char*)&d, NV_INTEGER);
	nv_stack(RANDNOD, &RAND_init);
	d = (sh.pid&RANDMASK);
	nv_putval(RANDNOD, (char*)&d, NV_INTEGER);
	nv_stack(LINENO, &LINENO_init);
#ifdef _hdr_locale
	nv_stack(LCTYPENOD, &LC_TYPE_init);
	nv_stack(LCALLNOD, &LC_ALL_init);
	nv_stack(LCMSGNOD, &LC_MSG_init);
	nv_stack(LCCOLLNOD, &LC_COLL_init);
	nv_stack(LCNUMNOD, &LC_NUM_init);
	nv_stack(LANGNOD, &LANG_init);
#endif /* _hdr_locale */
#ifdef apollo
	nv_stack(SYSTYPENOD, &SYSTYPE_init);
#endif /* apollo */
#ifdef SHOPT_MULTIBYTE
	nv_stack(CSWIDTHNOD, &CSWIDTH_init);
#endif /* SHOPT_MULTIBYTE */
	(PPIDNOD)->nvalue.lp = (&sh.ppid);
	(TMOUTNOD)->nvalue.lp = (&sh.st.tmout);
	(MCHKNOD)->nvalue.lp = (long*)(&sh_mailchk);
	(OPTINDNOD)->nvalue.lp = (&sh.st.optindex);
	/* set up the seconds clock */
	sh.alias_tree = inittree(shtab_aliases);
	sh.track_tree = hashalloc(sh.var_tree,HASH_set,HASH_ALLOCATE,0);
	sh.bltin_tree = inittree((const struct shtable2*)shtab_builtins);
	sh.fun_tree = hashalloc(sh.bltin_tree,HASH_set,HASH_SCOPE|HASH_ALLOCATE,0);
}

/*
 * initialize name-value pairs
 */

static Hashtab_t *inittree(const struct shtable2 *name_vals)
{
	register Namval_t *np;
	register const struct shtable2 *tp;
	register unsigned n = 0;
	register Hashtab_t *treep;
	for(tp=name_vals;*tp->sh_name;tp++)
		n++;
	np = (Namval_t*)calloc(n,sizeof(Namval_t));
	if(!sh.bltin_nodes)
		sh.bltin_nodes = np;
	else if(name_vals==(const struct shtable2*)shtab_builtins)
		sh.bltin_cmds = np;
#ifndef xxx
	treep = hashalloc(sh.var_tree,HASH_set,HASH_BUCKET,HASH_name,"vars",sh.var_tree?0:HASH_free,nv_unset,0);
	treep->root->flags |= HASH_BUCKET;
#else
	treep = hashalloc(sh.var_tree,HASH_name,"vars",sh.var_tree?0:HASH_unset,nv_unset,0);
#endif
	for(tp=name_vals;*tp->sh_name;tp++,np++)
	{
		np->name = (char*)tp->sh_name;
		np->nvenv = 0;
		if(name_vals==(const struct shtable2*)shtab_builtins)
		{
			np->nvalue.bfp = ((struct shtable3*)tp)->sh_value;
			if(*np->name=='/')
			{
				np->nvenv = np->name;
				np->name = path_basename(np->name);
			}
		}
		else
			np->nvalue.cp = (char*)tp->sh_value;
		nv_setattr(np,tp->sh_number);
		if(nv_isattr(np,NV_INTEGER))
			nv_setsize(np,10);
		else
			nv_setsize(np,0);
		nv_link(treep,np);
	}
	hashset(treep,HASH_ALLOCATE);
	return(treep);
}

/*
 * read in the process environment and set up name-value pairs
 * skip over items that are not name-value pairs
 */

static void env_init(void)
{
	register char *cp;
	register Namval_t	*np;
	register char **ep=environ;
	register char *next=0;
	if(ep)
	{
		while(cp= *ep++)
		{
			if(*cp=='A' && cp[1]=='_' && cp[2]=='_' && cp[3]=='z' && cp[4]=='=')
				next = cp+4;
			else if(np=nv_open(cp,sh.var_tree,(NV_EXPORT|NV_IDENT|NV_ASSIGN))) 
			{
				nv_onattr(np,NV_IMPORT);
				np->nvenv = cp;
				nv_close(np);
			}
		}
		while(cp=next)
		{
			if(next = strchr(++cp,'='))
				*next = 0;
			np = nv_search(cp+2,sh.var_tree,NV_ADD);
			if(nv_isattr(np,NV_IMPORT|NV_EXPORT))
			{
                                nv_newattr(np,*(unsigned char*)cp-' '|NV_IMPORT|NV_EXPORT,*(unsigned char*)(cp+1)-' ');
			}
		}
	}
	if(nv_isattr(PWDNOD,NV_TAGGED))
	{
		nv_offattr(PWDNOD,NV_TAGGED);
		path_pwd(0);
	}
	if(cp = nv_getval(SHELLNOD))
	{
		cp = path_basename(cp);
		if(strmatch(cp,rsh_pattern))
			sh_onoption(SH_RESTRICTED); /* restricted shell */
	}
	return;
}

/*
 * terminate shell and free up the space
 */
int sh_term(void)
{
	sfdisc(sfstdin,SF_POPDISC);
	free((char*)sh.outbuff);
	stakset(NIL(char*),0);
	return(0);
}

