#ident	"@(#)ksh93:src/cmd/ksh93/bltins/sleep.c	1.2"
#pragma prototyped
/*
 * sleep delay
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
#include	<errno.h>
#include	"builtins.h"
#include	"FEATURE/time"
#include	"FEATURE/poll"
#ifdef _NEXT_SOURCE
#   define sleep	_ast_sleep
#endif /* _NEXT_SOURCE */
#ifdef _lib_poll_notimer
#   undef _lib_poll
#endif /* _lib_poll_notimer */

int	b_sleep(register int argc,char *argv[],void *extra)
{
	register char *cp;
	register double d;
	time_t tloc = 0;
	NOT_USED(extra);
	while((argc = optget(argv,(const char *)gettxt(sh_optsleep_id,sh_optsleep)))) switch(argc)
	{
		case ':':
			error(2, opt_arg);
			break;
		case '?':
			error(ERROR_usage(2), opt_arg);
			break;
	}
	argv += opt_index;
	if(error_info.errors || !(cp= *argv) || !(strmatch(cp,e_numeric)))
		error(ERROR_usage(2),optusage((char*)0));
	if((d=atof(cp)) > 1.0)
	{
		sfsync(sh.outpool);
		time(&tloc);
		tloc += (time_t)(d+.5);
	}
	while(1)
	{
		time_t now;
		errno = 0;
		sh.lastsig=0;
		sh_delay(d);
		if(tloc==0 || errno!=EINTR || sh.lastsig)
			break;
		sh_sigcheck();
		if(tloc < (now=time(NIL(time_t*))))
			break;
		d = (double)(tloc-now);
		if(sh.sigflag[SIGALRM]&SH_SIGTRAP)
			sh_timetraps();
	}
	return(0);
}

static char expired;
static void completed(void * handle)
{
	NOT_USED(handle);
	expired = 1;
}

unsigned sleep(unsigned sec)
{
	pid_t newpid, curpid=getpid();
	void *tp;
	expired = 0;
	sh.lastsig = 0;
	tp = (void*)timeradd(1000*sec, 0, completed, (void*)0);
	do
	{
		if(!sh.waitevent || (*sh.waitevent)(-1,0L)==0)
			pause();
		if(sh.sigflag[SIGALRM]&SH_SIGTRAP)
			sh_timetraps();
		if((newpid=getpid()) != curpid)
		{
			curpid = newpid;
			alarm(1);
		}
	}
	while(!expired && sh.lastsig==0);
	if(!expired)
		timerdel(tp);
	sh_sigcheck();
	return(0);
}

/*
 * delay execution for time <t>
 */

void	sh_delay(double t)
{
	register int n = (int)t;
#ifdef _lib_poll
	struct pollfd fd;
	if(t<=0)
		return;
	else if(n > 30)
	{
		sleep(n);
		t -= n;
	}
	if(n=(int)(1000*t))
	{
		if(!sh.waitevent || (*sh.waitevent)(-1,(long)n)==0)
			poll(&fd,0,n);
	}
#else
#   if defined(_lib_select) && defined(_mem_tv_usec_timeval)
	struct timeval timeloc;
	if(t<=0)
		return;
	if(n=(int)(1000*t) && sh.waitevent && (*sh.waitevent)(-1,(long)n))
		return;
	n = (int)t;
	timeloc.tv_sec = n;
	timeloc.tv_usec = 1000000*(t-(double)n);
	select(0,(fd_set*)0,(fd_set*)0,(fd_set*)0,&timeloc);
#   else
#	ifdef _lib_select
		/* for 9th edition machines */
		if(t<=0)
			return;
		if(n > 30)
		{
			sleep(n);
			t -= n;
		}
		if(n=(int)(1000*t))
		{
			if(!sh.waitevent || (*sh.waitevent)(-1,(long)n)==0)
				select(0,(fd_set*)0,(fd_set*)0,n);
		}
#	else
		struct tms tt;
		if(t<=0)
			return;
		sleep(n);
		t -= n;
		if(t)
		{
			clock_t begin = times(&tt);
			if(begin==0)
				return;
			t *= sh.lim.clk_tck;
			n += (t+.5);
			while((times(&tt)-begin) < n);
		}
#	endif
#   endif
#endif /* _lib_poll */
}
