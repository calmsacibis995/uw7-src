#ident	"@(#)kern-i386at:psm/cbus/cbus_globals.c	1.2"
#ident  "$Header$"

/*++

Copyright (c) 1992-1997  Corollary Inc.

Module Name:

    cbus_globals.c

Abstract:

	this module contains all global data used by the
	Corollary C-bus PSM.

Function Prefix:

	none.

--*/

#include <cbus_includes.h>

//
// revision of the Corollary C-bus PSM.
//
PCHAR					CbusVersion = "1.0.0";

//
// values to pass to os_set_msparam().
//
UCHAR					CbusPlatformName[] = "C-bus";
ms_bool_t				CbusSwSysdump = MS_TRUE;
ms_time_t				CbusTimeRes = {1000, 0};
ms_time_t				CbusTick1Res = {100000, 0};
ms_time_t				CbusTick1Max = {27000000, 0};
ms_islot_t				CbusIslotMax = CBUS_MAXIRQS - 1;
ms_cpu_t				CbusShutdownCaps =
					      (MS_SD_HALT | MS_SD_AUTOBOOT | MS_SD_BOOTPROMPT);
//
// resource topology structure describes the number and arrangement
// of CPUs, I/O bus bridges, memories and caches in the system.
//
ms_topology_t *			CbusMsopTopology;

//
// PSM MSOP global variables.
//
ms_event_t				CbusMsopClockEvent = MS_EVENT_TICK_1;
ms_lockp_t				CbusMsopAttachLock;
ms_cpu_t				CbusMsopBootCpu = MS_CPU_ANY;
ms_intr_dist_t *		CbusMsopIntrp[CBUS_IDT_VECTORS];
i8254_params_t			CbusMsopI8254Params;

CBUS_MSOP_RAWTIME_T		CbusMsopRawTime = {0, 0, 0};

CBUS_MSOP_CPU_INFO_T	CbusMsopCpuInfo[CBUS_MAXCPUS];

//
// globals for all C-bus architectures.
//
CBUS_CONFIGURATION_T	CbusConfiguration;
CBUS_EXT_CFG_OVERRIDE_T	CbusGlobal;
CBUS_HARDWARE_INFO_T	CbusHardwareInfo;
CBUS_EXT_ID_INFO_T		CbusExtIdInfo[CBUS_MAXSLOTS];
PCSR					Cbus2CsrSpace[CBUS_MAXCPUS];
ULONG					CbusBusType;
ULONG					CbusNumCpus;
ULONG					CbusCpuIds[CBUS_MAXCPUS];
ULONG					CbusIrqPolarity;
ULONG					CbusXintVector;
ULONG					CbusSpuriousVector;
ULONG					CbusVendor;
ULONG					CbusVendorClass;
ULONG					CbusValidIds;

//
// C-bus II specific globals.
//
CBUS2_VECTORS_T			Cbus2SlotData[CBUS_IDT_VECTORS];
ULONG					Cbus2FixLevelInterrupts = 1;
ULONG					Cbus2FixSpuriousClock = 1;
ULONG					Cbus2BridgesFound;
PCSR					Cbus2BridgeCsr[CBUS2_MAX_BRIDGES];

PCBUS_SWITCH	CbusSwitch[] = {
	&Cbus2Switch,
};

PCBUS_SWITCH	CbusSwitchp = NULL;

ULONG			CbusNumberSwitches =
						sizeof(CbusSwitch)/sizeof(struct CbusSwitch *);
