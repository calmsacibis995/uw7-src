/*
 *	@(#)pnp.c	7.1	10/22/97	12:29:02
 * File pnp.c for the Plug-and-Play driver
 *
 * @(#) pnp.c 65.6 97/07/12 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */


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

#ifdef UNIXWARE
int PnPdevflag = 0;	/* see man page for devflag(D1) */
#endif

const char	PnP_name[] = "PnP";	/* Driver prefix */
const char	PnP_cmdName[] = "pnp";	/* Command line driver name */

u_long		PnP_NumNodes = 0;
u_long		PnP_NodeSize = 0;
PnP_device_t	*PnP_devices = NULL;
PNP_MUTEXLOCK_T	PnP_devices_lock;
int		PnP_Alive = 0;
u_long		PnP_dataPort = PNP_MIN_READ_DATA_PORT;
caddr_t		PnP_rom_vaddr = NULL;
int		PnP_bios_len;
int		PnP_open_count = 0;

STATIC int PnP_ReadBootSpec(void);
STATIC void PnP_ConfigCheck(void);
STATIC void PnP_ResetLFSR(void);
STATIC int PnP_FindPNPbios(void);
STATIC int PnP_GatherBiosDevices(void);
STATIC u_long PnP_GatherDevices(void);
STATIC const cardID_t *PnP_FindCard(void);
STATIC void PnP_ConfigBootSpec(void);
STATIC void PnP_InvalidCfgMsg(const char *idStr, const char *valBuf,
							const char *msg);
STATIC void PnP_ConfigResMgr(void);
STATIC int PnP_AssignRes(PnP_device_t *dp, u_long devNum, PnP_res_type_t type,
						    u_long value, u_long limit);
STATIC const char *PnP_ResName(PnP_res_type_t type);

/* Functions from pnp_resmgr.c */
#ifdef UNIXWARE
void PnP_ReadResMgr(void);
void PnP_FreeAssignmentTable(void);
#endif

#ifdef PNP_DEBUG
    void pnp_dump(const char *msg, const void *buf, u_long len, u_long rel_adr);
#else
#   define pnp_dump(msg, buf, len, rel_adr)
#endif

/*
 * The major Plug-and-Play configuration is performed during the
 * init of the PnP driver.  This driver does not itself control
 * hardware.  There is no need for read or write routines.
 */

ENTRY_RETVAL
PnPinit()
{
    static int	initDone = 0;

    if (initDone)
	ENTRY_NOERR;

    initDone = 1;

    /*
     * PnPinit() may be called from another driver via
     * PnP_bus_data(), PnP_get_res() ...  However since PnPinit()
     * is called at least during its own init time, we can
     * safely assume that one will never get to this point
     * except at init time.
     */

    PNP_DPRINT(CE_CONT, "%s: init - DEBUG is ON\n", PnP_name);

    /*
     * Find out if we are in the game.
     */

    if (PnP_ReadBootSpec())
	ENTRY_NOERR;

    /*
     * The BIOS may have PnP features that have already
     * configured the devices.  Otherwise go find the cards the
     * hard way.
     */

    if (!PnP_FindPNPbios())
    {
	PNP_DPRINT(CE_CONT, "%s: PnP BIOS detected\n", PnP_name);

	PnP_GatherBiosDevices();
    }
    else
	PnP_GatherDevices();

    /*
     * Since we get to play this round, say so.
     */

#ifdef UNIXWARE
    cmn_err(CE_CONT, "%s: nodes=%lu auto=%d warn=%d\n", PnP_name,
#else	/* OpenServer */
    printcfg(PnP_name,
	    0,		/* base - No hardware address */
	    0,		/* offset - No hardware address */
	    -1,		/* vec - No IRQ */
	    -1,		/* dma - No DMA */
	    "nodes=%lu auto=%d warn=%d",
#endif
				    PnP_NumNodes,
				    PnP_auto ? 1 : 0,
				    PnP_warn ? 1 : 0);

    PnP_Alive = 1;

    if (!PnP_NumNodes)
    {
	PNP_DPRINT(CE_CONT, "%s: No devices detected\n", PnP_name);
	PnP_SetRunState();
	ENTRY_NOERR;
    }

    /*
     * Start configuring with those specifically requested.
     */

    PnP_ConfigBootSpec();

    if (!PnP_auto)
    {
	PnP_SetRunState();
	ENTRY_NOERR;
    }

    /*
     * Configure what we can from the configurations preserved
     * from the last boot.
     */

    PnP_ConfigResMgr();

    /*
     * Unless we are being quiet, display all of the
     * unconfigured cards so that we know there ID.
     */

    if (PnP_warn)
	PnP_ConfigCheck();

    /*
     * Place all cards in the operational state.
     */

    PnP_SetRunState();

    ENTRY_NOERR;
}

/*
 * This is only called during PnPinit().  PNP_LOCK(s) is not required.
 */

STATIC int
PnP_ReadBootSpec()
{
    /*
     * We can be asked not to play.
     *   pnp.auto	PnP_auto will be set TRUE
     *   pnp.auto=on	PnP_auto will be set TRUE
     *   pnp.auto=off	PnP_auto will be set FALSE
     */

#ifndef UNIXWARE
    if (getbsflag(PnP_cmdName, "disable") != BS_ABSENT)
    {
	PNP_DPRINT(CE_CONT, "%s: Disabled\n", PnP_name);
	return 1;
    }
#endif	/* !UNIXWARE */

    /*
     * We may not be on an ISA box or we may have been asked not
     * to play.  If so, just go away.
     */

    if (!(ARCH & (AT | EISA)))
    {
	PNP_DPRINT(CE_CONT, "%s: non-ISA; therefore no PnP\n", PnP_name);
	return 1;
    }

    /*
     * We can be asked to replace the default value for PnP_auto
     *   pnp.auto	PnP_auto will be set TRUE
     *   pnp.auto=on	PnP_auto will be set TRUE
     *   pnp.auto=off	PnP_auto will be set FALSE
     */

#ifndef UNIXWARE
    if (getbsflag(PnP_cmdName, "auto") != BS_ABSENT)
    {
	const char	*value;

	PnP_auto = 1;

	if (((value = getbsvalue(PnP_cmdName, "auto")) != NULL) &&
	    (kstrncmp(value, "off", 3) == 0))
		PnP_auto = 0;;
    }

    /*
     * We can be asked to replace the default value for PnP_warn
     *   pnp.warn	PnP_warn will be set TRUE
     *   pnp.warn=on	PnP_warn will be set TRUE
     *   pnp.warn=off	PnP_warn will be set FALSE
     */

    if (getbsflag(PnP_cmdName, "warn") != BS_ABSENT)
    {
	const char	*value;

	PnP_warn = 1;

	if (((value = getbsvalue(PnP_cmdName, "warn")) != NULL) &&
	    (kstrncmp(value, "off", 3) == 0))
		PnP_warn = 0;;
    }
#endif	/* !UNIXWARE */

    return 0;
}

/*
 * This is only called during PnPinit().  PNP_LOCK(s) is not required.
 */

STATIC void
PnP_ConfigCheck()
{
    PnP_device_t	*dp;

    for (dp = PnP_devices; dp; dp = dp->next)
    {
	u_char		device;

	for (device = 0; device < dp->devCnt; ++device)
	    if (PnP_ReadActive(dp, device))
		break;

	if (device >= dp->devCnt)
	    cmn_err(CE_NOTE, "%s: Card not configured: %s unit %d",
					PnP_name,
					PnP_idStr(dp->id.vendor),
					dp->unit);
    }
}

/*
 * This is only called during PnPinit().  PNP_LOCK(s) is not required.
 */

STATIC int
PnP_FindPNPbios()
{

#ifdef UNIXWARE		/* ## Use 16-bit virtual engine in UNIXWARE */

    return -1;

#else			/* OpenServer */

    const pnp_bios_t	*bios;
    caddr_t		addr;
    paddr_t		bios_start;
    paddr_t		base_paddr;
    caddr_t		base_vaddr;
    u_long		size;
    PnP_ISA_Config_t	Cfg;
    int			ret;
    static const struct seg_desc biosCode0GDT =
			MKDSCR(0L,0xFFFFL,KTEXT_ACC1,TEXT_ACC2);
    static const struct seg_desc biosCode1GDT =
			MKDSCR(0L,0xFFFFL,KTEXT_ACC1,TEXT_ACC2);
    static const struct seg_desc biosDataGDT =
			MKDSCR(0L,0xFFFFL,KDATA_ACC1,DATA_ACC2);


    /*
     * We can be asked to ignore the presence of the BIOS PnP. 
     * If so, we just do not look for it.
     */

    if (getbsflag(PnP_cmdName, "nobios") != BS_ABSENT)
	return -1;

    /*
     * The PnP BIOS signature structure is located on a 16 byte
     * boundry somewhere in the physical address range
     * 0x0f0000-0x0fffff.
     */

    bios_start = (paddr_t)0x0f0000;
    PnP_bios_len = 0x0fffff + 1 - 0x0f0000;

    if (!(PnP_rom_vaddr = PHYSMAP(bios_start, PnP_bios_len)))
    {
	cmn_err(CE_WARN, "%s: Cannot access PnP BIOS ROM", PnP_name);
	return -1;
    }

    PNP_DPRINT(CE_CONT,
		"%s: PnP_FindPNPbios() sptalloc(0x%x-0x%x) -> 0x%x-0x%x\n",
					PnP_name,
					bios_start,
					bios_start + PnP_bios_len - 1,
					PnP_rom_vaddr,
					PnP_rom_vaddr + PnP_bios_len - 1);

    for (addr = PnP_rom_vaddr; addr < (PnP_rom_vaddr+PnP_bios_len); addr += 16)
    {
	register u_char		sum;
	register const char	*tp;

	bios = (pnp_bios_t *)addr;

	if ((bios->sig.dword != PNPSIG) || (bios->len < PNPSIGLEN))
	    continue;

	sum = 0;
	for (tp = addr; tp < addr+PNPSIGLEN; ++tp)
	    sum += *tp;

	if (sum == 0)
	    break;
    }

    if (addr >= (PnP_rom_vaddr+PnP_bios_len))
    {
	PHYSFREE(PnP_rom_vaddr, PnP_bios_len);
	PnP_rom_vaddr=NULL;
	return -1;
    }

    PNP_DPRINT(CE_CONT, "%s: BIOS signature at 0x%x\n", PnP_name, ktop(bios));
    pnp_dump("Installation Check", bios, PNPSIGLEN, ktop(bios));

    PnP_BIOS_entry = (caddr_t)bios->pm16offset;
    PNP_DPRINT(CE_CONT, "%s: PnP BIOS code at %x:%x-%x %x\n",
				    PnP_name, PNPCODE1SEL,
				    bios->pm16codebase,
				    bios->pm16codebase + (64 * 1024) - 1,
				    PnP_BIOS_entry);

    PNP_DPRINT(CE_CONT, "%s: PnP BIOS data at %x:%x-%x\n",
				    PnP_name, PNPDATSEL,
				    bios->pm16database,
				    bios->pm16database + (64 * 1024) - 1);

    /*
     * If the ROM is not within the range already mapped,
     * get another map.  We need access to at least 64k.
     */

    base_paddr = bios->pm16codebase;

    if ((base_paddr < bios_start) ||
	(base_paddr > (bios_start+PnP_bios_len-(64 * 1024))))
    {
	PHYSFREE(PnP_rom_vaddr, PnP_bios_len);
	PnP_rom_vaddr=NULL;

	bios_start = base_paddr & ~(4096-1);
	PnP_bios_len = 64 * 1024;

	if (!(PnP_rom_vaddr = PHYSMAP(bios_start, PnP_bios_len)))
	{
	    cmn_err(CE_WARN, "%s: Cannot access PnP BIOS ROM", PnP_name);
	    return -1;
	}

	PNP_DPRINT(CE_CONT,
		"%s: PnP_FindPNPbios() sptalloc(0x%x-0x%x) -> 0x%x-0x%x\n",
				    PnP_name,
				    bios_start,
				    bios_start + PnP_bios_len - 1,
				    PnP_rom_vaddr,
				    PnP_rom_vaddr + PnP_bios_len - 1);
    }

    /*
     * Setup the required GDT entry
     * The sanity checks and recovery are because this is a new
     * driver and I know it will be ported to old OS versions
     * without providing a new sys/seg.h and ml/tables2.c.
     *
     * ## By the time we get here, the entries in gdt[] have
     * been changed from struct seg_desc to struct dscr.  So
     * this code is broken.  We need to understand that here and
     * set it up correctly.
     */

    size = (gdtend + 1) / sizeof(struct seg_desc);
    if ((seltoi(PNPCODE0SEL) >= size) ||
	(seltoi(PNPCODE1SEL) >= size) ||
	(seltoi(PNPDATSEL) >= size))
    {
	cmn_err(CE_WARN, "%s: gdt entry is invalid\n", PnP_name);
	PnP_NumNodes = 0;
	PnP_BIOS_entry = 0;	/* Force local attempt */
	return -1;
    }

    if (!gdt[seltoi(PNPCODE0SEL)].s_base && !gdt[seltoi(PNPCODE0SEL)].s_limacc)
    {
	PNP_DPRINT(CE_CONT, "%s: Providing gdt low page code entry\n", PnP_name);
	bcopy(&biosCode0GDT, &gdt[seltoi(PNPCODE0SEL)], sizeof(biosCode0GDT));
    }

    if (!gdt[seltoi(PNPCODE1SEL)].s_base && !gdt[seltoi(PNPCODE1SEL)].s_limacc)
    {
	PNP_DPRINT(CE_CONT, "%s: Providing gdt bios code entry\n", PnP_name);
	bcopy(&biosCode1GDT, &gdt[seltoi(PNPCODE1SEL)], sizeof(biosCode1GDT));
    }

    gdt[seltoi(PNPCODE1SEL)].s_base = bios->pm16codebase;

    if (!gdt[seltoi(PNPDATSEL)].s_base && ! gdt[seltoi(PNPDATSEL)].s_limacc)
    {
	PNP_DPRINT(CE_CONT, "%s: Providing gdt bios data entry\n", PnP_name);
	bcopy(&biosDataGDT, &gdt[seltoi(PNPDATSEL)], sizeof(biosDataGDT));
    }

    gdt[seltoi(PNPDATSEL)].s_base = bios->pm16database;

    /* ## reload gdtr ?? */

    /*
     * Get the ISA PnP Configuration from the BIOS
     */

#ifdef PNP_DEBUG
    /* calldebug(); */
#endif

    if ((ret = PnP_bios_call(0x40U, &Cfg)) != 0)
    {
	cmn_err(CE_WARN, "%s: BIOS config request failed: 0x%x", PnP_name, ret);
	PnP_NumNodes = 0;
	PnP_BIOS_entry = 0;	/* Force local attempt */
	return -1;
    }

    PnP_NumNodes = Cfg.NumNodes;
    PnP_dataPort = Cfg.ReadDataPort;
    PNP_DPRINT(CE_CONT, "%s: BIOS reported NumNodes %d, ReadDataPort %x\n",
					PnP_name, PnP_NumNodes, PnP_dataPort);
    return 0;

#endif	/* OpenServer */
}

STATIC int
PnP_GatherBiosDevices()
{
    u_long		Node;
    PnP_device_t	**dpp;


    PnP_NodeSize = 0;
    dpp = &PnP_devices;
    for (Node = 1; Node <= PnP_NumNodes; ++Node)
    {
	u_short			n;
	u_char			*tp;
	PnP_device_t		*dp;
	PnP_device_t		*tdp;
	static const size_t	alen = sizeof(PnP_device_t);
	int			endFound;

	/*
	 * Get a new structure to work with.
	 */

	if (!(dp = (PnP_device_t *)kmem_zalloc(alen, KM_NOSLEEP | KM_NO_DMA)))
	{
	    cmn_err(CE_WARN, "%s: Cannot get memory", PnP_name);
	    PnP_NumNodes = Node;
	    return ENOMEM;
	}

	*dpp = dp;
	dpp = &(*dpp)->next;

	/*
	 * Wakeup the card and read vendor, serial and chkSum.
	 */

	PnP_Wake(dp->id.CSN = Node);

	tp = (u_char *)&dp->id;
	for (n = 0; n < 9; ++n)
	    *tp++ = PnP_readResByte();

	/*
	 * Find out what unit number this will be
	 */

	for (tdp = PnP_devices; tdp; tdp = tdp->next)
	    if ((tdp->id.vendor == dp->id.vendor) &&
		(tdp->unit >= dp->unit))
		    dp->unit = tdp->unit + 1;

	/*
	 * Count the resources and node size
	 */

	for (endFound = 0; !endFound && (dp->NodeSize < 0xffffU); )
	{
	    u_short	len;
	    u_short	n;
	    u_char	resByte;

	    ++dp->ResCnt;

	    if (!((resByte = PnP_readResByte()) & 0x80U))
	    {
		len = resByte & 0x07U;

		switch ((resByte >> 3) & 0x0fU)
		{
		    case 0x02U:		/* Logical device ID */
			++dp->devCnt;
			break;

		    case 0x0fU:		/* End tag */
			endFound = 1;
			break;
		}
	    }
	    else
	    {
		len = PnP_readResByte() | (PnP_readResByte() << 8);
		dp->NodeSize += 2;
	    }

	    dp->NodeSize += (u_long)len + 1;
	    for (n = 0; n < len; ++n)
		PnP_readResByte();
	}

	if (!dp->devCnt)
	    dp->devCnt = 1;

	if (dp->NodeSize > PnP_NodeSize)
	    PnP_NodeSize = dp->NodeSize;

	/*
	 * And now a few words for debugging
	 */

	PNP_DPRINT(CE_CONT, "%s: %s: vendor 0x%x, serial 0x%x, unit %d, CSN 0x%b\n",
						    PnP_name,
						    PnP_idStr(dp->id.vendor),
						    dp->id.vendor,
						    dp->id.serial,
						    dp->unit,
						    dp->id.CSN);
	PNP_DPRINT(CE_CONT, "%s:          ResCnt %lu, NodeSize %lu, devCnt %u\n",
						    PnP_name,
						    dp->ResCnt,
						    dp->NodeSize,
						    dp->devCnt);
    }

    return 0;
}

/*
 * This is only called during PnPinit().  PNP_LOCK(s) is not required.
 */

STATIC u_long
PnP_GatherDevices()
{
    const cardID_t	*id;
    PnP_device_t	*dp;
    PnP_device_t	**dpp;
    static const size_t	len = sizeof(PnP_device_t);


    /*
     * Initialize to a known starting state
     */

    if (PnP_devices)
    {
	PnP_device_t	*ndp;

	for (dp = PnP_devices; dp; dp = ndp)
	{
	    ndp = dp->next;
	    kmem_free(dp, len);
	}

	PnP_devices = NULL;
    }

    /*
     * Initiation LFSR function
     */

    PnP_ResetLFSR();

    PnP_writeCmd(PNP_REG_ConfigControl, 0x07);
    SUSPEND(2000);	/* 2 msec */

    PnP_ResetLFSR();

    /*
     * Search for all the cards
     */

    PnP_NumNodes = 0;
    PnP_NodeSize = 0;
    for (dpp = &PnP_devices; (id = PnP_FindCard()) != NULL; dpp = &(*dpp)->next)
    {
	int	unit;
	int	endFound;

	/*
	 * Find out what unit number this will be
	 */

	unit = 0;
	for (dp = PnP_devices; dp; dp = dp->next)
	    if ((dp->id.vendor == id->vendor) &&
		(dp->unit >= unit))
		    unit = dp->unit + 1;

	/*
	 * List the new one
	 * We cannot sleep for it, we are in init().
	 */

	if (!(dp = (PnP_device_t *)kmem_zalloc(len, KM_NOSLEEP | KM_NO_DMA)))
	{
	    cmn_err(CE_WARN, "%s: Cannot get memory", PnP_name);
	    break;
	}

	dp->id = *id;		/* structure copy */
	dp->unit = unit;
	*dpp = dp;

	/*
	 * Count the resources and node size
	 */

	for (endFound = 0; !endFound && (dp->NodeSize < 0xffffU); )
	{
	    u_short	len;
	    u_short	n;
	    u_char	resByte;

	    ++dp->ResCnt;

	    if (!((resByte = PnP_readResByte()) & 0x80U))
	    {
		len = resByte & 0x07U;

		switch ((resByte >> 3) & 0x0fU)
		{
		    case 0x02U:		/* Logical device ID */
			++dp->devCnt;
			break;

		    case 0x0fU:		/* End tag */
			endFound = 1;
			break;
		}
	    }
	    else
	    {
		len = PnP_readResByte() | (PnP_readResByte() << 8);
		dp->NodeSize += 2;
	    }

	    dp->NodeSize += (u_long)len + 1;
	    for (n = 0; n < len; ++n)
		PnP_readResByte();
	}

	if (!dp->devCnt)
	    dp->devCnt = 1;

	if (dp->NodeSize > PnP_NodeSize)
	    PnP_NodeSize = dp->NodeSize;

	/*
	 * And now a few words for debugging
	 */

	PNP_DPRINT(CE_CONT, "%s: %s: vendor 0x%x, serial 0x%x, unit %d, CSN 0x%b\n",
						    PnP_name,
						    PnP_idStr(dp->id.vendor),
						    dp->id.vendor,
						    dp->id.serial,
						    dp->unit,
						    dp->id.CSN);
	PNP_DPRINT(CE_CONT, "%s:          ResCnt %lu, NodeSize %lu, devCnt %u\n",
						    PnP_name,
						    dp->ResCnt,
						    dp->NodeSize,
						    dp->devCnt);
    }

    return PnP_NumNodes;
}

/*
 * This is only called during PnPinit().  PNP_LOCK(s) is not required.
 */

STATIC void
PnP_ResetLFSR()
{
    static const u_char	primeLFSR[] =
    {
	0x006a, 0x00b5, 0x00da, 0x00ed, 0x00f6, 0x00fb, 0x007d, 0x00be,
	0x00df, 0x006f, 0x0037, 0x001b, 0x000d, 0x0086, 0x00c3, 0x0061,
	0x00b0, 0x0058, 0x002c, 0x0016, 0x008b, 0x0045, 0x00a2, 0x00d1,
	0x00e8, 0x0074, 0x003a, 0x009d, 0x00ce, 0x00e7, 0x0073, 0x0039
    };
    int		ix;

    outb(PNP_ADDRESS_PORT, 0);
    outb(PNP_ADDRESS_PORT, 0);

    SUSPEND(2000);	/* 2 msec */

    for (ix = 0; ix < sizeof(primeLFSR); ++ix)
	outb(PNP_ADDRESS_PORT, primeLFSR[ix]);

    SUSPEND(2000);
}

/*
 * This routine finds one more card.  This is only called during
 * PnPinit().  PNP_LOCK(s) is not required.
 */

STATIC const cardID_t *
PnP_FindCard()
{
    static int		first = 1;
    static cardID_t	cardID;
    u_char		chkSum;
    int			valid;
    register u_char	byte1;
    register u_char	byte2;

    if (PnP_dataPort > PNP_MAX_READ_DATA_PORT)
	return NULL;

    do
    {
	int	byte;
	int	bit;
	u_char	*id;

	PnP_Wake(0);
	PnP_writeCmd(PNP_REG_SetReadDataPort, PnP_dataPort >> 2);
	SUSPEND(1000);

	outb(PNP_ADDRESS_PORT, PNP_REG_ConfigIsolate);
	SUSPEND(1000);

	chkSum = 0x6aU;
	valid = 0;

	/*
	 * Get the vendor ID and serial number
	 */

	id = (u_char *)&cardID;

	for (byte = 0; byte < (2*sizeof(u_long)); ++byte)
	{
	    id[byte] = 0;

	    for (bit = 0; bit < 8; ++bit)
	    {
		register u_char	newBit;

		byte1 = inb(PnP_dataPort);
		SUSPEND(250);
		byte2 = inb(PnP_dataPort);
		SUSPEND(250);

		id[byte] >>= 1;

		if ((byte1 == 0x55U) && (byte2 == 0xaaU))
		{
		    id[byte] |= (newBit = 0x80U);
		    valid = 1;
		}
		else
		    newBit = 0;

		if (((chkSum >> 1) ^ chkSum) & 0x01)
		    newBit ^= 0x80U;

		chkSum >>= 1;
		chkSum |= newBit;
	    }
	}

	/*
	 * Get the checksum
	 */

	cardID.chkSum = 0;
	for (bit = 0; bit < 8; ++bit)
	{
	    byte1 = inb(PnP_dataPort);
	    SUSPEND(250);
	    byte2 = inb(PnP_dataPort);
	    SUSPEND(250);

	    cardID.chkSum >>= 1;

	    if ((byte1 == 0x55U) && (byte2 == 0xaaU))
	    {
		cardID.chkSum |= 0x80U;
		valid = 1;
	    }
	}

    } while ((cardID.chkSum != chkSum) &&
	     first &&
	     !valid &&
	     ((PnP_dataPort += 4) <= PNP_MAX_READ_DATA_PORT));

    if (first)
    {
	PNP_DPRINT(CE_CONT, "%s: PnP_dataPort: 0x%x\n", PnP_name, PnP_dataPort);
	first = 0;
    }

    if (cardID.chkSum != chkSum)
	return NULL;

    /*
     * Since CSN is only a u_char and we do not use CSN=0, ther
     * can not be more than 255 PnP devices.  Throw this one
     * away and do not look for more.
     */

    if (PnP_NumNodes >= 255)
    {
	cmn_err(CE_NOTE, "%s: Too many devices: %s",
					PnP_name,
					PnP_idStr(cardID.vendor));
	return NULL;
    }

    /*
     * Assign a CSN to the card.
     *
     * Thus spoke the spec:
     *    Valid Card Select Numbers for identified ISA cards
     *    range from 1 to 255 and must be assigned sequentially
     *    starting from 1.  The CSN is never set to zero.
     */

    PnP_writeCmd(PNP_REG_CSN, cardID.CSN = ++PnP_NumNodes);
    return &cardID;
}

/*
 * A resource is assigned a device if the resource is defined
 * within the boot string and is valid for the device as
 * described within the PNP hardware and does not conflict with
 * already configured PCI or PNP devices. All devices are
 * searched in this single pass assignment phase.  Assignments
 * are made on a first detected, first assigned basis.  Boot
 * string assignments take the form: 
 *   pnp.<pnpID>=(<res_name>:<dev_num>:<value>,...)
 *   pnp.<pnpID>=<unit>(<res_name>:<dev_num>:<value>,...)<unit>...
 *      Where res_name is one of: irq, dma, io, ram, rom
 *
 * This is only called during PnPinit().  PNP_LOCK(s) is not required.
 */

STATIC void
PnP_ConfigBootSpec()
{
#ifndef UNIXWARE		/* ## Insert UW bootstring code */
    PnP_device_t	*dp;

    for (dp = PnP_devices; dp; dp = dp->next)
    {
	char		valBuf[B_MAXSTRLEN];
	const char	*idStr = PnP_idStr(dp->id.vendor);
	char		*tp;
	u_short		unit;


	if (!(idStr = PnP_idStr(dp->id.vendor)))
	    continue;

	if (getbsword(PnP_cmdName, idStr, valBuf, sizeof(valBuf)) != BS_VALUE)
	    continue;

	PNP_DPRINT(CE_CONT, "%s: BootSpec: %s\n", PnP_name, valBuf);

	if (!*valBuf)
	{
	    PnP_InvalidCfgMsg(idStr, valBuf, "no value");
	    continue;
	}

	tp = valBuf;
	do
	{
	    /*
	     * Gather the unit number or take the default of zero
	     *    <unit>(<res_name>:<dev_num>:<value>,...)<unit>...
	     */

	    unit = 0;
	    if ((*tp >= '0') && (*tp <= '9'))
		while ((*tp >= '0') && (*tp <= '9'))
		{
		    unit = (unit * 10) + *tp - '0';
		    ++tp;
		}

	    if (*tp != '(')
	    {
		PnP_InvalidCfgMsg(idStr, valBuf, "'(' expected");
		break;
	    }

	    ++tp;	/* Pass the '(' */

	    PNP_DPRINT(CE_CONT, "%s: BootSpec unit %d: %s\n", PnP_name, unit, tp);

	    if (unit == dp->unit)
	    {
		u_long		devNum = 0;
		PnP_res_type_t	type = PNP_NONE;
		u_long		value = 0;
		u_long		limit = 0;

#ifdef NOT_YET	/* ## */
		/*
		 * Gather the settings
		 *	<res_name>:<dev_num>:<value>,...)
		 */

		##
#endif

		PnP_AssignRes(dp, devNum, type, value, limit);
	    }

	    /*
	     * Find the end of this unit spec
	     */

	    while (*tp && (*tp != ')'))
		++tp;

	    if (*tp)
		++tp;	/* Pass the ')' */

	} while (*tp);
    }
#endif
}

/*
 * This is only called during PnPinit().  PNP_LOCK(s) is not required.
 */

STATIC void
PnP_InvalidCfgMsg(const char *idStr, const char *valBuf, const char *msg)
{
    cmn_err(CE_NOTE, "%s: Invalid config spec: %s: %s.%s=%s",
					PnP_name,
					PnP_name,
					msg,
					idStr,
					valBuf);
}


STATIC void
PnP_ConfigResMgr()
{
    PnP_device_t	*dp;
    PnP_resSpec_t	*rp;
    rm_key_t		key;


#ifdef UNIXWARE
    /* Create the assignment table from the Res Mgr. */
    PnP_ReadResMgr();
#endif


    /*
     * Handle every entry in the assignment list.
     */

    for (dp = PnP_devices; dp; dp = dp->next)
    {
	/*
	 * Find all of the settings that we know for this card
	 */

	/* Loop through PnP_assign structure from Space.c */
	for (rp = PnP_assign; rp->type != PNP_NONE; ++rp)
	{
	    if ((rp->vendor == dp->id.vendor) &&
		((rp->serial == PNP_ANY_SERIAL) ||
		 (rp->serial == dp->id.serial)))
	    {
		if (rp->serial == PNP_ANY_SERIAL)
		    rp->serial = dp->id.serial;		/* First card takes */

		PnP_AssignRes(dp, rp->devNum, rp->type, rp->value, rp->limit);
	    }
	}
    }

#ifdef UNIXWARE
    PnP_FreeAssignmentTable();
#endif

}


ENTRY_RETVAL
#ifdef UNIXWARE
PnPopen(dev_t dev, int rw, int flag, cred_t *crp)
#else
PnPopen(dev_t dev, int rw, int flag)
#endif
{
    ++PnP_open_count;
    if (!PnP_Alive)
    {
	PNP_DPRINT(CE_CONT, "%s: Not alive\n", PnP_name);
	ENTRY_ERR(ENXIO);
    }
    ENTRY_NOERR;
}

ENTRY_RETVAL
PnPclose(dev_t dev, int rw, int flag)
{
    --PnP_open_count;
    ENTRY_NOERR;
}

/*
 * This is routine may only be called during PnPinit() or while
 * PNP_LOCK(s) is held.
 */

u_char
PnP_readRegister(PnP_Register_t regNum)
{
    outb(PNP_ADDRESS_PORT, regNum);
    return inb(PnP_dataPort);
}

/*
 * This is routine may only be called during PnPinit() or while
 * PNP_LOCK(s) is held.
 */

void
PnP_writeCmd(PnP_Register_t regNum, u_char value)
{
    outb(PNP_ADDRESS_PORT, regNum);
    outb(PNP_WRITE_DATA_PORT, value);
}

/*
 * This is routine may only be called during PnPinit() or while
 * PNP_LOCK(s) is held.
 */

void
PnP_Wake(u_char csn)
{
    PnP_writeCmd(PNP_REG_WakeCommand, csn);
}

/*
 * This is routine may only be called during PnPinit() or while
 * PNP_LOCK(s) is held.
 */

void
PnP_SetRunState()
{
    PnP_Wake(0);	/* All cards to sleep state */
}

/*
 * This is routine may only be called during PnPinit() or while
 * PNP_LOCK(s) is held.
 */

u_char
PnP_ReadActive(const PnP_device_t *dp, u_char device)
{
    PnP_Wake(dp->id.CSN);
    PnP_writeCmd(PNP_REG_LogicalDevNum, device);
    return PnP_readRegister(PNP_REG_Activate);
}

/*
 * This is routine may only be called during PnPinit() or while
 * PNP_LOCK(s) is held.
 */

int
PnP_WriteActive(const PnP_device_t *dp, u_char device, u_char active)
{
    PnP_Wake(dp->id.CSN);

    PnP_writeCmd(PNP_REG_LogicalDevNum, device);
    PnP_writeCmd(PNP_REG_Activate, active);
    return 0;
}

/*
 * This is routine may only be called during PnPinit() or while
 * PNP_LOCK(s) is held.
 */

u_char
PnP_readResByte()
{
    while (!PnP_readRegister(PNP_REG_Status) & 0x01U)
	;

    return PnP_readRegister(PNP_REG_ResourceData);
}

/*
 * This is routine may only be called during PnPinit() or while
 * PNP_LOCK(s) is held.
 */

void
PnP_writeResByte(u_char data)
{
    while (!PnP_readRegister(PNP_REG_Status) & 0x01U)
	;

    PnP_writeCmd(PNP_REG_ResourceData, data);
}

/*
 * This is routine may only be called during PnPinit() or while
 * PNP_LOCK(s) is held.
 */

STATIC int
PnP_AssignRes(PnP_device_t *dp, u_long devNum, PnP_res_type_t type,
						    u_long value, u_long limit)
{
    int		err = 0;
    u_char	active = 0x01;

    /*
     * Range check
     */

    switch (type)
    {
	case PNP_NONE:
	case PNP_DISABLE:
	    break;
	case PNP_IRQ_0:
	case PNP_IRQ_1:
	    PNP_DPRINT(CE_CONT, "%s: %s unit %d, devNum %u, %s %u\n",
					PnP_name,
					PnP_idStr(dp->id.vendor),
					dp->unit,
					devNum,
					PnP_ResName(type),
					value);
	    if (value > 15)
		err = ERANGE;
	    break;

	case PNP_DMA_0:
	case PNP_DMA_1:
	    PNP_DPRINT(CE_CONT, "%s: %s unit %d, devNum %u, %s %u\n",
					PnP_name,
					PnP_idStr(dp->id.vendor),
					dp->unit,
					devNum,
					PnP_ResName(type),
					value);
	    if (value & 0xfffffff8)
		err = ERANGE;
	    break;

	case PNP_IO_0:
	case PNP_IO_1:
	case PNP_IO_2:
	case PNP_IO_3:
	case PNP_IO_4:
	case PNP_IO_5:
	case PNP_IO_6:
	case PNP_IO_7:
	    PNP_DPRINT(CE_CONT, "%s: %s unit %d, devNum %u, %s 0x%x\n",
					PnP_name,
					PnP_idStr(dp->id.vendor),
					dp->unit,
					devNum,
					PnP_ResName(type),
					value);
	    if (value & 0xffff0000)
		err = ERANGE;
	    break;

	case PNP_MEM_0:
	case PNP_MEM_1:
	case PNP_MEM_2:
	case PNP_MEM_3:
	    PNP_DPRINT(CE_CONT, "%s: %s unit %d, devNum %u, %s 0x%x-0x%x\n",
					PnP_name,
					PnP_idStr(dp->id.vendor),
					dp->unit,
					devNum,
					PnP_ResName(type),
					value,
					limit);
	    if ((value & 0xff0000ff) || (limit & 0xff0000ff))
		err = ERANGE;
	    break;

	case PNP_MEM32_0:
	case PNP_MEM32_1:
	case PNP_MEM32_2:
	case PNP_MEM32_3:
	    PNP_DPRINT(CE_CONT, "%s: %s unit %d, devNum %u, %s 0x%x-0x%x\n",
					PnP_name,
					PnP_idStr(dp->id.vendor),
					dp->unit,
					devNum,
					PnP_ResName(type),
					value,
					limit);
	    break;

	default:
	    PNP_DPRINT(CE_CONT, "%s: %s unit %d, devNum %u, undefined 0x%x 0x%x\n",
					PnP_name,
					PnP_idStr(dp->id.vendor),
					dp->unit,
					devNum,
					value,
					limit);
	    cmn_err(CE_WARN, "%s: Unknown resource request", PnP_name);
	    return EINVAL;
    }

    if (err)
    {
	cmn_err(CE_WARN, "%s: Resource range error", PnP_name);
	return err;
    }

    /*
     * Write this resource
     */

    PnP_Wake(dp->id.CSN);
    PnP_writeCmd(PNP_REG_LogicalDevNum, devNum);

    switch (type)
    {
	case PNP_NONE:
	    break;

	case PNP_DISABLE:
	    active = 0x00;
	    break;

#define IRQ_TYPE	0x02	/* High true, Edge sensitive */

	case PNP_IRQ_0:
	    PnP_writeCmd(PNP_REG_IRQ_0_lvl, (u_char)value);
	    PnP_writeCmd(PNP_REG_IRQ_0_type, IRQ_TYPE);
	    break;

	case PNP_IRQ_1:
	    PnP_writeCmd(PNP_REG_IRQ_1_lvl, (u_char)value);
	    PnP_writeCmd(PNP_REG_IRQ_1_type, IRQ_TYPE);
	    break;

	case PNP_DMA_0:
	    PnP_writeCmd(PNP_REG_DMA_0, (u_char)value);
	    break;

	case PNP_DMA_1:
	    PnP_writeCmd(PNP_REG_DMA_1, (u_char)value);
	    break;

	case PNP_IO_0:
	    PnP_writeCmd(PNP_REG_IO_0_Hi, (u_char)(value >> 8));
	    PnP_writeCmd(PNP_REG_IO_0_Lo, (u_char)value);
	    break;

	case PNP_IO_1:
	    PnP_writeCmd(PNP_REG_IO_1_Hi, (u_char)(value >> 8));
	    PnP_writeCmd(PNP_REG_IO_1_Lo, (u_char)value);
	    break;

	case PNP_IO_2:
	    PnP_writeCmd(PNP_REG_IO_2_Hi, (u_char)(value >> 8));
	    PnP_writeCmd(PNP_REG_IO_2_Lo, (u_char)value);
	    break;

	case PNP_IO_3:
	    PnP_writeCmd(PNP_REG_IO_3_Hi, (u_char)(value >> 8));
	    PnP_writeCmd(PNP_REG_IO_3_Lo, (u_char)value);
	    break;

	case PNP_IO_4:
	    PnP_writeCmd(PNP_REG_IO_4_Hi, (u_char)(value >> 8));
	    PnP_writeCmd(PNP_REG_IO_4_Lo, (u_char)value);
	    break;

	case PNP_IO_5:
	    PnP_writeCmd(PNP_REG_IO_5_Hi, (u_char)(value >> 8));
	    PnP_writeCmd(PNP_REG_IO_5_Lo, (u_char)value);
	    break;

	case PNP_IO_6:
	    PnP_writeCmd(PNP_REG_IO_6_Hi, (u_char)(value >> 8));
	    PnP_writeCmd(PNP_REG_IO_6_Lo, (u_char)value);
	    break;

	case PNP_IO_7:
	    PnP_writeCmd(PNP_REG_IO_7_Hi, (u_char)(value >> 8));
	    PnP_writeCmd(PNP_REG_IO_7_Lo, (u_char)value);
	    break;

	case PNP_MEM_0:
	    PnP_writeCmd(PNP_REG_Memory_0_BaseHi, (u_char)(value >> 16));
	    PnP_writeCmd(PNP_REG_Memory_0_BaseLo, (u_char)(value >> 8));
	    if (PnP_readRegister(PNP_REG_Memory_0_Control) & 0x01U)
	    {
		if (limit)
		    limit -= value - 1;	/* limit -> range */
		PnP_writeCmd(PNP_REG_Memory_0_LimitHi, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory_0_LimitLo, (u_char)(limit >> 8));
	    }
	    else
	    {
		PnP_writeCmd(PNP_REG_Memory_0_LimitHi, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory_0_LimitLo, (u_char)(limit >> 8));
	    }
	    break;

	case PNP_MEM_1:
	    PnP_writeCmd(PNP_REG_Memory_1_BaseHi, (u_char)(value >> 16));
	    PnP_writeCmd(PNP_REG_Memory_1_BaseLo, (u_char)(value >> 8));
	    if (PnP_readRegister(PNP_REG_Memory_1_Control) & 0x01U)
	    {
		if (limit)
		    limit -= value - 1;	/* limit -> range */
		PnP_writeCmd(PNP_REG_Memory_1_LimitHi, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory_1_LimitLo, (u_char)(limit >> 8));
	    }
	    else
	    {
		PnP_writeCmd(PNP_REG_Memory_1_LimitHi, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory_1_LimitLo, (u_char)(limit >> 8));
	    }
	    break;

	case PNP_MEM_2:
	    PnP_writeCmd(PNP_REG_Memory_2_BaseHi, (u_char)(value >> 16));
	    PnP_writeCmd(PNP_REG_Memory_2_BaseLo, (u_char)(value >> 8));
	    if (PnP_readRegister(PNP_REG_Memory_2_Control) & 0x01U)
	    {
		if (limit)
		    limit -= value - 1;	/* limit -> range */
		PnP_writeCmd(PNP_REG_Memory_2_LimitHi, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory_2_LimitLo, (u_char)(limit >> 8));
	    }
	    else
	    {
		PnP_writeCmd(PNP_REG_Memory_2_LimitHi, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory_2_LimitLo, (u_char)(limit >> 8));
	    }
	    break;

	case PNP_MEM_3:
	    PnP_writeCmd(PNP_REG_Memory_3_BaseHi, (u_char)(value >> 16));
	    PnP_writeCmd(PNP_REG_Memory_3_BaseLo, (u_char)(value >> 8));
	    if (PnP_readRegister(PNP_REG_Memory_3_Control) & 0x01U)
	    {
		if (limit)
		    limit -= value - 1;	/* limit -> range */
		PnP_writeCmd(PNP_REG_Memory_3_LimitHi, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory_3_LimitLo, (u_char)(limit >> 8));
	    }
	    else
	    {
		PnP_writeCmd(PNP_REG_Memory_3_LimitHi, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory_3_LimitLo, (u_char)(limit >> 8));
	    }
	    break;

	case PNP_MEM32_0:
	    PnP_writeCmd(PNP_REG_Memory32_0_Base3, (u_char)(value >> 24));
	    PnP_writeCmd(PNP_REG_Memory32_0_Base2, (u_char)(value >> 16));
	    PnP_writeCmd(PNP_REG_Memory32_0_Base1, (u_char)(value >> 8));
	    PnP_writeCmd(PNP_REG_Memory32_0_Base0, (u_char)value);
	    if (PnP_readRegister(PNP_REG_Memory32_0_Control) & 0x01U)
	    {
		if (limit)
		    limit -= value - 1;	/* limit -> range */
		PnP_writeCmd(PNP_REG_Memory32_0_Limit3, (u_char)(limit >> 24));
		PnP_writeCmd(PNP_REG_Memory32_0_Limit2, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory32_0_Limit1, (u_char)(limit >> 8));
		PnP_writeCmd(PNP_REG_Memory32_0_Limit0, (u_char)limit);
	    }
	    else
	    {
		PnP_writeCmd(PNP_REG_Memory32_0_Limit3, (u_char)(limit >> 24));
		PnP_writeCmd(PNP_REG_Memory32_0_Limit2, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory32_0_Limit1, (u_char)(limit >> 8));
		PnP_writeCmd(PNP_REG_Memory32_0_Limit0, (u_char)limit);
	    }
	    break;

	case PNP_MEM32_1:
	    PnP_writeCmd(PNP_REG_Memory32_1_Base3, (u_char)(value >> 24));
	    PnP_writeCmd(PNP_REG_Memory32_1_Base2, (u_char)(value >> 16));
	    PnP_writeCmd(PNP_REG_Memory32_1_Base1, (u_char)(value >> 8));
	    PnP_writeCmd(PNP_REG_Memory32_1_Base0, (u_char)value);
	    if (PnP_readRegister(PNP_REG_Memory32_1_Control) & 0x01U)
	    {
		if (limit)
		    limit -= value - 1;	/* limit -> range */
		PnP_writeCmd(PNP_REG_Memory32_1_Limit3, (u_char)(limit >> 24));
		PnP_writeCmd(PNP_REG_Memory32_1_Limit2, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory32_1_Limit1, (u_char)(limit >> 8));
		PnP_writeCmd(PNP_REG_Memory32_1_Limit0, (u_char)limit);
	    }
	    else
	    {
		PnP_writeCmd(PNP_REG_Memory32_1_Limit3, (u_char)(limit >> 24));
		PnP_writeCmd(PNP_REG_Memory32_1_Limit2, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory32_1_Limit1, (u_char)(limit >> 8));
		PnP_writeCmd(PNP_REG_Memory32_1_Limit0, (u_char)limit);
	    }
	    break;

	case PNP_MEM32_2:
	    PnP_writeCmd(PNP_REG_Memory32_2_Base3, (u_char)(value >> 24));
	    PnP_writeCmd(PNP_REG_Memory32_2_Base2, (u_char)(value >> 16));
	    PnP_writeCmd(PNP_REG_Memory32_2_Base1, (u_char)(value >> 8));
	    PnP_writeCmd(PNP_REG_Memory32_2_Base0, (u_char)value);
	    if (PnP_readRegister(PNP_REG_Memory32_2_Control) & 0x01U)
	    {
		if (limit)
		    limit -= value - 1;	/* limit -> range */
		PnP_writeCmd(PNP_REG_Memory32_2_Limit3, (u_char)(limit >> 24));
		PnP_writeCmd(PNP_REG_Memory32_2_Limit2, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory32_2_Limit1, (u_char)(limit >> 8));
		PnP_writeCmd(PNP_REG_Memory32_2_Limit0, (u_char)limit);
	    }
	    else
	    {
		PnP_writeCmd(PNP_REG_Memory32_2_Limit3, (u_char)(limit >> 24));
		PnP_writeCmd(PNP_REG_Memory32_2_Limit2, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory32_2_Limit1, (u_char)(limit >> 8));
		PnP_writeCmd(PNP_REG_Memory32_2_Limit0, (u_char)limit);
	    }
	    break;

	case PNP_MEM32_3:
	    PnP_writeCmd(PNP_REG_Memory32_3_Base3, (u_char)(value >> 24));
	    PnP_writeCmd(PNP_REG_Memory32_3_Base2, (u_char)(value >> 16));
	    PnP_writeCmd(PNP_REG_Memory32_3_Base1, (u_char)(value >> 8));
	    PnP_writeCmd(PNP_REG_Memory32_3_Base0, (u_char)value);
	    if (PnP_readRegister(PNP_REG_Memory32_3_Control) & 0x01U)
	    {
		if (limit)
		    limit -= value - 1;	/* limit -> range */
		PnP_writeCmd(PNP_REG_Memory32_3_Limit3, (u_char)(limit >> 24));
		PnP_writeCmd(PNP_REG_Memory32_3_Limit2, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory32_3_Limit1, (u_char)(limit >> 8));
		PnP_writeCmd(PNP_REG_Memory32_3_Limit0, (u_char)limit);
	    }
	    else
	    {
		PnP_writeCmd(PNP_REG_Memory32_3_Limit3, (u_char)(limit >> 24));
		PnP_writeCmd(PNP_REG_Memory32_3_Limit2, (u_char)(limit >> 16));
		PnP_writeCmd(PNP_REG_Memory32_3_Limit1, (u_char)(limit >> 8));
		PnP_writeCmd(PNP_REG_Memory32_3_Limit0, (u_char)limit);
	    }
	    break;

	default:
	    err = EINVAL;
    }

    if (err == 0)
	PnP_WriteActive(dp, devNum, active);

    PnP_SetRunState();
    return err;
}

/*
 * This is routine may only be called during PnPinit() or while
 * PNP_LOCK(s) is held.
 */

PnP_device_t *
PnP_FindUnit(const PnP_unitID_t *up)
{
    PnP_device_t	*dp;

    for (dp = PnP_devices; dp; dp = dp->next)
	if (((up->vendor == PNP_ANY_VENDOR) ||
	     (up->vendor == dp->id.vendor)) &&
	    ((up->serial == PNP_ANY_SERIAL) ||
	     (up->serial == dp->id.serial)) &&
	    ((up->unit == PNP_ANY_UNIT) ||
	     (up->unit == dp->unit)) &&
	    ((up->node == PNP_ANY_NODE) ||
	     (up->node == dp->id.CSN)))
		    return dp;

    return NULL;
}

STATIC const char *
PnP_ResName(PnP_res_type_t type)
{
    struct
    {
	PnP_res_type_t	type;
	const char	*name;
    } resNames[] =
    {
	{ PNP_NONE, "none" },

	{ PNP_IRQ_0, "IRQ 0" },
	{ PNP_IRQ_1, "IRQ 1" },

	{ PNP_DMA_0, "DMA 0" },
	{ PNP_DMA_1, "DMA 1" },

	{ PNP_IO_0, "I/O 0" },
	{ PNP_IO_1, "I/O 1" },
	{ PNP_IO_2, "I/O 2" },
	{ PNP_IO_3, "I/O 3" },
	{ PNP_IO_4, "I/O 4" },
	{ PNP_IO_5, "I/O 5" },
	{ PNP_IO_6, "I/O 6" },
	{ PNP_IO_7, "I/O 7" },

	{ PNP_MEM_0, "MEM 0" },
	{ PNP_MEM_1, "MEM 1" },
	{ PNP_MEM_2, "MEM 2" },
	{ PNP_MEM_3, "MEM 3" },

	{ PNP_MEM32_0, "MEM32 0" },
	{ PNP_MEM32_1, "MEM32 1" },
	{ PNP_MEM32_2, "MEM32 2" },
	{ PNP_MEM32_3, "MEM32 3" },

	{ PNP_NONE, NULL }
    };
    int		n;

    for (n = 0; resNames[n].name; ++n)
	if (type == resNames[n].type)
	    return resNames[n].name;

    return "unknown";
}

#ifdef PNP_DEBUG
#define DMPLEN 16

void
pnp_dump(const char *msg, const void *buf, u_long len, u_long rel_adr)
{
    register int	x, y;
    u_char		text[DMPLEN];
    int			state;
    const u_char	*adr = (u_char *)buf;

#ifdef UNIXWARE
    PUTCHAR_INIT(160);		/* buffer size on UW/Gemini */
#endif


    printf("%s length: %lu", msg, len);

    if (!len)
    {
	putchar('\n');
	return;
    }

    state = 0;			/* We have not yet printed anything */
    while (len)
    {
	if ((state > 0) &&
	    (len >= DMPLEN) &&
	    (bcmp(adr, &adr[-DMPLEN], DMPLEN) == 0))
	{
	    if (state == 1)
	    {
		printf("\n  *");
		state = 2;	/* We are marking SAME */
	    }

	    adr += DMPLEN;
	    rel_adr += DMPLEN;
	    len -= DMPLEN;
	    continue;
	}

	state = 1;		/* There is a buffer to look back at */

	printf("\n  %x ", rel_adr);
	rel_adr += DMPLEN;

	for(x=0; x < DMPLEN && len; x++, len--)
		printf(" %b", (u_int)(text[x] = *adr++));
	for(y=x; y < DMPLEN; y++)
		printf("   ");
	printf(" :");
	for(y=0; y < x; y++)
	{
	    if ((text[y] >= 0x20U) && (text[y] < 0x80U))
		putchar(text[y]);
	    else
		putchar('.');
	}
	putchar(':');
    }

    if (state == 2)
	printf("\n  %4.4lx ", rel_adr);

    putchar('\n');
}

#endif



/* DLM wrapper code so we can load dynamically on UW/GEMINI */
#ifdef UNIXWARE


#include "sys/moddefs.h"


#define DRVNAME "PnP - ISA Plug and Play driver"


STATIC int      PnP_load();
STATIC int      PnP_unload();


MOD_DRV_WRAPPER(PnP, PnP_load, PnP_unload, NULL, DRVNAME);


/* Just call PnPinit() */
STATIC int
PnP_load()
{
        return PnPinit();
}




/*
** unload should clean up all our buffers so that we don't
** leak kernel memory if we load and unload several times.
*/
STATIC int
PnP_unload()
{
        /* If somebody still has us open, don't unload us! */
        if(PnP_open_count > 0)
                return(EBUSY);


        /* Free up sptalloc'd space */
        if(PnP_rom_vaddr)
		{
                PHYSFREE(PnP_rom_vaddr, PnP_bios_len);
		PnP_rom_vaddr=NULL;
		}


        /* Free kmem_zalloc'd space */
        if (PnP_devices)
        {
                PnP_device_t    *ndp, *dp;

                /* Traverse linked list */
                for (dp = PnP_devices; dp; dp = ndp)
                {
                    ndp = dp->next;
                    kmem_free(dp, sizeof(PnP_device_t));
                }

                PnP_devices = NULL;
        }

        return(0);
}


#endif
