#ident	"@(#)ksh93:src/cmd/ksh93/sh/args.c	1.2"
#pragma prototyped
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Rewritten by David Korn
 * AT&T Bell Laboratories
 *
 */

#include	"defs.h"
#include	"path.h"
#include	"builtins.h"
#include	"terminal.h"
#ifdef SHOPT_KIA
#   include	"shlex.h"
#   include	"io.h"
#endif /* SHOPT_KIA */

extern char	*gettxt();

#define NUM_OPTS	20
#define SORT		1
#define PRINT		2

static int 		arg_expand(struct argnod*,struct argnod**);
static void		print_opts(Shopt_t,int);

#ifdef SHOPT_KIA
    static	char	*kiafile;
#endif /* SHOPT_KIA */
static char		*null;
static struct dolnod	*argfor; /* linked list of blocks to be cleaned up */
static struct dolnod	*dolh;
/* The following order is determined by sh_optset */
static const Shopt_t flagval[]  =
{
	0, SH_DICTIONARY|SH_NOEXEC, SH_INTERACTIVE, SH_RESTRICTED, SH_CFLAG,
	SH_ALLEXPORT, SH_NOTIFY, SH_ERREXIT, SH_NOGLOB, SH_TRACKALL,
	SH_KEYWORD, SH_MONITOR, SH_NOEXEC, SH_PRIVILEGED, SH_SFLAG, SH_TFLAG,
	SH_NOUNSET, SH_VERBOSE,  SH_XTRACE, SH_NOCLOBBER,  0 
};
static char flagadr[NUM_OPTS+1];

/* ======== option handling	======== */

/*
 *  This routine turns options on and off
 *  The options "Micr" are illegal from set command.
 *  The -o option is used to set option by name
 *  This routine returns the number of non-option arguments
 */
int sh_argopts(int argc,register char *argv[])
{
	register int n;
	register Shopt_t newflags=sh.options, opt;
	int setflag=0, action=0, trace=(int)sh_isoption(SH_XTRACE);
	Namval_t *np = NIL(Namval_t*);
	const char *cp;
	int verbose;
	if(argc>0)
		setflag = 4;
	else
		argc = -argc;
	while((n = optget(argv,setflag?gettxt(sh_optset_id,sh_optset):gettxt(sh_optksh_id,sh_optksh))))
	{
		switch(n)
		{
	 	    case 'A':
			np = nv_open(opt_arg,sh.var_tree,NV_NOASSIGN|NV_ARRAY|NV_VARNAME);
			if(*opt_option=='-')
				nv_unset(np);
			continue;
		    case 'o':
			if(!opt_arg)
			{
				action = PRINT;
				verbose = (*opt_option=='-');
				continue;
			}
			n = sh_lookup(opt_arg,shtab_options);
			opt = 1L<<n;
			if(n<=0 || (setflag && (opt&(SH_INTERACTIVE|SH_RESTRICTED))))
			{
				error(2, gettxt(e_option_id,e_option), opt_arg);
				error_info.errors++;
			}
			break;
		    case 's':
			if(setflag)
			{
				action = SORT;
				continue;
			}
#ifdef SHOPT_KIA
			goto skip;
		    case 'R':
			if(setflag)
				n = ':';
			else
			{
				kiafile = opt_arg;
				n = 'n';
			}
			/* FALL THRU */
		    skip:
#endif /* SHOPT_KIA */

		    default:
			if(cp=strchr(sh_optksh,n))
				opt = flagval[cp-sh_optksh];
			break;
		    case ':':
			error(2, opt_arg);
			continue;
		    case '?':
			error(ERROR_usage(2), opt_arg);
			return(-1);
		}
		if(*opt_option=='-')
		{
			if(opt&(SH_VI|SH_EMACS|SH_GMACS))
				newflags &= ~(SH_VI|SH_EMACS|SH_GMACS);
			newflags |= opt;
		}
		else
		{
			if(opt==SH_XTRACE)
				trace = 0;
			newflags &= ~opt;
		}
	}
	if(error_info.errors)
		error(ERROR_usage(2),optusage(NIL(char*)));
	/* check for '-' or '+' argument */
	if((cp=argv[opt_index]) && cp[1]==0 && (*cp=='+' || *cp=='-') &&
		strcmp(argv[opt_index-1],"--"))
	{
		opt_index++;
		newflags &= ~(SH_XTRACE|SH_VERBOSE);
		trace = 0;
	}
	if(trace)
		sh_trace(argv,1);
	argc -= opt_index;
	argv += opt_index;
	/* cannot set -n for interactive shells since there is no way out */
	if(sh_isoption(SH_INTERACTIVE))
		newflags &= ~SH_NOEXEC;
	if(action==PRINT)
		print_opts(newflags,verbose);
	if(setflag)
	{
		if(action==SORT)
		{
			if(argc>0)
				strsort(argv,argc,strcoll);
			else
				strsort(sh.st.dolv+1,sh.st.dolc,strcoll);
		}
		if((newflags&SH_PRIVILEGED) && !sh_isoption(SH_PRIVILEGED))
		{
			if((sh.userid!=sh.euserid && setuid(sh.euserid)<0) ||
				(sh.groupid!=sh.egroupid && setgid(sh.egroupid)<0) ||
				(sh.userid==sh.euserid && sh.groupid==sh.egroupid))
				newflags &= ~SH_PRIVILEGED;
		}
		else if(!(newflags&SH_PRIVILEGED) && sh_isoption(SH_PRIVILEGED))
		{
			setuid(sh.userid);
			setgid(sh.groupid);
			if(sh.euserid==0)
			{
				sh.euserid = sh.userid;
				sh.egroupid = sh.groupid;
			}
		}
		if(np)
		{
			nv_setvec(np,argc,argv);
			nv_close(np);
		}
		else if(argc>0 || ((cp=argv[-1]) && strcmp(cp,"--")==0))
			sh_argset(argv-1);
	}
	else if(newflags&SH_CFLAG)
	{
		if(!(sh.comdiv = *argv++))
		{
			error(2,gettxt(e_cneedsarg_id,e_cneedsarg));
			error(ERROR_usage(2),optusage(NIL(char*)));
		}
		argc--;
	}
	sh.options = newflags;
#ifdef SHOPT_KIA
	if(kiafile)
	{
		if(!(shlex.kiafile=sfopen(NIL(Sfio_t*),kiafile,"w+")))
			error(ERROR_system(3),gettxt(e_create_id,e_create),kiafile);
		if(!(shlex.kiatmp=sftmp(2*SF_BUFSIZE)))
			error(ERROR_system(3),gettxt(e_tmpcreate_id,e_tmpcreate));
		shlex.kiabegin = sftell(shlex.kiafile);
		shlex.entity_tree = hashalloc(sh.var_tree,HASH_set,HASH_ALLOCATE,0);
		shlex.scriptname = strdup(sh_fmtq(argv[0]));
		shlex.script=kiaentity(shlex.scriptname,-1,'p',-1,0,0,'s',0,"");
		shlex.fscript=kiaentity(shlex.scriptname,-1,'f',-1,0,0,'s',0,"");
		shlex.unknown=kiaentity("<unknown>",-1,'p',-1,0,0,'0',0,"");
		kiaentity("<unknown>",-1,'p',0,0,shlex.unknown,'0',0,"");
		shlex.current = shlex.script;
		kiafile = 0;
	}
#endif /* SHOPT_KIA */
	return(argc);
}

/*
 * returns the value of $-
 */
char *sh_argdolminus(void)
{
	register const char *cp=sh_optksh;
	register char *flagp=flagadr;
	while(cp< &sh_optksh[NUM_OPTS])
	{
		if(sh.options&flagval[cp-sh_optksh])
			*flagp++ = *cp;
		cp++;
	}
	*flagp = 0;
	return(flagadr);
}

/*
 * set up positional parameters 
 */
void sh_argset(char *argv[])
{
	sh_argfree(dolh,0);
	dolh = sh_argcreate(argv);
	/* link into chain */
	dolh->dolnxt = argfor;
	argfor = dolh;
	sh.st.dolc = dolh->dolnum-1;
	sh.st.dolv = dolh->dolval;
}

/*
 * free the argument list if the use count is 1
 * If count is greater than 1 decrement count and return same blk
 * Free the argument list if the use count is 1 and return next blk
 * Delete the blk from the argfor chain
 * If flag is set, then the block dolh is not freed
 */
struct dolnod *sh_argfree(struct dolnod *blk,int flag)
{
	register struct dolnod*	argr=blk;
	register struct dolnod*	argblk;
	if(argblk=argr)
	{
		if((--argblk->dolrefcnt)==0)
		{
			argr = argblk->dolnxt;
			if(flag && argblk==dolh)
				dolh->dolrefcnt = 1;
			else
			{
				/* delete from chain */
				if(argfor == argblk)
					argfor = argblk->dolnxt;
				else
				{
					for(argr=argfor;argr;argr=argr->dolnxt)
						if(argr->dolnxt==argblk)
							break;
					if(!argr)
						return(NIL(struct dolnod*));
					argr->dolnxt = argblk->dolnxt;
					argr = argblk->dolnxt;
				}
				free((void*)argblk);
			}
		}
	}
	return(argr);
}

/*
 * grab space for arglist and copy args
 * The strings are copied after the argment vector
 */
struct dolnod *sh_argcreate(register char *argv[])
{
	register struct dolnod *dp;
	register char **pp=argv, *sp;
	register int 	size=0,n;
	/* count args and number of bytes of arglist */
	while(sp= *pp++)
		size += strlen(sp);
	n = (pp - argv)-1;
	dp=new_of(struct dolnod,n*sizeof(char*)+size+n);
	dp->dolrefcnt=1;	/* use count */
	dp->dolnum = n;
	dp->dolnxt = 0;
	pp = dp->dolval;
	sp = (char*)dp + sizeof(struct dolnod) + n*sizeof(char*);
	while(n--)
	{
		*pp++ = sp;
		sp = strcopy(sp, *argv++) + 1;
	}
	*pp = NIL(char*);
	return(dp);
}

/*
 *  used to set new arguments for functions
 */
struct dolnod *sh_argnew(char *argi[], struct dolnod **savargfor)
{
	register struct dolnod *olddolh = dolh;
	*savargfor = argfor;
	dolh = 0;
	argfor = 0;
	sh_argset(argi);
	return(olddolh);
}

/*
 * reset arguments as they were before function
 */
void sh_argreset(struct dolnod *blk, struct dolnod *afor)
{
	while(argfor=sh_argfree(argfor,0));
	argfor = afor;
	if(dolh = blk)
	{
		sh.st.dolc = dolh->dolnum-1;
		sh.st.dolv = dolh->dolval;
	}
}

/*
 * increase the use count so that an sh_argset will not make it go away
 */
struct dolnod *sh_arguse(void)
{
	register struct dolnod *dh;
	if(dh=dolh)
		dh->dolrefcnt++;
	return(dh);
}

/*
 *  Print option settings on standard output
 *  if mode==1 for -o format, otherwise +o format
 */
static void print_opts(Shopt_t oflags,register int mode)
{
	register const Shtable_t *tp = shtab_options;
	Shopt_t value;
	if(mode)
		sfputr(sfstdout,gettxt(e_heading_id,e_heading),-1);
	else
		sfwrite(sfstdout,"set",3);
	while(value=tp->sh_number)
	{
		value = 1L<<value;
		if(mode)
		{
			char const *msg, *msg_id;
			sfputr(sfstdout,tp->sh_name,' ');
			sfnputc(sfstdout,' ',16-strlen(tp->sh_name));
			if(oflags&value) {
				msg = e_on;
				msg_id = e_on_id;
			} else {
				msg = e_off;
				msg_id = e_off_id;
			}
			sfputr(sfstdout,ERROR_translate(gettxt(msg_id,msg),1),'\n');
		}
		else if(oflags&value)
			sfprintf(sfstdout," -o %s",tp->sh_name);
		tp++;
	}
	if(!mode)
		sfputc(sfstdout,'\n');
}

/*
 * build an argument list
 */
char **sh_argbuild(int *nargs, const struct comnod *comptr)
{
	register struct argnod	*argp;
	struct argnod *arghead=0;
	{
		register const struct comnod	*ac = comptr;
		/* see if the arguments have already been expanded */
		if(!ac->comarg)
		{
			*nargs = 0;
			return(&null);
		}
		else if(!(ac->comtyp&COMSCAN))
		{
			register struct dolnod *ap = (struct dolnod*)ac->comarg;
			*nargs = ap->dolnum;
			return(ap->dolval+ap->dolbot);
		}
		sh.lastpath = 0;
		*nargs = 0;
		if(ac)
		{
			argp = ac->comarg;
			while(argp)
			{
				*nargs += arg_expand(argp,&arghead);
				argp = argp->argnxt.ap;
			}
			argp = arghead;
		}
	}
	{
		register char	**comargn;
		register int	argn;
		register char	**comargm;
		argn = *nargs;
		/* allow room to prepend args */
#ifdef SHOPT_VPIX
		argn += 2;
#else
		argn += 1;
#endif /* SHOPT_VPIX */

		comargn=(char**)stakalloc((unsigned)(argn+1)*sizeof(char*));
		comargm = comargn += argn;
		*comargn = NIL(char*);
		if(!argp)
		{
			/* reserve an extra null pointer */
			*--comargn = 0;
			return(comargn);
		}
		while(argp)
		{
			struct argnod *nextarg = argp->argchn.ap;
			argp->argchn.ap = 0;
			*--comargn = argp->argval;
			if(!(argp->argflag&ARG_RAW))
				sh_trim(*comargn);
			if(!(argp=nextarg) || (argp->argflag&ARG_MAKE))
			{
				if((argn=comargm-comargn)>1)
					strsort(comargn,argn,strcoll);
				comargm = comargn;
			}
		}
		return(comargn);
	}
}

/* Argument expansion */
static int arg_expand(register struct argnod *argp, struct argnod **argchain)
{
	register int count = 0;
	argp->argflag &= ~ARG_MAKE;
#ifdef SHOPT_DEVFD
	if(*argp->argval==0 && (argp->argflag&ARG_EXP))
	{
		/* argument of the form (cmd) */
		register struct argnod *ap;
		int monitor, fd, pv[2];
		ap = (struct argnod*)stakseek(ARGVAL);
		ap->argflag |= ARG_MAKE;
		ap->argflag &= ~ARG_RAW;
		ap->argchn.ap = *argchain;
		*argchain = ap;
		count++;
		stakwrite(e_devfdNN,8);
		sh_pipe(pv);
		fd = argp->argflag&ARG_RAW;
		stakputs(fmtbase((long)pv[fd],10,0));
		ap = (struct argnod*)stakfreeze(1);
		sh.inpipe = sh.outpipe = 0;
		if(monitor = (sh_isstate(SH_MONITOR)!=0))
			sh_offstate(SH_MONITOR);
		if(fd)
		{
			sh.inpipe = pv;
			sh_exec((union anynode*)argp->argchn.ap,(int)sh_isstate(SH_ERREXIT));
		}
		else
		{
			sh.outpipe = pv;
			sh_exec((union anynode*)argp->argchn.ap,(int)sh_isstate(SH_ERREXIT));
		}
		if(monitor)
			sh_onstate(SH_MONITOR);
		close(pv[1-fd]);
		sh_iosave(-pv[fd], sh.topfd);
	}
	else
#endif	/* SHOPT_DEVFD */
	if(!(argp->argflag&ARG_RAW))
		count = sh_macexpand(argp,argchain);
	else
	{
		argp->argchn.ap = *argchain;
		*argchain = argp;
		argp->argflag |= ARG_MAKE;
		count++;
	}
	return(count);
}

