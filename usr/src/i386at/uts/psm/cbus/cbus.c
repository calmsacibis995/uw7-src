#ident	"@(#)kern-i386at:psm/cbus/cbus.c	1.16.2.1"
#ident	"$Header$"

/*++

Copyright (c) 1992-1997  Corollary Inc.

Module Name:

    cbus.c

Abstract:

	this module implements functions that comprise the initial back-end
	that are called by the MSOPs.  these functions are responsible for
	identifying the underlying C-bus platform and providing enough
	interface for the MSOP routines to properly set up the C-bus hardware.
	this MSOP back-end, in turn, calls hardware-specific back-ends to
	perform the necessary operations.

Function Prefix:

	functions in this routine are prefixed with Cbus.

--*/

#include <cbus_includes.h>

STATIC ULONG	RrdSignature[] = { 0xdeadbeef, 0 };

STATIC CHAR		CorollaryCopyright[] = 
					   "Copyright(C) Corollary, Inc. 1991. All Rights Reserved";

//
// functions defined in this module.
//
ms_bool_t					CbusPresent(void);
void						CbusGetConfiguration(PUCHAR, PUCHAR);
void						CbusDefaultRegisters(void);
void						CbusReadExtIds(PCBUS_EXT_ID_INFO);
void						CbusHandleRrdOverride(PCBUS_EXT_CFG_OVERRIDE,ULONG);
PUCHAR						CbusFindString(PUCHAR, PUCHAR, ULONG);
void						CbusI486CacheOn(void);
void						CbusI486CacheOff(void);
void						CbusSetup(void);
void						CbusInitializeCpu(ms_cpu_t);
void						CbusEnableInterrupt(ms_cpu_t,ms_islot_t,ms_ivec_t);
void						CbusDisableInterrupt(ms_cpu_t,ms_islot_t,ms_ivec_t);
void						CbusEoiInterrupt(ms_cpu_t,ms_islot_t,ms_ivec_t);
void						CbusIpiCpu(ms_cpu_t);
ms_rawtime_t				CbusTimeGet(void);
void						CbusStartCpu(ms_cpu_t);
void						CbusOffLinePrep(ms_cpu_t);
void						CbusOffLineSelf(void);
void						CbusShowState(void);
ms_bool_t					CbusServiceTimer(ms_cpu_t, ms_islot_t, ms_ivec_t);
void						CbusServiceXint(ms_cpu_t, ms_islot_t, ms_ivec_t);
void						CbusServiceSpurious(ms_cpu_t,ms_islot_t,ms_ivec_t);
void						CbusPokeLed(ms_cpu_t, ULONG);

//
// C-bus back-end functions that are called
// by the C-bus MSOP routines.
//
CBUS_FUNCTIONS_T CbusFunctions = {
	CbusPresent,
	CbusInitializeCpu,
	CbusEnableInterrupt,
	CbusDisableInterrupt,
	CbusEoiInterrupt,
	CbusIpiCpu,
	CbusTimeGet,
	CbusStartCpu,
	CbusOffLinePrep,
	CbusOffLineSelf,
	CbusShowState,
	CbusServiceTimer,
	CbusServiceXint,
	CbusServiceSpurious,
	CbusPokeLed
};

/*++

Routine Description:

	check if this is a Corollary architecture.
	if so, parse the RRD configuration tables and
	set up the platform.

Arguments:

	none.

Return Value:

	MS_TRUE if it is a Corollary architecture, MS_FALSE otherwise.

--*/
STATIC ms_bool_t
CbusPresent(
	void
)
{
	PUCHAR	rrdSignaturePtr = NULL;
	PUCHAR	corollaryCopyrightPtr = NULL;
	PUCHAR	vaddrBuffer;
	ULONG	index;

#ifdef DEBUG
	os_printf("CbusPresent:\n");
#endif
	vaddrBuffer = (PUCHAR)os_physmap(CBUS_RRD_RAM, CBUS_RRD_RAM_SIZE);

	rrdSignaturePtr = CbusFindString(vaddrBuffer,
		(PUCHAR)RrdSignature, CBUS_RRD_RAM_SIZE);

	corollaryCopyrightPtr = CbusFindString(vaddrBuffer,
		(PUCHAR)CorollaryCopyright, CBUS_RRD_RAM_SIZE);

	//
	// only the RRD signature is absolutely required.
	// older versions of the RRD don't have the copyright string.
	//
	if (rrdSignaturePtr == NULL) {
		//
		// free mapping resource.
		//
		os_physmap_free(vaddrBuffer, CBUS_RRD_RAM_SIZE);
		return MS_FALSE;
	}

	//
	// read the RRD configuration tables into memory.
	//
	CbusGetConfiguration(corollaryCopyrightPtr, rrdSignaturePtr);

	//
	// for each unique Corollary architecture, call the back-end
	// present function to determine whether the platform matches
	// that architecture.
	//
	for (index = 0; index < CbusNumberSwitches; index++) {
		if ((*CbusSwitch[index]->Present)()) {
			CbusSwitchp = CbusSwitch[index];
			//
			// initialize bridges and interrupts.
			// also, initialize IPL translation table.
			//
			(*CbusSwitchp->ParseRrd)(CbusExtIdInfo, CbusValidIds);

			//
			// initialize the platform.
			//
			CbusSetup();

#if 0
			//
			// free mapping resource.
			//
			os_physmap_free(vaddrBuffer, CBUS_RRD_RAM_SIZE);
#endif

#ifdef DEBUG
			os_printf("CbusPresent: done\n");
#endif
			return MS_TRUE;
		}
	}
	
	//
	// free mapping resource.
	//
	os_physmap_free(vaddrBuffer, CBUS_RRD_RAM_SIZE);

	//
	// not a Corollary architecture after all.
	//
	return MS_FALSE;
}

/*++

Routine Description:

	all Corollary architectures, regardless of RRD configuration table
	differences, call this routine.  careful attention is taken to ensure
	backwards compatibility.

Arguments:

	none.

Return Value:

	none.

--*/
STATIC void
CbusGetConfiguration(
	PUCHAR							corollaryCopyrightPtr,
	PUCHAR							rrdSignaturePtr
)
{
	PCBUS_PROCESSOR_CONFIGURATION	pcPtr;
	PCBUS_OEM_ROM_INFORMATION		oemPtr;
	PCBUS_EXT_MEMORY_BOARD			memPtr;
	PCBUS_EXT_CFG_HEADER			headerPtr;
	ULONG							overrideLength = 0;
	PUCHAR							sourcePtr;
	ULONG							index;

	//
	// set up some of the CSRs to default values.
	// they are overriden later as needed.
	//
	CbusDefaultRegisters();

#ifdef DEBUG
	os_printf("CbusGetConfiguration:\n");
#endif

	//
	// current C-bus standard system parameters;
	// initialize them here so they are overridden later
	// as needed.
	//
	CbusGlobal.UseHoles = 1;
	CbusGlobal.BootId = CBUS_BRIDGE_ID;
	CbusGlobal.CbusIo = CBUS_IO;

	//
	// baseram defines where the kernel is mapping
	// the base of physical memory.  for the
	// multiprocessor kernel, all physical memory is
	// mapped at 64Mb; for the uniprocessor kernels,
	// all physical memory used to be mapped relative
	// to zero, but is now also at 64Mb.
	//
	CbusGlobal.BaseRam = CBUS_MEM;
	CbusGlobal.ResetVec = CbusGlobal.BaseRam + CBUS_MEM;
	CbusGlobal.MemoryCeiling = CbusGlobal.BaseRam + CBUS_MEM;

	//
	// read in the memory configuration structure.
	//
	sourcePtr = rrdSignaturePtr;

	bcopy(sourcePtr, (PUCHAR)&CbusConfiguration, sizeof(CbusConfiguration));

	//
	// read in the CPU configuration structure.
	// this information used to be held in the configuration
	// structure, but now it is in the "extended configuration"
	// structure which follows the configuration structure.
	// multiple structures are strung together with a "checkword",
	// "length", and "data" structure.  the first null "checkword"
	// entry marks the end of the extended configuration 
	// structure.
	//
	sourcePtr += sizeof(CbusConfiguration);
	headerPtr = (PCBUS_EXT_CFG_HEADER)sourcePtr;

	pcPtr = CbusHardwareInfo.ProcessorConfiguration;
	memPtr = CbusHardwareInfo.ExtMemBoard;

	//
	// default to RRD ROM version 1.50 for compatibility.
	//
	oemPtr = &CbusHardwareInfo.OemRomInfo;
	oemPtr->OemNumber = CTAB_OEM_COROLLARY;
	oemPtr->OemRomVersion = 1;
	oemPtr->OemRomRelease = 50;
	oemPtr->OemRomRevision = 0;

	// 
	// RRD ROM versions from 1.50 and above use the extended configuration
	// information structure, denoted by CTAB_EXT_CHECKWORD.
	//
	if (*(unsigned *)sourcePtr == CTAB_EXT_CHECKWORD) {
		do {
			sourcePtr += sizeof(CBUS_EXT_CFG_HEADER_T);

			switch (headerPtr->ExtCfgCheckword) {

			case CTAB_EXT_CHECKWORD:
				bcopy(sourcePtr, (char *)pcPtr, headerPtr->ExtCfgLength);
				break;

			case CTAB_EXT_VENDOR_INFO:
				bcopy(sourcePtr, (char *)oemPtr, headerPtr->ExtCfgLength);
				break;

			case CTAB_EXT_MEM_BOARD:
				bcopy(sourcePtr, (char *)memPtr, headerPtr->ExtCfgLength);
				break;

			case CTAB_EXT_CFG_OVERRIDE:
				if (!corollaryCopyrightPtr)
					break;

				//
				// we just copy the size of the structures
				// we know about.  if an RRD tries to pass us
				// more than we know about, we ignore the
				// overflow.
				//
				overrideLength = MIN(sizeof(CBUS_EXT_CFG_OVERRIDE_T), 
					headerPtr->ExtCfgLength);
				if (overrideLength) {
					CbusHandleRrdOverride((PCBUS_EXT_CFG_OVERRIDE)sourcePtr,
						overrideLength);
				}
				break;

			case CTAB_EXT_ID_INFO:
				if (!corollaryCopyrightPtr) {
					break;
				}
				CbusReadExtIds((PCBUS_EXT_ID_INFO)sourcePtr);
				break;

			case CTAB_EXT_CFG_END:
				break;

			default:

				//
				// skip unrecognized configuration entries.
				//
				break;
			}
			sourcePtr += headerPtr->ExtCfgLength;
			headerPtr = (PCBUS_EXT_CFG_HEADER)sourcePtr;

		} while (headerPtr->ExtCfgCheckword != CTAB_EXT_CFG_END);
	}
	else {
		// 
		// if there is no extended configuration structure,
		// this must be an old RRD ROM.  set up the CPU
		// configuration structure to look the same.  this
		// code is here to support RRD releases prior to 1.50.
		//
		oemPtr->OemRomRelease = 49;

		pcPtr += CBUS_LOWCPUID;

		for (index = CBUS_LOWCPUID; index < CBUS_HICPUID; index++, pcPtr++) {
			switch(CbusConfiguration.Slot[index]) {
			case CTAB_ATSIO386:
				pcPtr->ProcType = CTAB_PT_386;
				pcPtr->IoFunction = CTAB_IOF_SIO;
				break;
			case CTAB_ATSCSI386:
				pcPtr->ProcType = CTAB_PT_386;
				pcPtr->IoFunction = CTAB_IOF_SCSI;
				break;
			case CTAB_ATSIO486:
				pcPtr->ProcType = CTAB_PT_486;
				pcPtr->ProcAttr = CTAB_PA_CACHE_OFF;
				pcPtr->IoFunction = CTAB_IOF_SIO;
				break;
			case CTAB_ATSIO486C:
				pcPtr->ProcType = CTAB_PT_486;
				pcPtr->ProcAttr = CTAB_PA_CACHE_ON;
				pcPtr->IoFunction = CTAB_IOF_SIO;
				break;
			case CTAB_ATBASE386:
				pcPtr->ProcType = CTAB_PT_386;
				pcPtr->IoFunction = CTAB_IOF_ISA_BRIDGE;
				break;
			case CTAB_ATBASE486:
				pcPtr->ProcType = CTAB_PT_486;
				pcPtr->ProcAttr = CTAB_PA_CACHE_OFF;
				pcPtr->IoFunction = CTAB_IOF_ISA_BRIDGE;
				break;
			case CTAB_ATBASE486C:
				pcPtr->ProcType = CTAB_PT_486;
				pcPtr->ProcAttr = CTAB_PA_CACHE_ON;
				pcPtr->IoFunction = CTAB_IOF_ISA_BRIDGE;
				break;
			case CTAB_ATP2486C:
				pcPtr->ProcType = CTAB_PT_486;
				pcPtr->ProcAttr = CTAB_PA_CACHE_ON;
				pcPtr->IoFunction = CTAB_IOF_P2;
				break;
			default:
				break;
			}
		}
	}

	//
	// determine if this C-bus two-board set is EISA.
	//
	pcPtr = &CbusHardwareInfo.ProcessorConfiguration[CbusGlobal.BootId];

	if (pcPtr->IoFunction == CTAB_IOF_EISA_BRIDGE)
		CbusVendorClass = CTAB_CRLLRYEISA;
}

/*++

Routine Description:

    initialize multiprocessor control register
    offsets to standard Corollary defaults -
    called from CbusPresent() routines when first
    identifying a platform.

Arguments:

	none.

Return Value:

	none.

--*/
STATIC void
CbusDefaultRegisters(
    void
)
{
	CbusGlobal.CReset = CBUS_CRESET;
	CbusGlobal.SReset = CBUS_SRESET;
	CbusGlobal.Contend = CBUS_CONTEND;
	CbusGlobal.SetIda = CBUS_SETIDA;
	CbusGlobal.CSwi = CBUS_CSWI;
	CbusGlobal.SSwi = CBUS_SSWI;
	CbusGlobal.CNmi = CBUS_CNMI;
	CbusGlobal.SNmi = CBUS_SNMI;
	CbusGlobal.SLed = CBUS_SLED;
	CbusGlobal.CLed = CBUS_CLED;
	CbusGlobal.MachineType = CTAB_MACHINE_CBUS1;
}

/*++

Routine Description:

	read the extended id information into the global CbusExtIdInfo[] table.
	maintain the depth of the table with CbusValidIds.

	this information is set by some C-bus architectures and
	_ALL_ C-bus II platforms.

Arguments:

	extIdPtr	- pointer to the Extended ID Information structure
				  in the RRD RAM.

Return Value:

	none.

--*/
STATIC void
CbusReadExtIds(
	PCBUS_EXT_ID_INFO	extIdPtr
)
{
	ULONG				index;

	for (index = 0; index < CBUS_MAX_IDS && extIdPtr->Id != 0x7f;
		index++, extIdPtr++) {
			bcopy(extIdPtr, &CbusExtIdInfo[index], sizeof(CBUS_EXT_ID_INFO_T));
	}
	CbusValidIds = index;
}

/*++

Routine Description:

	set some of the newer fields of the RRD configuration tables
	for architectures that pre-dated the creation of these fields.

Arguments:

	extCfgPtr	- size of the configuration table to copy
				  from RRD RAM.

	length		- length to copy.  in some cases, length may
				  not represent the entire size of the table,
				  just the size known by that platform.

Return Value:

	none.

--*/
STATIC void
CbusHandleRrdOverride(
	PCBUS_EXT_CFG_OVERRIDE	extCfgPtr,
	ULONG					length
)
{
	//
	// it is possible that the RRD is older (ie: pre-XM),
	// and thus did not fill in any fields after CledVal.
	// meaning that we must assign defaults for the following:
	//		MachineType
	//		SupportedEnvironments
	//		BroadcastId
	// note that XM and C-bus II have an RRD which fills in
	// these 3 fields, and thus we set our defaults to C-bus.
	//
	CbusGlobal.MachineType = CTAB_MACHINE_CBUS1;
	CbusGlobal.SupportedEnvironments = 0;
	CbusGlobal.BroadcastId = CBUS_ALL_CPUID;

	bcopy(extCfgPtr, &CbusGlobal, length);
}

/*++

Routine Description:

	search a range of memory for a particular string.

Arguments:

	vaddrBuffer		- the virtually mapped range of memory to search.

	searchString	- the string to search.

	length			- the length of virtually mapped memory to search.

Return Value:

	the virtually mapped address where the strings was found or
	NULL if not found.

--*/
STATIC PUCHAR
CbusFindString(
	PUCHAR			vaddrBuffer,
	PUCHAR			searchString,
	ULONG			length
)
{
	ULONG			iIndex;
	ULONG			jIndex = 0;

#ifdef DEBUG
	os_printf("CbusFindString: vaddrBuffer=%x - ", vaddrBuffer);
#endif
	if (vaddrBuffer == NULL) {
		return NULL;
	}

	while (searchString[jIndex])  {
#ifdef DEBUG
		os_printf("[%x]", searchString[jIndex]);
#endif
		jIndex++;
	}
#ifdef DEBUG
		os_printf("\n", searchString[jIndex]);
#endif

	if (jIndex-- == 0) 
		return vaddrBuffer;

	length -= jIndex;

	for ( ; length; length--, vaddrBuffer++) {
		if (*vaddrBuffer != *searchString)
			continue;

		if (*(vaddrBuffer + jIndex) != *(searchString + jIndex))
			continue;

		for (iIndex = 1; iIndex < jIndex; iIndex++)
			if (*(vaddrBuffer + iIndex) != *(searchString + iIndex))
				break;

		if (iIndex >= jIndex) {
#ifdef DEBUG
			os_printf("CbusFindString: found @ %x\n", vaddrBuffer);
#endif
			return vaddrBuffer;
		}
	}

#ifdef DEBUG
	os_printf("CbusFindString: not found\n");
#endif
	return NULL;
}

/*++

Routine Description:

	enable the internal CPU cache.

Arguments:

	none.

Return Value:

	none.

--*/
STATIC void
CbusI486CacheOn(
	void
)
{
	asm(".set    CR0_CE, 0xbfffffff");
	asm(".set    CR0_WT, 0xdfffffff");
	asm("movl    %cr0, %eax");

	//
	// flush internal 486 cache.
	//
	asm(".byte	0x0f");		
	asm(".byte	0x09");

	asm("andl	$CR0_CE, %eax");
	asm("andl	$CR0_WT, %eax");

	//
	// flush queues.
	//
	asm("jmp	i486flush1");
	asm("i486flush1:");
	asm("movl    %eax, %cr0");
}

/*++

Routine Description:

	disable the internal CPU cache.

Arguments:

	none.

Return Value:

	none.

--*/
STATIC void
CbusI486CacheOff(
	void
)
{
	asm(".set    CR0_CD, 0x20000000");
	asm(".set    CR0_NW, 0x40000000");
	asm("movl    %cr0, %eax");

	//
	// flush internal 486 cache.
	//
	asm(".byte	0x0f");		
	asm(".byte	0x09");

	asm("orl	$CR0_CD, %eax");
	asm("orl	$CR0_NW, %eax");

	//
	// flush queues.
	//
	asm("jmp	i486flush2");
	asm("i486flush2:");
	asm("movl    %eax, %cr0");
}

/*++

Routine Description:

	call the back-end to initialize the underlying C-bus platform.

Arguments:

	none.

Return Value:

	none.

--*/
STATIC void
CbusSetup(
	void
)
{
#ifdef DEBUG
	ULONG	index;
#endif

	(*CbusSwitchp->Setup)();

#ifdef DEBUG
	//
	// display the CPUs found and which slot they are located.
	//
	for (index = 0; index <= CbusNumCpus; index++) {
		os_printf("CbusSetup: index=%x\n", index);
		if (CbusExtIdInfo[index].Id == CbusGlobal.BootId)
			continue;
#if 0
		os_printf("CbusSetup: found cpu=%x slot=%x\n",
			index, CbusCpuIds[index - 1]);
#endif
	}
	os_printf("CbusSetup: done\n");
#endif
}

/*++

Routine Description:

	call the back-end to initialize the CPU.  called on behalf
	of a CPU by the CPU itself.

Arguments:

	cpu	- CPU to initialize.

Return Value:

	none.

--*/
STATIC void
CbusInitializeCpu(
	ms_cpu_t						cpu
)
{
	PCBUS_PROCESSOR_CONFIGURATION	pcPtr;
	ULONG							statusFlags;

#ifdef DEBUG
	os_printf("CbusInitializeCpu: cpu=%x bootcpu=%x\n",
		cpu, CbusMsopBootCpu);
#endif
	if (cpu == CbusMsopBootCpu) {
		//
		// initialize the i8254 programmable interrupt timer.
		//
		i8254_init(&CbusMsopI8254Params, I8254_CTR0_PORT, os_tick_period);

		//
	 	// os_alloc(), called by i8254_init(), is forcing an sti.
		// we don't want interrupts enabled at this time.
	 	//
		statusFlags = intr_disable();
	}

	(void)(*CbusSwitchp->InitializeCpu)(cpu);

	//
	// check RRD table for whether to enable or disable
	// the internal CPU cache.
	//
	pcPtr = &CbusHardwareInfo.ProcessorConfiguration[cpu];
	if (pcPtr->ProcType == CTAB_PT_486) {
		if (pcPtr->ProcAttr == CTAB_PA_CACHE_ON) {
			CbusI486CacheOn();
		}
		else {
			CbusI486CacheOff();
		}
	}
#ifdef DEBUG
	os_printf("CbusInitializeCpu: done\n");
#endif
}

/*++

Routine Description:

	call the back-end to enable the interrupt.

Arguments:

	cpu		- CPU to enable the interrupt.

	islot	- interrupt slot to enable.

	vector	- vector to enable.

Return Value:

	none.

--*/
STATIC void
CbusEnableInterrupt(
	ms_cpu_t	cpu,
	ms_islot_t	islot,
	ms_ivec_t	vector
)
{
#ifdef DEBUG
	os_printf("CbusEnableInterrupt: cpu=%x islot=%x vector=%x\n",
		cpu, islot, vector);
#endif
	(*CbusSwitchp->EnableInterrupt)(cpu, islot, vector);
}

/*++

Routine Description:

	call the back-end to disable the interrupt.

Arguments:

	cpu		- CPU to disable the interrupt.

	islot	- interrupt slot to disable.

	vector	- vector to disable.

Return Value:

	none.

--*/
STATIC void
CbusDisableInterrupt(
	ms_cpu_t	cpu,
	ms_islot_t	islot,
	ms_ivec_t	vector
)
{
#ifdef DEBUG
	os_printf("CbusDisableInterrupt: cpu=%x islot=%x vector=%x\n",
		cpu, islot, vector);
#endif
	(*CbusSwitchp->DisableInterrupt)(cpu, islot, vector);
}

/*++

Routine Description:

	call the back-end to EOI the interrupt.

Arguments:

	cpu		- CPU to EOI the interrupt.

	islot	- interrupt slot to EOI.

	vector	- vector to EOI.

Return Value:

	none.

--*/
STATIC void
CbusEoiInterrupt(
	ms_cpu_t	cpu,
	ms_islot_t	islot,
	ms_ivec_t	vector
)
{
	(*CbusSwitchp->EoiInterrupt)(cpu, islot, vector);
}

/*++

Routine Description:

	call the back-end to IPI the cpu.

Arguments:

	cpu    - CPU to IPI.

Return Value:

	none.

--*/
STATIC void
CbusIpiCpu(
	ms_cpu_t	cpu
)
{
	(*CbusSwitchp->IpiCpu)(cpu);
}

/*++

Routine Description:

	call the back-end to get the free-running, high-resolution clock counter.

Arguments:

	none.

Return Value:

	current time stamp that is opaque to the base kernel.

--*/
STATIC ms_rawtime_t
CbusTimeGet(
	void
)
{
	return (*CbusSwitchp->TimeGet)();
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
CbusStartCpu(
	ms_cpu_t	cpu
)
{
#ifdef DEBUG
	os_printf("CbusStartCpu: cpu=%x\n", cpu);
#endif
	(*CbusSwitchp->StartCpu)(cpu);
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
CbusOffLinePrep(
	ms_cpu_t	cpu
)
{
#ifdef DEBUG
	os_printf("CbusOffLinePrep: cpu=%x\n", cpu);
#endif
	(*CbusSwitchp->OffLinePrep)(cpu);
}

/*++

Routine Description:

	called on a CPU which is being brought offline.
	all OS-specific offline processing has already been done.

	called with interrupts disabled at the CPU.

Arguments:

	none.

Return Value:

	none.

--*/
STATIC void
CbusOffLineSelf(
	void
)
{
#ifdef DEBUG
	os_printf("CbusOffLineSelf: cpu=%x\n", os_this_cpu);
#endif
	(*CbusSwitchp->OffLineSelf)();
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
CbusShowState(
	void
)
{
	(*CbusSwitchp->ShowState)();
}

/*++

Routine Description:

	timer interrupt service entry point.
	call the back-end to manage the PSM-specific,
	high-resolution counter and EOI the interrupt.

Arguments:

	cpu		- CPU to EOI the timer interrupt.

	islot	- interrupt slot to EOI.

	vector	- vector to EOI.

Return Value:

	MS_TRUE if the system timer is spurious, MS_FALSE otherwise.

--*/
STATIC ms_bool_t
CbusServiceTimer(
	ms_cpu_t	cpu,
	ms_islot_t	islot,
	ms_ivec_t	vector
)
{
	return (*CbusSwitchp->ServiceTimer)(cpu, islot, vector);
}

/*++

Routine Description:

	service a cross-processor interrupt.

Arguments:

	cpu		- cpu that received the cross-processor interrupt.

	islot	- interrupt slot that generated the cross-processor interrupt.

	vector	- vector that generated the cross-processor interrupt.

Return Value:

	none.

--*/
STATIC void
CbusServiceXint(
	ms_cpu_t	cpu,
	ms_islot_t	islot,
	ms_ivec_t	vector
)
{
	(*CbusSwitchp->ServiceXint)(cpu, islot, vector);
}

/*++

Routine Description:

	service a spurious interrupt.

Arguments:

	cpu		- cpu that received the spurious interrupt.

	islot	- interrupt slot that generated the spurious interrupt.

	vector	- vector that generated the spurious interrupt.

Return Value:

	none.

--*/
STATIC void
CbusServiceSpurious(
	ms_cpu_t	cpu,
	ms_islot_t	islot,
	ms_ivec_t	vector
)
{
	PSM_ASSERT(vector == CbusSpuriousVector);

	(*CbusSwitchp->ServiceSpurious)(cpu, islot, vector);
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
CbusPokeLed(
	ms_cpu_t	cpu,
	ULONG		flag
)
{
	(*CbusSwitchp->PokeLed)(cpu, flag);
}
