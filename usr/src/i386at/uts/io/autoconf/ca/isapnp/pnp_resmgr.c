/*
 *	@(#)pnp_resmgr.c	7.2	1/20/98	13:24:06
 */

/* Interface between PnP driver and the Res Managaer */

#ifdef UNIXWARE		/* All code in this module is UNIXWARE-only */

			/* Common include files */
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/cmn_err.h"
#include "sys/bootinfo.h"
#include "sys/errno.h"
#include "sys/param.h"
#include "sys/kmem.h"
#include "sys/immu.h"

#ifdef UNIXWARE		/* UW/GEMINI specific includes */
# include "sys/cred.h"
# include "sys/confmgr.h"
# include "sys/cm_i386at.h"
# include "sys/conf.h"
# include "sys/ddi.h"
#else			/* OpenServer specific includes */
# include "sys/arch.h"
# include "sys/strg.h"
# include "sys/ci/cilock.h"
# include "sys/seg.h"
#endif

#ifdef PNP_DEBUG
#   include <sys/xdebug.h>
#endif

#include "sys/pnp.h"
#include "space.h"
#include "pnp_private.h"
#include "pnp_bios.h"


PnP_resSpec_t	*PnP_assign = NULL;

STATIC int	PnP_assignSize = 0;


/*
** Guarantee that PnP_assign is free.
*/
STATIC void
PnP_FreeAssignmentTable(void)
{
	if(!PnP_assign)
		return;

	kmem_free(PnP_assign, sizeof(PnP_resSpec_t)*(PnP_assignSize));

	PnP_assign = NULL;
}



/*
** This function reads a PNP resource from a specific key in the
** res manager.
**
** It returns a pointer to a static resSpec structure which is overwritten
** on each call.
**
** If key is invalid, the entry in the res manager does not contain any PnP
** resource information, or offset is out of range, a null pointer is returned.
*/
STATIC PnP_resSpec_t *
PnP_ReadResMgrKey(rm_key_t key, int offset)
{
	cm_args_t		cma;
	static PnP_resSpec_t	res;
	int			ret = 0;
	int			valueIsRange = 0;
	static long		vendor_l;

			/* Table of PNP parameters to read from res mgr */
	struct {
		char *param;
		void *target;
		size_t len;
	} plist[] = {
		{ CM_PNPTYPE,	&(res.type),	sizeof(res.type)	},
		{ CM_PNPVENDOR,	&(vendor_l),	sizeof(vendor_l)	},
		{ CM_PNPSERNO,	&(res.serial),	sizeof(res.serial)	},
		{ CM_PNPDEVNO,	&(res.devNum),	sizeof(res.devNum)	},
		{ CM_PNPLIMIT,	&(res.limit),	sizeof(res.limit)	},
		{ NULL, NULL, 0 }
	}, *pindex;


	/* obtain lock on this entry in the res mgr. */
	cm_begin_trans(key, RM_READ);

	for(pindex=plist; pindex->len; ++pindex)
		{
		/* n will be offset for CM_PNP_TYPE, 0 otherwise */
		cma.cm_n = (pindex == plist) ? offset : 0;
		cma.cm_key = key;
		cma.cm_param = pindex->param;
		cma.cm_val = pindex->target;
		cma.cm_vallen = pindex->len;

		ret = cm_getval(&cma);

		if(ret == EINVAL)			/* key out of range */
			{
			cm_end_trans(key);
			PNP_DPRINT(CE_CONT, "<key out of range>");
			return((PnP_resSpec_t *)0);
			}

		if(ret == ENOENT && pindex==plist)	/* "n" is invalid */
			{
			/* bail out silently */
			cm_end_trans(key);
			return((PnP_resSpec_t *)0);
			}

		if(ret != 0)				/* No such parameter */
			{
			res.type = PNP_NONE;
			cm_end_trans(key);

			if(pindex != plist)	/* partial PNP entry */
				PNP_DPRINT(CE_CONT, "%s: key %d missing %s #%d",
						PnP_name, key, pindex->param,
						cma.cm_n);

			PNP_DPRINT(CE_CONT, "<no usable PNP info>");
			return((PnP_resSpec_t *)0);
			}
		}

	/*
	** Now we've got everything we need except for the actual
	** value of the reosurce which will be in one of several
	** places depending on res.type.
	*/
	switch(res.type)
		{
		case PNP_IRQ_0:		/* value is in IRQ field */
		case PNP_IRQ_1:
			cma.cm_n = (res.type & 0x0f);
			cma.cm_param = CM_IRQ;
			break;

		case PNP_DMA_0:		/* value is in DMAC field */
		case PNP_DMA_1:
			cma.cm_n = (res.type & 0x0f);
			cma.cm_param = CM_DMAC;
			break;

		case PNP_IO_0:		/* value is in IOADDR field */
		case PNP_IO_1:
		case PNP_IO_2:
		case PNP_IO_3:
		case PNP_IO_4:
		case PNP_IO_5:
		case PNP_IO_6:
		case PNP_IO_7:
			cma.cm_n = (res.type & 0x0f);
			cma.cm_param = CM_IOADDR;
			valueIsRange = 1;
			break;

		case PNP_MEM_0:		/* value is in MEMADDR field */
		case PNP_MEM_1:
		case PNP_MEM_2:
		case PNP_MEM_3:
		case PNP_MEM32_0:
		case PNP_MEM32_1:
		case PNP_MEM32_2:
		case PNP_MEM32_3:
			cma.cm_n = (res.type & 0x0f);
			cma.cm_param = CM_MEMADDR;
			break;

		case PNP_DISABLE:
		default:
			cma.cm_param = NULL;	/* don't read value */
			break;
		}


	/*** Read the value from the res mgr ***/

	/*
	** We have to treat IOADDR differently since it is stored
	** as a cm_range_t in the res mgr and as a long in resSpec.
	*/
	if(valueIsRange)
		{
		cm_range_t ioaddr;

		cma.cm_val = &ioaddr;
		cma.cm_vallen = sizeof(ioaddr);

		ret=cm_getval(&cma);

		/* Put the start address into res */
		res.value = ioaddr.startaddr;
		}

	else if(cma.cm_param != NULL)
		{
		cma.cm_val = &(res.value);
		cma.cm_vallen = sizeof(res.value);

		ret=cm_getval(&cma);
		}

	else		/* We don't care what the value is. */
		ret=0;	/* Automatic success */

	/* Release lock on this res mgr key */
	cm_end_trans(key);

	if(ret != 0)
		{
		cmn_err(CE_WARN, "%s: Corrupt res mgr entry key=%d offset=%d",
					PnP_name, key, offset);
		cmn_err(CE_CONT, "%s: Can't read %s #%d",
					PnP_name, cma.cm_param, cma.cm_n);

		return((PnP_resSpec_t *)0);
		}
	else
        	/* res.vendor is unsigned long, resmgr can only save signed long. Convert.*/
        	if (vendor_l < 0) res.vendor=vendor_l+0xffffffff;
		else res.vendor=vendor_l;
		PNP_DPRINT(CE_CONT,
		"%s: serial=0x%8.8x devNum=%d res.type=%d",
		PnP_idStr(res.vendor), res.serial, res.devNum, res.type);


	/* return a pointer to our static data structure */
	return(&res);
}



/*
** Fill in the array PnP_assign
*/
STATIC void
PnP_ReadResMgr(void)
{
	/*
	** We're dealing with a dynamic list that we want to treat
	** as an array.  So we'll create a linked list and then
	** collapse it.
	*/

	typedef struct rrmnode {
		struct rrmnode *next;
		PnP_resSpec_t data;
	} rrmnode;


	rrmnode		*first = NULL, **nptr = &first, *iter, *nextiter;
	PnP_resSpec_t	*dp, *resdata;

	int		devn, resn;	/* Indices into 2d table: dev X res */

	cm_args_t	deva;		/* For calling cm_getval() */

	int		devret=0;	/* Return val from cm_getval() */

	int		assign;		/* Index into assign table */

	rm_key_t	pnpkey, devkey;
	unsigned long	devunit;

	char		modname[16];	/* Modnames better not get longer */


	/* Make sure any old table is gone */
	PnP_FreeAssignmentTable();


	/* Lock PnP entry in Res Mgr */
	pnpkey = cm_getbrdkey(PnP_name, 0);

	if(pnpkey != RM_NULL_KEY)
		cm_begin_trans(pnpkey, RM_READ);
	else
		cmn_err(CE_WARN, "%s: No %s key in res mgr.",
						PnP_name, PnP_name);

	/* Build linked list */
	if(pnpkey != RM_NULL_KEY)
	  for(devn=0; 1; ++devn)
	    {
	    /* Read the module name of the next PnP device */
	    deva.cm_n = devn;
	    deva.cm_key = pnpkey;
	    deva.cm_param = CM_PNPMODNAME;
	    deva.cm_val = modname;
	    deva.cm_vallen = sizeof modname;

	    if((devret = cm_getval(&deva)) != 0)
		break;

	    /* Read the unit number of the next PnP device */
	    deva.cm_param = CM_PNPUNIT;
	    deva.cm_val = &devunit;
	    deva.cm_vallen = sizeof(devunit);

	    if((devret = cm_getval(&deva)) != 0)
		break;

	    PNP_DPRINT(CE_CONT, "%s: Getting resmgr info for %s #%d",
						PnP_name, modname, devunit);

	    /* Get the rm key for the next PnP device */
	    devkey = cm_getbrdkey(modname, devunit);

	    if(devkey != RM_NULL_KEY)
	      for(resn=0; resdata = PnP_ReadResMgrKey(devkey, resn); ++resn)
		{
		/* Add a new node to the linked list */
		*nptr = kmem_zalloc(sizeof(rrmnode), KM_SLEEP);
		dp = &((*nptr)->data);
		nptr = &((*nptr)->next);
		++PnP_assignSize;

		/* Copy in the information */
		bcopy(resdata, dp, sizeof(PnP_resSpec_t));
		}
	    }

	/* Make space for the terminating null entry */
	++PnP_assignSize;

	/* Release lock on PnP key */
	if(pnpkey != RM_NULL_KEY)
		cm_end_trans(pnpkey);

	/* Flatten out the linked list into an array */
	PnP_assign = kmem_zalloc(sizeof(PnP_resSpec_t)*(PnP_assignSize),
				KM_SLEEP);

	/* Copy our linked list into the array, freeing as we go */
	for(assign = 0, iter = first; iter; ++assign, iter=nextiter)
		{
		bcopy(&(iter->data), &(PnP_assign[assign]),
						sizeof(PnP_resSpec_t));
		nextiter = iter->next;
		kmem_free(iter, sizeof(rrmnode));
		}

	/* Put in final entry */
	PnP_assign[assign].type = PNP_NONE;
}


#endif	/* UNIXWARE */
