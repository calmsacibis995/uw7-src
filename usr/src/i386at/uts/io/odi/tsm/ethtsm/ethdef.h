#ifndef _IO_TSM_ETHTSM_ETHDEF_H
#define _IO_TSM_ETHTSM_ETHDEF_H

#ident	"@(#)ethdef.h	2.1"
#ident	"$Header$"

#ifdef _KERNEL_HEADERS

#include <io/odi/msm/msmstruc.h>

#elif defined(_KERNEL)

#include <sys/msmstruc.h>

#endif

/*
 * This defines the size of each Frame-Types MAC Header.
 */
#define	SIZE_E8023			14
#define	SIZE_E8022			17
#define	SIZE_EII			14
#define	SIZE_ESNAP			22

/*
 * These are used for supporting Assembly HSMs.
 */
#define E8022_DESCRIPT_LEN      	14
#define E8023_DESCRIPT_LEN      	14
#define EII_DESCRIPT_LEN                11
#define ESNAP_DESCRIPT_LEN      	13

/*
 * This is the Maximum size a packet can be.
 */
#define MAX8023LENGTH			1500
#define MAX_PACKET_LENGTH		1514

#define MULTICASTBIT			0x01
#define GROUPBIT			0x01
#define LOCALBIT			0x02

#define MIN_PKT_SIZE			60

#define STATISTICSMASK           	0x0D300003

/*
 * These bits define 802.2 types.
 */
#define S_OR_U_FORMAT			0x01
#define U_FORMAT			0x02

typedef struct	_MEDIA_HEADER_ {
	NODE_ADDR	MH_Destination;
	NODE_ADDR	MH_Source;
	UINT8		MH_Length[2];
	UINT8		MH_DSAP;
	UINT8		MH_SSAP;
	UINT8		MH_Ctrl0;
	UINT8		MH_SNAP[5];
} MEDIA_HEADER;

typedef	struct	_TCB_FRAGMENTSTRUCT_ {
	UINT32		TCB_FragmentCount;
	FRAGMENTSTRUCT	TCB_Fragment;
} TCB_FRAGMENTSTRUCT;

#define NUMBER_OF_PROMISCUOUS_COUNTERS  32

typedef	struct _TCB_ {
	UINT32			TCB_DriverWS[3];
	UINT32			TCB_DataLen;
	TCB_FRAGMENTSTRUCT	*TCB_FragStruct;
	UINT32			TCB_MediaHeaderLen;
	MEDIA_HEADER		TCB_MediaHeader;
} TCB;

typedef struct _MEDIA_DATA_ {
	UINT32			MaxMulticastAddresses;
	GROUP_ADDR_LIST_NODE		*MulticastAddressList;
	UINT32			MulticastAddressCount;
	TCB			*TCBHead;
	TCB			*TCBList;
	MLID_STATS_TABLE	*NewStatsPtr;
	void			(*TransmitRoutine)(TCB *);
	UINT32			PromiscuousMode;
	UINT8		PromiscuousCounters[NUMBER_OF_PROMISCUOUS_COUNTERS];
	UINT32			RxStatus;
} MEDIA_DATA;

ODISTAT		CEtherTSMRegisterHSM(DRIVER_PARM_BLOCK *DriverParameterBlock,
			CONFIG_TABLE **configTable);
RCB 		*CEtherTSMFastProcessGetRCB(DRIVER_DATA *driverData,
			RCB *rcb, UINT32 pktSize, UINT32 rcvStatus,
			UINT32 newRcbSize);
void		CEtherTSMFastSendComplete(DRIVER_DATA *driverData, TCB *tcb,
			UINT32 transmitStatus);
TCB		*CEtherTSMGetNextSend(DRIVER_DATA *driverData,
			CONFIG_TABLE **configTable, UINT32 *lengthToSend,
			void **TCBPhysicalPtr);
RCB		*CEtherTSMGetRCB(DRIVER_DATA *driverData, UINT8 *lookAheadData,
			UINT32 pktSize, UINT32 rcvStatus, UINT32 *startByte,
			UINT32 *numBytes);
RCB 		*CEtherTSMProcessGetRCB(DRIVER_DATA *driverData, RCB *rcb,
			UINT32 pktSize, UINT32 rcvStatus, UINT32 newRcbSize);
void		CEtherTSMFastRcvComplete(DRIVER_DATA *driverData, RCB *rcb);
void		CEtherTSMFastRcvCompleteStatus(DRIVER_DATA *driverData,
			RCB *rcb, UINT32 packetLength, UINT32 packetStatus);
UINT32		CEtherTSMGetHSMIFLevel();
void		CEtherTSMRcvComplete(DRIVER_DATA *driverData, RCB *rcb);
void		CEtherTSMRcvCompleteStatus(DRIVER_DATA *driverData, RCB *rcb,
			UINT32 packetLength, UINT32 packetStatus);
void		CEtherTSMSendComplete(DRIVER_DATA *driverData, TCB *tcb,
			UINT32 transmitStatus);
void		CEtherTSMECBRcvComplete(DRIVER_DATA *driverData, ECB *ecb);
void		CEtherTSMPipelineRcvComplete(DRIVER_DATA *driverData, ECB *ecb,
			UINT32 packetLength, UINT32 packetStatus);
ODISTAT  	CEtherTSMUpdateMulticast(DRIVER_DATA *driverData);
UINT32 		CMediaSendRaw8023(SHARED_DATA *sharedData, ECB *ecb, TCB *tcb);
UINT32 		CMediaSend8022Over8023(SHARED_DATA *sharedData, ECB *ecb,
			TCB *tcb);
UINT32 		CMediaSend8022Snap(SHARED_DATA *sharedData, ECB *ecb, TCB *tcb);
UINT32 		CMediaSendEthernetII(SHARED_DATA *sharedData, ECB *ecb,
			TCB *tcb);
GROUP_ADDR_LIST_NODE *EthFindAddressInMCTable(SHARED_DATA *sharedData,
			UINT8 *mcAddress);
ODISTAT		MediaAdjust(FRAME_DATA *frameData);
ODISTAT		MediaInit(DRIVER_DATA *driverData, FRAME_DATA *frameData);
ODISTAT		MediaReset(DRIVER_DATA *driverData, FRAME_DATA *frameData);
ODISTAT		MediaShutdown(DRIVER_DATA *driverData, FRAME_DATA *frameData,
			UINT32 shutdownType);
void		MediaSend(ECB *ecb, CONFIG_TABLE *configTable);
ODISTAT		MediaAddMulticast(DRIVER_DATA *driverData,
			NODE_ADDR *McAddress);
ODISTAT		MediaDeleteMulticast(DRIVER_DATA *driverData,
			NODE_ADDR *McAddress);
ODISTAT		MediaNodeOverride(FRAME_DATA *frameData, MEON mode);
ODISTAT		MediaAdjustNodeAddress(FRAME_DATA *frameData);
ODISTAT		MediaSetLookAheadSize(DRIVER_DATA *driverData,
			FRAME_DATA *frameData, UINT32 size);
ODISTAT		MediaPromiscuousChange(DRIVER_DATA *driverData,
			FRAME_DATA *frameData, UINT32 PromiscuousState,
			UINT32 *PromiscuousMode);
ODISTAT		MediaRegisterMonitor(DRIVER_DATA *driverData,
			FRAME_DATA *frameData, void (*TXRMonRoutine)(TCB *),
			BOOLEAN MonitorState);
ODISTAT		MediaGetParameters(CONFIG_TABLE *configTable);
ODISTAT		MediaGetMulticastInfo(struct _DRIVER_DATA_ *driverData,
			ECB *MulticastInfoECB);

#endif	/* _IO_TSM_ETHTSM_ETHDEF_H */
