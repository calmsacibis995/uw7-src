/*
 *	@(#)ioctl.c	7.1	10/22/97	12:29:00
 * File ioctl.c for the Plug-and-Play driver
 *
 * @(#) ioctl.c 65.4 97/07/11 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/cmn_err.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/file.h>
#ifdef UNIXWARE
# include <sys/cred.h>
#else
# include <sys/ci/cilock.h>
#endif

#include "sys/pnp.h"
#include "space.h"
#include "pnp_private.h"

ENTRY_RETVAL
#ifdef UNIXWARE
PnPioctl(dev_t dev, int cmd, faddr_t arg, int mode, cred_t *crp, int *rvalp)
#else
PnPioctl(dev_t dev, int cmd, faddr_t arg, int mode)
#endif
{
    int			s;
    const PnP_device_t	*dp;

    switch (cmd)
    {
	case PNP_BUS_PRESENT:
	    {
		PnP_busdata_t	bus;
		int		err;

		if (!(mode & FREAD))
		{
		    ENTRY_ERR(EPERM);
		    break;
		}

		bzero(&bus, sizeof(bus));
		if ((err = PnP_bus_data(&bus)) != 0)
		{
		    ENTRY_ERR(err);
		    break;
		}

		if (copyout(&bus, arg, sizeof(bus)) == -1)
		    ENTRY_ERR(EFAULT);
	    }
	    break;

	case PNP_FIND_UNIT:
	    {
		PnP_findUnit_t		unit;

		if (!(mode & FREAD))
		{
		    ENTRY_ERR(EPERM);
		    break;
		}

		if (copyin(arg, &unit, sizeof(unit)) == -1)
		{
		    ENTRY_ERR(EFAULT);
		    break;
		}

		PNP_LOCK(s);
		if (!(dp = PnP_FindUnit(&unit.unit)))
		    ENTRY_ERR(ENODEV);
		else
		{
		    unit.unit.vendor = dp->id.vendor;
		    unit.unit.serial = dp->id.serial;
		    unit.unit.unit = dp->unit;
		    unit.unit.node = dp->id.CSN;

		    unit.NodeSize = dp->NodeSize;
		    unit.ResCnt = dp->ResCnt;
		    unit.devCnt = dp->devCnt;

		    if (copyout(&unit, arg, sizeof(unit)) == -1)
			ENTRY_ERR(EFAULT);
		}
		PNP_UNLOCK(s);
	    }
	    break;

	case PNP_READ_TAG:
	    {
		PnP_TagReq_t		tagReq;
		u_char			*tagPtr;
		u_long			tagLen;
		int			err;


		if (!(mode & FREAD))
		{
		    ENTRY_ERR(EPERM);
		    break;
		}

		if (copyin(arg, &tagReq, sizeof(tagReq)) == -1)
		{
		    ENTRY_ERR(EFAULT);
		    break;
		}

		if (!(tagPtr = tagReq.tagPtr) || (tagReq.tagLen < 1))
		{
		    ENTRY_ERR(EINVAL);	/* We need at least one byte */
		    break;
		}

		tagLen = tagReq.tagLen;
		if (!(tagReq.tagPtr = kmem_zalloc(tagLen, KM_SLEEP|KM_NO_DMA)))
		{
		    ENTRY_ERR(ENOMEM);
		    break;
		}

		if ((err = PnP_ReadTag(&tagReq)) != 0)
		{
		    kmem_free(tagReq.tagPtr, tagLen);
		    ENTRY_ERR(err);
		    break;
		}

		if (copyout(tagReq.tagPtr, tagPtr, tagReq.tagLen) == -1)
		{
		    kmem_free(tagReq.tagPtr, tagLen);
		    ENTRY_ERR(EFAULT);
		    break;
		}

		kmem_free(tagReq.tagPtr, tagLen);
		tagReq.tagPtr = tagPtr;

		if (copyout(&tagReq, arg, sizeof(tagReq)) == -1)
		{
		    ENTRY_ERR(EFAULT);
		    break;
		}
	    }
	    break;

	case PNP_READ_REG:
	    {
		PnP_RegReq_t	regReq;
		u_char		*regBuf;
		u_char		n;

		if (!(mode & FREAD))
		{
		    ENTRY_ERR(EPERM);
		    break;
		}

		if (copyin(arg, &regReq, sizeof(regReq)) == -1)
		{
		    ENTRY_ERR(EFAULT);
		    break;
		}

		if (!regReq.regCnt || !regReq.regBuf)
		{
		    ENTRY_ERR(EINVAL);
		    break;
		}

		if (!(regBuf = kmem_zalloc(regReq.regCnt, KM_SLEEP|KM_NO_DMA)))
		{
		    ENTRY_ERR(ENOMEM);
		    break;
		}

		PNP_LOCK(s);
		if (!(dp = PnP_FindUnit(&regReq.unit)))
		{
		    PNP_UNLOCK(s);
		    kmem_free(regBuf, regReq.regCnt);
		    ENTRY_ERR(ENODEV);
		    break;
		}

		PnP_Wake(dp->id.CSN);
		PnP_writeCmd(PNP_REG_LogicalDevNum, regReq.devNum);

		for (n = 0; n < regReq.regCnt; ++n)
		{
		    outb(PNP_ADDRESS_PORT, regReq.regNum++);
		    regBuf[n] = inb(PnP_dataPort);
		}

		PnP_SetRunState();
		PNP_UNLOCK(s);
		if (copyout(regBuf, regReq.regBuf, regReq.regCnt) == -1)
		    ENTRY_ERR(EFAULT);

		kmem_free(regBuf, regReq.regCnt);
	    }
	    break;

	case PNP_WRITE_REG:
	    {
		PnP_RegReq_t	regReq;
		u_char		*regBuf;
		u_char		n;

		if (!(mode & FWRITE))
		{
		    ENTRY_ERR(EPERM);
		    break;
		}

		if (copyin(arg, &regReq, sizeof(regReq)) == -1)
		{
		    ENTRY_ERR(EFAULT);
		    break;
		}

		if (!regReq.regCnt || !regReq.regBuf)
		{
		    ENTRY_ERR(EINVAL);
		    break;
		}

		if (!(regBuf = kmem_zalloc(regReq.regCnt, KM_SLEEP|KM_NO_DMA)))
		{
		    ENTRY_ERR(ENOMEM);
		    break;
		}

		if (copyin(regReq.regBuf, regBuf, regReq.regCnt) == -1)
		{
		    ENTRY_ERR(EFAULT);
		    break;
		}

		PNP_LOCK(s);
		if (!(dp = PnP_FindUnit(&regReq.unit)))
		{
		    PNP_UNLOCK(s);
		    kmem_free(regBuf, regReq.regCnt);
		    ENTRY_ERR(ENODEV);
		    break;
		}

		PnP_Wake(dp->id.CSN);
		PnP_writeCmd(PNP_REG_LogicalDevNum, regReq.devNum);

		for (n = 0; n < regReq.regCnt; ++n)
		    PnP_writeCmd(regReq.regNum++, regBuf[n]);

		PnP_SetRunState();
		PNP_UNLOCK(s);
		kmem_free(regBuf, regReq.regCnt);
	    }
	    break;

	case PNP_READ_ACTIVE:
	    {
		PnP_Active_t		act;


		if (!(mode & FREAD))
		{
		    ENTRY_ERR(EPERM);
		    break;
		}

		if (copyin(arg, &act, sizeof(act)) == -1)
		{
		    ENTRY_ERR(EFAULT);
		    break;
		}

		PNP_LOCK(s);
		if (!(dp = PnP_FindUnit(&act.unit)))
		    ENTRY_ERR(ENODEV);
		else
		{
		    if (act.device >= dp->devCnt)
			ENTRY_ERR(EINVAL);
		    else
		    {
			act.active = PnP_ReadActive(dp, act.device);
			PnP_SetRunState();

			if (copyout(&act, arg, sizeof(act)) == -1)
			    ENTRY_ERR(EFAULT);
		    }
		}
		PNP_UNLOCK(s);
	    }
	    break;

	case PNP_WRITE_ACTIVE:
	    {
		PnP_Active_t		act;


		if (!(mode & FWRITE))
		{
		    ENTRY_ERR(EPERM);
		    break;
		}

		if (copyin(arg, &act, sizeof(act)) == -1)
		{
		    ENTRY_ERR(EFAULT);
		    break;
		}

		PNP_LOCK(s);
		if (!(dp = PnP_FindUnit(&act.unit)))
		    ENTRY_ERR(ENODEV);
		else
		{
		    if (act.device >= dp->devCnt)
			ENTRY_ERR(EINVAL);
		    else
		    {
			PnP_WriteActive(dp, act.device, act.active);
			act.active = PnP_ReadActive(dp, act.device);
			PnP_SetRunState();

			if (copyout(&act, arg, sizeof(act)) == -1)
			    ENTRY_ERR(EFAULT);
		    }
		}
		PNP_UNLOCK(s);
	    }
	    break;

	default:
	    ENTRY_ERR(EINVAL);
    }

ENTRY_NOERR;
}

