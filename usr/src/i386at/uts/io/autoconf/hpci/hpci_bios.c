#ident	"@(#)kern-i386at:io/autoconf/hpci/hpci_bios.c	1.1.1.2"
#ident	"$Header$"

#include <proc/regset.h>
#include <proc/cguser.h>
#include <svc/errno.h>
#include <svc/psm.h>
#include <svc/systm.h>
#include <svc/v86bios.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/emask.h>
#include <util/engine.h>
#include <mem/kmem.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/autoconf/hpci/hpci_bios.h>
#include <io/autoconf/ca/pci/pci.h>

extern int pci_read_devconfig8(cgnum_t cgnum, uchar_t bus,
			       uchar_t devfun, uchar_t *buf,
			       ushort_t offset);
extern int pci_read_devconfig16(cgnum_t cgnum, uchar_t bus,
				uchar_t devfun, ushort_t *buf,
				ushort_t offset);
extern int pci_read_devconfig32(cgnum_t cgnum, uchar_t bus,
				uchar_t devfun, uint_t *buf,
				ushort_t offset);
extern int pci_write_devconfig8(cgnum_t cgnum, uchar_t bus,
				uchar_t devfun, uchar_t *buf,
				ushort_t offset);
extern int pci_write_devconfig16(cgnum_t cgnum, uchar_t bus,
				 uchar_t devfun, ushort_t *buf,
				 ushort_t offset);
extern int pci_write_devconfig32(cgnum_t cgnum, uchar_t bus,
				 uchar_t devfun, uint_t *buf,
				 ushort_t offset);
extern int pci_bios_call(cgnum_t cgnum, regs *reg,
			 unsigned char command);

extern int pci__get_irq_routing_options( uchar_t *buf, size_t *sz, ushort_t *bitmap);

/*
 * int
 * pci_bios_present(cgnum_t cgnum, uchar_t *maxbus, ushort_t *bus_rev, uchar_t *conf_cycle_type)
 * 
 * Calling/Exit State:
 *	Invoke the "PCI BIOS PRESENT" BIOS call on the specified CG.
 *	Report the available data back to the caller.  A quick way to
 *	determine whether you're on a PCI system
 *
 * Arguments:
 *	cgnum:				the CG to test
 *	maxbus				pointer to uchar_t, max pci bus number
 *	bus_rev				pointer to ushort_t, PCI 2.0 or higher
 *	conf_cycle_type			pointer to uchar_t, type 1 or 2
 *
 * Return values: 0 if successful, -1 if BIOS call failed somehow
 */
int
pci_bios_present(rm_key_t hpci, uchar_t *maxbus, ushort_t *bus_rev, uchar_t *conf_cycle_type)
{
	regs reg;
	int ret;
	cm_args_t cma;
	cgid_t	mycgid;


	ASSERT(KS_HOLD0LOCKS());
	ASSERT(maxbus != NULL && bus_rev != NULL && conf_cycle_type != NULL);
	cma.cm_key = hpci;
	cma.cm_param = CM_CGID;
	cma.cm_val = &mycgid;
	cma.cm_vallen = sizeof(mycgid);
	cma.cm_n = 0;
	cm_begin_trans(hpci, RM_READ);
	ret = cm_getval(&cma);
	cm_end_trans(hpci);
	bzero(&reg, sizeof(reg));
	pci_bios_call((ret == ENOENT)? 0 : cgid2cgnum(mycgid), &reg, PCI_BIOS_PRESENT);

	if (reg.edx.edx != PCI_ID_REG_VAL)
		return PCI_FAILURE;

	*maxbus = reg.ecx.byte.cl;
	*bus_rev = reg.ebx.word.bx;
	*conf_cycle_type = reg.eax.byte.al;
	return 0;
}
/*
 * int
 * pci_generate_special_cycle(cgnum_t cgnum, uchar_t bus, uint_t data)
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
pci_generate_special_cycle(rm_key_t hpci, uchar_t bus, uint_t data)
{
        regs reg;
	cm_args_t cma;
	cgid_t	mycgid;
	int ret;

	ASSERT(KS_HOLD0LOCKS());
	cma.cm_key = hpci;
	cma.cm_param = CM_CGID;
	cma.cm_val = &mycgid;
	cma.cm_vallen = sizeof(mycgid);
	cma.cm_n = 0;
	cm_begin_trans(hpci, RM_READ);
	ret = cm_getval(&cma);
	cm_end_trans(hpci);
        bzero(&reg, sizeof(reg));
        reg.ebx.byte.bh = bus;
        reg.edx.edx = data;
        pci_bios_call((ret == ENOENT)? 0 : cgid2cgnum(mycgid), &reg, PCI_GENERATE_SPECIAL_CYCLE);
        return (int) reg.eax.byte.ah;
}

/*
 * int
 * pci_<read|write>_byte|word|dword(cgnum_t cgnum, uchar_t bus,
 *					uchar_t devfun, ushort_t offset,
 *					value)
 * 
 * Calling/Exit State:
 *	read or write the # of bits at the given PCI device/function, bus
 * 	config space location.
 *
 *
 * Arguments:
 *	cgnum_t cgnum: CG containing desired PCI bus hierarchy
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
pci_read_byte(rm_key_t hpci, uchar_t bus, uchar_t devfun, ushort_t offset, uchar_t *ret)
{
	cm_args_t cma;
	cgid_t	mycgid;
	int	retfromcm;

	cma.cm_key = hpci;
	cma.cm_param = CM_CGID;
	cma.cm_val = &mycgid;
	cma.cm_vallen = sizeof(mycgid);
	cma.cm_n = 0;
	cm_begin_trans(hpci, RM_READ);
	retfromcm = cm_getval(&cma);
	cm_end_trans(hpci);

	return pci_read_devconfig8((retfromcm == ENOENT)? 0 : cgid2cgnum(mycgid), bus, devfun, ret, offset);
}

int
pci_read_word(rm_key_t hpci, uchar_t bus, uchar_t devfun, ushort_t offset, ushort_t *ret)
{
	cm_args_t cma;
	cgid_t	mycgid;
	int	retfromcm;

	cma.cm_key = hpci;
	cma.cm_param = CM_CGID;
	cma.cm_val = &mycgid;
	cma.cm_vallen = sizeof(mycgid);
	cma.cm_n = 0;
	cm_begin_trans(hpci, RM_READ);
	retfromcm = cm_getval(&cma);
	cm_end_trans(hpci);
	return pci_read_devconfig16((retfromcm == ENOENT)? 0 : cgid2cgnum(mycgid), bus, devfun, ret, offset);
}	

int
pci_read_dword(rm_key_t hpci, uchar_t bus, uchar_t devfun, ushort_t offset, uint_t *ret)
{
	cm_args_t cma;
	cgid_t	mycgid;
	int	retfromcm;

	cma.cm_key = hpci;
	cma.cm_param = CM_CGID;
	cma.cm_val = &mycgid;
	cma.cm_vallen = sizeof(mycgid);
	cma.cm_n = 0;
	cm_begin_trans(hpci, RM_READ);
	retfromcm = cm_getval(&cma);
	cm_end_trans(hpci);
	return pci_read_devconfig32((retfromcm == ENOENT)? 0 : cgid2cgnum(mycgid), bus, devfun, ret, offset);
}

/* write routines */

int
pci_write_byte(rm_key_t hpci, uchar_t bus, uchar_t devfun, ushort_t offset, uchar_t buf)
{
	cm_args_t cma;
	cgid_t	mycgid;
	int	retfromcm;

	cma.cm_key = hpci;
	cma.cm_param = CM_CGID;
	cma.cm_val = &mycgid;
	cma.cm_vallen = sizeof(mycgid);
	cma.cm_n = 0;
	cm_begin_trans(hpci, RM_READ);
	retfromcm = cm_getval(&cma);
	cm_end_trans(hpci);
	return pci_write_devconfig8((retfromcm == ENOENT)? 0 : cgid2cgnum(mycgid), bus, devfun, &buf, offset);
}

int
pci_write_word(rm_key_t hpci, uchar_t bus, uchar_t devfun, ushort_t offset, ushort_t buf)
{
	cm_args_t cma;
	cgid_t	mycgid;
	int	retfromcm;

	cma.cm_key = hpci;
	cma.cm_param = CM_CGID;
	cma.cm_val = &mycgid;
	cma.cm_vallen = sizeof(mycgid);
	cma.cm_n = 0;
	cm_begin_trans(hpci, RM_READ);
	retfromcm = cm_getval(&cma);
	cm_end_trans(hpci);

	return pci_write_devconfig16((retfromcm == ENOENT)? 0 : cgid2cgnum(mycgid), bus, devfun, &buf, offset);
}

int
pci_write_dword(rm_key_t hpci, uchar_t bus, uchar_t devfun, ushort_t offset, uint_t buf)
{
	cm_args_t cma;
	cgid_t	mycgid;
	int	ret;

	cma.cm_key = hpci;
	cma.cm_param = CM_CGID;
	cma.cm_val = &mycgid;
	cma.cm_vallen = sizeof(mycgid);
	cma.cm_n = 0;
	cm_begin_trans(hpci, RM_READ);
	ret = cm_getval(&cma);
	cm_end_trans(hpci);
	return pci_write_devconfig32((ret == ENOENT)? 0 : cgid2cgnum(mycgid), bus, devfun, &buf, offset);
}
int
pci_set_hw_interrupt(rm_key_t hpci, uchar_t bus, uchar_t devfun, uchar_t ipin,
uchar_t iline)
{
        regs reg;
	cm_args_t cma;
	cgid_t	mycgid;
	int	ret;

	cma.cm_key = hpci;
	cma.cm_param = CM_CGID;
	cma.cm_val = &mycgid;
	cma.cm_vallen = sizeof(mycgid);
	cma.cm_n = 0;
	cm_begin_trans(hpci, RM_READ);
	ret = cm_getval(&cma);
	cm_end_trans(hpci);

	ASSERT(KS_HOLD0LOCKS());
	bzero(&reg, sizeof(reg));
	reg.ebx.byte.bh = bus;
	reg.ebx.byte.bl = devfun;
	reg.ecx.byte.cl = ipin;
	reg.ecx.byte.ch = iline;

	pci_bios_call((ret == ENOENT)? 0 : cgid2cgnum(mycgid), &reg, PCI_SET_PCI_HARDWARE_INTERRUPT);
	return (int) reg.eax.byte.ah;
}
int
pci_find_pci_device(rm_key_t hpci, uchar_t *bus, uchar_t *devfun, 
			ushort_t vendor_id, ushort_t device_id, ushort_t index)
{
	regs	reg;
	cm_args_t cma;
	cgid_t	mycgid;
	int	ret;

	cma.cm_key = hpci;
	cma.cm_param = CM_CGID;
	cma.cm_val = &mycgid;
	cma.cm_vallen = sizeof(mycgid);
	cma.cm_n = 0;
	cm_begin_trans(hpci, RM_READ);
	ret = cm_getval(&cma);
	cm_end_trans(hpci);

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(bus != NULL && devfun != NULL);
	bzero(&reg, sizeof(reg));
	reg.ecx.word.cx = device_id;
	reg.edx.word.dx = vendor_id;
	reg.esi.word.si = (short) index;
	pci_bios_call((ret == ENOENT)? 0 : cgid2cgnum(mycgid), &reg, PCI_FIND_PCI_DEVICE);
	if (reg.eax.byte.ah != 0)
		return reg.eax.byte.ah;
	*bus = reg.ebx.byte.bh;
	*devfun = reg.ebx.byte.bl;
	return 0;
}
int
pci_find_pci_class_code(rm_key_t hpci, uchar_t *bus, uchar_t *devfun, 
			uint_t class_code, ushort_t index)
{
	regs reg;
	cm_args_t cma;
	cgid_t	mycgid;
	int	retfromcm;


	cma.cm_key = hpci;
	cma.cm_param = CM_CGID;
	cma.cm_val = &mycgid;
	cma.cm_vallen = sizeof(mycgid);
	cma.cm_n = 0;
	cm_begin_trans(hpci, RM_READ);
	retfromcm = cm_getval(&cma);
	cm_end_trans(hpci);

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(bus != NULL && devfun != NULL);
	bzero(&reg, sizeof(reg));
	reg.ecx.ecx = class_code;
	pci_bios_call((retfromcm == ENOENT)? 0 : cgid2cgnum(mycgid), &reg, PCI_FIND_CLASS_CODE);
	if (reg.eax.byte.ah != 0)
		return reg.eax.byte.ah;
	*bus = reg.ebx.byte.bh;
	*devfun = reg.ebx.byte.bl;
	return 0;
}
extern int upyet;
int
pci_get_irq_routing_options(rm_key_t hpci, ushort_t *sz, struct pci_irqrouting_entry *p, ushort_t *bitmap)
{
	cm_args_t cma;
	cgid_t	mycgid;
	int ret;
	size_t mysize;

	extern int upyet;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(sz != 0);

	cma.cm_key = hpci;
	cma.cm_param = CM_CGID;
	cma.cm_val = &mycgid;
	cma.cm_vallen = sizeof(mycgid);
	cma.cm_n = 0;
	cm_begin_trans(hpci, RM_READ);
	ret = cm_getval(&cma);
	cm_end_trans(hpci);
	mysize = *sz;
	ret = pci__get_irq_routing_options((uchar_t *)p, &mysize, bitmap);
	*sz = mysize;
	return ret;

}
