#ident	"@(#)kern-pdi:io/target/sdi/sdi_bind.c	1.15.6.1"
#ident	"$Header$"

/*
 * Protection for uniprocessor HBA drivers.
 */

#include <io/target/sdi/sdi.h>
#include <mem/kmem.h>
#include <proc/bind.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/types.h>

STATIC	long		sdi_bind_freeblk();
STATIC	struct hbadata	*sdi_bind_getblk();
STATIC	long		sdi_bind_icmd();
STATIC	void		sdi_bind_getinfo();
STATIC	long		sdi_bind_send();
STATIC	int		sdi_bind_xlat();
STATIC	int		sdi_bind_open();
STATIC	int		sdi_bind_close();
STATIC	int		sdi_bind_ioctl();

struct	shad_hbatbl	*sdi_shad_hbatbl;

struct	hba_info sdi_hbabind_info = {
	NULL, 0,
	sdi_bind_freeblk,
	sdi_bind_getblk,
	sdi_bind_icmd,
	sdi_bind_getinfo,
	sdi_bind_send,
	sdi_bind_xlat,
	sdi_bind_open,
	sdi_bind_close,
	sdi_bind_ioctl
};

/*
 * STATIC	long
 * sdi_bind_freeblk(struct hbadata *hbap, int ha)
 *
 * Calling/Exit State:
 *	None
 */
STATIC	long
sdi_bind_freeblk(struct hbadata *hbap, int ha)
{
	struct shad_hbatbl *shbap;
#ifndef	UNIPROC
	engine_t *oldengp;
#endif	/* !UNIPROC */
	long retval;

	shbap = &sdi_shad_hbatbl[ha];

	if (servicing_interrupt() || sdi_sleepflag == KM_NOSLEEP) {
		DISABLE_PRMPT();
		retval = (*shbap->info->hba_freeblk)(hbap);
		ENABLE_PRMPT();
		return (retval);
	}

#ifndef	UNIPROC
	oldengp = kbind(shbap->enginep);
#endif	/* !UNIPROC */
	DISABLE_PRMPT();

	if (u.u_lwpp != NULL) {
		u.u_lwpp->l_notrt++;
		retval = (*shbap->info->hba_freeblk)(hbap);
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
	} else
		retval = (*shbap->info->hba_freeblk)(hbap);

	ENABLE_PRMPT();
#ifndef	UNIPROC
	kunbind(oldengp);
#endif	/* !UNIPROC */

	return (retval);
}

/*
 * STATIC struct hbadata *
 * sdi_bind_getblk(int ha)
 *
 * Calling/Exit State:
 *	None
 */
STATIC	struct	hbadata *
sdi_bind_getblk(int sleepflag, int ha)
{
	struct shad_hbatbl *shbap;
#ifndef	UNIPROC
	engine_t *oldengp;
#endif	/* !UNIPROC */
	struct hbadata *retval;

	shbap = &sdi_shad_hbatbl[ha];

	if (servicing_interrupt() || sdi_sleepflag == KM_NOSLEEP) {
		DISABLE_PRMPT();
		retval = (*shbap->info->hba_getblk)(sleepflag);
		ENABLE_PRMPT();
		return (retval);
	}

#ifndef	UNIPROC
	oldengp = kbind(shbap->enginep);
#endif	/* !UNIPROC */
	DISABLE_PRMPT();

	if (u.u_lwpp != NULL) {
		u.u_lwpp->l_notrt++;
		retval = (*shbap->info->hba_getblk)(sleepflag);
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
	} else
		retval = (*shbap->info->hba_getblk)(sleepflag);

	ENABLE_PRMPT();
#ifndef	UNIPROC
	kunbind(oldengp);
#endif	/* !UNIPROC */

	return (retval);
}

/*
 * STATIC	long
 * sdi_bind_icmd(struct hbadata *hba, int sleepflag)
 *
 * Calling/Exit State:
 *	None
 */
STATIC	long
sdi_bind_icmd(struct hbadata *hba, int sleepflag)
{
	int	ha;
	struct shad_hbatbl *shbap;
	int pdi_version;
#ifndef	UNIPROC
	engine_t *oldengp;
#endif	/* !UNIPROC */
	long retval;

	ASSERT(hba->sb->sb.sb_type == SFB_TYPE ||
	       hba->sb->sb.sb_type == SCB_TYPE ||
	       hba->sb->sb.sb_type == ISCB_TYPE);

	switch(hba->sb->sb.sb_type)	{
		case SFB_TYPE:
			pdi_version = hba->sb->sb.SFB.sf_dev.pdi_adr_version;
			if (! pdi_version)
				ha = SDI_EXHAN(&(hba->sb->sb.SFB.sf_dev));
			else
				ha = SDI_HAN_32(&(hba->sb->sb.SFB.sf_dev));
			break;

		case SCB_TYPE:
		case ISCB_TYPE:
			pdi_version =  hba->sb->sb.SCB.sc_dev.pdi_adr_version;
			if (! pdi_version)
				ha = SDI_EXHAN(&(hba->sb->sb.SCB.sc_dev));
			else
				ha = SDI_HAN_32(&(hba->sb->sb.SCB.sc_dev));
			break;
	}

	shbap = &sdi_shad_hbatbl[ha];

	if (servicing_interrupt() || sdi_sleepflag == KM_NOSLEEP) {
		DISABLE_PRMPT();
		retval = (*shbap->info->hba_icmd)(hba, sleepflag);
		ENABLE_PRMPT();
		return (retval);
	}

#ifndef	UNIPROC
	oldengp = kbind(shbap->enginep);
#endif	/* !UNIPROC */
	DISABLE_PRMPT();
	if (u.u_lwpp != NULL) {
		u.u_lwpp->l_notrt++;
		retval = (*shbap->info->hba_icmd)(hba, sleepflag);
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
	} else
		retval = (*shbap->info->hba_icmd)(hba, sleepflag);

	ENABLE_PRMPT();
#ifndef	UNIPROC
	kunbind(oldengp);
#endif	/* !UNIPROC */

	return (retval);
}

/*
 * STATIC	void
 * sdi_bind_getinfo(struct scsi_ad *sa, struct hbagetinfo *getinfo)
 *
 * Calling/Exit State:
 *	None
 */
STATIC	void
sdi_bind_getinfo(struct scsi_ad *sa, struct hbagetinfo *getinfo)
{
	int	ha;
	struct shad_hbatbl *shbap;
#ifndef	UNIPROC
	engine_t *oldengp;
#endif	/* !UNIPROC */

	if (! sa->pdi_adr_version )
		ha = SDI_EXHAN(sa);
	else
		ha = SDI_HAN_32(sa);
	shbap = &sdi_shad_hbatbl[ha];

	if (servicing_interrupt() || sdi_sleepflag == KM_NOSLEEP) {
		DISABLE_PRMPT();
		(*shbap->info->hba_getinfo)(sa, getinfo);
		ENABLE_PRMPT();
		return;
	}

#ifndef	UNIPROC
	oldengp = kbind(shbap->enginep);
#endif	/* !UNIPROC */
	DISABLE_PRMPT();
	if (u.u_lwpp != NULL) {
		u.u_lwpp->l_notrt++;
		(*shbap->info->hba_getinfo)(sa, getinfo);
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
	} else
		(*shbap->info->hba_getinfo)(sa, getinfo);

	ENABLE_PRMPT();
#ifndef	UNIPROC
	kunbind(oldengp);
#endif	/* !UNIPROC */
}

/*
 * STATIC	long
 * sdi_bind_send(struct hbadata *hba)
 *
 * Calling/Exit State:
 *	None
 */
STATIC	long
sdi_bind_send(struct hbadata *hba, int sleepflag)
{
	int	ha;
	struct shad_hbatbl *shbap;
	int pdi_version;
#ifndef	UNIPROC
	engine_t *oldengp;
#endif	/* !UNIPROC */
	long retval;

	pdi_version = hba->sb->sb.SCB.sc_dev.pdi_adr_version;

	if (!pdi_version)
		ha = SDI_EXHAN(&(hba->sb->sb.SCB.sc_dev));
	else
		ha = SDI_HAN_32(&(hba->sb->sb.SCB.sc_dev));

	shbap = &sdi_shad_hbatbl[ha];

	if (servicing_interrupt() || sdi_sleepflag == KM_NOSLEEP) {
		DISABLE_PRMPT();
		retval = (*shbap->info->hba_send)(hba, sleepflag);
		ENABLE_PRMPT();
		return (retval);
	}

#ifndef	UNIPROC
	oldengp = kbind(shbap->enginep);
#endif	/* !UNIPROC */
	DISABLE_PRMPT();
	if (u.u_lwpp != NULL) {
		u.u_lwpp->l_notrt++;
		retval = (*shbap->info->hba_send)(hba, sleepflag);
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
	} else
		retval = (*shbap->info->hba_send)(hba, sleepflag);

	ENABLE_PRMPT();
#ifndef	UNIPROC
	kunbind(oldengp);
#endif	/* !UNIPROC */

	return (retval);
}

/*
 * STATIC	int
 * sdi_bind_xlat(struct hbadata *hba, int flag, struct proc *procp, int sleepflag)
 *
 * Calling/Exit State:
 *	None
 */
STATIC	int
sdi_bind_xlat(struct hbadata *hba, int flag, struct proc *procp, int sleepflag)
{
	int	ha;
	struct shad_hbatbl *shbap;
	int retval;
	int pdi_version;
#ifndef	UNIPROC
	engine_t *oldengp;
#endif	/* !UNIPROC */

	ASSERT(hba->sb->sb.sb_type == SFB_TYPE ||
	       hba->sb->sb.sb_type == SCB_TYPE ||
	       hba->sb->sb.sb_type == ISCB_TYPE);

	switch(hba->sb->sb.sb_type)	{
		case SFB_TYPE:
			pdi_version = hba->sb->sb.SFB.sf_dev.pdi_adr_version;
			if (! pdi_version)
				ha = SDI_EXHAN(&(hba->sb->sb.SFB.sf_dev));
			else
				ha = SDI_HAN_32(&(hba->sb->sb.SFB.sf_dev));
			break;

		case SCB_TYPE:
		case ISCB_TYPE:
			pdi_version = hba->sb->sb.SCB.sc_dev.pdi_adr_version;
			if (! pdi_version)
				ha = SDI_EXHAN(&(hba->sb->sb.SCB.sc_dev));
			else
				ha = SDI_HAN_32(&(hba->sb->sb.SCB.sc_dev));
			break;
	}

	shbap = &sdi_shad_hbatbl[ha];

	if (servicing_interrupt() || sdi_sleepflag == KM_NOSLEEP) {
		DISABLE_PRMPT();
		retval = (*shbap->info->hba_xlat)(hba, flag, procp, sleepflag);
		ENABLE_PRMPT();
	}
	else {
#ifndef	UNIPROC
		oldengp = kbind(shbap->enginep);
#endif	/* !UNIPROC */
		DISABLE_PRMPT();
		if (u.u_lwpp != NULL) {
			u.u_lwpp->l_notrt++;
			retval = (*shbap->info->hba_xlat)(hba, flag, procp,
							sleepflag);
			ASSERT(u.u_lwpp->l_notrt != 0);
			u.u_lwpp->l_notrt--;
		} else
			retval = (*shbap->info->hba_xlat)(hba, flag, procp,
							sleepflag);
		ENABLE_PRMPT();
#ifndef	UNIPROC
		kunbind(oldengp);
#endif	/* !UNIPROC */
	}
	return (retval);
}

/*
 * STATIC	int
 * sdi_bind_open(dev_t *devp, int flags, int otype, cred_t *credp)
 *
 * Calling/Exit State:
 *	None
 */
STATIC	int
sdi_bind_open(dev_t *devp, int flags, int otype, cred_t *credp)
{
	int	ha;
	struct shad_hbatbl *shbap;
#ifndef	UNIPROC
	engine_t *oldengp;
#endif	/* !UNIPROC */
	int retval;
	label_t saveq;

	ha = SC_EXHAN(*devp);

	shbap = &sdi_shad_hbatbl[ha];

#ifndef	UNIPROC
	oldengp = kbind(shbap->enginep);
#endif	/* !UNIPROC */
	DISABLE_PRMPT();
	if (u.u_lwpp != NULL)
		u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		retval = EINTR;
	}
	else {
		retval = (*shbap->info->hba_open)(devp, flags, otype, credp);
	}

	u.u_qsav = saveq;
	ASSERT(u.u_lwpp == NULL || u.u_lwpp->l_notrt != 0);
	if (u.u_lwpp != NULL)	
		u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
#ifndef	UNIPROC
	kunbind(oldengp);
#endif	/* !UNIPROC */

	return (retval);
}

/*
 * STATIC	int
 * sdi_bind_close(dev_t dev, int flags, int otype, cred_t *credp)
 *
 * Calling/Exit State:
 *	None
 */
STATIC	int
sdi_bind_close(dev_t dev, int flags, int otype, cred_t *credp)
{
	int	ha;
	struct shad_hbatbl *shbap;
#ifndef	UNIPROC
	engine_t *oldengp;
#endif	/* !UNIPROC */
	int retval;
	label_t saveq;

	ha = SC_EXHAN(dev);

	shbap = &sdi_shad_hbatbl[ha];

#ifndef	UNIPROC
	oldengp = kbind(shbap->enginep);
#endif	/* !UNIPROC */
	DISABLE_PRMPT();
	if (u.u_lwpp != NULL)
		u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		retval = EINTR;
	}
	else {
		retval = (*shbap->info->hba_close)(dev, flags, otype, credp);
	}

	u.u_qsav = saveq;
	ASSERT(u.u_lwpp == NULL || u.u_lwpp->l_notrt != 0);
	if (u.u_lwpp != NULL)
		u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
#ifndef	UNIPROC
	kunbind(oldengp);
#endif	/* !UNIPROC */

	return (retval);
}

/*
 * STATIC	int
 * sdi_bind_ioctl(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *credp,
 * int *rval_p)
 *
 * Calling/Exit State:
 *	None
 */
STATIC	int
sdi_bind_ioctl(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *credp, int *rval_p)
{
	int	ha;
	struct shad_hbatbl *shbap;
#ifndef	UNIPROC
	engine_t *oldengp;
#endif	/* !UNIPROC */
	int retval;
	label_t saveq;

	ha = SC_EXHAN(dev);

	shbap = &sdi_shad_hbatbl[ha];

#ifndef	UNIPROC
	oldengp = kbind(shbap->enginep);
#endif	/* !UNIPROC */
	DISABLE_PRMPT();
	if (u.u_lwpp != NULL)
		u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		retval = EINTR;
	}
	else {
		retval = (*shbap->info->hba_ioctl)(dev, cmd, arg, mode, credp, rval_p);
	}

	u.u_qsav = saveq;
	ASSERT(u.u_lwpp == NULL || u.u_lwpp->l_notrt != 0);
	if (u.u_lwpp != NULL)
		u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
#ifndef	UNIPROC
	kunbind(oldengp);
#endif	/* !UNIPROC */

	return (retval);
}
