/*
 * File dlpiut.h
 *
 *      Copyright (C) The Santa Cruz Operation, 1994-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#pragma comment(exestr, "@(#) @(#)dlpiut.h	28.1")

#ifndef dlpi_frame_test
#    define dlpi_frame_test
#endif

#define uchar unsigned char
#define ushort unsigned short

extern char		clientname[];	/* client's ipc name */
extern char		servername[];	/* servers ipc name */
extern int		sfd;			/* server's mailbox */
extern int		cfd;			/* client's mailbox */

#define MAXMSG	32000	/* tied to ipc.c IPC_BUFSIZ */
extern char		ibuf[];
extern char		obuf[];

#define ADDR_LEN	6
#define ROUTE_LEN	31
#define PARMS_LEN	32	/* kludge - really need size od struct param */
#define	TAB_SIZ		1024	/* kludge - shouldn't be limited */
#define STRLEN		80	/* max len of internal strings */
#define MSGLEN		128	/* max len of message string */

#define	ARGS	15	/* maximum number of arguments for a command */

#define MAXFRAME	2100	/* size of frame buffer */
#define BIFRAME		64
/*
 * one of these for each command
 */
typedef struct parsedef {
	char	p_name[12];		/* command name */
	char	*p_usage;		/* usage message */
	uchar	p_interface;		/* which interface allowed for */
	uchar	p_arg[ARGS];		/* list each argument */
	uchar	p_got[ARGS];		/* used during parsing */
} parse_t;

/*
 * argument structure, one for each possible argument
 * indexed by entries in the parse arrays.
 * 0 index is not used.
 */
typedef struct argdef {
	char	a_name[14];		/* name of argument */
	char	*a_usage;		/* usage message */
	uchar	a_type;			/* type of argument */
	void	*a_address;		/* ptr to where argument data goes */
} arg_t;

/*
 * allowable command types
 */
#define C_OPEN			0
#define	C_CLOSE			1
#define	C_BIND			2
#define	C_UNBIND		3
#define	C_SBIND			4
#define	C_ADDMCA		5
#define	C_DELMCA		6
#define	C_GETMCA		7
#define	C_GETADDR		8
#define	C_PROMISC		9
#define	C_SYNCSEND		10
#define	C_SYNCRECV		11
#define	C_SENDLOOP		12
#define	C_RECVLOOP		13
#define	C_SEND			14
#define C_TXID			15
#define C_RXID			16
#define C_TTEST			17
#define C_RTEST			18
#define	C_SRMODE		19
#define	C_SENDLOAD		20
#define	C_RECVLOAD		21
#define	C_SENDFILE		22
#define	C_RECVFILE		23
#define C_BILOOPMODE	24
#define C_BILOOPRX		25
#define C_BILOOPTX		26
#define C_SRCLR			27
#define C_SETSRPARMS	28
#define	C_GETRADDR		29
#define	C_SETADDR		30
#define	C_SETALLMCA		31
#define	C_DELALLMCA		32
/*
 * allowable argument types
 */
#define T_NONE			((uchar)-1)		/* bogus entry */
#define T_INT			0				/* basic integer */
#define	T_STR			1				/* string max of STRLEN */
#define	T_HEXINT		2				/* convert hex to an int */
#define	T_YESNO			3				/* yes = 1, no = 0 to a int */
#define	T_ADDR			4				/* a six byte hex address */
#define	T_TABLE			5				/* table of up addrs */
#define	T_FRAME			6				/* framing type specifier */
#define	T_INTERFACE		7				/* interface type specifier */
#define	T_MEDIA			8				/* media type specifier */
#define	T_SRMODE		9				/* source routing mode specifier */
#define	T_HMS			10				/* HHMMSS time format */
#define T_ROUTE			11				/* source route */
#define T_PARMS			12				/* sr parameters */
/*
 * define the command structure here, a spot for each argument is reserved.
 */

/* c_interface and p_interface */
#define I_DLPI			1
#define I_MDI			2
#define I_BOTH			3

/* c_media */
#define M_ETHERNET		0
#define M_TOKEN			1
#define M_NONE          2

/* c_framing */
#define	F_ETHERNET		0
#define	F_802_3			1
#define	F_XNS			2
#define	F_LLC1_802_3	3
#define	F_SNAP_802_3	4
#define	F_802_5			5
#define	F_SNAP_802_5	6
#define F_LLC1_802_5	7

typedef struct cmddef {
	/*
	 * The command is parsed into here
	 */
	int		c_cmd;

	/*
	 * The arguments for the command are parsed into here
	 */
	int		c_fd;						/* descriptor */
	char	c_device[80];				/* device name */
	int		c_interface;				/* interface dlpi/mdi */
	int		c_media;					/* media token/ethernet */
	int		c_sap;						/* sap */
	int		c_error;					/* error expected back */
	uchar	c_omchnaddr[ADDR_LEN];		/* other machine's addr */
	uchar	c_ourdstaddr[ADDR_LEN];		/* our dst addr */
	uchar	c_odstaddr[ADDR_LEN];		/* other's dst addr */
	int		c_tabsiz;					/* our multicast table size */
	uchar	c_table[TAB_SIZ][ADDR_LEN];	/* mc table data */
	char	c_msg[STRLEN];				/* sync or send msg */
	int		c_timeout;					/* sync or send timeout */
	int		c_framing;					/* framing type */
	int		c_delay;					/* delay between frames in ms */
	int		c_loop;						/* loopback frames expected */
	int		c_match;					/* wait for matching sync msg */
	int		c_len;						/* length for single frame */
	uchar	c_srmode;					/* source routing mode */
	int		c_windowsz;					/* send ahead windowsize load */
	int		c_verify;					/* % of frames to verify load */
	int		c_duration;					/* time in secconds HHMMSS */
	int		c_multisync;				/* multiple process sync */
	char	c_file[STRLEN];				/* file name */
	int		c_biloop;					/* two-way DLPI loopback*/
	uchar	c_route[ROUTE_LEN];			/* routing info */
	uchar	c_parms[PARMS_LEN];			/* ASR parameters */
} cmd_t;

/* we parse into this structure */
extern cmd_t cmd;

/* per file descriptor section */
#define MAXFD	2048	/* should allocate dynamically per NETXSAPMAXSAPS */

typedef struct fddef {
	int		f_open;					/* true if in use */
	char	f_name[STRLEN];			/* device name */
	uchar	f_ouraddr[ADDR_LEN];	/* our source address */
	uchar	f_addrgood;				/* source address good */
	int		f_fd;					/* file descriptor */
	int		f_interface;			/* interface in use */
	int		f_media;				/* media in use */
} fd_t;

extern fd_t	fds[MAXFD];
extern int	srvterm;					/* flag to terminate server */

/*
 * misc frame header stuff
 */
#define MAC_FC_LLC	0x40		/* token-ring field control (FC) */
#define MAC_AC		0x10		/* token-ring Access control */

#define MAC_UI		0x03		/* unnumbered information frame */

#define EMIN		60		/* minimum ethernet frame size */

#define SMALL_TIMEOUT	2		/* minimum retry in seconds */

#define LOOP_TIMEOUT	15		/* seconds to retry */

/*
 * misc stuff
 */
#define T_SENDER	'S'
#define T_RECEIVER	'R'
#define T_FILE		'X'

/*
 * load test stuff
 */
#define LOAD_DELAY	20		/* delay time to begin load test */

/* load test protocol */
#define LOAD_DATA	'D'		/* first data frame */
#define LOAD_SDATA	'd'		/* subsequent data frames */
#define	LOAD_ACK	'A'		/* ack to data frame */

#pragma pack(1)
/* load test protocol structure */
typedef struct loaddef {
	uchar	l_type;
	short	l_sap;
	int	l_seq;
} load_t;
#pragma pack()

/* load test statistics structures */
typedef struct txstatsdef {
	int okack;		/* acks ok */
	int noack;		/* ack timeout */
	int badack;		/* ack type byte not match */
	int sapack;		/* ack sap not match */
	int seqackhi;		/* acks with higher than expected seq # */
	int seqacklo;		/* acks with lower than expected seq # */
} txstats_t;

typedef struct rxstatsdef {
	int okframes;
	int badsap;
	int badlen;
	int badtype;
	int badseqhi;
	int badseqlo;
	int dupseq;
	int baddata;
} rxstats_t;

/*
 * The following are in api_dlpi.c
 */

int		dlpi_bind(int fd, int sap);
int		dlpi_sbind(int fd, int sap);
int		dlpi_unbind(int fd);
int		dlpi_addmca(int fd, uchar *mca);
int		dlpi_delmca(int fd, uchar *mca);
int		dlpi_getmca(int fd);
int		dlpi_getaddr(int fd, uchar *addr);
int		dlpi_writeframe(int fd, uchar *frame, int len);
int		dlpi_txframe(int fd, uchar *frame, int len);
int		dlpi_rxframe(int fd, uchar *frame, int maxlen);
int		dlpi_sendxid(int fd, int pfbit);
int		dlpi_flush(int fd);
int		dlpi_srmode(int fd, int srmode);
int		dlpi_srclr(int fd, uchar *addr);
int		dlpi_setsrparms(int fd, uchar *parms);
int		dlpi_biloopmode(int fd, int biloop);
int		dlpi_getmedia(int fd);
int		dlpi_getraddr(int fd, uchar *addr);
int		dlpi_setaddr(int fd, uchar *addr);
int		dlpi_enaballmca(int fd);
int		dlpi_disaballmca(int fd);

/*
 * The following are in api_mdi.c
 */

int		mdi_bind(int fd, int sap);
int		mdi_addmca(int fd, uchar *mca);
int		mdi_delmca(int fd, uchar *mca);
int		mdi_getmca(int fd);
int		tablecmp(uchar *ptr, int len);
int		mdi_getaddr(int fd, uchar *addr);
int		mdi_txframe(int fd, uchar *frame, int len);
int		mdi_rxframe(int fd, uchar *frame, int maxlen);
int		mdi_flush(int fd);
int		mdi_getmedia(int fd);
int		mdi_getraddr(int fd, uchar *addr);
int		mdi_setaddr(int fd, uchar *addr);
int		mdi_setallmca(int fd);
int		mdi_delallmca(int fd);
int		mdi_promisc(int fd);

/*
 * The following are in client.c
 */

void	client(int argc, const char * const *argv);

/*
 * The following are in dump.c
 */

void	dump(uchar *addr, int len);

/*
 * The following are in execute.c
 */

int		execute(void);
int		c_syncsend(int silent);
int		c_syncrecv(int silent);
int		c_srmode(void);

/*
 * The following are in framing.c
 */

int		build_txmsg(fd_t *fp, char *msg, int type);
int		build_txpattern(fd_t *fp, int len);
int		build_rxpattern(fd_t *fp, int len);
int		getaddr(fd_t *fp);
int		txframe(fd_t *fp);
int		rxframe(fd_t *fp, int timeout);
void	outrxmsg(fd_t *fp, int silent);
int		chkrxmsg(fd_t *fp, int type);
int		matchrxmsg(fd_t *fp, int len);
uchar	*getrxdataptr(fd_t *fp);
uchar	*gettxdataptr(fd_t *fp);
int		getrxdatalen(fd_t *fp);
int		rxdataoffset(fd_t *fp);
void	framesize(fd_t *fp, int *min, int *max);
void	makepattern(uchar *cp, int len);
int		frame_pcmp(fd_t *fp, int len);
int		frame_lcmp(fd_t *fp, int len, int offset);
int		api_bind(fd_t *fp, int sap);
void	api_flush(fd_t *fp);
int		bilooprx(fd_t *fp, int biloop, int framing, int sap, uchar *odstaddr,
								uchar *omchnaddr, uchar *route);
int		bilooptx(fd_t *fp, int biloop, int framing, int sap, uchar *ourdstaddr,
								uchar *route);

/*
 * The following are in ipc.c
 */

int		mcreat(char *qname);
int		mopen(char *qname);
int		mclose(int fd);
int		mdelete(int fd);
int		mrecv(int fd, char *buf, int mlen);
int		msend(int fd, char *buf, int mlen);

/*
 * The following are in load.c
 */

int		c_sendload(void);
int		c_recvload(void);

/*
 * The following are in parse.c
 */

int		parse(void);
int		verify(void);
void	usage(void);

/*
 * The following are in server.c
 */

int		server(void);
void	errlogprf(const char *Fmt, ...);
void	error(const char *Fmt, ...);
void	tableout(uchar *ptr, int len);
void	varout(char *var, char *msg);

