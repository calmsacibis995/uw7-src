#ifndef	_IO_ODISR_ROUTE_H
#define	_IO_ODISR_ROUTE_H

#ident	"@(#)route.h	2.1"
#ident	"$Header$"

#ifdef _KERNEL_HEADERS

#include <io/odi/odi.h>

#else 

#include <sys/odi.h>

#endif

#define	RT_DFLT_TIME_COUNT 10L		/* Default # of secs for counter */
#define	RT_AES_TIME_PERIOD 1000L	/* period for AES Events, 1000 ms. */

/*
 * Source Routing Control Field Equates.
 */
#define	SR_CONTROL_FLD_SZ 0x2		/* Size of the Control Field 0 & 1 */

/*
 * Control Field 0.
 */
#define	SR_NON_BRDCAST_MASK	0x1F	/* non Broadcast Indicators mask */
#define	SR_BRDCAST_IND_MASK	0xE0	/* Broadcast Indicators mask */
#define	SR_LENGTH_MASK	 	0x1F	/* Length Mask */
#define	SR_ROUT_SZ_MASK		0x1F	/* Routing Size Mask */

#define	SR_DEST_BRDCAST_BIT	0x80	/* MSB Dest. Node Address Bit */
					/* for Broadcasts */
#define	SR_GEN_BRDCAST_MASK	0x80	/* General Broadcast Mask */
#define	SR_LIM_BRDCAST_MASK	0xC0	/* Limited Broadcast Mask */

#define	SR_SROUTE_BROADCAST	0xC0	/* Single Route Broadcast */
#define	SR_AROUTE_BROADCAST	0x80	/* All Routes Broadcast */
#define	SR_NON_BROADCAST	0x00	/* Non Broadcast, ie. specific Route */

/*
 * Control Field 1
 */
#define	SR_DIRECT_IND	0x80	/*Direction Indicator 	*/
#define	SR_FRAME_SZ_MASK 0x70	/* Largest Frame Bits	*/
#define	SR_LARGEST_FRAME 0x70	/* Largest Frame size 	*/



/* Source Routing Information Field Equates	*/

#define	RT_DFLT_TRA_COUNT     5L /*Default This Ring Alternate Count*/
#define	RT_MAX_ROUT_INFO_SZ 30L /*Max Routing Info Size, 13 hops*2+4=30	*/

#define	SR_RII_ADDR_INDIC 0x80	/*RII Indicator for Source Address	*/
#define	SR_RII_ADDR_IND_MASK 0x7F /*RII Indicator Mask for Source Address*/


/*	Source Routing routine Control Functions	*/

#define	SRCFN_LOADBRD	0L	/*Load Board	*/
#define	SRCFN_UNLOADBRD	1L	/*Unload Board	*/
#define	SRCFN_CLR_SR	2L	/*Clear Source Routing Table for Board	*/
#define	SRCFN_CHG_UKNDA	3L	/*Change Unknown Destination Address Route*/
#define	SRCFN_CHG_GENBRD 4L 	/*Change General Broadcast Route*/
#define	SRCFN_CHG_MULBRD 5L 	/*Change Multicast Broadcast Route*/
#define	SRCFN_CHG_BRDRSP 6L 	/*Change Broadcast Response Type*/
#define	SRCFN_CHG_SRTIMER 7L	
		/*Change Source Routing Update Table Timer	*/
#define	SRCFN_REMOV_NODE 8L /*	Remove Node from Source Route Table*/


/*Frame Type (ID) Definitons */

#define	ODI_FRAMEID_TOKEN_RING		4
#define	ODI_FRAMEID_TOKEN_RING_SNAP	11
#define	ODI_FRAMEID_FDDI_8022		20
#define	ODI_FRAMEID_FDDI_SNAP		23


/*========[ Type Definitions ]================*/

/*
//disable Bothersome warnings.
//warning C4309: 'cast' : truncation of constant value
*/

#pragma warning(disable:4309)	

/*	Set PRAGMA to pack these structures	*/

#pragma	pack(1)


/*Source Routine Field	for SRT (TRN has 7 Designator Fields)	*/

typedef	struct	_SRField_ {
	UINT8	SRCntlFld[2];	/*Control Field	*/
	UINT16	SRDsgnFld[12];	/*Designator Fields*/
}SRFIELD, *PSRFIELD;


/*	Source Routing Information structure	*/
/*	These nodes are allocated dynamically, ie. on the fly. 
	with aged out Nodes
	being reused rather than returned to the Memory Pool.  A new Node is 
	allocated only when no aged out Nodes can be found.  
	This hopefully keep the database list of nodes small.													*/

typedef	struct	_RouteInfoNode_	{
	struct	_RouteInfoNode_	*pRI_NextNode;
	UINT32	RI_Size;
	UINT8	RI_Address[ADDR_SIZE];
	UINT8	RI_SRField[RT_MAX_ROUT_INFO_SZ];/*SR Field*/
	UINT32	RI_Timer;	/*	Last Receive Timer	*/
	}ROUTEINFONODE, *PROUTEINFONODE;


/*	Board Source Routing Tracking structure	*/

typedef	struct	_Board_SRTrack_	{
	struct	_Board_SRTrack_	*pNextSRTrack;
	UINT32		SRBoard;
		/*Board Number being Source Routed		*/
	UINT8	GenBrd_UnknownDestAddr[2];	
		/*General Broadcast, Unknown Dest Addr	*/
	UINT8	GenBrd_BroadcastFrames[2];	
		/*General Broadcast, Broadcast Frame	*/
	UINT8	GenBrd_MulticastFrames[2];	
		/*General Broadcast, Multicast Frame	*/
	UINT32	SRBoardAge;
	UINT8	Brdcast_Response[2];
	BOOLEAN		Brdcast_ThisRingAlt;
	BOOLEAN	Brdcast_ThisRingOnly;
	BOOLEAN	PrevBrdcast_ThisRingAlt;
	UINT32	Brdcast_TRACfgCount;			
		/*Configured This Ring Alternate Count	*/
	UINT32	Brdcast_TRACurrCount;		
		/*Current This Ring Alternate Count	*/
	ROUTEINFONODE		*pBoardSRDataBase;
}BOARD_SRTRACK, *PBOARD_SRTRACK;
	

/*========[ Function Prototypes ]================*/


/*========[ Global Variables ]================*/


/*	Reset PRAGMA to normal after packing above structures	*/

#pragma	pack()

#define ODISR_HIERINIT 32

#endif		
