#ident "@(#)v86.c	11.1	10/22/97	11:58:09"
#ident "$Header$"
/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */
/*
 *	Thu May  8 12:38:49 PDT 1997	-	hiramc@sco.COM
 *	- as per Jun's changes, remove the disable_v86bios check
 */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/log.h"

#include "sys/signal.h"		/* needed for sys/user.h */
#include "sys/dir.h"		/* needed for sys/user.h */
#include "sys/seg.h"		/* needed for sys/user.h */

#include "sys/user.h"
#include "sys/errno.h"
#include "sys/conf.h"
#include "sys/debug.h"
#include "sys/cmn_err.h"
#include "sys/mse.h"
#include "sys/resmgr.h"
#include "sys/confmgr.h"
#include "sys/moddefs.h"
#include "sys/ddi.h"

#include "v86.h"

int v86xdevflag = 0;

#ifdef NOT_YET
static int v86x_is_locked = 0;
static pl_t oldprio;
#endif

LKINFO_DECL(v86xlockinfo, "X11R6.1::v86xlock", 0);
static lock_t *v86xlock;
int v86xopen(), v86xclose(), v86xioctl();

#define DRVNAME "v86x BIOS Driver"
static int v86x_load(), v86x_unload();
MOD_DRV_WRAPPER(v86x, v86x_load, v86x_unload, NULL, DRVNAME);

static int
v86x_load()
{
  v86xlock = LOCK_ALLOC(1, plbase, &v86xlockinfo, KM_NOSLEEP);
  return(0);
}

static int
v86x_unload()
{
  LOCK_DEALLOC(v86xlock);
  return(0);
}

v86xopen(dev, flag, otyp, crp)
     dev_t *dev;
     cred_t *crp;
{

  if (crp->cr_uid != 0)
    {
      return (EPERM);
    }
  else
    {
      return (0);
    }
}

v86xclose(dev, flag, otyp, crp)
{
#ifdef NOT_YET
  extern int v86x_is_locked;
  extern pl_t oldprio;

  if (v86x_is_locked)
    UNLOCK(v86xlock, oldprio);
#endif

  return(0);
}


v86xioctl(dev, cmd, arg, mode, crp, rvalp)
     dev_t *dev;
     int cmd, mode;
     caddr_t arg;
     cred_t *crp;
     int *rvalp;
{
  int status = 0;

  switch(cmd)
    {

    case V86_CALLBIOS:          /* Execute BIOS in v86x mode */
    case V86_CALLROM:           /* Call ROM in v86x mode */
      {
        intregs_t regs;

        if(copyin(arg, &regs, sizeof(intregs_t)) == -1)
          {
            status = EFAULT;
            break;
          }

        v86bios(&regs);

        if(copyout(&regs, arg, sizeof(intregs_t)) == -1)
          {
            status = EFAULT;
            break;
          }

        break;                 
      }

#ifdef NOT_YET
    case V86X_LOCK:
      oldprio = LOCK(v86xlock, plbase);
      v86x_is_locked = 1;
      break;

    case V86X_UNLOCK:
      if (v86x_is_locked)
        UNLOCK(v86xlock, oldprio);
      break;
#endif

    default:
      status = EINVAL;
      break;

    } /* End switch cmd */


  return(status);

}
