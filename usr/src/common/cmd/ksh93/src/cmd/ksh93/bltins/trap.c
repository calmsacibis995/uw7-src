#ident	"@(#)ksh93:src/cmd/ksh93/bltins/trap.c	1.1"
#pragma prototyped
/*
 * trap  [-p]  action sig...
 * kill  [-l] [sig...]
 * kill  [-s sig] pid...
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
#include	<ctype.h>
#include	"jobs.h"
#include	"builtins.h"

#define L_FLAG	1
#define S_FLAG	2

static const char trapfmt[] = "trap -- %s %s\n";

static int	sig_number(const char*);
static void	sig_list(int);

int	b_trap(int argc,char *argv[], void *extra)
{
	register char *arg = argv[1];
	register int sig, pflag = 0;
	NOT_USED(argc);
	NOT_USED(extra);
	while (sig = optget(argv, (const char *)gettxt(sh_opttrap_id,sh_opttrap))) switch (sig)
	{
	    case 'p':
		pflag=1;
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
		error(ERROR_usage(2),optusage((char*)0));
	if(arg = *argv)
	{
		register int	clear;
		char *action = arg;
		if(!pflag)
		{
			/* first argument all digits or - means clear */
			while(isdigit(*arg))
				arg++;
			clear = (arg!=action && *arg==0);
			if(!clear)
			{
				++argv;
				if(*action=='-' && action[1]==0)
					clear++;
			}
			while(!argv[0])
				error(ERROR_exit(1),gettxt(e_condition_id,e_condition));
		}
		while(arg = *argv++)
		{
			sig = sig_number(arg);
			if(sig<0)
			{
				error(2,gettxt(e_trap_id,e_trap),arg);
				return(1);
			}
			/* internal traps */
			if(sig&SH_TRAP)
			{
				sig &= ~SH_TRAP;
				if(sig>SH_DEBUGTRAP)
				{
					error(2,gettxt(e_trap_id,e_trap),arg);
					return(1);
				}
				if(pflag)
				{
					if(arg=sh.st.trap[sig])
						sfputr(sfstdout,sh_fmtq(arg),'\n');
					continue;
				}
				if(sh.st.trap[sig])
					free(sh.st.trap[sig]);
				sh.st.trap[sig] = 0;
				if(!clear && *action)
					sh.st.trap[sig] = strdup(action);
				if(sig == SH_DEBUGTRAP)
				{
					if(sh.st.trap[sig])
						sh.trapnote |= SH_SIGTRAP;
					else
						sh.trapnote = 0;
				}
				continue;
			}
			if(sig>sh.sigmax)
			{
				error(2,gettxt(e_trap_id,e_trap),arg);
				return(1);
			}
			else if(pflag)
			{
				if(arg=sh.st.trapcom[sig])
					sfputr(sfstdout,sh_fmtq(arg),'\n');
			}
			else if(clear)
				sh_sigclear(sig);
			else
			{
				if(sig >= sh.st.trapmax)
					sh.st.trapmax = sig+1;
				if(arg=sh.st.trapcom[sig])
					free(arg);
				sh.st.trapcom[sig] = strdup(action);
				sh_sigtrap(sig);
			}
		}
	}
	else /* print out current traps */
		sig_list(-1);
	return(0);
}

int	b_kill(int argc,char *argv[], void *extra)
{
	register char *signame;
	register int sig=SIGTERM, flag=0, n;
	NOT_USED(argc);
	NOT_USED(extra);
	while((n = optget(argv,(const char *)gettxt(sh_optkill_id,sh_optkill)))) switch(n)
	{
		case ':':
			if((signame=argv[opt_index++]) && (sig=sig_number(signame+1))>=0)
			{
				if(argv[opt_index] && strcmp(argv[opt_index],"--")==0)
					opt_index++;
				goto endopts;
			}
			opt_index--;
			error(2, opt_arg);
			break;
		case 'n':
			sig = (int)opt_num;
			break;
		case 's':
			flag |= S_FLAG;
			signame = opt_arg;
			break;
		case 'l':
			flag |= L_FLAG;
			break;
		case '?':
			error(ERROR_usage(2), opt_arg);
			break;
	}
endopts:
	argv += opt_index;
	if(error_info.errors || flag==(L_FLAG|S_FLAG) || (!(*argv) && !(flag&L_FLAG)))
		error(ERROR_usage(2),optusage((char*)0));
	/* just in case we send a kill -9 $$ */
	sfsync(sfstderr);
	if(flag&L_FLAG)
	{
		if(!(*argv))
			sig_list(0);
		else while(signame = *argv++)
		{
			if(isdigit(*signame))
				sig_list((atoi(signame)&0177)+1);
			else
			{
				if((sig=sig_number(signame))<0)
				{
					sh.exitval = 2;
					error(ERROR_exit(1),gettxt(e_nosignal_id,e_nosignal),signame);
				}
				sfprintf(sfstdout,"%d\n",sig);
			}
		}
		return(sh.exitval);
	}
	if(flag&S_FLAG)
	{
		if((sig=sig_number(signame)) < 0 || sig > sh.sigmax)
			error(ERROR_exit(1),gettxt(e_nosignal_id,e_nosignal),signame);
	}
	if(job_walk(sfstdout,job_kill,sig,argv))
		sh.exitval = 1;
	return(sh.exitval);
}

/*
 * Given the name or number of a signal return the signal number
 */

static int sig_number(const char *string)
{
	register int n;
	char *last;
	if(isdigit(*string))
	{
		n = strtol(string,&last,10);
		if(*last)
			n = -1;
	}
	else
	{
		register int c;
		n = staktell();
		do
		{
			c = *string++;
			if(islower(c))
				c = toupper(c);
			stakputc(c);
		}
		while(c);
		stakseek(n);
		n = sh_lookup(stakptr(n),shtab_signals);
		n &= (1<<SH_SIGBITS)-1;
		if(n < SH_TRAP)
			n--;
	}
	return(n);
}

/*
 * if <flag> is positive, then print signal name corresponding to <flag>
 * if <flag> is zero, then print all signal names
 * if <flag> is negative, then print all traps
 */
static void sig_list(register int flag)
{
	register const struct shtable4	*tp;
	register int sig = sh.sigmax+1;
	const char *names[SH_TRAP];
	const char *traps[SH_DEBUGTRAP+1];
	tp=shtab_signals;
	if(flag==0)
	{
		/* not all signals may be defined, so initialize */
		while(--sig >= 0)
			names[sig] = 0;
		for(sig=SH_DEBUGTRAP; sig>=0; sig--)
			traps[sig] = 0;
	}
	while(*tp->sh_name)
	{
		sig = tp->sh_number;
		sig &= ((1<<SH_SIGBITS)-1);
		if(sig==flag)
		{
			sfprintf(sfstdout,"%s\n",tp->sh_name);
			return;
		}
		else if(sig&SH_TRAP)
			traps[sig&~SH_TRAP] = (char*)tp->sh_name;
		else if(sig < sizeof(names)/sizeof(char*))
			names[sig] = (char*)tp->sh_name;
		tp++;
	}
	if(flag > 0)
		sfprintf(sfstdout,"%d\n",flag-1);
	else if(flag<0)
	{
		/* print the traps */
		register char *trap,*sname,**trapcom;
		sig = sh.st.trapmax;
		/* use parent traps if otrapcom is set (for $(trap)  */
		trapcom = (sh.st.otrapcom?sh.st.otrapcom:sh.st.trapcom);
		while(--sig >= 0)
		{
			if(!(trap=trapcom[sig]))
				continue;
			if(!(sname=(char*)names[sig+1]))
			{
				sname="SIG??";
				sname[3] = (sig/10)+'0';
				sname[4] = (sig%10)+'0';
			}
			sfprintf(sfstdout,trapfmt,sh_fmtq(trap),sname);
		}
		for(sig=SH_DEBUGTRAP; sig>=0; sig--)
		{
			if(!(trap=sh.st.trap[sig]))
				continue;
			sfprintf(sfstdout,trapfmt,sh_fmtq(trap),traps[sig]);
		}
	}
	else
	{
		/* print all the signal names */
		for(sig=2; sig <= sh.sigmax; sig++)
		{
			if(names[sig])
				sfputr(sfstdout,names[sig],'\n');
			else
				sfprintf(sfstdout,"SIG%d\n",sig-1);
		}
	}
}

