#ident	"@(#)osocket.c	1.8"
#ident	"$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

/* Enhanced Application Binary Compatibility */
/* SCO Sockets emulation driver.		     */

#include <acc/priv/privilege.h>
#include <util/types.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/param.h>
#include <svc/clock.h>
#include <svc/systm.h>
#include <io/uio.h>
#include <svc/errno.h>
#include <proc/signal.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <fs/fstyp.h>
#include <io/stropts.h>
#include <io/stream.h>
#include <io/streamio.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/filio.h>
#include <fs/specfs/specfs.h>
#include <mem/kmem.h>
#include <mem/vmparam.h>
#include <util/cmn_err.h>
#include <net/convsa.h>
#include <net/timod.h>
#include <net/tiuser.h>
#include <net/tihdr.h>
#include <net/sockmod.h>
#include <net/socket.h>
#include <net/sockio.h>
#include <net/socketvar.h>
#include <net/un.h>
#include <util/debug.h>
#include <proc/session.h>
#include <io/poll.h>
#include <net/inet/in.h>
#include <net/inet/if.h>
#include <io/mkdev.h>
#include <io/ioctl.h>
#include <net/un.h>

#include <net/osocket/osocket.h>
#include <svc/sco.h>

#include <util/mod/moddefs.h>

#define	DRVNAME	"osocket - SCO socket emulation driver"
#define LOCAL_ADDR 1
#define REMOTE_ADDR 2

extern int strmsgsz;
extern int strctlsz;

STATIC int osoc_getsocket_with_dev(struct osocket **, dev_t);
STATIC int osoc_sofree(struct osocket *);
STATIC int osoc_soreceive(struct osocket *, struct msghdr *, int, int *);
STATIC int osoc_sosend(struct osocket *, struct msghdr *, int, int *);
STATIC int osoc_addproto(struct osocknewproto *);
STATIC int osoc_relprotos(void);
STATIC int osoc_do_ioctl(struct osocket *, char *, int, int, int *);
STATIC int osoc_sockopen(int, int, int, int *);
STATIC int osoc_create(struct osocket **, int, int, int, int);
STATIC int osoc_smodopen(struct osocket *, int);
STATIC int osoc_getargs(caddr_t, caddr_t, int);
STATIC int osoc_dobind(struct osocket *, struct sockaddr *, int, char *,int *);
STATIC int osoc_aligned_copy(char *, int, int, char *, int *);
STATIC int osoc_ualigned_copy(char *, int, int, char *, int *);
STATIC int osoc_dounbind(struct osocket *);
STATIC int osoc_cpaddr(char *, int, char *, int, int *);
STATIC int osoc_doaccept(struct osocket *, struct osockaddr *, int, 
			int *, int *);
STATIC int osoc_getmsg(struct file *, struct strbuf *, struct strbuf *, 
		int *, rval_t *);
STATIC int osoc_doclose(struct osocket *);
STATIC int osoc_is_ok(struct osocket *, long, int *);
STATIC int osoc_do_fcntl(struct file *, int, int, rval_t *);
STATIC int osoc_doconnect1(struct osocket *, int, int);
STATIC int osoc_doconnect2(struct osocket *, struct t_call *);
STATIC int osoc_snd_conn_req(struct osocket *, struct t_call *);
STATIC int osoc_rcv_conn_con(struct osocket *);
STATIC int osoc_putmsg(struct file *, struct strbuf *, struct strbuf *, 
		int, rval_t *);
STATIC int osoc_dogetname(struct osocket *, struct osockaddr *, int *, int, int);
STATIC int osoc_getsocket_with_fd(struct osocket **, int);
STATIC int osoc_dosetsockopt(struct osocket *, int, int, char *, int, int);
STATIC int osoc_getudata(struct osocket *, int);
STATIC int osoc_doadjtime(struct timeval *, struct timeval *);
STATIC int osoc_tlitosyserr(int);
struct hold;
STATIC int osoc_get_msg_slice(struct msghdr *, char **, int,int, struct hold *);
STATIC int osoc_msgio(struct file *, struct strbuf *, struct strbuf *, 
		rval_t *, int, unsigned char *, int *);

MOD_DRV_WRAPPER(osoc, NULL, NULL, NULL, DRVNAME);

/*
 * This is defined in kernel space.c since it is needed 
 * to hold the protocol mapping even when the module is
 * unloaded.
 */
extern struct odomain  *osoc_family;

/*
 * Define local and external routines.
 */

int             osocdevflag = 0;
int             osockinited = 0;

struct socketsysa {
	struct socksysreq *argp;
};

#define OSOCCLONE_MIN	1
#define OSOCCLONE_NAME	"/dev/socksys_c"


extern int num_osockets;
extern struct osocket *osocket_tab[];
extern char osoc_domainbuf[];

extern int osoc_ncalls;
extern int (*osoc_call[]) ();
extern int osocopen(dev_t *, int, int, cred_t *);
int socketsys(struct socketsysa *, rval_t *);

/* 
 * svr4_to_sco error table was here... it did errno xlatation here instead
 * of the system call level because ISC COFF was different... this lead to
 * 'regular', non-socket related errnos not being translated. We have
 * moved the translation to the common system call area (coff_errno) now
 * that we are SCO
 */


/*
 * int osocopen(dev_t *devp, int flag, int type, cred_t *cr)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
int
osocopen(dev_t *devp, int flag, int type, cred_t *cr)
{
	int		rval;
	minor_t		sockno;
	minor_t		free_soc;

	rval = 0;

	if (!osockinited) { 
		/*
		 * Reserve first osocket table entry so as to 
		 * provide a mechanism to do get a socket and 
		 * support admin functions.
		 */
		osocket_tab[0] = OSOCK_RESERVE;
		/* 
		 * also reserve the second as the 'clone' device
		 */
		osocket_tab[1] = OSOCK_RESERVE;

		osockinited = 1;
	}

	sockno = getminor(*devp);
	if (sockno == OSOCCLONE_MIN ) {
		/*
		 * clone open. return the next free device.
		 */
		/*
	 	* Look for a free socket.
	 	* First socket slot is reserved for pseudo system call interface.
		* second for the clone device.
	 	*/
		for (free_soc = 2; free_soc < num_osockets; free_soc++)
			if (osocket_tab[free_soc] == OSOCK_AVAIL)
				break;

		if (free_soc >= num_osockets) {
			rval = ENXIO;
		}
		else {
			osocket_tab[free_soc] = OSOCK_INPROGRESS;
			*devp = makedevice(getemajor(*devp), free_soc);

		}
	}

	return(rval);
}

/*
 * int osocclose(dev_t dev, int flag, int otyp, cred_t *cr)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
int
osocclose(dev_t dev, int flag, int otyp, cred_t *cr)
{
	int		rval;
	minor_t		sockno;
	struct osocket	*so;

	sockno = getminor(dev);

	if (sockno < 2)		/* Pseudo Socket and Clone socket */
		return(0);

	rval = osoc_getsocket_with_dev(&so, dev);
	if (rval) {
		return(rval);
	}

	/* Break down the parallel Socket/Transport Provider */
	if (so->so_sfp) {
		closef(so->so_sfp);
		if (so->so_sfd) {
			setf(so->so_sfd, NULLFP);
			so->so_sfd = 0;
		}
	}

	rval = osoc_sofree(so);
	if (rval)
		/*
		 *+ osocclose: Error Dropping socket
		 */
		cmn_err(CE_WARN, 
			"osocclose: Error Dropping socket %d \n", sockno);

	osocket_tab[sockno] = OSOCK_AVAIL;
	return(rval);
}

/*
 * int osocread(dev_t dev, struct uio *uiop, cred_t *cr)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
int
osocread(dev_t dev, struct uio *uiop, cred_t *cr)
{
	minor_t		sockno;
	struct osocket 	*so;
	struct msghdr	msg;
	int		rval;
	int		retval;
	struct vnode	*sys_vp;

	sockno = getminor(dev);
	if (sockno < 2 ) {
		return(rval);
	}

	rval = osoc_getsocket_with_dev(&so, dev);
	if (rval) {
		return(rval);
	}

	sys_vp = so->so_svp;
	if (sys_vp) {
		msg.msg_iovlen = uiop->uio_iovcnt;
		msg.msg_iov = uiop->uio_iov;
		msg.msg_namelen = 0;
		msg.msg_name = NULL;
		msg.msg_accrightslen = 0;
		msg.msg_accrights = NULL;
		retval = 0;
		rval = osoc_soreceive(so, &msg, 0, &retval);
		uiop->uio_resid -= retval;
		uiop->uio_offset += retval;
	} else
		rval = EINVAL;

	return(rval);
}

/*
 * int osocwrite(dev_t dev, struct uio *uiop, cred_t *cr)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
int
osocwrite(dev_t dev, struct uio *uiop, cred_t *cr)
{
	minor_t		sockno;
	struct osocket 	*so;
	struct msghdr	msg;
	int		rval;
	int		retval;
	struct vnode	*sys_vp;

	sockno = getminor(dev);
	if (sockno < 2 ) {
		return(rval);
	}

	rval = osoc_getsocket_with_dev(&so, dev);
	if (rval) {
		return(rval);
	}

	sys_vp = so->so_svp;
	if (sys_vp) {
		msg.msg_iovlen = uiop->uio_iovcnt;
		msg.msg_iov = uiop->uio_iov;
		msg.msg_namelen = 0;
		msg.msg_name = NULL;
		msg.msg_accrightslen = 0;
		msg.msg_accrights = NULL;
		retval = 0;
		rval = osoc_sosend(so, &msg, 0, &retval);
		uiop->uio_resid -= retval;
		uiop->uio_offset += retval;
	} else
		rval = EINVAL;

	return(rval);
}

/*
 * int 
 * osocioctl(dev_t dev, u_int cmd, caddr_t arg,int flag, cred_t *cr, int *rvalp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
int
osocioctl(dev_t dev, u_int cmd, caddr_t arg, int flag, cred_t *cr, int *rvalp)
{
	int		in_out[(OIOCPARM_MASK+ sizeof(int) - 1)/sizeof(int)];
	uint		size;
	struct osocket  *so;
	int		rval;
	int		retval;
	minor_t		sockno;
	int		sockfunc;
	int		*args;
	int		pid;
	struct _si_user	*siptr;
	struct file	*sys_fp;
	struct vnode	*sys_vp;

	rval = 0;
	sockno = getminor(dev);
	so = (struct osocket *)OSOCK_AVAIL;
	if (sockno > 1 ) {
		if (((int)sockno < 0) || ((int)sockno > num_osockets)) {
			return(rval);
		} else 
			so = osocket_tab[sockno];
		/*
		 * A socket minor number -- Veriry if setup is done
		 */
		if (((so == OSOCK_AVAIL) || 
		    (so == OSOCK_INPROGRESS) ||
		    (so == OSOCK_RESERVE) ||
		    (so->so_svp == NULL)) &&
			cmd != OSIOCPROTO && cmd != OSIOCXPROTO) {

			return(rval);

		} else {
			sys_fp = so->so_fp;
			sys_vp = so->so_svp;
			siptr = &so->so_user;
		}
	} else if (cmd != OSIOCSOCKSYS) {
		return(rval);
	} else {
		sys_fp = NULL;
		sys_vp = NULL;
		siptr = NULL;
	}

	/*
	 * Extract the size of the input/output arguments
	 */

	size = (cmd & ~(OIOC_INOUT | OIOC_VOID)) >> 16;
	if (size > sizeof(in_out)) {
		return(rval);
	}

	if (cmd & OIOC_IN) {
		if (size) {
			if (copyin(arg, (caddr_t)in_out, size)) {
				return(rval);
			}
		} else
			*(caddr_t *) in_out = arg;

	} else if ((cmd & OIOC_OUT) && size) {

		/*
		 * Initialize the stack var.
		 */
		bzero((caddr_t)in_out, size);

	} else if (cmd & OIOC_VOID) {
		
		*(caddr_t *) in_out = arg;

	}

	switch (cmd) {
	case OSIOCPROTO:
		/* Add a new protocol to protosw */
		if (!pm_denied(cr, P_SYSOPS))
			rval = osoc_addproto((struct osocknewproto *)in_out);
		else
			rval = EPERM;
		break;

        case OSIOCXPROTO:
		/* Zap the protosw */
		if (!pm_denied(cr, P_SYSOPS))
			osoc_relprotos();
		else
			rval = EPERM;
                break;

	case OSIOCSOCKSYS:

		args = ((struct osocksysreq *)in_out)->args;
		sockfunc = *args++;
		if ((sockfunc < 0) || (sockfunc >= osoc_ncalls))
			sockfunc = 0;

		rval = (*osoc_call[sockfunc])(args, rvalp);
		break;

	case OFIONREAD:
		if (sys_vp == NULL) {
			rval = EINVAL;
			break;
		}

		rval = strioctl(sys_vp, FIONREAD, (int)in_out, sys_fp->f_flag, 
				K_TO_K, cr, rvalp);

		break;

	case OFIONBIO:
		if (sys_vp == NULL) {
			rval = EINVAL;
			break;
		}

		rval = strioctl(sys_vp, FIONBIO, (int)in_out, sys_fp->f_flag, 
				K_TO_K, cr, rvalp);
		if (!rval) {
			if (in_out[0]) {
				sys_fp->f_flag |= FNDELAY;
			} else {
				sys_fp->f_flag &= ~FNDELAY;
			}
		}
		break;

	case OFIOASYNC:
		/*
 		 * Enable or disable asynchronous I/O
		 * Facilitate SIGIO.
		 */

		/*
		 * Turn on or off async I/O.
		 */
		if (sys_vp == NULL) {
			rval = EINVAL;
			break;
		}

		retval = 0;
		if (in_out[0]) {
			/*
			 * Turn ON SIGIO if
			 * it is not already on.
			 */
			if ((siptr->flags & S_SIGIO) != 0)
				break;
	
			if (siptr->flags & S_SIGURG)
				retval = S_RDBAND|S_BANDURG;
			retval |= S_RDNORM|S_WRNORM;
	
			rval = strioctl(sys_vp, I_SETSIG, (int)&retval, 
					sys_fp->f_flag, K_TO_K, 
					cr, rvalp);
			if (rval)
				break;
	
			siptr->flags |= S_SIGIO;
			break;
		}
	
		/*
		 * Turn OFF SIGIO if
		 * not already off.
		 */
		if ((siptr->flags & S_SIGIO) == 0)
			break;
	
		siptr->flags &= ~S_SIGIO;
	
		if (siptr->flags & S_SIGURG)
			retval = S_RDBAND|S_BANDURG;
	
		rval = strioctl(sys_vp, I_SETSIG, (int)&retval, 
				sys_fp->f_flag, K_TO_K, 
				cr, rvalp);
	
		break;

	case OSIOCGPGRP:
		if (sys_vp == NULL) {
			rval = EINVAL;
			break;
		}

		rval = strioctl(sys_vp, I_GETSIG, (int)in_out, 
				sys_fp->f_flag, K_TO_K, 
				cr, rvalp);
		if (rval == EINVAL) {
			in_out[0] = 0;
			rval = 0;
		} 
		if (!rval && 
		   (in_out[0] & (S_RDBAND|S_BANDURG|S_RDNORM|S_WRNORM)))
			*(pid_t *)in_out = u.u_lwpp->l_lwpid;
		else	
			*(pid_t *)in_out = 0;

		break;

	case OSIOCSPGRP:
		/*
		 * Facilitate receipt of SIGURG.
		 *
		 * We are forgiving in that if a
		 * process group was specified rather
		 * than a process id, we will only
		 * fail it if the process group
		 * specified is not the callers.
		 */
		if (sys_vp == NULL) {
			rval = EINVAL;
			break;
		}

		pid = *(pid_t *)in_out;
		if (pid < 0) {
			pid = -pid;
			if (pid != u.u_procp->p_pgid) {
				rval = EINVAL;
				break;
			}
		} else	{
			if (pid != u.u_lwpp->l_lwpid) {
				rval = EINVAL;
				break;
			}
		}

		retval = 0;
		if (siptr->flags & S_SIGIO)
			retval = S_RDNORM|S_WRNORM;
		retval |= S_RDBAND|S_BANDURG;
		rval = strioctl(sys_vp, I_SETSIG, (int)&retval, 
				sys_fp->f_flag, K_TO_K, 
				cr, rvalp);
		break;

	case OSIOCATMARK:
		if (sys_vp == NULL) {
			rval = EINVAL;
			break;
		}

		retval = 0;
		rval = strioctl(sys_vp, I_ATMARK, LASTMARK, 
				sys_fp->f_flag, K_TO_K, 
				cr, &retval);
		if (!rval) {
			*(int *)in_out = retval;
			*rvalp = 0;
		}
		break;


	case OSIOCGIFFLAGS:
		{
			/* This Request will pass the user datastructure */
			/* for the "struct oifreq"			 */

			struct ifreq	*ifr;
			struct oifreq	*oifr;
			int		len;

			if (sys_vp == NULL) {
				rval = EINVAL;
				break;
			}

			oifr = (struct oifreq *)&in_out[0];
			if (size < sizeof(struct oifreq)) {
				rval = EINVAL;
				break;
			}

			len = sizeof(struct ifreq);

			ifr = kmem_zalloc(len, KM_SLEEP);
			bcopy(oifr->ifr_name, ifr->ifr_name, 
				      sizeof(ifr->ifr_name));
			retval = 0;
			rval = osoc_do_ioctl(so, (char *)ifr, len, SIOCGIFFLAGS, 
					     &retval);
			if ((rval == 0) && (retval >= 0)) {
				/* 
				 * Get the flags from the provider and 
				 * copy them to the equivalent position
				 */
				oifr->ifr_flags = ifr->ifr_flags;
			}
			kmem_free((caddr_t)ifr, len);
		}
		break;


	case OSIOCGIFCONF:
		{
			struct ifreq	*ifr;
			struct oifconf	*oifc;
			struct oifreq	*oifr;
			caddr_t		ptr;
			int		len;
			int		olen;

			if (sys_vp == NULL) {
				rval = EINVAL;
				break;
			}

			oifc = (struct oifconf *)&in_out[0];
			oifr = oifc->ifc_req;
			len = oifc->ifc_len;

			if (len <= 0)
				break;

			/* There may be more than one provider */
			ifr = kmem_zalloc(len, KM_SLEEP);
			ptr = (caddr_t)ifr;
			retval = 0;
			rval = osoc_do_ioctl(so, (char *)ifr, len, SIOCGIFCONF, 
					     &retval);
			olen = 0;
			if (rval == 0) {
				/* 
				 * Get the contents of each provider and 
				 * copy them to the equivalent position
				 */
				while (retval > 0) {
					rval = copyout(ifr->ifr_name, 
							oifr->ifr_name, 
							sizeof(ifr->ifr_name));
					if (rval != 0) {
						rval = EFAULT;
						break;
					}
					if ((retval - sizeof(ifr->ifr_name)) >=
					    sizeof(struct osockaddr)) {
						rval = copyout(
						    (caddr_t)&ifr->ifr_addr, 
						    (caddr_t)&oifr->oifr_addr,
						    sizeof(struct osockaddr));
						if (rval != 0) {
							rval = EFAULT;
							break;
						}
					} else {
						rval = EINVAL;
						break;
					}
					retval -= sizeof(struct ifreq);
					olen += sizeof(struct oifreq);
					ifr++;
					oifr++;
				}
				oifc->ifc_len = olen;
			}
			kmem_free(ptr, len);
		}
		break;

	default:
		rval = EINVAL;
		break;

	}

	/*
	 * Copyout the data to user.
	 */
	if (rval == 0 && (cmd & OIOC_OUT) && size) {
		if (copyout((caddr_t)in_out, arg, size))
			rval = EFAULT;
	}

	return(rval);
}

/*
 * int osocchpoll(dev_t dev, short events, int anyyet, short *reventsp,
 *			struct pollhead **phpp)
 *
 * Calling/Exit State:
 */
int
osocchpoll(dev_t dev, short events, int anyyet, short *reventsp,
	struct pollhead **phpp)
{
	minor_t		sockno;
	struct osocket 	*so;
	int		rval;
	struct vnode	*sys_vp;

	sockno = getminor(dev);
	if (sockno  < 2 ) {
		return(rval);
	}

	rval = osoc_getsocket_with_dev(&so, dev);
	if (rval) {
		return(rval);
	}
	sys_vp = so->so_svp;
	if (sys_vp)
		rval = strpoll(sys_vp->v_stream, events, anyyet, 
			       reventsp, phpp);
	else
		rval = EINVAL;

	return(rval);
}

/*
 * int osocmmap()
 *
 * Calling/Exit State:
 */
int
osocmmap()
{
	return(0);
}

/*
 * int osocsegmap()
 *
 * Calling/Exit State:
 */
int
osocsegmap()
{
	return(0);
}


/*
 * int osoc_addproto(struct osocknewproto *nproto)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_addproto(struct osocknewproto *nproto)
{
	struct odomain	*domp;
	struct oprotosw	*prp;
	int		s;

	/* Check if the family exits */
	for (domp = osoc_family; domp; domp = domp->dom_next)
		if (domp->dom_family == nproto->family)
			break;

	/* Allocate space for the family */
	if (domp == NULL) {
		domp = (struct odomain *) kmem_zalloc(sizeof(struct odomain), 
						   KM_SLEEP);
		s = splstr();
		domp->dom_family = nproto->family;
		domp->dom_protosw = NULL;
		domp->dom_next = osoc_family;
		osoc_family= domp;
		splx(s);
	}

	/* Check if the type/protocol exists */
	for (prp = domp->dom_protosw; prp; prp = prp->pr_next) {
		if (prp->pr_type == nproto->type
		    && prp->pr_protocol == nproto->proto) {
			/*
			 * Do not have to free the memory allocated 
			 * for the family because this protocol/type
			 * would not have existed.
			 */
			return (EPROTOTYPE);
		}
	}

	/* Allocate space for the type/protocol */
	prp = (struct oprotosw *) kmem_zalloc(sizeof(struct oprotosw), 
					      KM_SLEEP);

	/* Add the type/protocol to the family */
	s = splstr();
	prp->pr_type = nproto->type;
	prp->pr_domain = domp;
	prp->pr_protocol = nproto->proto;
	prp->pr_flags = nproto->flags;
	prp->pr_device = nproto->dev;
	prp->pr_next = domp->dom_protosw;
	domp->dom_protosw = prp;
	splx(s);
	return (0);
}

/*
 * int osoc_relprotos()
 *
 * Calling/Exit State:
 */
STATIC int
osoc_relprotos()
{
	struct odomain	*domp;
	struct oprotosw	*prp;
	struct odomain	*dompnext;
	struct oprotosw	*prpnext;
	int		s;

	/* Free allocated space for all families and protocols */
	s = splstr();
	for (domp = osoc_family; domp; domp = dompnext) {
		for (prp = domp->dom_protosw; prp; prp = prpnext) {
			prpnext = prp->pr_next;
			kmem_free(prp, sizeof(struct oprotosw));
		}
		dompnext = domp->dom_next;
		kmem_free(domp, sizeof(struct odomain));
	}
	osoc_family = NULL;
	splx(s);
	return(0);
}

/*
 * struct oprotosw *osoc_gettype(int family, int type)
 *
 * Calling/Exit State:
 */
struct oprotosw *
osoc_gettype(int family, int type)
{
	struct odomain	*domp;
	struct oprotosw	*prp;

	/* Get the family */
	for (domp = osoc_family; domp; domp = domp->dom_next)
		if (domp->dom_family == family)
			break;
	if (!domp)
		return (NULL);

	/* Found the family -- Search for the type */
	for (prp = domp->dom_protosw; prp; prp = prp->pr_next)
		if (prp->pr_type && prp->pr_type == type)
			return (prp);
	return (NULL);
}

/*
 * struct oprotosw *osoc_getproto(int family, int type, int proto)
 * 	Match the type and protocol  -- Special handling for Raw Sockets
 *
 * Calling/Exit State:
 */
struct oprotosw *
osoc_getproto(int family, int type, int proto)
{
	struct odomain	*domp;
	struct oprotosw	*prp;
	struct oprotosw	*maybe;

	maybe  = NULL;

	if (family == 0)
		return (NULL);

	for (domp = osoc_family; domp; domp = domp->dom_next)
		if (domp->dom_family == family)
			break;

	if (!domp)
		return (NULL);

	/* _s_match() code */
	for (prp = domp->dom_protosw; prp; prp = prp->pr_next) {
		if (proto) {
			if ((prp->pr_type == type) && 
			    (prp->pr_protocol == proto))
				return (prp);
			if ((type == OSOCK_RAW) &&
			    (prp->pr_type == OSOCK_RAW) &&
			    (prp->pr_protocol == 0) &&
			    (maybe == (struct oprotosw *) NULL)) {
				maybe = prp;
			}
		} else if (prp->pr_type == type) 
			return (prp);
	}
	return (maybe);
}


/*
 * The socket functions translated from the user level library 
 * libsocket/socket
 */

extern int nosys(char *, rval_t *);

struct accepta;
STATIC int osoc_accept(struct accepta *, int *);
struct binda;
STATIC int osoc_bind(struct binda *, int *);
struct connecta;
STATIC int osoc_connect(struct connecta *, int *);
struct getpeera;
STATIC int osoc_getpeername(struct getpeera *, int *);
struct getsocka;
STATIC int osoc_getsockname(struct getsocka *, int *);
struct getsockopta;
STATIC int osoc_getsockopt(struct getsockopta *, int *);
struct listena;
STATIC int osoc_listen(struct listena *, int *);
struct recva;
STATIC int osoc_recv(struct recva *, int *);
struct recvfa;
STATIC int osoc_recvfrom(struct recvfa *, int *);
struct senda;
STATIC int osoc_send(struct senda *, int *);
struct sendfa;
STATIC int osoc_sendto(struct sendfa *, int *);
struct setsockopta;
STATIC int osoc_setsockopt(struct setsockopta *, int *);
struct shutdowna;
STATIC int osoc_shutdown(struct shutdowna *, int *);
struct socketa;
STATIC int osoc_socket(struct socketa *, int *);
struct getipdoma;
STATIC int osoc_getipdomain(struct getipdoma *, int *);
struct setipdoma;
STATIC int osoc_setipdomain(struct setipdoma *, int *);
struct adjtimea;
STATIC int osoc_adjtime(struct adjtimea *, int *);
struct setreuida;
STATIC int osoc_setreuid(struct setreuida *, int *);
struct setregida;
STATIC int osoc_setregid(struct setregida *, int *);
struct gettimea;
STATIC int osoc_gettime(struct gettimea *, int *);
struct settimea;
STATIC int osoc_settime(struct settimea *, int *);
struct recvmsga;
STATIC int osoc_recvmsg_wrapper(struct recvmsga *, int *);
struct sendmsga;
STATIC int osoc_sendmsg(struct sendmsga *, int *);
struct sockpaira;
STATIC int osoc_sockpair(struct sockpaira *, int *);
STATIC int osoc_nosys(void);

int	(*osoc_call[]) () = {
	                nosys,			/* NOT USED		 */
	                osoc_accept,		/* OSO_ACCEPT		 */
	                osoc_bind,		/* OSO_BIND		 */
	                osoc_connect,		/* OSO_CONNECT		 */
	                osoc_getpeername,	/* OSO_GETPEERNAME	 */
	                osoc_getsockname,	/* OSO_GETSOCKNAME	 */
	                osoc_getsockopt,	/* OSO_GETSOCKOPT	 */
	                osoc_listen,		/* OSO_LISTEN		 */
	                osoc_recv,		/* OSO_RECV		 */
	                osoc_recvfrom,		/* OSO_RECVFROM		 */
	                osoc_send,		/* OSO_SEND		 */
	                osoc_sendto,		/* OSO_SENDTO		 */
	                osoc_setsockopt,	/* OSO_SETSOCKOPT	 */
	                osoc_shutdown,		/* OSO_SHUTDOWN		 */
	                osoc_socket,		/* OSO_SOCKET		 */
	                osoc_nosys,		/* OSO_SELECT		 */
			osoc_getipdomain,	/* OSO_GETIPDOMAIN	 */
			osoc_setipdomain,	/* OSO_SETIPDOMAIN	 */
			osoc_adjtime,		/* OSO_ADJTIME		 */
			osoc_setreuid,		/* OSO_SETREUID		 */
			osoc_setregid,		/* OSO_SETREGID		 */
			osoc_gettime,  		/* OSO_GETTIME		 */
			osoc_settime,           /* OSO_SETTIME	         */
			osoc_nosys,             /* SO_GETITIMER          */
			osoc_nosys,             /* SO_SETITIMER          */
			osoc_recvmsg_wrapper,	/* SO_RECVMSG            */
			osoc_sendmsg,           /* SO_SENDMSG            */
			osoc_sockpair           /* SO_SOCKPAIR           */

};

int osoc_ncalls = sizeof(osoc_call) / sizeof(osoc_call[0]);

/*
 * STATIC int osoc_nosys()
 *
 * Calling/Exit State:
 */
STATIC int
osoc_nosys()
{
	return(EINVAL);
}

struct socketa {
	int             family;
	int             type;
	int             proto;
};

/*
 * STATIC int osoc_socket(struct socketa *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_socket(struct socketa *uap, int *rvalp)
{
	int		error;
	struct osocket	*so;

	/*
	 * We don't support SOCK_STREAM on AF_UNIX, therefore if we see one,
	 * (eg. from socketpair()) quietly turn it into a SOCK_DGRAM.
	 */
	
	if (uap->family == OAF_UNIX)
		uap->type = OSOCK_DGRAM;

	error = osoc_sockopen(uap->family, uap->type, uap->proto, rvalp);
	if (!error) {
		error = osoc_getsocket_with_fd(&so, *rvalp);
		if (!error && so->so_sfd) {
			setf(so->so_sfd, NULLFP);
			so->so_sfd = 0;
		} else {
			/*
			** It must be setting up the family/type/proto 
			** structures using the Reserved Pseudo socket
			*/
			error = 0;
		}
	}
	return(error);
}

/*
 * STATIC int osoc_sockopen(int family, int type, int proto, int *rvalp)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_sockopen(int family, int type, int proto, int *rvalp)
{
	minor_t	sockno;
	int	fd;
	int	rdev;
	int	rval;
	int	flags;
	struct	vnode *dev_vp;
	struct osocket  *so;
	struct file *fp;

	if ((family < 0) || (proto < 0))
		return(EPROTONOSUPPORT);

	flags = FREAD|FWRITE;

	if((rval = vn_open(OSOCCLONE_NAME, UIO_SYSSPACE, flags, 0, &dev_vp, 0)) != 0) {
		return(rval);
	}

	if ((rval = falloc(dev_vp, flags, &fp, &fd)) != 0) {
		VOP_CLOSE(dev_vp, flags, 1, 0, u.u_lwpp->l_cred);
		return (rval);
	}

	so = OSOCK_AVAIL;

	/*
	 * If type and proto are non-zero then the BIND process fails
	 * For now turn-off the protocol since we have only one protocol
	 * for each type of socket.
	 */

	rval = osoc_create(&so, family, type, proto, fd);

	if (rval) {
		closef(fp);
		setf(fd, NULLFP);
		return(rval);
	}

	so->so_fd = fd;
	so->so_fp = fp;
	so->so_uvp = dev_vp;
	*rvalp = fd;
	sockno = getminor(dev_vp->v_rdev);
	osocket_tab[sockno] = so;
	return(0);
}

/*
 * STATIC int 
 * osoc_create(struct osocket **sopp, int family, int type, int proto, int fd)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_create(struct osocket **sopp, int family, int type, int proto, int fd)
{
	struct oprotosw *prp = (struct oprotosw *) NULL;
	struct osocket *so;
	int	rval;

	/* Search for a match in family/type or family/type/proto */
	if (family != 0 || proto != 0 || type != 0) {
		if (proto)
			prp = osoc_getproto(family, type, proto);
		else
			prp = osoc_gettype(family, type);
		if (prp == NULL)
			return (EPROTONOSUPPORT);
		if (prp->pr_type != type)
			return (EPROTOTYPE);
	}

	so = (struct osocket *) kmem_zalloc(sizeof(struct osocket), KM_SLEEP);
	so->so_type = (short)type;
	if ((type == OSOCK_RAW) && pm_denied(u.u_lwpp->l_cred, P_SYSOPS)) {
		kmem_free(so, sizeof(struct osocket));
		return (EACCES);
	}

	*sopp = so;

	if (prp == NULL) /* protoless */
		return (0);

	so->so_proto = *prp;
	if (rval = osoc_smodopen(so, proto))
		return (rval);

	so->so_user.family = family;
	so->so_user.fd = fd;

	return(0);
}

struct binda {
	int             s;
	caddr_t         name;
	int             namelen;
};

/*
 * STATIC int osoc_bind(struct binda *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_bind(struct binda *uap, int *rvalp)
{
	
	struct osocket *so;
	struct _si_user *siptr;
	int	rval;

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	siptr = &so->so_user;

	rval = osoc_getargs((caddr_t)&so->so_addr, uap->name, uap->namelen);

	if (rval)
		return(rval);

	if (siptr->udata.so_state & SS_ISBOUND) {
		return (EINVAL);
	}

	/*
	 * Only AF_INET domains
	 */
	if (so->so_addr.sa_family !=  OAF_INET)
	{
		return (EINVAL);
	}

	/* Convert the sockaddr just read in to new_sockaddrs */
	OLD_SOCKADDR_TO_SOCKADDR(&(so->so_addr), uap->namelen);

	rval = osoc_dobind(so, (struct sockaddr *)&so->so_addr, uap->namelen,
			                                            NULL, NULL);
	return(rval);
}

/*
 * STATIC int osoc_dobind(struct osocket *so, struct osockaddr *name, 
 *	int namelen, char *raddr, int *raddrlen)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_dobind(struct osocket *so, struct sockaddr *name, int namelen, 
		char *raddr, int *raddrlen)
{
	struct _si_user		*siptr;
	char			*buf;
	struct T_bind_req	*bind_req;
	struct T_bind_ack	*bind_ack;
	struct bind_ux		bind_ux;
	int			size;
	int			rval;

	siptr = (struct _si_user *)&so->so_user;

	if (siptr->family == OAF_UNIX)
	{
		bzero(&bind_ux, sizeof (bind_ux));

		if (name == NULL) {
			bind_ux.name.sun_family = AF_UNIX;
			bind_ux.name.sun_len = SUN_LEN(&(bind_ux.name)) + 1;
		} else {
			/* Currently we don't support binding to a specific path */
			return(EINVAL);
		}
		name = (struct sockaddr *)&bind_ux;
		namelen = sizeof (bind_ux);
	} else {
		namelen = MIN(namelen, siptr->udata.addrsize);
	}

	buf = siptr->ctlbuf;
	/* LINTED pointer alignment */
	bind_req = (struct T_bind_req *)buf;
	size = sizeof (*bind_req);

	if (buf != (char *)NULL && (siptr->ctlsize >= size)) {
		bind_req->PRIM_type = T_BIND_REQ;
		bind_req->ADDR_length = name == NULL ? 0 : namelen;
		bind_req->ADDR_offset = 0;
		bind_req->CONIND_number = 0;
	} else {
		return(EFAULT);
	}

	if ((int)bind_req->ADDR_length > 0) {
		osoc_aligned_copy(buf, bind_req->ADDR_length, size,
				(caddr_t)name,
				(int *)&bind_req->ADDR_offset);
		size = bind_req->ADDR_offset + bind_req->ADDR_length;
	} 

	if (siptr->ctlsize < (size + bind_req->ADDR_length)) {
		return(EFAULT);
	}
	rval = osoc_do_ioctl(so, buf, size, TI_BIND, NULL);
	if (rval)
		return (rval);

	/* LINTED pointer alignment */
	bind_ack = (struct T_bind_ack *)buf;
	buf += bind_ack->ADDR_offset;
	size -= bind_ack->ADDR_offset;

	/*
	 * Check that the address returned by the
	 * transport provider meets the criteria.
	 */
	rval = 0;

	if (siptr->family == OAF_INET) {
		if (name != (struct sockaddr *)NULL) {
			struct sockaddr_in	*rname;
			struct sockaddr_in	*aname;
		
			/* LINTED pointer alignment */
			rname = (struct sockaddr_in *)buf;
			aname = (struct sockaddr_in *)name;
		
			if (aname->sin_port != 0 &&
			    aname->sin_port != rname->sin_port)
				rval = EADDRINUSE;
		
			if (aname->sin_addr.s_addr != INADDR_ANY &&
			    aname->sin_addr.s_addr != rname->sin_addr.s_addr)
				rval = EADDRNOTAVAIL;
		}

		if (rval) {
			osoc_dounbind(so);
			return (rval);
		}
	}

	/*
	 * Copy back the bound address if requested.
	 */
	if (raddr != NULL) {
		if (KADDR(raddr))
		{
			/* Kernel user, don't convert to an old sockaddr.*/
			bcopy(buf,raddr,bind_ack->ADDR_length);
			*raddrlen = size;
		} else {
			/* Convert the sockaddr back to an old sockaddr */
			SOCKADDR_TO_OLD_SOCKADDR(buf);
			rval = osoc_cpaddr(raddr, *raddrlen,
					   buf, bind_ack->ADDR_length, &size );
			if (!rval && (raddrlen != NULL))
				copyout((caddr_t)&size, (caddr_t)raddrlen, 
					sizeof(size));
			else if (raddrlen != NULL) {
				rval = 0;
				copyout((caddr_t)&rval, (caddr_t)raddrlen, 
					sizeof(rval));
			}
		}
	}

	siptr->udata.so_state |= SS_ISBOUND;

	return (0);
}

/*
 * STATIC int osoc_dounbind(struct osocket *so)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_dounbind(struct osocket *so)
{
	struct _si_user	*siptr;
	int	rval;

	siptr = &so->so_user;

	/* LINTED pointer alignment */
	((struct T_unbind_req *)siptr->ctlbuf)->PRIM_type = T_UNBIND_REQ;

	rval = osoc_do_ioctl(so, siptr->ctlbuf,
				sizeof (struct T_unbind_req),
					TI_UNBIND, NULL);
	if (rval)
		return (rval);

	siptr->udata.so_state &= ~SS_ISBOUND;
	return (0);
}


/* We make the socket module do the unbind,
 * if necessary, to make the timing window
 * of error as small as possible.
 */
struct listena {
	int	s;
	int    qlen;
};

/*
 * STATIC int osoc_listen(struct listena *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_listen(struct listena *uap, int *rvalp)
{
	struct osocket 		*so;
	char			*buf;
	struct T_bind_req	*bind_req;
	int			size;
	struct _si_user		*siptr;
	int			rval;


	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	siptr = &so->so_user;

	if (siptr->udata.servtype == T_CLTS)
		return (EOPNOTSUPP);

	if (siptr->family != OAF_INET)
		return (EINVAL);

	buf = siptr->ctlbuf;
	/* LINTED pointer alignment */
	bind_req = (struct T_bind_req *)buf;
	size = sizeof (struct T_bind_req);

	if (buf != (char *)NULL && (siptr->ctlsize >= size)) {
		bind_req->PRIM_type = T_BIND_REQ;
		bind_req->ADDR_offset = sizeof (*bind_req);
		bind_req->CONIND_number = uap->qlen;
	}

	if ((siptr->udata.so_state & SS_ISBOUND) == 0) {
		int	family;

		family = siptr->family;

		bcopy((caddr_t)&family, buf + bind_req->ADDR_offset,
				sizeof (short));
		bind_req->ADDR_length = 0;
	} else	bind_req->ADDR_length = siptr->udata.addrsize;

	rval = osoc_do_ioctl(so, siptr->ctlbuf, sizeof (*bind_req) +
				bind_req->ADDR_length, SI_LISTEN, NULL);

	if (rval)
		return (rval);

	siptr->udata.so_options |= OSO_ACCEPTCONN;
	return (0);
}

struct accepta {
	int             s;
	caddr_t         addr;
	int            *addrlen;
};

/*
 * STATIC int osoc_accept(struct accepta *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_accept(struct accepta *uap, int *rvalp)
{
	int             namelen;
	struct _si_user	*siptr;
	struct osocket *so;
	int		rval;

	namelen = 0;
	if (uap->addr && uap->addrlen) {
		if (copyin((caddr_t) uap->addrlen,
			   (caddr_t) &namelen, sizeof(namelen)))
			return(EFAULT);
	}

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	siptr = &so->so_user;
	if (siptr->udata.servtype == T_CLTS)
		return (EOPNOTSUPP);

	/*
	 * Make sure a listen() has been done
	 * actually if the accept() has not been done, then the
	 * effect will be that the user blocks forever.
	 */
	if ((siptr->udata.so_options & OSO_ACCEPTCONN) == 0)
		return (EINVAL);

	/* LINTED pointer alignment */
	rval = osoc_doaccept(so,  (struct osockaddr *)uap->addr, namelen, 
			uap->addrlen, rvalp);
	return (rval);
} 

/*
 * STATIC int osoc_doaccept(struct osocket *so, struct osockaddr *addr,
 *	int len, int *addrlen, int *rvalp)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_doaccept(struct osocket *so, struct osockaddr *addr, int len, 
		int *addrlen, int *rvalp)
{
	struct _si_user		*siptr;
	struct _si_user		*nsiptr;
	struct T_conn_res	*cres;
	int			s2;
	union T_primitives	*pptr;
	struct strfdinsert	strfdinsert;
	int			flg;
	struct strbuf		ctlbuf;
	int			nsys_fd;
	struct file 		*sys_fp;
	struct file 		*nsys_fp;
	int			retval;
	int			rval;
	int			size;
	rval_t			rv;
	int			domain;
	int			type;
	int			proto;
	int			nfd;
	struct osocket		*nso;

	flg = 0;
	siptr = &so->so_user;
	sys_fp = so->so_sfp;

	/*
	 * Get/wait for the T_CONN_IND.
	 */
	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;

	/*
	 * Get message from Sockmod 
	 * We don't expect any data, so no data
	 * buffer is needed.
	 */
	rval = osoc_getmsg(sys_fp, &ctlbuf, NULL, &flg, &rv);
	if (rval) {
		if (rval == EAGAIN)
			rval = EWOULDBLOCK;
		return (rval);
	}
	/*
	 * did I get entire message?
	 */
	if (rv.r_val1)
		return (EIO);

	/*
	 * is ctl part large enough to determine type
	 */
	if (ctlbuf.len < sizeof (long))
		return (EPROTO);

	/* LINTED pointer alignment */
	pptr = (union T_primitives *)ctlbuf.buf;
	switch (pptr->type) {
		case T_CONN_IND:
			if (ctlbuf.len < (sizeof (struct T_conn_ind)+
				pptr->conn_ind.SRC_length)) {
				return (EPROTO);
			}
			if (addr && addrlen) {
				char *tmp;
				/* Convert the sockaddr back to an old sockaddr */
				tmp = (char *)(ctlbuf.buf +
					pptr->conn_ind.SRC_offset);
				SOCKADDR_TO_OLD_SOCKADDR(tmp);
				rval = osoc_cpaddr((char *)addr, len,
					ctlbuf.buf + pptr->conn_ind.SRC_offset,
					pptr->conn_ind.SRC_length, &size);

				if (rval == -1)
					return (EFAULT);
				copyout((caddr_t)&size, (caddr_t)addrlen, sizeof(size));
			}
			break;

		default:
			return(EPROTO);
	}

	/*
	 * Open a new instance to do the accept on
	 */
	domain = so->so_user.family;
	type = so->so_proto.pr_type;
	proto = so->so_proto.pr_protocol;

	rval = osoc_sockopen(domain, type, proto, (int *)&rv);
	if (rval)
		return(rval);
	nfd = rv.r_val1;
	nso = NULL;
	rval = osoc_getsocket_with_fd(&nso, nfd);
	if (rval) 
		return(rval);
	if (nso->so_proto.pr_device != so->so_proto.pr_device) {
		osoc_doclose(nso);
		return(EINVAL);
	}
	nsiptr = &nso->so_user;
	s2 = nsiptr->fd;
	nsys_fd = nso->so_sfd;
	nsys_fp = nso->so_sfp;

	/*
	 * must be bound for TLI.
	 */
	rval = osoc_dobind(nso, NULL, 0, NULL, NULL);
	if (rval) {
		osoc_doclose(nso);
		return (rval);
	}
	/* LINTED pointer alignment */
	cres = (struct T_conn_res *)siptr->ctlbuf;
	cres->PRIM_type = T_CONN_RES;
	cres->OPT_length = 0;
	cres->OPT_offset = 0;
	cres->SEQ_number = pptr->conn_ind.SEQ_number;

	strfdinsert.ctlbuf.maxlen = siptr->ctlsize;
	strfdinsert.ctlbuf.len = sizeof (*cres);
	strfdinsert.ctlbuf.buf = (caddr_t)cres;

	strfdinsert.databuf.maxlen = 0;
	strfdinsert.databuf.len = -1;
	strfdinsert.databuf.buf = NULL;

	strfdinsert.fildes = nsys_fd;
	strfdinsert.offset = sizeof (long);
	strfdinsert.flags = 0;

	rval = strioctl(so->so_svp, I_FDINSERT, (int)&strfdinsert, 
			sys_fp->f_flag, K_TO_K, u.u_lwpp->l_cred, &retval);

	/* Blow away the parallel file-des to sockmod/Transport Provider */
	setf(nsys_fd, NULLFP);
	nso->so_sfd = 0;
	if (rval) {
		osoc_doclose(nso);
		return (rval);
	}
	if (!osoc_is_ok(so, T_CONN_RES, &rval)) {
		osoc_doclose(nso);
		return (rval);
	}

	/*
	 * New socket must have attributes of the
	 * accepting socket.
	 */
	nsiptr->udata.so_state |= OSS_ISCONNECTED;
	nsiptr->udata.so_options = siptr->udata.so_options & ~OSO_ACCEPTCONN;

	/*
	 * The accepted socket inherits the non-blocking and SIGIO
	 * attributes of the accepting socket.
	 */
	rval = osoc_do_fcntl(sys_fp, F_GETFL, 0, &rv);
	if (rval) {
		/*
		 *+ osoc_doaccept: fcntl: F_GETFL failed
		 */
		cmn_err(CE_WARN,
			"osoc_doaccept: fcntl: F_GETFL failed %d\n", rval);
		rval = 0;
	} else	{
		flg = rv.r_val1;
		flg &= (FREAD|FWRITE|FASYNC|FNDELAY);
		osoc_do_fcntl(nsys_fp, F_SETFL, flg, &rv);
	}
	*rvalp = s2;
	return (rval);
}

struct connecta {
	int             s;
	caddr_t         name;
	int             namelen;
};

/*
 * STATIC int osoc_connect(struct connecta *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_connect(struct connecta *uap, int *rvalp)
{
	struct osocket 		*so;
	int			rval;

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	rval = osoc_getargs((caddr_t)&so->so_addr, uap->name, uap->namelen);

	/* Convert the sockaddr just read in to new_sockaddrs */
	OLD_SOCKADDR_TO_SOCKADDR(&(so->so_addr), uap->namelen);

	if (rval)
		return(rval);

	rval = osoc_doconnect1(so, uap->namelen, 1);

	return(rval);

}


/*
 * STATIC int osoc_doconnect1(struct osocket *so, int namelen, int nameflag)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_doconnect1(struct osocket *so, int namelen, int nameflag)
{
	struct _si_user			*siptr;
	struct sockaddr			*name;
	struct t_call			sndcall;
	struct t_call			*call;
	struct sockaddr_in 		*saddr_in;

	siptr = &so->so_user;
	name = (struct sockaddr *)&so->so_addr;
	call = &sndcall;

	bzero((caddr_t)call, sizeof (*call));

	if (name->sa_family != AF_INET)
		return(EINVAL);

	if (namelen < sizeof (struct sockaddr_in))
		return (EINVAL);
	saddr_in = (struct sockaddr_in *)name;
	bzero((caddr_t)&saddr_in->sin_zero, 8);

	call->addr.buf = (caddr_t)name;
	call->addr.len = MIN(namelen, siptr->udata.addrsize);

	return(osoc_doconnect2(so, call));

}

/*
 * STATIC int osoc_doconnect2(struct osocket *so, struct t_call *call)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_doconnect2(struct osocket *so, struct t_call *call)
{
	struct _si_user			*siptr;
	int				fctlflg;
	struct file			*sys_fp;
	rval_t				rv;
	int				rval;

	siptr = &so->so_user;
	sys_fp = so->so_sfp;

	rval = osoc_do_fcntl(sys_fp, F_GETFL, 0, &rv);
	if (rval)
		return(rval);

	fctlflg = rv.r_val1;

	if (fctlflg & O_NDELAY && siptr->udata.servtype != T_CLTS) {
		/*
		 * Secretly tell sockmod not to pass
		 * up the T_CONN_CON, because we
		 * are not going to wait for it.
		 * (But dont tell anyone - especially
		 * the transport provider).
		 */
		call->opt.len = (ulong)-1;	/* secret sign */
	}

	/*
	 * Must be bound for TPI.
	 */
	if ((siptr->udata.so_state & SS_ISBOUND) == 0) {
		rval = osoc_dobind(so, NULL, 0, NULL, NULL);
		if (rval)
			return (rval);

	}

	rval = osoc_snd_conn_req(so, call);
	if (rval)
		return (rval);

	/*
	 * If no delay, return with error if not CLTS.
	 */
	if (fctlflg & O_NDELAY && siptr->udata.servtype != T_CLTS) {
		siptr->udata.so_state |= SS_ISCONNECTING;
		return (EINPROGRESS);
	}

	/*
	 * If CLTS, don't get the connection confirm.
	 */
	if (siptr->udata.servtype == T_CLTS) {
		if (call->addr.len == 0)
			/*
			 * Connect to Null address, breaks
			 * the connection.
			 */
			siptr->udata.so_state &= ~OSS_ISCONNECTED;
		else	siptr->udata.so_state |= OSS_ISCONNECTED;
		return (0);
	}
	rval = osoc_rcv_conn_con(so);
	if (rval)
		return (rval);
	siptr->udata.so_state |= OSS_ISCONNECTED;
	return (0);
}

/*
 * STATIC int osoc_snd_conn_req(struct osocket *so, struct t_call *call)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_snd_conn_req(struct osocket *so, struct t_call *call)
{
	struct _si_user		*siptr;
	struct file		*sys_fp;
	struct T_conn_req	*creq;
	char			*buf;
	int			size;
	struct strbuf		ctlbuf;
	int			rval;
	rval_t			rv;

	siptr = &so->so_user;
	sys_fp = so->so_sfp;

	buf = siptr->ctlbuf;
	/* LINTED pointer alignment */
	creq = (struct T_conn_req *)buf;
	creq->PRIM_type = T_CONN_REQ;
	creq->DEST_length = call->addr.len;
	creq->DEST_offset = 0;
	creq->OPT_length = call->opt.len;
	creq->OPT_offset = 0;
	size = sizeof (struct T_conn_req);

	if ((int)call->addr.len > 0 && buf != (char *)NULL) {
		osoc_aligned_copy(buf, call->addr.len, size,
			call->addr.buf, (int *)&creq->DEST_offset);
		size = creq->DEST_offset + creq->DEST_length;
	}
	if ((int)call->opt.len > 0 && buf != (char *)NULL) {
		osoc_aligned_copy(buf, call->opt.len, size,
			call->opt.buf, (int *)&creq->OPT_offset);
		size = creq->OPT_offset + creq->OPT_length;
	}

	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = size;
	ctlbuf.buf = buf;

	rval = osoc_putmsg(sys_fp, &ctlbuf, (call->udata.len? 
		(struct strbuf *)&call->udata: (struct strbuf *)NULL), 0, &rv);
	if (rval)
		return (rval);

	if (!osoc_is_ok(so, T_CONN_REQ, &rval))
		return (rval);

	return (0);
}

/*
 * STATIC int osoc_rcv_conn_con(struct osocket *so)
 * 	Rcv_conn_con - get connection confirmation off
 * 	of read queue
 *
 * Calling/Exit State:
 */
STATIC int
osoc_rcv_conn_con(struct osocket *so)
{
	struct _si_user		*siptr;
	struct file		*sys_fp;
	struct strbuf		ctlbuf;
	struct strbuf		databuf;
	union T_primitives	*pptr;
	int			rval;
	rval_t			rv;
	int			flg;
	char			dbuf[128];

	siptr = &so->so_user;
	sys_fp = so->so_sfp;

	flg = 0;
	if (siptr->udata.servtype == T_CLTS)
		return (EOPNOTSUPP);

	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;

	databuf.maxlen = sizeof (dbuf);
	databuf.len = 0;
	databuf.buf = dbuf;

	/*
	 * No data expected, but we play safe.
	 */
	rv.r_val1 = 0;
	rval = osoc_getmsg(sys_fp, &ctlbuf, &databuf, &flg, &rv);
	if (rval) {
		if (rval == ENXIO)
			rval = ECONNREFUSED;
		return (rval);
	}

	/*
	 * did we get entire message
	 */
	if (rv.r_val1)
		return (EIO);

	/*
	 * is cntl part large enough to determine message type?
	 */
	if (ctlbuf.len < sizeof (long))
		return (EPROTO);

	/* LINTED pointer alignment */
	pptr = (union T_primitives *)ctlbuf.buf;
	switch (pptr->type) {
		case T_CONN_CON:
			return (0);

		case T_DISCON_IND:
			if (ctlbuf.len < sizeof (struct T_discon_ind))
				rval = ECONNREFUSED;
			else	rval = pptr->discon_ind.DISCON_reason;
			return (rval);

		default:
			break;
	}

	return (EPROTO);
}

struct recva {
	int             s;
	caddr_t         buf;
	int             len;
	int             flags;
};

/*
 * STATIC int osoc_recv(struct recva *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_recv(struct recva *uap, int *rvalp)
{
	struct osocket 		*so;
	struct msghdr		msg;
	struct iovec		msg_iov[1];
	int			rval;

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	msg.msg_iovlen = 1;
	msg.msg_iov = msg_iov;
	msg.msg_iov[0].iov_base = uap->buf;
	msg.msg_iov[0].iov_len = uap->len;
	msg.msg_namelen = 0;
	msg.msg_name = NULL;
	msg.msg_accrightslen = 0;
	msg.msg_accrights = NULL;

	rval = osoc_soreceive(so, &msg, uap->flags, rvalp);
	return(rval);
}

struct recvfa {
	int             s;
	caddr_t         buf;
	int             len;
	int             flags;
	caddr_t         from;
	int            *fromlen;
};

/*
 * STATIC int osoc_recvfrom(struct recvfa *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_recvfrom(struct recvfa *uap, int *rvalp)
{
	struct socket *so;
	int		flen;
	struct iovec	msg_iov[1];
	struct msghdr	msg;
	int		rval;

	rval = osoc_getsocket_with_fd((struct osocket **)&so, uap->s);
	if (rval)
		return(rval);

	flen = 0;
	if (uap->from && uap->fromlen && 
	    copyin((caddr_t) uap->fromlen, (caddr_t) &flen,
		   sizeof(flen)))
		return(EFAULT);

	msg.msg_iovlen = 1;
	msg.msg_iov = msg_iov;
	msg.msg_iov[0].iov_base = uap->buf;
	msg.msg_iov[0].iov_len = uap->len;
	msg.msg_namelen = flen;
	msg.msg_name = uap->from;
	msg.msg_accrightslen = 0;
	msg.msg_accrights = NULL;

	rval = osoc_soreceive((struct osocket *)so, &msg, uap->flags, rvalp);

	if (!rval && uap->fromlen) {
		flen = msg.msg_namelen;
		rval = copyout((caddr_t)&flen, (caddr_t)uap->fromlen,
	    		       sizeof(int));
		if (rval)
			rval = EFAULT;
	}

	return(rval);
}

struct senda {
	int             s;
	caddr_t         buf;
	int             len;
	int             flags;
};

/*
 * STATIC int osoc_send(struct senda *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_send(struct senda *uap, int *rvalp)
{
	struct osocket 		*so;
	struct msghdr		msg;
	struct iovec		msg_iov[1];
	int			rval;

	if ((uap->len <= 0) || (uap->buf == NULL))
		return(0);

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	msg.msg_iovlen = 1;
	msg.msg_iov = msg_iov;
	msg.msg_iov[0].iov_base = uap->buf;
	msg.msg_iov[0].iov_len = uap->len;
	msg.msg_namelen = 0;
	msg.msg_name = NULL;
	msg.msg_accrightslen = 0;
	msg.msg_accrights = NULL;

	return (osoc_sosend(so, &msg, uap->flags, rvalp));
}

struct sendfa {
	int             s;
	caddr_t         buf;
	int             len;
	int             flags;
	caddr_t         to;
	int             tolen;
};

/*
 * STATIC int osoc_sendto(struct sendfa *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_sendto(struct sendfa *uap, int *rvalp)
{

	struct socket *so;
	struct msghdr	msg;
	struct iovec	msg_iov[1];
	int		rval;

	rval = osoc_getsocket_with_fd((struct osocket **)&so, uap->s);
	if (rval)
		return(rval);

	msg.msg_iovlen = 1;
	msg.msg_iov = msg_iov;
	msg.msg_iov[0].iov_base = uap->buf;
	msg.msg_iov[0].iov_len = uap->len;
	msg.msg_namelen = uap->tolen;
	msg.msg_name = uap->to;
	msg.msg_accrightslen = 0;
	msg.msg_accrights = NULL;

	return (osoc_sosend((struct osocket *)so, &msg, uap->flags, rvalp));

}

/*
 * Get name of peer for connected socket. 
 */
struct getpeera {
	int             fdes;
	caddr_t         name;
	int            *namelen;
};

/*
 * STATIC int osoc_getpeername(struct getpeera *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_getpeername(struct getpeera *uap, int *rvalp)
{
	struct socket		*so;
	int			len;
	int			rval;
	int			sname;

	rval = osoc_getsocket_with_fd((struct osocket **)&so, uap->fdes);
	if (rval)
		return(rval);

	if (uap->name == NULL || uap->namelen == NULL)
		return (EINVAL);

	if (copyin((caddr_t) uap->namelen, (caddr_t) &len, sizeof(len)))
		return(EFAULT);

	if (len > sizeof(struct osockaddr))
		len = sizeof(struct osockaddr);

	return(osoc_dogetname((struct osocket *)so,  (struct osockaddr *)uap->name,
			      uap->namelen, len, REMOTE_ADDR));

}

/*
 * Get socket name. 
 */
struct getsocka {
	int			s;
	struct osockaddr	*name;
	int			*namelen;
};

/*
 * STATIC int osoc_getsockname(struct getsocka *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_getsockname(struct getsocka *uap, int *rvalp)
{
	struct osocket	*so;
	int             len;
	int		rval;

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	if (uap->name == NULL || uap->namelen == NULL)
		return (EINVAL);

	if (copyin((caddr_t) uap->namelen, (caddr_t) (&len), sizeof(len))) {
		return(EFAULT);
	}

	if (len > sizeof(struct osockaddr))
		len = sizeof(struct osockaddr);

	return (osoc_dogetname(so, uap->name, uap->namelen, len, LOCAL_ADDR));
}

/*
 * STATIC int osoc_dogetname(struct osocket *so, struct osockaddr *name,
 *				int *namelen, int len)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_dogetname(struct osocket *so, struct osockaddr *name, int *namelen, int len, int who)
{
	struct _si_user		*siptr;
	struct strbuf		ctlbuf;
	struct T_addr_req	areq;
	struct T_addr_ack	*aackp;
	char			*addr;
	struct file		*sys_fp;
	int			rval,count,addrlen,flg;
	rval_t			rv;

	flg = 0;
	siptr = &so->so_user;
	sys_fp = so->so_sfp;

	areq.PRIM_type = T_ADDR_REQ;
	ctlbuf.len = sizeof(areq);
	ctlbuf.buf = (caddr_t)&areq;

	if (rval = osoc_putmsg(sys_fp, &ctlbuf, NULL, 0, &rv))
	{
		return(rval);
	}

	/*
	 * Get/wait for the T_ADDR_ACK.
	 */
	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;

	if (rval = osoc_getmsg(sys_fp, &ctlbuf, NULL, &flg, &rv))
	{
		if (rval == EAGAIN)
			rval = EWOULDBLOCK;
		return (rval);
	}

	if (ctlbuf.len < sizeof(struct T_addr_ack)) {
		return EIO;
	}

	/* LINTED pointer alignment */
	aackp = (struct T_addr_ack *)ctlbuf.buf;
	if (aackp->PRIM_type != T_ADDR_ACK) {
		return EINVAL;
	}

	switch (who) {
	case LOCAL_ADDR:
		addr = (char *)aackp + aackp->LOCADDR_offset;
		addrlen = (int)aackp->LOCADDR_length;
		break;
	case REMOTE_ADDR:
		addr = (char *)aackp + aackp->REMADDR_offset;
		addrlen = (int)aackp->REMADDR_length;
		break;
	default:
		return EPROTO;
	}
	/* Convert the sockaddr back to an old sockaddr */
	SOCKADDR_TO_OLD_SOCKADDR((struct sockaddr *)addr);
	
	count = addrlen;
	rval = osoc_cpaddr((char *)name, len, addr, addrlen, &count);

	if (!rval)
		copyout((caddr_t)&addrlen, (caddr_t)namelen, sizeof(int));

	return (0);
}

struct getsockopta {
	int	s;
	int	level;
	int	optname;
	caddr_t	optval;
	int	*optlen;
};

/*
 * STATIC int osoc_getsockopt(struct getsockopta *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_getsockopt(struct getsockopta *uap, int *rvalp)
{
	struct osocket  *so;

	int			sys_optlen;
	int			sys_optval;
	char			*buf;
	struct T_optmgmt_req	*opt_req;
	struct T_optmgmt_ack	*opt_ack;
	struct _si_user		*siptr;
	int			size;
	struct opthdr		*opt;
	int			retlen;
	int			rval;


	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	siptr = &so->so_user;
	sys_optlen = 0;
	if (uap->optval) {
		if (uap->optlen) {
			rval = copyin((caddr_t) uap->optlen, 
				      (caddr_t)&sys_optlen, sizeof(sys_optlen));
		} else
			rval++;

		if (rval)
			return(EFAULT);
	}

	if (uap->level == OSOL_SOCKET && uap->optname == OSO_TYPE) {
		if (sys_optlen < sizeof (int))
			return (EINVAL);

		if (siptr->udata.servtype == T_CLTS)
			sys_optval = SOCK_DGRAM;
		else	sys_optval = SOCK_STREAM;

		sys_optlen = sizeof (int);

		if (uap->optval)
			rval = copyout(uap->optval, (caddr_t)sys_optval, 
				       sizeof(int));
		if (!rval)
			rval = copyout((caddr_t)uap->optlen, 
				       (caddr_t)sys_optlen, sizeof(int));
		if (rval)
			return(EFAULT);

		return (0);
	}

	buf = siptr->ctlbuf;
	/* LINTED pointer alignment */
	opt_req = (struct T_optmgmt_req *)buf;
	opt_req->PRIM_type = T_OPTMGMT_REQ;
	opt_req->OPT_length = sizeof (*opt) + sys_optlen;
	opt_req->OPT_offset = sizeof (*opt_req);
	opt_req->MGMT_flags = T_CHECK;
	size = sizeof (*opt_req) + opt_req->OPT_length;

	if (size > siptr->ctlsize)
		return(EFAULT);

	/* LINTED pointer alignment */
	opt = (struct opthdr *)(buf + opt_req->OPT_offset);
	opt->level = uap->level;
	opt->name = uap->optname;
	opt->len = sys_optlen;

	rval = osoc_do_ioctl(so, buf, size, TI_OPTMGMT, &retlen);
	if (rval)
		return(rval);

	if (retlen < (sizeof (*opt_ack) + sizeof (*opt)))
		return(EPROTO);

	/* LINTED pointer alignment */
	opt_ack = (struct T_optmgmt_ack *)buf;
	/* LINTED pointer alignment */
	opt = (struct opthdr *)(buf + opt_ack->OPT_offset);

	sys_optlen = opt->len;
	rval = copyout((caddr_t)opt + sizeof (*opt), uap->optval, opt->len);
	if (!rval && uap->optlen)
		rval = copyout((caddr_t)&sys_optlen, (caddr_t)uap->optlen, 
				sizeof(sys_optlen));
	if (rval)
		return (EFAULT);

	return (0);
}

/*
 * STATIC int osoc_dosetsockopt(struct osocket *so, int level, 
 *		int optname, char *optval, int optlen, int copyflag)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_dosetsockopt(struct osocket *so, int level, int optname, char *optval, 
		int optlen, int copyflag)
{
	struct _si_user		*siptr;
	char			*buf;
	struct T_optmgmt_req	*opt_req;
	int			size;
	struct opthdr		*opt;
	int			rval;
	int			totsize;

	rval = 0;
	siptr = &so->so_user;
	buf = siptr->ctlbuf;
	totsize = sizeof (*opt_req) + sizeof (*opt) + optlen;
	
	if (buf && (siptr->ctlsize >= totsize)) {
		/* LINTED pointer alignment */
		opt_req = (struct T_optmgmt_req *)buf;
		opt_req->PRIM_type = T_OPTMGMT_REQ;
		opt_req->OPT_length = sizeof (*opt) + optlen;
		opt_req->OPT_offset = sizeof (*opt_req);
		opt_req->MGMT_flags = T_NEGOTIATE;
	
		/* LINTED pointer alignment */
		opt = (struct opthdr *)(buf + sizeof (*opt_req));
		opt->level = level;
		opt->name = optname;
		opt->len = optlen;
		if (copyflag == U_TO_K)
			rval = copyin(optval, (caddr_t)opt + sizeof (*opt), 
				      optlen);
		else
			(void)bcopy(optval, (caddr_t)opt + sizeof (*opt), 
				    optlen);
		if (rval)
			return(rval);
	
		size = opt_req->OPT_offset + opt_req->OPT_length;
	
		rval = osoc_do_ioctl(so, buf, size, TI_OPTMGMT, 0);
	} else {
		rval = EFAULT;
	}
	return (rval);
}

struct setsockopta {
	int             s;
	int             level;
	int             optname;
	caddr_t         optval;
	int             optlen;
};

/*
 * STATIC int osoc_setsockopt(struct setsockopta *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_setsockopt(struct setsockopta *uap, int *rvalp)
{
	struct osocket  *so;
	int		rval;

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);

	rval = osoc_dosetsockopt(so, uap->level, uap->optname, uap->optval, 
			  uap->optlen, U_TO_K);
	return(rval);
}


struct shutdowna {
	int             s;
	int             how;
};

/*
 * STATIC int osoc_shutdown(struct shutdowna *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_shutdown(struct shutdowna *uap, int *rvalp)
{
	struct osocket  	*so;
	struct   _si_user	*siptr;
	int			rval;
	int			sys_how;

	rval = osoc_getsocket_with_fd(&so, uap->s);
	if (rval)
		return(rval);
	
	siptr = &so->so_user;

	sys_how = uap->how;
	if (sys_how < 0 || sys_how > 2)
		return (EINVAL);

	if ((siptr->udata.so_state & SS_ISCONNECTED) == 0) {
		rval = osoc_getudata(so, 0);
		if (rval)
			return (rval);
		if ((siptr->udata.so_state & SS_ISCONNECTED) == 0)
			return (ENOTCONN);
	}

	sys_how = uap->how;
	rval = osoc_do_ioctl(so, (char *)&sys_how, sizeof (sys_how), SI_SHUTDOWN, 0);
	if (rval) {
		if (rval != EPIPE)
			return (rval);
		else	
			rval= 0;
	}

	/*
	 * If we got EPIPE back from the ioctl, then we can
	 * no longer talk to sockmod. The best we can do now
	 * is set our local state and hope the user doesn't
	 * use read/write.
	 */
	if (sys_how == 0 || sys_how == 2)
		siptr->udata.so_state |= SS_CANTRCVMORE;
	if (sys_how == 1 || sys_how == 2)
		siptr->udata.so_state |= SS_CANTSENDMORE;

	return (0);
}

struct adjtimea {
	struct timeval *delta;
	struct timeval *olddelta;
};

/*
 * STATIC int osoc_adjtime(struct adjtimea *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_adjtime(struct adjtimea *uap, int *rvalp)
{
	int	rval;

	rval = osoc_doadjtime(uap->delta, uap->olddelta);
	return(rval);
}

struct setipdoma {
	caddr_t	namep;
	int	size;
};

/*
 * STATIC int osoc_setipdomain(struct setipdoma *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_setipdomain(struct setipdoma *uap, int *rvalp)
{
	if (pm_denied(u.u_lwpp->l_cred, P_SYSOPS))
		return(EPERM);

	/* Reserve one byte for the NULL termination */
	if (uap->size < 1 || uap->size > (OMAXHOSTNAMELEN - 1))
		return (EINVAL);

	if (copyin(uap->namep, osoc_domainbuf, uap->size))
		return(EFAULT);
	osoc_domainbuf[uap->size] = 0;

	return(0);
}

struct getipdoma {
	caddr_t	namep;
	int	size;
};

/*
 * STATIC int osoc_getipdomain(struct getipdoma *uap, int *rvalp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_getipdomain(struct getipdoma *uap, int *rvalp)
{
	int 		domainlen;

	domainlen = strlen(osoc_domainbuf) + 1;

	if (domainlen > uap->size)
		return (EFAULT);
	if (copyout(osoc_domainbuf, uap->namep, domainlen))
		return(EFAULT);
	return(0);
}

struct setreuida {
	uid_t	ruid;
	uid_t	euid;
};

STATIC int
osoc_setreuid(struct setreuida *uap, int *rvalp)
{
	cred_t	*credp = u.u_lwpp->l_cred;
	cred_t	*newcredp;
	uid_t	ruid, euid;


	/*
	 * in an SCO binary, uid is a unsigned short, so the -1
	 * bit value is only 16 bits. Also since it is unsigned,
	 * UIDs can be > 2^15 so we don't check for negative vals
	 */
	if (uap->ruid == (unsigned short) -1)
		ruid = credp->cr_ruid;
	else if ((ruid = uap->ruid) > (unsigned short)MAXUID)
		return EINVAL;
	
	if (uap->euid == (unsigned short)-1)
		euid = credp->cr_uid;
	else if ((euid = uap->euid) > (unsigned short)MAXUID)
		return EINVAL;

	if (ruid != credp->cr_ruid && pm_denied(credp, P_SETUID))
		return(EPERM);

	if (euid != credp->cr_ruid &&
	    euid != credp->cr_uid &&
	    euid != credp->cr_suid &&
	    pm_denied(credp, P_SETUID))
		return EPERM;
	/*
	 * Permission checks pass.
	 * Perform the operation only if we're really changing something.
	 * XXX - Note that we are not changing credp->cr_suid here.
	 */

	if (ruid != credp->cr_ruid || euid != credp->cr_uid) {
		newcredp = crdup2(credp);
		newcredp->cr_ruid = ruid;
		newcredp->cr_uid = euid;
		pm_recalc(newcredp);	/* MUST be done before crinstall() */
		crinstall(newcredp);
	}

	return 0;
}

struct setregida {
	gid_t	rgid;
	gid_t	egid;
};

STATIC int osoc_setregid(struct setregida *uap, int *rvalp)
{
	gid_t rgid, egid;
	cred_t *credp = u.u_lwpp->l_cred;
	cred_t *newcredp;

	/*
	 * in an SCO binary, gid is a unsigned short, so the -1
	 * bit value is only 16 bits. Also since it is unsigned,
	 * GIDs can be > 2^15 so we don't check for negative vals
	 */
	if (uap->rgid == (unsigned short) -1)
		rgid = credp->cr_rgid;
	else if ((rgid = uap->rgid) > (unsigned short)MAXUID)
		return EINVAL;
	
	if (uap->egid == (unsigned short)-1)
		egid = credp->cr_gid;
	else if ((egid = uap->egid) > (unsigned short)MAXUID)
		return EINVAL;

	if (rgid != credp->cr_rgid && rgid != credp->cr_sgid &&
	    pm_denied(credp, P_SETUID))
		return EPERM;

	if (egid != credp->cr_rgid &&
	    egid != credp->cr_gid &&
	    egid != credp->cr_sgid &&
	    pm_denied(credp, P_SETUID))
		return EPERM;

	/*
	 * Permission checks pass.
	 * Perform the operation only if we're really changing something.
	 * XXX - Note that we are not changing credp->cr_sgid here.
	 */

	if (rgid != credp->cr_rgid || egid != credp->cr_gid) {
		newcredp = crdup2(credp);
		newcredp->cr_rgid = rgid;
		newcredp->cr_gid = egid;
		crinstall(newcredp);
	}

	return 0;
}

struct gettimea {
	struct timeval *tp;
};

STATIC int osoc_gettime(struct gettimea *uap, int *rvalp)
{
	struct timeval tv;
	timestruc_t atv;

	GET_HRESTIME(&atv)

	if (uap->tp == NULL)
		return (0);

	tv.tv_sec = atv.tv_sec;
	tv.tv_usec = atv.tv_nsec / (NANOSEC/MICROSEC);
	return(copyout((caddr_t)&tv, (caddr_t)uap->tp, sizeof(tv)));
}

struct settimea {
	struct timeval *tp;
};

STATIC int osoc_settime(struct settimea *uap, int *rvalp)
{
	struct timeval tv;
	timestruc_t atv;
	int error;

	if (uap->tp == NULL)
		return (0);

	if (error = pm_denied(u.u_lwpp->l_cred, P_SYSOPS)) {
		adt_stime(error, &atv);
		return (error);
	}

	if ((copyin((caddr_t)uap->tp, (caddr_t)&tv, sizeof(tv))) != 0) {
		error = EFAULT;
	} else {

		atv.tv_sec = tv.tv_sec;
		atv.tv_nsec = tv.tv_usec * (NANOSEC/MICROSEC);
	
		if (tv.tv_sec < 0 || tv.tv_usec < 0 || tv.tv_usec >= MICROSEC) {
			/*
			 * The time value is invalid.
			 */
			error = EINVAL;
		} else {
			settime(&atv);
			wtodc(&atv);
		}
	}
	return(error);
}

struct sendmsga {
	int	s;
	caddr_t	msg;
	int	flags;
};

STATIC int
osoc_sendmsg(struct sendmsga *uap, int *rvalp)
{
	struct socket	*so;
	struct msghdr	msg;
	int		rval;

	if (rval = osoc_getsocket_with_fd((struct osocket **)&so, uap->s))
		return(rval);

	if (copyin((caddr_t)uap->msg, (caddr_t)&msg, sizeof(struct msghdr)))
		return EFAULT;

	return (osoc_sosend((struct osocket *)so, &msg, uap->flags, rvalp));
}

struct sockpaira {
	int	sock1;
	int	sock2;
};

STATIC int
osoc_sockpair(struct sockpaira *uap, int *rvalp)
{
	struct osocket	*so, *nso;
	struct bind_ux	bind_ux, nbind_ux;
	struct t_call	sndcall;
	int		size, rval;

	
	if (rval = osoc_getsocket_with_fd(&so, uap->sock1))
		return(rval);

	if (rval = osoc_getsocket_with_fd(&nso, uap->sock2))
		return(rval);


	bzero((caddr_t)&nbind_ux, sizeof (nbind_ux));
	bzero((caddr_t)&bind_ux, sizeof (bind_ux));
	bzero((caddr_t)&sndcall, sizeof (sndcall));

	/* Both sockets are unbound at this stage, so bind them. */
	if (rval = osoc_dobind(so, NULL, 0, (char *)&bind_ux, &size))
		return(rval);

	if (size != sizeof(struct bind_ux))
		return EPROTO;

	if (rval = osoc_dobind(nso, NULL, 0, (char *)&nbind_ux, &size))
		return(rval);

	if (size != sizeof(struct bind_ux))
		return EPROTO;

	/*
	 * connect so->nso
	 */
	sndcall.addr.buf = (caddr_t)&nbind_ux;
	sndcall.addr.len = sizeof (nbind_ux);
	if (rval = osoc_doconnect2(so, &sndcall))
		return rval;
	/*
	 * connect nso->so
	 */
	sndcall.addr.buf = (caddr_t)&bind_ux;
	sndcall.addr.len = sizeof (bind_ux);
	if (rval = osoc_doconnect2(nso, &sndcall))
		return(rval);

	return (0);

}

/* 
 * Functions from libsocket/socket/_utility.c
 */

/*
 * STATIC int osoc_smodopen(struct osocket *so, int proto)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_smodopen(struct osocket *so, int proto)
{
	int	fd;
	int	rval;
	int	retval;
	struct  file *fp;
	struct	vnode *sys_vp;
	int	flags;
	int	arg;

	retval = 0;
	flags = FREAD|FWRITE;
	sys_vp = makespecvp(so->so_proto.pr_device, VCHR);

	if ((rval = VOP_OPEN(&sys_vp, flags, u.u_lwpp->l_cred)) != 0) {
		VN_RELE(sys_vp);
		return(rval);
	}

	if ((rval = falloc(sys_vp, flags, &fp, &fd)) != 0) {
		VOP_CLOSE(sys_vp, flags, 1, 0, u.u_lwpp->l_cred);
		return rval;
	}

	/* Must allocate a file pointer and a file descriptor	*/
	/* So that we can do an accept.				*/

	/* Push the Socket Module */

	rval = strioctl(sys_vp, I_PUSH, (int)"sockmod", 
			fp->f_flag, K_TO_K, u.u_lwpp->l_cred, &retval);
	if (rval) {
		closef(fp);
		setf(fd, NULLFP);
		return (rval);
	}

	if (retval) {
		if ((retval & 0xff) == TSYSERR)
			rval = (retval >> 8) & 0xff;
		else    
			rval = osoc_tlitosyserr(retval & 0xff);
		closef(fp);
		setf(fd, NULLFP);
		return (rval);
	}


	/* Set the stream head close time to 0 */
	/* Do not care about the return value from strioctl */

	arg = 0;
	strioctl(sys_vp, I_SETCLTIME, (int)&arg, fp->f_flag, K_TO_K, 
		u.u_lwpp->l_cred, &retval);

	/* Turn on SIGPIPE stream head write option */

	rval = strioctl(sys_vp, I_SWROPT, SNDPIPE, fp->f_flag, K_TO_K, 
			u.u_lwpp->l_cred, &retval);

	if (rval) {
		/*
		 *+ osoc_open: Cannot set SNDPIPE
		 */
		cmn_err(CE_WARN, 
		    "osoc_open: Cannot set SNDPIPE : %d", rval);
		closef(fp);
		setf(fd, NULLFP);
		return (rval);
	}

	if( retval ) {
		if ((retval & 0xff) == TSYSERR)
			rval = (retval >> 8) & 0xff;
		else    
			rval = osoc_tlitosyserr(retval & 0xff);
		closef(fp);
		setf(fd, NULLFP);
		return (rval);
	}

	so->so_sfd = fd;
	so->so_sfp = fp;
	so->so_svp = sys_vp;

	rval = osoc_getudata(so, 1);
	if (rval) {
		/* Freeup the sockets here */
		so->so_sfd = 0;
		so->so_sfp = NULL;
		so->so_svp = NULL;
		closef(fp);
		setf(fd, NULLFP);
		return (rval);
	}

	if (proto && (so->so_proto.pr_flags & OPR_BINDPROTO)) {
		/* Need to send down the protocol number */
		rval = osoc_dosetsockopt(so, SOL_SOCKET, SO_PROTOTYPE, 
				(char *)&proto, sizeof (proto), K_TO_K);
		if (rval) {
			/* Freeup the sockets here */
			so->so_sfd = 0;
			so->so_sfp = NULL;
			so->so_svp = NULL;
			closef(fp);
			setf(fd, NULLFP);
			return(rval);
		}
	}
	return(0);
}

/*
 * STATIC int osoc_getudata(struct osocket *so, int init)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_getudata(struct osocket *so, int init)
{
	struct si_udata	udata;
	struct _si_user	*nsiptr;
	int		retlen;
	int		retval;
	int		rval;
	int		arg;

	rval = osoc_do_ioctl(so, (caddr_t)&udata, sizeof (struct si_udata),
			SI_GETUDATA, &retlen);
	if (rval)
		return(rval);

	if (retlen != sizeof (struct si_udata)) {
		rval = EPROTO;
		return (rval);
	}

	nsiptr = &so->so_user;
	if (init) {
		nsiptr->ctlsize = sizeof(union T_primitives) + udata.addrsize
			+ sizeof (long) + udata.optsize + sizeof (long);
		nsiptr->ctlbuf = kmem_zalloc(nsiptr->ctlsize, KM_SLEEP);

		/* Init nsiptr->fd in osoc_create() */

		nsiptr->udata = udata;		/* structure copy */
		nsiptr->family = -1;
		nsiptr->flags = 0;

		/*
		 * Get SIGIO and SIGURG disposition
		 * and cache them.
		 */
		arg = 0;
		rval = strioctl(so->so_svp, I_GETSIG, (int)&arg, 0, K_TO_K, 
				u.u_lwpp->l_cred, &retval);

		/* Check for any registered events */
		/* If there are no registered events then rval == EINVAL */
		if (rval && (rval != EINVAL))
			return (rval);

		rval = 0;
		if (retval & (S_RDNORM|S_WRNORM))
			nsiptr->flags |= S_SIGIO;

		if (retval & (S_RDBAND|S_BANDURG))
			nsiptr->flags |= S_SIGURG;

		return (0);
	} else {
		nsiptr->udata = udata;		/* Structure Copy */
	}

	return (0);
}

/*
 * STATIC int 
 * osoc_do_ioctl(struct osocket *so, char *buf, int size, int cmd, int *retlen)
 * 	timod ioctl
 *
 * Calling/Exit State:
 */
STATIC int
osoc_do_ioctl(struct osocket *so, char *buf, int size, int cmd, int *retlen)
{
	int	retval;
	int	rval;
	struct strioctl		strioc;
	struct file	*sys_fp;
	struct vnode	*sys_vp;

	sys_fp = so->so_sfp;
	sys_vp = so->so_svp;

	strioc.ic_cmd = cmd;
	strioc.ic_timout = -1;
	strioc.ic_len = size;
	strioc.ic_dp = buf;

	rval = strioctl(sys_vp, I_STR, (int)&strioc, 
			sys_fp->f_flag, K_TO_K, u.u_lwpp->l_cred, &retval);
	if (rval) {
		/*
		 * Map the rval as appropriate.
		 */
		switch (rval) {
			case ENOTTY:
			case ENODEV:
			case EINVAL:
				rval = ENOTSOCK;
				break;

			case EBADF:
				break;

			case ENXIO:
				rval = EPIPE;

			default:
				break;
		}
		return (rval);
	}

	if (retval && cmd != SIOCGIFCONF) {
		if ((retval & 0xff) == TSYSERR)
			rval = (retval >>  8) & 0xff;
		else
			rval = osoc_tlitosyserr(retval & 0xff);
		return (rval);
	}
	if (retlen)
		*retlen = strioc.ic_len;
	return (0);
}

/*
 * STATIC int osoc_is_ok(struct osocket *so, long type, int *rvalp)
 * 	Wait for T_OK_ACK
 *
 * Calling/Exit State:
 */
STATIC int
osoc_is_ok(struct osocket *so, long type, int *rvalp)
{

	struct _si_user			*siptr;
	struct strbuf			ctlbuf;
	union T_primitives		*pptr;
	int				flags;
	int				rval;
	int				fmode;
	rval_t				rv;
	struct file			*sys_fp;

	
	*rvalp = 0;
	siptr = &so->so_user;
	sys_fp = so->so_sfp;

	fmode = osoc_do_fcntl(sys_fp, F_GETFL, 0, &rv);
	if (fmode & O_NDELAY) {
		osoc_do_fcntl(sys_fp, F_SETFL, fmode & ~O_NDELAY, &rv);
	}

	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;
	ctlbuf.maxlen = siptr->ctlsize;
	flags = RS_HIPRI;

	rv.r_val1 = 0;
	while ((rval = osoc_getmsg(sys_fp, &ctlbuf, NULL, &flags, &rv)) != 0) {
		if (rval == EINTR)
			continue;
		*rvalp = rval;
		return (0);
	}

	/*
	 * Did I get entire message
	 */
	if (rv.r_val1) {
		*rvalp = EIO;
		return (0);
	}

	/*
	 * Is ctl part large enough to determine type?
	 */
	if (ctlbuf.len < sizeof (long)) {
		*rvalp = EPROTO;
		return (0);
	}

	if (fmode & O_NDELAY)
		(void)osoc_do_fcntl(sys_fp, F_SETFL, fmode, &rv);

	/* LINTED pointer alignment */
	pptr = (union T_primitives *)ctlbuf.buf;
	switch (pptr->type) {
		case T_OK_ACK:
			if ((ctlbuf.len < sizeof (struct T_ok_ack)) ||
			    (pptr->ok_ack.CORRECT_prim != type)) {
				*rvalp = EPROTO;
				return (0);
			}
			return (1);

		case T_ERROR_ACK:
			if ((ctlbuf.len < sizeof (struct T_error_ack)) ||
			    (pptr->error_ack.ERROR_prim != type)) {
				*rvalp = EPROTO;
				return (0);
			}
			if (pptr->error_ack.TLI_error == TSYSERR)
				*rvalp = pptr->error_ack.UNIX_error;
			else	*rvalp = osoc_tlitosyserr(pptr->error_ack.TLI_error);
			return (0);

		default:
			*rvalp = EPROTO;
			return (0);
	}
}

/*
 * Translate a TLI error into a system error as best we can.
 */
ushort osoc_tlierrs[] = {
		0,		/* no error	*/
		EADDRNOTAVAIL,  /* TBADADDR	*/
		ENOPROTOOPT,	/* TBADOPT	*/
		EACCES,		/* TACCES	*/
		EBADF,		/* TBADF	*/
		EADDRNOTAVAIL,	/* TNOADDR	*/
		EPROTO,		/* TOUTSTATE	*/
		EPROTO,		/* TBADSEQ	*/
		0,		/* TSYSERR - will never get	*/
		EPROTO,		/* TLOOK - should never be sent by transport */
		EMSGSIZE,	/* TBADDATA	*/
		EMSGSIZE,	/* TBUFOVFLW	*/
		EPROTO,		/* TFLOW	*/
		EWOULDBLOCK,	/* TNODATA	*/
		EPROTO,		/* TNODIS	*/
		EPROTO,		/* TNOUDERR	*/
		EINVAL,		/* TBADFLAG	*/
		EPROTO,		/* TNOREL	*/
		EOPNOTSUPP,	/* TNOTSUPPORT	*/
		EPROTO,		/* TSTATECHNG	*/
};

/*
 * STATIC int osoc_tlitosyserr(int terr)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_tlitosyserr(int terr)
{
	if (terr > (sizeof (osoc_tlierrs) / sizeof (ushort)))
		return (EPROTO);
	else	return (int)osoc_tlierrs[terr];
}

/*
 * STATIC int osoc_do_fcntl(struct file *fp, int cmd, int arg, rval_t *rvp)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_do_fcntl(struct file *fp, int cmd, int arg, rval_t *rvp)
{
	int	rval;

	rval = 0;
	switch(cmd) {
	case F_GETFL:
		rvp->r_val1 = fp->f_flag + FOPEN;
		break;

	case F_SETFL:
		if ((arg & (FNONBLOCK|FNDELAY)) == (FNONBLOCK|FNDELAY))
			arg &= ~FNDELAY;
 		/*
		 * FRAIOSIG is a new flag added for the raw
		 * disk async io feature. This only applies
 		 * to character special files. But in case
		 */

		/* This is a socket - fd -- Thus a special file */
		/* SPECFS at this time does not have a setfl()	*/

		arg &= FMASK;
		fp->f_flag &= (FREAD|FWRITE);
		fp->f_flag |= (arg-FOPEN) & ~(FREAD|FWRITE);
		break;
	default:
		rval = EINVAL;
	}
	return(rval);
}

/*
 * STATIC int osoc_recvaccrights(struct osocket *so, struct msghdr *msg,
 *			int fmode, int *rvalp)
 * 	Get access rights and associated data.
 * 	Only UNIX domain supported.
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_recvaccrights(struct osocket *so, struct msghdr *msg, 
			int fmode, int *rvalp)
{
	/* Only UNIX domain supported and osocket does not support AF_UNIX. */
	*rvalp = -1;
	return(EINVAL);
}

/*
 * STATIC int osoc_msgpeek(struct osocket *so, struct strbuf *ctlbuf,
 *		struct strbuf *rcvbuf, int fmode)
 * 	Peeks at a message. If no messages are
 * 	present it will block in a poll().
 * 	Note ioctl(I_PEEK) does not block.
 *
 * Calling/Exit State:
 * 	Returns:
 *	0	On success
 *	-1	On error. In particular, EBADMSG is returned if access
 *		are present.
 */
STATIC int
osoc_msgpeek(struct osocket *so, struct strbuf *ctlbuf, 
		struct strbuf *rcvbuf, int fmode)
{
	int			retval;
	struct strpeek		strpeek;
	int			rval;
	struct file		*sys_fp;
	struct vnode		*sys_vp;

	sys_fp = so->so_sfp;
	sys_vp = so->so_svp;
	
	strpeek.ctlbuf.buf = ctlbuf->buf;
	strpeek.ctlbuf.maxlen = ctlbuf->maxlen;
	strpeek.ctlbuf.len = 0;
	strpeek.databuf.buf = rcvbuf->buf;
	strpeek.databuf.maxlen = rcvbuf->maxlen;
	strpeek.databuf.len = 0;
	strpeek.flags = 0;

	for (;;) {
		rval = strioctl(sys_vp, I_PEEK, (int)&strpeek, sys_fp->f_flag,
				K_TO_K, u.u_lwpp->l_cred, &retval);
		if (rval)
			return(rval);

		if (retval == 1) {
			ctlbuf->len = strpeek.ctlbuf.len;
			rcvbuf->len = strpeek.databuf.len;
			return (0);
		} else	if ((fmode & O_NDELAY) == 0) {
			retval = 0;
			rval = strwaitq(sys_vp->v_stream, GETWAIT, 0, 
					     sys_fp->f_flag, &retval);
			if (rval || retval)
				return(rval);
		} else	{
			return(EAGAIN);
		}
	}
	/* NOTREACHED*/
}

/*
 * STATIC int osoc_recvmsg(struct osocket *so, struct msghdr *msg, 
 *		int flags, int *rvalp)
 * 	Receive a message according to flags.
 *
 * Calling/Exit State:
 * 	On Returns:
 *	count 	in *rvalp
 *	 0 	return val on success
 *	-1 	return val on error
 */
int
osoc_recvmsg(struct osocket *so, struct msghdr *msg, int flags, int *rvalp)
{  
	int		fmode;
	int		len;
	int		pos;
	int		i;
	struct strbuf	ctlbuf;
	struct strbuf	rcvbuf;
	int 		addrlen;
	int		flg;
	struct _si_user	*siptr;
	int		rval;
	int		retval;
	struct file	*sys_fp;
	rval_t		rv;
	caddr_t		kbuf;
	int		klen;
	int		count;

	siptr = &so->so_user;
	sys_fp = so->so_sfp;

	for (i = 0, len = 0; i < msg->msg_iovlen; i++)
		len += msg->msg_iov[i].iov_len;

	if (len == 0 && msg->msg_accrightslen == 0)
		return (0);

	/*
	 * Allocate a kernel Memory for the receive buffer
	 */
	klen = len;
	kbuf = kmem_alloc(klen, KM_SLEEP);
	rcvbuf.buf = kbuf;

	rval = osoc_do_fcntl(sys_fp, F_GETFL, 0, &rv);
	if (rval) {
		kmem_free(kbuf, klen);
		return(rval);
	}
	
	fmode = rv.r_val1;

tryagain:
	rcvbuf.maxlen = len;
	rcvbuf.len = 0;

	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;

	if (flags & MSG_OOB) {
		/*
		 * Handles the case when MSG_PEEK is set
		 * or not.
		 */
		rval = osoc_do_ioctl(so, rcvbuf.buf, rcvbuf.maxlen, flags,
								&rcvbuf.len);
		if (rval)
			goto rcvout;
	} else if (flags & MSG_PEEK) {
		rval = osoc_msgpeek(so, &ctlbuf, &rcvbuf, fmode);
		if (rval) {
			if (rval == EBADMSG) {
				rval = 0;
				rval = osoc_recvaccrights(so, msg, fmode, 
							  &retval);
				rcvbuf.len = retval;
			}
			goto rcvout;
		}
	} else	{
		flg = 0;
		/*
		 * Have to prevent spurious SIGPOLL signals
		 * which can be caused by the mechanism used
		 * to cause a SIGURG.
		 */
		rval = osoc_getmsg(sys_fp, &ctlbuf, &rcvbuf, &flg, &rv);
		if (rval) {
			if (rval == EBADMSG) {
				rval = 0;
				rval = osoc_recvaccrights(so, msg, fmode,
							  &retval);
				rcvbuf.len = retval;
			}
			goto rcvout;
		}
	}

	if (rcvbuf.len == -1)
		rcvbuf.len = 0;

	if (ctlbuf.len == sizeof (struct T_exdata_ind) &&
		/* LINTED pointer alignment */
		*(long *)ctlbuf.buf == T_EXDATA_IND && rcvbuf.len == 0) {
		/*
		 * Must be the message indicating the position
		 * of urgent data in the data stream - the user
		 * should not see this.
		 */
		if (flags & MSG_PEEK) {
			/*
			 * Better make sure it goes.
			 */
			flg = 0;
			(void)osoc_getmsg(sys_fp, &ctlbuf, &rcvbuf, &flg, &rv);
		}
		goto tryagain;
	}

	/*
	 * Copy it all back as per the users
	 * request.
	 */
	for (i=pos=0, len=rcvbuf.len; i < msg->msg_iovlen; i++) {
		count = MIN(msg->msg_iov[i].iov_len, len);
		if (copyout(&rcvbuf.buf[pos], msg->msg_iov[i].iov_base, 
					count)) {
			rval = EFAULT;
			goto rcvout;
		}
		pos += count;
		len -= count;
		if (len == 0)
			break;
		else if (len < 0 ) {
			/*
			 *+ osoc_recvmsg negative len
			 */
			cmn_err(CE_WARN, "osoc_recvmsg negative len %d\n",
				len);
		}
	}

	/*
	 * Copy in source address if requested.
	 */
rcvout:
	if (rval == 0 && msg->msg_name && msg->msg_namelen) {
		if (siptr->udata.servtype == T_CLTS) {
			if (ctlbuf.len != 0) {
				struct T_unitdata_ind *udata_ind;
				char *tmp;

				/* LINTED pointer alignment */
				udata_ind = (struct T_unitdata_ind *)ctlbuf.buf;
				tmp = (char *)(udata_ind->SRC_offset + ctlbuf.buf);

				/* Convert the sockaddr back to an old sockaddr */
				SOCKADDR_TO_OLD_SOCKADDR(tmp);
				osoc_cpaddr(
					msg->msg_name,
					msg->msg_namelen,
					udata_ind->SRC_offset + ctlbuf.buf,
					udata_ind->SRC_length, &count);

				msg->msg_namelen = count;
			}
		} else	{
			if (rval) {
				rval = osoc_dogetname(so, 
					/* LINTED pointer alignment */
					(struct osockaddr *)msg->msg_name, 
					&addrlen, msg->msg_namelen, REMOTE_ADDR);
				if (rval) {
					rval = 0;
					msg->msg_namelen = 0;
				}
				msg->msg_namelen = addrlen;
			}
		}
	}

	kmem_free(kbuf, klen);
	if (!rval)
		*rvalp = rcvbuf.len;
	return (rval);
}

struct recvmsga {
	int	s;
	struct msghdr *msg;
	int flags;
};

STATIC int
osoc_recvmsg_wrapper(struct recvmsga *uap, int *rvalp)
{
	struct osocket	*so;
	struct msghdr	msg;
	int		rval;
	
	if (rval = osoc_getsocket_with_fd(&so, uap->s))
		return(rval);

	if (copyin((caddr_t)uap->msg, (caddr_t)&msg, sizeof(struct msghdr)))
		return EFAULT;

	return(osoc_recvmsg(so, &msg, uap->flags, rvalp));
}

/*
 * STATIC int osoc_soreceive(struct osocket *so, struct msghdr *msg,
 *		int flags, int *rvalp)
 * 	Common receive code.
 *
 * Calling/Exit State:
 */
STATIC int
osoc_soreceive(struct osocket *so, struct msghdr *msg, int flags, int *rvalp)
{
	struct _si_user		*siptr;
	int			rval;

	rval = 0;
	siptr = &so->so_user;

	if (siptr->udata.so_state & OSS_CANTRCVMORE)
		return (0);

	if (siptr->udata.servtype == T_COTS ||
			siptr->udata.servtype == T_COTS_ORD) {
		if ((siptr->udata.so_state & OSS_ISCONNECTED) == 0) {
			rval = osoc_getudata(so, 0);
			if (rval)
				return (rval);

			if ((siptr->udata.so_state & OSS_ISCONNECTED) == 0)
				return (ENOTCONN);
		}
	}

	if ((siptr->udata.so_state & SS_ISBOUND) == 0) {
		/*
		 * Need to bind it for TLI.
		 */
		rval = osoc_dobind(so, NULL, 0, NULL, NULL);
		if (rval)
			return (rval);
	}

	rval = osoc_recvmsg(so, msg, flags, rvalp);

	return (rval);
}

/*
 * This Code is here to preserve state between osoc_get_msg_slice and
 * osoc_sosend.  It was previously doing this in the user libs by 
 * using static variables -- VERY VERY BAD.
 */

struct hold {
	char	*pos;
	int	left;
	int	i;
};

/*
 * STATIC int osoc_sosend(struct osocket *so, struct msghdr *msg,
 *		int flags, int *rvalp)
 * 	Common send code.
 *
 * Calling/Exit State:
 */
STATIC int
osoc_sosend(struct osocket *so, struct msghdr *msg, int flags, int *rvalp)
{
	int		i;
	int		len;
	int		retval;
	struct strbuf	ctlbuf;
	struct strbuf	databuf;
	struct _si_user	*siptr;
	int		rval;
	rval_t		rv;
	char		*kbuf;
	int		klen;
	struct file	*sys_fp;

	kbuf = NULL;
	klen = 0;
	rval = 0;
	siptr = &so->so_user;
	sys_fp = so->so_sfp;

	if (siptr->udata.so_state & SS_CANTSENDMORE) {
		return (EPIPE);
	}

	if ((siptr->udata.servtype == T_CLTS && msg->msg_namelen <= 0) ||
					siptr->udata.servtype != T_CLTS) {
		if ((siptr->udata.so_state & SS_ISCONNECTED) == 0) {
			rval = osoc_getudata(so, 0);
			if (rval)
				return (rval);
			if ((siptr->udata.so_state & SS_ISCONNECTED) == 0) {
				if (siptr->udata.servtype == T_CLTS)
					rval = EDESTADDRREQ;
				else	rval = ENOTCONN;
				return (rval);
			}
		}
	}

	if ((siptr->udata.so_state & SS_ISBOUND) == 0) {
		/*
		 * Need to bind it for TLI.
		 */
		rval = osoc_dobind(so, NULL, 0, NULL, NULL);
		if (rval)
			return (rval);
	}

	for (i= 0, len = 0; i < msg->msg_iovlen; i++)
		len += msg->msg_iov[i].iov_len;

	if (flags & MSG_DONTROUTE) {
		int	val;

		val = 1;
		rval = osoc_dosetsockopt(so, SOL_SOCKET, SO_DONTROUTE, 
				(char *)&val, sizeof (val), K_TO_K);
		if (rval)
			return (rval);
	}

	/*
	 * Access rights only in UNIX domain.
	 */
	if (msg->msg_accrightslen) {
		rval = EOPNOTSUPP;
		goto sndout;
	}

	if (flags & MSG_OOB) {
		/*
		 * If the socket is SOCK_DGRAM or
		 * AF_UNIX which we know is not to support
		 * MSG_OOB or the TP does not support the
		 * notion of expedited data then we fail.
		 *
		 * Otherwise we hope that the TP knows
		 * what to do.
		 */
		if (siptr->family == OAF_UNIX ||
				siptr->udata.servtype == T_CLTS ||
				siptr->udata.etsdusize == 0) {
			rval = EOPNOTSUPP;
			goto sndout;
		}
	}

	if (siptr->udata.servtype == T_CLTS) {
		struct T_unitdata_req	*udata_req;
		char			*dbuf;
		char			*tmpbuf;
		int			pos;
		int			tmpcnt;

		if (len < 0 || len > siptr->udata.tidusize) {
			rval = EMSGSIZE;
			goto sndout;
		}

		if ((siptr->udata.so_state & OSS_ISCONNECTED) == 0) {
			switch (siptr->family) {
			case AF_INET:
				if (msg->msg_namelen !=
						sizeof (struct sockaddr_in))
					rval = EINVAL;
				break;

			default:
				if (msg->msg_namelen > siptr->udata.addrsize)
					rval = EINVAL;
				break;
			}
			if (rval)
				goto sndout;
		}

		if (msg->msg_namelen > 0 && siptr->family == AF_UNIX) {
			rval = EOPNOTSUPP;
			goto sndout;
		}

		klen = len;
		kbuf = dbuf = kmem_alloc(klen, KM_SLEEP);
		/*
		 * Have to make one buffer
		 */
		for (i= 0, pos = 0; i < msg->msg_iovlen; i++) {
			rval = copyin(msg->msg_iov[i].iov_base,
					&dbuf[pos],
					msg->msg_iov[i].iov_len);
			if (rval) {
				rval = EFAULT;
				goto sndout;
			}
			pos += msg->msg_iov[i].iov_len;
		}

		if (msg->msg_accrightslen) {
			rval = EOPNOTSUPP;
			kmem_free(kbuf, klen);
			kbuf = NULL;
			goto sndout;
		}

		tmpbuf = siptr->ctlbuf;
		/* LINTED pointer alignment */
		udata_req = (struct T_unitdata_req *)tmpbuf;
		udata_req->PRIM_type = T_UNITDATA_REQ;
		udata_req->DEST_length = MIN(msg->msg_namelen,
				siptr->udata.addrsize);
		udata_req->DEST_offset = 0;
		tmpcnt = sizeof (*udata_req);

		if ((int)udata_req->DEST_length > 0 && tmpbuf != (char *)NULL) {
			/* Copy the msg_name from User Space */
			osoc_ualigned_copy(tmpbuf, udata_req->DEST_length, 
				tmpcnt, msg->msg_name, 
				(int *)&udata_req->DEST_offset);
			tmpcnt += udata_req->DEST_length;
		}

		ctlbuf.len = tmpcnt;
		ctlbuf.buf = tmpbuf;

		databuf.len = len == 0 ? -1 : len;
		databuf.buf = dbuf;

		rval = osoc_putmsg(sys_fp, &ctlbuf, &databuf, 0, &rv);
		if (rval) {
			if (rval == EAGAIN)
				rval = ENOMEM;
		}
		kmem_free(kbuf, klen);
		kbuf = NULL;

		if (rval == 0) {
			retval = databuf.len == -1 ? 0 : databuf.len;
		}
		goto sndout;
	} else	{
		struct T_data_req	*data_req;
		int			tmp;
		int			tmpcnt;
		int			firsttime;
		char			*tmpbuf;
		struct hold		hold;

		if (len == 0) {
			retval = 0;
			goto sndout;
		}

		if (msg->msg_accrightslen) {
			rval = EOPNOTSUPP;
			goto sndout;
		}

		/* LINTED pointer alignment */
		data_req = (struct T_data_req *)siptr->ctlbuf;

		ctlbuf.len = sizeof (*data_req);
		ctlbuf.buf = siptr->ctlbuf;

		/* Allocate space for the whole message */
		klen = len;
		kbuf = kmem_alloc(klen, KM_SLEEP);

		tmp = len;
		firsttime = 0;
		while (tmpcnt = osoc_get_msg_slice(msg, &tmpbuf,
				siptr->udata.tidusize, firsttime, &hold)) {
			if (flags & MSG_OOB) {
				data_req->PRIM_type = T_EXDATA_REQ;
				if ((tmp - tmpcnt) != 0)
					data_req->MORE_flag = 1;
				else	data_req->MORE_flag = 0;
			} else	{
				data_req->PRIM_type = T_DATA_REQ;
			}

			/*
			 * Urgent data.
			 */
			if (tmpcnt > klen) {
				/* Just in case that the above went beyond */
				/* required message */
				kmem_free(kbuf, klen);
				kbuf = NULL;
				rval = EFAULT;
				goto sndout;
			}
			rval = copyin(tmpbuf, kbuf, tmpcnt);
			if (!rval) {
				databuf.len = tmpcnt;
				databuf.buf = kbuf;
				rval = osoc_putmsg(sys_fp, &ctlbuf, &databuf, 
						  0, &rv);
			} else {
				rval = EFAULT;
			}

			if (rval) {
				if (len == tmp) {
					if (rval == EAGAIN)
						rval = ENOMEM;
					kmem_free(kbuf, klen);
					kbuf = NULL;
					goto sndout;
				} else	{
					rval = 0;
					retval = len - tmp;
					kmem_free(kbuf, klen);
					kbuf = NULL;
					goto sndout;
				}
			}
			firsttime = 1;
			tmp -= tmpcnt;
		}
		retval = len - tmp;
		kmem_free(kbuf, klen);
		kbuf = NULL;
	}
sndout:
	if (flags & MSG_DONTROUTE) {
		int	val;

		val = 0;
		rval = osoc_dosetsockopt(so, SOL_SOCKET, SO_DONTROUTE, 
			(char *)&val, sizeof (val), K_TO_K);
	}
	if (rval) {
		if (rval == ENXIO || rval == EIO)
			rval = EPIPE;
		return (rval);
	}
	*rvalp = retval;
	return	(rval);
}

/*
 * STATIC int osoc_get_msg_slice(struct msghdr *msg, char **ptr, int askedfor,
 *		int firsttime, struct hold *hold)
 * 	On return, ptr points at the next slice of
 * 	data of askedfor size. Returns the actual
 * 	amount.
 *
 * Calling/Exit State:
 */
STATIC int
osoc_get_msg_slice(struct msghdr *msg, char **ptr, int askedfor, 
			int firsttime, struct hold *hold)
{
	int		count;

	if (!firsttime) {
		if (msg->msg_iovlen <= 0) {
			*ptr = NULL;
			return (0);
		}
		hold->i = 0;
		hold->left = msg->msg_iov[hold->i].iov_len;
		hold->pos = msg->msg_iov[hold->i].iov_base;
	}
again:
	if (hold->left) {
		if (hold->left > askedfor) {
			*ptr = hold->pos;
			hold->pos += askedfor;
			hold->left -= askedfor;
			return (askedfor);
		} else	{
			*ptr = hold->pos;
			count = hold->left;
			hold->left = 0;
			hold->i++;
			return (count);
		}
	}

	if (hold->i == msg->msg_iovlen)
		return (0);

	hold->pos = msg->msg_iov[hold->i].iov_base;
	hold->left = msg->msg_iov[hold->i].iov_len;

	goto again;
}

/*
 * STATIC int osoc_doclose(struct osocket *so)
 * 	Close a socket on last file table reference removal.
 *	Initiate disconnect if connected.
 *	Free socket when disconnect complete.
 *
 * Calling/Exit State:
 */
STATIC int
osoc_doclose(struct osocket *so)
{
	int             rval;
	
	rval = 0;
	if (so && so->so_fp) {
		/* The device close function will call osoc_sofree() */
		rval = closef(so->so_fp);
		if(so->so_fd) {
			setf(so->so_fd, NULLFP);
			so->so_fd = 0;
		}
	}
	return (rval);
}

/*
 * STATIC int osoc_sofree(struct osocket *so)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_sofree(struct osocket *so)
{
	if ((so == OSOCK_AVAIL) || 
	    (so == OSOCK_RESERVE) || 
	    (so == OSOCK_INPROGRESS)) {
		return(EINVAL);
	} else  {
		if (so->so_user.ctlbuf)
			kmem_free(so->so_user.ctlbuf, so->so_user.ctlsize);
		kmem_free(so, sizeof(struct osocket));
	}
	return (0);
}


/*
 * STATIC int osoc_aligned_copy(char *buf, int len, int init_offset,
 *			 char *datap, int *rtn_offset)
 * 	Copy data to output buffer and align it as in input buffer
 * 	This is to ensure that if the user wants to align a network
 * 	addr on a non-word boundry then it will happen.
 *
 * Calling/Exit State:
 */

STATIC int
osoc_aligned_copy(char *buf, int len, int init_offset, 
		char *datap, int *rtn_offset)
{
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
 * STATIC int osoc_ualigned_copy(char *buf, int len, int init_offset,
 *			char *datap, int *rtn_offset)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_ualigned_copy(char *buf, int len, int init_offset, 
		char *datap, int *rtn_offset)
{
	int	rval;
	char	*tmp;

	if (VALID_USR_RANGE(buf, len))
		return(EFAULT);
	if (!VALID_USR_RANGE(datap, len))
		return(EFAULT);
	if (VALID_USR_RANGE(rtn_offset, 4))
		return(EFAULT);
		
	*rtn_offset = ROUNDUP(init_offset) + ((unsigned int)datap&0x03);

	tmp = buf + *rtn_offset;
	rval = copyin(datap, tmp, len);

	/* Convert the sockaddr just read in to new_sockaddrs */
	OLD_SOCKADDR_TO_SOCKADDR(tmp, len);
	return (rval ? EFAULT : 0);
}

/*
 * STATIC int osoc_cpaddr(char *to, int tolen, char *from,
 *		int fromlen, int *rsizep)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_cpaddr(char *to, int tolen, char *from, int fromlen, int *rsizep)
{
	int rval;

	if (!VALID_USR_RANGE(to, tolen))
		return(EFAULT);
	if (VALID_USR_RANGE(from, tolen))
		return(EFAULT);
	if (VALID_USR_RANGE(rsizep, 4))
		return(EFAULT);

	uzero(to, tolen);
	if (tolen > sizeof (struct sockaddr_in))
		tolen = sizeof (struct sockaddr_in);
	rval = copyout(from, to, MIN(fromlen, tolen));
	if (rval == 0)
		*rsizep = tolen;
	return(rval);
}

/*
 * STATIC int osoc_getsocket_with_fd(struct osocket **sopp, int fd)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_getsocket_with_fd(struct osocket **sopp, int fd)
{
	struct file *fp;
	dev_t           rdev;
	int		rval;

	rval = getf(fd, &fp);
	if (rval)
		return (EINVAL);

	rdev = fp->f_vnode->v_rdev;
	FTE_RELE(fp);
	return (osoc_getsocket_with_dev(sopp, rdev));
}

/*
 * STATIC int osoc_getsocket_with_dev(struct osocket **sopp, dev_t rdev)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_getsocket_with_dev(struct osocket **sopp, dev_t rdev)
{
	struct osocket  *so;
	int             min;

	min = getminor(rdev);
	if (!osockinited ||
	    (min < 0) ||
	    (min >= num_osockets)) {
		return (ENOTSOCK);
	}

	so = osocket_tab[min];
	if ((so == OSOCK_AVAIL) || 
	    (so == OSOCK_RESERVE) || 
	    (so == OSOCK_INPROGRESS)) {
		return(EINVAL);
	}

	/*Initialized Socket ? */
	if (so->so_svp == NULL)
		return (EINVAL);

	*sopp = so;
	return (0);
}


/*
 * STATIC int osoc_getargs(caddr_t snamep, caddr_t unamep, int namelen)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_getargs(caddr_t snamep, caddr_t unamep, int namelen)
{

	if ((unamep == NULL) || (namelen == 0))
		return (EINVAL);

	if (!VALID_USR_RANGE(unamep, namelen))
		return(EFAULT);
	if (VALID_USR_RANGE(snamep, namelen))
		return(EFAULT);
	if (copyin(unamep, snamep, namelen)) {
		return(EFAULT);
	} 
	return (0);
}


/*
 * STATIC int osoc_getmsg(struct file *fp, struct strbuf *ctl,
 *		struct strbuf *data, int *flags, rval_t *rvp)
 * The osoc_getmsg(), osoc_putmsg() and osoc_msgio() are duplicates of the 
 * Base getmsg/putmsg/msgio with handling to use kernel addresses instead
 * of user addresses.
 *
 * Calling/Exit State:
 */
STATIC int
osoc_getmsg(struct file *fp, struct strbuf *ctl, struct strbuf *data, 
		int *flags, rval_t *rvp)
{
	int error;
	int localflags;
	int realflags = 0;
	unsigned char pri = 0;

	/*
	 * Convert between old flags (localflags) and new flags (realflags).
	 */
	localflags = *flags;
	switch (localflags) {
	case 0:
		realflags = MSG_ANY;
		break;

	case RS_HIPRI:
		realflags = MSG_HIPRI;
		break;

	default:
		return (EINVAL);
	}

	if ((error = osoc_msgio(fp, ctl, data, rvp, FREAD, 
				&pri, &realflags)) == 0) {
		/*
		 * massage realflags based on localflags.
		 */
		if (realflags == MSG_HIPRI)
			localflags = RS_HIPRI;
		else
			localflags = 0;
		*flags = localflags;
	}
	return (error);
}

/*
 * STATIC int osoc_putmsg(struct file *fp, struct strbuf *ctl,
 *			struct strbuf *data, int flags, rval_t *rvp)
 *
 * Calling/Exit State:
 */
STATIC int
osoc_putmsg(struct file *fp, struct strbuf *ctl, struct strbuf *data, 
		int flags, rval_t *rvp)
{
	unsigned char pri = 0;

	switch (flags) {
	case RS_HIPRI:
		flags = MSG_HIPRI;
		break;

	case 0:
		flags = MSG_BAND;
		break;

	default:
		return (EINVAL);
	}
	return (osoc_msgio(fp, ctl, data, rvp, FWRITE, &pri, &flags));
}

/*
 * STATIC int osoc_msgio(struct file *fp, struct strbuf *ctl, 
 * struct strbuf *data, rval_t *rvp, int mode, unsigned char *prip, int *flagsp)
 *
 * Common code for osoc_getmsg and osoc_putmsg calls: check permissions,
 * copy in args, do preliminary setup, and switch to
 * appropriate stream routine.
 *
 * Calling/Exit State:
 */
STATIC int
osoc_msgio(struct file *fp, struct strbuf *ctl, struct strbuf *data, 
		rval_t *rvp, int mode, unsigned char *prip, int *flagsp)
{
	vnode_t *vp;
	struct strbuf msgctl, msgdata;
	int error;

	if ((fp->f_flag & mode) == 0)
		return (EBADF);
	vp = fp->f_vnode;
	if ((vp->v_type != VFIFO && vp->v_type != VCHR) || vp->v_stream == NULL)
		return (ENOSTR);

	/* Setup Control */
	if (ctl)
		msgctl = *ctl;		/* Structure Copy */
	else {
		msgctl.len = -1;
		msgctl.maxlen = -1;
	}

	/* Setup Data */
	if (data)
		msgdata = *data;		/* Structure Copy */
	else {
		msgdata.len = -1;
		msgdata.maxlen = -1;
	}

	if (mode == FREAD) {
		error = strgetmsg(vp, &msgctl, &msgdata, prip,
				  flagsp, fp->f_flag, K_TO_K, rvp);
		if (error)
			return(error);

		if (ctl)
			*ctl = msgctl;		/* Structure Copy */
		if (data)
			*data = msgdata;	/* Structure Copy */
	} else  {
		/*
		 * FWRITE case 
		 */
		error = strputmsg(vp, &msgctl, &msgdata, *prip, 
				*flagsp, fp->f_flag, K_TO_K, u.u_lwpp->l_cred);
	}

	return(error);
}


/*
 * STATIC int osoc_doadjtime(struct timeval *delta, struct timeval *olddelta)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
osoc_doadjtime(struct timeval *delta, struct timeval *olddelta)
{
	struct timeval	tv, oldtv;

	if (pm_denied(u.u_lwpp->l_cred, P_SYSOPS))
		return EPERM;
	if (copyin((caddr_t)delta, (caddr_t)&tv, sizeof tv))
		return EFAULT;

	clockadj(&tv, &oldtv, B_TRUE);

	if (olddelta) {
		if (copyout((caddr_t)&oldtv, (caddr_t)olddelta, sizeof oldtv))
			return EFAULT;
	}

	return 0;
}

/*
 * int socketsys(struct socketsysa *uap, rval_t *rvp)
 *	System call version of SIOCSOCKSYS. 
 *
 * Calling/Exit State:
 */

int
socketsys(struct socketsysa *uap, rval_t *rvp)
{
	int rval = 0, error;
	struct socksysreq sreq;
	int cmd;
       
	if (!osockinited) { 
		/*
		 * Reserve first osocket table entry so as to 
		 * provide a mechanism to do get a socket and 
		 * support admin functions.
		 */
		osocket_tab[0] = OSOCK_RESERVE;
		/* 
		 * also reserve the second as the 'clone' device
		 */
		osocket_tab[1] = OSOCK_RESERVE;

		osockinited = 1;
	}
	if (ucopyin((caddr_t)uap->argp, (caddr_t)&sreq, sizeof(sreq), 0) != 0) {
                return (EFAULT);
	}

	cmd = sreq.args[0];
	
	if((cmd < 0) || (cmd >= osoc_ncalls))
		cmd = 0;

	error = (*osoc_call[cmd])(&sreq.args[1], &rval);
	rvp->r_val1 = rval;
	return(error);
}

/* Enhanced Application Binary Compatibility */
