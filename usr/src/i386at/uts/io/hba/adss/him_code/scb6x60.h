#ident	"@(#)kern-pdi:io/hba/adss/him_code/scb6x60.h	1.4.2.1"

/* Adaptec SCB Manager definitions used by both the OS specific code that
   invokes the Hardware Interface Module (HIM) and the 6X60 device driver code
   itself.  Note that because there is ONLY source level compatability between
   these two components, there is considerable flexibility in the definition
   of OS specific data structure fields, functions and procedures to maximize
   implementation efficiency.  The use of macros for 6X60 register IO
   functions are a good example of this approach.  For each of the other data
   structures used, you may ADD new fields at will. */

#define REGISTER register

#if !defined(DBG)
#define DBG 0
#endif

#if (DBG)
#define PRIVATE
#else
#define PRIVATE STATIC
#endif

#if defined(EZ_SCSI)
typedef BOOL BOOLEAN;
typedef unsigned long PHYSICAL_ADDRESS;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif

#if defined(NETWARE)
#define FALSE 0
#define TRUE !FALSE
#define VOID void
typedef unsigned char BOOLEAN;
typedef unsigned long PHYSICAL_ADDRESS;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif

#if defined(OS_2)
typedef unsigned char BOOLEAN;
typedef unsigned long PHYSICAL_ADDRESS;
#endif

#if defined(SCO_UNIX)
#define FALSE 0
#define TRUE !FALSE
#define VOID void
typedef unsigned char BOOLEAN;
typedef char CHAR;
typedef int INT;
typedef unsigned long PHYSICAL_ADDRESS;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif

#if defined(USL_UNIX)
#ifndef FALSE
#define FALSE 0
#define TRUE !FALSE
#endif
#define VOID void
typedef unsigned char BOOLEAN;
typedef char CHAR;
typedef int INT;
typedef unsigned long PHYSICAL_ADDRESS;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif

/* The SCSI Control Block (SCB), the basic IO request packet used to
   communicate work from the OS specific code to the 6X60 HIM and to
   return completion status. */

typedef struct _SCB {
   struct _SCB *chain;			/* This MUST be the first element! */
   ULONG length;
   VOID *osRequestBlock;
   struct _SCB *linkedScb;
   UCHAR function;
   UCHAR scbStatus;
   USHORT flags;
   UCHAR targetStatus;
   UCHAR scsiBus;
   UCHAR targetID;
   UCHAR lun;
   UCHAR queueTag;
   UCHAR tagType;
   UCHAR cdbLength;
   UCHAR senseDataLength;
   UCHAR *cdb;
   UCHAR *senseData;
   UCHAR *dataPointer;
   ULONG dataLength;
   ULONG dataOffset;
   PHYSICAL_ADDRESS segmentAddress;
   ULONG segmentLength;
   ULONG transferLength;
   ULONG transferResidual;
   ULONG provisionalTransfer;
#ifdef USL_UNIX
	unsigned char	scsi_cmd[12];	/* SCSI Command			*/
	int		adapter;	/* adapter this scb is for	*/
	unsigned short	c_active;	/* marks this SCB as busy	*/
	struct sb	*c_bind;	/* associated SCSI block	*/
	struct sg_array	*c_sg_p;	/* pointer to s/g list		*/
	struct _SCB	*c_next;	/* pointer to next SCB struct   */
	int		tm_id;		/* timeout id			*/
#endif
#if defined(SCO_UNIX)
   CHAR use_flag;
   CHAR smad_num;
   INT sgCount;
#endif
} ADSS_SCB;

/* Values defined for the SCB 'function' field. */

#if defined(NETWARE)
#define SCB_IO_CONTROL		0x00
#define SCB_EXECUTE		0x02
#define SCB_RECEIVE_EVENT	0x03
#define SCB_BUS_DEVICE_RESET	0x04
#define SCB_SHUTDOWN		0x07
#define SCB_FLUSH		0x08
#define SCB_ABORT_REQUESTED	0x10
#define SCB_RELEASE_RECOVERY	0x11
#define SCB_SCSI_RESET		0x12
#define SCB_TERMINATE_IO	0x14
#define SCB_SUSPEND_IO		0x81
#define SCB_RESUME_IO		0x82
#endif

#if    defined(CHICAGO) || defined(EZ_SCSI) || defined(OS_2) \
    || defined(SCO_UNIX) || defined(USL_UNIX) || defined(WINDOWS_NT)
#define SCB_EXECUTE		0x00
#define SCB_IO_CONTROL		0x02
#define SCB_RECEIVE_EVENT	0x03
#define SCB_SHUTDOWN		0x07
#define SCB_FLUSH		0x08
#define SCB_ABORT_REQUESTED	0x10
#define SCB_RELEASE_RECOVERY	0x11
#define SCB_SCSI_RESET		0x12
#define SCB_BUS_DEVICE_RESET	0x13
#define SCB_TERMINATE_IO	0x14
#define SCB_SUSPEND_IO		0x81
#define SCB_RESUME_IO		0x82
#endif

/* Values defined for the SCB 'status' field. */

#define SCB_PENDING			0x00
#define SCB_COMPLETED_OK		0x01
#define SCB_ABORTED			0x02
#define SCB_ABORT_FAILURE		0x03
#define SCB_ERROR			0x04
#define SCB_BUSY			0x05
#define SCB_INVALID_SCSI_BUS		0x07
#define SCB_TIMEOUT			0x09
#define SCB_SELECTION_TIMEOUT		0x0A
#define SCB_MESSAGE_REJECTED		0x0D
#define SCB_SCSI_BUS_RESET		0x0E
#define SCB_PARITY_ERROR		0x0F
#define SCB_REQUEST_SENSE_FAILURE	0x10
#define SCB_DATA_OVERRUN		0x12
#define SCB_BUS_FREE			0x13
#define SCB_PROTOCOL_ERROR		0x14
#define SCB_INVALID_LENGTH		0x15
#define SCB_INVALID_LUN			0x20
#define SCB_INVALID_TARGET_ID		0x21
#define SCB_INVALID_FUNCTION		0x22
#define SCB_ERROR_RECOVERY		0x23
#define SCB_TERMINATED			0x24
#define SCB_TERMINATE_IO_FAILURE	0x25

#define SCB_SENSE_DATA_VALID		0x80

#define SCB_STATUS(status) ((status) & ~SCB_SENSE_DATA_VALID)

/* Values defined for the SCB 'flags' field */

#define SCB_ENABLE_TAGGED_QUEUING	0x0002
#define SCB_DISABLE_DISCONNECT		0x0004
#define SCB_DISABLE_NEGOTIATIONS	0x0008
#define SCB_DISABLE_AUTOSENSE		0x0020
#define SCB_DATA_IN			0x0040
#define SCB_DATA_OUT			0x0080
#define SCB_NO_QUEUE_FREEZE		0x0100
#define SCB_ENABLE_CACHE		0x0200
#define SCB_VIRTUAL_SCATTER_GATHER	0x4000
#define SCB_DISABLE_DMA			0x8000

/* Values defined for logging errors to the SCB Manager. */

#define SCB_ERROR_PARITY		0x0001
#define SCB_ERROR_UNEXPECTED_DISCONNECT	0x0002
#define SCB_ERROR_INVALID_RESELECTION	0x0003
#define SCB_ERROR_BUS_TIMEOUT		0x0004
#define SCB_ERROR_PROTOCOL		0x0005
#define SCB_ERROR_HOST_ADAPTER		0x0006
#define SCB_ERROR_REQUEST_TIMEOUT	0x0007
#define SCB_WARNING_NO_INTERRUPTS	0x0008
#define SCB_ERROR_FIRMWARE		0x0009
#define SCB_WARNING_FIRMWARE		0x000A
#define SCB_WARNING_DMA_HANG		0x8001

/* The Logical Unit Control Block (LUCB) structure.  It's principal use is to
   provide queues for the management of active and pending SCSI requests
   (SCB's). */

typedef struct {
   BOOLEAN busy;
   ADSS_SCB *queuedScb;
   ADSS_SCB *activeScb;
} LUCB;

/* Data structure to record debugging trace information (when enabled) */

typedef struct {
   UCHAR sequence;
   UCHAR event;
   UCHAR scsiPhase;
   UCHAR busID;
   USHORT data[2];
   UCHAR scsiSig;
   UCHAR sStat0;
   UCHAR sStat1;
   UCHAR sStat2;
   UCHAR dmaCntrl0;
   UCHAR dmaStat;
   UCHAR fifoStat;
   UCHAR currentState;
} TRACE_LOG;

/* Values defined for the Trace Log 'event' field */

#define BUS_FREE 0x00
#define TARGET_REQ 0x01
#define DATA_PHASE_PIO 0x02
#define DATA_PHASE_DMA 0x03
#define QUIESCE_DMA 0x04
#define UPDATE_DATA_POINTER 0x05
#define MESSAGE_IN 0x10
#define NEGOTIATE_SDTR 0x11
#define BITBUCKET_AND_ABORT 0x12
#define RESET_DEVICE 0x17
#define RESET_BUS 0x1F
#define INITIATE_IO 0x40
#define RESELECTION 0x41
#define SELECTION 0x42
#define BUS_RESET 0x4F
#define ISR 0x07F
#define INTERRUPT 0x0FF

/* The 6X60 IO registers are viewed as an array of UCHAR, USHORT or ULONG as
   appropriate and are referenced by an IO space "pointer" to this array. */

typedef union {
   UCHAR ioPort[32];
   struct {
      UCHAR scsiSeq;
      UCHAR sXfrCtl0;
      UCHAR sXfrCtl1;
      UCHAR scsiSig;
      UCHAR scsiRate;
      union {
	UCHAR scsiID;
	UCHAR selID;
      } no1;
      UCHAR scsiDat;
      UCHAR scsiBus;
      UCHAR stCnt0;
      UCHAR stCnt1;
      UCHAR stCnt2;
      union {
	UCHAR clrSInt0;
	UCHAR sStat0;
      } no2;
      union {
	UCHAR clrSInt1;
	UCHAR sStat1;
      } no3;
      UCHAR sStat2;
      union {
	UCHAR scsiTest;
	UCHAR sStat3;
      } no4;
      union {
	UCHAR clrSErr;
	UCHAR sStat4;
      } no5;
      UCHAR sIMode0;
      UCHAR sIMode1;
      UCHAR dmaCntrl0;
      UCHAR dmaCntrl1;
      UCHAR dmaStat;
      UCHAR fifoStat;
      UCHAR dmaData;
      UCHAR ioPortX57;
      union {
	UCHAR brstCntrl;
	UCHAR dmaData32;
      } no6;
      UCHAR ioPortX59;
      UCHAR portA;
      UCHAR portB;
      UCHAR rev;
      UCHAR stack;
      UCHAR test;
      UCHAR ID;
   } no7;
} AIC6X60_REG;

#ifdef USL_UNIX
#define	NULL		0

#define	SStat4		(int)&hacb->baseAddress->no7.no5.sStat4
#define	ClrSErr		(int)&hacb->baseAddress->no7.no5.clrSErr
#define	DmaData32	(int)&hacb->baseAddress->no7.no6.dmaData32
#define	BrstCntrl	(int)&hacb->baseAddress->no7.no6.brstCntrl
#define	SStat0		(int)&hacb->baseAddress->no7.no2.sStat0
#define	ClrSInt0	(int)&hacb->baseAddress->no7.no2.clrSInt0
#define	SStat1		(int)&hacb->baseAddress->no7.no3.sStat1
#define	ClrSInt1	(int)&hacb->baseAddress->no7.no3.clrSInt1
#define	SStat3		(int)&hacb->baseAddress->no7.no4.sStat3
#define	ScsiTest	(int)&hacb->baseAddress->no7.no4.scsiTest
#define	SelID		(int)&hacb->baseAddress->no7.no1.selID
#define	ScsiID		(int)&hacb->baseAddress->no7.no1.scsiID
#define	StCnt0		(int)&hacb->baseAddress->no7.stCnt0
#define	StCnt1		(int)&hacb->baseAddress->no7.stCnt1
#define	StCnt2		(int)&hacb->baseAddress->no7.stCnt2
#define	ScsiBus		(int)&hacb->baseAddress->no7.scsiBus
#define	ScsiDat		(int)&hacb->baseAddress->no7.scsiDat
#define	IoPortX59	(int)&hacb->baseAddress->no7.ioPortX59
#define	PortA		(int)&hacb->baseAddress->no7.portA
#define	PortB		(int)&hacb->baseAddress->no7.portB
#define	Rev		(int)&hacb->baseAddress->no7.rev
#define	Stack		(int)&hacb->baseAddress->no7.stack
#define	Test		(int)&hacb->baseAddress->no7.test
#define	Id		(int)&hacb->baseAddress->no7.ID
#define	ScsiSeq		(int)&hacb->baseAddress->no7.scsiSeq
#define	SIMode0		(int)&hacb->baseAddress->no7.sIMode0
#define	SIMode1		(int)&hacb->baseAddress->no7.sIMode1
#define	DmaCntrl0	(int)&hacb->baseAddress->no7.dmaCntrl0
#define	DmaCntrl1	(int)&hacb->baseAddress->no7.dmaCntrl1
#define	DmaStat		(int)&hacb->baseAddress->no7.dmaStat
#define	DmaData		(int)&hacb->baseAddress->no7.dmaData
#define	FifoStat	(int)&hacb->baseAddress->no7.fifoStat
#define	IoPortX57	(int)&hacb->baseAddress->no7.ioPortX57
#define	SXfrCtl0	(int)&hacb->baseAddress->no7.sXfrCtl0
#define	SXfrCtl1	(int)&hacb->baseAddress->no7.sXfrCtl1
#define	ScsiSig		(int)&hacb->baseAddress->no7.scsiSig
#define	ScsiRate	(int)&hacb->baseAddress->no7.scsiRate
#define SStat0		(int)&hacb->baseAddress->no7.no2.sStat0
#define SStat2		(int)&hacb->baseAddress->no7.sStat2
#define	SStat3		(int)&hacb->baseAddress->no7.no4.sStat3
#endif

/* The Host Adapter Control Block (HACB) structure used to record the current
   state of SCSI bus "conversations" in progress. */

typedef struct _HACB {
   ULONG length;
   AIC6X60_REG *baseAddress;
   UCHAR scsiPhase;
   UCHAR ownID;
   UCHAR busID;
   UCHAR lun;
   union {
      UCHAR adapterConfiguration;
      struct {
         BOOLEAN fastSCSI:1;
         BOOLEAN defaultConfiguration:1;
         BOOLEAN initialReset:1;
         BOOLEAN noDisconnect:1;
         BOOLEAN checkParity:1;
         BOOLEAN initiateSDTR:1;
         BOOLEAN useDma:1;
         BOOLEAN synchronous:1;
      } no9;
   } no8;
   union {
      UCHAR currentState;
      struct {
         BOOLEAN bitBucket:1;
         BOOLEAN disconnectOK:1;
         BOOLEAN dmaActive:1;
         BOOLEAN msgParityError:1;
         BOOLEAN parityError:1;
         BOOLEAN queuesFrozen:1;
         BOOLEAN deferredIsrActive:1;
         BOOLEAN isrActive:1;
      } no13;
   } no14;
   ULONG disableINT;
   ADSS_SCB *deferredScb;
   ADSS_SCB *eligibleScb;
   ADSS_SCB *queueFreezeScb;
   ADSS_SCB *resetScb;
   union {
      ULONG nexus[  (  2 * sizeof(VOID *) + sizeof(PHYSICAL_ADDRESS)
                     + 3 * sizeof(ULONG) + 16 * sizeof(UCHAR))
                  / sizeof(ULONG)];
      struct {
         ADSS_SCB *activeScb;
         UCHAR *dataPointer;
         ULONG dataLength;
         PHYSICAL_ADDRESS segmentAddress;
         ULONG segmentLength;
         ULONG dataOffset;
         UCHAR msgOutLen;
         UCHAR msgOut[7];
         UCHAR msgInLen;
         UCHAR msgIn[7];
      } no12;
   } no15;
   UCHAR targetStatus;
   UCHAR reservedForAlignment1;
   UCHAR syncCycles[8];
   UCHAR syncOffset[8];
   ULONG cQueuedScb;
   ULONG cActiveScb;
   UCHAR negotiateSDTR;
   struct {
      UCHAR extMsgCode;
      UCHAR extMsgLength;
      UCHAR extMsgType;
      UCHAR transferPeriod;
      UCHAR reqAckOffset;
   } sdtrMsg;
   UCHAR requestSenseCdb[6];
   UCHAR sStat0;
   UCHAR maskedSStat0;
   UCHAR sStat1;
   UCHAR maskedSStat1;
   USHORT selectTimeLimit;
   UCHAR sXfrCtl1Image;
   BOOLEAN irqConnected;
   UCHAR clockPeriod;
   UCHAR IRQ;
   UCHAR dmaChannel;
   UCHAR revision;
   UCHAR dmaBusOnTime;
   UCHAR dmaBusOffTime;
   ULONG signature;
   ULONG scsiCount;
#if defined(CHICAGO) || defined(WINDOWS_NT)
   SCB internalScb;
#endif
#if (DBG_TRACE)
   ULONG traceCount;
   ULONG traceIndex;
   TRACE_LOG traceLog[128];
   BOOLEAN traceEnabled;
#endif
#if defined(EZ_SCSI) || defined(SCO_UNIX)
   LUCB lucb[8][8];
#endif
#if defined(NETWARE)
   LUCB lucb[64];
#endif
#ifdef USL_UNIX
	unsigned char		pad;		/* keep things aligned	     */
	unsigned char		ha_id;		/* Host adapter SCSI id      */
	int			ha_vect;	/* Interrupt vector number   */
	int			ha_npend;	/* # of jobs sent to HA      */
	char			*ha_name;	/* name of driver	     */
	struct scsi_lu		*ha_dev;	/* Logical unit queues	     */
	struct sg_array		*ha_sgnext;	/* next free s/g structure   */
	struct sg_array		*ha_sglist;	/* list of s/g structures    */
	LUCB			*ha_lucb;	/* local unit queues	     */
	ADSS_SCB		*ha_scb_next;	/* next command block	     */
	ADSS_SCB		*ha_scb;	/* Controller command blocks */
	ADSS_SCB		*ha_poll;	/* polling SCB for init time */
#ifndef PDI_SVR42
	void		*ha_sg_lock;	  /* Lock for s/g structs */
	void		*ha_scb_lock;	  /* Lock for command blocks */
	void		*ha_hba_lock;	  /* Lock for controller */
	int		ha_opri;	  /* spl for HIM lock */
#endif
#endif
} HACB;

#ifdef USL_UNIX
#define	Nexus			no15.nexus
#define	IsrActive		no14.no13.isrActive
#define	DeferredIsrActive	no14.no13.deferredIsrActive
#define	QueuesFrozen		no14.no13.queuesFrozen
#define	ParityError		no14.no13.parityError
#define	MsgParityError		no14.no13.msgParityError
#define	DmaActive		no14.no13.dmaActive
#define	DisconnectOK		no14.no13.disconnectOK
#define	BitBucket		no14.no13.bitBucket
#define	MsgIn			no15.no12.msgIn
#define	MsgInLen		no15.no12.msgInLen
#define	MsgOut			no15.no12.msgOut
#define	MsgOutLen		no15.no12.msgOutLen
#define	DataOffset		no15.no12.dataOffset
#define	SegmentLength		no15.no12.segmentLength
#define	SegmentAddress		no15.no12.segmentAddress
#define	DataLength		no15.no12.dataLength
#define	DataPointer		no15.no12.dataPointer
#define	ActiveScb		no15.no12.activeScb
#define FastSCSI		no8.no9.fastSCSI
#define DefaultConfiguration	no8.no9.defaultConfiguration
#define InitialReset		no8.no9.initialReset
#define NoDisconnect		no8.no9.noDisconnect
#define CheckParity		no8.no9.checkParity
#define InitiateSDTR		no8.no9.initiateSDTR
#define UseDma			no8.no9.useDma
#define Synchronous		no8.no9.synchronous
#endif

#define BUS_FREE_PHASE 0xFF		/* Special token */
#define DISCONNECTED 0xFF

#define WATCHDOG_DMA_HANG 50000		/* Microseconds before checking DMA */
#define WATCHDOG_POLL_IRQ 10000		/* Microseconds before polling IRQ */

/* Public functions and procedures used for communications between the OS
   specific code and the 6X60 HIM.  Some are defined within the HIM code
   (HIM6X60.C) and the others are the responsibility of the implementor for a
   particular OS/hardware platform.  Note that in cases where the
   functionality is a superset of that provided by the OS (e.g. NETWARE, OS/2
   and Unix do not support the concept of a "deferred ISR") it is possible to
   use macros to eliminate the need to implement the function in the OS
   specific code.*/

UCHAR HIM6X60AbortSCB(HACB *hacb, ADSS_SCB *scb, ADSS_SCB *scbToAbort);

VOID HIM6X60AssertINT(HACB *hacb);

#if defined(EZ_SCSI)
#define HIM6X60CompleteSCB(hacb, scb)
#else
VOID HIM6X60CompleteSCB(HACB *hacb, ADSS_SCB *scb);
#endif

#if   defined(EZ_SCSI) || defined(NETWARE) || defined(OS_2) \
   || defined(SCO_UNIX) || defined(USL_UNIX)
#define HIM6X60DeferISR(hacb, deferredProcedure) deferredProcedure(hacb)
#elif defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60DeferISR(hacb, deferredProcedure) \
   ScsiPortNotification(CallEnableInterrupts, hacb, deferredProcedure)
#else
VOID HIM6X60DeferISR(HACB *hacb, VOID (*deferredProcedure)(HACB *hacb));
#endif

#if defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60Delay(microseconds) ScsiPortStallExecution(microseconds)
#elif defined(SCO_UNIX)
#define HIM6X60Delay(microseconds) suspend((microseconds + 99) /100)
#elif defined(USL_UNIX)
#define HIM6X60Delay(microseconds) drv_usecwait(microseconds)
#else
VOID HIM6X60Delay(ULONG microseconds);
#endif

VOID HIM6X60DisableINT(HACB *hacb);

VOID HIM6X60DmaProgrammed(HACB *hacb);

BOOLEAN HIM6X60EnableINT(HACB *hacb);

#ifdef USL_UNIX
VOID HIM6X60Event(HACB *hacb, UCHAR event);
#else
VOID HIM6X60Event(HACB *hacb, UCHAR event, ...);
#endif

#if   defined(EZ_SCSI) || defined(NETWARE) || defined(OS_2) \
   || defined(SCO_UNIX) || defined(USL_UNIX)
#define HIM6X60ExitDeferredISR(hacb, deferredProcedure) deferredProcedure(hacb)
#elif defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60ExitDeferredISR(hacb, deferredProcedure) \
   ScsiPortNotification(CallDisableInterrupts, hacb, deferredProcedure)
#else
VOID HIM6X60ExitDeferredISR(HACB *hacb, VOID (*deferredProcedure)(HACB *hacb));
#endif

BOOLEAN HIM6X60FindAdapter(AIC6X60_REG *baseAddress);

#if defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60FlushDMA(hacb) ScsiPortFlushDma(hacb)
#elif defined(EZ_SCSI)
#define HIM6X60FlushDMA(hacb)
#else
VOID HIM6X60FlushDMA(HACB *hacb);
#endif

BOOLEAN HIM6X60GetConfiguration(HACB *hacb);

#if defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60GetLUCB(hacb, scsiBus, targetID, lun) \
   ScsiPortGetLogicalUnit(hacb, scsiBus, targetID, lun)
#elif defined(EZ_SCSI) || defined(SCO_UNIX)
#define HIM6X60GetLUCB(hacb, scsiBus, targetID, lun) &hacb->lucb[targetID][lun]
#else
LUCB *HIM6X60GetLUCB(HACB *hacb, UCHAR scsiBus, UCHAR targetID, UCHAR lun);
#endif

#if defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60GetPhysicalAddress(hacb, scb, virtualAddress, bufferOffset, \
                                  length) \
   ScsiPortConvertUlongToPhysicalAddress(0); *length = 0x00FFFFFF
#elif defined(EZ_SCSI)
#define HIM6X60GetPhysicalAddress(hacb, scb, virtualAddress, bufferOffset, \
                                  length) \
   (PHYSICAL_ADDRESS) 0; *length = 0x00FFFFFF;
#elif defined(USL_UNIX)
#define HIM6X60GetPhysicalAddress(hacb, scb, virtualAddress, bufferOffset, \
                                  length) \
	0; *length = 0;
#else
PHYSICAL_ADDRESS HIM6X60GetPhysicalAddress(HACB *hacb, ADSS_SCB *scb,
                                           VOID *virtualAddress,
                                           ULONG bufferOffset, ULONG *length);
#endif

USHORT HIM6X60GetStackContents(HACB *hacb, VOID *stackContents,
                               USHORT maxStackSize);

#if defined(CHICAGO) || defined(EZ_SCSI) || defined(NETWARE) || defined(WINDOWS_NT)
#define HIM6X60GetVirtualAddress(hacb, scb, virtualAddress, bufferOffset, \
                                 length) \
   virtualAddress; *length = *length
#else
VOID *HIM6X60GetVirtualAddress(HACB *hacb, ADSS_SCB *scb, VOID *virtualAddress,
                               ULONG bufferOffset, ULONG *length);
#endif

BOOLEAN HIM6X60Initialize(HACB *hacb);

BOOLEAN HIM6X60IRQ(HACB *hacb);

BOOLEAN HIM6X60ISR(HACB *hacb);

#if defined(EZ_SCSI) || defined(SCO_UNIX) || defined(USL_UNIX)
#define HIM6X60LogError(hacb, scb, scsiBus, targetID, lun, errorClass, errorID)
#else
VOID HIM6X60LogError(HACB *hacb, ADSS_SCB *scb, UCHAR scsiBus, UCHAR targetID,
                     UCHAR lun, USHORT errorClass, USHORT errorID);
#endif

#if defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60MapDMA(hacb, scb, virtualAddress, bufferOffset, length, \
                      memoryWrite) \
   ScsiPortIoMapTransfer(hacb, scb->osRequestBlock, virtualAddress, length)
#elif defined(EZ_SCSI)
#define HIM6X60MapDMA(hacb, scb, virtualAddress, bufferOffset, length, \
                      memoryWrite)
#else
VOID HIM6X60MapDMA(HACB *hacb, ADSS_SCB *scb, VOID *virtualAddress,
                   PHYSICAL_ADDRESS physicalAddress, ULONG length,
                   BOOLEAN memoryWrite);
#endif

UCHAR HIM6X60QueueSCB(HACB *hacb, ADSS_SCB *scb);

BOOLEAN HIM6X60ResetBus(HACB *hacb, UCHAR scsiBus);

UCHAR HIM6X60TerminateSCB(HACB *hacb, ADSS_SCB *scb, ADSS_SCB *scbToTerminate);

#if defined(CHICAGO) || defined(WINDOWS_NT)
#define HIM6X60Watchdog(hacb, watchdogProcedure, microseconds) \
   ScsiPortNotification(RequestTimerCall, hacb, watchdogProcedure, \
                        microseconds)
#elif defined(EZ_SCSI) || defined(SCO_UNIX) || defined(USL_UNIX)
#define HIM6X60Watchdog(hacb, watchdogProcedure, microseconds)
#else
VOID HIM6X60Watchdog(HACB *hacb, VOID (*watchdogProcedure)(HACB *hacb),
                     ULONG microseconds);
#endif

/* 'Event' definitions to use with HIM6X60Event */

#define EVENT_SCSI_BUS_RESET 0x02

/* Macros for convenience */

#define LAST(array) ((sizeof(array) / sizeof(array[0])) - 1)
#define ADSS_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define ADSS_MIN(x, y) (((x) < (y)) ? (x) : (y))

#if (DBG_TRACE)
#define DISABLE_TRACE hacb->traceEnabled = FALSE
#define ENABLE_TRACE hacb->traceEnabled = TRUE
#define TRACE(hacb, event, data0, data1) debugTrace(hacb, event, data0, data1)
#else
#define DISABLE_TRACE
#define ENABLE_TRACE
#define TRACE(hacb, event, data0, data1)
#endif

/* OS specific macros to provide access to the 6X60 IO registers and to allow
   the manipulation of a PHYSICAL_ADDRESS type which may vary in different
   machine and OS environments.  The conventions adopted for conditional
   compilation for supported operating systems are (at present) NETWARE,
   WINDOWS_NT, OS_2, SCO_UNIX and USL_UNIX. */

#if defined(CHICAGO) || defined(WINDOWS_NT)

#define BLOCKINDWORD(port, buffer, count) \
   ScsiPortReadPortBufferUlong((ULONG *) &hacb->baseAddress->port, \
                               (ULONG *) buffer, count)

#define BLOCKINPUT(port, buffer, count) \
   ScsiPortReadPortBufferUchar(&hacb->baseAddress->port, buffer, count)

#define BLOCKINWORD(port, buffer, count) \
   ScsiPortReadPortBufferUshort((USHORT *) &hacb->baseAddress->port, \
                                (USHORT *) buffer, count)

#define BLOCKOUTDWORD(port, buffer, count) \
   ScsiPortWritePortBufferUlong((ULONG *) &hacb->baseAddress->port, \
                                (ULONG *) buffer, count)

#define BLOCKOUTPUT(port, buffer, count) \
   ScsiPortWritePortBufferUchar(&hacb->baseAddress->port, buffer, count)

#define BLOCKOUTWORD(port, buffer, count) \
   ScsiPortWritePortBufferUshort((USHORT *) &hacb->baseAddress->port, \
                                 (USHORT *) buffer, count)
#if (DBG)
VOID DbgBreakPoint(VOID);
#define BREAKPOINT DbgBreakPoint()
#else
#define BREAKPOINT
#endif

#define INPUT(port) \
   ScsiPortReadPortUchar(&hacb->baseAddress->port)

#define OUTPUT(port, value) \
   ScsiPortWritePortUchar(&hacb->baseAddress->port, value)

#define PHYSICAL_TO_ULONG(physicalAddress) \
   ScsiPortConvertPhysicalAddressToUlong(physicalAddress)

#define UPDATE_PHYSICAL_ADDRESS(physicalAddress, count)

#define ZERO_PHYSICAL_ADDRESS(physicalAddress)

#endif

#if defined (EZ_SCSI)

int repinsd(int port, UCHAR *buffer, int count);
int repinsb(int port, UCHAR *buffer, int count);
int repinsw(int port, UCHAR *buffer, int count);
int repoutsd(int port, UCHAR *buffer, int count);
int repoutsb(int port, UCHAR *buffer, int count);
int repoutsw(int port, UCHAR *buffer, int count);

#define BLOCKINDWORD(port, buffer, count) \
   repinsd((int) ((long) &hacb->baseAddress->port), buffer, (int) count)

#define BLOCKINPUT(port, buffer, count) \
   repinsb((int) ((long) &hacb->baseAddress->port), buffer, (int) count)

#define BLOCKINWORD(port, buffer, count) \
   repinsw((int) ((long) &hacb->baseAddress->port), buffer, (int) count)

#define BLOCKOUTDWORD(port, buffer, count) \
   repoutsd((int) ((long) &hacb->baseAddress->port), buffer, (int) count)

#define BLOCKOUTPUT(port, buffer, count) \
   repoutsb((int) ((long) &hacb->baseAddress->port), buffer, (int) count)

#define BLOCKOUTWORD(port, buffer, count) \
   repoutsw((int) ((long) &hacb->baseAddress->port), buffer, (int) count)

#define BREAKPOINT

#define INPUT(port) _inp((int) ((long) &hacb->baseAddress->port))

#define OUTPUT(port, value) _outp((int) ((long) &hacb->baseAddress->port), \
                                  (int) (value))

#define PHYSICAL_TO_ULONG(physicalAddress) physicalAddress

#define UPDATE_PHYSICAL_ADDRESS(physicalAddress, count) \
   physicalAddress += count

#define ZERO_PHYSICAL_ADDRESS(physicalAddress) physicalAddress = 0

#endif

#if defined(NETWARE)

#define BLOCKINDWORD(port, buffer, count) \
   repinsd((ULONG *) &hacb->baseAddress->port, buffer, count)

#define BLOCKINPUT(port, buffer, count) \
   repinsb(&hacb->baseAddress->port, buffer, count)

#define BLOCKINWORD(port, buffer, count) \
   repinsw((USHORT *) &hacb->baseAddress->port, buffer, count)

#define BLOCKOUTDWORD(port, buffer, count) \
   repoutsd((ULONG *) &hacb->baseAddress->port, buffer, count)

#define BLOCKOUTPUT(port, buffer, count) \
   repoutsb(&hacb->baseAddress->port, buffer, count)

#define BLOCKOUTWORD(port, buffer, count) \
   repoutsw((USHORT *) &hacb->baseAddress->port, buffer, count)

#define BREAKPOINT

#define INPUT(port) inp(&hacb->baseAddress->port)

#define OUTPUT(port, value) outp(&hacb->baseAddress->port, value)

#define PHYSICAL_TO_ULONG(physicalAddress) physicalAddress

#define UPDATE_PHYSICAL_ADDRESS(physicalAddress, count) \
   physicalAddress += count

#define ZERO_PHYSICAL_ADDRESS(physicalAddress) physicalAddress = 0

#endif

#if defined(OS_2)

VOID breakpoint(VOID);
UCHAR inp(UCHAR *port);
VOID outp(UCHAR *port, UCHAR value);
VOID repinsb(UCHAR *port, VOID *buffer, ULONG count);
VOID repinsd(ULONG *port, VOID *buffer, ULONG count);
VOID repinsw(USHORT *port, VOID *buffer, ULONG count);
VOID repoutsb(UCHAR *port, VOID *buffer, ULONG count);
VOID repoutsd(ULONG *port, VOID *buffer, ULONG count);
VOID repoutsw(USHORT *port, VOID *buffer, ULONG count);

#define BLOCKINDWORD(port, buffer, count) \
   repinsd((ULONG *) &hacb->baseAddress->port, buffer, count)

#define BLOCKINPUT(port, buffer, count) \
   repinsb(&hacb->baseAddress->port, buffer, count)

#define BLOCKINWORD(port, buffer, count) \
   repinsw((USHORT *) &hacb->baseAddress->port, buffer, count)

#define BLOCKOUTDWORD(port, buffer, count) \
   repoutsd((ULONG *) &hacb->baseAddress->port, buffer, count)

#define BLOCKOUTPUT(port, buffer, count) \
   repoutsb(&hacb->baseAddress->port, buffer, count)

#define BLOCKOUTWORD(port, buffer, count) \
   repoutsw((USHORT *) &hacb->baseAddress->port, buffer, count)

#if (DBG)
#define BREAKPOINT breakpoint()
#else
#define BREAKPOINT
#endif

#define INPUT(port) inp(&hacb->baseAddress->port)

#define OUTPUT(port, value) outp(&hacb->baseAddress->port, value)

#define PHYSICAL_TO_ULONG(physicalAddress) physicalAddress

#define UPDATE_PHYSICAL_ADDRESS(physicalAddress, count) \
   physicalAddress += count

#define ZERO_PHYSICAL_ADDRESS(physicalAddress) physicalAddress = 0

#endif

#if defined(SCO_UNIX)

INT inb(INT port);
INT outb(INT port, INT value);
INT repinsd(INT port, CHAR *buffer, INT count);
INT repinsb(INT port, CHAR *buffer, INT count);
INT repinsw(INT port, CHAR *buffer, INT count);
INT repoutsd(INT port, CHAR *buffer, INT count);
INT repoutsb(INT port, CHAR *buffer, INT count);
INT repoutsw(INT port, CHAR *buffer, INT count);

#define BLOCKINDWORD(port, buffer, count) \
   repinsd((INT) &hacb->baseAddress->port, buffer, (INT) count)

#define BLOCKINPUT(port, buffer, count) \
   repinsb((INT) &hacb->baseAddress->port, buffer, (INT) count)

#define BLOCKINWORD(port, buffer, count) \
   repinsw((INT) &hacb->baseAddress->port, buffer, (INT) count)

#define BLOCKOUTDWORD(port, buffer, count) \
   repoutsd((INT) &hacb->baseAddress->port, buffer, (INT) count)

#define BLOCKOUTPUT(port, buffer, count) \
   repoutsb((INT) &hacb->baseAddress->port, buffer, (INT) count)

#define BLOCKOUTWORD(port, buffer, count) \
   repoutsw((INT) &hacb->baseAddress->port, buffer, (INT) count)

#define BREAKPOINT

#define INPUT(port) inb((INT) &hacb->baseAddress->port)

#define OUTPUT(port, value) outb((INT) &hacb->baseAddress->port, (CHAR) value)

#define PHYSICAL_TO_ULONG(physicalAddress) physicalAddress

#define UPDATE_PHYSICAL_ADDRESS(physicalAddress, count) \
   physicalAddress += count

#define ZERO_PHYSICAL_ADDRESS(physicalAddress) physicalAddress = 0

#endif

#if defined(USL_UNIX)

#define BLOCKINDWORD(port, buffer, count) \
   repinsd(port, buffer, count)

#define BLOCKINPUT(port, buffer, count) \
   repinsb(port, buffer, count)

#define BLOCKINWORD(port, buffer, count) \
   repinsw(port, buffer, count)

#define BLOCKOUTDWORD(port, buffer, count) \
   repoutsd(port, buffer, count)

#define BLOCKOUTPUT(port, buffer, count) \
   repoutsb(port, buffer, count)

#define BLOCKOUTWORD(port, buffer, count) \
   repoutsw(port, buffer, count)

#define BREAKPOINT

#define INPUT(port) inb(port)

#define OUTPUT(port, value) outb(port, value)

#define PHYSICAL_TO_ULONG(physicalAddress) physicalAddress

#define UPDATE_PHYSICAL_ADDRESS(physicalAddress, count) \
   physicalAddress += count

#define ZERO_PHYSICAL_ADDRESS(physicalAddress) physicalAddress = 0

#endif
