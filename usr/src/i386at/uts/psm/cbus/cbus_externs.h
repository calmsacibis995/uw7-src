#ifndef _PSM_CBUS_CBUS_EXTERNS_H
#define _PSM_CBUS_CBUS_EXTERNS_H

#ident	"@(#)kern-i386at:psm/cbus/cbus_externs.h	1.2"
#ident	"$Header$"

/*++

Copyright (c) 1992-1997  Corollary Inc.

Header Name:

    cbus_externs.h

Abstract:

    this module provides the externs for the C-bus PSM.

--*/

//
// revision of the Corollary C-bus PSM.
//
extern PCHAR				CbusVersion;

//
// MSOP functions described and informed to the kernel via this structure.
//
extern msop_func_t			CbusMsops[];

//
// values to pass to os_set_msparam().
//
extern UCHAR				CbusPlatformName[];
extern ms_bool_t			CbusSwSysdump;
extern ms_time_t			CbusTimeRes;
extern ms_time_t			CbusTick1Res;
extern ms_time_t			CbusTick1Max;
extern ms_islot_t			CbusIslotMax;
extern ms_cpu_t				CbusShutdownCaps;
//
// resource topology structure describes the number and arrangement
// of CPUs, I/O bus bridges, memories and caches in the system.
//
extern ms_topology_t *		CbusMsopTopology;

//
// PSM MSOP global variables.
//
extern ms_event_t			CbusMsopClockEvent;
extern ms_lockp_t			CbusMsopAttachLock;
extern ms_cpu_t				CbusMsopBootCpu;
extern ms_intr_dist_t *		CbusMsopIntrp[];
extern i8254_params_t		CbusMsopI8254Params;

extern CBUS_MSOP_RAWTIME_T	CbusMsopRawTime;

extern CBUS_MSOP_CPU_INFO_T	CbusMsopCpuInfo[];

//
// globals for all C-bus architectures.
//
extern CBUS_CONFIGURATION_T	CbusConfiguration;
extern CBUS_EXT_CFG_OVERRIDE_T	CbusGlobal;
extern CBUS_HARDWARE_INFO_T		CbusHardwareInfo;
extern CBUS_EXT_ID_INFO_T	CbusExtIdInfo[];
extern PCSR					Cbus2CsrSpace[];
extern ULONG				CbusBusType;
extern ULONG				CbusNumCpus;
extern ULONG				CbusCpuIds[];
extern ULONG				CbusIrqPolarity;
extern ULONG				CbusXintVector;
extern ULONG				CbusSpuriousVector;
extern ULONG				CbusVendor;
extern ULONG				CbusVendorClass;
extern ULONG				CbusValidIds;

//
// C-bus II specific globals.
//
extern PCSR					Cbus2CsrSpace[];
extern CBUS2_VECTORS_T		Cbus2SlotData[];
extern ULONG				cbus2_irqline_to_vector[];
extern ULONG				Cbus2FixLevelInterrupts;
extern ULONG				Cbus2FixSpuriousClock;
extern ULONG				Cbus2BridgesFound;
extern PCSR					Cbus2BridgeCsr[];

//
// C-bus back-end functions that are called
// by the C-bus MSOP routines.
//
extern CBUS_FUNCTIONS_T		CbusFunctions;
extern CBUS_SWITCH_T		Cbus2Switch;
extern PCBUS_SWITCH			CbusSwitch[];
extern PCBUS_SWITCH			CbusSwitchp;
extern ULONG				CbusNumberSwitches;

#endif // _PSM_CBUS_CBUS_EXTERNS_H
