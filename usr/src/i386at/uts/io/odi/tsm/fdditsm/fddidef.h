#ifndef	_IO_ODI_TSM_FDDIDEF_H
#define	_IO_ODI_TSM_FDDIDEF_H

#ident	"@(#)fddidef.h	2.1"
#ident	"$Header$"

#ifdef _KERNEL_HEADERS

#include <io/odi/msm/msmstruc.h>

#elif defined(_KERNEL)

#include <sys/msmstruc.h>

#endif /* _KERNEL_HEADERS */

/*
 *      Defines
 */

/*
 * These are used for indexing into a specific Frame-Type.
 */

#define	FDDI_8022_INTERNALID	0
#define	FDDI_SNAP_INTERNALID	1

/*
 * These are the NOVELL assigned Frame IDs.
 */

#define	FDDI_8022_ID	20
#define	FDDI_SNAP_ID	23

/*
 * This defines the size of each Frame-Types MAC Header.
 */

#define	SIZE_FDDI		16 /*14*/
#define	SIZE_FDDISNAP		21 /*17*/

/*
 * These are used for supporting Assembly HSMs.	
 */

#define	FDDI_DESCRIPT_LEN		10
#define	FDDI_SNAP_DESCRIPT_LEN		9

/*
 * PktHdr0 Constants.
 */
#define	TOKEN_TYPE_MASK		0xCF 	/*Token type mask.*/
#define	TOKEN_TYPE_NONE		0x00	/*No token is required.*/
#define	TOKEN_TYPE_RESTRICT	0x10	/*Restricted Token used.*/
#define	TOKEN_TYPE_UNRESTRICT	0x20	/*Unrestricted Token used.*/
#define	TOKEN_TYPE_BOTH		0x30	/*Restricted & Unrestricted.*/

#define	SYNCH_FRAME		0x08	/*Synchronous frame TX.*/
#define	IMM_MODE		0x04	/*Ignore ring_operation.*/
#define	SEND_FIRST		0x02	/*Frame is first sent.*/
#define	BEACON_FRAME		0x01	/*Send only in tx_beacon state.*/

/*
 * PktHdr1 Equates.
 */
#define	SEND_LAST		0x40	/*Release token after this*/
					/*frame is sent.*/
#define	APPEND_CRC		0x20	/*Adapter append CRC.*/

#define	TOKEN_SEND_MASK		0xE7	/*Token send mask.*/
#define	TOKEN_SEND_NO_TOKEN	0x00	/*No token released.*/
#define	TOKEN_SEND_UNRESTRICT	0x08	/*Unrestricted token released.*/
#define	TOKEN_SEND_RESTRICT	0x10	/*Restricted token released.*/
#define	TOKEN_SEND_SAME		0x18	/*Save as receive released.*/

#define	EXTRA_FS_MASK		0xF8	/*Extra Frame State mask.*/
#define	EXTRA_NONE		0x00	/*TR RR II II.*/
#define	EXTRA_RR		0x01	/*TR RR RR II.*/
#define	EXTRA_SR		0x05	/*TR RR SR II.*/
#define	EXTRA_RS		0x01	/*TR RR RS II.*/
#define	EXTRA_SS		0x06	/*TR RR SS II.*/
#define	EXTRA_RT		0x03	/*TR RR RT II.*/
#define	EXTRA_ST		0x07	/*TR RR ST II.*/

/*
 * This is the Maximum size a header can be.
 */
#define	MAX_MAC_HEADER		sizeof (MEDIA_HEADER) + SOURCE_MAX_SIZE	

/*
 * This is the Maximum size a packet can be.
 */
#define MAX_PACKET_SIZE		4500 - 9

#define	FC_LLC_FRAME		0x51		/*LLC Frame Control.*/
#define  FC_DATA_FRAME          0x10
#define	SOURCE_ROUTING_BIT	0x01		/*Routing field exists.*/
#define	SOURCE_SIZE_MASK	0x1F		/*Routing Size Mask.*/
#define	TX_8022_SNAP		0x03AAAAh	/*SNAP DSAP, SSAP, CTRL0.*/
#define	SNAP_INFO_SIZE		5		/*SNAP header info size.*/
#define	SOURCE_MAX_SIZE		30

#define	MULTICASTBIT		0x01
#define	GROUPBIT		0x01
#define	LOCALBIT		0x02
#define	BROADCAST		0xffff

/*
 * These bits define 802.2 types.
 */
#define	S_OR_U_FORMAT		0x01
#define	U_FORMAT		0x02

/*
 * v3.00 Assembly Statistics Table
 */
#define	STATISTICSMASK		0x0D2813FF
#define	FDDISTATSCOUNT		10
#define	LONGCOUNTER		0x00
#define	LARGECOUNTER		0x01

#define	NUMBER_OF_PROMISCUOUS_COUNTERS	32
#define	MAX_MULTICAST		32

/*
 * ECB_DataLength bit assignmets for pipeline and ecb aware adapters.
 */
#define	HSM_HAS_BEEN_IN_GETRCB	0x80000000		/* bit 31 */
#define	PSTK_WANTS_FRAME	0x40000000		/* bit 30 */
#define	HSM_TOLD_TO_SKIP	0x007f0000		/* bits 23 through 16 */
#define	HSM_TOLD_TO_COPY	0x0000ffff		/* bits 15 through 0 */

/*
 *
 *      TypeDefs
 *
 */

typedef struct
{
	UINT8	*MinNodeAddress;
	UINT8	*MaxNodeAddress;
	UINT32	MinRetries;
	UINT32	MaxRetries;
	UINT32	NumberFrames;
}
LANConfigurationLimitStructure;

typedef struct	_MEDIA_HEADER_
{
	UINT8		MH_FrameControl;
	NODE_ADDR	MH_Destination;
	NODE_ADDR	MH_Source;
	UINT8		MH_DSAP;
	UINT8		MH_SSAP;
	UINT8		MH_Ctrl0;
	UINT8		MH_SNAP [5];
}
MEDIA_HEADER;

typedef	struct	_TCB_FRAGMENTSTRUCT_
{
	UINT32		TCB_FragmentCount;
	FRAGMENTSTRUCT	TCB_Fragment;
}TCB_FRAGMENTSTRUCT;

typedef	struct _TCB_
{
	UINT32			TCB_DriverWS[3];
	UINT32			TCB_DataLen;
	TCB_FRAGMENTSTRUCT	*TCB_FragStruct;
	UINT32			TCB_MediaHeaderLen;
	MEDIA_HEADER		TCB_MediaHeader;
	UINT8			TCB_SRPad[SOURCE_MAX_SIZE];
}TCB;

typedef struct	_MEDIA_DATA_	{
	UINT32			MaxMulticastAddresses;
	GROUP_ADDR_LIST_NODE		*MulticastAddressList;
	UINT32			MulticastAddressCount;
	TCB			*TCBHead;
	TCB			*TCBList;
	MLID_STATS_TABLE	*NewStatsPtr;
	void			(*TransmitRoutine)(TCB *);
	UINT32			PromiscuousMode;
	UINT8			PromiscuousCounters[NUMBER_OF_PROMISCUOUS_COUNTERS];
	UINT32			RxStatus;
}MEDIA_DATA;

typedef	struct	_M_ADAPTER_DS_	
{
	UINT32	MADS_MulticastCount;
}
M_ADAPTER_DS;

/*
 *
 *      Prototype Definitions
 *
 */

UINT32 CInitializeFDDITSM(
		MODULE_HANDLE *moduleHandle,
		SCREEN_HANDLE	*screenID
);
void CRemoveFDDITSM(
		void
);
ODISTAT	CFDDITSMRegisterHSM(
		DRIVER_PARM_BLOCK	*DriverParameterBlock,
		CONFIG_TABLE		**configTable
);
GROUP_ADDR_LIST_NODE *FDDIFindAddressInMCTable(
		SHARED_DATA		*sharedData,
		NODE_ADDR		*mcAddress
);
ODISTAT	FDDIMediaAdjust(
		FRAME_DATA		*frameData
);
ODISTAT	FDDIMediaInit(
		DRIVER_DATA	*driverData,
		FRAME_DATA		*frameData
);
ODISTAT	FDDIMediaReset(
		DRIVER_DATA	*driverData,
		FRAME_DATA		*frameData
);
ODISTAT	FDDIMediaShutdown(
		DRIVER_DATA	*driverData,
		FRAME_DATA		*frameData,
		UINT32			shutdownType
);
void FDDIMediaSend(
		ECB			*ecb,
		CONFIG_TABLE		*configTable
);
ODISTAT	FDDIMediaAddMulticast(
		DRIVER_DATA		*driverData,
		NODE_ADDR		*McAddress
);
ODISTAT	FDDIMediaDeleteMulticast(
		DRIVER_DATA		*driverData,
		NODE_ADDR		*McAddress
);
ODISTAT	FDDIMediaNodeOverride(
		FRAME_DATA		*frameData,
		MEON			mode
);
ODISTAT	FDDIMediaAdjustNodeAddress(
		FRAME_DATA		*frameData
);
ODISTAT	FDDIMediaSetLookAheadSize(
		DRIVER_DATA		*driverData,
		FRAME_DATA		*frameData,
		UINT32			sizea
);
ODISTAT	FDDIMediaPromiscuousChange(
		DRIVER_DATA		*driverData,
		FRAME_DATA		*frameData,
		UINT32			PromiscuousState,
		UINT32			*PromiscuousMode
);
ODISTAT	FDDIMediaRegisterMonitor(
		DRIVER_DATA	*driverData,
		FRAME_DATA		*frameData,
		void			(*TXRMonRoutine)(TCB *),
		BOOLEAN			MonitorState
);

ODISTAT	FDDIMediaGetParameters(
		CONFIG_TABLE		*configTable
);
ODISTAT FDDIMediaGetMulticastInfo(
		struct _DRIVER_DATA_	*driverData,
		ECB			*MulticastInfoECB
);
void FDDIBuildASMStatStrings(
		StatTableEntry		*tableEntry
);
ODISTAT	CFDDITSMUpdateMulticast(
		DRIVER_DATA		*driverData
);
UINT32 FDDICMediaSend8022(
		SHARED_DATA		*sharedData,
		ECB			*ecb,
		TCB			*tcb
);
UINT32 FDDICMediaSendSNAP(
		SHARED_DATA		*sharedData,
		ECB			*ecb,
		TCB			*tcb
);
TCB *CFDDITSMGetNextSend(
		DRIVER_DATA		*driverData,
		CONFIG_TABLE		**configTable,
		UINT32			*lengthToSend,
		void			**TCBPhysicalPtr
);
void CFDDITSMFastSendComplete(
		DRIVER_DATA		*driverData,
		TCB			*tcb,
		UINT32			transmitStatus
);
void CFDDITSMSendComplete(
		DRIVER_DATA		*driverData,
		TCB			*tcb,
		UINT32			transmitStatus
);
RCB *CFDDITSMGetRCB(
		DRIVER_DATA		*driverData,
		UINT8			*packetHdr,
		UINT32			pktSize,
		UINT32			rcvStatus,
		UINT32			*startByte,
		UINT32			*numBytes
);
RCB *CFDDITSMProcessGetRCB(
		DRIVER_DATA		*driverData,
		RCB			*rcb,
		UINT32			pktSize,
		UINT32			rcvStatus,
		UINT32			newRcbSize
);
RCB *CFDDITSMFastProcessGetRCB(
		DRIVER_DATA		*driverData,
		RCB			*rcb,
		UINT32			pktSize,
		UINT32			rcvStatus,
		UINT32			newRcbSize
);
void CFDDITSMFastRcvComplete(
		DRIVER_DATA		*driverData,
		RCB			*rcb
);
void CFDDITSMFastRcvCompleteStatus(
		DRIVER_DATA		*driverData,
		RCB			*rcb,
		UINT32			packetLength,
		UINT32			packetStatusa
);
UINT32	CFDDITSMGetHSMIFLevel(
		void
);

void CFDDITSMRcvComplete(
		DRIVER_DATA		*driverData,
		RCB			*rcb
);
void CFDDITSMRcvCompleteStatus(
		DRIVER_DATA		*driverData,
		RCB			*rcb,
		UINT32			packetLength,
		UINT32			packetStatus
);
void CFDDITSMECBRcvComplete(
		DRIVER_DATA		*driverData,
		ECB			*ecb
);
void CFDDITSMPipelineRcvComplete(
		DRIVER_DATA		*driverData,
		ECB			*ecb,
		UINT32			packetLength,
		UINT32			packetStatus
);

#endif	/* _IO_TSM_FDDITSM_FDDIDEF_H */
