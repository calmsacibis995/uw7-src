#ident	"@(#)kern-i386at:psm/cbus/cbus_II.c	1.2"
#ident  "$Header$"

/*++

Copyright (c) 1992-1997  Corollary Inc.

Module Name:

    cbus_II.c

Abstract:

	this module implements the C-bus II specific functions that
	define the Corollary hardware architecture PSM back-end for C-bus II.

	the C-bus II architecture includes a 400MB/sec CPU-memory bus, as well
	as multiple PCI buses.  up to 10 Pentium CPUs can be supported
	in C-bus II.  C-bus II is fully symmetric: all CPUs can reach all
	memory and I/O devices, and similarly, all memory and I/O devices can
	reach all CPUs.  the CBC supports fully distributed, lowest in group
	interrupts, as well as broadcast capabilities.  each C-bus II CPU has
	an internal CPU write-back cache, as well as up to 2MB of L2 direct
	mapped write back cache and a fully associative write back L3 victim cache.

Function Prefix:

	functions in this routine are prefixed with Cbus2.

--*/

#include <cbus_includes.h>

//
// functions defined in this module.
//
ms_bool_t					Cbus2Present(void);
void						Cbus2ParseRrd(PCBUS_EXT_ID_INFO, ULONG);
void						Cbus2InitializeInterrupts(void);
void						Cbus2Setup(void);
void						Cbus2InitializeCpu(ms_cpu_t);
void						Cbus2DisableMyInterrupts(ms_cpu_t);
void						Cbus2EnableInterrupt(ms_cpu_t,ms_islot_t,ms_ivec_t);
void						Cbus2DisableInterrupt(ms_cpu_t,
								ms_islot_t, ms_ivec_t);
void						Cbus2EoiInterrupt(ms_cpu_t, ms_islot_t, ms_ivec_t);
void						Cbus2IpiCpu(ms_cpu_t);
ms_rawtime_t				Cbus2TimeGet(void);
void						Cbus2StartCpu(ms_cpu_t);
void						Cbus2OffLinePrep(ms_cpu_t);
void						Cbus2OffLineSelf(void);
void						Cbus2ShowState(void);
ms_bool_t					Cbus2ServiceTimer(ms_cpu_t, ms_islot_t, ms_ivec_t);
void						Cbus2ServiceXint(ms_cpu_t, ms_ivec_t);
void						Cbus2ServiceSpurious(ms_cpu_t, ms_ivec_t);
void						Cbus2PokeLed(ms_cpu_t, ULONG);
void						Cbus2Disable8259s(USHORT);
ULONG						Cbus2QueryInterruptPolarity(void);
ms_bool_t					Cbus2CheckSpuriousClock(PCSR, ULONG);

CBUS_SWITCH_T Cbus2Switch = {
	Cbus2Present,
	Cbus2ParseRrd,
	Cbus2Setup,
	Cbus2InitializeCpu,
	Cbus2EnableInterrupt,
	Cbus2DisableInterrupt,
	Cbus2EoiInterrupt,
	Cbus2IpiCpu,
	Cbus2TimeGet,
	Cbus2StartCpu,
	Cbus2OffLinePrep,
	Cbus2OffLineSelf,
	Cbus2ShowState,
	Cbus2ServiceTimer,
	Cbus2ServiceXint,
	Cbus2ServiceSpurious,
	Cbus2PokeLed
};

/*++

Routine Description:

	check if this is a Corollary C-bus II architecture.

Arguments:

	none.

Return Value:

	MS_TRUE if it is a Corollary C-bus II architecture, MS_FALSE otherwise.

--*/
STATIC ms_bool_t
Cbus2Present(
	void
)
{
	ULONG	pciId;

	if ((CbusGlobal.MachineType & (CTAB_MACHINE_CBUS2)) == 0) {
		return MS_FALSE;
	}

#ifdef DEBUG
	os_printf("Cbus2Present: C-bus II system found\n");
#endif

	//
	// determine bus type of C-bus II platform.  default to EISA
	// and then check for PCI.  PCI is checked by reading Host Bus
	// Bridge Register 0.  this gives us the PCI ID for the Host
	// Bus Bridge.  if this matches the PCI ID for C-bus II,
	// then it is PCI.
	//
	CbusBusType = MSR_BUS_EISA;
	outl(PCI_CONFIG_REGISTER_1, PCI_READ_HOST_REG_0);
	pciId = inl(PCI_CONFIG_REGISTER_2) & 0xffff;

	// Some DPBs have no hard-wired pciId. If PCI_CONFIG_REGISTER_1 is
	// latching writes assume the system has PCI. All C-bus systems
	// are PCI Mode 1.
	if ((pciId == CBUS2_PCI_ID) ||
            (inl(PCI_CONFIG_REGISTER_1) == PCI_READ_HOST_REG_0))
		CbusBusType = MSR_BUS_PCI;

	outl(PCI_CONFIG_REGISTER_1, 0);

	CbusVendor = CTAB_OEM_COROLLARY;

	CbusVendorClass = CTAB_CBUS2EISA;

	return MS_TRUE;
}

/*++

Routine Description:

	check for C-bus II I/O bridges and disable their incoming interrupts here.

Arguments:

	cbusExtIdInfo	- Supplies a pointer to the RRD extended ID
					  information table.

	cbusValidIds	- Supplies the number of valid entries in the RRD extended
					  ID information table.

Return Value:

	none.

--*/
STATIC void
Cbus2ParseRrd(
	PCBUS_EXT_ID_INFO	cbusExtIdInfo,
	ULONG				cbusValidIds
)
{
	PCBUS_EXT_ID_INFO	cbusExtIdInfop;
	ULONG				index;
	ULONG 				vector;
	ULONG 				intrControl;
    PCSR				csr;

	cbusExtIdInfop = cbusExtIdInfo;
#ifdef DEBUG
	os_printf("Cbus2ParseRrd: cbusValidIds = %x\n", cbusValidIds);
#endif

	for (index = 0; index < cbusValidIds; index++, cbusExtIdInfop++) {

		if ((cbusExtIdInfop->PelFeatures & CTAB_ELEMENT_BRIDGE) == 0) {
			//
			// skip I/O bridges.
			//
			continue;
		}

		csr = (PCSR)os_physmap(cbusExtIdInfop->PelStart,
			cbusExtIdInfop->PelSize);

		//
		// to go from 8259 to CBC mode for interrupt handling:
		//
		//	a) disable PC compatible interrupts, ie: stop each
		//	   bridge CBC from asking its 8259 to satisfy INTA
		//	   pulses to the CPU.
		//	b) mask off ALL 8259 interrupt input lines EXCEPT
		//	   for IRQ0.  since clock interrupts are not external
		//	   in the Intel chipset, the bridge 8259 must enable
		//	   them even when the CBC is enabled.  putting the
		//	   8259 in pass-through mode (ie: the 8259 IRQ0 input
		//	   will just be wired straight through) WILL NOT
		//	   allow the 8259 to actually talk to the CPU; it
		//	   just allows the interrupt to be seen by the CBC.
		//	   the CBC is responsible for all the CPU interrupt
		//	   handshaking.
		//	c) initialize the hardware interrupt map for the IRQ0
		//	   entry.
		//	d) enable each participating element's (ie: CPUs only)
		//	   interrupt configuration register for the vector
		//	   the PSM has programmed IRQ0 to actually generate.
		//
		//	IT IS CRITICAL THAT THE ABOVE STEPS HAPPEN IN THE
		//	ORDER OUTLINED, OTHERWISE YOU MAY SEE SPURIOUS
		//	INTERRUPTS.
		//
		// now process this I/O bridge:
		//
		// currently assumes that all bridges will be of the same
		// flavor. if this element is a bridge, map it systemwide
		// and disable all incoming interrupts on this bridge.
		// any extra bridges beyond our configuration maximum
		// are just disabled, and not used by the PSM.
		//
		if (Cbus2BridgesFound < CBUS2_MAX_BRIDGES) {
			Cbus2BridgeCsr[Cbus2BridgesFound] = csr;
			Cbus2BridgesFound++;
		}
	
		if (cbusExtIdInfop->PelFeatures & CTAB_ELEMENT_HAS_8259) {
			intrControl = inb(CbusGlobal.Control8259Mode);
			outb(CbusGlobal.Control8259Mode,
				(UCHAR)(intrControl | CbusGlobal.Control8259ModeVal));
			//
			// disable all inputs in the 8259 IMRs except for the
			// IRQ0 and explicitly force these masks onto the 8259s.
			// called to switch into full distributed interrupt chip mode.
			// our distributed interrupt chip logic in the CBC handles
			// all interrupts from this point on.  the only reason we have
			// to leave IRQ0 enabled is because Intel's EISA chipset doesn't
			// leave an external IRQ0 clock line for us to wire into the CBC.
			// hence, we have wired it from the 8259 into the CBC, and must
			// leave it enabled in the 8259 IMR.  note that we will never allow
			// the now passive 8259 to respond to a CPU INTA cycle, but we do
			// need to see the interrupt ourselves in the CBC so we can drive
			// an appropriate vector during the INTA.
			//
			// note that the IBM implementation of the C-bus II system
			// does have routing control over IRQ0.  therefore, for
			// that platform only, all inputs on the 8259 must be masked.
			//
			// this is the ONLY place in the PSM where the 8259s are accessed.
			//
			if (CbusHardwareInfo.OemRomInfo.OemNumber == CTAB_OEM_IBM_MCA)
				Cbus2Disable8259s(0xFFFF);
			else
				Cbus2Disable8259s(0xFFFE);
		}
	
		//
		// in the midst of setting up the EISA element CBCs,
		// a device interrupt that was pending in a bridge's 8259 ISRs
		// may be lost.  none should be fatal, even an 8042 keystroke,
		// since the keyboard driver does a flush on open, and will,
		// thus recover in the same way the standard uniprocessor
		// system does when it initializes 8259s.
		//
	}

	Cbus2InitializeInterrupts();

#ifdef DEBUG
	os_printf("Cbus2ParseRrd: done\n");
#endif
}

/*++

Routine Description:

	set up some global data for the front-end and initialize the
	CbusSlotData[] table.

Arguments:

	none.

Return Value:

	none.

--*/
STATIC void
Cbus2InitializeInterrupts(
	void
)
{
	PCSR		bridgeCsr = (PCSR)Cbus2BridgeCsr[CBUS2_BOOT_BRIDGE];
	ms_islot_t	slot;

#ifdef DEBUG
	os_printf("Cbus2InitializeInterrupts:\n");
#endif
	//
	// query the polarity of the interrupts and
	// latch for later inquiry when enabling the interrupts.
	//
	CbusIrqPolarity = Cbus2QueryInterruptPolarity();

	CbusXintVector = CBUS2_IPI_VECTOR;
	CbusSpuriousVector = CBUS2_SPURIOUS_VECTOR;

	//
	// for each slot, set up a convenient
	// table for obtaining the hardware interrupt
	// map entry on the bridge for the IRQ.
	//
	for (slot = 0; slot < CbusIslotMax + 1; slot++) {
		Cbus2SlotData[slot].HardwareIntrMap = (&bridgeCsr->HwIntrMap[slot]);
	}
}

/*++

Routine Description:

	establish CSR mappings for all C-bus II CPUs.

Arguments:

	none.

Return Value:

	none.

--*/
STATIC void
Cbus2Setup(
	void
)
{
	PCBUS_EXT_ID_INFO	infop = CbusExtIdInfo;
	ULONG				index1;
	ULONG				index2;
	ULONG				index3;

	CbusNumCpus = 1;
	infop = CbusExtIdInfo;
	index2 = 1;
	for (index1 = 0; index1 < CbusValidIds; index1++, infop++)  {
		if (infop->Id == CbusGlobal.BroadcastId) {
			continue;
		}

		if (infop->ProcType == CTAB_PT_NO_PROCESSOR || infop->Pm == NULL) {
			continue;
		}

 		if (infop->Id == CbusGlobal.BootId) {
			index3 = 0;
		}
		else {
			index3 = index2;
			index2++;
			CbusNumCpus++;
		}

		CbusCpuIds[index3] = infop->Id;

		Cbus2CsrSpace[index3] = 
			(PCSR)os_physmap(infop->PelStart, infop->PelSize);

#ifdef DEBUG
		os_printf("Cbus2Setup: cpu=%x id=%x vaddr=%x\n",
			index3, CbusCpuIds[index3], Cbus2CsrSpace[index3]);
#endif

		if (index3 && Cbus2CsrSpace[index3]) {
			CBUS2_WRITE((ULONG)Cbus2CsrSpace[index3] + CbusGlobal.SReset,
				CbusGlobal.SResetVal);
		}
	}
}

/*++

Routine Description:

	initialize this CPU's CSR, interrupts, spurious interrupt
	and IPI vector.

Arguments:

   cpu	- supplies a logical cpu number.

Return Value:

   none.

--*/
STATIC void
Cbus2InitializeCpu(
	ms_cpu_t	cpu
)
{
	PCSR		csr;
	ULONG		cbcConfig;

	csr = Cbus2CsrSpace[cpu];

	//
	// generate NMIs (trap 2) when we get error interrupts.
	//
	csr->ErrorVector.LowDword = CBUS_NMIFLT;
	csr->InterruptControl.LowDword = CbusGlobal.IntrControlMask;
	csr->FaultControl.LowDword = CbusGlobal.FaultControlMask;

	//
	// initialize the spurious vector for the CBC
	// to generate when it detects inconsistencies.
	//
	csr->SpuriousVector.CsrRegister = CBUS2_SPURIOUS_VECTOR;

	//
	// disable all of this CPU's incoming interrupts _AND_
	// any generated by its local CBC (otherwise they could go to
	// any CPU).
	//
	Cbus2DisableMyInterrupts(cpu);

	if (cpu == CbusMsopBootCpu) {
		//
		// need to enable the CBC for the timer interrupt.
		//
		Cbus2EnableInterrupt(cpu, CBUS_TIMER_SLOT, CBUS_TIMER_VECTOR);

		if (CbusGlobal.Cbus2Features & CBUS2_ENABLED_PW) {
			//
			// setida is a misleading name - if the
			// posted-writes bit is enabled, then allow EISA
			// I/O cycles to use posted writes.
			//
			// call a function here so the compiler won't use byte
			// enables here - we must force a dword access.
			//
			cbcConfig = CBUS2_READ_CSR(&csr->CbcConfiguration.LowDword);

			CBUS2_WRITE_CSR(&csr->CbcConfiguration.LowDword,
				cbcConfig & ~CBC_DISABLE_PW);
		}

		if (CbusGlobal.Cbus2Features & CBUS2_DISABLE_LEVEL_TRIGGERED_INT_FIX)
			Cbus2FixLevelInterrupts = 0;

		if (CbusGlobal.Cbus2Features & CBUS2_DISABLE_SPURIOUS_CLOCK_CHECK)
			Cbus2FixSpuriousClock = 0;
	}

	//
	// initialize the CBC, high-resolution, 100nsec counter.
	//
	CBUS2_WRITE_CSR(&csr->SystemTimer, (ULONG)0);

	//
	// for each CPU, the IPI and spurious vectors are hardware enabled.
	// the slot is a don't care in this case.
	//
	Cbus2EnableInterrupt(cpu, -1, CbusXintVector);
	Cbus2EnableInterrupt(cpu, -1, CbusSpuriousVector);

	//
	// ensure that the CBC isn't blocking interrupts via its TPR.
	//
	CBUS2_WRITE_CSR(&csr->TaskPriority, (ULONG)0);
}

/*++

Routine Description:

	by default, disable all of the calling CPU's
	interrupt configuration registers (ICRs) so that it
	will take no interrupts.

	all bridges have had their interrupts disabled already.
	as each interrupt is enabled, it will need to be enabled
	at the bridge, and also on each CPU participating in the
	reception.

Arguments:

   cpu - supplies the caller's logical cpu number whose
		 interrupts will be disabled.

Return Value:

   none.

--*/
STATIC void
Cbus2DisableMyInterrupts(
	ms_cpu_t	cpu
)
{
	ULONG		vector;
    PCSR		csr;

	csr = Cbus2CsrSpace[cpu];

	for (vector = 0; vector < CBC_INTR_CONFIG_ENTRIES; vector++) {
		csr->InterruptConfig[vector].CsrRegister = CBC_HW_IMODE_DISABLED;
	}

	//
	// in the midst of setting up the bridge element CBCs,
	// a device interrupt that was pending in a bridge's
	// 8259 ISRs may be lost.  none should be fatal, even an
	// 8042 keystroke, since the keyboard driver should do a flush
	// on open, and thus recover in the same way the standard
	// uniprocessor kernel does when it initializes 8259s.
	//
}

/*++

Routine Description:

	if requested to enable the IPI or spurious vectors, just configure
	the interrupt configuration register and return.

	otherwise, determine the setting for the bridge hardware interrupt
	map entry (LEVEL or EDGE).

	Note: this routine must be executed by every CPU wishing to
	participate in the interrupt receipt.  also, remember that the
	hardware interrupt map entry must be programmed _BEFORE_ the
	interrupt configuration register for bridge interrupts.

Arguments:

	cpu		- CPU to enable the interrupt.

	slot	- interrupt slot to enable.

	vector	- vector to enable.

Return Value:

	none.

--*/
STATIC void
Cbus2EnableInterrupt(
	ms_cpu_t		cpu,
	ms_islot_t		slot,
	ms_ivec_t		vector
)
{
	PHWINTRMAP		hwIntrEntry;			// CBC entry generating the intr
	PCSR			csr;
	ULONG			csrVal;
	ULONG			irqLine;

#ifdef DEBUG
	os_printf("Cbus2EnableInterrupt: cpu=%x slot=%x vector=%x\n",
		cpu, slot, vector);
#endif

	csr = Cbus2CsrSpace[cpu];

	//
	// if it is the IPI or spurious vector, all that's needed
	// is to enable the ICR.
	//
	if (vector == CbusXintVector || vector ==  CbusSpuriousVector) {
        csr->InterruptConfig[vector].CsrRegister = CBC_HW_IMODE_ALLINGROUP;
        return;
	}

	hwIntrEntry = Cbus2SlotData[slot].HardwareIntrMap;

	//
	// all interrupts occur on the boot CPU and,
	// if they are bridge interrupts (IRQs),
	// then they require an EOI.  IPIs and spurious
	// interrupts do not require an EOI.  based upon
	// the polarity of the interrupt, determine the
	// interrupt setting (LOW or HIGH).  Cbus2EoiNeeded[]
	// table and a local value used to set up the hardware.
	// the table is used later for the level-triggered EOI
	// hardware workaround.
	//
	if ((CbusIrqPolarity >> slot) & 1) {
		csrVal = CBC_HW_LEVEL_LOW;
	}
	else {
		csrVal = CBC_HW_EDGE_RISING;
	}

	//
	// if it is the clock interrupt, set it for CBC_HW_IMODE_ALLINGROUP.
	//
	if (vector == CBUS_TIMER_VECTOR) {
        csr->InterruptConfig[vector].CsrRegister = CBC_HW_IMODE_ALLINGROUP;
        goto setHwIntrMap;
	}

	//
	// all other interrupts get set up as ALLINGROUP.
	// if Lowest In Group (LIG) is ever utilized,
	// it would be done here and conditionalized
	// with CbusGlobal.Cbus2Features & CBUS2_DISTRIBUTE_INTERRUPTS.
	//
	csr->InterruptConfig[vector].CsrRegister = CBC_HW_IMODE_ALLINGROUP;

setHwIntrMap:
	hwIntrEntry->CsrRegister = (csrVal | vector);
	Cbus2SlotData[slot].HardwareIntrMapValue = (csrVal | vector);
}

/*++

Routine Description:

	disable the specified interrupt so it can not occur on the calling
	CPU upon return from this routine.

Arguments:

	cpu		- CPU to disable the interrupt.

	slot	- interrupt slot to disable.

	vector	- vector to disable.

Return Value:

	none.

--*/
STATIC void
Cbus2DisableInterrupt(
	ms_cpu_t		cpu,
	ms_islot_t		slot,
	ms_ivec_t		vector
)
{
	PHWINTRMAP		hwIntrEntry;			// CBC entry generating the intr
	PCSR			csr;

#ifdef DEBUG
	os_printf("Cbus2DisableInterrupt: cpu=%x slot=%x vector=%x\n",
		cpu, slot, vector);
#endif

	//
	// point at the hardware interrupt map entry address on
	// the CBC of the bridge whose interrupt slot is specified.
	//
	hwIntrEntry = Cbus2SlotData[slot].HardwareIntrMap;

	//
	// reaching out to the specific bridge CBC will disable the
	// interrupt at the source, and now NO cpu will see it.
	//
	hwIntrEntry->CsrRegister = CBC_HW_MODE_DISABLED;

	//
	// tell the world that _THIS CPU_ is no longer
	// participating in receipt of this interrupt.  this code
	// really is optional since we have already killed the
	// interrupt at the source.  but it's a useful template
	// if we only wish for this particular CPU to no
	// longer participate in the interrupt arbitration.
	//
	csr = Cbus2CsrSpace[cpu];
	csr->InterruptConfig[vector].CsrRegister = CBC_HW_IMODE_DISABLED;
}

/*++

Routine Description:

	if the interrupt slot is not a bridge interrupt, then return -
	there is no interrupt needed for bridge interrupts.  otherwise,
	EOI the interrupt.

	Note that the CBC workaround is checked here for special
	EOI processing.

Arguments:

	cpu		- CPU to EOI the interrupt.

	slot	- interrupt slot to EOI.

	vector	- vector to EOI.

Return Value:

	none.

--*/
STATIC void
Cbus2EoiInterrupt(
	ms_cpu_t	cpu,
	ms_islot_t	slot,
	ms_ivec_t	vector
)
{
	PCSR		bridgeCsr;				// CSR to EOI
	PHWINTRMAP	hwIntrEntry;			// pointer to hardware intr map
	ULONG		hwIntrValue;			// value to restore to hwintrmap
	ULONG		statusFlags;			// flags returned by intr_disable()

	//
	// if the interrupt slot is not a bridge interrupt slot, return.
	//
	if (slot > CbusIslotMax)
		return;

	//
	// if the CBC hardware workaround is in effect,
	// then special EOI processing is needed.  to EOI
	// a level-triggered interrupt, write a 0 to the
	// hardware interrupt map entry on the bridge and
	// then re-write the original value.
	//
	if (Cbus2FixLevelInterrupts && ((CbusIrqPolarity >> slot) & 1)) {
		//
		// point at the hardware interrupt map entry address on
		// the CBC of the bridge whose interrupt slot is specified.
		//
		hwIntrEntry = Cbus2SlotData[slot].HardwareIntrMap;
		hwIntrValue = Cbus2SlotData[slot].HardwareIntrMapValue;
		statusFlags = intr_disable();
		*(ULONG *)hwIntrEntry = NULL;
		*(ULONG *)hwIntrEntry = hwIntrValue;
		intr_restore(statusFlags);
		return;
	}

	//
	// otherwise, simply EOI the interrupt.
	//
	bridgeCsr = (PCSR)Cbus2BridgeCsr[CBUS2_BOOT_BRIDGE];
	bridgeCsr->HwIntrMapEoi[slot].LowDword = 1;
}

/*++

Routine Description:

	send an IPI to the requested CPU.

Arguments:

	cpu    - CPU to IPI.

Return Value:

	none.

--*/
STATIC void
Cbus2IpiCpu(
	ms_cpu_t	cpu
)
{
	PCSR		csr;						// CSR to IPI

	csr = Cbus2CsrSpace[cpu];
	*((ULONG *)&(csr->IntrReq[CbusXintVector])) = 1;
}

/*++

Routine Description:

	get the current value of a free-running, high-resolution clock
	counter in PSM-specific units.

Arguments:

	none.

Return Value:

	current time stamp that is opaque to the base kernel.

--*/
STATIC ms_rawtime_t
Cbus2TimeGet(
	void
)
{
	ms_rawtime_t	rawTime;
	ULONG			oldLock;

	//
	// CbusMsopRawTime.Lock is used to guard against the interim updating of
	// CbusMsopRawTime.Time.  if CbusMsopRawTime.Lock is non-zero and didn't
	// change while retrieving CbusRawTime.Time, then it is returned.
	//
	do {
		oldLock = CbusMsopRawTime.Lock;
		rawTime = CbusMsopRawTime.Time;
	} while (oldLock == 0 || oldLock != CbusMsopRawTime.Lock);

	return rawTime;
}

/*++

Routine Description:

	start the specified CPU running kernel code at the indicated address.

Arguments:

	cpu			- CPU to start running.

Return Value:

	none.

--*/
STATIC void
Cbus2StartCpu(
	ms_cpu_t	cpu
)
{
	CBUS2_WRITE((ULONG)Cbus2CsrSpace[cpu] + CbusGlobal.SReset,
		CbusGlobal.SResetVal);
	psm_time_spin(1000);
	CBUS2_WRITE((ULONG)Cbus2CsrSpace[cpu] + CbusGlobal.CReset,
		CbusGlobal.CResetVal);
}

/*++

Routine Description:

	called on a CPU which is about to be brought offline.
	simply call Cbus2DisableMyInterrupts() to disable
	all interrupts for this CPU.

Arguments:

	cpu	- CPU which is about to be brought offline.

Return Value:

	none.

--*/
STATIC void
Cbus2OffLinePrep(
	ms_cpu_t	cpu
)
{
	Cbus2DisableMyInterrupts(cpu);
}

/*++

Routine Description:

	called on a CPU which is being brought offline.  all OS-specific offline
	processing will already have been done.  the CPU is no longer expected
	to field interrupts and may either spin waiting to be brought back
	online or may be completely shut down.

	called with interrupts disabled at the CPU.

Arguments:

	cpu	- CPU which is about to be brought offline.

Return Value:

	none.

--*/
STATIC void
Cbus2OffLineSelf(
	void
)
{
	asm("cli");
	asm("wbinvd");
	asm("hlt");

	//
	// not reached.
	//
}

/*++

Routine Description:

	called during a system crash.  this routine prints out platform-specific
	state that can be useful for analysis.

	called with interrupts disabled at the CPU.

Arguments:

	none.

Return Value:

	none.

--*/
STATIC void
Cbus2ShowState(
	void
)
{
}

/*++

Routine Description:

	timer interrupt service entry point.
	manage the PSM-specific, high-resolution counter and
	EOI the interrupt.

Arguments:

	cpu		- CPU to EOI the timer interrupt.

	slot	- interrupt slot to EOI.

	vector	- vector to EOI.

Return Value:

	MS_TRUE if this system timer interrupt is spurious, MS_FALSE otherwise.

--*/
STATIC ms_bool_t
Cbus2ServiceTimer(
	ms_cpu_t		cpu,
	ms_islot_t		slot,
	ms_ivec_t		vector
)
{
	PCSR			csr;
	ULONG			newLock;
	ULONG			systemTimer;
	ULONG			newSystemTimer;
	UCHAR			timerFlag;
	ms_bool_t		returnValue;

	if (cpu == CbusMsopBootCpu) {
		//
		// we need to rearm the clock in the case of
		// the IBM 720 due to it being a Microchannel system.
		//
		if (CbusHardwareInfo.OemRomInfo.OemNumber == CTAB_OEM_IBM_MCA) {
			timerFlag = inb(I8254_AUX_PORT);
			timerFlag |= 0x80;
			outb(I8254_AUX_PORT, timerFlag);
		}

		//
		// read the CBC 100nsec system timer and convert it
		// to nanoseconds.  add it to the global, PSM-specific,
		// high-resolution counter.  if the low 32-bits wrap,
		// then increment the high.
		//
		csr = Cbus2CsrSpace[cpu];
		newLock = CbusMsopRawTime.Lock + 1;
		CbusMsopRawTime.Lock = NULL;
		systemTimer = CBUS2_READ_CSR(&csr->SystemTimer);

		//
		// ignore this interrupt if it is spurious.
		//
		if (Cbus2CheckSpuriousClock(csr, systemTimer) == MS_TRUE) {
	        Cbus2EoiInterrupt(cpu, slot, vector);
			return MS_TRUE;
		}

		newSystemTimer = CBUS2_READ_CSR(&csr->SystemTimer);
		systemTimer -= newSystemTimer;
		systemTimer *= CBC_SYSTEM_TIMER_TO_NSECS;
		systemTimer += CbusMsopRawTime.Time.msrt_lo;
		if (systemTimer >= CBUS_NANOSECONDS_IN_SECOND) {
			systemTimer -= CBUS_NANOSECONDS_IN_SECOND;
			CbusMsopRawTime.Time.msrt_hi++;
		}
		CbusMsopRawTime.Time.msrt_lo = systemTimer;
		CbusMsopRawTime.Lock = newLock ? newLock : 1;
        Cbus2EoiInterrupt(cpu, slot, vector);

		return MS_FALSE;
	}
	return MS_TRUE;
}

/*++

Routine Description:

	service a cross-processor interrupt.

Arguments:

	cpu		- cpu that received the cross-processor interrupt.

	vector	- vector that generated the cross-processor interrupt.

Return Value:

	none.

--*/
STATIC void
Cbus2ServiceXint(
	ms_cpu_t	cpu,
	ms_ivec_t	vector
)
{
}

/*++

Routine Description:

	service a spurious interrupt.

Arguments:

	cpu		- cpu that received the spurious interrupt.

	vector	- vector that generated the spurious interrupt.

Return Value:

	none.

--*/
STATIC void
Cbus2ServiceSpurious(
	ms_cpu_t	cpu,
	ms_ivec_t	vector
)
{
}

/*++

Routine Description:

	poke the cpu LED on or off.

Arguments:

	cpu		- cpu whose LED we want to turn on or off.

	flag	- flag to turn the LED on or off.

Return Value:

	none.

--*/
STATIC void
Cbus2PokeLed(
	ms_cpu_t	cpu,
	ULONG		flag
)
{
	PCSR		csr = Cbus2CsrSpace[cpu];

	if (flag == MS_TRUE) {
		csr->Led.LowDword = 1;
	}
	else {
		csr->Led.LowDword = 0;
	}
}

/*++

Routine Description:


Arguments:

	islotMask	- at 16-bit mask for each interrupt slot is passed
				  to this routine.  for each bit is set, i8259_intr_mask()
				  is called to mask that interrupt slot at the 8259.

Return Value:

	none.

--*/
STATIC void
Cbus2Disable8259s(
	USHORT	islotMask
)
{
	ms_islot_t	islot;

	for (islot = 0; islot < CbusIslotMax + 1; islot++) {
		if (islotMask & (1 << islot)) {
			i8259_intr_mask(islot);
		}
	}
}

/*++

Routine Description:

	called once to read the EISA interrupt configuration registers.
	this will tell us which interrupt lines are level-triggered and
	which are edge-triggered.  note that irqlines 0, 1, 2, 8 and 13
	are not valid in the 4D0/4D1 registers and are defaulted to edge.

Arguments:

	none.

Return Value:

	the interrupt line polarity of all the EISA irqlines in the system.

--*/
STATIC ULONG
Cbus2QueryInterruptPolarity(
	void
)
{
	unsigned long	interruptLines = 0;

	//
	// read the edge-level control register (ELCR) so we'll know how
	// to mark each driver's interrupt line (ie: edge or level triggered).
	//
	interruptLines = ( ((unsigned long)inb(PIC2_ELCR_PORT) << 8) |
					   ((unsigned long)inb(PIC1_ELCR_PORT)) );
	//
	// explicitly mark irqlines 0, 1, 2, 8 and 13 as edge.  leave all
	// other irqlines at their current register values.  if the system
	// has a Microchannel Bus, then use the ELCR register contents as is.
	//
	if (CbusHardwareInfo.OemRomInfo.OemNumber != CTAB_OEM_IBM_MCA)
		interruptLines &= ELCR_MASK;

	return interruptLines;
}

/*++

Routine Description:

	this function is responsible for detecting spurious system clock
	interrupts.  if it correctly determines that a clock interrupt is
	spurious, the function returns MS_TRUE and the PSM will skip this
	interrupt.  note that the interrupt must still be EOI'd in this case.

Arguments:

	csr			- pointer to CSR space for the cpu that
				  received the system timer interrupt.

	systemTimer	- current CBC system timer value.

Return Value:

	MS_TRUE if the system timer interrupt is spurious, MS_FALSE otherwise.

--*/
STATIC ms_bool_t
Cbus2CheckSpuriousClock(
	PCSR		csr,
	ULONG		systemTimer
)
{
	ULONG		newSystemTimer;

	if (Cbus2FixSpuriousClock == 0)
		return MS_FALSE;

	//
	// the system timer is in 100nsec units.  use this timer
	// to determine if the current clock interrupt is a spurious
	// clock interrupt.  if the current clock interrupt is spurious,
	// then the CSR system timer value will be less than 10msec.
	// after determining that this is a valid interrupt, the
	// system timer value is carefully reset to account for clock
	// interrupts that have been held off.
	//
	if (systemTimer < CSRTICKS_PER_TICK)
		return MS_TRUE;

	newSystemTimer = (systemTimer % CSRTICKS_PER_TICK) + 1;
	CBUS2_WRITE_CSR(&csr->SystemTimer, newSystemTimer);

	return MS_FALSE;
}
