#ident	"@(#)ksh93:src/cmd/ksh93/bltins/alarm.c	1.1"
#pragma prototyped
/*
 * alarm [-r] [varname [+]when]
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
#include	<stak.h>
#include	"builtins.h"
#include	"FEATURE/time"

#define R_FLAG	1
#define L_FLAG	2

struct	tevent
{
	Namfun_t	fun;
	Namval_t	*node;
	Namval_t	*action;
	struct tevent	*next;
	long		milli;
	int		flags;
	void            *timeout;
};

static const char ALARM[] = "alarm";

static void	trap_timeout(void*);

/*
 * insert timeout item on current given list in sorted order
 */
static void *time_add(struct tevent *item, void *list)
{
	register struct tevent *tp = (struct tevent*)list;
	if(!tp || item->milli < tp->milli)
	{
		item->next = tp;
		list = (void*)item;
	}
	else
	{
		while(tp->next && item->milli > tp->next->milli)
			tp = tp->next;
		item->next = tp->next;
		tp->next = item;
	}
	tp = item;
	tp->timeout = (void*)timeradd(tp->milli,tp->flags&R_FLAG,trap_timeout,(void*)tp);
	return(list);
}

/*
 * delete timeout item from current given list, delete timer
 */
static 	void *time_delete(register struct tevent *item, void *list)
{
	register struct tevent *tp = (struct tevent*)list;
	if(item==tp)
		list = (void*)tp->next;
	else
	{
		while(tp && tp->next != item)
			tp = tp->next;
		if(tp)
			tp->next = item->next;
	}
	if(item->timeout)
		timerdel((void*)item->timeout);
	return(list);
}

static void	print_alarms(void *list)
{
	register struct tevent *tp = (struct tevent*)list;
	while(tp)
	{
		if(tp->timeout)
		{
			register char *name = nv_name(tp->node);
			if(tp->flags&R_FLAG)
			{
				double d = tp->milli;
				sfprintf(sfstdout,gettxt(e_alrm1_id,e_alrm1),name,d/1000.);
			}
			else
				sfprintf(sfstdout,gettxt(e_alrm2_id,e_alrm2),name,nv_getnum(tp->node));
		}
		tp = tp->next;
	}
}

static void	trap_timeout(void* handle)
{
	register struct tevent *tp = (struct tevent*)handle;
	sh.trapnote |= SH_SIGTRAP;
	if(!(tp->flags&R_FLAG))
		tp->timeout = 0;
	tp->flags |= L_FLAG;
	sh.sigflag[SIGALRM] |= SH_SIGTRAP;
	if(sh_isstate(SH_TTYWAIT))
		sh_timetraps();
}

void	sh_timetraps(void)
{
	register struct tevent *tp, *tpnext;
	register struct tevent *tptop;
	while(1)
	{
		sh.sigflag[SIGALRM] &= ~SH_SIGTRAP;
		tptop= (struct tevent*)sh.st.timetrap;
		for(tp=tptop;tp;tp=tpnext)
		{
			tpnext = tp->next;
			if(tp->flags&L_FLAG)
			{
				tp->flags &= ~L_FLAG;
				if(tp->action)
					sh_fun(tp->action,tp->node);
				tp->flags &= ~L_FLAG;
				if(!tp->flags)
				{
					nv_unset(tp->node);
					nv_close(tp->node);
				}
			}
		}
		if(!(sh.sigflag[SIGALRM]&SH_SIGTRAP))
			break;
	}
}


/*
 * This trap function catches "alarm" actions only
 */
static char *setdisc(Namval_t *np, const char *event, Namval_t* action, Namfun_t
 *fp)
{
        register struct tevent *tp = (struct tevent*)fp;
	if(!event)
		return(action?"":(char*)ALARM);
	if(strcmp(event,ALARM)!=0)
	{
		/* try the next level */
		return(nv_setdisc(np, event, action, fp));
	}
	if(action==np)
		action = tp->action;
	else
		tp->action = action;
	return(action?(char*)action:"");
}

/*
 * catch assignments and set alarm traps
 */
static void putval(Namval_t* np, const char* val, int flag, Namfun_t* fp)
{
	register struct tevent *tp;
	register double d;
	if(val)
	{
		double now;
#ifdef _lib_gettimeofday
		struct timeval tmp;
		gettimeofday(&tmp,NIL(struct timezone*));
		now = tmp.tv_sec + 1.e-6*tmp.tv_usec;
#else
		now = (double)time(NIL(time_t*));
#endif /*_lib_gettimeofday */
		nv_putv(np,val,flag,fp);
		d = nv_getnum(np);
		tp = (struct tevent*)fp;
		if(*val=='+')
		{
			double x = d + now;
			nv_putv(np,(char*)&x,NV_INTEGER,fp);
		}
		else
			d -= now;
		tp->milli = 1000*(d+.0005);
		if(tp->timeout)
			sh.st.timetrap = time_delete(tp,sh.st.timetrap);
		if(tp->milli > 0)
			sh.st.timetrap = time_add(tp,sh.st.timetrap);
	}
	else
	{
		tp = (struct tevent*)nv_stack(np, (Namfun_t*)0);
		sh.st.timetrap = time_delete(tp,sh.st.timetrap);
		if(tp->action)
			nv_close(tp->action);
		nv_unset(np);
		free((void*)fp);
	}
}

static const Namdisc_t alarmdisc =
{
	sizeof(struct tevent),
	putval,
	0,
	0,
	setdisc,
};

int	b_alarm(int argc,char *argv[], void *extra)
{
	register int n,rflag=0;
	register Namval_t *np;
	register struct tevent *tp;
	NOT_USED(extra);
	while (n = optget(argv,(const char *)gettxt(sh_optalarm_id,sh_optalarm))) switch (n)
	{
	    case 'r':
		rflag = R_FLAG;
		break;
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_arg);
		break;
	}
	argc -= opt_index;
	argv += opt_index;
	if(error_info.errors)
		error(ERROR_usage(2),optusage((char*)0));
	if(argc==0)
	{
		print_alarms(sh.st.timetrap);
		return(0);
	}
	if(argc!=2)
		error(ERROR_usage(2),optusage((char*)0));
	np = nv_open(argv[0],sh.var_tree,NV_ARRAY|NV_VARNAME|NV_NOASSIGN);
	if(!nv_isnull(np))
		nv_unset(np);
	nv_setattr(np, NV_INTEGER|NV_DOUBLE);
	if(!(tp = newof(NIL(struct tevent*),struct tevent,1,0)))
		error(ERROR_exit(1),gettxt(e_nospace_id,e_nospace));
	tp->fun.disc = &alarmdisc;
	tp->flags = rflag;
	tp->node = np;
	nv_stack(np,(Namfun_t*)tp);
	nv_putval(np, argv[1], 0);
	return(0);
}

