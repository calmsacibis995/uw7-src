#ident	"@(#)kern-pdi:io/hba/adsa/him_code/him.c	1.5"

/* $Header$ */
/****************************************************************************
*
*  Module Name:  HIM.C
*
*  Description:
*
*  Programmers:  Paul von Stamwitz
*                Chuck Fannin
*
*  Notes:        NONE
*
*  Entry Point(s):
*    scb_send    - Standard host adapter operations.
*    int_handler - Arrow interrupt handler.
*    scb_special - Abort, Host Adapter reset operations.
*
*  Revisions -
*
****************************************************************************/
#include <io/hba/adsa/him_code/him_scb.h>
#include <io/hba/adsa/him_code/him_equ.h>
#include <io/hba/adsa/him_code/seq_off.h>

/*********************************************************************
*
*  Function Prototypes
*
*********************************************************************/

int      scb_send( him_config *, scb_struct * );
UBYTE    int_handler( him_config * );
int      scb_special( UBYTE, him_config *, scb_struct * );

void     scb_enable_int( him_config * );
void     scb_disable_int( him_config * );
UBYTE    scb_poll_int( him_config * );

void     reset_channel( him_struct * );
void     reset_scsi( him_struct * );

void     scb_LoadEntryTbl( him_struct *, UWORD );

int      scb_int_enque(him_struct *, scb_struct * );
int      scb_int_preempt(him_struct *, UBYTE );
scb_struct *scb_int_deque(him_struct * );
void     int_check_condition( him_struct *, scb_struct * );

int      scb_ext_enque( him_struct *, scb_struct * );
int      scb_ext_preempt( him_struct *, UBYTE );
scb_struct *scb_ext_deque( him_struct *);
void     ext_check_condition( him_struct *, scb_struct * );
void     scb_ext_addfree( him_struct *, UBYTE );

int      SaveDrvrScratchRAM( him_config * );
int      SaveBIOSScratchRAM( him_config * );
int      SaveBIOSSequencer( him_config * );
int      LoadSequencer( him_config * , UBYTE *, int);
int      LoadDrvrScratchRAM( him_config * );
int      LoadBIOSScratchRAM( him_config * );
void     mov_ptr_to_scratch( UWORD, DWORD );

int      scb_AsynchEvent( DWORD, him_config *, void *);
int      scb_channel_check( UWORD);

#ifdef _HIM_DEBUG

UBYTE    send_command( him_struct * );
void     terminate_command( him_struct * , scb_struct * );
void     post_command( him_struct * );
UBYTE    intselto( him_struct * );
void     abort_channel( him_struct *, UBYTE );
void     badseq( him_struct *, scb_struct * );
void     cdb_abort( him_struct * , scb_struct * );
void     check_length( him_struct * , scb_struct * );
void     extmsgi( him_struct * , scb_struct * );
UBYTE    extmsgo( him_struct * , scb_struct * );
void     handle_msgi( him_struct * , scb_struct * );
UBYTE    intfree( him_struct * );
void     intsrst( him_struct * );
void     negotiate( him_struct * , scb_struct * );
UBYTE    parity_error( him_struct * );
void     sendmo( him_struct * , scb_struct * );
UBYTE    syncset( scb_struct * );
void     synnego( him_struct * , scb_struct * );
UBYTE    target_abort( him_struct * , scb_struct * );
UBYTE    wt4req( him_struct * );
void     scb_renego( him_config * , UBYTE );
UBYTE    scb_non_init( him_config * , scb_struct * );
void     scb_abort( him_config * , scb_struct * );
void     scb_abort_active( him_config *, scb_struct * );
void     scb_abort_ext( him_config *, scb_struct * );
UBYTE    scb_send_trm_msg( him_config *, UBYTE );
UBYTE    scb_trm_cmplt( him_config *, UBYTE, UBYTE );
UBYTE    wt4bfree( him_struct * );
void     scb_enque( him_config * , scb_struct *);
void     scb_enque_hd( him_config * , scb_struct * );
UBYTE    scb_deque( him_config * , scb_struct *);
void     scb_hard_reset( him_config *, scb_struct * );
void     scb_bus_reset( him_config * );
void     ha_hard_reset( him_struct * );
void     scb_proc_bkpt( him_struct * );
scb_struct *find_chained_scb( him_config * , scb_struct * );
void     adjust_him_environment( him_config * );
void     insert_bookmark( him_struct * );
void     remove_bookmark( him_struct * );
int      wt4msgo( him_struct * );

#else

static UBYTE    send_command( him_struct * );
static void     terminate_command( him_struct * , scb_struct * );
static void     post_command( him_struct * );
static UBYTE    intselto( him_struct * );
static void     abort_channel( him_struct *, UBYTE );
static void     badseq( him_struct *, scb_struct * );
static void     cdb_abort( him_struct * , scb_struct * );
static void     check_length( him_struct * , scb_struct * );
static void     extmsgi( him_struct * , scb_struct * );
static UBYTE    extmsgo( him_struct * , scb_struct * );
static void     handle_msgi( him_struct * , scb_struct * );
static UBYTE    intfree( him_struct * );
static void     intsrst( him_struct * );
static void     negotiate( him_struct * , scb_struct * );
static UBYTE    parity_error( him_struct * );
static void     sendmo( him_struct * , scb_struct * );
static UBYTE    syncset( scb_struct * );
static void     synnego( him_struct * , scb_struct * );
static UBYTE    target_abort( him_struct * , scb_struct * );
static UBYTE    wt4req( him_struct * );
static void     scb_renego( him_config * , UBYTE );
static UBYTE    scb_non_init( him_config * , scb_struct * );
static void     scb_abort( him_config * , scb_struct * );
static void     scb_abort_active( him_config *, scb_struct * );
static void     scb_abort_ext( him_config *, scb_struct * );
static UBYTE    scb_send_trm_msg( him_config *, UBYTE );
static UBYTE    scb_trm_cmplt( him_config *, UBYTE, UBYTE );
static UBYTE    wt4bfree( him_struct * );
static void     scb_enque( him_config * , scb_struct *);
static void     scb_enque_hd( him_config * , scb_struct * );
static UBYTE    scb_deque( him_config * , scb_struct *);
static void     scb_hard_reset( him_config *, scb_struct * );
static void     scb_bus_reset( him_config * );
static void     ha_hard_reset( him_struct * );
static void     scb_proc_bkpt( him_struct * );
static scb_struct *find_chained_scb( him_config * , scb_struct * );
static void     adjust_him_environment( him_config * );
static void     insert_bookmark( him_struct * );
static void     remove_bookmark( him_struct * );
static int      wt4msgo( him_struct * );

#endif

extern   UBYTE E_Seq_00[];
extern   UBYTE E_Seq_01[];
extern   int   E_SeqExist[];
extern   UBYTE E_scratch_code[];
extern   int   E_scratch_code_size;

extern   void  SCBCompleted (him_config *, scb_struct *);

/*********************************************************************
*
*   scb_send routine -
*
*   Entry for execution of a STB from the host.
*
*  Return Value:  
*                  
*  Parameters:    config_ptr
*                 in: scb_ptr - SCB structure contains opcode and any
*                               needed parameters / pointers.
*
*  Activation:    ASPI layer, normal operation.
*
*  Remarks:       When HIM / Sequencer handling of an SCB is complete,
*                 the HIM layer calls SCBCompleted in the ASPI layer.
*
*********************************************************************/
int scb_send (him_config *config_ptr, scb_struct *scb_pointer)
{
   register him_struct *ha_ptr;
   register scb_struct *scb_ptr;
   register UBYTE temp;
   UWORD sav_state;
   int efl;                   /* -ali */

   ha_ptr = config_ptr->Cfe_HaDataPtr;

   /*   INTR_OFF; */
   efl = SaveAndDisable();       /* -ali */
   sav_state = ha_ptr->g_state;
   SET_STATE(config_ptr, WATCH_ON); 

   /* Check for non-initiator SCSI command */

   if ((scb_pointer->SCB_Cmd != EXEC_SCB) &&
       (scb_pointer->SCB_Cmd != NO_OPERATION))
   {
      scb_non_init (config_ptr, scb_pointer);
      /* INTR_ON; */
      RestoreState(efl);      /* -ali */
      return(0);
   }
   if ((ha_ptr->HaFlags & HAFL_SWAP_ON) &&
       (ha_ptr->HaFlags & (HAFL_BIOS_CACHED | HAFL_BIOS_RAM)))
   {
      if (scb_special( H_BIOS_OFF, config_ptr, scb_pointer))
      {
         return(-1);
      }
   }
   /*Temporary Patch, no OSM chained SCB's, or tagging */
   scb_pointer->SCB_Next = (scb_struct * ) NOT_DEFINED;

   if (ha_ptr->Head_Of_Queue != (scb_struct * ) NOT_DEFINED)    /* Find insertion point */
   {
      scb_ptr = ha_ptr->End_Of_Queue;           /* Add at End of existing Q */
      scb_ptr->SCB_Next = scb_pointer;
      scb_ptr = scb_ptr->SCB_Next;
   }
   else
   {
      scb_ptr = scb_pointer;                    /* Inactive Q, initialize */
      ha_ptr->Head_Of_Queue = scb_ptr;
      /* INTR_OFF; */
      adjust_him_environment( config_ptr);      /* Re-adjust pointers */
      ha_ptr->AConfigPtr->Cfe_ConfigFlags &= ~DRIVER_IDLE;
      if (config_ptr->Cfe_SCSIChannel == B_CHANNEL)
         ha_ptr->AConfigPtr->Cfe_ConfigFlags &= ~DRIVER_IDLE;
      if (ha_ptr->AConfigPtr->Cfe_ScbParam.Prm_MaxNTScbs != ha_ptr->max_nontag_cmd)
      {
         if ( 0 < ha_ptr->AConfigPtr->Cfe_ScbParam.Prm_MaxNTScbs < 3)
            ha_ptr->max_nontag_cmd = ha_ptr->AConfigPtr->Cfe_ScbParam.Prm_MaxNTScbs;
      }
      if (ha_ptr->AConfigPtr->Cfe_ScbParam.Prm_MaxTScbs !=
         ha_ptr->max_tag_cmd)
      {
         if (ha_ptr->HaFlags & HAFL_SWAP_ON)
         {
            temp = MAX_EXT_TAG;
         }
      else
      {
         temp = MAX_INT_TAG;
      }
      if ( 0 < ha_ptr->AConfigPtr->Cfe_ScbParam.Prm_MaxTScbs < temp)
         ha_ptr->max_tag_cmd = ha_ptr->AConfigPtr->Cfe_ScbParam.Prm_MaxTScbs;
      }
   }
   for (;;scb_ptr = scb_ptr->SCB_Next)          /* Change SCB state variables */
   {
      scb_ptr->SCB_Stat = SCB_PENDING;
      scb_ptr->SCB_MgrStat = SCB_PROCESS;
      if (scb_ptr->SCB_Next == (scb_struct * ) NOT_DEFINED) 
    break;
   }
   ha_ptr->End_Of_Queue = scb_ptr;
   /* INTR_ON; */

   /* Arrow interrupt off, must exit */

   if (sav_state == WATCH_OFF)
   {
      /* INTR_OFF; */
      if (send_command(ha_ptr))                    /* Try to Send to Arrow */
      {
         OUTBYTE(ha_ptr->hcntrl, INBYTE(ha_ptr->hcntrl) | SWINT);
      }
      SET_STATE (config_ptr, WATCH_OFF);
   }
   /* INTR_ON; */
   RestoreState(efl);      /* -ali */
   return(0);
}
/*********************************************************************
*
*  int_handler interrupt service routine -
*
*  Return Value:  Interrupt Status
*                  
*  Parameters:    config_ptr
*
*  Activation:    Arrow interrupt via Aspi Layer
*                  
*  Remarks:       More than one interrupt status bit can legally be
*                 set. It is also possible that no bits are set, in
*                 the case of using int_handler for polling.
*                  
*********************************************************************/
UBYTE int_handler (him_config *config_ptr)
{
   register him_struct *ha_ptr;
   register scb_struct *scb_ptr;
   register UBYTE swap_mode, i, byte_buf;
   register UWORD hcntrl;
   UBYTE int_status, ret_status, bios_int = 0;
   int efl;          /* -ali */
   UWORD sav_state;

   ha_ptr    = config_ptr->Cfe_HaDataPtr;
   hcntrl    = ha_ptr->hcntrl;
   swap_mode = ha_ptr->HaFlags & HAFL_SWAP_ON;

   /* INTR_OFF; */
   sav_state = ha_ptr->g_state;
   efl = SaveAndDisable();       /* -ali */
   READ_HINTSTAT( hcntrl, ha_ptr->intstat, ret_status);
   int_status = ret_status;
   ret_status &= ANYINT;

   if ((sav_state == WATCH_ON) ||  (ret_status == 0))
   {
      RestoreState(efl);           /* -ali */
      return(ret_status);
   }
   SET_STATE(config_ptr, WATCH_ON); 

   while ((int_status & ANYINT) || (ha_ptr->done_cmd))
   {
      if (int_status & CMDCMPLT)                   /* Command Complete int's first */
      {
         while ((scb_ptr = ha_ptr->DequeScb( ha_ptr)) != (scb_struct * ) NOT_DEFINED)
         {
            if ((DWORD) scb_ptr == (DWORD) ha_ptr)
            {                                      /* BIOS SCB complete */
               ret_status |= NOT_OUR_INT;
               bios_int = 1;
            }
            else if ((DWORD) scb_ptr == (DWORD) ha_ptr->AConfigPtr) 
            {                                      /* "Spurious" completion */
               ret_status |= OUR_OTHER_INT;
            }
            else
            {                                      /* Driver SCB complete */
               ret_status |= OUR_CC_INT;
               if (scb_ptr->SCB_MgrStat == SCB_AUTOSENSE)
               {
                  scb_ptr->SCB_HaStat   = HOST_NO_STATUS;
                  scb_ptr->SCB_MgrStat  = SCB_DONE_ERR;
                  scb_ptr->SCB_TargStat = UNIT_CHECK;
               }
               /* INTR_OFF; */
               terminate_command( ha_ptr, scb_ptr);
               /* INTR_ON; */
            }
         }
         while (ha_ptr->done_cmd && (ret_status & OUR_INT))
         {
            /* INTR_OFF; */
            send_command(ha_ptr);               /* Send queued SCBs to Arrow */
            post_command(ha_ptr);               /* Post 1 SCB completion to host */
            /* INTR_ON; */
         }
         if (!swap_mode)
         {
            PAUSE_SEQ(hcntrl);
            if (bios_int)                          /* If BIOS interrupt, replace in fifo */
            {                                      /* (Internal SCB code only) */
               OUTBYTE(ha_ptr->qoutfifo, 0);
               bios_int = 0;
            }
            else
            {
               if (INBYTE( ha_ptr->qoutcnt) == 0)
               {
                  OUTBYTE( ha_ptr->clrint, CLRCMDINT);
               }
            }
            UNPAUSE_SEQ(hcntrl, ha_ptr->intstat);
         }
      }
      if (int_status & (SEQINT | BRKINT))    /* Check for other interrupts */
      {
         GET_SCB_INDEX( i )
    
         if (!swap_mode && (ha_ptr->actstat[i] == AS_SCB_BIOS))
         {
            ret_status |= NOT_OUR_INT;
            break;
         }
         OUTBYTE(ha_ptr->clrint, (SEQINT | BRKINT));
         ret_status |= OUR_OTHER_INT;

         ha_ptr->cur_scb_ptr = i;
         scb_ptr = ha_ptr->actptr[i];

         if (scb_ptr == NOT_DEFINED)
         {
            int_status = (int_status & ANYINT) | ABORT_TARGET;
            scb_ptr = (scb_struct *) NOT_DEFINED;
            ha_ptr->cur_scb_ptr = NULL_INDEX;
         }
      }
      /* INTR_WINDOW; */
      RestoreState(SaveAndEnable());      /* -ali */

      if (int_status & SEQINT)                  /* Process sequencer interrupt */
      {
         int_status &= INTCODE;
         if      (int_status == DATA_OVERRUN)     check_length(ha_ptr, scb_ptr);
         else if (int_status == CDB_XFER_PROBLEM) cdb_abort(ha_ptr, scb_ptr);
         else if (int_status == HANDLE_MSG_OUT)   sendmo(ha_ptr, scb_ptr);
         else if (int_status == SYNC_NEGO_NEEDED) negotiate(ha_ptr, scb_ptr);
         else if (int_status == CHECK_CONDX)      ha_ptr->ChekCond(ha_ptr, scb_ptr);
         else if (int_status == PHASE_ERROR)      badseq(ha_ptr, scb_ptr);
         else if (int_status == EXTENDED_MSG)     extmsgi(ha_ptr, scb_ptr);
         else if (int_status == UNKNOWN_MSG)      handle_msgi(ha_ptr, scb_ptr);
         else if (int_status == ABORT_TARGET)     target_abort(ha_ptr, scb_ptr);
         else if (int_status == NO_ID_MSG)        target_abort(ha_ptr, scb_ptr);
      }
      else if (int_status & SCSIINT)            /* Process SCSI interrupt */
      {
         byte_buf = INBYTE(ha_ptr->sstat1) & INBYTE(ha_ptr->simode1);
         if (byte_buf & SCSIRSTI)
         {
            intsrst(ha_ptr);
            ret_status |= OUR_OTHER_INT;
         }
         /* Select timeout must be checked before bus free since
            bus free will be set due to a selection timeout */
         else if (byte_buf & SELTO)
            ret_status |= intselto(ha_ptr);
         else if (byte_buf & BUSFREE)
            ret_status |= intfree(ha_ptr);
         else if (byte_buf & SCSIPERR)
            ret_status |= parity_error(ha_ptr);
         else
            OUTBYTE(ha_ptr->clrint, CLRSCSINT);
      }
      else if (int_status & BRKINT)             /* Process sequencer breakpoint */
      {
         scb_proc_bkpt( ha_ptr);
      }
      UNPAUSE_SEQ(hcntrl, ha_ptr->intstat);
      if (ret_status & OUR_INT)
      {
         /* INTR_OFF; */
         send_command(ha_ptr);
         post_command(ha_ptr);
         /* INTR_ON; */
      }
      /* INTR_WINDOW; */
      RestoreState(SaveAndEnable());   /*INTR_WINDOW; -ali */
      if (ret_status & BIOS_INT)
         break;

      READ_HINTSTAT( hcntrl, ha_ptr->intstat, int_status);
   }
   /* INTR_OFF; */
   if (ret_status & OUR_INT)
      send_command(ha_ptr);

   SET_STATE(config_ptr, sav_state);

   if (ha_ptr->Head_Of_Queue == (scb_struct * ) NOT_DEFINED)
   {
      ha_ptr->AConfigPtr->Cfe_ConfigFlags |= DRIVER_IDLE;
      if (config_ptr->Cfe_SCSIChannel == B_CHANNEL)
         ha_ptr->BConfigPtr->Cfe_ConfigFlags |= DRIVER_IDLE;
   }
   RestoreState(efl);         /* -ali */
   return (ret_status);
}
/*********************************************************************
*
*  scb_enable_int
*  scb_disable_int
*  scb_poll_int
*
*  Modules used for polled mode of operation
*
*  Return Value:  None
*                  
*  Parameters:    config_ptr
*
*  Activation:    int_handler
*                  
*  Remarks:
*                  
*********************************************************************/
void scb_enable_int (him_config *config_ptr)
{
   return;  /* Noop to minimally disrupt OSMs */
}

void scb_disable_int (him_config *config_ptr)
{
   return;  /* Noop to minimally disrupt OSMs */
}

UBYTE scb_poll_int (him_config *config_ptr)
{
   him_struct *ha_ptr;
   UBYTE ival = 0;
   ha_ptr = config_ptr->Cfe_HaDataPtr;

   READ_HINTSTAT( ha_ptr->hcntrl, ha_ptr->intstat, ival);
   ival &= ANYINT;

   return (ival);
}
/*********************************************************************
*
*  scb_special -
*
*  Perform command not requiring an SCB: Abort,
*                                        Soft Reset,
*                                        Hard Reset
*
*  Return Value:  0 = Reset completed. (Also returned for abort opcode)
*                 1 = Soft reset failed, Hard reset performed. 
*                 2 = Reset failed, hardware error.
*              0xFF = Busy could not swap sequencers  MDL 
*                  
*  Parameters: UBYTE Opcode: 00 = Abort SCB
*                            01 = Soft Reset Host Adapter
*                            02 = Hard Reset Host Adapter
*                            05 = Load BIOS Sequencer  MDL
*                            06 = Load swapping Sequencer MDL
*
*              scb_struct *: ptr to SCB for Abort, NOT_DEFINED otherwise.
*
*  Activation:    
*
*  Remarks:       
*                 
*********************************************************************/
int scb_special ( UBYTE spec_opcode,
         him_config *config_ptr,
         scb_struct *scb_pointer)
{
   him_struct *ha_ptr;
   UBYTE i, j, tarlun;
   int retval = 0;
   int efl; /* -ali */
   UWORD sav_state;

   ha_ptr = config_ptr->Cfe_HaDataPtr;

   /* INTR_OFF; */
   efl = SaveAndDisable(); /* INTR_OFF; -ali */
   sav_state = ha_ptr->g_state;
   SET_STATE(config_ptr, WATCH_ON);

   switch (spec_opcode)
   {
      case ABORT_SCB:
      scb_abort(config_ptr, scb_pointer);
      break;

      case HARD_HA_RESET:
      ha_hard_reset(ha_ptr);
      break;

      case FORCE_RENEGOTIATE:
      PAUSE_SEQ(ha_ptr->hcntrl);

      tarlun = (UBYTE) scb_pointer;
      scb_renego(config_ptr, tarlun);

      UNPAUSE_SEQ(ha_ptr->hcntrl, ha_ptr->intstat);
      break;

      case REALIGN_DATA:
      break;

      case BIOS_ON:
      /* wait for all I/O activity to finish */
      if (ha_ptr->Head_Of_Queue != (scb_struct * ) NOT_DEFINED)
      {
         retval = 0xFF;       /* busy back to driver, please try again */
         break;
      }
      if (ha_ptr->Hse_AccessMode == SMODE_NOSWAP)
         break;

      if (!(ha_ptr->HaFlags & HAFL_BIOS_CACHED))
      {
         PAUSE_SEQ(ha_ptr->hcntrl);
         ha_ptr->HaFlags |= HAFL_BIOS_CACHED;
         if ((ha_ptr->HaFlags & HAFL_BIOS_RAM) == 0)                 /* Swap Scratch-RAM */
         {
            SaveDrvrScratchRAM(config_ptr);
            LoadBIOSScratchRAM(config_ptr);
            ha_ptr->HaFlags |= HAFL_BIOS_RAM;
         }
         retval = 0;
         if (ha_ptr->AConfigPtr->Cfe_ConfigFlags & BIOS_AVAILABLE)
         {
            retval = LoadSequencer( config_ptr, (UBYTE *) &(ha_ptr->Hse_BiosSeq.InstrBytes[0]), SEQMAX);
         }
         /* Clear out SCB's */

         i = INBYTE(ha_ptr->scbptr);               /* Save current channel */
         for (j = 0; j < HARD_QDEPTH; j++)         /* Re-Initialize internal SCB's */
         {
            OUTBYTE(ha_ptr->scbptr, j);
            OUTBYTE(ha_ptr->scb00, 0x00);
         }
         OUTBYTE(ha_ptr->scbptr, i);               /* Restore channel */

         if ((retval == 0) &&
            (ha_ptr->AConfigPtr->Cfe_ConfigFlags & BIOS_AVAILABLE))
         {
            UNPAUSE_SEQ(ha_ptr->hcntrl, ha_ptr->intstat);
         }
      }
      break;

      case H_BIOS_OFF:
      if (ha_ptr->Hse_AccessMode == SMODE_NOSWAP)
         break;

      if (ha_ptr->HaFlags & HAFL_BIOS_CACHED)
      {
         PAUSE_SEQ(ha_ptr->hcntrl);
         ha_ptr->HaFlags &= ~HAFL_BIOS_CACHED;
         OUTBYTE(ha_ptr->clrint, CLRCMDINT | LED_OLVTT);
         SaveBIOSScratchRAM( config_ptr);

         if (ha_ptr->HaFlags & HAFL_BIOS_RAM)                        /* Swap Scratch-RAM */
         {
            ha_ptr->HaFlags &= ~HAFL_BIOS_RAM;
         }
         if (ha_ptr->AConfigPtr->Cfe_ConfigFlags & BIOS_AVAILABLE)
         {
            SaveBIOSSequencer( config_ptr);
         }
         retval = LoadDrvrScratchRAM( config_ptr);
         if (retval == 0)
         {
            retval = LoadSequencer( config_ptr, (UBYTE *) &E_Seq_01, E_SeqExist[1]);
         }
         /* Clear out SCB's */

         i = INBYTE(ha_ptr->scbptr);               /* Save current channel */
         for (j = 0; j < HARD_QDEPTH; j++)         /* Re-Initialize internal SCB's */
         {
            OUTBYTE(ha_ptr->scbptr, j);
            OUTBYTE(ha_ptr->scb00, 0x00);
         }
         OUTBYTE(ha_ptr->scbptr, i);               /* Restore channel */

         if (retval == 0)
         {
            UNPAUSE_SEQ(ha_ptr->hcntrl, ha_ptr->intstat);
         }
      }
      break;

      case BIOS_OFF:
      break;

      default:
      break;
   }
   /* INTR_ON; */
   if (sav_state == WATCH_OFF)
   {
      if (send_command(ha_ptr))
      {
         OUTBYTE(ha_ptr->hcntrl, INBYTE(ha_ptr->hcntrl) | SWINT);
      }
      SET_STATE(config_ptr, WATCH_OFF);
   }
   RestoreState(efl); /* INTR_ON; -ali */
   return (retval);
}
/*********************************************************************
*
*  send_command routine -
*
*  Send SCB's to Arrow
*
*  Return Value:  ret_status (unsigned char)
*                  
*  Parameters: ha_ptr
*
*  Activation: scb_send
*              int_handler
*                  
*  Remarks:                
*                  
*********************************************************************/
UBYTE send_command (him_struct *nha_ptr)
{
   register him_struct *ha_ptr;
   register scb_struct *scb_ptr;
   UBYTE j = 0,
   ret_status = 0;
   int efl;          /* -ali */

   ha_ptr = nha_ptr;
      
   if (ha_ptr->Head_Of_Queue == NOT_DEFINED)    /* Nothing to send? */
   {
      return (ret_status);
   }
   efl = SaveAndDisable();       /* -ali */
   for (scb_ptr = ha_ptr->Head_Of_Queue; ha_ptr->free_scb;
   scb_ptr = scb_ptr->SCB_Next)
   {
      switch (scb_ptr->SCB_MgrStat)
      {
    /* Check for space in Arrow, mark SCB as Ready or Waiting */

    case SCB_PROCESS:
       if (scb_ptr->SCB_Cmd == EXEC_SCB)
       {
          j = ((scb_ptr->SCB_Tarlun >> 4) | (scb_ptr->SCB_Tarlun & CHANNEL) & 0x0F);

          ha_ptr->act_chtar[j]++;

          if ((scb_ptr->SCB_Cntrl & TAG_ENABLE) &&
        (ha_ptr->act_chtar[j] > ha_ptr->max_tag_cmd))
          {
        scb_ptr->SCB_MgrStat = SCB_WAITING;
        break;
          }
          else if (!(scb_ptr->SCB_Cntrl & TAG_ENABLE) &&
             (ha_ptr->act_chtar[j] > ha_ptr->max_nontag_cmd))
          {
        scb_ptr->SCB_MgrStat = SCB_WAITING;
        break;
          }
          scb_ptr->SCB_MgrStat = SCB_READY;
       }
       else
       {
          scb_ptr->SCB_MgrStat = SCB_DONE_ILL;
          ++ha_ptr->done_cmd;
          ret_status = 1;
          break;
       }

    case SCB_READY:               /* If Ready, load into Arrow */
       scb_ptr->SCB_MgrStat = SCB_ACTIVE;
       ha_ptr->EnqueScb( ha_ptr, scb_ptr);
       break;
      }
      if (scb_ptr->SCB_Next == NOT_DEFINED) 
    break;
   }
   RestoreState(efl);         /* -ali */
   return(ret_status);
}
/*********************************************************************
*
*  terminate_command routine -
*
*  Mark SCB as done and find next SCB to execute
*
*  Return Value:  None
*                  
*  Parameters: ha_ptr
*              scb_ptr
*
*  Activation: int_handler
*              abort_channel  
*              badseq
*              check_condition
*              intfree
*              intselto
*                
*  Remarks:       INTR_OFF (i.e. system interrupts off) must be executed
*                 prior to calling this routine.
*                  
*********************************************************************/
void terminate_command (him_struct *nha_ptr, scb_struct *nscb_ptr)
{
   register him_struct *ha_ptr;
   register scb_struct *scb_ptr;
   register UBYTE tar_ch_index, max_cmd, cur_tarchan;

   if (nscb_ptr == NOT_DEFINED)                 /* Exit if Null SCB */
      return;

   ha_ptr = nha_ptr;
   scb_ptr = nscb_ptr;

   ++ha_ptr->free_scb;                       /* Update counters */
   ++ha_ptr->done_cmd;

   scb_ptr->SCB_MgrStat = SCB_DONE;          /* Update status */
   if (scb_ptr->SCB_HaStat)
   {
      scb_ptr->SCB_MgrStat = SCB_DONE_ERR;
   }
   switch (scb_ptr->SCB_TargStat)
   {
      case UNIT_GOOD:
      case UNIT_MET:
      case UNIT_INTERMED:
      case UNIT_INTMED_GD:
    break;
      default:
    scb_ptr->SCB_MgrStat = SCB_DONE_ERR;
   }
   cur_tarchan = scb_ptr->SCB_Tarlun & (TARGET_ID | CHANNEL);
   tar_ch_index= ((cur_tarchan >> 4) | (cur_tarchan & CHANNEL) & 0x0F);

   if ((scb_ptr->SCB_Cntrl & TAG_ENABLE) == 0)
   {
      max_cmd = ha_ptr->max_nontag_cmd;
   }
   else
   {
      max_cmd = ha_ptr->max_tag_cmd;
   }
   if (ha_ptr->act_chtar[tar_ch_index] > max_cmd) 
   {                                      /* Mark next SCB for Arrow */
      for (;;)
      {
         if ((scb_ptr->SCB_Next == NOT_DEFINED) ||
             (scb_ptr->SCB_Next->SCB_Cmd == BOOKMARK))
         {
            scb_ptr = ha_ptr->Head_Of_Queue;
         }
         else
         {
            scb_ptr = scb_ptr->SCB_Next;
         }

         if (scb_ptr->SCB_MgrStat == SCB_WAITING)
         {
            if (scb_ptr->SCB_Tarlun == cur_tarchan)
            {
               scb_ptr->SCB_MgrStat = SCB_READY;
               break;
            }
         }
         if (scb_ptr == nscb_ptr)
         {
            break;
         }
      }
   }
   if (ha_ptr->act_chtar[tar_ch_index])
   {
      --(ha_ptr->act_chtar[tar_ch_index]);
   }
   return;
}
/*********************************************************************
*
*  post_command routine -
*
*  Return completed SCB to ASPI layer
*
*  Return Value:  None
*                  
*  Parameters: ha_ptr
*
*  Activation: int_handler
*              badseq
*              intsrst
*
*  Remarks:
*
*********************************************************************/
void post_command (him_struct *nha_ptr)
{
   register him_struct *ha_ptr;
   register scb_struct *scb_ptr;
   register scb_struct *previous_ptr;
   him_config *config_ptr;
   int efl;       /* -ali */

   ha_ptr = nha_ptr;
   previous_ptr = NOT_DEFINED;

   if (ha_ptr->Head_Of_Queue == NOT_DEFINED)    /* Exit if Null SCB Q */
   {
      if (ha_ptr->done_cmd)
      {
         ha_ptr->done_cmd = 0;
      }
      return;
   }

   efl = SaveAndDisable();       /* -ali */
   scb_ptr = ha_ptr->Head_Of_Queue;

   while (ha_ptr->done_cmd)
   {
      if (scb_ptr->SCB_MgrStat & SCB_DONE_MASK)
      {
         efl = SaveAndDisable(); /* INTR_OFF; -ali */
         if (scb_ptr == ha_ptr->Head_Of_Queue)
         {
            ha_ptr->Head_Of_Queue = scb_ptr->SCB_Next;
         }
         else
         {
            previous_ptr->SCB_Next = scb_ptr->SCB_Next;
         }
         if (scb_ptr->SCB_Next == NOT_DEFINED)
            ha_ptr->End_Of_Queue = previous_ptr;

         --ha_ptr->done_cmd;
         scb_ptr->SCB_Stat = scb_ptr->SCB_MgrStat;
         if (scb_ptr->SCB_Tarlun & CHANNEL)
            config_ptr = ha_ptr->BConfigPtr;
         else 
            config_ptr = ha_ptr->AConfigPtr;

         /* INTR_ON; */
         SCBCompleted(config_ptr, scb_ptr);                  /* post */
         /* INTR_OFF; */

         break;
      }
      else
      {
         previous_ptr = scb_ptr;
         scb_ptr = scb_ptr->SCB_Next;

         if (scb_ptr == NOT_DEFINED)
         {
            if (ha_ptr->done_cmd)
            {
               ha_ptr->done_cmd--;
            }
            break;
         }
      }
   }
   RestoreState(efl);         /* -ali */
   return;
}
/*********************************************************************
*
*  External SCB Handling Routines :
*
*  scb_ext_enque
*  scb_ext_deque
*  scb_ext_preempt
*  scb_ext_addfree
*
*********************************************************************/
/*********************************************************************
*
*  scb_ext_enque
*
*  Queue an external SCB for the Arrow sequencer
*
*  Return Value:  None, for now
*                  
*  Parameters:    nha_ptr - current ha_struct
*                 scb_ptr - scb to send to Arrow
*
*  Activation:    send_command
*                  
*  Remarks:                
*                  
*********************************************************************/
int scb_ext_enque (him_struct *nha_ptr, scb_struct *scb_ptr)
{
   DWORD phys_scb_addr;
   UBYTE scb_index;
   int efl; /* -ali */

   /* INTR_OFF; */
   if (nha_ptr->free_scb)
   {
      efl = SaveAndDisable(); /* INTR_OFF; - ali */
      /* Add physical SCB address to scb_array */
   
      if (scb_ptr->SCB_Cntrl & TAG_ENABLE)   /* Tagged SCB, get index from top */
      {
         scb_index = nha_ptr->free_ptr_list[nha_ptr->free_hi];
         nha_ptr->free_hi--;
      }
      else                                /* Tagged SCB, get index from bottom */
      {
         scb_index = nha_ptr->free_ptr_list[nha_ptr->free_lo];
         nha_ptr->free_lo++;
      }
      phys_scb_addr = (DWORD) (scb_ptr->SCB_CDBPtr - SCB_CDB_OFFSET);   /* Calculate address */
      nha_ptr->scb_array[scb_index] = phys_scb_addr;        /* Write to physical table */
      nha_ptr->actptr[scb_index] = scb_ptr;                 /* Write to logical table */
      nha_ptr->actstat[scb_index] = AS_SCB_ACTIVE;

      --nha_ptr->free_scb;

      PAUSE_SEQ(nha_ptr->hcntrl);

      nha_ptr->qin_array[nha_ptr->qin_index++] = scb_index; /* Add to array */
      OUTBYTE(nha_ptr->qin_cnt, INBYTE(nha_ptr->qin_cnt) + 1);

      UNPAUSE_SEQ(nha_ptr->hcntrl, nha_ptr->intstat);
      RestoreState(efl); /* INTR_ON; -ali */
      return (0);
   }
   return (-1);
}
/*********************************************************************
*
*  scb_ext_deque
*
*  Deque a completed external SCB
*
*  Return Value:  
*                  
*  Parameters:    
*
*  Activation:    
*                  
*  Remarks:                
*                  
*********************************************************************/
scb_struct *scb_ext_deque (him_struct *nha_ptr)
{
   register scb_struct *scb_ptr;
   UBYTE scb_index;
   int efl; /* -ali */

   /* INTR_OFF; */
   efl = SaveAndDisable(); /* INTR_OFF; -ali */
   OUTBYTE( nha_ptr->clrint, CLRCMDINT);
   INBYTE( nha_ptr->clrint);

   scb_index = nha_ptr->qout_array[nha_ptr->qout_index];   /* Get completed SCB pointer */
   nha_ptr->cur_scb_ptr = scb_index;

   if (scb_index != 0xFF)
   {
      scb_ptr = nha_ptr->actptr[scb_index];
      nha_ptr->qout_array[nha_ptr->qout_index] = 0xFF;
      nha_ptr->qout_index++;
      scb_ext_addfree(nha_ptr, scb_index);
   }
   else
   {
      scb_ptr = NOT_DEFINED;
   }
   /* INTR_ON; */
   RestoreState(efl); /* INTR_ON; -ali */

   return (scb_ptr);
}
/*********************************************************************
*
*  scb_ext_preempt
*
*  Remove an active SCB from sequencer control and set appropriate
*  completion status.
*
*  Return Value:  None, yet
*                  
*  Parameters:    ha_ptr
*                 compstat - desired completion status
*
*  Activation:    int_handler       (Normal completion)
*                 abort_channel     (Exception condition)
*                 badseq            (" ")
*                 intfree           (" ")
*                 intselto          (" ")
*                 target_abort      (" ")
*                 scb_hard_reset    (")
*                 scb_trm_cmplt     (")
*                 check_condition   ?????
*                  
*  Remarks:       INTR_OFF (i.e. system interrupts off) must be executed
*                 prior to calling this routine.
*                 The chip MUST be paused prior to calling this routine!
*
*********************************************************************/
int scb_ext_preempt (him_struct *ha_ptr, UBYTE compstat)
{
   scb_struct *scb_ptr;
   UBYTE scb_index, j;
   UWORD port_addr;
   int i;

   if ((ha_ptr->cur_scb_ptr != NULL_INDEX) &&
       (ha_ptr->actptr[ha_ptr->cur_scb_ptr] != NOT_DEFINED))
   {
      scb_index = ha_ptr->cur_scb_ptr;
      scb_ptr = ha_ptr->actptr[scb_index];
   }
   else
   {
      return(0);
   }
   /* Set flag for sequencer to abort */
   scb_ptr->SCB_RsvdX[SCB_CHAIN_INDEX] |= SCB_CHAIN_ABORT;

   scb_ptr->SCB_HaStat = compstat;
   scb_ext_addfree(ha_ptr, scb_index);

   i = INBYTE(ha_ptr->scsiseq + QIN_PTR_ARRAY);
   while (i != ha_ptr->qin_index)                        /* Clear from QIN array */
   {
      if (ha_ptr->qin_array[i] == scb_index)             /* Found in QIN array */
      {
         ha_ptr->qin_array[i] = 0xFF;
         break;
      }
      if (++i == 256)
      {
         i = 0;
      }
   }

   i = ((scb_ptr->SCB_Tarlun >> 4) | (scb_ptr->SCB_Tarlun & CHANNEL) & 0x0F);       /* Insert into Next_SCB_Array */
   port_addr = ha_ptr->scsiseq + NEXT_SCB_ARRAY + i;
   if (((j = INBYTE(port_addr)) & NEXT_SCB_NULL) == scb_index)    /* CDB waiting? */
   {
      OUTBYTE(port_addr, NEXT_SCB_NULL);                          /* Yes, deactivate it */
      INBYTE(port_addr);
   }


   for (i = 0; i < 256; i++)                             /* Clear from BUSY array */
   {
      if (ha_ptr->busy_array[i] == scb_index)            /* Found in BUSY array */
      {
         ha_ptr->busy_array[i] = 0xFF;

      /* Need to deal with a waiting SCB by pushing it back into the
         qin_ptr_array behind the request sense SCB. Coming Soon! */

         if (j != NEXT_SCB_NULL)                                  /* CDB waiting? */
         {
            j |= 0x80;                                   /* Yes, activate */
            OUTBYTE(port_addr, j);
         }
         break;
      }
   }

   terminate_command(ha_ptr, scb_ptr);

   return (0);
}
/*********************************************************************
*
*  scb_ext_addfree routine -
*
*  Clear external scb ptr and index, add scb_ptr to free list  
*
*  Return Value:  None
*                  
*  Parameters:    ha_ptr
*                 scb_ptr - Index of scb address
*
*  Activation:    intsrst
*                  
*  Remarks:                
*                  
*********************************************************************/
void scb_ext_addfree (him_struct *ha_ptr, UBYTE scb_ptr)
{
   if ((scb_ptr < 0x7F) && (ha_ptr->free_lo > 0))        /* Add scb_ptr to free list */
   {
      ha_ptr->free_lo--;
      ha_ptr->free_ptr_list[ha_ptr->free_lo] = scb_ptr;
   }
   else
   {
      ha_ptr->free_hi++;
      ha_ptr->free_ptr_list[ha_ptr->free_hi] = scb_ptr;
   }
   ha_ptr->scb_array[scb_ptr] = PHYS_SCB_NULL;           /* Clear scb_array entry */
   ha_ptr->actptr[scb_ptr] = NOT_DEFINED;                /* Clear actptr array entry */
   ha_ptr->actstat[scb_ptr] = AS_SCBFREE;

   return;
}
/*********************************************************************
*
*  check_condition routine -
*
*  handle response to target check condition
*
*  Return Value:  None
*                  
*  Parameters:    ha_ptr
*                 scb_ptr
*
*  Activation:    int_handler
*                  
*  Remarks:                
*                  
*********************************************************************/

void ext_check_condition (him_struct *ha_ptr, scb_struct *scb_ptr)
{
   union sense {
      UBYTE sns_array[4];
      DWORD sns_ptr;
   } sns_segptr;
   UBYTE i, j, status, tmp_index;
   DWORD port_addr;
   int efl; /* -ali */

   i = scb_ptr->SCB_Tarlun;
   if (i & CHANNEL)                             /* Setup renegotiation */
   {
      scb_renego(ha_ptr->BConfigPtr, i);
   }
   else
   {
      scb_renego(ha_ptr->AConfigPtr, i);
   }

   if (scb_ptr->SCB_TargStat != UNIT_CHECK)     /* 1st CC, get status from SCB */
   {
      status = INBYTE(ha_ptr->scb14);           
      scb_ptr->SCB_TargStat = status;
   }
   else                                         /* Second consecutive CC? */
   {
      status = 0;
      scb_ptr->SCB_HaStat = HOST_SNS_FAIL;      /* Yes, we're done */
   }
   if ((status == UNIT_CHECK) && (scb_ptr->SCB_Flags & AUTO_SENSE))
   {
      /* Move in request sense CDB address */
      
      sns_segptr.sns_ptr = scb_ptr->SCB_CDBPtr + MAX_CDB_LEN;

      for (i = 0; i < 6; ++i)
         scb_ptr->SCB_CDB[i] = 0x00;
      scb_ptr->SCB_CDB[0] = 0x03;               /* Modify External SCB */
      scb_ptr->SCB_CDB[4] = (UBYTE) scb_ptr->SCB_SenseLen;

      scb_ptr->SCB_Cntrl  = REJECT_MDP;
      scb_ptr->SCB_SegCnt = 0x01;
      scb_ptr->SCB_CDBLen = 0x06;
      scb_ptr->SCB_SegPtr = sns_segptr.sns_ptr;
      scb_ptr->SCB_TargStat = 0x00;
      scb_ptr->SCB_ResCnt = (DWORD) 0;
      for (i = 0; i < 13; i++)
         scb_ptr->SCB_RsvdX[i] = 0;

      /* Need to deal with a waiting SCB by pushing it back into the
         qin_ptr_array behind the request sense SCB. Coming Soon! */

      i = ((scb_ptr->SCB_Tarlun >> 4) | (scb_ptr->SCB_Tarlun & CHANNEL) & 0x0F);       /* Insert into Next_SCB_Array */
      port_addr = ha_ptr->scsiseq + NEXT_SCB_ARRAY + i;

      if ((j = INBYTE(port_addr)) != NEXT_SCB_NULL)
      {
         OUTBYTE(port_addr, NEXT_SCB_NULL);
         port_addr = ha_ptr->scsiseq + QIN_PTR_ARRAY;
         i = INBYTE(port_addr) - 1;
         OUTBYTE(port_addr, i);
         i = INBYTE(ha_ptr->qin_cnt) + 1;
         tmp_index = ha_ptr->qin_index - i;
         ha_ptr->qin_array[tmp_index] = (j & NEXT_SCB_NULL);
         OUTBYTE(ha_ptr->qin_cnt, i);
      }
      /* Now place request sense SCB at head of queue */

      /* Get current QIN ptr and decrement, write */
      
      port_addr = ha_ptr->scsiseq + QIN_PTR_ARRAY;
      i = INBYTE(port_addr) - 1;
      OUTBYTE(port_addr, i);

      i = INBYTE(ha_ptr->qin_cnt) + 1;       /* Get current QINCNT, increment */
      tmp_index = ha_ptr->qin_index - i;     /* Array location to write index to */
      ha_ptr->qin_array[tmp_index] = ha_ptr->cur_scb_ptr;   /* Write to array */
      OUTBYTE(ha_ptr->qin_cnt, i);           
      ha_ptr->busy_array[scb_ptr->SCB_Tarlun] = 0xFF;

      scb_ptr->SCB_MgrStat = SCB_AUTOSENSE;
   }
   else
   {
      efl = SaveAndDisable(); /* INTR_OFF; -ali */
      ha_ptr->PreemptScb(ha_ptr, scb_ptr->SCB_HaStat);
      RestoreState(efl); /* INTR_ON; -ali */
   }
   OUTBYTE(ha_ptr->sxfrctl0, INBYTE(ha_ptr->sxfrctl0) | CLRCHN);
   OUTBYTE(ha_ptr->simode1, INBYTE(ha_ptr->simode1) & ~ENBUSFREE);
   INBYTE(ha_ptr->scsidatl);

   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) IDLE_LOOP_ENTRY >> 10);

   if ((wt4bfree(ha_ptr)) & REQINIT)
      badseq(ha_ptr, scb_ptr);
}
/*********************************************************************
*
*  External SCB Handling Routines :
*
*  scb_int_enque
*  scb_int_deque
*  scb_int_preempt
*  scb_int_addfree
*
*********************************************************************/
void scb_LoadEntryTbl ( him_struct *ha_ptr, UWORD ScbMethod )
{
   if (ScbMethod == SMODE_NOSWAP)
   {
      ha_ptr->DequeScb   = scb_int_deque;
      ha_ptr->EnqueScb   = scb_int_enque;
      ha_ptr->PreemptScb = (void (*)()) scb_int_preempt;
      ha_ptr->ChekCond   = int_check_condition;
   }
   else
   {
      ha_ptr->EnqueScb   = scb_ext_enque;
      ha_ptr->DequeScb   = scb_ext_deque;
      ha_ptr->PreemptScb = (void (*)()) scb_ext_preempt;
      ha_ptr->ChekCond   = ext_check_condition;
   }
   return;
}
/*********************************************************************
*
*  scb_int_enque
*
*  Queue an external SCB for the Arrow sequencer
*
*  Return Value:  None, for now
*                  
*  Parameters:    nha_ptr - current ha_struct
*                 scb_ptr - scb to send to Arrow
*
*  Activation:    send_command
*                  
*  Remarks:                
*                  
*********************************************************************/
int scb_int_enque (him_struct *nha_ptr, scb_struct *scb_ptr)
{
   UBYTE scb_index, j, cnt, *ptr;

   if (nha_ptr->free_scb)
   {
      /* Get free SCB # */
   
      scb_index = nha_ptr->free_ptr_list[nha_ptr->free_lo];
      nha_ptr->actstat[scb_index] = AS_SCB_ACTIVE;
      nha_ptr->free_lo++;

      --nha_ptr->free_scb;

      nha_ptr->actptr[scb_index] = scb_ptr;                 /* Write to logical table */

      PAUSE_SEQ(nha_ptr->hcntrl);

      j = INBYTE(nha_ptr->scbptr);
      OUTBYTE(nha_ptr->scbptr, scb_index);
      OUTBYTE(nha_ptr->scbcnt, SCBAUTO);

      for (cnt = 0, ptr = &scb_ptr->SCB_Cntrl; cnt < SCB_LENGTH; cnt++)
      OUTBYTE (nha_ptr->scb00, *(ptr++));

      OUTBYTE(nha_ptr->scbcnt, 0x00);
      OUTBYTE(nha_ptr->qinfifo, scb_index);
      OUTBYTE(nha_ptr->scbptr, j);

      UNPAUSE_SEQ(nha_ptr->hcntrl, nha_ptr->intstat);
   }

   return(0);
}
/*********************************************************************
*
*  scb_int_deque
*
*  Deque a completed external SCB
*
*  Return Value:  
*                  
*  Parameters:    
*
*  Activation:    
*                  
*  Remarks:                
*                  
*********************************************************************/
scb_struct *scb_int_deque (him_struct *nha_ptr)
{
   register scb_struct *scb_ptr;
   UBYTE scb_index;

   PAUSE_SEQ(nha_ptr->hcntrl);

   if (INBYTE(nha_ptr->qoutcnt) == 0)              /* Check Queue, return if empty */
   {
      UNPAUSE_SEQ(nha_ptr->hcntrl, nha_ptr->intstat);
      return( NOT_DEFINED );
   }
   scb_index = INBYTE(nha_ptr->qoutfifo);          /* Get SCB index from queue */

   if (nha_ptr->actstat[scb_index] == AS_SCB_ACTIVE)
   {
      scb_ptr = nha_ptr->actptr[scb_index];                 /* Active Driver SCB */
      nha_ptr->free_lo--;                                   /* Decrement stack pointer  */
      nha_ptr->free_ptr_list[nha_ptr->free_lo] = scb_index; /* Push onto stack          */
      nha_ptr->actptr[scb_index] = NOT_DEFINED;             /* Clear actptr array entry */
      nha_ptr->actstat[scb_index] = AS_SCBFREE;
   }
   else if (nha_ptr->actstat[scb_index] == AS_SCB_BIOS)
   {
      scb_ptr = (scb_struct *) nha_ptr;           /* Active BIOS SCB */
   }
   else
   {
      scb_ptr = (scb_struct *) nha_ptr->AConfigPtr;   /* Spurious Interrupt */
   }
   UNPAUSE_SEQ(nha_ptr->hcntrl, nha_ptr->intstat);

   return (scb_ptr);
}
/*********************************************************************
*
*  scb_int_preempt
*
*  Remove an active SCB from sequencer control and set appropriate
*  completion status.
*
*  Return Value:  None, yet
*                  
*  Parameters:    ha_ptr
*                 compstat - desired completion status
*
*  Activation:    int_handler       (Normal completion)
*                 abort_channel     (Exception condition)
*                 badseq            (" ")
*                 handle_msgi       (" ")
*                 intfree           (" ")
*                 intselto          (" ")
*                 target_abort      (" ")
*                 scb_hard_reset    (")
*                 scb_trm_cmplt     (")
*                 check_condition   ?????
*                  
*  Remarks:       INTR_OFF (i.e. system interrupts off) must be executed
*                 prior to calling this routine.
*                  
*********************************************************************/
int scb_int_preempt (him_struct *nha_ptr, UBYTE compstat)
{
   scb_struct *scb_ptr;
   int i, j, scb_save, scb_index;

   if ((nha_ptr->cur_scb_ptr != NULL_INDEX) &&
       (nha_ptr->cur_scb_ptr < 4) &&
       (nha_ptr->actptr[nha_ptr->cur_scb_ptr] != NOT_DEFINED))
   {
      scb_index = nha_ptr->cur_scb_ptr;
      scb_ptr = nha_ptr->actptr[scb_index];
   }
   else
   {
      return(0);                       /* No valid ptr, return */
   }
   scb_save = INBYTE(nha_ptr->scbptr);
   scb_ptr->SCB_HaStat = compstat;

   /* SCB index to Free List */
   
   nha_ptr->free_lo--;                                   /* Decrement stack pointer  */
   nha_ptr->free_ptr_list[nha_ptr->free_lo] = scb_index; /* Push onto stack          */
   nha_ptr->actptr[scb_index] = NOT_DEFINED;             /* Clear actptr array entry */
   nha_ptr->actstat[scb_index] = AS_SCBFREE;

   for (i = (UWORD) INBYTE(nha_ptr->qincnt); i > 0; i--)  /* Clear from QINFIFO */
   {
      j = (UWORD) INBYTE(nha_ptr->qinfifo);     /* Read FIFO */
      if (j != scb_index)                 
      {
         OUTBYTE(nha_ptr->qinfifo, j);          /* Replace if not preempted */
      }
   }
   for (i = (UWORD) INBYTE(nha_ptr->qoutcnt); i > 0; i--) /* Check & Clear from QOUTFIFO */
   {
      j = (UWORD) INBYTE(nha_ptr->qoutfifo); /* Read from FIFO ** Replace if not preempted */
      if (j != scb_index)
      {
         OUTBYTE(nha_ptr->qoutfifo, j);   /* Replace if not preempted */
      }
   }
   OUTBYTE(nha_ptr->scbptr, scb_index);

   if (compstat != HOST_ABT_HOST)               /* All cases except Host Abort */
   {
      OUTBYTE(nha_ptr->scb00, 0x00);            /* Clear scb_array entry */
   }
   else                                         /* Host Abort case */
   {
      if (INBYTE(nha_ptr->scb00) & (SWAIT | SDISCON))
      {
         OUTBYTE(nha_ptr->scb00, 0x00);         /* Clear scb_array entry */
      }
      /* If this SCB is currently connected, we must be hung, so reset bus */
      else if((scb_save == scb_index) &&
              (INBYTE(nha_ptr->scsisig) & BSYI) &&
              ((INBYTE(nha_ptr->sblkctl) & SELBUS1) == scb_ptr->SCB_Tarlun & SELBUS1) &&
              ((INBYTE(nha_ptr->scsiid) & TARGET_ID) == scb_ptr->SCB_Tarlun & TARGET_ID))
      {
         badseq(nha_ptr, (scb_struct *) NOT_DEFINED);
      }
      OUTBYTE(nha_ptr->seqaddr0, (UBYTE) IDLE_LOOP_ENTRY >> 2);
      OUTBYTE(nha_ptr->seqaddr0 + 1, (UBYTE) IDLE_LOOP_ENTRY >> 10);
   }
   terminate_command(nha_ptr, scb_ptr);

   return (0);
}

/*********************************************************************
*
*  General Exception handling routines
*
*********************************************************************/
/*********************************************************************
*
*  abort_channel routine -
*
*  <brief description>
*
*  Return Value:  None
*
*  Parameters:    ha_ptr
*
*  Activation:    intsrst
*
*  Remarks:       INTR_OFF (i.e. system interrupts off) must be executed
*                 prior to calling this routine.
*
*********************************************************************/
void abort_channel (him_struct *ha_ptr, UBYTE abt_status)
{
   UBYTE channel;
   UWORD i;
   scb_struct *scb_ptr;

   channel = INBYTE(ha_ptr->sblkctl) & SELBUS1;
                                                       /* Clear active SCBs */
   for (scb_ptr = ha_ptr->Head_Of_Queue;
        ((scb_ptr != NOT_DEFINED) && (scb_ptr->SCB_Cmd != BOOKMARK));
        scb_ptr = scb_ptr->SCB_Next)
   {
      if ((scb_ptr->SCB_MgrStat == SCB_ACTIVE) &&
          ((scb_ptr->SCB_Tarlun & CHANNEL) == channel))
      {
         for (i = 0; i < 256; i++)
         {
            if (ha_ptr->actptr[i] == scb_ptr)
            {
               ha_ptr->cur_scb_ptr = i;                /* Need this line CSF */
               ha_ptr->PreemptScb(ha_ptr, abt_status); /* Remove SCB from sequencer */
               break;
            }
         }
      }
   }

   /* Clear waiting SCBs for channel */

   for (scb_ptr = ha_ptr->Head_Of_Queue;
        ((scb_ptr != NOT_DEFINED) && (scb_ptr->SCB_Cmd != BOOKMARK));
        scb_ptr = scb_ptr->SCB_Next)
   {
      if ((scb_ptr->SCB_MgrStat == SCB_PROCESS) ||
          (scb_ptr->SCB_MgrStat & (SCB_READY | SCB_WAITING)))
      {
         if ((scb_ptr->SCB_Tarlun & CHANNEL) == channel)
         {
            scb_ptr->SCB_HaStat = abt_status;
            --ha_ptr->free_scb;                 /* Adjust free_scb counter */
            terminate_command(ha_ptr, scb_ptr); /* Routine will ++free_scb */
         }
      }
   }
   if (ha_ptr->Head_Of_Queue == NOT_DEFINED)
   {
      ha_ptr->AConfigPtr->Cfe_ConfigFlags |= DRIVER_IDLE;
      if (channel)
      {
         ha_ptr->BConfigPtr->Cfe_ConfigFlags |= DRIVER_IDLE;
      }
   }
}
/*********************************************************************
*
*  badseq routine -
*
*  Terminate SCSI command sequence because sequence that is illegal,
*  or if we just can't handle it.
*
*  Return Value:  None
*                  
*  Parameters: ha_ptr
*              scb_ptr
*
*  Activation: int_handler
*              cdb_abort
*              extmsgi
*                  
*  Remarks:                
*                  
*********************************************************************/
void badseq (him_struct *ha_ptr, scb_struct *scb_ptr)
{
   UBYTE i, n;
   int efl; /* -ali */
   him_config *config_ptr;

   if (INBYTE(ha_ptr->sblkctl) & SELBUS1)          /* Select channel */
      config_ptr = ha_ptr->BConfigPtr;
   else
      config_ptr = ha_ptr->AConfigPtr;

   if ((INBYTE(ha_ptr->sstat1) & BUSFREE) && (scb_ptr == NOT_DEFINED))
   {
      scb_AsynchEvent(AE_HA_RST, config_ptr, 0L);  /* asynchronous event notification */

      reset_scsi(ha_ptr);                    /* Do a bus reset */
      reset_channel(ha_ptr);                 /* Channel must be cleared */

      /* asynchronous event notification */

      GET_SCB_INDEX( i )

      n = ha_ptr->actstat[i];                /* Get command active status */

      efl = SaveAndDisable(); /* INTR_OFF; -ali */
      abort_channel(ha_ptr, HOST_PHASE_ERR); /* abort command */
      RestoreState(efl); /* INTR_ON; -ali */

      if (n != AS_SCBFREE)                   /* Post command if active */
      {
         post_command(ha_ptr);
      }
   }
   else
   {
      OUTBYTE(ha_ptr->clrsint1, CLRBUSFREE);
      OUTBYTE(ha_ptr->clrint, CLRSCSINT);
   }
   OUTBYTE(config_ptr->Cfe_PortAddress + EISA_HOST + DFCNTRL, FIFORESET);
   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) (IDLE_LOOP_ENTRY >> 2));
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) (IDLE_LOOP_ENTRY >> 10));
}
/*********************************************************************
*
*  intfree routine -
*
*  Acknowledge and clear SCSI Bus Free interrupt
*
*  Return Value:  None
*                  
*  Parameters: ha_ptr
*
*  Activation: int_handler
*                  
*  Remarks:                
*                  
*********************************************************************/
UBYTE intfree (him_struct *ha_ptr)
{
   UBYTE index;
   int efl; /* -ali */

   GET_SCB_INDEX( index )

   if (ha_ptr->actstat[index] == AS_SCB_BIOS)
   {
      return(NOT_OUR_INT);
   }
   OUTBYTE(ha_ptr->simode1, INBYTE(ha_ptr->simode1) & ~ENBUSFREE);
   OUTBYTE(ha_ptr->clrsint1, CLRBUSFREE);    /* Clear interrupts */
   OUTBYTE(ha_ptr->clrint, CLRSCSINT);

   if (ha_ptr->actstat[index] != AS_SCBFREE) /* Terminate, Post SCB */
   {
      ha_ptr->cur_scb_ptr = index;
      efl = SaveAndDisable(); /* INTR_OFF; -ali */
      ha_ptr->PreemptScb(ha_ptr, HOST_BUS_FREE);
      RestoreState(efl); /* INTR_ON; -ali */
      post_command(ha_ptr);
   }
   if (ha_ptr->AConfigPtr->Cfe_PortAddress > 0xbff)
   {
      OUTBYTE(ha_ptr->AConfigPtr->Cfe_PortAddress + EISA_HOST + DFCNTRL, FIFORESET);
   }
   else
   {
      OUTBYTE(ha_ptr->AConfigPtr->Cfe_PortAddress + ISA_HOST + DFCNTRL, FIFORESET);
   }
   /* Restart sequencer from idle loop*/
   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) (IDLE_LOOP_ENTRY >> 2));
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) (IDLE_LOOP_ENTRY >> 10));

   return(OUR_OTHER_INT);
}
/*********************************************************************
*
*  intselto routine -
*
*  Handle SCSI selection timeout
*
*  Return Value:  None
*                  
*  Parameters: ha_ptr
*
*  Activation: int_handler
*                  
*  Remarks:                
*                  
*********************************************************************/
UBYTE intselto (him_struct *ha_ptr)
{
   scb_struct *scb_ptr;
   him_config *config_ptr;   
   UBYTE storage, channel, i, ret_status = 0;
   UWORD port_addr;
   int efl; /* -ali */

   config_ptr = ha_ptr->AConfigPtr;           /* Channel does not matter */
   OUTBYTE(ha_ptr->scsiseq, (INBYTE(ha_ptr->scsiseq) & ~(ENSELO + ENAUTOATNO)));

   if (config_ptr->Cfe_ReleaseLevel == 0x01)       /* REV 'C' */
   {
      OUTBYTE(ha_ptr->simode1, INBYTE(ha_ptr->simode1) & ~ENSELTIMO);
   }

   storage = INBYTE(ha_ptr->scbptr);      /* Save current SCB ptr */
   channel = INBYTE(ha_ptr->sblkctl) & (SELBUS1 | SELWIDE);

   if (ha_ptr->HaFlags & HAFL_SWAP_ON)    /* Identify timed out SCB, swapping */
   {
      port_addr = ha_ptr->active_scb - 1;
      if (channel & SELWIDE)              /* Wide is channel 0 */
      {
         channel = 0;
      }
      else if (channel & SELBUS1)         /* Else, might be channel B */
      {
         port_addr = port_addr + 2;
      }
      ha_ptr->cur_scb_ptr = INBYTE(port_addr);

      /* Check if selection timeout is for an aborted SCB */

      scb_ptr = ha_ptr->actptr[ha_ptr->cur_scb_ptr];
      if (scb_ptr != NOT_DEFINED)
      {
         if ((INBYTE(ha_ptr->scsiid) & TARGET_ID) !=
             (scb_ptr->SCB_Tarlun & TARGET_ID))
         {
            ha_ptr->cur_scb_ptr = NULL_INDEX;
         }
      }
      ret_status = OUR_OTHER_INT;
   }
   else                             /* Identify timed out SCB, internal */
   {
      for (i = 0; i < QDEPTH; i++)
      {
         OUTBYTE(ha_ptr->scbptr, i);
         if (INBYTE(ha_ptr->scb00) & SWAIT)
         {
            scb_ptr = ha_ptr->actptr[i];
            if ((ha_ptr->actstat[i] == AS_SCB_BIOS) ||
            (scb_ptr == NOT_DEFINED))
            {
               ret_status = NOT_OUR_INT;        /* Ignore BIOS SCB */
               break;
            }
            if ((scb_ptr->SCB_Tarlun & CHANNEL) != (channel & SELBUS1))
               continue;
            ret_status = OUR_OTHER_INT;
            ha_ptr->cur_scb_ptr = i;
            break;
         }
      }
   }
   if (ret_status != NOT_OUR_INT)               /* Clear interrupts */
   {
      OUTBYTE(ha_ptr->clrsint1, CLRSELTIMO | CLRBUSFREE);
      OUTBYTE(ha_ptr->clrint, CLRSCSINT);
      if (ret_status == OUR_OTHER_INT)          /* Terminate external SCB */
      {
         efl = SaveAndDisable();
         ha_ptr->PreemptScb(ha_ptr, HOST_SEL_TO);     
         RestoreState(efl);
      }
   }
   if (config_ptr->Cfe_ReleaseLevel == 0x01)
   {
      OUTBYTE(ha_ptr->seqaddr0, (UBYTE) (IDLE_LOOP_ENTRY >> 2));
      OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) (IDLE_LOOP_ENTRY >> 10));
   }
   OUTBYTE(ha_ptr->scbptr, storage);
   return(ret_status);
}
/*********************************************************************
*
*  parity_error routine -
*
*  handle SCSI parity errors
*
*  Return Value:  mask for int_handler return parameter
*                  
*  Parameters: ha_ptr
*
*  Activation: int_handler
*                  
*  Remarks:                
*                  
*********************************************************************/
UBYTE parity_error (him_struct *ha_ptr)
{
   UBYTE i;

   i = INBYTE(ha_ptr->scbptr);

   if (ha_ptr->actstat[i] == AS_SCB_BIOS)
      return(NOT_OUR_INT);

   wt4req(ha_ptr);

   i = INBYTE(ha_ptr->sxfrctl1);              /* Turn parity checking off.  */
   OUTBYTE(ha_ptr->sxfrctl1,i & ~ENSPCHK);    /* It will be turned back on  */
                     /* in message out phase       */
   OUTBYTE(ha_ptr->clrint,CLRSCSINT);
   return(OUR_OTHER_INT);
}
/*********************************************************************
*
*  target_abort routine -
*
*  Abort current target
*
*  Return Value: none
*             
*  Parameters: ha_ptr
*              scb_ptr
*
*  Activation: int_handler
*              handle_msgi
*             
*  Remarks:   
*             
*********************************************************************/
UBYTE target_abort (him_struct *ha_ptr, scb_struct *scb_ptr)
{
   him_config *config_ptr;
   UBYTE index, rtn_code = 1;
   UBYTE abt_msg = MSG06;           /* Default msg - Bus Device Reset */
   int efl; /* -ali */

   if (INBYTE(ha_ptr->sblkctl) & SELBUS1)          /* Select channel */
      config_ptr = ha_ptr->BConfigPtr;
   else
      config_ptr = ha_ptr->AConfigPtr;

   if ((INBYTE(ha_ptr->intstat) & INTCODE) == NO_ID_MSG)
   {
      OUTBYTE(ha_ptr->scsisig, MIPHASE | ATNO);
      INBYTE(ha_ptr->scsidatl);
   }
   else if ((INBYTE(ha_ptr->scsisig) & BUSPHASE) == MIPHASE)
   {
      if (wt4msgo(ha_ptr))                         /* Set ATN to force MSG OUT */
      {
         badseq(ha_ptr, scb_ptr);
         return(rtn_code);
      }
   }
   if (wt4req(ha_ptr) == MOPHASE)                  /* If Msg Out, then abort */
   {                                               
      if (scb_ptr != NOT_DEFINED)                  /* SCB pointer valid?  */
      {
         abt_msg = MSG06;                          /* Abort */
         if (INBYTE(ha_ptr->scb00) & (TAG_ENABLE | SDISCON))
         {
            abt_msg = MSG0D;                       /* Abort tag           */
         }
      }
      if (scb_send_trm_msg(config_ptr, abt_msg) & BUSFREE)
      {
         rtn_code = 0;
         OUTBYTE(ha_ptr->clrsint1, CLRBUSFREE);
         OUTBYTE(ha_ptr->clrint, CLRSCSINT);
         OUTBYTE(ha_ptr->seqaddr0, (UBYTE) (IDLE_LOOP_ENTRY >> 2));
         OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) (IDLE_LOOP_ENTRY >> 10));

         GET_SCB_INDEX( index )
         if (ha_ptr->actstat[index] != AS_SCBFREE)
         {
            efl = SaveAndDisable();                      /* INTR_OFF; -ali */
            ha_ptr->PreemptScb(ha_ptr, HOST_ABT_HA);     /* Terminate external SCB */
            RestoreState(efl);                           /* INTR_ON; -ali */
            scb_ptr->SCB_MgrStat = SCB_ABORTED;
            post_command(ha_ptr);
         }
      }
   }
   if (rtn_code)                       /* Abort unsuccessful, clean up */
   {
      badseq(ha_ptr, scb_ptr);
   }
   return(rtn_code);
}
/*********************************************************************
*
*  cdb_abort routine -
*
*  Send SCSI abort msg to selected target
*
*  Return Value:  None
*                  
*  Parameters:    ha_ptr,
*                 stb_ptr
*
*  Activation:    int_handler
*                  
*  Remarks:       limited implementation, at present
*                  
*********************************************************************/
void cdb_abort (him_struct *ha_ptr, scb_struct *scb_ptr)
{
   if ((INBYTE(ha_ptr->scsisig) & BUSPHASE) != MIPHASE)
   {
      badseq(ha_ptr, scb_ptr);
      return;
   }
   if (INBYTE(ha_ptr->scsibusl) == MSG03)
   {
      OUTBYTE(ha_ptr->scsisig, MIPHASE);
      INBYTE(ha_ptr->scsidatl);
      OUTBYTE(ha_ptr->seqaddr0, (UBYTE) (SIOSTR3_ENTRY >> 2));
      OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) (SIOSTR3_ENTRY >> 10));
   }
   else
   {
      if (wt4msgo(ha_ptr))
      {
         badseq(ha_ptr, scb_ptr);
         return;
      }
      OUTBYTE(ha_ptr->scsidatl, MSG07);
      wt4req(ha_ptr);
      OUTBYTE(ha_ptr->seqaddr0, (UBYTE) (SIO204_ENTRY >> 2));
      OUTBYTE((ha_ptr->seqaddr0 + 1), (UBYTE) (SIO204_ENTRY >> 10));
   }
}
/*********************************************************************
*
*  wt4msgo routine -
*
*  handshake messages in while waiting for message out phase
*
*  Return Value:  None
*                  
*  Parameters:    ha_ptr,
*
*  Activation:    target_abort, cdb_abort
*                  
*  Remarks:       
*                  
*********************************************************************/
int wt4msgo (him_struct *ha_ptr)
{
   UBYTE phase = 0;

   OUTBYTE(ha_ptr->scsisig, MIPHASE | ATNO);
   do
   {
      INBYTE(ha_ptr->scsidatl);
      phase = wt4req(ha_ptr);
   } while (phase == MIPHASE);
   if (phase != MOPHASE)
   {
      return(-1);
   }
   OUTBYTE(ha_ptr->scsisig, MOPHASE);
   OUTBYTE(ha_ptr->clrsint1, CLRATNO);
   return(0);
}
/*********************************************************************
*
*     reset_channel routine -
*
*  Reset SCSBI bus and clear all associated interrupts.
*  Also clear synchronous / wide mode.
*
*  Return Value:  None
*                  
*  Parameters: ha_ptr
*
*  Activation: stb_init
*              badseq
*              intsrst
*                  
*  Remarks:                
*                  
*********************************************************************/
void reset_channel (him_struct *ha_ptr)
{
   him_config *config_ptr;   
   UBYTE i, j, channel;

   if (INBYTE(ha_ptr->sblkctl) & SELBUS1)
   {
      channel = CHANNEL;
      config_ptr = ha_ptr->BConfigPtr;
   }
   else
   {
      channel = 0;
      config_ptr = ha_ptr->AConfigPtr;
   }

   OUTBYTE(ha_ptr->scsiseq,0);
   OUTBYTE(ha_ptr->clrsint0,0xff);
   OUTBYTE(ha_ptr->clrsint1,0xff);

   OUTBYTE(ha_ptr->sxfrctl0, CLRSTCNT | CLRCHN);
   if (config_ptr->Cfe_PortAddress > 0xbff)
   {
      OUTBYTE(config_ptr->Cfe_PortAddress + EISA_HOST + DFCNTRL, FIFORESET);
   }
   else
   {
      OUTBYTE(config_ptr->Cfe_PortAddress + ISA_HOST + DFCNTRL, FIFORESET);
   }

   if (config_ptr->Cfe_ReleaseLevel == 0x01)
      OUTBYTE(ha_ptr->simode1,ENSCSIPERR | ENSCSIRST);               /* REV 'C' */
   else
      OUTBYTE(ha_ptr->simode1,ENSCSIPERR | ENSELTIMO | ENSCSIRST);   /* Later versions */

   if (config_ptr->Cfe_SCSIChannel == A_CHANNEL)
      j = 0;
   else
      j = 8;

   /* Re-Initialize sync/wide negotiation parameters */
   for (i = 0; i < config_ptr->Cfe_MaxTargets; i++)   
   {
      if (config_ptr->Cfe_ScsiOption[i] & (WIDE_MODE + SYNC_MODE))
         OUTBYTE((ha_ptr->xfer_option) + j, 0x8f);
      else
         OUTBYTE((ha_ptr->xfer_option) + j, 0);
      j++;
   }
   i = INBYTE(ha_ptr->scbptr);               /* Save current channel */
   for (j = 0; j < HARD_QDEPTH; j++)         /* Re-Initialize internal SCB's */
   {
      if ((INBYTE(ha_ptr->scb00 + 1) & CHANNEL) != channel)
         continue;
      OUTBYTE(ha_ptr->scbptr, j);
      OUTBYTE(ha_ptr->scb00, 0x00);
   }
   OUTBYTE(ha_ptr->scbptr, i);               /* Restore channel */

   if (ha_ptr->HaFlags & HAFL_SWAP_ON)
   {
      /* Clear active_scb, waiting_scb_chx flags */
   }
}
/*********************************************************************
*
*  check_condition routine -
*
*  handle response to target check condition
*
*  Return Value:  None
*                  
*  Parameters:    ha_ptr
*                 scb_ptr
*
*  Activation:    int_handler
*                  
*  Remarks:                
*                  
*********************************************************************/
void int_check_condition (him_struct *ha_ptr,scb_struct *scb_ptr)
{
   union sense {
      UBYTE sns_array[4];
      DWORD sns_ptr;
   } sns_segptr;
   UBYTE i;
   UBYTE j;
   UBYTE status;
   UBYTE queue[QDEPTH];
   int efl; /* -ali */

   i = scb_ptr->SCB_Tarlun;
   if (i & CHANNEL)
   {
      scb_renego(ha_ptr->BConfigPtr, i);
   }
   else
   {
      scb_renego(ha_ptr->AConfigPtr, i);
   }

   if (scb_ptr->SCB_TargStat == UNIT_CHECK)
   {
      scb_ptr->SCB_HaStat = HOST_SNS_FAIL;
      status = 0;
   }
   else 
   {
      status = INBYTE(ha_ptr->scb14);
      scb_ptr->SCB_TargStat = status;
   }
   if ((status == UNIT_CHECK) && (scb_ptr->SCB_Flags & AUTO_SENSE))
   {
      OUTBYTE(ha_ptr->scb00,REJECT_MDP);
      OUTBYTE(ha_ptr->scb02,0x01);
      sns_segptr.sns_ptr = scb_ptr->SCB_CDBPtr + MAX_CDB_LEN;

      for (i = 0; i < 4; i++)
         OUTBYTE((ha_ptr->scb03) + i, sns_segptr.sns_array[i]);

      OUTBYTE(ha_ptr->scb11,0x06);
      for (i = 0; i < 5; i++) OUTBYTE((ha_ptr->scb14) + i,0x00);
      for (i = 0; i < 6; ++i) scb_ptr->SCB_CDB[i] = 0x00;
      scb_ptr->SCB_CDB[0] = 0x03;
      scb_ptr->SCB_CDB[4] = (UBYTE) scb_ptr->SCB_SenseLen;
      i = 0;
      queue[i++] = INBYTE(ha_ptr->scbptr);
      while (INBYTE(ha_ptr->qincnt) > 0)
         queue[i++] = INBYTE(ha_ptr->qinfifo);
      j = 0;
      while (j < i) OUTBYTE(ha_ptr->qinfifo,queue[j++]);

      scb_ptr->SCB_MgrStat = SCB_AUTOSENSE;
   }
   else
   {
      efl = SaveAndDisable(); /* INTR_OFF; -ali */
      ha_ptr->PreemptScb(ha_ptr, scb_ptr->SCB_HaStat);
      RestoreState(efl); /* INTR_ON; -ali */
   }
   OUTBYTE(ha_ptr->sxfrctl0,INBYTE(ha_ptr->sxfrctl0) | CLRCHN);
   OUTBYTE(ha_ptr->simode1, INBYTE(ha_ptr->simode1) & ~ENBUSFREE);
   INBYTE(ha_ptr->scsidatl);
   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) (IDLE_LOOP_ENTRY >> 2));
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) (IDLE_LOOP_ENTRY >> 10));
   if ((wt4bfree(ha_ptr)) & REQINIT)
      badseq(ha_ptr,scb_ptr);
}
/*********************************************************************
*
*  check_length routine -
*
*  Check for underrun/overrun conditions following data transfer
*
*  Return Value: None
*                  
*  Parameters: ha_ptr
*              scb_ptr
*
*  Activation: int_handler
*                  
*  Remarks:                
*                  
*********************************************************************/
void check_length (him_struct *ha_ptr,scb_struct *scb_ptr)
{
   UBYTE *ptr;
   UBYTE phase;
   UBYTE i;

   if ((phase = (INBYTE(ha_ptr->scsisig) & BUSPHASE)) == STPHASE)
   {
#ifndef USL
	/* For some reason, Victoria has this commented out */
      if (scb_ptr->SCB_Flags & NO_UNDERRUN)
    return;
#endif
      if (scb_ptr->SCB_TargStat == UNIT_CHECK)
    return;
      OUTBYTE(ha_ptr->scbcnt,SCB15 + SCBAUTO);
      ptr = &scb_ptr->SCB_TargStat;    /* Can't point directly to ResCnt   */
                   /* since it's not defined as a byte */
      for (i = 0; i < 0x04; i++) *(++ptr) = INBYTE(ha_ptr->scb00);
      OUTBYTE(ha_ptr->scbcnt,0x00);
   }
   else if ((phase & CDO) == 0)
   {
      OUTBYTE(ha_ptr->scsisig,phase);
      i = INBYTE(ha_ptr->sxfrctl1);
      OUTBYTE(ha_ptr->sxfrctl1, i | BITBUCKET);
      while ((INBYTE(ha_ptr->sstat1) & PHASEMIS) == 0);
      OUTBYTE(ha_ptr->sxfrctl1,i);
   }
#ifndef USL
   scb_ptr->SCB_HaStat = HOST_DU_DO;
#endif
}
/*********************************************************************
*
*  extmsgi routine -
*
*  Receive and interpret extended message in
*
*  Return Value:  None
*                  
*  Parameters: ha_ptr
*              scb_ptr
*
*  Activation: int_handler
*                  
*  Remarks:
*                  
*********************************************************************/
void extmsgi (him_struct *ha_ptr, scb_struct *scb_ptr)
{
   him_config *config_ptr;   
   UBYTE c, i, j;
   UBYTE phase, nego_flag = NONEGO, scsi_rate;
   UBYTE max_width = 0, max_rate, max_offset = NARROW_OFFSET;

   j = (scb_ptr->SCB_Tarlun >> 4);
   if (scb_ptr->SCB_Tarlun & CHANNEL)
   {
      config_ptr = ha_ptr->BConfigPtr;
      j |= CHANNEL;
   }
   else
   {
      config_ptr = ha_ptr->AConfigPtr;
   }
   c = INBYTE(ha_ptr->pass_to_driver);

   scb_ptr->SCB_MgrWork[0] = MSG01;                   /* Extended Message */
   scb_ptr->SCB_MgrWork[1] = c--;                     /* Remaining length */
   scb_ptr->SCB_MgrWork[2] = INBYTE(ha_ptr->scsibusl); /* Request Code */

   for (i = 3; c > 0; --c)          /* Get rest of extended message */
   {
      INBYTE(ha_ptr->scsidatl);                    /* Get next byte */
      if ((phase = wt4req(ha_ptr)) != MIPHASE)     /* Check phase transition */
      {
         if ((INBYTE(ha_ptr->scsisig) & (BUSPHASE | ATNI)) == (MOPHASE | ATNI))
         {
            if (scb_ptr->SCB_Flags & NEGO_IN_PROG)
            {
               OUTBYTE(ha_ptr->scsisig, MOPHASE | ATNO);
               OUTBYTE((ha_ptr->xfer_option) + j, 0x8f);
            }
            else
            {
               OUTBYTE(ha_ptr->clrsint1, CLRATNO);
               OUTBYTE(ha_ptr->scsisig, MOPHASE);
            }
            i = INBYTE(ha_ptr->sxfrctl0);
            j = INBYTE(ha_ptr->sxfrctl1);
            OUTBYTE(ha_ptr->sxfrctl1, j & ~ENSPCHK); /* Clear parity error   */
            OUTBYTE(ha_ptr->sxfrctl1, j | ENSPCHK);
            OUTBYTE(ha_ptr->clrint, CLRSCSINT);
            OUTBYTE(ha_ptr->sxfrctl0, i & ~SPIOEN);  /* Place message parity */
            OUTBYTE(ha_ptr->scsidatl, MSG09);        /* error on bus without */
            OUTBYTE(ha_ptr->sxfrctl0, i | SPIOEN);   /* an ack.              */
         }
         else
         {
            if (phase == ERR)
               return;
            badseq(ha_ptr, scb_ptr);
         }
         return;
      }
      if (i < 5)                    /* Get last byte without ACK'ing it */
         scb_ptr->SCB_MgrWork[i++] = INBYTE(ha_ptr->scsibusl);
   }
   i = (scb_ptr->SCB_Tarlun >> 4);  /* Extract target SCSI ID */

   max_rate = (((config_ptr->Cfe_ScsiOption[i] & SYNC_RATE) >> 4) * 6) + 25;

   if (config_ptr->Cfe_ScsiOption[i] & SXFR2)
      ++max_rate;
   if (INBYTE(ha_ptr->sblkctl) & SELWIDE)
      max_width = WIDE_WIDTH;

   switch (scb_ptr->SCB_MgrWork[2])
   {
      case MSGWIDE:

      if (scb_ptr->SCB_MgrWork[1] == 2)
      {
         OUTBYTE((ha_ptr->xfer_option) + j, 0);
         OUTBYTE(ha_ptr->scsirate, 0);
         if (scb_ptr->SCB_MgrWork[3] > max_width)
         {
            scb_ptr->SCB_MgrWork[3] = max_width;
            nego_flag = NEEDNEGO;
         }
         if ((scb_ptr->SCB_Flags & NEGO_IN_PROG) == 0)
            break;
         scb_ptr->SCB_Flags &= ~NEGO_IN_PROG;
         if (nego_flag == NONEGO)
         {
            if (scb_ptr->SCB_MgrWork[3])
            {
               OUTBYTE((ha_ptr->xfer_option) + j, WIDEXFER);
               OUTBYTE(ha_ptr->scsirate,WIDEXFER);
               max_offset = WIDE_OFFSET;
            }
            if (config_ptr->Cfe_ScsiOption[i] & SYNC_MODE)
            {
               scb_ptr->SCB_MgrWork[1] = 3;
               scb_ptr->SCB_MgrWork[2] = MSGSYNC;
               scb_ptr->SCB_MgrWork[3] = max_rate;
               scb_ptr->SCB_MgrWork[4] = max_offset;
               OUTBYTE(ha_ptr->scsisig, ATNO | MIPHASE);
               scb_ptr->SCB_Flags |= NEGO_IN_PROG;
            }
            return;
         }
      }
      scb_ptr->SCB_MgrWork[1] = 2;

      case MSGSYNC:

      if (scb_ptr->SCB_MgrWork[1] == 3)
      {
         scsi_rate = INBYTE((ha_ptr->xfer_option) + j);
         if (scsi_rate == NEEDNEGO)
            scsi_rate = 0;
         else
            scsi_rate &= WIDEXFER;

         /* Set synchronous transfer rate */
         OUTBYTE((ha_ptr->xfer_option) + j, scsi_rate);
         OUTBYTE(ha_ptr->scsirate, scsi_rate);

         if (scb_ptr->SCB_MgrWork[4])
         {
            if (scsi_rate)
               max_offset = WIDE_OFFSET;
            if (scb_ptr->SCB_MgrWork[4] > max_offset)
            {
               scb_ptr->SCB_MgrWork[4] = max_offset;
               nego_flag = NEEDNEGO;
            }
            if (scb_ptr->SCB_MgrWork[3] < max_rate)
            {
               scb_ptr->SCB_MgrWork[3] = max_rate;
               nego_flag = NEEDNEGO;
            }
            else if (scb_ptr->SCB_MgrWork[3] > 68)
            {
               scb_ptr->SCB_MgrWork[4] = 0;
               nego_flag = NEEDNEGO;
               OUTBYTE(ha_ptr->scsisig, ATNO | MIPHASE);
               scb_ptr->SCB_Flags |= NEGO_IN_PROG;
               return;
            }
         }
         if ((scb_ptr->SCB_Flags & NEGO_IN_PROG) == 0)
            break;
         scb_ptr->SCB_Flags &= ~NEGO_IN_PROG;
         if (nego_flag == NONEGO)
         {
            /* Set synchronous transfer rate */
            scsi_rate += syncset(scb_ptr) + scb_ptr->SCB_MgrWork[4];
            OUTBYTE((ha_ptr->xfer_option) + j, scsi_rate);
            OUTBYTE(ha_ptr->scsirate, scsi_rate);
            return;
         }
      }
      default:

      OUTBYTE(ha_ptr->scsisig, MIPHASE | ATNO);
      INBYTE(ha_ptr->scsidatl);
      if ((phase = wt4req(ha_ptr)) == MOPHASE)
      {
         OUTBYTE(ha_ptr->scsisig,MOPHASE);
         OUTBYTE(ha_ptr->clrsint1,CLRATNO);
         OUTBYTE(ha_ptr->sxfrctl0,INBYTE(ha_ptr->sxfrctl0) & ~SPIOEN);
         OUTBYTE(ha_ptr->scsidatl,MSG07);
         OUTBYTE(ha_ptr->sxfrctl0,INBYTE(ha_ptr->sxfrctl0) | SPIOEN);
      }
      else
      {
         if (phase == ERR)
            return;
         badseq(ha_ptr, scb_ptr);
      }
      return;
   }
   scb_ptr->SCB_MgrWork[0] = 0xff;
   OUTBYTE(ha_ptr->scsisig,ATNO | MIPHASE);
   scb_ptr->SCB_Flags |= NEGO_IN_PROG;
}
/*********************************************************************
*
*  extmsgo routine -
*
*  Send extended message out
*
*  Return Value: current scsi bus phase (unsigned char)
*             
*  Parameters: ha_ptr
*              scb_ptr
*
*  Activation: negotiate
*              sendmo
*              synnego
*             
*  Remarks:   
*             
*********************************************************************/
UBYTE extmsgo (him_struct *ha_ptr,scb_struct *scb_ptr)
{
   UBYTE c;
   UBYTE i = 0;
   UBYTE phase;
   UBYTE j;
   UBYTE scsi_rate = 0;

   /*@ Fix for target-initiated sync negotiation problem */
   UBYTE savcnt0,savcnt1,savcnt2;                                       /* save for stcnt just in case of pio */
   UWORD base_addr;

   base_addr = ha_ptr->scsidatl & 0xfc00;
   savcnt0 = INBYTE(base_addr + STCNT0);                        /* save STCNT here */
   savcnt1 = INBYTE(base_addr + STCNT1);                        /* save STCNT here */
   savcnt2 = INBYTE(base_addr + STCNT2);                        /* save STCNT here */
   /*@ Fix for target-initiated sync negotiation problem */

   for (c = scb_ptr->SCB_MgrWork[1] + 1; c > 0; --c)
   {
      OUTBYTE(ha_ptr->scsidatl,scb_ptr->SCB_MgrWork[i++]);
      if ((phase = wt4req(ha_ptr)) != MOPHASE)
      {
         OUTBYTE(ha_ptr->clrsint1,CLRATNO);

         /*@ Fix for target-initiated sync negotiation problem */
         OUTBYTE(base_addr + STCNT0,savcnt0);   /* restore STCNT */
         OUTBYTE(base_addr + STCNT1,savcnt1);   /* restore STCNT */
         OUTBYTE(base_addr + STCNT2,savcnt2);   /* restore STCNT */
         /*@ Fix for target-initiated sync negotiation problem */

         return(phase);
      }
   }
   if (scb_ptr->SCB_Flags & NEGO_IN_PROG)
   {
      j = (scb_ptr->SCB_Tarlun >> 4) | (scb_ptr->SCB_Tarlun & CHANNEL);
      if (scb_ptr->SCB_MgrWork[2] == MSGWIDE)
      {
         if (scb_ptr->SCB_MgrWork[3])
            scsi_rate = WIDEXFER;
      }
      else
      {
         /* Set synchronous transfer rate */
         scsi_rate = INBYTE((ha_ptr->xfer_option) + j)
                     + syncset(scb_ptr)
                     + scb_ptr->SCB_MgrWork[4];
      }
      OUTBYTE((ha_ptr->xfer_option) + j, scsi_rate);
      OUTBYTE(ha_ptr->scsirate,scsi_rate);
   }
   scb_ptr->SCB_Flags ^= NEGO_IN_PROG;

   OUTBYTE(ha_ptr->clrsint1,CLRATNO);
   OUTBYTE(ha_ptr->scsidatl,scb_ptr->SCB_MgrWork[i]);

   /*@ Fix for target-initiated sync negotiation problem */
   OUTBYTE(base_addr + STCNT0,savcnt0);         /* restore STCNT */
   OUTBYTE(base_addr + STCNT1,savcnt1);         /* restore STCNT */
   OUTBYTE(base_addr + STCNT2,savcnt2);         /* restore STCNT */
   /*@ Fix for target-initiated sync negotiation problem */

   return(wt4req(ha_ptr));
}
/*********************************************************************
*
*  handle_msgi routine -
*
*  Handle Message In
*
*  Return Value: none
*
*  Parameters: ha_ptr
*              scb_ptr
*
*  Activation: int_handler
*
*  Remarks:
*
*********************************************************************/
void handle_msgi (him_struct *ha_ptr,scb_struct *scb_ptr)
{
   UBYTE rejected_msg;
   UBYTE phase;

   if ((INBYTE(ha_ptr->scsisig) & ATNI) == 0)
   {
      if (INBYTE(ha_ptr->scsibusl) == MSG07)
      {
         rejected_msg = INBYTE(ha_ptr->pass_to_driver); /* Get rejected msg    */
         if (rejected_msg & (MSGID | MSGTAG))           /* If msg Identify or  */ 
         {                                              /* tag type, abort     */
            OUTBYTE(ha_ptr->scsisig,MIPHASE | ATNO);
            INBYTE(ha_ptr->scsidatl);
            target_abort(ha_ptr,scb_ptr);
            return;
         }
      }
      else
      {
         OUTBYTE(ha_ptr->scsisig,MIPHASE | ATNO);
         do
         {
            INBYTE(ha_ptr->scsidatl);
            phase = wt4req(ha_ptr);
         } while (phase == MIPHASE);

         if (phase != MOPHASE)
         {
            badseq(ha_ptr,scb_ptr);
            return;
         }
      OUTBYTE(ha_ptr->scsisig,MOPHASE);
      OUTBYTE(ha_ptr->clrsint1,CLRATNO);
      OUTBYTE(ha_ptr->scsidatl,MSG07);
      return;
      }
   }
   INBYTE(ha_ptr->scsidatl);
}
/*********************************************************************
*
*  intsrst routine -
*
*  Process case where other device resets scsi bus
*
*  Return Value:  None
*                  
*  Parameters: ha_ptr
*
*  Activation: int_handler
*                  
*  Remarks:       INTR_OFF (i.e. system interrupts off) must be executed
*                 prior to calling this routine.
*                  
*********************************************************************/
void intsrst (him_struct *ha_ptr)
{
   him_config *config_ptr;

   if (INBYTE(ha_ptr->sblkctl) & SELBUS1)          /* Select channel */
      config_ptr = ha_ptr->BConfigPtr;
   else
      config_ptr = ha_ptr->AConfigPtr;
   while (INBYTE(ha_ptr->sstat1) & SCSIRSTI)
   {
      OUTBYTE(ha_ptr->clrsint1, CLRSCSIRSTI);
      OUTBYTE(ha_ptr->clrint, CLRSCSINT);
   }
   /* INTR_OFF; */
   /* Leave interrrupts off     */
   /* until BOOKMARK is removed */
   /* Insert "Bookmark" in queue */

   /* asynchronous event notification */
   scb_AsynchEvent(AE_3PTY_RST, config_ptr, 0L);

   insert_bookmark(ha_ptr);

   OUTBYTE(ha_ptr->scsiseq, 0);
   if (INBYTE(ha_ptr->scsisig))
   {
      reset_scsi(ha_ptr);
   }

   reset_channel(ha_ptr);

   abort_channel(ha_ptr, HOST_ABT_HA);

   remove_bookmark(ha_ptr);

   /* INTR_ON; */
   
   while (ha_ptr->done_cmd)         /* Post completed commands back to host */
   {
      post_command(ha_ptr);
   }
   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) IDLE_LOOP_ENTRY >> 10);
}
/*********************************************************************
*
*  negotiate routine -
*
*  Initiate synchronous and/or wide negotiation
*
*  Return Value:  None
*                  
*  Parameters: ha_ptr
*              scb_ptr
*
*  Activation: int_handler
*              sendmo
*                  
*  Remarks:                
*                  
*********************************************************************/
void negotiate (him_struct *ha_ptr,scb_struct *scb_ptr)
{
   him_config *config_ptr;   
   UBYTE i;
   UBYTE j;

   i = (scb_ptr->SCB_Tarlun >> 4);
   if (scb_ptr->SCB_Tarlun & CHANNEL)
   {
      config_ptr = ha_ptr->BConfigPtr;
      j = i | CHANNEL;
   }
   else
   {
      config_ptr = ha_ptr->AConfigPtr;
      j = i;
   }
   if ((INBYTE(ha_ptr->scsisig) & BUSPHASE) != MOPHASE)
   {
      OUTBYTE((ha_ptr->xfer_option) + j,0);
      OUTBYTE(ha_ptr->scsirate,0);
      goto von_kludge;
   }      
   if (INBYTE((ha_ptr->xfer_option) + j) == NEEDNEGO)
   {
      OUTBYTE((ha_ptr->xfer_option) + j,0);
      OUTBYTE(ha_ptr->scsirate,0);
      scb_ptr->SCB_MgrWork[0] = MSG01;

      switch (config_ptr->Cfe_ScsiOption[i] & (WIDE_MODE | SYNC_MODE))
      {
         case (WIDE_MODE | SYNC_MODE):
         case WIDE_MODE:

         scb_ptr->SCB_MgrWork[1] = 2;
         scb_ptr->SCB_MgrWork[2] = MSGWIDE;
         scb_ptr->SCB_MgrWork[3] = WIDE_WIDTH;

         if (extmsgo(ha_ptr,scb_ptr) != MIPHASE)
            goto von_kludge;

         switch (INBYTE(ha_ptr->scsibusl))
         {
            case MSG01:
               scb_ptr->SCB_Flags |= NEGO_IN_PROG;
               goto von_kludge;

            case MSG07:
            if (config_ptr->Cfe_ScsiOption[i] & SYNC_MODE)
            {
               OUTBYTE(ha_ptr->scsisig,MIPHASE | ATNO);
               INBYTE(ha_ptr->scsidatl);
               if (wt4req(ha_ptr) == MOPHASE)
               {
               OUTBYTE(ha_ptr->scsisig,MOPHASE | ATNO);
               break;
               }
            }
            else
            {
               OUTBYTE(ha_ptr->scsisig,MIPHASE);
               INBYTE(ha_ptr->scsidatl);
            }

            default:
            goto von_kludge;
         }
         case SYNC_MODE:

         scb_ptr->SCB_MgrWork[1] = 3;
         scb_ptr->SCB_MgrWork[2] = MSGSYNC;
         scb_ptr->SCB_MgrWork[3] = ((config_ptr->Cfe_ScsiOption[i] & SYNC_RATE) >> 4) * 6;
         scb_ptr->SCB_MgrWork[3] += 25;
         if (config_ptr->Cfe_ScsiOption[i] & SXFR2) ++scb_ptr->SCB_MgrWork[3];
            scb_ptr->SCB_MgrWork[4] = NARROW_OFFSET;
         synnego(ha_ptr,scb_ptr);
         goto von_kludge;
      }
   }
   sendmo(ha_ptr,scb_ptr);

von_kludge:
   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) (SIOSTR3_ENTRY >> 2));
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) (SIOSTR3_ENTRY >> 10));
   return;
}
/*********************************************************************
*
*  reset_scsi routine -
*
*  Perform SCSI bus reset
*
*  Return Value:  None
*                  
*  Parameters:    ha_ptr
*
*  Activation:    stb_init
*                 badseq
*                  
*  Remarks:       Sequencer must be already paused.
*                  
*********************************************************************/
void reset_scsi (him_struct *ha_ptr)
{
   UWORD i = 256;

   OUTBYTE(ha_ptr->scsiseq, SCSIRSTO);
   while (1) { if (!i--) break; }
   OUTBYTE(ha_ptr->scsiseq, 0);
}
/*********************************************************************
*
*  sendmo routine -
*
*  send message out
*
*  Return Value:  None
* 
*  Parameters:    ha_ptr
*                 stb_ptr
*
*  Activation:    int_handler
*                 negotiate                  
*
*  Remarks:                
*                  
*********************************************************************/
void sendmo (him_struct *nha_ptr,scb_struct *nscb_ptr)
{
   register him_struct *ha_ptr;
   register scb_struct *scb_ptr;
   register UBYTE j;
   register UBYTE scsi_rate;

   ha_ptr = nha_ptr;
   scb_ptr = nscb_ptr;

   j = (scb_ptr->SCB_Tarlun >> 4) | (scb_ptr->SCB_Tarlun & CHANNEL);
   OUTBYTE(ha_ptr->scsisig,MOPHASE);

   if (INBYTE(ha_ptr->scsisig) & ATNI)
   {
      if (scb_ptr->SCB_Flags & NEGO_IN_PROG)
      {
         if (scb_ptr->SCB_MgrWork[0] == 0xff)
         {
            scb_ptr->SCB_MgrWork[0] = 0x01;
            if (extmsgo(ha_ptr,scb_ptr) != MIPHASE) return;
            if (INBYTE(ha_ptr->scsibusl) != MSG07) return;
            if (scb_ptr->SCB_MgrWork[2] == MSGSYNC)
            {
               /* Set synchronous transfer rate */
               scsi_rate = INBYTE((ha_ptr->xfer_option) + j) & WIDEXFER;
            }
            else
               scsi_rate = 0;
            OUTBYTE((ha_ptr->xfer_option) + j,scsi_rate);
            OUTBYTE(ha_ptr->scsirate,scsi_rate);
            INBYTE(ha_ptr->scsidatl);
            return;
         }
         scb_ptr->SCB_Flags &= ~NEGO_IN_PROG;
         if (scb_ptr->SCB_MgrWork[0] == 0x01)
            synnego(ha_ptr,scb_ptr);
         else if (INBYTE((ha_ptr->xfer_option) + j) == 0x8f)
            negotiate(ha_ptr,scb_ptr);
         return;
      }
      j = INBYTE(ha_ptr->sxfrctl1);           /* Turn off parity checking to*/
      OUTBYTE(ha_ptr->sxfrctl1,j & ~ENSPCHK); /* clear any residual error.  */
      OUTBYTE(ha_ptr->sxfrctl1,j | ENSPCHK);  /* Turn it back on explicitly */
      /* because it may have been   */
      /* cleared in 'parity_error'. */
      /* (It had to been previously */
      /* set or we wouldn't have    */
      /* gotten here.)              */
      OUTBYTE(ha_ptr->clrsint1,CLRSCSIPERR | CLRATNO);
      OUTBYTE(ha_ptr->clrint,CLRSCSINT);
      OUTBYTE(ha_ptr->scsidatl,MSG05);
   }
   else
      OUTBYTE(ha_ptr->scsidatl,MSG08);

   while (INBYTE(ha_ptr->scsisig) & ACKI);
   return;
}
/*********************************************************************
*
*  syncset routine -
*
*  Set synchronous transfer rate based on negotiation
*
*  Return Value:  synchronous transfer rate (unsigned char)
*                  
*  Parameters: ha_ptr
*
*  Activation: extmsgi
*              extmsgo
*                  
*  Remarks:
*                  
*********************************************************************/
UBYTE syncset (scb_struct *scb_ptr)
{
   UBYTE sync_rate;

   if (scb_ptr->SCB_MgrWork[3] == 25)
      sync_rate = 0;
   else if (scb_ptr->SCB_MgrWork[3] <= 31)
      sync_rate = 0x10;
   else if (scb_ptr->SCB_MgrWork[3] <= 37)
      sync_rate = 0x20;
   else if (scb_ptr->SCB_MgrWork[3] <= 43)
      sync_rate = 0x30;
   else if (scb_ptr->SCB_MgrWork[3] <= 50)
      sync_rate = 0x40;
   else if (scb_ptr->SCB_MgrWork[3] <= 56)
      sync_rate = 0x50;
   else if (scb_ptr->SCB_MgrWork[3] <= 62)
      sync_rate = 0x60;
   else
      sync_rate = 0x70;

   return(sync_rate);
}
/*********************************************************************
*
*  synnego routine -
*
*  <brief description>
*
*  Return Value:  None
*                  
*  Parameters: ha_ptr
*              stb_ptr
*
*  Activation: negotiate
*              sendmo
*                  
*  Remarks:                
*                  
*********************************************************************/
void synnego (him_struct *ha_ptr,scb_struct *scb_ptr)
{
   if (extmsgo(ha_ptr,scb_ptr) != MIPHASE)
      return;

   switch (INBYTE(ha_ptr->scsibusl))
   {
      case MSG01:

      scb_ptr->SCB_Flags |= NEGO_IN_PROG;
      return;

      case MSG07:

      while (1)            /* Process any number of Message Rejects */
      {
         OUTBYTE(ha_ptr->scsisig,MIPHASE);
         INBYTE(ha_ptr->scsidatl);
         if (wt4req(ha_ptr) != MIPHASE)
            break;
         if (INBYTE(ha_ptr->scsibusl) != MSG07)
            break;
      }
      return;
   }
}
/*********************************************************************
*
*  wt4req routine -
*
*  wait for target to assert REQ.
*
*  Return Value:  current SCSI bus phase
*
*  Parameters     ha_ptr
*
*  Activation:    most other HIM routines
*
*  Remarks:       bypasses sequencer
*
*********************************************************************/
UBYTE wt4req (him_struct *ha_ptr)
{
   UBYTE stat;
   UBYTE phase;

   for (;;)
   {
      while (INBYTE(ha_ptr->scsisig) & ACKI);
      while (((stat = INBYTE(ha_ptr->sstat1)) & REQINIT) == 0)
      {
    if (stat & (BUSFREE | SCSIRSTI))
       return(ERR);
      }
      OUTBYTE(ha_ptr->clrsint1,CLRSCSIPERR);
      phase = INBYTE(ha_ptr->scsisig) & BUSPHASE;
      if ((phase & IOI) &&
     (phase != DIPHASE) &&
     (INBYTE(ha_ptr->sstat1) & SCSIPERR))
      {
    OUTBYTE(ha_ptr->scsisig,phase);
    INBYTE(ha_ptr->scsidatl);
    continue;
      }
      return(phase);
   }
}
/*********************************************************************
*
*  Start of Fault Tolerant code enhancements. 
*
*  CSF 7-20-92
*
*********************************************************************/
/*********************************************************************
*  scb_non_init -
*
*  Parse non-initiator command request, activate abort, device reset
*  or read sense routines.
*
*  Return Value:  
*                  
*  Parameters:    
*
*  Activation: scb_send
*
*  Remarks:       
*                 
*********************************************************************/
UBYTE scb_non_init (him_config *config_ptr, scb_struct *scb_pointer)
{
   him_struct *ha_ptr;
   UBYTE retval = 0;

   ha_ptr = config_ptr->Cfe_HaDataPtr;

   switch (scb_pointer->SCB_Cmd)
   {
      case HARD_RST_DEV:
    scb_enque( config_ptr, scb_pointer);
    scb_hard_reset( config_ptr, scb_pointer);
    break;

      case READ_SENSE:
    scb_enque_hd( config_ptr, scb_pointer);
    /* scb_read_sense( config_ptr, scb_pointer); */
    break;

      case NO_OPERATION:
    break;

      case SOFT_RST_DEV:
    scb_enque( config_ptr, scb_pointer);
    /* scb_soft_reset( config_ptr, scb_pointer); */
    break;

      default:
    scb_enque( config_ptr, scb_pointer);
    scb_pointer->SCB_MgrStat = SCB_DONE_ILL;
    ++ha_ptr->done_cmd;

   }
   return (retval);
}
/*********************************************************************
*  scb_abort -
*
*  Determine state of SCB to be aborted and abort if not ACTIVE.
*
*  Return Value:  
*                  
*  Parameters:    
*
*  Activation: scb_non_init
*
*  Remarks:       INTR_OFF (i.e. system interrupts off) must be executed
*                 prior to calling this routine.
*
*********************************************************************/
void scb_abort (him_config *config_ptr, scb_struct *scb_pointer)
{
   him_struct *ha_ptr;
   scb_struct *scb_ptr;

   ha_ptr = config_ptr->Cfe_HaDataPtr;

   /* Verify SCB exists */

   if (find_chained_scb(config_ptr, scb_pointer) == NOT_DEFINED)
   {
      return;
   }

   scb_ptr = scb_pointer;

   /* Take action based on current SCB state */

   switch (scb_ptr->SCB_MgrStat)
   {
      case  SCB_DONE:         /* SCB already completed , just return */
      case  SCB_DONE_ABT:
      case  SCB_DONE_ERR:
      case  SCB_DONE_ILL:

      break;

      case  SCB_ACTIVE:       /* Active SCB */

      PAUSE_SEQ(ha_ptr->hcntrl);
      for (ha_ptr->cur_scb_ptr = 0; ha_ptr->cur_scb_ptr < 256; ha_ptr->cur_scb_ptr++)
      {
         if (ha_ptr->actptr[ha_ptr->cur_scb_ptr] == scb_ptr)
         {
            ha_ptr->PreemptScb(ha_ptr, HOST_ABT_HOST); /* Remove SCB from sequencer */
            break;
         }
      }
      UNPAUSE_SEQ(ha_ptr->hcntrl, ha_ptr->intstat);
      scb_ptr->SCB_MgrStat = SCB_ABORTED;         /* re-write MgrStat */
      break;
      
      case  SCB_PROCESS:      /* SCB not yet loaded into Arrow */
      case  SCB_WAITING:
      case  SCB_READY:

      scb_ptr->SCB_HaStat  = HOST_ABT_HOST;
      terminate_command(ha_ptr, scb_ptr);
      scb_ptr->SCB_MgrStat = SCB_ABORTED;         /* re-write MgrStat */
      break;

      default:

      break;

   }
   post_command(ha_ptr);
   return;
}
/*********************************************************************
*  scb_enque -
*
*  Add SCB to tail of SCB Queue. If Queue doesn't exist, initialize it.
*
*  Return Value:  None
*                  
*  Parameters:    config_ptr,
*                 scb_pointer
*
*  Activation:    scb_non_init,
*
*  Remarks:       Duplicates logic presently found in scb_send
*                 
*********************************************************************/

void scb_enque (him_config *config_ptr, scb_struct *scb_pointer)
{
   him_struct *ha_ptr;
   scb_struct *scb_ptr;

   ha_ptr = config_ptr->Cfe_HaDataPtr;

   if (ha_ptr->Head_Of_Queue == NOT_DEFINED)
   {
      scb_ptr = scb_pointer;
      ha_ptr->Head_Of_Queue = scb_ptr;
      ha_ptr->AConfigPtr->Cfe_ConfigFlags &= ~DRIVER_IDLE;
      if (config_ptr->Cfe_SCSIChannel == B_CHANNEL)
    ha_ptr->BConfigPtr->Cfe_ConfigFlags &= ~DRIVER_IDLE;
   }
   else
   {
      scb_ptr = ha_ptr->End_Of_Queue;
      scb_ptr->SCB_Next = scb_pointer;
      scb_ptr = scb_ptr->SCB_Next;
   }
   for (;;scb_ptr = scb_ptr->SCB_Next)
   {
      scb_ptr->SCB_Stat = SCB_PENDING;
      scb_ptr->SCB_MgrStat = SCB_PROCESS;
      if (scb_ptr->SCB_Next == NOT_DEFINED) 
    break;
   }
   ha_ptr->End_Of_Queue = scb_ptr;
}
/*********************************************************************
*  scb_enque_hd -
*
*  Add SCB to head of SCB Queue. If Queue doesn't exist, initialize it.
*
*  Return Value:  None
*                  
*  Parameters:    config_ptr,
*                 scb_pointer
*
*  Activation:    scb_non_init,
*
*  Remarks:       Used for Read Sense SCB opcode.
*                 Cannot add chained SCBs to head.
*                 
*********************************************************************/
void scb_enque_hd (him_config *config_ptr, scb_struct *scb_pointer)
{
   him_struct *ha_ptr;
   scb_struct *scb_ptr;

   ha_ptr = config_ptr->Cfe_HaDataPtr;
   scb_ptr = scb_pointer;

   if (ha_ptr->Head_Of_Queue == NOT_DEFINED)
   {
      ha_ptr->Head_Of_Queue = ha_ptr->End_Of_Queue = scb_ptr;
      ha_ptr->AConfigPtr->Cfe_ConfigFlags &= ~DRIVER_IDLE;
      if (config_ptr->Cfe_SCSIChannel == B_CHANNEL)
    ha_ptr->BConfigPtr->Cfe_ConfigFlags &= ~DRIVER_IDLE;
   }
   else
   {
      scb_ptr->SCB_Next     = ha_ptr->Head_Of_Queue;
      ha_ptr->Head_Of_Queue = scb_ptr;
   }
   scb_ptr->SCB_Stat     = SCB_PENDING;
   scb_ptr->SCB_MgrStat  = SCB_PROCESS;
}
/*********************************************************************
*  scb_deque -
*
*  Remove SCB from queue.
*
*  Return Value:  x00 = Successful
*                 xFF = Unsuccessful (SCB is ACTIVE)
*                  
*  Parameters:    config_ptr,
*                 scb_pointer
*
*  Activation:    
*
*  Remarks:       Will not remove ACTIVE SCB
*                 Duplicates logic presently found in ???
*                 
*********************************************************************/
UBYTE scb_deque (him_config *config_ptr, scb_struct *scb_pointer)
{
   scb_struct *previous_ptr;
   him_struct *ha_ptr;

   ha_ptr = config_ptr->Cfe_HaDataPtr;
   previous_ptr = NOT_DEFINED;

   if (scb_pointer->SCB_MgrStat == SCB_ACTIVE)   /* Can't deque active SCB */
   {
      return (DQ_FAIL);
   }
   if (scb_pointer == ha_ptr->Head_Of_Queue)
   {
      ha_ptr->Head_Of_Queue = scb_pointer->SCB_Next;
   }
   else
   {
      previous_ptr           = find_chained_scb(config_ptr, scb_pointer);
      previous_ptr->SCB_Next = scb_pointer->SCB_Next;
   }
   if (scb_pointer->SCB_Next == NOT_DEFINED)
      ha_ptr->End_Of_Queue = previous_ptr;

   scb_pointer->SCB_Stat = scb_pointer->SCB_MgrStat;

   if (ha_ptr->Head_Of_Queue == NOT_DEFINED)
   {
      ha_ptr->AConfigPtr->Cfe_ConfigFlags |= DRIVER_IDLE;
      if (config_ptr->Cfe_SCSIChannel == B_CHANNEL)
    ha_ptr->BConfigPtr->Cfe_ConfigFlags |= DRIVER_IDLE;
   }
}
/*********************************************************************
*  find_chained_scb -
*
*  Scan queue for SCB with SCB_Next equal to current SCB.
*
*  Return Value:  NOT_DEFINED ptr  = Search unsuccessful, Error.
*                 Valid ptr = Ptr to SCB chained to current SCB.
*                 
*                  
*  Parameters:    config_ptr,
*                 scb_pointer
*
*  Activation:    
*
*  Remarks:       Duplicates logic presently found in scb_send
*                 
*********************************************************************/
scb_struct *find_chained_scb (him_config *config_ptr, scb_struct *scb_pointer)
{
   scb_struct *tmp_ptr;
   him_struct *ha_ptr;

   ha_ptr  = config_ptr->Cfe_HaDataPtr;
   tmp_ptr = ha_ptr->Head_Of_Queue;

   /* Scan queue for scb_pointer or End of Queue,
      whichever comes first */
   
   while ((tmp_ptr != scb_pointer) &&        
     (ha_ptr->End_Of_Queue != tmp_ptr) &&
     (tmp_ptr->SCB_Next != NOT_DEFINED))
   {
      tmp_ptr = tmp_ptr->SCB_Next;
   }
   if (tmp_ptr == scb_pointer)
   {
      return (tmp_ptr);                      /* Search Successful */
   }
   else
   {
      return (NOT_DEFINED);                  /* Search Failed */
   }
}
/*********************************************************************
*  scb_hard_reset -
*
*  Determine state of target to be reset, issue Bus Reset and clean
*  up all SCBs.
*
*  Return Value:  
*                  
*  Parameters:    config_ptr
*                 scb_pointer
*
*  Activation:    scb_non_init
*
*  Remarks:       
*                 
*********************************************************************/
void scb_hard_reset (him_config *config_ptr, scb_struct *scb_pointer)
{
   him_struct *ha_ptr;
   scb_struct *scb_ptr;
   scb_struct *tmp_ptr;
   UWORD port_addr;
   UBYTE i, 
    j = 0,
    channel,
    avail_scb = 0xFF,
    *ptr, cnt, byte_buf,
    queue[QDEPTH];
   int efl; /* -ali */

   ha_ptr = config_ptr->Cfe_HaDataPtr;
   channel = scb_pointer->SCB_Tarlun & CHANNEL;

   if (((scb_pointer->SCB_Tarlun & 0xF0) >> 4) == config_ptr->Cfe_ScsiId)
   {
      scb_bus_reset(config_ptr);

      scb_pointer->SCB_TargStat = UNIT_GOOD;
      scb_pointer->SCB_HaStat   = HOST_NO_STATUS;

      /* INTR_OFF; */
      efl = SaveAndDisable(); /* INTR_OFF; -ali */
      terminate_command(ha_ptr, scb_pointer);
      /* INTR_ON; */
      RestoreState(efl); /* INTR_ON; -ali */

      post_command(ha_ptr);
      return;
   }

   /* Clear SCB's from Queue */

   tmp_ptr = ha_ptr->Head_Of_Queue;

   while (tmp_ptr != ha_ptr->End_Of_Queue)
   {
      if ((tmp_ptr != scb_pointer) &&
    (tmp_ptr->SCB_Tarlun & 0xF0 == scb_pointer->SCB_Tarlun & 0xF0))
      {
    scb_deque(config_ptr, tmp_ptr);
      }
      tmp_ptr = tmp_ptr->SCB_Next;
   }

   /* Pause Arrow and Clear */

   PAUSE_SEQ(ha_ptr->hcntrl);
   
   for (i = 0; i < QDEPTH; i++)
   {    
      if (ha_ptr->actstat[i] == AS_SCBFREE)
      {
    avail_scb = i;
    continue;                           /* SCB Inactive */
      }

      scb_ptr = ha_ptr->actptr[i];

      if ((scb_ptr->SCB_Tarlun & 0xF0) != (scb_pointer->SCB_Tarlun & 0xF0))
    continue;                           /* SCB other SCSI ID */

      if ((scb_ptr->SCB_Tarlun & CHANNEL) != channel)
    continue;

      avail_scb = i;
      ha_ptr->free_scb++;

      OUTBYTE(ha_ptr->scbptr,i);             /* Clear Arrow SCB */
      OUTBYTE(ha_ptr->scb00,00);

      ha_ptr->actstat[i] = AS_SCBFREE;       /* Zero Active Status, most important! */
      ha_ptr->actptr[i] = NOT_DEFINED;
      scb_ptr->SCB_MgrStat = SCB_DONE_ABT;   /* Remove SCB from Queue */
      scb_deque(config_ptr, scb_ptr);
   }

   while (INBYTE(ha_ptr->qincnt))
   {
      i = INBYTE(ha_ptr->qinfifo);
      if (ha_ptr->actstat[i])
    queue[j++] = i;
   }

   for (i = 0; i < j; i++)
      OUTBYTE(ha_ptr->qinfifo,queue[i]);

   /* Setup to issue device reset */

   if (avail_scb != 0xFF)
   {
      /* Use available SCB to select device for reset/abort */
      ha_ptr->actstat[avail_scb] = AS_SCB_RST;
      --ha_ptr->free_scb;
      scb_pointer->SCB_MgrStat = SCB_ACTIVE;
      ha_ptr->actptr[avail_scb] = scb_pointer;

      j = INBYTE(ha_ptr->scbptr);
      OUTBYTE(ha_ptr->scbptr,avail_scb);
      OUTBYTE(ha_ptr->scbcnt,SCBAUTO);

      for (cnt = 0, ptr = &scb_pointer->SCB_Cntrl; cnt < SCB_LENGTH; cnt++)
      OUTBYTE (ha_ptr->scb00,*(ptr++));

      OUTBYTE(ha_ptr->scbcnt,0x00);
      OUTBYTE(ha_ptr->qinfifo,avail_scb);
      OUTBYTE(ha_ptr->scbptr,j);

      /* Setup Special Breakpoint */

      port_addr = ha_ptr->seqctl;
      byte_buf = (UBYTE) (ha_ptr->sel_cmp_brkpt);   /* Setup BP Low Address */
      OUTBYTE(port_addr + BRKADDR0, byte_buf);
   
      byte_buf = (UBYTE) ((ha_ptr->sel_cmp_brkpt >> 8) & ~BRKDIS); /* Setup BP High Address & Enable */
      OUTBYTE(port_addr + BRKADDR1, byte_buf);

      byte_buf = INBYTE(port_addr + SEQCTL) | BRKINTEN;  /* Enable BP Interrupt */
      OUTBYTE(port_addr + SEQCTL, byte_buf);

      UNPAUSE_SEQ(ha_ptr->hcntrl, ha_ptr->intstat);
      /* INTR_ON; */
   }
   return;
}
/*********************************************************************
*  
*  scb_renego -
*  
*  Reset scratch RAM to initiate or suppress sync/wide negotiation.
*
*  Return Value:  void
*             
*  Parameters: config_ptr
*              tarlun      - Target SCSI ID / Channel / LUN,
*                            same format as in SCB.
*
*  Activation: scb_special
*
*  Remarks:    
*                 
*********************************************************************/
void scb_renego (him_config *config_ptr, UBYTE tarlun)
{
   him_struct *ha_ptr;
   UBYTE option_index, scratch_index;

   ha_ptr = config_ptr->Cfe_HaDataPtr;

   option_index = scratch_index = (tarlun & TARGET_ID) >> 4;   /* Extract SCSI ID */
   if (tarlun & CHANNEL)                                       /* Array offset for channel */
      scratch_index += 8;

   if (config_ptr->Cfe_ScsiOption[option_index] & (WIDE_MODE + SYNC_MODE))   /* Write scratch RAM, renegotiate */
   {
      OUTBYTE((ha_ptr->xfer_option) + scratch_index, 0x8f);
   }
   else
   {
      OUTBYTE((ha_ptr->xfer_option) + scratch_index, 0);       /* Write scratch RAM, asynch. */
   }
   return;
}
/*********************************************************************
*  scb_send_trm_msg -
*
*  Send termination message out (Abort or Bus Device Reset) to target.
*
*  Return Value: High 3 bits - Bus Phase from SCSISIG
*                Bit 3 - Bus Free from SSTAT1
*                Bit 0 - Reqinit from SSTAT1
*
*  Parameters: term_msg - Message to send (Bus Device Reset,
*                         Abort or Abort Tag)
*
*              tgtid    - SCSI ID of target to send message to.
*
*              scsi_state - 0C : No SCSI devices connected.
*                           00 : Specified target currently connected.
*                           88 : Specified target selection in progress.
*                           40 : Other device connected.
*
*  Activation: scb_abort_active
*              scb_reset_active
*
*  Remarks:
*
*********************************************************************/
UBYTE scb_send_trm_msg ( him_config *config_ptr,
          UBYTE term_msg)
{
   him_struct *ha_ptr;
   UBYTE byte_buf = 0,
         phase = 0;

   ha_ptr = config_ptr->Cfe_HaDataPtr;

   /* Wait for req & message out */

   /* INTR_OFF; */

   while (!(INBYTE(ha_ptr->scsisig) & REQI));
   phase = (INBYTE(ha_ptr->scsisig) & BUSPHASE);

   if (phase != MOPHASE)
   {
      scb_bus_reset(config_ptr);
      return (1);
   }
   OUTBYTE(ha_ptr->clrsint1, CLRATNO);
   OUTBYTE(ha_ptr->scsisig, MOPHASE);
   OUTBYTE(ha_ptr->simode1, INBYTE(ha_ptr->simode1) & ~ENBUSFREE);
   OUTBYTE(ha_ptr->scsidatl, term_msg);      /* Send message */
   while (INBYTE(ha_ptr->scsisig) & ACKI);

   byte_buf = wt4bfree(ha_ptr);

   return (byte_buf);
}
UBYTE wt4bfree (him_struct *ha_ptr)
{
   DWORD bftimer = 100000;
   UBYTE byte_buf = 0;

   while (bftimer-- && !byte_buf)         /* Wait for Bus Free */
   {
      byte_buf = (INBYTE(ha_ptr->sstat1) & (BUSFREE | REQINIT));
   }
   if (byte_buf & BUSFREE)                /* Mask to BUSFREE XOR REQINIT */
      byte_buf &= BUSFREE;

   /* OR in Bus Phase from SCSISIG */

   byte_buf |= INBYTE(ha_ptr->scsisig) & BUSPHASE;
   return (byte_buf);
}
UBYTE scb_trm_cmplt ( him_config *config_ptr,
            UBYTE busphase, UBYTE term_msg)
{
   him_struct *ha_ptr;
   scb_struct *scb_ptr;
   UBYTE byte_buf,
    i, j = 0;

   ha_ptr = config_ptr->Cfe_HaDataPtr;
   byte_buf = busphase;

   if ((byte_buf & REQINIT) && ((byte_buf & BUSPHASE) == STPHASE))
   {
      OUTBYTE(ha_ptr->scsisig, STPHASE);
      if (INBYTE(ha_ptr->scsidatl) == UNIT_BUSY)
      {
    OUTBYTE(ha_ptr->scsisig,MIPHASE);
    while (((byte_buf = INBYTE(ha_ptr->sstat1)) & REQINIT) == 0)
    {
       if (byte_buf & (BUSFREE | SCSIRSTI))
          break;
    }
    if (byte_buf & REQINIT)
       INBYTE(ha_ptr->scsidatl);
    i = INBYTE( ha_ptr->scbptr);
    ha_ptr->actstat[i] = 0;
    scb_ptr = ha_ptr->actptr[i];
    ha_ptr->actptr[i] = NOT_DEFINED;
    scb_ptr->SCB_HaStat   = HOST_NO_STATUS;
    scb_ptr->SCB_TargStat = UNIT_BUSY;

    i = ((scb_ptr->SCB_Tarlun >> 4) | (scb_ptr->SCB_Tarlun & CHANNEL) & 0x0F);         /* External SCB */

    ++(ha_ptr->act_chtar[i]);

    i =  scb_ptr->SCB_Tarlun >> 4;
    /* INTR_OFF; */
    terminate_command( ha_ptr, scb_ptr);
    OUTBYTE(ha_ptr->seqaddr0, (UBYTE) (IDLE_LOOP_ENTRY >> 2));
    OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) (IDLE_LOOP_ENTRY >> 10));
    /* INTR_ON; */
    return (0);
      }
   }
   if (byte_buf)                          /* Bus Free, normal SCB termination */
   {
      OUTBYTE(ha_ptr->clrsint1,CLRBUSFREE);
      OUTBYTE(ha_ptr->clrint,CLRSCSINT);
      i = INBYTE( ha_ptr->scbptr);
      scb_ptr = ha_ptr->actptr[i];
      if (scb_ptr->SCB_TargStat == UNIT_BUSY)
    return (0);
      if (term_msg == MSG0C)
      {
    ha_ptr->actstat[i] = 0;
    scb_ptr->SCB_HaStat   = HOST_NO_STATUS;
    scb_ptr->SCB_TargStat = UNIT_GOOD;

/*         i =  scb_ptr->SCB_Tarlun >> 3;  */       /* External SCB */
/*         i = ((scb_ptr->SCB_Tarlun >> 4) | (scb_ptr->SCB_Tarlun & CHANNEL) & 0x0F); */   /* External SCB */
    /* ++(ha_ptr->act_chtar[i]); */
/*         i =  scb_ptr->SCB_Tarlun >> 4; */

    /* INTR_OFF; */
    terminate_command( ha_ptr, scb_ptr);
    /* INTR_ON; */

/*         if (config_ptr->Cfe_SCSIChannel != A_CHANNEL)
       i = i + 8; */
/*         if (config_ptr->Cfe_ScsiOption[i] & (WIDE_MODE + SYNC_MODE)) */

    j = (scb_ptr->SCB_Tarlun >> 4);
    if (config_ptr->Cfe_ScsiOption[j] & (WIDE_MODE + SYNC_MODE))

       OUTBYTE((ha_ptr->xfer_option) + i, 0x8f);
    else
       OUTBYTE((ha_ptr->xfer_option) + i, 0);
      }
      OUTBYTE(ha_ptr->seqaddr0, (UBYTE) (IDLE_LOOP_ENTRY >> 2));
      OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) (IDLE_LOOP_ENTRY >> 10));
      /* INTR_ON; */
      return (0);
   }
   else                                /* No Bus Free, we're hung */
   {
      scb_bus_reset(config_ptr);
      /* INTR_ON; */
      return (1);
   }
}
void scb_proc_bkpt (him_struct *ha_ptr)
{
   scb_struct *scb_ptr;
   him_config *config_ptr;
   UWORD port_addr;
   UBYTE byte_buf, i;

   if (INBYTE(ha_ptr->sblkctl) & SELBUS1)          /* Select channel */
      config_ptr = ha_ptr->BConfigPtr;
   else
      config_ptr = ha_ptr->AConfigPtr;

   GET_SCB_INDEX( i )

   if (ha_ptr->actstat[i] == AS_SCB_RST)  /* If special scb, process */
   {
      port_addr = ha_ptr->seqctl;
      OUTBYTE(port_addr + BRKADDR1, BRKDIS); /* Disable Breakpoint */

      byte_buf = INBYTE(port_addr + SEQCTL) & ~BRKINTEN;
      OUTBYTE(port_addr + SEQCTL, byte_buf); /* Disable BP Interrupt */

      scb_ptr = ha_ptr->actptr[i];
      i = scb_send_trm_msg(config_ptr, MSG0C);   /* Yes, send BDR message */
      scb_trm_cmplt(config_ptr, i, MSG0C);
   }
}
/*********************************************************************
*  scb_bus_rst -
*
*  Perform SCSI Bus Reset and clear SCB queue.
*
*  Return Value:  
*                  
*  Parameters:    
*
*  Activation:    
*
*  Remarks:       
*                 
*********************************************************************/
void  scb_bus_reset (him_config *config_ptr)
{
   UBYTE i;
   UWORD base_addr, x;

   him_struct *ha_ptr;
   ha_ptr = config_ptr->Cfe_HaDataPtr;
   base_addr = config_ptr->Cfe_PortAddress;

   /* INTR_OFF; */
   PAUSE_SEQ(ha_ptr->hcntrl);

   reset_scsi(ha_ptr);                 /* Bus Reset */

   OUTBYTE(ha_ptr->scsiseq, 0);        /* Clear interrupts */
   OUTBYTE(ha_ptr->clrsint0, 0xff);
   OUTBYTE(ha_ptr->clrsint1, 0xff);
   if (config_ptr->Cfe_ReleaseLevel == 0x01)
      OUTBYTE(ha_ptr->simode1,ENSCSIPERR | ENSCSIRST);               /* REV 'C' */
   else
      OUTBYTE(ha_ptr->simode1,ENSCSIPERR | ENSELTIMO | ENSCSIRST);   /* Later versions */
   OUTBYTE(ha_ptr->clrint, ANYINT);

   while (INBYTE(ha_ptr->qincnt))      /* Clear Q In FIFO */
   {
      i = INBYTE(ha_ptr->qinfifo);
   }

   while (INBYTE(ha_ptr->qoutcnt))     /* Clear Q Out FIFO */
   {
      i = INBYTE(ha_ptr->qoutfifo);
   }

   for (i = 0; i < QDEPTH; i++)        /* Clear Active SCBs */
   {
      OUTBYTE(ha_ptr->scbptr,i);
      OUTBYTE(ha_ptr->scb00,00);
   }

   /* Clean out Host Adapter Configuration and Data structures */

   /* ha_ptr->Head_Of_Queue = ha_ptr->End_Of_Queue = NOT_DEFINED;
   ha_ptr->done_cmd = 0;
   ha_ptr->free_scb = QDEPTH; */
   for (x = 0 ; x < 16 ; x++) ha_ptr->act_chtar[x] = 0;

   /* Restart Sequencer */

   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) (IDLE_LOOP_ENTRY >> 2));
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) (IDLE_LOOP_ENTRY >> 10));

   UNPAUSE_SEQ(ha_ptr->hcntrl, ha_ptr->intstat);
   /* INTR_ON; */
}
/*********************************************************************
*  ha_hard_rst -
*
*  Perform Hard host adapter reset.
*
*  Return Value:  
*                  
*  Parameters:    
*
*  Activation:    
*
*  Remarks:       INTR_OFF (i.e. system interrupts off) must be executed
*                 prior to calling this routine.
*                 
*********************************************************************/
void  ha_hard_reset (him_struct *ha_ptr)
{
   him_config *config_ptr;
   UWORD i, j = 0;
   UWORD base_addr, sblkctl_data;

   config_ptr = ha_ptr->AConfigPtr;
   base_addr = config_ptr->Cfe_PortAddress;

   PAUSE_SEQ(ha_ptr->hcntrl);

   OUTBYTE( ha_ptr->clrint, ANYINT);
   /* Insert "Bookmark" in queue */

   insert_bookmark(ha_ptr);

   /* A Channel */
   
   sblkctl_data = INBYTE( ha_ptr->sblkctl);
   OUTBYTE( ha_ptr->sblkctl, (INBYTE(ha_ptr->sblkctl) & ~SELBUS1));
   if (config_ptr->Cfe_ConfigFlags & RESET_BUS)
   {
      reset_scsi(ha_ptr);                          /* Bus Reset */
   }
   reset_channel(ha_ptr);
   abort_channel(ha_ptr, HOST_ABT_HA);

   if ((scb_channel_check(config_ptr->Cfe_PortAddress) == CHNLCFG_TWIN) &&
       (ha_ptr->BConfigPtr != NOT_DEFINED))
   {
      config_ptr = ha_ptr->BConfigPtr;
      OUTBYTE( ha_ptr->sblkctl, sblkctl_data | SELBUS1);
      OUTBYTE( ha_ptr->sblkctl, sblkctl_data | SELBUS1);

      if (config_ptr->Cfe_ConfigFlags & RESET_BUS)
      {
         reset_scsi(ha_ptr);                       /* Bus Reset */
      }
      reset_channel(ha_ptr);
      abort_channel(ha_ptr, HOST_ABT_HA);
   }
   OUTBYTE(ha_ptr->sblkctl, sblkctl_data);

   remove_bookmark(ha_ptr);
   
   while (ha_ptr->done_cmd)         /* Post completed commands back to host */
   {
      post_command(ha_ptr);
   }

   /* Clean out Host Adapter Configuration and Data structures */

   ha_ptr->done_cmd = 0;
   ha_ptr->free_lo = 0;
   if (config_ptr->Cfe_ConfigFlags & BIOS_ACTIVE)
   {
      j++;
   }
   for (i = 0; i < SOFT_QDEPTH ; i++)
   {
      ha_ptr->free_ptr_list[i] = (UBYTE) j++;
   }
   if (ha_ptr->HaFlags & HAFL_SWAP_ON)                  /* Optima */
   {
      ha_ptr->free_scb = SOFT_QDEPTH;
   }
   else if (ha_ptr->AConfigPtr->Cfe_ConfigFlags & BIOS_ACTIVE)
   {
      ha_ptr->free_scb = HARD_QDEPTH - 1;             /* Internal, w/BIOS */
   }
   else
   {
      ha_ptr->free_scb = HARD_QDEPTH;                 /* Internal, no BIOS */
   }

   for (i = 0 ; i < 16 ; i++)
   {
      ha_ptr->act_chtar[i] = 0;
   }

   if (ha_ptr->Head_Of_Queue == (scb_struct * ) NOT_DEFINED)
   {
      ha_ptr->AConfigPtr->Cfe_ConfigFlags |= DRIVER_IDLE;
      if (config_ptr->Cfe_SCSIChannel == B_CHANNEL)
      {
         ha_ptr->BConfigPtr->Cfe_ConfigFlags |= DRIVER_IDLE;
      }
   }

   /* Restart Sequencer */

   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) IDLE_LOOP_ENTRY >> 10);
   UNPAUSE_SEQ(ha_ptr->hcntrl, ha_ptr->intstat);

}
/*********************************************************************
*
*  scb_AsynchEvent -
*
*  Notify host of an asynchronous event.
*
*  Return Value:  0 - Notification Successful
*                 1 - Notification Unsuccessful
*                  
*  Parameters:    
*
*  Activation:    
*
*  Remarks:       
*                 
*********************************************************************/
int scb_AsynchEvent (DWORD EventType,
                    him_config *config_ptr, void *data)
{
   if ((config_ptr->Cfe_ConfigFlags & EXTD_CFG) &&
       (config_ptr->Cfe_CallBack[CALLBACK_ASYNC] != NOT_DEFINED))
   {
      config_ptr->Cfe_CallBack[CALLBACK_ASYNC]( EventType, config_ptr, data );
      return(NOERR);
   }
   return(ERR);
}
/*********************************************************************
*
*   SaveDrvrScratchRAM routine -
*
*   This routine saves the present state of the scratch RAM in the sequencer
*
*  Return Value:  0x00      - successful
*                 <nonzero> - failed
*                  
*  Parameters:    config_ptr
*
*  Activation:    initialization, scb_special
*                  
*  Remarks:                
*
*********************************************************************/
int SaveDrvrScratchRAM (him_config *config_ptr)
{
   int i;
   UWORD port_addr;
   him_struct *ha_ptr;

   ha_ptr = config_ptr->Cfe_HaDataPtr;
   port_addr = config_ptr->Cfe_PortAddress + EISA_SCRATCH1;
   for (i = 0; i < 64; ++i)
   {
      ha_ptr->DrvrScratchRAM[i] = INBYTE(port_addr + i);
   }
   return(0);
}

/*********************************************************************
*
*   SaveBIOSScratchRAM routine -
*
*   This routine saves the present state of the scratch RAM in the sequencer
*
*  Return Value:  0x00      - successful
*                 <nonzero> - failed
*                  
*  Parameters:    config_ptr
*
*  Activation:    initialization, scb_special
*                  
*  Remarks:                
*
*********************************************************************/
int SaveBIOSScratchRAM (him_config *config_ptr)
{
   int i;
   UWORD port_addr;
   him_struct *ha_ptr;

   ha_ptr = config_ptr->Cfe_HaDataPtr;
   port_addr = config_ptr->Cfe_PortAddress + EISA_SCRATCH1;
   for (i = 0; i < 64; ++i)
   {
      ha_ptr->BIOSScratchRAM[i] = INBYTE(port_addr + i);
   }
   return(0);
}
/*********************************************************************
*
*   SaveBIOSSequencer routine -
*
*   This routine saves the BIOS sequencer code
*
*  Return Value:  0x00      - successful
*                 <nonzero> - failed
*                  
*  Parameters:    config_ptr
*
*  Activation:    initialization, scb_special
*                  
*  Remarks:                
*
*********************************************************************/
int SaveBIOSSequencer (him_config *config_ptr)
{
   int i;
   UWORD port_addr;
   him_struct *ha_ptr;

   ha_ptr = config_ptr->Cfe_HaDataPtr;
   port_addr = config_ptr->Cfe_PortAddress + EISA_SEQ;

   OUTBYTE(port_addr + SEQCTL,  FAILDIS + PERRORDIS + LOADRAM);
   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) (IDLE_LOOP_ENTRY >> 2));
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) (IDLE_LOOP_ENTRY >> 10));
   for (i = 0; i < 1792; ++i)
   {
      ha_ptr->Hse_BiosSeq.InstrBytes[i] = INBYTE(port_addr + SEQRAM);
   }
/*   OUTBYTE(port_addr + SEQCTL, PERRORDIS); */
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS + SEQRESET);
   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) IDLE_LOOP_ENTRY >> 10);
   OUTBYTE(port_addr + BRKADDR1, BRKDIS);

   return(0);
}

/*********************************************************************
*
*   LoadSequencer routine -
*
*   This routine loads the sequencer for the driver/BIOS into Arrow
*
*  Return Value:  0x00      - successful
*                 <nonzero> - failed
*                  
*  Parameters:    config_ptr
*
*  Activation:    initialization, scb_special
*                  
*  Remarks:                
*
*********************************************************************/
int LoadSequencer (him_config *config_ptr, UBYTE * seqcode, int seqsize)
{
   int i;
   UWORD port_addr;
   him_struct *ha_ptr;
   UBYTE *scode;

   ha_ptr = config_ptr->Cfe_HaDataPtr;
   port_addr = config_ptr->Cfe_PortAddress + EISA_SEQ;

   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS + SEQRESET);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS + LOADRAM);
   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) (IDLE_LOOP_ENTRY >> 2));
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) (IDLE_LOOP_ENTRY >> 10));

   scode = seqcode;
   for (i = 0; i < seqsize; ++i)
   {
      OUTBYTE(port_addr + SEQRAM, *scode++ );
   }
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS + LOADRAM);
   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) (IDLE_LOOP_ENTRY >> 2));
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) (IDLE_LOOP_ENTRY >> 10));

   scode = seqcode;
   for (i = 0; i < seqsize; ++i)
   {
      if (INBYTE(port_addr + SEQRAM) != *scode++ ) {
    	return(ERR);
	}
   }
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS + SEQRESET);
   OUTBYTE(port_addr + SEQCTL, FAILDIS);

   INBYTE(port_addr + SEQCTL);
   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) IDLE_LOOP_ENTRY >> 10);
   OUTBYTE(port_addr + BRKADDR1, BRKDIS);

   /** Enable Autoflush **/
   /* OUTBYTE(ha_ptr->sblkctl, (INBYTE(ha_ptr->sblkctl) & ~AUTOFLUSHDIS)); */

   return(0);
}

/*********************************************************************
*
*  mov_ptr_to_scratch -
*
*  Move physical memory pointer to scratch RAM in Intel order ( 0, 1, 2, 3 )
*
*
*  Return Value:  VOID
*
*  Parameters:    port_addr - starting address in arrow scratch/scb area
*
*                 ptr_buf - Physical address to be moved
*
*  Activation:    scb_init_extscb
*                  
*  Remarks:       
*
*********************************************************************/
void mov_ptr_to_scratch (UWORD port_addr, DWORD ptr_buf)
{
   union outbuf
   {
     UBYTE byte_buf[4];
     DWORD mem_ptr;
   } out_array;

   out_array.mem_ptr = ptr_buf;
   OUTBYTE(port_addr,  out_array.byte_buf[0]);
   INBYTE(port_addr);

   OUTBYTE(port_addr + 1, out_array.byte_buf[1]);
   INBYTE(port_addr + 1);

   OUTBYTE(port_addr + 2, out_array.byte_buf[2]);
   INBYTE(port_addr + 2);

   OUTBYTE(port_addr + 3, out_array.byte_buf[3]);
   INBYTE(port_addr + 3);
}
/*********************************************************************
*
*   LoadBIOSScratchRAM routine -
*
*   This routine loads the scratch RAM in the sequencer
*
*  Return Value:  0x00      - successful
*                 <nonzero> - failed
*                  
*  Parameters:    config_ptr
*
*  Activation:    initialization, scb_special
*                  
*  Remarks:                
*
*********************************************************************/
int LoadBIOSScratchRAM (him_config *config_ptr)
{
   int i;
   UWORD port_addr;
   him_struct *ha_ptr;

   ha_ptr = config_ptr->Cfe_HaDataPtr;
   port_addr = config_ptr->Cfe_PortAddress + EISA_SCRATCH1;

   for (i = 20; i < 64; i++)
   {
      OUTBYTE((port_addr + i), ha_ptr->BIOSScratchRAM[i]);
      INBYTE(port_addr + i);
   }
   return(0);
}
/*********************************************************************
*
*  LoadDrvrScratchRAM routine -
*
*  This routine loads the scratch RAM in the sequencer. When setting
*  up for Optima mode, it is important to bypass the 2740 registers
*  that also determine termination power, interrupt vector, etc. To
*  do this, the values to be written to registers shared with the 
*  2740 are written to low scratch memory and copied to the correct
*  locations by using a small piece of sequencer code.
*
*  Return Value:  0x00      - successful
*                 <nonzero> - failed
*                  
*  Parameters:    config_ptr
*
*  Activation:    initialization, scb_special
*                  
*  Remarks:                
*
*********************************************************************/
int LoadDrvrScratchRAM (him_config *config_ptr)
{
   int i;
   UWORD port_addr;
   DWORD ptr_buf;
   him_struct *ha_ptr;

   ha_ptr = config_ptr->Cfe_HaDataPtr;

   /* Initialize Qout_Ptr_Array, Busy_Ptr_Array */
   
   port_addr = ha_ptr->scsiseq + NEXT_SCB_ARRAY;
   ptr_buf = (DWORD) ha_ptr->Hse_HimDataPhysaddr + 1024 + 256;
   mov_ptr_to_scratch(port_addr, ptr_buf);

   port_addr = ha_ptr->scsiseq + NEXT_SCB_ARRAY + 4;
   ptr_buf = (DWORD) ha_ptr->Hse_HimDataPhysaddr + 1024 + 512;
   mov_ptr_to_scratch(port_addr, ptr_buf);

   /* Use Arrow "high scratch" sequencer macro */

   port_addr = config_ptr->Cfe_PortAddress + EISA_SEQ;

   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS + SEQRESET);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS + LOADRAM);

   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) IDLE_LOOP_ENTRY >> 10);
   for (i = 0; i < E_scratch_code_size; i++)
      OUTBYTE(port_addr + SEQRAM, E_scratch_code[i]);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS + LOADRAM);

   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) IDLE_LOOP_ENTRY >> 10);
   for (i = 0; i < E_scratch_code_size; i++)
   {
      if (INBYTE(port_addr + SEQRAM) != E_scratch_code[i])
    return(ERR);
   }
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS + SEQRESET);

   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) IDLE_LOOP_ENTRY >> 10);
   OUTBYTE(port_addr + BRKADDR1, BRKDIS);

   /* Turn Interrupts off, we'll poll for completion */

   OUTBYTE(ha_ptr->hcntrl, (INBYTE(ha_ptr->hcntrl) & ~INTEN));

   UNPAUSE_SEQ(ha_ptr->hcntrl, ha_ptr->intstat);

   /* At this time, the sequencer moves values from low scratch
      to addresses in high scratch that are also mapped to the
      configuration chip if written by the host processor.

      Wait for PAUSE to indicate completion */

   while (!(INBYTE(ha_ptr->hcntrl) & PAUSEACK));

   OUTBYTE(ha_ptr->clrint, SEQINT);    /* Clear interrupt */
   OUTBYTE(ha_ptr->hcntrl, (INBYTE(ha_ptr->hcntrl) | INTEN));

   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS + SEQRESET);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
   INBYTE(port_addr + SEQCTL);

   port_addr = config_ptr->Cfe_PortAddress + EISA_SCRATCH1;

   for (i = 20; i < 55; i++)
   {
      OUTBYTE((port_addr + i),ha_ptr->DrvrScratchRAM[i]);
      INBYTE(port_addr + i);
   }
   ha_ptr->qout_index = 0;

   return(0);
}
/*********************************************************************
*
*   adjust_him_environment
*
*   re-adjust
*
*  Return Value:  void
*                  
*  Parameters:    config_ptr
*
*  Activation:    scb_send (called when idle is detected)
*                  
*  Remarks:                
*
*********************************************************************/
void adjust_him_environment (him_config *config_ptr)
{
   register him_struct *ha_ptr;

   ha_ptr = config_ptr->Cfe_HaDataPtr;

   if (config_ptr->Cfe_SCSIChannel)
      ha_ptr->BConfigPtr = config_ptr;
   else
      ha_ptr->AConfigPtr = config_ptr;

   scb_LoadEntryTbl( ha_ptr, ha_ptr->Hse_AccessMode );
}
/*********************************************************************
*
*  insert_bookmark
*
*  This routine inserts a "bookmark" SCB allocated in him_struct onto
*  the queue. It is used to abort all commands up to, but not including
*  the bookmark. 
*
*  Return Value:  void
*                  
*  Parameters:    ha_ptr
*
*  Activation:    intrst
*                 ha_hard_reset
*                  
*  Remarks:       INTR_OFF (i.e. system interrupts off) must be executed
*                 prior to calling this routine.
*
*********************************************************************/
void insert_bookmark (him_struct *ha_ptr)
{
   scb_struct *mark_ptr = &(ha_ptr->scb_mark);

   mark_ptr->SCB_Cmd  = BOOKMARK;
   mark_ptr->SCB_Next = NOT_DEFINED;

   if (ha_ptr->Head_Of_Queue != NOT_DEFINED)    /* Find insertion point */
   {
      ha_ptr->End_Of_Queue->SCB_Next = mark_ptr; /* Add at End of existing Q */
   }
   else
   {
      ha_ptr->Head_Of_Queue = mark_ptr;         /* Inactive Q, initialize */
   }
   ha_ptr->End_Of_Queue = mark_ptr;
   return;
}
/*********************************************************************
*
*  remove_bookmark
*
*  This routine removes a "bookmark" SCB from the queue.
*
*  Return Value:  void
*                  
*  Parameters:    ha_ptr
*
*  Activation:    intrst
*                 ha_hard_reset
*                  
*  Remarks:       INTR_OFF (i.e. system interrupts off) must be executed
*                 prior to calling this routine.
*
*********************************************************************/
void remove_bookmark (him_struct *ha_ptr)
{
   scb_struct *mark_ptr = &(ha_ptr->scb_mark);
   scb_struct *scb_ptr;

   scb_ptr = NOT_DEFINED;                    /* Remove Bookmark from queue */
   if (ha_ptr->Head_Of_Queue == mark_ptr)
   {
      ha_ptr->Head_Of_Queue = mark_ptr->SCB_Next;
   }
   else
   {
      scb_ptr = ha_ptr->Head_Of_Queue;
      while (1)
      {
         if (scb_ptr->SCB_Next == mark_ptr)
            break;
         scb_ptr = scb_ptr->SCB_Next;
      }
      scb_ptr->SCB_Next = mark_ptr->SCB_Next;
   }
   if (mark_ptr->SCB_Next == NOT_DEFINED)
   {
      ha_ptr->End_Of_Queue = scb_ptr;
   }
   return;
}
/*********************************************************************
*
*  routine - scb_channel_check
*
*  Return Value:
*             
*  Parameters:
*
*  Activation:
*                  
*  Remarks: Sequencer must be paused.
*                  
*********************************************************************/

int scb_channel_check (UWORD base_addr)
{
   int   channel_config = 0;
   UWORD sblkctl_sav,
         sblkctl = base_addr + SBLKCTL,
         scsisig = base_addr + SCSISIG;
         
   sblkctl_sav = INBYTE(sblkctl);
   OUTBYTE(sblkctl, ((sblkctl_sav | SELBUS1) & ~SELWIDE));
   INBYTE(sblkctl);
   OUTBYTE(sblkctl, ((sblkctl_sav | SELBUS1) & ~SELWIDE));

   if (INBYTE(scsisig) & CDI)                   /* Wide configuration */
   {
      if (INBYTE(base_addr + SCSISIG) & MSGI)   /* Verify single ended or differential */
         channel_config = CHNLCFG_DIFFWIDE;
      else
         channel_config = CHNLCFG_WIDE;
   }
   else                                         /* 8 bit configuration */
   {
      if ((INBYTE(scsisig) & (BSYI + MSGI)) != MSGI)
         channel_config = CHNLCFG_TWIN;
      else
         channel_config = CHNLCFG_SINGLE;
   }
   OUTBYTE(sblkctl, sblkctl_sav);               /* Restore initial setting */
   return(channel_config);
}
