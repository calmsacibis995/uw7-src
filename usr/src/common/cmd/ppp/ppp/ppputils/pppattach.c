#ident "@(#)pppattach.c	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1997 SCO ltd
 * All rights reserved.
 *
 */

/*
 *  pppattach/pppdetach/pppstatus/ppplinkadd/ppplinkdrop
 *
 * utility to attach/detach a named ppp bundle 
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <locale.h>
#include <dial.h> 
#include <sys/ppp.h>
#include <sys/types.h>
#include <libgen.h>
#include <dlfcn.h>
#include <string.h>
#include <curses.h>
#include <signal.h>

#include "psm.h"
#include "ulr.h"
#include "ppp_msg.h"
#include "ppptalk.h"
#include "pathnames.h"

#define BOLD_ON		if (window) attron(A_BOLD);
#define BOLD_OFF 	if (window) attroff(A_BOLD);
#define CLEAR 		if (window) clear();

static int sigs[] = {
	SIGHUP,
	SIGINT,
	SIGQUIT,
	SIGILL,
	SIGABRT,
	SIGFPE,
	SIGBUS,
	SIGSEGV,
	SIGALRM,
	SIGTERM,
	SIGUSR1,
	SIGUSR2,
	SIGCLD,
	SIGCHLD,
	SIGXCPU,
	SIGXFSZ,
	0
};

/*
 * Exit codes
 * 0: success
 * 1: invalid arguments or command
 * 2: invalid bundle name 
 * 3: timeout occured before acceptable state as been reached
 *    (note that ppp will continue to attempt to make the connection 
 *     even though the utility has exited)
 * 4: unexpected error from the ppp daemon (pppd)
 * 5: can't attach as bundle is already attached
 * 6: dialler error
 * 7: other
 */
#define EXIT_ARGS	1
#define EXIT_NAME	2
#define EXIT_TIMEOUT	3
#define EXIT_PPPD	4
#define EXIT_ATTACHED	5
#define EXIT_DIALER	6
#define EXIT_OTHER	7

/* cmds */

#define ATTACH	1
#define DETACH	2
#define ADDLINK	3
#define RMLINK	4
#define STATUS	5

/* Status of bundle */

#define STATUS_UNK	0x0	/* unknown - used to null flags */	
#define STATUS_UP	0x1	/* bundle is fully usable */
#define STATUS_DOWN	0x2	/* user will not be able to use the bundle */	
#define STATUS_NEG	0x4	/* negotiating or transient state */	
				/* vvv usable but some links may not be up */

#define STATUS_PART_UP	(STATUS_UP | STATUS_DOWN) 

#define DEF_PERIOD 500		/* default status sample period */

nl_catd catd;

#ifndef MSGSTR
#define MSGSTR(num,str) catgets(catd, MS_PPP, (num), (str))
#endif
#ifndef MC_FLAGS
#define MC_FLAGS NL_CAT_LOCALE
#endif

int 	so;			/* Socket to pppd */

char 	*name;			/* how this binary was called */
int	cmd	= 0;		/* command as called */
int 	window	= 0;		/* windowed output */
int 	maxx	= 0;		/* window size */
int 	maxy	= 0;		/* window size */
int	verbose = PSM_V_SILENT;	/* level of verbosity for psm modules */
int 	timeout	= 120;		/* timeout value in seconds */
int 	period	= DEF_PERIOD;	/* sample period in ms for status*/
int 	async	= 0;		/* asynchronous opperation */
int 	(*pf)(...) = (int (*)(...))printf; /* print function */

struct itimerval it;		/* for timeouts */
struct ulr_prim *gmsg;

struct cp_tab_s {
	char *cp_proto;
	void *cp_handle;
	struct psm_entry_s *cp_entry;
} cp_tab[MAX_CP];

char *phase_name[5];

int lower_status(act_hdr_t *ah, char *name, char *link);

void
exit_clean(int val)
{
	if(window)
		endwin();
	exit(val);
}

/*
 * pmsg - responsible for deciding which messages are output.
 */
void
pmsg(int level, int msgno, char *fmt, ...)
{
        va_list args;
	FILE *fp;
	
	if(level == MSG_INFO)
		fp = stdout;
	else
		fp = stderr;

	/*
	 * Use our normal reporting format
	 */

	va_start(args, fmt);

	/*
	 * Get the message from msg catalog
	 */
	switch (msgno) {
	case -1:
		break;
	case 0:
#ifdef NOT
		if (window)
			printw("\nNeeds I12N : ");
		else
			fprintf(fp, "Needs I12N : ");
#endif
		break;
	default:
		fmt = MSGSTR(msgno, fmt);
		break;
	}

	switch (level) {
	case MSG_INFO:
		if (window) {
			vwprintw(stdscr, fmt, args);
		}
		else
			vfprintf(fp, fmt, args);
		break;

	case MSG_WARN:
		if (window) {
			printw(MSGSTR(PPP_WARN, "WARNING: "));
			vwprintw(stdscr, fmt, args);
		}
		else {
			fprintf(fp, MSGSTR(PPP_WARN, "WARNING: "));
			vfprintf(fp, fmt, args);
		}
		break;

	case MSG_ERROR:
		if (window){
			printw(MSGSTR(PPP_ERROR, "ERROR: "));
			vwprintw(stdscr, fmt, args);
		}
		else {
			fprintf(fp, MSGSTR(PPP_ERROR, "ERROR: "));
			vfprintf(fp, fmt, args);
		}
		break;

	case MSG_DEBUG:
		if (window) {
			printw("DEBUG: ");
			vwprintw(stdscr, fmt, args);
		}
		else {
			fprintf(fp, "DEBUG: ");
			vfprintf(fp, fmt, args);
		}
		break;
	}

	if (window)
		refresh();
	else
		fflush(fp);

	va_end(args);
}

/*
 * Get a token
 */
char *
gettok(char *p, char *tok)
{
	int len = 0;
	
	/* Find the token */
	while (*p && !isspace(*p) && (*p != '=') && len < MAXTOK) {
		tok[len++] = *p;
		p++;
	}
	tok[len] = 0;

	/*
	 * If we have a token that is too long ... skip over its
	 * end .. until we find a white space character
	 */
	if (len == MAXTOK) {
		if (*p && !isspace(*p))
			pmsg(MSG_WARN, PPP_TOKLONG,
			     "Token too long, truncated (%s)\n", tok);
		while(*p && !isspace(*p))
			p++;
	}

	/* Move p to next none space character */

	while(*p && isspace(*p))
		p++;

	return(p);
}

/*
 * Send a message to the Daemon
 */
int
ulr_send(int so, struct ulr_prim *msg, int len)
{
	int ret;

	ASSERT(len > 0);

	ret = send(so, (char *)msg, len, 0);	
	if (ret < 0) {
		pmsg(MSG_ERROR, PPP_FAIL2D,
		     "Failed to send message to the PPP Daemon\n");
		return(ret);
	} else if (ret < len) {
		pmsg(MSG_ERROR, PPP_FAIL2D,
		     "Failed to send message to the PPP Daemon\n");
		return(ret);
	}
	return(0);
}

/*
* Read a message from the Daemon
*/
int
ulr_read(int so, struct ulr_prim *msg, int len)
{
	int rval;
	char *p;

	p = (char *)msg;
	memset(p, 0, len);

	rval = recv(so, p, len, 0);
	if (rval <= 0) {
		pmsg(MSG_ERROR, PPP_FAILFD,
		     "Failed reading message from the PPP Daemon %d, %d\n",
		     rval, errno);
		return rval;
	}
	return 0;
}

/*
 * Send a message to the Daemon .. then wait for a reply
 */
int
ulr_sr(int so, struct ulr_prim *msg, int slen, int rlen)
{
	int ret;

	ret = ulr_send(so, msg, slen);
	if (ret)
		return(ret);

	/* Looks like things went well, wait for response */

	ret = ulr_read(so, msg, rlen);
	if (ret)
		return ret;

	if (msg->error)
		return msg->error;

	return ret;
}

/*
 * The message sent to us has its ID set to that required.
 * Try searching each definition type an entry with the same id.
 */
int
cfg_findid(int so, struct ulr_cfg *msg, char *id, int type)
{
	int ret;

	ASSERT(type >= 0 && type < DEF_MAX);

	msg->prim = ULR_GET_CFG;
	strcpy(msg->ucid, id);
	msg->type = type;

	ret = ulr_sr(so, (struct ulr_prim *)msg,
		     sizeof(struct ulr_cfg), ULR_MAX_MSG);
	if (ret) {
		if (ret != ENOENT)
			pmsg(MSG_ERROR, PPP_UNEXP,
			   "Unexpected error from pppd (%d)\n", ret);
			return(-1);
	} else
		return(msg->type);
}

void
display_dial_error(int err)
{
	switch (err) {
	case INTRPT:
		pmsg(MSG_ERROR, PPP_INTRPT,
		     "Dial failed - Interrupt occured\n");
		break;
	case D_HUNG:
		pmsg(MSG_ERROR, PPP_D_HUNG,
		     "Dialer failed\n");
		break;
	case NO_ANS:
		pmsg(MSG_ERROR, PPP_NO_ANS,
	     "Dial failed - No answer (login or invoke scheme failed)\n");
		break;
	case ILL_BD:
		pmsg(MSG_ERROR, PPP_ILL_BD,
		     "Dial failed - Illegal baud rate\n");
		break;
	case A_PROB:
		pmsg(MSG_ERROR, PPP_A_PROB,
		     "Dial failed - ACU problem\n");
		break;
	case L_PROB:
		pmsg(MSG_ERROR, PPP_L_PROB,
		     "Dial failed - Line problem\n");
		break;
	case NO_Ldv:
		pmsg(MSG_ERROR, PPP_NO_Ldv,
		     "Dial failed - Can't open Devices file\n");
		break;
	case DV_NT_A:
		pmsg(MSG_ERROR, PPP_DV_NT_A,
		     "Dial failed - Requested device not available\n");
		break;
	case DV_NT_K:
		pmsg(MSG_ERROR, PPP_DV_NT_K,
		     "Dial failed - Requested device not known\n");
		break;
	case NO_BD_A:
		pmsg(MSG_ERROR, PPP_NO_BD_A,
		     "Dial failed - No device available at requested baud\n");
		break;
	case NO_BD_K:
		pmsg(MSG_ERROR, PPP_NO_BD_K,
		     "Dial failed - No device known at requested baud\n");
		break;
	case DV_NT_E:
		pmsg(MSG_ERROR, PPP_DV_NT_E,
		     "Dial failed - Requested speed does not match\n");
		break;
	case BAD_SYS:
		pmsg(MSG_ERROR, PPP_BAD_SYS,
		     "Dial failed - System not in Systems file\n");
		break;
	case CS_PROB:
		pmsg(MSG_ERROR, PPP_CS_PROB,
	         "Dial failed - Could not connect to the connection server\n");
		break;
	case DV_W_TM:
		pmsg(MSG_ERROR, PPP_DV_W_TM,
		     "Dial failed - Not allowed to call at this time\n");
		break;
	}
}

/*
 * Bringup another link on a bundle
 */
void
cmd_linkadd(char *args)
{
	char tok[MAXTOK +1];
	int type, ret;
	struct ulr_attach *m = (struct ulr_attach *)gmsg;

	args = gettok(args, tok);

	if (!tok || *tok == 0) {
		pmsg(MSG_ERROR, PPP_NOBUNDLE,
		     "Must specify bundle for request.\n");
		exit_clean(EXIT_ARGS);
	}

	/*
	 * Check the specified ID exists and is a DEF_OUT
	 */
	ret = cfg_findid(so, (struct ulr_cfg *)m, tok, DEF_BUNDLE);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPP_NOTBUNDLE,
		     "%s is not a bundle definition.\n", tok);
		exit_clean(EXIT_NAME);
	}

	if (*args) {
		ret = cfg_findid(so, (struct ulr_cfg *)m, args, DEF_LINK);
		if (ret < 0) {
			pmsg(MSG_ERROR, PPP_NOTLINK,
			     "%s is not a link definition.\n", args);
			exit_clean(EXIT_NAME);
		}
	} 

	/* Send the linkadd  operation */
	strcpy(m->bundle, tok);
	strcpy(m->link, args);
	m->prim = ULR_LINKADD;

	ret = ulr_sr(so, (struct ulr_prim *)m,
		     sizeof(struct ulr_attach), ULR_MAX_MSG);

	switch (ret) {
	case 0:
		break;
	case ENOENT:
		pmsg(MSG_ERROR, PPP_ENOBUNDLE,
		     "No such active bundle (reset).\n");
		exit_clean(EXIT_NAME);
		break;
	case EINVAL:
		pmsg(MSG_ERROR, PPP_BADLINKBUNDLE,
		     "Invalid link/bundle for operation\n");
		exit_clean(EXIT_NAME);
		break;
	case ENXIO:
		pmsg(MSG_ERROR, PPP_NOLINKDEF, "No links defined\n");
		exit_clean(EXIT_OTHER);
		break;
	case EBUSY:
		pmsg(MSG_ERROR, PPP_MAXLINKS, "Maxlinks reached.\n");
		exit_clean(EXIT_OTHER);
		break;
	case EAGAIN:
		pmsg(MSG_ERROR, PPP_NOLINKAVAIL, "No link available\n");
		exit_clean(EXIT_OTHER);
		break;
	case EIO:
		/* Dial error */
		display_dial_error(m->derror);
		exit_clean(EXIT_DIALER);
		break;
	default:
		pmsg(MSG_ERROR, PPP_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		exit_clean(EXIT_PPPD);
		break;
	}
}

/*
 * Drop a link from a bundle
 */
void
cmd_linkdrop(char *args)
{
	char tok[MAXTOK +1];
	int type, ret;
	struct ulr_attach *m = (struct ulr_attach *)gmsg;

	args = gettok(args, tok);

	if (!tok || *tok == 0) {
		pmsg(MSG_ERROR, PPP_NOBUNDLE,
		     "Must specify bundle for request.\n");
		exit_clean(EXIT_ARGS);
	}

	/*
	 * Check the specified ID exists and is a DEF_OUT
	 */
	ret = cfg_findid(so, (struct ulr_cfg *)m, tok, DEF_BUNDLE);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPP_NOTBUNDLE,
		     "%s is not a bundle definition.\n", tok);
		exit_clean(EXIT_NAME);
	}

	if (*args) {
		ret = cfg_findid(so, (struct ulr_cfg *)m, args, DEF_LINK);
		if (ret < 0) {
			pmsg(MSG_ERROR, PPP_NOTLINK,
			     "%s is not a link definition.\n", args);
			exit_clean(EXIT_NAME);
		}
	}

	/* Send the linkdrop  operation */
	strcpy(m->bundle, tok);
	strcpy(m->link, args);
	m->prim = ULR_LINKDROP;

	ret = ulr_sr(so, (struct ulr_prim *)m, sizeof(struct ulr_attach),
		     sizeof(struct ulr_attach));

	switch (ret) {
	case 0:
		break;
	case ENOENT:
		pmsg(MSG_ERROR, PPP_ENOBUNDLE,
		     "No such active bundle (reset).\n");
		exit_clean(EXIT_NAME);
		break;
	case EBUSY:
		pmsg(MSG_ERROR, PPP_MINLINKS, "Minlinks reached.\n");
		exit_clean(EXIT_OTHER);
		break;
	case EINVAL:
		pmsg(MSG_ERROR, PPP_BADLINKBUNDLE,
		     "Invalid link/bundle for operation\n");
		exit_clean(EXIT_NAME);
		break;
	default:
		pmsg(MSG_ERROR, PPP_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		exit_clean(EXIT_PPPD);
		break;
	}
}

void
cmd_attach(char *bundle)
{
	int type, ret;
	struct ulr_attach *m = (struct ulr_attach *)gmsg;

	/* Get the bundle */
 
	if (!bundle || *bundle == 0) {
		pmsg(MSG_ERROR, PPP_NOBUNDLE,
		     "Must specify bundle for request.\n");
		exit_clean(EXIT_ARGS);
	}

	/*
	 * Check the specified ID exists and is a DEF_OUT
	 */
	ret = cfg_findid(so, (struct ulr_cfg *)m, bundle, DEF_BUNDLE);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPP_NOTBUNDLE,
		     "%s is not a bundle definition.\n", bundle);
		exit_clean(EXIT_NAME);
	}

	/* Send the attach operation */

	m->prim = ULR_ATTACH;
	strcpy(m->bundle, bundle);

	ret = ulr_sr(so,  (struct ulr_prim *)m,
		     sizeof(struct ulr_attach), ULR_MAX_MSG);

	switch (ret) {
	case 0:
		break;
	case ENOENT:
		pmsg(MSG_ERROR, PPP_ENOBUNDLE,
		     "No such active bundle (reset).\n");
		exit_clean(EXIT_NAME);
		break;
	case EINVAL:
		pmsg(MSG_ERROR, PPP_EINVBUNDLE,
		     "Invalid bundle for operation\n");
		exit_clean(EXIT_NAME);
		break;
	case ENXIO:
		pmsg(MSG_ERROR, PPP_NOLINKDEF, "No links defined\n");
		exit_clean(EXIT_OTHER);
		break;
	case EAGAIN:
		pmsg(MSG_ERROR, PPP_NOLINKAVAIL, "No link available\n");
		exit_clean(EXIT_OTHER);
		break;
	case EIO:
		/* Dial error */
		display_dial_error(m->derror);
		exit_clean(EXIT_DIALER);
		break;
	default:
		pmsg(MSG_ERROR, PPP_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		exit_clean(EXIT_PPPD);
		break;
	}
}

void
cmd_detach(char *bundle)
{
	int type, ret;
	struct ulr_attach *m = (struct ulr_attach *)gmsg;

	if (!bundle || *bundle == 0) {
		pmsg(MSG_ERROR, PPP_NOBUNDLE,
		     "Must specify bundle for request.\n");
		exit_clean(EXIT_ARGS);
	}

	/*
	 * Check the specified ID exists and is a DEF_OUT
	 */
	ret = cfg_findid(so, (struct ulr_cfg *)m, bundle, DEF_BUNDLE);
	if (ret < 0) {
		pmsg(MSG_ERROR, PPP_NOTBUNDLE,
		     "%s is not a bundle definition.\n", bundle);
		exit_clean(EXIT_NAME);
	}

	/* Send the detach  operation */

	m->prim = ULR_DETACH;
	strcpy(m->bundle, bundle);

	ret = ulr_sr(so,  (struct ulr_prim *)m,
		     sizeof(struct ulr_attach), ULR_MAX_MSG);

	switch (ret) {
	case 0:
		break;
	case ENOENT:
		pmsg(MSG_ERROR, PPP_ENOBUNDLE,
		     "No such active bundle (reset).\n");
		exit_clean(EXIT_NAME);
		break;
	default:
		pmsg(MSG_ERROR, PPP_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		exit_clean(EXIT_PPPD);
		break;
	}
}

/* 
 * Given a protocol name, eg ip, fund the shared object that
 * supports this protocol.
 */
struct psm_entry_s *
cp_get_tab(char *proto)
{
	int i;
	void *handle;
	char buf[256];
	struct cp_tab_s *tp = cp_tab;

	/* Check if already opened this protocol */

	for (i = 0; i < MAX_CP; i++, tp++) {
		if (tp->cp_handle) {
			if (strcmp(tp->cp_proto, proto) == 0)
				return tp->cp_entry;
		} else {
			/* Not loaded ... do it now */

			sprintf(buf, "%s%s_parse.so", PSM_SO_PATH, proto);
			handle = dlopen(buf, RTLD_LAZY);
			if (!handle) {
				pmsg(MSG_ERROR, PPP_NOSUPT,
				     "Error loading %s support (%s)\n",
				     proto, dlerror());
				return NULL;
			}

			/* Find the entry point */

			tp->cp_entry = (struct psm_entry_s *)
				dlsym(handle, "psm_entry");

			if (!tp->cp_entry) {
				return NULL;
			}

			tp->cp_handle = handle;
			tp->cp_proto = strdup(proto);
			return tp->cp_entry;
		}
	}
	return NULL;
}

/*
 * Get and display status for protocols on the link/bundle
 */
int
psm_status(char *id, int type)
{
	struct ulr_psm_stat *m = (struct ulr_psm_stat *)gmsg;
	int i = 0, ret;
	struct psm_entry_s *e;
	int st = STATUS_UNK;
	int fsm_st;
	
	strcpy(m->id, id);
	m->type = type;
	m->prim = ULR_PSM_STATUS;

	do {
		m->cookie = i;

		ret = ulr_sr(so,  (struct ulr_prim *)m,
			     sizeof(struct ulr_psm_stat), ULR_MAX_MSG);
		switch (ret) {
		case 0:
			break;
		default:
			pmsg(MSG_ERROR, PPP_UNEXP,
			     "Unexpected error from pppd (%d)\n", ret);
			exit_clean(EXIT_PPPD);
		}
		
		e = cp_get_tab(m->psmid);
		if (!e) {
			pmsg(MSG_ERROR, PPP_PROTONAVAIL,
			     "Support for protocol %s not available\n",
			     m->psmid);
		} else {
			if (*e->psm_status) {
				fsm_st = (*e->psm_status)(&m->data, 
				0, verbose, pf);
				if (e->psm_flags & PSMF_FSM)
					switch(fsm_st) { 
					case -1:
					case OPENED:
						st |= STATUS_UP;
						break;
					case CLOSED:
					case STOPPING:
						st |= STATUS_DOWN;
						break;
					default:
						st |= STATUS_NEG;
						break;
					}
			}
		}
		i++;
	} while (m->more);

	if (st == STATUS_UNK)
		return STATUS_UP; /* assume the best */
	else
		return st;
}

int
links_status(char *name, char *link)
{
	struct ulr_list_links *ll;
	struct ulr_status ustatus;
	int i, *ip, ret;
	int status = STATUS_UNK;

	ll = (struct ulr_list_links *)malloc(ULR_MAX_MSG);
	if (!ll) {
		pmsg(MSG_ERROR, 0, "No memory\n");
		return;
	}

	ll->prim = ULR_LIST_LINKS;
	strcpy(ll->bundle, name);

	ret = ulr_sr(so, (struct ulr_prim *)ll, sizeof(*ll), ULR_MAX_MSG);
	switch (ret) {
	case 0:
		break;
	default:
		pmsg(MSG_ERROR, PPP_UNEXP,
		     "Unexpected error from pppd (%d)\n", ret);
		exit_clean(EXIT_PPPD);
	}

	ip = ll->index;
	while (ll->numlinks-- > 0) {
		ustatus.prim = ULR_STATUS;
		ustatus.type = DEF_LINK;
		ustatus.index = *ip;
		
		ret = ulr_sr(so, (struct ulr_prim *)&ustatus,
			     sizeof(ustatus), sizeof(ustatus));
		switch (ret) {
		case 0:
			status |= lower_status(&ustatus.ah, ustatus.id, link);
			break;
		default:
			pmsg(MSG_ERROR, PPP_UNEXP,
			     "Unexpected error from pppd (%d)\n", ret);
			exit_clean(EXIT_PPPD);
		}
		
		ip++;
	}
	free(ll);
	if (status == STATUS_UNK)
		return STATUS_UP;
	else
		return status;
}

/* 
 * get the status of the lower elements of the bundle 
 *
 * Return values are STATUS_UP, STATUS_DOWN, STATUS_PART_UP
 */

int
lower_status(act_hdr_t *ah, char *name, char *link)
{
	act_bundle_t	*ab;
	act_link_t	*al;
	int status	= STATUS_UNK;
	int vo		= verbose;	
	int x,y;

	switch (ah->ah_type) {
	case DEF_LINK:
		/* if we're trying to look at information specific
		 * to a nominated link, link will be set. If link 
		 * is set and this isn't the link the silence 
		 * output temporarily. 
 		 *
		 * If we're asking for all link information 
		 * (ie link == NULL)
		 * AND we're windowed 
		 * check it will fit on the screen before printing it 
		 *
		 */
		if ( link && strcmp(name, link) != 0 ) {
			verbose = PSM_V_SILENT;
		} else if ( (link == NULL) && window ) {
			getyx(stdscr,y,x);
			if ( (maxy - y) < 2 )
				verbose = PSM_V_SILENT;
		}

		al = &ah->ah_link;
		if(verbose==PSM_V_LONG) 
			pmsg(MSG_INFO, -1, "        ");
		if (verbose){
			pmsg(MSG_INFO, PPP_LINKSTAT1,
			   "%s\t\tPhase:%s\tStatus:", name, 
			   phase_name[ah->ah_phase]);
			if (al->al_flags & ALF_INCOMING)
				pmsg(MSG_INFO, PPP_INCOMING,"Incoming\n");
			if (al->al_flags & ALF_OUTGOING)
				pmsg(MSG_INFO, PPP_OUTGOING,"Outgoing\n");
			if (al->al_flags & ALF_DIAL)
				pmsg(MSG_INFO, PPP_DIALING,"Dialing\n");
			if (al->al_flags & ALF_HANGUP)
				pmsg(MSG_INFO, PPP_HANGUP,"Hangup\n");
			if (al->al_flags == 0)
				pmsg(MSG_INFO, PPP_READY,"Ready\n");
		}
		status |= psm_status(name, ah->ah_type);
		if(verbose==PSM_V_LONG) 
			pmsg(MSG_INFO, -1, "\n");
	 	verbose = vo;	

		break;
	case DEF_BUNDLE:
		if (verbose) {
			CLEAR;
			ab = &ah->ah_bundle;
			BOLD_ON;
			pmsg(MSG_INFO, PPP_BUNDLE, "Bundle\n");
			BOLD_OFF;
			pmsg(MSG_INFO, PPP_PHASE, 
			   "%s\t\tPhase:%s\n",
			   name, phase_name[ah->ah_phase]);
			pmsg(MSG_INFO, PPP_BUND_OLINK, 
			   "        Open Links  : %d\n", ab->ab_open_links);
			pmsg(MSG_INFO, PPP_BUND_TLINK,
			   "        Total Links : %d\n", ab->ab_numlinks);
		}
		if (verbose == PSM_V_LONG) {
			pmsg(MSG_INFO, PPP_BUND_FLAGS,
			   "        Flags       : 0x%x\n", ab->ab_flags);
			pmsg(MSG_INFO, PPP_BUND_LOCMR,
			   "        Local MRRU  : %d\n", ab->ab_local_mrru);
			pmsg(MSG_INFO, PPP_BUND_PEEMR,
			   "        Peer MRRU   : %d\n", ab->ab_peer_mrru);
			pmsg(MSG_INFO, PPP_BUND_MTU,
			   "        Mtu         : %d\n", ab->ab_mtu);
		}
		if (verbose) {
			BOLD_ON;
			pmsg(MSG_INFO, PPP_BUNDLEPROT,"\nBundle protocols\n\n");
			BOLD_OFF;
		}
		status |= psm_status(name, ah->ah_type);
		/*
		 * Get the status of all the bundles links (or at least
		 * the first few hundred)
		 */
		BOLD_ON;
		if(verbose)
			pmsg(MSG_INFO, PPP_LINKPROT, "\nLink protocols\n\n");
		BOLD_OFF;
		status |= links_status(name, link);
		break;
	}

	if (status == STATUS_UNK)
		return STATUS_UP;
	else
		return status;

}

/*
 * Get the number of open links and total links  for the bundle
 */
void
get_numlinks(int so, char *id, int *olinks, int* tlinks)
{
	struct ulr_status *m = (struct ulr_status *)gmsg;
	int ret;
	act_bundle_t    *ab;

	strcpy(m->id, id);
	m->prim = ULR_STATUS;
	m->type = DEF_BUNDLE;
	m->index = -1;

	ret = ulr_sr(so, (struct ulr_prim *)m,
		     sizeof(struct ulr_status), ULR_MAX_MSG);

	switch (ret) {
		case 0:
			break;
		case ENOENT:
			pmsg(MSG_INFO, PPP_NOTACTIVE, 
			   "%s not currently active.\n", id);
			exit_clean(EXIT_NAME);
		default:
			pmsg(MSG_ERROR, PPP_UNEXP,
			"Unexpected error from pppd (%d)\n", ret);
			exit_clean(EXIT_PPPD);
	}

	*olinks = m->ah.ah_bundle.ab_open_links;
	*tlinks = m->ah.ah_bundle.ab_numlinks;
}

/*
 * Get the status for the specified type and id
 *
 */
int
get_status(int so, char *id, int type, char *link)
{
	struct ulr_status m;
	int ret;
	int lstatus;
	int bphase;

	strcpy(m.id, id);
	m.prim = ULR_STATUS;
	m.type = type;
	m.index = -1;

	ret = ulr_sr(so, (struct ulr_prim *)&m,
		     sizeof(struct ulr_status), sizeof(struct ulr_status));

	switch (ret) {
		case 0:
			break;
		case ENOENT:
			pmsg(MSG_INFO, PPP_NOTACTIVE, 
			   "%s not currently active.\n", id);
			exit_clean(EXIT_NAME);
		default:
			pmsg(MSG_ERROR, PPP_UNEXP,
			"Unexpected error from pppd (%d)\n", ret);
			exit_clean(EXIT_PPPD);
	}
	bphase = m.ah.ah_phase;

	if (window)
		move(0,0);

	lstatus = lower_status(&m.ah, id, link);

	if (window)
		refresh();

	if (bphase != PHASE_NETWORK) 
		return STATUS_DOWN;
	else 
		return lstatus;
}


/* 
 * Attempt to detach the named bundle
 * if timeout == 0 -> tries indefinitely
 *
 * Return values
 * 	0 == success
 *	1 == other failure
 *	3 == timedout still trying
 * 
 */
int 
detach_bundle(char *bundle)
{
	int status = STATUS_UP;
	int t = 0;

	cmd_detach(bundle);

	if (async)
		return 0;

	while( status != STATUS_DOWN ) {
		status=get_status(so, bundle, DEF_BUNDLE, NULL);
		/*  
		 * in window mode status information is 
		 * displayed by get_status 
		 */
		nap(period);
	}

	return 0; 
}

rmlink_bundle(char *args)
{
	char bundle[MAXTOK +1];
	char link[MAXTOK +1];
	int t = 0;
	char *targs;
	int olinks, orig_olinks;
	int tlinks, orig_tlinks;

	targs = gettok(args, bundle);
	gettok(targs, link);

	get_numlinks(so, bundle, &olinks, &tlinks);
	orig_olinks=olinks;
	orig_tlinks=tlinks;

	cmd_linkdrop(args);

	if (async)
		return 0;

	while(orig_olinks==olinks || orig_tlinks==tlinks) {
		get_status(so, bundle, DEF_BUNDLE, link);
		get_numlinks(so, bundle, &olinks, &tlinks);
		/*  
		 * in window mode status information is 
		 * displayed by get_status 
		 */
		nap(period);
	}

	return 0;
}

addlink_bundle(char *args)
{
	char bundle[MAXTOK +1];
	char link[MAXTOK +1];
	int t = 0;
	int status = STATUS_DOWN;
	char *targs;

	targs = gettok(args, bundle);
	gettok(targs, link);

	cmd_linkadd(args);

	if (async)
		return 0;

	while(status != STATUS_UP) {
		status = get_status(so, bundle, DEF_BUNDLE, link);
		/*  
		 * in window mode status information is 
		 * displayed by get_status 
		 */
		nap(period);
	}

	return 0;
}

/* 
 * Attempt to attach the named bundle
 * if timeout == 0 -> tries indefinitely
 *
 * Return values
 * 	0 == success
 *	1 == failure
 * 
 */

attach_bundle(char *bundle)
{
	int t = 0;
	int status = STATUS_DOWN;

	cmd_attach(bundle);

	if (async)
		return 0;

	while(status != STATUS_UP) {
		status = get_status(so, bundle, DEF_BUNDLE, NULL);
		/*  
		 * in window mode status information is 
		 * displayed by get_status 
		 */
		nap(period);
	}

	return 0;
}

status_bundle(char *bundle)
{
	while(1) {
		get_status(so, bundle, DEF_BUNDLE, NULL);
		nap(period);
	}
}

usage()
{
	switch (cmd) {
		case ATTACH:
			pmsg(MSG_ERROR, PPP_AT_USAGE, 
		         "usage: %s [ -t timeout ] [ -p period ] [ -v ] [ -a ] bundle_name\n"
			 ,name);
			break;
		case DETACH:
			pmsg(MSG_ERROR, PPP_DE_USAGE, 
		         "usage: %s [ -t timeout ] [ -p period ] [ -v ] [ -a ] bundle_name\n"
			 ,name);
			break;
		case ADDLINK:
			pmsg(MSG_ERROR, PPP_AD_USAGE, 
		         "usage: %s [ -t timeout ] [ -p period ] [ -v ] [ -a ] bundle_name [ link_name ]\n"
			 ,name);
			break;
		case RMLINK:
			pmsg(MSG_ERROR, PPP_RM_USAGE, 
		         "usage: %s [ -t timeout ] [ -p period ] [ -v ] [ -a ] bundle_name [ link_name ]\n"
			 ,name);
			break;
		case STATUS:
			pmsg(MSG_ERROR, PPP_ST_USAGE, 
		         "usage: %s [ -t timeout ] [ -p period ] [ -v ] [ -a ] bundle_name\n"
			 ,name);
			break;
	}

	exit_clean(EXIT_ARGS);
}

void 
sigtrap(int sig)
{

	if (sig == SIGALRM)  {
		pmsg(MSG_WARN, PPP_TIMEOUT,"Command timed out.\n");
		exit_clean(EXIT_TIMEOUT);
	}

	pmsg(MSG_ERROR, PPP_CAUGHTSIG, "Caught signal %d\n", sig);
	exit_clean(EXIT_OTHER);
}

void
config_sigs()
{
	int *sp;
	struct sigaction sa;

	sa.sa_handler = sigtrap;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags   = 0;

	for (sp = sigs; *sp; sp++) {
		if (sigaction(*sp, &sa, (struct sigaction *) 0) < 0) {
			pmsg(MSG_ERROR, PPP_CONFIGSIG,"Configuring signals\n"); 
			exit_clean(EXIT_OTHER);
		}
	}

	if (timeout) {
		it.it_interval.tv_sec	= 0;
		it.it_interval.tv_usec	= 0;
		it.it_value.tv_sec 	= timeout;
		it.it_value.tv_usec 	= 0;
		setitimer(ITIMER_REAL, &it, NULL);
	}

}

/*
 * Initialise internationalised strings 
 */

init_str()
{
	phase_name[0]=MSGSTR(PPP_DEAD, "Dead");
	phase_name[1]=MSGSTR(PPP_ESTB, "Established");
	phase_name[2]=MSGSTR(PPP_AUTH, "Authenticate");
	phase_name[3]=MSGSTR(PPP_NETW, "Network");
	phase_name[4]=MSGSTR(PPP_TERM, "Terminate");
}

main(int argc, char **argv)
{
	extern int optind;
	extern char *optarg;
	int c;
	char *bundle;
	struct sigaction sa;
	int rval = 0;
	int len;
	char *args;
	int i;
	
	name = argv[0];
	setlocale(LC_ALL,"");
	catd = catopen(MF_PPP, MC_FLAGS);
	init_str();

	if(strcmp(basename(argv[0]), "pppattach") == 0)
		cmd = ATTACH;
	if(strcmp(basename(argv[0]), "pppdetach") == 0)
		cmd = DETACH;
	if(strcmp(basename(argv[0]), "ppplinkadd") == 0)
		cmd = ADDLINK;
	if(strcmp(basename(argv[0]), "ppplinkdrop") == 0)
		cmd = RMLINK;
	if(strcmp(basename(argv[0]), "pppstatus") == 0) {
		cmd = STATUS;
		verbose = PSM_V_SHORT;
		window++;
		pf = (int (*)(...))printw;
		timeout = 0;
	}

/* parse command line for (optional) arguments */

	while ( (c = getopt(argc, argv, "avt:p:")) != -1 ) {
		switch(c) {
			case 'a':
				async++;
				break;
			case 'v':
				if(cmd == STATUS){
					window = 0;
					pf = (int (*)(...))printf;
					verbose = PSM_V_LONG;
				}
				else{
					window++;
					pf = (int (*)(...))printw;
					verbose = PSM_V_SHORT;
				}
				break;
			case 'p':
				period = 1000*atoi(optarg);
				if (period < 1)
					period = DEF_PERIOD;
				break;
			case 't':
				timeout = atoi(optarg);
				break;
			default:
				/* make sure we're in curses 
				 * mode if we need to be or 
				 * the usage message gets corrupted 
				 */
				if(window) {
					initscr();
					move(0,0);
				}
				usage();
				break;
		}
	}

	if(window) {
		initscr();
		move(0,0);
		getmaxyx(stdscr, maxy, maxx);
	}

	if ( optind >= argc )
		usage(NULL);

	bundle = argv[optind];

	/* if there are any remaining arguments
	 * they should be link names. The commands
	 * expect these as a simple string so we have to 
 	 * do a bit of concatenation here 
	 */

	args=(char *)argvtostr(&argv[optind]);
		
	config_sigs();

	so = ppp_sockinit();
	if (so < 0) {
		pmsg(MSG_ERROR, PPP_FAILCON_PPPD, 
		   "Cannot connect to pppd\n");
		exit_clean(EXIT_PPPD);
	}

	gmsg = (struct ulr_prim *)malloc(ULR_MAX_MSG);
	if (!gmsg) {
		pmsg(MSG_ERROR, 0, "Failed to allocate memory\n");
		exit_clean(EXIT_PPPD);
	}

	switch(cmd) {
		case ATTACH:
			rval=attach_bundle(bundle); 
			break;
		case DETACH:
			rval=detach_bundle(bundle);
			break;
		case STATUS:
			status_bundle(bundle);
			break;
		case ADDLINK:
			addlink_bundle(args);
			break;
		case RMLINK:
			rmlink_bundle(args);
			break;
		default:
			usage(NULL);
			break;
	}

	exit_clean(rval);
}
