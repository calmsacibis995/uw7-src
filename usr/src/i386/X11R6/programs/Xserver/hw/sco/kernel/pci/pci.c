#ident "@(#)pci.c	11.2	11/18/97	10:02:31"
/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */
/*
 *	S002 Tue Nov 18 10:00:44 PST 1997	-	hiramc@sco.COM
 *	- change type declaration of cgnum variable for Jun's BL15
 *	- pci changes, now cgnum_t, was ms_cgnum_t
 *	S001 Mon May 12 10:00:38 PDT 1997	-	hiramc@sco.COM
 *	- sparticus promotion - add cgnum argument to all calls on the
 *	- pci system
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

#include "sys/pci.h"
#include "pcix.h"

int pcixdevflag = 0;
int pcixopen(), pcixclose(), pcixioctl();

static cgnum_t cgnum = 0;		/*	S002	*/

#define DRVNAME "pcix - PCI bus Driver"
STATIC int pcix_load(), pcix_unload();
MOD_DRV_WRAPPER(pcix, pcix_load, pcix_unload, NULL, DRVNAME);

STATIC int
pcix_load()
{
    return(0);
}

STATIC int
pcix_unload()
{
    return(0);
}

pcixopen(dev, flag, otyp, crp)
    dev_t *dev;
    cred_t *crp;
{
    return 0;
}

pcixclose(dev, flag, otyp, crp)
{
    return(0);
}


/* OSR5 /dev/pci interface */

/*
 *	Copyright (C) The Santa Cruz Operation, 1994.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */

#include "sys/conf.h"			/* L004 */
#include "sys/errno.h"
#include "sys/user.h"
#include "sys/cmn_err.h"
#include "sys/bootinfo.h"
#include "sys/debug.h"
#include "sys/open.h"

/*
 ** Private function prototypes
 */
static unsigned long confdword(unsigned short, unsigned short,
                               unsigned short, unsigned short,
                               unsigned long, int);

static unsigned short confword(unsigned short, unsigned short,
                               unsigned short, unsigned short,
                               unsigned short, int);

static unsigned char confbyte(unsigned short, unsigned short,
                              unsigned short, unsigned short,
                              unsigned char, int);


static int initialised = 0;	/* Have we initialised yet */
static int accessmech;	        /* PCI config space access mech in use, -1 == none */
static unsigned short numbuses;	/* How many buses are in this machine */
static unsigned short maxdevs;	/* Max number devices per bus */
static unsigned short maxfunc;	/* Max number of functions per device */


/*
 ** Init routine
 ** This driver doesn't have anything important to do during init, so
 ** just print a pretty welcome message and setup our statics.
 */
pciinit()
{
    struct pci_bus_data pciBusData;    

    if(initialised)
        return(numbuses > 0 ? 0 : -1);
    
    initialised = 1;            /* Stay out of here from now on */

    /*
     ** Check if we think there is a PCI bus.
     */

	/*	S001 vvv	*/
    if ((pci_verify(cgnum) == 0) && (find_pci_id(cgnum, &pciBusData) == 0))
    {
        maxfunc = PCI_MAX_FUNCS; /* Number of functions/device */
        numbuses = pciBusData.pci_maxbus + 1;
        
        switch(pciBusData.pci_conf_cycle_type & 0x03)
        {

          case 1:

            accessmech = 1;
            maxdevs = PCI_DEVS_MECH1;
            break;

          case 2:

            accessmech = 2;
            maxdevs = PCI_DEVS_MECH2;
            break;

          default:  /* Either boots gone wild, or it's a new PCI spec */

            accessmech = -1;
            maxdevs = 0;
            maxfunc = 0;
            cmn_err(CE_WARN, "pci: Unknown PCI access mechanism!");
            numbuses = 0;
        } 
    } 

    else
    {
        accessmech = -1;               /* None in this machine */
        numbuses = 0;
        maxdevs = 0;
    }
    
    return(numbuses > 0 ? 0 : -1);
    
}


/*
 ** Ioctl routines
 */
pcixioctl(dev, cmd, arg, mode, crp, rvalp)
    dev_t *dev;
    int cmd, mode;
    caddr_t arg;
    cred_t *crp;
    int *rvalp;
{

    switch(cmd)
    {

      case PCI_BUS_PRESENT:     /* Is there a PCI bus? */
	{
            struct pci_busdata info;

            info.bus_present = pci_buspresent(&info.businfo);

            if(copyout(&info, arg, sizeof(struct pci_busdata)) == -1)
            {
                return (EFAULT);
            }

            break;              /* End PCI_BUS_PRESENT */
	}

      case PCI_FIND_DEVICE:     /* Given vendor and device IDs */
	{
            struct pci_devicedata param;

            if(copyin(arg, &param, sizeof(struct pci_devicedata)) == -1)
            {
                return(EFAULT);
            }

            param.device_present =
                pci_finddevice(param.vend_id, param.dev_id,
                               param.index, &param.info);

            if(copyout(&param, arg, sizeof(struct pci_devicedata)) == -1)
            {
                return(EFAULT);
            }

            break;              /* End PCI_FIND_DEVICE */
	}

      case PCI_FIND_CLASS:      /* Given base and sub classes */
	{
            struct pci_classdata param;

            if(copyin(arg, &param, sizeof(struct pci_classdata)) == -1)
            {
                return(EFAULT);
            }

            param.device_present =
                pci_findclass(param.base_class, param.sub_class,
                              param.index, &param.info);

            if(copyout(&param, arg, sizeof(struct pci_classdata)) == -1)
            {
                return(EFAULT);
            }

            break;              /* End PCI_FIND_CLASS */
	}

      case PCI_SPECIAL_CYCLE:
	{
            struct pci_specialdata data;

            if(copyin(arg, &data, sizeof(struct pci_specialdata)) == -1)
            {
                return(EFAULT);
            }

            pci_specialcycle(data.busno, data.busdata);

            break;              /* End PCI_SPECIAL_CYCLE */
	}

      case PCI_READ_CONFIG:
	{
            struct pci_configdata param;

            if(copyin(arg, &param, sizeof(struct pci_configdata)) == -1)
            {
                return(EFAULT);
            }

            if(param.size == sizeof(unsigned char))
            {
                param.status =
                    pci_readbyte(&param.info, param.reg,
                                 (unsigned char *)&param.data);
            } /* End if byte */

            else if(param.size == sizeof(unsigned short))
            {
                param.status =
                    pci_readword(&param.info, param.reg,
                                 (unsigned short *)&param.data);

            } /* End if word */

            else if(param.size == sizeof(unsigned long))
            {
                param.status = pci_readdword(&param.info,
                                             param.reg, &param.data);

            } /* End if dword */

            else
            {
                return(EINVAL);
            }

            if(copyout(&param, arg, sizeof(struct pci_configdata)) == -1)
            {
                return(EFAULT);
            }

            break;              /* End PCI_READ_CONFIG */
	}

      case PCI_WRITE_CONFIG:
	{
            struct pci_configdata param;

            if (crp->cr_uid != 0)
            {
                return (EPERM);
            }

            if(copyin(arg, &param, sizeof(struct pci_configdata)) == -1)
            {
                return(EFAULT);
            }

            if(param.size == sizeof(unsigned char))
            {
                param.status =
                    pci_writebyte(&param.info, param.reg,
                                  (unsigned char)param.data);

            } /* End if byte */

            else if(param.size == sizeof(unsigned short))
            {
                param.status = 
                    pci_writeword(&param.info, param.reg,
                                  (unsigned short)param.data);

            } /* End if word */

            else if(param.size == sizeof(unsigned long))
            {
                param.status = pci_writedword(&param.info,
                                              param.reg, param.data);

            } /* End if dword */

            else
            {
                return(EINVAL);
            }

            if(copyout(&param, arg, sizeof(struct pci_configdata)) == -1)
            {
                return(EFAULT);
            }

            break;              /* End PCI_WRITE_CONFIG */
	}

      case PCI_SEARCH_BUS:
	{
            struct pci_headerinfo param;

            if(copyin(arg, &param, sizeof(struct pci_headerinfo)) == -1)
            {
                return(EFAULT);
            }

            param.device_present =
                pci_search(param.vend_id, param.dev_id,
                           param.base_class, param.sub_class,
                           param.index, &param.info);

            if(copyout(&param, arg, sizeof(struct pci_headerinfo)) == -1)
            {
                return(EFAULT);
            }

            break;              /* End PCI_SEARCH_BUS */
	}

      case PCI_TRANS_BASE:      /* L005 begin */
	{
            struct pci_baseinfo param;

            if(copyin(arg, &param, sizeof(struct pci_baseinfo)) == -1)
            {
                return(EFAULT);
            }

            param.mappedbase = param.iobase;
            param.status = pci_transbase(&param.mappedbase, &param.info);

            if(copyout(&param, arg, sizeof(struct pci_baseinfo)) == -1)
            {
                return(EFAULT);
            }

            break;
	} /* End PCI_TRANS_BASE, L005 end */

      default:
        return(EINVAL);

    } /* End switch cmd */

    return(0);

}

/*
*******************************************************************************
**
** Kernel service routines.
**
*******************************************************************************
*/

/*
 ** Function returns 1 if a PCI bus is connected, 0 otherwise.
 */

int
pci_buspresent(struct pci_businfo *infptr)
{
    int gotone;

    if(!initialised)            /* Make sure we are set up ok */
        pciinit();

    if(accessmech != -1 && infptr != NULL){ /* Is there a PCI bus? */
        ASSERT(accessmech > 0 && accessmech < 3);

        infptr->numbuses = numbuses;
        infptr->mechanism = accessmech;
        gotone = 1;
    }
    else{
        infptr->numbuses = 0;
        infptr->mechanism = 0;
        gotone = 0;
    }

    return(gotone);

} /* End function pci_buspresent */


/*
 ** Searches buses for the Nth occurance of vend_id, and dev_id, where N is
 ** index. If a match is found returns 1 and completes the devinfo structure.
 ** Note that index is counted from 0, as per the PCI BIOS.
 */

int
pci_finddevice(unsigned short vend_id, unsigned short dev_id,
               unsigned short index, struct pci_devinfo *infptr)
    
{
    unsigned long data;
    unsigned short bus, device, func, vend, dev, multimax;
    int found = 0;
    
    if(!initialised)            /* Make sure we are set up ok */
        pciinit();
    
    if(accessmech == -1 || infptr == NULL)
        return(0);
    
    ASSERT(accessmech > 0 && accessmech < 3);
    
    /*
     ** Scan the buses for the index th device
     */
    for(bus = 0; bus < numbuses; bus++){
        for(device = 0; device < maxdevs; device++){

            if(confbyte(bus, device, 0,PCI_HEAD, 0, PCI_IN) & 0x80)
                multimax = maxfunc; /* Multi function */
            else
                multimax = 1;	/* Single function device */

            for(func = 0; func < multimax; func++){

                data = confdword(bus, device, func,
                                 PCI_VENDID, 0, PCI_IN);

                vend = data & 0xFFFF; /* Extract vend id */
                if(vend == 0xFFFF)
                    if(func == 0)
                        break;	/* Empty slot */
                    else
                        continue; /* Empty func */

                if(vend != vend_id)
                    continue;	/* Not what we want */

                dev = (data >> 16) & 0xFFFF;

                if(dev != dev_id)
                    continue;

                if(index == found++){ /* Got it */
                    /*
                     ** Complete the devinfo struture
                     */
                    infptr->slotnum = device;
                    infptr->funcnum = func;
                    infptr->busnum = bus;
                    return(1);

                } /* End got it */
            }
        } /* End for device */
    } /* End for bus */

    return(0);

} /* End function pci_finddevice */


/*
 ** Searches buses for the Nth occurance of base_class, and sub_class,
 ** where N is index. If a match is found returns 1 and completes the
 ** devinfo structure.
 ** Note that index is counted from 0, as per the PCI BIOS.
 */

int
pci_findclass(unsigned short base_class, unsigned short sub_class,
              unsigned short index, struct pci_devinfo *infptr)
{
    unsigned long data;
    unsigned short bus, dev, func, class, datashort, multimax;
    int found = 0;
	
    if(!initialised)		/* Check we are init'ed */
        pciinit();

    if(accessmech == -1 || infptr == NULL)
        return(0);

    ASSERT(accessmech > 0 && accessmech < 3);

    /*
     ** Scan the bus for the wanted class.
     */

    class = (base_class << 8) + sub_class; /* Predefined header format */

    for(bus = 0; bus < numbuses; bus++){
        for(dev = 0; dev < maxdevs; dev++){

            if(confbyte(bus, dev, 0, PCI_HEAD, 0, PCI_IN) & 0x80)
                multimax = maxfunc; /* Multi function */
            else
                multimax = 1;   /* Single function */

            for(func = 0; func < multimax; func++){

                /*
                 ** Check if there is a device/function present. 
                 */
                data = confdword(bus, dev, func, PCI_VENDID,
                                 0, PCI_IN);

                if((data & 0xFFFF) == 0xFFFF)
                    if(func == 0)
                        break;  /* Empty slot */
                    else
                        continue; /* No func */

                /*
                 ** Now check the class code.
                 */
                data = confdword(bus, dev, func, PCI_CLASS,
                                 0, PCI_IN);
                datashort = data >> 16;

                if(datashort == class && index == found++){
                    infptr->slotnum = dev;
                    infptr->funcnum = func;
                    infptr->busnum = bus;
                    return(1);

                } /* End found == index */
            } /* End for func */
        } /* End for dev */
    } /* End for bus */

    return(0);

} /* End function pci_findclass */


/*
** Function to generate a PCI special cycle. Note that bridge is not required
** to provide a mechanism for allowing special cycle generation.
** We can't tell if the special cycle works, so we just do the right thing
** and hope that it is ok.
*/

void
pci_specialcycle(unsigned short busno, unsigned long data)
{
    int s;
    unsigned long specialaddr;

	if(!initialised)		/* Check we are init'ed */
            pciinit();
    
    if(accessmech == -1 || busno >= numbuses)
        return;
    
    ASSERT(accessmech > 0 && accessmech < 3);
    
    generate_pci_special_cycle((uchar_t)busno, data);
    
    return;
    
} /* End function pci_specialcycle */


/*
 ** pci_readxxxx functions to read bytes, words, and dwords from PCI
 ** config space. Note that readword and readdword enforce correct register
 ** alignment.
 ** N.B.
 **	It is not guaranteed safe to read more data than requested since some
 ** registers may be destructive readout, therefore there are 3 seperate
 ** support routines to read byte, word, and dword data.
 */

int
pci_readbyte(struct pci_devinfo *infptr,
             unsigned short reg, unsigned char *dataptr) 
{
    unsigned long vendid;       /* L004 */
    unsigned short bus, dev, func;

    if(!initialised)		/* Check we are init'ed */
        pciinit();

    if(accessmech == -1 || dataptr == NULL || infptr == NULL)
        return(0);

    bus = infptr->busnum;
    dev = infptr->slotnum;
    func = infptr->funcnum;

    if(bus >= numbuses || dev >= maxdevs ||
       func >= maxfunc || reg >= PCI_CONFIG_LIMIT)
        return(0);

    ASSERT(accessmech > 0 && accessmech < 3);

    /*
     ** Make sure that function is valid...
     */
    if(func && !(confbyte(bus, dev, 0, PCI_HEAD, 0, PCI_IN) & 0x80))
        return(0);

    /* L004 begin
     ** and that device is present
     */
    vendid = confdword(bus, dev, func, PCI_VENDID, 0, PCI_IN);

    if((vendid & 0xFFFF) == 0xFFFF)
        return(0);		/* L004 end */

    *dataptr = confbyte(bus, dev, func, reg, 0,PCI_IN);

    return(1);

} /* End function pci_readbyte */


int
pci_readword(struct pci_devinfo *infptr,
             unsigned short reg, unsigned short *dataptr)
{
    unsigned long vendid;       /* L004 */
    unsigned short bus, dev, func;

    if(!initialised)		/* Check we are init'ed */
        pciinit();

    /*
     ** Make sure everything checks out.
     */
    if(accessmech == -1 || dataptr == NULL || infptr == NULL)
        return(0);

    bus = infptr->busnum;
    dev = infptr->slotnum;
    func = infptr->funcnum;

    if(bus >= numbuses || dev >= maxdevs || func >= maxfunc || 
       reg >= (unsigned short)(PCI_CONFIG_LIMIT - 1) || reg % 2)
        return(0);

    ASSERT(accessmech > 0 && accessmech < 3);

    /*
     ** Make sure that function is valid.....
     */
    if(func && !(confbyte(bus, dev, 0, PCI_HEAD, 0, PCI_IN) & 0x80))
        return(0);

    /* L004 begin,
     ** and that device is present.
     */
    vendid = confdword(bus, dev, func, PCI_VENDID, 0, PCI_IN);

    if((vendid & 0xFFFF) == 0xFFFF)
        return(0);		/* L004 end */

    *dataptr = confword(bus, dev, func, reg, 0, PCI_IN);

    return(1);

} /* End function pci_readword */


int
pci_readdword(struct pci_devinfo *infptr,
              unsigned short reg, unsigned long *dataptr)
{
    unsigned long vendid;       /* L004 */
    unsigned short bus, dev, func;

    if(!initialised)		/* Check we are init'ed */
        pciinit();

    if(accessmech == -1 || infptr == NULL || dataptr == NULL)
        return(0);

    bus = infptr->busnum;
    dev = infptr->slotnum;
    func = infptr->funcnum;

    if(bus >= numbuses || dev >= maxdevs || func >= maxfunc || 
       reg >= (unsigned short)(PCI_CONFIG_LIMIT - 3) || reg % 4)
        return(0);

    ASSERT(accessmech > 0 && accessmech < 3);

    /*
     ** Make sure that function is valid....
     */
    if(func && !(confbyte(bus, dev, 0, PCI_HEAD, 0, PCI_IN) & 0x80))
        return(0);

    /* L004 begin,
     ** and that device is present.
     */
    vendid = confdword(bus, dev, func, PCI_VENDID, 0, PCI_IN);

    if((vendid & 0xFFFF) == 0xFFFF)
        return(0);		/* L004 end */

    *dataptr = confdword(bus, dev, func, reg, 0, PCI_IN);

    return(1);

} /* End function pci_readdword */


/*
 ** pci_writexxxx functions to write bytes, words and dwords to PCI config
 ** space. Note that the word and dword variants enforce correct register
 ** offset alignment.
 */

int
pci_writebyte(struct pci_devinfo *infptr,
              unsigned short reg, unsigned char data) 
{
    unsigned long vendid;       /* L004 */
    unsigned short bus, dev, func;

    if(!initialised)		/* Check we are init'ed */
        pciinit();

    if(accessmech == -1 || infptr == NULL)
        return(0);

    bus = infptr->busnum;
    dev = infptr->slotnum;
    func = infptr->funcnum;

    if(bus >= numbuses || dev >= maxdevs || func >= maxfunc ||
       reg >= PCI_CONFIG_LIMIT)
        return(0);

    ASSERT(accessmech > 0 && accessmech < 3);

    /*
     ** Make sure that function is valid....
     */
    if(func && !(confbyte(bus, dev, 0, PCI_HEAD, 0, PCI_IN) & 0x80))
        return(0);

    /* L004 begin,
     ** and that device is present.
     */
    vendid = confdword(bus, dev, func, PCI_VENDID, 0, PCI_IN);

    if((vendid & 0xFFFF) == 0xFFFF)
        return(0);		/* L004 end */

    (void)confbyte(bus, dev, func, reg, data, PCI_OUT);

    return(1);

} /* End function pci_writebyte */


int
pci_writeword(struct pci_devinfo *infptr,
              unsigned short reg, unsigned short data)
{
    unsigned long vendid;       /* L004 */
    unsigned short bus, dev, func;

    if(!initialised)		/* Check we are init'ed */
        pciinit();

    /*
     ** Make sure everything checks out.
     */
    if(accessmech == -1 || infptr == NULL)
        return(0);

    bus = infptr->busnum;
    dev = infptr->slotnum;
    func = infptr->funcnum;

    if(bus >= numbuses || dev >= maxdevs || func >= maxfunc ||
       reg >= (unsigned short)(PCI_CONFIG_LIMIT - 1) || reg % 2)
        return(0);

    ASSERT(accessmech > 0 && accessmech < 3);

    /*
     ** Make sure that function is valid....
     */
    if(func && !(confbyte(bus, dev, 0, PCI_HEAD, 0, PCI_IN) & 0x80))
        return(0);

    /* L004 begin,
     ** and that device is present.
     */
    vendid = confdword(bus, dev, func, PCI_VENDID, 0, PCI_IN);

    if((vendid & 0xFFFF) == 0xFFFF)
        return(0);		/* L004 end */

    (void)confword(bus, dev, func, reg, data, PCI_OUT);

    return(1);

} /* End function pci_writeword */


int
pci_writedword(struct pci_devinfo *infptr,
               unsigned short reg, unsigned long data)
{
    unsigned long vendid;       /* L004 */
    unsigned short bus, dev, func;

    if(!initialised)		/* Check we are init'ed */
        pciinit();

    if(accessmech == -1 || infptr == NULL)
        return(0);

    bus = infptr->busnum;
    dev = infptr->slotnum;
    func = infptr->funcnum;

    if(bus >= numbuses || dev >= maxdevs || func >= maxfunc ||
       reg >= (unsigned short)(PCI_CONFIG_LIMIT - 3) || reg % 4)
        return(0);

    ASSERT(accessmech > 0 && accessmech < 3);

    /*
     ** Make sure that function is valid....
     */
    if(func && !(confbyte(bus, dev, 0, PCI_HEAD, 0, PCI_IN) & 0x80))
        return(0);

    /* L004 begin,
     ** and that device is present
     */
    vendid = confdword(bus, dev, func, PCI_VENDID, 0, PCI_IN);

    if((vendid & 0xFFFF) == 0xFFFF)
        return(0);		/* L004 end */

    (void)confdword(bus, dev, func, reg, data, PCI_OUT);

    return(1);

} /* End function pci_writedword */


/*
 ** Function to search PCI buses for Nth device matching non-wildcard input
 ** parameters. Wildcards are specified by 0xFFFF.
 ** Returns 1, and completes devinfo structure if a match is made, else
 ** returns 0.
 ** NB. If someone specifies all wildcards then they will just match the
 ** Nth device.
 */

int
pci_search(unsigned short vend_id, unsigned short dev_id,
           unsigned short base_class, unsigned short sub_class,
           unsigned short index, struct pci_devinfo *infptr)
{
    unsigned long data;
    unsigned short bus, dev, func, word, vend, multimax, found = 0;

    if(!initialised)
        pciinit();		/* Make sure we are set up ok */

    if(accessmech == -1 || index == 0xFFFF || infptr == NULL)
        return(0);

    ASSERT(accessmech > 0 && accessmech < 3);

    /*
     ** Scan all devices and functions on all buses for a N matches on all
     ** non wildcard parameters, where N is index.
     */
    for(bus = 0; bus < numbuses; bus++){
        for(dev = 0; dev < maxdevs; dev++){

            if(confbyte(bus, dev, 0, PCI_HEAD, 0, PCI_IN) & 0x80)
                multimax = maxfunc; /* Multi fn device */
            else
                multimax = 1;   /* Single fn device */

            for(func = 0; func < multimax; func++){
				
                /*
                 ** Check if there is a function/device here.
                 */
                data = confdword(bus, dev, func, PCI_VENDID,
                                 0, PCI_IN);

                if((vend = (unsigned short)data) == 0xFFFF)
                    if(func == 0)
                        break;  /* Empty slot */
                    else
                        continue; /* No func */

                if(vend_id != 0xFFFF && vend_id != vend)
                    continue;	/* Not this one */

                if(dev_id != 0xFFFF && dev_id != (data >> 16))
                    continue;	/* Nope */

                data = confdword(bus, dev, func, PCI_CLASS,
                                 0, PCI_IN);

                if(base_class != 0xFFFF && base_class !=
                   (unsigned short)((data >> 24) & 0x00FF))
                    continue;

                if(sub_class != 0xFFFF && sub_class !=
                   (unsigned short)((data >> 16) & 0x00FF))
                    continue;

                if(index == found++){ /* This is it! */
                    infptr->slotnum = dev;
                    infptr->funcnum = func;
                    infptr->busnum = bus;
                    return(1);

                } /* End found == index */
            } /* End for func */
        } /* End for dev */
    } /* End for bus */

    return(0);

} /* End function pci_search */


/* L005 begin,
** Stub to be replaced in corollary PCI driver to map IO base addresses in
** devices connected to auxiliary PCI buses.
** Returns 1 for success, 0 otherwise.
*/

int
pci_transbase(unsigned long *iobase, struct pci_devinfo *infoptr)
{

    if(infoptr == (struct pci_devinfo *)NULL)
        return(0);

    return(1);

} /* End function pci_transbase, L005 end */



/*
******************************************************************************
**
** PCI driver support routines
**
******************************************************************************
*/

/*
** Three similar functions to read a byte, word, or dword of data from
** config space.
** Note that register offset, reg should be specified in bytes from the
** start of the config space.
** Also note that these are dumb routines and if they are asked to do something
** stupid, they will do something stupid, it is the calling codes
** responsibility to ensure that parameters are valid, and functions are
** present etc.
*/

static
unsigned long
confdword(unsigned short bus, unsigned short device,
          unsigned short func, unsigned short reg,
          unsigned long data, int direction)
    
{

    unsigned long dword;

    ASSERT(accessmech > 0 && accessmech < 3);

    if (direction == PCI_OUT)
    {
			/*	S001 vvv	*/
        pci_write_devconfig32(cgnum, (uchar_t)bus,
                              (uchar_t)((device << 3) | func),
                              (uint_t*)&data,
                              (ushort_t)reg);
        dword = data;
    }
    else
    {
			/*	S001 vvv	*/
        pci_read_devconfig32(cgnum, (uchar_t)bus,
                             (uchar_t)((device << 3) | func),
                             (uint_t*)&dword,
                             (ushort_t)reg);
        
    }

    return(dword);

} /* End function confdword */


static
unsigned short
confword(unsigned short bus, unsigned short device,
         unsigned short func, unsigned short reg,
         unsigned short data, int direction)
    
{
    unsigned short word;

    if (direction == PCI_OUT)
    {
			/*	S001 vvv	*/
        pci_write_devconfig16(cgnum, (uchar_t)bus,
                              (uchar_t)((device << 3) | func),
                              (ushort_t*)&data,
                              (ushort_t)reg);
        word = data;
    }
    else
    {
			/*	S001 vvv	*/
        pci_read_devconfig16(cgnum, (uchar_t)bus,
                             (uchar_t)((device << 3) | func),
                             (ushort_t*)&word,
                             (ushort_t)reg);
        
    }

    return(word);

} /* End function confword */



static
unsigned char
confbyte(unsigned short bus, unsigned short device,
         unsigned short func, unsigned short reg,
         unsigned char databyte, int direction)

{
    unsigned long data;

    if (direction == PCI_OUT)
    {
			/*	S001 vvv	*/
        pci_write_devconfig8(cgnum, (uchar_t)bus,
                             (uchar_t)((device << 3) | func),
                             (uchar_t*)&databyte,
                             (ushort_t)reg);
        data = databyte;
    }
    else
    {
			/*	S001 vvv	*/
        pci_read_devconfig8(cgnum, (uchar_t)bus,
                            (uchar_t)((device << 3) | func),
                            (uchar_t*)&data,
                            (ushort_t)reg);
        
    }

    return(data);


} /* End function confbyte */


/*
** End of source module pci.c
*/

							/* END SCO_BASE */

