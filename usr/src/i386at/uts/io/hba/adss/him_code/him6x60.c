#ident	"@(#)kern-pdi:io/hba/adss/him_code/him6x60.c	1.3"

/*-----------------------------------------------------------------------------

 		******** PROPRIETARY PROGRAM MATERIAL ********
 		******** (C) 1992, 1993 ADAPTEC, INC. ********

 This source manuscript is proprietary to Adaptec, Inc. and is not to be
 reproduced, used or disclosed except in accordance with Adaptec software
 license.

 Module:	him6x60.c

 Author(s):	Peter Johansson (Congruent Software, Inc.)

 Abstract:	Operating system/platform independent SCSI device driver for
 		the Adaptec AIC-6260 and AIC-6360 host bus controllers.

 Revisions:	10DEC92	PGJ	Initial release.
		08JAN93	PGJ	Don't worry if target ignores ATN after the
				IDENTIFY message (this is how SCSI-1 devices
				reject SDTR); program 6X60 transfer rate
				control register when SDTR message is sent
				(this is in case a synchronous DATA IN phase
				immediately follows MESSAGE OUT); examine 6X60
				Stack to inherit any SDTR agreements
				negotiated by the BIOS or other prior
				controller of the host adapter; don't assert
				SCSI bus RST during initialization unless the
				bus is not free; support fast SCSI and 32-bit
				programmed IO for 6360.
		05FEB93	PGJ	Support 8-bit DMA for 6X60 channel 0 and
				16-bit DMA for 6360 channels 5 through 7; fix
				infinite loops in 'unlinkEligibleScb' and
				'unlinkQueuedScb'; use deferred ISR for
				programmed IO data phases.
		24FEB93	PGJ	Prevent recursion in deferred ISR; during DATA
				OUT programmed IO, don't enable REQINIT until
				both data and SCSI FIFO's are empty
		15MAR93 PGJ	Implement support for SCB flags to suppress
				DMA or SDTR negotiations; don't use DMA for
				automatic REQUEST SENSE; move REQUEST SENSE
				CDB to HACB to avoid potential DS not equal to
				SS problems during interrupt service; add
				count variable to allow nesting of calls to
				disable/enable 6X60 INT signal; implement
				virtual address scatter/gather support
		22MAR93 PGJ	Whenever a selection is started, SXFRCTL1 must
				be reprogrammed to enable the hardware
				selection timer in case the BIOS left it
				disabled
		31MAR93 PGJ	Modify DMA paradigm to support the models used
				by Netware, OS/2 and Unix; set a flag when
				default (hard-coded) configuration is returned
				(e.g. for AHA-1510)
		30APR93 PGJ	New procedure to read 6X60 Stack; add support
				for BUS DEVICE RESET and TERMINATE IO PROCESS
				messages; remove time limits on waits for
				target REQ in programmed IO data phases; add
				data transfer direction flag to HIM6X60MapDMA
				procedural interface; check jumpers or Stack
				to enable 6360 features; fix calculation of
				the data transfer residual; eliminate the need
				for programmed IO when the DMA controller must
				be reprogrammed during a DATA IN phase; power
				down 6360 whenever no SCSI requests are
				outstanding and power it up when IO started;
				revert to asynchronous after BUS DEVICE RESET;
				new SCB_SUSPEND_IO and SCB_RESUME_IO functions;
				DMA bus on/off times configurable
		13MAY93 PGJ	Clear PWRDWN in initialization and before SCSI
				bus RST; ensure INTEN is set whenever a
				selection is started; correct parity error
				recovery logic
		25MAY93 PGJ	Add watchdog logic to detect misconfiguration
				of IRQ level
		25JUN93 PGJ	Add watchdog logic to detect erroneous ATDONE
				during DMA in contention with another channel;
				avoid possibility of infinite loop in DATA IN
				or DATA OUT programmed IO while awaiting REQ;
				correct ABORT and TERMINATE IO PROCESS logic
				when a request is issued during automatic
				REQUEST SENSE; return SCB_INVALID_LUN if a
				target sends MESSAGE REJECT after an IDENTIFY
				message
		21JUL93 PGJ	Fix bug in SCB_RESUME_IO logic when queued
				work must be resumed
		10AUG93 PGJ	Replace 'enable6360' configuration parameter
				with new 'FastSCSI' parameter (obtained from
				bit 4 of the second configuration byte); new
				meanings for Stack configuration bytes if 0x52
				signature is present
		20AUG93	PGJ	In SCSI bus free processing, don't write
				CLRSELDI because the 6X60 may "forget" about
				a reselection in progress
		30AUG93 PGJ	Turn external LED controlled from AHA-1510 and
				AHA-152X on at the start of a selection or
				reselection and off when bus free is detected
		27SEP93 PS	Conditional includes for use with EZ-SCSI
		04OCT93 PGJ	Enhance ABORT and TERMINATE IO PROCESS logic
				to avoid SCSI bus RST whenever possible; fix
				bug so that SDTR agreements are updated in the
				6X60 Stack when the new 0x52 BIOS signature is
				present

-----------------------------------------------------------------------------*/

#if defined(CHICAGO)
#include <memory.h>
#include <miniport.h>
#include <srb.h>
#endif

#if defined(EZ_SCSI)
#include <conio.h>
#include <memory.h>
#include <windows.h>
#endif

#if defined(NETWARE)
#include "string.h"
#endif

#if defined(OS_2)
#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#include <memory.h>
#include <os2.h>
#if (DEBUG_VERSION)
#define DBG TRUE
#else
#define DBG FALSE
#endif
#endif

#if defined(SCO_UNIX)
#include <memory.h>
#if defined(M_S_UNIX)
#include "sys/types.h"
#include "sys/param.h"
#else
#include "../h/types.h"
#include "../h/param.h"
#endif
#if defined(DEBUG)
#define DBG TRUE
#else
#define DBG FALSE
#endif
#endif

#if defined(USL_UNIX)
#endif

#if defined(WINDOWS_NT)
#include <memory.h>
#include <miniport.h>
#include <srb.h>
#endif

#include <io/hba/adss/him_code/aic6x60.h>
#include <io/hba/adss/him_code/scb6x60.h>
#include <io/hba/adss/him_code/him_scsi.h>

#ifdef USL_UNIX
#define	memcmp(src, dest, size) bcmp(src, dest, size)
#define	memcpy(dest, src, length) bcopy(src, dest, length)
#define memset(dest, value, size) adss_set(dest, value, size)
#else
#pragma intrinsic(memcmp, memcpy, memset)
#endif

/* Function prototypes (alphabetical, for easy reference) */

PRIVATE VOID bitbucketAndABORT(HACB *hacb, UCHAR scbStatus);
PRIVATE VOID dataInPIO(HACB *hacb);
PRIVATE VOID dataOutPIO(HACB *hacb);
#ifdef OLD_STUFF
PRIVATE VOID dataPhaseDMA(HACB *hacb);
#endif
PRIVATE VOID debugTrace(HACB *hacb, UCHAR event, ULONG data0, ULONG data1);
PRIVATE VOID deferredIsr(HACB *hacb);
PRIVATE VOID initiateIO(HACB *hacb);
PRIVATE VOID interpretMessageIn(HACB *hacb);
PRIVATE VOID isr(HACB *hacb);
PRIVATE VOID linkScb(ADSS_SCB **queueHead, ADSS_SCB *scb);
PRIVATE VOID linkScbPreemptive(ADSS_SCB **queueHead, ADSS_SCB *scb);
PRIVATE VOID negotiateSDTR(HACB *hacb);
PRIVATE VOID prepareMessageOut(HACB *hacb, UCHAR message);
PRIVATE VOID quiesceDmaAndSCSI(HACB *hacb);
PRIVATE VOID reselection(HACB *hacb);
PRIVATE VOID resetSDTR(HACB *hacb, USHORT busID);
PRIVATE BOOLEAN samePhaseREQuest(HACB *hacb, BOOLEAN pioReady);
PRIVATE VOID ScsiBusFree(HACB *hacb);
PRIVATE VOID ScsiBusReset(HACB *hacb);
PRIVATE VOID selection(HACB *hacb);
PRIVATE VOID targetREQuest(HACB *hacb);
PRIVATE BOOLEAN unlinkScb(ADSS_SCB **queueHead, ADSS_SCB *scb);
PRIVATE VOID updateDataPointer(HACB *hacb);
PRIVATE VOID updateSDTR(HACB *hacb, USHORT busID, UCHAR syncCycles,
                        UCHAR syncOffset, BOOLEAN negotiationsCompleted);
PRIVATE VOID watchdog(HACB *hacb);
/*-----------------------------------------------------------------------------
 This procedure attempts to locate an ISA 6260 or 6360 by examining the base
 IO address supplied.  Return TRUE or FALSE according to whether or not a 6X60
 was detected. */

BOOLEAN HIM6X60FindAdapter(AIC6X60_REG *baseAddress) {

   HACB temporaryHacb;
   HACB *hacb = &temporaryHacb;

   hacb->baseAddress = baseAddress;
   return(   ((INPUT(ScsiSeq) & (ENSELO | ENAUTOATNO | SCSIRSTO)) == 0)
          && ((INPUT(SXfrCtl0)
                   & (SCSIEN | DMAEN | CLRSTCNT | SPIOEN | CLRCH | 0x05)) == 0)
          && ((INPUT(SXfrCtl1) & (BITBUCKET | SWRAPEN | 0X01)) == 0)
          && ((INPUT(SStat0) & (SELDO | SPIORDY)) == 0)
          && (INPUT(SStat2) == SEMPTY)
          && (INPUT(SStat3) == 0)
          && ((INPUT(DmaStat) & (DFIFOFULL | DFIFOEMP | 0x01)) == DFIFOEMP));

}

/*-----------------------------------------------------------------------------
 OS specific code sometimes needs to know the exact configuration of the BIOS
 (flopticals supported, which device numbers have been assigned to the
 stnadard and floptical drives, whether or not greater than 1 Gb support is
 enabled, etc.).  Rather than keep pace with exactly how present (and future)
 versions of the BIOS are storing the information in the Stack, this support
 procedure has been provided so that the user can interpret the Stack
 information directly. */

USHORT HIM6X60GetStackContents(HACB *hacb, VOID *StackContents,
                               USHORT maxStackSize) {

   USHORT StackSize;

   memset(StackContents, 0, (ULONG) maxStackSize);
   StackSize = (INPUT(Rev) == 0) ? 16 : 32;
   if (StackContents != NULL) {
      OUTPUT(DmaCntrl1, (UCHAR) ((StackSize == 16) ? 0 : ENSTK32));
      BLOCKINPUT(Stack, StackContents, ADSS_MIN(StackSize, maxStackSize));
      OUTPUT(DmaCntrl1, 0);
   }
   return(StackSize);

}
/*-----------------------------------------------------------------------------
 This procedure attempts to retrieve configuration information for a 6X60
 located at the base address supplied in the HACB.  It also initializes the
 HACB with constant information in addition to the information retrieved from
 the 6X60.  First check for an Adaptec BIOS signature value in the Stack.  If
 a valid signature is present, the BIOS has executed and stored configuration
 information (as well as the results of any already concluded SDTR
 negotiations) in the Stack and this can be used instead of external jumpers.
 Otherwise, read the PORTA and PORTB registers.  On a 152X plug-in card, these
 are connected to jumpers that determine the configuration.  On a bare-bones
 1510 card (no readable jumpers or BIOS), use hard-coded defaults.  Note that
 if no BIOS has executed, it is assumed that this driver needs to issue an
 initial SCSI bus RST. */

BOOLEAN HIM6X60GetConfiguration(HACB *hacb) {

   UCHAR checksum, config1, config2, scsirate, ScsiRateOffset;
   ULONG i;

   if (hacb->length < sizeof(HACB))
      return(FALSE);
   if (!HIM6X60FindAdapter(hacb->baseAddress))
      return(FALSE);
   OUTPUT(DmaCntrl1, 0);			/* Check Stack for signature */
   hacb->signature = INPUT(Stack);
   if (   hacb->signature == 0x52		/* 6360 BIOS? */
       || hacb->signature == 0x53) {
      checksum = (UCHAR) hacb->signature;	/* Maybe, if checksum OK */
      OUTPUT(DmaCntrl1, ENSTK32 | 0x01);	/* 6360 has bigger Stack */
      for (i = 0x01; i <= 0x1F; i++)
         if (i <= 0x02 || i >= 0x0C && i <= 0x0F || i >= 0x12)
            checksum += INPUT(Stack);	/* Nonvolatile locations, only */
         else
            INPUT(Stack);
      if (checksum == 0xFF)
         OUTPUT(DmaCntrl1, 0x01);	/* Reposition for configuration */
      else
         hacb->signature = 0;		/* Stack fails checksum test */
   } else if (hacb->signature == 0x55)	/* Motherboard 6260? */
      hacb->signature |= (USHORT) INPUT(Stack) << 8;
   else if (hacb->signature == 0) {	/* Possible 152X BIOS? */
      hacb->signature |= ((ULONG) INPUT(Stack)) << 8;
      hacb->signature |= ((ULONG) INPUT(Stack)) << 16;
      hacb->signature |= ((ULONG) INPUT(Stack)) << 24;
      if (     hacb->signature != 0x03020100
            || INPUT(Stack) != 0x04
            || INPUT(Stack) != 0x05
            || INPUT(Stack) != 0x06
            || INPUT(Stack) != 0x07)
         hacb->signature = 0;
   } else
      hacb->signature = 0;
   if (     hacb->signature == 0x52
         || hacb->signature == 0x53
         || hacb->signature == 0x54
         || hacb->signature == 0xAA55) {
      config1 = INPUT(Stack);
      config2 = INPUT(Stack);
   } else {
      config1 = INPUT(PortA);
      config2 = INPUT(PortB);
   }
   if (config2 == 0xFF) {			/* No jumpers available? */
      hacb->DefaultConfiguration = TRUE;
      hacb->ownID = 7;
      hacb->IRQ = 11;
      hacb->dmaChannel = 0;
      hacb->CheckParity = TRUE;
      hacb->FastSCSI = ((hacb->revision = INPUT(Rev)) > 0);
      hacb->NoDisconnect = FALSE;
      hacb->InitiateSDTR = TRUE;
      hacb->UseDma = FALSE;
   } else {					/* OK! Jumpered information */
      hacb->ownID = config1 & 0x07;
      hacb->IRQ = 9 + ((int)(config1 & 0x18) >> 3);
      if ((hacb->dmaChannel = ((int)config1 & 0x60) >> 5) != 0)
         hacb->dmaChannel += 4;
      hacb->CheckParity = ((config1 & 0x80) == 0);
      if ((hacb->revision = INPUT(Rev)) == 0)
         hacb->FastSCSI = FALSE;
      else if (hacb->signature == 0x52 || hacb->signature == 0)
         hacb->FastSCSI = ((config2 & 0x10) != 0);
      else
         hacb->FastSCSI = ((config2 & 0x01) != 0);
      hacb->NoDisconnect = ((config2 & 0x04) == 0);
      hacb->InitiateSDTR = ((config2 & 0x08) != 0);
      hacb->UseDma = (   (config2 & 0x80) != 0
                      && (hacb->revision > 0 || hacb->dmaChannel == 0));
   }
   hacb->scsiPhase = BUS_FREE_PHASE;
   hacb->busID = hacb->lun = DISCONNECTED;
   hacb->Synchronous = TRUE;
   hacb->clockPeriod = 50;
   hacb->negotiateSDTR = ((hacb->InitiateSDTR) ? 0xFF : 0);
   hacb->sdtrMsg.extMsgCode = EXTENDED_MSG;
   hacb->sdtrMsg.extMsgLength = SYNCHRONOUS_DATA_TRANSFER_MSG_LEN;
   hacb->sdtrMsg.extMsgType = SYNCHRONOUS_DATA_TRANSFER_MSG;
   hacb->sdtrMsg.transferPeriod = (hacb->FastSCSI)
                                   ? (int)(2 * hacb->clockPeriod) >> 2
                                   : (int)(4 * hacb->clockPeriod) >> 2;
   hacb->sdtrMsg.reqAckOffset = 8;
   hacb->requestSenseCdb[0] = 0x03;	/* REQUEST SENSE operation */
   hacb->selectTimeLimit = 256;
   hacb->dmaBusOnTime = 15;		/* Default 15 us bus on time... */
   hacb->dmaBusOffTime = 1;		/* ...and 1 us bus off time */
   if (     hacb->signature == 0x52
         || hacb->signature == 0x53
         || hacb->signature == 0x54
         || hacb->signature == 0xAA55) {
      ScsiRateOffset = (hacb->signature == 0xAA55) ? 3 : 2;
      for (i = 0; i <= 7; i++)
         if (i != hacb->ownID) {
            OUTPUT(DmaCntrl1,
                   (UCHAR) (((hacb->ownID - i) & 0x07) + ScsiRateOffset));
            scsirate = INPUT(Stack);
            if (scsirate == 0x80)
               hacb->negotiateSDTR &= ~(1 << i);
            else if ((scsirate & 0x80) != 0) {
               hacb->negotiateSDTR &= ~(1 << i);
               hacb->syncCycles[i] = ((int)(scsirate & SXFR) >> 4) + 2;
               hacb->syncOffset[i] = scsirate & SOFS;
            }
         }
      OUTPUT(DmaCntrl1, 0);
   } else if (hacb->signature == 0x03020100) {
      OUTPUT(DmaCntrl1, 8);
      for (i = 0; i <= 7; i++) {
         scsirate = INPUT(Stack);
         if (scsirate == 0x80)
            hacb->negotiateSDTR &= ~(1 << i);
         else if ((scsirate & 0x80) != 0) {
            hacb->negotiateSDTR &= ~(1 << i);
            hacb->syncCycles[i] = ((int)(scsirate & SXFR) >> 4) + 2;
            hacb->syncOffset[i] = scsirate & SOFS;
         }
      }
      OUTPUT(DmaCntrl1, 0);
   } else
      hacb->InitialReset = TRUE;		/* No BIOS has executed */
   return(TRUE);

}
/*-----------------------------------------------------------------------------
 If the 6X60 device driver has earlier reported to its client that one or more
 SCSI interface controllers are present, the procedure below is called for
 each one located.  Set the 6X60 registers to a predetermined baseline state
 and perform some simple sanity checks.  Then see if an initial SCSI bus RST
 is requested (from configuration information) or if the SCSI bus is not free
 before asserting RST for 25us.  If no RST is required, note that the 6X60
 must be programmed to it's "idle" state where it is receptive to reselection
 attempts by targets (see 'ScsiBusFree').  If any of these tests fail, return
 FALSE so that our OS specific code will not try to use this particular host
 adapter. */

BOOLEAN HIM6X60Initialize(HACB *hacb) {

   ULONG i;

   OUTPUT(DmaCntrl1, 0);		/* Make sure PWRDWN is cleared */
   hacb->FastSCSI &= ((hacb->revision = INPUT(Rev)) > 0);
   if (!hacb->Synchronous) {
      hacb->InitiateSDTR = FALSE;
      hacb->InitialReset |= (hacb->negotiateSDTR != 0xFF);
      hacb->negotiateSDTR = 0;
   } else if (!hacb->InitiateSDTR)
      hacb->negotiateSDTR = 0;
   hacb->UseDma &= (hacb->revision > 0 || hacb->dmaChannel == 0);
   hacb->sdtrMsg.transferPeriod = (hacb->FastSCSI)
                                   ? (int)(2 * hacb->clockPeriod) >> 2
                                   : (int)(4 * hacb->clockPeriod) >> 2;
   hacb->selectTimeLimit = ((int)(hacb->selectTimeLimit + 31) / 32) * 32;
   if (hacb->selectTimeLimit == 32)
      hacb->sXfrCtl1Image = STIMESEL32 | ENSTIMER;
   else if (hacb->selectTimeLimit == 64)
      hacb->sXfrCtl1Image = STIMESEL64 | ENSTIMER;
   else if (hacb->selectTimeLimit == 128)
      hacb->sXfrCtl1Image = STIMESEL128 | ENSTIMER;
   else {
      hacb->selectTimeLimit = 256;
      hacb->sXfrCtl1Image = STIMESEL256 | ENSTIMER;
   }
   hacb->dmaBusOnTime = ADSS_MIN(15, hacb->dmaBusOnTime);
   hacb->dmaBusOffTime = ADSS_MIN(15, hacb->dmaBusOffTime);
   ENABLE_TRACE;
   OUTPUT(ScsiSeq, 0);			/* Inhibit selection/reselection */
   OUTPUT(SXfrCtl0, CLRSTCNT | CLRCH);	/* Clear channel, transfer counters */
   OUTPUT(SXfrCtl0, CHEN);
   if (hacb->CheckParity)
      OUTPUT(SXfrCtl1, hacb->sXfrCtl1Image |= ENSPCHK);
   else
      OUTPUT(SXfrCtl1, hacb->sXfrCtl1Image);
   OUTPUT(ScsiSig, 0);			/* Assert no SCSI bus control lines */
   OUTPUT(ScsiRate, 0);			/* Default asynchronous transfers */
   OUTPUT(ScsiDat, 0);			/* Assert no data signals, either */
   OUTPUT(ScsiID, (UCHAR) (hacb->ownID << 4));	/* Our own SCSI ID */
   OUTPUT(ClrSInt0, 0xFF & ~SETSDONE);
   OUTPUT(ClrSInt1, 0xFF);
   OUTPUT(ClrSErr, CLRSYNCERR | CLRFWERR | CLRFRERR);
   OUTPUT(SIMode0, 0);			/* Mask off all interrupt sources */
   OUTPUT(SIMode1, 0);
   OUTPUT(DmaCntrl0, RSTFIFO);		/* Clear data transfer FIFO */
   OUTPUT(BrstCntrl, (UCHAR) (hacb->dmaBusOnTime << 4 | hacb->dmaBusOffTime));
   if ((INPUT(DmaStat) & INTSTAT) != 0)
      return(FALSE);
   for (i = 0; i < 0x1000000; i += 0x333333) {
      OUTPUT(StCnt0, (UCHAR) i);
      OUTPUT(StCnt1, (UCHAR) i);
      OUTPUT(StCnt2, (UCHAR) i);
      if (i != (           INPUT(StCnt0)
                | ((ULONG) INPUT(StCnt1)) << 8
                | ((ULONG) INPUT(StCnt2)) << 16))
         return(FALSE);
   }
   if (hacb->InitialReset || INPUT(ScsiSig) != 0) {	/* RST necessary? */
      OUTPUT(SIMode1, ENSCSIRST);
      OUTPUT(ScsiSeq, SCSIRSTO);
      HIM6X60Delay(25);			/* Minimum 25us assertion */
      OUTPUT(ScsiSeq, 0);
      if (!HIM6X60ISR(hacb) || (hacb->sStat1 & SCSIRSTI) == 0)
         return(FALSE);
   } else {
      OUTPUT(ScsiSeq, ENRESELI);
      OUTPUT(SIMode0, ENSELDI);		/* These interrupts (ONLY) ... */
      OUTPUT(SIMode1, ENSCSIRST);	/* ...in "idle" mode */
      OUTPUT(DmaCntrl0, INTEN);
      OUTPUT(DmaCntrl1, PWRDWN);
   }
   return(TRUE);

}
/*-----------------------------------------------------------------------------
 When the OS specific code requests that an outstanding SCB be aborted, it may
 be in one of a large number of different states.  Go through all the relevant
 queues looking for the targeted SCB and take appropriate action.  In the
 order that the locations are checked:

    SCB is currently connected on the SCSI bus.
	It may be too late to do anything about this (for example, the
	interrupt service is awaiting a REQ for a status or command byte), but
	note that an abort has been requested and save the identity of the
	abort SCB in the extension.  When the targeted SCB eventually
	completes (either aborted or normally) we will then be able to notify
	our client of the completion for the abort SCB.

    SCB is in progress but disconnected.
	It is likely we can send an ABORT message.  Take the steps described
	above, but also put the targeted SCB back on the eligible queue to
	initiate a selection of the device.

    SCB has not yet been started.
	Easiest of all, just mark the SCB aborted and remove it from the
	relevant queue.  Note that not yet started SCB's may be found on
	either the Host Adapter Control Block (HACB) eligible queue or the
	Logical Unit Control Block (LUCB) waiting queue.

 Of course, if the SCB can't be found anywhere, just tell the OS specific code
 this is the case and don't worry. */

UCHAR HIM6X60AbortSCB(HACB *hacb, ADSS_SCB *scb, ADSS_SCB *scbToAbort) {

   LUCB *lucb;
   ADSS_SCB *nextScb;

   if (scb->length < sizeof(ADSS_SCB))
      scb->scbStatus = SCB_INVALID_LENGTH;
   else {
      if ((lucb = HIM6X60GetLUCB(hacb, 0, scb->targetID, scb->lun)) == NULL)
         scb->scbStatus = SCB_ABORT_FAILURE;
      else if (scbToAbort == hacb->ActiveScb)		/* On the SCSI bus? */
         scbToAbort->linkedScb = scb;			/* Must notify later */
      else if (scbToAbort == lucb->activeScb) {		/* Already underway? */
         scbToAbort->linkedScb = scb;		/* May be able to send ABORT */
         if (scbToAbort->scbStatus == SCB_PENDING) {
            linkScbPreemptive(&hacb->eligibleScb, scbToAbort);
            initiateIO(hacb);				/* Try to select now */
         }
      } else if (unlinkScb(&hacb->eligibleScb, scbToAbort)) {
         hacb->cActiveScb--;
         scbToAbort->scbStatus = SCB_ABORTED;
         lucb->activeScb = NULL;
         if (lucb->queuedScb == NULL || hacb->QueuesFrozen)
            lucb->busy = FALSE;
         else {
            lucb->busy = TRUE;
            unlinkScb(&lucb->queuedScb, nextScb = lucb->queuedScb);
            hacb->cQueuedScb--;
            linkScb(&hacb->eligibleScb, nextScb);
            hacb->cActiveScb++;
         }
      } else if (unlinkScb(&lucb->queuedScb, scbToAbort)) {
         hacb->cQueuedScb--;
         scbToAbort->scbStatus = SCB_ABORTED;
      } else {
         scbToAbort = NULL;
         scb->scbStatus = SCB_ABORT_FAILURE;
      }
      if (scbToAbort != NULL)
         if (scbToAbort->scbStatus == SCB_ABORTED) {
            HIM6X60CompleteSCB(hacb, scbToAbort);
            scb->scbStatus = SCB_COMPLETED_OK;		/* Abort succeeded */
         }
   }
   return(scb->scbStatus);

}
/*-----------------------------------------------------------------------------
 When the OS specific device driver requests that a SCSI command be executed
 for a particular target and LUN, we have to associate it with the queues
 maintained for the device.  First, see if there is a Logical Unit Control
 Block (LUCB) on hand --- the OS specific code keeps track of these.  There
 always should be a matching LUCB, but if there isn't, don't do anything ---
 the request is expected to eventually time out  (NB: the reason for this "do
 nothing" approach is the lack of a meaningful status code to return to our
 client).  Otherwise, see if the device is already busy.  If so, just add the
 new SCB to the queue of SCB's awaiting execution (they will be started first
 come, first serve as preceding SCB's complete).  If the device is idle, mark
 it busy and place the SCB on a host adapter queue of work eligible to be
 started.  In this case, call the IO initiation procedure to try to commence
 the SCB execution. */

UCHAR HIM6X60QueueSCB(HACB *hacb, ADSS_SCB *scb) {

   LUCB *lucb;
   UCHAR lun, targetID;
   ADSS_SCB *nextScb, *queueFreezeScb;

   if (scb->length < sizeof(ADSS_SCB))
      scb->scbStatus = SCB_INVALID_LENGTH;
   else if (   scb->function == SCB_EXECUTE
            || scb->function == SCB_BUS_DEVICE_RESET
            || scb->function == SCB_RELEASE_RECOVERY
            || scb->function == SCB_TERMINATE_IO) {
      scb->transferResidual = scb->dataLength;
      if ((lucb = HIM6X60GetLUCB(hacb, 0, scb->targetID, scb->lun)) == NULL)
         return(SCB_PENDING);
      if (     scb->function == SCB_EXECUTE
            && (scb->flags & (SCB_DATA_OUT | SCB_DATA_IN)) != 0) {
         if (hacb->UseDma && (scb->flags & SCB_DISABLE_DMA) == 0) {
            scb->segmentAddress = HIM6X60GetPhysicalAddress(hacb, scb,
                                                          scb->dataPointer, 0,
                                                          &scb->segmentLength);
            scb->segmentLength = ADSS_MIN(scb->segmentLength, scb->dataLength);
         } else if ((scb->flags & SCB_VIRTUAL_SCATTER_GATHER) != 0) {
            scb->dataPointer = HIM6X60GetVirtualAddress(hacb, scb,
                                                        scb->dataPointer, 0,
                                                        &scb->segmentLength);
            scb->segmentLength = ADSS_MIN(scb->segmentLength, scb->dataLength);
         } else
            scb->segmentLength = scb->dataLength;
      }
      if (hacb->resetScb != NULL) 
         linkScb(&hacb->deferredScb, scb);
      else if (hacb->QueuesFrozen || lucb->busy) {
         linkScb(&lucb->queuedScb, scb);
         hacb->cQueuedScb++;
      } else {
         lucb->busy = TRUE;
         linkScb(&hacb->eligibleScb, scb);
         hacb->cActiveScb++;
         initiateIO(hacb);
      }
   } else if (scb->function == SCB_SCSI_RESET)
      linkScb(&hacb->resetScb, scb);
   else if (scb->function == SCB_SUSPEND_IO) {
      hacb->QueuesFrozen = TRUE;
      if (hacb->cActiveScb == 0)
         scb->scbStatus = SCB_COMPLETED_OK;
      else {
         scb->scbStatus = SCB_PENDING;
         linkScb(&hacb->queueFreezeScb, scb);
      }
   } else if (scb->function == SCB_RESUME_IO) {
      while (hacb->queueFreezeScb != NULL) {
         unlinkScb(&hacb->queueFreezeScb,
                   queueFreezeScb = hacb->queueFreezeScb);
         queueFreezeScb->scbStatus = SCB_COMPLETED_OK;
         HIM6X60CompleteSCB(hacb, queueFreezeScb);
      }
      hacb->QueuesFrozen = FALSE;
      for (targetID = 0; targetID < 8; targetID++)
         if (targetID != hacb->ownID)
            for (lun = 0; lun < 8; lun++)
               if ((lucb = HIM6X60GetLUCB(hacb, 0, targetID, lun)) != NULL)
                  if (lucb->queuedScb != NULL) {
                     lucb->busy = TRUE;
                     unlinkScb(&lucb->queuedScb, nextScb = lucb->queuedScb);
                     hacb->cQueuedScb--;
                     linkScb(&hacb->eligibleScb, nextScb);
                     hacb->cActiveScb++;
                     initiateIO(hacb);
                  }
      scb->scbStatus = SCB_COMPLETED_OK;
   }
   return(scb->scbStatus);

}
/*-----------------------------------------------------------------------------
 Just as for the preceding ABORT message procedure, when the OS specific code
 requests that an outstanding SCB be terminated, it may be in one of a large
 number of different states.  Go through all the relevant queues and take
 appropriate action.  In the order that the locations are checked:

    SCB is currently connected on the SCSI bus.
	It may be too late to do anything about this (for example, the
	interrupt service is awaiting a REQ for a status or command byte), but
	note that an I/O process termination has been requested and save the
	identity of the requesting SCB in the extension.  When the targeted
	SCB eventually completes (either terminated or normally) we will then
	be able to notify our client of the completion of the requesting SCB.

    SCB is in progress but disconnected.
	It is likely we can send a TERMINATE I/O PROCESS message.  Take the
	steps described above, but also put the targeted SCB back on the
	eligible queue to initiate a selection of the device.

    SCB has not yet been started.
	Easiest of all, just mark the SCB terminated and remove it from the
	relevant queue.  Note that not yet started SCB's may be found on
	either the Host Adapter Control Block (HACB) eligible queue or the
	Logical Unit Control Block (LUCB) waiting queue.

 Of course, if the SCB can't be found anywhere, just tell the OS specific code
 this is the case and don't worry. */

UCHAR HIM6X60TerminateSCB(HACB *hacb, ADSS_SCB *scb, ADSS_SCB *scbToTerminate) {

   LUCB *lucb;
   ADSS_SCB *nextScb;

   if (scb->length < sizeof(ADSS_SCB))
      scb->scbStatus = SCB_INVALID_LENGTH;
   else {
      if ((lucb = HIM6X60GetLUCB(hacb, 0, scb->targetID, scb->lun)) == NULL)
         scb->scbStatus = SCB_TERMINATE_IO_FAILURE;
      else if (scbToTerminate == hacb->ActiveScb)	/* On the SCSI bus? */
         scbToTerminate->linkedScb = scb;		/* Must notify later */
      else if (scbToTerminate == lucb->activeScb) {	/* Already underway? */
         scbToTerminate->linkedScb = scb;	/* May be able to terminate */
         if (scbToTerminate->scbStatus == SCB_PENDING) {
            linkScbPreemptive(&hacb->eligibleScb, scbToTerminate);
            initiateIO(hacb);				/* Try to select now */
         }
      } else if (unlinkScb(&hacb->eligibleScb, scbToTerminate)) {
         hacb->cActiveScb--;
         scbToTerminate->scbStatus = SCB_TERMINATED;
         lucb->activeScb = NULL;
         if (lucb->queuedScb == NULL || hacb->QueuesFrozen)
            lucb->busy = FALSE;
         else {
            lucb->busy = TRUE;
            unlinkScb(&lucb->queuedScb, nextScb = lucb->queuedScb);
            hacb->cQueuedScb--;
            linkScb(&hacb->eligibleScb, nextScb);
            hacb->cActiveScb++;
         }
      } else if (unlinkScb(&lucb->queuedScb, scbToTerminate)) {
         hacb->cQueuedScb--;
         scbToTerminate->scbStatus = SCB_TERMINATED;
      } else {
         scbToTerminate = NULL;
         scb->scbStatus = SCB_TERMINATE_IO_FAILURE;
      }
      if (scbToTerminate != NULL)
         if (scbToTerminate->scbStatus == SCB_TERMINATED) {
            HIM6X60CompleteSCB(hacb, scbToTerminate);
            scb->scbStatus = SCB_COMPLETED_OK;	/* Terminate I/O succeeded */
         }
   }
   return(scb->scbStatus);

}
/*-----------------------------------------------------------------------------
 SCB's eligible to be started (i.e for which we must initiate a selection) are
 kept on a first-come, first-served "eligible" queue maintained at the HACB.
 This simple procedure takes the first entry from this list, if present, and
 attempts to start it if the SCSI bus is idle.  Note that a SCSI bus event
 could be taking place at the very same time, e.g. a reselection, so the
 appropriate interrupts must be enabled and provisions made for a selection
 failure because of such a collision.  The method chosen is to leave the SCB
 on the eligible queue until selection is complete.  If selection fails for
 any reason (other than selection timeout), the active SCB, bus ID and LUN
 fields in the HACB are reset and the SCB will be restarted at the next
 opportunity. */

VOID initiateIO(HACB *hacb) {

   ADSS_SCB *scb;

   if (hacb->resetScb != NULL && hacb->cActiveScb == 0)
      HIM6X60ResetBus(hacb, 0);
   else if (hacb->busID == DISCONNECTED && hacb->eligibleScb != NULL) {
      hacb->ActiveScb = scb = hacb->eligibleScb;
      hacb->busID = scb->targetID;
      hacb->lun = scb->lun;
      TRACE(hacb, INITIATE_IO, (ULONG) scb, 0);
      if (hacb->disableINT == 0)		/* Maybe BIOS cleared it... */
         OUTPUT(DmaCntrl0, INTEN);		/* ...so make sure INTEN set */
      OUTPUT(DmaCntrl1, 0);			/* Make sure PWRDWN is clear */
      OUTPUT(SXfrCtl1, hacb->sXfrCtl1Image);	/* Set selection timeout */
      OUTPUT(SIMode0, ENSELDO | ENSELDI | ENSELINGO);
      OUTPUT(SIMode1, ENSELTIMO | ENSCSIRST);
      OUTPUT(ScsiID, (UCHAR) (hacb->ownID << 4 | hacb->busID));
      OUTPUT(ScsiSeq, ENSELO | ENRESELI | ENAUTOATNO);
      if (!hacb->irqConnected)				/* Verified IRQ yet? */
         HIM6X60Watchdog(hacb, watchdog, WATCHDOG_POLL_IRQ);
   }

}
/*-----------------------------------------------------------------------------
 This is an entry point provided to the OS specific device driver for its use
 if it ever wishes to assert RST on the SCSI bus.  Whatever damage to current
 IO that is done by the reset is sorted out by the 6X60 interrupt service (see
 'ScsiBusReset' for details. */

BOOLEAN HIM6X60ResetBus(HACB *hacb, UCHAR scsibus) {

   if (scsibus == 0) {
      TRACE(hacb, RESET_BUS, (ULONG) hacb->ActiveScb, 0);
      DISABLE_TRACE;
      OUTPUT(DmaCntrl1, 0);	/* Clear PWRDWN, else no RST on SCSI bus */
      OUTPUT(ScsiSeq, SCSIRSTO);
      HIM6X60Delay(25);
      OUTPUT(ScsiSeq, 0);
      return(TRUE);
   } else
      return(FALSE);

}

/*-----------------------------------------------------------------------------
 These procedures are provided so that the OS specific code may have control
 over whether or not the INT output of the 6X60 may be asserted or not. */

VOID HIM6X60DisableINT(HACB *hacb) {

   if (hacb->disableINT++ == 0)
      OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) & ~INTEN));

}

BOOLEAN HIM6X60EnableINT(HACB *hacb) {

   if (hacb->disableINT == 0) {
      OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) | INTEN));
      return(TRUE);
   } else if (--hacb->disableINT == 0) {
      OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) | INTEN));
      return(TRUE);
   } else
      return(FALSE);

}

/*-----------------------------------------------------------------------------
 The procedure below may be used to dynamically determine the IRQ level
 associated with the 6X60.  It asserts SWINT to cause INT to be raised.  When
 the operating system specific code interrupt handler is invoked, it should
 call HIM6X60ISR in order to clear SWINT. */

VOID HIM6X60AssertINT(HACB* hacb) {

   OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) | SWINT));

}
/*-----------------------------------------------------------------------------
 This procedure is called by the OS specific interrupt routine when the 6X60
 (or some other device!) has asserted the IRQ associated with this 6X60.
 "Skim milk masquerades as cream..." so first check to see if our device has
 an interrupt asserted.  No?  Return FALSE to let the OS routine know it might
 be some other device's interrupt.  Otherwise, mask off the INT signal but
 continue to call the interrupt service procedure until all interrupts go
 away.  This approach saves the unnecessary overhead of returns to the OS
 specific code, EOI's to the interrupt controller, task switches, etc., etc. */

BOOLEAN HIM6X60IRQ(HACB *hacb) {

   if ((INPUT(DmaStat) & INTSTAT) != 0) {
      TRACE(hacb, INTERRUPT, hacb->cQueuedScb, hacb->cActiveScb);
      hacb->IsrActive = TRUE;
      if (hacb->disableINT++ == 0)
         OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) & ~(INTEN | SWINT)));
      if (!hacb->irqConnected) {		/* Our first interrupt? */
         hacb->irqConnected = TRUE;		/* OK, rest assumed to work */
         HIM6X60Watchdog(hacb, NULL, 0);	/* Cancel the watchdog */
      }
      do
         isr(hacb);
      while ((INPUT(DmaStat) & INTSTAT) != 0);
      if (hacb->disableINT == 0)
         OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) | INTEN));
      else if (--hacb->disableINT == 0)
         OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) | INTEN));
      hacb->IsrActive = FALSE;
      return(TRUE);
   } else
      return(FALSE);

}

/*-----------------------------------------------------------------------------
 This procedure is essentially the same as the one above BUT it is to be used
 ONLY for POLLING interrupts.  The reason is the code in the previous
 procedure that declares the IRQ "connected" if it is ever invoked.  The same
 assumption cannot be made here. */

BOOLEAN HIM6X60ISR(HACB *hacb) {

   TRACE(hacb, ISR, (ULONG) hacb->ActiveScb, 0);
   if ((INPUT(DmaStat) & INTSTAT) != 0) {
      hacb->IsrActive = TRUE;
      if (hacb->disableINT++ == 0)
         OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) & ~(INTEN | SWINT)));
      do
         isr(hacb);
      while ((INPUT(DmaStat) & INTSTAT) != 0);
      if (hacb->disableINT == 0)
         OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) | INTEN));
      else if (--hacb->disableINT == 0)
         OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) | INTEN));
      hacb->IsrActive = FALSE;
      return(TRUE);
   } else
      return(FALSE);

}
/*-----------------------------------------------------------------------------
 The deferred interrupt service procedure below executes at an interruptible
 level on the processor.  It is used only for programmed IO data phases, in
 order that other interrupts may be serviced while we are busy with this
 processor-intensive activity.  Note that once the data phase is complete, any
 fresh interrupts are serviced at this deferred level; this is to avoid
 unnecessary overhad with the return to a high priority level, new hardware
 interrupt service, EOI's, etc. */

VOID deferredIsr(HACB *hacb) {

   hacb->DeferredIsrActive = TRUE;
   if (hacb->scsiPhase == DATA_OUT_PHASE)
      dataOutPIO(hacb);
   else
      dataInPIO(hacb);
   while ((INPUT(DmaStat) & INTSTAT) != 0)
      isr(hacb);
   hacb->DeferredIsrActive = FALSE;
   HIM6X60ExitDeferredISR(hacb, HIM6X60EnableINT);
   
}

/*-----------------------------------------------------------------------------
 The watchdog procedure below guards against two dangers.  First, 6360 parts
 with a revision level of one have a hardware design error so that terminal
 count (TC) is monitored without qualification by DACKn.  This can cause a
 6360 DMA transfer to "hang" if another DMA channel (e.g. floppy) asserts TC
 before the SCSI data phase completes.  This situation can be remedied by
 clearing ENDMA and then resetting it (this causes a resumption of DMA
 transfers), but the system performance would be terribly degraded.  So, if
 this "collision" ever occurs, fall back to programmed IO.  The other
 circumstance for which the watchdog guards is a user configuration error in
 specification of the IRQ level.  If an interrupt condition is ever present
 when this watchdog is invoked, interrupts are not properly vectored to the
 interrupt service routine.  Recovery in this case is to always poll the
 interrupt state of the 6X60 whenever the watchdog is invoked.  This causes
 some performance degradation, so an error message is logged to the user so
 that the configuration may be corrected. */

VOID watchdog(HACB *hacb) {

   if (HIM6X60ISR(hacb)) {
      if (!hacb->irqConnected && hacb->IRQ != 0xFF) {
         hacb->IRQ = 0xFF;		/* Truth is, we don't have a clue! */
         HIM6X60LogError(hacb, NULL, 0, 0, 0, SCB_WARNING_NO_INTERRUPTS, 0);
      }
   } else if (   hacb->DmaActive
              && (INPUT(DmaStat) & ATDONE) != 0
              && (INPUT(SStat0) & DMADONE) == 0) {
      HIM6X60LogError(hacb, hacb->ActiveScb, 0, hacb->busID, hacb->lun,
                      SCB_WARNING_DMA_HANG, hacb->scsiPhase);
      OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) & ~ENDMA));
      OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) | ENDMA));
   }
#ifdef OLD_STUFF
   if (hacb->DmaActive && hacb->revision == 1)
      HIM6X60Watchdog(hacb, watchdog, WATCHDOG_DMA_HANG);
   else if (!hacb->irqConnected && (   hacb->scsiPhase != DISCONNECTED
#else
   if (!hacb->irqConnected && (   hacb->scsiPhase != DISCONNECTED
#endif
                                    || hacb->cActiveScb > 0))
      HIM6X60Watchdog(hacb, watchdog, WATCHDOG_POLL_IRQ);

}
/*-----------------------------------------------------------------------------
 The interrupt service procedure below handles one interrupt condition at a
 time, ranked according to their priority.  The fact that some 6X60 interrupts
 are mutually exclusive has been taken into account in the logic design.
 Because actions taken by the interrupt service may in turn generate a new
 interrupt condition, this procedure is called from the higher-level
 HIM6X60IRQ or HIM6X60ISR as long as INTSTAT is asserted.
 
 In the order of priority in which the interrupt causes are examined:
  
    * SCSI Errors:  These are not really expected (they are the fault of
      erroneous software or else a 6X60 hardware malfunction), so take the
      easy, drastic way out and assert RST on the SCSI bus.

    * BUSFREE, SCSIRSTI or SELTO:  In all three cases, the common element is
      that the bus is now free and whatever was underway has come to an end.
      Note that the BUSFREE interrupt is the normal conclusion to most SCSI
      commands and is the signal that it is OK to look for new work to start.

    * SELINGO:  The significance of this interrupt is that the 6X60 has
      successfully arbitrated for the SCSI bus and has commenced selection.
      This means the bus is no longer free, so it is OK to enable the BUSFREE
      interrupt.  This bit is cleared by the 6X60 as soon as selection is
      complete, so even though an IRQ has been generated we may not find
      SELINGO asserted.

    * SELDI:  A SCSI target has reselected the 6X60.

    * SELDO:  A selection begun by 'initiateIO' has successfully selected the
      SCSI target.

    * DMADONE:  The preceding data transfer has "gone about as fur as it can
      go."  PHASEMIS may or may not be asserted.  It is necessary to figure
      out how much data was transferred so that the current pointers in the
      HACB can be adjusted.

    * PHASEMIS:  The SCSI target has REQuested a new, different information
      transfer phase.  Even if DMA was in use, DMADONE may or may not be
      asserted.  Before servicing the target, adjust the current pointers in
      the HACB if the preceding phase was a data transfer phase.

 Note that in the case of the last two interrupts, the state machine that is
 responsible for responding to the target REQuests for information transfer is
 driven ONLY by a REQ asserted on the SCSI bus.  If the expected REQ is not
 yet present, enable the REQINIT interrupt so we may exit the interrupt
 service and wait for it. */

VOID isr(HACB *hacb) {

   hacb->sStat0 = INPUT(SStat0);
   hacb->maskedSStat0 = hacb->sStat0 & INPUT(SIMode0);
   hacb->sStat1 = INPUT(SStat1);
   hacb->maskedSStat1 = hacb->sStat1 & INPUT(SIMode1);
   if (INPUT(SStat4) != 0) {
      OUTPUT(ClrSErr, CLRSYNCERR | CLRFWERR | CLRFRERR);
      if (hacb->ActiveScb != NULL)
         hacb->ActiveScb->scbStatus = SCB_PROTOCOL_ERROR;
      HIM6X60ResetBus(hacb, 0);
   } else if ((hacb->maskedSStat1 & (SELTO | SCSIRSTI | BUSFREE)) != 0)
      ScsiBusFree(hacb);
   else if ((hacb->maskedSStat0 & SELINGO) != 0) {
      OUTPUT(ClrSInt1, CLRBUSFREE);	/* OK to watch for BUSFREE now */
      OUTPUT(SIMode0, ENSELDO | ENSELDI);
      OUTPUT(SIMode1, ENSELTIMO | ENSCSIRST | ENBUSFREE);
   } else if ((hacb->maskedSStat0 & SELDI) != 0)
      reselection(hacb);
   else if ((hacb->maskedSStat0 & SELDO) != 0)
      selection(hacb);
   else {
#ifdef OLD_STUFF
      if (hacb->DmaActive)			/* If data phase DMA... */
         quiesceDmaAndSCSI(hacb);		/* ...let it settle down */
#endif
      if ((INPUT(SStat1) & REQINIT) == 0)
         OUTPUT(SIMode1, (UCHAR) (INPUT(SIMode1) | ENREQINIT));
      else {
         if ((hacb->scsiPhase & NON_DATA_PHASES) == 0)
            updateDataPointer(hacb);
         targetREQuest(hacb);	/* Go service (potentially) new phase */
      }
   }

}
/*-----------------------------------------------------------------------------
 The SCSI bus is free, either because the 6X60 has dropped SEL after the
 selection timeout expired, the active SCSI target has dropped BSY or because
 RST was detected and everyone got off the bus.  First, in case DMA was
 active, shut down the system DMA controller for our channel.  Then reprogram
 the 6X60 to a known state.  This is important because all SCSI transactions,
 in error or successful, are expected to pass by this way.  The "idle" state
 of the 6X60 is to watch for either RST or a target reselection.  Last, sort
 out the impact of the bus free on whatever IO was in progress.  If it was an
 attempted selection, it has timed out.  If we were trying to select a target
 to send it an abort message and it has timed out, there is little choice but
 assert RST to get its attention.  If a RST was detected, any connected SCSI
 IO is terminated plus all SCB's that had commenced but are presently
 disconnected.  Or, if it simply bus free, sort out whether it was expected or
 unexpected.  Before returning, clean up the HACB state variables. */

VOID ScsiBusFree(HACB *hacb) {

   LUCB *lucb;
   ADSS_SCB *linkedScb, *nextScb, *queueFreezeScb, *scb = NULL;

#ifdef OLD_STUFF
   if (hacb->DmaActive) {	/* Do we need to quiesce any system DMA? */
      if (hacb->revision == 1)
         HIM6X60Watchdog(hacb, NULL, 0);
      HIM6X60FlushDMA(hacb);
      hacb->DmaActive = FALSE;
   }
#endif
   OUTPUT(ScsiSeq, 0);
   OUTPUT(SXfrCtl0, CHEN | CLRSTCNT | CLRCH);
   OUTPUT(SXfrCtl1, hacb->sXfrCtl1Image);
   OUTPUT(ScsiRate, 0);
   OUTPUT(ScsiSig, 0);
   OUTPUT(ScsiDat, 0);
   OUTPUT(DmaCntrl0, RSTFIFO);
   OUTPUT(ClrSInt0, 0xFF & ~(SETSDONE | CLRSELDI));
   OUTPUT(ClrSInt1, 0xFF);
   OUTPUT(SIMode0, ENSELDI);
   OUTPUT(SIMode1, ENSCSIRST);
   OUTPUT(ScsiSeq, ENRESELI);
   OUTPUT(PortA, LED_OFF);
   TRACE(hacb, BUS_FREE, 0, 0);
   if ((hacb->maskedSStat1 & SCSIRSTI) != 0)
      ScsiBusReset(hacb);
   else if (hacb->ActiveScb != NULL) {
      scb = hacb->ActiveScb;
      if ((hacb->maskedSStat1 & SELTO) != 0)
         scb->scbStatus = SCB_SELECTION_TIMEOUT;
      else if (hacb->DisconnectOK) {
         hacb->DisconnectOK = FALSE;
         if (     hacb->scsiPhase == MESSAGE_OUT_PHASE
               && SCB_STATUS(scb->scbStatus) == SCB_PENDING)
            scb->scbStatus = (hacb->MsgOut[0] == ABORT_MSG) ? SCB_ABORTED
                                                            : SCB_COMPLETED_OK;
      } else {
         scb->scbStatus = SCB_BUS_FREE;
         HIM6X60LogError(hacb, scb, 0, hacb->busID, hacb->lun,
                         SCB_ERROR_UNEXPECTED_DISCONNECT, hacb->scsiPhase);
      }
      if (SCB_STATUS(scb->scbStatus) == SCB_PENDING)
         scb = NULL;				/* SCB not yet completed */
      else {
         unlinkScb(&hacb->eligibleScb, scb);
         if ((lucb = HIM6X60GetLUCB(hacb, 0, scb->targetID,
                                    scb->lun)) != NULL) {
            lucb->activeScb = NULL;
            if (lucb->queuedScb == NULL || hacb->QueuesFrozen)
               lucb->busy = FALSE;
            else {
               lucb->busy = TRUE;
               unlinkScb(&lucb->queuedScb, nextScb = lucb->queuedScb);
               hacb->cQueuedScb--;
               linkScb(&hacb->eligibleScb, nextScb);
               hacb->cActiveScb++;
            }
         }
      }
   }
   hacb->scsiPhase = BUS_FREE_PHASE;		/* Update SCSI state in HACB */
   hacb->busID = hacb->lun = DISCONNECTED;
   memset(&hacb->Nexus, 0, sizeof(hacb->Nexus));
   initiateIO(hacb);				/* Check for more work */
   if (scb != NULL) {				/* Did an SCB just complete? */
      if ((linkedScb = scb->linkedScb) != NULL)
         if (linkedScb->function == SCB_TERMINATE_IO)
            if (     SCB_STATUS(scb->scbStatus) == SCB_ERROR
                  && scb->targetStatus == COMMAND_TERMINATED)
               linkedScb->scbStatus = SCB_COMPLETED_OK;
            else
               linkedScb->scbStatus = SCB_TERMINATE_IO_FAILURE;
         else if (scb->scbStatus == SCB_ABORTED)
            linkedScb->scbStatus = SCB_COMPLETED_OK;
         else
            linkedScb->scbStatus = SCB_ABORT_FAILURE;
      HIM6X60CompleteSCB(hacb, scb);
      hacb->cActiveScb--;
      if (linkedScb != NULL)
         HIM6X60CompleteSCB(hacb, linkedScb);
   }
   if (hacb->cActiveScb == 0) {
      while (hacb->queueFreezeScb != NULL) {
         unlinkScb(&hacb->queueFreezeScb,
                   queueFreezeScb = hacb->queueFreezeScb);
         queueFreezeScb->scbStatus = SCB_COMPLETED_OK;
         HIM6X60CompleteSCB(hacb, queueFreezeScb);
      }
      if (hacb->revision > 0)
         OUTPUT(DmaCntrl1, PWRDWN);		/* In case we're a laptop... */
   }

}
/*-----------------------------------------------------------------------------
 The current 6X60 software implementation chooses the SCSI "hard reset"
 option.  Why not the "soft" option, you ask?  Well, the software engineer
 took the easy way out because it wasn't known whether or not the File System
 (for the particular operating system under which we are executing) had been
 designed with "soft reset" in mind.  What this means is once RST has been
 detected we must cancel any and all SDTR agreements previously negotiated
 (and make a note to renegotiate them at the next SCSI command) and terminate
 all active and pending SCB's.  If the SCSI bus reset is the result of an
 earlier call to HIM6X60ResetBus there may be queued SCB's that arrived after
 the reset was requested (so called "deferred" SCB's).  If so, queue all of
 them up on the eligible list BUT note that none of this IO will be resumed
 until the first call to HIM6X60QueueSCB by the OS specific code.  The issue
 here is the adequacy of SCSI implementations in all sorts of target devices:
 some get caught with their pants down too soon after RST and don't even
 respond to a selection, others get it together to respond BUSY but don't have
 anything else (such as sense data) available for an appreciable length of
 time (seconds) while a few report UNIT ATTENTION very soon after RST.  At
 present, the 6X60 driver leaves it to the OS specific code to resume IO. */

VOID ScsiBusReset(HACB *hacb) {

   LUCB *lucb;
   UCHAR lun, targetID;
   ADSS_SCB *linkedScb, *scb;

   TRACE(hacb, BUS_RESET, 0, 0);
   OUTPUT(ClrSInt1, CLRSCSIRSTI);
   resetSDTR(hacb, hacb->ownID);
   for (targetID = 0; targetID < 8; targetID++) {
      if (targetID != hacb->ownID)
         for (lun = 0; lun < 8; lun++)
            if ((lucb = HIM6X60GetLUCB(hacb, 0, targetID, lun)) != NULL) {
               lucb->busy = FALSE;
               if (lucb->activeScb != NULL) {
                  unlinkScb(&hacb->eligibleScb, scb = lucb->activeScb);
                  lucb->activeScb = NULL;
                  if ((scb->scbStatus & SCB_SENSE_DATA_VALID) != 0)
                     scb->scbStatus = SCB_REQUEST_SENSE_FAILURE;
                  else if (scb->scbStatus == SCB_PENDING)
                     scb->scbStatus = SCB_SCSI_BUS_RESET;
                  if ((linkedScb = scb->linkedScb) != NULL)
                     linkedScb->scbStatus = SCB_COMPLETED_OK;
                  HIM6X60CompleteSCB(hacb, scb);
                  hacb->cActiveScb--;
                  if (linkedScb != NULL)
                     HIM6X60CompleteSCB(hacb, linkedScb);
               }
               while (lucb->queuedScb != NULL) {
                  unlinkScb(&lucb->queuedScb, scb = lucb->queuedScb);
                  hacb->cQueuedScb--;
                  scb->scbStatus = SCB_SCSI_BUS_RESET;
                  HIM6X60CompleteSCB(hacb, scb);
               }
            }
   }
   while (hacb->eligibleScb != NULL) {
      unlinkScb(&hacb->eligibleScb, scb = hacb->eligibleScb);
      if ((scb->scbStatus & SCB_SENSE_DATA_VALID) != 0)
         scb->scbStatus = SCB_REQUEST_SENSE_FAILURE;
      else if (scb->scbStatus == SCB_PENDING)
         scb->scbStatus = SCB_SCSI_BUS_RESET;
      if ((linkedScb = scb->linkedScb) != NULL)
         linkedScb->scbStatus = SCB_COMPLETED_OK;
      HIM6X60CompleteSCB(hacb, scb);
      hacb->cActiveScb--;
      if (linkedScb != NULL)
         HIM6X60CompleteSCB(hacb, linkedScb);
   }
   while (hacb->resetScb != NULL) {
      unlinkScb(&hacb->resetScb, scb = hacb->resetScb);
      scb->scbStatus = SCB_COMPLETED_OK;
      HIM6X60CompleteSCB(hacb, scb);
   }
#ifdef USL_UNIX
   HIM6X60Event(hacb, EVENT_SCSI_BUS_RESET);
#else
   HIM6X60Event(hacb, EVENT_SCSI_BUS_RESET, 0);
#endif
   while (hacb->deferredScb != NULL) {
      unlinkScb(&hacb->deferredScb, scb = hacb->deferredScb);
      if ((lucb = HIM6X60GetLUCB(hacb, 0, scb->targetID, scb->lun)) != NULL)
         if (!lucb->busy) {
            lucb->busy = TRUE;
            linkScb(&hacb->eligibleScb, scb);
            hacb->cActiveScb++;
         } else {
            linkScb(&lucb->queuedScb, scb);
            hacb->cQueuedScb++;
         }
   }

}
/*-----------------------------------------------------------------------------
 ENSELDI has been asserted by the 6X60 to indicate either a selection or a
 reselection by another SCSI device.  Since this driver does not implement
 target mode, we can safely assume it is a reselection.  First, if the
 reselection collided with (took priority over) an attempted selection by the
 AIC-6X60 of another SCSI device, clean up the state variables in the HACB
 (the SCB remains on the eligible queue to be retried the next time the SCSI
 bus is free).  Then, take a look at the SCSI data bus for the bit pattern to
 identify the selecting device.  Only two bits should be present, our own ID
 and that of the other device.  Anything else is an error and the only
 recourse is RST on the SCSI bus.  For kosher reselections, reprogram the 6X60
 to be on the lookout for the first target REQ (expected) or a SCSI bus reset
 or bus free (unexpected).  Also set up for any synchronous data phases that
 may follow. */

VOID reselection(HACB *hacb) {

   UCHAR bitwiseID, busID;

   if (hacb->ActiveScb != NULL) {	/* A selection was underway? */
      hacb->ActiveScb = NULL;		/* If so, tidy up... */
      hacb->busID = hacb->lun = DISCONNECTED;
   }
   TRACE(hacb, RESELECTION, INPUT(SelID), 0);
   bitwiseID = INPUT(SelID) ^ 1 << hacb->ownID;	/* What bits are present? */
   for (busID = DISCONNECTED; bitwiseID != 0; bitwiseID >>= 1) {
      busID++;
      if ((bitwiseID & 0x01) != 0) {
         if ((bitwiseID ^ 0x01) != 0)
            busID = DISCONNECTED;	/* Oops! More than just one bit! */
         break;
      }
   }
   if (busID == DISCONNECTED) {		/* 6X60 is NOT supposed to interrupt */
      HIM6X60LogError(hacb, NULL, 0, 0xFF, 0xFF, SCB_ERROR_HOST_ADAPTER, 0);
      HIM6X60ResetBus(hacb, 0);		/* Only remedy is drastic, alas */
   } else {
      hacb->busID = busID;		/* LUN will be IDENTIFY'ed later */
      hacb->targetStatus = 0xFF;	/* In case STATUS phase absent */
      OUTPUT(PortA, LED_ON);		/* External indication of BSY */
      OUTPUT(ScsiSeq, ENAUTOATNP);	/* Disable reselection logic */
      OUTPUT(ClrSInt0, CLRSELDI);	/* So that SELDI can clear later */
      OUTPUT(ClrSInt1, CLRBUSFREE);	/* OK to watch for BUSFREE now */
      OUTPUT(SIMode0, 0);		/* Disable SELDO/SELDI */
      OUTPUT(SIMode1, ENSCSIRST | ENBUSFREE | ENREQINIT);
      OUTPUT(ScsiRate, (UCHAR) ((hacb->syncOffset[busID] == 0)
              ? 0
              : (hacb->syncCycles[busID] - 2) << 4 | hacb->syncOffset[busID]));
   }

}
/*-----------------------------------------------------------------------------
 Oh frabjous day! SELDO has signalled that the selection commenced by
 'initiateIO' has been acknowledged by the target.  Note that because of the
 self-clearing nature of the SELINGO interrupt bit, we may or may not have
 "seen" this interrupt prior to SELDO.  Doesn't matter, SELDO cannot occur
 without it so make sure the same housekeeping is done in either case.  Most
 of the processing here is in preparation for the MESSAGE phase expected
 from the target in response to our ATN signal.  If (inexplicably) there is no
 SCB to correspond to this selection, send a BUS DEVICE RESET (even though
 this is a case of a software engineer on drugs and not the fault of the
 target).  Otherwise, build an IDENTIFY message according to the instructions
 in the SCB.  If we've been instructed to abort this SCB, follow the IDENTIFY
 message with an ABORT.  Also, this is as good a time as any to load the
 current pointers into the HACB from the SCB extension.  Since the target
 might switch to a synchronous data phase after MESSAGE OUT, program the 6X60
 for this contingency. */

VOID selection(HACB *hacb) {

   UCHAR busID = hacb->busID;
   LUCB *lucb;
   ADSS_SCB *scb = hacb->ActiveScb;

   hacb->MsgOutLen = 1;	/* There will ALWAYS be at least one message byte */
   if (scb == NULL)
      hacb->MsgOut[0] = BUS_DEVICE_RESET_MSG;
   else {
      unlinkScb(&hacb->eligibleScb, scb);	/* Selection OK, unlink SCB */
      hacb->targetStatus = 0xFF;		/* In case no STATUS phase */
      lucb = HIM6X60GetLUCB(hacb, 0, scb->targetID, scb->lun);
      lucb->activeScb = scb;
      hacb->DataPointer = scb->dataPointer;	/* Current data pointer */
      hacb->DataLength = scb->dataLength;
      hacb->DataOffset = scb->dataOffset;
      hacb->SegmentAddress = scb->segmentAddress;
      hacb->SegmentLength = scb->segmentLength;
      if (scb->function == SCB_BUS_DEVICE_RESET)
         hacb->MsgOut[0] = BUS_DEVICE_RESET_MSG;
      else if (   hacb->NoDisconnect
               || (scb->flags & SCB_DISABLE_DISCONNECT) != 0)
         hacb->MsgOut[0] = IDENTIFY_MSG | scb->lun;
      else
         hacb->MsgOut[0] = IDENTIFY_MSG | PERMIT_DISCONNECT | scb->lun;
      if (scb->linkedScb != NULL)
         if (scb->linkedScb->function == SCB_TERMINATE_IO)
            hacb->MsgOut[hacb->MsgOutLen++] = TERMINATE_IO_PROCESS_MSG;
         else
            hacb->MsgOut[hacb->MsgOutLen++] = ABORT_MSG;
      else if (scb->function == SCB_RELEASE_RECOVERY)
         hacb->MsgOut[hacb->MsgOutLen++] = RELEASE_RECOVERY_MSG;
      else if (   (hacb->negotiateSDTR & 1 << busID) != 0
               && (scb->flags & SCB_DISABLE_NEGOTIATIONS) == 0) {
         memcpy(&hacb->MsgOut[hacb->MsgOutLen], &hacb->sdtrMsg,
                sizeof(hacb->sdtrMsg));
         hacb->MsgOutLen += sizeof(hacb->sdtrMsg);
      }
   }
   TRACE(hacb, SELECTION, hacb->MsgOut[0],
         (hacb->MsgOutLen == 1) ? 0 : hacb->MsgOut[1]);
   OUTPUT(PortA, LED_ON);		/* External indication of BSY */
   OUTPUT(ClrSInt1, CLRBUSFREE);	/* In case we missed SELINGO */
   OUTPUT(ScsiSeq, ENAUTOATNP);		/* Disable (re)selection... */
   OUTPUT(SIMode0, 0);			/* ..and their associated interrupts */
   OUTPUT(SIMode1, ENSCSIRST | ENBUSFREE | ENREQINIT);
   OUTPUT(ScsiRate, (UCHAR) ((hacb->syncOffset[busID] == 0)
              ? 0
              : (hacb->syncCycles[busID] - 2) << 4 | hacb->syncOffset[busID]));

}

#ifdef OLD_STUFF
/*-----------------------------------------------------------------------------
 When DMA is used for DATA OUT or DATA IN, the (approaching) end of the
 information transfer may be signalled by either DMADONE or PHASEMIS, both of
 which interrupts are enabled on the 6X60 when the data phase is started.
 However, the state of the DMA controller and the 6X60 FIFO's may not be
 stable until certain conditions obtain.  The relationship of the transfer
 length used to program the DMA controller and the amount of data the target
 is willing to send or receive over the SCSI bus is summarized below.  The
 right hand column indicates the 6X60 conditions that need to exist before
 data transfer in the system is stable.
 
 	DATA OUT
 		DMA < SCSI		DMADONE (& FIFOs empty)
 		DMA = SCSI		DMADONE & PHASEMIS (& FIFOs empty)
 		DMA > SCSI		PHASEMIS & ATDONE
 			(but excess data can be entirely contained in FIFOs)
 		DMA > SCSI		PHASEMIS & FIFOs full
 			(excess data too large to fit into the FIFOs)

 	DATA IN
 		DMA < SCSI		DMADONE & FIFOs full
 			(excess data too large to fit into the FIFOs)
 		DMA < SCSI		DMADONE & PHASEMIS
 			(but excess data can be entirely contained in FIFOs)
 		DMA = SCSI		DMADONE & PHASEMIS & FIFOs empty
 		DMA > SCSI		PHASEMIS & FIFOs empty

 If none of the above conditions are satisfied within a reasonable amount of
 time, assume the DMA controller is none the less quiet and ask the OS
 specific code to flush its activity. */

VOID quiesceDmaAndSCSI(HACB *hacb) {

   USHORT watchdog = 0;

   if (hacb->revision == 1)
      HIM6X60Watchdog(hacb, NULL, 0);
   if (hacb->scsiPhase == DATA_OUT_PHASE)
      while (   (INPUT(SStat0) & DMADONE) == 0
             && (   (INPUT(SStat1) & PHASEMIS) == 0
                 || (   (INPUT(DmaStat) & ATDONE) == 0
                     && (INPUT(DmaStat) & DFIFOFULL) == 0))
             && ++watchdog != 0xFFFF) {
         ;
      }
   else
      while (   (   (INPUT(SStat0) & DMADONE) == 0
                 || (   (INPUT(DmaStat) & DFIFOFULL) == 0
                     && (INPUT(SStat1) & PHASEMIS) == 0))
             && (   (INPUT(SStat1) & PHASEMIS) == 0
                 || (INPUT(DmaStat) & DFIFOEMP) == 0)
             && ++watchdog != 0xFFFF) {
         if (     hacb->revision == 1
               && (INPUT(DmaStat) & ATDONE) != 0
               && (INPUT(SStat0) & DMADONE) == 0) {
            HIM6X60LogError(hacb, hacb->ActiveScb, 0, hacb->busID, hacb->lun,
                            SCB_WARNING_DMA_HANG, hacb->scsiPhase);
            OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) & ~ENDMA));
            OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) | ENDMA));
         }
      }
   OUTPUT(SXfrCtl0, CHEN | SCSIEN);	/* Disconnect host and SCSI FIFO's */
   OUTPUT(ClrSInt0, CLRDMADONE);
#ifdef OLD_STUFF
   HIM6X60FlushDMA(hacb);
   TRACE(hacb, QUIESCE_DMA,
         INPUT(StCnt0) | ((USHORT) INPUT(StCnt1)) << 8, watchdog);
   hacb->DmaActive = FALSE;
#endif

}
#endif

/*-----------------------------------------------------------------------------
 The SCSI data pointer is implemented as a current buffer location (both
 virtual and physical) and a remaining transfer length (overall and for the
 current segment, be it virtual or physical) in the HACB.  When a 6X60
 interrupt has terminated a DATA OUT or DATA IN phase, it is time to update
 these data pointers.  If we have been in bitbucket mode, there must be a new
 phase requested; no updates are necessary, just nuke the bitbucket mode. If
 programmed IO was used for the phase just finished, we may need to either
 retrieve some data remaining in the 6X60 FIFO's (DATA IN phase) or adjust the
 data pointer backwards by data left over in the FIFO's, already accounted for
 in the data pointer but not accepted by the target (DATA OUT phase).  Updates
 after DMA phases complete are a little more involved.  If it was DATA OUT,
 it's simple --- just use the 6X60 count of data bytes ACKnowledged on the
 SCSI bus.  If the last phase was DATA IN, alas, some operating
 systems/platforms do not provide us with a count of data actually DMA'ed into
 memory.  Figure it out:  If ATDONE is asserted, the DMA controller signalled
 terminal count complete and we may use the original DMA transfer length as
 the count. Otherwise we have to adjust the 6X60 count and adjust it by
 whatever is still lingering in both the host and SCSI FIFO's.  Before we
 leave, if there is a new phase requested, clear all the FIFO's of (now)
 unecessary data. */

VOID updateDataPointer(HACB *hacb) {

   ULONG fifoCount = 0, xferCount;
   ADSS_SCB *scb = hacb->ActiveScb;

   if (hacb->BitBucket) {			/* BITBUCKET mode... */
      hacb->BitBucket = FALSE;
      OUTPUT(SXfrCtl0, CHEN | CLRSTCNT | CLRCH);
      OUTPUT(SXfrCtl0, CHEN);
      OUTPUT(SXfrCtl1, hacb->sXfrCtl1Image);
      OUTPUT(DmaCntrl0, RSTFIFO);
      TRACE(hacb, UPDATE_DATA_POINTER, 0, INPUT(DmaCntrl0));
   } else if (   !hacb->UseDma			/* ...or automatic PIO... */
              || (scb->flags & SCB_DISABLE_DMA) != 0) {
      if ((hacb->maskedSStat1 & PHASEMIS) != 0) {
         if (hacb->scsiPhase == DATA_IN_PHASE)
            dataInPIO(hacb);
         else {
            fifoCount = INPUT(FifoStat) + (INPUT(SStat2) & SFCNT);
            if (fifoCount != 0) {
               hacb->DataPointer -= fifoCount;
               hacb->DataLength += fifoCount;
               hacb->DataOffset -= fifoCount;
               hacb->SegmentLength += fifoCount;
            }
         }
         OUTPUT(SXfrCtl0, CHEN | CLRSTCNT | CLRCH);
         OUTPUT(SXfrCtl0, CHEN);
         OUTPUT(DmaCntrl0, RSTFIFO);
      }
      TRACE(hacb, UPDATE_DATA_POINTER, fifoCount, INPUT(DmaCntrl0));
   } else {					/* ...or DMA transfer */
      hacb->scsiCount = (           INPUT(StCnt0)
                         | ((ULONG) INPUT(StCnt1)) << 8
                         | ((ULONG) INPUT(StCnt2)) << 16);
      if (hacb->scsiPhase == DATA_OUT_PHASE)
         xferCount = hacb->scsiCount;
      else if (   (INPUT(DmaStat) & ATDONE) == 0
               || hacb->scsiCount < hacb->SegmentLength)
         xferCount = hacb->scsiCount
                      - (INPUT(FifoStat) + (INPUT(SStat2) & SFCNT));
      else
         xferCount = hacb->SegmentLength;
      hacb->DataPointer += xferCount;
      hacb->DataLength -= xferCount;
      hacb->DataOffset += xferCount;
      UPDATE_PHYSICAL_ADDRESS(hacb->SegmentAddress, xferCount);
      hacb->SegmentLength -= xferCount;
      if ((INPUT(SStat1) & PHASEMIS) == 0)
         OUTPUT(DmaCntrl0, (UCHAR) (INPUT(DmaCntrl0) & ~ENDMA));
      else {
         OUTPUT(SXfrCtl0, CHEN | CLRSTCNT | CLRCH);
         OUTPUT(SXfrCtl0, CHEN);
         OUTPUT(DmaCntrl0, RSTFIFO);
      }
      TRACE(hacb, UPDATE_DATA_POINTER, xferCount, INPUT(DmaCntrl0));
   }

}
/*-----------------------------------------------------------------------------
 You may think it's the SCSI host adapter in charge of things, but it's really
 the target cracking the whip and REQuesting bytes of information.  The
 procedure below is just a state machine that attempts to keep the target
 happy no matter what information transfer phase transitions it leads us
 through */

VOID targetREQuest(HACB *hacb) {

   UCHAR msgBytes, ScsiSignals;
   BOOLEAN pioReady;
   ADSS_SCB *scb = hacb->ActiveScb;

   hacb->scsiPhase = (ScsiSignals = INPUT(ScsiSig)) & SCSI_PHASE_MASK;
   TRACE(hacb, TARGET_REQ, 0, 0);
   pioReady = ((hacb->sStat0 & SPIORDY) != 0);
   OUTPUT(SIMode0, 0);
   OUTPUT(SIMode1, ENSCSIRST | ENPHASEMIS | ENBUSFREE);
   OUTPUT(SXfrCtl0, CHEN | CLRSTCNT);
   OUTPUT(ScsiSig, (UCHAR) (ScsiSignals & (CD | IO | MSG | ATN)));
   OUTPUT(ClrSInt1, CLRPHASECHG);
   switch (hacb->scsiPhase) {
      case DATA_OUT_PHASE:
         if (scb == NULL)
            bitbucketAndABORT(hacb, 0);
         else if ((scb->flags & SCB_DATA_OUT) == 0) {
            bitbucketAndABORT(hacb, SCB_PROTOCOL_ERROR);
            HIM6X60LogError(hacb, hacb->ActiveScb, 0, hacb->busID, hacb->lun,
                            SCB_ERROR_PROTOCOL, hacb->scsiPhase);
         } else if (hacb->DataLength == 0)
            bitbucketAndABORT(hacb, SCB_DATA_OVERRUN);
#ifdef OLD_STUFF
         else if (hacb->UseDma && (scb->flags & SCB_DISABLE_DMA) == 0)
            dataPhaseDMA(hacb);
#endif
         else if (hacb->DeferredIsrActive)
            dataOutPIO(hacb);
         else {
            hacb->disableINT++;		/* INT not allowed in deferred ISR! */
            HIM6X60DeferISR(hacb, deferredIsr);
         }
         break;

      case DATA_IN_PHASE:
         if (scb == NULL)
            bitbucketAndABORT(hacb, 0);
         else if (   (scb->flags & SCB_DATA_IN) == 0
                  && (scb->scbStatus & SCB_SENSE_DATA_VALID) == 0) {
            bitbucketAndABORT(hacb, SCB_PROTOCOL_ERROR);
            HIM6X60LogError(hacb, hacb->ActiveScb, 0, hacb->busID, hacb->lun,
                            SCB_ERROR_PROTOCOL, hacb->scsiPhase);
         } else if (hacb->DataLength == 0)
            bitbucketAndABORT(hacb, SCB_DATA_OVERRUN);
#ifdef OLD_STUFF
         else if (hacb->UseDma && (scb->flags & SCB_DISABLE_DMA) == 0)
            dataPhaseDMA(hacb);
#endif
         else if (hacb->DeferredIsrActive)
            dataInPIO(hacb);
         else {
            hacb->disableINT++;		/* INT not allowed in deferred ISR! */
            HIM6X60DeferISR(hacb, deferredIsr);
         }
         break;

      case COMMAND_PHASE:
         OUTPUT(DmaCntrl0, ENDMA | FIFO_8 | FIFO_PIO | FIFO_WRITE | RSTFIFO);
         OUTPUT(SXfrCtl0, CHEN | SCSIEN | DMAEN);
         if ((scb->scbStatus & SCB_SENSE_DATA_VALID) != 0) {
            hacb->requestSenseCdb[4] = scb->senseDataLength;
            BLOCKOUTPUT(DmaData, hacb->requestSenseCdb,
                        sizeof(hacb->requestSenseCdb));
         } else
            BLOCKOUTPUT(DmaData, scb->cdb, scb->cdbLength);
         break;

      case MESSAGE_OUT_PHASE:
         if ((INPUT(SStat1) & SCSIPERR) != 0) {
            if (hacb->revision == 0)
               OUTPUT(SXfrCtl1, (UCHAR) (hacb->sXfrCtl1Image & ~ENSPCHK));
            OUTPUT(ClrSInt1, CLRSCSIPERR);
            hacb->ParityError = TRUE;
            prepareMessageOut(hacb, INITIATOR_DETECTED_ERROR_MSG);
            HIM6X60LogError(hacb, hacb->ActiveScb, 0, hacb->busID, hacb->lun,
                            SCB_ERROR_PARITY, hacb->scsiPhase);
         } else if (hacb->MsgOutLen == 0)
            prepareMessageOut(hacb, ABORT_MSG);
         OUTPUT(DmaCntrl0, 0);
         OUTPUT(SXfrCtl0, CHEN | SPIOEN);
         msgBytes = hacb->MsgOutLen;
         while (hacb->MsgOutLen != 0 && samePhaseREQuest(hacb, pioReady)) {
            if (hacb->MsgOutLen == 1)
               OUTPUT(ClrSInt1, CLRATNO);		/* Last byte */
            OUTPUT(ScsiDat, hacb->MsgOut[msgBytes - hacb->MsgOutLen--]);
            pioReady = FALSE;
         }
         if (msgBytes > 1 && (hacb->MsgOut[0] & IDENTIFY_MSG) != 0)
            memcpy(&hacb->MsgOut[0], &hacb->MsgOut[1], msgBytes - 1);
         if ((hacb->MsgOut[0] & IDENTIFY_MSG) == IDENTIFY_MSG)
            ;
         else if (hacb->MsgOut[0] == BUS_DEVICE_RESET_MSG) {
            resetSDTR(hacb, hacb->busID);
            hacb->DisconnectOK = TRUE;
         } else if (   hacb->MsgOut[0] == ABORT_MSG
                    || hacb->MsgOut[0] == RELEASE_RECOVERY_MSG)
            hacb->DisconnectOK = TRUE;
         else if (hacb->MsgOut[0] == INITIATOR_DETECTED_ERROR_MSG)
            hacb->ParityError = FALSE;
         else if (hacb->MsgOut[0] == MESSAGE_PARITY_ERROR_MSG)
            hacb->MsgParityError = FALSE;
         else if (memcmp(&hacb->MsgOut, &hacb->sdtrMsg,
                         sizeof(hacb->sdtrMsg) - 2) == 0) {
            if (hacb->MsgOutLen != 0)
               updateSDTR(hacb, hacb->busID, 0, 0, TRUE);
            else if (memcmp(&hacb->MsgIn, &hacb->sdtrMsg,
                            sizeof(hacb->sdtrMsg) - 2) == 0)
               hacb->negotiateSDTR &= ~(1 << hacb->busID);
         }
         if (hacb->MsgOutLen != 0) {		/* No response to ATN? */
            OUTPUT(ClrSInt1, CLRATNO);		/* OK, don't insist */
            hacb->MsgOutLen = 0;
         }
         break;

      case STATUS_PHASE:
         OUTPUT(DmaCntrl0, 0);
         OUTPUT(SXfrCtl0, CHEN | SPIOEN);
         while (samePhaseREQuest(hacb, pioReady)) {
            if ((INPUT(SStat1) & SCSIPERR) == 0)
               hacb->targetStatus = INPUT(ScsiDat);
            else {
               if (hacb->revision == 0)
                  OUTPUT(SXfrCtl1, (UCHAR) (hacb->sXfrCtl1Image & ~ENSPCHK));
               OUTPUT(ClrSInt1, CLRSCSIPERR);
               hacb->targetStatus = 0xFF;	/* Impossible status */
               hacb->ParityError = TRUE;
               prepareMessageOut(hacb, INITIATOR_DETECTED_ERROR_MSG);
               HIM6X60LogError(hacb, hacb->ActiveScb, 0, hacb->busID,
                               hacb->lun, SCB_ERROR_PARITY, hacb->scsiPhase);
               INPUT(ScsiDat);		/* Discard whatever is on the bus */
            }
            pioReady = FALSE;
         }
         break;

      case MESSAGE_IN_PHASE:
         OUTPUT(DmaCntrl0, 0);
         OUTPUT(SXfrCtl0, CHEN | SPIOEN);
         while (samePhaseREQuest(hacb, pioReady)) {
            if ((INPUT(SStat1) & SCSIPERR) != 0) {
               if (hacb->revision == 0)
                  OUTPUT(SXfrCtl1, (UCHAR) (hacb->sXfrCtl1Image & ~ENSPCHK));
               OUTPUT(ClrSInt1, CLRSCSIPERR);
               hacb->MsgInLen = 0;	/* Prepare to receive message again */
               hacb->MsgParityError = TRUE;
               prepareMessageOut(hacb, MESSAGE_PARITY_ERROR_MSG);
               HIM6X60LogError(hacb, hacb->ActiveScb, 0, hacb->busID,
                               hacb->lun, SCB_ERROR_PARITY, hacb->scsiPhase);
            }
            if (hacb->MsgParityError)
               INPUT(ScsiDat);			/* Discard the bad byte */
            else
               interpretMessageIn(hacb);	/* Accumulate/decode bytes */
            pioReady = FALSE;
         }
         break;

      default:
         break;
   }

}
/*-----------------------------------------------------------------------------
 For short transfers of information in 6X60 manual programmed IO mode (only
 MESSAGE IN, MESSAGE and STATUS bytes) there is less overhead to just wait
 on the SPIORDY bit than there is to enable an interrupt and leave the
 interrupt service.  If any other interrupt condition arises, we must bail out
 as well.  This does make the simplifying assumption that all targets are
 going to be reasonably prompt with these sorts of information transfers.  If
 ever this is not always the case, the failure will manifest through a SCSI
 bus reset and error log entry. */

BOOLEAN samePhaseREQuest(HACB *hacb, BOOLEAN pioReady) {

   USHORT watchdog = 0;

   while (   (INPUT(DmaStat) & INTSTAT) == 0
          && (INPUT(SStat0) & SPIORDY) == 0
          && pioReady == FALSE) {
      if (++watchdog == 0xFFFF) {
         HIM6X60LogError(hacb, hacb->ActiveScb, 0, hacb->busID, hacb->lun,
                         SCB_ERROR_BUS_TIMEOUT, hacb->scsiPhase);
         if (hacb->ActiveScb != NULL)
            hacb->ActiveScb->scbStatus = SCB_TIMEOUT;
         HIM6X60ResetBus(hacb, 0);
         return(FALSE);		/* NOW there's an interrupt condition! */
      }
   }
   return((INPUT(DmaStat) & INTSTAT) == 0);

}

/*-----------------------------------------------------------------------------
 Because the SCSI target is actually in control of all the information
 transfer phases, sometimes we have to humor it and either accept (and
 discard) ANY data it sends us or generate pad data to send it.  The 6X60 does
 this automatically, in either direction, if we assert BITBUCKET.  It's also
 important to assert ATN before we start; eventually the target will tire of
 data transfer and should transition to a MESSAGE phase.  If and when that
 happens, the target will be properly quashed with an ABORT message. */

VOID bitbucketAndABORT(HACB *hacb, UCHAR scbStatus) {

   TRACE(hacb, BITBUCKET_AND_ABORT, scbStatus, 0);
   if (scbStatus != 0 && hacb->ActiveScb != NULL)
      hacb->ActiveScb->scbStatus = scbStatus;
   hacb->BitBucket = TRUE;
   prepareMessageOut(hacb, ABORT_MSG);
   OUTPUT(SXfrCtl1, (UCHAR) (hacb->sXfrCtl1Image | BITBUCKET));

}
/*-----------------------------------------------------------------------------
 Programmed IO data transfer during a DATA OUT phase makes use of block IO
 instructions to transfer 16 bits of data for each IO register reference into
 the 6X60 host FIFO until all the available data is exhausted or an interrupt
 condition occurs.  Note that the data pointer in the HACB is advanced as we
 go along.  This means that we may have to adjust it back down if there is
 data left in the FIFO's at the end of the data transfer phase (see
 'updateDataPointer')). */

VOID dataOutPIO(HACB *hacb) {

   UCHAR DmaStatus, xferMode;
   ULONG watchdog, xferCount;

   TRACE(hacb, DATA_PHASE_PIO, hacb->DataLength, (ULONG) hacb->DataPointer);
   OUTPUT(DmaCntrl0, xferMode = ((hacb->revision == 0)
                        ? ENDMA | FIFO_16 | FIFO_PIO | FIFO_WRITE
                        : ENDMA | FIFO_16 | FIFO_PIO | DWORDPIO | FIFO_WRITE));
   OUTPUT(SXfrCtl0, CHEN | SCSIEN | DMAEN);
   while (hacb->DataLength > 0) {
      if (hacb->SegmentLength == 0) {
         hacb->DataPointer = HIM6X60GetVirtualAddress(hacb, hacb->ActiveScb,
                                                      hacb->DataPointer,
                                                      hacb->DataOffset,
                                                      &hacb->SegmentLength);
         hacb->SegmentLength = ADSS_MIN(hacb->SegmentLength, hacb->DataLength);
      } 
      while (hacb->SegmentLength > 0) {
         watchdog = 0;
         while ((  (DmaStatus = INPUT(DmaStat))
                 & (INTSTAT | DFIFOEMP)) == 0)
            if (++watchdog == 0x000FFFFF)
               if ((INPUT(ScsiSig) & REQ) != 0)
                  HIM6X60ResetBus(hacb, 0);
               else {
                  OUTPUT(ClrSInt1, CLRREQINIT);
                  if ((INPUT(ScsiSig) & REQ) != 0)
                     watchdog = 0;
                  else {
                     OUTPUT(SIMode1,
                            ENSCSIRST | ENPHASEMIS | ENBUSFREE | ENREQINIT);
                     return;
                  }
               }
         if ((DmaStatus & INTSTAT) != 0)	/* Possible phase change? */
            return;
         xferCount = (DmaStatus & DFIFOEMP) ? ADSS_MIN(hacb->SegmentLength, 128)
                                            : ADSS_MIN(hacb->SegmentLength, 64);
         if ((xferCount & 0x03) == 0 && hacb->revision > 0) {
            if (xferMode != (  ENDMA | FIFO_16 | FIFO_PIO | DWORDPIO
                             | FIFO_WRITE))
               OUTPUT(DmaCntrl0, xferMode = ENDMA | FIFO_16 | FIFO_PIO
                                             | DWORDPIO | FIFO_WRITE);
            BLOCKOUTDWORD(DmaData32, hacb->DataPointer, xferCount >> 2);
         } else if (xferCount & 0x01) {
            if (xferMode != (ENDMA | FIFO_8 | FIFO_PIO | FIFO_WRITE))
               OUTPUT(DmaCntrl0,
                      xferMode = ENDMA | FIFO_8 | FIFO_PIO | FIFO_WRITE);
            BLOCKOUTPUT(DmaData, hacb->DataPointer, xferCount);
         } else {
            if (xferMode != (ENDMA | FIFO_16 | FIFO_PIO | FIFO_WRITE))
               OUTPUT(DmaCntrl0,
                      xferMode = ENDMA | FIFO_16 | FIFO_PIO | FIFO_WRITE);
            BLOCKOUTWORD(DmaData, hacb->DataPointer, xferCount >> 1);
         }
         hacb->DataPointer += xferCount;
         hacb->DataLength -= xferCount;
         hacb->DataOffset += xferCount;
         hacb->SegmentLength -= xferCount;
      }
   }
   while ((INPUT(DmaStat) & INTSTAT) == 0)
      if ((INPUT(DmaStat) & DFIFOEMP) != 0 && (INPUT(SStat2) & SEMPTY) != 0) {
         OUTPUT(SIMode1, ENSCSIRST | ENPHASEMIS | ENBUSFREE | ENREQINIT);
         return;
      }
}
/*-----------------------------------------------------------------------------
 Programmed IO data transfer during a DATA IN phase is essentially the same as
 during data out, except for the direction.  See the comments for the
 preceding routine.  A significant difference, though, is that we cannot
 overshoot the amount of data transferred (as in a DATA OUT phase) so there is
 never any need for retroactive data pointer adjustment when the phase
 completes. */

VOID dataInPIO(HACB *hacb) {

   UCHAR DmaStatus, xferMode;
   ULONG watchdog, xferCount;

   TRACE(hacb, DATA_PHASE_PIO, hacb->DataLength, (ULONG) hacb->DataPointer);
   OUTPUT(DmaCntrl0, xferMode = ((hacb->revision == 0)
                         ? ENDMA | FIFO_16 | FIFO_PIO | FIFO_READ
                         : ENDMA | FIFO_16 | FIFO_PIO | DWORDPIO | FIFO_READ));
   OUTPUT(SXfrCtl0, SCSIEN | DMAEN | CHEN);
   while (hacb->DataLength > 0) {
      if (hacb->SegmentLength == 0) {
         hacb->DataPointer = HIM6X60GetVirtualAddress(hacb, hacb->ActiveScb,
                                                      hacb->DataPointer,
                                                      hacb->DataOffset,
                                                      &hacb->SegmentLength);
         hacb->SegmentLength = ADSS_MIN(hacb->SegmentLength, hacb->DataLength);
      } 
      while (hacb->SegmentLength > 0) {
         watchdog = 0;
         while ((  (DmaStatus = INPUT(DmaStat))
                 & (INTSTAT | DFIFOFULL)) == 0)
            if (++watchdog == 0x000FFFFF)
               if ((INPUT(ScsiSig) & REQ) != 0)
                  HIM6X60ResetBus(hacb, 0);
               else {
                  OUTPUT(ClrSInt1, CLRREQINIT);
                  if ((INPUT(ScsiSig) & REQ) != 0)
                     watchdog = 0;
                  else {
                     OUTPUT(SIMode1,
                            ENSCSIRST | ENPHASEMIS | ENBUSFREE | ENREQINIT);
                     return;
                  }
               }
         if ((DmaStatus & DFIFOFULL) == 0) {
            if ((xferCount = INPUT(FifoStat)) == 0)
               return;			/* Other interrupt service needed... */
         } else
            xferCount = ((DmaStatus & DFIFOFULL) == 0) ? 64 : 128;
         xferCount = ADSS_MIN(xferCount, hacb->SegmentLength);
         if ((xferCount & 0x03) == 0 && hacb->revision > 0) {
            if (xferMode != (  ENDMA | FIFO_16 | FIFO_PIO | DWORDPIO
                             | FIFO_READ))
               OUTPUT(DmaCntrl0, xferMode = ENDMA | FIFO_16 | FIFO_PIO
                                             | DWORDPIO | FIFO_READ);
            BLOCKINDWORD(DmaData32, hacb->DataPointer, xferCount >> 2);
         } else if (xferCount & 0x01) {
            if (xferMode != (ENDMA | FIFO_8 | FIFO_PIO | FIFO_READ))
               OUTPUT(DmaCntrl0,
                      xferMode = ENDMA | FIFO_8 | FIFO_PIO | FIFO_READ);
            BLOCKINPUT(DmaData, hacb->DataPointer, xferCount);
         } else {
            if (xferMode != (ENDMA | FIFO_16 | FIFO_PIO | FIFO_READ))
               OUTPUT(DmaCntrl0,
                      xferMode = ENDMA | FIFO_16 | FIFO_PIO | FIFO_READ);
            BLOCKINWORD(DmaData, hacb->DataPointer, xferCount >> 1);
         }
         hacb->DataPointer += xferCount;
         hacb->DataLength -= xferCount;
         hacb->DataOffset += xferCount;
         hacb->SegmentLength -= xferCount;
      }
   }
   OUTPUT(SIMode1, ENSCSIRST | ENPHASEMIS | ENBUSFREE | ENREQINIT);

}

#ifdef OLD_STUFF
/*-----------------------------------------------------------------------------
 DMA data transfer for the 6X60 uses the system DMA controller as the master
 because the ISA plug-in card (or 6X60 on the motherboard) has no bus
 master capabilities.  Note that scatter/gather is implemented in this
 procedure by making note of the remaining, contiguous data transfer length
 that can be accomodated by DMA.  When this expires (if the SCSI target is
 still in a data transfer phase), a DMADONE interrupt is asserted and this
 procedure is ultimately called to map the next segment for data transfer.
 The function HIM6X60GetPhysicalAddress converts the linear address to a
 physical (bus) address, after which HIM6X60MapDMA programs the system
 DMA controller to be ready for the transfer.  Once this has been done, it is
 a simple matter to program the 6X60 to begin the data transfer. */

VOID dataPhaseDMA(HACB *hacb) {

   if (hacb->SegmentLength == 0) {
      hacb->SegmentAddress = HIM6X60GetPhysicalAddress(hacb, hacb->ActiveScb,
                                                       hacb->DataPointer,
                                                       hacb->DataOffset,
                                                       &hacb->SegmentLength);
      hacb->SegmentLength = ADSS_MIN(hacb->SegmentLength, hacb->DataLength);
   }
   HIM6X60MapDMA(hacb, hacb->ActiveScb, hacb->DataPointer,
                 hacb->SegmentAddress, hacb->SegmentLength,
                 (BOOLEAN) (hacb->scsiPhase == DATA_IN_PHASE));
   hacb->DmaActive = TRUE;

}

/*-----------------------------------------------------------------------------
 This procedure is called by the OS specific code in response to the
 HIM6X60MapDMA call above.  At this point, the system DMA controller has been
 programmed with the appropriate physical address, transfer count and transfer
 mode for the type of DMA channel.  We may enable the desired mode of
 transfers on the 6X60 and DMA will commence.  Because the 6360 revision 1
 parts have a hardware design flaw that can cause DMA transfers to "hang" if
 another channel (e.g. floppy) is in use, in this case we must also start a
 watchdog timer to guard against this possibility.  Note that this procedure
 may be called within the context of the interrupt service or later, hence the
 need to check whether or not to set INTEN in the DMA control register. */

VOID HIM6X60DmaProgrammed(HACB *hacb) {

   UCHAR dmaControl, fifoCount;

   TRACE(hacb, DATA_PHASE_DMA, hacb->SegmentLength,
         PHYSICAL_TO_ULONG(hacb->SegmentAddress));
   OUTPUT(SIMode0, ENDMADONE);
   if ((fifoCount = INPUT(FifoStat)) != 0) {
      fifoCount += (INPUT(SStat2) & SFCNT) - (INPUT(SStat3) & OFFCNT);
      if (fifoCount != 0)
         OUTPUT(StCnt0, fifoCount);
   }
   dmaControl = ENDMA | FIFO_DMA;
   dmaControl |= (hacb->dmaChannel == 0) ? FIFO_8 : FIFO_16;
   dmaControl |= (hacb->scsiPhase == DATA_OUT_PHASE) ? FIFO_WRITE : FIFO_READ;
   if (hacb->disableINT == 0)
      dmaControl |= INTEN;
   OUTPUT(DmaCntrl0, dmaControl);
   OUTPUT(SXfrCtl0, CHEN | SCSIEN | DMAEN);
   if (hacb->irqConnected && hacb->revision == 1)
      HIM6X60Watchdog(hacb, watchdog, WATCHDOG_DMA_HANG);

}
#endif

/*-----------------------------------------------------------------------------
 A MESSAGE byte with no parity errors is present on the SCSI bus and ACK
 has not yet been asserted by the 6X60.  Read the UNLATCHED SCSI data to
 obtain the message byte for processing so that the ACKnowledge can be held
 off.  If a single-byte messge has been received, respond appropriately (this
 may include asserting ATN before ACK is raised so that we may send a MESSAGE
 byte in the next phase).  If the message is an extended, multiple byte
 message defer processing until all the message bytes have been received.  In
 both cases, at the end of processing, read the message byte a second time but
 from latched SCSI data; this instructs the 6X60 to generate ACK to complete
 the SCSI bus handshake. */

VOID interpretMessageIn(HACB *hacb) {

   LUCB *lucb;
   BOOLEAN messageComplete = TRUE;
   ADSS_SCB *scb;

   TRACE(hacb, MESSAGE_IN, ((USHORT) hacb->MsgInLen) << 8 | INPUT(ScsiBus),
         hacb->targetStatus);
   hacb->MsgIn[hacb->MsgInLen++] = INPUT(ScsiBus);
   if ((hacb->MsgIn[0] & IDENTIFY_MSG) == IDENTIFY_MSG)
      if (hacb->lun != DISCONNECTED)
         prepareMessageOut(hacb, ABORT_MSG);
      else {
         lucb = HIM6X60GetLUCB(hacb, 0, hacb->busID,
                               hacb->lun = hacb->MsgIn[0] & 0x07);
         hacb->ActiveScb = scb = lucb->activeScb;
         if (scb == NULL) {
            prepareMessageOut(hacb, BUS_DEVICE_RESET_MSG);
            HIM6X60LogError(hacb, NULL, 0, hacb->busID, 0xFF,
                            SCB_ERROR_INVALID_RESELECTION, hacb->MsgIn[0]);
         } else if (scb->linkedScb != NULL) {
            unlinkScb(&hacb->eligibleScb, scb);
            if (scb->linkedScb->function == SCB_TERMINATE_IO)
               prepareMessageOut(hacb, TERMINATE_IO_PROCESS_MSG);
            else
               prepareMessageOut(hacb, ABORT_MSG);
         } else {		/* IDENTIFY message implies RESTORE POINTERS */
            hacb->DataPointer = scb->dataPointer;
            hacb->DataLength = scb->dataLength;
            hacb->DataOffset = scb->dataOffset;
            hacb->SegmentAddress = scb->segmentAddress;
            hacb->SegmentLength = scb->segmentLength;
         }
      }
   else if ((scb = hacb->ActiveScb) == NULL)
      prepareMessageOut(hacb, BUS_DEVICE_RESET_MSG);
   else switch (hacb->MsgIn[0]) {
      case COMMAND_COMPLETE_MSG:
         if ((scb->scbStatus & SCB_SENSE_DATA_VALID) == 0) {
            scb->transferLength += scb->provisionalTransfer
                                    + (scb->dataLength - hacb->DataLength);
            scb->transferResidual -= scb->provisionalTransfer
                                      + (scb->dataLength - hacb->DataLength);
            switch (scb->targetStatus = hacb->targetStatus) {
               case CONDITION_MET:
               case GOOD:
               case INTERMEDIATE:
               case INTERMEDIATE_CONDITION_MET:
                  if (hacb->ParityError) {
                     hacb->ParityError = FALSE;
                     scb->scbStatus = SCB_PARITY_ERROR;
                  } else
                     scb->scbStatus = SCB_COMPLETED_OK;
                  break;

               case CHECK_CONDITION:
               case COMMAND_TERMINATED:
                  if (scb->scbStatus != SCB_PENDING)
                     break;
                  else if (   (scb->flags & SCB_DISABLE_AUTOSENSE) == 0
                           && scb->senseData != NULL
                           && scb->senseDataLength != 0) {
                     scb->flags |= SCB_DISABLE_DMA;
                     scb->scbStatus = SCB_SENSE_DATA_VALID | SCB_PENDING;
                     scb->dataPointer = scb->senseData;
                     scb->dataLength = scb->senseDataLength;
                     scb->dataOffset = 0;
                     ZERO_PHYSICAL_ADDRESS(scb->segmentAddress);
                     scb->segmentLength = scb->senseDataLength;
                     linkScbPreemptive(&hacb->eligibleScb, scb);
                  } else
                     scb->scbStatus = SCB_ERROR;
                  break;

               case BUSY:
               case QUEUE_FULL:
                  scb->scbStatus = SCB_BUSY;
                  break;

               default:
                  scb->scbStatus = SCB_ERROR;
                  break;
            }
         } else if (hacb->targetStatus == GOOD)
            scb->scbStatus |= SCB_ERROR;
         else
            scb->scbStatus = SCB_REQUEST_SENSE_FAILURE;
         hacb->DisconnectOK = TRUE;
         break;

      case DISCONNECT_MSG:
         if ((scb->scbStatus & SCB_SENSE_DATA_VALID) == 0)
            scb->provisionalTransfer = scb->dataLength - hacb->DataLength;
         hacb->DisconnectOK = TRUE;
         break;

      case SAVE_DATA_POINTER_MSG:
         if ((scb->scbStatus & SCB_SENSE_DATA_VALID) == 0) {
            scb->transferLength += scb->dataLength - hacb->DataLength;
            scb->transferResidual -= scb->dataLength - hacb->DataLength;
         }
         scb->dataPointer = hacb->DataPointer;
         scb->dataLength = hacb->DataLength;
         scb->dataOffset = hacb->DataOffset;
         scb->segmentAddress = hacb->SegmentAddress;
         scb->segmentLength = hacb->SegmentLength;
         break;

      case RESTORE_POINTERS_MSG:
         hacb->DataPointer = scb->dataPointer;
         hacb->DataLength = scb->dataLength;
         hacb->DataOffset = scb->dataOffset;
         hacb->SegmentAddress = scb->segmentAddress;
         hacb->SegmentLength = scb->segmentLength;
         scb->provisionalTransfer = 0;
         break;

      case EXTENDED_MSG:
         if (     hacb->MsgInLen == 1
               || hacb->MsgInLen < hacb->MsgIn[1] + 2)
            messageComplete = FALSE;
         else if (   hacb->MsgIn[2] == SYNCHRONOUS_DATA_TRANSFER_MSG
                  && hacb->Synchronous)
            negotiateSDTR(hacb);
         else
            prepareMessageOut(hacb, MESSAGE_REJECT_MSG);
         break;

      case MESSAGE_REJECT_MSG:
         if ((hacb->MsgOut[0] & IDENTIFY_MSG) == IDENTIFY_MSG)
            scb->scbStatus = SCB_INVALID_LUN;
         else if (memcmp(&hacb->MsgOut, &hacb->sdtrMsg,
                         sizeof(hacb->sdtrMsg) - 2) == 0) 
            updateSDTR(hacb, hacb->busID, 0, 0, TRUE);
         else if (scb->linkedScb != NULL) {
            scb->linkedScb->scbStatus = SCB_MESSAGE_REJECTED;
            HIM6X60CompleteSCB(hacb, scb->linkedScb);
            scb->linkedScb = NULL;
         } else
            scb->scbStatus = SCB_MESSAGE_REJECTED;
         hacb->MsgOutLen = 1;		/* Just in case ATN is still on */
         hacb->MsgOut[0] = NOP_MSG;
         break;

      default:
         prepareMessageOut(hacb, MESSAGE_REJECT_MSG);
         break;
   }
   if (messageComplete)
      hacb->MsgInLen = 0;		/* Reset for any new message(s) */
   INPUT(ScsiDat);			/* Reading latched data releases ACK */

}
/*-----------------------------------------------------------------------------
 Synchronous data transfer negotiations should be able to complete with the
 exchange of a message from the initiator and one from the target, but this
 procedure will accommodate redundant targets that insist on echoing any
 changed values until both messages match each other exactly.  If the target
 proposes a synchronous agreement with an SDTR, the 6X60 accepts it by
 responding with an SDTR whose minimum transfer period is greater than or
 equal to that suggested by the target and whose REQ/ACK offset field is less
 than or equal to that suggested by the target.  The only cases in which
 synchronous negotiations are refused are a) the HACB control variable
 'synchronous' is FALSE or b) the target wishes to transfer data at too slow a
 rate.  The latter is a 6X60 limitation:  the largest transfer period
 programmable in the SCSIRATE register is 9 clock periods (450ns with a 20 MHz
 clock).  When the 6X60 initiates SDTR negotiations (a user configurable
 option), this procedure checks for a) the return of an SDTR message with a
 non-zero REQ/ACK offset (synchronous is OK), b) the return of an SDTR message
 with zero in the REQ/ACK offset (use asynchronous) or c) the return of a
 MESSAGE REJECT message (use asynchronous mode).  Note that described in the
 narration here, some of these events actually take place in the higher level
 'interpretMessageIn' procedure that calls this one. */

VOID negotiateSDTR(HACB *hacb) {

   USHORT busID = hacb->busID;		/* Optimize array references */
   UCHAR syncCycles, syncOffset;

   TRACE(hacb, NEGOTIATE_SDTR,
         ((USHORT) hacb->MsgOut[4]) << 8 | hacb->MsgOut[3],
         ((USHORT) hacb->MsgIn[4]) << 8 | hacb->MsgIn[3]);
   hacb->negotiateSDTR |= 1 << busID;	/* Mark negotiations in progress */
   if (hacb->MsgIn[4] == 0)		/* Target refuses synchronous? */
      updateSDTR(hacb, busID, 0, 0, TRUE);
   else if (  (  ((USHORT) hacb->MsgIn[3] << 2)
               + hacb->clockPeriod - 1)
            / hacb->clockPeriod > 9) {	/* Target too slow for 6X60? */
      prepareMessageOut(hacb, MESSAGE_REJECT_MSG);
      updateSDTR(hacb, busID, 0, 0, TRUE);
   } else {
      syncCycles = (hacb->MsgIn[3] < hacb->sdtrMsg.transferPeriod)
                     ? (((USHORT) hacb->sdtrMsg.transferPeriod << 2)
                        + hacb->clockPeriod - 1) / hacb->clockPeriod
                     : (((USHORT) hacb->MsgIn[3] << 2) + hacb->clockPeriod - 1)
                        / hacb->clockPeriod;
      syncOffset = ADSS_MIN(hacb->MsgIn[4], hacb->sdtrMsg.reqAckOffset);
      updateSDTR(hacb, busID, syncCycles, syncOffset, FALSE);
      if (memcmp(&hacb->MsgOut, &hacb->sdtrMsg,
                 sizeof(hacb->sdtrMsg) - 2) == 0)
         hacb->negotiateSDTR &= ~(1 << busID);
      else {
         memcpy(&hacb->MsgOut, &hacb->sdtrMsg,
                hacb->MsgOutLen = sizeof(hacb->sdtrMsg));
         hacb->MsgOut[3] = ADSS_MAX(hacb->sdtrMsg.transferPeriod, hacb->MsgIn[3]);
         hacb->MsgOut[4] = ADSS_MIN(hacb->sdtrMsg.reqAckOffset, hacb->MsgIn[4]);
      }
   }
   if ((hacb->negotiateSDTR & (1 << busID)) != 0)
      OUTPUT(ScsiSig, (UCHAR) (INPUT(ScsiSig) & (CD | IO | MSG) | ATN));

}
/*-----------------------------------------------------------------------------
 When SDTR negotiations are completed (either because a pair of SDTR messages
 have been exchanged, a MESSAGE REJECT message has been received or the target
 refused the MESSAGE phase all together), it is necessary to record the
 results in the relevant data structures and to program the 6X60 rate control
 register.  Note the importance of recording the outcome in the 6X60 Stack
 locations:  this is so a subsequent user (after a system boot or double boot)
 of the 6X60 can inherit the results of SDTR negotiations.  This is essential
 unless a RST is issued to cancel all synchronous agreements. */

VOID updateSDTR(HACB *hacb, USHORT busID, UCHAR syncCycles,
                UCHAR syncOffset, BOOLEAN negotiationsCompleted) {

   UCHAR StackOffset;

   hacb->syncCycles[busID] = syncCycles;
   hacb->syncOffset[busID] = syncOffset;
   if (negotiationsCompleted)
      hacb->negotiateSDTR &= ~(1 << hacb->busID);
   if (     hacb->signature == 0x52
         || hacb->signature == 0x53
         || hacb->signature == 0x54)
      StackOffset = ((hacb->ownID - busID) & 0x07) + 2;
   else if (hacb->signature == 0xAA55)
      StackOffset = ((hacb->ownID - busID) & 0x07) + 3;
   else if (hacb->signature == 0x03020100)
      StackOffset = 8 + busID;
   else
      StackOffset = 0;
   OUTPUT(ScsiRate, (UCHAR) ((syncOffset == 0)
                                      ? 0
                                      : ((syncCycles - 2) << 4) | syncOffset));
   if (StackOffset != 0) {
      OUTPUT(DmaCntrl1, StackOffset);
      OUTPUT(Stack, (UCHAR) ((syncOffset == 0)
                               ? 0x80
                               : 0x80 | ((syncCycles - 2) << 4) | syncOffset));
   }

}
/*-----------------------------------------------------------------------------
 If a SCSI bus RST has been issued or detected, all existing SDTR negotiations
 are invalidated.  We must update the relevant data structures in both the
 HACB and the 6X60 Stack locations. */

VOID resetSDTR(HACB *hacb, USHORT busID) {

   UCHAR StackData[8], StackOffset;

   if (busID == hacb->ownID) {
      memset(&hacb->syncCycles, 0, sizeof(hacb->syncCycles));
      memset(&hacb->syncOffset, 0, sizeof(hacb->syncOffset));
      if (hacb->Synchronous && hacb->InitiateSDTR) {
         hacb->negotiateSDTR = 0xFF;
         memset(StackData, 0, sizeof(StackData));
      } else {
         hacb->negotiateSDTR = 0;
         memset(StackData, 0x80, sizeof(StackData));
      }
      OUTPUT(ScsiRate, 0);
      if (     hacb->signature == 0x52
            || hacb->signature == 0x53
            || hacb->signature == 0x54) {
         OUTPUT(DmaCntrl1, 3);
         BLOCKOUTPUT(Stack, StackData, sizeof(StackData) - 1);
      } else if (hacb->signature == 0xAA55) {
         OUTPUT(DmaCntrl1, 4);
         BLOCKOUTPUT(Stack, StackData, sizeof(StackData) - 1);
      } else if (hacb->signature == 0x03020100) {
         OUTPUT(DmaCntrl1, 8);
         BLOCKOUTPUT(Stack, StackData, sizeof(StackData));
      }
   } else {
      hacb->syncCycles[busID] = 0;
      hacb->syncOffset[busID] = 0;
      if (hacb->Synchronous && hacb->InitiateSDTR)
         hacb->negotiateSDTR |= 1 << hacb->busID;
      if (     hacb->signature == 0x52
            || hacb->signature == 0x53
            || hacb->signature == 0x54)
         StackOffset = ((hacb->ownID - busID) & 0x07) + 2;
      else if (hacb->signature == 0xAA55)
         StackOffset = ((hacb->ownID - busID) & 0x07) + 3;
      else if (hacb->signature == 0x03020100)
         StackOffset = 8 + busID;
      else
         StackOffset = 0;
      OUTPUT(ScsiRate, 0);
      if (StackOffset != 0) {
         OUTPUT(DmaCntrl1, StackOffset);
         OUTPUT(Stack, (UCHAR) ((hacb->Synchronous && hacb->InitiateSDTR)
                                ? 0 : 0x80));
      }
   }

}
/*-----------------------------------------------------------------------------
 Procedures to manipulate queues of SCB's, linking one in at the head or tail
 of a queue or searching a queue for the specified SCB and snipping it out. */

VOID linkScbPreemptive(ADSS_SCB **queueHead, ADSS_SCB *scb) {

   scb->chain = *queueHead;
   *queueHead = scb;

}

VOID linkScb(ADSS_SCB **queueHead, ADSS_SCB *scb) {

   while (*queueHead != NULL)
      queueHead = &(*queueHead)->chain;
   scb->chain = *queueHead;
   *queueHead = scb;

}

BOOLEAN unlinkScb(ADSS_SCB **queueHead, ADSS_SCB *scb) {

   while (*queueHead != NULL)
      if (*queueHead == scb) {
         *queueHead = scb->chain;
         scb->chain = NULL;
         return(TRUE);
      } else
         queueHead = &(*queueHead)->chain;
   return(FALSE);

}

/*-----------------------------------------------------------------------------
 These simple little steps to build a one-byte message in the HACB message out
 control structures and to assert ATN to attract the target's notice were
 needed so often it was getting tiresome. */

VOID prepareMessageOut(HACB *hacb, UCHAR message) {

   hacb->MsgOutLen = 1;
   hacb->MsgOut[0] = message;
   OUTPUT(ScsiSig, (UCHAR) (INPUT(ScsiSig) & (CD | IO | MSG) | ATN));

}
#if (DBG_TRACE)

/*-----------------------------------------------------------------------------
 Debugging trace procedure maintains a circular log of significant events and
 corresponding AIC-6X60 hardware state when the DBG_TRACE switch is TRUE. */

VOID debugTrace(HACB *hacb, UCHAR event, ULONG data0, ULONG data1) {

   TRACE_LOG *traceLog;

   if (!hacb->traceEnabled)
      return;
   traceLog = &hacb->traceLog[hacb->traceIndex];
   hacb->traceIndex = (hacb->traceIndex + 1) & LAST(hacb->traceLog);
   traceLog->sequence = (UCHAR) hacb->traceCount++;
   traceLog->event = event;
   traceLog->scsiPhase = hacb->scsiPhase;
   traceLog->busID = hacb->busID;
   traceLog->data[0] = (USHORT) data0;
   traceLog->data[1] = (USHORT) data1;
   traceLog->ScsiSig = INPUT(ScsiSig);
   traceLog->SStat0 = INPUT(SStat0);
   traceLog->SStat1 = INPUT(SStat1);
   traceLog->SStat2 = INPUT(SStat2);
   traceLog->DmaCntrl0 = INPUT(DmaCntrl0);
   traceLog->DmaStat = INPUT(DmaStat);
   traceLog->FifoStat = INPUT(FifoStat);
   traceLog->currentState = hacb->currentState;

}

#endif
