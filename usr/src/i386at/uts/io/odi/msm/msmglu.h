#ifndef	_IO_MSM_MSMGLU_H
#define	_IO_MSM_MSMGLU_H

#ident	"@(#)msmglu.h	2.1"
#ident	"$Header$"

extern void	MSMAlertFatal(void);
extern void	MSMAlertWarning(void);
extern void	MSMAlloc(void);
extern void	MSMAllocPages(void);
extern void	MSMAllocateRCB(void);
extern void	MSMDeRegisterSharedMemory(void);
extern void	MSMDriverRemove(void);
extern void	MSMEnablePolling(void);
extern void	MSMFree(void);
extern void	MSMFreePages(void);
extern void	GetCurrentTime(void);
extern void	GetHardwareBusType(void);
extern void	GetProcessorSpeedRating(void);
extern void	MSMInitAlloc(void);
extern void	MSMInitFree(void);
extern void	MSMParseDriverParameters(void);
extern void	MSMParseCustomKeywords(void);
extern void	MSMPrintString(void);
extern void	MSMPrintStringFatal(void);
extern void	MSMPrintStringWarning(void);
extern void	MSMReadEISAConfig(void);
extern void	MSMReadPhysicalMemory(void);
extern void	MSMRegisterHardwareOptions(void);
extern void	MSMRegisterMLID(void);
extern void	MSMRegisterSharedMemory(void);
extern void	MSMReturnDriverResources(void);
extern void	MSMReturnRcvECB(void);
extern void	MSMScheduleAESCallBack(void);
extern void	MSMScheduleIntTimeCallBack(void);
extern void	LSLServiceEvents(void);
extern void	MSMSetHardwareInterrupt(void);
extern void	MSMWritePhysicalMemory(void);

extern void	AsmDriverISR(struct _DRIVER_DATA_ *driverData);
extern ODISTAT	AsmDriverMulticastChange(struct _DRIVER_DATA_ *driverData,
			CONFIG_TABLE *configTable, GROUP_ADDR_LIST_NODE *mcTable,
			UINT32 numEntries, UINT32 functionalTable);
extern void	AsmDriverPoll(struct _DRIVER_DATA_ *driverData,
			CONFIG_TABLE *configTable);
extern ODISTAT	AsmDriverReset(struct _DRIVER_DATA_ *driverData,
			CONFIG_TABLE *configTable);
extern void	AsmDriverSend(struct _DRIVER_DATA_ *driverData,
			CONFIG_TABLE *configTable, struct _TCB_ *tcb,
			UINT32 pktSize, void *PhysTcb);
extern ODISTAT	AsmDriverShutdown(struct _DRIVER_DATA_ *driverData,
			CONFIG_TABLE *configTable, UINT32 shutDownType);
extern void	AsmDriverTxTimeout(struct _DRIVER_DATA_ *driverData,
			CONFIG_TABLE *configTable);
extern ODISTAT	AsmDriverPromiscuousChange(struct _DRIVER_DATA_ *driverData,
			CONFIG_TABLE *configTable, UINT32 promiscuousMode);
extern ODISTAT	AsmDriverStatisticsChange(struct _DRIVER_DATA_ *driverData,
			CONFIG_TABLE *configTable);
extern ODISTAT	AsmDriverLookAheadChange(struct _DRIVER_DATA_ *driverData,
			CONFIG_TABLE *configTable);
extern ODISTAT	AsmDriverManagement(struct _DRIVER_DATA_ *driverData,
			CONFIG_TABLE *configTable, struct ECB *ecb);
extern void	AsmDriverEnableInterrupt(struct _DRIVER_DATA_ *driverData);
extern BOOLEAN	AsmDriverDisableInterrupt(struct _DRIVER_DATA_ *driverData,
			BOOLEAN switchVal);
extern void	AsmDriverAES(void *driverData, CONFIG_TABLE
			*configTable);
extern void	AsmDriverCallBack(void *driverData, CONFIG_TABLE
			*configTable);
extern void	AsmCustomKeywordRoutine(CONFIG_TABLE *, void **,
			int, UINT32 (*fcn)(CONFIG_TABLE *, PUINT8, UINT32));

#endif	/* _IO_MSM_MSMGLU_H */
