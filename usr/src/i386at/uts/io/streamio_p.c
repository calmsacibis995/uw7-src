#ident	"@(#)kern-i386at:io/streamio_p.c	1.10.2.1"
#ident	"$Header$"

/*
 * Platform-specific STREAM I/O processing
 */

#include <acc/audit/audit.h>
#include <acc/dac/acl.h>
#include <acc/mac/cca.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/filio.h>
#include <fs/ioccom.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/poll.h>
#include <io/sad/sad.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strsubr.h>
#include <io/open.h>
#include <io/termio.h>
#include <io/termios.h>
#include <io/ttold.h>
#include <io/uio.h>
#include <io/gvid/genvid.h>
#include <io/kd/kd.h>
#include <io/ws/vt.h>
#include <io/mouse.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/secsys.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <io/xque/xque.h>

/* Enhanced Application Compatibility Support */

#include <svc/isc.h>
#include <svc/sco.h>
#include <io/event/event.h>

STATIC int	remap_streams_ioctls(dev_t, int);
STATIC int	remap_posix_ioctls(dev_t, int);
STATIC int	modeswitch3_2(int);
int stri386ioctl(struct vnode *, int *, int, int *, int *);

extern void	ws_setcompatflgs(dev_t);
extern void	ws_clrcompatflgs(dev_t);
extern int	ws_sysv_iscompatset(dev_t, int);
extern void	ws_sysv_clrcompatflgs(dev_t, int);
extern void	ws_sysv_setcompatflgs(dev_t, int);
extern int	ws_iscompatset(dev_t);
extern int	ws_isvdcset(dev_t);

/* End Enhanced Application Compatibility Support */

struct strioctlstate {
	int	keymap_type;
	caddr_t	event_qaddr;
	int	sco_xioc_cmd;
};

/*
 * void
 * strioctl_p(struct vnode *vp, int *cmdp, int *argp, int flag, int copyflag,
 *		cred_t *crp, int *rvalp, struct strioctlstate **iocstatep, 
 *			mblk_t **mpp, int *errorp)
 *
 * Calling/Exit State:
 *	- Assumes sd_mutex unlocked.
 *	- Return 0 on success, otherwise return 1.
 *
 * Description:
 *	Do any platform-specific ioctl processing for streams.
 */
/* ARGSUSED */
void
strioctl_p(struct vnode *vp, int *cmdp, int *argp, int flag,
		int copyflag, cred_t *crp, int *rvalp, 
		struct strioctlstate **iocstatepp, mblk_t **mpp, int *errorp)
{
	int	keymap_type = -1;
	int	sco_xioc_cmd = 0;
	int	cmd = *cmdp;
	caddr_t event_qaddr = NULL;
	struct stdata *stp;
	struct strioctlstate *iocstatep;
	boolean_t kmflag;	/* flag to indicate if kmem should be freed */


	*errorp = 0;
	stp = vp->v_stream;

#define LONGIOCTYPE     0xffffff00


	if (*iocstatepp == NULL) {

		*iocstatepp = kmem_zalloc(sizeof(struct strioctlstate), KM_SLEEP);
		kmflag = B_TRUE;
		iocstatep = *iocstatepp;
	
		/* Enhanced Application Compatibility Support */

		/*
		 * If it is an ISC POSIX exec then translate ISC POSIX values
		 * to that of SVR4.
		 * If is a SCO exec AND it is a SCO Old XIOC termios
		 * translate it to the BSC version
		 */

		if (ISC_USES_POSIX) {
			if ((*cmdp & LONGIOCTYPE) == ISC_TIOC) {
				if (*cmdp == ISC_TCGETPGRP) {
					/* This check is made in SVR4.0 Library */
					if (stp->sd_sessp->s_sidp !=
						u.u_procp->p_sessp->s_sidp) {
						*errorp = ENOTTY;
						return;
					}
					*cmdp = TIOCGPGRP;
				} else if (cmd == ISC_TCSETPGRP)
					*cmdp = TIOCSPGRP;
			}

		} else if (IS_SCOEXEC) {

			switch (*cmdp & LONGIOCTYPE) {
			case LDEV_MOUSE:
			case EVLD_IOC:
				switch (*cmdp) {
				case LDEV_ATTACHQ:
				case LDEV_MSEATTACHQ: {
					file_t *fp;
					ASSERT(KS_HOLD0LOCKS());

					if (getf(*argp, &fp))
						*argp = -1;
					else 
						*argp = fp->f_vnode->v_rdev;
					FTE_RELE(fp);
					kmflag = B_FALSE;
					break;
				}
				case LDEV_GETEV:
				case LDEV_SETRATIO:
				case LDEV_SETTYPE:
					break;
				}

				break;

	                case EQIOC:
				switch(*cmdp) {
				case EQIO_GETQP:
					event_qaddr = (caddr_t) *argp;
					*argp = vp->v_rdev;
					kmflag = B_FALSE;
				}

				break;

			case SCO_OXIOC:
				/* stri386ioctl needs the original cmd */
				sco_xioc_cmd = (*cmdp & ~IOCTYPE) | SCO_XIOC;
				kmflag = B_FALSE;
				break;

			case SCO_TIOC:
				if (*cmdp == SCO_TIOCGPGRP) {
					/* This check is made in SVR4.0 lib. */
					if (stp->sd_sessp->s_sidp !=
						u.u_procp->p_sessp->s_sidp) {
						*errorp = ENOTTY;
						return;
					}
					*cmdp = TIOCGPGRP;
				} else if (*cmdp == SCO_TIOCSPGRP)
					*cmdp = TIOCSPGRP;

				break;

			case SCO_OLD_C_IOC:
				*cmdp = (*cmdp & ~IOCTYPE) | SCO_C_IOC;
				break;

			case MIOC:
			case WSIOC:
				switch(*cmdp) {
				case GIO_KEYMAP:
				case KDDFLTKEYMAP:
				case PIO_KEYMAP:
					keymap_type = SCO_FORMAT;
					kmflag = B_FALSE;
					break;
				}

			} /* end switch */

		} else {

			switch (*cmdp & LONGIOCTYPE) {
			case LDEV_MOUSE:
			case EVLD_IOC:
				switch (*cmdp) {
				case LDEV_MSEATTACHQ: 
				case LDEV_ATTACHQ: {
					file_t *fp;
					ASSERT(KS_HOLD0LOCKS());

					if (getf(*argp, &fp))
						*argp = -1;
					else 
						*argp = fp->f_vnode->v_rdev;
					FTE_RELE(fp);
					kmflag = B_FALSE;
					break;
				}

				break;
			}

	                case EQIOC:
				switch(*cmdp) {
				case EQIO_GETQP:
					event_qaddr = (caddr_t) *argp;
					*argp = vp->v_rdev;
					kmflag = B_FALSE;
				}

				break;

			case MIOC:
			case WSIOC:
				switch(*cmdp) {
				case GIO_KEYMAP:
				case KDDFLTKEYMAP:
				case PIO_KEYMAP:
					keymap_type = USL_FORMAT;
					kmflag = B_FALSE;
					break;
				}
				break;
			}
		}
	
        	/* End Enhanced Application Compatibility Support */

		if (stri386ioctl(vp, cmdp, *argp, rvalp, errorp)) {
			/* release memory before returning error */
			kmem_free(iocstatep, sizeof(struct strioctlstate));
			*iocstatepp = NULL;
			return;
		}

		/* Enhanced Application Compatibility Support */

		/*
		 * stri386ioctl needs the original cmd 
		 */

		if (sco_xioc_cmd) {
			if ((*cmdp & MODESWITCH) != MODESWITCH)
				*cmdp = sco_xioc_cmd;
		}

		/*
		 * release memory if the ioctl state is not required
		 */
		if (kmflag) {
			kmem_free(iocstatep, sizeof(struct strioctlstate));
			*iocstatepp = NULL;
			return; 
		}

		/*
		 * save the platform specific ioctl state
		 */
		iocstatep->keymap_type = keymap_type;
		iocstatep->sco_xioc_cmd = sco_xioc_cmd;
		iocstatep->event_qaddr = event_qaddr;

		/* End Enhanced Application Compatibility Support */ 
	} else {

		mblk_t *bp1 = NULL;

		/*
		 * Do any platform specific ioctl processing.
		 */

		iocstatep = *iocstatepp;

		/*
		 * restore the above platform specific saved state
		 */
		ASSERT(iocstatep != NULL);
		keymap_type = iocstatep->keymap_type;
		event_qaddr = iocstatep->event_qaddr;

		ASSERT(*mpp == NULL);

		/* Enhanced Application Compatibility Support */

		switch (*cmdp) {

		default:
			if (keymap_type != -1) {
				struct keymap_flags *kp;
				int size = sizeof(struct keymap_flags);

				while (!(bp1 = allocb(size, BPRI_HI))) {
					if (*errorp = strwaitbuf(size, BPRI_HI, stp))
						return;
				}
				bp1->b_wptr += size;
				/* LINTED pointer alignment */
				kp = (struct keymap_flags *) bp1->b_rptr;
				kp->km_type = keymap_type;
				kp->km_magic = KEYMAP_MAGIC;

			} else if (event_qaddr != NULL) {
				struct event_getq_info *xp;
				int size = sizeof(struct event_getq_info);

				while (!(bp1 = allocb(size, BPRI_HI))) {
					if (*errorp = strwaitbuf(size, BPRI_HI, stp))
						return;
				}
				bp1->b_wptr += size;
				/* LINTED pointer alignment */
				xp = (struct event_getq_info *) bp1->b_rptr;
				xp->einfo_addr = event_qaddr;
				xp->einfo_rdev  = (dev_t) *argp;
			}

			/* return the message block */
			*mpp = bp1;

		} /* end switch */

		/* End Enhanced Application Compatibility Support */

		/*
		 * release memory allocated to save ioctl state 
		 */
		kmem_free(iocstatep, sizeof(struct strioctlstate));
	}

	return;
}


/* 
 * int
 * stri386ioctl(struct vnode *, int *, int, int *, int *)
 *
 * Calling/Exit State:
 *	- Assumes no locks are held on entry/exit.
 *	- Return 0 if we did not service the ioctl, 1 if we did.
 *
 * Description:
 *	386-specific ioctl handler for redirection of ioctls from /dev/vt*
 *	to associated video memory device (/dev/kdvm00 for /dev/vt00, etc.)
 *	This routine must be defined for anyone using the STREAMS-based
 *	keyboard/display driver.
 */
/* ARGSUSED */
int 
stri386ioctl(struct vnode *vp, int *cmdp, int arg, int *rvalp, int *errorp)
{
        dev_t		dev = 0;
	int		cmd = *cmdp;
	major_t		majnum;
	minor_t		minnum;
	int		oldpri;
 	extern gvid_t	Gvid;
 	extern int	gvidflg;
	extern lock_t	*gvid_mutex;
	extern sv_t	*gvidsv;
 

	if (vp != NULL) {

		/*
		 * we are called FROM strioctl and NOT from the KD driver
		 */

        	dev = vp->v_rdev;
		*errorp = 0;

	 	if (!(gvidflg & GVID_SET)) 
			/* cannot deal with ioctl for the moment. */
 			return (0); 

 		dev = vp->v_rdev;
 		majnum = getmajor(dev);
 
		oldpri = LOCK(gvid_mutex, plhi);

 		while (gvidflg & GVID_ACCESS) { /* sleep */
			if (!SV_WAIT_SIG(gvidsv, PRIMED, gvid_mutex)) {
 				*errorp = EINTR;
				/*
 				 * even if ioctl was not ours, we've
 			 	 * effectively handled it 
				 */
 				return (1);
 			}
			oldpri = LOCK(gvid_mutex, plhi);
		}

		gvidflg |= GVID_ACCESS; 
	
 		/* return if ioctl meant for someone else */
 		if (majnum != Gvid.gvid_maj) {
			gvidflg &= ~GVID_ACCESS; 
			SV_SIGNAL(gvidsv, 0);
			UNLOCK(gvid_mutex, oldpri);
			return (0);
		}

 		minnum = getminor(dev);
	 	if (minnum >= Gvid.gvid_num) {
 			*errorp = ENXIO;
			gvidflg &= ~GVID_ACCESS; 
			SV_SIGNAL(gvidsv, 0);
			UNLOCK(gvid_mutex, oldpri);
			/* this technically shouldn't happen */
 			return (1);
 		}
 
		/* done with lookup in Gvid. Release access to it */
		gvidflg &= ~GVID_ACCESS; 
		SV_SIGNAL(gvidsv, 0);
		UNLOCK(gvid_mutex, oldpri);
	}


	/*
	 * screen out ioctls that we know do not get redirected 
	 */

	switch (cmd & 0xffffff00) {
	/* Enhanced Application Compatibility Support */
	case S_IOC:		/* 'X'<<8 */
		if (vp == NULL) 
			return(1);
		break;

	case O_MODESWITCH:	/* 'S'<<8 */
		*cmdp = remap_streams_ioctls(dev, cmd);
		break;

	case USL_OLD_MODESWITCH:  /* 'x'<<8 */
		*cmdp = remap_posix_ioctls(dev, cmd);
		break;

	case LDEV_MOUSE:
	case EVLD_IOC:
	case SCO_C_IOC:
	/* End Enhanced Application Compatibility Support */
	case KIOC:
	case VTIOC:
	case MODESWITCH:
	case CGAIOC:
	case EGAIOC:
	case MCAIOC:
	case PGAIOC:
	case CONSIOC:	/* also handles GIO_COLOR */
	case MAPADAPTER:
	case MIOC:
	case WSIOC:
	case GIO_ATTR:
		break;

	default:	/* not redirected */
		if (vp == NULL)
			return (1); 
		return (0);
	}

	if (vp == NULL)
		return(0); 

 	switch (*cmdp) {
	case WS_GETSYSVCOMPAT:
		*rvalp = ws_sysv_iscompatset(dev, arg);
		return (1);

	case WS_CLRSYSVCOMPAT:
		ws_sysv_clrcompatflgs(dev, arg);
		return (1);

	case WS_SETSYSVCOMPAT:
		ws_sysv_setcompatflgs(dev, arg);
		return (1);

	case WS_SETXXCOMPAT:
		ws_setcompatflgs(dev);
		return (1);
 
	case WS_CLRXXCOMPAT:
		ws_clrcompatflgs(dev);
		return (1);
 
	case WS_GETXXCOMPAT:
		*rvalp = ws_iscompatset(dev);
		return (1);
 
	case XENIX_SPECIAL_IOPRIVL: 
		/* resolve conflict with new TERMIOS TCGETX, STSET ioctls */
 		if (isXOUT || ws_iscompatset(dev)) {
			*cmdp = SPECIAL_IOPRIVL;
			/*
			 *+ The value of XENIX_SPECIAL_IOPRIVL conflicts
			 *+ conflicts with the new TERMIO TCGETX, STSET.
			 */
			cmn_err(CE_NOTE, 
				"redirecting XENIX_SPECIAL_IOPRIVL");
		}
		break;

 	} /* switch */
 
	return (0);
}

/* Enhanced Application Compatibility Support */

/*
 * STATIC int
 * modeswitch3_2(int)
 *	Remap SVR3.2 video mode ioctls to SVR4 video mode ioctls.
 *
 * Calling/Exit State:
 *	Return the new ioctl command.
 */
STATIC int
modeswitch3_2(int cmd)
{
static struct {
	int	svr4_val;
	} modeswitch_tbl[] = {
/*SVR4 */	/*USL SVR3.2 */ 
DM_B40x25,	/* 0 */		/* 40x25 black & white text */
DM_C40x25,	/* 1 */		/* 40x25 color text */
DM_B80x25,	/* 2 */		/* 80x25 black & white text */
DM_C80x25,	/* 3 */		/* 80x25 color text */
DM_BG320,	/* 4 */		/* 320x200 black & white graphics */
DM_CG320,	/* 5 */		/* 320x200 color graphics */
DM_BG640,	/* 6 */		/* 640x200 black & white graphics */
DM_EGAMONO80x25,/* 7 */		/* EGA mode 7 */
LOAD_COLOR,	/* 8 */		/* mode for loading color characters */
LOAD_MONO,	/* 9 */		/* mode for loading mono characters */
DM_ENH_B80x43,	/* 10 */	/* 80x43 black & white text */
DM_ENH_C80x43,	/* 11 */	/* 80x43 color text */
DM_ENH_C80x43,	/* 12 */	/* Not in SVR3.2. 80x43 color text */
DM_CG320_D,	/* 13 */	/* EGA mode D */
DM_CG640_E,	/* 14 */	/* EGA mode E */
DM_EGAMONOAPA,	/* 15 */	/* EGA mode F */
DM_CG640x350,	/* 16 */	/* EGA mode 10 */
DM_ENHMONOAPA2,	/* 17 */	/* EGA mode F with extended memory */
DM_ENH_CG640,	/* 18 */	/* EGA mode 10* */
DM_ENH_B40x25,	/* 19 */	/* enhanced 40x25 black & white text */
DM_ENH_C40x25,	/* 20 */	/* enhanced 40x25 color text */
DM_ENH_B80x25,	/* 21 */	/* enhanced 80x25 black & white text */
DM_ENH_C80x25,	/* 22 */	/* enhanced 80x25 color text */
DM_ENH_CGA,	/* 23 */	/* AT&T 640x400 CGA hw emulation mode */
DM_ATT_640,	/* 24 */	/* AT&T 640x400 16 color graphics */
DM_VGA_C40x25,	/* 25 */	/* VGA 40x25 color text */
DM_VGA_C80x25,	/* 26 */	/* VGA 80x25 color text */
DM_VGAMONO80x25,/* 27 */	/* VGA mode 7 */
DM_VGA640x480C,	/* 28 */	/* VGA 640x480 2 color graphics */
DM_VGA640x480E,	/* 29 */	/* VGA 640x480 16 color graphics */
DM_VGA320x200,	/* 30 */	/* VGA 320x200 256 color graphics */
DM_VDC800x600E,	/* 31 */	/* VDC-600 800x600 16 color graphics */
DM_VDC640x400V	/* 32 */	/* VDC-600 640x400 256 color graphics */
};
	int	newcmd;

	if ((cmd & 0xff) > 32)
		return(cmd);
	newcmd = MODESWITCH | modeswitch_tbl[(cmd & 0xff)].svr4_val;
	return(newcmd);
}


/*
 * STATIC int
 * remap_streams_ioctls(dev_t, int)
 *	Translate SCO exec AND SCO Old XIOC termios to the BSC version.
 *
 * Calling/Exit State:
 *	Return the new ioctl command.
 */
STATIC int
remap_streams_ioctls(dev_t dev, int cmd)
{
	int	newcmd;


	if (VIRTUAL_XOUT) {
		if (ws_sysv_iscompatset(dev, 4))
			newcmd = STR | (cmd & 0xff);
		else	
			newcmd = MODESWITCH | (cmd & 0xff);
	} else if (isSCO) {
		if (ws_sysv_iscompatset(dev, 3) || ws_sysv_iscompatset(dev, 4))
			newcmd = STR | (cmd & 0xff);
		else	
			newcmd = MODESWITCH | (cmd & 0xff);
	} else { /* SVR4 */
		if (ws_sysv_iscompatset(dev, 3))
			newcmd = modeswitch3_2(cmd);
		else if (ws_sysv_iscompatset(dev, 4) || ws_iscompatset(dev))
			newcmd = MODESWITCH | (cmd & 0xff);
		else	
			newcmd = STR | (cmd & 0xff);
	}

	return(newcmd);
}


/*
 * STATIC int
 * remap_posix_ioctls(dev_t, int)
 *	Translate ISC POSIX exec to SVR4 exec.
 * 
 * Calling/Exit State:
 *	Return the new ioctl command.
 */
STATIC int
remap_posix_ioctls(dev_t dev, int cmd)
{
	int	newcmd;


	if (VIRTUAL_XOUT) {
		newcmd = (cmd & ~IOCTYPE) | SCO_XIOC;
	} else if (isSCO) {
		if (ws_isvdcset(dev) || ws_sysv_iscompatset(dev, 3))
			newcmd = modeswitch3_2(cmd);
 		else if (ws_iscompatset(dev))
			newcmd = (cmd & ~IOCTYPE) | SCO_XIOC;
		else
			newcmd = MODESWITCH | (cmd & 0xff);
	} else { /* SVR4 */
		if (ws_sysv_iscompatset(dev, 3))
			newcmd = modeswitch3_2(cmd);
		else if (ws_iscompatset(dev))
			newcmd = (cmd & ~IOCTYPE) | SCO_XIOC;
		else
			newcmd = MODESWITCH | (cmd & 0xff);
	}

	return(newcmd);
}

/* End Enhanced Application Compatibility Support */
