#ident "@(#)isdnmon.c	29.1"
#ident "$Header$"

/*
 * File isdnmon.c
 * ISDN Port Monitor
 *
 *	Copyright (C) The Santa Cruz Operation, 1996.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated
 *	as Confidential.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stropts.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sac.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/scoisdn.h>
#include <sys/scodlpi.h>
#include <signal.h>
#include <cs.h>
#include <dial.h>

#define	PMTAB_LINE_SIZE	256
#define FILENAME_SIZE	64
#define COMMENT		'#'
#define	TOKENSEP	':'
#define MAX_SVC		64
#define POLLTAB_SIZE	MAX_SVC + 1
#define POLL_DATA	POLLIN | POLLPRI
#define POLL_EVENTS	POLL_DATA | POLLHUP
#define POLL_TIMEOUT	30000		/* 30 seconds			*/
#define LISTEN_CONF_TIMEOUT	5000	/* 5 seconds			*/
#define ISDN_MSG_SIZE	1024
#define CALL_NUMBER_SIZE	64
#define SERVICE_PATH_SIZE	256
#define CONNECT_RETRY		10
#define CONNECT_TIMEOUT		60000 /* 60 seconds */
#define B_CHANNEL_SPEED		65536 /* 64K bps */
 
typedef enum disposition {		/* disposition after pmtab is read */
	UNCHANGED,			/* service entry was not changed   */
	MODIFY,				/* service entry was modified      */
	ADD,				/* service entry was added	   */
	REMOVE				/* service entry was removed       */
} dis_t;

typedef struct svc {
	char	*svc_tag;		/* null terminated service tag	*/
	char	*svc_flgs;		/* service flags		*/
	char	*svc_id;		/* user id for server to run as */
	char	*svc_res1;		/* reserved field		*/
	char	*svc_res2;		/* reserved field		*/
	char	*svc_scheme;		/* authentication scheme	*/
	char	*svc_pmspecific;	/* port monitor specific info	*/
	char	*svc_buffer;		/* above info stored here	*/
boolean_t	svc_enable_flg;		/* state of service enable flag */
boolean_t	svc_utmp;		/* state of utmp flag		*/
boolean_t	svc_active;		/* state of service		*/
dis_t		svc_disposition;	/* action to take on service	*/
struct	pollfd	*poll_p;		/* pointer to svc poll element	*/
struct 	svc	*next;			/* next service structure	*/
} svc_t;

svc_t	*svchead;
unsigned long	polltab_nfds;		/* nfds passed to poll()	*/
int	polltab_valid_fds;		/* polltab entries with valid fd */

char	pmtag[PMTAGSIZE];
int	pmstate;

char	*PMtab = "./_pmtab";
FILE	*pmtabfd;

char	*pmpipe = "_pmpipe";
int	pmfd;
struct	pollfd	*pollpmpipe;

char	*sacpipe = "../_sacpipe";
int	sacfd;

char	*pidfile = "_pid";
FILE	*pidfd;

struct	stat	statbuf;
int	debug;
int	calldebug;
	
FILE	*debugfd;
char	*debugfileprefix = "/var/saf/";
char	*debugfilesuffix = "/isdnmon.debug";
char	debugfile[FILENAME_SIZE];

FILE	*calldebugfd;
char	*calldebugfileprefix = "/var/saf/";
char	*calldebugfilemiddle = "/isdncall";
char	*calldebugfilesuffix = ".debug";
char	calldebugfile[FILENAME_SIZE];

char	*isdnfileprefix = "/dev/";
char	isdnfile[FILENAME_SIZE];

int	callfd;			/* used only by child call process */

struct	pmmsg	pm_msg;
struct	sacmsg	sac_msg;

char	line[POLLTAB_SIZE];
struct 	pollfd	polltab[MAX_SVC + 1];

isdnByte_t	isdn_msg[ISDN_MSG_SIZE];
isdnByte_t	isdn_data[ISDN_MSG_SIZE];

/* call handler process globals */

dial_service_t	call_type;
service_info_t	service_info;

isdn_pinfo_t pinfo_sync = PINFO_ISDN_SYNC;

char	caller_id[CALL_NUMBER_SIZE];
char	*caller_id_p;

char	called_id[CALL_NUMBER_SIZE];
char	*called_id_p;

char	service_path[SERVICE_PATH_SIZE];


isdnPLCI_t	connectedPLCI;
isdnNCCI_t	connectedNCCI;


void
logger(int logging, FILE *fd, char *fmt, ...)
{
/* struct	tm	local_time; */
va_list	ap;


/*	timestamp = time(0); */
/*	localtime_r(&timestamp, &local_time); */
/*	strftime(timebuf, 64, "%c", &local_time); */

	if (logging) {
/*		fprintf(fd, "%s ", timebuf); */
		va_start(ap, fmt);
		vfprintf(fd, fmt, ap);
		va_end(ap);
	}

}

/* Utility routines					*/

trace_msg(unsigned char *p, int len)
{
int	i;

	for (i = 0; i < len; i++) {
		logger(debug, debugfd, "%2.2X ", *p);
		p++;
	}
	logger(debug, debugfd, "\n");
}


void 
copy_struct(isdnByte_t **dst, isdnByte_t **src)
{
isdnByte_t	len;

	len = **src;
	**dst = **src;
	*dst += 1;
	*src += 1;
	memcpy(*dst, *src, len);
	*src += len;
	*dst += len;
}


/* pmtable (external _pmtab file) read routines				*/
/* svc table (internal representation of pmtable) routines	*/


svc_t *
create_svc_entry(char *line, char *startp) {
svc_t	*svc_p;
char	*p;
char	*nexttok;

	if ((svc_p = (svc_t *) malloc(sizeof(svc_t))) == NULL) {
		logger(debug, debugfd, "Cannot malloc service entry\n");
		return(NULL);
	}
	
	if ((p = strchr(startp, TOKENSEP )) == NULL) { /* service tag */
		logger(debug, debugfd, "no token scanning service tag\n");
		free(svc_p);
		return(NULL);
	}
	*p = 0;
	svc_p->svc_tag = startp;
	nexttok = ++p;

	if ((p = strchr(nexttok, TOKENSEP )) == NULL) { /* service flags */
		logger(debug, debugfd, "no token scanning service flags\n");
		free(svc_p);
		return(NULL);
	}
	*p = 0;
	svc_p->svc_flgs = nexttok;
	nexttok = ++p;

	if ((p = strchr(nexttok, TOKENSEP )) == NULL) { /* user id */
		logger(debug, debugfd, "no token scanning user id\n");
		free(svc_p);
		return(NULL);
	}
	*p = 0;
	svc_p->svc_id = nexttok;
	nexttok = ++p;

	if ((p = strchr(nexttok, TOKENSEP )) == NULL) { /* reserved field 1 */
		logger(debug, debugfd, "no token scanning reserved field 1\n");
		free(svc_p);
		return(NULL);
	}
	*p = 0;
	svc_p->svc_res1 = nexttok;
	nexttok = ++p;

	if ((p = strchr(nexttok, TOKENSEP )) == NULL) { /* reserved field 2 */
		logger(debug, debugfd, "no token scanning reserved field 2\n");
		free(svc_p);
		return(NULL);
	}
	*p = 0;
	svc_p->svc_res2 = nexttok;
	nexttok = ++p;

	if ((p = strchr(nexttok, TOKENSEP )) == NULL) { /* authentication */
		logger(debug, debugfd, "no token scanning authentication scheme\n");
		free(svc_p);
		return(NULL);
	}
	*p = 0;
	svc_p->svc_scheme = nexttok;
	nexttok = ++p;

	if ((p = strchr(nexttok, TOKENSEP )) != NULL) /* pm specific info */
		*p = 0;

	svc_p->svc_pmspecific = nexttok;

	svc_p->svc_buffer = line;
	svc_p->svc_enable_flg = B_TRUE;
	svc_p->svc_utmp = B_FALSE;

	return(svc_p);
}


void
check_flgs(svc_t *svc_p) {
char	*p;

	p = svc_p->svc_flgs;
	while (*p)  {
		switch (*p)  {
		case 'x':		/* service is turned off	*/
		case 'X':
			svc_p->svc_enable_flg = B_FALSE;
			break;
		case 'u':		/* create utmp entry		*/
			svc_p->svc_utmp = B_TRUE;
			break;
		default:
			logger(debug, debugfd,"unknown character %s in flgs\n", *p);
			break;
		}
		++p;
	}

}
	

void
add_svc_entry(svc_t *svc_p) {

	svc_p->svc_disposition = ADD;
	check_flgs(svc_p);
	svc_p->svc_active = B_FALSE;
	svc_p->poll_p = NULL;

	if (svchead == NULL) {
		svchead = svc_p;
		svc_p->next = NULL;
	} else {
		svc_p->next = svchead;
		svchead = svc_p;
	}

}


void
replace_svc_entry(svc_t *new) {
svc_t	*cur;
svc_t	*prev;

	new->svc_disposition = MODIFY;
	cur = svchead;
	while (cur != NULL) {
		if (strcmp(new->svc_tag, cur->svc_tag) == 0) { /* found it */

			new->svc_active = cur->svc_active;
			new->poll_p = cur->poll_p;
		  	new->next = cur->next;

			if (cur == svchead)
				svchead = new;
			else
				prev->next = new;

			free(cur->svc_buffer);
			free(cur);
			break;
		}
		prev = cur;
		cur = cur->next;
	}

}


void
remove_svc_entry(svc_t *svc_p) {
svc_t	*cur;
svc_t	*prev;

	cur = svchead;
	while (cur != NULL) {
		if (cur == svc_p) { /* found it */

			if (cur == svchead)
				svchead = cur->next;
			else
				prev->next = cur->next;

			free(cur->svc_buffer);
			free(cur);
			break;
		}
		prev = cur;
		cur = cur->next;
	}

}


void
existing_entry(svc_t *new, svc_t *cur) {

	if (strcmp(new->svc_flgs, cur->svc_flgs) != 0) {
		check_flgs(new);
		replace_svc_entry(new);
		return;
	}
	if (strcmp(new->svc_id, cur->svc_id) != 0) {
		replace_svc_entry(new);
		return;
	}

	if (strcmp(new->svc_res1, cur->svc_res1) != 0) {
		replace_svc_entry(new);
		return;
	}

	if (strcmp(new->svc_res2, cur->svc_res2) != 0) {
		replace_svc_entry(new);
		return;
	}

	if (strcmp(new->svc_scheme, cur->svc_scheme) != 0) {
		replace_svc_entry(new);
		return;
	}

	if (strcmp(new->svc_pmspecific, cur->svc_pmspecific) != 0) {
		replace_svc_entry(new);
		return;
	}
	cur->svc_disposition = UNCHANGED;

}


void
check_svc_add_or_mod(svc_t *svc_p) {
svc_t	*sp;

	sp = svchead;
	while (sp != NULL) {
		if (strcmp(svc_p->svc_tag, sp->svc_tag) == 0) {
			existing_entry(svc_p, sp);
			break;
		}
		sp = sp->next;
	}
	if (sp == NULL)
		add_svc_entry(svc_p);

}


void
print_svc_tab() {
svc_t	*svc_p;

	svc_p = svchead;
	while (svc_p != NULL) {
		logger(debug, debugfd, "%s:%s:%s:%s:%s:%s:%s:", svc_p->svc_tag,
					         svc_p->svc_flgs,
					         svc_p->svc_id,
					         svc_p->svc_res1,
					         svc_p->svc_res2,
					         svc_p->svc_scheme,
					         svc_p->svc_pmspecific);
		switch (svc_p->svc_enable_flg) {
			case B_TRUE:	logger(debug, debugfd, "en-TRUE:");
					break;

			case B_FALSE:	logger(debug, debugfd, "en-FALSE:");
					break;

			default:	logger(debug, debugfd, "en-INVALID");
					break;
		}

		switch (svc_p->svc_utmp) {
			case B_TRUE:	logger(debug, debugfd, "utmp-TRUE:");
					break;

			case B_FALSE:	logger(debug, debugfd, "utmp-FALSE:");
					break;

			default:	logger(debug, debugfd, "utmp-INVALID");
					break;
		}

		switch (svc_p->svc_active) {
			case B_TRUE:	logger(debug, debugfd, "act-TRUE:");
					break;

			case B_FALSE:	logger(debug, debugfd, "act-FALSE:");
					break;

			default:	logger(debug, debugfd, "act-INVALID");
					break;
		}

		switch (svc_p->svc_disposition) {
			case UNCHANGED:	logger(debug, debugfd, "UNCHANGED");
					break;

			case MODIFY:	logger(debug, debugfd, "MODIFY");
					break;

			case ADD:	logger(debug, debugfd, "ADD");
					break;

			case REMOVE:	logger(debug, debugfd, "REMOVE");
					break;

			default:	logger(debug, debugfd, "INVALID");
					break;

		}
		logger(debug, debugfd, "\n");
		svc_p = svc_p->next;
	}

}


void
set_disposition() {
svc_t	*sp;

	sp = svchead;
	while (sp != NULL) {
		sp->svc_disposition = REMOVE;
		sp = sp->next;
	}

}


int
get_pmtab_line(char **bufp, char **startp)
{
char	*line;
char	*p;
char	*bp;
	

	do {

		if ((line = (char *) malloc(PMTAB_LINE_SIZE)) == NULL) {
			logger(debug, debugfd, "Cannot malloc line buffer\n");
			return(-1);
		}
		*bufp = line;

		if (fgets(line, PMTAB_LINE_SIZE, pmtabfd) == NULL) {
			if (feof(pmtabfd)) {
				logger(debug, debugfd, "Reached EOF\n");
				return(-1);
			}
			if (ferror(pmtabfd)) {
				logger(debug, debugfd, "Error reading PMtab\n");
				return(-1);
			}
			logger(debug, debugfd, "Unknown error reading PMtab\n");
			return(-1);
		}

		if (*(line + strlen(line) - 1) != '\n') {
			logger(debug, debugfd, "line too long\n");
			return(-1);
		}
		*(line + strlen(line) - 1) = (char)0; /* delete newline	*/

		if (strlen(line) && (p = strchr(line, COMMENT)))
			*p = (char)0;		/* delete comments	*/
		if (!strlen(line))
			continue;

		bp = line;
		p = bp + strlen(bp) - 1;	/* bp->start; p->end	*/
		while ((p != bp) && (isspace(*p)))  {
			*p = (char)0;		/* delete terminating spaces */
			--p;
		}

		while (*bp)			/* del beginning white space*/
			if (isspace(*bp))
				++bp;
			else
				break;

		if (!strlen(bp))		/* anything left?	*/
			continue;

		*startp = bp;
		return(0);
	} while (1);
}


void
read_pmtab()
{
char	*line;
char	*startp;
svc_t	*svc_p;

	if ((pmtabfd = fopen(PMtab, "r")) == NULL) {
		logger(debug, debugfd, "PMtab open failed\n");
		return;
	}
/*	get_version(); */
	set_disposition();
 	while (get_pmtab_line(&line, &startp) == 0) {
		logger(debug, debugfd, "%s\n", startp);
		svc_p = create_svc_entry(line, startp);
		if (svc_p == NULL) {
			logger(debug, debugfd, "error creating service entry \n");
			continue;
		}
		check_svc_add_or_mod(svc_p);
	}
	
	print_svc_tab();

	fclose(pmtabfd);

	return;
}


/* poll table routines				*/

void
init_polltab()
{
int	i;

	for (i = 0; i < POLLTAB_SIZE; i++) {
		polltab[i].fd = -1;
		polltab[i].events = POLL_EVENTS;
		polltab[i].revents = 0;
	}
	polltab_nfds = 0;
	polltab_valid_fds = 0;

}


struct pollfd
*add_polltab(int fd)
{
int	i;

	if (polltab_valid_fds >= POLLTAB_SIZE) {
		logger(debug, debugfd, "add_polltab failed, polltab full\n");
		return (NULL);
	}

	for (i = 0; i < POLLTAB_SIZE; i++)
		if (polltab[i].fd == -1)
			break;

	polltab[i].fd = fd;

	polltab_valid_fds++;
	if (i >= polltab_nfds)
		polltab_nfds = i + 1;

	return (&polltab[i]);
}


int
remove_polltab(struct pollfd *rempolltab_p)
{
int	i;
int	j;

	i = rempolltab_p - polltab; /* get polltab index */
	if ((i >= polltab_nfds) || (i < 0)) { /* is index within valid range? */
		logger(debug, debugfd, "remove_polltab failed, pointer out of range\n");
		return (1);
	}

	if (polltab[i].fd == -1) { /* is polltab entry empty? */
		logger(debug, debugfd, "remove_polltab failed, polltab entry unused\n");
		return (1);
	}

	polltab[i].fd = -1; /* clear polltab entry */
	polltab[i].revents = 0;

	polltab_valid_fds--; /* decrement count of valid polltab entries */
	if ((i + 1) == polltab_nfds) { /* if removing last entry in array */
		for (j = i - 1; j >= 0; j--) /* search for new last entry */
			if (polltab[j].fd != -1) /* found it */
				break;
		polltab_nfds = j + 1;
	}

	return (0);

}


/* high level service routines		*/

void
stop_svc(svc_t *svc_p)
{
int	result;

	if (svc_p->svc_active == B_TRUE) {

		close(svc_p->poll_p->fd);
		result = remove_polltab(svc_p->poll_p);
		if (result != 0 )
			logger(debug, debugfd, "bad poll pointer given to remove_polltab while stopping service %s\n",
				svc_p->svc_tag);
		svc_p->poll_p = NULL;
		svc_p->svc_active = B_FALSE;
		logger(debug, debugfd, "%s service stopped\n", svc_p->svc_tag);
	}

}


int listen(int fd)
{
struct	strbuf	msg_ctl;
struct	strbuf	msg_data;
isdnByte_t	*msg_p;
isdn_msg_hdr_t	*hdr_p;
isdn_listen_req_t	*req_p;
struct		pollfd	fds[1];
int		poll_result;
int		flags = 0;
isdnInfo_t	Info;
int		expected_conf	= 1;

	msg_p = isdn_msg;

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;
	hdr_p->DL_prim = DL_ISDN_MSG;
	hdr_p->Cmd = ISDN_LISTEN;
	hdr_p->SubCmd = ISDN_REQ;
	hdr_p->MsgNum = expected_conf;
	msg_p += sizeof(isdn_msg_hdr_t);

	req_p = (isdn_listen_req_t *)msg_p;
	req_p->ctlr = 1;
	req_p->infoMask = 0;              /* No information messages */
	req_p->cipMask = ISDN_CIPMSK_ANY; /* any match */
	req_p->cipMask2 = 0;              /* reserved */
	msg_p += sizeof(isdnCtrlr_t) + sizeof(isdnInfoMsk_t)
			+ sizeof(isdnCIPmask_t) + sizeof(isdnCIPmask_t);

	*msg_p = 0; /* Calling party number */
	msg_p += 1;

	*msg_p = 0; /* Called party subaddress */
	msg_p += 1;

	hdr_p->Length = msg_p - isdn_msg - sizeof(isdnDword_t);

	msg_ctl.maxlen = 0;         /* Not used */
	msg_ctl.len = msg_p - isdn_msg;
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = 0;        /* Not used */
	msg_data.len = 0;
	msg_data.buf = 0;

	if (putmsg(fd, &msg_ctl, &msg_data, 0) < 0) {
		logger(debug, debugfd, "Listen Request putmsg failed error %d", errno);
		return -1;
	}

	fds[0].fd = fd;
	fds[0].events = POLL_DATA;
	fds[0].revents = 0;
	poll_result = poll(fds, 1, LISTEN_CONF_TIMEOUT);

	if (poll_result < 0) {
		logger(debug, debugfd, "poll failed waiting Listen Confirmation\n");
		return -1;
	}

	if (poll_result == 0) {
		logger(debug, debugfd, "Timeout waiting Listen Confirmation\n");
		return -1;
	}

	if ((fds[0].revents & POLL_DATA) == 0) {
		printf("Bad poll event mask waiting Listen Confirmation\n");
		return -1;
	}

	msg_ctl.maxlen = sizeof(isdn_msg);
	msg_ctl.len = 0;            /* Not used */
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = sizeof(isdn_data);
	msg_data.len = 0;           /* Not used */
	msg_data.buf = (void *)isdn_data;

	if (getmsg(fd, &msg_ctl, &msg_data, &flags) < 0) {
		logger(debug, debugfd, "Listen Confirmation getmsg failed error %d\n", errno);
		return -1;
	}

	if (msg_ctl.len <= 0 ) {
		logger(debug, debugfd, "No control data expecting Listen Confirmation\n");
		return -1;
	}

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;

	if (hdr_p->Length < (sizeof(isdn_msg_hdr_t) + sizeof(isdnCtrlr_t)
		+ sizeof(isdnInfo_t) - sizeof(isdnDword_t))) {
		logger(debug, debugfd, "Listen Confirmation message too small\n");
		return -1;
	}

	if (hdr_p->Cmd != ISDN_LISTEN) {
		logger(debug, debugfd, "Listen Confirmation command is not ISDN_LISTEN\n");
		return -1;
	}

	if (hdr_p->SubCmd != ISDN_CONF) {
		logger(debug, debugfd, "Listen Confirmation subcommand is not ISDN_CONF\n");
		return -1;
	}

	msg_p = isdn_msg;
	msg_p += sizeof(isdn_msg_hdr_t) + sizeof(isdnCtrlr_t);
	Info = *(isdnInfo_t *)msg_p;
	if (Info != 0) {
		logger(debug, debugfd, "Listen Confirmation error %2.2X\n", Info);
		return -1;
	}

	return 0;
}


void
start_svc(svc_t *svc_p)
{
int	isdnfd;
struct	strioctl	strioctl;
isdn_register_params_t	rp;

	if ((pmstate == PM_ENABLED) && (svc_p->svc_active == B_FALSE)
		&& (svc_p->svc_enable_flg == B_TRUE)) {

		/* open /dev/netx */
		strcpy(isdnfile, isdnfileprefix);
		strcat(isdnfile, svc_p->svc_tag);

		isdnfd = open(isdnfile, O_RDWR);
		if (isdnfd < 0) {
			logger(debug, debugfd, "Service %s not started\n", svc_p->svc_tag);
			logger(debug, debugfd, "Error %d opening %s for service %s\n",
					errno, isdnfile, svc_p->svc_tag);
			return;
       		}

		/* register stream */
		rp.level3cnt = 2;
		rp.datablkcnt = 100;
		rp.datablklen = 2048;
		strioctl.ic_cmd = ISDN_REGISTER;
		strioctl.ic_timout = 0;
		strioctl.ic_dp = (void *)&rp;
		strioctl.ic_len = sizeof(rp);
		if ((ioctl(isdnfd, I_STR, &strioctl)) < 0 ) {
			close(isdnfd);
			logger(debug, debugfd, "Service %s not started\n", svc_p->svc_tag);
			logger(debug, debugfd, "Register failed\n");
			return;
		}

		/* issue listen, wait for conf */
		
		if (listen(isdnfd) < 0) {
			close(isdnfd);
			logger(debug, debugfd, "Service %s not started\n", svc_p->svc_tag);
			return;
		}
			

		/* add to poll table */
		svc_p->poll_p = add_polltab(isdnfd);
		if (svc_p->poll_p == NULL) {
			close(isdnfd);
			logger(debug, debugfd, "Add poll table failed\n", svc_p->svc_tag);
			logger(debug, debugfd, "Service %s not started\n", svc_p->svc_tag);
			return;
		}

		svc_p->svc_active = B_TRUE;
		logger(debug, debugfd, "%s service started\n", svc_p->svc_tag);
	}

}



void
modify_svc(svc_t *svc_p)
{

	if (svc_p->svc_active == B_TRUE) {
		if (svc_p->svc_enable_flg == B_FALSE) {
			logger(debug, debugfd, "%s disabled\n", svc_p->svc_tag);
			stop_svc(svc_p);
		}
	} else { /* svc_active == B_FALSE */
		if (svc_p->svc_enable_flg == B_TRUE) {
			logger(debug, debugfd, "%s enabled\n", svc_p->svc_tag);
			start_svc(svc_p);
		}
	}

}


void
remove_svc(svc_t *svc_p)
{
	stop_svc(svc_p);
	remove_svc_entry(svc_p);

	logger(debug, debugfd, "%s removed\n", svc_p->svc_tag);

}


/* initialization routines			*/

void
pmtab_act()
{
svc_t	*svc_p;

	svc_p = svchead;
	while (svc_p != NULL) {
		switch (svc_p->svc_disposition) {
			case MODIFY:	modify_svc(svc_p);
					break;

			case ADD:	start_svc(svc_p);
					break;

			case REMOVE:	remove_svc(svc_p);
					break;

			case UNCHANGED:
			default:	break;

		}
		svc_p = svc_p->next;
	}

}


void
terminate()
{
svc_t	*svc_p;

	svc_p = svchead;
	while (svc_p != NULL) {
		stop_svc(svc_p);
		svc_p = svc_p->next;
	}
	logger(debug, debugfd, "isdnmon terminated\n");
	exit(1);

}


void
get_pmtag()
{
char	*env_p;

	/* Get my port monitor tag out of the environment		*/
	if ((env_p = getenv("PMTAG")) == NULL) {
		/* no place to write */
		fprintf(stderr, "Get PMTAG failed\n");
		exit(1);
	}
	strcpy(pmtag, env_p);

}


void
init_debugfile()
{
	if ((strlen(debugfileprefix) + strlen(pmtag) + strlen(debugfilesuffix)) >
		FILENAME_SIZE)
		exit(1);

	strcpy(debugfile, debugfileprefix);
	strcat(debugfile, pmtag);
	strcat(debugfile, debugfilesuffix);

	debug = FALSE;
	if (stat(debugfile, &statbuf) == 0) { /* debugfile exists, then log debug */
		if ((debugfd = fopen(debugfile, "a")) != NULL) {
			debug = TRUE;
			setvbuf(debugfd, NULL, _IOLBF, 0);
		}
	}
}


void
get_pmstate()
{
char	*pmstate_p;

	if ((pmstate_p = getenv("ISTATE")) == NULL) {
		logger(debug, debugfd, "Get ISTATE failed\n");
		terminate();
	}
	
	if (!strcmp(pmstate_p, "enabled"))
		pmstate = PM_ENABLED;
	else
		pmstate = PM_DISABLED;


}				


void
setup_pidfile()
{

	pidfd = fopen(pidfile, "w");
	if (pidfd == NULL) {
		logger(debug, debugfd, "Could not open pid file\n");
		terminate();
	}
	
	if (lockf(fileno(pidfd), F_TEST, 0) < 0) {
		logger(debug, debugfd, "pid file already locked\n");
		terminate();
	}
	fprintf(pidfd, "%d", getpid());
	rewind(pidfd);
	if (lockf(fileno(pidfd), F_LOCK, 0) < 0) {
		logger(debug, debugfd, "pid file lock failed\n");
		terminate();
	}
	
}	 


void
open_sac_pipes()
{

	pmfd = open(pmpipe, O_RDWR);
	if (pmfd < 0) {
		logger(debug, debugfd, "Could not open pmpipe\n");
		terminate();
	}

	sacfd = open(sacpipe, O_RDWR);
	if (sacfd < 0) {
		logger(debug, debugfd, "Could not open sacpipe\n");
		terminate();
	}

	/* initialize constants in return message */
	strcpy(pm_msg.pm_tag, pmtag);
	pm_msg.pm_size = 0;
	pm_msg.pm_maxclass = 1;

	pollpmpipe = add_polltab(pmfd); /* put pmpipe in poll list */
	if (pollpmpipe == NULL) {
		logger(debug, debugfd, "Could not allocate poll struct for pmpipe\n");
		terminate();
	}

}


void
init()
{
	pmstate = PM_STARTING;
	svchead = NULL;
	sigset(SIGCHLD, SIG_IGN);
	get_pmtag();
	init_debugfile();
	logger(debug, debugfd, "isdnmon invoked\n");
	get_pmstate();
	setup_pidfile();
	init_polltab();
	open_sac_pipes();
	read_pmtab();
	if (pmstate == PM_ENABLED)
		pmtab_act();

}

/* call handler process routines				*/

/* All these routines are executed as a			*/
/* call handling process spawned by isdnmon.	*/

void stop_listen()
{
struct	strbuf	msg_ctl;
struct	strbuf	msg_data;
isdnByte_t	*msg_p;
isdn_msg_hdr_t	*hdr_p;
isdn_listen_req_t	*req_p;
int		flags = 0;
int		expected_conf	= 1;

	msg_p = isdn_msg;

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;
	hdr_p->DL_prim = DL_ISDN_MSG;
	hdr_p->Cmd = ISDN_LISTEN;
	hdr_p->SubCmd = ISDN_REQ;
	hdr_p->MsgNum = expected_conf;
	msg_p += sizeof(isdn_msg_hdr_t);

	req_p = (isdn_listen_req_t *)msg_p;
	req_p->ctlr = 1;
	req_p->infoMask = 0;               /* No information messages */
	req_p->cipMask = ISDN_CIPMSK_NONE; /* stop listening */
	req_p->cipMask2 = 0;               /* reserved */
	msg_p += sizeof(isdnCtrlr_t) + sizeof(isdnInfoMsk_t)
			+ sizeof(isdnCIPmask_t) + sizeof(isdnCIPmask_t);

	*msg_p = 0; /* Calling party number */
	msg_p += 1;

	*msg_p = 0; /* Called party subaddress */
	msg_p += 1;

	hdr_p->Length = msg_p - isdn_msg - sizeof(isdnDword_t);

	msg_ctl.maxlen = 0;         /* Not used */
	msg_ctl.len = msg_p - isdn_msg;
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = 0;        /* Not used */
	msg_data.len = 0;
	msg_data.buf = 0;

	if (putmsg(callfd, &msg_ctl, &msg_data, 0) < 0) {
		logger(calldebug, calldebugfd, "Stop Listen Request putmsg failed error %d", errno);
	}
	/* Don't get ISDN_LISTEN.ISDN_CONF, isdnmon is in M_DATA mode  */
	/* and is in connect_b3_active state.  It might pick up        */
	/* application data instead of listen confirmation.            */
	/* Dlpi module will free ISDN_LISTEN.ISDN_CONF.                */
	/* If ISDN_LISTEN.ISDN_CONF fails, dlpi module will free       */
	/* any ISDN_CONNECT.ISDN_IND messages when in M_DATA mode.     */
}


void
mdata_ioctl()
{
struct	strioctl	strioctl;
isdnNCCI_t	mdata_NCCI;

	mdata_NCCI = connectedNCCI;
	strioctl.ic_cmd = DLPI_ISDN_MDATA_ON;
	strioctl.ic_timout = 0;
	strioctl.ic_dp = (void *)&mdata_NCCI;
	strioctl.ic_len = sizeof(mdata_NCCI);
	if ((ioctl(callfd, I_STR, &strioctl)) < 0 ) {
		logger(calldebug, calldebugfd, "DLPI_ISDN_MDATA_ON ioctl failed\n");
		exit(0);
	}
}


void
connect_b3_active_ind()
{
struct	strbuf	msg_ctl;
struct	strbuf	msg_data;
isdnByte_t	*msg_p;
isdn_msg_hdr_t	*hdr_p;
struct		pollfd	fds[1];
int		poll_result;
int		flags = 0;
isdnWord_t	expected_resp;
int		message_retry = 0;

	while (message_retry < CONNECT_RETRY) {

		fds[0].fd = callfd;
		fds[0].events = POLL_DATA;
		fds[0].revents = 0;
		poll_result = poll(fds, 1, CONNECT_TIMEOUT);

		if (poll_result < 0) {
			logger(calldebug, calldebugfd, "poll failed waiting Connect_B3_Active Indication\n");
			exit(0);
		}

		if (poll_result == 0) {
			logger(calldebug, calldebugfd, "Timeout waiting Connect_B3_Active Indication\n");
			exit(0);
		}

		if ((fds[0].revents & POLL_DATA) == 0 ) {
			logger(calldebug, calldebugfd, "Bad poll event mask waiting Connect_B3_Active Indication\n");
			exit(0);
		}

		msg_ctl.maxlen = sizeof(isdn_msg);
		msg_ctl.len = 0;            /* Not used */
		msg_ctl.buf = (void *)isdn_msg;

		msg_data.maxlen = sizeof(isdn_data);
		msg_data.len = 0;           /* Not used */
		msg_data.buf = (void *)isdn_data;

		if (getmsg(callfd, &msg_ctl, &msg_data, &flags) < 0) {
			logger(calldebug, calldebugfd, "Connect_B3_Active Indication getmsg failed");
			exit(0);
		}

		if (msg_ctl.len <= 0 ) {
			logger(calldebug, calldebugfd, "No control data expecting Connect_B3_Active Indication\n");
			exit(0);
		}

		hdr_p = (isdn_msg_hdr_t *)isdn_msg;

		if (hdr_p->Cmd != ISDN_CONNECT_B3_ACTIVE) {
			logger(calldebug, calldebugfd, "Command=%2.2X is not ISDN_CONNECT_B3_ACTIVE\n", hdr_p->Cmd);
			message_retry++;
			continue;
		}

		if (hdr_p->SubCmd != ISDN_IND) {
			logger(calldebug, calldebugfd, "Subcommand=%2.2X is not ISDN_IND\n", hdr_p->SubCmd);
			message_retry++;
			continue;
		}

		break;
	}

	if (message_retry >= CONNECT_RETRY) {
		logger(calldebug, calldebugfd, "Connect_B3_Active message retry limit reached\n");
		exit(0);
	}
	if (hdr_p->Length < (sizeof(isdn_msg_hdr_t) + sizeof(isdnNCCI_t) + 1
				- sizeof(isdnDword_t))) {
		logger(calldebug, calldebugfd, "Connect_B3_Active Indication message too small\n");
		exit(0);
	}

	mdata_ioctl(); /* M_DATA or M_HANGUP upstream only now */

	expected_resp = hdr_p->MsgNum;

	msg_p = isdn_msg;
	msg_p += sizeof(isdn_msg_hdr_t);
	if (*(isdnNCCI_t *)msg_p != connectedNCCI) {
		logger(calldebug, calldebugfd, "Bad NCCI in Connect_B3_Active Indication\n");
		exit(0);
	}

	msg_p = isdn_msg;

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;
	hdr_p->DL_prim = DL_ISDN_MSG;
	hdr_p->Cmd = ISDN_CONNECT_B3_ACTIVE;
	hdr_p->SubCmd = ISDN_RESP;
	hdr_p->MsgNum = expected_resp;
	msg_p += sizeof(isdn_msg_hdr_t);

	*(isdnNCCI_t *)msg_p = connectedNCCI;
	msg_p += sizeof(isdnNCCI_t);

	hdr_p->Length = msg_p - isdn_msg - sizeof(isdnDword_t);

	msg_ctl.maxlen = 0;         /* Not used */
	msg_ctl.len = msg_p - isdn_msg;
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = 0;        /* Not used */
	msg_data.len = 0;
	msg_data.buf = 0;

	if (putmsg(callfd, &msg_ctl, &msg_data, 0) < 0) {
		logger(calldebug, calldebugfd, "Connect_B3_Active Response putmsg failed\n");
		exit(0);
	}

}


void
connect_b3_ind()
{
struct	strbuf	msg_ctl;
struct	strbuf	msg_data;
isdnByte_t	*msg_p;
isdn_msg_hdr_t	*hdr_p;
struct		pollfd	fds[1];
int		poll_result;
int		flags = 0;
isdnWord_t	expected_resp;
int		message_retry = 0;

	while (message_retry < CONNECT_RETRY) {

		fds[0].fd = callfd;
		fds[0].events = POLL_DATA;
		fds[0].revents = 0;
		poll_result = poll(fds, 1, CONNECT_TIMEOUT);

		if (poll_result < 0) {
			logger(calldebug, calldebugfd, "poll failed waiting Connect_B3 Indication\n");
			exit(0);
		}

		if (poll_result == 0) {
			logger(calldebug, calldebugfd, "Timeout waiting Connect_B3 Indication\n");
			exit(0);
		}

		if ((fds[0].revents & POLL_DATA) == 0 ) {
			logger(calldebug, calldebugfd, "Bad poll event mask waiting Connect_B3 Indication\n");
			exit(0);
		}

		msg_ctl.maxlen = sizeof(isdn_msg);
		msg_ctl.len = 0;            /* Not used */
		msg_ctl.buf = (void *)isdn_msg;

		msg_data.maxlen = sizeof(isdn_data);
		msg_data.len = 0;           /* Not used */
		msg_data.buf = (void *)isdn_data;

		if (getmsg(callfd, &msg_ctl, &msg_data, &flags) < 0) {
			logger(calldebug, calldebugfd, "Connect_B3 Indication getmsg failed");
			exit(0);
		}

		if (msg_ctl.len <= 0 ) {
			logger(calldebug, calldebugfd, "No control data expecting Connect_B3 Indication\n");
			exit(0);
		}

		hdr_p = (isdn_msg_hdr_t *)isdn_msg;

		if (hdr_p->Cmd != ISDN_CONNECT_B3) {
			logger(calldebug, calldebugfd, "Command=%2.2X is not ISDN_CONNECT_B3\n", hdr_p->Cmd);
			message_retry++;
			continue;
		}

		if (hdr_p->SubCmd != ISDN_IND) {
			logger(calldebug, calldebugfd, "Subcommand=%2.2X is not ISDN_IND\n", hdr_p->SubCmd);
			message_retry++;
			continue;
		}

		break;
	}

	if (message_retry >= CONNECT_RETRY) {
		logger(calldebug, calldebugfd, "Connect_B3 message retry limit reached\n");
		exit(0);
	}
	
	if (hdr_p->Length < (sizeof(isdn_msg_hdr_t) + sizeof(isdnNCCI_t) + 1
						- sizeof(isdnDword_t))) {
		logger(calldebug, calldebugfd, "Connect_B3 Indication message too small\n");
		exit(0);
	}
	expected_resp = hdr_p->MsgNum;

	msg_p = isdn_msg;
	msg_p += sizeof(isdn_msg_hdr_t);
	connectedNCCI = *(isdnNCCI_t *)msg_p;

	msg_p = isdn_msg;

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;
	hdr_p->DL_prim = DL_ISDN_MSG;
	hdr_p->Cmd = ISDN_CONNECT_B3;
	hdr_p->SubCmd = ISDN_RESP;
	hdr_p->MsgNum = expected_resp;
	msg_p += sizeof(isdn_msg_hdr_t);

	*(isdnNCCI_t *)msg_p = connectedNCCI;
	msg_p += sizeof(isdnNCCI_t);

	*(isdnReject_t *)msg_p = ISDN_REJECT_OK;
	msg_p += sizeof(isdnReject_t);

	*msg_p = 0; /* empty NCPI structure */
	msg_p += 1;

	hdr_p->Length = msg_p - isdn_msg - sizeof(isdnDword_t);

	msg_ctl.maxlen = 0;         /* Not used */
	msg_ctl.len = msg_p - isdn_msg;
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = 0;        /* Not used */
	msg_data.len = 0;
	msg_data.buf = 0;

	if (putmsg(callfd, &msg_ctl, &msg_data, 0) < 0) {
		logger(calldebug, calldebugfd, "Connect_B3 Response putmsg failed\n");
		exit(0);
	}

}

	
void
connect_active_ind()
{
struct	strbuf	msg_ctl;
struct	strbuf	msg_data;
isdnByte_t	*msg_p;
isdn_msg_hdr_t	*hdr_p;
struct		pollfd	fds[1];
int		poll_result;
int		flags = 0;
isdnWord_t	expected_resp;
int		message_retry = 0;

	while (message_retry < CONNECT_RETRY) {

		fds[0].fd = callfd;
		fds[0].events = POLL_DATA;
		fds[0].revents = 0;
		poll_result = poll(fds, 1, CONNECT_TIMEOUT);

		if (poll_result < 0) {
			logger(calldebug, calldebugfd, "poll failed waiting Connect_Active Indication\n");
			exit(0);
		}

		if (poll_result == 0) {
			logger(calldebug, calldebugfd, "Timeout waiting Connect_Active Indication\n");
			exit(0);
		}

		if ((fds[0].revents & POLL_DATA) == 0 ) {
			logger(calldebug, calldebugfd, "Bad poll event mask waiting Connect_Active Indication\n");
			exit(0);
		}

		msg_ctl.maxlen = sizeof(isdn_msg);
		msg_ctl.len = 0;            /* Not used */
		msg_ctl.buf = (void *)isdn_msg;

		msg_data.maxlen = sizeof(isdn_data);
		msg_data.len = 0;           /* Not used */
		msg_data.buf = (void *)isdn_data;

		if (getmsg(callfd, &msg_ctl, &msg_data, &flags) < 0) {
			logger(calldebug, calldebugfd, "Connect_Active Indication getmsg failed");
			exit(0);
		}

		if (msg_ctl.len <= 0 ) {
			logger(calldebug, calldebugfd, "No control data expecting Connect_Active Indication\n");
			exit(0);
		}

		hdr_p = (isdn_msg_hdr_t *)isdn_msg;

		if (hdr_p->Cmd != ISDN_CONNECT_ACTIVE) {
			logger(calldebug, calldebugfd, "Command=%2.2X is not ISDN_CONNECT_ACTIVE\n", hdr_p->Cmd);
			message_retry++;
			continue;
		}

		if (hdr_p->SubCmd != ISDN_IND) {
			logger(calldebug, calldebugfd, "Subcommand=%2.2X is not ISDN_IND\n", hdr_p->SubCmd);
			message_retry++;
			continue;
		}

		break;
	}

	if (message_retry >= CONNECT_RETRY) {
		logger(calldebug, calldebugfd, "Connect_Active message retry limit reached\n");
		exit(0);
	}
	
	if (hdr_p->Length < (sizeof(isdn_msg_hdr_t) + sizeof(isdnPLCI_t)
						- sizeof(isdnDword_t))) {
		logger(calldebug, calldebugfd, "Connect_Active Indication message too small\n");
		exit(0);
	}

	expected_resp = hdr_p->MsgNum;

	msg_p = isdn_msg;
	msg_p += sizeof(isdn_msg_hdr_t);
	if (*(isdnPLCI_t *)msg_p != connectedPLCI) {
		logger(calldebug, calldebugfd, "Bad PLCI in Connect_Active Indication\n");
		exit(0);
	}

	msg_p = isdn_msg;

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;
	hdr_p->DL_prim = DL_ISDN_MSG;
	hdr_p->Cmd = ISDN_CONNECT_ACTIVE;
	hdr_p->SubCmd = ISDN_RESP;
	hdr_p->MsgNum = expected_resp;
	msg_p += sizeof(isdn_msg_hdr_t);

	*(isdnPLCI_t *)msg_p = connectedPLCI;
	msg_p += sizeof(isdnPLCI_t);

	hdr_p->Length = msg_p - isdn_msg - sizeof(isdnDword_t);

	msg_ctl.maxlen = 0;         /* Not used */
	msg_ctl.len = msg_p - isdn_msg;
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = 0;        /* Not used */
	msg_data.len = 0;
	msg_data.buf = 0;

	if (putmsg(callfd, &msg_ctl, &msg_data, 0) < 0) {
		logger(calldebug, calldebugfd, "Connect_Active Response putmsg failed\n");
		exit(0);
	}

}


void
reject_call()
{
struct	strbuf	msg_ctl;
struct	strbuf	msg_data;
isdn_msg_hdr_t	*hdr_p;
isdnByte_t	*msg_p;
isdnWord_t	expected_resp;
isdnPLCI_t	rejectPLCI;

	logger(debug, debugfd, "entered reject call\n");
	hdr_p = (isdn_msg_hdr_t *)isdn_msg;
	expected_resp = hdr_p->MsgNum;

	msg_p = isdn_msg;
	msg_p += sizeof(isdn_msg_hdr_t);
	rejectPLCI = *(isdnPLCI_t *)msg_p;

	hdr_p->DL_prim = DL_ISDN_MSG;
	hdr_p->Cmd = ISDN_CONNECT;
	hdr_p->SubCmd = ISDN_RESP;
	hdr_p->MsgNum = expected_resp;

	msg_p = isdn_msg;
	msg_p += sizeof(isdn_msg_hdr_t);

	*(isdnPLCI_t *)msg_p = rejectPLCI;
	msg_p += sizeof(isdnPLCI_t);

	*(isdnReject_t *)msg_p = ISDN_REJECT_NORMAL;
	msg_p += sizeof(isdnReject_t);

	memset(msg_p, 0, 5);	/* empty B protocol,			*/
				/*       Connected number, 		*/
				/*       Connected subaddress,		*/
				/*       LLC,				*/
				/*       Additional Info structures	*/
	msg_p += 5;

	hdr_p->Length = msg_p - isdn_msg - sizeof(isdnDword_t);

	msg_ctl.maxlen = 0;         /* Not used */
	msg_ctl.len = msg_p - isdn_msg;
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = 0;        /* Not used */
	msg_data.len = 0;
	msg_data.buf = 0;

	if (putmsg(callfd, &msg_ctl, &msg_data, 0) < 0) {
		logger(debug, debugfd, "Reject Connect Response putmsg failed\n");
		return;
	}

}


void
accept_call()
{
struct	strbuf	msg_ctl;
struct	strbuf	msg_data;
isdn_msg_hdr_t	*hdr_p;
isdnByte_t		*msg_p;
isdnWord_t		expected_resp;
isdnByte_t		*pinfo_p;

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;
	expected_resp = hdr_p->MsgNum;

	msg_p = isdn_msg;
	msg_p += sizeof(isdn_msg_hdr_t);
	connectedPLCI = *(isdnPLCI_t *)msg_p;

	hdr_p->DL_prim = DL_ISDN_MSG;
	hdr_p->Cmd = ISDN_CONNECT;
	hdr_p->SubCmd = ISDN_RESP;
	hdr_p->MsgNum = expected_resp;

	msg_p = isdn_msg;
	msg_p += sizeof(isdn_msg_hdr_t);

	*(isdnPLCI_t *)msg_p = connectedPLCI;
	msg_p += sizeof(isdnPLCI_t);

	*(isdnReject_t *)msg_p = ISDN_REJECT_OK;
	msg_p += sizeof(isdnReject_t);

	/* B protocol structrue */

	pinfo_p = (isdnByte_t *)&pinfo_sync.protocol_info;
	copy_struct(&msg_p, &pinfo_p); /* B protocol */

	memset(msg_p, 0, 2);	/* empty Connected number          */
	msg_p += 2;				/* empty Connected subaddress      */

	pinfo_p += *pinfo_p + 1; /* move past BC structure in pinfo */

	copy_struct(&msg_p, &pinfo_p); /* LLC */

	pinfo_p += *pinfo_p + 1; /* move past HLC structure in pinfo */

	copy_struct(&msg_p, &pinfo_p); /* Additional Info */

	hdr_p->Length = msg_p - isdn_msg - sizeof(isdnDword_t);

	msg_ctl.maxlen = 0;         /* Not used */
	msg_ctl.len = msg_p - isdn_msg;
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = 0;        /* Not used */
	msg_data.len = 0;
	msg_data.buf = 0;

	if (putmsg(callfd, &msg_ctl, &msg_data, 0) < 0) {
		logger(calldebug, calldebugfd, "Accept Connect Response putmsg failed\n");
		exit(0);
	}

}


void
call_invoke()
{
int	invoke_pid;

	service_info.caller_id = caller_id_p;
	service_info.type = call_type;
	service_info.speed = B_CHANNEL_SPEED;
	service_info.device_name = isdnfile;
	service_info.pinfo_len = sizeof(isdnNCCI_t);
	service_info.pinfo = &connectedNCCI;
	if ((invoke_pid 
	= ics_call_invoke(callfd, &service_info, service_path))	 < 0) {
		logger(calldebug, calldebugfd, "ics_call_invoke failed errno=%d\n", errno);
		logger(calldebug, calldebugfd, "service path=%s\n", service_path);
	}
	logger(calldebug, calldebugfd, "invoked service %s pid %d\n ",
									service_path,
									invoke_pid);
}


int
get_call_type()
{
isdnByte_t	*msg_p;

	/* determine call type */
	msg_p = isdn_msg;
	msg_p += sizeof(isdn_msg_hdr_t) + sizeof(isdnPLCI_t);
	if (*(isdnCIPvalue_t *)msg_p == ISDN_CIPVAL_UNRESTRICTED) {
		call_type = DIAL_ISDN_SYNC;
		logger(calldebug, calldebugfd, "call type= unrestricted digital information ISDN_SYNC\n");
		return(0);
	}
	logger(calldebug, calldebugfd, "call type not supported, CIP value=%d\n",
										*(isdnCIPvalue_t *)msg_p);
	return(-1);

}


int
call_filter()
{
isdn_msg_hdr_t	*hdr_p;
isdnByte_t	*msg_p;
cf_value_t	call_filter_return;
ulong	call_id_len;

	/* determine caller id */
	msg_p = isdn_msg;
	msg_p += sizeof(isdn_msg_hdr_t) + sizeof(isdnPLCI_t)
			+ sizeof(isdnCIPvalue_t);

	/* Called party number, get it now, it is used later */
	if ((*msg_p == 0) || (*msg_p == 1)) {
		called_id_p = NULL;		/* no number */
		msg_p += *msg_p + 1;
	} else {
		call_id_len = *msg_p - 1;
		if (call_id_len > (CALL_NUMBER_SIZE - 1)) {
			logger(calldebug, calldebugfd, "reject call, called id too long\n");
			reject_call();
			return(-1);
		}

		msg_p += 2;		/* skip over numbering plan id */
		strncpy(called_id, (char *)msg_p, call_id_len);
							/* terminate string */
		memset(called_id + call_id_len, 0, 1);
		called_id_p = called_id;
		msg_p += call_id_len;
	}


	/* Calling party number - caller id */	
	if (*msg_p <= 2)
		caller_id_p = NULL;		/* no number */
	else {
		call_id_len = *msg_p - 2;
		if (call_id_len > (CALL_NUMBER_SIZE - 1)) {
			logger(calldebug, calldebugfd, "reject call, caller id too long\n");
			reject_call();
			return(-1);
		}

		msg_p += 3;		/* skip over numbering plan id */
		   	/* and presentation and screening indicator	*/
		strncpy(caller_id, (char *)msg_p, call_id_len);
							/* terminate string */
		memset(caller_id + call_id_len, 0, 1);
		caller_id_p = caller_id;
	}

	logger(calldebug, calldebugfd, "caller number = %s\n", caller_id_p);
	logger(calldebug, calldebugfd, "called number = %s\n", called_id_p);
	call_filter_return = ics_call_filter(caller_id_p, call_type);

	if (call_filter_return == ICS_REJECT) {
		logger(calldebug, calldebugfd, "reject call, ics_call_filter reject\n");
		reject_call();
		return(-1);
	}

	return(0);

}


int
call_service()
{

	ics_call_service(call_type, called_id_p, isdnfile, service_path);
	if (strlen(service_path) == 0) {
		/* try service path for any device */
		ics_call_service(call_type, NULL, NULL, service_path);
		if (strlen(service_path) == 0) {
			logger(calldebug, calldebugfd, "reject call, no service path\n");
			reject_call();
			return (-1);
		}
	}

	return (0);

}
	

void
process_call()
{

	if ((strlen(calldebugfileprefix) + strlen(pmtag)
			+ strlen(calldebugfilemiddle) 
			+ strlen(calldebugfilesuffix)) > FILENAME_SIZE)
		exit(1);

	strcpy(calldebugfile, calldebugfileprefix);
	strcat(calldebugfile, pmtag);
	strcat(calldebugfile, calldebugfilemiddle);
	strcat(calldebugfile, calldebugfilesuffix);

	calldebug = FALSE;
	if (stat(calldebugfile, &statbuf) == 0) {
	/* calldebug file exists, then log call debug */
		sprintf(calldebugfile, "%s%s%s%d%s\0", calldebugfileprefix
												, pmtag
												, calldebugfilemiddle
												, getpid()
												, calldebugfilesuffix);
		if ((calldebugfd = fopen(calldebugfile, "w")) != NULL) {
			calldebug = TRUE;
			setvbuf(calldebugfd, NULL, _IOLBF, 0);
		}
	}

	logger(calldebug, calldebugfd, "process_call invoked\n");

	if (get_call_type() < 0)
		exit(0);

	if (call_filter() < 0)
		exit(0);

	if (call_service() < 0)
		exit(0);

	accept_call();
	connect_active_ind();
	connect_b3_ind();
	connect_b3_active_ind();
	stop_listen();
	call_invoke();
	close(callfd);
	logger(calldebug, calldebugfd, "process_call terminated\n");
	exit(0);

}
/* end call handler process			*/


/*  isdnmon's main device message handling routine */
/*  Call handling process my be invoked here		*/

void
process_msg(svc_t *svc_p)
{
struct	strbuf	msg_ctl;
struct	strbuf	msg_data;
isdn_msg_hdr_t	*hdr_p;
int		flags = 0;
pid_t	 ch_pid;

	logger(debug, debugfd, "entered process_msg\n");
	/* check for connect indication */
	msg_ctl.maxlen = sizeof(isdn_msg);
	msg_ctl.len = 0;            /* Not used */
	msg_ctl.buf = (void *)isdn_msg;

	msg_data.maxlen = sizeof(isdn_data);
	msg_data.len = 0;           /* Not used */
	msg_data.buf = (void *)isdn_data;

	if (getmsg(svc_p->poll_p->fd, &msg_ctl, &msg_data, &flags) < 0) {
		logger(debug, debugfd, "%s Connect Indication getmsg failed\n", svc_p->svc_tag);
		return;
	}

	if (msg_ctl.len <= 0 ) {
		logger(debug, debugfd, "%s No control data expecting Connect Indication\n", svc_p->svc_tag);
		return;
	}

	hdr_p = (isdn_msg_hdr_t *)isdn_msg;

	if (hdr_p->Cmd != ISDN_CONNECT) {
		logger(debug, debugfd, "%s Command=%2.2X is not CONNECT\n",
						hdr_p->Cmd, svc_p->svc_tag);
		return;
	}

	if (hdr_p->SubCmd != ISDN_IND) {
		logger(debug, debugfd, "%s Subcommand=%2.2X is not IND\n",
						hdr_p->SubCmd, svc_p->svc_tag);
		return;
	}
	
	if (hdr_p->Length < (sizeof(isdn_msg_hdr_t) + sizeof(isdnPLCI_t)
			+ sizeof(isdnCIPvalue_t) - sizeof(isdnDword_t)
			+ 8)) { /* 8 empty structures */
		logger(debug, debugfd, "%s Connect Indication message too small\n",
						svc_p->svc_tag);
		return;
	}

	/* Got connect indication */
	/* prepare for fork	*/
	strcpy(isdnfile, isdnfileprefix); /* save device name for child */
	strcat(isdnfile, svc_p->svc_tag);
	callfd = svc_p->poll_p->fd;	/* save callfd for child */

	if ((ch_pid = fork()) < 0) {	/* fork failed, reject call */
		reject_call();
		return;
	}
	else if (ch_pid == 0)	  	/* child, process call */
	       	process_call();		/* exit before return  */

	/* port monitor parent process */
	logger(debug, debugfd, "Connect Indication - spawned call handler pid %d\n", ch_pid);

	stop_svc(svc_p); 		/* restart listen on new fd */
	start_svc(svc_p);		/* with stop/start	    */
	logger(debug, debugfd, "exited process_msg\n");

}


/* main loop high level routines	*/

void
service_svcs()
{
svc_t	*svc_p;

	svc_p = svchead;
	while (svc_p != NULL) {

		if ((svc_p->svc_active == B_TRUE)
			&& (svc_p->poll_p != NULL)) {

			if ((svc_p->poll_p->revents & (POLL_EVENTS)) != 0) {

				if ((svc_p->poll_p->revents & POLLHUP) != 0) {
					logger(debug, debugfd, "hangup on svc\n");
					/* do what? */
					stop_svc(svc_p);
				}
				else
					process_msg(svc_p);

				if (svc_p->poll_p != NULL)
					svc_p->poll_p->revents = 0;
			}
		}
	svc_p = svc_p->next;
	}

}


void
check_svcs()
{
svc_t	*svc_p;

	svc_p = svchead;
	while (svc_p != NULL) {
		start_svc(svc_p);	/* This won't start running or	*/
		svc_p = svc_p->next;	/* disabled services.  Just	*/
					/* inactive, enabled services   */	
	}

}
	

void
service_pmpipe()
{
svc_t	*sp;

	if (read(pmfd, &sac_msg, sizeof(sac_msg)) != sizeof(sac_msg)) {
		logger(debug, debugfd, "pmpipe read failed");
		terminate();
	}

	switch (sac_msg.sc_type) {
		case SC_STATUS:
			logger(debug, debugfd, "status message from sac\n");
			pm_msg.pm_type = PM_STATUS;
			pm_msg.pm_state = pmstate;
			break;

		case SC_ENABLE:
			logger(debug, debugfd, "enable message from sac\n");
			pm_msg.pm_type = PM_STATUS;
			if (pmstate == PM_DISABLED) {
				pmstate = PM_ENABLED; /* start_svc needs this */
				for (sp = svchead; sp != NULL; sp = sp->next)
					start_svc(sp);
			}
			pm_msg.pm_state = pmstate;
			break;

	       	case SC_DISABLE:
			logger(debug, debugfd, "disable message from sac\n");
			pm_msg.pm_type = PM_STATUS;
			if (pmstate == PM_ENABLED) {
				pmstate = PM_DISABLED;
				for (sp = svchead; sp != NULL; sp = sp->next)
					stop_svc(sp);
			}
			pm_msg.pm_state = pmstate;
			break;

		case SC_READDB:
			logger(debug, debugfd, "read pmtab message from sac\n");
			pm_msg.pm_type = PM_STATUS;
			pm_msg.pm_state = pmstate;
			read_pmtab();
			if (pmstate == PM_ENABLED)
				pmtab_act();
			break;

		default:
			logger(debug, debugfd, "unknown message from sac\n");
			pm_msg.pm_type = PM_UNKNOWN;
			pm_msg.pm_state = pmstate;
			break;
	}

	if (write(sacfd, &pm_msg, sizeof(pm_msg)) != sizeof(pm_msg))
		logger(debug, debugfd, "message to sac failed\n");

}


/* main loop */		

void
main()
{
int	poll_result;

	init();
	while (1) {

/*		logger(debug, debugfd, "beginning of main loop\n"); */
		poll_result = poll(polltab, polltab_nfds, POLL_TIMEOUT);

		if (poll_result < 0 ) {
			logger(debug, debugfd, "poll failed\n");
			terminate();
		}

		if (poll_result == 0) { /* timeout */

			/* retry failed service enables */
			check_svcs();
/*			logger(debug, debugfd, "poll timeout\n"); */
			continue;
		}

		if ((pollpmpipe->revents & (POLL_EVENTS)) != 0) {

			if ((pollpmpipe->revents & POLLHUP) != 0) {
				logger(debug, debugfd, "hangup on pmpipe\n");
				terminate();
			}

			service_pmpipe();
			pollpmpipe->revents = 0;
		}

		service_svcs();
/*		logger(debug, debugfd, "end of main loop\n"); */

	}

}
