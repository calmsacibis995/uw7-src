#ifndef _IO_MSM_MSM_H   /* wrapper symbol for kernel use */
#define _IO_MSM_MSM_H   /* subject to change without notice */

#ident	"@(#)msm.h	2.1"
#ident	"$Header$"

#ifdef  _KERNEL_HEADERS

#include <io/odi/msm/msmnbi.h>

#elif defined(_KERNEL)

#include <sys/msmnbi.h>

#endif

struct	_DRIVER_PARM_BLOCK_;
struct	_TSM_PARM_BLOCK_;
struct	_MEDIA_DATA_;
struct	_MSM_PROTECTION_OBJ_;
struct	_DRIVER_DATA_;

#define	ALL_ONES		-1
#define MAX_FRAME_TYPES		4

/*
 * portable alias definitions.
 */

#define	COPY_UINT16(dest_addr, src_addr)	\
					COPY_WORD(src_addr, dest_addr)
#define	COPY_UINT32(dest_addr, src_addr)	\
					COPY_LONG(src_addr, dest_addr)
#define	COPY_ADDR(dest_addr, src_addr)	COPY_NODE(src_addr, dest_addr)

#define	GET_UINT16(addr)		GET_WORD(addr)
#define	GET_UINT32(addr)		GET_LONG(addr)
#define	PUT_UINT16(addr, value)		PUT_WORD(value, addr)
#define	PUT_UINT32(addr, value)		PUT_LONG(value, addr)

#define	GET_HILO_UINT16(addr)		GET_HILO_WORD(addr)
#define	GET_HILO_UINT32(addr)		GET_HILO_LONG(addr)
#define	GET_LOHI_UINT16(addr)		GET_LOHI_WORD(addr)
#define	GET_LOHI_UINT32(addr)		GET_LOHI_LONG(addr)

#define	PUT_HILO_UINT16(addr, value)	PUT_HILO_WORD(value, addr)
#define	PUT_HILO_UINT32(addr, value)	PUT_HILO_LONG(value, addr)
#define	PUT_LOHI_UINT16(addr, value)	PUT_LOHI_WORD(value, addr)
#define	PUT_LOHI_UINT32(addr, value)	PUT_LOHI_LONG(value, addr)

#define	VALUE_TO_HILO_UINT16(addr)	VALUE_TO_HILO_WORD(addr)
#define	VALUE_TO_HILO_UINT32(addr)	VALUE_TO_HILO_LONG(addr)
#define	VALUE_TO_LOHI_UINT16(addr)	VALUE_TO_LOHI_WORD(addr)
#define	VALUE_TO_LOHI_UINT32(addr)	VALUE_TO_LOHI_LONG(addr)

#define	VALUE_FROM_HILO_UINT16(addr)	VALUE_FROM_HILO_WORD(addr)
#define	VALUE_FROM_HILO_UINT32(addr)	VALUE_FROM_HILO_LONG(addr)
#define	VALUE_FROM_LOHI_UINT16(addr)	VALUE_FROM_LOHI_WORD(addr)
#define	VALUE_FROM_LOHI_UINT32(addr)	VALUE_FROM_LOHI_LONG(addr)

#define	COPY_TO_HILO_UINT16(dest_addr, src_addr)	\
					COPY_TO_HILO_WORD(src_addr, dest_addr)
#define	COPY_TO_HILO_UINT32(dest_addr, src_addr)	\
					COPY_TO_HILO_LONG(src_addr, dest_addr)
#define	COPY_TO_LOHI_UINT16(dest_addr, src_addr)	\
					COPY_TO_LOHI_WORD(src_addr, dest_addr)
#define	COPY_TO_LOHI_UINT32(dest_addr, src_addr)	\
					COPY_TO_LOHI_LONG(src_addr, dest_addr)

#define	COPY_FROM_HILO_UINT16(dest_addr, src_addr)	\
					COPY_FROM_HILO_WORD(src_addr, dest_addr)
#define	COPY_FROM_HILO_UINT32(dest_addr, src_addr)	\
					COPY_FROM_HILO_LONG(src_addr, dest_addr)
#define	COPY_FROM_LOHI_UINT16(dest_addr, src_addr)	\
					COPY_FROM_LOHI_WORD(src_addr, dest_addr)
#define	COPY_FROM_LOHI_UINT32(dest_addr, src_addr)	\
					COPY_FROM_LOHI_LONG(src_addr, dest_addr)

#define	HOST_TO_HILO_UINT16(addr)	HOST_TO_HILO_WORD(addr)
#define	HOST_TO_HILO_UINT32(addr)	HOST_TO_HILO_LONG(addr)
#define	HOST_TO_LOHI_UINT16(addr)	HOST_TO_LOHI_WORD(addr)
#define	HOST_TO_LOHI_UINT32(addr)	HOST_TO_LOHI_LONG(addr)

#define	HOST_FROM_HILO_UINT16(addr)	HOST_FROM_HILO_WORD(addr)
#define	HOST_FROM_HILO_UINT32(addr)	HOST_FROM_HILO_LONG(addr)
#define	HOST_FROM_LOHI_UINT16(addr)	HOST_FROM_LOHI_WORD(addr)
#define	HOST_FROM_LOHI_UINT32(addr)	HOST_FROM_LOHI_LONG(addr)

#define	UINT16_EQUAL(addr1, addr2)	WORD_EQUAL(addr1, addr2)
#define	UINT32_EQUAL(addr1, addr2)	LONG_EQUAL(addr1, addr2)
#define	ADDR_EQUAL(addr1, addr2)	NODE_EQUAL(addr1, addr2)

typedef struct _FRAGMENT_LIST_STRUCT_ {
	UINT32		FragmentCount;
	FRAGMENT_STRUCT	FragmentStruct[1];
} FRAGMENT_LIST_STRUCT;

typedef struct _MSM_TCB_FRAGMENTSTRUCT_ {
	UINT32		TCB_FragmentCount;
	FRAGMENTSTRUCT	TCB_Fragment;
} MSM_TCB_FRAGMENTSTRUCT;

typedef struct _MSM_TCB_ {
	UINT32			TCB_DriverWS[3];
	UINT32			TCB_DataLen;
	MSM_TCB_FRAGMENTSTRUCT	*TCB_FragStruct;
} MSM_TCB;

typedef enum _CHSM_COMPLETE_ {
	CHSM_COMPLETE_STATISTICS = 0,
	CHSM_COMPLETE_MULTICAST = 1,
	CHSM_COMPLETE_SHUTDOWN = 2,
	CHSM_COMPLETE_RESET = 3,
	CHSM_COMPLETE_LOOK_AHEAD = 4,
	CHSM_COMPLETE_PROMISCUOUS = 5
} CHSM_COMPLETE;

typedef enum _AES_TYPE_ {
	AES_TYPE_PRIVILEGED_ONE_SHOT	= 0,
	AES_TYPE_PRIVILEGED_CONTINUOUS	= 1,
	AES_TYPE_PROCESS_ONE_SHOT	= 2,
	AES_TYPE_PROCESS_CONTINUOUS	= 3
} AES_TYPE;

typedef struct _MLID_AES_ECB_ {
	struct _MLID_AES_ECB_	*NextLink;
	void			(* _cdecl DriverAES)(struct _DRIVER_DATA_ *,
					CONFIG_TABLE *, ...);
	AES_TYPE		AesType;
	UINT32			TimeInterval;
	void			*AesContext;
	UINT8			AesReserved[30];
} MLID_AES_ECB;

typedef struct	_EXTRA_CONFIG_ISR_ {
	UINT32			(_cdecl * ISRRoutine)(void *MagicNumber);
	void			*ISRReserved0;
	void			*ISRReserved1;
	void			*ISRReserved2;
	void			*ISRReserved3;
} EXTRA_CONFIG_ISR;

typedef struct _EXTRA_CONFIG_ {
	struct _EXTRA_CONFIG_	*NextLink;
	UINT32			(_cdecl * ISRRoutine0)(void *MagicNumber);
	void			*ISR0Reserved0;
	void			*ISR0Reserved1;
	void			*ISR0Reserved2;
	UINT32			InterruptCookie0;
	UINT32			(_cdecl * ISRRoutine1)(void *MagicNumber);
	void			*ISR1Reserved0;
	void			*ISR1Reserved1;
	void			*ISR1Reserved2;
	UINT32			InterruptCookie1;
	IO_CONFIG		IOConfig;
} EXTRA_CONFIG;

typedef enum _REG_TYPE_ {
	REG_TYPE_NEW_ADAPTER,
	REG_TYPE_NEW_FRAME,
	REG_TYPE_NEW_CHANNEL,
	REG_TYPE_FAIL
} REG_TYPE;

typedef	struct _RCB_ {
	union {
		UINT8	RWs_i8val[8];
		UINT16	RWs_i16val[4];
		UINT32	RWs_i32val[2];
		UINT64	RWs_I64val;
	} RCBDriverWS;
	UINT8		RCBReserved[(UINT32)&(((struct ECB *)0)->
				ECB_FragmentCount) -
				(UINT32)&(((struct ECB *)0)->ECB_Status)];
	UINT32		RCBFragCount;
	FRAGMENTSTRUCT	RCBFragStruct;
} RCB;

#define	SPARE_ECB_STATUS	0xeeee

typedef	struct	_SHARED_DATA_ {
#ifdef IAPX386
	UINT32				CMSMReservedForCHSM[18];
	UINT32				CMSMAsmHSMUpdate;
	struct _MSTATS_			*CMSMAsmStatsPtr;
#else
	UINT32				CMSMReservedForCHSM[20];
#endif
	struct _DRIVER_PARM_BLOCK_	*CMSMAsmParmBlock;
	UINT32				CMSMAdapterIntDisabled;
	UINT32				CMSMNeedContiguousECB;
	struct _GENERIC_STATS_		*CMSMGenericStatsPtr;
	struct _MSM_PROTECTION_OBJ_	*CMSMProtectionObject;
	UINT32				CMSMMaxFrameHeaderSize;
	UINT8				CMSMPhysNodeAddress[8];
	void				*CMSMSendListHead;
	void				*CMSMSendListTail;
	struct _DRIVER_PARM_BLOCK_	*CMSMDPBlockPtr;
	void				*CMSMMediaDataPtr;
	CONFIG_TABLE			*CMSMDefaultVirtualBoard;
	UINT32				CMSMDriverInterrupt;
	UINT32				CMSMInCriticalSection;
	MLID_STATS_TABLE		*CMSMStatsPtr;
	CONFIG_TABLE			*CMSMVirtualBoardLink[4];
	UINT32				CMSMStatusFlags;
	volatile UINT32			CMSMTxFreeCount;
} SHARED_DATA;

#define	DADSP_TO_CMSMADSP(n)	((SHARED_DATA *)n - 1)

#define	PERMANENT_SHUTDOWN	0
#define	TEMPORARY_SHUTDOWN	1

/*
 * CMSMStatusFlags bit definitions.
 */
#define	SHUTDOWN		0x01
#define	TXQUEUED		0x02

struct	_TCB_;
struct	_TCB_FRAGMENTSTRUCT_;
struct	_MEDIA_HEADER_;

typedef	struct _FRAME_DATA_ {
	CONFIG_TABLE			ConfigTable;
	void				*Reserved[20];
	void				*DriverData;
	UINT32				InternalMediaID;
	UINT32				MediaHeaderSize;
	UINT8				PacketType[8];
	UINT32				BitSwapAddressFlag;
	struct _DRIVER_PARM_BLOCK_	*DPBlock;
	struct _TSM_PARM_BLOCK_		*MPBlock;
	void				*FirmwareBuffer;
	UINT32				HardwareIsRegistered;
	void				(_cdecl * SendEntry)(ECB *ecb,
						CONFIG_TABLE *configTable);
	UINT32				(_cdecl * SendRoutine)(
						SHARED_DATA *sharedData,
						ECB *ecb,
						struct _TCB_ *tcb);
	UINT32				(_cdecl * Driver_Control)(void);
	void				*Nothing;
} FRAME_DATA;

#define CONFIG_TO_MSM(n)	(FRAME_DATA *)((UINT8 *)n -	\
				(UINT32)&((FRAME_DATA *)0)->ConfigTable)
#define AES_TO_MLID(n)		(MLID_AES_ECB *)((UINT8 *)n -	\
				(UINT32)&((MLID_AES_ECB *)0)->AesReserved)

typedef struct ScreenStruct {
	void	*filler;
} SCREEN_HANDLE;

typedef struct _CHSM_STACK_ {
	struct _MODULE_HANDLE_	*ModuleHandle;		/* just an int */
	SCREEN_HANDLE		*ScreenHandle;		/* NULL */
	MEON			*CommandLine;
	MEON			*ModuleLoadPath; 	/* NULL */
	UINT32			UnitializedDataLength; 	/* 0 */
	void			*CustomDataFileHandle;
	UINT32			(* FileRead)(void *FileHandle, /* NULL */
					UINT32 FileOffset,
					void *FileBuffer,
					UINT32 FileSize);
	UINT32                  *CustomDataOffset;	/* NULL */
	UINT32			CustomDataSize;		/* NULL */
	UINT32			NumMsgs;		/* NULL */
	MEON			**Msgs;			/* msg file name */
} CHSM_STACK;

typedef struct _DRIVER_PARM_BLOCK_ {
	UINT32			DriverParameterSize;
	CHSM_STACK		*DriverInitParmPointer;
	PVOID			DriverModuleHandle;
	void			*DBP_Reserved0;
	void			*DriverAdapterPointer;
	CONFIG_TABLE		*DriverConfigTemplatePtr;
	UINT32			DriverFirmwareSize;
	void			*DriverFirmwareBuffer;
	UINT32                  DPB_Reserved1;
	void			*DPB_Reserved2;
	void			*DPB_Reserved3;
	void			*DPB_Reserved4;
	UINT32			DriverAdapterDataSpaceSize;
	struct _DRIVER_DATA_	*DriverAdapterDataSpacePtr;
	UINT32			DriverStatisticsTableOffset;
	UINT32			DriverEndOfChainFlag;
	UINT32			DriverSendWantsECBs;
	UINT32			DriverMaxMulticast;
	UINT32			DriverNeedsBelow16Meg;
	void			*DPB_Reserved5;
	void			*DPB_Reserved6;
	void			(_cdecl * DriverISRPtr)(struct _DRIVER_DATA_
					*driverData);
	ODISTAT			(_cdecl * DriverMulticastChangePtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE *configTable,
					GROUP_ADDR_LIST_NODE *mcTable,
					UINT32 numEntries, UINT32
					functionalTable);
	void			(_cdecl * DriverPollPtr)(struct _DRIVER_DATA_
					*driverData, CONFIG_TABLE
					*configTable);
	ODISTAT			(_cdecl * DriverResetPtr)(struct _DRIVER_DATA_
					*driverData, CONFIG_TABLE
					*configTable);
 	void			(_cdecl * DriverSendPtr)(struct _DRIVER_DATA_
					*driverData, CONFIG_TABLE
					*configTable, struct _TCB_ *tcb,
					UINT32 pktSize, void *PhysTcb);
	ODISTAT			(_cdecl * DriverShutdownPtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE *configTable,
					UINT32 shutDownType);
	void			(_cdecl * DriverTxTimeoutPtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE *configTable);
	ODISTAT			(_cdecl * DriverPromiscuousChangePtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE *configTable, UINT32
					promiscuousMode);
	ODISTAT			(_cdecl * DriverStatisticsChangePtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE *configTable);
	ODISTAT			(_cdecl * DriverRxLookAheadChangePtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE *configTable);
	ODISTAT			(_cdecl * DriverManagementPtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE *configTable,
					struct ECB *ecb);
	void			(_cdecl * DriverEnableInterruptPtr)(struct
					_DRIVER_DATA_ *driverData);
	BOOLEAN			(_cdecl * DriverDisableInterruptPtr)(struct
					_DRIVER_DATA_ *driverData,
					BOOLEAN switchVal);
	void			(_cdecl * DriverISRPtr2)(struct _DRIVER_DATA_
					*driverData);
	MEON			***DriverMessagesPtr;
	MEON_STRING		*HSMSpecVersionStringPtr;

} DRIVER_PARM_BLOCK;

#define	ASM_DPBLOCK_SIZE	136

#ifdef	IAPX386

/*
 * v3.00 Assembly HSMs stats table.
 */
typedef struct _MSTATS_ {
	UINT8		MSTATS_MajorVer;
	UINT8		MSTATS_MinorVer;
	UINT16		MSTATS_NumGenericCounters;
	UINT32		MSTATS_ValidMask0;
	GENERIC_STATS	MSTATS_Generic;
} MSTATS;

#endif

struct	_MODULE_HANDLE_;
struct	_DRIVER_OPTION_;

/*
 * MSM Specific declarations.
 */
extern	UINT8	MSMBitSwapTable[];

void	_cdecl	CMSMAddToCounter(STAT_TABLE_ENTRY *statTableEntryPtr,
			UINT32 value);
void *	_cdecl	CMSMAlloc(const struct _DRIVER_DATA_ *driverData,
			UINT32 nBytes);
void *	_cdecl	CMSMAllocPages(const struct _DRIVER_DATA_ *driverData,
			UINT32 nBytes);
RCB *	_cdecl	CMSMAllocateRCB(const struct _DRIVER_DATA_ *driverData,
			UINT32 nBytes, void **PhysicalRCB);
void	_cdecl	CMSMControlComplete(struct _DRIVER_DATA_ *driverData,
			CHSM_COMPLETE controlFunction, ODISTAT
			completionStatus);
void	_cdecl	CMSMDisableHardwareInterrupt(const struct _DRIVER_DATA_
			*driverData);
void	_cdecl	CMSMDriverRemove(struct _MODULE_HANDLE_ *);

/*
 * the following two functions are used to get the original ECB (with
 * Logical addresses in its frag list) back if the driver has set the
 * MM_FRAGS_PHYS_BIT and needs Logical fragments. the original ECB is
 * stored in the ECB_ESR field.
 */
ECB *	_cdecl	CMSMECBPhysToLogFrags(ECB *ecb);
ECB *	_cdecl	CMSMTCBPhysToLogFrags(struct _TCB_ *tcb);
void	_cdecl	CMSMEnableHardwareInterrupt(const struct _DRIVER_DATA_
			*driverData);
ODISTAT	_cdecl	CMSMEnablePolling(const struct _DRIVER_DATA_ *driverData);
void	_cdecl	CMSMEnqueueSend(FRAME_DATA *frameData, const struct
			_DRIVER_DATA_ *driverData, struct ECB *ecb);
void	_cdecl	CMSMFree(const struct _DRIVER_DATA_ *driverData, void *dataPtr);
void	_cdecl	CMSMFreePages(const struct _DRIVER_DATA_ *driverData,
			void *dataPtr);
UINT32	_cdecl	CMSMGetCurrentTime(void);
UINT32	_cdecl	CMSMGetMicroTimer(void);
void *	_cdecl	CMSMGetPhysical(void *logicalAddr);
ECB *	_cdecl	CMSMGetPhysicalECB(struct _DRIVER_DATA_ *driverData, ECB *ecb);
UINT32	_cdecl	CMSMGetPollSupportLevel(void);
void	_cdecl	CMSMIncrCounter(STAT_TABLE_ENTRY *statTableEntryPtr);
void *	_cdecl	CMSMInitAlloc(UINT32 nbytes);

ODISTAT	_cdecl	CMSMParseDriverParameters(DRIVER_PARM_BLOCK *hsmParmBlock,
			struct _DRIVER_OPTION_ *driverOption);
ODISTAT	_cdecl	CMSMParseCTSMParameters(CONFIG_TABLE *configTable,
			struct _DRIVER_OPTION_ *driverOption);
ODISTAT		CMSMASCIINodeToHex(CONFIG_TABLE *configTable,
			MEON_STRING *asciiNode);

typedef	enum _MSG_TYPE_ {
	MSG_TYPE_INIT_INFO,
	MSG_TYPE_INIT_WARNING,
	MSG_TYPE_INIT_ERROR,
	MSG_TYPE_RUNTIME_INFO,
	MSG_TYPE_RUNTIME_WARNING,
	MSG_TYPE_RUNTIME_ERROR
} MSG_TYPE;

void	_cdecl	CMSMPrintString(const CONFIG_TABLE *configTable, MSG_TYPE
			msgType, MEON_STRING *msg, void *parm1, void *parm2);
void	_cdecl	CMSMReadPhysicalMemory(UINT32 nbytes, void *destAddr,
			void *srcBusTag, const void *physSrcAddr);
REG_TYPE _cdecl	CMSMRegisterHardwareOptions(CONFIG_TABLE *configTable,
			struct _DRIVER_DATA_ **driverData);
ODISTAT	_cdecl	CMSMRegisterMLID(struct _DRIVER_DATA_ *driverData,
			CONFIG_TABLE *configTable);
ODISTAT	_cdecl	CMSMRegisterResource(const struct _DRIVER_DATA_	*driverData,
			CONFIG_TABLE *configTable, EXTRA_CONFIG *extraConfig);
ODISTAT	_cdecl	CMSMRegisterTSM(DRIVER_PARM_BLOCK *DriverParameterBlock,
			TSM_PARM_BLOCK *MediaParameterBlock, CONFIG_TABLE
			**configTable);
void	_cdecl	CMSMReturnDriverResources(const CONFIG_TABLE *configTable);
ECB *	_cdecl	CMSMReturnPhysicalECB(const struct _DRIVER_DATA_
			*driverData, ECB *ecb);
void	_cdecl	CMSMReturnRCB(const struct _DRIVER_DATA_ *driverData,
			RCB *rcb);
ODISTAT	_cdecl	CMSMScheduleAES(const struct _DRIVER_DATA_ *driverData,
			MLID_AES_ECB *mlidAESECB);
void	_cdecl	CMSMServiceEvents(void);
void	_cdecl	CMSMYieldWithDelay(void);

ODISTAT	_cdecl	CMSMSetHardwareInterrupt(struct _DRIVER_DATA_ *driverData,
			const CONFIG_TABLE *configTable);
void	_cdecl	CMSMWritePhysicalMemory(UINT32 nbytes, void *destBusTag,
			void *physDestAddr, const void *srcAddr);

UINT32	_cdecl	CMSMGetAlignment (UINT32 type );
ODI_NBI	_cdecl	CMSMGetBusInfo(void *busTag, UINT32 *physicalMemAddressSize,
			UINT32 *ioAddrSize );
ODI_NBI	_cdecl	CMSMGetBusName(void *busTag, MEON_STRING **busName );
ODI_NBI _cdecl	CMSMGetBusSpecificInfo(void *busTag, UINT32 size,
			void *busSpecificInfo);
ODI_NBI	_cdecl	CMSMGetBusTag(const MEON_STRING	*busName, void **busTag );
ODI_NBI	_cdecl	CMSMGetBusType(void *busTag, UINT32 *busType );
ODI_NBI _cdecl	CMSMGetCardConfigInfo(void *busTag, UINT32 uniqueIdentifier,
		UINT32 size, UINT32 parm1, UINT32 parm2, void *configInfo);
ODI_NBI	_cdecl	CMSMGetSlot(void *busTag, MEON_STRING *slotName, UINT16 *slot);
ODI_NBI	_cdecl	CMSMGetSlotName(void *busTag, UINT16 slot,
			MEON_STRING **slotName);
ODI_NBI _cdecl	CMSMGetUniqueIdentifier(void *busTag, UINT32 *parameters,
			UINT32 parameterCount, UINT32 *uniqueIdentifier);
ODI_NBI _cdecl	CMSMGetUniqueIdentifierParameters(void *busTag,
			UINT32 uniqueIdentifier, UINT32 parameterCount,
			UINT32 *parameters);
ODI_NBI _cdecl	CMSMGetInstanceNumber(void *busTag, UINT32 uniqueIdentifier,
			UINT16 *instanceNumber);
ODI_NBI _cdecl	CMSMGetInstanceNumberMapping(UINT16 instanceNumber,
			void **busTag, UINT32 *uniqueIdentifier);
UINT8	_cdecl	CMSMRdConfigSpace8(void *busTag,  UINT32 uniqueIdentifier,
			UINT32 offset);
UINT16	_cdecl	CMSMRdConfigSpace16(void *busTag,  UINT32 uniqueIdentifier,
			UINT32 offset);
UINT32	_cdecl	CMSMRdConfigSpace32(void *busTag,  UINT32 uniqueIdentifier,
			UINT32 offset);
ODI_NBI	_cdecl	CMSMScanBusInfo(UINT32 *scanSequence, void **busTag,
			UINT32 *busType, MEON_STRING **busName );
ODI_NBI _cdecl  CMSMSearchAdapter(UINT32 *scanSequence, UINT32 busType,
			UINT32 productIDLen, const MEON *productID,
			void **busTag, UINT32 *uniqueIdentifier);
void 	_cdecl	CMSMWrtConfigSpace8(void *busTag, UINT32 uniqueIdentifier,
			UINT32 offset, UINT8 writeVal);
void 	_cdecl	CMSMWrtConfigSpace16(void *busTag, UINT32 uniqueIdentifier,
			UINT32 offset, UINT16 writeVal);
void 	_cdecl	CMSMWrtConfigSpace32(void *busTag, UINT32 uniqueIdentifier,
			UINT32 offset, UINT32 writeVal);

/* TODO NESL
UINT32 	_cdecl	CMSMNESLDeRegisterConsumer(NESL_ECB *Consumer);
UINT32	_cdecl	CMSMNESLDeRegisterProducer(NESL_ECB *Producer);
void 	_cdecl	CMSMNESLProduceEvent(NESL_ECB *ProducerNecb,
			NESL_ECB **ConsumerNecb, EPB *EventParmBlock,
			struct DRIVER_DATA *driverData);
UINT32	_cdecl	CMSMNESLRegisterConsumer(NESL_ECB *Consumer);
UINT32	_cdecl	CMSMNESLRegisterProducer(NESL_ECB *Producer);
*/

#define CMP_NODE(a,b) ( \
	((UINT32 *)(a))[0] == ((UINT32 *)(b))[0] &&\
	((UINT16 *)(a))[2] == ((UINT16 *)(b))[2] \
)

struct IOOptionStructure {
        UINT32  NumberOfOptions;
        UINT32  OptionData[1];
};

typedef struct AdapterOptionDefinitionStructure {
        struct IOOptionStructure *IOSlot;
        struct IOOptionStructure *IOPort0;
        struct IOOptionStructure *IOLength0;
        struct IOOptionStructure *IOPort1;
        struct IOOptionStructure *IOLength1;
        struct IOOptionStructure *MemoryDecode0;
        struct IOOptionStructure *MemoryLength0;
        struct IOOptionStructure *MemoryDecode1;
        struct IOOptionStructure *MemoryLength1;
        struct IOOptionStructure *Interrupt0;
        struct IOOptionStructure *Interrupt1;
        struct IOOptionStructure *DMA0;
        struct IOOptionStructure *DMA1;
        struct IOOptionStructure *Channel;
} HSM_OPTIONS;

/*
 * flags that can be set on the OptionData 
 */
#define OD_IS_RANGE                     0x80000000
#define OD_HAS_INCREMENT        	0x40000000

/*
 * ParseIOParameters bits
 */
#define NeedsSlotBit                    0x00000001
#define NeedsIOPort0Bit                 0x00000002
#define NeedsIOLength0Bit               0x00000004
#define NeedsIOPort1Bit                 0x00000008
#define NeedsIOLength1Bit               0x00000010
#define NeedsMemoryDecode0Bit   	0x00000020
#define NeedsMemoryLength0Bit   	0x00000040
#define NeedsMemoryDecode1Bit   	0x00000080
#define NeedsMemoryLength1Bit   	0x00000100
#define NeedsInterrupt0Bit              0x00000200
#define NeedsInterrupt1Bit              0x00000400
#define NeedsDMA0Bit                    0x00000800
#define NeedsDMA1Bit                    0x00001000
#define NeedsChannelBit                 0x00002000

/*
 * ParseLANParameters needFlags defines
 */
#define CAN_SET_NODE_ADDRESS    	0x40000000
#define MUST_SET_NODE_ADDRESS   	0x80000000

#define CALLS_INTO_HSM 15

typedef struct _MSM_PROTECTION_OBJ_ {
        lock_t                  *AdapterSpinLock;
        lock_t                  *unused;
} MSM_PROTECTION_OBJ;

/*
 * flags for msm_read_eisa_config.
 */
#define	MSM_EISA_SUCCESS			0x0
#define	MSM_EISA_INVALID_SLOT			0x80
#define	MSM_EISA_INVALID_FUNC			0x81
#define	MSM_EISA_EMPTY_SLOT			0x83
#define	MSM_EISA_NOMEM				0x87

#ifdef IAPX386

#define CMSM_MLID_CFG_MAJOR     		1
#define CMSM_MLID_CFG_MINOR     		20
#define CMSM_ORIG_CFG_MAJOR     		1
#define CMSM_ORIG_CFG_MINOR     		20

#define AMSM_MLID_CFG_MAJOR     		1
#define AMSM_MLID_CFG_MINOR     		12

#define CMSM_IO_CFG_MAJOR       		1
#define CMSM_IO_CFG_MINOR       		0

#define ASM_STAT_WAIT_TIME      		4 * 18
#define ASM_STYLE_TABLE_FLAG    		0x10000000

#endif

#define FORMAT_STRING_LEN 128                   /* size for FormatString */

/*
 * stuff for reading in message file.
 */
#define	MSM_MESSAGE_COUNT_OFFSET		110
#define	MSM_MESSAGE_POINTER_OFFSET		118
#define	MSM_MAX_MSG_COUNT			0x00020000

#define	BROADCASTSIZE				8
#define	DEFLOOKAHEADSIZE			18

#ifdef DEBUG

/*
 * variant of LOCK_OWNED assertion at load time.
 */
extern	void					*lsl_current_adapterdata;

#define	ODI_ASSERT(a)				ASSERT(a ||		\
						lsl_current_adapterdata)

#else

#define	ODI_ASSERT(a)

#endif

#endif /* _IO_MSM_MSM_H */
