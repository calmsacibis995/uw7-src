#ifndef	_IO_MSM_MSMSTRUC_H_
#define	_IO_MSM_MSMSTRUC_H_

#ident	"@(#)msmstruc.h	2.1"
#ident	"$Header$"

#ifdef  _KERNEL_HEADERS

#include <io/odi/msm/msm.h>

#elif defined(_KERNEL)

#include <sys/msm.h>

#endif

#define GET_HI_LO(x)						\
	((UINT16)(x) << 8) | (UINT16)((x) >> 8)

#define BYTE_MOVE(Dest, Source)					\
	(UINT8)*Dest = (UINT8)*Source

#define WORD_MOVE(Dest, Source)					\
	*(PUINT16)Dest = *(PUINT16)Source

#define DWORD_MOVE(Dest, Source)				\
	*(PUINT32)Dest = *(PUINT32)Source

#define CMPADDRESS(a1, a2)					\
	((*(PUINT32)a1 ^ *(PUINT32)a2)	\
	|| (((PUINT16)(a1))[2] ^ ((PUINT16)(a2))[2]))

#define RESERVED		0

typedef struct _MSM_IOCTL_TIMEOUT_ARGS_ {
	struct _MSM_IOCTL_TIMEOUT_ARGS_	*nextlink;
	ECB				*asyncECB;
	UINT32				ioctl;
	UINT32				brdnum;
	UINT32				cookie;
	UINT32				parm1;
	UINT32				parm2;
	UINT32				parm3;
} MSM_IOCTL_TIMEOUT_ARGS;

/*
 * definitions for Custom Keyword Types.
 */
#define	T_REQUIRED		0x8000	/* Keyword must be entered. */
#define	T_STRING		0x0100	/* String input(default). */
#define	T_NUMBER		0x0200	/* Dword Decimal input. */
#define	T_HEX_NUMBER		0x0300	/* Dword Hex input. */
#define	T_HEX_STRING		0x0400	/* 6-byte hex string input. */
#define	T_LENGTH_MASK		0x00FF	/* Length Mask */
#define T_TYPE_MASK         	0xFF00  /* Type Mask. */

typedef	struct _ADAPTER_DATA_ {
	struct _ADAPTER_DATA_		*InstanceLink;
	DRIVER_PARM_BLOCK		DPBlock;
	TSM_PARM_BLOCK			MPBlock;

	/*
	 * the following ECBs are here for asyncronous call back of IOCTLs.
	 */
	ECB                     	*GetStatsECB;
	ECB                     	*ShutDownECB;
	ECB                     	*ResetECB;
	ECB                     	*MulticastECB;
	ECB                     	*PromiscuousECB;
	ECB                     	*LookAheadSizeECB;

	struct _RTAG_HANDLE_    	*AllocRTag;
	struct _RTAG_HANDLE_    	*Below16MegRTag;
	struct _RTAG_HANDLE_    	*ECBRTag;
	UINT32                  	RxBelow16;
	void                    	*ECBBufferPool;
	void                    	*ECBBufferIndex;
	void                    	*MaxECBAddress;

#ifdef   IAPX386
	MLID_AES_ECB			MlidIntAES;
	MLID_AES_ECB			MlidAES;
	UINT32				HSMInC;
#endif

	struct _ADAPTER_DATA_   	*PollingLink;
	UINT32                  	UnBindOnRemove;
	UINT32                  	MaxTxFreeCount;
	ECB                     	*TxECBList;
	ECB                     	*TxECBHead;
	MLID_AES_ECB            	*AESList;
	EXTRA_CONFIG            	*ExtraConfigList;
	void                    	(* DriverIntReschedule)
					(struct _DRIVER_DATA_ *driverData);
#ifdef UNIXWARE
	UINT32				AdapterMatchValue;
	UINT32				InterruptCookie0;
	UINT32				InterruptCookie1;
	clock_t				isrtime;
	MSM_IOCTL_TIMEOUT_ARGS		*ioctltimeouts;
	int				initmemtype;
	int				initmemsrc;
#endif
	/*
	 * the following variables will be accessed by TSM's and HSM's and
	 * must remain in the following order.
	 */
#ifdef IAPX386
	UINT32                  	ReservedForCHSM[18];
	UINT32                  	AsmHSMUpdate;
	MSTATS                  	*AsmStatsPtr;
#else
	UINT32                  	ReservedForCHSM[20];
#endif
	DRIVER_PARM_BLOCK       	*AsmParmBlock;
	UINT32                  	AdapterIntDisabled; 

	UINT32                  	NeedContiguousECB;
	GENERIC_STATS           	*GenericStatsPtr;
#ifdef NETWARE
	void                    	*AdapterSpinLock;
#elif defined (UNIXWARE)
	MSM_PROTECTION_OBJ		*ProtectionObject;
#endif
	UINT32                  	MaxFrameHeaderSize;
	UINT8                   	PhysNodeAddress[ADDR_SIZE];
	UINT8                   	PhysNodeFill[2];
	void                    	*SendListHead;
	void                    	*SendListTail;
	DRIVER_PARM_BLOCK       	*DPBlockPtr;
	void                    	*MediaDataPtr;
	CONFIG_TABLE            	*DefaultVirtualBoard;
	UINT32                  	DriverInterrupt;
	UINT32                  	InCriticalSection;
	MLID_StatsTable         	*StatsPtr;
	CONFIG_TABLE            	*VirtualBoardLink[4];
	UINT32                  	StatusFlags;
	UINT32                  	TxFreeCount;
} ADAPTER_DATA;

/*
 * fake type definition to keep .h files happy. _DRIVER_DATA_ is
 * defined in the HSM and not accessed directly by the MSM or TSM.
 */
typedef struct _DRIVER_DATA_ {
        void    *FakeData;
} DRIVER_DATA;

typedef struct _MODULE_HANDLE_ {
        void    *FakeData;
} MODULE_HANDLE;

/*
 * this is a template to put over the top of an C DRIVER_PARM_BLOCK
 * at the DPB_Reserved1 field. This makes easy access to the ASM fields.
 */
typedef struct _ASM_DRIVER_KEWYORD_STRUCT_ {
	UINT32				DriverNumKeywords;
	MEON                    	**DriverKeywordText;
	UINT32                  	*DriverKeywordTextLen;
	UINT32                  	(_cdecl ** DriverProcesskeywordTab)(
					CONFIG_TABLE *configTable, PMEON
					string, UINT32 value);
} ASM_DRIVER_KEYWORD_STRUCT;

#define MSM_HIERPOLL		18
#define MSM_HIERADAPTER		24

#define	MSM_PHYS_LOGICAL_ADDR	0x1000000

#define	MAXINTVEC		256
#define	INTSHARED		0x00001
#define	INTALLOCED		0x00002

#endif	/* _IO_MSM_MSMSTRUC_H_ */
