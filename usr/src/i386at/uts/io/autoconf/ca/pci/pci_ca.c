#ident	"@(#)kern-i386at:io/autoconf/ca/pci/pci_ca.c	1.15.13.1"
#ident	"$Header$"

#include <proc/regset.h>
#include <proc/seg.h>
#include <svc/errno.h>
#include <svc/psm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <svc/v86bios.h>

#include <io/ddi.h>
#include <io/ddi_i386at.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/autoconf/ca/ca.h>
#include <io/autoconf/ca/pci/pci.h>

void get_routing_info(ms_cgnum_t cgnum, struct pci_bus_data *bus_datap);
uchar_t translate_interrupt(struct pci_bus_data *bus_datap,
			    uchar_t bus, uchar_t dev, uchar_t ipin,
			    uchar_t iline);

extern void pci_alloc_bus_data(void);
extern pci_read_devconfig8(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
			   uchar_t *buf, ushort_t offset);
extern pci_read_devconfig16(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
			    ushort_t *buf, ushort_t offset);
extern pci_read_devconfig32(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
			    uint_t *buf, ushort_t offset);
extern pci_write_devconfig8(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
			    uchar_t *buf, ushort_t offset);
extern pci_write_devconfig16(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
			     ushort_t *buf, ushort_t offset);
extern pci_write_devconfig32(ms_cgnum_t cgnum, uchar_t bus, uchar_t devfun,
			     uint_t *buf, ushort_t offset);
extern register_pci_device(ms_cgnum_t cgnum, uchar_t bus, uchar_t dev,
			   uchar_t fun, uchar_t header);
extern pci_backpatch_ide(struct config_info *, struct config_info *,
			 struct config_info *, uchar_t);
extern pci_read_exp_rom_signature(ms_cgnum_t,
				  struct pci_rom_signature_data *);
extern char * pci_scan;
static find_exp_rom_data(struct pci_bus_data *bus_datap,
			 ushort_t vendor_id, ushort_t device_id,
			 uint_t *addr, uint_t *length);

#if defined(DEBUG) || defined(DEBUG_TOOLS)
STATIC int ca_pci_debug = 0;
#define DBG1(s)	if (ca_pci_debug == 1) printf s
#define DBG2(s)	if (ca_pci_debug == 2) printf s
#else
#define	DBG1(s)
#define DBG2(s)
#endif

/*
 * Autoconfig -- PCI CA access
 */

/*
 * The functions provided within read the devices on the various PCI
 * buses and create entries for them in the configuration manager-maintained
 * database (via cm_register_devconfig.)
 * This relies heavily on 'pci_bios.c'
 *
 * major routines:
 * ca_pci_init: Find the # of PCI buses, then all the PCI devices
 * on the bus.
 *
 *
 */

struct pci_bus_data *pci_cg_bus_data = NULL;

int
ca_pci_init(ms_resource_t *resourcep)
{
	regs	reg;
	size_t rom_sig_size;
	uchar_t	curbus, devnum, funcnum, header, maxfuncs;
	ushort_t widebus; /* >8bit container prevents bus number wrap */
	ushort_t vendor_id;
	struct rm_args rma;
	static struct pci_routebuffer rb;
  	static struct pci_routebuffer *rbp;
 	caddr_t v86bios_vbase = NULL;	/* base virtual address of
						V86 BIOS call */
 	caddr_t v86bios_vdata;	/* virtual address for data argument */
	ms_cgnum_t cgnum = resourcep->msr_cgnum;
	struct pci_bus_data *bus_datap;

	if (pci_cg_bus_data == NULL) 
		pci_alloc_bus_data();
	bus_datap = &pci_cg_bus_data[cgnum];
	
	/*
	 * Map the page of physical memory containing the Virtual x86
	 * BIOS base to a virtual address.
	 *
	 * BUG:  we skip this step on non-zero CGs, since limitations
	 *	 on early NUMA firmware prevent us from making BIOS
	 *	 calls on on-boot CGs.  The loss it not essential, since
	 *	 we can emulate the BIOS calls that access PCI
	 *	 Configuration Space.
	 */
	if (cgnum == 0 /*BUG*/) {
		v86bios_vbase = physmap(V86BIOS_PBASE, MMU_PAGESIZE,
					KM_NOSLEEP);
		if (v86bios_vbase == NULL) {
			/*
			 *+ Not enough virtual space. Check memory
			 *+ configured in your system.
			 */
			cmn_err(CE_WARN,
				"!ca_pci_init: Not enough virtual space"
				" for CG %d", cgnum);
			return -1;
		}
		v86bios_vdata = v86bios_vbase + V86BIOS_PDATA_BASE;
	}

	if (pci_verify(cgnum) != 0)
		return -1; /* already been called once */
	LKINFO_INIT(bus_datap->lockinfo, "AUTO-CONF:pci:pci_lock", 0);
	if (!(bus_datap->bios_call_lockp = LOCK_ALLOC(1, plhi,
				&bus_datap->lockinfo, KM_NOSLEEP)))
		return -1;

	if (find_pci_id(cgnum, bus_datap) != 0) {
		cmn_err(CE_WARN,"!PCI BIOS present but no bus devices "
			"on CG %d", cgnum);
		return -1;
	}
	DBG1(("Max PCI bus number on CG %x is %x\n", cgnum,
	      bus_datap->pci_maxbus));

	/*
	 * If the bus adheres to revision 2.1 of the PCI spec, then
	 * call the BIOS to retrieve its interrupt routing tables.
	 * This will come in handy later when determining the system
	 * slot number (an arbitrary label distinct from the slot's PCI
	 * device number) of each controller.
	 *
	 * BUG:  we only run this step on non-NUMA systems, since
	 *	 limitations on early NUMA firmware prevent us from
	 *	 making BIOS calls on non-boot CGs.  The loss is not
	 *	 essential, since all relevant interrupt routing
	 *	 information is	available from the resource topology
	 * 	 structure, and the slot number has little importance.
	 */
	rma.rm_key = RM_KEY;
	(void)strcpy(rma.rm_param, CM_PCI_VERSION);
	rma.rm_val = &(bus_datap->pci_bus_rev);
	rma.rm_vallen = sizeof(bus_datap->pci_bus_rev);
	(void) _rm_addval(&rma, UIO_SYSSPACE);

	if (bus_datap->pci_bus_rev == PCI_REV_2_1 &&
	    (os_cgnum_max == 0) &&	/*BUG*/
	    (strcmp(pci_scan, "2.0") != 0)) {
		rbp = (struct pci_routebuffer *) v86bios_vdata;
		rbp->sz = 0;
		rbp->addr = 0;
		rbp->selector = 0;

		bzero(&reg, sizeof(reg));
		reg.edi.edi = V86BIOS_PDATA_BASE;
		pci_bios_call(cgnum, &reg, PCI_GET_PCI_IRQ_ROUTING_OPTIONS);

		if (rbp->sz == 0) {
			cmn_err(CE_WARN, "IRQ table size == 0 on CG %d",
				cgnum);
			return -1;
		}

		if ((bus_datap->pci_irq_buffer
			= kmem_zalloc(rbp->sz, KM_NOSLEEP)) == NULL){
				cmn_err(CE_WARN, "!kmem_zalloc failed "
					"for IRQ buffer on CG %d", cgnum);
				return -1;
			}

		rbp->selector = 0; /* magic value for ES */
		rbp->addr = (void *) (V86BIOS_PDATA_BASE +
					sizeof(struct pci_routebuffer));
				/* lowest 1MB address space */

		bzero(&reg, sizeof(reg));
		reg.edi.edi = V86BIOS_PDATA_BASE; 
		pci_bios_call(cgnum, &reg, PCI_GET_PCI_IRQ_ROUTING_OPTIONS);
		if (reg.eax.byte.ah != 0){
			cmn_err(CE_WARN, "Cannot find IRQ routing info "
				"on CG %d", cgnum);
			kmem_free(bus_datap->pci_irq_buffer,
				  rbp->sz);
			bus_datap->pci_irq_buffer = 0;
		} 
		bcopy(rbp->addr,
		      bus_datap->pci_irq_buffer, rbp->sz);

		bus_datap->pci_irq_size = rbp->sz;
		bus_datap->pci_irq_bitmap = reg.ebx.word.bx;
	}

	/*
	 * Save information about the Expansion ROMs found on this CG. 
	 */
	rom_sig_size = sizeof(struct pci_rom_signature_data) * PCI_EXP_ROM_SIZE/PCI_EXP_ROM_HDR_CHUNK;
	bus_datap->rom_signature_buf = kmem_zalloc(rom_sig_size, KM_NOSLEEP);

	if (bus_datap->rom_signature_buf == NULL){
		cmn_err(CE_WARN, "!kmem_zalloc failed for ROM signature"
			" data on CG %d", cgnum);
		return -1;
	}
	bus_datap->num_rom_signatures = pci_read_exp_rom_signature(cgnum,
					bus_datap->rom_signature_buf);
	if (bus_datap->num_rom_signatures < 0) { /* error */
		cmn_err(CE_WARN, "!failure when reading exp ROM area "
			"on CG %d", cgnum);
		kmem_free(bus_datap->rom_signature_buf, rom_sig_size);
		return -1;
	}

	/*
	 * Get the real interrupt routing on this CG from the PSM's
	 * topology structure.
	 */
	get_routing_info(cgnum, bus_datap);

	/*
	 * Start off by assuming that each PCI bus in the hierarchy is
	 * a host bridge (and is thus its own parent).  When we walk
	 * the tree in the next step, we'll correct this assumption for
	 * any PCI-to-PCI bridges we discover.  We can't initialize
	 * each entry in that later loop, because we'd end up
	 * overwriting the bus number that gets stored when a PPB is
	 * discovered on its parent.
	 */
	for (widebus = 0; widebus <= bus_datap->pci_maxbus; widebus++)
		 bus_datap->bus_table[widebus].parent_bus = widebus;

	/*
	 * Now search the entire PCI bus hierarchy on this CG.  Skip
	 * any device whose Vendor ID register is invalid, as that
	 * value indicates the absence of a real PCI device there.
	 */
	for (widebus = 0; widebus <= bus_datap->pci_maxbus; widebus++) {
	   curbus = widebus;
	   for (devnum = 0;
		devnum < (((bus_datap->pci_conf_cycle_type & 0x03)
			  == 2) ? 16 : 32);
		devnum++) {
		pci_read_devconfig16(cgnum, curbus, (devnum<<3)|0,
				     &vendor_id, PCI_VENDOR_ID_OFFSET);
		if (vendor_id == PCI_INVALID_VENDOR_ID)
			continue;
		if (pci_read_devconfig8(cgnum, curbus, (devnum<<3)|0,
					&header, PCI_HEADER_OFFSET) != 0)
			continue;
		/*
		 * It's only safe to read the Config Space of a non-zero
		 * function if the Multi-function bit is set in the
		 * Header register of function 0.  However, non-zero
		 * functions need not be contiguously numbered (e.g.,
		 * the absence of Function 1 need not imply the absence
		 * of Function 2).
		 */
		if ((header & PCI_HEADER_MULTIFUNC_MASK) == 0)
			maxfuncs = 1;
		else
			maxfuncs = MAX_PCI_FUNCTIONS;

		for (funcnum = 0; funcnum < maxfuncs; funcnum++) {
			pci_read_devconfig8(cgnum, curbus,
					    (devnum<<3)|funcnum,
					    &header, PCI_HEADER_OFFSET);
			header &= ~PCI_HEADER_MULTIFUNC_MASK;
			/*
			 * Ignore anything that's not an ordinary PCI
			 * device or a PCI-to-PCI bridge.
			 */
			if (header == PCI_ORDINARY_DEVICE_HEADER
			    || header == PCI_TO_PCI_BRIDGE_HEADER)
				register_pci_device(cgnum, curbus, devnum,
						    funcnum, header);
		}
	  }
	}

	if (v86bios_vbase != NULL)
		physmap_free(v86bios_vbase, MMU_PAGESIZE, 0);

	kmem_free(bus_datap->rom_signature_buf, rom_sig_size);
	return 0;
}

/*
 * Register the PCI device or PCI-to-PCI Bridge at the specified
 * PCI hierarchy location.  The <header> parameter is the value that
 * was read from the device's Header Type register, but with the
 * Multifunction bit masked off; it allows us to distinguish between
 * PPBs and ordinary PCI devices.
 */
struct config_info *
get_pci_device_cip(ms_cgnum_t cgnum, uchar_t bus, uchar_t dev,
		uchar_t fun, uchar_t header, int *rvalp,
		struct config_info *cips)
{
	int n, i, nio, nmem, ret; 
	uint_t fff = 0xFFFFFFFF;
	ushort_t command, dis_command, vendor_id, device_id,
		 svendor_id, sdevice_id;
	uchar_t devfun, orig_iline, iline, ipin, sub_class, base_class,
		prog_intf, secondary_bus_num, subordinate_bus_num;
	uchar_t max_bars = (header == PCI_TO_PCI_BRIDGE_HEADER
		         ? MAX_PPB_BASE_REGISTERS : MAX_BASE_REGISTERS);
	struct pci_bus_num_info *infop;
	struct config_info *cip;
	struct config_info *cip2;
	struct config_info *primary;
	struct config_info *secondary;
	static struct config_irq_info iattrib;
	msr_routing_t *rip;
	regs reg;
	struct pci_bus_data *bus_datap = &pci_cg_bus_data[cgnum];
	struct pci_irq_routing_entry *irqbuf
		= bus_datap->pci_irq_buffer;
	static struct config_irq_info *irq_attrib = &iattrib;
	uint_t base_regs[MAX_BASE_REGISTERS], reg_lengths[MAX_BASE_REGISTERS];

	devfun = (dev << 3) | fun;
	pci_read_devconfig16(cgnum, bus, devfun, &vendor_id,
			     PCI_VENDOR_ID_OFFSET);
	if (vendor_id == PCI_INVALID_VENDOR_ID){
		*rvalp = 0;
		return NULL;
	}
	pci_read_devconfig16(cgnum, bus, devfun, &device_id,
			     PCI_DEVICE_ID_OFFSET);
	DBG1(("Registering device on CGNUM %x, bus %x, dev %x, func %x\n",
		cgnum, bus, dev, fun));
	DBG1(("\tVendor ID = %x, Device ID = %x\n", vendor_id, device_id));
	pci_read_devconfig8(cgnum, bus, devfun, &ipin,
			     PCI_INTERPT_PIN_OFFSET);
	pci_read_devconfig8(cgnum, bus, devfun, &base_class,
			     PCI_BASE_CLASS_OFFSET);
	pci_read_devconfig8(cgnum, bus, devfun, &sub_class,
			     PCI_SUB_CLASS_OFFSET);
	pci_read_devconfig8(cgnum, bus, devfun, &prog_intf,
			     PCI_PROG_INTF_OFFSET);

	/*
	 * Read the Interrupt Line register and then see if this value
	 * needs to be adjusted.  If so, write the correct value back
	 * out to the register.
	 */
	if (ipin != 0) {
		pci_read_devconfig8(cgnum, bus, devfun, &orig_iline,
				    PCI_INTERPT_LINE_OFFSET);
		iline = translate_interrupt(bus_datap, bus, dev, ipin,
					    orig_iline);
		DBG1(("\tInterrupt Line %x\n", iline));
		if (iline != orig_iline)
			pci_write_devconfig8(cgnum, bus, devfun, &iline,
					     PCI_INTERPT_LINE_OFFSET);
	}

	pci_read_devconfig16(cgnum, bus, devfun, &command,
			     PCI_COMMAND_OFFSET);

	/*
	 * If the device is not a bridge of some sort (PPB, host bridge,
	 * or ISA bridge), then it must have either I/O Space or Memory
	 * Space enabled in order for us to register it.
	 */
	if (base_class != PCI_CLASS_TYPE_BRIDGE
	    && (command & 0x3) == 0) {
		cmn_err(CE_NOTE,"!device 0x%x%x NOT ENABLED on CG %d!",
			vendor_id, device_id, cgnum);
			*rvalp = -1;
			return NULL;
	}

	if(!cips)
	{
		cip = NULL;
		cip2 = NULL;
		primary = NULL;
		secondary = NULL;
	} else {
		cip = &cips[0];
		cip2 = &cips[1];
		primary = &cips[2];
		secondary = &cips[3];
	}
	/*
	 * Allocate a config_info structure for the device and fill in.
	 */
	if(!cips)
	{
		CONFIG_INFO_KMEM_ZALLOC(cip);
	}
	cip->ci_cgnum = cgnum;
	cip->ci_busid = CM_BUS_PCI;
	cip->ci_pci_cgnum = cgnum;
	cip->ci_pci_busnumber = bus;
	cip->ci_pci_devfuncnumber = devfun;
	cip->ci_pcivendorid = vendor_id;
	cip->ci_pcidevid = device_id;
	cip->ci_pcisclassid = base_class;
	cip->ci_pcissclassid = sub_class;
	cip->ci_pcislot = -1;

	/*
	 * PPBs don't have Subsystem Vendor/Device ID registers, so
	 * only read them for ordinary PCI devices.
	 */
	if (header == PCI_ORDINARY_DEVICE_HEADER) {
		pci_read_devconfig16(cgnum, bus, devfun, &svendor_id,
				     PCI_SVEN_ID_OFFSET);
		pci_read_devconfig16(cgnum, bus, devfun, &sdevice_id,
				     PCI_SDEV_ID_OFFSET);
		if (svendor_id != 0 &&
		    svendor_id != PCI_INVALID_VENDOR_ID) {
			cip->ci_pcisvenid = svendor_id;
			cip->ci_pcisdevid = sdevice_id;
		}
	}

	/*
	 * Conversely, there are some registers that only PPBs have.
 	 * Update the bus number table to include the coordinates of
	 * the PPB as determined by reading its Secondary Bus Number
	 * and Subordinate Bus Number registers.  Note that we don't
	 * need to read the Primary Bus Number register, since we
	 * already know that the PPB's immeidate parent is <bus>.
	 * Some of this information should eventually end up in the
	 * config_info structure, once it has been enhanced to include
	 * PPB fields.
	 */
	if (header == PCI_TO_PCI_BRIDGE_HEADER) {
		pci_read_devconfig8(cgnum, bus, (dev << 3) | fun,
				    &secondary_bus_num,
				    PCI_PPB_SECONDARY_BUS_NUM_OFFSET);
		pci_read_devconfig8(cgnum, bus, (dev << 3) | fun,
				    &subordinate_bus_num,
				    PCI_PPB_SUBORDINATE_BUS_NUM_OFFSET);
		DBG1(("Registering PPB on CGNUM %x, bus %x, dev %x, "
		      "func %x as bus %x\n", cgnum, bus, dev, fun,
		      secondary_bus_num));
		infop = &bus_datap->bus_table[secondary_bus_num];
		infop->parent_bus = bus;
		infop->subordinate_bus = subordinate_bus_num ;
		infop->dev_on_parent = dev;
		infop->func_on_parent = fun;
	}

	/*
	 * If the interrupt routing table contains information about
	 * this device's board slot number, use it instead of the -1
	 * value we saved earlier.
	 */
	if (irqbuf != 0)
		for (i = 0;
		     i < (bus_datap->pci_irq_size /
			  sizeof(struct pci_irq_routing_entry));
		     i++) {
			if ((irqbuf[i].device_number >> 3) == dev &&
			     irqbuf[i].bus_number == bus) {
				cip->ci_pcislot = irqbuf[i].slot;
				break;
			}
		}

	for (i = 0; i < max_bars; i++)
		pci_read_devconfig32(cgnum, bus, devfun,
			   &base_regs[i],
			   PCI_BASE_REGISTER_OFFSET + (i * 4));
	dis_command = command & PCI_COMMAND_DISABLE;

	/*
	 * If a register is not 0, read the length it specifies.
	 * Disable the device (via writing 0's to low-order 2 bits).
	 * Get the length associated with the register.
	 * If I/O, store where it is mapped to, along with its length
	 * If it's memory, check the extent of the range first.
	 * Right now, we don't handle memory addresses above the 4GB
	 * boundary, so punt (break from loop and put out a warning
	 * message) for any 64-bit memory range that lies above that
	 * boundary; but treat sub-4GB 64-bit ranges as if they were
	 * 32-bit memory ranges.  Handle valid memory ranges similar to
	 * I/O ranges, except that we must disable memory access
	 * instead of I/O access when manipulating the BAR.
	 */
	bzero(reg_lengths, sizeof(reg_lengths));
	pci_write_devconfig16(cgnum, bus, devfun, &dis_command,
			PCI_COMMAND_OFFSET);
	nio = nmem = 0;
	for (i = 0; i < max_bars; i++){
		if (base_regs[i] != 0){
		   pci_write_devconfig32(cgnum, bus, devfun,
			   &fff,
			   PCI_BASE_REGISTER_OFFSET + (i * 4));
		   pci_read_devconfig32(cgnum, bus, devfun, 
			   &reg_lengths[i],
			   PCI_BASE_REGISTER_OFFSET + (i * 4));
		   pci_write_devconfig32(cgnum, bus, devfun, 
			   &base_regs[i],
			   PCI_BASE_REGISTER_OFFSET + (i * 4));
		}
	}
	pci_write_devconfig16(cgnum, bus, devfun, &command,
			      PCI_COMMAND_OFFSET);
	if (find_exp_rom_data(bus_datap, vendor_id, device_id,
			     (uint_t *)&cip->ci_membase[nmem],
			     (uint_t *)&cip->ci_memlength[nmem])
		== 0) {
		DBG1(("\tExpROM at %x, length %x\n",
		      cip->ci_membase[nmem], cip->ci_memlength[nmem]));
		nmem++;
	}
	for (i = 0; i < max_bars; i++){
		if (base_regs[i] == 0)
			continue;
		if (base_regs[i] & 0x01){ /* I/O base addr */
			base_regs[i] &= 0xFFFFFFFC; /* 0 low 2 bits*/
			cip->ci_ioport_base[nio] =
				CA_MAKE_EXT_IO_ADDR(cgnum, base_regs[i]);
			reg_lengths[i] ^= 0xFFFFFFFF;
			reg_lengths[i] |= 1;
			reg_lengths[i]++;
			reg_lengths[i] &= 0xFFFFFFFF;
			cip->ci_ioport_length[nio] = reg_lengths[i];
			DBG1(("\tI/O BAR at %x, length %x\n",
			      cip->ci_ioport_base[nio],
			      cip->ci_ioport_length[nio]));
			nio++;
		}
		else switch(base_regs[i] & 0x07){/* memory base addr */
		   case 0x04: /*64 bit address space */
			if (i == max_bars - 1
			    || base_regs[i+1] != 0)
				cmn_err(CE_NOTE, "!PCI board 0x%x,0x%x "
					"on CG %d: no support for "
					"memory space above 4GB",
					vendor_id, device_id, cgnum);
			else
				/*
				 * Treat this 64-bit BAR as a 32-bit
				 * BAR by copying its 32 lower bits to
				 * the 32 upper bits and masking off
				 * the type bits.  We'll process this
				 * fake new BAR the next time through
				 * the loop.
				 */
				base_regs[i+1] = base_regs[i] & (~0x7);
			break;
		   case 0x06: /*reserved*/
			cmn_err(CE_NOTE, "!PCI board 0x%x,0x%x on CG "
				"%d: Unexpected memory map request",
				vendor_id, device_id, cgnum);
			break;
		   default: /*presumed already mapped */
			base_regs[i] &= 0xFFFFFFF0; /* 0 low 4 bits */
			cip->ci_membase[nmem] = base_regs[i];
			reg_lengths[i] ^= 0xFFFFFFFF;
			reg_lengths[i] |= 0xF;
			reg_lengths[i]++;
			cip->ci_memlength[nmem] = reg_lengths[i];
			DBG1(("\tMem BAR at %x, length %x\n",
			      cip->ci_membase[nmem],
			      cip->ci_memlength[nmem]));
			nmem++;
			}
		}
	iattrib.cirqi_trigger = (uchar_t) 1; /* level*/
	iattrib.cirqi_level = (uchar_t) 0; /* active low */
	iattrib.cirqi_type = (uchar_t) 1; /* sharable */
	iattrib.cirqi_rsvrd = (uchar_t) 0; /* fill it out */
	if (base_class == PCI_CLASS_TYPE_MASS_STORAGE &&
		sub_class == PCI_SUB_CLASS_IDE){
		cip->ci_pcivendorid = PCI_INVALID_VENDOR_ID;
		cip->ci_pcidevid = 1;
	}

	bcopy((uchar_t *) &iattrib, &cip->ci_irqattrib[0], 1);
	if (ipin != 0){
		cip->ci_numirqs = 1;
		cip->ci_irqline[0] = iline;
	}

	if (!(base_class == PCI_CLASS_TYPE_MASS_STORAGE &&
		sub_class == PCI_SUB_CLASS_IDE)){
		cip->ci_numioports = (uchar_t) nio;
		cip->ci_nummemwindows = (uchar_t) nmem;
	}
	else {
		/* delete what's been added after making copy
		* then add in 2 entries for ide, if its valid*/
		if (!primary && (primary = (struct config_info *) kmem_zalloc(
				CONFIG_INFO_SIZE, KM_NOSLEEP))
		    == NULL) {
			cmn_err(CE_WARN, "pci_ca Not enough memory\n");
			*rvalp = ENOMEM;
			return NULL;
		}
		if (!secondary && (secondary = (struct config_info *) kmem_zalloc(
				CONFIG_INFO_SIZE, KM_NOSLEEP))
		    == NULL) {
			cmn_err(CE_WARN, "pci_ca Not enough memory\n");
			*rvalp = ENOMEM;
			return NULL;
		}
		ret = pci_backpatch_ide(cip, primary,
					secondary, prog_intf);
cmn_err(CE_NOTE, "!ret = %d\n", ret);
		switch (ret) {
			case 1 : /* primary only */
				if(!cips)
					kmem_free(secondary, CONFIG_INFO_SIZE);
				*cip = *primary;
				if(!cips)
					kmem_free(primary, CONFIG_INFO_SIZE);
				*rvalp = 0;
				return cip;
			case 2: /* secondary only */
				if(!cips)
					kmem_free(primary, CONFIG_INFO_SIZE);
				*cip = *secondary;
				if(!cips)
					kmem_free(secondary, CONFIG_INFO_SIZE);
				*rvalp = 0;
				return cip;
			case 3 : /* primary and secondary exist */
				*cip = *primary;
				if(!cips)
				{
					CONFIG_INFO_KMEM_ZALLOC(cip2);
				}
				*cip2 = *secondary;
				if(!cips)
				{
					kmem_free(primary, CONFIG_INFO_SIZE);
					kmem_free(secondary, CONFIG_INFO_SIZE);
				}
				*rvalp = 0;
				return cip;
			default:
				if(!cips)
				{
					kmem_free(primary, CONFIG_INFO_SIZE);
					kmem_free(secondary, CONFIG_INFO_SIZE);
				}
#if 0
				CONFIG_INFO_KMEM_FREE(cip);
#else /* ul95-29302 */
				cip->ci_pcidevid = 0xffff;
#endif
				*rvalp = -1;
				return NULL;
		}
	}
	*rvalp = 0;
	return cip;
}
STATIC int
register_pci_device(ms_cgnum_t cgnum, uchar_t bus, uchar_t dev,
		    uchar_t fun, uchar_t header)
{
        int rval;
        (void) get_pci_device_cip(cgnum, bus, dev, fun, header, &rval, NULL);
        return rval;
}
STATIC int
find_exp_rom_data(struct pci_bus_data *bus_datap, ushort_t vid,
		  ushort_t did, uint_t *base, uint_t *len)
{
	int i;

	for (i = 0; i < bus_datap->num_rom_signatures; i++){
		if (bus_datap->rom_signature_buf[i].vendor_id == vid &&
		    bus_datap->rom_signature_buf[i].device_id == did &&
		    bus_datap->rom_signature_buf[i].used == 0){
			*base = bus_datap->rom_signature_buf[i].addr;
			*len = bus_datap->rom_signature_buf[i].length;
			bus_datap->rom_signature_buf[i].used = 1;
			return 0;
		}
	}
	return -1; /* did not match or none free */
}

/*
 * This function's only callers are in io/odi/lsl/lslrealmode.c, and
 * we retain it only for backward compatibility with those callers.
 * Only CG 0 (i.e., non-NUMA) bus data is returned.
 */
void *
get_pci_bus_data(void)
{
	return (void *) &(pci_cg_bus_data[0]);
}

int
pci_backpatch_ide(struct config_info *in, struct config_info *pri,
		  struct config_info *sec, uchar_t prog_intf){

		int num_pri, num_sec;
		extern ide_detect(ushort_t);
		static struct config_irq_info compat_ide_iattrib = {
			0, /* EDGE*/
			1, /* active HIGH */
			1, /* sharable */
			0, /* fill it out */
		};

		num_sec = num_pri = 0;
cmn_err(CE_NOTE, "!in pci_ide_backpatch\n");
		*pri = *sec = *in; /* duplicate the stuff that matters */
		pri->ci_numioports = sec->ci_numioports = 0;
		pri->ci_nummemwindows = sec->ci_nummemwindows = 0;

		prog_intf &= 0x0f; /* only care about low 4 bits */
cmn_err(CE_NOTE, "!prog int = 0x%x\n", prog_intf);


		if (!(prog_intf & 0x01) || (in->ci_ioport_base[0] == 0x1f0)){
cmn_err(CE_NOTE, "!ide_detect 1f0 != 0\n");
			if (ide_detect(0x1f0) != 0){

				pri->ci_ioport_base[0] = 0x1f0;
				pri->ci_ioport_length[0] = 8;
				pri->ci_ioport_base[1] = 0x3f0;
				pri->ci_ioport_length[1] = 4;
				pri->ci_numirqs = 1;
				pri->ci_irqline[0] = 14;
				pri->ci_numioports = 2;
				bcopy((uchar_t *) &compat_ide_iattrib,
						&pri->ci_irqattrib[0], 1);
				num_pri = 1;
			}
			else
				num_pri = 0;
		}
		else {
			if (ide_detect(in->ci_ioport_base[0]) != 0){
cmn_err(CE_NOTE, "!ide_detect 0x%x != 0\n", in->ci_ioport_base[0]);
				pri->ci_ioport_base[0] =
						in->ci_ioport_base[0];
				pri->ci_ioport_length[0] =
						in->ci_ioport_length[0];
				pri->ci_ioport_base[1] =
					in->ci_ioport_base[1] - 4;
				pri->ci_ioport_length[1] =
					in->ci_ioport_length[1];
				pri->ci_numirqs = 1;
				pri->ci_irqline[0] =
					in->ci_irqline[0];
				pri->ci_numioports = 2;
				num_pri = 1;
			}
			else
				num_pri = 0;
		}
		if (!(prog_intf & 0x04) || (in->ci_ioport_base[2] == 0x170)){
			if (ide_detect(0x170) != 0){
cmn_err(CE_NOTE, "!ide_detect 170 != 0\n");
				sec->ci_ioport_base[0] = 0x170;
				sec->ci_ioport_length[0] = 8;
				sec->ci_ioport_base[1] = 0x370;
				sec->ci_ioport_length[1] = 4;
				sec->ci_numirqs = 1;
				sec->ci_irqline[0] = 15;
				sec->ci_numioports = 2;
				sec->ci_pci_devfuncnumber |= 7; /* turn on low*/
				bcopy((uchar_t *) &compat_ide_iattrib,
						&sec->ci_irqattrib[0], 1);
				num_sec = 1;
			}
			else
				num_sec = 0;
		}
		else {
			if (ide_detect(in->ci_ioport_base[2]) != 0){
cmn_err(CE_NOTE, "!ide_detect 0x%x != 0\n", in->ci_ioport_base[2]);
				sec->ci_ioport_base[0] =
					in->ci_ioport_base[2];
				sec->ci_ioport_length[0] =
					in->ci_ioport_length[2];
				sec->ci_ioport_base[1] =
					in->ci_ioport_base[3] - 4;
				sec->ci_ioport_length[1] =
					in->ci_ioport_length[3];
				sec->ci_numirqs = 1;
				sec->ci_irqline[0] =
					in->ci_irqline[0];
				sec->ci_numioports = 2;
				sec->ci_pci_devfuncnumber |= 7; /* turn on low*/
				num_sec = 1;
			}
			else
				num_sec = 0;
		}
		return num_pri + 2 * num_sec; 
}

#include <io/hba/ide/ata/ata.h>
STATIC
ide_detect(ushort_t base)
{
	short ret;
cmn_err(CE_NOTE, "!in ide_detect base = %x\n", base);
	if (base == 0) return 0;
	/*
	 * If anything is configured on the bus, there must always be a master.
	 */
	outb(base + AT_DRVHD, ATDH_DRIVE0);

	/*
	 * If nothing is hanging from here we expect the inb to be 0xFF.
	 */
	drv_usecwait(1000);
	ret = inb(base + AT_DRVHD);
cmn_err(CE_NOTE, "!ret = %x\n", ret);
	if (ret == ATDH_DRIVE0)
		return 1;
	else return 0;
}

/*
 * Search the Resource Topology structure to find any PCI interrupt
 * routing information that might be available for the specified CG
 * PCI bus data.
 */
STATIC void
get_routing_info(ms_cgnum_t cgnum, struct pci_bus_data *bus_datap)
{
	ms_resource_t *p;

	bus_datap->nre = 0;
	bus_datap->rip = (msr_routing_t *) NULL;

	for (p = os_topology_p->mst_resources;
	     p < &os_topology_p->mst_resources[os_topology_p->mst_nresource];
	     p++) {
		if (p->msr_cgnum != cgnum)
			continue;
		if (p->msr_type != MSR_BUS)
			continue;
		if (p->msri.msr_bus.msr_bus_type != MSR_BUS_PCI)
			continue;
		if (p->msri.msr_bus.msr_n_routing == 0)
			break;
		if (p->msri.msr_bus.msr_intr_routing == NULL)
			break;
		bus_datap->nre = p->msri.msr_bus.msr_n_routing;
		bus_datap->rip = p->msri.msr_bus.msr_intr_routing;
		break;
	}
}

/*
 * Determine what interrupt line a given PCI device will use.  The
 * value of the Interrupt Line register is not always the answer, since
 * the BIOS assigns this assuming that the PIC(s) will operate in
 * standard 8259 mode, and the interrupt routing will be different if
 * they operate in full APIC mode.  We also need to apply the standard
 * routing for interrupts of devices behind PCI-to-PCI bridges (as
 * defined in the "PCI-to-PCI Bridge Specification 1.0") recursively
 * until we reach either a level for which the interrupt routing table
 * has a matching entry, or a host bridge, in which case we use the
 * original value of the Interrupt Line register.
 *
 */
STATIC uchar_t
translate_interrupt(struct pci_bus_data *bus_datap, uchar_t bus,
		    uchar_t dev, uchar_t ipin, uchar_t iline)
{
	int n;
	msr_routing_t *rip;
	uint_t isource, islot;

	for (;;) {
		/*
		 * Search the PSM-provided interrupt routing table 
		 * for this PCI bus hierarchy, looking for an entry
		 * that matches our pin on our device on our bus.
		 * If found, return the line used by that pin.
		 */
		for (n = 0, rip = bus_datap->rip;
		     n < bus_datap->nre;
		     n++, rip++) {
			isource = rip->msr_isource;
			islot = rip->msr_islot;

			if (MSR_ISOURCE_PCI_BUS(isource) == bus
			    && MSR_ISOURCE_PCI_DEV(isource) == dev
			    && MSR_ISOURCE_PCI_PIN(isource) == ipin - 1)
				return islot;
		}

		/*
	 	 * If we made it all the way up to a host bridge without
		 * finding a routing entry, then return the original
		 * Interrupt Line register value.
		 */
		if (bus_datap->bus_table[bus].parent_bus == bus)
			return iline;

		/*
		 * Otherwise, use the standard PCI-to-PCI bridge
		 * routine rules to convert the interrupt to a pin
		 * on its parent bus, and then loop again to search
		 * for *that* interrupt.
		 */
		ipin = 1 + ((ipin - 1 + dev) % 4);
		dev = bus_datap->bus_table[bus].dev_on_parent;
		bus =  bus_datap->bus_table[bus].parent_bus;
	}
}
