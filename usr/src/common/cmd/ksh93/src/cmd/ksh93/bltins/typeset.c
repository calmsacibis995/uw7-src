#ident	"@(#)ksh93:src/cmd/ksh93/bltins/typeset.c	1.2"
#pragma prototyped
/*
 * export [-p] [arg...]
 * readonly [-p] [arg...]
 * typeset [options]  [arg...]
 * alias [-ptx] [arg...]
 * unalias [arg...]
 * builtin [-sd] [-f file] [name...]
 * set [options] [name...]
 * unset [-fnv] [name...]
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 2B-102
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *   research!dgk
 *
 */

#include	"defs.h"
#include	<error.h>
#include	"path.h"
#include	"name.h"
#include	"history.h"
#include	"builtins.h"
#include	"variables.h"

extern char	*gettxt();

/* use these prototyped because dlhdr.h may be wrong */
extern void	*dlopen(const char*,int);
extern void	*dlsym(void*, const char*);
extern char	*dlerror(void);
#define DL_MODE	1

static int	print_namval(Sfio_t*, Namval_t*, int);
static void	print_attribute(Namval_t*);
static void	print_all(Sfio_t*, Hashtab_t*);
static void	print_scan(Sfio_t*, int, Hashtab_t*, int);
static int	b_unall(int, char**, Hashtab_t*);
static int	b_common(char**, int, Hashtab_t*);
static void	pushname(Namval_t*);
static void	unref(Namval_t*);
static char	**argnam;
static int	argnum;
static int 	aflag;
static int	scanmask;
static Hashtab_t *scanroot;
static Sfio_t	*outfile;
static void(*nullscan)(Namval_t*);
static char	*prefix;

static char *get_tree(Namval_t*, Namfun_t *);
static void put_tree(Namval_t*, const char*, int,Namfun_t*);

static const Namdisc_t treedisc =
{
	0,
	put_tree,
	get_tree
};


/*
 * The following few builtins are provided to set,print,
 * and test attributes and variables for shell variables,
 * aliases, and functions.
 * In addition, typeset -f can be used to test whether a
 * function has been defined or to list all defined functions
 * Note readonly is same as typeset -r.
 * Note export is same as typeset -x.
 */

int    b_read_export(int argc,char *argv[],void *extra)
{
	register int flag;
	char *command = argv[0];
	NOT_USED(argc);
	NOT_USED(extra);
	aflag = '-';
	argnum = 0;
	prefix = 0;
	while((flag = optget(argv,(const char *)gettxt(sh_optexport_id,sh_optexport)))) switch(flag)
	{
		case 'p':
			prefix = command;
			break;
		case ':':
			error(2, opt_arg);
			break;
		case '?':
			error(ERROR_usage(0), opt_arg);
			return(2);
	}
	if(error_info.errors)
		error(ERROR_usage(2),optusage(NIL(char*)));
	argv += (opt_index-1);
	if(*command=='r')
		flag = (NV_ASSIGN|NV_RDONLY|NV_VARNAME);
	else
		flag = (NV_ASSIGN|NV_EXPORT|NV_IDENT);
	if(*argv)
		return(b_common(argv,flag,sh.var_tree));
	print_scan(sfstdout,flag,sh.var_tree,aflag=='+');
	return(0);
}


int    b_alias(int argc,register char *argv[],void *extra)
{
	register unsigned flag = NV_ARRAY|NV_NOSCOPE|NV_ASSIGN;
	register Hashtab_t *troot = sh.alias_tree;
	register int n;
	NOT_USED(argc);
	NOT_USED(extra);
	prefix=0;
	if(*argv[0]=='h')
		flag = NV_TAGGED;
	if(argv[1])
	{
		opt_char = 0;
		opt_index = 1;
		*opt_option = 0;
		argnum = 0;
		aflag = *argv[1];
		while((n = optget(argv,(const char *)gettxt(sh_optalias_id, sh_optalias)))) switch(n)
		{
		    case 'p':
			prefix = argv[0];
			break;
		    case 't':
			flag |= NV_TAGGED;
			break;
		    case 'x':
			flag |= NV_EXPORT;
			break;
		    case ':':
			error(2, opt_arg);
			break;
		    case '?':
			error(ERROR_usage(0), opt_arg);
			return(2);
		}
		if(error_info.errors)
			error(ERROR_usage(2),optusage(NIL(char*)));
		argv += (opt_index-1);
		if(flag&NV_TAGGED)
		{
			if(argv[1] && strcmp(argv[1],"-r")==0)
			{
				/* hack to handle hash -r */
				nv_putval(PATHNOD,nv_getval(PATHNOD),NV_RDONLY);
				return(0);
			}
			troot = sh.track_tree;
		}
	}
	return(b_common(argv,flag,troot));
}


int    b_typeset(int argc,register char *argv[],void *extra)
{
	register int flag = NV_VARNAME|NV_ASSIGN;
	register int n;
	Hashtab_t *troot = sh.var_tree;
	int isfloat = 0;
	NOT_USED(argc);
	NOT_USED(extra);
	aflag = argnum  = 0;
	prefix = 0;
	while((n = optget(argv,(const char *)gettxt(sh_opttypeset_id,sh_opttypeset))))
	{
		switch(n)
		{
			case 'A':
				flag |= NV_ARRAY;
				break;
			case 'E':
				/* The following is for ksh88 compatibility */
				if(opt_char && !strchr(argv[opt_index],'E'))
				{
					argnum = (int)opt_num;
					break;
				}
			case 'F':
				if(!opt_arg || (argnum = opt_num) <0)
					argnum = 10;
				isfloat = 1;
				if(n=='E')
					flag |= NV_EXPNOTE;
				break;
			case 'n':
				flag &= ~NV_VARNAME;
				flag |= (NV_REF|NV_IDENT);
				break;
			case 'H':
				flag |= NV_HOST;
				break;
#ifdef SHOPT_OO
			case 'C':
				prefix = opt_arg;
				flag |= NV_IMPORT;
				break;
#endif /* SHOPT_OO */
			case 'L':
				if(argnum==0)
					argnum = (int)opt_num;
				if(argnum < 0)
					error(ERROR_exit(1), gettxt(e_badfield_id, e_badfield), argnum);
				flag &= ~NV_RJUST;
				flag |= NV_LJUST;
				break;
			case 'Z':
				flag |= NV_ZFILL;
				/* FALL THRU*/
			case 'R':
				if(argnum==0)
					argnum = (int)opt_num;
				if(argnum < 0)
					error(ERROR_exit(1), gettxt(e_badfield_id, e_badfield), argnum);
				flag &= ~NV_LJUST;
				flag |= NV_RJUST;
				break;
			case 'f':
				flag &= ~(NV_VARNAME|NV_ASSIGN);
				troot = sh.fun_tree;
				break;
			case 'i':
				if(!opt_arg || (argnum = opt_num) <0)
					argnum = 1;
				flag |= NV_INTEGER;
				break;
			case 'l':
				flag |= NV_UTOL;
				break;
			case 'p':
				prefix = argv[0];
				continue;
			case 'r':
				flag |= NV_RDONLY;
				break;
			case 't':
				flag |= NV_TAGGED;
				break;
			case 'u':
				flag |= NV_LTOU;
				break;
			case 'x':
				flag &= ~NV_VARNAME;
				flag |= (NV_EXPORT|NV_IDENT);
				break;
			case ':':
				error(2, opt_arg);
				break;
			case '?':
				error(ERROR_usage(0), opt_arg);
				return(2);
		}
		if(aflag==0)
			aflag = *opt_option;
	}
	argv += opt_index;
	/* handle argument of + and - specially */
	if(*argv && argv[0][1]==0 && (*argv[0]=='+' || *argv[0]=='-'))
		aflag = *argv[0];
	else
		argv--;
	if((flag&NV_INTEGER) && (flag&(NV_LJUST|NV_RJUST|NV_ZFILL)))
		error_info.errors++;
	if(troot==sh.fun_tree && ((isfloat || flag&~(NV_FUNCT|NV_TAGGED|NV_EXPORT|NV_LTOU))))
		error_info.errors++;
	if(error_info.errors)
		error(ERROR_usage(2),optusage(NIL(char*)));
	if(isfloat)
		flag |= NV_INTEGER|NV_DOUBLE;
	if(sh.fn_depth)
		flag |= NV_NOSCOPE;
	return(b_common(argv,flag,troot));
}

static int     b_common(char **argv,register int flag,Hashtab_t *troot)
{
	register char *name;
	int nvflags=(flag&(NV_ARRAY|NV_NOSCOPE|NV_VARNAME|NV_IDENT|NV_ASSIGN));
	int r=0, ref=0;
#ifdef SHOPT_OO
	char *base=0;
	if(flag&NV_IMPORT)
	{
		if(!argv[1])
			error(ERROR_exit(1),gettxt(":105", "requires arguments"));
		if(aflag!='-')
			error(ERROR_exit(1),gettxt(":106", "cannot be unset"));
		base = prefix;
		prefix = 0;
		flag &= ~(NV_IMPORT|NV_ASSIGN);
	}
#endif /* SHOPT_OO */
	flag &= ~(NV_ARRAY|NV_NOSCOPE|NV_VARNAME|NV_IDENT);
	if(argv[1])
	{
		if(flag&NV_REF)
		{
			flag &= ~NV_REF;
			ref=1;
			if(aflag!='-')
				nvflags |= NV_REF;
		}
		while(name = *++argv)
		{
			register unsigned newflag;
			register Namval_t *np;
			unsigned curflag;
			if(troot == sh.fun_tree)
			{
				/*
				 *functions can be exported or
				 * traced but not set
				 */
				flag &= ~NV_ASSIGN;
				if(flag&NV_LTOU)
				{
					/* Function names cannot be special builtin */
					if((np=nv_search(name,sh.bltin_tree,0)) && nv_isattr(np,BLT_SPC))
						error(ERROR_exit(1),gettxt(e_badfun_id,e_badfun),name);
					np = nv_open(name,sh.fun_tree,NV_ARRAY|NV_IDENT|NV_NOSCOPE);
				}
				else
					np = nv_search(name,sh.fun_tree,HASH_NOSCOPE);
				if(np && ((flag&NV_LTOU) || !nv_isnull(np) || nv_isattr(np,NV_LTOU)))
				{
					if(flag==0)
					{
						print_namval(sfstdout,np,0);
						continue;
					}
					if(sh.subshell)
						sh_subfork();
					if(aflag=='-')
						nv_onattr(np,flag|NV_FUNCTION);
					else if(aflag=='+')
						nv_offattr(np,flag);
				}
				else
					r++;
				continue;
			}
			np = nv_open(name,troot,nvflags);
			/* tracked alias */
			if(troot==sh.track_tree && aflag=='-')
			{
				nv_onattr(np,NV_NOALIAS);
				path_alias(np,path_absolute(nv_name(np),NIL(char*)));
				continue;
			}
			if(flag==NV_ASSIGN && !ref && aflag!='-' && !strchr(name,'='))
			{
				if(troot!=sh.var_tree && (nv_isnull(np) || !print_namval(sfstdout,np,0)))
				{
					sfprintf(sfstderr,gettxt(e_noalias_id,e_noalias),name);
					r++;
				}
				continue;
			}
			if(troot==sh.var_tree && (nvflags&NV_ARRAY))
				nv_setarray(np,nv_associative);
#ifdef SHOPT_OO
			if(base)
			{
				Namval_t *nq, *nr;
				nv_offattr(np,NV_IMPORT);
				if(!(nq=nv_search(base,troot,0)))
					error(ERROR_exit(1),gettxt(e_badbase_id,e_badbase),base);
				/* check for loop */
				for(nr=nq; nr; nr = nv_class(nr))
				{
					if(nr==np)
						error(ERROR_exit(1),gettxt(e_loop_id,e_loop),base);
				}
				np->nvenv = (char*)nq;
				if(nq->nvfun && (nq->nvfun)->disc->create)
					(*(nq->nvfun)->disc->create)(np,0,nq->nvfun);
			}
#endif /* SHOPT_OO */
			curflag = np->nvflag;
			flag &= ~NV_ASSIGN;
			if (aflag == '-')
			{
				if((flag&NV_EXPORT) && strchr(nv_name(np),'.'))
					error(ERROR_exit(1),gettxt(e_badexport_id,e_badexport),nv_name(np));
#ifdef SHOPT_BSH
				if(flag&NV_EXPORT)
					nv_offattr(np,NV_IMPORT);
#endif /* SHOPT_BSH */
				newflag = curflag;
				if(flag&~NV_NOCHANGE)
					newflag &= NV_NOCHANGE;
				newflag |= flag;
				if (flag & (NV_LJUST|NV_RJUST))
				{
					if(!(flag&NV_RJUST))
						newflag &= ~NV_RJUST;
					else if(!(flag&NV_LJUST))
						newflag &= ~NV_LJUST;
				}
				if (flag & NV_UTOL)
					newflag &= ~NV_LTOU;
				else if (flag & NV_LTOU)
					newflag &= ~NV_UTOL;
			}
			else
			{
				if((flag&NV_RDONLY) && (curflag&NV_RDONLY))
					error(ERROR_exit(1),gettxt(e_readonly_id,e_readonly),nv_name(np));
				newflag = curflag & ~flag;
			}
			if (aflag && (argnum>0 || (curflag!=newflag)))
			{
				if(sh.subshell)
					sh_assignok(np,1);
				if(troot!=sh.var_tree)
					nv_setattr(np,newflag);
				else
					nv_newattr (np, newflag,argnum);
			}
			/* set or unset references */
			if(ref)
			{
				if(aflag=='-')
					nv_setref(np);
				else
					unref(np);
			}
			nv_close(np);
		}
	}
	else
	{
		if(aflag)
		{
			if(troot==sh.fun_tree)
			{
				flag |= NV_FUNCTION;
				prefix = 0;
			}
			else if(troot==sh.var_tree)
				flag |= (nvflags&NV_ARRAY);
			print_scan(sfstdout,flag,troot,aflag=='+');
		}
		else if(troot==sh.alias_tree)
			print_scan(sfstdout,0,troot,0);
		else
			print_all(sfstdout,troot);
	}
	return(r);
}

typedef int (*Fptr_t)(int, char*[], void*);

#define GROWLIB	4
static void **liblist;

/*
 * This allows external routines to load from the same library */
void **sh_getliblist(void)
{
	return(liblist);
}

/*
 * add change or list built-ins
 * adding builtins requires dlopen() interface
 */
int	b_builtin(int argc,char *argv[],void *extra)
{
	register char *arg=0, *name;
	register int n, r=0, flag=0;
	register Namval_t *np;
	int dlete=0;
	static int maxlib, nlib;
	Fptr_t addr;
	void *library=0;
	char *errmsg;
	NOT_USED(argc);
	NOT_USED(extra);
	while((n = optget(argv,(const char *)gettxt(sh_optbuiltin_id, sh_optbuiltin)))) switch(n)
	{
	    case 's':
		flag = BLT_SPC;
		break;
	    case 'd':
		dlete=1;
		break;
	    case 'f':
#ifdef SHOPT_DYNAMIC
		arg = opt_arg;
#else
		error(2, gettxt(":107", "adding built-ins not supported"));
		error_info.errors++;
#endif /* SHOPT_DYNAMIC */
		break;
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_arg);
		break;
	}
	argv += opt_index;
	if(error_info.errors)
		error(ERROR_usage(2),optusage(NIL(char*)));
	if(arg || *argv)
	{
		if(sh_isoption(SH_RESTRICTED))
			error(ERROR_exit(1),gettxt(e_restricted_id,e_restricted),argv[-opt_index]);
		if(sh.subshell)
			sh_subfork();
	}
	if(arg)
	{
		if(!(library = dlopen(arg,DL_MODE)))
		{
			error(ERROR_exit(0),gettxt(":108","%s: %s"),arg,dlerror());
			return(1);
		}
		/* 
		 * see if library is already on search list
		 * if it is, move to head of search list
		 */
		for(n=r=0; n < nlib; n++)
		{
			if(r)
				liblist[n-1] = liblist[n];
			else if(liblist[n]==library)
				r++;
		}
		if(r)
			nlib--;
		else
		{
			typedef void (*Iptr_t)(int);
			Iptr_t initfn;
			if((initfn = (Iptr_t)dlsym(library,"lib_init")))
				(*initfn)(0);
		}
		if(nlib >= maxlib)
		{
			/* add library to search list */
			maxlib += GROWLIB;
			if(nlib>0)
				liblist = (void**)realloc((void*)liblist,(maxlib+1)*sizeof(void**));
			else
				liblist = (void**)malloc((maxlib+1)*sizeof(void**));
		}
		liblist[nlib++] = library;
		liblist[nlib] = 0;
	}
	else if(*argv==0 && !dlete)
	{
		print_scan(sfstdout, flag, sh.bltin_tree, 1);
		return(0);
	}
	r = 0;
	flag = staktell();
	while(arg = *argv)
	{
		name = path_basename(arg);
		stakputs("b_");
		stakputs(name);
		errmsg = 0;
		addr = 0;
		for(n=(nlib?nlib:dlete); --n>=0;)
		{
			/* (char*) added for some sgi-mips compilers */ 
			if(dlete || (addr = (Fptr_t)dlsym(liblist[n],stakptr(flag))))
			{
				if(sh_addbuiltin(arg, addr,0) < 0)
					errmsg = "restricted name";
				else
					errmsg = 0;
				break;
			}
		}
		if(!dlete && !addr)
		{
			if(np=nv_search(name,sh.bltin_tree,0))
			{
				if(sh_addbuiltin(arg, np->nvalue.bfp,0) < 0)
					errmsg = "restricted name";
			}
			else
				errmsg = "not found";
		}
		if(errmsg)
		{
					/* ???? */
			error(ERROR_exit(0),gettxt(":108","%s: %s"),*argv,errmsg);
			r = 1;
		}
		stakseek(flag);
		argv++;
	}
	return(r);
}

int    b_set(int argc,register char *argv[],void *extra)
{
	NOT_USED(extra);
	prefix=0;
	if(argv[1])
	{
		if(sh_argopts(argc,argv) < 0)
			return(2);
		sh_offstate(SH_VERBOSE|SH_MONITOR);
		sh_onstate(sh_isoption(SH_VERBOSE|SH_MONITOR));
	}
	else
		/*scan name chain and print*/
		print_scan(sfstdout,0,sh.var_tree,0);
	return(0);
}

/*
 * The removing of Shell variable names, aliases, and functions
 * is performed here.
 * Unset functions with unset -f
 * Non-existent items being deleted give non-zero exit status
 */

int    b_unalias(int argc,register char *argv[],void *extra)
{
	NOT_USED(extra);
	return(b_unall(argc,argv,sh.alias_tree));
}

int    b_unset(int argc,register char *argv[],void *extra)
{
	NOT_USED(extra);
	return(b_unall(argc,argv,sh.var_tree));
}

static int b_unall(int argc, char **argv, register Hashtab_t *troot)
{
	register Namval_t *np;
	register struct slnod *slp;
	register const char *name;
	register int r;
	int nflag = 0;
	int all=0;
	NOT_USED(argc);
	if(troot!=sh.var_tree && sh.subshell)
		sh_subfork();
	if(troot==sh.alias_tree)
		name = gettxt(sh_optunalias_id,sh_optunalias);
	else
		name = gettxt(sh_optunset_id,sh_optunset);
	while(r = optget(argv,name)) switch(r)
	{
		case 'f':
			troot = sh.fun_tree;
			break;
		case 'a':
			all=1;
			break;
		case 'n':
			nflag = NV_NOREF;
		case 'v':
			troot = sh.var_tree;
			break;
		case ':':
			error(2, opt_arg);
			break;
		case '?':
			error(ERROR_usage(0), opt_arg);
			return(2);
	}
	argv += opt_index;
	if(error_info.errors || (*argv==0 &&!all))
		error(ERROR_usage(2),optusage(NIL(char*)));
	r = 0;
	if(troot==sh.var_tree)
		nflag |= NV_VARNAME;
	if(all)
	{
		hashfree(troot);
		sh.alias_tree = hashalloc(sh.var_tree,HASH_set,HASH_ALLOCATE,0);
	}
	else while(name = *argv++)
	{
		if(np=nv_open(name,troot,NV_NOADD|nflag))
		{
			if(troot!=sh.var_tree)
			{
				if(is_abuiltin(np))
				{
					r = 1;
					continue;
				}
				else if(slp=(struct slnod*)(np->nvenv))
				{
					/* free function definition */
					register char *cp= strrchr(name,'.');
					if(cp)
					{
						Namval_t *npv;
						*cp = 0;
						npv = nv_open(name,sh.var_tree,NV_ARRAY|NV_VARNAME|NV_NOADD);
						*cp++ = '.';
						if(npv)
							nv_setdisc(npv,cp,NIL(Namval_t*),(Namfun_t*)npv);
					}
					stakdelete(slp->slptr);
					np->nvenv = 0;
					hashlook(troot,(char*)np,HASH_DELETE|HASH_BUCKET,(char*)0);
					continue;
				}
			}
#ifdef apollo
			else
			{
				short namlen;
				name = nv_name(np);
				namlen =strlen(name);
				ev_$delete_var(name,&namlen);
			}
#endif /* apollo */
			if(sh.subshell)
				np=sh_assignok(np,0);
			nv_unset(np);
			nv_close(np);
		}
		else
			r = 1;
	}
	return(r);
}



/*
 * print out the name and value of a name-value pair <np>
 */

static int print_namval(Sfio_t *file,register Namval_t *np,register int flag)
{
	register char *cp;
	sh_sigcheck();
	if(flag)
		flag = '\n';
	if(nv_isattr(np,NV_NOPRINT)==NV_NOPRINT)
	{
		if(is_abuiltin(np))
			sfputr(file,np->nvenv?np->nvenv:nv_name(np),'\n');
		return(0);
	}
	if(prefix)
		sfputr(file,prefix,' ');
	if(is_afunction(np))
	{
		if(!flag && !np->nvalue.ip)
			sfputr(file,"typeset -fu",' ');
		else if(!flag && !nv_isattr(np,NV_FPOSIX))
			sfputr(file,"function",' ');
		if(!np->nvalue.ip || np->nvalue.rp->hoffset<0)
			flag = '\n';
		sfputr(file,nv_name(np),flag?flag:-1);
		if(!flag)
		{
			sfputr(file,nv_isattr(np,NV_FPOSIX)?"()\n{":"\n{",'\n');
			hist_list(sh.hist_ptr,file,np->nvalue.rp->hoffset,0,"\n");
		}
		return(nv_size(np)+1);
	}
	if(cp=nv_getval(np))
	{
		register Namarr_t *ap;
		sfputr(file,sh_fmtq(nv_name(np)),-1);
		if(!flag)
		{
			flag = '=';
		        if(ap = nv_arrayptr(np))
				sfprintf(file,"[%s]", sh_fmtq(nv_getsub(np)));
		}
		sfputc(file,flag);
		if(flag != '\n')
			sfputr(file,sh_fmtq(cp),'\n');
		return(1);
	}
	else if(scanmask && scanroot==sh.var_tree)
		sfputr(file,nv_name(np),'\n');
	return(0);
}

/*
 * print attributes at all nodes
 */

static void	print_all(Sfio_t *file,Hashtab_t *root)
{
	outfile = file;
	nv_scan(root, print_attribute, 0, 0);
}

#include	<fcin.h>
#include	"argnod.h"

/*
 * format initialization list given a list of assignments <argp>
 */
static void genvalue(struct argnod *argp, const char *prefix, int n, int indent)
{
	register struct argnod *ap;
	register char *cp,*nextcp;
	register int m,isarray;
	Namval_t *np;
	if(n==0)
		m = strlen(prefix);
	else
		m = strchr(prefix,'.')-prefix;
	m++;
	if(outfile)
	{
		sfwrite(outfile,"(\n",2);
		indent++;
	}
	for(ap=argp; ap; ap=ap->argchn.ap)
	{
		if(ap->argflag==ARG_MAC)
			continue;
		cp = ap->argval+n;
		if(n==0 && cp[m-1]!='.')
		{
			ap->argflag = ARG_MAC;
			continue;
		}
		if(n && cp[m-1]==0)
			continue;
		if(n==0 || strncmp(ap->argval,prefix-n,m+n)==0)
		{
			cp +=m;
			if(nextcp=strchr(cp,'.'))
			{
				if(outfile)
				{
					sfnputc(outfile,'\t',indent);
					sfwrite(outfile,cp,nextcp-cp);
					sfputc(outfile,'=');
				}
				genvalue(argp,cp,n+m ,indent);
				if(outfile)
					sfputc(outfile,'\n');
				continue;
			}
			ap->argflag = ARG_MAC;
			if(!(np=nv_search(ap->argval,sh.var_tree,0)))
				continue;
			if(np->nvfun && np->nvfun->disc== &treedisc)
				continue;
			isarray=0;
			if(nv_isattr(np,NV_ARRAY))
			{
				isarray=1;
				if(array_elem(nv_arrayptr(np))==0)
					isarray=2;
				else
					nv_putsub(np,NIL(char*),ARRAY_SCAN);
			}
			if(!outfile)
			{
				nv_close(np);
				continue;
			}
			sfnputc(outfile,'\t',indent);
			if(nv_isattr(np,~NV_DEFAULT))
				print_attribute(np);
			sfputr(outfile,cp,(isarray==2?'\n':'='));
			if(isarray)
			{
				if (isarray==2)
					continue;
				sfwrite(outfile,"(\n",2);
				sfnputc(outfile,'\t',++indent);
			}
			while(1)
			{
				char *fmtq;
				if(isarray)
				{
					sfprintf(outfile,"[%s]",sh_fmtq(nv_getsub(np)));
					sfputc(outfile,'=');
				}
				if(!(fmtq = sh_fmtq(nv_getval(np))))
					fmtq = "";
				sfputr(outfile,fmtq,'\n');
				if(!ap || !nv_nextsub(np))
					break;
				sfnputc(outfile,'\t',indent);
			}
			if(isarray)
			{
				sfnputc(outfile,'\t',--indent);
				sfwrite(outfile,")\n",2);
			}
		}
	}
	if(outfile)
	{
		if(indent > 1)
			sfnputc(outfile,'\t',indent-1);
		sfputc(outfile,')');
	}
}

/*
 * walk the virtual tree and print or delete name-value pairs
 */
static char *walk_tree(register Namval_t *np, int dlete)
{
	static Sfio_t *out;
	int n;
	Fcin_t save;
	int savtop = staktell();
	char *savptr = stakfreeze(0);
	register struct argnod *ap; 
	struct argnod *arglist=0;
	char *name = nv_name(np);
	stakseek(ARGVAL);
	stakputs("\"${!");
	stakputs(name);
	stakputs("@}\"");
	ap = (struct argnod*)stakfreeze(1);
	ap->argflag = ARG_MAC;
	fcsave(&save);
	n = sh_macexpand(ap,&arglist);
	fcrestore(&save);
	ap = arglist;
	if(dlete)
		outfile = 0;
	else if(!(outfile=out))
		outfile = out =  sfnew((Sfio_t*)0,(char*)0,-1,-1,SF_WRITE|SF_STRING);
	else
		sfseek(outfile,0L,SEEK_SET);
	prefix = "typeset";
	aflag = '=';
	genvalue(ap,name,0,0);
	stakset(savptr,savtop);
	if(!outfile)
		return((char*)0);
	sfputc(out,0);
	return((char*)out->data);
}

/*
 * get discipline for compound initializations
 */
static char *get_tree(register Namval_t *np, Namfun_t *fp)
{
	NOT_USED(fp);
	return(walk_tree(np,0));
}

/*
 * put discipline for compound initializations
 */
static void put_tree(register Namval_t *np, const char *val, int flags,Namfun_t *fp)
{
	walk_tree(np,1);
	if(fp = nv_stack(np,NIL(Namfun_t*)))
	{
		free((void*)fp);
		if(np->nvalue.cp && !nv_isattr(np,NV_NOFREE))
			free((void*)np->nvalue.cp);
	}
	if(val)
		nv_putval(np,val,flags);
}

/*
 * Insert discipline to cause $x to print current tree
 */
void nv_setvtree(register Namval_t *np)
{
	register Namfun_t *nfp = newof(NIL(void*),Namfun_t,1,0);
	nfp->disc = &treedisc;
	nv_stack(np, nfp);
}

/*
 * print the attributes of name value pair give by <np>
 */
static void	print_attribute(register Namval_t *np)
{
	register const Shtable_t *tp;
	register char *cp;
	register unsigned val;
	register unsigned mask;
#ifdef SHOPT_OO
	Namval_t *nq;
	char *cclass=0;
#endif /* SHOPT_OO */
	if (nv_isattr(np,~NV_DEFAULT))
	{
		if(prefix)
			sfputr(outfile,prefix,' ');
		for(tp = shtab_attributes; *tp->sh_name;tp++)
		{
			val = tp->sh_number;
			mask = val;
			/*
			 * the following test is needed to prevent variables
			 * with E attribute from being given the F
			 * attribute as well
			 */
			if(val==(NV_INTEGER|NV_DOUBLE) && nv_isattr(np,NV_EXPNOTE))
				continue;
			if(val&NV_INTEGER)
				mask |= NV_DOUBLE;
			else if(val&NV_HOST)
				mask = NV_HOST;
			if(nv_isattr(np,mask)==val)
			{
				if(val==NV_ARRAY)
				{
					Namarr_t *ap = nv_arrayptr(np);
					if(array_assoc(ap))
						cp = "associative";
					else
						cp = "indexed";
					if(!prefix)
						sfputr(outfile,cp,' ');
					else if(*cp=='i')
						continue;
				}
				if(prefix)
				{
					if(*tp->sh_name=='-')
						sfprintf(outfile,"%.2s ",tp->sh_name);
				}
				else
					sfputr(outfile,tp->sh_name+2,' ');
		                if ((val&(NV_LJUST|NV_RJUST|NV_ZFILL)) && !(val&NV_INTEGER) && val!=NV_HOST)
					sfprintf(outfile,"%d ",nv_size(np));
			}
		        if(val == NV_INTEGER && nv_isattr(np,NV_INTEGER))
			{
				if(nv_size(np) != 10)
				{
					if(nv_isattr(np, NV_DOUBLE))
						cp = "precision";
					else
						cp = "base";
					if(!prefix)
						sfputr(outfile,cp,' ');
					sfprintf(outfile,"%d ",nv_size(np));
				}
				break;
			}
		}
#ifdef SHOPT_OO
		if(nq=nv_class(np))
		{
			if(prefix && *prefix=='t')
				cclass = "-C";
			else if(!prefix)
				cclass = "class";
			if(cclass)
				sfprintf(outfile,"%s %s ",cclass,nv_name(nq));
		
		}
#endif /* SHOPT_OO */
		if(aflag)
			return;
		sfputr(outfile,nv_name(np),'\n');
	}
}

/*
 * print the nodes in tree <root> which have attributes <flag> set
 * of <option> is non-zero, no subscript or value is printed.
 */

static void print_scan(Sfio_t *file, int flag, Hashtab_t *root, int option)
{
	register char **argv;
	register Namval_t *np;
	register int namec;
	Namval_t *onp = 0;
	flag &= ~NV_ASSIGN;
	scanmask = flag&~NV_NOSCOPE;
	scanroot = root;
	outfile = file;
	if(flag&NV_INTEGER)
		scanmask |= (NV_DOUBLE|NV_EXPNOTE);
	namec = nv_scan(root,nullscan,scanmask,flag);
	argv = argnam  = (char**)stakalloc((namec+1)*sizeof(char*));
	namec = nv_scan(root, pushname, scanmask, flag);
	strsort(argv,namec,strcoll);
	while(namec--)
	{
		if((np=nv_search(*argv++,root,0)) && np!=onp && (!nv_isnull(np) || np->nvfun || nv_isattr(np,~NV_NOFREE)))
		{
			onp = np;
			if((flag&NV_ARRAY) && nv_aindex(np)>=0)
				continue;
			if(!flag && nv_isattr(np,NV_ARRAY))
			{
				if(array_elem(nv_arrayptr(np))==0)
					continue;
				nv_putsub(np,NIL(char*),ARRAY_SCAN);
				do
				{
					print_namval(file,np,option);
				}
				while(!option && nv_nextsub(np));
			}
			else
				print_namval(file,np,option);
		}
	}
}

/*
 * add the name of the node to the argument list argnam
 */

static void pushname(Namval_t *np)
{
	*argnam++ = nv_name(np);
}

/*
 * The inverse of creating a reference node
 */
static void unref(register Namval_t *np)
{
	if(!nv_isattr(np,NV_REF))
		return;
	nv_offattr(np,NV_NOFREE|NV_REF);
	np->nvalue.cp = strdup(nv_name(np->nvalue.np));
	return;
}

