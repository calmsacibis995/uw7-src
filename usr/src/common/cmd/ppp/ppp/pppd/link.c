#ident	"@(#)link.c	1.9"
#ident "$Header$"

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/conf.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <termio.h>
#include <termios.h>
#include <sys/termiox.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dial.h>
#include <string.h>

#include "ppp_cfg.h"
#include "psm.h"
#include "ppp_type.h"
#include "act.h"
#include "hist.h"
#include "ppp_proto.h"

#define FIELD_SEP ' ' /* Field seperator */

extern int pcid_fd;

/*
 * Called with the links lock held.
 */
int
link_dial(act_hdr_t *al, char *sysname, char *telno, int *dial_error)
{
	int ret;
 	struct cfg_link *cl = (struct cfg_link *)al->ah_cfg;
	calls_t call;
	dial_status_t dstatus;

	*dial_error = 0;

	/*
	 * Check if we already have a physical connection
	 */
	if (al->ah_link.al_fd >= 0) {
		hist_add(HOP_ADD, al, EBUSY);
		return EBUSY;
	}
	
	al->ah_link.al_reason = 0;
	/*
	 * Obtain a connection
	 */
	call.flags = 0;
	call.attr = NULL;               /* termio attributes */
	call.speed = -1;                /* any speed */
	call.device_name = ucfg_str(&cl->ln_ch, cl->ln_dev);
	call.telno = telno;              /* Telephone number to dial */
	call.caller_telno = NULL;
	call.sysname = sysname;		/* System name to call */
	call.function = FUNC_CHAT;

	/* Set class based on required type */

	switch (cl->ln_type) {
	case LNK_ISDN:
		call.class = "ISDN_SYNC";
		break;
	case LNK_ISDNVOC:
		call.class = "ISDN_ASYNC";
		break;
	case LNK_TCP:
		call.class = "???";
		break;
	case LNK_ANALOG:
	case LNK_STATIC:
	default:
		call.class = NULL;
	}

	call.protocol = NULL;
	call.pinfo_len = 0;

	ppplog(MSG_INFO_LOW, al,
	       "Dialing remote system '%s', telno '%s', dev '%s'\n",
	       call.sysname, call.telno, call.device_name);

	al->ah_link.al_fd = dials(&call, &al->ah_link.al_connect, &dstatus);

	if (al->ah_link.al_fd < 0) {
		*dial_error = dstatus.status_code;
		al->ah_link.al_flags = 0;
		ppplog(MSG_INFO_LOW, al, "Dial failure: error %d\n",
		       *dial_error);
		al->ah_link.al_reason |= ALR_DIAL;
		hist_add(HOP_ADD, al, ENODEV);
		return 0;
	}

	ppplog(MSG_DEBUG, al, "Dialing Completed.\n");
	al->ah_link.al_flags = ALF_INUSE | ALF_PHYS_UP | ALF_OUTGOING;
	hist_add(HOP_ADD, al, 0);
	return 0;
}

int
link_undial(act_hdr_t *al)
{
	int ret;
	act_hdr_t *ab;

	ppplog(MSG_INFO_LOW, al, "Closing link to remote system.\n");

	/*
	 * Check if we already have a physical connection
	 */
	if (al->ah_link.al_fd < 0)
		return EUNATCH;

	if (al->ah_link.al_flags & ALF_COND) {
		ret = ioctl(pcid_fd, I_UNLINK, al->ah_index);
		if (ret) {
			ppplog(MSG_ERROR, al, "Failed to I_UNLINK link (%d)\n",
			       errno);
			hist_add(HOP_DROP, al, errno);
			return errno;
		}
	}

	undials(al->ah_link.al_fd, al->ah_link.al_connect);

	hist_add(HOP_DROP, al, 0);

	ppplog(MSG_DEBUG, al, "Closed link.\n");

	al->ah_link.al_fd = -1;
	al->ah_link.al_connect = NULL;

	if (al->ah_link.al_uid) {
		free(al->ah_link.al_uid);
		al->ah_link.al_uid = NULL;
	}

	if (al->ah_link.al_cid) {
		free(al->ah_link.al_cid);
		al->ah_link.al_cid = NULL;
	}

	ab = al->ah_link.al_bundle;
	if (ab)
		ab->ah_bundle.ab_dropindex = al->ah_link.al_cindex;

	fsm_state(al->ah_link.al_lcp, CLOSE, 0);
	fsm_state(al->ah_link.al_lcp, DOWN, 0);
	ppp_phase(al, PHASE_DEAD);
	return 0;
}

/*
 * Perform any actions to ensure an async serial line is
 * ready for use by PPP
 */
STATIC int
link_async_cond(act_hdr_t *al)
{
	struct termios trm;
	struct cfg_link *cl = (struct cfg_link *)al->ah_cfg;

	/* Set defined flow control (if applicable) */

	if (tcgetattr(al->ah_link.al_fd, &trm) < 0) {
		ppplog(MSG_ERROR, al,
		       "link_asyc_cond: tcgetattr() failed, error %d\n",
		       errno);
		return errno;
	}
	
	/*
	 * First disable hardware flow control just in case it was 
	 * previously enabled
	 */

	if (control_hwd_flow(al->ah_link.al_fd, 0) == -1) {
		ppplog(MSG_ERROR, al,
		       "Failed to disable hardware flow control (%d)\n",
		       errno);
		return errno;
	}

	/* Ignore break condition */
	trm.c_iflag = IGNBRK;

	/* set line flow control mode */	

	switch (cl->ln_flow) {
	case FLOW_SOFT:
		trm.c_iflag |= IXON|IXOFF;
		ppplog(MSG_INFO_MED, al, "Software flow control selected\n");
		break;

	case FLOW_HARD:
		if (control_hwd_flow(al->ah_link.al_fd, 1) == -1) {
			ppplog(MSG_ERROR, al,
		       "Failed to enable hardware flow control, error %d\n",
			       errno);
			return errno;
		}
		ppplog(MSG_INFO_MED, al, "Hardware flow control selected\n");
		break;
	default:
		ppplog(MSG_INFO_MED, al, "No flow control selected\n");
		break;
	}

	trm.c_lflag = 0;
	trm.c_oflag = 0;

	/* Hangup on close. */
	trm.c_cflag |= HUPCL;

	trm.c_cc[VINTR] = 0;
	trm.c_cc[VQUIT] = 0;
	trm.c_cc[VERASE] = 0;
	trm.c_cc[VKILL] = 0;
	trm.c_cc[VTIME] = 0;
	trm.c_cc[VEOL2] = 0;
	trm.c_cc[VMIN] = 1;

	if (tcsetattr(al->ah_link.al_fd, TCSANOW, &trm) < 0) {
		ppplog(MSG_ERROR, al,
		       "link_async_cond: tcsetattr() failed, error %d\n",
		       errno);
		return errno;
	}
	return 0;
}

/*
 * Set up a physical link for use by PPP
 *
 * This function must be called with the link lock held.
 */
int
link_condition(act_hdr_t *al)
{
	char modname[FMNAMESZ + 1], *p, *m,  xmodname[FMNAMESZ + 1];
	int ret;
	struct cfg_link *cl = (struct cfg_link *)al->ah_cfg;

	ASSERT(MUTEX_LOCKED(&al->ah_mutex));

	if (al->ah_link.al_flags & ALF_COND)
		return EBUSY;

	if (!(al->ah_link.al_flags & ALF_PHYS_UP))
		return EUNATCH;

	ppplog(MSG_DEBUG, al, "link_condition: reported speed = %d\n",
	       al->ah_link.al_connect->speed);

	/*
	 * If the bandwidth wasn't configured, use that
	 * obtained from Call Services
	 */
	if (al->ah_link.al_bandwidth == 0)
		al->ah_link.al_bandwidth = al->ah_link.al_connect->speed;

	/*
	 * Pop any defined modules
	 */
	p = ucfg_str(&cl->ln_ch, cl->ln_pop);
	
	while (p && *p && isspace(*p))
		p++;

	while (p && *p) {
		m = modname;

		while(*p && *p != FIELD_SEP) {
			*m = *p;
			m++;
			p++;
		}
		*m = 0;

		if (*p == FIELD_SEP)
			p++;

		while (*p && isspace(*p))
			p++;

		ppplog(MSG_DEBUG, al, "link_condition: Pop %s ...\n", modname);

		/*
		 * Check that the module that is under the stream head is
		 * the one we expect
		 */
		ret = ioctl(al->ah_link.al_fd, I_LOOK, xmodname);
		if (ret < 0) {
			ppplog(MSG_ERROR, al,
			       "Failed to I_LOOK (%d)\n", errno);
			if (errno == EINVAL)
				/* No module present */
				continue;
			else {
				ret = errno;
				goto exit;
			}
		}

		if (strcmp(xmodname, modname) != 0) {
			ppplog(MSG_WARN, al,
 "Module below stream head is not %s as expected (found %s) - didn't POP\n",
			       modname, xmodname);
			continue;
		}

		ret = ioctl(al->ah_link.al_fd, I_POP, 0);
		if (ret < 0) {
			ppplog(MSG_ERROR, al,
			       "Failed to I_POP module %s (%d)\n",
			       modname, errno);
			ret = errno;
			goto exit;
		}
	}

	/*
	 * Push any defined modules
	 */

	p = ucfg_str(&cl->ln_ch, cl->ln_push);
	while (p && *p && isspace(*p))
		p++;

	while (p && *p) {
		m = modname;
		while(*p && *p != FIELD_SEP) {
			*m = *p;
			m++;
			p++;
		}
		*m = 0;

		if (*p == FIELD_SEP)
			p++;

		while (*p && isspace(*p))
			p++;

		ppplog(MSG_DEBUG, al, "link_condition: Push %s ...\n", modname);

		ret = ioctl(al->ah_link.al_fd, I_PUSH, modname);
		if (ret < 0) {
			ppplog(MSG_ERROR, al,
			       "Failed to I_PUSH module %s (%d)\n",
			       modname, errno);
			ret = errno;
			goto exit;
		}
	}

	switch (al->ah_link.al_type) {
	case LNK_STATIC:
	case LNK_ANALOG:
	case LNK_ISDNVOC:
		/* All Async ... */
		ret = link_async_cond(al);
		break;

	case LNK_ISDN:
		ret = 0;
		break;	
	
	case LNK_TCP:
		/* Could be a pty */
		if (isatty(al->ah_link.al_fd))
			ret = link_async_cond(al);
		else
			ret = 0;
		break;
	}

	if (ret)
		goto exit;

	/* Link this below the pcid driver */

	ret = ioctl(pcid_fd, I_LINK, al->ah_link.al_fd);
	if (ret < 0) {
		ppplog(MSG_ERROR, al, "I_LINK of link below ppp failed (%d)\n",
		       errno);
		ret = errno;
		goto exit;
	}

	al->ah_index = ret;

	ppplog(MSG_INFO_LOW, al, "Links index %d\n", al->ah_index);

	ret = 0;
	al->ah_link.al_flags |= ALF_COND;

	ppp_phase(al, PHASE_ESTAB);
exit:
	if (ret) {
		link_undial(al);
		al->ah_link.al_flags = 0;
		al->ah_link.al_reason |= ALR_DIAL;
	}
	return(ret);
}

/*
 * This function is called when we have an incoming call on a
 * device. It ensures the line is ready for use by PPP
 */
int
link_incoming(struct act_hdr_s *al, int fd, char *uid, calls_t *call)
{
	int ret;
	act_hdr_t *ab = NULL;

	ASSERT(MUTEX_LOCKED(&al->ah_mutex));

	ppplog(MSG_INFO_MED, al, "Incoming call on link.\n");

	/*
	 * Store the id parameters int the link structure
	 */
	al->ah_link.al_fd = fd;
	al->ah_link.al_reason = 0;

	/*
	 * These parameter are used for authentication and
	 * incomig configuration determination.
	 */
	al->ah_link.al_uid = strdup(uid);
	al->ah_link.al_cid = strdup(call->caller_telno);
	al->ah_link.al_connect = call;

	ASSERT(al->ah_link.al_flags == 0);

	al->ah_link.al_flags = ALF_PHYS_UP | ALF_INUSE | ALF_INCOMING;

	/*
	 * If we have a user id of 'root' then we assume that we had
	 * an auto detected PPP session.
	 */
	if (!*uid)
		al->ah_link.al_flags |= ALF_AUTO;
	
	ret = link_condition(al);
	if (ret) {
		if (ab) {
			ATOMIC_INT_INCR(al->ah_refcnt);
			MUTEX_UNLOCK(&al->ah_mutex);
			MUTEX_UNLOCK(&ab->ah_mutex);
			MUTEX_LOCK(&al->ah_mutex);
			ATOMIC_INT_DECR(al->ah_refcnt);
		}
		ppplog(MSG_DEBUG, al, "link_incoming: ret = %d\n", ret);
		al->ah_link.al_reason |= ALR_DIAL;
		hist_add(HOP_ADD, al, ENODEV);
		return(ret);
	}

	if (!(al->ah_link.al_flags & ALF_AUTO))
		/*
		 * If the link was not auto detected PPP, then try and
		 * add it to a bundle before starting PPP negotiations
		 */
		ab = (act_hdr_t *)bundle_incoming_join(al);

	fsm_state(al->ah_link.al_lcp, OPEN, 0);

	if (ab) {
		ATOMIC_INT_INCR(al->ah_refcnt);
		MUTEX_UNLOCK(&al->ah_mutex);
		MUTEX_UNLOCK(&ab->ah_mutex);
		MUTEX_LOCK(&al->ah_mutex);
		ATOMIC_INT_DECR(al->ah_refcnt);
	}

	ppplog(MSG_DEBUG, al, "link_incoming: ret = %d\n", ret);
	return(ret);
}

/*
 * Do a STREAMS ioctl call
 */
int
sioctl(int fd, int cmd, caddr_t dp, int len)
{
	struct strioctl	iocb;

	iocb.ic_cmd = cmd;
	iocb.ic_timout = 15;
	iocb.ic_dp = dp;
	iocb.ic_len = len;
	return ioctl(fd, I_STR, &iocb);
}

int
control_hwd_flow(int fd, int enable)
{
	struct termiox termx;
    
	if (sioctl(fd, TCGETX, (caddr_t) &termx, sizeof(termx)) == -1) 
		return(-1);

	if (enable)
		termx.x_hflag |= (RTSXOFF | CTSXON);
	else
		termx.x_hflag &= ~(RTSXOFF | CTSXON);

	if (sioctl(fd, TCSETX, (caddr_t) &termx, sizeof(termx)) == -1) 
		return(-1);

	return(0);
}
