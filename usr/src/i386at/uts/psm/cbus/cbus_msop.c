#ident	"@(#)kern-i386at:psm/cbus/cbus_msop.c	1.4"
#ident	"$Header$"

/*++

Copyright (c) 1992-1997  Corollary Inc.

Module Name:

	cbus_msop.c

Abstract:

	this module comprises all the MSOP interface routines that represent
	the PSM interface to the kernel.  as much as possible, the routines
	in this file try to isolate the operating specific functions allowing
	the other files to be architecture specific.

Function Prefix:

	functions in this routine are prefixed with CbusMsop.

--*/

#include <cbus_includes.h>

//
// functions defined in this module.
//
ms_bool_t					CbusMsopinitpsm(void);
void						CbusMsopInitCpu(void);
ms_bool_t					CbusMsopIntrAttach(ms_intr_dist_t *);
void						CbusMsopIntrDetach(ms_intr_dist_t *);
void						CbusMsopIntrMask(ms_intr_dist_t *);
void						CbusMsopIntrUnmask(ms_intr_dist_t *);
void						CbusMsopIntrComplete(ms_intr_dist_t *);
void						CbusMsopXpost(ms_cpu_t, ms_event_t);
void						CbusMsopTick2(ms_time_t);
void						CbusMsopTimeGet(ms_rawtime_t *);
void						CbusMsopTimeAdd(ms_rawtime_t *, ms_rawtime_t *);
void						CbusMsopTimeSub(ms_rawtime_t *, ms_rawtime_t *);
void						CbusMsopTimeCvt(ms_time_t *, ms_rawtime_t *);
void						CbusMsopIdleSelf(void);
void						CbusMsopIdleExit(ms_cpu_t);
void						CbusMsopStartCpu(ms_cpu_t, ms_paddr_t);
void						CbusMsopOffLinePrep(void);
void						CbusMsopOffLineSelf(void);
void						CbusMsopShowState(void);
void						CbusMsopShutdown(ULONG);
void						CbusMsopInitMsparam(void);
ms_intr_dist_t *			CbusMsopServiceInterrupt(ms_ivec_t);
ms_intr_dist_t *			CbusMsopServiceTimer(ms_ivec_t);
ms_intr_dist_t *			CbusMsopServiceXint(ms_ivec_t);
ms_intr_dist_t *			CbusMsopServiceSpurious(ms_ivec_t);
ms_intr_dist_t *			CbusMsopServiceStray(ms_ivec_t);

//
// MSOP functions described and informed to the kernel via this structure.
//
msop_func_t	CbusMsops[] = {
	{ MSOP_INIT_CPU,		(void *)CbusMsopInitCpu },
	{ MSOP_INTR_ATTACH,		(void *)CbusMsopIntrAttach },
	{ MSOP_INTR_DETACH,		(void *)CbusMsopIntrDetach },
	{ MSOP_INTR_MASK,		(void *)CbusMsopIntrMask },
	{ MSOP_INTR_UNMASK,		(void *)CbusMsopIntrUnmask },
	{ MSOP_INTR_COMPLETE,	(void *)CbusMsopIntrComplete },
	{ MSOP_TICK_2,			(void *)CbusMsopTick2 },
	{ MSOP_TIME_GET,		(void *)CbusMsopTimeGet },
	{ MSOP_TIME_ADD,		(void *)CbusMsopTimeAdd },
	{ MSOP_TIME_SUB,		(void *)CbusMsopTimeSub },
	{ MSOP_TIME_CVT,		(void *)CbusMsopTimeCvt },
	{ MSOP_TIME_SPIN,		(void *)psm_time_spin },
	{ MSOP_XPOST,			(void *)CbusMsopXpost },
	{ MSOP_RTODC,			(void *)psm_mc146818_rtodc },
	{ MSOP_WTODC,			(void *)psm_mc146818_wtodc },
	{ MSOP_IDLE_SELF,		(void *)CbusMsopIdleSelf },
	{ MSOP_IDLE_EXIT,		(void *)CbusMsopIdleExit },
	{ MSOP_START_CPU,		(void *)CbusMsopStartCpu },
	{ MSOP_OFFLINE_PREP,	(void *)CbusMsopOffLinePrep },
	{ MSOP_OFFLINE_SELF,	(void *)CbusMsopOffLineSelf },
	{ MSOP_SHUTDOWN,		(void *)CbusMsopShutdown },
	{ MSOP_SHOW_STATE,		(void *)CbusMsopShowState },
	{ 0,					NULL }
};

/*++

Routine Description:

	after verifying that this is a C-bus platform, scan the table to find
	the system configuration, and initialize accordingly.

Arguments:

	none.

Return Value:

	none.

--*/
ms_bool_t
CbusMsopinitpsm(
	void
)
{
	ms_resource_t *	resourcep;
	ULONG			index;

	if ((*CbusFunctions.Present)() == MS_FALSE) {

#ifdef DEBUG
		os_printf(
			"CbusMsopInitPsm: Not a Corollary C-bus Architecture System\n");
#endif

		return MS_FALSE;
	}

	if (os_register_msops(CbusMsops) != MS_TRUE) {
		return MS_FALSE;
	}

	os_printf("Corollary C-bus PSM Version %s\n", CbusVersion);

	//
	// this is the commit point.  if we get here, we must
	// return MS_TRUE, committing the system to run with this PSM.
	// next we do our allocations based upon what the back-end
	// modules have found.
	//
	CbusMsopTopology = (ms_topology_t *)os_alloc(sizeof(ms_topology_t));

	CbusMsopTopology->mst_nresource = 0;

	//
	// Find what PSM cannot fill
	//
	for (index = 0; index < os_default_topology->mst_nresource; index++ ) {
	    if ((os_default_topology->mst_resources[index].msr_type != MSR_CPU)
	    && (os_default_topology->mst_resources[index].msr_type != MSR_BUS))
		CbusMsopTopology->mst_nresource++;
	}

	//
	// Add CPUS and bus
	//
	CbusMsopTopology->mst_nresource += CbusNumCpus + 1;

	resourcep = CbusMsopTopology->mst_resources = (ms_resource_t *)
	    os_alloc(CbusMsopTopology->mst_nresource * sizeof(ms_resource_t));
	
	//
	// initialize resources.  fill resource table for each CPU.
	//
	for (index = 0; index < CbusNumCpus; index++, resourcep++) {
		resourcep->msr_cgnum = 0;
		resourcep->msr_private = MS_FALSE;
		resourcep->msr_private_cpu = MS_CPU_ANY;
		resourcep->msr_type = MSR_CPU;
		resourcep->msri.msr_cpu.msr_clockspeed = 0;
		resourcep->msri.msr_cpu.msr_cpuid = index;
	}

	//
	// fill bus description.  there is no routing information.
	//
	resourcep->msr_cgnum = 0;
	resourcep->msr_private = MS_FALSE;
	resourcep->msr_private_cpu = MS_CPU_ANY;
	resourcep->msr_type = MSR_BUS;
	resourcep->msri.msr_bus.msr_bus_type = CbusBusType;
	resourcep->msri.msr_bus.msr_bus_number = 0;
	resourcep->msri.msr_bus.msr_intr_routing = (msr_routing_t *) 0;
	resourcep->msri.msr_bus.msr_n_routing = 0;
	resourcep++;

	//
	// Now merge with the os_default_topology
	//
	for (index = 0; index < os_default_topology->mst_nresource; index++ ) {
	    if ((os_default_topology->mst_resources[index].msr_type != MSR_CPU)
	    && (os_default_topology->mst_resources[index].msr_type != MSR_BUS))
		*resourcep++ = os_default_topology->mst_resources[index];
	}

	CbusMsopInitMsparam();

	CbusMsopAttachLock = os_mutex_alloc();

	psm_time_spin_init();

	return MS_TRUE;
}

/*++

Routine Description:

	initialize the interrupt control system.  at this point,
	all interrupts are disabled, since none have been attached.
	called when the system is being initialized.  this function
	must be called with the IE flag off.

Arguments:

	none.

Return Value:

	none.

--*/
STATIC void
CbusMsopInitCpu(
	void
)
{
	ULONG	index;

	PSM_ASSERT(!is_intr_enabled());

	if (CbusMsopBootCpu == MS_CPU_ANY) {

		//
		// save the cpu number of this cpu - it will handle global clocks.
		// note that this is also a 'first_time' flag so this code is
		// executed only on the boot CPU.
		//
		CbusMsopBootCpu = os_this_cpu;

		//
		// assign all vectors to CbusMsopServiceStray().  then assign the ones
		// we really use to the appropriate routines.  this allows us to return
		// the right idtp for unused entries.
		//
		os_claim_vectors(CBUS_FIRST_VECTOR,
			CBUS_LAST_VECTOR - CBUS_FIRST_VECTOR,
			CbusMsopServiceStray);
		os_claim_vectors(CBUS_FIRST_VECTOR,
			CBUS_TIMER_VECTOR - CBUS_FIRST_VECTOR,
			CbusMsopServiceInterrupt);
		os_claim_vectors(CBUS_TIMER_VECTOR, 1, CbusMsopServiceTimer);
		os_claim_vectors(CbusXintVector, 1, CbusMsopServiceXint);
		os_claim_vectors(CbusSpuriousVector, 1, CbusMsopServiceSpurious);
	}
	(*CbusFunctions.InitializeCpu)(os_this_cpu);
}

/*++

Routine Description:

	attach the specified interrupt source and enable distribution
	of interrupts for this slot in the interrupt controller.
	called when the system is being initialized.  this function
	must be called with the IE flag off.

Arguments:

	idtp	- pointer to interrupt structure for this slot.

Return Value:

	MS_TRUE if successful, MS_FALSE if not.

--*/
STATIC ms_bool_t
CbusMsopIntrAttach(
	ms_intr_dist_t *	idtp
)
{
	ms_cpu_t			cpu;
	ms_islot_t			slot;
	ms_lockstate_t		lockState;

#ifdef DEBUG
	os_printf("CbusMsopIntrAttach: idtp=%x cpu=%x slot=%x\n",
		idtp, CbusMsopBootCpu, idtp->msi_slot);
#endif

	PSM_ASSERT(!is_intr_enabled());

	//
	// the boot CPU receives all the interrupts.
	//
	cpu = CbusMsopBootCpu;
	slot = idtp->msi_slot;

	//
	// validate the slot.
	//
	if ((slot > CbusIslotMax) || (slot == CBUS_CASC_SLOT) ||
	    (slot == CBUS_TIMER_SLOT)) {
			return MS_FALSE;
	}

	//
	// set the PSM-specific field in the interrupt distributed
	// structure to be the vector for this interrupt slot.
	//
	idtp->msi_mspec = (void *)(CBUS_FIRST_VECTOR + slot);

	//
	// detach the slot before potentially reassigning it.
	//
	CbusMsopIntrDetach(idtp);

	lockState = os_mutex_lock(CbusMsopAttachLock);

	//
	// save the idtp for interrupt service routines.
	//
	CbusMsopIntrp[(int)idtp->msi_mspec] = idtp;

	idtp->msi_flags |= MSI_ORDERED_COMPLETES;
	idtp->msi_flags &= ~MSI_NONMASKABLE;

	//
	// call back-end to enable the interrupt.
	//
	(*CbusFunctions.EnableInterrupt)(cpu, slot, idtp->msi_mspec);

	os_mutex_unlock(CbusMsopAttachLock, lockState);

#ifdef DEBUG
	os_printf("CbusMsopIntrAttach: returning\n");
#endif

	return MS_TRUE;
}

/*++

Routine Description:

	detach the specified interrupt source in the interrupt controller.
	after this function returns, there should be no more interrupt
	deliveries using the specified interrupt distributed structure pointer.

Arguments:

	idtp	- pointer to interrupt structure for this slot.

Return Value:

	none.

--*/
STATIC void
CbusMsopIntrDetach(
	ms_intr_dist_t *	idtp
)
{
	ms_cpu_t			cpu;
	ms_islot_t			slot;
	ms_ivec_t			vector;
	ms_lockstate_t		lockState;

#ifdef DEBUG
	os_printf("CbusMsopIntrDetach: idtp=%x cpu=%x slot=%x vector=%x\n",
		idtp, CbusMsopBootCpu, idtp->msi_slot, idtp->msi_mspec);
#endif

	PSM_ASSERT(!is_intr_enabled());

	//
	// the boot CPU receives all the interrupts.
	//
	cpu = CbusMsopBootCpu;
	slot = idtp->msi_slot;
	vector = (ms_ivec_t)idtp->msi_mspec;

	//
	// validate the slot.
	//
	if ((slot > CbusIslotMax) || (slot == CBUS_CASC_SLOT) ||
	    (slot == CBUS_TIMER_SLOT) ||
	    CbusMsopIntrp[vector] == os_intr_dist_stray) {
			return;
	}

	lockState = os_mutex_lock(CbusMsopAttachLock);

	(*CbusFunctions.DisableInterrupt)(cpu, slot, vector);

	CbusMsopIntrp[vector] = os_intr_dist_stray;

	os_mutex_unlock(CbusMsopAttachLock, lockState);

#ifdef DEBUG
	os_printf("CbusMsopIntrDetach: returning\n");
#endif
}

/*++

Routine Description:

	mask off interrupt requests coming in on the specified islot
	request line on the specified CPU.  for simplicity sake,
	CbusMsopIntrDetach() works.

Arguments:

	idtp	- pointer to interrupt structure for this slot.

Return Value:

	none.

--*/
STATIC void
CbusMsopIntrMask(
	ms_intr_dist_t *	idtp
)
{
	PSM_ASSERT(!is_intr_enabled());

	CbusMsopIntrDetach(idtp);
}

/*++

Routine Description:

	unmask interrupt requests coming in on the specified islot
	request line on the specified CPU.  for simplicity sake,
	CbusMsopIntrAttach() works.

Arguments:

	idtp	- pointer to interrupt structure for this slot.

Return Value:

	none.

--*/
STATIC void
CbusMsopIntrUnmask(
	ms_intr_dist_t *	idtp
)
{
	PSM_ASSERT(!is_intr_enabled());

	CbusMsopIntrAttach(idtp);
}

/*++

Routine Description:

	call the back-end routine to perform an EOI.

Arguments:

	idtp	- pointer to interrupt structure for this slot.

Return Value:

	none.

--*/
STATIC void
CbusMsopIntrComplete(
	ms_intr_dist_t *	idtp
)
{
	ms_cpu_t			cpu;
	ms_islot_t			slot;
	ms_ivec_t			vector;
	ms_lockstate_t		lockState;

	PSM_ASSERT(!is_intr_enabled());

	//
	// the boot CPU receives all the interrupts.
	//
	cpu = CbusMsopBootCpu;
	slot = idtp->msi_slot;
	vector = (ms_ivec_t)idtp->msi_mspec;

	(*CbusFunctions.EoiInterrupt)(cpu, slot, vector);
}

/*++

Routine Description:

	call the back-end routine to perform a cross-processor interrupt.
	called with interrupts disabled on this CPU.

Arguments:

	cpu			- this is either a specific cpu to interrupt or
				  MS_CPU_ALL_BUT_ME.

	eventMask	- this is a bitmask of one or more OS-specific event flags
				  MS_EVENT_OS_1 through MS_EVENT_OS_4.

Return Value:

	none.

--*/
STATIC void
CbusMsopXpost(
	ms_cpu_t	cpu,
	ms_event_t	eventMask
)
{
	PSM_ASSERT(!is_intr_enabled());

	if (cpu != MS_CPU_ALL_BUT_ME) {
		atomic_or(&CbusMsopCpuInfo[cpu].EventFlags, eventMask);
		(*CbusFunctions.IpiCpu)(cpu);
	}
	else {
		for (cpu = 0; cpu < CbusNumCpus; cpu++) {
			if (cpu != os_this_cpu) {
			    atomic_or(&CbusMsopCpuInfo[cpu].EventFlags, eventMask);
			    (*CbusFunctions.IpiCpu)(cpu);
			}
		}
	}
}

/*++

Routine Description:

	enable or disable the second heartbeat clock tick interrupt.

Arguments:

	time	- tick period for secondary tick events.

Return Value:

	none.

--*/
STATIC void
CbusMsopTick2(
	ms_time_t	time
)
{
	PSM_ASSERT(!is_intr_enabled());

	if (time.mst_sec || time.mst_nsec) {
		PSM_ASSERT(time.mst_sec == os_tick_period.mst_sec && 
			time.mst_nsec == os_tick_period.mst_nsec);
		CbusMsopClockEvent = MS_EVENT_TICK_1 | MS_EVENT_TICK_2;
	}
	else {
		CbusMsopClockEvent = MS_EVENT_TICK_1;
	}
}

/*++

Routine Description:

	get the current value of a free-running, high-resolution clock
	counter in PSM-specific units.  this counter should not wrap
	around in less that 1 hour.

Arguments:

	none.

Return Value:

	gtime	- current time stamp that is opaque to the base kernel.

--*/
STATIC void
CbusMsopTimeGet(
	ms_rawtime_t *	gtime
)
{
	*gtime = (*CbusFunctions.TimeGet)();
}

/*++

Routine Description:

	add two PSM-specific high-resolution time values.

Arguments:

	dst	- first high-resolution timer value.

	src	- second high-resolution timer value.

Return Value:

	dst	- result of the addition.

--*/
STATIC void
CbusMsopTimeAdd(
	ms_rawtime_t *	dst,
	ms_rawtime_t *	src
)
{
	LONG			lo;

	dst->msrt_hi += src->msrt_hi;
	lo = dst->msrt_lo + src->msrt_lo;
	if (lo >= CBUS_NANOSECONDS_IN_SECOND) {
		lo -= CBUS_NANOSECONDS_IN_SECOND;
		dst->msrt_hi++;
	}
	dst->msrt_lo = lo;
}

/*++

Routine Description:

	subtract two PSM-specific high-resolution time values.

Arguments:

	dst	- first high-resolution timer value.

	src	- second high-resolution timer value.

Return Value:

	dst	- result of the dst - src.

--*/
STATIC void
CbusMsopTimeSub(
	ms_rawtime_t *	dst,
	ms_rawtime_t *	src
)
{
	LONG			lo;

	dst->msrt_hi -= src->msrt_hi;
	lo = (LONG)dst->msrt_lo - (LONG)src->msrt_lo;
	if (lo < 0) {
		lo += CBUS_NANOSECONDS_IN_SECOND;
		dst->msrt_hi--;
	}
	dst->msrt_lo = (ULONG)lo;
}

/*++

Routine Description:

	convert a PSM-specific high-resolution time value to seconds
	and nanoseconds.

Arguments:

	src	- time value to convert.

Return Value:

	dst	- converted time value.

--*/
STATIC void
CbusMsopTimeCvt(
	ms_time_t *		dst,
	ms_rawtime_t *	src
)
{
	dst->mst_sec = src->msrt_hi;
	dst->mst_nsec = src->msrt_lo;
}

/*++

Routine Description:

	called on a CPU which is otherwise idle.  this function should not return
	until MSOP_IDLE_EXIT has been called (from another CPU).  note the call
	to MSOP_IDLE_EXIT may happen before, or at any point during,
	MSOP_IDLE_SELF.  in this case, MSOP_IDLE_SELF must return right away
	rather than waiting for another MSOP_IDLE_EXIT.

Arguments:

	none.

Return Value:

	none.

--*/
STATIC void
CbusMsopIdleSelf(
	void
)
{
	ULONG		statusFlags;

	PSM_ASSERT(is_intr_enabled());

	(*CbusFunctions.PokeLed)(os_this_cpu, MS_FALSE);

	statusFlags = intr_disable();

	while(CbusMsopCpuInfo[os_this_cpu].IdleFlag == MS_FALSE) {
		asm("sti");
		asm("hlt");
		statusFlags = intr_disable();
	}

	intr_restore(statusFlags);

	(*CbusFunctions.PokeLed)(os_this_cpu, MS_TRUE);

	CbusMsopCpuInfo[os_this_cpu].IdleFlag = MS_FALSE;
}

/*++

Routine Description:

	called to bring the specified CPU out of idle state
	(i.e. waiting in MSOP_IDLE_SELF).  this should cause
	MSOP_IDLE_SELF to exit as soon as possible.

Arguments:

	cpu	- CPU to bring out of idle.

Return Value:

	none.

--*/
STATIC void
CbusMsopIdleExit(
	ms_cpu_t	cpu
)
{
	CbusMsopCpuInfo[cpu].IdleFlag = 1;

	if (cpu != os_this_cpu)
		CbusMsopXpost (cpu, MS_EVENT_PSM_2);

}

/*++

Routine Description:

	start the specified CPU running kernel code at the indicated address.

Arguments:

	cpu			- CPU to start running.

	resetCode	- physical kernel start up address.

Return Value:

	none.

--*/
STATIC void
CbusMsopStartCpu(
	ms_cpu_t	cpu,
	ms_paddr_t	resetCode
)
{
	typedef struct _reset_addr_t {
		USHORT	offset;
		USHORT	segment;
	} START_ADDR;
	START_ADDR	startAddr;
	PUCHAR		vector;
	PUCHAR		source;
	ULONG		statusFlags;
	ULONG		index;

	PSM_ASSERT(is_intr_enabled());

	while (CbusMsopCpuInfo[cpu].CpuOnLine == MS_TRUE) {
		//
		// wait for the CPU to finish CbusMsopOffLineSelf().
		//
		;
	}

	//
	// give it some time.
	//
	psm_time_spin(1000);

	statusFlags = intr_disable();

	//
	// historically, we have never temporarily switched
	// CMOS to WARM_RESET.
	//

	//
	// get the real address segment:offset format.
	//
	startAddr.offset = resetCode & 0x0f;
	startAddr.segment = (resetCode >> 4) & 0xFFFF;

	//
	// now put the address into warm reset vector (40:67).
	//
	vector = (PUCHAR)((ULONG)os_page0 + CBUS_RESET_VECTOR);

	//
	// copy byte-by-byte since the reset vector port is
	// not word aligned.
	//
	source = (PUCHAR)&startAddr;
	for (index = 0; index < sizeof(startAddr); index++) {
		*vector++ = *source++;
	}

	(*CbusFunctions.StartCpu)(cpu);

	CbusMsopCpuInfo[cpu].CpuOnLine = MS_TRUE;

	intr_restore(statusFlags);
}

/*++

Routine Description:

	called on a CPU which is about to be brought offline.  this function
	gives the PSM a chance to redistribute any MS_CPU_ANY interrupts
	which may have been assigned to this CPU by the PSM.

Arguments:

	none.

Return Value:

	none.

--*/
STATIC void
CbusMsopOffLinePrep(
	void
)
{
	(*CbusFunctions.PokeLed)(os_this_cpu, MS_FALSE);
	(*CbusFunctions.OffLinePrep)(os_this_cpu);
}

/*++

Routine Description:

	called on a CPU which is being brought offline.  all OS-specific offline
	processing will already have been done.  the CPU is no longer expected
	to field interrupts and halts (either here or in the back-end).

	called with interrupts disabled at the CPU.

Arguments:

	none.

Return Value:

	none.

--*/
STATIC void
CbusMsopOffLineSelf(
	void
)
{
	PSM_ASSERT(!is_intr_enabled());

	CbusMsopCpuInfo[os_this_cpu].CpuOnLine = MS_FALSE;

	//
	// the call to this back-end function may not return.
	//
	(*CbusFunctions.OffLineSelf)();

	for (;;)
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
CbusMsopShowState(
	void
)
{
	(*CbusFunctions.ShowState)();
}

/*++

Routine Description:

	shutdown and/or reboot the system.  this operation handles
	low-level machine state.  any software state involved in
	shutdown/reboot will have been taken care of by the core OS.

Arguments:

	action	- MS_SD_HALT
				put the system in a non-interactive halted state in
				which no s/w is active.  from this state, a user may
				reboot or shut off the power.

			  MS_SD_POWEROFF
				shut off the power to the system.  this is like an
				automatic MS_SD_HALT and power-off.  this is implemented
				exactly the same as MS_SD_HALT.

			  MS_SD_AUTOBOOT
				with no interaction, reboot the system.

			  MS_SD_BOOTPROMPT
				reboot to an interactive boot prompt from which the user
				can specify an alternative kernel to boot.

Return Value:

	none.

--*/
STATIC void
CbusMsopShutdown(
	ULONG	action
)
{
	PSM_ASSERT(!is_intr_enabled());

	CbusMsopOffLinePrep();

	psm_softreset(os_page0);

	switch (action) {

	case MS_SD_HALT:
	case MS_SD_POWEROFF:
		break;

	case MS_SD_AUTOBOOT:
	case MS_SD_BOOTPROMPT:
		psm_sysreset();
		break;

	}

	for (;;) {
		asm("   hlt     ");
	}
}

/*++

Routine Description:

	initialize MSPARAM parameters.

Arguments:

	none.

Return Value:

	none.

--*/
STATIC void
CbusMsopInitMsparam(
	void
)
{
	os_set_msparam(MSPARAM_PLATFORM_NAME, &CbusPlatformName);
	os_set_msparam(MSPARAM_SW_SYSDUMP, &CbusSwSysdump);
	os_set_msparam(MSPARAM_TIME_RES, &CbusTimeRes);
	os_set_msparam(MSPARAM_TICK_1_RES, &CbusTick1Res);
	os_set_msparam(MSPARAM_TICK_1_MAX, &CbusTick1Max);
	os_set_msparam(MSPARAM_ISLOT_MAX, &CbusIslotMax);
	os_set_msparam(MSPARAM_SHUTDOWN_CAPS, &CbusShutdownCaps);
	os_set_msparam(MSPARAM_TOPOLOGY, CbusMsopTopology);
}

/*++

Routine Description:

	general interrupt service entry point.
	processing here would be for pre- interrupt service routines.

Arguments:

	vector	- vector that generated the interrupt.

Return Value:

	the ISR for this vector.

--*/
STATIC ms_intr_dist_t *
CbusMsopServiceInterrupt(
	ms_ivec_t	vector
)
{
	PSM_ASSERT(!is_intr_enabled());
	PSM_ASSERT(vector >= CBUS_FIRST_VECTOR && vector < CBUS_TIMER_VECTOR);

	return CbusMsopIntrp[vector];
}

/*++

Routine Description:

	timer interrupt service entry point.
	post the events and distribute the timer interrupt
	to all the CPUs, other than the BSP.
	call the back-end to manage the PSM-specific,
	high-resolution counter and EOI the interrupt.

Arguments:

	vector	- vector that generated the timer interrupt.

Return Value:

	os_intr_dist_nop.

--*/
STATIC ms_intr_dist_t *
CbusMsopServiceTimer(
	ms_ivec_t	vector
)
{
	PSM_ASSERT(!is_intr_enabled());
	PSM_ASSERT(vector == CBUS_TIMER_VECTOR);

	if ((*CbusFunctions.ServiceTimer)(os_this_cpu,
	     CBUS_TIMER_SLOT, vector) == MS_TRUE) {
			//
			// this interrupt is spurious, don't perform
			// system accounting.
			//
			return os_intr_dist_nop;
	}

	os_post_events(CbusMsopClockEvent);

	//
	// distribute the clock interrupt to all CPUs
	// other than the boot CPU.
	//
	if (os_this_cpu == CbusMsopBootCpu) {
		CbusMsopXpost(MS_CPU_ALL_BUT_ME, MS_EVENT_PSM_1);
	}

	return os_intr_dist_nop;
}

/*++

Routine Description:

	service a cross-processor interrupt.

Arguments:

	vector	- vector that generated the cross-processor interrupt.

Return Value:

	os_intr_dist_nop.

--*/
STATIC ms_intr_dist_t *
CbusMsopServiceXint(
	ms_ivec_t	vector
)
{
	ms_event_t	eventFlags;

	PSM_ASSERT(!is_intr_enabled());
	PSM_ASSERT(vector == CbusXintVector);

	(*CbusFunctions.ServiceXint)(os_this_cpu,
		vector - CBUS_FIRST_VECTOR, vector);

	eventFlags = (ms_event_t)
		atomic_fnc(&CbusMsopCpuInfo[os_this_cpu].EventFlags);

	//
	// clock propagation.
	//
	if (eventFlags & MS_EVENT_PSM_1) {
		eventFlags &= ~MS_EVENT_PSM_1;
		os_post_events(CbusMsopClockEvent);
	}

	eventFlags &= ~MS_EVENT_PSM_2;

	os_post_events(eventFlags);

	return os_intr_dist_nop;
}

/*++

Routine Description:

	service a spurious interrupt.

Arguments:

	vector	- vector that generated the spurious interrupt.

Return Value:

	os_intr_dist_nop.

--*/
STATIC ms_intr_dist_t *
CbusMsopServiceSpurious(
	ms_ivec_t	vector
)
{
	PSM_ASSERT(vector == CbusSpuriousVector);

	(*CbusFunctions.ServiceSpurious)(os_this_cpu,
		vector - CBUS_FIRST_VECTOR, vector);

	return os_intr_dist_nop;
}

/*++

Routine Description:

	service a stray interrupt that is currently unused and unassigned.

Arguments:

	vector	- vector that generated the stray interrupt.

Return Value:

	os_intr_dist_stray.

--*/
STATIC ms_intr_dist_t *
CbusMsopServiceStray(
	ms_ivec_t vector
)
{
	PSM_ASSERT(CbusMsopIntrp[vector] == os_intr_dist_stray);

	os_intr_dist_stray->msi_slot = 0xffff;

	return os_intr_dist_stray;
}
