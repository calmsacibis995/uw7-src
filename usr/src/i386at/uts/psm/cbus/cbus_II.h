#ifndef _PSM_CBUS_CBUS_II_H
#define _PSM_CBUS_CBUS_II_H

#ident	"@(#)kern-i386at:psm/cbus/cbus_II.h	1.1"
#ident	"$Header$"

/*++

Copyright (c) 1992-1997  Corollary Inc.

Header Name:

    cbus_II.h

Abstract:

    this header provides the C-bus II hardware architecture definitions.

--*/

//
// general C-bus II defines.
//
#define	PCI_CONFIG_REGISTER_1		0xCF8
#define	PCI_CONFIG_REGISTER_2		0xCFC

#define	PCI_READ_HOST_REG_0			0x80000000

#define CBUS2_PCI_ID				0x118C

#define CBUS2_MAX_BRIDGES			2
#define	CBUS2_BOOT_BRIDGE			0

#define CBUS2_WRITE(addr, val)		(*(PULONG)(addr) = (ULONG)(val))
#define CBUS2_READ_CSR(addr)		(*(PULONG)(addr))
#define CBUS2_WRITE_CSR(addr, val)	CBUS2_WRITE((addr), (val))

#define	CBUS2_EISA_CONTROL_PORT		((USHORT)0xf2)
#define	CBUS2_DISABLE_8259			((UCHAR)0x4)

#define	CBUS2_BIOS_ADDRESS			0xFFFE0000
#define CBUS2_BIOS_SIZE				0xFFFF

//
// hardware features or software workarounds passed by RRD.
//
#define CBUS2_ENABLED_PW						0x01
#define CBUS2_DISTRIBUTE_INTERRUPTS				0x02
#define CBUS2_DISABLE_LEVEL_TRIGGERED_INT_FIX	0x04
#define CBUS2_DISABLE_SPURIOUS_CLOCK_CHECK      0x08
#define CBUS2_ENABLE_BROADCAST                  0x10

//
// for C-bus II, the CBC interrupt hardware supports all 256 interrupt
// priorities, and unlike the APIC, doesn't disable receipt of interrupts
// at granularities of 16-deep buckets.  instead the CBC uses the whole byte,
// instead of 4 bits like the APIC, giving us a granularity of 1.
// we use the same priority bucketing as our C-bus XM symmetric scheme
// (ie: each spl level gets 0x10 possible interrupts).  an important difference
// to remember is that the spl level to block a given interrupt must be at least
// as high as the highest priority assigned to that level.  (the APIC ignores
// the low 4 bits, so the blocking taskpri is just any vector with the high
// 4 bits equal to or greater than the highest assigned priority at that level).
//

//
// processor traps and various reserved vectors use up the first 0x20 vectors.
// by PSM convention, i8259 vectors use 0x20 through 0x3F.
//
//	IPI:							0xFE
//	Spurious Vector:				0xFF
//

//
// the next two vectors are private to the C-bus II PSM.
//
#define CBUS2_IPI_VECTOR			0xFE
#define CBUS2_SPURIOUS_VECTOR		0xFF

//
// a table converting software interrupt ipls to C-bus II specific offsets
// within a given CSR space.  note that all task priorities are shifted
// by the C-bus II register width (64 bits) to create the correct hardware
// offset to poke to cause the interrupt.  this table is declared here to
// optimize the assembly software interrupt request lookup.
//
#define CBUS2_REGISTER_SHIFT		3

//
// RRD will provide an entry for every C-bus II element.  to avoid
// using an exorbitant number of PTEs, this entry will specify
// only the CSR space within each C-bus II element's space.  and only
// a subset of that, as well, usually on the order of 4 pages.
// code wishing to access other portions of the cbus_element
// space will need to subtract down from the RRD-specified CSR
// address and map according to their particular needs.
//
#define CBC_MAX_CSR_BYTES			0x10000

//
// system timer increments every 100 nanoseconds.
//
#define CBC_SYSTEM_TIMER_TO_NSECS	100

//
// CBC definitions for the hardware interrupt map register.
//
#define CBC_HW_MODE_DISABLED		0x000
#define CBC_HW_EDGE_RISING			0x100	// ie: ISA card interrupts
#define CBC_HW_EDGE_FALLING			0x200
#define CBC_HW_LEVEL_HIGH			0x500
#define CBC_HW_LEVEL_LOW			0x600

#define CBC_HWINTR_MAP_ENTRIES		0x20

//
// CBC definitions for the interrupt configuration register.
//
#define CBC_HW_IMODE_DISABLED		0x0
#define CBC_HW_IMODE_ALLINGROUP		0x1
#define CBC_HW_IMODE_LIG			0x2
#define CBC_INTR_CONFIG_ENTRIES		0x100

//
// CBC definitions for the interrupt control register.
//
#define CBC_ICTL_DNRE				0x1	// directed NMI enable
#define CBC_ICTL_FLTE				0x2	// fault enable
#define CBC_ICTL_BDCE				0x4	// bus data correctable ecc enable
#define CBC_ICTL_TOE				0x8	// bus timeout enable
#define CBC_ICTL_BACE				0x10// bus address correctable ecc enable
#define CBC_ICTL_STOE				0x20// split transaction timeout enable
#define CBC_ICTL_BREE				0x40// bus retry error enable
#define CBC_ICTL_BRTE				0x80// bus retry timeout enable

//
// CBC definition for the interrupt indication register.
//
#define CBC_IIND_DNRN				0x1	// directed NMI
#define CBC_IIND_FLTN				0x2	// fault NMI
#define CBC_IIND_BDCI				0x4	// bus data correctable ecc interrupt
#define CBC_IIND_TOI				0x8	// bus timeout interrupt
#define CBC_IIND_BACI				0x10// bus address correctable ecc intr
#define CBC_IIND_STOI				0x20// split transaction timeout intr
#define CBC_IIND_BREI				0x40// bus retry error interrupt
#define CBC_IIND_BRTI				0x80// bus retry timeout interrupt

//
// bit 7 of the CBC configuration register must be turned off to enable
// posted writes for EISA I/O cycles.
//
#define CBC_DISABLE_PW  			0x80

//
// calculation to determine how many C-bus II system timer ticks
// are in a system clock tick.
//
#define CSRTICKS_PER_TICK			(os_tick_period.mst_nsec / \
									 CBC_SYSTEM_TIMER_TO_NSECS)

//
// General notes:
//
//	- ALL reserved fields must be zero filled on writes to
//	  ensure future compatibility.
//
//	- general CSR register length is 64 bits.
//

typedef struct _csr_register_t {
	ULONG		LowDword;
	ULONG		HighDword;
} CSR_REGISTER_T, *PCSR_REGISTER;

typedef union _elementid_t {
	struct {
		ULONG	ElementId : 4;
		ULONG	Reserved0 : 28;
		ULONG	Reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} ELEMENTID_T, *PELEMENTID;

typedef union _spurious_t {
	struct {
		ULONG	Vector : 8;
		ULONG	Reserved0 : 24;
		ULONG	Reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} SPURIOUS_T;

//
// the hardware interrupt map table (16 entries) is indexed by IRQ.
// lower numerical irq lines will receive higher interrupt priority.
//
// each CBC has its own hardware interrupt map registers.  note that
// each processor gets his own CBC, but it need only be used in this
// mode if there is an I/O card attached to its CBC.  each EISA bridge
// will have a CBC, which is used to access any devices on that EISA bus.
//
typedef union _hwintrmap_t {
	struct {
		ULONG	Vector : 8;
		ULONG	Mode : 3;
		ULONG	Reserved0 : 21;
		ULONG	Reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} HWINTRMAP_T, *PHWINTRMAP;

//
//
// the list of C-bus II CBC vector data:
//
// InUse is 0 if the entry is unused.
//
// IrqLine is the IRQ line this vector maps to.  this is used on interrupt
// receipt to translate the vector into an IRQ line the driver can understand.
//
typedef struct _cbus2_vectors_t {
	ULONG		IrqLine;				// irqline this vector is assigned to
	PHWINTRMAP	HardwareIntrMap;		// hardware intr map entry addr
	ULONG		HardwareIntrMapValue;	// hardware intr map entry value
	ULONG		Unused;
} CBUS2_VECTORS_T, *PCBUS2_VECTORS;

// 256 intrconfig registers for vectors 0 to 255.  this determines how
// a given processor will react to each of these vectors.  each processor
// has his own intrconfig table in his element space.
//
typedef union _intrconfig_t {
	struct {
		ULONG	IMode : 2;
		ULONG	Reserved0 : 30;
		ULONG	Reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} INTRCONFIG_T, *PINTRCONFIG;

//
// 256 interrupt request registers for vectors 0 to 255.
// parallel to the interrupt config register set above.
// this is used to send the corresponding interrupt vector.
// which processor gets it is determined by which element's
// address space you write to.
//
// The IRQ field should always be set when accessed by software.
// only hardware LIG arbitration will clear it.
//
typedef union _intrreq_t {
	struct {
		ULONG	Irq : 1;
		ULONG	Reserved0 : 31;
		ULONG	Reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} INTRREQ_T, *PINTRREQ;

//
// the C-bus II task priority register bit layout and
// minimum/maximum values are defined in cbus.h, as
// they need to be shared with the C-bus XM symmetric.
//

//
// used to read/write the current task priority.  reads DO NOT
// have to be AND'ed with 0xff - this register has been
// guaranteed by both Corollary (for the CBC) and Intel
// (for the APIC) so that bits 8-31 will always read zero.
// (the Corollary guarantee is written, the Intel is verbal).
//
// note that this definition is being used both for the
// 64-bit CBC and the 32-bit APIC, even though the APIC
// really only has the low 32 bits.
//
// task priority ranges from a low of 0 (all interrupts unmasked)
// to a high of 0xFF (all interrupts masked) on both CBC and APIC.
//
typedef union _taskpri_t {
	struct {
		ULONG	Pri : 8;
		ULONG	Zero : 24;
		ULONG	Reserved1 : 32;
	} ra;
	struct {
		ULONG	LowDword;
		ULONG	HighDword;
	} rb;
} TASKPRI_T, *PTASKPRI;

//
// offsets of various distributed interrupt registers within the CSR space.
//
typedef struct _csr {
	UCHAR			Pad0[0x10];							// 0x0000
	CSR_REGISTER_T	Reset;							    // 0x0010
	CSR_REGISTER_T	Nmi;					    		// 0x0018
	CSR_REGISTER_T	Led;					    		// 0x0020
	ELEMENTID_T		ElementId;							// 0x0028
	UCHAR			Pad1[0x100 - 0x28 - sizeof (ELEMENTID_T)];
	CSR_REGISTER_T	BridgeSelection;					// 0x0100
	UCHAR			Pad2[0x200 - 0x100 - sizeof (CSR_REGISTER_T)];
	TASKPRI_T		TaskPriority;						// 0x0200
	CSR_REGISTER_T	Pad3;								// 0x0208
	CSR_REGISTER_T	FaultControl;						// 0x0210
	CSR_REGISTER_T	FaultIndication;					// 0x0218
	CSR_REGISTER_T	InterruptControl;					// 0x0220
	CSR_REGISTER_T	ErrorVector;						// 0x0228
	CSR_REGISTER_T	InterruptIndication;				// 0x0230
	CSR_REGISTER_T	PendingPriority;					// 0x0238
	SPURIOUS_T		SpuriousVector;						// 0x0240
	CSR_REGISTER_T	WriteOnly;							// 0x0248
	UCHAR			Pad4[0x600 - 0x248 - sizeof (CSR_REGISTER_T)];
	HWINTRMAP_T		HwIntrMap[CBC_HWINTR_MAP_ENTRIES];	// 0x0600
	CSR_REGISTER_T	HwIntrMapEoi[CBC_HWINTR_MAP_ENTRIES];// 0x0700
	INTRCONFIG_T	InterruptConfig[CBC_INTR_CONFIG_ENTRIES];// 0x0800
	INTRREQ_T		IntrReq[CBC_INTR_CONFIG_ENTRIES];	// 0x1000
	UCHAR			Pad5[0x2000-0x1000-CBC_INTR_CONFIG_ENTRIES*sizeof(INTRREQ_T)];
	CSR_REGISTER_T	SystemTimer;				     	// 0x2000
	UCHAR			Pad6[0x3000 - 0x2000 - sizeof(CSR_REGISTER_T)];
	CSR_REGISTER_T	EccClear;							// 0x3000
	CSR_REGISTER_T	EccSyndrome;						// 0x3008
	CSR_REGISTER_T	EccWriteAddress;					// 0x3010
	CSR_REGISTER_T	EccReadAddress;						// 0x3018
	UCHAR			Pad7[0x8000 - 0x3018 - sizeof(CSR_REGISTER_T)];
	CSR_REGISTER_T	CbcConfiguration; 				    // 0x8000
} CSR_T, *PCSR;

#define CsrRegister					rb.LowDword

#endif // _PSM_CBUS_CBUS_II_H
