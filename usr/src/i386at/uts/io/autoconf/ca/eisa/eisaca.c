#ident	"@(#)kern-i386at:io/autoconf/ca/eisa/eisaca.c	1.28.6.1"
#ident	"$Header$"

/*
 * Autoconfig -- CA/EISA inferface.
 */

#include <svc/eisa.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>

#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/autoconf/ca/ca.h>
#include <io/autoconf/ca/eisa/nvm.h>
#include <svc/v86bios.h>

#ifdef DEBUG
STATIC int ca_eisa_debug = 0;
#define DEBUG1(a)	if (ca_eisa_debug == 1) printf a
#define DEBUG2(a)	if (ca_eisa_debug == 2) printf a
#else
#define DEBUG1(a)
#define DEBUG2(a)
#endif /* DEBUG */

/*
 * Macros to get bytes from longs.
 */
#define BYTE0(x)	((x) & 0x0FF)
#define BYTE1(x)	(((x) >> 8) & 0x0FF)
#define BYTE2(x)	(((x) >> 16) & 0x0FF)
#define BYTE3(x)	(((x) >> 24) & 0x0FF)

/*
 * The following is defined in eisa.cf/Space.c
 */
extern int ca_eisa_realmode;
extern uint_t eisa_slot_mask;
extern uint_t eisa_dma_mask;

/*
 * int
 * ca_eisa_access_type(void)
 *
 * Calling/Exit State:
 *	Return the access type -- real mode or prot mode.
 */
int
ca_eisa_access_type(void)
{
	return (ca_eisa_realmode);
}


/*
 * int
 * ca_v86bios_eisa_read_nvm(int slot, uchar_t *data, int *errorp)
 *	Fill the data buffer from an already read eisa cmos memory.
 *
 * Calling/Exit State:
 *	Inputs:
 *		<data> is a pointer to a 16K buffer allocated by the caller.
 *		<errorp> is a return pointer to the slot status.
 *
 *	Outputs:
 *		Returns number of functions.
 *		<errorp> is a return pointer to the slot status.
 *
 * Remarks:
 *	The "ca_v86bios_eisa_read_nvm" is a function for extracting config.
 *	data from an already read EISA non-volatile memory. The NVRAM 
 *	space was read while there was window to make real mode bios
 *	calls. The EISA NVRAM data is stored in the following fromat:
 *
 *	---------------------------------
 *	|	Slot Information	|
 *	---------------------------------
 *	|	1st Function Info.	|
 *	---------------------------------
 *	|	2nd Function Info.	|
 *	---------------------------------
 *	|				|
 *	---------------------------------
 *	|	nth Function Info.	|
 *	---------------------------------
 */

int
ca_v86bios_eisa_read_nvm(int slot, uchar_t *data, int *errorp)
{
	int		f;
	uchar_t		*dst;
	size_t		sz;
	eisa_nvm_slotinfo_t 	
	  		slotinfo;

	if ((*errorp = v86bios_eisa_read_slot(slot, &slotinfo)))
		return 0;

	/*
	 * Copy the NVRAM data information. The size of the data
	 * buffer must not be greater than the max eisa buffer size.
	 * sz below is used to check it out.
	 */
	bcopy(&slotinfo, data, EISA_NVM_SLOTINFO_SIZE);

	f = 0;
	sz = EISA_NVM_SLOTINFO_SIZE + EISA_NVM_FUNCINFO_SIZE; 
	dst = data + EISA_NVM_SLOTINFO_SIZE;

	for (; (f < slotinfo.functions) && (sz <= EISA_BUFFER_SIZE); 
	     f++, sz += EISA_NVM_FUNCINFO_SIZE,
		     dst += EISA_NVM_FUNCINFO_SIZE) {
		v86bios_eisa_read_func(slot, f, dst);
	}	
	
#ifdef DEBUG
	cmn_err(CE_CONT, 
		"Exiting ca_v86bios_eisa_read_nvm: nfuncs=0x%x\n", slotinfo.functions);
#endif

	return (slotinfo.functions);
}

/*
 * int
 * ca_eisa_read_nvm(int slot, uchar_t *data, int *errorp)
 *	Fill the data buffer from EISA cmos memory.
 *
 * Calling/Exit State:
 *	<slot> is the EISA device slot number.
 *	<data> is a pointer to a minimum 4K size buffer allocated by the caller.
 *	<errorp> is a return pointer to the slot status.
 *	
 * Remarks:
 *	The "ca_eisa_read_nvm" is a wrapper function for extracting 
 *	configuration data from EISA non-volatile memory thru either
 *	protected-mode BIOS calls or thru mapping the data that was 
 *	read during system startup using real-mode BIOS calls.
 */
int
ca_eisa_read_nvm(int slot, uchar_t *data, int *errorp)
{
	ASSERT(slot < EISA_MAX_SLOTS);

	if (ca_eisa_access_type()) {
		int	nfuncs = 0;		/* number of functions */
	
		if (cm_bustypes() & CM_BUS_EISA)
			nfuncs = ca_v86bios_eisa_read_nvm(slot, data, errorp);
		return (nfuncs);
	}

	return (eisa_read_nvm(slot, data, errorp));
}


#define	NEXT_FUNCINFO(funcp)	((funcp)++)

/*
 * The three character "ASCII" string identifies the commonly
 * used function types.
 */
char *eisa_types[] = {
	"COM",			/* communication ports */
	"KEY",			/* keyboard */
	"MEM",			/* memory board */
	"MFC",			/* multifunction board */
	"MSD",			/* mass storage device */
	"NET",			/* network adapter */
	"VID",			/* video display adapter */
	"NPX",			/* numeric coprocessor */
	"OSE",			/* operating system/environment */
	"OTH",			/* other */
	"PAR",			/* parallel port */
	"PTR",			/* pointing device */
	"SYS",			/* system board */
	"CPU",			/* processor board */
	"SCSI"			/* SCSI device */
};

#define EISA_TYPES      (sizeof eisa_types / sizeof eisa_types[0])

#define	EISA_EXP_IOAS	8	/* # of I/O address ranges */

/*
 * Table of expansion slot I/O addresses.
 */
struct eisa_ioa {
	ushort_t	sioa;
	ushort_t	eioa;
	uchar_t		ioaflag;
#define	EISA_SLOT_IOA		0x01		/* slot dependent i/o addr */
#define	EISA_ALIAS_IOA		0x02		/* slot independent i/o addr */
} eisa_exp_slot_ioa[EISA_EXP_IOAS] = {
	{ 0x0000, 0x00ff, EISA_SLOT_IOA },
	{ 0x0100, 0x03ff, EISA_ALIAS_IOA },
	{ 0x0400, 0x04ff, EISA_SLOT_IOA },
	{ 0x0500, 0x07ff, EISA_ALIAS_IOA },
	{ 0x0800, 0x08ff, EISA_SLOT_IOA },
	{ 0x0900, 0x0bff, EISA_ALIAS_IOA },
	{ 0x0c00, 0x0cff, EISA_SLOT_IOA },
	{ 0x0d00, 0x0fff, EISA_ALIAS_IOA },
};


#ifdef NOTYET

/*
 * boolean_t
 * eisa_check_ioa_range(ushort_t ioa, ushort_t ioa1)
 *
 * Calling/Exit State:
 *	Return B_TRUE if <ioa> and <ioa1> are in the same range
 *	of eisa_exp_slot_ioa table, otherwise return B_FALSE.
 */
boolean_t
eisa_check_ioa_range(ushort_t ioa, ushort_t ioa1)
{
	int i;


	ioa = (ioa < 0x1000 ? ioa : ioa % 0x1000);
	ioa1 = (ioa1 < 0x1000 ? ioa1 : ioa1 % 0x1000);

	for (i = 0; i < 8; i++) {
		if (ioa >= eisa_exp_slot_ioa[i].sioa &&
		    ioa <= eisa_exp_slot_ioa[i].eioa &&
		    ioa1 >= eisa_exp_slot_ioa[i].sioa &&
		    ioa1 <= eisa_exp_slot_ioa[i].eioa)
			return B_TRUE;
	}

	return B_FALSE;
}

#endif /* NOTYET */


/*
 * boolean_t
 * eisa_check_slot_ioa(ushort_t addr)
 *
 * Calling/Exit State:
 *	Return B_TRUE if it is a slot-specific address, otherwise
 *	return B_FALSE.
 */
boolean_t
eisa_check_slot_ioa(ushort_t addr)
{
	int i;
	ushort_t ioa = (addr < 0x1000 ? addr : addr % 0x1000);


	for (i = 0; i < 8; i++) {
		if (ioa >= eisa_exp_slot_ioa[i].sioa &&
		    ioa <= eisa_exp_slot_ioa[i].eioa &&
		    eisa_exp_slot_ioa[i].ioaflag == EISA_SLOT_IOA)
			return B_TRUE;
	}

	return B_FALSE;
}


/*
 * int 
 * ca_eisa_parse_nvm(char *data, int nfuncs, struct config_info *cip)
 *
 * Calling/Exit State:
 *	Return 0 on success, otherwise return a non-zero value.
 *	<data> is the slot EISA NVRAM data.
 *	<nfuncs> is the number of functions to parse.
 *	<cip> is a pointer to <config_info> data structure to be initialized.
 *
 * Remarks:
 *	See comments above for the format of NVRAM data.
 *
 *	Resource information for a single device can span multiple 
 *	consecutive function blocks.
 *
 *	Each slot can have unspecified number of function blocks.
 *
 *	In order to distinguish multiple devices per board, we can
 *	use one of the following heuristic:
 *		
 *	1) repeat
 *		If a bit mask for the resource (IRQ, DMA, IOPORT, MEM) is set
 *			mark the beginning of the next device
 *		else
 *			set a resource bit in the <mask> 
 *
 *	   until all the function blocks in the slot are parsed
 *
 *	2) Use "type" or "subtype" string as a delimiter between
 *	   logical devices.
 *
 *	3) The primary device in the NVRAM is before the secondary
 *	   device in a multi-function card.
 *
 *
 *	On the PC, XT, AT and ISA platforms, only 1K of the total 64K
 *	I/O address space is used. The first 256 bytes are reserved for
 *	I/O platform resources, such as the interrupt and DMA controllers
 *	(addresses 0x00 -- 0xff). The remaining 768 bytes are available
 *	to general I/O slave card resources (addresses 0x100 -- 0x3ff).
 *	In that only 1K of the address space is supported since ISA add-on
 *	I/O slave cards only decode the first 10 address signal lines.
 *
 *	EISA System I/O Map looks like the following:
 *
 *	----------------------------------
 *	| ISA System board peripherals.  | 0x0000 - 0x00ff
 *	----------------------------------
 *	| ISA expansion boards           | 0x0100 - 0x03ff
 *	----------------------------------
 *	| Reserved-system board cntls    | 0x0400 - 0x04ff
 *	----------------------------------
 *	| Alias of 0x100 -- 0x3ff        | 0x0500 - 0x07ff
 *	----------------------------------
 *	| System Board                   | 0x0800 - 0x08ff
 *	----------------------------------
 *	| Alias of 0x100 -- 0x3ff        | 0x0900 - 0x0Bff
 *	----------------------------------
 *	| System Board                   | 0x0C00 - 0x0Cff
 *	----------------------------------
 *	| Alias of 0x100 -- 0x3ff        | 0x0D00 - 0x0fff
 *	----------------------------------
 *	---------------------------------- 
 *	| Slot "z"                       | 0z000 - 0z0ff
 *	----------------------------------
 *	| Alias of 0x100 -- 0x3ff        | 0z100 - 0z3ff
 *	----------------------------------
 *	| Slot "z"                       | 0z400 - 0z4ff
 *	----------------------------------
 *	| Alias of 0x100 -- 0x3ff        | 0z500 - 0z7ff
 *	----------------------------------
 *	| Slot "z"                       | 0z800 - 0z8ff
 *	----------------------------------
 *	| Alias of 0x100 -- 0x3ff        | 0z900 - 0zBff
 *	----------------------------------
 *	| Slot "z"                       | 0zC00 - 0zCff
 *	----------------------------------
 *	| Alias of 0x100 -- 0x3ff        | 0zD00 - 0zFFF
 *	----------------------------------
 *
 *	The system board I/O range resides at I/O addresses between
 *	0x000 and 0xfff in slot 0.
 *
 *	The I/O addresses between 0x1000 - 0xffff that are NOT identified
 *	as "Alias of 0x100-0x3ff" are reserved for slot-specific addressing
 *	of expansion boards.
 *
 *	The I/O addresses marked as "Alias of 0x100-0x3ff" are the ISA
 *	expansion cards emulated addresses.
 *
 *	I/O addresses between 0x400 and 0x4ff are reserved for current
 *	and future EISA system board peripherals defined by this spec.
 *
 *	The system board manufacturers may use system board addresses
 *	0x800-0x80f and 0xc00-0xc0f for manufacturer-specific I/O devices.
 */
int
ca_eisa_parse_nvm(char *data, int nfuncs, struct config_info *cip)
{
	int	i;		/* resource index in nvm data */
	int	j;		/* resource index in config_info */
	int	f;		/* function number */
	eisa_nvm_funcinfo_t *funcp;
	eisa_nvm_slotinfo_t *slotp;


	slotp = (eisa_nvm_slotinfo_t *)data;
	funcp = (eisa_nvm_funcinfo_t *)(data + EISA_NVM_SLOTINFO_SIZE);

	bcopy(funcp->boardid, &cip->ci_eisabrdid, 4);

	switch (slotp->dupid.type) {
	case EISA_NVM_EXP_SLOT:
		DEBUG1(("%d (0x%x): Found an expansion slot\n", f, funcp));
		break;
	case EISA_NVM_EMB_SLOT:
		DEBUG1(("%d (0x%x): Found an embedded slot\n", f, funcp));
		break;
	case EISA_NVM_VIR_SLOT:
		DEBUG1(("%d (0x%x): Found a virtual slot\n", f, funcp));
		break;
	default:
		/*
		 *+ An unknown EISA bus slot which is neither an expansion,
		 *+ embedded or a virtual slot. 
		 */
		cmn_err(CE_NOTE,
			"ca_eisa_parse_nvm: Unknown slot type");
		return 0;
	};

	cip->ci_eisa_reserved = slotp->dupid.type;

	DEBUG1(("data=0x%x, slotp=0x%x, funcp=0x%x boardid=%d\n", 
		data, slotp, funcp, cip->ci_eisabrdid));
	eisa_print_slot(slotp);
	eisa_print_func(funcp);

	ASSERT(slotp->functions);

	for (f = 0; f < nfuncs; f++) { 

		/*
		 * Check if the functions are disabled.
		 */
		if (funcp->fib.disable) {
			/* no functions to parse */
			DEBUG1(("%d (0x%x): No function blocks\n", f, funcp));
			NEXT_FUNCINFO(funcp);
			continue;
		}

		/*
		 * Parse memory (base, size and attributes).
		 */
		if (funcp->fib.memory) {
			struct config_memory_info	*cmemip;

			DEBUG1(("%d (0x%x): Parsing MEM\n", f, funcp));

			for (j = cip->ci_nummemwindows, i = 0; j < MAX_MEM_REGS; j++, i++) {
				if (i == NVM_MAX_MEMORY)
					break;

				cmemip = (struct config_memory_info *)&cip->ci_memattr[j];
				cmemip->cmemi_rdwr = 
					funcp->enfi_memory[i].config.write;
				cmemip->cmemi_shared = 
					funcp->enfi_memory[i].config.share;

				/*
				 * Save the memory base address.
				 */
				cip->ci_membase[j] = 
					(funcp->enfi_memory[i].start[2] << 16) |
					(funcp->enfi_memory[i].start[1] << 8) |
					funcp->enfi_memory[i].start[0];
				cip->ci_membase[j] *= 0x100;

				/*
				 * Save the memory size/range.
				 */
				if (funcp->enfi_memory[i].size) {
					cip->ci_memlength[j] = 
						funcp->enfi_memory[i].size;
					cip->ci_memlength[j] *= 0x400;
				} else {
					cip->ci_memlength[j] = 64 * 0x100000;
				}

				cip->ci_nummemwindows++;

				if (funcp->enfi_memory[i].config.more == 0)
					break;
			}

			/*
			 * Concatenate/Merge sequential memory ranges. The
			 * assumption here is that the entries are sorted in
			 * ascending order.
			 */
			if (cip->ci_nummemwindows > 1) {
				ulong_t base = cip->ci_membase[0];
				ulong_t length = cip->ci_memlength[0];
				ushort_t nmemws = 1;

				for (i = 0; ((i < cip->ci_nummemwindows) &&
					((i+1) < cip->ci_nummemwindows)); i++) {
				    if ((cip->ci_membase[i] + cip->ci_memlength[i]) == cip->ci_membase[i+1]) {
				        length += cip->ci_memlength[i+1];
					continue;
				    } else {
					cip->ci_membase[nmemws - 1] = base;
					cip->ci_memlength[nmemws - 1] = length;
					base = cip->ci_membase[i+1];
					length = cip->ci_memlength[i+1];
					nmemws++;
				    }
				}
				cip->ci_membase[nmemws - 1] = base;
				cip->ci_memlength[nmemws - 1] = length;
				cip->ci_nummemwindows = nmemws;
			}
		} 

		/*
		 * Parse interrupt configuration information (irq line,
		 * sensitivity, and shareabitlity).
		 */
		if (funcp->fib.irq) {
			struct config_irq_info	*cirqp;

			DEBUG1(("%d (0x%x): Parsing IRQ\n", f, funcp));

			for (j = cip->ci_numirqs, i = 0; j < MAX_IRQS; j++, i++) {
				if (i == NVM_MAX_IRQ)
					break;

				cirqp = (struct config_irq_info *)&cip->ci_irqattrib[j];
				cirqp->cirqi_trigger = 
					funcp->enfi_irq[i].trigger;
				cirqp->cirqi_type = 
					funcp->enfi_irq[i].share;
				cip->ci_irqline[j] = 
					funcp->enfi_irq[i].line;

				cip->ci_numirqs++;

				if (funcp->enfi_irq[i].more == 0)
					break;
			}
		} 

		/*
		 * Parse DMA channel and attributes (type)
		 */
		if (funcp->fib.dma) {
			struct config_dma_info	*cdmaip;
			uint_t	bit;

			DEBUG1(("%d (0x%x): Parsing DMA\n", f, funcp));

			for (j = cip->ci_numdmas, i = 0; j < MAX_DMA_CHANNELS; j++, i++) {
				if (i == NVM_MAX_DMA)
					break;

				cdmaip = (struct config_dma_info *)&cip->ci_dmaattrib[j];
				/*	
				 * The adse resmgr entry gets a bogus DMA
				 * channel entry because of an ambiguous
				 * multi-function ECU entry that mixed in
				 * information about the Adaptec controller's
				 * on-board floppy controller with legitimate
				 * information about the Adaptec controller.
				 * Since other HBAs provide this capability,
				 * the recommendation is to make a change in
				 * one place rather than possibly modify 
				 * several HBA drivers.
				 *
				 * KLUDGE: Modify the EISA configuration access 
				 * parse code to specifically prevent "DMA=2"
				 * entry in EISA NVRAM from being added to the
				 * the resmgr. This is OK because DMA channel
				 * 2 is defined as belonging to the floppy on
				 * AT-class systems. This behavior is also 
				 * controllable via a space.c file dma mask.
				 */
				bit = 1;
				bit <<= funcp->enfi_dma[i].channel;
				if (eisa_dma_mask & bit) {
					if (funcp->enfi_dma[i].more == 0)
						break;
					else
						continue;
				}

				if (funcp->enfi_dma[i].timing == NVM_DMA_TYPEC)
					cdmaip->cdmai_timing2 = 0x01;
				else
					cdmaip->cdmai_timing1 = 
						funcp->enfi_dma[i].timing;

				cdmaip->cdmai_type = funcp->enfi_dma[i].share;

				switch (funcp->enfi_dma[i].width) {
				case NVM_DMA_BYTE:
					cdmaip->cdmai_transfersize = 0x00;
					break;
				case NVM_DMA_WORD:
					cdmaip->cdmai_transfersize = 0x10;
					break;
				case NVM_DMA_DWORD:
					cdmaip->cdmai_transfersize = 0x11;
					break;
				};

				cip->ci_dmachan[j] = funcp->enfi_dma[i].channel;
				cip->ci_numdmas++;

				if (funcp->enfi_dma[i].more == 0)
					break;
			}
		} 

		/*
		 * Parse I/O PORT (base address and size).
		 */
		if (funcp->fib.port) {
			DEBUG1(("%d (0x%x): Parsing Port\n", f, funcp));

			for (j = cip->ci_numioports, i = 0; j < MAX_IO_PORTS; j++, i++) {
				if (i == NVM_MAX_PORT)
					break;

				/*
				 * 00000 => 1 Ports, 11111 => 32 Ports 
				 *			(EISA Spec. )
				 */
				cip->ci_ioport_base[j] = 
					funcp->enfi_port[i].address;

				/*
				 * Convert the slot independent addresses to
				 * the slot dependent addresses. 
				 *
				 * The emulated addresses are not used by the
				 * driver, but we need to make the conversion
				 * blindly to make the idtools (idcheck, etc)
				 * happy so as to prevent any conflicts in a 
				 * multi-controller system (e.g multiple DPT
				 * controllers configured at same emulated
				 * address 0x170-0x177).
				 *
				 * (See remarks above in the commentary).
				 */
				if (cip->ci_ioport_base[j] < 0x1000) {
					cip->ci_ioport_base[j] =
					    (cip->ci_eisa_slotnumber * 0x1000) +
					    cip->ci_ioport_base[j];
				}

				cip->ci_ioport_length[j] = 
					funcp->enfi_port[i].count + 1;
				cip->ci_numioports++;

				if (funcp->enfi_port[i].more == 0)
					break;
			}

			/*
			 * Concatenate/Merge sequential ioports. The 
			 * assumption here is that the entries are sorted 
			 * in ascending order.
			 */
			if (cip->ci_numioports > 1) {
				ushort_t base = cip->ci_ioport_base[0];
				ushort_t length = cip->ci_ioport_length[0];
				ushort_t ioports = 1;

				for (i = 0; ((i < cip->ci_numioports) &&
					((i+1) < cip->ci_numioports)); i++) {
				    if ((cip->ci_ioport_base[i] + cip->ci_ioport_length[i]) == cip->ci_ioport_base[i+1]) {
				        length += cip->ci_ioport_length[i+1];
					continue;
				    } else {
					cip->ci_ioport_base[ioports - 1] = base;
					cip->ci_ioport_length[ioports - 1] = length;
					base = cip->ci_ioport_base[i+1];
					length = cip->ci_ioport_length[i+1];
					ioports++;
				    }
				}
				cip->ci_ioport_base[ioports - 1] = base;
				cip->ci_ioport_length[ioports - 1] = length;
				cip->ci_numioports = ioports;
			}

#ifdef NOTYET
			/*
			 * There should be no gaps in the I/O space.
			 *
			 * Calculate the min and max of the I/O base addr
			 * and add the length of the max I/O base address
			 * back to the max I/O base address.
			 */
			if (cip->ci_numioports > 1) {
				ushort_t minslioa = 0;	/* min slot-dep IOA */
				ushort_t minalioa = 0;	/* min alias IOA */
				ushort_t maxslioa = 0;	/* max slot-dep IOA */
				ushort_t maxalioa = 0;	/* max alias IOA */
				ushort_t slioasize, alioasize;
				ushort_t ioa, ioasize;

				for (i = 0; i < cip->ci_numioports; i++) {
				    ioa = cip->ci_ioport_base[i];
				    ioasize = cip->ci_ioport_length[i];

				    if (eisa_check_exp_slot(ioa)) {
					if (minslioa && maxslioa) {
					    /*
					     * Set minslioa and maxslioa if 
					     * the addresses are within the 
					     * same range.
					     */
					    if (eisa_check_ioa_range(minslioa, ioa)
					    	minslioa = min(minslioa, ioa);
					    if (eisa_check_ioa_range(maxslioa, ioa)
						maxslioa = max(maxslioa, ioa);
					    slioasize = ((maxslioa == ioa) ?
							ioasize : slioasize);  
					} else {
					    minslioa = ioa;
					    maxslioa = ioa;
					    slioasize = ioasize;
					}
				    } else {
					if (minalioa && maxalioa) {
					    /*
					     * Set minslioa and maxslioa if 
					     * the addresses are within the 
					     * same range.
					     */
					    if (eisa_check_ioa_range(minalioa, ioa))
				    		minalioa = min(minalioa, ioa);
					    if (eisa_check_ioa_range(maxalioa, ioa))
						maxalioa = max(maxalioa, ioa);
					    alioasize = ((maxalioa == ioa) ?
							ioasize : alioasize);
					} else {
					    minalioa = ioa;
					    maxalioa = ioa;
					    alioasize = ioasize;
					}
				    }
				}

				cip->ci_numioports = 0;

				if (minslioa && maxalioa) {
				    i = cip->ci_numioports++;
				    cip->ci_ioport_base[i] = minslioa;
				    cip->ci_ioport_length[i] = 
					(maxslioa - minslioa) + slioasize;
				}

				if (minalioa && maxalioa) {
				    i = cip->ci_numioports++;
				    cip->ci_ioport_base[i] = minalioa;
				    cip->ci_ioport_length[i] = 
					(maxalioa - minalioa) + alioasize;
				}
			}
#endif /* NOTYET */
		}

		/*
		 * Parse type and subtype string.
		 */
		if (funcp->fib.type) {
			for (i = 0; i < EISA_TYPES; i++) {
				if (eisa_parse_func((void *)funcp,
				    (void *)eisa_types[i], 0) == CA_SUCCESS)
					break;
			}

			DEBUG1(("%d (0x%x): Parsing Type %s\n", 
				f, funcp, ((i < EISA_TYPES) ? eisa_types[i] : "UNKNOWN")));

			strncpy(cip->ci_type, (char *)funcp->type, MAX_TYPE);
		} 

		/*
		 * Parse free-form data.
		 */
		if (funcp->fib.data) {
			DEBUG1(("%d (0x%x): Parsing Free-form Data\n", f, funcp));
		}

		NEXT_FUNCINFO(funcp);
	}

#ifdef NOTYET
	/*
	 * Initialize the I/O parameter to maximum values for the slot
	 * specific I/O port ranges that may possibly be consumed by
	 * the driver. It is only necessary if there is no indication
	 * thru the NVRAM that the I/O resource is used by the device.
	 * Since, these I/O addresses cannot be allocated to any device,
	 * it does not harm to initialize them to the maximum allowable 
	 * values. It is assumed that the driver has the intelligence,
	 * to know if it requires any I/O port resource.
	 */
	if (cip->ci_numioports == 0) {
		for (i = 0; i < EISA_EXP_IOAS; i++) {
			if (eisa_exp_slot_ioa[i].ioaflag == EISA_ALIAS_IOA)
				continue;
			ASSERT(eisa_exp_slot_ioa[i].ioaflag == EISA_SLOT_IOA);
			cip->ci_ioport_base[cip->ci_numioports] = 
				(cip->ci_eisa_slotnumber * 0x1000) +
				eisa_exp_slot_ioa[i].sioa;
			cip->ci_ioport_length[cip->ci_numioports] =
				eisa_exp_slot_ioa[i].eioa - eisa_exp_slot_ioa[i].sioa;
			cip->ci_numioports++;
		}
	} 
#endif /* NOTYET */

	if (cip->ci_numioports == 0) {
		/* assign the maximum slot address */
		cip->ci_ioport_base[cip->ci_numioports] =
			(cip->ci_eisa_slotnumber * 0x1000) + 0xfff;
		cip->ci_ioport_length[cip->ci_numioports] = 0;
		cip->ci_numioports++;
	}

	return 0; 
}


/*
 * int
 * ca_eisa_read_devconfig(uchar_t slot, uchar_t func, char *buf, size_t off, size_t len)
 *	Read device specific configuration information.
 *
 * Calling/Exit State:
 *	Returns number of bytes read on success, otherwise returns -1.
 */
int
ca_eisa_read_devconfig(uchar_t slot, uchar_t func, char *buf, size_t off, size_t len)
{
	char	*ebuf;		/* EISA NVM buffer */
	int	nfuncs;		/* number of functions */
	int	error = 0;


	if (off > EISA_BUFFER_SIZE || buf == NULL)
		return (-1);
 
	if ((ebuf = (char *) kmem_zalloc(
			EISA_BUFFER_SIZE, KM_NOSLEEP)) == 0) {
		/*
		 *+ Could not allocate buffer to store the EISA
		 *+ non-volitile memory data.
		 */
		cmn_err(CE_WARN,
			"ca_eisa_read_devconfig: could not allocate"
			" memory for EISA buffer\n");
		return (-1);
	}

	nfuncs = ca_eisa_read_nvm(slot, (uchar_t *)ebuf, &error);

	/*
	 * The size of NVM data must be greater than the offset requested
	 * by the driver.
	 */
	if ((nfuncs * EISA_NVM_FUNCINFO_SIZE) < off) {
		kmem_free(ebuf, EISA_BUFFER_SIZE);
		return (-1);
	} else {
		bcopy(ebuf + (func * EISA_NVM_FUNCINFO_SIZE + off), buf, len);
		kmem_free(ebuf, EISA_BUFFER_SIZE);
		return (len);
	}
}


/*
 * int
 * ca_eisa_write_devconfig(uchar_t slot, uchar_t func, char *buf, size_t off, size_t len)
 *	Write device specific configuration information.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
ca_eisa_write_devconfig(uchar_t slot, uchar_t func, char *buf, size_t off, size_t len)
{
	return (-1);
}


/*
 * int
 * ca_eisa_devconfig_size(uchar_t slot)
 *	Get the maximum size of device configuration space information.
 *
 * Calling/Exit State:
 *	None.
 */
size_t
ca_eisa_devconfig_size(uchar_t slot)
{
	char	*buf;
	size_t	sz = 0;
	int	nfuncs;		/* number of functions */
	int	error;


	if ((buf = (char *) kmem_zalloc(
			EISA_BUFFER_SIZE, KM_NOSLEEP)) == 0) {
		/*
		 *+ Could not allocate buffer to store the EISA
		 *+ non-volitile memory data.
		 */
		cmn_err(CE_WARN,
			"ca_eisa_devconfig_size: could not allocate"
			" memory for EISA buffer");
		return (0);
	}

	if ((nfuncs = ca_eisa_read_nvm(slot, (uchar_t *)buf, &error)))
		sz = EISA_NVM_SLOTINFO_SIZE + nfuncs * EISA_NVM_FUNCINFO_SIZE;

	kmem_free(buf, EISA_BUFFER_SIZE);

	return (sz);
}


/*
 * int
 * ca_eisa_init(void)
 *	Read and parse the configuration space for each slot and
 *	initialize the config_info for each device on the board.
 *
 * Calling/Exit State:
 *	Return 0 on success and non-zero on failure.
 */
int
ca_eisa_init(void)
{
	int	s;		/* slot number */
	int	f;		/* function number */
	int	pfuncs;		/* number of functions parsed */
	int	nfuncs;		/* number of functions */
	char	*buf;
	struct config_info *cip;
	uint_t	bit;
	int	error = 0;
	int	ret = 0;


	if ((buf = (char *) kmem_zalloc(
			EISA_BUFFER_SIZE, KM_NOSLEEP)) == 0) {
		/*
		 *+ Could not allocate buffer to store the EISA
		 *+ non-volatile memory data.
		 */
		cmn_err(CE_WARN,
			"ca_eisa_init: could not allocate"
			" memory for EISA buffer\n");
		return (ENOMEM);
	}

	for (s = 0, bit = 1; s < EISA_MAX_SLOTS; s++, bit <<= 1) {
		if (bit & eisa_slot_mask)
			continue;
		if ((nfuncs = ca_eisa_read_nvm(s, (uchar_t *)buf, &error))) {
			CONFIG_INFO_KMEM_ZALLOC(cip);
			cip->ci_busid = CM_BUS_EISA;
			cip->ci_eisa_slotnumber = s;
			cip->ci_eisa_funcnumber = 0;
			ca_eisa_parse_nvm(buf, nfuncs, cip);
			bzero(buf, EISA_SLOTINFO_SIZE + 
					nfuncs * EISA_FUNCINFO_SIZE);
		} else {
			if (error == EISA_CORRUPT_NVRAM || 
			    error == EISA_INVALID_SETUP) {
				ret = EIO;
				break;
			}
		}
	}

	kmem_free(buf, EISA_BUFFER_SIZE);

	return (ret);
}


/*
 * boolean_t
 * ca_eisa_clone_slot(ulong_t ba, ulong_t dba)
 *
 * Calling/Exit State:
 *	<ba> is the bus_access information for an already registered device.
 *	<dba> is the bus_access information for a similar device found in
 *	another slot.
 *
 *	Return B_TRUE if it is a clone slot, otherwise return B_FALSE.
 */
boolean_t
ca_eisa_clone_slot(ulong_t ba, ulong_t dba)
{
	ASSERT(BYTE0(ba) < EISA_MAX_SLOTS);
	ASSERT(BYTE0(dba) < EISA_MAX_SLOTS);

	/*
	 * Check slot no. and slot type -- vir, exp or emb.
	 *
	 * This is to differentiate system resource
	 * information for a single device distributed
	 * across multiple slots. We cannot just rely
	 * on slot no. to distinguish between a virtual
	 * or an expansion slot, because the notion of
	 * virtual slot is indicated by a type field in
	 * the eisa_nvm_slotinfo_t data structure and
	 * an expansion slot (slot# < 16) can be marked
	 * as a virtual slot by this field.
	 */
	if ((BYTE0(ba) != BYTE0(dba)) &&
	    (BYTE2(dba) == EISA_NVM_VIR_SLOT))
		return B_TRUE;

	return B_FALSE;
}

/*
** int
** eisa_read_devconfig32( uchar_t slot, uchar_t func, uint_t *buf, size_t off )
**
** Description:
**	Read 4 bytes of device specific configuration information.
**
** Calling/Exit State:
**	Returns 0 for success, EINVAL otherwise.
*/

int
eisa_read_devconfig32( uchar_t slot, uchar_t func, uint_t *buf, size_t off )
{

#ifdef CM_TEST
	printf( "In eisa_read_devconfig32()\n" );
#endif /* CM_TEST */

	if ( ca_eisa_read_devconfig( slot, func, (char *)buf, off, 4 ) == 4 )
		return 0;

	return EINVAL;
}

/*
** int
** eisa_write_devconfig32( uchar_t slot, uchar_t func, uint_t *buf, size_t off )
**
** Description:
**	Write 4 bytes of device specific configuration information.
**
**	CURRENTLY NOT SUPPORTED !!!
**
** Calling/Exit State:
**	Returns 0 for success, EINVAL otherwise.
*/

int
eisa_write_devconfig32( uchar_t slot, uchar_t func, uint_t *buf, size_t off )
{

#ifdef CM_TEST
	printf( "In eisa_write_devconfig32()\n" );
#endif /* CM_TEST */

	return EINVAL;
}
