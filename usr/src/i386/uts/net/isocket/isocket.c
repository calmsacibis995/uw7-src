#ident	"@(#)isocket.c	1.3"
#ident	"$Header$"

/*
 *	Streams module for
 *	Socket to TLI conversion (isocket).
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1988 INTERACTIVE Systems Corporation
 *	All rights reserved.
 *
 *	RESTRICTED RIGHTS:
 *
 *	These programs are supplied under a license.  They may be used,
 *	disclosed, and/or copied only as permitted under such license
 *	agreement.  Any copy must contain the above copyright notice and
 *	this restricted rights notice.  Use, copying, and/or disclosure
 *	of the programs is strictly prohibited unless otherwise provided
 *	in the license agreement.
 */

#ifdef DEBUG
#define SOCDEBUG
#endif

#include <acc/priv/privilege.h>
#include "util/types.h"
#include "util/sysmacros.h"
#include "util/param.h"
#include "svc/errno.h"
#include "proc/signal.h"
#include "io/conf.h"
#include "io/stream.h"
#include "io/stropts.h"
#include "util/cmn_err.h"
#include "mem/kmem.h"
#include "mem/vmparam.h"
#include "util/debug.h"
#include "net/tihdr.h"
#include "net/tiuser.h"
#include "net/timod.h"
#include "net/tiuser.h"
#include "net/sockmod.h"
#include "net/socket.h"
#include "net/sockio.h"
#include "net/inet/in.h"
#include "net/inet/in_systm.h"
#include "net/inet/tcp.h"
#include "net/inet/if.h"
#include "net/inet/if_arp.h"
#include "net/inet/route.h"
#include "net/isocket/isocket.h"
#include <util/mod/moddefs.h>
#include "io/ddi.h"

STATIC int isoctemplate(struct isocdev *);
STATIC int isocldata(queue_t *, mblk_t *, int);
STATIC int isociocresp(queue_t *, mblk_t *);
STATIC int isocudata(queue_t *, mblk_t *, int);
STATIC void isocmread(queue_t *, mblk_t *);
STATIC int isoclproto(queue_t *, mblk_t *);
STATIC int isocuproto (queue_t *, mblk_t *);
STATIC int isocatmark(queue_t *);
STATIC int isoc_shutdown(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocretname(queue_t *, mblk_t *, struct isocdev *);
STATIC int isoc_udatadone(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocbindack(queue_t *, mblk_t *, struct isocdev *);
STATIC int isoclistenack(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocoptack(queue_t *, mblk_t *, struct isocdev *);
STATIC int isoc_shdownack(queue_t *, mblk_t *, struct isocdev *);

STATIC int isocsocket(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocbind(queue_t *, mblk_t *, struct isocdev *, int);
STATIC int isoclisten(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocconn(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocconncon(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocconnind(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocaccept(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocfdinsert(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocsendup(queue_t *, mblk_t *);
STATIC int isocsend(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocdataind(queue_t *, mblk_t *, struct isocdev *, int);
STATIC int isocudata(queue_t *, mblk_t *, int);
STATIC int isocudataind(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocldata(queue_t *, mblk_t *, int);
STATIC int isocdiscreq(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocgetname(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocgetpeer(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocgetopt(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocsetopt(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocshutdown(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocdiscind(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocordrelind(queue_t *, mblk_t *, struct isocdev *);
STATIC int isocterror(queue_t *, mblk_t *, struct isocdev *);
STATIC int isoc_aligned_copy(char *, int, int, char *, int *);
STATIC int isoc_sendioctl(queue_t *, mblk_t *, struct isocdev *, 
		mblk_t *, int, int, int);
STATIC int isoc_smoderror(queue_t *, mblk_t *, struct isocdev *);
STATIC int isoc_sendproto(queue_t *, mblk_t *, struct isocdev *, int, int);
STATIC int isoc_optionlength(int, int);
STATIC int isoc_route(queue_t *, mblk_t *, struct isocdev *, int);
STATIC int isocretopt(queue_t *, mblk_t *, int, int);
STATIC int isoc_getudata(queue_t *, mblk_t *, struct isocdev *);

int isocdevflag = 0;

MOD_STR_WRAPPER(isoc, NULL, NULL, "isoc - ISC socket emulation");

# define SOCTRACE	0x01		/* routine entry */
# define SOCCMDS	0x02		/* command flow */
# define SOCTRC		0x04		/* misc. routine entry */
# define SOCERRS	0x10		/* error reports */


#ifdef SOCDEBUG
STATIC int isocdebug = 0;
STATIC char	*hex_sprintf();

#define LOG0(type, str ) \
		{ \
			if (isocdebug & type) \
				printf (str);\
		}

#define LOG1(type, str, a1) \
		{ \
			if (isocdebug & type) \
				printf (str, a1);\
		}

#define LOG2(type, str, a1, a2) \
		{ \
			if (isocdebug & type) \
				printf (str, a1, a2);\
		}

#define LOG3(type, str, a1, a2, a3) \
		{ \
			if (isocdebug & type) \
				printf (str, a1, a2, a3);\
		}

#define LOG4(type, str, a1, a2, a3, a4)  \
		{ \
			if (isocdebug & type) \
				printf (str, a1, a2, a3, a4); \
		}
#else
#define LOG0(type, str)
#define LOG1(type, str, a1)
#define LOG2(type, str, a1, a2)
#define LOG3(type, str, a1, a2, a3)
#define LOG4(type, str, a1, a2, a3, a4)
#endif /* SOCDEBUG */

#define inbcopy(src, dst, len) 	bcopy(src, dst, len)

/*
 * Standard streams initialization for a module
 */

STATIC int isocopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int isocclose(queue_t *, int, cred_t *);
STATIC int isocrput(queue_t *, mblk_t *);
STATIC int isocwput(queue_t *, mblk_t *);
STATIC int isocrsrv(queue_t *);
STATIC int isocwsrv(queue_t *);
STATIC void isocioctl(queue_t *, mblk_t *);
STATIC void isocack(queue_t *, mblk_t *, int, int);
STATIC void isocaccack(queue_t *);

STATIC struct module_info isocminfo = {
	0, "isocket", 0, MAXSOCSZ, SOCHIWAT, SOCLOWAT
};

STATIC struct qinit isocrinit = {		/* read */
	isocrput, isocrsrv, isocopen, isocclose, NULL, &isocminfo, NULL
};

STATIC struct qinit isocwinit = {		/* write */
	isocwput, isocwsrv, NULL,    NULL,     NULL, &isocminfo, NULL
};

struct streamtab isocinfo = {
	&isocrinit, &isocwinit, NULL, NULL };


/*
 * mapping of TLI primitives to isocket primitives for error returns
 */
#define N_TLI_PRIMS	11
STATIC int isocTLI_prims[N_TLI_PRIMS] = {
	ISC_SO_CONNECT, 	/* T_CONN_REQ		0 */
	ISC_SO_FDINSERT, 	/* T_CONN_RES		1 */
	ISC_SO_SHUTDOWN, 	/* T_DISCON_REQ		2 */
	-1, 	 		/* T_DATA_REQ		3 */
	-1, 			/* T_EXDATA_REQ		4 */
	-1, 			/* T_INFO_REQ		5 */
	ISC_SO_BIND, 		/* T_BIND_REQ		6 */
	-1, 	 		/* T_UNBIND_REQ		7 */
	ISC_SO_SEND, 		/* T_UNITDATA_REQ	8 */
	ISC_SO_SETOPT, 		/* T_OPTMGMT_REQ	9 */
	ISC_SO_SHUTDOWN 	/* T_ORDREL_REQ		10 */
};


/*
 * Mapping of TLI errors to system errors and extended errors for isocket
 * users.  These won't be used if the original extended error is available
 * in UNIX_error.
 */
#define N_TLI_ERRS	20
STATIC int UNIX_error[N_TLI_ERRS] = {
	0,
	EDESTADDRREQ, 	/* 1  TBADADDR: incorrect addr format */
	ENOPROTOOPT, 	/* 2  TBADOPT: incorrect option format */
	EACCES, 	/* 3  TACCES: incorrect permissions */
	EBADF, 		/* 4  TBADF: illegal transport fd */
	EADDRNOTAVAIL, 	/* 5  TNOADDR: couldn't allocate addr */
	0, 		/* 6  TOUTSTATE: out of state */
	EPROTO, 	/* 7  TBADSEQ: bad call sequence number */
	0, 		/* 8  TSYSERR: system error */
	EPROTO, 	/* 9  TLOOK: event requires attention */
	EMSGSIZE, 	/* 10 TBADDATA: illegal amount of data */
	ENOSR, 		/* 11 TBUFOVFLW: buffer not large enough */
	EAGAIN, 	/* 12 TFLOW: flow control */
	EAGAIN, 	/* 13 TNODATA: no data */
	EPROTO, 	/* 14 TNODIS: discon_ind not found on q */
	EPROTO, 	/* 15 TNOUDERR: unitdata error not found */
	EPROTO, 	/* 16 TBADFLAG: bad flags */
	EPROTO, 	/* 17 TNOREL: no ord rel found on q */
	EOPNOTSUPP, 	/* 18 TNOTSUPPORT: primitive not supported */
	EALREADY	/* 19 TSTATECHNG: state is in process of changing */
};


/*
 * Mapping of SVR4 isocket ioctls to ISC's and vice versa. 
 * Table is indexed by the lowest 8 bit value. Some values clash, however, 
 * fortunately they are either irrelevant or should be unique, for the purpose
 * of the ISC's isocket module compatibility to SVR4 work.
 */
#define BAD_CMD		0xff
#define IOCTL_INDEX(x)	(x&0xff)
#define N_ISC_sioctl	42
STATIC unsigned long ISC_to_SVR4_sioctl[N_ISC_sioctl] = {

	/*  0 */		SIOCSHIWAT, 	
	/*  1 */		SIOCGHIWAT,
	/*  2 */		SIOCSLOWAT,
	/*  3 */		SIOCGLOWAT,
	/*  4 */		BAD_CMD,
	/*  5 */		BAD_CMD,
	/*  6 */		BAD_CMD,
	/*  7 */		SIOCATMARK,
	/*  8 */		SIOCSPGRP,
	/*  9 */		SIOCGPGRP,
	/* SIOCSHUTDOWN, which is also 10, 
	 * is handled at the isocket module level and
	 * do not go through this table.
	 */
	/*  10 */		SIOCADDRT,
	/*  11 */		SIOCDELRT,
	/*  12 */		SIOCSIFADDR,
	/*  13 */		SIOCGIFADDR,
	/*  14 */		SIOCSIFDSTADDR,
	/*  15 */		SIOCGIFDSTADDR,
	/*  16 */		SIOCSIFFLAGS,
	/*  17 */		SIOCGIFFLAGS,
	/*  18 */		SIOCGIFBRDADDR,
	/*  19 */		SIOCSIFBRDADDR,
	/*  20 */		SIOCGIFCONF,
	/*  21 */		SIOCGIFNETMASK,
	/*  22 */		SIOCSIFNETMASK,
	/*  23 */		SIOCGIFMETRIC,
	/*  24 */		SIOCSIFMETRIC,
	/*  25 */		SIOCSIFNAME,
	/*  26 */		BAD_CMD,
	/*  27 */		BAD_CMD,
	/*  28 */		BAD_CMD,
	/*  29 */		BAD_CMD,
	/*  30 */		SIOCSARP,
	/*  31 */		SIOCGARP,
	/*  32 */		SIOCDARP,
	/*  33 */		BAD_CMD,
	/*  34 */		BAD_CMD,
	/*  35 */		BAD_CMD,
	/*  36 */		BAD_CMD,
	/*  37 */		BAD_CMD,
	/* SIOCGPHYSADDR can be mapped to SIOCGENADDR. However, 
	 * SIOCSCKSUM, is also a 38 and it is not supported by SVR4. 
	 * So to make it unambiguous, we do not support both. 
	 * SIOCGPHYSADDR is only used by administrative type 
	 * applications, and we do not provide binary compatibility 
	 * for those anyway. 
	 */
	/*  38 */		BAD_CMD,
	/*  39 */		BAD_CMD,
	/*  40 */		BAD_CMD,
	/* SIOCGETNAME is made obsolete and replaced by TI_GETMYNAME
	 * However, TI_GETMYNAME is complicated. For now we implement 
	 * it as SIOCGETNAME.  
	 */
	/*  41 */		SIOCGETNAME,
};


/*
 * Mapping of SVR4 networking error codes to ISC error codes. 
 * There are two exceptional error codes that are not represented in the table,
 * that is, EWOULDBLOCK and ELOOP. ISC defines both as part of the extended 
 * error codes, while SVR4 maps EWOULDBLOCK to EAGAIN, and ELOOP is defined as
 * part of shared library error codes.
 */
#define N_SVR4NET_ERRS	58
#define SVR4_NETERR_BASE	95
STATIC int	SVR4_to_ISC_error[N_SVR4NET_ERRS] = {
	/* BSD Networking Software */
	/* SVR4 error number */
	/*  95 */		ISC_ENOTSOCK, /* Socket call on non-isocket */
	/*  96 */		ISC_EDESTADDRREQ,
	/*  97 */		ISC_EMSGSIZE,
	/*  98 */		ISC_EPROTOTYPE,
	/*  99 */		ISC_ENOPROTOOPT,
	/*  100 */		NULL,
	/*  101 */		NULL,
	/*  102 */		NULL,
	/*  103 */		NULL,
	/*  104 */		NULL,
	/*  105 */		NULL,
	/*  106 */		NULL,
	/*  107 */		NULL,
	/*  108 */		NULL,
	/*  109 */		NULL,
	/*  110 */		NULL,
	/*  111 */		NULL,
	/*  112 */		NULL,
	/*  113 */		NULL,
	/*  114 */		NULL,
	/*  115 */		NULL,
	/*  116 */		NULL,
	/*  117 */		NULL,
	/*  118 */		NULL,
	/*  119 */		NULL,
	/*  120 */		ISC_EPROTONOSUPPORT,
	/*  121 */		ISC_ESOCKTNOSUPPORT,
	/*  122 */		ISC_EOPNOTSUPP,
	/*  123 */		ISC_EPFNOSUPPORT,
	/*  124 */		ISC_EAFNOSUPPORT,
	/*  125 */		ISC_EADDRINUSE,
	/*  126 */		ISC_EADDRNOTAVAIL,
	/*  127 */		ISC_ENETDOWN,
	/*  128 */		ISC_ENETUNREACH,
	/*  129 */		ISC_ENETRESET,
	/*  130 */		ISC_ECONNABORTED,
	/*  131 */		ISC_ECONNRESET,
	/*  132 */		ISC_ENOBUFS,
	/*  133 */		ISC_EISCONN,
	/*  134 */		ISC_ENOTCONN,

	/* XENIX has 135 - 142 */
	/*  135 */		EUCLEAN,
	/*  136 */		NULL,
	/*  137 */		ENOTNAM,
	/*  138 */		ENAVAIL,
	/*  139 */		EISNAM,
	/*  140 */		EREMOTEIO,
	/*  141 */		EINIT,
	/*  142 */		EREMDEV,

	/*  143 */		ISC_ESHUTDOWN,
	/*  144 */		ISC_ETOOMANYREFS,
	/*  145 */		ISC_ETIMEDOUT,
	/*  146 */		ISC_ECONNREFUSED,
	/*  147 */		ISC_EHOSTDOWN,
	/*  148 */		ISC_EHOSTUNREACH,
	/*	ISC_EWOULDBLOCK, mapped to EAGAIN 	*/
	/*  149 */		ISC_EALREADY,
	/*  150 */		ISC_EINPROGRESS,

	/* SUN Network File System */
	/*  151 */		NULL	/* ESTALE - Stale NFS file handle */
};


/*
 * STATIC int
 * isocopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *crp)
 *	isocopen - stream open routine.
 *
 * Calling/Exit State:
 */

/*ARGSUSED*/
STATIC int
isocopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *crp)
{
	struct isocdev *sd;

	if (q->q_ptr != NULL)
		return (0);		/* already attached */

	if (sflag != MODOPEN)
		return (EINVAL);

	sd = (struct isocdev *)kmem_zalloc(sizeof(struct isocdev), KM_SLEEP);


	/* setup data structures */
	WR(q)->q_ptr = (caddr_t) sd;
	q->q_ptr = (caddr_t) sd;
	sd->isoc_qptr = WR(q);
	sd->isoc_state = ISC_SS_OPENED;

	/* SVR4 data-structures */
	/* Sockmod user data */
	sd->isoc_udata = (struct si_udata *) 
		kmem_zalloc(sizeof(struct si_udata), KM_SLEEP);

	/* additional space for aligned copy */
	sd->isoc_roundup = ROUNDUP(1) * 2;
	return (0);
}


/*
 * STATIC int isocdrain (queue_t *q)
 *
 * Calling/Exit State:
 */
STATIC int
isocdrain (queue_t *q)
{
	struct isocdev *sd;

	sd = (struct isocdev *) q->q_ptr;

	if (q->q_count) {
		sd->isoc_flags |= SOCF_DRAIN;
		sleep((caddr_t)&q->q_count, PZERO + 1);
	}
	return(0);
}

/*
 * STATIC int isocclose(queue_t *q, int flag, cred_t *crp)
 *	isocclose - stream close routine.
 *
 * Calling/Exit State:
 */
/*ARGSUSED*/
STATIC int
isocclose(queue_t *q, int flag, cred_t *crp)
{
	struct isocdev *sd;

	LOG2 (SOCTRACE, "isocclose(%x, %x)\n", q, flag);

	sd = (struct isocdev *) q->q_ptr;

	/* only call isocdrain if we are connected */
	if (sd->isoc_state == ISC_SS_CONNECTED || 
	    sd->isoc_state == ISC_SS_CONN_WONLY) {
		isocdrain(WR(q));
	}
	if (sd->isoc_head && sd->isoc_head->isoc_state == ISC_SS_WACK_CRES) {
		mblk_t * mp;
		struct isocdev *osd = sd->isoc_head;

		(void) isoctemplate (sd);
		sd->isoc_state = ISC_SS_CONNECTED;
		osd->isoc_state = ISC_SS_PASSIVE;
		osd->isoc_spend = NULL;

		if ((mp = allocb (SO_OK_ACK_SIZE, BPRI_HI)) != NULL)
			isocack (osd->isoc_qptr, mp, ISC_SO_FDINSERT, 
				 M_PCPROTO);

		if (osd->isoc_npend > 0) {
			struct SO_ok_ack *ack;
			mp = allocb(SO_OK_ACK_SIZE, BPRI_HI); /* important */
			if (mp != NULL) {
				mp->b_datap->db_type = M_PROTO;
				/* LINTED pointer alignment */
				ack = (struct SO_ok_ack *)(mp->b_rptr);
				mp->b_wptr = mp->b_rptr + SO_OK_ACK_SIZE;
				ack->PRIM_type = ISC_SO_OK_ACK;
				ack->CORRECT_prim = ISC_SO_ACCEPT;
				qreply(osd->isoc_qptr, mp); /* put as M_PROTO */
			}
		}
	}

	if (sd->isoc_opt)
		freemsg (sd->isoc_opt);

	if (sd->isoc_template)
		freeb (sd->isoc_template);

	while (sd->isoc_pendlist != NULL) {
		mblk_t * mblk;
		mblk = sd->isoc_pendlist;
		sd->isoc_pendlist = mblk->b_next;
		mblk->b_next = NULL;
		freemsg(mblk);
	}

	/* Release the SVR4 Private data structures */
	kmem_free(sd->isoc_udata, sizeof(struct si_udata));
	sd->isoc_udata = NULL;

	/* Release the socdev data structure */
	kmem_free((caddr_t)sd, sizeof(struct isocdev));
	return(0);
}

/*
 * STATIC int isocrput(queue_t *q, mblk_t *mp)
 *	isocrput - stream read put routine.
 *
 * Calling/Exit State:
 */
STATIC int
isocrput(queue_t *q, mblk_t *mp)
{
	LOG2 (SOCTRC, "isocrput(%x, %x)\n", q, mp);

	switch (mp->b_datap->db_type) {

	case M_FLUSH:		/* canonical module flush handling */
		if (*mp->b_rptr & FLUSHR)
			flushq(q, FLUSHDATA);
		if (*mp->b_rptr & FLUSHW)
			flushq(WR(q), FLUSHDATA);
		putnext(q, mp);	/* send it on */
		break;

	case M_ERROR:
	case M_SETOPTS:		/* in case of future change */
		putnext(q, mp);
		break;

	case M_DATA:					/* no waiting on data */
		if (isocldata(q, mp, 1))		/* !canput */
			return(0);
		break;

	case M_PROTO:
	case M_PCPROTO:
		putq(q, mp);
		qenable(q);
		break;

	case M_IOCACK:
	case M_IOCNAK:
		if (isociocresp(q, mp))		/* !canput */
			return(0);
		break;

	default:

		LOG1 (SOCERRS, "isocrput: Unexpected packet type: %d\n", mp->b_datap->db_type);
		freemsg (mp);
	}
	return(0);
}

/*
 * STATIC int isocwput(queue_t *q, mblk_t *mp)
 *	isocwput - stream write put routine.
 *
 * Calling/Exit State:
 */
STATIC int
isocwput(queue_t *q, mblk_t *mp)
{

	LOG2 (SOCTRC, "isocwput(%x, %x)\n", q, mp);

	switch (mp->b_datap->db_type) {

	case M_IOCTL:		/* no waiting in ioctl's */
		isocioctl(q, mp);
		break;

	case M_FLUSH:		/* canonical module flush handling */
		if (*mp->b_rptr & FLUSHW)
			flushq (q, FLUSHDATA);
		if (*mp->b_rptr & FLUSHR)
			flushq(RD(q), FLUSHDATA);
		putnext(q, mp);	/* send it on */
		break;

	case M_DATA:				/* no waiting on data */
		if (isocudata(q, mp, 1))		/* !canput */
			return(0);
		break;

	case M_PROTO:
	case M_PCPROTO:
		putq(q, mp);
		qenable(q);
		break;

	case M_READ:
		isocmread(q, mp);
		break;

	default:

		LOG1 (SOCERRS, "isocwput: Unexpected packet type: %d\n", mp->b_datap->db_type);
		freemsg (mp);
	}
	return(0);
}

/*
 * STATIC int isocrsrv(queue_t *q)
 *	isocrsrv - stream read service routine.
 *
 * Calling/Exit State:
 */
STATIC int
isocrsrv(queue_t *q)
{
	mblk_t * mp;

	LOG1 (SOCTRC, "isocrsrv(%x)\n", q);

	while ((mp = getq (q)) != NULL) {
		switch (mp->b_datap->db_type) {

		case M_DATA:
			if (isocldata (q, mp, 0))	/* !canput */
				return(0);
			break;

		case M_PCPROTO:
		case M_PROTO:
			if (isoclproto (q, mp))		/* !canput */
				return(0);
			break;

		default:
			LOG1 (SOCERRS, "isocrsrv: Unexpected packet type: %d\n",
				    mp->b_datap->db_type);
			freemsg (mp);
		}
	}
	return(0);
}

/*
 * STATIC int isocwsrv(queue_t *q)
 *	isocwsrv - stream write service routine.
 *
 * Calling/Exit State:
 */
STATIC int
isocwsrv(queue_t *q)
{
	mblk_t * mp;
	struct isocdev *sd = (struct isocdev *) q->q_ptr;

	LOG1 (SOCTRC, "isocwsrv(%x)\n", q);

	while ((mp = getq (q)) != NULL) {
		if (!q->q_count && sd->isoc_flags & SOCF_DRAIN) {
			wakeup((caddr_t)&q->q_count);
		}
		switch (mp->b_datap->db_type) {

		case M_DATA:
			if (isocudata (q, mp, 0))	/* !canput */
				return(0);
			break;

		case M_PCPROTO:
		case M_PROTO:
			if (isocuproto (q, mp))		/* !canput */
				return(0);
			break;

		default:
			LOG1 (SOCERRS,  "isocwsrv: Unexpected packet type:%d\n",
				    mp->b_datap->db_type);
			freemsg (mp);
		}
	}
	return(0);
}

/*
 * STATIC void isocioctl(queue_t *q, mblk_t *mp)
 *	isocioctl - handle ioctl's from the stream head
 *
 * Calling/Exit State:
 */
STATIC void
isocioctl(queue_t *q, mblk_t *mp)
{
	struct iocblk *iocp;
	struct isocdev *sd;
	mblk_t	*ump;

	LOG2 (SOCTRACE, "isocioctl(%x, %x)\n", q, mp);

	sd = (struct isocdev *) q->q_ptr;
	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;

	/* Pending Command for error reporting */
	sd->isoc_cmd = iocp->ioc_cmd;
	sd->isoc_smodcmd = 0;

	switch (iocp->ioc_cmd) {

	case ISC_SIOCATMARK:
		if (iocp->ioc_count != sizeof (int)) {
			iocp->ioc_error = EINVAL;
			goto iocnak;
		}
		/* LINTED pointer alignment */
		*(int *)mp->b_cont->b_rptr = isocatmark (q);
		break;

	case ISC_SIOCSPGRP:			/* not supported */
	case ISC_SIOCGPGRP:
		iocp->ioc_error = ENXIO;
		goto iocnak;

	case ISC_SIOCOOBSIG: 
		 {		/* set/unset priority data signal */
			int	sig;
			if (iocp->ioc_count != sizeof (int)) {
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}
			/* LINTED pointer alignment */
			if ((sig = *(int *)mp->b_cont->b_rptr) >= NSIG) {
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}
			sd->isoc_prisig = (ushort)sig;
		}
		break;

	case ISC_SIOCSHUTDOWN:
		if (isoc_shutdown(q, mp, sd)) {
			goto iocnak;
		}
		return; /* ioc_shutdown sends its own ack to the ioctl */


	case ISC_SIOCSHIWAT:
	case ISC_SIOCGHIWAT:
	case ISC_SIOCSLOWAT:
	case ISC_SIOCGLOWAT:
	default:				/* unrecognized; forward */
		LOG1 (SOCCMDS, "isocioctl: forwarding ioctl: %x\n", iocp->ioc_cmd);

		/* Ensure that the forwarded ioctl is atomic */
		if (sd->isoc_flags & SOCF_IOCTLWAT) {
			iocp->ioc_error = EPROTO;
			goto iocnak;
		}
		sd->isoc_flags |= SOCF_IOCTLWAT;
		sd->isoc_ioctl = iocp->ioc_id;
		sd->isoc_ioctlcmd = iocp->ioc_cmd;
		/* Map ISC isocket ioctls to that of SVR4 */
		 { 
			int	index;
			long	cmd;

			index = IOCTL_INDEX(iocp->ioc_cmd);
			if (index < 0 || index >= N_ISC_sioctl) {
				/* Ioctl is out of range of the 
				 *table, return a NAK 
				 */
				iocp->ioc_error = ENXIO; /* Not supported */
				goto iocnak;
			} else {
				cmd = ISC_to_SVR4_sioctl[index];
				if (cmd == BAD_CMD) {
					/* Not supported */
					iocp->ioc_error = ENXIO;
					goto iocnak;
				} else
					iocp->ioc_cmd = cmd;
			}

			/* SVR4 requires an ifreq datastructure
			 * ISC passes in an ifconf datastructure that is different from SVR4
			 *
			 * Reserve space for ifc_len
			 * Assuming Compiler alignemnt 
			 */

			if (sd->isoc_ioctlcmd == ISC_SIOCGIFCONF) {
				ump = mp->b_cont;
				if (ump == NULL)
					goto iocnak;
				
				ump->b_rptr += sizeof(int);
			}
			putnext (q, mp);
		}
		return;
	}
	sd->isoc_cmd = 0;
	mp->b_datap->db_type = M_IOCACK;
	qreply (q, mp);
	return;

iocnak:
	sd->isoc_cmd = 0;
	mp->b_datap->db_type = M_IOCNAK;
	qreply (q, mp);
}

/*
 * STATIC int isociocresp(queue_t *q, mblk_t *mp)
 *	isociocresp - handle ioctl ACK's and NAK's
 *
 * Calling/Exit State:
 */
STATIC int
isociocresp(queue_t *q, mblk_t *mp)
{
	struct iocblk *iocp;
	struct isocdev *sd;
	int	*ifc_lenp;
	mblk_t 	*ump;


	LOG2 (SOCTRACE, "isociocresp(%x, %x)\n", q, mp);

	sd = (struct isocdev *) q->q_ptr;
	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;
	if (!(sd->isoc_flags & SOCF_IOCTLWAT) || 
	    (sd->isoc_ioctl != iocp->ioc_id)) {

		LOG1 (SOCERRS, "isociocresp: Unexpected ioctl response: %x\n",
			    iocp->ioc_cmd);
		freemsg (mp);
		return(0);
	}

	sd->isoc_flags &= ~SOCF_IOCTLWAT;
	sd->isoc_ioctl = 0;

	/* handle locally generated ioctl's */
	/* Use saved sockmod cmd because sockmod changes this on us */
	switch (sd->isoc_smodcmd) {	
	case TI_GETMYNAME:		/* get local isocket name */
		if (mp->b_cont != NULL) {
			if (mp->b_datap->db_type == M_IOCACK) {
				int len = mp->b_cont->b_wptr - 
					       mp->b_cont->b_rptr;

				if (len > sizeof (struct isockaddr ))
					sd->isoc_slen = 
					    sizeof (struct isockaddr );
				else
					sd->isoc_slen = (ushort)len;
				inbcopy (mp->b_cont->b_rptr, 
					 &sd->isoc_saddr, 
					 sd->isoc_slen);
			}
			freemsg (mp->b_cont);
			mp->b_cont = NULL;
		}
		return (isocretname (q, mp, sd));

	case SI_GETUDATA:
		return (isoc_udatadone(q, mp, sd));

	case TI_BIND:
		return (isocbindack (q, mp, sd));

	case SI_LISTEN:
		return (isoclistenack (q, mp, sd));

	case TI_OPTMGMT:
		return(isocoptack(q, mp, sd));

	case SI_SHUTDOWN:
		return(isoc_shdownack(q, mp, sd));

	default:			
		LOG1 (SOCTRACE, "isociocresp User ioctl 0x%x \n", iocp->ioc_cmd); 

		/* send up response to user ioctl */
		/* Map the ioctls back from SVR4's to ISC's */
		if ( iocp->ioc_cmd != 
		   ISC_to_SVR4_sioctl[IOCTL_INDEX(sd->isoc_ioctlcmd)]) {
			/* Not the ioctl we are expecting */
			iocp->ioc_cmd = sd->isoc_ioctlcmd;
			mp->b_datap->db_type = M_IOCNAK;
		} else
			iocp->ioc_cmd = sd->isoc_ioctlcmd;

		if (sd->isoc_ioctlcmd == ISC_SIOCGIFCONF) {
			ump = mp->b_cont;
			if (ump == NULL) {
				mp->b_datap->db_type = M_IOCNAK;
			} else {
				/* SVR4 fills in an ifreq datastructure 
				 * ISC expects an ifconf datastructure that is different 
				 * from SVR4 
				 *
				 * Use the reserved space allocated in isocioctl()
				 * Assuming Compiler alignemnt 
				 */
				ump->b_rptr -= sizeof(int);
				/* LINTED pointer alignment */
				ifc_lenp = (int *)ump->b_rptr;
				*ifc_lenp = iocp->ioc_count;
				iocp->ioc_count += sizeof(int);
				iocp->ioc_rval = 0;
			}
		}


		putnext (q, mp);
	}
	sd->isoc_ioctlcmd = 0;
	return (0);
}

/*
 * STATIC void isocack (queue_t *q, mblk_t *mp, int prim, int type)
 *	isocack - build and send an ok acknowledgement for the primitive.
 *	Try to reuse the original message block if it is large enough.
 *
 * Calling/Exit State:
 */
STATIC void
isocack (queue_t *q, mblk_t *mp, int prim, int type)
{
	mblk_t *nmp;
	struct SO_ok_ack *sprim;

	LOG3 (SOCTRACE, "isocack(%x, %x, %x)\n", q, mp, prim);

	if (SO_OK_ACK_SIZE <= (mp->b_datap->db_lim - mp->b_datap->db_base)) {
		mp->b_rptr = mp->b_datap->db_base;
		nmp = mp;
	} else if ((nmp = allocb (SO_OK_ACK_SIZE, BPRI_HI)) == NULL) {
		/*
		 *+ Kernel could not allocate memory for a streams message.
		 */
		cmn_err(CE_WARN,  "isocack: allocb failed");
		freemsg (mp);
	} else
		freemsg (mp);

	nmp->b_datap->db_type = (uchar_t)type;
	/* LINTED pointer alignment */
	sprim = (struct SO_ok_ack *)nmp->b_rptr;
	sprim->PRIM_type = ISC_SO_OK_ACK;
	sprim->CORRECT_prim = prim;
	nmp->b_wptr = nmp->b_rptr + SO_OK_ACK_SIZE;

	if (q->q_flag & QREADR)		/* read queue; send on up */
		putnext (q, nmp);
	else	/* write queue; reply */
		qreply (q, nmp);
}

/*
 * STATIC int isocnak(queue_t *q, mblk_t *mp, int err, int syserr, int prim)
 *	isocnak - build and send an error acknowledment for the primitive.
 *	Try to reuse the original message block if it is large enough.
 *	All operations have potential error acknowledgments.
 *
 * Calling/Exit State:
 */
STATIC int
isocnak(queue_t *q, mblk_t *mp, int err, int syserr, int prim)
{
	mblk_t * nmp;
	struct SO_error_ack *sprim;

	LOG3(SOCTRACE, "isocnak(q, mp, %x, %x, %x)\n",  err, syserr, prim);

	if (SO_ERROR_ACK_SIZE <= (mp->b_datap->db_lim - mp->b_datap->db_base)) {
		if (mp->b_cont) {
			freemsg (mp->b_cont);
			mp->b_cont = NULL;
		}
		mp->b_rptr = mp->b_datap->db_base;
		nmp = mp;
	} else if ((nmp = allocb (SO_ERROR_ACK_SIZE, BPRI_MED)) == NULL) {
		putbq (q, mp);
		if (bufcall (SO_ERROR_ACK_SIZE, BPRI_MED, qenable, (long)q))
			return (1);
		else {
			/* desparation time; use what we got */
			/* they can only close */
			nmp = mp;
			nmp->b_datap->db_type = M_ERROR;
			nmp->b_wptr = nmp->b_rptr + 1;
			*nmp->b_rptr = ENOSR;
			goto punt;
		}
	} else
		freemsg (mp);

	/* Map SVR4 UNIX error code to that of ISC when applicable.  */
	if ( (syserr >= SVR4_NETERR_BASE) && (syserr <= 
	    (SVR4_NETERR_BASE + N_SVR4NET_ERRS)))
		syserr = SVR4_to_ISC_error[syserr - SVR4_NETERR_BASE];
	nmp->b_datap->db_type = M_PCPROTO;
	/* LINTED pointer alignment */
	sprim = (struct SO_error_ack *)nmp->b_rptr;
	sprim->PRIM_type = ISC_SO_ERROR_ACK;
	sprim->ERROR_prim = prim;		/* this is the failing opcode */
	sprim->SOCK_error = err;
	sprim->UNIX_error = (err == ISC_SO_SYSERR) ? syserr : 0;

	LOG2 (SOCCMDS, "isocnak: %x, %x\n", sprim->SOCK_error, sprim->UNIX_error);

	nmp->b_wptr = nmp->b_rptr + SO_ERROR_ACK_SIZE;

punt:
	if (q->q_flag & QREADR)		/* read queue; send on up */
		putnext (q, nmp);
	else	/* write queue; reply */
		qreply (q, nmp);
	return (0);
}

/*
 * STATIC int isocuproto (queue_t *q, mblk_t *mp)
 *	isocuproto - read isocket protocol packets from stream head.
 *
 * Calling/Exit State:
 */
STATIC int
isocuproto (queue_t *q, mblk_t *mp)
{
	struct isocdev *sd = (struct isocdev *) q->q_ptr;
	/* LINTED pointer alignment */
	union SO_primitives *sprim = (union SO_primitives *)mp->b_rptr;

	LOG2 (SOCTRC, "isocuproto(%x, %x)\n", q, mp);

#ifdef SOCDEBUG
	if (sd->isoc_flags & SOCF_IOCTLWAT) {
		LOG1(SOCERRS, "Got proto while waiting for ioctl: %x\n", 
		sprim->SP_type);
	}
#endif /* SOCDEBUG */

	/* Pending Command -- For Error reporting */
	sd->isoc_cmd = sprim->SP_type;

	switch (sprim->SP_type) {
	case ISC_SO_SOCKET:
		if (sd->isoc_state == ISC_SS_OPENED) {
			if (isocsocket (q, mp, sd))
				return (1);
		} else
			return (isocnak (q, mp, ISC_SO_OUTSTATE, 0, 
					 ISC_SO_SOCKET));
		break;

	case ISC_SO_BIND:
		if (sd->isoc_state == ISC_SS_UNBND) {
			if (isocbind (q, mp, sd, SOMAXCONN))
				return (1);
		} else if (sd->isoc_state > ISC_SS_UNBND)
			return (isocnak (q, mp, ISC_SO_SYSERR, EINVAL, 
					 ISC_SO_BIND));
		else
			return (isocnak (q, mp, ISC_SO_OUTSTATE, 0, 
					 ISC_SO_BIND));
		break;

	case ISC_SO_LISTEN:			/* let isoclisten check state */
		if (isoclisten (q, mp, sd))
			return (1);
		break;

	case ISC_SO_ACCEPT:
		if (sd->isoc_state == ISC_SS_PASSIVE || 
		    sd->isoc_state == ISC_SS_ACCEPTING) {
			if (isocaccept (q, mp, sd))
				return (1);
		} else
			return (isocnak (q, mp, ISC_SO_OUTSTATE, 0, 
					 ISC_SO_ACCEPT));
		break;

	case ISC_SO_FDINSERT:
		if (sd->isoc_state == ISC_SS_WAIT_FD) {
			if (isocfdinsert (q, mp, sd))
				return (1);
		} else
			return (isocnak (q, mp, ISC_SO_OUTSTATE, 0, 
					 ISC_SO_FDINSERT));
		break;

	case ISC_SO_DISC_REQ:
		if (sd->isoc_state == ISC_SS_WAIT_FD) {
			if (isocdiscreq (q, mp, sd))
				return (1);
		} else
			return (isocnak (q, mp, ISC_SO_OUTSTATE, 0, 
					 ISC_SO_DISC_REQ));
		break;

	case ISC_SO_CONNECT:			/* let isocconn check state */
		if (isocconn (q, mp, sd))
			return (1);
		break;

	case ISC_SO_GETNAME:
		if (sd->isoc_state > ISC_SS_UNBND) {
			if (isocgetname (q, mp, sd))
				return (1);
		} else
			return (isocnak (q, mp, ISC_SO_OUTSTATE, 0, 
					 ISC_SO_GETNAME));
		break;

	case ISC_SO_GETPEER:
		if (sd->isoc_state == ISC_SS_CONNECTED || 
		    sd->isoc_state == ISC_SS_CONN_WONLY || 
		    sd->isoc_state == ISC_SS_CONN_RONLY) {
			if (isocgetpeer (q, mp, sd))
				return (1);
		} else
			return (isocnak (q, mp, ISC_SO_SYSERR, ENOTCONN, 
					 ISC_SO_GETPEER));
		break;

	case ISC_SO_GETOPT:
		if (isocgetopt (q, mp, sd))
			return (1);
		break;

	case ISC_SO_SETOPT:
		if (isocsetopt (q, mp, sd))
			return (1);
		break;

	case ISC_SO_SEND:		/* let isocsend check state */
		if (isocsend (q, mp, sd))
			return (1);
		break;

	case ISC_SO_SHUTDOWN:
		if (sd->isoc_state >= ISC_SS_CONNECTED && 
		    sd->isoc_state < ISC_SS_SHUTDOWN) {
			if (isocshutdown (q, mp, sd))
				return (1);
		} else
			return (isocnak (q, mp, ISC_SO_SYSERR, ENOTCONN, 
					 ISC_SO_SHUTDOWN));
		break;

	default:
		LOG1 (SOCERRS, "Got unrecognized or unexpected protocol:%d\n",
			    sprim->SP_type);
		freemsg (mp);
	}
	return (0);
}

/*
 * STATIC int isoclproto(queue_t *q, mblk_t *mp)
 *	isoclproto - read TLI protocol packets from lower stream.
 *
 * Calling/Exit State:
 */
STATIC int
isoclproto(queue_t *q, mblk_t *mp)
{
	struct isocdev *sd;
	/* LINTED pointer alignment */
	union T_primitives *tprim = (union T_primitives *)mp->b_rptr;
	struct T_ok_ack *ok;

	LOG2 (SOCTRC, "isoclproto(%x, %x)\n", q, mp);

	sd = (struct isocdev *) q->q_ptr;

	switch (tprim->type) {
	case T_CONN_CON:
		if (isocconncon (q, mp, sd))
			return (1);
		break;

	case T_CONN_IND:
		if (isocconnind (q, mp, sd))
			return (1);
		break;

	case T_DISCON_IND:
		if (isocdiscind (q, mp, sd))
			return (1);
		break;

	case T_ORDREL_IND:
		if (isocordrelind (q, mp, sd))
			return (1);
		break;

	case T_DATA_IND:
	case T_EXDATA_IND:
		if (isocdataind (q, mp, sd, (tprim->type == T_EXDATA_IND)))
			return (1);
		break;

	case T_ERROR_ACK:
		if (isocterror (q, mp, sd))
			return (1);
		break;

	case T_OK_ACK:
		/* LINTED pointer alignment */
		ok = (struct T_ok_ack *) mp->b_rptr;
		switch (ok->CORRECT_prim) {
		case T_CONN_REQ:	/* connection request received */

			if (sd->isoc_state == ISC_SS_WACK_CREQ) {
				if (sd->isoc_type == ISC_SOCK_STREAM)
					sd->isoc_state = ISC_SS_WCON_CREQ;
				else {
					sd->isoc_state = ISC_SS_CONNECTED;
					sd->isoc_smodcmd = 0;
					freemsg(mp->b_cont);
					mp->b_cont = NULL;
					isocack (q, mp, ISC_SO_CONNECT, 
						 M_PCPROTO);
					 (void) isoctemplate (sd);
				}
			} else
				return (isocnak (q, mp, ISC_SO_OUTSTATE, 0, 
						 ISC_SO_CONNECT));
			break;

		case T_CONN_RES:	/* remote connection responded */
			if (sd->isoc_state == ISC_SS_WACK_CRES) {
				struct isocdev *nsd = sd->isoc_spend;

				(void) isoctemplate (nsd);
				nsd->isoc_state = ISC_SS_CONNECTED;
				nsd->isoc_head = NULL;
				sd->isoc_state = ISC_SS_PASSIVE;
				sd->isoc_spend = NULL;
				isocack (q, mp, ISC_SO_FDINSERT, M_PCPROTO);
			} else
				return (isocnak (q, mp, ISC_SO_OUTSTATE, 0, 
					ISC_SO_FDINSERT));
			/*
			 * need something here to tell the user that
			 * there are pending connection requests.
			 * Cannot just send since the previous ACK has
			 * not been processed by user and you cannot
			 * have two M_PCPROTOs at the stream head at
			 * the same time.
			 */
			if (sd->isoc_npend > 0) {
				struct SO_ok_ack *ack;

				/* important */
				mp = allocb(SO_OK_ACK_SIZE, BPRI_HI);
				if (mp != NULL) {
					mp->b_datap->db_type = M_PROTO;
					/* LINTED pointer alignment */
					ack = (struct SO_ok_ack *)(mp->b_rptr);
					mp->b_wptr = mp->b_rptr + 
						     SO_OK_ACK_SIZE;
					ack->PRIM_type = ISC_SO_OK_ACK;
					ack->CORRECT_prim = ISC_SO_ACCEPT;
					putnext(q, mp); /* put as M_PROTO */
				}
			}
			break;

		case T_DISCON_REQ:
			if (sd->isoc_state == ISC_SS_SHUTDOWN) {
				sd->isoc_state = ISC_SS_PASSIVE;
			} else if (sd->isoc_state == ISC_SS_CONN_RONLY) {
				sd->isoc_state = ISC_SS_PASSIVE;
				putctl1 (q->q_next, M_HANGUP, 0); /* tell the user */
				freemsg (mp);
			} else if /* ACK of DISCON_IND on pending connection */
			(sd->isoc_state == ISC_SS_PASSIVE || 
			    sd->isoc_state == ISC_SS_WAIT_FD) {
				freemsg (mp);
			} else
				return (isocnak (q, mp, ISC_SO_OUTSTATE, 0, ISC_SO_SHUTDOWN));
			break;

		case T_UNBIND_REQ:
			freemsg (mp);
			break;

		default:
			LOG1 (SOCERRS, "isoclproto: Got unexpected ack: %d\n", tprim->type);
			freemsg (mp);
		}
		break;

	case T_UNITDATA_IND:
		if (isocudataind (q, mp, sd))
			return (1);
		break;

	case T_UDERROR_IND:

#ifdef SOCDEBUG
		/* LINTED pointer alignment */
		LOG1(SOCERRS, "isoclproto: Got T_UDERROR_IND: error=%d\n",
			((struct T_uderror_ind *)mp->b_rptr)->ERROR_type );
#endif /* SOCDEBUG */

		freemsg (mp);
		break;

	case T_OPTMGMT_ACK:
		if (isocoptack (q, mp, sd))
			return (1);
		break;

	default:
		LOG1 (SOCERRS, "isoclproto: Got unrecognized protocol: %d\n", tprim->type);
		freemsg (mp);
		break;
	}
	return (0);
}

/*
 * STATIC int isocsocket(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isocsocket - stash the isocket data given to the library
 *	by the user making the isocket library call.
 *
 * Calling/Exit State:
 */
STATIC int
isocsocket(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	/* LINTED pointer alignment */
	struct SO_socket *sprim = (struct SO_socket *) mp->b_rptr;

	int	rval;

	LOG3 (SOCTRACE, "isocsocket(%x, %x, %x)\n", q, mp, sd);

	sd->isoc_domain = sprim->SOCK_domain;
	sd->isoc_type = sprim->SOCK_type;
	sd->isoc_proto = sprim->SOCK_prot;
	if (sd->isoc_domain != ISC_PF_INET || (sd->isoc_type != ISC_SOCK_STREAM && 
	    sd->isoc_type != ISC_SOCK_DGRAM && sd->isoc_type != ISC_SOCK_RAW))
		return (isocnak (q, mp, ISC_SO_SYSERR, EPROTONOSUPPORT, ISC_SO_SOCKET));

	rval = isoc_getudata(q, mp, sd);
	return (rval);
}

/*
 * STATIC int isoc_socketdone(queue_t *q, mblk_t *mp, struct isocdev *sd)
 * 	Called from the read side to indicate that the SI_GETUDATA
 * 	request is completed.
 *
 * Calling/Exit State:
 */
STATIC int
isoc_socketdone(queue_t *q, mblk_t *mp, struct isocdev *sd)
{

	/* Just received the the socket user data */
	sd->isoc_state = ISC_SS_UNBND;
	if (sd->isoc_type == ISC_SOCK_RAW) {		/* bind to isoc_proto */
		if (isocbind (WR(q), NULL, sd, 0)) {
			sd->isoc_state = ISC_SS_OPENED;
			if (isocnak (q, mp, ISC_SO_SYSERR, EINVAL, 
				     ISC_SO_SOCKET))
				return (1);
			return (0);
		}
		freemsg (mp);
	} else
		isocack (q, mp, ISC_SO_SOCKET, M_PCPROTO);
	return (0);
}

/*
 * STATIC int isocbind(queue_t *q, mblk_t *mp, struct isocdev *sd, int number)
 *
 *	isocbind - send a T_bind primitive downstream to bind the stream
 *	to the address supplied by the user.  If user does connect with
 *	no bind, do a bind to any port with no listen capability.  If this
 *	is a raw isocket, bind to the isoc_proto value.  An N_bind structure
 *	is identical to a T_bind, so this will work.
 *
 * Calling/Exit State:
 */
STATIC int
isocbind(queue_t *q, mblk_t *mp, struct isocdev *sd, int number)
{
	struct SO_bind *sprim;
	struct T_bind_req *tprim;
	int	len;
	mblk_t * bmp;
	int	size;
	int	rval;

	LOG4(SOCTRACE, "isocbind(%x, %x, %x, %x)\n", q, mp, sd, number);

	if (!canput (q->q_next)) {		/* can we send a T_BIND? */
		if (mp)
			putbq (q, mp);
		return (1);
	}
	if (mp) {
		/* LINTED pointer alignment */
		sprim = (struct SO_bind *) mp->b_rptr;
		/*
		 * Use new CONN_queue parameter value only if it is present.
		 * This allows programs linked with the old library to 
		 * still work.
	 	 */
		if (sprim->ADDR_offset >= sizeof (struct SO_bind ))
			number = sprim->CONN_queue;
		if (sprim->ADDR_length > sizeof (struct isockaddr ))
			sd->isoc_slen = sizeof (struct isockaddr );
		else
			sd->isoc_slen = sprim->ADDR_length;
		inbcopy (mp->b_rptr + sprim->ADDR_offset, &sd->isoc_saddr, 
			sd->isoc_slen);

		LOG1 (SOCCMDS, "isocbind: %s\n", 
			hex_sprintf(&sd->isoc_saddr, sd->isoc_slen));
	} else {
		if (sd->isoc_type == ISC_SOCK_RAW) {
			/* 
			 *sd->isoc_slen = 1;
			 * This causes trouble for SVR4, because binding 
			 * a raw isocket to protocol type 
			 * is ISC implementation specific. 
			 */
			sd->isoc_slen = 0;
			sd->isoc_saddr.sa_family = sd->isoc_proto;
		} else {
			/* 
			 * This gives the listen problems in sockmod 
			 * because the bind address does not match up
			 *
			 *
			 *sd->isoc_slen = sizeof (struct sockaddr_in );
			 *bzero (&sd->isoc_saddr, sizeof (sd->isoc_saddr));
			 */
			sd->isoc_slen = 0;
			bzero (&sd->isoc_saddr, sizeof (struct sockaddr_in));
		}
	}

	len = sizeof (struct T_bind_req ) + sd->isoc_slen + sd->isoc_roundup;
	size = sizeof (struct T_bind_req );
	if ((bmp = allocb(len, BPRI_MED)) == NULL) {
		putbq (q, mp);
		(void) bufcall (len, BPRI_MED, qenable, (long)q);
		return(1);
	}

	/* LINTED pointer alignment */
	tprim = (struct T_bind_req *)bmp->b_rptr;
	tprim->PRIM_type = T_BIND_REQ;
	tprim->ADDR_length = sd->isoc_slen;
	tprim->ADDR_offset = 0;
	tprim->CONIND_number = number;

	if ((int)tprim->ADDR_length > 0) {
		rval = isoc_aligned_copy((char *)tprim, tprim->ADDR_length, 
				size, (char *)&sd->isoc_saddr, 
				(int *)&tprim->ADDR_offset);
		size = tprim->ADDR_offset + tprim->ADDR_length;

#ifdef SOCDEBUG
		if (rval != 0 ) {
			LOG4(SOCTRACE, "isocbind(%x, %x, %x, rval %d)\n", 
				 q, mp, sd, rval);
		}
#endif /* SOCDEBUG */
		/* 
		inbcopy(&sd->isoc_saddr, bmp->b_rptr + tprim->ADDR_offset, 
		sd->isoc_slen);
		*/
	} 

	bmp->b_wptr = bmp->b_rptr + size;
	ASSERT(size <= len);

	rval = isoc_sendioctl(q, mp, sd, bmp, size, TI_BIND, ISC_SS_WACK_BREQ);
	return (rval);

}

/*
 * STATIC int isocbindack(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isocbindack - verify the address returned by the T_bind primitive
 *	from downstream with the address requested by the SO_bind supplied
 *	by the user.  Either ack or nack to the user.
 *
 * Calling/Exit State:
 */
STATIC int
isocbindack(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct T_bind_ack *tprim;
	struct iocblk *iocp;
	mblk_t *bmp;
	int rval;

	LOG3 (SOCTRACE, "isocbindack(%x, %x, %x)\n", q, mp, sd);

	/* LINTED pointer alignment */
	iocp =  (struct iocblk *)mp->b_rptr;
	if (iocp->ioc_rval) {
		rval = isoc_smoderror(q, mp, sd);
		sd->isoc_smodcmd = 0;
		return(rval);
	}

	sd->isoc_smodcmd = 0;
	bmp = mp->b_cont;
	mp->b_cont = NULL;
	/* LINTED pointer alignment */
	tprim = (struct T_bind_ack *) bmp->b_rptr;

	if (sd->isoc_state != ISC_SS_WACK_BREQ) {
		if (isocnak (q, mp, ISC_SO_OUTSTATE, 0, ISC_SO_BIND))
			return (1);
		sd->isoc_state = ISC_SS_UNBND;
		return (0);
	}


	/* SVR4 does error checking for EADDRINUSE and EADDRNOTAVAIL */

	sd->isoc_pend = tprim->CONIND_number;	/* do we need this? */
	if (tprim->ADDR_length > sizeof (struct isockaddr ))
		sd->isoc_slen = sizeof (struct isockaddr );
	else
		sd->isoc_slen = tprim->ADDR_length;
	inbcopy (bmp->b_rptr + tprim->ADDR_offset, &sd->isoc_saddr, sd->isoc_slen);

	LOG1 (SOCCMDS, "isocbindack: %s\n", hex_sprintf(&sd->isoc_saddr, sd->isoc_slen));

	sd->isoc_state = ISC_SS_BOUND;
	freemsg(bmp);

	/* connect/listen/send waiting */
	if (sd->isoc_flags & SOCF_BINDWAT) {     
		sd->isoc_flags &= ~SOCF_BINDWAT;
		freemsg (mp);
		qenable (WR(q));
	} else if (sd->isoc_type == ISC_SOCK_RAW)	/* raw isocket */
		isocack (q, mp, ISC_SO_SOCKET, M_PCPROTO);
	else	/* an actual bind request */
		isocack (q, mp, ISC_SO_BIND, M_PCPROTO);
	return (0);
}

/*
 * STATIC int isoclisten(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *
 *	isoclisten - stash the backlog count given to the library
 *	by the user making the listen library call and change the
 *	state.  Check for a ISC_SOCK_STREAM.  If a bind hasn't been done
 *	yet, do it and wait for the ack, then do the listen.  If
 *	there is a connection pending, send an ack upstream so the
 *	user can poll for an accept.  The accept library routine must
 *	swallow the ack.
 *
 * Calling/Exit State:
 */
STATIC int
isoclisten(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	/* LINTED pointer alignment */
	struct SO_listen *sprim = (struct SO_listen *) mp->b_rptr;

	mblk_t	*ump;
	struct T_bind_req *tprim;
	int	len;
	int	size;
	int	rval;

	LOG3 (SOCTRACE, "isoclisten(%x, %x, %x)\n", q, mp, sd);

	if (sd->isoc_state != ISC_SS_UNBND && 
	    sd->isoc_state != ISC_SS_WACK_BREQ && 
	    sd->isoc_state != ISC_SS_BOUND)
		return (isocnak (q, mp, ISC_SO_OUTSTATE, 0, ISC_SO_LISTEN));
	if (sd->isoc_type != ISC_SOCK_STREAM)
		return (isocnak (q, mp, ISC_SO_SYSERR, EOPNOTSUPP, 
				 ISC_SO_LISTEN));
	sd->isoc_backlog = sprim->CONN_queue;
	if (sd->isoc_state == ISC_SS_UNBND) {		/* do bind */
		/* 
		 *If we cannot send a TI_BIND or do not have a msgb 
		 * who will do the bind ? 
		 */

		(void) isocbind (q, NULL, sd, sd->isoc_backlog);
		sd->isoc_flags |= SOCF_BINDWAT;
		putbq (q, mp);
		return (1);			/* wait for bind response */
	}
	if (sd->isoc_state == ISC_SS_WACK_BREQ) {/* bind not finished */
		putbq (q, mp);
		return (1);			/* wait for bind response */
	}


	/* 
	 * Here I know that I have bound to an address
	 * Unfortunately, sockmod will be doing an unbind/followed 
	 * by a bind
	 */
	len = sizeof (struct T_bind_req) + sd->isoc_slen + sd->isoc_roundup;
	size = sizeof (struct T_bind_req );
	if ((ump = allocb(len, BPRI_MED)) == NULL) {
		putbq (q, mp);
		(void) bufcall (len, BPRI_MED, qenable, (long)q);
		return(1);
	}

	/* LINTED pointer alignment */
	tprim = (struct T_bind_req *)ump->b_rptr;
	tprim->PRIM_type = T_BIND_REQ;
	tprim->ADDR_length = sd->isoc_slen;
	tprim->ADDR_offset = sizeof(struct T_bind_req);
	tprim->CONIND_number = sd->isoc_backlog;

	size += tprim->ADDR_length;
	ump->b_wptr = ump->b_rptr + size;
	ASSERT(size <= len);

	rval = isoc_sendioctl(q, mp, sd, ump, size, SI_LISTEN, sd->isoc_state);
	return (rval);
}

/*
 * STATIC int isoclistenack(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *
 * Calling/Exit State:
 */
STATIC int
isoclistenack(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct T_bind_ack *tprim;
	struct iocblk *iocp;
	mblk_t *ump;
	int rval;


	LOG3 (SOCTRACE, "isoclistenack(%x, %x, %x)\n", q, mp, sd);

	/* LINTED pointer alignment */
	iocp =  (struct iocblk *)mp->b_rptr;

	if (iocp->ioc_rval) {
		rval = isoc_smoderror(q, mp, sd);
		sd->isoc_smodcmd = 0;
		return(rval);
	}

	sd->isoc_smodcmd = 0;
	ump = mp->b_cont;
	mp->b_cont = NULL;
	/* LINTED pointer alignment */
	tprim = (struct T_bind_ack *) ump->b_rptr;

	if ((tprim->PRIM_type != T_BIND_ACK) ||
	    (sd->isoc_state != ISC_SS_BOUND)) {
		freemsg(ump);
		if (isocnak (q, mp, ISC_SO_OUTSTATE, 0, ISC_SO_LISTEN))
			return (1);
		sd->isoc_state = ISC_SS_UNBND;
		return (0);
	}

	sd->isoc_pend = tprim->CONIND_number;	/* do we need this? */
	if (tprim->ADDR_length > sizeof (struct isockaddr ))
		sd->isoc_slen = sizeof (struct isockaddr );
	else
		sd->isoc_slen = tprim->ADDR_length;
	inbcopy (ump->b_rptr + tprim->ADDR_offset, &sd->isoc_saddr, sd->isoc_slen);

	LOG1 (SOCCMDS, "isoclistenack: %s\n", hex_sprintf(&sd->isoc_saddr, sd->isoc_slen));

	freemsg(ump);
	sd->isoc_options |= ISC_SO_ACCEPTCONN;
	sd->isoc_state = ISC_SS_PASSIVE;
	isocack (q, mp, ISC_SO_LISTEN, M_PCPROTO);

	/* This is done in socconnind too */
	if (sd->isoc_npend) {
		ump = allocb (SO_OK_ACK_SIZE, BPRI_MED);
		if (ump != NULL)
			isocack (q, ump, ISC_SO_ACCEPT, M_PROTO);
	}
	return (0);

}

/*
 * STATIC int isocconn(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *
 *	isocconn - send a T_CONN_REQ primitive downstream to connect the
 *	stream to the address supplied by the user.  If a bind hasn't
 *	been done yet, do it and wait for the ack, then do the connect.
 *
 * Calling/Exit State:
 */

STATIC int
isocconn(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	/* LINTED pointer alignment */
	struct SO_connect *sprim = (struct SO_connect *) mp->b_rptr;
	struct T_conn_req *tprim;
	mblk_t * ump;
	long	len;
	int	size;
	int	rval;

	LOG3 (SOCTRACE, "isocconn(%x, %x, %x)\n", q, mp, sd);

	if (sd->isoc_state == ISC_SS_OPENED)
		return (isocnak (q, mp, ISC_SO_OUTSTATE, 0, ISC_SO_CONNECT));

	if (sd->isoc_type == ISC_SOCK_STREAM) {	/* handle state error returns */
		if (sd->isoc_state > ISC_SS_WACK_CRES)
			return (isocnak (q, mp, ISC_SO_SYSERR, EISCONN, 
				ISC_SO_CONNECT));
		if (sd->isoc_state == ISC_SS_WACK_CREQ)
			return (isocnak (q, mp, ISC_SO_SYSERR, EALREADY, 
				ISC_SO_CONNECT));
		if (!canput (q->q_next)) {	/* can we send a T_CONN_REQ? */
			putbq (q, mp);
			return (1);
		}
	}
	if (sd->isoc_state == ISC_SS_UNBND) {		/* do bind */
		(void) isocbind (q, NULL, sd, 0);
		sd->isoc_flags |= SOCF_BINDWAT;
		putbq (q, mp);
		return (1);			/* wait for bind response */
	}
	if (sd->isoc_state == ISC_SS_WACK_BREQ) {/* bind not finished */
		putbq (q, mp);
		return (1);			/* wait for bind response */
	}

	if (sprim->DEST_length > sizeof (struct isockaddr ))
		sd->isoc_dlen = sizeof (struct isockaddr );
	else
		sd->isoc_dlen = sprim->DEST_length;

	inbcopy (mp->b_rptr + sprim->DEST_offset, &sd->isoc_daddr, 
		 sd->isoc_dlen);

	LOG1 (SOCCMDS, "isocconn: %s\n", hex_sprintf(&sd->isoc_daddr, sd->isoc_dlen));

	if (sd->isoc_type != ISC_SOCK_STREAM) {
		/* Set the right network family. 
		 * The address returned by recvfrom() does not
		 *  contain the network family.
		 */
		struct isockaddr *saddr;

		saddr = &sd->isoc_daddr;
		if (saddr->sa_family == 0)
			saddr->sa_family = AF_INET;

	}

	size = sizeof (struct T_conn_req );
	len = size + sprim->DEST_length + sd->isoc_roundup;

	if (len <= (mp->b_datap->db_lim - mp->b_datap->db_base)) {
		mp->b_rptr = mp->b_datap->db_base;
		ump = mp;
	} else if ((ump = allocb (len, BPRI_MED)) == NULL) {
		putbq (q, mp);
		(void) bufcall(sizeof(struct iocblk), BPRI_MED, qenable, (long)q);
		return (1);
	} else
		freemsg (mp);

	ump->b_datap->db_type = M_PROTO;
	/* LINTED pointer alignment */
	tprim = (struct T_conn_req *)ump->b_rptr;
	tprim->PRIM_type = T_CONN_REQ;
	tprim->DEST_length = sd->isoc_dlen;
	tprim->DEST_offset = 0;
	tprim->OPT_length = 0;
	tprim->OPT_offset = 0;

	if (tprim->DEST_length > 0) {
		isoc_aligned_copy((char *)tprim, sd->isoc_dlen, size,
			  (char *)&sd->isoc_daddr, (int *)&tprim->DEST_offset); 
		size = tprim->DEST_offset + tprim->DEST_length;
		/*
		inbcopy (&sd->isoc_daddr, 
			nmp->b_rptr + tprim->DEST_offset, 
			sd->isoc_dlen);
		*/
	} else {
		/* Null destination address */
		tprim->DEST_offset = sizeof (struct T_conn_req);
	}
	ump->b_wptr = ump->b_rptr + size;

	/* 
	 * If ISC_SOCK_STREAM 
	 *	wait for T_OK_ACK 
	 *	and
	 *	wait for T_CONN_CON
	 * else 
	 *	wait for T_OK_ACK 
	 */
	rval = isoc_sendproto(q, ump, sd, T_CONN_REQ, ISC_SS_WACK_CREQ);
	return (rval);

}

/*
 * STATIC int isocconncon(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isocconncon - verify the address returned by the T_conn_con primitive
 *	from downstream with the address requested by the SO_connect supplied
 *	by the user.  Either ack or nack to the user.
 *
 * Calling/Exit State:
 */
STATIC int
isocconncon(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	/* LINTED pointer alignment */
	struct T_conn_con *tprim = (struct T_conn_con *) mp->b_rptr;

	LOG3 (SOCTRACE, "isocconncon(%x, %x, %x)\n", q, mp, sd);

	if (sd->isoc_state != ISC_SS_WCON_CREQ) {
		if (isocnak (q, mp, ISC_SO_OUTSTATE, 0, ISC_SO_CONNECT))
			return (1);
		sd->isoc_state = ISC_SS_BOUND;
		return (0);
	}

	sd->isoc_smodcmd = 0;
	if (tprim->RES_length > sizeof (struct isockaddr ))
		sd->isoc_dlen = sizeof (struct isockaddr );
	else
		sd->isoc_dlen = tprim->RES_length;
	inbcopy (mp->b_rptr + tprim->RES_offset, &sd->isoc_daddr, 
		 sd->isoc_dlen);

	LOG1 (SOCCMDS, "isocconncon: %s\n", 
			 hex_sprintf(&sd->isoc_daddr, sd->isoc_dlen));

#ifdef SOCDEBUG
	if (tprim->OPT_length > 0) {
		LOG2 (SOCCMDS, "isocconncon: options len = %x, %s\n", 
		 tprim->OPT_length, hex_sprintf(mp->b_rptr + tprim->OPT_offset, 
		 tprim->OPT_length));
	}
#endif /* SOCDEBUG */

	(void) isoctemplate (sd);
	isocack (q, mp, ISC_SO_CONNECT, M_PCPROTO);
	sd->isoc_state = ISC_SS_CONNECTED;
	return (0);
}

/*
 * STATIC int isocconnind(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *
 *	isocconnind - store a connection indication depending on
 *	the current state.  If there is a connection pending, send an
 *	ack upstream so the user can poll for an accept.  The accept
 *	library routine must swallow the ack.
 *
 * Calling/Exit State:
 */
STATIC int
isocconnind(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	LOG3 (SOCTRACE, "isocconnind(%x, %x, %x)\n", q, mp, sd);

	if (sd->isoc_state != ISC_SS_BOUND && 
	    sd->isoc_state != ISC_SS_PASSIVE && 
	    sd->isoc_state != ISC_SS_ACCEPTING && 
	    sd->isoc_state != ISC_SS_WAIT_FD)
		return (isocnak (q, mp, ISC_SO_OUTSTATE, 0, ISC_SO_ACCEPT));

	/*
	 * too many pending connect indications; free it.  The max length of
	 * the queue of pending connections should be the same as TCP, this
	 * conditional matches the test that TCP does in sonewconn().
	 */

	if (sd->isoc_npend >= (ushort)((ushort)(3 * sd->isoc_backlog) / 2)) {
		struct T_discon_req *tprim;
		mp->b_datap->db_type = M_PROTO;
		/* LINTED pointer alignment */
		tprim = (struct T_discon_req *)mp->b_rptr;
		tprim->PRIM_type = T_DISCON_REQ;
		/* reject queued indication */
		/* LINTED pointer alignment */
		tprim->SEQ_number = ((struct T_conn_ind *)mp->b_rptr)->SEQ_number;
		mp->b_wptr = mp->b_rptr + sizeof (struct T_discon_req );
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
		qreply(q, mp);		/* send it back */
		return (0);
	}
	if (sd->isoc_pendlist == NULL)
		sd->isoc_pendlist = mp;
	else {
		mblk_t *omp = sd->isoc_pendlist;

		while (omp->b_next != NULL)
			omp = omp->b_next;
		omp->b_next = mp;
	}
	mp->b_next = NULL;
	mp->b_prev = NULL;
	sd->isoc_npend++;

	LOG2 (SOCCMDS, "isocconnind: added %x to pend list, %d\n", mp, sd->isoc_npend);

	if (sd->isoc_state == ISC_SS_ACCEPTING)
		isocaccack (q);
	else if (sd->isoc_state == ISC_SS_PASSIVE && sd->isoc_npend == 1) {
		mblk_t * nmp = allocb (SO_OK_ACK_SIZE, BPRI_MED);
		if (nmp != NULL)
			isocack (q, nmp, ISC_SO_ACCEPT, M_PROTO);
	}
	return (0);
}

/*
 * STATIC int isocaccept(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *
 *	isocaccept - look for a previous T_CONN_IND on this stream or wait
 *	for a new one.  If found, call isocaccack.
 *	Check for a ISC_SOCK_STREAM.
 *
 * Calling/Exit State:
 */
STATIC int
isocaccept(queue_t *q, mblk_t *mp, struct isocdev *sd)
{

	LOG3 (SOCTRACE, "isocaccept(%x, %x, %x)\n", q, mp, sd);

	if (sd->isoc_type != ISC_SOCK_STREAM)
		return (isocnak (q, mp, ISC_SO_SYSERR, EOPNOTSUPP, 
			ISC_SO_ACCEPT));
	sd->isoc_state = ISC_SS_ACCEPTING;
	freemsg (mp);
	if (sd->isoc_npend > 0)
		isocaccack (RD(q));
	return (0);
}

/*
 * STATIC void isocaccack (queue_t *q)
 *
 *	isocaccack - a T_CONN_IND has arrived and we are in the ISC_SS_ACCEPTING
 *	state, so we acknowledge the accept with the arrived connection
 *	and enter state ISC_SS_WAIT_FD while we wait for a new stream to be
 *	opened for the actual connection.
 *
 * Calling/Exit State:
 */
STATIC void
isocaccack (queue_t *q)
{
	struct SO_accept_ack *ack;
	struct isocdev *sd;
	struct T_conn_ind *tprim;
	mblk_t * mp;
	int size;
	int len;

	sd = (struct isocdev *) q->q_ptr;
	/* LINTED pointer alignment */
	tprim = (struct T_conn_ind *) sd->isoc_pendlist->b_rptr;

	LOG1 (SOCTRACE, "isocaccack(%x)\n", q);

	len = SO_ACCEPT_ACK_SIZE + tprim->SRC_length + sd->isoc_roundup;
	if ((mp = allocb (len, BPRI_HI)) == NULL) {
		if (bufcall(len, BPRI_HI, isocaccack, (long)q) == 0) {
			/*
			 *+ Kernel could not allocate memory for a streams 
			 *+ message.  
			 */
			cmn_err(CE_WARN, "ioscaccack: Losing an accept ack\n" );
		}
		return;
	}
	mp->b_datap->db_type = M_PCPROTO;
	/* LINTED pointer alignment */
	ack = (struct SO_accept_ack *)mp->b_rptr;
	ack->PRIM_type = ISC_SO_ACCEPT_ACK;
	ack->SOCK_domain = sd->isoc_domain;
	ack->SOCK_type = sd->isoc_type;
	ack->SOCK_prot = sd->isoc_proto;
	ack->SEQ_number = tprim->SEQ_number;
	ack->ADDR_length = tprim->SRC_length;
	ack->ADDR_offset = SO_ACCEPT_ACK_SIZE;

	inbcopy ((caddr_t) tprim + tprim->SRC_offset, 
		mp->b_rptr + ack->ADDR_offset, ack->ADDR_length);
	size = ack->ADDR_offset + ack->ADDR_length;


	mp->b_wptr = mp->b_rptr + size;
	putnext (q, mp);

	LOG1 (SOCCMDS, "isocaccack: put ACCEPT_ACK up stream %x\n", q->q_next);

	sd->isoc_state = ISC_SS_WAIT_FD;
}

/*
 * STATIC int isocfdinsert(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isocfdinsert - receive a stream queue pointer from the user.
 *	This stream queue is the stream that is to be connected to
 *	the pending T_conn_ind on this stream with the appropriate
 *	sequence number.
 *
 * Calling/Exit State:
 */
STATIC int
isocfdinsert(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct SO_fdinsert *sprim;
	struct T_conn_ind *tprim;
	struct T_conn_res *tres;
	queue_t * nq;
	mblk_t * mpc, *mdat;
	struct isocdev *nsd;

	/* LINTED pointer alignment */
	sprim = (struct SO_fdinsert *) mp->b_rptr;

	LOG3 (SOCTRACE, "isocfdinsert(%x, %x, %x)\n", q, mp, sd);

	if (!canput (q->q_next)) {	/* can we send a T_CONN_RES? */
		putbq (q, mp);
		return (1);
	}

	/*
	 * find the pending connection and verify the sequence numbers
	 */
	LOG2 (SOCCMDS, "isocfdinsert: looking at %x on pend list, %d\n",
			sd->isoc_pendlist, sd->isoc_npend);

	mpc = sd->isoc_pendlist;
	/* LINTED pointer alignment */
	tprim = (struct T_conn_ind *) mpc->b_rptr;
	if (tprim->SEQ_number != sprim->SEQ_number) {

		LOG2 (SOCERRS, "isocfdinsert: seqno's different: %x %x\n", 
				tprim->SEQ_number,
				sprim->SEQ_number);

		return (isocnak (q, mp, ISC_SO_SYSERR, EPROTO, 
			ISC_SO_FDINSERT));
	}
	sd->isoc_pendlist = mpc->b_next;
	mpc->b_next = NULL;
	sd->isoc_npend--;

	LOG2 (SOCCMDS, "isocfdinsert: took %x off pend list, %d\n", 
			mpc, sd->isoc_npend);


	/*
	 * get the accepting stream queue and fill it in from this queue
	 */
	nq = sprim->STR_ptr;

	/* go upstream to this module */
	while (nq && nq->q_qinfo != RD(q)->q_qinfo)	
		nq = nq->q_next;

	if (nq == NULL) {

		LOG1 (SOCCMDS, "isocfdinsert: nq null: %x\n", sprim->STR_ptr);

		freemsg (mpc);
		return (isocnak (q, mp, ISC_SO_SYSERR, EPROTO, 
			ISC_SO_FDINSERT));
	}
	nsd = (struct isocdev *) nq->q_ptr;
	sd->isoc_spend = nsd;
	nsd->isoc_slen = sd->isoc_slen;
	inbcopy (&sd->isoc_saddr, &nsd->isoc_saddr, nsd->isoc_slen);
	if (tprim->SRC_length > sizeof (struct isockaddr ))
		nsd->isoc_dlen = sizeof (struct isockaddr );
	else
		nsd->isoc_dlen = tprim->SRC_length;
	inbcopy (mpc->b_rptr + tprim->SRC_offset, &nsd->isoc_daddr, 
		 nsd->isoc_dlen);
	nsd->isoc_pend = sd->isoc_pend;
	nsd->isoc_backlog = sd->isoc_backlog;
	nsd->isoc_sequence = sprim->SEQ_number;
	nsd->isoc_flags = sd->isoc_flags;
	nsd->isoc_head = sd;

	/*
	 * get any pending data and send it upstream
	 */
	if (mpc->b_cont != NULL) {	/* data present on T_CONN_IND */
		mdat = unlinkb (mpc);
		(void) isocsendup (nq, mdat);
	}

	if (mpc->b_prev != NULL) {		/* T_DISCON_IND received */

		freemsg (mpc->b_prev);
		freemsg (mpc);
		isocack (q, mp, ISC_SO_FDINSERT, M_PCPROTO);
		nsd->isoc_state = ISC_SS_CONNECTED;
		sd->isoc_state = ISC_SS_PASSIVE;
		sd->isoc_spend = NULL;
		putctl1 (nq->q_next, M_HANGUP, 0); /* tell the user */
		return (0);
	}

	/*
	 * convert the T_CONN_IND to a T_CONN_RES and send it downstream
	 */
	/*
	 * Assumes that CONN_IND message is atleast as big as
	 * CONN_RES
	 */
	mpc->b_datap->db_type = M_PROTO;
	/* LINTED pointer alignment */
	tres = (struct T_conn_res *) mpc->b_rptr;
	tres->PRIM_type = T_CONN_RES;
	tres->QUEUE_ptr = sprim->STR_ptr;
	tres->OPT_length = 0;
	tres->OPT_offset = 0;
	tres->SEQ_number = sprim->SEQ_number;
	mpc->b_wptr = mpc->b_rptr + sizeof (struct T_conn_res );

	isoc_sendproto(q, mpc, sd, T_CONN_RES, ISC_SS_WACK_CRES);

	nsd->isoc_state = ISC_SS_WACK_CRES;
	freemsg (mp);
	return (0);
}

/*
 * STATIC int isocsendup(queue_t *q, mblk_t *mp)
 *	isocsendup - send data upstream.  Send address info depending on
 *	the protocol.
 *
 * Calling/Exit State:
 */
STATIC int
isocsendup(queue_t *q, mblk_t *mp)
{
	struct isocdev *sd;

	LOG2 (SOCTRACE, "isocsendup(%x, %x)\n", q, mp);

	sd = (struct isocdev *) q->q_ptr;
	if (sd->isoc_type == ISC_SOCK_STREAM) {	
		mblk_t * nmp;

		/* strip off any proto headers */
		while (mp->b_datap->db_type != M_DATA) {
			nmp = unlinkb (mp);
			if (nmp == NULL) {
				/* convert mp to empty data message */
				mp->b_datap->db_type = M_DATA;
				mp->b_wptr = mp->b_rptr;
			} else {
				freeb (mp);
				mp = nmp;
			}
		}
	}
	if (!canput (q->q_next)) {
		putbq (q, mp);
		return (1);
	}
	putnext (q, mp);
	return (0);
}

/*
 * STATIC int isocsend(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *
 *	isocsend - send a TLI data message downstream corresponding to the
 *	isocket data message received from the user.
 *
 * Calling/Exit State:
 */
STATIC int
isocsend(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct SO_send *sprim;
	mblk_t * nmp;
/*
	int	len;
*/
	int	size;
	int	rval;

	LOG3 (SOCTRACE, "isocsend(%x, %x, %x)\n", q, mp, sd);

	/* LINTED pointer alignment */
	sprim = (struct SO_send *) mp->b_rptr;

	if (!canput (q->q_next)) {			/* flow control */
		putbq (q, mp);
		return (1);
	}
	if (sd->isoc_type == ISC_SOCK_DGRAM) {
		if (sd->isoc_state == ISC_SS_UNBND) {		/* do bind */
			(void) isocbind (q, NULL, sd, 0);
			sd->isoc_flags |= SOCF_BINDWAT;
			putbq (q, mp);
			return (1);		/* wait for bind response */
		}
		if (sd->isoc_state == ISC_SS_WACK_BREQ) {/* bind not finished */
			putbq (q, mp);
			return (1);		/* wait for bind response */
		}
	}

	if (sd->isoc_template == NULL)
		if (isoctemplate (sd) > 0) {
			putbq (q, mp);
			(void) bufcall (64, BPRI_MED, qenable, (long)q);
			return (1);
		}
	if ((nmp = copyb (sd->isoc_template)) == NULL) {
		putbq (q, mp);
		(void) bufcall (sd->isoc_template->b_wptr - 
				sd->isoc_template->b_rptr, 
				BPRI_MED, qenable, (long)q);
		return (1);
	}

	nmp->b_cont = mp->b_cont;			/* transfer data */
	mp->b_cont = NULL;

	if (sd->isoc_type == ISC_SOCK_STREAM) {
		struct T_data_req *tdr;

		/* T_DATA_REQ */
		if (sd->isoc_state != ISC_SS_CONNECTED && 
		    sd->isoc_state != ISC_SS_CONN_WONLY) {
			freemsg (nmp);
			return (isocnak (q, mp, ISC_SO_OUTSTATE, 0, 
				ISC_SO_SEND));
		}
#ifdef SOCDEBUG
		if (sprim->DEST_length > 0) {
			LOG0 (SOCERRS, "isocsend: ISC_SOCK_STREAM send address ignored\n");
		}
#endif /* SOCDEBUG */
		/* LINTED pointer alignment */
		tdr = (struct T_data_req *) nmp->b_rptr;
		if (sprim->DATA_flags & ISC_MSG_OOB)
			tdr->PRIM_type = T_EXDATA_REQ;

		/* We do not check the size of the data buffer 
		 * against the size of TIDU_size.
		 */

	} else {
		struct T_unitdata_req *tdr;
		unsigned char	*dest;
		int	dlen;

		/* T_UNITDATA_REQ */

		/* 
		 * If I am connected then I will not be using
		 * my passed in address
		 */
		if (sd->isoc_state == ISC_SS_CONNECTED || 
		    sd->isoc_state == ISC_SS_CONN_WONLY) {
			dest = (unsigned char *) & sd->isoc_daddr;
			dlen = sd->isoc_dlen;
		} else {
			dest = mp->b_rptr + sprim->DEST_offset;
			dlen = sprim->DEST_length;
		}

		if (dlen == 0) {

			LOG0 (SOCERRS, "isocsend: datagram with no address specified\n");

			freemsg (nmp);
			return (isocnak (q, mp, ISC_SO_SYSERR, EDESTADDRREQ, 
					 ISC_SO_SEND));
		}
		/*
		 * Check that raw isocket address is the correct length 
		 */
		if ((sd->isoc_type == ISC_SOCK_RAW) &&
		    (dlen != sizeof(struct sockaddr_in )) && 
		    (dlen != IN_MINADDRLEN)) {

			LOG0 (SOCERRS, "isocsend: raw isocket inval addr \n");

			freemsg (nmp);
			return (isocnak (q, mp, ISC_SO_SYSERR, EINVAL, 
					 ISC_SO_SEND));
		}

		/* max pkt size */
		if (msgdsize (nmp->b_cont) > q->q_next->q_maxpsz) {

			LOG0 (SOCERRS, "isocsend: datagram too big: %d\n");

			freemsg (nmp);
			return (isocnak (q, mp, ISC_SO_SYSERR, EMSGSIZE, ISC_SO_SEND));
		}
		size = sizeof (struct T_unitdata_req);
/*
		len = size + dlen + sd->isoc_roundup;
*/
		/* LINTED pointer alignment */
		tdr = (struct T_unitdata_req *) nmp->b_rptr;
		tdr->DEST_length = dlen;

		if ((int)tdr->DEST_length > 0) {
			/* Possible error here -- need to report it */
			rval = isoc_aligned_copy((char *)tdr, tdr->DEST_length,
				size, (char *)dest, (int *)&tdr->DEST_offset);
	
			size = tdr->DEST_offset + tdr->DEST_length;

			/*
			inbcopy (dest, nmp->b_rptr + tdr->DEST_offset, dlen);
			*/
		} 

		nmp->b_wptr = nmp->b_rptr + size;

		/* Look at isoc_options whether the SO_DONTROUTE is on.
		 * If it is set, send data down; otherwise set the 
		 * option. The data will be saved at isoc_opt and it 
		 * is expected to be sent, when the set option request
		 * is acknowledged.
		 */
		if ((sprim->DATA_flags & MSG_DONTROUTE) &&
		    (!(sd->isoc_options & SO_DONTROUTE)) && 
		    (isoc_optionlength (ISC_SOL_SOCKET, ISC_SO_DONTROUTE) != 0)) {

			/* Save the SO_send data */
			sd->isoc_opt = nmp;

			/*
			 * Check if an error took place and if the original 
			 * message was placed back on the queue.
			 */
			rval = isoc_route(q, NULL, sd, 1);
			if (rval) {
				mp->b_cont = nmp->b_cont; /* return the data */
				sd->isoc_opt = NULL;
				nmp->b_cont = NULL;
				freemsg(nmp);
				putbq (q, mp);
				(void)bufcall(rval, BPRI_MED, qenable, (long)q);
				return(1);
			}
			freemsg(mp);
			return(0);
		}
	}
	putnext (q, nmp);	/* send it on */
	isocack (q, mp, ISC_SO_SEND, M_PCPROTO);
	return (0);
}

/*
 * STATIC int 
 * isocdataind(queue_t *q, mblk_t *mp, struct isocdev *sd, int priority)
 *
 *	isocdataind - send up the appropriate primitive when data is received
 *	from a lower protocol.
 *
 * Calling/Exit State:
 */
STATIC int
isocdataind(queue_t *q, mblk_t *mp, struct isocdev *sd, int priority)
{
	struct SO_unitdata_ind *sprim;
	mblk_t * nmp;

	LOG4(SOCTRACE, "isocdataind(%x, %x, %x, %x)\n", q, mp, sd, priority);

	if (sd->isoc_state != ISC_SS_CONNECTED && 
	    sd->isoc_state != ISC_SS_CONN_RONLY) {

		LOG1 (SOCERRS, "isocdataind: data in wrong state: %d\n", sd->isoc_state);

		freemsg (mp);
		return (0);
	}
	if (sd->isoc_type != ISC_SOCK_STREAM) {

		LOG1 (SOCERRS, "isocdataind: T_DATA_IND in wrong type: %d\n", 
				sd->isoc_type);

		freemsg (mp);
		return (0);
	}
	if (!canput (q->q_next)) {
		putbq (q, mp);
		return (1);
	}

	/* build ISC_SO_SEND */
	if (priority && !(sd->isoc_options & ISC_SO_OOBINLINE)) {
		if (SO_UNITDATA_IND_SIZE <= 
		    (mp->b_datap->db_lim - mp->b_datap->db_base)) {
			mp->b_rptr = mp->b_datap->db_base;
			nmp = mp;
		} else if ((nmp = allocb (SO_UNITDATA_IND_SIZE, BPRI_MED)) == 
			    NULL) {
			putbq (q, mp);
			(void)bufcall(SO_UNITDATA_IND_SIZE, BPRI_MED, 
					qenable, (long)q);
			return (1);
		} else
			freemsg (mp);

		nmp->b_datap->db_type = M_PROTO;
		/* LINTED pointer alignment */
		sprim = (struct SO_unitdata_ind *) nmp->b_rptr;
		sprim->PRIM_type = ISC_SO_UNITDATA_IND;
		sprim->SRC_length = 0;
		sprim->SRC_offset = 0;
		sprim->DATA_flags = ISC_MSG_OOB;
		nmp->b_wptr = nmp->b_rptr + SO_UNITDATA_IND_SIZE;
		putnext (q, nmp);

		/* signal user */
		if (sd->isoc_prisig > 0)
			putctl1 (q->q_next, M_PCSIG, sd->isoc_prisig); 
	} else if (mp->b_cont) {
		/* if data, send it up */
		putnext (q, mp->b_cont);
		mp->b_cont = NULL;
		freeb (mp);
		/* signal user */
		if (priority && sd->isoc_prisig > 0)
			putctl1 (q->q_next, M_PCSIG, sd->isoc_prisig); 
	}
	/* JEFF
	 * ISC_BUG
	 * Who is freeing the mp
	 */

	return (0);
}

/*
 * STATIC int isocudata(queue_t *q, mblk_t *mp, int put)
 *	isocudata - read isocket data packets from stream head.
 *
 * Calling/Exit State:
 */
STATIC int
isocudata(queue_t *q, mblk_t *mp, int put)
{
	struct isocdev *sd = (struct isocdev *) q->q_ptr;
	mblk_t *nmp;

	LOG2 (SOCTRACE, "isocudata(%x, %x)\n", q, mp);

	if (sd->isoc_state != ISC_SS_CONNECTED && sd->isoc_state != ISC_SS_CONN_WONLY) {
		/* if (state == ISC_SS_CONN_RONLY) just drop? */

		LOG1 (SOCERRS, "isocudata: data in wrong state: %d\n", sd->isoc_state);

		freemsg (mp);
		return (0);
	}
	if (!canput (q->q_next)) {
		if (put)
			putq (q, mp);
		else
			putbq (q, mp);
		return (1);
	}
	if (sd->isoc_type == ISC_SOCK_STREAM || sd->isoc_type == ISC_SOCK_DGRAM) {
		if (sd->isoc_template == NULL)
			if (isoctemplate (sd) > 0) {
				if (put)
					putq (q, mp);
				else
					putbq (q, mp);
				(void)bufcall(64, BPRI_MED, qenable, (long)q);
				return (1);
			}
		if ((nmp = copyb (sd->isoc_template)) == NULL) {
			if (put)
				putq (q, mp);
			else
				putbq (q, mp);
			(void) bufcall (sd->isoc_template->b_wptr - sd->isoc_template->b_rptr,
			    BPRI_MED, qenable, (long)q);
			return (1);
		}
		nmp->b_cont = mp;
		putnext (q, nmp);
	} else {

		LOG1 (SOCERRS, "isocudata: isocktype %d trying to send data\n", sd->isoc_type);

		freemsg (mp);
	}
	return (0);
}

/*
 * STATIC int isocudataind(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *
 *	isocudataind - send up the unitdata primitive when unit data is received
 *	from a lower protocol.  This routine overlays the SO_unitdata_ind
 *	on the T_unitdata_ind, since they are the same for the first three
 *	fields.  The destination address is not moved.
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
isocudataind(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct T_unitdata_ind *tprim;
	struct SO_unitdata_ind *sprim;

	LOG2(SOCTRACE, "isocudataind(%x, %x, sd)\n", q, mp);

	if (!canput (q->q_next)) {
		putbq (q, mp);
		return (1);
	}

	/* LINTED pointer alignment */
	tprim = (struct T_unitdata_ind *) mp->b_rptr;
	sprim = (struct SO_unitdata_ind *) tprim;

	sprim->PRIM_type = ISC_SO_UNITDATA_IND;
	sprim->DATA_flags = 0;		/* no expedited unitdata */

	/* 
	 * This is assuming that b_cont does not exist 
	 */

	mp->b_wptr = mp->b_rptr + sprim->SRC_offset + sprim->SRC_length;
	putnext (q, mp);
	return (0);
}



/*
 * STATIC int isocldata(queue_t *q, mblk_t *mp, int put)
 *	isocldata - read isocket data packets from lower modules.
 *
 * Calling/Exit State:
 */
STATIC int
isocldata(queue_t *q, mblk_t *mp, int put)
{
	struct isocdev *sd;

	sd = (struct isocdev *) q->q_ptr;

	LOG3 (SOCTRACE, "isocldata(%x, %x, %x)\n", q, mp, put);

	if (sd->isoc_state != ISC_SS_CONNECTED && 
	    sd->isoc_state != ISC_SS_CONN_RONLY) {
		/* if (state == ISC_SS_CONN_WONLY) just drop? */

		LOG1 (SOCERRS, "isocldata: data in wrong state: %d\n", sd->isoc_state);

		freemsg (mp);
		return (0);
	}
	if (!canput (q->q_next)) {
		if (put)
			putq (q, mp);
		else
			putbq (q, mp);
		return (1);
	}
	if (sd->isoc_type == ISC_SOCK_STREAM) {
		putnext (q, mp);
	} else {

		LOG1 (SOCERRS, "isocldata: isocktype %d got M_DATA\n", sd->isoc_type);

		freemsg (mp);
	}
	return (0);
}

/*
 * STATIC int isocdiscreq(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isocdiscreq - receive a disconnect request from the user.
 *	This is currently only valid if in the FDINSERT state.  Disconnect
 *	the pending T_conn_ind on this stream with the appropriate
 *	sequence number.
 *
 * Calling/Exit State:
 */
STATIC int
isocdiscreq(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct T_conn_ind *cprim;
	/* LINTED pointer alignment */
	struct SO_disc_req *sprim = (struct SO_disc_req *) mp->b_rptr;
	struct T_discon_req *dprim;
	mblk_t * mpc, *nmp;

	LOG3 (SOCTRACE, "isocdiscreq(%x, %x, %x)\n", q, mp, sd);

	/*
	 * put the message back on the queue if we can't send a message
	 * down stream or if we can't allocate the mblk needed to send 
	 * the next ACCEPT upstream
	 */
	if (!canput (q->q_next)) {		/* can we send a T_DISCON_REQ? */
		putbq (q, mp);
		return (1);
	}
	if ((nmp = allocb (SO_OK_ACK_SIZE, BPRI_MED)) == NULL) {
		putbq (q, mp);
		return (1);
	}
	/*
	 * find the pending connection and verify the sequence numbers
	 */

	LOG2 (SOCCMDS, "isocdiscreq: looking at %x on pend list, %d\n",
		    sd->isoc_pendlist, sd->isoc_npend);

	mpc = sd->isoc_pendlist;
	/* LINTED pointer alignment */
	cprim = (struct T_conn_ind *) mpc->b_rptr;
	if (cprim->SEQ_number != sprim->SEQ_number) {

		LOG2 (SOCERRS, "isocdiscreq: seqno's different: %x %x\n", 
		   cprim->SEQ_number, sprim->SEQ_number);

		return (isocnak (q, mp, ISC_SO_SYSERR, EPROTO, ISC_SO_DISC_REQ));
	}
	sd->isoc_pendlist = mpc->b_next;
	mpc->b_next = NULL;
	sd->isoc_npend--;

	LOG2 (SOCCMDS, "isocdiscreq: took %x off pend list, %d\n", mpc, sd->isoc_npend);

	/* 
	 * Reject queued indication
	 * Use the connect indication received from TCP for the disconnect
	 * request
	 */
	mpc->b_datap->db_type = M_PROTO;
	/* LINTED pointer alignment */
	dprim = (struct T_discon_req *)mpc->b_rptr;
	dprim->PRIM_type = T_DISCON_REQ;
	dprim->SEQ_number = sprim->SEQ_number;
	mpc->b_wptr = mpc->b_rptr + sizeof (struct T_discon_req );

	/*
	 * Free any data that may have been sent with the original 
	 * connect request
	 */
	if (mpc->b_cont) {
		freemsg(mpc->b_cont);
		mpc->b_cont = NULL;
	}
	putnext (q, mpc);

	/*
	 * Send an OK ack for the disconnect request upstream
	 */
	isocack (q, mp, ISC_SO_DISC_REQ, M_PROTO);

	/*
	 * Adjust the isocket's state and send an ISC_SO_ACCEPT ACK uptream
	 * If there are more connect requests pending on this isocket's
	 * queue, send up a ISC_SO_ACCEPT and reset the state to ISC_SS_FDINSERT,
	 * else set the state to ISC_SS_PASSIVE
	 */
	sd->isoc_state = ISC_SS_PASSIVE;
	if (sd->isoc_npend > 0)
		isocack (q, nmp, ISC_SO_ACCEPT, M_PROTO);
	else
		freemsg (nmp);
	return(0);
}

/*
 * STATIC int isocdiscind(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isocdiscind - determine what to do about a disconnection indication
 *	depending on the current state.  Usually end up in state BOUND.
 *
 * Calling/Exit State:
 */
STATIC int
isocdiscind(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	/* LINTED pointer alignment */
	struct T_discon_ind *tprim = (struct T_discon_ind *) mp->b_rptr;

	LOG3 (SOCTRACE, "isocdiscind(%x, %x, %x)\n", q, mp, sd);

	if (sd->isoc_flags & SOCF_DRAIN) {
		wakeup((caddr_t)&(OTHERQ(q)->q_count));
	}

	switch (sd->isoc_state) {

	case ISC_SS_WCON_CREQ:
		if (isocnak (q, mp, ISC_SO_SYSERR, tprim->DISCON_reason, ISC_SO_CONNECT))
			return (1);
		sd->isoc_state = ISC_SS_BOUND;
		break;

	case ISC_SS_ACCEPTING:
		if (isocnak (q, mp, ISC_SO_SYSERR, tprim->DISCON_reason, ISC_SO_CONNECT))
			return (1);
		sd->isoc_state = ISC_SS_PASSIVE;
		break;

	case ISC_SS_PASSIVE:
	case ISC_SS_WAIT_FD:
		if (sd->isoc_npend) {
			mblk_t * m = sd->isoc_pendlist;
			mblk_t * prev = m;
			struct T_conn_ind *oprim;
			while (m != NULL) {
				/* LINTED pointer alignment */
				oprim = (struct T_conn_ind *) m->b_rptr;
				if (oprim->SEQ_number == tprim->SEQ_number) {

					LOG1 (SOCERRS, 
					    "isocdiscind: drop pending isocket = %d\n", 
					    tprim->SEQ_number);

					if (m == sd->isoc_pendlist)
						sd->isoc_pendlist = m->b_next;
					else
						prev->b_next = m->b_next;
					sd->isoc_state = ISC_SS_PASSIVE;
					sd->isoc_npend--;
					freeb(m);     /* free the connection we just dropped */
					freemsg(mp);  /* free the discon_ind */
					return (0);
				}
				prev = m;
				m = m->b_next;
			}

			LOG0 (SOCERRS, "isocdiscind: sequence numbers don't match\n");

			/* we didn't get a match so delete the first pending connection */
			if (sd->isoc_pendlist != NULL) {
				m = sd->isoc_pendlist;
				sd->isoc_pendlist = sd->isoc_pendlist->b_next;
				freeb (m);    /* free the connection we just dropped */
				freemsg(mp);  /* free the discon_ind */
				sd->isoc_npend--;
				sd->isoc_state = ISC_SS_PASSIVE;
				return (0);
			}
		}
		freemsg (mp);
		break;


	case ISC_SS_WACK_CRES:

		/* never got a response beyond the first SYN
		 * most likely timed out and tcp_drop was called.
		 * clean up the "almost" connected isocket by
		 * removing it from the listening isocdev.
		 * send a ack to the listening isocket on it's
		 * queue so the user level will be 
		 * kept from getting out of
		 * sync, this also splits the isockets apart.
		 * then send a HANGUP
		 * on the new isockets queue to close it.
		 */


		if (sd->isoc_spend) {
			mblk_t * mp;
			struct isocdev *nsd = sd->isoc_spend;

#ifdef CLDEBUG
			/*
			 *+ isocekt debugging message.
			 */
			cmn_err(CE_INFO,  "isocclose: cleanup #59\n");
#endif
			(void) isoctemplate (nsd);
			/* skip ISC_SS_CONNECTED to ISC_SS_BOUND */
			nsd->isoc_state = ISC_SS_BOUND; 
			sd->isoc_state = ISC_SS_PASSIVE;

			if ((mp = allocb (SO_OK_ACK_SIZE, BPRI_HI)) != NULL)
				isocack (q, mp, ISC_SO_FDINSERT, M_PCPROTO);
			sd->isoc_spend = NULL;

			putctl1 (RD(nsd->isoc_qptr)->q_next, M_HANGUP, 0);
			/* Break it to the user */

			if (sd->isoc_npend > 0) {
				struct SO_ok_ack *ack;
				mp = allocb(SO_OK_ACK_SIZE, BPRI_HI); /* important */
				if (mp != NULL) {
					mp->b_datap->db_type = M_PROTO;
					/* LINTED pointer alignment */
					ack = (struct SO_ok_ack *)(mp->b_rptr);
					mp->b_wptr = mp->b_rptr + SO_OK_ACK_SIZE;
					ack->PRIM_type = ISC_SO_OK_ACK;
					ack->CORRECT_prim = ISC_SO_ACCEPT;
					qreply(sd->isoc_qptr, mp); /* put as M_PROTO */
				}
			}
		} else {
			freemsg (mp);
		}
		break;
#ifdef notdef
		if (sd->isoc_head) {
			int	s;
			struct isocdev *osd = sd->isoc_head;

#ifdef CLDEBUG
			/*
			 *+ isocekt debugging message.
			 */
			cmn_err(CE_INFO,  "isocdiscind: cleanup #1\n");
#endif

			sd->isoc_state = ISC_SS_CONNECTED;
			osd->isoc_state = ISC_SS_PASSIVE;

			osd->isoc_spend = NULL;

			isocack (osd->isoc_qptr, mp, ISC_SO_FDINSERT, M_PCPROTO);

			if (osd->isoc_npend > 0) {
				struct SO_ok_ack *ack;
				mp = allocb(SO_OK_ACK_SIZE, BPRI_HI); /* important */
				if (mp != NULL) {
					mp->b_datap->db_type = M_PROTO;
					ack = (struct SO_ok_ack *)(mp->b_rptr);
					mp->b_wptr = mp->b_rptr + SO_OK_ACK_SIZE;
					ack->PRIM_type = ISC_SO_OK_ACK;
					ack->CORRECT_prim = ISC_SO_ACCEPT;
					qreply(osd->isoc_qptr, mp); /* put as M_PROTO */
				}
			}
		} else {
			freemsg (mp);
		}
		break;
#endif

	case ISC_SS_CONN_RONLY:
	case ISC_SS_CONN_WONLY:
	case ISC_SS_CONNECTED:
	case ISC_SS_SHUTDOWN:
		sd->isoc_state = ISC_SS_BOUND;
		if (mp->b_cont != NULL) {
			/* make sure all data goes to user */
			putnext(q, mp->b_cont);
			mp->b_cont = NULL;
		}
		putctl1 (q->q_next, M_HANGUP, 0);		/* tell the user */
		freemsg (mp);
		sd->isoc_state = ISC_SS_BOUND;
		break;

	default:
		if (isocnak (q, mp, ISC_SO_OUTSTATE, 0, ISC_SO_CONNECT))
			return (1);
	}
	return (0);
}

/*
 * STATIC int isocordrelind(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isocordrelind - determine what to do about a orderly release indication
 *	depending on the current state.  Usually end up in state BOUND.
 *
 * Calling/Exit State:
 */
STATIC int
isocordrelind(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct stroptions *opts;
	mblk_t * nmp;

	LOG3 (SOCTRACE, "isocordrel(%x, %x, %x)\n", q, mp, sd);

	if (sd->isoc_flags & SOCF_DRAIN) {
		wakeup((caddr_t)&(OTHERQ(q)->q_count));
	}

	switch (sd->isoc_state) {
	case ISC_SS_CONN_RONLY:
	case ISC_SS_SHUTDOWN:
		sd->isoc_state = ISC_SS_BOUND;
		if (mp->b_cont != NULL) {
			/* make sure all data goes to user */
			putnext(q, mp->b_cont);
			mp->b_cont = NULL;
		}
		putctl1 (q->q_next, M_HANGUP, 0);		/* tell the user */
		freemsg (mp);
		sd->isoc_state = ISC_SS_BOUND;
		break;

	case ISC_SS_CONNECTED:
		sd->isoc_state = ISC_SS_CONN_WONLY;
		/* FALLTHROUGH */
	case ISC_SS_CONN_WONLY:
		if (mp->b_cont != NULL) {
			/* make sure all data goes to user */
			putnext(q, mp->b_cont);
			mp->b_cont = NULL;
		}
		mp->b_datap->db_type = M_DATA;
		mp->b_wptr = mp->b_rptr; /* put a zero length packet upstream */
		putnext(q, mp);
		nmp = allocb(sizeof(struct stroptions ), BPRI_MED);/*important*/
		if (nmp != NULL) {
			nmp->b_datap->db_type = M_SETOPTS;
			/* LINTED pointer alignment */
			opts = (struct stroptions *)nmp->b_rptr;
			opts->so_flags = SO_MREADON;
			nmp->b_wptr = nmp->b_rptr + sizeof(struct stroptions );
			putnext(q, nmp);
		}
		break;

	default:
		if (isocnak (q, mp, ISC_SO_OUTSTATE, 0, ISC_SO_CONNECT))
			return (1);
	}
	return (0);
}

/*
 * STATIC int isocmread(queue_t *q, mblk_t *mp)
 *
 * isocmread - received an M_READ.  Return a zero length M_DATA to the user
 * to indicate EOF and prevent the application from blocking.  This routine 
 * should only be called if an orderly release that put the isocket into the
 * ISC_SS_CONN_WONLY state has been received and the user issues a read.
 *
 * Calling/Exit State:
 */
STATIC void
isocmread(queue_t *q, mblk_t *mp)
{
	struct isocdev *sd = (struct isocdev *) q->q_ptr;

	if (sd->isoc_state == ISC_SS_CONN_WONLY) {
		mp->b_datap->db_type = M_DATA;
		mp->b_wptr = mp->b_rptr;
		qreply(q, mp);
	} else {
		freemsg(mp);
	}
	return;
}

/*
 * STATIC int isocgetpeer(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isocgetpeer - tell the user what the peer isocket name is.
 *
 * Calling/Exit State:
 */
STATIC int
isocgetpeer(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct SO_getpeer_ack *sprim;
	mblk_t * nmp;
	int len;

	LOG3 (SOCTRACE, "isocgetpeer(%x, %x, %x)\n", q, mp, sd);

	len = sizeof (struct SO_getpeer_ack ) + sd->isoc_dlen;
	if (len <= (mp->b_datap->db_lim - mp->b_datap->db_base)) {
		mp->b_rptr = mp->b_datap->db_base;
		nmp = mp;
	} else if ((nmp = allocb (len, BPRI_MED)) == NULL) {
		putbq (q, mp);
		(void) bufcall (len, BPRI_MED, qenable, (long)q);
		return (1);
	} else
		freemsg (mp);
	nmp->b_datap->db_type = M_PCPROTO;
	/* LINTED pointer alignment */
	sprim = (struct SO_getpeer_ack *)nmp->b_rptr;
	sprim->PRIM_type = ISC_SO_GETPEER_ACK;
	sprim->PEER_offset = sizeof (struct SO_getpeer_ack );
	sprim->PEER_length = sd->isoc_dlen;
	inbcopy ((char *) & sd->isoc_daddr, nmp->b_rptr + sprim->PEER_offset,
	    sd->isoc_dlen);
	nmp->b_wptr = nmp->b_rptr + len;
	qreply (q, nmp);
	return (0);
}


/*
 * STATIC int isocgetname(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *
 *	isocgetname - tell the user what the local isocket name is.
 *	We ask the lower level what they think our name is.  
 *	If they respond negatively, we
 *	just send back what we have.
 *
 * Calling/Exit State:
 */
STATIC int
isocgetname(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	mblk_t *ump;
	int	size;
	int	rval;

	LOG3 (SOCTRACE, "isocgetname(%x, %x, %x)\n", q, mp, sd);

	if (!canput (q->q_next)) {
		putbq (q, mp);
		return (1);
	}

	size = sizeof (struct isockaddr);
	if ((ump = allocb (size, BPRI_MED)) == NULL) {
		putbq (q, mp);
		(void) bufcall (size, BPRI_MED, qenable, (long)q);
		return (1);
	}

	rval = isoc_sendioctl (q, mp, sd, ump, size, TI_GETMYNAME, 
			       sd->isoc_state);
	return(rval);
}

/*
 * STATIC int isocretname(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isocretname - really tell the user what the local isocket name is.
 *	Return the isockaddr value in the sd structure to the user.
 *
 * Calling/Exit State:
 */
STATIC int
isocretname(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct SO_getname_ack *sprim;
	mblk_t * nmp;
	int len;

	LOG3 (SOCTRACE, "isocretname(%x, %x, %x)\n", q, mp, sd);

	len = sizeof (struct SO_getname_ack ) + sd->isoc_slen;
	if (len <= (mp->b_datap->db_lim - mp->b_datap->db_base)) {
		mp->b_rptr = mp->b_datap->db_base;
		nmp = mp;
	} else if ((nmp = allocb (len, BPRI_MED)) == NULL) {
		putbq (q, mp);
		(void) bufcall (len, BPRI_MED, qenable, (long)q);
		return (1);
	} else
		freemsg (mp);
	nmp->b_datap->db_type = M_PCPROTO;
	/* LINTED pointer alignment */
	sprim = (struct SO_getname_ack *)nmp->b_rptr;
	sprim->PRIM_type = ISC_SO_GETNAME_ACK;
	sprim->NAME_offset = sizeof (struct SO_getname_ack );
	sprim->NAME_length = sd->isoc_slen;
	inbcopy ((char *) & sd->isoc_saddr, nmp->b_rptr + sprim->NAME_offset,
	    sd->isoc_slen);
	nmp->b_wptr = nmp->b_rptr + len;

	putnext (q, nmp);
	return (0);
}

/*
 * STATIC int isocgetopt(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isocgetopt - return the value of the selected option to the user.
 *
 * Calling/Exit State:
 */
STATIC int
isocgetopt(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct SO_getopt *sprim;
	mblk_t *ump;
	struct opthdr *opt;
	int	optname, level, optval;
	int len, optlen;
	struct T_optmgmt_req *tprim;
	int	rval;


	LOG3 (SOCTRACE, "isocgetopt(%x, %x, %x)\n", q, mp, sd);

	/* LINTED pointer alignment */
	sprim = (struct SO_getopt *) mp->b_rptr;
	level = sprim->PROT_level;
	optname = sprim->OPT_name;
	if ((optlen = isoc_optionlength (level, optname)) == 0)
		return (isocnak (q, mp, ISC_SO_SYSERR, ENOPROTOOPT, 
				ISC_SO_GETOPT));

	sprim->PRIM_type = optlen;		/* stash for isocretopt */

	/*
	 *	look at local options
	 */
	if (level == ISC_SOL_SOCKET) {
		switch (optname) {
		case ISC_SO_OOBINLINE:		/* local option */
			optval = (sd->isoc_options & optname) ? 1 : 0;
			return (isocretopt (q, mp, optval, 0));

		case ISC_SO_ERROR:		/* shouldn't get here */
			optval = 0;
			return (isocretopt (q, mp, optval, 0));

		case ISC_SO_TYPE:
			optval = sd->isoc_type;
			return (isocretopt (q, mp, optval, 0));
		}
	}

	/*
	 *	look at options previously set
	 */
	/* 
	 *  There is no real need to keep the set options at the isocket 
	 *  module level anymore, because unlike ISC's tcp/ip, SVR4's 
	 *  tcp/ip keeps the set options at the tcp/ip level for all 
	 *  types of isocket, not just ISC_SOCK_STREAM.
	 *
	 *  if (sd->isoc_opt) {
	 *  	opt = (struct opthdr *) sd->isoc_opt->b_rptr;
	 *  	len = sd->isoc_opt->b_wptr - sd->isoc_opt->b_rptr;
	 *  	do {
	 *  		if (opt->level == level && opt->name == optname) {
	 *  			if (optlen == 4)
	 *  				optval = *(int *)OPTVAL(opt);
	 *  			else
	 *  			if (optlen == 2)
	 *  				optval = *(short *)OPTVAL(opt);
	 *  			else
	 *  				optval = *(int *)OPTVAL(opt); 
	 *  			return (isocretopt (q, mp, optval, 0));
	 *      		}
	 *  		len -= (opt->len + 3*sizeof(long));
	 *  		opt = (struct opthdr *) ((caddr_t) opt + 
	 *  			opt->len + 3*sizeof(long));
	 *  	} while (len > 0);
	 *  }
	 */

	/*
	 *	not found; ask lower level through a T_OPTMGMT_REQ. 
	 */
	if (!canput (q->q_next)) {
		putbq (q, mp);
		return (1);
	}
	len = sizeof (struct T_optmgmt_req ) + sizeof(struct opthdr ) + 
	    OPTLEN(optlen);
	if ((ump = allocb (len, BPRI_MED)) == NULL) {
		putbq (q, mp);
		(void) bufcall (len, BPRI_MED, qenable, (long)q);
		return (1);
	}


	/* 
	 * Sending down as an M_IOCTL 
	 * ump->b_datap->db_type = M_PROTO;
	 */
	/* LINTED pointer alignment */
	tprim = (struct T_optmgmt_req *) ump->b_rptr;
	tprim->PRIM_type = T_OPTMGMT_REQ;
	tprim->OPT_length = sizeof(struct opthdr ) + OPTLEN(optlen);
	tprim->OPT_offset = sizeof (struct T_optmgmt_req );
	tprim->MGMT_flags = T_CHECK;
	/* LINTED pointer alignment */
	opt = (struct opthdr *) (ump->b_rptr + tprim->OPT_offset);
	opt->len = OPTLEN(optlen);
	opt->level = level;
	opt->name = optname;
	ump->b_wptr = ump->b_rptr + len;

	rval = isoc_sendioctl(q, mp, sd, ump, len, TI_OPTMGMT, sd->isoc_state);

	return (rval);
}

/*
 * STATIC int isocretopt(queue_t *q, mblk_t *mp, int optval, int type)
 *	isocretopt - really return the value of the selected option to the user.
 *
 * Calling/Exit State:
 */
STATIC int
isocretopt(queue_t *q, mblk_t *mp, int optval, int type)
{
	mblk_t * nmp, *mcont;
	struct SO_getopt_ack *sack;
	int	optname, level;
	int len, optlen;

	LOG4(SOCTRACE, "isocretopt(%x, %x, %x, %x)\n", q, mp, optval, type);

	if (type == 1) {		/* results are in a struct opthdr */
		struct opthdr *opt;

		ASSERT(mp->b_cont != NULL);
		mcont = mp->b_cont;
		mp->b_cont = NULL;
		/* LINTED pointer alignment */
		opt = (struct opthdr *) mcont->b_rptr;
		level = opt->level;
		optname = opt->name;
		optlen = opt->len;
		if (optlen == 4)
			optval = *(long *)OPTVAL(opt);
		else if (optlen == 2)
			optval = (int)(*(short *)OPTVAL(opt));
		else
			optval = *((int *)OPTVAL(opt));
	} else {
		/* results are in a struct SO_getopt */

		struct SO_getopt *sprim;

		/* LINTED pointer alignment */
		sprim = (struct SO_getopt *) mp->b_rptr;
		level = sprim->PROT_level;
		optname = sprim->OPT_name;
		optlen = sprim->PRIM_type;
	}
	if (optlen < 4)
		optlen = 4;

	len = sizeof (struct SO_getopt_ack ) + optlen;
	if (len <= (mp->b_datap->db_lim - mp->b_datap->db_base)) {
		mp->b_rptr = mp->b_datap->db_base;
		nmp = mp;
	} else if ((nmp = allocb (len, BPRI_MED)) == NULL) {
		if (type == 1)
			mp->b_cont = mcont;
		else
			/* LINTED pointer alignment */
			*(int *) mp->b_rptr = ISC_SO_GETOPT;
		putbq (q, mp);
		(void) bufcall (len, BPRI_MED, qenable, (long)q);
		return (1);
	} else
		freemsg (mp);

	nmp->b_datap->db_type = M_PCPROTO;
	/* LINTED pointer alignment */
	sack = (struct SO_getopt_ack *)nmp->b_rptr;
	sack->PRIM_type = ISC_SO_GETOPT_ACK;
	sack->PROT_level = level;
	sack->OPT_name = optname;
	sack->OPT_length = optlen;
	sack->OPT_offset = sizeof (struct SO_getopt_ack );
	if (optlen > 4)
		inbcopy ((caddr_t)optval, nmp->b_rptr + sack->OPT_offset, 
			 optlen);
	else
		/* LINTED pointer alignment */
		*(int *)(nmp->b_rptr + sack->OPT_offset) = optval;
	nmp->b_wptr = nmp->b_rptr + len;
	if (type == 1)
		freemsg (mcont);

	LOG3 (SOCCMDS, "isocgetopt: level %x option %x: %s\n", level, optname,
		    hex_sprintf (&optval, optlen));

	/* 
	 * This will always be a write queue ???
	 */

	if (q->q_flag & QREADR)		/* read queue; send on up */
		putnext (q, nmp);
	else	/* write queue; reply */
		qreply (q, nmp);
	return (0);
}

/*
 * STATIC int isocsetopt(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isocsetopt - set the value of the option selected by the user.
 *
 * Calling/Exit State:
 */
STATIC int
isocsetopt(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct SO_setopt *sprim;
	struct opthdr *opt;
	mblk_t *ump;
	int	optname, level;
	int len, optlen;
	struct T_optmgmt_req *tprim;
	int	rval;


	LOG3 (SOCTRACE, "isocsetopt(%x, %x, %x)\n", q, mp, sd);

	/* LINTED pointer alignment */
	sprim = (struct SO_setopt *) mp->b_rptr;
	level = sprim->PROT_level;
	optname = sprim->OPT_name;
	if ((optlen = isoc_optionlength (level, optname)) == 0)
		return (isocnak (q, mp, ISC_SO_SYSERR, ENOPROTOOPT, 
			ISC_SO_SETOPT));

	if (optlen != sprim->OPT_length)
		return (isocnak (q, mp, ISC_SO_SYSERR, EINVAL, ISC_SO_SETOPT));

	LOG4(SOCCMDS, "isocsetopt: level %x option %x length %d: %s\n", 
		level, optname, optlen, 
		hex_sprintf (mp->b_rptr + sprim->OPT_offset, optlen));

	if (level == ISC_SOL_SOCKET) {
		if (optname < 0x1000) {
			int optval;

			/* LINTED pointer alignment */
			optval = *(int *) mp->b_rptr + sprim->OPT_offset;
			if (optval)
				sd->isoc_options |= optname;
			else
				sd->isoc_options &= ~optname;
		}
		switch (optname) {
		case ISC_SO_OOBINLINE:		/* local option */
			isocack (q, mp, ISC_SO_SETOPT, M_PCPROTO);
			return (0);

		case ISC_SO_ERROR:		/* only valid on getopt */
		case ISC_SO_TYPE:
			return (isocnak (q, mp, ISC_SO_SYSERR, ENOPROTOOPT, 
					 ISC_SO_SETOPT));
		}
	}

	/*
	 * negotiate with lower level through an ioctl (not limited by TLI
	 * to only the bind state)
	 */
	if (!canput (q->q_next)) {
		putbq (q, mp);
		return (1);
	}

	len = sizeof (struct T_optmgmt_req ) + sizeof(struct opthdr ) + 
	    OPTLEN(optlen);
	if ((ump = allocb (len, BPRI_MED)) == NULL) {
		putbq (q, mp);
		(void) bufcall (len, BPRI_MED, qenable, (long)q);
		return (1);
	}

	/* 
	 * Send as an M_IOCTL 
	 * ump->b_datap->db_type = M_PROTO;
	 */

	/* LINTED pointer alignment */
	tprim = (struct T_optmgmt_req *) ump->b_rptr;
	tprim->PRIM_type = T_OPTMGMT_REQ;
	tprim->OPT_length = sizeof(struct opthdr ) + OPTLEN(optlen);
	tprim->OPT_offset = sizeof (struct T_optmgmt_req );
	tprim->MGMT_flags = T_NEGOTIATE;
	/* LINTED pointer alignment */
	opt = (struct opthdr *) (ump->b_rptr + tprim->OPT_offset);
	opt->len = OPTLEN(optlen);
	opt->level = level;
	opt->name = optname;
	inbcopy (mp->b_rptr + sprim->OPT_offset, OPTVAL(opt), OPTLEN(optlen));
	ump->b_wptr = ump->b_rptr + len;

	rval = isoc_sendioctl(q, mp, sd, ump, len, TI_OPTMGMT, sd->isoc_state);

	return (rval);
}

/*
 * STATIC int isocoptack(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isocoptack - gets a TLI option management package from below and
 *	generates the appropriate SO_getopt_ack/SO_ok_ack for the user.
 *
 * Calling/Exit State:
 */
STATIC int
isocoptack(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct T_optmgmt_ack *tprim;
	struct opthdr *opt;
	struct SO_getopt_ack *sprim;
	int	optlen;
	
	struct iocblk *iocp;
	mblk_t	*ump;
	mblk_t	*nmp;
	int	rval;
	int	len;

	LOG3 (SOCTRACE, "isocoptack(%x, %x, %x)\n", q, mp, sd);

	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	ump = mp->b_cont;
	if (iocp->ioc_rval) {
		rval = isoc_smoderror(q, mp, sd);
		sd->isoc_smodcmd = 0;
		return(rval);
	}
		
	/* LINTED pointer alignment */
	tprim = (struct T_optmgmt_ack *) ump->b_rptr;
	/* LINTED pointer alignment */
	opt = (struct opthdr *) (ump->b_rptr + tprim->OPT_offset);

	LOG3 (SOCTRACE, "isocoptack(%x, %x, %x)\n", q, mp, sd);

#ifdef SOCDEBUG
	if (tprim->OPT_length != opt->len) {
		LOG2 (SOCERRS, "isocoptack: different lengths: %x, %x\n",
			tprim->OPT_length, opt->len);
	}
#endif /* SOCDEBUG */

	if (!canput (q->q_next)) {
		putbq (q, mp);
		return (1);
	}

	sd->isoc_smodcmd = 0;

	if (tprim->PRIM_type != T_OPTMGMT_ACK) {
		if (isocnak (q, mp, ISC_SO_OUTSTATE, 0, ISC_SO_GETOPT))
			return (1);
		
		return (0);
	}

	if ((optlen = isoc_optionlength (opt->level, opt->name)) == 0) {

		LOG2 (SOCERRS, "isocoptack: invalid option: %x, %x\n", 
				opt->level, opt->name);

		freemsg (mp);
		return (0);
	}
	LOG4(SOCCMDS, "isocoptack: level %x option %x length %d: %s\n", 
			opt->level, opt->name, optlen, 
			hex_sprintf (OPTVAL(opt), optlen));

	/* Lower level Options are no longer saved at the isocket module level.
	 * isocaddopts (mp, sd, 0);
	 */
	if (tprim->MGMT_flags & T_NEGOTIATE) {

		/* it was a setopt */

		/* If isoc_opt is holding a data or isoc_flag is 
		 * SOCF_DONTROUTE, it means set option is initiated locally. 
		 * Therefore, send the pending data out followed by a command 
		 * to clear the option or just drop the it 
		 */
		if ((opt->name == SO_DONTROUTE) && (sd->isoc_opt)) {

			/* send the pending data on the Write Q */

			/* 
			 * Can't send the data on the read q 
			 * putnext (q, sd->isoc_opt);
			 */
			 
			nmp = sd->isoc_opt;
			sd->isoc_opt = NULL;
			putnext (WR(q), nmp);

			if (!(sd->isoc_options & SO_DONTROUTE) &&
			     (isoc_optionlength (SOL_SOCKET, 
						 SO_DONTROUTE) != 0)) {
				sd->isoc_flags |= SOCF_DONTROUTE;
				rval = isoc_route(q, NULL, sd, 0);
				if (rval)
					sd->isoc_flags &= ~SOCF_DONTROUTE;
			}
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
			isocack (q, mp, ISC_SO_SEND, M_PCPROTO);
			return(0);
		} else if ((opt->name == SO_DONTROUTE) && 
			 (sd->isoc_flags & SOCF_DONTROUTE)) {
			sd->isoc_flags &= ~SOCF_DONTROUTE;
			freemsg(mp);
			return(0);
		}
		freemsg(mp->b_cont);
		mp->b_cont = NULL;
		isocack (q, mp, ISC_SO_SETOPT, M_PCPROTO);
		return (0);
	}

	mp->b_cont = NULL;
	freemsg(mp);

	if (opt->len > 0 && opt->len < optlen)
		optlen = opt->len;

	len = SO_GETOPT_ACK_SIZE + optlen;

	/* 
	 * This code assumes the mblock is long enough
	 */
	if (len > (ump->b_datap->db_lim - ump->b_datap->db_base))
		return (isocnak (q, ump, ISC_SO_SYSERR, EINVAL, ISC_SO_GETOPT));

	ump->b_datap->db_type = M_PCPROTO;
	ump->b_rptr = ump->b_datap->db_base;
	/* LINTED pointer alignment */
	sprim = (struct SO_getopt_ack *) ump->b_rptr;	/* mblock long enough */
	sprim->PRIM_type = ISC_SO_GETOPT_ACK;
	sprim->PROT_level = opt->level;
	sprim->OPT_name = opt->name;
	sprim->OPT_length = optlen;
	sprim->OPT_offset = SO_GETOPT_ACK_SIZE;
	inbcopy (OPTVAL(opt), ump->b_rptr + sprim->OPT_offset, 
		 sprim->OPT_length);
	ump->b_wptr = ump->b_rptr + SO_GETOPT_ACK_SIZE + sprim->OPT_length;
	putnext (q, ump);
	return (0);
}

/*
 * STATIC int isoc_shutdown(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *
 *	isoc_shutdown - shut down the receive, send, or both sides of the 
 * 		isocket  using an ioctl.  This replaces isocshutdown which 
 *		has been left in the module for backwards compatibility with
 *		programs using an old version of libinet.
 *
 * Calling/Exit State:
 */
STATIC int
isoc_shutdown(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	int	type;  /* type of shutdown request */
	struct iocblk *iocp;

	mblk_t	*ump;
	int	len;

	LOG3 (SOCTRACE, "isoc_shutdown(%x, %x, %x)\n", q, mp, sd);

	/*
	 * If we are not in either ISC_SS_CONNECTED, ISC_SS_CONN_WONLY or 
	 * ISC_SS_CONN_RONLY state, return 
	 */

	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	if (sd->isoc_state < ISC_SS_CONNECTED || 
	    sd->isoc_state >= ISC_SS_SHUTDOWN) {
		iocp->ioc_error = ENOTCONN;
		return(1);
	}

	/*
	 * check the length of the data field
	 */
	ump = mp->b_cont;
	if (ump == NULL) {
		iocp->ioc_error = EINVAL;
		return(1);
	}
	if ((ump->b_wptr - ump->b_rptr) != sizeof(int)) {
		iocp->ioc_error = EINVAL;
		return(1);
	}

	if (!canput (q->q_next)) {
		putbq (q, mp);
		return (0);
	}

	type = (int)*ump->b_rptr;
	len = sizeof (type);

	/*
	 * take the needed action depending on the type of shutdown request
	 */
	switch (type) {
	case 0:
	case 1:
		break;
	case 2:
		/* disallow both reads and writes */
		if (sd->isoc_state == ISC_SS_CONN_RONLY) {
			/* send up an ioctl ack */
			mp->b_datap->db_type = M_IOCACK;
			qreply (q, mp);
			return(0);
		}
		break;

	default:
		/* bad value specified */
		iocp->ioc_error = EINVAL;
		return(1);
	}

	/* Send down the request */
	(void)isoc_sendioctl(q, mp, sd, ump, len, SI_SHUTDOWN, sd->isoc_state);
	return(0);
}

/*
 * STATIC int isoc_shdownack(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *
 * Calling/Exit State:
 */
STATIC int
isoc_shdownack(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct iocblk *iocp;
	mblk_t	*ump;
	int	rval;
	int	type;
	int	error;

	LOG3 (SOCTRACE, "isoc_shdownack(%x, %x, %x)\n", q, mp, sd);

	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	ump = mp->b_cont;
	if (iocp->ioc_rval) {
		rval = isoc_smoderror(q, mp, sd);
		sd->isoc_smodcmd = 0;
		return(rval);
	}

	if (ump == NULL) {
		if (isocnak (q, mp, ISC_SO_SYSERR, EINVAL, sd->isoc_cmd))
			return (1);
		return(0);
	}

	if (!canput (q->q_next)) {
		putbq (q, mp);
		return (1);
	}

	sd->isoc_smodcmd = 0;
	type = (int)*ump->b_rptr;

	/*
	 * Setup the internal data-structures depending on the type of 
	 * shutdown request
	 */
	error = 0;
	switch (type) {
	case 0: 
		/* disallow reads */
		if (sd->isoc_state == ISC_SS_CONN_RONLY)
			sd->isoc_state = ISC_SS_SHUTDOWN;
		else
			sd->isoc_state = ISC_SS_CONN_WONLY;
		noenable (RD(q));	/* disable our read queue */
		mp->b_datap->db_type = M_IOCACK;
		break;

	case 1: 
		/* disallow writes */
		if (sd->isoc_state == ISC_SS_CONN_WONLY)
			sd->isoc_state = ISC_SS_SHUTDOWN;
		else
			sd->isoc_state = ISC_SS_CONN_RONLY;

		/* 
		 * return an ioctl ack
		 */
		mp->b_datap->db_type = M_IOCACK;
		break;

	case 2:	
		/* disallow both reads and writes */
		sd->isoc_state = ISC_SS_SHUTDOWN;

		/* send up an ioctl ack */
		mp->b_datap->db_type = M_IOCACK;
		break;

	default: 
		 /* bad value specified */
		error = EINVAL;
		mp->b_datap->db_type = M_IOCNAK;
	}


	switch(sd->isoc_cmd) {
	case ISC_SIOCSHUTDOWN:
		/* Request originally came in as an ioctl */
		if (error) {
			iocp->ioc_rval = error;
			mp->b_datap->db_type = M_IOCNAK;
		} else 
			mp->b_datap->db_type = M_IOCACK;
		
		putnext(q, mp);
		break;

	case ISC_SO_SHUTDOWN:
		/* Request originally came in as an M_PROTO */
		if (error)
			isocnak(q, mp, ISC_SO_SYSERR, error, ISC_SO_SHUTDOWN);
		else
			isocack(q, mp, ISC_SO_SHUTDOWN, M_PCPROTO);
		break;
	default:
		isocnak(q, mp, ISC_SO_OUTSTATE, 0, ISC_SO_SHUTDOWN);
	}

	return (0);
}

/*
 * STATIC int isocshutdown(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isocshutdown - shut down the receive, send, or both sides of the 
 * 		isocket. Left in for backwards compatibility with older 
 *		versions of libinet.
 *
 * Calling/Exit State:
 */
STATIC int
isocshutdown(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct SO_shutdown *sprim;
	int	type;
	int	*howptr;
	mblk_t	*ump;
	int	len;

	LOG3 (SOCTRACE, "isocshutdown(%x, %x, %x)\n", q, mp, sd);

	/* LINTED pointer alignment */
	sprim = (struct SO_shutdown *) mp->b_rptr;
	type = sprim->SHUT_down;

	if (!canput (q->q_next)) {
		putbq (q, mp);
		return (1);
	}

	/*
	 * take the needed action depending on the type of shutdown request
	 */
	switch (type) {
	case 0:
	case 1:
		break;
	case 2:
	default:
		/* disallow both reads and writes */
		if (sd->isoc_state == ISC_SS_CONN_RONLY) {
			/* send up an ack */
			isocack (q, mp, ISC_SO_SHUTDOWN, M_PCPROTO);
			return(0);
		}
		type = 2;
		break;
	}

	len = sizeof (type);
	if ((ump = allocb (len, BPRI_MED)) == NULL) {
		putbq (q, mp);
		(void) bufcall (len, BPRI_MED, qenable, (long)q);
		return (1);
	}

	/* LINTED pointer alignment */
	howptr = (int *)ump->b_rptr;
	*howptr = type;
	ump->b_wptr = ump->b_rptr + len;

	/* Send down the request */
	(void)isoc_sendioctl(q, mp, sd, ump, len, SI_SHUTDOWN, sd->isoc_state);
	return(0);

}

/*
 * STATIC int isocatmark(queue_t *q)
 *	isocatmark - return whether (1) or not (0) the user's current
 *	read pointer (first message on stream head) is "at the mark"
 *	(out of band data).
 *
 * Calling/Exit State:
 */
STATIC int
isocatmark(queue_t *q)
{
	queue_t *qh = RD(q);
	struct SO_unitdata_ind *sprim;

	LOG1 (SOCTRACE, "isocatmark(%x)\n", q);

	while (qh->q_next != NULL)			/* find stream head */
		qh = qh->q_next;
	if (qh->q_first == NULL)			/* nothing on queue */
		return (0);
	if (qh->q_first->b_datap->db_type != M_PROTO) /* not a unitdata */
		return (0);
	/* LINTED pointer alignment */
	sprim = (struct SO_unitdata_ind *) qh->q_first->b_rptr;
	if (sprim->PRIM_type != ISC_SO_UNITDATA_IND)
		return (0);
	if (sprim->DATA_flags & MSG_OOB)
		return (1);
	return (0);
}

/*
 * STATIC int isoctemplate(struct isocdev *sd)
 *	isoctemplate - make a template of the TLI proto message block
 *	to accompany any data sent downstream.  Leave room for a
 *	SO_DONTROUTE option in the datagram template.  If a template
 *	already is allocated when this routine is called, update it
 *	with new information.
 *
 * Calling/Exit State:
 */
STATIC int
isoctemplate(struct isocdev *sd)
{
	mblk_t *nmp;

	LOG1 (SOCTRACE, "isoctemplate(%x)\n", sd);

	if (sd->isoc_type == ISC_SOCK_STREAM) {
		struct T_data_req *tdr;

		/* make a T_DATA_REQ */
		if (sd->isoc_template == NULL) {
			nmp = allocb(sizeof (struct T_data_req ),BPRI_MED);
			if (nmp == NULL) {
				(void)bufcall(sizeof (struct T_data_req ), 
					BPRI_MED, (void (*)())isoctemplate, (long)sd);
				return (1);
			}
		} else
			nmp = sd->isoc_template;
		nmp->b_datap->db_type = M_PROTO;
		/* LINTED pointer alignment */
		tdr = (struct T_data_req *) nmp->b_rptr;
		tdr->PRIM_type = T_DATA_REQ;
		tdr->MORE_flag = 0;
		nmp->b_wptr = nmp->b_rptr + sizeof (struct T_data_req );
	} else {
		struct T_unitdata_req *tdr;
		int	len;
		int	size;

		/* make a T_UNITDATA_REQ */
		if (sd->isoc_type == ISC_SOCK_RAW)
			len = sizeof (struct T_unitdata_req ) + 
			      sizeof(struct opthdr ) + 8;
		else
			len = sizeof (struct T_unitdata_req ) + 
			      sizeof(struct opthdr ) + 4;
		if (sd->isoc_dlen > 0)
			len += sd->isoc_dlen;
		else
			len += sd->isoc_slen;

		len += sd->isoc_roundup;
		if (sd->isoc_template == NULL) {
			if ((nmp = allocb (len, BPRI_MED)) == NULL) {
				(void) bufcall (len, BPRI_MED, 
					(void (*)())isoctemplate, (long)sd);
				return (1);
			}
		} else
			nmp = sd->isoc_template;
		size = sizeof (struct T_unitdata_req );
		nmp->b_datap->db_type = M_PROTO;
		/* LINTED pointer alignment */
		tdr = (struct T_unitdata_req *) nmp->b_rptr;
		tdr->PRIM_type = T_UNITDATA_REQ;
		tdr->DEST_length = sd->isoc_dlen;
		tdr->DEST_offset = sizeof (struct T_unitdata_req );
		if (sd->isoc_dlen > 0) {
			(void)isoc_aligned_copy((char *)tdr, 
					sd->isoc_dlen,
					sizeof (struct T_unitdata_req ),
					(char *)&sd->isoc_daddr, 
					(int *)&tdr->DEST_offset);
			size += tdr->DEST_offset + tdr->DEST_length;
			/*
			inbcopy (&sd->isoc_daddr, 
				nmp->b_rptr + tdr->DEST_offset,
			    sd->isoc_dlen);
			*/
		}
		tdr->OPT_length = 0;
		tdr->OPT_offset = size;
		nmp->b_wptr = nmp->b_rptr + len - sizeof(struct opthdr ) - 4;
	}
	sd->isoc_template = nmp;
	return (0);
}

/*
 * STATIC int isoc_optionlength(int level, int optname)
 *	isoc_optionlength - return 1 if option is unknown; 0 otherwise.
 *
 * Calling/Exit State:
 */
STATIC int	
isoc_optionlength(int level, int optname)
{
	int	len = 0;

	switch (level) {
	case SOL_SOCKET:
		switch (optname) {
		case SO_DEBUG:
		case ISC_SO_ACCEPTCONN:
		case SO_REUSEADDR:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_BROADCAST:
		case SO_USELOOPBACK:
		case SO_OOBINLINE:
		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
		case SO_ERROR:
		case SO_TYPE:
			len = 4;
			break;

		case SO_LINGER:
			len = sizeof (struct linger );
			break;

		default:
			LOG1 (SOCERRS, "iosc_optionlength: Unrecognized isocket option: %x\n",
				    optname);
		}
		break;

	case IPPROTO_TCP:
		switch (optname) {
		case TCP_NODELAY:
		case TCP_MAXSEG:
		/* Not supported by SVR4 -
 		 *
	         *case TCP_OOBMODE:
 		 */
			len = 4;
			break;

		default:
			LOG1 (SOCERRS, "isoc_optionlength: Unrecognized tcp option: %x\n",
				    optname);
		}
		break;

	case IPPROTO_IP:
		switch (optname) {
		case IP_OPTIONS:
			len = 4;
			break;

		default:
			LOG1 (SOCERRS, "isoc_optionlength: Unrecognized ip option: %x\n",
				    optname);
		}
		break;

	default:
		LOG2 (SOCERRS, "isoc_optionlength: Unrecognized option level: %x %x\n",
			    level, optname);
	}
	return (len);
}

/*
 * STATIC int isoc_route(queue_t *q, mblk_t *mp, struct isocdev *sd, int val)
 *
 * Calling/Exit State:
 */
STATIC int
isoc_route(queue_t *q, mblk_t *mp, struct isocdev *sd, int val)
{ 
	int	len, optlen;
	mblk_t * nmp;
	struct T_optmgmt_req *prim;
	struct opthdr *topt;
	
	/*
	 * We are here because we had to turning routing on/off
	 * depending on the value of val.
	 */

	LOG4(SOCTRACE, "isoc_doroute(%x, %x, %x, %x)\n", q, mp, sd, val);

	nmp = NULL;
	optlen = isoc_optionlength (SOL_SOCKET, SO_DONTROUTE);
	ASSERT(optlen != 0);
	len = sizeof (struct T_optmgmt_req ) + sizeof(struct opthdr ) +
		      OPTLEN(optlen);
	if ((nmp = allocb (len, BPRI_MED)) == NULL)
		return(len);

	/*
	 * Send as an IOCTL
	 * nmp->b_datap->db_type = M_PROTO;
	 */
	/* LINTED pointer alignment */
	prim = (struct T_optmgmt_req *) nmp->b_rptr;
	prim->PRIM_type = T_OPTMGMT_REQ;
	prim->OPT_length = sizeof(struct opthdr ) + OPTLEN(optlen);
	prim->OPT_offset = sizeof (struct T_optmgmt_req );
	prim->MGMT_flags = T_NEGOTIATE;
	/* LINTED pointer alignment */
	topt = (struct opthdr *) (nmp->b_rptr + prim->OPT_offset);
	topt->len = OPTLEN(optlen);
	topt->level = SOL_SOCKET;
	topt->name = SO_DONTROUTE;
	*(long *)OPTVAL(topt) = val; /* Turn the option on or off */
	nmp->b_wptr = nmp->b_rptr + len;
	return (isoc_sendioctl(q, mp, sd, nmp, len, TI_OPTMGMT, 
			       sd->isoc_state));
}

/*
 * STATIC int isoc_getudata(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *
 * Calling/Exit State:
 */
STATIC int
isoc_getudata(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	mblk_t *ump;
	int	rval;
	
	/* Need to send stuff downstream */
	/* Put the message back if you cannot proceed */
	if (!canput (q->q_next)) {
		putbq (q, mp);
		return (1);
	}

	LOG3 (SOCTRACE, "isocsocket get udata (%x, %x, %x)\n", q, mp, sd);

	if ((ump = allocb(sizeof(struct si_udata), BPRI_MED)) == NULL) {
		putbq (q, mp);
		(void) bufcall(sizeof(struct si_udata), BPRI_MED, qenable, (long)q);
		return (1);
	}

	ump->b_wptr = ump->b_rptr + sizeof (struct si_udata);
	rval = isoc_sendioctl (q, mp, sd, ump, sizeof (struct si_udata), 
			      SI_GETUDATA, sd->isoc_state);
	return(rval);
}

/*
 * STATIC int isoc_udatadone(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *
 * Calling/Exit State:
 */
STATIC int
isoc_udatadone(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	int		rval = 0;
	mblk_t		*ump;

	ASSERT(mp->b_cont != NULL);

	sd->isoc_smodcmd = 0;
	if (mp->b_datap->db_type != M_IOCACK)
		return (isocnak (q, mp, ISC_SO_SYSERR, rval, ISC_SO_GETUDATA));

	/* Copy our udata message */
	bcopy(mp->b_cont->b_rptr, sd->isoc_udata, sizeof(struct si_udata)); 

	ump = mp->b_cont;
	mp->b_cont = NULL;
	freemsg(ump);

	switch (sd->isoc_state) {
	case ISC_SS_OPENED:
		rval = isoc_socketdone(q, mp, sd);
		break;

	default:
		LOG1 (SOCERRS, "isoc_udatadone: Unexpected State:%d\n",
			    sd->isoc_state);

		rval = isocnak (q, mp, ISC_SO_OUTSTATE, 0, ISC_SO_GETUDATA);

	}
	return(rval);
}

/*
 * STATIC int
 * isoc_sendioctl(queue_t *q, mblk_t *mp, struct isocdev *sd, 
 *		mblk_t *ump, int len, int smodcmd, int state)
 *
 * Calling/Exit State:
 */
STATIC int
isoc_sendioctl(queue_t *q, mblk_t *mp, struct isocdev *sd, 
		mblk_t *ump, int len, int smodcmd, int state)
{
	struct iocblk *iocp;
	mblk_t	*nmp;
	uint	id;

	/* Some callers will pass a NULL mp  eg. isocbind */
	if ( mp && (sizeof(struct iocblk ) <= 
	    (mp->b_datap->db_lim - mp->b_datap->db_base))) {
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
		mp->b_rptr = mp->b_datap->db_base;
		nmp = mp;
	} else if ((nmp = allocb (sizeof (struct iocblk ), BPRI_MED)) == NULL) {
		if (ump)
			freemsg(ump);
		if (mp) {
			putbq (q, mp);
			(void) bufcall(sizeof(struct iocblk), BPRI_MED, 
				       qenable, (long)q);
		}
		return (sizeof(struct iocblk));
	} else if (mp)
		freemsg (mp);

	nmp->b_datap->db_type = M_IOCTL;
	nmp->b_wptr = nmp->b_rptr + sizeof (struct iocblk );
	nmp->b_cont = ump;

	drv_getparm(LBOLT, (ulong*)&id) ;
	/* LINTED pointer alignment */
	iocp = (struct iocblk *) nmp->b_rptr;
	iocp->ioc_id = id;
	iocp->ioc_cmd = smodcmd;
	iocp->ioc_count = len;
	iocp->ioc_error = 0;
	iocp->ioc_rval = 0;

	sd->isoc_state = (ushort)state;
	sd->isoc_smodcmd = smodcmd;
	sd->isoc_flags |= SOCF_IOCTLWAT;
	sd->isoc_ioctl = iocp->ioc_id;
	putnext (q, nmp);
	return (0);
}

/*
 * STATIC int
 * isoc_sendproto(queue_t *q, mblk_t *mp, struct isocdev *sd, 
 *		int smodcmd, int state)
 *
 * Calling/Exit State:
 */
STATIC int
isoc_sendproto(queue_t *q, mblk_t *mp, struct isocdev *sd, 
		int smodcmd, int state)
{

	sd->isoc_state = (ushort)state;
	sd->isoc_smodcmd = smodcmd;
	putnext (q, mp);
	return (0);
}

/*
 * STATIC int
 * isoc_aligned_copy(char *buf, int len, int init_offset,
 *		char *datap, int *rtn_offset)
 *
 * 	Copy data to output buffer and align it as in input buffer
 * 	This is to ensure that if the user wants to align a network
 * 	addr on a non-word boundry then it will happen.
 *
 * Calling/Exit State:
 */
STATIC int
isoc_aligned_copy(char *buf, int len, int init_offset,
		char *datap, int *rtn_offset)
{
	/* Check if the addresses are NOT user addresses */
	if (VALID_USR_RANGE(buf, len))
		return(EFAULT);
	if (VALID_USR_RANGE(datap, len))
		return(EFAULT);
	if (VALID_USR_RANGE(rtn_offset, 4))
		return(EFAULT);
		
	*rtn_offset = ROUNDUP(init_offset) + ((unsigned int)datap&0x03);
	bcopy(datap, (buf + *rtn_offset), len);
	return(0);
}

/*
 * STATIC int isoc_smoderror(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isoc_smoderror - handle a t_error_ack from the TLI module.
 *
 * Calling/Exit State:
 */
STATIC int
isoc_smoderror(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	struct iocblk *iocp;
	int	prim;
	int rval;
	int tli_cmd;
	int tli_error;
	

	LOG3 (SOCTRACE, "isoc_smoderror(%x, %x, %x)\n", q, mp, sd);

	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	tli_error = iocp->ioc_rval;
	if ((tli_error & 0xff) == TSYSERR)
		rval = (tli_error >> 8) & 0xffff;
	else {
		if ((tli_error > N_TLI_ERRS) || tli_error < 0 )
			rval = EPROTO;
		else
			rval = UNIX_error[tli_error];
	}

	tli_cmd = -1;
	prim = sd->isoc_cmd;

	switch(sd->isoc_smodcmd) {
	case TI_BIND:
		tli_cmd = T_BIND_REQ;
		break;

	case SI_LISTEN:
		tli_cmd = T_BIND_REQ;
		break;

	case TI_OPTMGMT:
		tli_cmd = T_OPTMGMT_REQ;
		break;

	case SI_SHUTDOWN:
		/* This could have been the ioctl ISC_SIOCSHUTDOWN */
		break;
	}

	if (prim <= 0) {
		freemsg(mp);
		return(0);
	}
	
	if (isocnak(q, mp, ISC_SO_SYSERR, rval, prim))
		return(1);

	switch (tli_cmd) {			/* fix the state */

	case T_BIND_REQ:
		if (sd->isoc_state == ISC_SS_WACK_BREQ)
			if (sd->isoc_type == ISC_SOCK_RAW)
				sd->isoc_state = ISC_SS_OPENED;
			else
				sd->isoc_state = ISC_SS_UNBND;
		break;

	case T_CONN_REQ:	/* ISC_SO_CONNECT */
		if (sd->isoc_state == ISC_SS_WACK_CREQ)
			sd->isoc_state = ISC_SS_BOUND;
		break;

	case T_CONN_RES:	/* ISC_SO_FDINSERT */
		if (sd->isoc_state == ISC_SS_WACK_CRES)
			sd->isoc_state = ISC_SS_PASSIVE;
		if (sd->isoc_spend != NULL)
			sd->isoc_spend->isoc_state = ISC_SS_BOUND;
		break;

	case T_DISCON_REQ:	/* ISC_SO_SHUTDOWN */
		if (sd->isoc_state == ISC_SS_SHUTDOWN || 
		    sd->isoc_state == ISC_SS_CONN_RONLY)
			/* already replied to user */
			sd->isoc_state = ISC_SS_PASSIVE;
		break;

	case T_DATA_REQ:		/* no state change with data */
	case T_EXDATA_REQ:		/* no state change with data */
	case T_INFO_REQ:		/* no state change with info */
	case T_UNBIND_REQ:
	case T_UNITDATA_REQ:		/* no state change with data */
	case T_OPTMGMT_REQ:		/* no state change with options */
	case T_ORDREL_REQ:
		break;

	default:
		LOG1 (SOCERRS, "isoc_smoderror: unexpected error primitive: %x\n",
				 sd->isoc_smodcmd);
	}
	return (0);
}

/*
 * STATIC int isocterror(queue_t *q, mblk_t *mp, struct isocdev *sd)
 *	isocterror - handle a t_error_ack from the TLI module.
 *
 * Calling/Exit State:
 */
STATIC int
isocterror(queue_t *q, mblk_t *mp, struct isocdev *sd)
{
	/* LINTED pointer alignment */
	struct T_error_ack *tprim = (struct T_error_ack *) mp->b_rptr;
	long	eprim = tprim->ERROR_prim;
	int	error, serror, prim;

	LOG3 (SOCTRACE, "isocterror(%x, %x, %x)\n", q, mp, sd);

	prim = 0;
	if (tprim->TLI_error == TOUTSTATE)
		serror = ISC_SO_OUTSTATE;
	else
		serror = ISC_SO_SYSERR;
	error = tprim->UNIX_error;
	if (eprim < 0 || eprim >= N_TLI_PRIMS) {

		LOG1 (SOCERRS, "isocterror: invalid error primitive: %x\n", eprim);

		return (isocnak (q, mp, ISC_SO_SYSERR, EPROTO, prim));
	}
	if (tprim->TLI_error != TSYSERR && error == 0)
		if (tprim->TLI_error < 0 || tprim->TLI_error >= N_TLI_ERRS)
			error = EPROTO;
		else
			error = UNIX_error[tprim->TLI_error];
	if ((prim = isocTLI_prims[eprim]) >= 0) {
		if (prim == ISC_SO_BIND)			/* handle special cases */
			if (sd->isoc_flags & SOCF_BINDWAT)
				if (sd->isoc_backlog > 0)
					prim = ISC_SO_LISTEN;
				else
					prim = ISC_SO_CONNECT;
			else if (sd->isoc_type == ISC_SOCK_RAW)
				prim = ISC_SO_SOCKET;
		if (isocnak (q, mp, serror, error, prim))
			return (1);
	} else {			/* no error returns for these */
		freemsg (mp);
		return (0);
	}

	switch (eprim) {			/* fix the state */

	case T_BIND_REQ:
		if (sd->isoc_state == ISC_SS_WACK_BREQ)
			if (sd->isoc_type == ISC_SOCK_RAW)
				sd->isoc_state = ISC_SS_OPENED;
			else
				sd->isoc_state = ISC_SS_UNBND;
		break;

	case T_CONN_REQ:	/* ISC_SO_CONNECT */
		if (sd->isoc_state == ISC_SS_WACK_CREQ)
			sd->isoc_state = ISC_SS_BOUND;
		break;

	case T_CONN_RES:	/* ISC_SO_FDINSERT */
		if (sd->isoc_state == ISC_SS_WACK_CRES)
			sd->isoc_state = ISC_SS_PASSIVE;
		if (sd->isoc_spend != NULL)
			sd->isoc_spend->isoc_state = ISC_SS_BOUND;
		break;

	case T_DISCON_REQ:	/* ISC_SO_SHUTDOWN */
		if (sd->isoc_state == ISC_SS_SHUTDOWN || sd->isoc_state == ISC_SS_CONN_RONLY)
			sd->isoc_state = ISC_SS_PASSIVE;	/* already replied to user */
		break;

	case T_DATA_REQ:		/* no state change with data */
	case T_EXDATA_REQ:		/* no state change with data */
	case T_INFO_REQ:		/* no state change with info */
	case T_UNBIND_REQ:
	case T_UNITDATA_REQ:		/* no state change with data */
	case T_OPTMGMT_REQ:		/* no state change with options */
	case T_ORDREL_REQ:
		break;

	default:

		LOG1 (SOCERRS, "isocterror: unexpected error primitive: %x\n", eprim);

	}
	return (0);
}

#ifdef SOCDEBUG
/*
 * STATIC char * hex_sprintf(u_char *ap, int len)
 *	Convert a string of characters to hex (loggable) representation.
 *
 * Calling/Exit State:
 */
STATIC char *
hex_sprintf(u_char *ap, int len)
{
	int i;
	static char	hexbuf[100];
	char	*cp = hexbuf;
	static char	digits[] = "0123456789abcdef";

	for (i = 0; i < len; i++) {
		*cp++ = digits[*ap >> 4];
		*cp++ = digits[*ap++ & 0xf];
		*cp++ = ':';
	}
	*--cp = 0;
	return (hexbuf);
}
#endif /* SOCDEBUG */
