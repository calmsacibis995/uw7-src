/*		copyright	"%c%" 	*/


#ident	"@(#)nodes.h	1.4"
#ident	"$Header$"

/*******************************************************************************
 *
 * FILENAME:    nodes.h
 *
 * DESCRIPTION: Definitions for node structures used by lpsched
 *
 * SCCS:	nodes.h 1.4  7/21/97 at 14:32:48
 *
 * CHANGE HISTORY:
 *
 * 21-07-97  Paul Cunningham        ul97-19724
 *           Added define for TIMEOUT_GSTAT and changed structure of susp_node
 *           to add timeoutAt, these are used by lpsched to timeout suspended
 *           remote "ask for status" commands.
 *
 *******************************************************************************
 */
#define TIMEOUT_GSTAT 5		/* timeout (secs) a remote "ask for status" */

typedef struct alert_node	ALERT;
typedef struct cstat_node	CSTATUS;
typedef struct exec_node	EXEC;
typedef struct form_node	_FORM;
typedef struct fstat_node	FSTATUS;
typedef struct pstat_node	PSTATUS;
typedef struct pwstat_node	PWSTATUS;
typedef struct sstat_node	SSTATUS;
typedef struct rstat_node	RSTATUS;
typedef struct waiting_node	WAITING;
typedef struct susp_node	SUSPENDED;

struct alert_node
{
    short	active;			/* Non-zero if triggered     */
    EXEC	*exec;			/* Index into EXEC table     */
    char	*msgfile;
};

struct cstat_node
{
    short	status;
    char	*rej_reason;
    time_t	rej_date;
    CLASS	*class;
};

struct exec_node
{
    int		pid;			/* process-id of exec		*/
    int		status;			/* low order bits from wait	*/
    long	key;			/* private key for security	*/
    short	errno;			/* copy of child's errno	*/
    short	type;			/* type of exec, EX_...		*/
    ushort	flags;			/* flags, EXF_...		*/
    MESG	*md;
    union ex
    {
	RSTATUS		*request;
	FSTATUS		*form;
	PWSTATUS	*pwheel;
	PSTATUS		*printer;
    } ex;
};

struct waiting_node
{
    WAITING	*next;
    MESG	*md;
    PSTATUS	*printer;
};

struct susp_node
{
    SUSPENDED	*next;
    time_t	timeoutAt;  /* timeout (secs) for suspended command */
    char	*message;
    MESG	*md;
};

struct sstat_node
{
    short	status;
    SYSTEM	*system;
    WAITING	*waiting;
    WAITING	*ps_waiting;	/* printer status waiting */
    time_t	laststat;
    EXEC	*exec;
};

/* sstat_node.status */
#define SS_RETRYING	0x0001	/* remote maybe down - retrying */

#define	EX_INTERF	1	/* exec interface for ex.printer	*/
#define	EX_SLOWF	2	/* exec slow filter for ex.request	*/
#define	EX_ALERT	3	/* exec alert for ex.printer		*/
#define	EX_FALERT	4	/* exec alert for ex.form		*/
#define	EX_PALERT	5	/* exec alert for ex.pwheel		*/
#define	EX_NOTIFY	6	/* exec notification for ex.request	*/

#define REX_INTERF	1	/* send print request to remote		*/
#define REX_CANCEL	2	/* send cancellation to remote		*/
#define REX_NOTIFY	3	/* send job termination note to remote	*/
#define REX_STATUS	4	/* send status request to remote	*/
#define REX_MOVE	5

#define	EXF_RESTART	0x0001	/* restart the exec			*/
#define	EXF_KILLED	0x0002	/* terminate() has killed the exec	*/
#if	defined(CHECK_CHILDREN)
#define	EXF_GONE	0x0004	/* child has disappeared		*/
#endif	/* CHECK_CHILDREN */
#define EXF_WAITCHILD	0x0008	/* waiting for R_NEW_CHILD		*/
#define EXF_WAITJOB	0x0010	/* waiting for R_SEND_JOB		*/

/*
**	Possible values for FLT.type
*/
#define        FLT_FILES       1	/* remove alloc'd files		*/
#define        FLT_CHANGE      2	/* clear RS_CHANGING for .r1	*/

struct fstat_node
{
    _FORM	*form;
    ALERT	*alert;
    short	requests;		/* Number of events thus far */
    short	requests_last;		/* # when alert last sent */
    short	trigger;		/* Trigger when this value   */
    short	mounted;		/* # times currently mounted */
    char	**users_allowed;
    char	**users_denied;
    char	*cpi;
    char	*lpi;
    char	*plen;
    char	*pwid;
};

struct pstat_node
{
    short	status;			/* Current Status of printer */
    short	job_seq_no;
    RSTATUS	*request;
    PRINTER	*printer;
    ALERT	*alert;
    EXEC	*exec;
    FSTATUS	*form;
    char	*pwheel_name;
    PWSTATUS	*pwheel;
    char	*dis_reason;
    char	*rej_reason;
    char	**users_allowed;
    char	**users_denied;
    char	**forms_allowed;
    char	**forms_denied;
    char	*cpi;
    char	*lpi;
    char	*plen;
    char	*pwid;
    time_t	dis_date;
    time_t	rej_date;
    short	last_dial_rc;		/* last exit from dial() */
    short	nretry;			/* number of dial attempts */
    SSTATUS	*system;
    char	*remote_name;
    PSTATUS	*next_list;		/* used for temporary linking */
    time_t	laststat;
    short	nrequests;		/* TEMP ONLY! (used variously) */
    /* info for remote printers - via R_INQUIRE_REMOTE_PRINTER */
    short	rmt_status;
    time_t	rmt_dis_date;
    time_t	rmt_rej_date;
    char	*rmt_dis_reason;
    char	*rmt_rej_reason;
    char	*rmt_req_id;
    char	*rmt_form;
    char	*rmt_pwheel;
};

struct pwstat_node
{
    PWHEEL	*pwheel;
    ALERT	*alert;
    short	requests;
    short	requests_last;		/* # when alert last sent */
    short	trigger;
    short	mounted;
};

#if	defined(OLD_MSG_STUFF)

#define CLIENT_NEW		1
#define CLIENT_PROTOCOL		2
#define CLIENT_TALKING		3

struct mque_node
{
    char		*msgbuf;
    unsigned int	size;
    struct mque_node	*next;
};

/*
**  ES Note:
**  This code isn't used any more (I think) but I am afraid to take
**  it out lest I break something.
*/
struct client_node
{
    ushort		uid;		/* User id of client		*/
    ushort		gid;		/* Group id of client		*/
    char		admin;		/* Non-zero if client is admin	*/
    char		state;		/* (CLIENT_... value)		*/
    FLT			*flt;		/* Linked list of fault actions */
    char		*fifo;		/* Name of client's fifo	*/
    char		*system;	/* Name of client's system	*/
    int			fd;		/* File-descriptor of open fifo	*/
    char		authcode[HEAD_AUTHCODE_LEN];
					/* ``Password'' to verify client*/
    MQUE		*mque;		/* Linked list of pending msgs.	*/
};

#else

#define	NODE		MESG
#define send		mputm

#endif

struct rstat_node
{
    long	status;
    long	rank;
    MESG	*md;
    
    char	*req_file;
    char	*slow;
    char	*fast;
    short	copies;		/* # copies interface is to make */    
    short	reason;		/* reason for failing _validate() */

    SECURE	*secure;
    REQUEST	*request;
    PSTATUS	*printer;
    SSTATUS	*system;
    FSTATUS	*form;
    char	*pwheel_name;
    PWSTATUS	*pwheel;
    EXEC	*exec;		/* Pointer to running filter or notify */

    char	*printer_type;
    char	*cpi;
    char	*lpi;
    char	*plen;
    char	*pwid;
    short	new_dest_status;
    PSTATUS	*new_dest;
    SUSPENDED	*move_suspend;
    SLOWPARM	*slowparm;	/* post-filtering parm values */

    RSTATUS	*next;
    RSTATUS	*prev;
};

# define	RSS_MARK	0x00000001
# define	RSS_RANK	0x00000002
# define	RSS_SENDREMOTE	0x00000004 /* request needs to be sent */
# define	RSS_PWMAND	0x00000008 /* pwheel must be mounted */
# define	RSS_GETSTATUS	0x00000010 /* get remote status */
# define	RSS_RECVSTATUS	0x00000020 /* waiting for remote status */
# define	RSS_PRINTSTAT	0x00000040 /* request is for status of printer,
					      not status of requests */
# define	RSS_FOR_USER	0x00000080 /* set when for a status request
					      for a user 'lpstat' - need to
					      quickly fail if remote is not
					      availble */
# define	RSS_REMOTE	0x00000100
# define	RSS_MOVE	0x00000200

struct form_node
{
    SCALED	plen;
    SCALED	pwid;
    SCALED	lpi;
    SCALED	cpi;
    int	np;
    char	*chset;
    short	mandatory;
    char	*rcolor;
    char	*comment;
    char	*conttype;
    char	*name;
    FALERT	alert;
};

# define	LP_EXEC		0
# define	LP_SCHED	1
# define	LP_NET		2
# define	LP_FILTER	3
# define	LP_PRINTER	4
# define	LP_ALERT	5
# define	LP_NOTIFY	6
# define	LP_SYSTEM	7
# define	LP_PWHEEL	8
# define	LP_FORM		9
# define	LP_LATER	10
# define	LP_ALARM	11

# define	LP_ACTIVE	0x080
# define	LP_ALERTING	0x100

# define	MINUTES			(60)

# define	USER_STATUS_EXPIRED	(2 * MINUTES)

# define	SYSTEM_STATUS_EXPIRED	(10 * MINUTES)

#define BEGIN_WALK_LOOP(PRS, CONDITION) \
	_BEGIN_WALK_LOOP("WALK", PRS, CONDITION) 

#define _BEGIN_WALK_LOOP(X, PRS, CONDITION) \
	{ \
		ENTRY (X) \
		register RSTATUS	*pnext = 0; \
		for (PRS = Request_List; PRS; PRS = pnext) { \
			pnext = PRS->next; \
			if (CONDITION) {

#define	END_WALK_LOOP \
			} \
		} \
	}

#define BEGIN_WALK_BY_PRINTER_LOOP(PRS, PPS)	\
	_BEGIN_WALK_LOOP("WALK_BY_PRINTER", PRS, PRS->printer == PPS)

#define BEGIN_WALK_BY_DEST_LOOP(PRS, DEST) \
	_BEGIN_WALK_LOOP("WALK_BY_DEST", PRS, STREQU(PRS->request->destination, DEST))

#define BEGIN_WALK_BY_FORM_LOOP(PRS, PFS) \
	_BEGIN_WALK_LOOP("WALK_BY_FORM", PRS, PRS->form == PFS)

#define	BEGIN_WALK_BY_PWHEEL_LOOP(PRS, NAME) \
	_BEGIN_WALK_LOOP("WALK_BY_PWHEEL", PRS, PRS->pwheel_name && STREQU(PRS->pwheel_name, NAME))
