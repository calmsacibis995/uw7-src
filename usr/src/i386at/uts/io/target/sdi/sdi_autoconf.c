#ident	"@(#)kern-pdi:io/target/sdi/sdi_autoconf.c	1.11.10.1"
#ident	"$Header$"

#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/conf.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/mod.h>

#include <io/ddi.h>

#define	_SDI_AUTOCONF
#include <io/target/sdi/sdi.h>
#undef	_SDI_AUTOCONF

#include <svc/errno.h>

#define SDI_AUTOCONF_DEBUG
#define	MAXDIGIT	20
#define	MAXDESCLEN	128

/*
 * STATIC void
 * sdi_itoa(uint n, char s[])
 *	Internal routine to convert unsigned integer
 *	to ASCII.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
sdi_itoa(uint n, char s[])
{
	char	buf[MAXDIGIT], *bp;
	int	i;

	bp = buf;
	i = 0;

	do {
		*bp++ = n % 10 + '0';
		i++;
	} while (((n /= 10) > 0) && i < MAXDIGIT);

	bp--;

	while (bp >= buf)	{
		*s++ = *bp--;
	}

	*s = '\0';
}

/*
 * STATIC char *
 * sdi_mkname(char *drvname, int inst, char *desc_str)
 *	Internal routine to construct an HBA name string
 *	in the form:
 *		"(drvname,inst) desc_str"
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC char *
sdi_mkname(char *drvname, int inst, char *desc_str)
{
	char	buf[CM_MODNAME_MAX+MAXDIGIT+MAXDESCLEN+3], *bp;

	bp = buf;

	*bp++ = '(';

	strcpy(bp, drvname);

	while (*bp != '\0')	{
		bp++;
	}

	*bp++ = ',';
	sdi_itoa(inst+1, bp);

	while (*bp != '\0')	{
		bp++;
	}

	*bp++ = ')';

	if (strlen(desc_str) < MAXDESCLEN)	{
		strcpy(bp, desc_str);
	}

	if ((bp = kmem_alloc(strlen(buf)+1, sdi_sleepflag)) == NULL) {
		return (NULL);
	}

	strcpy(bp, buf);

	return (bp);
}

#ifdef	DEBUG
#define	SDI_CM_GETVAL(p)	(rv = cm_getval(p))
#else
#define	SDI_CM_GETVAL(p)	cm_getval(p)
#endif

/*
 * int
 * sdi_hba_getconf(rm_key_t rm_key, HBA_IDATA_STRUCT *idp)
 *	Populate the given hba_idata structure with configuration
 *	information found in the autoconfig Resource Manager database
 *	entry defined by rm_key.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
sdi_hba_getconf(rm_key_t rm_key, HBA_IDATA_STRUCT *idp)
{
	cm_args_t	cma;
	cm_num_t	num;
	struct	cm_addr_rng	rng;
	int	rv;
	int	i;
	char	buffer[15];

	cma.cm_key = rm_key;
	cma.cm_n   = 0;

	/*
	 * First read range values.
	 */
	cma.cm_val = &rng;

	/*
	 * Get IO address range.
	 */
	cma.cm_param = CM_IOADDR;
	cma.cm_vallen = sizeof(struct cm_addr_rng);

	cm_begin_trans(rm_key, RM_RDWR);
	if ((rv = cm_getval(&cma)) != 0)	{
		/*
		 * Initial check for invalid rm_key.
		 */
		if(rv == EINVAL)	{
			cm_end_trans(rm_key);
			return (EINVAL);
		}
		ASSERT(rv == ENOENT);
		idp->ioaddr1 = 0;
		if((idp->version_num & HBA_VMASK) >= HBA_UW21_IDATA) {
			idp->ioaddr2 = 0;
		}
	}
	else	{
		idp->ioaddr1 = rng.startaddr;

		/*
		 * Don't even bother to look for a second I/O address unless the
		 * idata version has space to store it.
		 */
		if((idp->version_num & HBA_VMASK) >= HBA_UW21_IDATA) {
			cma.cm_n   = 1;
			if ((rv = cm_getval(&cma)) != 0)	{
				idp->ioaddr2 = 0;
			}
			else	{
				idp->ioaddr2 = rng.startaddr;
			}
			cma.cm_n   = 0;
		}
	}

	/*
	 * NOTE: At this point it is assumed that all failures of
	 * cm_getval() result from missing parameters in the database.
	 * This is a valid assumption, since the other failure cases
	 * indicate we have a bad key or cma.cm_vallen is wrong.
	 *
	 * If we find parameters are missing, we assume the
	 * hardware doesn't require them, and pass back default
	 * values.
	 */

	/*
	 * Get MEM address range.
	 */
	cma.cm_param = CM_MEMADDR;
	cma.cm_vallen = sizeof(struct cm_addr_rng);

	if (SDI_CM_GETVAL(&cma) != 0)	{
		ASSERT(rv == ENOENT);
		idp->idata_memaddr = 0;
	}
	else	{
		idp->idata_memaddr = rng.startaddr;
	}

	/*
	 * Now, read numeric values.
	 */
	cma.cm_val = &num;

	/*
	 * Get IRQ.
	 */
	cma.cm_param = CM_IRQ;
	cma.cm_vallen = sizeof(cm_num_t);

	if (SDI_CM_GETVAL(&cma) != 0)	{
		ASSERT(rv == ENOENT);
		idp->iov = 0;
	}
	else	{
		idp->iov = num;
	}

	/*
	 * Get DMAC.
	 */
	cma.cm_param = CM_DMAC;
	cma.cm_vallen = sizeof(cm_num_t);

	if (SDI_CM_GETVAL(&cma) != 0)	{
		ASSERT(rv == ENOENT);
		idp->dmachan1 = -1;
	}
	else	{
		idp->dmachan1 = num;
	}

	/*
	 * Get UNIT (cntlr).
	 */
	cma.cm_param = CM_UNIT;
	cma.cm_vallen = sizeof(cm_num_t);

	if (SDI_CM_GETVAL(&cma) != 0)	{
		ASSERT(rv == ENOENT);
		/*
		 * If there is no UNIT value in the database,
		 * set it to -1 so pdicfg will know to change
		 * it to the proper value.
		 */

		num = -1;
		cm_addval(&cma);
		idp->cntlr = -1;
	}
	else {
		idp->cntlr = num;
	}

	/*
	 * Get BINDCPU
	 */
	cma.cm_param = CM_BINDCPU;
	cma.cm_vallen = sizeof(cm_num_t);

	if (SDI_CM_GETVAL(&cma) != 0) {
	        ASSERT(rv == ENOENT);
	        idp->idata_cpubind = -1; 
	}
	else {
		idp->idata_cpubind = num;
	}

	/*
	 * Get HA_ID.
	 */
	cma.cm_param = SDI_HAID_PARAM;
	cma.cm_vallen = sizeof(cm_num_t);

	if (cm_getval(&cma) == 0)	{
		idp->ha_id = (unsigned char)num;
	}

	/*
	 * Get SCSI Channel ID's.
	 */
	if((idp->version_num & HBA_VMASK) == HBA_UW21_IDATA) {
		strcpy(buffer, SDI_CHANID_PARAM);
		for(i = 0; i < SDI_MAX_CHANS; i++) {
			/* Add the ascii number (0 - 7) to end string */
			buffer[strlen(SDI_CHANID_PARAM) - 1] = i + 0x30;
			cma.cm_param = buffer;
			cma.cm_vallen = sizeof(cm_num_t);

			if (SDI_CM_GETVAL(&cma) != 0)	{
				ASSERT(rv == ENOENT);
				idp->ha_chan_id[i] = idp->ha_id;
			}
			else {
				idp->ha_chan_id[i] = num;
			}
		}
	}
	cm_end_trans(rm_key);

	return (0);
}

/*
 * HBA_IDATA_STRUCT *
 * sdi_hba_autoconf(char *drvname, HBA_IDATA_STRUCT *idata, int *cntls)
 *	Allocate and initialize a new idata array for the calling HBA driver.
 *	The contents of the new array reflects the information found in
 *	the autoconfig Resource Manager database for hardware instances
 *	associated with the calling driver.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
HBA_IDATA_STRUCT *
sdi_hba_autoconf(char *drvname, HBA_IDATA_STRUCT *idata, int *cntls)
{
	rm_key_t	rm_key;
	HBA_IDATA_STRUCT	*nidata, *idp;
	int	nbrd, i, ver, ext_flg;
	unsigned char	ha_id;
	char	*desc_str;

#ifdef SDI_AUTOCONF_DEBUG
	int	j;

	cmn_err(CE_NOTE, "!In sdi_hba_autoconf(), driver = %s", drvname);
#endif
	/*
	 * The driver didn't pass in its proper name, this means we
	 * won't find any hardware for this driver name, so fail here.
	 */
	if (strlen(drvname) >= CM_MODNAME_MAX)	{
		*cntls = 0;
		return (NULL);
	}

	/*
	 * Get the number of hardware instances currently
	 * associated with the calling driver.
	 */
	if ((nbrd = _Compat_cm_getnbrd(drvname)) == 0)	{
#ifdef SDI_AUTOCONF_DEBUG
	cmn_err(CE_NOTE, "!sdi_hba_autoconf: no boards found for %s", drvname);
#endif
		*cntls = 0;
		return (NULL);
	}
#ifdef SDI_AUTOCONF_DEBUG
	cmn_err(CE_NOTE, "!sdi_hba_autoconf: %d boards found for %s",nbrd , drvname);
#endif

	/*
	 * The HBA_AUTOCONF flag will let other routines know
	 * this idata array was allocated by sdi_hba_autoconf().
	 */
	ver = idata->version_num | HBA_AUTOCONF;

	for (desc_str=idata->name; *desc_str!=')'&&*desc_str!='\0'; desc_str++);

	if (*desc_str == ')')	{
		desc_str++;
	}

	/*
	 * Allocate enough memory for each hardware instance.
	 * We check to see if the driver passed us a version 4 idata
	 * struct. If it did it means it is a UNIXWARE 2.0 driver
	 * that has not been updated to use the new version 5 idata.
	 * We must then allocate a version 4 idata instead of a version 5.
	 */
	if((ver & HBA_VMASK) == HBA_UW21_IDATA) {
		if ((nidata = kmem_zalloc(nbrd * sizeof(struct hba_idata_v5),
	    				sdi_sleepflag)) == NULL) {
			*cntls = 0;
			return (NULL);
		}
	} else {
		if ((nidata = kmem_zalloc(nbrd * sizeof(struct hba_idata_v4),
	    				sdi_sleepflag)) == NULL) {
			*cntls = 0;
			return (NULL);
		}
	}

	/*
	 * Use this as the initial default value. It can be changed
	 * by sdi_hba_getconf() or by the driver.
	 */
	ha_id = idata->ha_id;

	/*
	 * Populate each entry in the new idata array.
	 */
	for (i = 0, idp = nidata; i < nbrd; i++)	{
		idp->version_num = ver;
		idp->ha_id = ha_id;

		/*
		 * Get the Resource Manager key for the current
		 * hardware instance.
		 */
		if ((rm_key = _Compat_cm_getbrdkey(drvname, i)) == RM_NULL_KEY) {
			sdi_acfree(nidata, nbrd);
			*cntls = 0;
			return (NULL);
		}

		/*
		 * Assign configuration values from the Resource Manager
		 * to the corresponding fields of the idata structure.
		 */
		if (sdi_hba_getconf(rm_key, idp) != 0)	{
			sdi_acfree(nidata, nbrd);
			*cntls = 0;
			return (NULL);
		}

		/*
		 * Save the Resource Manager key, sdi_intr_attach() may need it
		 * later.
		 */
		idp->idata_rmkey = rm_key;

		/*
		 * Given the driver name and board instance, construct
		 * an HBA name string for this entry.
		 */
		idp->name = sdi_mkname(drvname, i, desc_str);

#ifdef SDI_AUTOCONF_DEBUG
	cmn_err(CE_NOTE, "!sdi_hba_autoconf, %s idata[%d]:", drvname, i);
	cmn_err(CE_CONT, "!name = %s", idp->name);
	cmn_err(CE_CONT, "!ioaddr1 = 0x%x", idp->ioaddr1);
	cmn_err(CE_CONT, "!memaddr = 0x%x", idp->idata_memaddr);
	cmn_err(CE_CONT, "!dmachan1 = %d", idp->dmachan1);
	cmn_err(CE_CONT, "!iov = %d", idp->iov);
	if((ver & HBA_VMASK) == HBA_UW21_IDATA)
		for(j = 0; j < SDI_MAX_CHANS; j++)
			cmn_err(CE_CONT,"!ha_chan_id[%d] = %d", j, idp->ha_chan_id[j]);
#endif
		HBA_IDATA_INC_PTR(idp);
	}

	*cntls = nbrd;

	return (nidata);
}

/*
 * void
 * sdi_acfree(HBA_IDATA_STRUCT *idata, int cntls)
 *	Free memory associated with the given idata array,
 *	initially allocated by sdi_hba_autoconf(). If needed,
 *	this routine will detach interrupts associated
 *	with any of the array's entries.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
sdi_acfree(HBA_IDATA_STRUCT *idata, int cntls)
{
	HBA_IDATA_STRUCT	*idp;
	int	i;

	if (idata == NULL)	{
		return;
	}

	/*
	 * If this array was not allocated by sdi_hba_autoconf(),
	 * do nothing.
	 */
	if ((idata->version_num & HBA_AUTOCONF) == 0) {
		return;
	}

	for (i = 0, idp = idata; i < cntls; i++)	{
		/*
		 * Detach any interrupts associated with these
		 * the idata entries.
		 */
		if (idp->idata_intrcookie != NULL)	{
			cm_intr_detach(idp->idata_intrcookie);
		}

		/*
		 * Free the memory allocated for the HBA name.
		 */
		if (idp->name != NULL)	{
			kmem_free(idp->name, strlen(idp->name)+1);
		}
		HBA_IDATA_INC_PTR(idp);
	}

	if((idata->version_num & HBA_VMASK) == HBA_UW21_IDATA) 
		kmem_free(idata, cntls*sizeof(struct hba_idata_v5));
	else
		kmem_free(idata, cntls*sizeof(struct hba_idata_v4));
}

static int	mp_df	= D_MP;
static int	up_df	= 0;

/*
 * void
 * sdi_intr_attach(HBA_IDATA_STRUCT *idata, int cntls, void (*intr)(), int devflag)
 *	Attach interrupts for each hardware instance whose hba_idata
 *	structure is marked active (the active field of the hba_idata
 *	structure is non-zero.)
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
sdi_intr_attach(HBA_IDATA_STRUCT *idata, int cntls, void (*intr)(), int devflag)
{
	int	i, *df;
	HBA_IDATA_STRUCT	*idp;

	if ((devflag & HBA_MP) != 0)	{
		df = &mp_df;
	}
	else	{
		df = &up_df;
	}

#ifdef SDI_AUTOCONF_DEBUG
	cmn_err(CE_NOTE, "!In sdi_intr_attach()");
#endif

	for (i = 0, idp = idata; i < cntls; i++)	{
		/*
		 * If the device is marked active, attach its interrupts.
		 */
		if (idp->active)	{
#ifdef SDI_AUTOCONF_DEBUG
	cmn_err(CE_NOTE, "!sdi_intr_attach: attaching interrupts for %s",
		idp->name);
#endif
			if (_Compat_cm_intr_attach(idp->idata_rmkey, intr, df, &(idp->idata_intrcookie)) == 0) {
				cmn_err(CE_WARN, "!sdi_intr_attach: failed to attach interrupts for %s\n",
					idp->name);
				idp->active = 0;
			}
		}
		HBA_IDATA_INC_PTR(idp);
	}
}

/*
 * void
 * sdi_mca_conf(HBA_IDATA_STRUCT *idata, int cntls, int (*drv_mca_conf)())
 *	Obtain and interpret MCA configuration information for the
 *	HBA driver.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
sdi_mca_conf(HBA_IDATA_STRUCT *idata, int cntls, int (*drv_mca_conf)())
{
	cm_args_t		cma;
	cm_num_t		numval;
	int	i, ioalen, memalen;
	HBA_IDATA_STRUCT	*idp;

	if ((idata->version_num & HBA_AUTOCONF) == 0)	{
		return;
	}

	cma.cm_n = 0;
	cma.cm_val = &numval;
	cma.cm_param = CM_BRDBUSTYPE;

	for (i=0, idp = idata; i < cntls; i++) {
		if (idp->idata_rmkey == NULL) {
			continue;
		}

		/*
		 * Check for MCA hardware instances.
		 */
		cma.cm_key = idp->idata_rmkey;
		cma.cm_vallen = sizeof(cm_num_t);

		cm_begin_trans(idp->idata_rmkey, RM_READ);
		if (cm_getval(&cma) != 0)	{
			cm_end_trans(idp->idata_rmkey);
			continue;
		}
		cm_end_trans(idp->idata_rmkey);

		if (numval != CM_BUS_MCA)	{
			continue;
		}

		/*
		 * Interpret POS data and populate idp accordingly.
		 */
		if ((*drv_mca_conf)(idp, &ioalen, &memalen) != 0)	{
			continue;
		}

		if (ioalen != 0)	{
			ioalen--;
		}
		if (memalen != 0)	{
			memalen--;
		}

#ifdef SDI_AUTOCONF_DEBUG
cmn_err(CE_CONT, "!sdi_mca_conf: %s, iov = %d, ioaddr1 = 0x%x-0x%x, memaddr = 0x%x-0x%x, dmachan = %d\n",
		idp->name,
    		idp->iov, idp->ioaddr1, idp->ioaddr1+ioalen,
    		idp->idata_memaddr, idp->idata_memaddr+memalen, idp->dmachan1);
#endif

		/*
		 * Put interpreted configuration information into the
		 * resmgr database entry defined by idp->idata_rm_key.
		 */
		if (cm_AT_putconf(idp->idata_rmkey, idp->iov, 0,
			    	idp->ioaddr1, idp->ioaddr1+ioalen,
			    	idp->idata_memaddr, idp->idata_memaddr+memalen,
			    	idp->dmachan1,
				CM_SET_IRQ|CM_SET_IOADDR|CM_SET_MEMADDR|CM_SET_DMAC,
				0) != 0)	{

			/*
			 *+ The driver could not update the resmgr database with
			 *+ its interpreted MCA configuration information.
			 *+ The device in question should still function, but its
			 *+ configuration information will not be displayed by the DCU.
			 */
			cmn_err(CE_WARN,
				"sdi_mca_conf: %s, could not update resmgr database",
				idp->name);
		}
		HBA_IDATA_INC_PTR(idp);
	}
}

/*
 * HBA_IDATA_STRUCT *
 * sdi_idata_alloc(rm_key_t rm_key, const HBA_IDATA_STRUCT *idata)
 *	Allocate and initialize a new idata element for the calling HBA 
 *	driver instance.
 *	The contents of the new instance reflects the information found in
 *	the autoconfig Resource Manager database for this hardware instance
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
HBA_IDATA_STRUCT *
sdi_idata_alloc(rm_key_t rm_key, const HBA_IDATA_STRUCT *idata)
{
	HBA_IDATA_STRUCT	*nidata;
	int 	ver, cntlr, instnum;
	char	*desc_str;
	char	drvname[CM_MODNAME_MAX];
	cm_args_t	cma;

#ifdef SDI_AUTOCONF_DEBUG
	int	j;

	cmn_err(CE_NOTE, "!In sdi_idata_alloc()");
#endif
	/*
	 * The HBA_IDATA_ALLOC flag will let other routines know
	 * this idata element was allocated by sdi_idata_alloc().
	 */
	ver = idata->version_num | HBA_IDATA_ALLOC;

	if((ver & HBA_VMASK) < HBA_UW21_IDATA) {
		return (NULL);
	}

	/* get the drvname */
	cma.cm_key = rm_key;
	cma.cm_n   = 0;
	cma.cm_param = CM_MODNAME;
	cma.cm_val = drvname;
	cma.cm_vallen = sizeof(drvname);

	cm_begin_trans(rm_key, RM_READ);
	if (cm_getval(&cma) != 0)	{
		cm_end_trans(rm_key);
		return (NULL);		/* cannot get the driver name */
	}

	/* get the instance number */
	cma.cm_val = &instnum;
	cma.cm_param = CM_INSTNUM;
	cma.cm_vallen = sizeof(cm_num_t);

	if (cm_getval(&cma) != 0)	{
		cm_end_trans(rm_key);
		return (NULL);		/* cannot get the instance number */
	}
	cm_end_trans(rm_key);

	for (desc_str=idata->name; *desc_str!=')'&&*desc_str!='\0'; desc_str++);

	if (*desc_str == ')')	{
		desc_str++;
	}

	/*
	 * Allocate memory for this hardware instance.
	 */

	if ((nidata = kmem_zalloc(sizeof(HBA_IDATA_STRUCT), 
					sdi_sleepflag)) == NULL) {
		return (NULL);
	}

	/*
	 * Populate each entry in the new idata array.
	 */
	nidata->version_num = ver;

	/*
	 * Use this as the initial default value. It can be changed
	 * by sdi_hba_getconf() or by the driver.
	 */
	nidata->ha_id = idata->ha_id;

	/*
	 * Assign configuration values from the Resource Manager
	 * to the corresponding fields of the idata structure.
	 */
	if (sdi_hba_getconf(rm_key, nidata) != 0)	{
		sdi_idata_free(nidata);
		return (NULL);
	}

	/*
	 * Save the Resource Manager key, sdi_intr_attach() may need it later.
	 */
	nidata->idata_rmkey = rm_key;

	/* Get an HBA number assigned by SDI */
	cntlr = sdi_gethbano(nidata->cntlr);
	if (cntlr < 0) {
		cmn_err(CE_WARN,"!%s: No HBA number available", idata->name);
		sdi_idata_free(nidata);
		return (NULL);
	}
	nidata->cntlr = cntlr;

	/*
	 * Given the driver name and board instance, construct
	 * an HBA name string for this entry.
	 */
	nidata->name = sdi_mkname(drvname, instnum, desc_str);

#ifdef SDI_AUTOCONF_DEBUG
	cmn_err(CE_NOTE, "!sdi_idata_alloc ");
	cmn_err(CE_CONT, "!%s:", drvname);
	cmn_err(CE_CONT, "!name = %s", nidata->name);
	cmn_err(CE_CONT, "!cntlr = %d", nidata->cntlr);
	cmn_err(CE_CONT, "!ioaddr1 = 0x%x", nidata->ioaddr1);
	cmn_err(CE_CONT, "!memaddr = 0x%x", nidata->idata_memaddr);
	cmn_err(CE_CONT, "!dmachan1 = %d", nidata->dmachan1);
	cmn_err(CE_CONT, "!iov = %d", nidata->iov);
	for(j = 0; j < SDI_MAX_CHANS; j++)
		cmn_err(CE_CONT,"!ha_chan_id[%d] = %d", j, nidata->ha_chan_id[j]);
#endif

	return (nidata);
}

/*
 * int
 * sdi_idata_modify(HBA_IDATA_STRUCT *idata, rm_key_t rm_key)
 *	modify idata elements for the calling HBA driver instance.
 *	The contents of the new instance reflects the information found in
 *	the autoconfig Resource Manager database for this hardware instance
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
sdi_idata_modify(HBA_IDATA_STRUCT *idata, rm_key_t rm_key) 
{
	int	save_cntlr;
	cm_args_t	cma;
	cm_num_t	num;
	int		rv;
#ifdef SDI_AUTOCONF_DEBUG
	int	j;

	cmn_err(CE_NOTE, "!In sdi_idata_modify()");
#endif

	ASSERT(idata->version_num & HBA_IDATA_ALLOC);

	/* save the controller number */
	save_cntlr = idata->cntlr;

	/*
	 * Assign configuration values from the Resource Manager
	 * to the corresponding fields of the idata structure.
	 */
	if (sdi_hba_getconf(rm_key, idata) != 0)	{
		return (-1);
	}

	/*
	 * Save the Resource Manager key, sdi_intr_attach() may need it later.
	 */
	idata->idata_rmkey = rm_key;
	idata->cntlr = save_cntlr;

	/* store the UNIT (cntlr) in resmgr */

	cma.cm_key = rm_key;
	cma.cm_n   = 0;
	cma.cm_val = &num;
	cma.cm_param = CM_UNIT;
	cma.cm_vallen = sizeof(cm_num_t);

	cm_begin_trans(rm_key, RM_RDWR);
	if (SDI_CM_GETVAL(&cma) != 0)	{
		ASSERT(rv == ENOENT);
		/*
		 * If there is no UNIT value in the database,
		 * set it to that of the previous controller
		 */

		num = idata->cntlr;
		cm_addval(&cma);
	}
	cm_end_trans(rm_key);

#ifdef SDI_AUTOCONF_DEBUG
	cmn_err(CE_NOTE, "!sdi_idata_modify ");
	cmn_err(CE_CONT, "!name = %s", idata->name);
	cmn_err(CE_CONT, "!ioaddr1 = 0x%x", idata->ioaddr1);
	cmn_err(CE_CONT, "!memaddr = 0x%x", idata->idata_memaddr);
	cmn_err(CE_CONT, "!dmachan1 = %d", idata->dmachan1);
	cmn_err(CE_CONT, "!iov = %d", idata->iov);
	for(j = 0; j < SDI_MAX_CHANS; j++)
		cmn_err(CE_CONT,"!ha_chan_id[%d] = %d", j, idata->ha_chan_id[j]);
#endif

	return (0);
}

/*
 * void
 * sdi_idata_free(HBA_IDATA_STRUCT *idata)
 *	Free memory associated with the given idata element,
 *	initially allocated by sdi_idata_alloc(). If needed,
 *	this routine will detach interrupts associated
 *	with the entry.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
sdi_idata_free(HBA_IDATA_STRUCT *idata)
{
	if (idata == NULL)	{
		return;
	}

	/*
	 * If this element was not allocated by sdi_idata_alloc(),
	 * do nothing.
	 */
	if ((idata->version_num & HBA_IDATA_ALLOC) == 0) {
		return;
	}

	/*
	 * Detach any interrupts associated with this idata entry.
	 */
	if (idata->idata_intrcookie != NULL)	{
		cm_intr_detach(idata->idata_intrcookie);
	}

	/*
	 * Free the memory allocated for the HBA name.
	 */
	if (idata->name != NULL)	{
		kmem_free(idata->name, strlen(idata->name)+1);
	}

	kmem_free(idata, sizeof(HBA_IDATA_STRUCT));
}
