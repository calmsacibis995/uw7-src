/*		copyright	"%c%" 	*/

#ident	"@(#)rwioctl.c	1.2"
#ident "$Header$"

#include	"uucp.h"
#include	<sys/stropts.h>

static void dump_stream();
#ifdef TLI
#include <xti.h>
static void tfaillog(), show_tlook();
#endif /*  TLI  */

extern void	sethup();
extern int	restline();
extern int	read(), write();


/*
 *	A static flag "rw_flag" is used to control particular I/O
 *	functions used. 
 *
 *	set_rw() is called if the current value is -1 (i.e. uninitialized)
 *	read(2)/write(2) are used if the file descriptor is a 'tty'
 */

static int rw_flag = -1;
#ifdef TLI
static struct t_info t_info;
static int got_t_info = -1;
#endif

static void
set_rw(int fd)
{
	if (rw_flag != -1)
		return;

	rw_flag = 1;	/* assume read()/write() */
#ifdef TLI
	if (isastream(fd)
		&& (ioctl(fd, I_FIND, "tirdwr") != 1)
		&& (ioctl(fd, I_FIND, "timod") == 1)) {
		rw_flag = 0;
		got_t_info = (t_getinfo(fd, &t_info));
		if (got_t_info != 0)
			tfaillog(fd, "set_rw: t_getinfo\n");
		return;
	}
#endif
	return;
}

int
Setup(int role, int *fdreadp, int *fdwritep)
{
	if (role == SLAVE) {
		*fdreadp = 0;
		*fdwritep = 1;
	}
	if (rw_flag == -1)
		set_rw(*fdreadp);

	if (Debug > 8)
		dump_stream(*fdreadp);

	if (role == SLAVE) {
		*fdreadp = 0;
		*fdwritep = 1;
		/* 2 has already been set to remote debug in main() */
#ifdef TLI
		if (!rw_flag) {
			if ( t_sync(*fdreadp) == -1 ) {
				tfaillog(*fdreadp, "tsetup: t_sync read\n");
				return(FAIL);
			}

			if ( t_sync(*fdwritep) == -1 ) {
				tfaillog(*fdwritep, "tsetup: t_sync write\n");
				return(FAIL);
			}
		}
#endif
	}
	return(SUCCESS);
}

/* ARGSUSED */
int
Teardown(int role, int fdread, int fdwrite)
{
	int ret;

	if (rw_flag == -1)
		set_rw(fdread);

	if (role == SLAVE) {
		ret = restline();
		DEBUG(4, "restline - %d\n", ret);
		sethup(0);
	}

	(void) fchmod(fdread, Dev_mode);

#ifdef TLI
	if (rw_flag) {
		(void)t_unbind(fdread);
		(void)t_close(fdread);
	} else
#endif
		(void) close(fdread);
		
	return(SUCCESS);
}

int
Read(int fd, char *buf, unsigned nbytes)
{
	int rcv_flags = 0;

	if (rw_flag == -1)
		set_rw(fd);

#ifdef TLI
	if (!rw_flag)
		return(t_rcv(fd, buf, nbytes, &rcv_flags));
	else
#endif
		return(read(fd, buf, nbytes));
}

#define N_CHECK 100

int
Write(int fd, char *buf, unsigned nbytes)
{
	register int i = 0, ret;
	static int n_write;

	if (rw_flag == -1)
		set_rw(fd);

#ifdef TLI
	if (!rw_flag) {
		if (got_t_info != 0) {
			tfaillog(fd, "twrite: t_getinfo\n");
			return(FAIL);
		}

		if (++n_write == N_CHECK) {
			n_write = 0;
			if (t_getstate(fd) != T_DATAXFER)
				return(FAIL);
		}
	
		while (nbytes > 0) {
			unsigned chunk;

			if (t_info.tsdu > 0 && nbytes > t_info.tsdu)
				chunk = t_info.tsdu;
			else
				chunk = nbytes;
			ret = t_snd(fd, &buf[i], chunk, NULL);
			if (ret != chunk) {
				tfaillog(fd, "Write: t_snd\n");
				return(ret < 0 ? ret : i + ret);
			}
			i+= ret;
			nbytes -= ret;
		}
	} else
#endif /* TLI */
		return(write(fd, buf, nbytes));
	return(i);
}

/*VARARGS2*/
int
Ioctl(int fd, int request, int arg)
{
	if (rw_flag == -1)
		set_rw(fd);
	
#ifdef TLI
	if (!rw_flag)
		return(SUCCESS);
	else
#endif
		return(ioctl(fd, request, arg));
}

#ifdef TLI
/*
 *	Report why a TLI call failed.
 */
void
tfaillog(fd, s)
int	fd;
char	*s;
{
	extern char	*sys_errlist[];
	extern char	*t_errlist[];
	extern int	t_errno, t_nerr;
	char	fmt[ BUFSIZ ];

	if (0 < t_errno && t_errno < t_nerr) {
		sprintf( fmt, "%s: %%s\n", s );
		DEBUG(5, fmt, t_errlist[t_errno]);
		logent(s, t_errlist[t_errno]);
		if ( t_errno == TSYSERR ) {
			strcpy(fmt, "tlicall: system error: %s\n");
			DEBUG(5, fmt, sys_errlist[errno]);
		} else if ( t_errno == TLOOK ) {
			show_tlook(fd);
		}
	} else {
		sprintf(fmt, "unknown tli error %d", t_errno);
		logent(s, fmt);
		sprintf(fmt, "%s: unknown tli error %d", s, t_errno);
		DEBUG(5, fmt, 0);
		sprintf(fmt, "%s: %%s\n", s);
		DEBUG(5, fmt, sys_errlist[errno]);
	}
	return;
}

void
show_tlook(fd)
int fd;
{
	register int reason;
	register char *msg;
	extern int	t_errno;
/*
 * Find out the current state of the interface.
 */
	errno = t_errno = 0;
	switch( reason = t_getstate(fd) ) {
	case T_UNBND:		msg = "T_UNBIND";	break;
	case T_IDLE:		msg = "T_IDLE";		break;
	case T_OUTCON:		msg = "T_OUTCON";	break;
	case T_INCON:		msg = "T_INCON";	break;
	case T_DATAXFER:	msg = "T_DATAXFER";	break;
	case T_OUTREL:		msg = "T_OUTREL";	break;
	case T_INREL:		msg = "T_INREL";	break;
	default:		msg = NULL;		break;
	}
	if( msg == NULL )
		return;

	DEBUG(5, "state is %s", msg);
	switch( reason = t_look(fd) ) {
	case -1:		msg = ""; break;
	case 0:			msg = "NO ERROR"; break;
	case T_LISTEN:		msg = "T_LISTEN"; break;
	case T_CONNECT:		msg = "T_CONNECT"; break;
	case T_DATA:		msg = "T_DATA";	 break;
	case T_EXDATA:		msg = "T_EXDATA"; break;
	case T_DISCONNECT:	msg = "T_DISCONNECT"; break;
	case T_ORDREL:		msg = "T_ORDREL"; break;
	case T_ERROR:		msg = "T_ERROR"; break;
	case T_UDERR:		msg = "T_UDERR"; break;
	default:		msg = "UNKNOWN ERROR"; break;
	}
	DEBUG(4, " reason is %s\n", msg);

	if ( reason == T_DISCONNECT )
	{
		struct t_discon	*dropped;
		if ( ((dropped = 
			(struct t_discon *)t_alloc(fd, T_DIS, T_ALL)) == 0) 
		||  (t_rcvdis(fd, dropped) == -1 )) {
			return;
		}
		DEBUG(5, "disconnect reason #%d\n", dropped->reason);
		t_free(dropped, T_DIS);
	}
	return;
}
#endif /*  TLI  */

static void
dump_stream(int fd)
{
    int nmod;

    if ( (nmod = ioctl(fd, I_LIST, (struct str_list *)NULL)) > 0 ) {
	int i;
	struct str_list strlist;
	struct str_mlist mlist[10];

	strlist.sl_nmods = nmod;
	strlist.sl_modlist = mlist;

	if ( ioctl(fd, I_LIST, &strlist) == 0 ) {
  		int iaf_flag = 0;
  		for ( i=0; i < nmod; i++ ) {
	    		DEBUG(6, "module '%s' on stream\n",
				strlist.sl_modlist[i].l_name);
	    		if (strcmp(strlist.sl_modlist[i].l_name, "iaf") == 0)
				iaf_flag++;
  		}
		DEBUG(6, "iaf flag is %d\n.", iaf_flag);
		if (iaf_flag) {
			char **iaptr, **retava();
			if ((iaptr = retava(fd)) != NULL) {
				while (*iaptr)
					DEBUG(6, "iaf AVA: %s\n", *(iaptr++));
			} else
				DEBUG(6, "retava returned %s\n", "NULL");
		}
	}
    }

}
