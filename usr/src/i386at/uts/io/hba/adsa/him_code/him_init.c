#ident	"@(#)kern-pdi:io/hba/adsa/him_code/him_init.c	1.4"

/* $Header$ */
/****************************************************************************
 *                                                                          *
 * Copyright 1993 Adaptec, Inc.,  All Rights Reserved.                      *
 *                                                                          *
 * This software contains the valuable trade secrets of Adaptec.  The       *
 * software is protected under copyright laws as an unpublished work of     *
 * Adaptec.  Notice is for informational purposes only and does not imply   *
 * publication.  The user of this software may make copies of the software  *
 * for use with parts manufactured by Adaptec or under license from Adaptec *
 * and for no other use.                                                    *
 *                                                                          *
 ****************************************************************************/
/****************************************************************************
 *
 *  Driver Module Name:  AIC-7770 HIM (Hardware Interface Module)
 *
 *  Version:      1.1    (Supports AIC-7770 Rev C silicon)
 *
 *  Source Code:  HIM_INIT.C  HIM.C
 *                HIM_REL.H   HIM_EQU.H   HIM_SCB.H
 *                SEQUENCE.H  SEQ_OFF.H
 *
 *  Base Code #:  549229-00
 *
 *  Description:  Hardware Interface Module for linking/compiling with
 *                software drivers supporting the AIC-7770 and AIC-7770
 *                based host adapters (ie. AHA-2740).
 *
 *  History:
 *
 *     02/??/93   V1.0 pilot release.
 *
 ****************************************************************************/

#include <io/hba/adsa/him_code/him_scb.h>
#include <io/hba/adsa/him_code/him_equ.h>
#include <io/hba/adsa/him_code/sequence.h>
#include <io/hba/adsa/him_code/seq_off.h>
#include <io/hba/adsa/him_code/seqmac.h>
#include <io/hba/adsa/him_code/him_rel.h>
#include <io/hba/adsa/him_code/vulture.h>

/****************************************************************************
*
*  Module Name:  HIM_INIT.C
*
*  Description:
*
*  Programmers:  Paul von Stamwitz
*
*  Notes:        NONE
*
*  Entry Point(s):
*
*     scb_findha     - Check port address for Host Adapter
*     scb_getconfig  - Initialize HIM data structures
*     scb_initHA     - Initialize Host Adapter
*
*  Revisions -
*
****************************************************************************/

UBYTE    scb_findha( UWORD );
void     scb_getconfig( him_config * );
UBYTE    scb_initHA( him_config * );
int      scb_init_extscb( him_config * );
int      scb_init_intscb( him_struct * );
void     scb_page_justify_hastruct( him_config * );
void     scb_write_scratch( him_config * );
int      scb_get_bios_info( UWORD, bios_info * );
void     scb_init_hastruct( him_struct * );
void     scb_calc_param( him_config * );

UBYTE Mbrstctl_req (UWORD , DWORD , UBYTE[64] );

extern   int   scb_channel_check( UWORD);
extern   void  mov_ptr_to_scratch ( UWORD, DWORD );
extern   int   LoadDrvrScratchRAM( him_config * );
extern   int   LoadBIOSScratchRAM( him_config * );
extern   int   SaveDrvrScratchRAM( him_config * );
extern   int   SaveBIOSScratchRAM( him_config * );
extern   int   SaveBIOSSequencer( him_config * );
extern   int   LoadSequencer( him_config * , UBYTE *, int);
extern   void  reset_scsi (him_struct *);
extern   void  reset_channel( him_struct * );

extern   void  scb_LoadEntryTbl( him_struct *, UWORD );

/*********************************************************************
*
*   scb_findha routine -
*
*   This routine will interrogate the hardware (if any) at the
*   specified port address to see if a supported host adapter is
*   present.
*
*  Return Value:  0x00 - no AIC-777x h.a. found
*                 0x01 - AIC-777x h.a. found, single channel
*                 0x02 - AIC-777x h.a. found, dual channel
*
*                 Bit 7 - Set if chip has not been configured
*
*  Parameters:    unsigned int base_addr -
*                 host adapter base port address
*
*  Activation:    ASPI layer, initialization.
*
*  Remarks:
*
*********************************************************************/

UBYTE scb_findha (UWORD base_addr)
{
   UWORD hcntrl_addr, bid_addr;
   UBYTE hcntrl_data, sblkctl_data;
   UBYTE num_of_ha = 0;
   UBYTE id, temp;

/*@VULTURE*/
   if (base_addr > 0x0bff)                /* EISA_RANGE? */
   {   
      hcntrl_addr = base_addr + EISA_HOST + HCNTRL;
      OUTBYTE(base_addr + EISA_HOST + BID0, 0x80);
      if (INBYTE(base_addr + EISA_HOST + BID0) == HA_ID_HI)
      {
         OUTBYTE(base_addr + EISA_HOST + BID0, 0x81);
         if (INBYTE(base_addr + EISA_HOST + BID1) == HA_ID_LO)
         {
            OUTBYTE(base_addr + EISA_HOST + BID0, 0x82);
            if (INBYTE(base_addr + EISA_HOST + BID2) == HA_PROD_HI)
            {
               OUTBYTE(base_addr + EISA_HOST + BID0, 0x83);
               id = INBYTE(base_addr + EISA_HOST + BID3) & ~BIEN;
               if (id == VL_ID || id == VL_IDA)
               {
                  ++num_of_ha;
                  hcntrl_data = INBYTE(hcntrl_addr);
                  PAUSE_SEQ(hcntrl_addr);
                  if (((sblkctl_data = INBYTE(base_addr+SBLKCTL)) & SELWIDE) == 0)
                  {
                     OUTBYTE(base_addr+SBLKCTL, sblkctl_data | SELBUS1);
                     if ((INBYTE(base_addr+SCSISIG) & (BSYI + MSGI)) != MSGI)
                     {
                        ++num_of_ha;         /* Dual channel HA */
                     }
                     OUTBYTE(base_addr+SBLKCTL, sblkctl_data);
                  }
                  OUTBYTE(hcntrl_addr, hcntrl_data);
                  OUTBYTE((base_addr + EISA_HOST + BCTL), ENABLE);
               }
            }
         }
      }
   }
/*@VULTURE*/
   if (num_of_ha == 0)
   {
      if (base_addr > 0x0bff)                   /* define ISA_RANGE           */
      {
         hcntrl_addr = base_addr + EISA_HOST + HCNTRL;
         bid_addr = base_addr + EISA_HOST + BID0;
         OUTBYTE(bid_addr, 0xff);
      }
      else
      {
         hcntrl_addr = base_addr + ISA_HOST + HCNTRL;
         bid_addr = base_addr + ISA_HOST + BID0;
      }
      if((INBYTE(bid_addr) == HA_ID_HI && INBYTE(bid_addr+1) == HA_ID_LO))
      {
         if (INBYTE(bid_addr+2) ==  HA_PROD_HI)
         {
            ++num_of_ha;
            if ((base_addr > 0xbff) &&
               ((INBYTE(base_addr + EISA_HOST + BCTL) & ENABLE) == 0))
               num_of_ha |= 0x80;

            hcntrl_data = INBYTE(hcntrl_addr);
            if ((hcntrl_data == (PAUSE | POWRDN)) && (num_of_ha & 0x80))
            {
               num_of_ha = 0;
            }
            else
            {
               PAUSE_SEQ(hcntrl_addr);
               if (((sblkctl_data = INBYTE(base_addr + SBLKCTL)) & SELWIDE) == 0)
               {
                  OUTBYTE((base_addr + SBLKCTL), sblkctl_data | SELBUS1);
                  if ((INBYTE(base_addr + SCSISIG) & (BSYI + MSGI)) != MSGI)
                  {
                     ++num_of_ha;         /* Dual channel HA */
                  }
                  OUTBYTE((base_addr + SBLKCTL), sblkctl_data);
               }
               if (((num_of_ha & 0x0F) == 1) || (num_of_ha & 0x80))
               {
                  temp = INBYTE(base_addr + EISA_SCRATCH2 + BIOS_CNTRL) & ~SC_PRI_CH_ID;
                  OUTBYTE(base_addr + EISA_SCRATCH2 + BIOS_CNTRL, temp);
               }
               OUTBYTE(hcntrl_addr, hcntrl_data);
               if ((num_of_ha & 0x80) == 0)
                  OUTBYTE((base_addr + EISA_HOST + BCTL), ENABLE);
            }
         }
      }
   }
   return(num_of_ha);
}
/*********************************************************************
*
*   scb_getconfig routine -
*
*   This routine initializes the members of the ha_Config and ha_struct
*   structures.
*
*  Return Value:  None
*                  
*  Parameters:    config_ptr
*              In:
*                 PortAddress
*                 HaDataPtr
*                 SCSIChannel
*
*              Out:
*                 ha_config structure will be initialized.
*
*  Activation:    ASPI layer, driver initialization
*                  
*  Remarks:                
*                  
*********************************************************************/
void scb_getconfig (him_config *config_ptr)
{
   him_struct *ha_ptr;
   UWORD base_addr, port_addr;
   UWORD scratch1, scratch2;
   UWORD host, array, hcntrl, scbptr, sblkctl;
   UBYTE hcntrl_data, sblkctl_data;
   UBYTE i, j, scsireg;
   UBYTE scan_type = EISA_SCAN;
   int   channel_config = 0;

   base_addr = config_ptr->Cfe_PortAddress;
   config_ptr->Cfe_ConfigFlags &= INIT_NEEDED | SAVE_SEQ | EXTD_CFG ;

   if (base_addr > 0x0bff)             /* define EISA_RANGE */
   {
      scratch1 = EISA_SCRATCH1;
      scratch2 = EISA_SCRATCH2;
      host     = EISA_HOST;
      array    = EISA_ARRAY;
      hcntrl   = base_addr + EISA_HOST + HCNTRL;
      scbptr   = base_addr + EISA_HOST + SCBPTR;
      sblkctl  = base_addr + SBLKCTL;
   }
   else                                /* define ISA_RANGE */
   {
      scratch1 = ISA_SCRATCH1;
      scratch2 = ISA_SCRATCH2;
      host     = ISA_HOST;
      array    = ISA_ARRAY;
      hcntrl   = base_addr + ISA_HOST + HCNTRL;
      scbptr   = base_addr + ISA_HOST + SCBPTR;
      sblkctl  = base_addr + SBLKCTL;
   }

/*@VULTURE*/
   if (base_addr > 0x0bff)                   /* EISA_RANGE? */
   {
     OUTBYTE(base_addr + EISA_HOST + BID0, 0x83);
     i = INBYTE(base_addr + EISA_HOST + BID3) & ~BIEN;
     if (i == VL_ID || i == VL_IDA) scan_type = VESA_SCAN;
   }
/*@VULTURE*/

   if (config_ptr->Cfe_SCSIChannel == A_CHANNEL)
   {
      j = 0;
      scsireg = INBYTE(base_addr + SCSISEQ)  |
                INBYTE(base_addr + SXFRCTL0) |
                INBYTE(base_addr + SXFRCTL1);
      if ((INBYTE(hcntrl) & CHIPRESET) || scsireg)    /* Initialize Arrow? */
      {
         config_ptr->Cfe_ConfigFlags |= INIT_NEEDED;
      }
      else
      {
         config_ptr->Cfe_ConfigFlags |= BIOS_AVAILABLE;
      }
   }
   else
   {
      j = 1;                                          /* Check A channel for initialization */
   }
   hcntrl_data = INBYTE(hcntrl);
   PAUSE_SEQ(hcntrl);

   port_addr = base_addr + array + SCB28;             /* Check SCB00 for BIOS */
   i = INBYTE(scbptr);
   OUTBYTE(scbptr, 0x00);

   if (config_ptr->Cfe_ConfigFlags & BIOS_AVAILABLE)
   {
      if (INBYTE(port_addr)    || INBYTE(port_addr + 1) ||
         INBYTE(port_addr + 2) || INBYTE(port_addr + 3))
         config_ptr->Cfe_ConfigFlags |= BIOS_ACTIVE | BIOS_AVAILABLE;
   }
   OUTBYTE(scbptr, i);

   /* Identify HW revision and record with FW version */

   sblkctl_data = INBYTE(sblkctl);
   OUTBYTE(sblkctl, config_ptr->Cfe_SCSIChannel | AUTOFLUSHDIS);
   if (INBYTE(sblkctl) == config_ptr->Cfe_SCSIChannel)
   {
      config_ptr->Cfe_ReleaseLevel = 0x01;           /* 'C' part, Rel_Level = 01 */
      config_ptr->Cfe_RevisionLevel = REV_LEVEL_1;   /* Current Rev_Level for 'C'*/
   }
   else
   {
      config_ptr->Cfe_ReleaseLevel = REL_LEVEL;      /* Current Release Level    */
      config_ptr->Cfe_RevisionLevel = REV_LEVEL;     /* Current Revision Level   */
      OUTBYTE(sblkctl, config_ptr->Cfe_SCSIChannel);
   }

   if (config_ptr->Cfe_SCSIChannel == A_CHANNEL)
   {
      scb_calc_param( config_ptr);
   }
   else
   {
      ha_ptr = config_ptr->Cfe_HaDataPtr;
      ha_ptr->BConfigPtr = config_ptr;
      config_ptr->Cfe_ScbParam.Prm_AccessMode = ha_ptr->Hse_AccessMode;
      config_ptr->Cfe_ConfigFlags |= (ha_ptr->Hse_ConfigFlags & INIT_NEEDED);
      config_ptr->Cfe_ConfigFlags |= TWO_CHNL;
   }
   /* Select B channel, 8 bit, by writing sblkctl twice */
   /* This is done to determine channel configuration   */

   channel_config = scb_channel_check(base_addr);

   switch (channel_config)
   {
      case CHNLCFG_SINGLE:

      config_ptr->Cfe_MaxTargets = 8;
      config_ptr->Cfe_ConfigFlags &= ~TWO_CHNL;
      break;

      case CHNLCFG_TWIN:

      config_ptr->Cfe_MaxTargets = 8;
      config_ptr->Cfe_ConfigFlags |= TWO_CHNL;
      break;

      case CHNLCFG_WIDE:

      config_ptr->Cfe_MaxTargets = 16;
      config_ptr->Cfe_ConfigFlags &= ~TWO_CHNL;
      config_ptr->Cfe_ConfigFlags &= ~DIFF_SCSI;
      break;

      case CHNLCFG_DIFFWIDE:

      config_ptr->Cfe_MaxTargets = 16;
      config_ptr->Cfe_ConfigFlags &= ~TWO_CHNL;
      config_ptr->Cfe_ConfigFlags |= DIFF_SCSI;
      break;

      default:
      break;

   }
   sblkctl_data = (sblkctl_data & ~SELBUS1) | config_ptr->Cfe_SCSIChannel;
   OUTBYTE(sblkctl, sblkctl_data);        /* Restore initial setting */
   /* Store Hardware ID string */
   
   /*@vulture*/
   if (scan_type == VESA_SCAN)       
   {
      OUTBYTE(base_addr + host + BID0, 0x82);
      config_ptr->Cfe_AdapterID = INBYTE(base_addr + host + BID2) << 8;
      OUTBYTE(base_addr + host + BID0, 0x83);
      config_ptr->Cfe_AdapterID |= INBYTE(base_addr + host + BID3) & ~BIEN;
   }
   /*@vulture*/

   if (scan_type == EISA_SCAN)
   {
      config_ptr->Cfe_AdapterID = INBYTE(base_addr + host + BID2) << 8;
      config_ptr->Cfe_AdapterID |= INBYTE(base_addr + host + BID3) & 0xff;
   }

   /* Setup Arrow operating registers */
   
   if (config_ptr->Cfe_ConfigFlags & INIT_NEEDED)
   {
      if (base_addr > 0x0bff)                /* define EISA_RANGE */
      {
/*@VULTURE*/
         if (scan_type == VESA_SCAN)
            e2prom(config_ptr);
/*@VULTURE*/
         port_addr = base_addr + scratch2;
         config_ptr->Cfe_IrqChannel = (INBYTE(port_addr + INTR_LEVEL) & 0x0f);
         if (INBYTE(port_addr + INTR_LEVEL) & 0x80)
            config_ptr->Cfe_ConfigFlags |= INTHIGH;
         config_ptr->Cfe_DmaChannel = 0xff;
         config_ptr->Cfe_BusRelease = INBYTE(port_addr + HOST_CONFIG) & 0x3f;
         config_ptr->Cfe_Threshold = INBYTE(port_addr + HOST_CONFIG) >> 6;

         if (config_ptr->Cfe_MaxTargets == 8)
            config_ptr->Cfe_ScsiId = INBYTE(port_addr + j + SCSI_CONFIG) & 0x07;
         else
            config_ptr->Cfe_ScsiId = INBYTE(port_addr + 1 + SCSI_CONFIG) & 0x0F;

         config_ptr->Cfe_ConfigFlags |= INBYTE(port_addr + j + SCSI_CONFIG) &
                                        (RESET_BUS + SCSI_PARITY + STIMESEL);
      }
      else                             /* Hard-coded defaults for ISA */
      {
         config_ptr->Cfe_IrqChannel = 11;       /* define IrqDEFAULT    */
         config_ptr->Cfe_DmaChannel = 5;        /* define DmaDEFAULT    */
         config_ptr->Cfe_BusOn = 11;            /* define BUSON_DEFAULT  */
         config_ptr->Cfe_BusOff = 4;            /* define BUSOFF_DEFAULT */
         config_ptr->Cfe_StrobeOn = 200;        /* define STBON_DEFAULT  */
         config_ptr->Cfe_StrobeOff = 200;       /* define STBOFF_DEFAULT */
         config_ptr->Cfe_ScsiId = 7;            /* define ScsiId_DEFAULT */
         config_ptr->Cfe_Threshold = 0;
         config_ptr->Cfe_ConfigFlags |= RESET_BUS + SCSI_PARITY
                                        + STIMESEL + INTHIGH;
      }
      /* SCSI negotiation parameters */

      if (scan_type == EISA_SCAN)
       {
         for (i = 0; i < config_ptr->Cfe_MaxTargets; i++)
         {
            config_ptr->Cfe_ScsiOption[i] = SYNC_MODE;
            if (config_ptr->Cfe_MaxTargets == 16)
            {
               config_ptr->Cfe_ScsiOption[i] |= WIDE_MODE;
            }
         }
         /* SCSI disconnect control */

         if (config_ptr->Cfe_MaxTargets == 8)
         {
            config_ptr->Cfe_AllowDscnt = 0xff;
         }
         else
         {
            config_ptr->Cfe_AllowDscnt = 0xffff;
         }
      }
   }
   else
   {
      config_ptr->Cfe_IrqChannel = INBYTE(base_addr + scratch2 + INTR_LEVEL) & 0x0F;
      config_ptr->Cfe_Threshold = (INBYTE(base_addr + host + BUSSPD) >> 6);
      if ((INBYTE(hcntrl) & IRQMS) == 0)
         config_ptr->Cfe_ConfigFlags |= INTHIGH;
      i = INBYTE(base_addr + host + BUSTIME);
      if (base_addr > 0x0bff)                /* define EISA_RANGE */
      {
         config_ptr->Cfe_BusRelease = (i & BOFF) >> 2;
         if (config_ptr->Cfe_BusRelease == 0) config_ptr->Cfe_BusRelease = 2;
      }
      else                                   /* define ISA_RANGE */
      {
         config_ptr->Cfe_DmaChannel = INBYTE(base_addr + scratch2 + DMA_CHANNEL);
         config_ptr->Cfe_BusOn = i & BON;
         config_ptr->Cfe_BusOff = (i & BOFF) >> 2;
         i = INBYTE(base_addr + host + BUSSPD);
         config_ptr->Cfe_StrobeOn = ((i & STBON) * 50) + 100;
         if (config_ptr->Cfe_StrobeOn == 450)
            config_ptr->Cfe_StrobeOn = 500;
         config_ptr->Cfe_StrobeOff = (((i & STBOFF) >> 3) * 50) + 100;
         if (config_ptr->Cfe_StrobeOff == 450)
            config_ptr->Cfe_StrobeOff = 500;
      }
      port_addr = base_addr + SCSIID;
      config_ptr->Cfe_ScsiId = (INBYTE(port_addr) & OID);
      port_addr = base_addr + SXFRCTL1;
      config_ptr->Cfe_ConfigFlags |= INBYTE(port_addr) & STIMESEL;
      /*
      WARNING! The following access of PARITY_OPTION assumes that it resides
      in the first 32-byte bank of scratch RAM. If it is ever moved to the
      high bank, the following code will work for EISA, but not for ISA.
      */
      if (INBYTE(base_addr + scratch1 + (PARITY_OPTION - EISA_SCRATCH1) + j))
         config_ptr->Cfe_ConfigFlags |= SCSI_PARITY;

      /* SCSI Negotiation parameters */
      
      port_addr = base_addr + XFER_OPTION;
      if (j)
         port_addr += 8;
      for (i = 0; i < config_ptr->Cfe_MaxTargets; i++)
      {                        
         config_ptr->Cfe_ScsiOption[i] = INBYTE(port_addr) & SXFR;

         if (config_ptr->Cfe_ScsiOption[i] > SYNCRATE_5MB)
         {
            config_ptr->Cfe_ScsiOption[i] = SYNCRATE_5MB;
         }
         else
         {
            config_ptr->Cfe_ScsiOption[i] = SYNCRATE_10MB;
         }

         if (INBYTE(port_addr) & SOFS)
            config_ptr->Cfe_ScsiOption[i] |= SYNC_MODE;
         if (INBYTE(port_addr) & WIDEXFER)
            config_ptr->Cfe_ScsiOption[i] |= WIDE_MODE;
         ++port_addr;
      }
      /*
      WARNING! The following accesses of DISCON_OPTION assumes that it resides
      in the first 32-byte bank of scratch RAM. If it is ever moved to the
      high bank, the following code will work for EISA, but not for ISA.
      */
      port_addr = base_addr + scratch1 + (DISCON_OPTION - EISA_SCRATCH1);
      if (config_ptr->Cfe_MaxTargets == 16)
      {
         config_ptr->Cfe_AllowDscnt |= (UBYTE) ~(INBYTE(port_addr));
         config_ptr->Cfe_AllowDscnt = config_ptr->Cfe_AllowDscnt << 8;
      }
      config_ptr->Cfe_AllowDscnt |= (~(INBYTE(port_addr + j)) & 0xff);
   }
   /* Primary/Secondary ID */

   port_addr = base_addr + scratch2 + BIOS_CNTRL;
   if (INBYTE(port_addr) & SC_PRI_CH_ID)
   {
      config_ptr->Cfe_ConfigFlags |= PRI_CH_ID;
   }
   else
   {
      config_ptr->Cfe_ConfigFlags &= ~PRI_CH_ID;
   }
   OUTBYTE(sblkctl, sblkctl_data);
   OUTBYTE(hcntrl, hcntrl_data);
}
/*********************************************************************
*
*   scb_initHA routine -
*
*   This routine initializes the host adapter.
*
*  Return Value:  0x00      - Initialization successful
*                 <nonzero> - Initialization failed
*                  
*  Parameters:    config_ptr
*                 h.a. config structure will be filled in
*                 upon initialization.
*
*  Activation:    Aspi layer, initialization.
*                  
*  Remarks:                
*
*********************************************************************/
UBYTE scb_initHA (him_config *config_ptr)
{
   him_struct *ha_ptr;
   UWORD port_addr, cnt, parity_addr, discon_addr, hcntrl;
   UBYTE i, sblkctl_data, temp = 0, temp1 = 0;
   int j;

   if (config_ptr->Cfe_PortAddress > 0x0bff)             /* define ISA_RANGE  */
   {
      parity_addr = config_ptr->Cfe_PortAddress + PARITY_OPTION;
      discon_addr = config_ptr->Cfe_PortAddress + DISCON_OPTION;
      port_addr   = config_ptr->Cfe_PortAddress + EISA_ARRAY + SCB28;
      hcntrl      = config_ptr->Cfe_PortAddress + EISA_HOST + HCNTRL;
   }
   else
   {
      parity_addr = config_ptr->Cfe_PortAddress + ISA_SCRATCH1 +
                     (PARITY_OPTION - EISA_SCRATCH1);
      discon_addr = config_ptr->Cfe_PortAddress + ISA_SCRATCH1 +
                     (DISCON_OPTION - EISA_SCRATCH1);
      port_addr   = config_ptr->Cfe_PortAddress + ISA_ARRAY + SCB28;
      hcntrl      = config_ptr->Cfe_PortAddress + ISA_HOST + HCNTRL;
   }
   PAUSE_SEQ(hcntrl);

   ha_ptr = config_ptr->Cfe_HaDataPtr;

   /* This is done to determine channel configuration   */

   if (scb_channel_check(config_ptr->Cfe_PortAddress) == CHNLCFG_TWIN)
      config_ptr->Cfe_ConfigFlags |= TWO_CHNL;
   else
      config_ptr->Cfe_ConfigFlags &= ~TWO_CHNL;

   if (config_ptr->Cfe_SCSIChannel == A_CHANNEL)
   {
      if (config_ptr->Cfe_ScbParam.Prm_AccessMode == 2)
      {
         temp = INBYTE(config_ptr->Cfe_PortAddress + SCSISEQ) |
                INBYTE(config_ptr->Cfe_PortAddress + SXFRCTL0) |
                INBYTE(config_ptr->Cfe_PortAddress + SXFRCTL1);
         if ((INBYTE(hcntrl) & CHIPRESET) || temp)    /* Initialize Arrow? */
         {
            config_ptr->Cfe_ConfigFlags |= BIOS_AVAILABLE;
         }
      }
      /* ja 3/17 MBRSTCTL bit required check */
    
      ha_ptr->HaFlags &= ~HAFL_MBRCTL_ON;          /* Test in default mode */
      OUTBYTE(config_ptr->Cfe_PortAddress + EISA_HOST + BUSTIME,
             (INBYTE(config_ptr->Cfe_PortAddress + EISA_HOST + BUSTIME) & 0xFE));
/*
//      if( Mbrstctl_req(config_ptr->Cfe_PortAddress) != DMA_PASS)
//      {
//         OUTBYTE(config_ptr->Cfe_PortAddress + EISA_HOST + BUSTIME,
//                (INBYTE(config_ptr->Cfe_PortAddress + EISA_HOST + BUSTIME) | 0x01));
//  
//         if (Mbrstctl_req(config_ptr->Cfe_PortAddress) == DMAER_PASS)
//         {
//            ha_ptr->HaFlags |= HAFL_MBRCTL_ON;
//         }
//         else
//         {
//            return(ERR);
//         }
//      }
*/

      if ((config_ptr->Cfe_ScbParam.Prm_AccessMode == 2) &&
         (config_ptr->Cfe_ReleaseLevel >= 2))
      {
         scb_page_justify_hastruct( config_ptr);   /* External SCB stuff */
      }
      ha_ptr = config_ptr->Cfe_HaDataPtr;
      ha_ptr->AConfigPtr = config_ptr;             /* Create link back from him_struct */
      scb_init_hastruct( ha_ptr);                  /* Initialize him_struct */
   }
   else
   {
      ha_ptr->BConfigPtr = config_ptr;
      config_ptr->Cfe_ConfigFlags |= (ha_ptr->Hse_ConfigFlags & INIT_NEEDED);
      config_ptr->Cfe_ScbParam.Prm_AccessMode = ha_ptr->Hse_AccessMode;
      if (ha_ptr->AConfigPtr->Cfe_ConfigFlags & BIOS_AVAILABLE)
         config_ptr->Cfe_ConfigFlags |= BIOS_AVAILABLE;
   }

   /*@BIOSDETECT*/
   if ((config_ptr->Cfe_ConfigFlags & BIOS_ACTIVE) && (config_ptr->Cfe_ScbParam.Prm_AccessMode != 2))
   {
      ha_ptr->free_scb = QDEPTH - 1;
      ha_ptr->actstat[0] = AS_SCB_BIOS;
   }
   else
   {
      OUTBYTE(ha_ptr->scbptr, 0x00);
      OUTBYTE(port_addr, 00);
      port_addr++;
      OUTBYTE(port_addr, 00);
      port_addr++;
      OUTBYTE(port_addr, 00);
      port_addr++;
      OUTBYTE(port_addr, 00);
   }
   if (config_ptr->Cfe_SCSIChannel == A_CHANNEL)
   {
      if ((config_ptr->Cfe_ConfigFlags & INTHIGH) == 0)
      {
         OUTBYTE(hcntrl, ((INBYTE(hcntrl) | (PAUSE | IRQMS)) & ~CHIPRESET));
      }
      else
      {
         OUTBYTE(hcntrl, ((INBYTE(hcntrl) | PAUSE & ~IRQMS) & ~CHIPRESET));
      }
      /**
      The reason that this is 1 is because in the driver I need to save the
      scratch RAM before the primary channel is initialized
      **/
      if (ha_ptr->Hse_AccessMode == 2)
      { 
         /* initialize the arrays */
         for (j = 0; j < 64; ++j)
         {
            ha_ptr->BIOSScratchRAM[j] = 0;
            ha_ptr->DrvrScratchRAM[j] = 0;
         }
         SaveBIOSScratchRAM(config_ptr);
         if ((config_ptr->Cfe_ConfigFlags & (BIOS_AVAILABLE | SAVE_SEQ)) == (BIOS_AVAILABLE | SAVE_SEQ))
         {
            temp = INBYTE(ha_ptr->seqaddr0);       /* save present location */
            temp1 = INBYTE(ha_ptr->seqaddr0 + 1);  /* save present location */
            SaveBIOSSequencer(config_ptr);
            OUTBYTE(ha_ptr->seqaddr0, temp);
            OUTBYTE(ha_ptr->seqaddr0 + 1, temp1);
         }
      } /* if */

      i = config_ptr->Cfe_Threshold << 6;
      if (config_ptr->Cfe_PortAddress > 0x0bff)    /* define EISA RANGE  */
      {
         port_addr = config_ptr->Cfe_PortAddress + EISA_HOST;
         OUTBYTE(port_addr + BCTL, ENABLE);
         if (config_ptr->Cfe_BusRelease == 2)
            OUTBYTE(port_addr + BUSTIME, 0);
         else
            OUTBYTE(port_addr + BUSTIME, config_ptr->Cfe_BusRelease << 2);
      }
      else                                         /* define ISA RANGE  */
      {
         port_addr = config_ptr->Cfe_PortAddress + ISA_HOST;
         OUTBYTE(port_addr + BUSTIME, (config_ptr->Cfe_BusOff << 2) | config_ptr->Cfe_BusOn);
         i = config_ptr->Cfe_Threshold << 6;

         if (config_ptr->Cfe_StrobeOff == 500)
            i |= 0x38;
         else
            i |= (((config_ptr->Cfe_StrobeOff / 50) - 2) << 3);

         if (config_ptr->Cfe_StrobeOn == 500)
            i |= 0x07;
         else
            i |= ((config_ptr->Cfe_StrobeOn /50) - 2);
      }
      OUTBYTE(port_addr + BUSSPD, i);
   }
   else
   {
      ++parity_addr;
      ++discon_addr;
   }
   sblkctl_data = INBYTE(ha_ptr->sblkctl);
   i = config_ptr->Cfe_SCSIChannel | (sblkctl_data & SELWIDE);
   OUTBYTE(ha_ptr->sblkctl, i);
   if (config_ptr->Cfe_ConfigFlags & INIT_NEEDED)
   {
      OUTBYTE(ha_ptr->scsiid, config_ptr->Cfe_ScsiId);
      if (config_ptr->Cfe_ConfigFlags & RESET_BUS)
         reset_scsi(ha_ptr);
      reset_channel(ha_ptr);
   }
   OUTBYTE(ha_ptr->sxfrctl1, (config_ptr->Cfe_ConfigFlags & STIMESEL) | ENSTIMER | ACTNEG);
   i = 0;
   if (config_ptr->Cfe_ConfigFlags & SCSI_PARITY)
      i = 0xff;
   OUTBYTE(parity_addr, i);
   if (config_ptr->Cfe_MaxTargets == 16)
      OUTBYTE(++parity_addr, i);
   OUTBYTE(discon_addr, ~config_ptr->Cfe_AllowDscnt);
   if (config_ptr->Cfe_MaxTargets == 16)
      OUTBYTE(++discon_addr, ~config_ptr->Cfe_AllowDscnt >> 8);

   i = 0;
   if (((config_ptr->Cfe_ConfigFlags & TWO_CHNL) == 0) ||
       (config_ptr->Cfe_SCSIChannel == B_CHANNEL))
   {                                               /* check if swapping reqd */
      if (ha_ptr->Hse_AccessMode == 2)
      {
         ha_ptr->HaFlags |= HAFL_SWAP_ON;
         sblkctl_data    &= ~AUTOFLUSHDIS;
         scb_init_extscb( config_ptr);
         scb_LoadEntryTbl( ha_ptr, SMODE_SWAP );
         if (LoadSequencer(config_ptr, (UBYTE *) &E_Seq_01, E_SeqExist[1]) != 0)
            return(ERR);
      } /* if */
      else
      {
         ha_ptr->HaFlags &= ~HAFL_SWAP_ON;
         sblkctl_data    &= ~AUTOFLUSHDIS;
         scb_LoadEntryTbl( ha_ptr, SMODE_NOSWAP );
         scb_init_intscb( ha_ptr);     /* Initialize internal specific parameters */
         if (config_ptr->Cfe_ConfigFlags & INIT_NEEDED)
         {
            if ((config_ptr->Cfe_ReleaseLevel >= 2) &&
               (E_SeqExist[2] != 0))
            {
               if (LoadSequencer(config_ptr, (UBYTE *) &E_Seq_02, E_SeqExist[2]) != 0)
                  return(ERR);
            }
            else
            {
               if (LoadSequencer(config_ptr, (UBYTE *) &E_Seq_00, E_SeqExist[0]) != 0)
                  return(ERR);
            }
         }
      }
   }
   /* Calculate device reset breakpoint */

   /* INTR_OFF; */
   port_addr = config_ptr->Cfe_PortAddress + EISA_SEQ;
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS + LOADRAM);
   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) START_LINK_CMD_ENTRY >> 2);
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) START_LINK_CMD_ENTRY >> 10);
   INBYTE(port_addr + SEQRAM);
   INBYTE(port_addr + SEQRAM);
   cnt  = (INBYTE(port_addr + SEQRAM) & 0xFF);
   cnt |= ((INBYTE(port_addr + SEQRAM) & 0x01) << 8);
   ha_ptr->sel_cmp_brkpt = cnt + 1;

   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + SEQRESET);
   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) IDLE_LOOP_ENTRY >> 10);
   OUTBYTE(port_addr + BRKADDR1, BRKDIS);
   OUTBYTE(ha_ptr->sblkctl, sblkctl_data);

   if (config_ptr->Cfe_ConfigFlags & SCSI_PARITY)
   {
      OUTBYTE(ha_ptr->sxfrctl0, (INBYTE(ha_ptr->sxfrctl0) | ENSPCHK));
   }
   OUTBYTE(ha_ptr->simode1, (INBYTE(ha_ptr->simode1) | ENSCSIRST));

   if (((config_ptr->Cfe_ConfigFlags & TWO_CHNL) == 0) ||
       (config_ptr->Cfe_SCSIChannel == B_CHANNEL))
   {
      config_ptr->Cfe_ConfigFlags |= DRIVER_IDLE;
      ha_ptr->AConfigPtr->Cfe_ConfigFlags |= DRIVER_IDLE;

      ha_ptr->g_state = WATCH_OFF;       /* ja 3/21 */
      OUTBYTE(hcntrl, (INBYTE(hcntrl) | INTEN));
      UNPAUSE_SEQ(ha_ptr->hcntrl, ha_ptr->intstat);
   }
   return(i);
}
/*********************************************************************
*
*  scb_init_hastruct routine -
*
*  
*
*  Return Value: 
*                
*                
*  Parameters:   
*                
*                
*
*  Activation:   
*                  
*  Remarks:                
*
*********************************************************************/
void scb_init_hastruct( him_struct *ha_ptr)
{
   him_config *config_ptr;
   UWORD base_addr, port_addr;
   UWORD scratch1, scratch2;
   UWORD seq, host, array;
   int   i;

   config_ptr = ha_ptr->AConfigPtr;
   base_addr = config_ptr->Cfe_PortAddress;

   if (base_addr > 0x0bff)                   /* define ISA_RANGE           */
   {
      scratch1 = EISA_SCRATCH1;
      scratch2 = EISA_SCRATCH2;
      seq      = EISA_SEQ;
      host     = EISA_HOST;
      array    = EISA_ARRAY;
   }
   else
   {
      scratch1 = ISA_SCRATCH1;
      scratch2 = ISA_SCRATCH2;
      seq      = ISA_SEQ;
      host     = ISA_HOST;
      array    = ISA_ARRAY;
   }
   port_addr = base_addr;
   ha_ptr->scsiseq  = port_addr + SCSISEQ;
   ha_ptr->sxfrctl0 = port_addr + SXFRCTL0;
   ha_ptr->sxfrctl1 = port_addr + SXFRCTL1;
   ha_ptr->scsisig  = port_addr + SCSISIG;
   ha_ptr->scsirate = port_addr + SCSIRATE;
   ha_ptr->scsiid   = port_addr + SCSIID;
   ha_ptr->scsidatl = port_addr + SCSIDATL;
   ha_ptr->clrsint0 = port_addr + CLRSINT0;
   ha_ptr->clrsint1 = port_addr + CLRSINT1;
   ha_ptr->sstat0   = port_addr + SSTAT0;
   ha_ptr->sstat1   = port_addr + SSTAT1;
   ha_ptr->simode1  = port_addr + SIMODE1;
   ha_ptr->scsibusl = port_addr + SCSIBUSL;
   ha_ptr->sblkctl  = port_addr + SBLKCTL;

   /*
   WARNING! XFER_OPTION and PASS_TO_DRIVER assignments assume that they
   reside in the first 32-byte bank of scratch RAM. If they are ever moved
   to the high bank, these assignments will work for EISA but not for ISA.
   */

   port_addr = base_addr + scratch1;
   ha_ptr->xfer_option    = port_addr + (XFER_OPTION - EISA_SCRATCH1);
   ha_ptr->pass_to_driver = port_addr + (PASS_TO_DRIVER - EISA_SCRATCH1);

   port_addr = base_addr + seq;
   ha_ptr->seqctl   = port_addr + SEQCTL;
   ha_ptr->seqaddr0 = port_addr + SEQADDR0;

   port_addr = base_addr + host;
   ha_ptr->hcntrl   = port_addr + HCNTRL;
   ha_ptr->scbptr   = port_addr + SCBPTR;
   ha_ptr->intstat  = port_addr + INTSTAT;
   ha_ptr->clrint   = port_addr + CLRINT;
   ha_ptr->scbcnt   = port_addr + SCBCNT;
   ha_ptr->qinfifo  = port_addr + QINFIFO;
   ha_ptr->qincnt   = port_addr + QINCNT;
   ha_ptr->qoutfifo = port_addr + QOUTFIFO;
   ha_ptr->qoutcnt  = port_addr + QOUTCNT;

   port_addr = base_addr + array;
   ha_ptr->scb00 = port_addr + SCB00;
   ha_ptr->scb02 = port_addr + SCB02;
   ha_ptr->scb03 = port_addr + SCB03;
   ha_ptr->scb11 = port_addr + SCB11;
   ha_ptr->scb14 = port_addr + SCB14;

   ha_ptr->Hse_ConfigFlags     = config_ptr->Cfe_ConfigFlags;
   ha_ptr->Hse_AccessMode      = config_ptr->Cfe_ScbParam.Prm_AccessMode;
   ha_ptr->free_scb            = (UBYTE) config_ptr->Cfe_ScbParam.Prm_NumberScbs;
   ha_ptr->max_nontag_cmd      = config_ptr->Cfe_ScbParam.Prm_MaxNTScbs;
   ha_ptr->max_tag_cmd         = config_ptr->Cfe_ScbParam.Prm_MaxTScbs;
   ha_ptr->Hse_HimDataSize     = (UWORD) config_ptr->Cfe_ScbParam.Prm_HimDataSize;
   ha_ptr->Hse_HimDataPhysaddr = (DWORD) config_ptr->Cfe_ScbParam.Prm_HimDataPhysaddr;

   ha_ptr->Head_Of_Queue = (scb_struct * ) NOT_DEFINED;
   ha_ptr->End_Of_Queue  = (scb_struct * ) NOT_DEFINED;

   ha_ptr->HaFlags       = 0;      
   ha_ptr->done_cmd      = 0;
   ha_ptr->idle_brkpt    = 0;
   ha_ptr->sel_cmp_brkpt = 0;

   ha_ptr->EnqueScb   = (int(*)()) NOT_DEFINED;  
   ha_ptr->ChekCond   = (void(*)()) NOT_DEFINED;  
   ha_ptr->DequeScb   = (scblink) NOT_DEFINED;  
   ha_ptr->PreemptScb = (void(*)()) NOT_DEFINED;  

   ha_ptr->free_lo = 0;
   ha_ptr->free_hi = 0;

   ha_ptr->cur_scb_ptr = 0;
   ha_ptr->qin_index   = 0;
   ha_ptr->qout_index  = 0;
   ha_ptr->active_scb  = 0;
   ha_ptr->qin_cnt     = 0;
   
   for (i = 0; i < 64; i++)
   {
      ha_ptr->BIOSScratchRAM[i] = 0;
      ha_ptr->DrvrScratchRAM[i] = 0;
      if (i < 16)
      {
         ha_ptr->act_chtar[i] = 0;
      }
   }
}
/*********************************************************************
*
*  scb_init_intscb routine -
*
*  
*
*  Return Value: 
*                
*                
*  Parameters:   
*                
*                
*
*  Activation:   
*                  
*  Remarks:                
*
*********************************************************************/
int scb_init_intscb( him_struct *ha_ptr)
{
   him_config *config_ptr;
   int i = 0, j = 0;

   config_ptr = ha_ptr->AConfigPtr;

   ha_ptr->qout_index = 0;

   for (i = 0; i < 16 ; i++)
   {
      ha_ptr->act_chtar[i] = 0;
   }

   /* scb_ptr_array, qin_ptr_array, qout_ptr_array, busy_ptr_array */

   /* QIN, QOUT, BUSY, SCB ptr tables, free ptr list */

   if (config_ptr->Cfe_ConfigFlags & BIOS_ACTIVE)
   {
      j++;
   }
   for (i = 0; i < HARD_QDEPTH ; i++)
   {
      ha_ptr->free_ptr_list[i] = j++;
   }
   ha_ptr->free_lo = 0;

   return(0);
}
/*********************************************************************
*
*  scb_calc_param routine -
*
*  
*
*  Return Value: 
*                
*                
*  Parameters:   
*                
*                
*
*  Activation:   
*                  
*  Remarks:                
*
*********************************************************************/
void   scb_calc_param ( him_config *config_ptr)
{
   /* Max Non-Tagged SCBs */
   
   if ((config_ptr->Cfe_ScbParam.Prm_MaxNTScbs < 1) ||
       (config_ptr->Cfe_ScbParam.Prm_MaxNTScbs > 2))
   {
      config_ptr->Cfe_ScbParam.Prm_MaxNTScbs = MAX_NONTAG;
   }

   /* D part, or later, with OPTIMA feature */

   if ((config_ptr->Cfe_ReleaseLevel > 1) &&
      (config_ptr->Cfe_ScbParam.Prm_AccessMode != SMODE_NOSWAP))
   {
      config_ptr->Cfe_ScbParam.Prm_AccessMode = SMODE_SWAP;

      /* Max Tagged SCBs */

      if ((config_ptr->Cfe_ScbParam.Prm_MaxTScbs == USE_DEFAULT) ||
          (config_ptr->Cfe_ScbParam.Prm_MaxTScbs > MAX_EXT_TAG))
      {
         config_ptr->Cfe_ScbParam.Prm_MaxTScbs = MAX_EXT_TAG;
      }

      /* Max Outstanding SCBs */

      if ((config_ptr->Cfe_ScbParam.Prm_NumberScbs == USE_DEFAULT) ||
          (config_ptr->Cfe_ScbParam.Prm_NumberScbs > SOFT_QDEPTH))
      {
         config_ptr->Cfe_ScbParam.Prm_NumberScbs = SOFT_QDEPTH;
      }
      /* Calculate data structure required */

      config_ptr->Cfe_ScbParam.Prm_HimDataSize = sizeof(him_struct) + HSE_PAD;

      if ((config_ptr->Cfe_ConfigFlags & SAVE_SEQ) == 0)
      {
         config_ptr->Cfe_ScbParam.Prm_HimDataSize -= SEQMAX;
      }
   }
   else                 /* C part, internal only, or D/E part intenal mode */
   {
      config_ptr->Cfe_ScbParam.Prm_AccessMode = SMODE_NOSWAP;

      /* Max Tagged SCBs */

      if ((config_ptr->Cfe_ScbParam.Prm_MaxTScbs == USE_DEFAULT) ||
          (config_ptr->Cfe_ScbParam.Prm_MaxTScbs > MAX_INT_TAG))
      {
         config_ptr->Cfe_ScbParam.Prm_MaxTScbs = MAX_INT_TAG;
      }

      /* Max Outstanding SCBs */

      if ((config_ptr->Cfe_ScbParam.Prm_NumberScbs == USE_DEFAULT) ||
          (config_ptr->Cfe_ScbParam.Prm_NumberScbs > HARD_QDEPTH))
      {
         config_ptr->Cfe_ScbParam.Prm_NumberScbs = HARD_QDEPTH;
      }
      /* Calculate data structure required */

      config_ptr->Cfe_ScbParam.Prm_HimDataSize = sizeof(him_struct) + HSE_PAD - SEQMAX;
   }
}
/*********************************************************************
*
*  scb_get_bios_info routine -
*
*  This routine retrieves information about the Arrow BIOS
*  configuration to the caller.
*
*  Return Value:  0x00 - Active BIOS, configuration info valid
*                 0xFF - No Active BIOS
*                 
*  Parameters:    base_addr - Arrow port address
*                 
*                 *bi_ptr - ptr to structure that BIOS
*                 information is copied to.
*
*  Activation:    Driver, initialization
*                  
*  Remarks:       Can be called before driver initializes Arrow,
*                 if desired.
*
*********************************************************************/
int scb_get_bios_info (UWORD base_addr, bios_info *bi_ptr)
{
   UWORD port_addr, scratch, host, array;
   UBYTE hcntrl_data, io_buf, i, j, bv_exist, bscb_addr;
   int   retval = 0;

   if (base_addr > 0x0bff)                /* Define EISA base addresses ... */
   {
      scratch = base_addr + EISA_SCRATCH2;
      host    = base_addr + EISA_HOST;
      array   = base_addr + EISA_ARRAY;
   }
   else                                   /* ... or ISA base addresses */
   {
      scratch = base_addr + ISA_SCRATCH2;
      host    = base_addr + ISA_HOST;
      array   = base_addr + ISA_ARRAY;
   }
   bi_ptr->bi_global = 0;
   bi_ptr->bi_first_drive = bi_ptr->bi_last_drive = 0xFF;
   for (i = 0; i < 8 ; i++)
   {
      bi_ptr->bi_drive_info[i] = 0xFF;
   }

   hcntrl_data = INBYTE(host + HCNTRL);   /* Pause Sequencer */
   PAUSE_SEQ(host + HCNTRL);

   i = INBYTE(host + SCBPTR);
   OUTBYTE((host + SCBPTR), 0x00);

   /* Must have BIOS loaded and active for INT13 drive info
      to be valid */

   /* Check for BIOS interrupt vector in scratch RAM,
      must be nonzero to indicate active BIOS */

   port_addr = scratch + BIOS_BASE;             
   bv_exist = INBYTE(port_addr) | INBYTE(port_addr + 1) |
              INBYTE(port_addr + 2) | INBYTE(port_addr + 3);

   /* Check for BIOS SCB address in SCB00,
      must be nonzero to indicate active BIOS */

   port_addr = array + SCB28;                   
   bscb_addr = INBYTE(port_addr) | INBYTE(port_addr + 1) |
               INBYTE(port_addr + 2) | INBYTE(port_addr + 3);

   if ((INBYTE(host + HCNTRL) & CHIPRESET) || !bv_exist || !bscb_addr)
   {
      OUTBYTE((host + SCBPTR), i);           /* BIOS inactive,         */
      OUTBYTE(host + HCNTRL, hcntrl_data);   /* Restore state and exit */

      retval = 0xFF;                           /* Parse return condition */
      return(retval);
   }
   OUTBYTE((host + SCBPTR), i);
   bi_ptr->bi_global = BI_BIOS_ACTIVE;

   port_addr = scratch + BIOS_GLOBAL;     /* Get Global BIOS parameters */
   io_buf = INBYTE(port_addr);
   if (io_buf & BIOS_GLOBAL_DOS5)
      bi_ptr->bi_global |= BI_DOS5;
   port_addr++;
   io_buf = INBYTE(port_addr);
   if (io_buf & BIOS_GLOBAL_GIG)
      bi_ptr->bi_global |= BI_GIGABYTE;

   if (io_buf & BIOS_GLOBAL_DUAL)         /* Temp. save # of channels */
      bi_ptr->bi_global |= 0x80;

   port_addr = scratch + BIOS_FIRST_LAST; /* First, Last drives */
   io_buf = INBYTE(port_addr);

   bi_ptr->bi_first_drive = io_buf & 0x0F;
   bi_ptr->bi_last_drive  = (io_buf & 0xF0) >> 4;
   port_addr = scratch + BIOS_CNTRL;
   bi_ptr->bi_bios_control = INBYTE(port_addr);

   /* Get individual drive IDs */

   port_addr = scratch + BIOS_DRIVES;
   j = bi_ptr->bi_last_drive - bi_ptr->bi_first_drive + 1;

   for (i = 0; i < j ; i++)
   {
      io_buf = INBYTE(port_addr) & 0x0F;  /* Get drive SCSI ID */
      if ((io_buf & 0x08) &&
         (bi_ptr->bi_global & 0x80))
         io_buf = (io_buf & 0x07) | 0x80; /* Set Channel B bit, if needed */
      bi_ptr->bi_drive_info[i] = io_buf;
      port_addr++;
   }
   bi_ptr->bi_global &= 0x07;             /* Clear dual channel status */

   OUTBYTE((host + HCNTRL), hcntrl_data);  /* Restore sequencer and return */
   return (0);
}
/*********************************************************************
*
*  scb_write_scratch routine -
*
*  Initialize XFER_OPTION fields in scratch RAM to 0x8F
*
*  Return Value: None
*                 
*  Parameters:   config_ptr
*
*  Activation:   ReInit in ASPI Driver, Used by NoOverlay version of
*                code for factory board tester.
*                 
*  Remarks:      Modified for 2840 & 2740 (work on tester program)
*
*********************************************************************/
void scb_write_scratch (him_config *config_ptr)
{
   him_struct *ha_ptr;
   UWORD port_addr;
   UBYTE i, byte_buf;
   UBYTE scan_type = EISA_SCAN;
   
   ha_ptr = config_ptr->Cfe_HaDataPtr;
   port_addr = config_ptr->Cfe_PortAddress;
   
   if(port_addr > 0x0bff)
   {
      OUTBYTE(port_addr + EISA_HOST + BID0, 0X83);
      i = INBYTE(port_addr + EISA_HOST + BID3) & ~BIEN;
      if(i == VL_ID || i == VL_IDA) scan_type = VESA_SCAN;
   }

   byte_buf = (INBYTE(ha_ptr->hcntrl) | PAUSE) & ~(CHIPRESET | INTEN);

   OUTBYTE(ha_ptr->hcntrl, byte_buf | CHIPRESET);   /* Reset chip, disable int */
   INBYTE(ha_ptr->hcntrl);
   INBYTE(ha_ptr->hcntrl);
   INBYTE(ha_ptr->hcntrl);
   OUTBYTE(ha_ptr->hcntrl, byte_buf);             /* Clear reset & pause */
   while (!(INBYTE(ha_ptr->hcntrl) & PAUSEACK));
   
   OUTBYTE(port_addr + EISA_HOST + BCTL, ENABLE); /* Set DMA Master Enable bit */

   OUTBYTE((port_addr + EISA_SCRATCH1 + 0x12),0);
   OUTBYTE((port_addr + EISA_SCRATCH1 + 0x13),0);   /* disconnection allowed */
   
   if (scan_type == VESA_SCAN)
   {

      OUTBYTE ((port_addr + CNTRL1_WR),0x69);   /* write vulture hardware reg    */
                                          /* bios mode 0                   */
      OUTBYTE ((port_addr + EISA_SCRATCH2 + INTR_LEVEL),0x8b);   /* set intr 11   */
    
      OUTBYTE ((port_addr + EISA_SCRATCH2 + HOST_CONFIG),0xe8);
      
      OUTBYTE((port_addr + EISA_SCRATCH2 + BIOS_CNTRL), 0);      /* Clear pri. channel select bit */
      config_ptr->Cfe_ConfigFlags = 0xe4;      /* int edge trigger high */
   }
   else
   {
      
      OUTBYTE ((port_addr + EISA_SCRATCH2 + INTR_LEVEL),0x0b);   /* set intr 11   */
    
      OUTBYTE ((port_addr + EISA_SCRATCH2 + HOST_CONFIG),0xe8);

      OUTBYTE(port_addr + EISA_SCRATCH2 + BIOS_CNTRL, 0xc1);
      config_ptr->Cfe_ConfigFlags = 0xe0;      /* int level trigger */
   }
   
   if (config_ptr->Cfe_MaxTargets == 8)
   {
      byte_buf = (config_ptr->Cfe_ScsiId & 0x07);
      byte_buf |= INIT_NEEDED | RESET_BUS | SCSI_PARITY;
      OUTBYTE(port_addr + EISA_SCRATCH2 + SCSI_CONFIG, byte_buf);
      OUTBYTE(port_addr + EISA_SCRATCH2 + SCSI_CONFIG + 1, byte_buf);
   }                                                
   else
   {
      /* Wide Initialization  (not supported now) */

   }

   port_addr = config_ptr->Cfe_PortAddress + XFER_OPTION;

   for (i = 0; i < config_ptr->Cfe_MaxTargets; i++)
   {
      OUTBYTE(port_addr++, 0x8F);
   }

   port_addr = config_ptr->Cfe_PortAddress + EISA_SCRATCH2;
   
   byte_buf = config_ptr->Cfe_IrqChannel & 0x0F;      /* Interrupt Number */
   if (config_ptr->Cfe_ConfigFlags & INTHIGH)      /* Level or Edge */
      byte_buf |= 0x80;
   OUTBYTE(port_addr, byte_buf);                   /* Set in Scratch */

   port_addr = config_ptr->Cfe_PortAddress;
   OUTBYTE(port_addr + EISA_HOST + BCTL, ENABLE);              /* Set DMA Master Enable bit */

   UNPAUSE_SEQ(ha_ptr->hcntrl, ha_ptr->intstat);
   return;
}

/*********************************************************************
*
*  scb_init_extscb routine -
*
*  Initialize external SCB array parameters in scratch RAM.
*
*  Return Value:  0x00      - Initialization successful
*                 <nonzero> - Initialization failed (sequencer problem)
*
*  Parameters:    config_ptr
*                 h.a. config structure, already initialized
*
*  Activation:    Driver, initialization
*
*  Remarks: 1) This routine was created separately from scb_initha
*              to facilitate ease of integration for the external
*              SCB implementation.
*
*           2) Due to the 2740 configuration chip memory mapping to
*              scratch locations 5A - 5F, these locations MUST be
*              initialized by the sequencer. This is accomplished by:
*
*              - Loading some special purpose code into the sequencer.
*              - Loading the values for 5A - 5F into SCB03.
*              - Starting the special purpose sequencer code.
*              - Waiting for a sequencer interrupt.
*
*              The special purpose sequencer code is then overwritten
*              when the runtime sequencer code is loaded.
*
*********************************************************************/
int scb_init_extscb (him_config *config_ptr)
{
   UWORD port_addr;
   DWORD ptr_buf;
   int i = 0;
   UBYTE hcntrl_data;
   him_struct *ha_ptr;
   ha_ptr = config_ptr->Cfe_HaDataPtr;

   ha_ptr->active_scb = ha_ptr->scsiseq + ACTIVE_SCB;

   ha_ptr->qin_cnt = ha_ptr->scsiseq + QIN_CNT;
   OUTBYTE(ha_ptr->qin_cnt, 0);
   INBYTE(ha_ptr->qin_cnt);

   ha_ptr->qout_index = 0;

   for (i = 0; i < 16 ; i++)
   {
      ha_ptr->act_chtar[i] = 0;
   }

   /* scb_ptr_array, qin_ptr_array, qout_ptr_array, busy_ptr_array */

   ha_ptr->scb_ptr_array  = &ha_ptr->scb_array[0];
   ha_ptr->qin_ptr_array  = &ha_ptr->qin_array[0];
   ha_ptr->qout_ptr_array = &ha_ptr->qout_array[0];
   ha_ptr->busy_ptr_array = &ha_ptr->busy_array[0];

   /* QIN, QOUT, BUSY, SCB ptr tables, free ptr list */

   for (i = 0; i < 256 ; i++)
   {
      ha_ptr->scb_array[i] = 0xFFFFFFFF;
      ha_ptr->qin_array[i] = 0xFF;
      ha_ptr->qout_array[i] = 0xFF;
      ha_ptr->busy_array[i] = 0xFF;
      ha_ptr->free_ptr_list[i] = i;
      ha_ptr->actstat[i] = AS_SCBFREE;
   }
   ha_ptr->free_lo = 0;
   ha_ptr->free_hi = 0xFE;

   /* Arrow scratch initialization */

   port_addr = ha_ptr->scsiseq + SCB_PTR_ARRAY;
   ptr_buf = (DWORD) ha_ptr->Hse_HimDataPhysaddr;
   mov_ptr_to_scratch(port_addr, ptr_buf);

   port_addr = ha_ptr->scsiseq + QIN_PTR_ARRAY;
   ptr_buf = (DWORD) ha_ptr->Hse_HimDataPhysaddr + 1024;
   mov_ptr_to_scratch(port_addr, ptr_buf);

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
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS + LOADRAM);

   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) IDLE_LOOP_ENTRY >> 10);
   for (i = 0; i < sizeof(E_scratch_code); i++)
      OUTBYTE(port_addr + SEQRAM, E_scratch_code[i]);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS + LOADRAM);

   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) IDLE_LOOP_ENTRY >> 10);
   for (i = 0; i < sizeof(E_scratch_code); i++)
   {
      if (INBYTE(port_addr + SEQRAM) != E_scratch_code[i])
         return(ERR);
   }
   OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
   OUTBYTE(port_addr + SEQCTL, FAILDIS + SEQRESET);

   OUTBYTE(ha_ptr->seqaddr0, (UBYTE) IDLE_LOOP_ENTRY >> 2);
   OUTBYTE(ha_ptr->seqaddr0 + 1, (UBYTE) IDLE_LOOP_ENTRY >> 10);
   OUTBYTE(port_addr + BRKADDR1, BRKDIS);

   /* Turn Interrupts off, we'll poll for completion */

   hcntrl_data = INBYTE(ha_ptr->hcntrl);            /* Save HCNTRL context */

   OUTBYTE(ha_ptr->hcntrl, hcntrl_data & ~INTEN);

   UNPAUSE_SEQ(ha_ptr->hcntrl, ha_ptr->intstat);

   /* At this time, the sequencer moves values from low scratch
      to addresses in high scratch that are also mapped to the
      configuration chip if written by the host processor.

      Wait for PAUSE to indicate completion */

   while (!(INBYTE(ha_ptr->hcntrl) & PAUSEACK));   /* Wait for completion */

   if (INBYTE(ha_ptr->intstat) & SEQINT)
   {
      OUTBYTE(ha_ptr->clrint, SEQINT);
      OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS);
      OUTBYTE(port_addr + SEQCTL, FAILDIS + PERRORDIS + SEQRESET);
      INBYTE(port_addr + SEQCTL);

      port_addr = ha_ptr->scsiseq + NEXT_SCB_ARRAY;
      for (i = 0; i < 16 ; i++)
      {
         OUTBYTE(port_addr, 0x7F);
         INBYTE(port_addr);
         port_addr++;
      }
      OUTBYTE(ha_ptr->hcntrl, hcntrl_data);            /* Restore context */
      return (0);
   }
   else
   {
      return (0xFF);
   }
}
/*********************************************************************
*
*  scb_page_justify_hastruct -
*
*  adjust ha_struct pointers to 256 byte physical boundary
*
*  Return Value:  VOID
*
*  Parameters:    config_ptr
*
*  Activation:    scb_getconfig
*                  
*  Remarks:       
*
*********************************************************************/
void scb_page_justify_hastruct (him_config *config_ptr)
{
   DWORD addr_buf, residue;
   if ((addr_buf = config_ptr->Cfe_ScbParam.Prm_HimDataPhysaddr) & 0x000000FF)
   {
      config_ptr->Cfe_ScbParam.Prm_HimDataPhysaddr = (addr_buf & 0xFFFFFF00) + 256;

      residue = 256 - (addr_buf & 0x000000FF);
      addr_buf = ((DWORD) config_ptr->Cfe_HaDataPtr) + residue;
      config_ptr->Cfe_HaDataPtr = (struct him_data_block *) addr_buf;
   }
}
/*@VULTURE*/
/*********************************************************************
*
*   e2prom routine -
*
*   This routine reads EEPROM and fill in related Arrow scratch RAM.
*   The scratch RAM locations are used in scb_getconfig.
*   This procedure replaces EISA configuration during power up (POST)
*   for Vulture.             
*
*********************************************************************/

void e2prom (him_config *config_ptr)
{
   UBYTE i,value1,value2,temp1,temp2,hcntrl_data;
   UWORD wvalue1,wvalue2;
   UWORD base_addr;

   base_addr=config_ptr->Cfe_PortAddress;
   hcntrl_data = INBYTE(base_addr + EISA_HOST + HCNTRL);
   /* pause sequencer for writing scratch ram */

   PAUSE_SEQ(base_addr + EISA_HOST + HCNTRL);

   base_addr &= 0xf000;          /* defensive */
   base_addr |= 0x0c00;
   temp2=0;
   wvalue1=read_eeprom (EE_IrqId,base_addr);
   value1=wvalue1 & 0x00ff;       /* get IrqId */
   i=(value1 & 0xf0)>>4;           /* get irq */
   if (i==9)  temp2 = 0x20;
   if (i==10) temp2 = 0x40;
   if (i==11) temp2 = 0x60;
   if (i==12) temp2 = 0x80;
   if (i==14) temp2 = 0xa0;
   if (i==15) temp2 = 0xc0;
   value2=wvalue1 >> 8;           /* get Bus Release time */
   temp1=((value1 >> 4) & 0x0f) | 0x80;    /* always edge trigger */
   OUTBYTE ((base_addr + EISA_SCRATCH2 + INTR_LEVEL),temp1);
   wvalue1 = read_eeprom(EE_CtrlBits,base_addr);
   if ((wvalue1 & 0x0020) != 0) temp2 |= 0x01;  /* attach termination bit */
   temp2 |= 0x08;          /* attach ram enable bit */
   OUTBYTE (base_addr + CNTRL1_WR,temp2);    /* write vulture hardware reg */
   temp1=(wvalue1>>2) & 0x03;       /* get threshold */
   temp1=(temp1 << 6) | value2;       /* merge with release time */
   OUTBYTE ((base_addr + EISA_SCRATCH2 + HOST_CONFIG),temp1);

   temp1=value1 & 0x0f; /* get scsi id */
   if ((wvalue1 & 0x0010) == 0x0010) temp1 |= 0x20; /* get parity bit */
   if ((wvalue1 & 0x0040) == 0x0040) temp1 |= 0x40; /* get reset  bit */
   i=(wvalue1 & 0x0003) << 3;        /* get seltmo */
   temp1 |= i;
   OUTBYTE ((base_addr + EISA_SCRATCH2 + SCSI_CONFIG),temp1);

   temp1=00;             /* select primary channel A */
                      /* for channel B, temp1=08  */

   OUTBYTE ((base_addr+EISA_SCRATCH2+BIOS_CNTRL),temp1);

   wvalue2 = 0;
   for (i=0; i<16; i++)
   {
      temp1 = (EE_Target0+i);
      wvalue1 = read_eeprom(temp1,base_addr);
      wvalue2 >>= 1;
      if(wvalue1 & 0x0010) wvalue2 |= 0x8000;   /* get disconnect bit */
      temp1 = 0;
      if(wvalue1 & 0x0008) temp1 = SYNC_MODE;   /* get mode */
      temp1 |= (wvalue1 & 0x0007) << 4;        /* merge with sync rate */
      config_ptr->Cfe_ScsiOption[i] = temp1;      /* for scb_getconfig */
      OUTBYTE (base_addr + EISA_SCRATCH1 + i, temp1); /* fill in scratch ram */
   }
   config_ptr->Cfe_AllowDscnt = wvalue2;             /* for scb_getconfig */

/* using double-negative logic in original HIM design */
   OUTBYTE((base_addr + EISA_SCRATCH1 + 0x12),~wvalue2&0xff);
   OUTBYTE((base_addr + EISA_SCRATCH1 + 0x13),(~wvalue2&0xff00)>>8);

   OUTBYTE(base_addr + EISA_HOST + HCNTRL, hcntrl_data); /* unpause sequencer */
   INBYTE(base_addr + EISA_HOST + HCNTRL);
}

/*********************************************************************
*
*   read_eeprom routine -
*
*   This routine read EEPROM NMC9346.  It is organized as 64x16.
*   Ref National Semiconductor NMC9346 data sheet.
*   Input eeprom_addr is from 0 to 1f (only 32 loc is used).
*   Input base_addr is the board address, ex. xc00, x is slot #.
*
*  Returns:
*
*   Bits 15 - 0 : Content of the EEPROM address eeprom_addr. 
*
*********************************************************************/

UWORD read_eeprom ( UBYTE eeprom_addr,UWORD base_addr)
{
   UBYTE i,temp;
   UWORD eep_data;

   eep_data = eeprom_addr|0x0180;
   wait2usec(base_addr);
   OUTBYTE ( base_addr + CNTRL0_WR, EEPROM_CS);
   wait2usec(base_addr);
   OUTBYTE ( base_addr + CNTRL0_WR, EEPROM_CS + EEPROM_SK); /* Assert chip select  */
   wait2usec(base_addr);
   OUTBYTE ( base_addr + CNTRL0_WR, EEPROM_CS);
   wait2usec(base_addr);

   for ( i=0; i < 9; i++ )                /* send address */
   {
      temp=0;
   if ((eep_data & 0x0100) == 0x0100) temp = 1;        /* get adr bit */
   OUTBYTE ((base_addr + CNTRL0_WR), temp | EEPROM_CS); /* output data first */
   wait2usec(base_addr);
   OUTBYTE ((base_addr + CNTRL0_WR), temp | EEPROM_CS | EEPROM_SK); /* then clock */
   wait2usec(base_addr);
   OUTBYTE ((base_addr + CNTRL0_WR),(temp | EEPROM_CS));
   wait2usec(base_addr);
   eep_data = eep_data << 1;                      /* update adr bit */
   }      
   /* Read EEEPROM data 16 bits */   

   for (eep_data=0, i=0; i < 16; i++ )
   {
   eep_data= eep_data << 1;
   OUTBYTE ( base_addr + CNTRL0_WR, EEPROM_CS + EEPROM_SK);
      wait2usec(base_addr);
   eep_data |=  INBYTE (base_addr + CNTRL1_RD ) & EEPROM_DO;     /* get read data bit */
   OUTBYTE ( base_addr + CNTRL0_WR, EEPROM_CS);
   wait2usec(base_addr);
   }
   OUTBYTE ( base_addr + CNTRL0_WR, EEPROM_CS + EEPROM_SK);  /* post read clean up */
   wait2usec(base_addr);
   OUTBYTE ( base_addr + CNTRL0_WR, EEPROM_CS);
   wait2usec(base_addr);

   OUTBYTE ((base_addr + CNTRL0_WR), 00);          /* Deassert all the signals */
   return (eep_data);
}

/*********************************************************************
*
*   wait2usec routine -
*
*   This routine uses Vulture timing circuit to wait 2 micro secs.
*   Since the timing is asynchronous nature (free run), two waits are
*   used to assure that we get at least 2 us    
*
*********************************************************************/

void wait2usec (UWORD base_addr)
{
   INBYTE (base_addr+CNTRL0_RD);                   /* clear timing flag  */
   while (((INBYTE (base_addr + CNTRL1_RD)) & EEPROM_TF) != 0x80); /* wait for flag to set */
   INBYTE (base_addr+CNTRL0_RD);                /* clear timing flag  */
   while (((INBYTE (base_addr + CNTRL1_RD)) & EEPROM_TF) != 0x80); /* wait for flag to set */
}
/*@VULTURE*/

/*********************************************************************
*
*  Mbrstctl_req routine  -
*
*  This routine performs a dma data transfer from system memory to Fifo.
*  next it Program I/O reads data from fifo. If match it returns 0, else
*  it returns 1. At first call to this routine, bit 0 reg BUSTIME (C85)
*  is 0 . If data does not match , bit 0 reg BUSTIME is set to 1 and this
*  routine is called for the 2nd time.
*
**********************************************************************/
  
UBYTE Mbrstctl_req (UWORD base_addr,
                    DWORD phys_addr,
                    UBYTE test_arr[64])
{
   UBYTE temp;
   int j, timeout = 10000;

   for (j = 0; j < 64; j++ )
   {
      test_arr[j] = j;
   }
   /* set up physical pointer */

   OUTBYTE(base_addr + EISA_HOST + HADDR0, (UBYTE) phys_addr );
   OUTBYTE(base_addr + EISA_HOST + HADDR1, (UBYTE) phys_addr >> 8);
   OUTBYTE(base_addr + EISA_HOST + HADDR2, (UBYTE) phys_addr >> 16);
   OUTBYTE(base_addr + EISA_HOST + HADDR3, (UBYTE) phys_addr >> 24);

   /* set up count */

   OUTBYTE(base_addr + EISA_HOST + HCNT0, (UBYTE) 0x40);
   OUTBYTE(base_addr + EISA_HOST + HCNT1, (UBYTE) 0x00);
   OUTBYTE(base_addr + EISA_HOST + HCNT2, (UBYTE) 0x00);

   /* Start transfer */

   OUTBYTE(base_addr + EISA_HOST + DFCNTRL, FIFORESET);
   OUTBYTE(base_addr + EISA_HOST + DFCNTRL, HDMAEN + DIRECTION);  /* start dma */

   while( (!(INBYTE(base_addr + EISA_HOST + DFSTATUS) & HDONE)) || (timeout--) );

   OUTBYTE(base_addr + EISA_HOST + DFCNTRL, DIRECTION);           /* stop dma */
                                 
   if (timeout == 0)                /* DMA incomplete, return */
      return (DMA_FAIL);

   for (j = 0; j < 64 ; j++)        /* read fifo by PIO */
   {
      if ((temp = INBYTE(base_addr + EISA_HOST + DFDAT)) != j)
         return (DMA_FAIL);
   }
   return (DMA_PASS);
}
