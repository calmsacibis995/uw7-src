/*
 * Copyright (c) 1998 The Santa Cruz Operation, Inc.. All Rights Reserved. 
 *                                                                         
 *        THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF THE               
 *                   SANTA CRUZ OPERATION INC.                             
 *                                                                         
 *   The copyright notice above does not evidence any actual or intended   
 *   publication of such source code.                                      
 */

/*
 * Copyright (c) 1998 The Santa Cruz Operation, Inc.. All Rights Reserved. 
 *                                                                         
 *        THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF THE               
 *                   SANTA CRUZ OPERATION INC.                             
 *                                                                         
 *   The copyright notice above does not evidence any actual or intended   
 *   publication of such source code.                                      
 */

#ident	"@(#)kern-i386at:io/autoconf/ca/pci/pci_bios.c	1.12.11.2"
#ident	"$Header$"

#include <proc/regset.h>
#include <svc/errno.h>
#include <svc/psm.h>
#include <svc/systm.h>
#include <svc/v86bios.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/emask.h>
#include <util/engine.h>

#include <io/ddi.h>
#include <io/ddi_i386at.h>
#include <io/autoconf/ca/ca.h>
#include <io/autoconf/ca/pci/pci.h>

#if defined(DEBUG) || defined(DEBUG_TOOLS)
STATIC int ca_pci_bios_debug = 0;
STATIC int ca_pci_bypass = 0;
#define DBG1(s)	if(ca_pci_bios_debug == 1) printf(s)
#define DBG2(s)	if(ca_pci_bios_debug == 2) printf(s)

#else
#define	DBG1(s)
#define DBG2(s)
#endif

/*
 * Autoconfig -- PCI BIOS setup (pci_verify) and BIOS calls
 */

/*
 * The functions provided within allow the pcica 'pci_init' function to
 * probe the PCI BIOS for various data elements. 
 *
 *
 * major routines:
 * pci_verify: finds the 32-bit BIOS entry point for the PCI BIOS and
 * saves it in the per-bus info structure.  Checks to be sure there
 * really is a PCI BIOS as well.

 * pci_read_devconfig8,16, 32 	read from a PCI card's config space
 * pci_write_devconfig8,16,32	write to a particular card's config space
 *
 * Uses: _pci_bios_call, the generic interface to do a protect mode BIOS call
 * given a protected-mode entry point.
 * 
 *
 *
 */

extern void pci_alloc_bus_data(void);
extern int pci_read_devconfig8(ms_cgnum_t cgnum, uchar_t bus,
			       uchar_t devfun, uchar_t *buf,
			       ushort_t offset);
extern int pci_read_devconfig16(ms_cgnum_t cgnum, uchar_t bus,
				uchar_t devfun, ushort_t *buf,
				ushort_t offset);
extern int pci_read_devconfig32(ms_cgnum_t cgnum, uchar_t bus,
				uchar_t devfun, uint_t *buf,
				ushort_t offset);
extern int pci_write_devconfig8(ms_cgnum_t cgnum, uchar_t bus,
				uchar_t devfun, uchar_t *buf,
				ushort_t offset);
extern int pci_write_devconfig16(ms_cgnum_t cgnum, uchar_t bus,
				 uchar_t devfun, ushort_t *buf,
				 ushort_t offset);
extern int pci_write_devconfig32(ms_cgnum_t cgnum, uchar_t bus,
				 uchar_t devfun, uint_t *buf,
				 ushort_t offset);
extern int pci_bios_call(ms_cgnum_t cgnum, regs *reg,
			 unsigned char command);

char	*pci_scan	= "YES";
extern int upyet;
extern struct pci_bus_data *pci_cg_bus_data;

/*
 * This variable exists only to work around limitations of NUMA
 * platforms which can't handle BIOS calls on non-boot CGs correctly.
 */
boolean_t are_cross_cg_bios_calls_supported = B_TRUE;

/*
 * int
 * pci_verify(ms_cgnum_t cgnum)
 * 
 * Calling/Exit State:
 *	Find if there is a PCI BIOS on the specified CG.
 *	return 0 if so, -1 if not
 */
int
pci_verify(ms_cgnum_t cgnum)
{
	char * bios32_service;
	caddr_t	pci_bios_paddr;
	char	*bios32_directory;
	int	i, offset, bios_size;
	int	pci_bios_length;
	int	pci_bios_offset;
	int	csum(char *);
	regs	reg;
	struct pci_bus_data  ignore;
	struct pci_bus_data *bus_datap;

        if (pci_cg_bus_data == NULL) 
                pci_alloc_bus_data();
        bus_datap = &pci_cg_bus_data[cgnum];

	/*
	* Algorithm:
	*
	*	According to the PCI BIOS spec 2.0, 32-bit protected
	*	mode BIOS access to the PCI devices is guaranteed.
	*	To find the BIOS itself, you have to see if:
	*
	*	1) there is a 'BIOS 32 directory'
	*	2) there's a PCI BIOS in that directory
	*	3) that PCI BIOS indeed IDs to being a PCI BIOS
	*
	*	In cases (1) and (2), this involves mapping in some 
	*	physical memory, and hunting for strings.  In case 3,
	*	a BIOS call (using _pci_bios_call) is actually
	*	made and a return value checked.
	*/
#ifdef DEBUG
	if (ca_pci_bypass == 1){
		cmn_err(CE_CONT, "Bypassing PCI bus check for CG %Ld!\n",
			cgnum);
		return -1;
	}
#endif
	if (!(strcmp(pci_scan,"YES") == 0 ||
		strcmp(pci_scan,"2.0") == 0 ||
		strcmp(pci_scan,"2.1") == 0)){
		cmn_err(CE_NOTE, "Bypassing PCI bus scan on CG %d.\n",
			cgnum);
		return -1;
	}

	return (find_pci_id(cgnum, &ignore));
}

/*
 * void
 * pci_alloc_bus_data(void)
 * 
 *	Allocate the CG-indexed array of PCI bus data if it isn't
 *	already allocated.  Panic if the allocation fails.
 */
void
pci_alloc_bus_data(void)
{
	if (pci_cg_bus_data != NULL)
		return;

	pci_cg_bus_data = kmem_zalloc((os_cgnum_max + 1) *
			  sizeof(struct pci_bus_data), KM_NOSLEEP); 

	if (pci_cg_bus_data == NULL) {
		/*
		 *+ Failed to allocate memory at system init.
		 *+ This is happening at a time where there should
		 *+ be a great deal of free memory on the system.
		 *+ Corrective action:  Check the kernel configuration
		 *+ for excessive static data space allocation or
		 *+ increase the amount of memory on the system.
		 */
		cmn_err(CE_PANIC, "ca_pci_init: kmem_zalloc failed");
	}
}

/*
 * int
 * csum(char *)
 * 
 * Calling/Exit State:
 *	Find if the 11 bytes pointed at by p sum to 0. Necessary
 *	for determining presence of BIOS 32 service dir. Return
 *	the sum.
 */
static int
csum(char *p)
{
	/* inelegant way to compute checksum of BIOS 32 service directory */
	uchar_t sum;
	if (p == 0) return -1;
	sum = p[0] + p[1] + p[2] + p[3] + p[4] + p[5] + p[6] + p[7] + p[8] +
		p[9] + p[10];	
	return (int) sum;
}


/*
 * int
 * find_pci_id(ms_cgnum_t cgnum, struct pci_bus_data *buf)
 * 
 * Calling/Exit State:
 *	Invoke the "PCI BIOS PRESENT" BIOS call on the specified CG.
 *	Report the available data back to the caller.  A quick way to
 *	determine whether you're on a PCI system
 *
 * Arguments:
 *	cgnum:				the CG to test
 *	struct pci_bus_data *buf:	pointer to store some bus info
 *
 * Return values: 0 if successful, -1 if BIOS call failed somehow
 */
int
find_pci_id(ms_cgnum_t cgnum, struct pci_bus_data *bus_datap)
{
	regs reg;

	bzero(&reg, sizeof(reg));
	pci_bios_call(cgnum, &reg, PCI_BIOS_PRESENT);

	if (reg.edx.edx != PCI_ID_REG_VAL)
		return -1;

	bus_datap->pci_maxbus = reg.ecx.byte.cl;
	bus_datap->pci_bus_rev = reg.ebx.word.bx;
	bus_datap->pci_conf_cycle_type = reg.eax.byte.al;
	return 0;
}

/*
 * int
 * pci_generate__special_cycle(ms_cgnum_t cgnum, uchar_t bus, uint_t data)
 *
 * Calling/Exit State:
 *      Generate a PCI special cycle. May cause system reset. Should
 *      normally never be done by a driver.
 *
 * Arguments:
 *	cgnum:		the CG to do this on
 *      uchar_t bus:    bus to generate special cycle on
 *      uint_t data:    special cycle data
 *
 * Return values: 0 if successful, AH value if fail (if it comes back)
 */
int
pci_generate__special_cycle(ms_cgnum_t cgnum, uchar_t bus, uint_t data)
{
        regs reg;

        bzero(&reg, sizeof(reg));
        reg.ebx.byte.bh = bus;
        reg.edx.edx = data;
        pci_bios_call(cgnum, &reg, PCI_GENERATE_SPECIAL_CYCLE);
        return (int) reg.eax.byte.ah;
}

/*
 * int
 * generate_pci__special_cycle(uchar_t bus, uint_t data)
 *
 * Calling/Exit State:
 *      Generate a PCI special cycle. May cause system reset. Should
 *      normally never be done by a driver.
 *
 * Arguments:
 *      uchar_t bus:    bus to generate special cycle on
 *      uint_t data:    special cycle data
 *
 * Return values: 0 if successful, AH value if fail (if it comes back)
 *
 * Note:
 *	This function exists *solely* for backward compatibility for
 *	its only caller, io/odi/lsl/lslrealmode.c.   It is hardwired
 * 	to generate a special cycle on CG 0, because ODI drivers are
 *	by definition not NUMA-capable.
 */
int
generate_pci_special_cycle(uchar_t bus, uint_t data)
{
	return pci_generate__special_cycle(0, bus, data);
}

/*
 * int
 * pci_<read|write>_devconfig[8|16|32](ms_cgnum_t cgnum, uchar_t bus,
 *					uchar_t devfun, ushort_t offset,
 *					value)
 * 
 * Calling/Exit State:
 *	read or write the # of bits at the given PCI device/function, bus
 * 	config space location.
 *
 *
 * Arguments:
 *	ms_cgnum_t cgnum: CG containing desired PCI bus hierarchy
 *	uchar_t bus:	bus # to write at
 *	uchar_t devfun:	PCI device/function byte(see PCI BIOS spec for details)
 *	ushort_t offset:Which of the 256 registers to write in
 *	value:	properly sized value to read from/write to
 *
 * Algorithm: read/write (based on name) the value given, using appropriate
 * PCI BIOS call. Uses _pci_bios_call entry point as to make call.
 *
 * Return values: 0 if successful, PCI_FAILURE if BIOS call failed somehow
 */
int
pci_read_devconfig8(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
		    uchar_t *ret, ushort_t offset)
{
	regs reg;
	pl_t oldprio;

	bzero(&reg, sizeof(reg));
	reg.ebx.byte.bh = bus;
	reg.ebx.byte.bl = devfun;
	reg.edi.word.di = offset;

	pci_bios_call(cgnum, &reg, PCI_READ_CONFIG_BYTE);
	*ret = reg.ecx.byte.cl;
	return (int) reg.eax.byte.ah == 0 ? 0 : EINVAL;
}

int
pci_read_devconfig16(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
		     ushort_t *ret, ushort_t offset)
{
	regs reg;
	pl_t oldprio;

	bzero(&reg, sizeof(reg));
	reg.ebx.byte.bh = bus;
	reg.ebx.byte.bl = devfun;
	reg.edi.word.di = offset;

	pci_bios_call(cgnum, &reg, PCI_READ_CONFIG_WORD);
	*ret = reg.ecx.word.cx;
	return (int) reg.eax.byte.ah == 0 ? 0 : EINVAL;
}	

int
pci_read_devconfig32(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
		     uint_t *ret, ushort_t offset)
{
	regs reg;
	pl_t oldprio;

	bzero(&reg, sizeof(reg));
	reg.ebx.byte.bh = bus;
	reg.ebx.byte.bl = devfun;
	reg.edi.word.di = offset;

	pci_bios_call(cgnum, &reg, PCI_READ_CONFIG_DWORD);
	*ret = reg.ecx.ecx;
	return (int) reg.eax.byte.ah == 0 ? 0 : EINVAL;
}

/* write routines */

int
pci_write_devconfig8(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
		     uchar_t *buf, ushort_t offset)
{
	regs reg;
	pl_t oldprio;

	bzero(&reg, sizeof(reg));
	reg.ebx.byte.bh = bus;
	reg.ebx.byte.bl = devfun;
	reg.edi.word.di = offset;
	reg.ecx.byte.cl = *buf;

	pci_bios_call(cgnum, &reg, PCI_WRITE_CONFIG_BYTE);
	return (int) reg.eax.byte.ah == 0 ? 0 : EINVAL;
}

int
pci_write_devconfig16(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
		      ushort_t *buf, ushort_t offset)
{
	regs reg;
	pl_t oldprio;

	bzero(&reg, sizeof(reg));
	reg.ebx.byte.bh = bus;
	reg.ebx.byte.bl = devfun;
	reg.edi.word.di = offset;
	reg.ecx.word.cx = *buf;

	pci_bios_call(cgnum, &reg, PCI_WRITE_CONFIG_WORD);
	return (int) reg.eax.byte.ah == 0 ? 0 : EINVAL;
}

int
pci_write_devconfig32(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
		      uint_t *buf, ushort_t offset)
{
	regs reg;
	pl_t oldprio;

	bzero(&reg, sizeof(reg));
	reg.ebx.byte.bh = bus;
	reg.ebx.byte.bl = devfun;
	reg.edi.word.di = offset;
	reg.ecx.ecx = *buf;

	pci_bios_call(cgnum, &reg, PCI_WRITE_CONFIG_DWORD);
	return (int) reg.eax.byte.ah == 0 ? 0 : EINVAL;
}


/*
 * int
 * pci_[read|write]devconfig(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
 *			uchar_t * buffer, int offset, int length)
 *
 * [read|write] 'length' bytes at offset 'offset' from device (bus, devfun) into
 * buffer. Note: locking handled by lower level routines now (vs. UW2.01)
 *
 * Calling/Exit State:
 *	Return the number of bytes read (length)
 * Error conditions:
 *	if (offset + length) >  the size of PCI config space, put out a warning
 *		message and return -1
 */
int
pci_read_devconfig(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
		   uchar_t *buffer, int offset, int length)
{
	int i;

	if (pci_verify(cgnum) != 0) /* can't set up BIOS access */
		return PCI_FAILURE;

	if (offset + length > 256){
		cmn_err(CE_WARN,
		       "!Tried to read past the end of PCI config space.\n");	
		return -1;
	}

	for (i = offset; i < offset + length; i++) {
		if (pci_read_devconfig8(cgnum, bus, devfun, buffer, i)
		    != 0)
			return -1;

		buffer++;
	}
	return length;
}
int
pci_write_devconfig(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
	 	    uchar_t *buffer, int offset, int length)
{
	int i;

	if (pci_verify(cgnum) != 0) /* can't set up BIOS access */
		return PCI_FAILURE;

	if (offset + length > 256) {
		cmn_err(CE_WARN,
		       "!Tried to write past the end of PCI config space.\n");	
		return -1;
	}

	for (i = offset; i < offset + length; i++) {
		if (pci_write_devconfig8(cgnum, bus, devfun, buffer, i)
		   != 0)
			return -1;

		buffer++;
	}

	return length;
}

/*
 * size_t
 * pci_devconfig_size(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun)
 *
 * Calling/Exit State:
 *	Return the size of device configuration space.
 */
/* ARGSUSED */
size_t
pci_devconfig_size(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun)
{
	if (pci_verify(cgnum) != 0) /* can't set up BIOS access */
		return (0);

	return (MAX_PCI_REGISTERS);
}

/*
 * int
 *	pci__get_irq_routing_options(uchar_t *buf, size_t *size,
 *			ushort_t *bitmap) 
 *
 *	Get PCI interrupt routing options. The caller has to allocate the
 *	buffer pointed to by <buf, *size> for DataBuffer. The IRQ bitmap is
 *	returned in bitmap. *size is upated to the value retuned by the
 *	BIOS. 
 * 
 * 	Returns 0 if successfull. Otherwise, PCI error code, if any. 
 *
 * REMARKS:
 * 	If *size == 0, then it will be updated with the required size,
 *	returning PCI_BUFFER_TOO_SMALL. Since the size is unknown, typecally
 *	this routine is called twice, one for the size, and another for
 *	getting real information. 
 *
 */

int
pci__get_irq_routing_options(uchar_t *buf, size_t *size, ushort_t *bitmap)
{
	regs			reg;
	v86bios_buf_t		b_copyout, b_copyin;
	struct pci_routebuffer	rb;
	int			ret;
	
	/*
	 * The RouteBuffer is located at the base of the buffer of V86BIOS,
	 * followed by DataBuffer. The location of the DataBuffer is
	 * sepcified by the rb.addr. The selector 
	 */

	rb.sz = *size;
	rb.addr = (void *) (V86BIOS_PDATA_BASE + sizeof (rb));
	rb.selector = 0;

	b_copyout.kaddr = &rb;
	b_copyout.v86addr = (void *) V86BIOS_PDATA_BASE;
	b_copyout.size = sizeof (rb);

	/*
	 * DataBuffer
	 */
	b_copyin.kaddr = buf;
	b_copyin.v86addr = (void *) (V86BIOS_PDATA_BASE + sizeof (rb));
	b_copyin.size = *size;
		
	bzero(&reg, sizeof(reg));
	reg.edi.edi = V86BIOS_PDATA_BASE;
	reg.eax.byte.ah = PCI_FUNCTION_ID;
	reg.eax.byte.al = PCI_GET_PCI_IRQ_ROUTING_OPTIONS;

	ret = v86bios_pci_bios_call_usebuf(&reg, &b_copyout, &b_copyin);

	*size = rb.sz;
	*bitmap = reg.ebx.word.bx;

	return ret;
	
}


/*
 * int
 * pci_read_exp_rom_signature(ms_cgnum_t cgnum,
 * 			   struct pci_rom_signature_data *buf)
 * 
 * Calling/Exit State:
 *	Search the expansion ROM area on a PCI CG for all ROM signatures
 *	and return the count.
 *
 * On failure, return -1, otherwise return the number of expansion ROM
 * signatures found.  Also, compute and store the address and offset of
 * each expansion ROM.
 *
 * Note: The PCI BIOS spec gives details on finding the expansion ROM
 * and getting the lengths and so on. 
 */
int
pci_read_exp_rom_signature(ms_cgnum_t cgnum,
			   struct pci_rom_signature_data *buf)
{
	char *e_rom_start = NULL; 
	struct pci_rom_header *p = NULL;
	struct pci_rom_data *q = NULL;
	int	i, count;

	/*
	 * To work around limitations of NUMA systems that cannot access
	 * the AT Hole of any CG besides the boot CG, we will ignore
	 * all Expansion ROMS not on the boot CG.  Ideally, we would
	 * translate all local Exp ROM physical addresses into the
	 * corresponding Global Coherent Memory physical addresses.
	 */
	if (cgnum != 0)
		return 0;
		
	e_rom_start = (char *)physmap((paddr_t)PCI_EXP_ROM_START_ADDR,
			PCI_EXP_ROM_SIZE,
			KM_NOSLEEP);
	if (e_rom_start == NULL){
		cmn_err(CE_WARN,"!Could not map PCI expansion ROM space.\n");
		return -1;
	}
	count = 0;
	for (i = 0; i < (PCI_EXP_ROM_SIZE / PCI_EXP_ROM_HDR_CHUNK); i++){
		q = NULL;
		p = (struct pci_rom_header *) ((char *) e_rom_start
					+ i * PCI_EXP_ROM_HDR_CHUNK);
		if (p->sig == PCI_EXP_ROM_HDR_SIG){

			if (!p->run_length) continue;

			q = (struct pci_rom_data *)( (char *) p + p->offset);

			if ((char *)q >= (e_rom_start + PCI_EXP_ROM_SIZE) ||
			    q->v_id == PCI_INVALID_VENDOR_ID) continue;
			if (bcmp(q->signature, PCI_EXP_ROM_DATA_SIG, 4) != 0)
				continue;

			if (q->base_class[2] == PCI_CLASS_TYPE_DISPLAY)
				continue;
			buf[count].vendor_id = q->v_id;
			buf[count].device_id = q->d_id;
			buf[count].addr = PCI_EXP_ROM_START_ADDR
					  + i*PCI_EXP_ROM_HDR_CHUNK;
			buf[count].length = (p->run_length * 512);
			buf[count].used = 0;
			count++;
		}
	}
	physmap_free(e_rom_start, PCI_EXP_ROM_SIZE, 0);
	return count;
}

#ifndef UNIPROC
/*
 * void
 * pci_emulate_bios_call() 
 * 
 * Calling/Exit State:
 *	Emulate BIOS calls on NUMA systems that can't support true
 *	BIOS calls on non-boot CGs.  We emulate the PCI_BIOS_PRESENT
 * 	call by assuming that any valid NUMA CG has a PCI bus hierarchy
 * 	that uses Configuration Mechanism #1, adheres to version 2.1
 *	of the PCI Specification, and which has all 256 possible PCI
 * 	buses.  We emulate the BIOS calls that read and write PCI
 *	Configuration Space by invoking PCI Configuration Mechanism #1
 *	against the I/O ports of the specified CG.
 */
int
pci_emulate_bios_call(ms_cgnum_t cgnum, regs *reg)
{
	unsigned int conf_addr = CA_MAKE_EXT_IO_ADDR(cgnum, 0xCF8);
	unsigned int conf_data = CA_MAKE_EXT_IO_ADDR(cgnum,
					0xCFC + (reg->edi.word.di & 3));
	unsigned int conf_value = 0x80000000 | (reg->ebx.byte.bh << 16)
			| (reg->ebx.byte.bl << 8) | reg->edi.word.di;

	if (reg->eax.byte.ah != PCI_FUNCTION_ID)
		return PCI_UNSUPPORTED_FUNCT;

	switch (reg->eax.byte.al) {
	case PCI_READ_CONFIG_BYTE:
		outl(conf_addr, conf_value);
		reg->ecx.byte.cl = inb(conf_data);
		break;
	case PCI_READ_CONFIG_WORD:
		outl(conf_addr, conf_value);
		reg->ecx.word.cx = inw(conf_data);
		break;
	case PCI_READ_CONFIG_DWORD:
		outl(conf_addr, conf_value);
		reg->ecx.ecx = inl(conf_data);
		break;
	case PCI_WRITE_CONFIG_BYTE:
		outl(conf_addr, conf_value);
		outb(conf_data, reg->ecx.byte.cl);
		break;
	case PCI_WRITE_CONFIG_WORD:
		outl(conf_addr, conf_value);
		outw(conf_data, reg->ecx.word.cx);
		break;
	case PCI_WRITE_CONFIG_DWORD:
		outl(conf_addr, conf_value);
		outl(conf_data, reg->ecx.ecx);
		break;
	case PCI_BIOS_PRESENT:
		reg->ecx.byte.cl = MAX_PCI_BUSES - 1;
		reg->ebx.word.bx = PCI_REV_2_1;
		reg->eax.byte.al = 0x11;
		reg->edx.edx = PCI_ID_REG_VAL;
		break;

	default:
		return PCI_UNSUPPORTED_FUNCT;
		/*NOTREACHED*/
		break;
	}

	reg->eax.byte.ah = PCI_SUCCESS;
	return PCI_SUCCESS;
}
#endif	/* #ifndef UNIXPROC */

/*
 * void
 * pci_bios_call_on_cg(ms_cgnum_t cgnum)
 * 
 * Calling/Exit State:
 *	Note that this function might be xcall()ed to an engine on
 *	the desired CG.
 */
void
pci_bios_call_on_cg(ms_cgnum_t *cgnump)
{
	struct pci_bus_data *bus_datap = &pci_cg_bus_data[*cgnump];
	regs *reg = bus_datap->bios_call_regs;

	if (bus_datap->bios_call_type & (V86BIOS_COPYOUT | V86BIOS_COPYIN)) {
		bus_datap->bios_call_rval =
			v86bios_pci_bios_call_usebuf(reg,
						     bus_datap->buf_out,
						     bus_datap->buf_in);
	}
	else
		bus_datap->bios_call_rval = v86bios_pci_bios_call(reg);
}

#ifndef UNIPROC
/*
 * void
 * pci_run_on_cg(ms_cgnum_t cgnum, void (*func)(), void *arg)
 *	Find any online engine belonging to the specified CG and invoke
 *	(*func)(arg) on it.
 *
 * Calling/Exit State:
 *	See xcall().
 *
 * Remarks:
 *	This function is not tested yet on generic ccNUMA machines (because
 *	such machines don't exist yet).
 *
 *	v86bios_pci_bios_call_usebuf() needs to be added.
 */
void
pci_run_on_cg(ms_cgnum_t cgnum, void (*func)(), void *arg)
{
	emask_t emask, resp_emask;
	ms_resource_t *p;
	ms_cpu_t cpuid;
	boolean_t is_online;
	pl_t old_pl;

	/*
	 * Lock down to a single engine for the duration of this call,
	 * since xcall() only works on *other* engines, and we thus
	 * have to avoid having os_this_cgnum change out from under us.
	 */
	old_pl = splhi();

	for (p = os_topology_p->mst_resources;
	     p < &os_topology_p->mst_resources[os_topology_p->mst_nresource];
	     p++) {
		if (p->msr_cgnum == cgnum && p->msr_type == MSR_CPU
		    && engine_state((cpuid = p->msri.msr_cpu.msr_cpuid),
				    ENGINE_ONLINE, &is_online) == 0
		    && is_online) {

			if (cgnum == os_this_cgnum) {
				(*func)(arg);
			} else {
				EMASK_CLRALL(&emask);
				EMASK_SET1(&emask, cpuid);
				xcall(&emask, &resp_emask, func, arg);
			}
			break;
		}
	}

	splx(old_pl);
}
#endif	/* #ifndef UNIPROC */

/*
 * int
 * pci_bios_call(ms_cgnum_t cgnum, regs *reg, unsigned char command)
 * 
 * Calling/Exit State:
 *	The caller must fill in all the registers in <reg> except for
 * 	AH and AL, which will get set automatically in this call (AH
 *	to PCI_FUNCTION_ID and AL to the <command> argument).
 */
int
pci_bios_call(ms_cgnum_t cgnum, regs *reg, unsigned char command)
{
	struct pci_bus_data *bus_datap = &pci_cg_bus_data[cgnum];

	reg->eax.byte.ah = PCI_FUNCTION_ID;
	reg->eax.byte.al = command;

	bus_datap->bios_call_regs = reg;

	/*
	 * Short-circuit the non-NUMA case, and emulate NUMA BIOS calls
	 * if necessary.  Otherwise, dispatch the BIOS call to run on
	 * the desired CG.
	 */
	if (os_cgnum_max == 0)
		pci_bios_call_on_cg(&cgnum);
#ifndef UNIPROC	
	else if (!are_cross_cg_bios_calls_supported)
		bus_datap->bios_call_rval
			= pci_emulate_bios_call(cgnum, reg);

	else
		pci_run_on_cg(cgnum, pci_bios_call_on_cg, &cgnum);
#endif
	
	return bus_datap->bios_call_rval;
}

/*
 * int
 * pci_bios_call_usebuf(ms_cgnum_t cgnum,
 *	regs *reg, unsigned char command,
 *	v86bios_buf_t *buf_out, v86bios_buf_t *buf_in)
 * 
 * Calling/Exit State:
 *	The caller must fill in all the registers in <reg> except for
 * 	AH and AL, which will get set automatically in this call (AH
 *	to PCI_FUNCTION_ID and AL to the <command> argument).
 *
 *	Data from the BIOS call is copied to buffer specified by
 *	<addr, size>.
 *
 * Remarks:
 *	This is not available NUMA machines because pci_emulate_bios_call()
 *	or pci_run_on_cg() don't support use of buffer.
 */
int
pci_bios_call_usebuf(ms_cgnum_t cgnum, regs *reg,
		     unsigned char command,
		     v86bios_buf_t *buf_out, v86bios_buf_t *buf_in)
{
	struct pci_bus_data *bus_datap = &pci_cg_bus_data[cgnum];

	reg->eax.byte.ah = PCI_FUNCTION_ID;
	reg->eax.byte.al = command;

	bus_datap->bios_call_regs = reg;
	if (buf_out != NULL)
		bus_datap->bios_call_type = V86BIOS_INT32 | V86BIOS_COPYOUT;

	if (buf_in != NULL)
		bus_datap->bios_call_type |= V86BIOS_COPYIN;
	
	bus_datap->buf_out = (void *) buf_out;
	bus_datap->buf_in = (void *) buf_in;
	/*
	 * Short-circuit the non-NUMA case, and emulate NUMA BIOS calls
	 * if necessary.  Otherwise, dispatch the BIOS call to run on
	 * the desired CG.
	 */
	if (os_cgnum_max == 0)
		pci_bios_call_on_cg(&cgnum);
#ifndef UNIPROC	
	else if (!are_cross_cg_bios_calls_supported)
		bus_datap->bios_call_rval
			= pci_emulate_bios_call(cgnum, reg);

	else
		pci_run_on_cg(cgnum, pci_bios_call_on_cg, &cgnum);
#endif
	
	return bus_datap->bios_call_rval;
}



