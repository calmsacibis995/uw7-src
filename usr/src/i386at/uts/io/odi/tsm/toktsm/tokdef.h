#ifndef _IO_ODI_TSM_TOKDEF_H
#define _IO_ODI_TSM_TOKDEF_H

#ident	"@(#)tokdef.h	2.1"
#ident	"$Header$"

#ifdef  _KERNEL_HEADERS

#include <io/odi/odi.h>
#include <io/odi/msm/msmstruc.h>

#elif defined(_KERNEL)

#include <sys/odi.h>
#include <sys/msmstruc.h>

#endif

/*
 * These are used for indexing into a specific Frame-Type.
 */
#define  TOKEN_INTERNALID  0
#define  TSNAP_INTERNALID  1

/*
 * Brouter Request Definitions.
 */
#define  BROUTER_SUPPORT               0x00
#define  SELECT_T_BRIDGING             0x01
#define  SELECT_SR_BRIDGING            0x02
#define  SELECT_PROMISCUOUS_MODE       0x03
#define  UPDATE_ADDRESS_FILTER_TABLE   0x04

#define  PMT_NONE                      0
#define  PMT_TRANSPARENT               1
#define  PMT_SOURCE_ROUTINE            2
#define  PMT_TSR_TRANSPARENT           3

#define  FTA_CLEAR_TABLE               0
#define  FTA_ADD_ADDRESSES             1
#define  FTA_DELETE_ADDRESSES          2

/*
 * These are the NOVELL assigned Frame IDs.
 */
#define	TOKEN_ID    			4
#define	TSNAP_ID     			11

#define	SIZE_TOKEN			17
#define	SIZE_TOKENSNAP			22
#define SIZE_MACTOK			14

#define TOKEN_DESCRIPT_LEN		10
#define TSNAP_DESCRIPT_LEN		15

/*
 * This is the Maximum size a packet can be.
 */
#define MAX_PACKET_SIZE			17960

#define MULTICASTBIT			0x80
#define GROUPBIT			0x80
#define LOCALBIT			0x40
#define BROADCAST			0xffffffff

/*
 * These bits define 802.2 types.
 */
#define S_OR_U_FORMAT			0x01
#define U_FORMAT			0x02

#define FC_NON_MAC_FRAME		0x40
#define SOURCE_ROUTING_BIT		0x80
#define SOURCE_SIZE_MASK		0x1f
#define TOKENSNAP_INFO			0x03AAAA
#define SNAP_INFO_SIZE			5
#define SOURCE_MAX_SIZE			30

/*
 * This is the Maximum size a header can be. snap header is max.
 */
#define MAX_MAC_HEADER			sizeof (MEDIA_HEADER) +	\
					SOURCE_MAX_SIZE 

/*
 * v3.00 Assembly Statistics Table.
 */
#define STATISTICSMASK			0x0D380000
#define TOKENSTATSCOUNT			13
#define LONGCOUNTER			0x00
#define LARGECOUNTER			0x01

#define NUMBER_OF_PROMISCUOUS_COUNTERS	32
#define MAX_MULTICAST			32

/*
 * ECB_DataLength bit assignmets for pipeline and ecb aware adapters.
 */
#define HSM_HAS_BEEN_IN_GETRCB  	0x80000000	/* bit 31 */
#define PSTK_WANTS_FRAME		0x40000000	/* bit 30 */
#define HSM_TOLD_TO_SKIP		0x007f0000	/* bits23 through 16 */
#define HSM_TOLD_TO_COPY		0x0000ffff	/* bits15 through 0 */

typedef struct {
        UINT8		*MinNodeAddress;
	UINT8		*MaxNodeAddress;
	UINT32		MinRetries;
	UINT32		MaxRetries;
	UINT32		NumberFrames;
} LANConfigurationLimitStructure;

typedef struct	_MEDIA_HEADER_ {
	UINT8		   	MH_AccessControl;
	UINT8		   	MH_FrameControl;
	NODE_ADDR   	   	MH_Destination;
	NODE_ADDR	   	MH_Source;
	UINT8		   	MH_DSAP;
	UINT8	   	   	MH_SSAP;
	UINT8		   	MH_Ctrl0;
	UINT8		   	MH_SNAP [5];
} MEDIA_HEADER;

typedef	struct	_TCB_FRAGMENTSTRUCT_ {
	UINT32			TCB_FragmentCount;
	FRAGMENTSTRUCT		TCB_Fragment;
} TCB_FRAGMENTSTRUCT;

typedef	struct _TCB_ {
	UINT32			TCB_DriverWS[3];
	UINT32			TCB_DataLen;
	TCB_FRAGMENTSTRUCT	*TCB_FragStruct;
	UINT32			TCB_MediaHeaderLen;
	MEDIA_HEADER		TCB_MediaHeader;
	UINT8			TCB_SRPad[SOURCE_MAX_SIZE];
} TCB , *PTCB;

typedef	struct	_FUNCTIONAL_TABLE_ {
	UINT8			FunctionalBit[32];
} FUNCTIONAL_TABLE, *PFUNCTIONAL_TABLE;

typedef struct	_MEDIA_DATA_	{
	UINT32			MaxMulticastAddresses;
	GROUP_ADDR_LIST_NODE		*MulticastAddressList;
	UINT32			MulticastAddressCount;
	FUNCTIONAL_TABLE    	FunctionalAddressList;
	TCB			*TCBHead;
	TCB			*TCBList;
	MLID_STATS_TABLE	*NewStatsPtr;
	void			(*TransmitRoutine)(TCB *);
	UINT32			PromiscuousMode;
	UINT8			PromiscuousCounters
					[NUMBER_OF_PROMISCUOUS_COUNTERS];
	UINT32			RxStatus;
} MEDIA_DATA;

ODISTAT		CTokenTSMRegisterHSM(DRIVER_PARM_BLOCK *DriverParameterBlock,
			CONFIG_TABLE **configTable);
GROUP_ADDR_LIST_NODE	*TokFindAddressInMCTable(SHARED_DATA *sharedData,
			UINT8 *mcAddress);
ODISTAT		MediaAdjust(FRAME_DATA *frameData);
ODISTAT		MediaInit(DRIVER_DATA *driverData, FRAME_DATA *frameData);
ODISTAT		MediaReset(DRIVER_DATA *driverData, FRAME_DATA *frameData);
ODISTAT		MediaShutdown(DRIVER_DATA *driverData, FRAME_DATA *frameData,
			UINT32 shutdownType);
void	   	MediaSend(ECB *ecb, CONFIG_TABLE *configTable);
ODISTAT		MediaAddMulticast(DRIVER_DATA *driverData,
			NODE_ADDR *McAddress);
ODISTAT		MediaDeleteMulticast(DRIVER_DATA *driverData,
			NODE_ADDR *McAddress);
ODISTAT		MediaNodeOverride(FRAME_DATA *frameData, MEON mode);
ODISTAT		MediaAdjustNodeAddress(FRAME_DATA *frameData);
ODISTAT		MediaSetLookAheadSize(DRIVER_DATA *driverData,
			FRAME_DATA *frameData, UINT32 size);
ODISTAT		MediaPromiscuousChange(DRIVER_DATA *driverData, FRAME_DATA
			*frameData, UINT32 PromiscuousState,
			UINT32 *PromiscuousMode);
ODISTAT		MediaRegisterMonitor(DRIVER_DATA *driverData,
			FRAME_DATA *frameData, void (*TXRMonRoutine)(TCB *),
			BOOLEAN MonitorState);
ODISTAT		MediaGetParameters(CONFIG_TABLE *configTable);
ODISTAT		MediaGetMulticastInfo(struct _DRIVER_DATA_ *driverData,
			ECB *MulticastInfoECB);
void     	TokenBuildASMStatStrings(StatTableEntry *tableEntry);
ODISTAT  	CTokenTSMUpdateMulticast(DRIVER_DATA *driverData);
UINT32   	CMediaSend8022(SHARED_DATA *sharedData, ECB *ecb, TCB *tcb);
UINT32   	CMediaSendSNAP(SHARED_DATA *sharedData, ECB *ecb, TCB *tcb);
TCB		*CTokenTSMGetNextSend(DRIVER_DATA *driverData,
			CONFIG_TABLE **configTable, UINT32 *lengthToSend,
			void **TCBPhysicalPtr);
void		CTokenTSMFastSendComplete(DRIVER_DATA *driverData, TCB *tcb,
			UINT32 transmitStatus);
void		CTokenTSMSendComplete(DRIVER_DATA *driverData, TCB *tcb,
			UINT32 transmitstatus);
RCB 		*CTokenTSMProcessGetRCB(DRIVER_DATA *driverData, RCB *rcb,
			UINT32 pktSize, UINT32 rcvStatus, UINT32 newRcbSize);
RCB 		*CTokenTSMFastProcessGetRCB(DRIVER_DATA *driverData,
			RCB *rcb, UINT32 pktSize, UINT32 rcvStatus,
			UINT32 newRcbSize);
RCB		*CTokenTSMGetRCB(DRIVER_DATA *driverData, UINT8 *packetHdr,
			UINT32 pktSize, UINT32 rcvStatus, UINT32 *startByte,
			UINT32 *numBytes);
void		CTokenTSMFastRcvComplete(DRIVER_DATA *driverData,
			RCB *rcb);
void		CTokenTSMFastRcvCompleteStatus(DRIVER_DATA *driverData,
			RCB *rcb, UINT32 packetLength, UINT32 packetStatus);
UINT32		CTokenTSMGetHSMIFLevel();
void		CTokenTSMRcvComplete(DRIVER_DATA *driverData, RCB *rcb);
void		CTokenTSMRcvCompleteStatus(DRIVER_DATA *driverData,
			RCB *rcb, UINT32 packetLength, UINT32 packetStatus);

#endif /* _IO_ODI_TSM_TOKDEF_H */
