#ifndef _IO_ODI_ODI_H   /* wrapper symbol for kernel use */
#define _IO_ODI_ODI_H   /* subject to change without notice */

#ident	"@(#)odi.h	9.1"
#ident	"$Header$"

#ifdef  _KERNEL_HEADERS

#include <fs/ioccom.h>
#include <io/conf.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <net/dlpi.h>
#include <net/sockio.h>
#include <net/inet/byteorder.h>
#include <net/inet/if.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#elif defined(_KERNEL)

#include <sys/ioccom.h>
#include <sys/conf.h>
#include <sys/stream.h>
#include <sys/strlog.h>
#include <sys/stropts.h>
#include <sys/immu.h>
#include <sys/kmem.h>
#include <sys/dlpi.h>
#include <sys/sockio.h>
#include <sys/byteorder.h>
#include <net/if.h>
#include <sys/clock.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/ksynch.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/types.h>

#endif  /* _KERNEL_HEADERS */

#define	FALSE			0x0
#define	TRUE			0x1
#define	UNUSED			0xFFFFFFFF
#define	BOOLEAN			unsigned char
#define ULONG                   ulong_t
#define USHORT                  ushort_t
#define UCHAR                   uchar_t
#define _cdecl

typedef	unsigned char		MEON;
typedef	unsigned char		MEON_STRING;
typedef	unsigned char		UINT8;
typedef	unsigned short		UINT16;
typedef	unsigned long		UINT32;
typedef	unsigned char		BYTE;
typedef	signed long		LONG;
typedef	signed char		INT8;
typedef	signed short		INT16;
typedef	signed long		INT32;

#define BCOPY(from, to, len)    bcopy((caddr_t)from,(caddr_t)to,(size_t)len)
#define BCMP(s1, s2, len)       bcmp((char *)s1,(char *)s2, (size_t)len )
#define BZERO(addr, len)        bzero((caddr_t)addr,(size_t)len)
#define memcpy(from, to, len)	bcopy((caddr_t)from,(caddr_t)to,(size_t)len)

/*
 * well defined sizes.
 */
#define	PID_SIZE		0x06	/* Number of Octets in Protocol */
					/* Identifier */
#define	ADDR_SIZE		0x06	/* Number of Octets in Address */

#define	DefaultNumECBs		0x00
#define	DefaultECBSize		1518L	/* not including ECB Structure */
#define	MinECBSize		(512+74+MAXMEDIAHEADERSIZE)L
					/* Max. ECB size < 64K  */
/*
 * 42 is mac. FDDI MAC Header.
 */
#define	MAXLOOKAHEADSIZE	128L	/* Max. LookAhead Data Size */

/*
 * define assumed Maximum Media Header Size that we'll encounter.
 * assume that it will be Token-Ring (with SRT).
 *
 * AC, FC, Dest[6], Source[6], SRFields[30], 802.2UI[3], SNAP[5] = 52
 */
#define	MAXMEDIAHEADERSIZE	52L

#define	MAXBOARDS		16L
#define	MAXSTACKS		16L

#define	MAXSTACKNAMELENGTH	65L
#define	MAXNAMELENGTH		64L

#define	MAXMULTICASTS		10L
#define MAX_FRAG_COUNT		16

/*
 * MLIDCFG_LookAheadSize values.
 */
#define DEFAULT_LOOK_AHEAD_SIZE	0x12    /* Default of 18 look ahead bytes */
#define MAX_LOOK_AHEAD_SIZE	0x80	/* Maximum 128 look ahead bytes */

/*
 * Rx Packet Attributes (ie. LkAhd_PktAttr)
 */
#define PAE_CRC_BIT		0x00000001      /* CRC Error */
#define PAE_CRC_ALIGN_BIT       0x00000002      /* CRC/Frame Alignment Error*/
#define PAE_RUNT_PACKET_BIT     0x00000004      /* Runt Packet */
#define PAE_TOO_BIG_BIT         0x00000010      /* Packet Too Large for Media */
#define PAE_NOT_ENABLED_BIT     0x00000020      /* Unsupported Frame */
#define PAE_MALFORMED_BIT       0x00000040      /* Malformed Packet */
#define PA_NO_COMPRESS_BIT      0x00004000      /* Do not compress received */
						/* packet */
#define PA_NONCAN_ADDR_BIT      0x00008000      /* Set if Addr. in Immediate */
						/* Address field is */
						/* noncanonical */
#define PAE_ERROR_MASK          (PAE_CRC_BIT | PAE_CRC_ALIGN_BIT |	\
				PAE_RUNT_PACKET_BIT | PAE_TOO_BIG_BIT | \
				PAE_NOT_ENABLED_BIT | PAE_MALFORMED_BIT)
/*
 * Rx Packet Destination Address Types	(ie. LkAhd_DestType)
 */
#define DT_MULTICAST		0x00000001	/* Multicast Dest. address */
						/* (Group Address) */
#define	DT_BROADCAST		0x00000002      /* Broadcast Dest. address */
#define DT_REMOTE_UNICAST	0x00000004	/* Remote Unicast Dest. */
						/* address */
#define DT_REMOTE_MULTICAST	0x00000008	/* Unsupported Multicast */
						/* address */
#define DT_SOURCE_ROUTE         0x00000010      /* Source Routed packet */
#define DT_ERRORED		0x00000020      /* Global Error, exculsive */
						/* bit */
#define DT_MAC_FRAME            0x00000040      /* MAC/SMT frames. */
						/* (ie. NON-DATAFrame) */
#define DT_DIRECT		0x00000080      /* Unicast for this */
						/* workstation */
#define DT_8022_TYPE_I		0x00000100      /* set if packet is */
						/* 802.2 Type I */
#define DT_8022_TYPE_II         0x00000200      /* set if packet is */
						/* 802.2 Type II */
#define DT_8022_BYTES_BITS      0x00000300
#define DT_RX_PRIORITY          0x00000400      /* Set if packet has */
						/* priority value other */
						/* than base values */
#define DT_PROMISCUOUS		(DT_ERRORED | DT_DIRECT | DT_MULTICAST |    \
				DT_BROADCAST | DT_REMOTE_UNICAST |	    \
				DT_REMOTE_MULTICAST | DT_NO_SROUTE |	    \
				DT_MAC_FRAME | DT_RX_PRIORITY)

#define	DT_MASK			(DT_MULTICAST | DT_BROADCAST |		    \
				DT_REMOTE_UNICAST | DT_REMOTE_MULTICAST |   \
				DT_SOURCE_ROUTE | DT_ERRORED | DT_MAC_FRAME \
				| DT_DIRECT | DT_RX_PRIORITY)
/*
 * ECB Definitions, stack ID Definitions.
 */
#define	ECB_RAWMODE		0xFFFF		/* Raw mode, ie. ECB */
						/* includes MAC Header */
						/* implies MLID should not  */
						/* build or strip MAC Header */
#define RAW_SEND		ECB_RAWMODE	/* was going to delete this, */
						/* but it's referenced in */
						/* fdditsm... */
#define ECB_RAWLLC		0xFFFE		/* Raw LLC, ie ECB includes */
						/* 802.2 header - implies */
						/* MLID should not build or */
						/* strip 802.2 header */
#define	ECB_MULTICAST		0x00000001	/* Multicast Dest. address */
						/* (Group Address) */
#define	ECB_BROADCAST		0x00000002	/* Broadcast Dest. address */
#define	ECB_UNICASTREMOTE	0x00000004	/* Remote Unicast Dest. */
						/* address */
#define	ECB_MULTICASTREMOTE	0x00000008	/* Unsupported Multicast */
						/* address */
#define	ECB_SOURCE_ROUTE	0x00000010      /* Source Routed packet */
#define	ECB_GLOBALERROR		0x00000020	/* Set if packet contains */
						/* errors (exculsive) */
						/* If set all other bits */
						/* should be reset. */
#define ECB_MACFRAME		0x00000040	/* Packet is not a data */
						/* packet.
						/* If set all other bits */
						/* should be reset. */
#define	ECB_UNICASTDIRECT	0x00000080	/* Unicast for this */
						/* workstation */
#define	ECB_MASK		0x000000FF	/* mask off allowable */
						/* Destination Address Types */
#define	ECB_TYPE_I		0x00000100	/* Set if packet is 802.2 */
						/* Type I */
#define	ECB_TYPE_II		0x00000200	/* Set if packet is 802.2 */
						/* Type II */
#define ECB_RX_PRIORITY		0x00000400	/* Set if packet has */
						/* priority value other */
						/* than base values */
#define	ECB_PROMISCUOUS		(ECB_ERRORED | ECB_MULTICAST | ECB_BROADCAST \
				| ECB_UNICASTREMOTE | ECB_MULTICASTREMOTE |  \
				ECB_SOURCE_ROUTE | ECB_MAC_FRAME |	     \
				ECB_RX_PRIORITY)
/*
 * PromiscuousChange state and mode values.
 */
#define PROM_STATE_OFF		0x00		/* Disable Promiscuous Mode */
#define PROM_STATE_ON 		0x01		/* Enable Promiscuous Mode */
#define PROM_MODE_QUERY		0x00		/* Query as to prom mode */
#define PROM_MODE_MAC		0x01		/* MAC frames */
#define PROM_MODE_NON_MAC	0x02		/* Non-MAC frames */
#define PROM_MODE_MACANDNON	0x03		/* Both MAC and Non-MAC */
						/* frames */
#define PROM_MODE_SMT		0x04		/* FDDI SMT Type MAC frames */

/*
 * system return code definitions
 */
typedef enum _ODISTAT_ {
	ODISTAT_SUCCESSFUL		= 0,
	ODISTAT_RESPONSE_DELAYED	= 1,
	ODISTAT_SUCCESS_TAKEN		= 2,
	ODISTAT_BAD_COMMAND		= -127,
	ODISTAT_BAD_PARAMETER		= -126,
	ODISTAT_DUPLICATE_ENTRY		= -125,
	ODISTAT_FAIL			= -124,
	ODISTAT_ITEM_NOT_PRESENT	= -123,
	ODISTAT_NO_MORE_ITEMS		= -122,
	ODISTAT_NO_SUCH_DRIVER		= -121,
	ODISTAT_NO_SUCH_HANDLER		= -120,
	ODISTAT_OUT_OF_RESOURCES	= -119,
	ODISTAT_RX_OVERFLOW		= -118,
	ODISTAT_IN_CRITICAL_SECTION	= -117,
	ODISTAT_TRANSMIT_FAILED		= -116,
	ODISTAT_PACKET_UNDELIVERABLE	= -115,
	ODISTAT_CANCELED		= -4
} ODISTAT;

/*
 * MLID Configuration Table Bit Defintions
 */

/*
 * MLID 'Flags' Bit Definitions
 */
#define MF_HUB_MANAGEMENT_BIT	0x0100
#define MF_SOFT_FILT_GRP_BIT	0x0200
#define MF_GRP_ADDR_SUP_BIT	0x0400
#define MF_MULTICAST_TYPE_BITS	0x0600
#define MF_RECONFIG_BIT		0x0800
#define MF_PRIORITYSUP_BIT	0x1000

/*
 * MLID 'ModeFlags' Bit Definitions.
 */
#define MM_REAL_DRV_BIT		0x0001
#define MM_USES_DMA_BIT		0x0002
#define MM_DEPENDABLE_BIT	0x0004		/* should only be set if */
						/* MM_POINT_TO_POINT_BIT */
						/* set, for hardware that is */
						/* normally dependable but */
						/* is not 100% guaranteed */
#define MM_MULTICAST_BIT	0x0008
#define MM_CSL_COMPLIANT_BIT	0x0010		/* Set if MLID is CSL */
						/* compliant */
#define MM_PREFILLED_ECB_BIT	0x0020		/* MLID supplies pre-filled */
						/* ECBs */
#define MM_RAW_SENDS_BIT	0x0040
#define MM_DATA_SZ_UNKNOWN_BIT	0x0080
#define MM_SMP_BIT		0x0100		/* Set if MLID is SMP */
						/* enabled.  */
#define MM_FRAG_RECEIVES_BIT	0x0400		/* MLID can handle 8/
						/* Fragmented Receive ECB. */
#define MM_C_HSM_BIT		0x0800		/* set if HSM written in C. */
#define MM_FRAGS_PHYS_BIT	0x1000		/* set if HSM wants Frags */
						/* with Physical Addresses. */
#define MM_PROMISCUOUS_BIT	0x2000		/* set if supports */
						/* Promiscuous Mode. */
#define MM_NONCANONICAL_BIT	0x4000		/* set if Config Node */
						/* Address Non-Canonical */
#define MM_PHYS_NODE_ADDR_BIT	0x8000		/* set if MLID utilizes */
						/* Physical Node Address. */
#define MM_CANONICAL_BITS	0xC000

/*
 * MLID 'SharingFlags' Bit Defintions.
 */
#define MS_SHUTDOWN_BIT		0x0001
#define MS_SHARE_PORT0_BIT	0x0002
#define MS_SHARE_PORT1_BIT	0x0004
#define MS_SHARE_MEMORY0_BIT	0x0008
#define MS_SHARE_MEMORY1_BIT	0x0010
#define MS_SHARE_IRQ0_BIT	0x0020
#define MS_SHARE_IRQ1_BIT	0x0040
#define MS_SHARE_DMA0_BIT	0x0080
#define MS_SHARE_DMA1_BIT	0x0100
#define MS_HAS_CMD_INFO_BIT	0x0200
#define MS_NO_DEFAULT_INFO_BIT	0x0400

/*
 * MLID 'LineSpeed' Bit Definitions.
 */
#define MLS_MASK		0x7FFFF
#define MLS_KILO_IND_BIT	0x80000

/*
 * STAT_TABLE_ENTRY Definitons.
 */
#define	ODI_STAT_UNUSED		0xFFFFFFFF	/* Statistics Table Entry */
						/* not in use.*/
#define	ODI_STAT_UINT32		0x00000000	/* Statistics Table Entry */
						/* UINT32 Counter */
#define	ODI_STAT_UINT64		0x00000001	/* Statistics Table Entry */
						/* UINT64 Counter */
#define ODI_STAT_MEON_STRING	0x00000002	/* Statistics Table Entry */
						/* Counter is a MEON_STRING */
#define ODI_STAT_UNTYPED	0x00000003	/* Statistics Table Entry */
						/* Counter is a UINT32 */
						/* length preceded array */
						/* UINT8 */
#define ODI_STAT_RESETABLE	0x80000000	/* Statistics Table Entry */
						/* Counter is resetable by */
						/* external entity */
#define	NULL		0

typedef	struct	_UINT64_ {
	UINT32	Low_UINT32;
	UINT32	High_UINT32;
} UINT64;

typedef	void		VOID;
typedef MEON		UNICODE_STRING;
typedef MEON		*PUNICODE_STRING;
typedef MEON		*PWSTR;

/*
 * declare the pointer for the ODI definitions
 */
typedef	MEON		*PMEON;
typedef	UINT8		*PUINT8;
typedef	UINT16		*PUINT16;
typedef	UINT32		*PUINT32;
typedef	UINT64		*PUINT64;
typedef	VOID		*PVOID;

typedef struct _PROT_ID_ {
        UINT8   protocolID[6];
} PROT_ID;

typedef struct  _NODE_ADDR_ {
        UINT8   nodeAddress[6];
} NODE_ADDR;

/*
 * Set PRAGMA to pack these structures
 */
#pragma	pack(1)

typedef	struct _STAT_TABLE_ENTRY_ {
	UINT32		StatUseFlag;	/* ODI_STAT_UNUSED Statistics Table */
					/* Entry not in use, OR */
					/* ODI_STAT_UINT32 *StatConter is a */
					/* pointer to an UINT32 Counter, OR */
					/* ODI_STAT_UINT32 *StatConter is a */
					/* a pointer to an UINT64 Counter */
	VOID		*StatCounter;	/* pointer to a UINT32 or UINT64 */
					/* counter. */
	MEON_STRING	*StatString;	/* pointer to a MEON String, */
					/* describing the statistics counter */
} StatTableEntry, *PStatTableEntry, STAT_TABLE_ENTRY;

/*
 * definitions for Information Block for passing API's, eg. Function Lists
 */
typedef	struct _INFO_BLOCK_ {
	UINT32	NumberOfAPIs;
	VOID	(**SupportAPIArray)();
} INFO_BLOCK, *PINFO_BLOCK;

typedef struct _INT_AES_ {
        struct _INT_AES_        *TLink;
        void                    (* TCallBackProcedure)();
        UINT32                  TCallBackEBXParameter;
        UINT32                  TCallBackWaitTime;
        void			*TResourceTag;
        UINT32                  TWorkWakeUpTime;
        UINT32                  TSignature;
} INT_AES;

typedef struct  _AES_ {
        struct _AES_            *AESLink;
        UINT32                  AESWakeUpDelayAmount;
        UINT32                  AESWakeUpTime;
        void                    (* AESProcessToCall)();
        void			*AESRTag;
        struct _AES_            *AESOldLink;
} AES;

#ifdef IAPX386

struct AESProcessStructure {
	struct AESProcessStructure	*AESLink;
	UINT32				AESWakeUpDelayAmount;
	UINT32				AESWakeUpTime;
	VOID				(* AESProcessToCall)();
	VOID				*AESRTag;
	VOID				*AESOldLink;
};

#define	ALink			AESLink
#define	AWakeUpDelayAmount	AESWakeUpDelayAmount
#define	AWakeUpTime		AESWakeUpTime
#define	AProcessToCall		AESProcessToCall
#define	ARTag			AESRTag
#define	AOldLink		AESOldLink

#endif

typedef	struct	_FRAGMENT_STRUCT_ {
	VOID			*FragmentAddress;
	UINT32			FragmentLength;
} FRAGMENTSTRUCT, FRAGMENT_STRUCT, *PFRAGMENTSTRUCT;

struct	msgb;
struct  lslsap;

typedef struct _manage_ecb_ {
	struct	ECB		*manage_nextlink;
	struct	ECB		*manage_prevlink;
	struct	msgb		*manage_pmsgb;
	void			(* manage_esr)(struct ECB *);
	struct	lslsap		*manage_sap;
	UINT32			manage_esrebxval;
	UINT32			manage_memtype;
	UINT32			manage_memsrc;
} manage_ecb;

typedef struct ECB {
	UINT8			ECB_RCBDriverWS[8];
	UINT16			ECB_Status;
	manage_ecb		*ECB_management;	/* only in Unixware! */
	UINT16			ECB_StackID;
	PROT_ID			ECB_ProtocolID;
	UINT32			ECB_BoardNumber;
	NODE_ADDR		ECB_ImmediateAddress;
	union {
		UINT8		DWs_i8val[4];
		UINT16		DWs_i16val[2];
		UINT32		DWs_i32val;
		VOID		*DWs_pval;
	} ECB_DriverWorkspace;

	union {
		UINT8		PWs_i8val[8];
		UINT16		PWs_i16val[4];
		UINT32		PWs_i32val[2];
		UINT64		PWs_i64val;
		void		*PWs_pval[2];
	} ECB_ProtocolWorkspace;

	UINT32			ECB_DataLength;
	UINT32			ECB_FragmentCount;
	FRAGMENT_STRUCT		ECB_Fragment[1];
} ECB;

/*
 * following macros for Unixware specific fields.
 */
#define ECB_NextLink            ECB_management->manage_nextlink
#define ECB_PreviousLink        ECB_management->manage_prevlink
#define ECB_mblk                ECB_management->manage_pmsgb
#define ECB_sap                 ECB_management->manage_sap
#define ECB_ESR                 ECB_management->manage_esr
#define	ECB_memtype		ECB_management->manage_memtype
#define	ECB_memsrc		ECB_management->manage_memsrc

/*
 * just mapping to new names.
 */
#define fLinkAddress            ECB_NextLink
#define fragmentDescriptor      ECB_Fragment
#define packetLength            ECB_DataLength
#define ESRAddress              ECB_ESR
#define fragmentCount           ECB_FragmentCount
#define boardNumber             ECB_BoardNumber
#define socketNumber            ECB_ProtocolWorkspace.PWs_i32val[1]
#define address                 FragmentAddress

#define ESREBXValue             ECB_management->manage_esrebxval

#define	ECB_TAIL		12

typedef	struct	_LOOKAHEAD_ {
	struct ECB	*LkAhd_PreFilledECB;
	UINT8		*LkAhd_MediaHeaderPtr;
	UINT32		LkAhd_MediaHeaderLen;
	UINT8		*LkAhd_DataLookAheadPtr;
	UINT32		LkAhd_DataLookAheadLen;
	UINT32		LkAhd_BoardNumber;
	UINT32		LkAhd_PktAttr;	/* now Packet Attributes instead */
					/* of ErrorStatus */
	UINT32		LkAhd_DestType;

	UINT32		LkAhd_FrameDataSize;
	UINT16		LkAhd_PadAlignBytes1;
	PROT_ID		LkAhd_ProtocolID;
	UINT16		LkAhd_PadAlignBytes2;
	NODE_ADDR	LkAhd_ImmediateAddress;
	UINT32		LkAhd_FrameDataStartCopyOffset;
	UINT32		LkAhd_FrameDataBytesWanted;
	ECB		*LkAhd_ReturnedECB;
	UINT32		LkAhd_PriorityLevel;
	void		*LkAhd_Reserved;
} LOOKAHEAD, *PLOOKAHEAD;

typedef struct _Lan_Memory_Configuration_ {
	void		*MemoryAddress;
	UINT16		MemorySize;
} Lan_Memory_Configuration;

/*
 * definitions for MLID Configuration, statistics tables and
 * misc. structures.
 */
typedef	struct _MLID_CONFIG_TABLE_ {
	MEON		MLIDCFG_Signature[26];
	UINT8		MLIDCFG_MajorVersion;
	UINT8		MLIDCFG_MinorVersion;
	NODE_ADDR	MLIDCFG_NodeAddress;
	UINT16		MLIDCFG_ModeFlags;
	UINT16		MLIDCFG_BoardNumber;
	UINT16		MLIDCFG_BoardInstance;
	UINT32		MLIDCFG_MaxFrameSize;
	UINT32		MLIDCFG_BestDataSize;
	UINT32		MLIDCFG_WorstDataSize;
	MEON_STRING	*MLIDCFG_CardName;
	MEON_STRING	*MLIDCFG_ShortName;
	MEON_STRING	*MLIDCFG_FrameTypeString;
	UINT16		MLIDCFG_Reserved0;
	UINT16		MLIDCFG_FrameID;
	UINT16		MLIDCFG_TransportTime;
	UINT32		(*MLIDCFG_SourceRouting)(UINT32, void*, void**,
				BOOLEAN);
	UINT16		MLIDCFG_LineSpeed;
	UINT16		MLIDCFG_LookAheadSize;
	UINT8		MLIDCFG_SGCount;
	UINT8		MLIDCFG_Reserved1;
	UINT16		MLIDCFG_PrioritySup;
	void		*MLIDCFG_Reserved2;
	UINT8		MLIDCFG_DriverMajorVer;
	UINT8		MLIDCFG_DriverMinorVer;
	UINT16		MLIDCFG_Flags;
	UINT16		MLIDCFG_SendRetries;
	void		*MLIDCFG_DriverLink;
	UINT16		MLIDCFG_SharingFlags;
	UINT16		MLIDCFG_Slot;
	UINT16		MLIDCFG_IOPort0;
	UINT16		MLIDCFG_IORange0;
	UINT16		MLIDCFG_IOPort1;
	UINT16		MLIDCFG_IORange1;
	Lan_Memory_Configuration
			LAN_MEMORY_CONFIGURATION[2];
	UINT8		MLIDCFG_Interrupt0;
	UINT8		MLIDCFG_Interrupt1;
	UINT8		MLIDCFG_DMALine0;
	UINT8		MLIDCFG_DMALine1;
	VOID		*MLIDCFG_ResourceTag;
	VOID		*MLIDCFG_Config;
	VOID		*MLIDCFG_CommandString;
	MEON_STRING	MLIDCFG_LogicalName[18];
	void		*MLIDCFG_LinearMemory0;
	void		*MLIDCFG_LinearMemory1;
	UINT16		MLIDCFG_ChannelNumber;
	void		*MLIDCFG_DBusTag;
	UINT8		MLIDCFG_DIOConfigMajorVer;
	UINT8		MLIDCFG_DIOConfigMinorVer;
} MLID_ConfigTable, *PMLID_ConfigTable, MLID_CONFIG_TABLE;

typedef	MLID_ConfigTable	CONFIG_TABLE;

#define MLIDCFG_MemoryAddress0	LAN_MEMORY_CONFIGURATION[0].MemoryAddress
#define MLIDCFG_MemorySize0	LAN_MEMORY_CONFIGURATION[0].MemorySize
#define MLIDCFG_MemoryAddress1	LAN_MEMORY_CONFIGURATION[1].MemoryAddress
#define MLIDCFG_MemorySize1	LAN_MEMORY_CONFIGURATION[1].MemorySize

#define	DCardName		MLIDCFG_CardName
#define	DShortName		MLIDCFG_ShortName
#define	DBoardNumber		MLIDCFG_BoardNumber
#define	DMaxDataSize		MLIDCFG_MaxFrameSize
#define	DMaxRecvSize		MLIDCFG_BestDataSize
#define	DRecvSize		MLIDCFG_WorstDataSize
#define	DLogicalName		MLIDCFG_LogicalName
#define	DNodeAddress		MLIDCFG_NodeAddress
#define	DReserved		MLIDCFG_SourceRouting
#define	DLink			MLIDCFG_DriverLink

typedef struct _IO_CONFIG_ {
	struct _IO_CONFIG_		*IO_DriverLink;
	UINT16				IO_SharingFlags;
	UINT16				IO_Slot;
	UINT16				IO_IOPort0;
	UINT16				IO_IORange0;
	UINT16				IO_IOPort1;
	UINT16				IO_IORange1;
	Lan_Memory_Configuration
					LAN_MEMORY_CONFIGURATION[2];
	UINT8				IO_Interrupt0;
	UINT8				IO_Interrupt1;
	UINT8				IO_DMALine0;
	UINT8				IO_DMALine1;
	struct ResourceTagStructure	*IO_ResourceTag;
	void				*IO_Config;
	void				*IO_CommandString;
	MEON_STRING			IO_LogicalName[18];
	void				*IO_LinearMemory0;
	void				*IO_LinearMemory1;
	UINT16				IO_ChannelNumber;
	void				*IO_DBusTag;
	UINT8				IO_DIOConfigMajorVer;
	UINT8				IO_DIOConfigMinorVer;
} IO_CONFIG;

#define IO_MemoryAddress0	LAN_MEMORY_CONFIGURATION[0].MemoryAddress
#define IO_MemorySize0		LAN_MEMORY_CONFIGURATION[0].MemorySize
#define IO_MemoryAddress1	LAN_MEMORY_CONFIGURATION[1].MemoryAddress
#define IO_MemorySize1		LAN_MEMORY_CONFIGURATION[1].MemorySize

typedef	struct _MLID_STATS_TABLE_ {
	UINT16			MStatTableMajorVer;
	UINT16			MStatTableMinorVer;
	UINT32			MNumGenericCounters;
	STAT_TABLE_ENTRY	*MGenericCountsPtr;
	UINT32			MNumMediaCounters;
	STAT_TABLE_ENTRY	*MMediaCountsPtr;
	UINT32			MNumCustomCounters;
	STAT_TABLE_ENTRY	*MCustomCountersPtr;
} MLID_StatsTable, *PMLID_StatsTable, MLID_STATS_TABLE;

#define NUM_GENERIC_MLID_COUNTERS               20

#define MLID_TOTAL_TX_PACKET_COUNT		0
#define MLID_TOTAL_RX_PACKET_COUNT		1
#define MLID_NO_ECB_AVAILABLE_COUNT		2
#define MLID_PACKET_TX_TOO_BIG_COUNT		3
#define MLID_PACKET_TX_TOO_SMALL_COUNT		4
#define MLID_PACKET_RX_OVERFLOW_COUNT		5
#define MLID_PACKET_RX_TOO_BIG_COUNT		6
#define MLID_PACKET_RX_TOO_SMALL_COUNT		7
#define MLID_PACKET_TX_MISC_ERROR_COUNT		8
#define MLID_PACKET_RX_MISC_ERROR_COUNT		9
#define MLID_RETRY_TX_COUNT			10
#define MLID_CHECKSUM_ERROR_COUNT		11
#define MLID_HARDWARE_RX_MISMATCH_COUNT		12
#define MLID_TOTAL_TX_OK_BYTE_COUNT		13
#define MLID_TOTAL_RX_OK_BYTE_COUNT		14
#define MLID_TOTAL_GROUP_ADDR_TX_COUNT		15
#define MLID_TOTAL_GROUP_ADDR_RX_COUNT		16
#define MLID_ADAPTER_RESET_COUNT		17
#define MLID_ADAPTER_OPR_TIME_STAMP		18
#define MLID_Q_DEPTH				19

#define NUM_TOKEN_SPECIFIC_COUNTERS             13

#define TRN_AC_ERROR_COUNT			0
#define TRN_ABORT_DELIMITER_COUNTER             1
#define TRN_BURST_ERROR_COUNTER                 2
#define TRN_FRAME_COPIED_ERROR_COUNTER		3
#define TRN_FREQUENCY_ERROR_COUNTER             4
#define TRN_INTERNAL_ERROR_COUNTER              5
#define TRN_LAST_RING_STATUS                    6
#define TRN_LINE_ERROR_COUNTER                  7
#define TRN_LOST_FRAME_COUNTER                  8
#define TRN_TOKEN_ERROR_COUNTER                 9
#define TRN_UPSTREAM_NODE_ADDRESS               10
#define TRN_LAST_RING_ID			11
#define TRN_LAST_BEACON_TYPE                    12

#define NUM_ETHERNET_SPECIFIC_COUNTERS          8

#define ETH_TX_OK_SINGLE_COLLISIONS_COUNT       0
#define ETH_TX_OK_MULTIPLE_COLLISIONS_COUNT     1
#define ETH_TX_OK_BUT_DEFERRED			2
#define ETH_TX_ABORT_LATE_COLLISION		3
#define ETH_TX_ABORT_EXCESS_COLLISION           4
#define ETH_TX_ABORT_CARRIER_SENSE		5
#define ETH_TX_ABORT_EXCESSIVE_DEFERRAL         6
#define ETH_RX_ABORT_FRAME_ALIGNMENT            7

#define NUM_FDDI_SPECIFIC_COUNTERS		10

#define FDDI_CONFIGURATION_STATE		0
#define FDDI_UPSTREAM_NODE                      1
#define FDDI_DOWNSTREAM_NODE			2
#define FDDI_FRAME_ERROR_COUNT			3
#define FDDI_FRAMES_LOST_COUNT			4
#define FDDI_RING_MANAGEMENT_STATE		5
#define FDDI_LCT_FAILURE_COUNT			6
#define FDDI_LEM_REJECT_COUNT			7
#define FDDI_LEM_COUNT				8
#define FDDI_L_CONNECTION_STATE			9

typedef	struct _MLID_Reg_ {
	VOID		(*MLIDSendHandler)(struct ECB *);
	INFO_BLOCK	*MLIDControlHandler;
        void            *MLIDSendContext;
        void            *MLIDResourceObj;
        void            *MLIDModuleHandle;
} MLID_Reg, *PMLID_Reg, MLID_REG;

/*
 * reset PRAGMA to normal after packing above structures.
 */
#pragma	pack()

struct	_SHARED_DATA_;
struct 	_DRIVER_DATA_;
struct 	_FRAME_DATA_;
struct	_TCB_;

typedef struct _TSM_CONFIG_LIMITS_ {
        UINT8   *MinNodeAddress;
        UINT8   *MaxNodeAddress;
        UINT32  MinRetries;
        UINT32  MaxCRetries;
        UINT32  NumberFrames;
} TSM_CONFIG_LIMITS;

/*
 * TSM parameter block. Only for TSMs which assume that the HSM is
 * MSM based.
 */
typedef struct	_TSM_PARM_BLOCK_ {
	UINT32			MediaParameterSize;
	TSM_CONFIG_LIMITS	*MediaConfigLimits;
	MEON_STRING		**MediaFrameDescriptTable;
	UINT8			(*MediaProtocolIDArray)[6];
	UINT8			*MediaIDArray;
	UINT8			*MediaHeaderSizeArray;
	UINT32			(_cdecl ** MediaSendRoutineArray)(
					struct _SHARED_DATA_ *sharedData,
					ECB *, struct _TCB_ *tcb);
	UINT32			MediaAdapterDataSpaceSize;
	struct _MEDIA_DATA_	*MediaAdapterPtr;
	ODISTAT			(_cdecl * MediaAdjustPtr)(struct _FRAME_DATA_
					*frameData);
	ODISTAT			(_cdecl * MediaInitPtr)(struct
					_DRIVER_DATA_ *driverData,
					struct _FRAME_DATA_ *frameData);
	ODISTAT			(_cdecl * MediaResetPtr)(struct
					_DRIVER_DATA_ *driverData,
					struct _FRAME_DATA_ *frameData);
	ODISTAT			(_cdecl * MediaShutdownPtr)(struct
					_DRIVER_DATA_ *driverData,
					struct _FRAME_DATA_ *frameData, UINT32
					shutdownType);
	void			(_cdecl * MediaSendPtr)(ECB *ecb,
					CONFIG_TABLE *configTable);
	struct _TCB_		*(_cdecl * MediaGetNextSendPtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE **configTable, UINT32
					*PacketSize, void **PhysTcb);
	ODISTAT			(_cdecl * MediaAddMulticastPtr)(struct
					_DRIVER_DATA_ *driverData,
					NODE_ADDR *McAddress);
	ODISTAT			(_cdecl * MediaDeleteMulticastPtr)(struct
					_DRIVER_DATA_ *driverData, NODE_ADDR
					*McAddress);
	ODISTAT			(_cdecl * MediaNodeOverridePtr)(struct
					_FRAME_DATA_ *frameData, MEON mode);
	ODISTAT			(_cdecl * MediaAdjustNodeAddressPtr)(
					struct _FRAME_DATA_ *frameData);
	ODISTAT			(_cdecl * MediaSetLookAheadSizePtr)(struct
					_DRIVER_DATA_ *driverData,
					struct _FRAME_DATA_ *frameData,
					UINT32 size);
	ODISTAT			(_cdecl * MediaPromiscuousChangePtr)(struct
					_DRIVER_DATA_ *driverData,
					struct _FRAME_DATA_ *frameData,
					UINT32 PromiscuousState,
					UINT32 *PromiscuousMode);
	ODISTAT			(_cdecl * MediaRegisterMonitorPtr)(struct
					_DRIVER_DATA_ *driverData,
					struct _FRAME_DATA_ *frameData, void
					(* _cdecl TXRMonRoutine)
					(struct _TCB_ *), BOOLEAN MonitorState);
	ODISTAT			(_cdecl * MediaGetParametersPtr)(
					CONFIG_TABLE	*configTable);
	ODISTAT			(_cdecl * MediaGetMulticastInfo)
					(struct _DRIVER_DATA_  *driverData,
					ECB *MulticastInfoECB);
} TSM_PARM_BLOCK;


typedef struct _ETHER_STATS_ {
    UINT32	TxOKSingleCollisions;
    UINT32	TxOKMultipleCollisions;
    UINT32	TxOKButDeferred;
    UINT32	TxAbortLateCollision;
    UINT32	TxAbortExcessCollision;
    UINT32	TxAbortCarrierSense;
    UINT32	TxAbortExcessiveDeferral;
    UINT32	RxAbortFrameAlignment;
} ETHER_STATS;

#define ETHER_IN_ERRS(E_S) ( (E_S).RxAbortFrameAlignment )
#define ETHER_OUT_ERRS(E_S) \
    ( (E_S).TxAbortCarrierSense + (E_S).TxAbortLateCollision + \
     (E_S).TxAbortExcessCollision + (E_S).TxAbortExcessiveDeferral )

typedef struct _TOKEN_STATS_ {
    UINT32	ACErrorCounter;
    UINT32	AbortDelimiterCounter;
    UINT32	BurstErrorCounter;
    UINT32	FrameCopiedErrorCounter;
    UINT32	FrequenceyErrorCounter;
    UINT32	InternalErrorCounter;
    UINT32	LastRingStatus;
    UINT32	LineErrorCounter;
    UINT32	LostFrameCounter;
    UINT32	TokenErrorCounter;
    /*
     * "CounterMask1" and addtional counters appear after these counters in
     * the Spec but are omitted.
     */
} TOKEN_STATS;

#define TOKEN_IN_ERRS(T_S) \
    ( (T_S).ACErrorCounter + (T_S).FrameCopiedErrorCounter )
#define TOKEN_OUT_ERRS(T_S) \
    ( (T_S).AbortDelimiterCounter + (T_S).BurstErrorCounter + \
     (T_S).LostFrameCounter )

typedef struct _FDDI_STATS_ {
    UINT32	FConfigurationState;
    UINT32	FUpstreamNodeHighDword;
    UINT32	FUpstreamNodeLowWord;
    UINT32	FDownstreamNodeHighDword;
    UINT32	FDownstreamNodeLowWord;
    UINT32	FFrameErrorCount;
    UINT32	FFrameLostCount;
    UINT32	FRingManagementCount;
    UINT32	FLCTFailureCount;
    UINT32	FLemRejectCount;
    /*
     * "CounterMask1" and addtional counters appear after these counters in
     * the Spec but are omitted here.
     */
} FDDI_STATS;

#define FDDI_IN_ERRS(F_S) ( (F_S).FFrameErrorCount + (F_S).FFrameLostCount )

typedef struct _GENERIC_STATS_ {
	UINT32	Gen_TotalTxPackets;
	UINT32	Gen_TotalRxPackets;
	UINT32	Gen_NoECBs;
	UINT32	Gen_TxTooBig;
	UINT32	Gen_TxTooSmall;
	UINT32	Gen_RxOverflow;
	UINT32	Gen_RxTooBig;
	UINT32	Gen_RxTooSmall;
	UINT32	Gen_TxMiscError;
	UINT32	Gen_RxMiscError;
	UINT32	Gen_TxRetryCount;
	UINT32	Gen_RxCheckSumError;
	UINT32	Gen_RxMisMatchError;
	UINT32	Gen_TotalTxOkByteLow;
	UINT32	Gen_TotalTxOkByteHigh;
	UINT32	Gen_TotalRxOkByteLow;
	UINT32	Gen_TotalRxOkByteHigh;
	UINT32	Gen_TotalGroupAddrTx;
	UINT32	Gen_TotalGroupAddrRx;
	UINT32	Gen_AdapterReset;
	UINT32	Gen_AdapterOPRTimeStamp;
	UINT32	Gen_Qdepth;
	union {
	    ETHER_STATS	Gen_EtherStats;
	    TOKEN_STATS	Gen_TokenStats;
	    FDDI_STATS	Gen_FDDIStats;
	} Gen_MediaSpecific;
} GENERIC_STATS;

#define GEN_IN_ERRS(G_S) \
    ( (G_S).Gen_RxTooBig + (G_S).Gen_RxTooSmall + (G_S).Gen_RxMiscError + \
     (G_S).Gen_RxOverflow + (G_S).Gen_RxCheckSumError + \
     (G_S).Gen_RxMisMatchError )
#define GEN_OUT_ERRS(G_S) \
    ( (G_S).Gen_TxTooBig + (G_S).Gen_TxTooSmall + (G_S).Gen_TxMiscError )


#define	NUM_ODI_IOCTLS				15

#define ODI_IOCTL_GetConfiguration          	0
#define ODI_IOCTL_GetStatistics             	1
#define ODI_IOCTL_AddMulticastAddress           2
#define ODI_IOCTL_DeleteMulticastAddress        3
#define ODI_IOCTL_Reserved0                     4
#define ODI_IOCTL_Shutdown                  	5
#define ODI_IOCTL_Reset                     	6
#define ODI_IOCTL_Reserved1                     7
#define ODI_IOCTL_Reserved2                     8
#define ODI_IOCTL_SetLookAheadSize              9
#define ODI_IOCTL_PromiscuousChange             10
#define ODI_IOCTL_RegisterMonitor		11
#define ODI_IOCTL_Reserved3                     12
#define ODI_IOCTL_Reserved4                     13
#define ODI_IOCTL_Management                	14

#define	NOT_PERIODIC				0
#define	PERIODIC				1

#define	LSLReturnRcvECB				lsl_free_ecb

typedef struct _GROUP_ADDR_LIST_NODE_ {
	NODE_ADDR	GRP_ADDR;
	UINT16		GRP_ADDR_COUNT;
} GROUP_ADDR_LIST_NODE;

/*
 * DEBUG stuff
 */
#define ODI_DEBUG			/* REMOVE THIS */
extern int odi_debug_printf();
#ifdef DEBUG
# define ODI_DEBUG
#endif
#ifdef ODI_DEBUG
extern int odi_debug;
# define _ODI_DPRINT(n) (odi_debug < n) ? 1 : odi_debug_printf
#else
# define _ODI_DPRINT(n) (1) ? 1 : odi_debug_printf
#endif
#define ODI_DPRINT1 _ODI_DPRINT(1)
#define ODI_DPRINT2 _ODI_DPRINT(2)
#define ODI_DPRINT3 _ODI_DPRINT(3)

#endif
