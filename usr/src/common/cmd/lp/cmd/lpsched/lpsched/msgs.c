/*		copyright	"%c%" 	*/


#ident	"@(#)msgs.c	1.2"
#ident  "$Header$"

#include	<stdarg.h>
#include	<limits.h>
#include	<sys/types.h>
#include	<poll.h>
#include	<stropts.h>
#include	<unistd.h>

# include	"lpsched.h"

#define WHO_AM_I	I_AM_LPSCHED
#include "oam.h"
 
#define TURN_OFF(X,F)	(void)Fcntl(X, F_SETFL, (Fcntl(X, F_GETFL, 0) & ~(F)))

#ifdef	DEBUG
static char time_buf[50];
#endif

#ifdef	__STDC__
static void	log_message_fact(char *, ...);
static void	do_msg3_2 ( MESG * md );
#else
static void	log_message_fact();
static void	do_msg3_2 ();
#endif

static void	net_shutdown();
static void	conn_shutdown();

extern void		dispatch();
void			shutdown_messages();
static char		*Message;
static int		MaxClients		= 0,
			do_msg();
extern int		Reserve_Fds;
extern int		Shutdown;

MESG			*Net_md;
int			Net_fd;

/*
 * Procedure:     take_message
 *
 * Restrictions:
 *               mlisten: None
 *               mread: None
 * Notes - WAIT FOR INTERRUPT OR ONE MESSAGE FROM USER PROCESS
*/

#ifdef	__STDC__
void take_message ( void )	/* funcdef */
#else
void take_message ( )
#endif
{
    DEFINE_FNNAME (take_message)

    int		bytes;
    int		slot;
    MESG *	md;

    for (EVER)	/* not really forever...returns are in the loop */
    {
	if ((md = mlisten()) == NULL)
	    switch(errno)
	    {
	      case EAGAIN:
	      case EINTR:
		return;

	      case ENOMEM:
		mallocfail();
		/* NOTREACHED */

	      default:
		lpfail (ERROR, E_SCH_USTREAMS, PERROR);
	    }
	
	/*
	 * Check for a dropped connection to a child.
	 * Normally a child should tell us that it is dying
	 * (with S_SHUTDOWN or S_SEND_CHILD), but it may have
	 * died a fast death. We'll simulate the message we
	 * wanted to get so we can use the same code to clean up.
	 */
	if (
		(md->event & POLLHUP) && !(md->event & POLLIN)
	     || (md->event & (POLLERR|POLLNVAL))
	) {
		switch (md->type) {

		case MD_CHILD:
			/*
			 * If the message descriptor is found in the
			 * exec table, it must be an interface pgm,
			 * notification, etc. Otherwise, it must be
			 * a network child.
			 */
			for (slot = 0; slot < ET_Size; slot++)
				if (Exec_Table[slot].md == md)
					break;
			if (slot < ET_Size) {
				(void)putmessage (
					Message,
					S_CHILD_DONE,
					Exec_Table[slot].key,
					slot,
					0,
					0
				);
#ifdef	DEBUG
				if (debug & (DB_EXEC|DB_DONE)) {
					execlog (
						"DROP! slot %d, pid %d\n",
						slot,
						Exec_Table[slot].pid
					);
					execlog ("%e", &Exec_Table[slot]);
				}
#endif
			} else
				(void)putmessage (Message, S_SHUTDOWN, 1);
			bytes = 1;
			break;

		default:
			bytes = -1;
			break;

		}

	} else
		bytes = mread(md, Message, MSGMAX);

	switch (bytes)
	{
	  case -1:
	    if (errno == EINTR)
		return;
	    else
		lpfail (ERROR, E_SCH_STREAMS, PERROR);
	    break;

	  case 0:
	    break;

	  default:
	    if (do_msg(md))
		return;
	    break;
	}
    }
}

/*
** do_msg() - HANDLE AN INCOMING MESSAGE
*/

#ifdef	__STDC__
static int do_msg ( MESG * md )	/* funcdef */
#else
static int do_msg (md)
    MESG	*md;
#endif
{
    DEFINE_FNNAME (do_msg)

    int			type;

#ifdef	DEBUG
    if (debug & DB_MESSAGES)
    {
	FILE	*fp = open_logfile("messages");

	if (fp)
	{
		time_t	now = time((time_t *)0);
		char	buffer[BUFSIZ];
		int	size	= stoh(Message + MESG_SIZE);
		int	type	= stoh(Message + MESG_TYPE);

		setbuf(fp, buffer);
		(void) cftime(time_buf, NULL, &now);
		(void) fprintf(
		    fp,
		    "RECV: %24.24s type %d size %d\n",
		    time_buf,
		    type,
		    size
		);
		(void) fputs("      ", fp);
		(void) fwrite(Message, 1, size, fp);
		(void) fputs("\n", fp);
		close_logfile(fp);
	}
    }
# endif

    switch (type = mtype(Message))
    {
      case S_NEW_QUEUE:
	do_msg3_2(md);
	break;

      default:
#ifdef	DEBUG
	log_message_fact ("      MESSAGE ACCEPTED: client %#0x", md);
#endif
	if (type != S_GOODBYE)
	{
	    md->wait = 0;
	    dispatch (type, Message, md);
	    /*
	     * The message may have caused the need to
	     * schedule something, so go back and check.
	     */
	    return(1);
	}
	break;
    }
    return(0);
}

/*
 * Procedure:     calculate_nopen
 *
 * Restrictions:
 *               fcntl(2): None
 * Notes - DETERMINE # FILE DESCRIPTORS AVAILABLE FOR QUEUES
*/

#ifdef	__STDC__
static void calculate_nopen ( void )	/* funcdef */
#else
static void calculate_nopen()
#endif
{
    DEFINE_FNNAME (calculate_nopen)

    int		fd, nopen;

    /*
     * How many file descriptorss are currently being used?
     */
    for (fd = nopen = 0; fd < OpenMax; fd++)
	if (fcntl(fd, F_GETFL, 0) != -1)
	    nopen++;

    /*
     * How many file descriptors are available for use
     * as open FIFOs? Leave one spare as a way to tell
     * clients we don't have any to spare (hmmm....) and
     * one for the incoming fifo.
     */

    MaxClients = OpenMax;
    MaxClients -= nopen;	/* current overhead */
    MaxClients -= Reserve_Fds;
    MaxClients -= 2;		/* incoming FIFO and spare outgoing */
    MaxClients--;		/* the requests log */
    MaxClients--;		/* HPI routines and lpsched log */

    return;
}

static void net_shutdown ( )
{
    DEFINE_FNNAME (net_shutdown)

    lpnote (INFO, E_SCH_NETTERM);
    Net_md = NULL;
    lpshut(1);
}

static void conn_shutdown ( )
{
    DEFINE_FNNAME (conn_shutdown)

    if (!Shutdown)
	lpnote (ERROR, E_SCH_PUBFAILED);
    lpshut(1);
}

/*
 * Procedure:     init_messages
 *
 * Restrictions:
 *               Chmod: None
 *               mcreate: None
 *               system: None
 * Notes - INITIALIZE MAIN MESSAGE QUEUE
*/

#ifdef	__STDC__
void init_messages ( void )	/* funcdef */
#else
void init_messages()
#endif
{
    DEFINE_FNNAME (init_messages)

    char	*cmd;
    MESG *	md;

    (void) signal(SIGPIPE, SIG_IGN);

    calculate_nopen ();

    if (cmd = makestr(RMCMD, " ", Lp_Public_FIFOs, "/*", (char *)0))
    {
	(void) system(cmd);
	Free(cmd);
    }
    if (cmd = makestr(RMCMD, " ", Lp_Private_FIFOs, "/*", (char *)0))
    {
	(void) system(cmd);
	Free(cmd);
    }

    Message = (char *)Malloc(MSGMAX);

    (void) Chmod(Lp_Public_FIFOs, 0773);
    (void) Chmod(Lp_Private_FIFOs, 0771);
    (void) Chmod(Lp_Tmp, 0711);
    
    if ((md = mcreate(Lp_FIFO)) == NULL)
	lpfail (ERROR, E_SCH_PUBMESS, PERROR);
    (void) mon_discon(md, conn_shutdown);
    
    if (mlisteninit(md) != 0)
	if (errno == ENOMEM)
	    mallocfail();
	else
	    lpfail (ERROR, E_SCH_STREAMS, PERROR);

    (void) Chmod(Lp_FIFO, 0666);
    return;
}

/*
 * Procedure:     init_network
 *
 * Restrictions:
 *               fork(2): None
 *               Fcntl: None
 *               execl: None
 *               mconnect: None
 *               mdisconnect: None
*/
void
#if	defined(__STDC_)
init_network ( void )
#else
init_network()
#endif
#ifdef	NETWORKING
{
    DEFINE_FNNAME (init_network)

    int		i;
    int		fds[2];

    if (Net_md)
	return;
    
    if (access(LPNET, X_OK) < 0) {
	lpnote (ERROR, E_SCH_NETFAILED, PERROR);
	lpshut(1);
	return;
    }

    if (pipe(fds) == -1)
	lpfail (ERROR, E_SCH_NETPIPE, PERROR);
    switch (fork())
    {
      case -1:	/* Failure */
	lpnote (ERROR, E_SCH_FORKFAILED);
	lpshut(1);
	break;
	
      case 0:	/* The Child */
	(void) Close(0);
	if (Fcntl(fds[1], F_DUPFD, 0) != 0)
	    exit(99);
	for (i = 1; i <= OpenMax; i++)
		(void) Close(i);
	(void) execl(LPNET, "lpNet", 0);
	exit(100);
	/* NOTREACHED */

      default:	/* The Parent */
	(void) Close(fds[1]);
	Net_fd = fds[0];
	if ((Net_md = mconnect(NULL, fds[0], fds[0])) == NULL)
	{
	    lpnote (ERROR, E_SCH_FAILEDBIND, PERROR);
	    (void) Close(fds[0]);
	    return;
	}
	Net_md->type = MD_CHILD;
	if (mlistenadd(Net_md, POLLIN) == -1)
	{
	    lpnote (ERROR, E_SCH_FAILEDATT, PERROR);
	    (void) mdisconnect(Net_md);
	    Net_md = NULL;
	    return;
	}
	(void) mon_discon(Net_md, net_shutdown);
    }
}
#else
{
	return;
}
#endif

/*
 * Procedure:     shutdown_messages
 *
 * Restrictions:
 *               Chmod: None
 *               mlistenreset: None
 *               mdestroy: None
*/
    
void
#ifdef	__STDC__
shutdown_messages ( void )	/* funcdef */
#else
shutdown_messages()
#endif
{
    DEFINE_FNNAME (shutdown_messages)

    MESG	*md;
    
    (void) Chmod(Lp_Public_FIFOs, 0770);
    (void) Chmod(Lp_Tmp, 0700);
    (void) Chmod(Lp_FIFO, 0600);
    md = mlistenreset();
    if (md)
	    (void) mdestroy(md);
}

#ifdef	DEBUG
/*VARARGS1*/
#ifdef	__STDC__
static void log_message_fact ( char * format , ... )	/* funcdef */
#else
static void log_message_fact (format, va_alist)
    char	*format;
    va_dcl
#endif
{
    DEFINE_FNNAME (log_message_fact)

    va_list	ap;

    if (debug & DB_MESSAGES)
    {
	FILE		*fp = open_logfile("messages");
	char		buffer[BUFSIZ];

#ifdef	__STDC__
	va_start (ap, format);
#else
	va_start (ap);
#endif
	if (fp) {
		setbuf (fp, buffer);
		(void) vfprintf (fp, format, ap);
		(void) fputs ("\n", fp);
		close_logfile (fp);
	}
	va_end (ap);
    }
    return;
}
#endif

/*
 * Procedure:     do_msg3_2
 *
 * Restrictions:
 *               Mknod: None
 *               Chown: None
 *               Chlvl: None
 *               Open: None
 *               Unlink: None
 *               mputm: None
 *               unlink(2): None
*/


/*
**	GROAN!!
** A pre-4.0 client wishes to talk. The protocol:
**
**	client			Spooler (us)
**
**	S_NEW_QUEUE  ------->
**				Create 2nd return fifo
**		     <-------   R_NEW_QUEUE on 1st fifo
**	close 1st fifo
**	prepare 2nd fifo
**	S_NEW_QUEUE  ------->
**				R_NEW_QUEUE with authcode
**				on 2nd fifo
**
** Thus this may be the first OR second S_NEW_QUEUE
** message for this client. We tell the difference
** by the "stage" field.
*/

#ifdef	__STDC__
static void do_msg3_2 ( MESG * md )	/* funcdef */
#else
static void do_msg3_2();
    MESG *	md;
#endif
{
    DEFINE_FNNAME (do_msg3_2)

    char *	pub_fifo;
    short	stage;
    char *	msg_fifo;
    char *	system;
    int		madenode;

    (void)getmessage (Message, S_NEW_QUEUE, &stage, &msg_fifo,&system);

    if (stage == 0)
    {
	/*
	** Make a new fifo, in the private dir rather
	** than the public dir. Use the original
	** for the ACK or NAK of this message.
	** Make the client the owner (user and group)
	** of the fifo, and make it readable only by the
	** client.
	**
	** ES Note:
	** This code is probably not used anymore but I have
	** added Chlvl just to be sure.
	*/

	md->file = makepath(Lp_Private_FIFOs, msg_fifo, (char *)0);
	pub_fifo = makepath(Lp_Public_FIFOs, msg_fifo, (char *)0);
	if (strlen(md->file)
	    && (madenode = (Mknod(md->file, (S_IFIFO|S_IRUSR|S_IWUSR), 0) == 0))
	    && Chown(md->file, md->uid, md->gid) == 0
	    && Chlvl(md->file, md->lid) == 0)
	{
	    /*
	    ** For "mknod()" to have succeeded, the
	    ** desired fifo couldn't have existed
	    ** already. Our creation gave modes
	    ** and owner that should prevent
	    ** anyone except who the client claims
	    ** to be from using the fifo. Thus
	    ** this fifo is ``safe''--anything we put
	    ** in can only be read by the client.
	    */

	    md->state = MDS_32PROTO;
	    if ((md->writefd = Open(pub_fifo, O_WRONLY | O_NDELAY, 0)) < 0)
	    {
		(void) Close(md->readfd);
		(void) Unlink(pub_fifo);
		Free(pub_fifo);
		return;
	    }
	    (void) mputm (md, R_NEW_QUEUE, MOK);
	    Free(pub_fifo);
	    
#ifdef	DEBUG
	    log_message_fact (
	"      NEW_QUEUE: client %#0x (%s) accepted at stage 0",
		md,
		msg_fifo
	    );
#endif
	}
	else
	{

	    /*
	    ** Another wise guy.
	    **
	    ** Before removing the second fifo, make sure
	    ** we actually created it!  If we did not get far
	    ** enough to set <madenode>, we never created the
	    ** fifo, so don't remove it!  The wise guy may be
	    ** trying to con us!
	    */

	    if (strlen(md->file) && madenode)
		(void)unlink (md->file);
	    Free (pub_fifo);
	    (void) Close(md->readfd);
#ifdef	DEBUG
	    log_message_fact (
"      NEW_QUEUE: client %#0x (%s) rejected at stage 0 (%s)",
		md,
		msg_fifo,
		PERROR
	    );
#endif
	}
    }
    else
    {
	if (md->state != MDS_32PROTO)
	    return;

	/*
	** The client has just sent the second S_NEW_QUEUE
	** so we will now send back another ACK on the
	** safe fifo.
	*/
	(void) Close(md->writefd);
	
	if ((md->writefd = Open(md->file, O_WRONLY | O_NDELAY, 0)) >= 0)
	{
	    md->state = MDS_32CONNECT;
	    (void) mputm (md, R_NEW_QUEUE, MOK);
#ifdef	DEBUG
	    log_message_fact (
	"      NEW_QUEUE: client %#0x (%s) accepted at stage 1",
		md,
		msg_fifo
	    );
#endif
	}
	else
	{
	    (void)unlink (md->file);
	    (void) Close(md->readfd);
#ifdef	DEBUG
	    log_message_fact (
"      NEW_QUEUE: client %#0x (%s) rejected at stage 1 (%s)",
		md,
		msg_fifo,
		PERROR
	    );
#endif
	}
    }
}
