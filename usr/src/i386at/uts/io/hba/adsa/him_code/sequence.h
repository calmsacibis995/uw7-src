#ident	"@(#)kern-pdi:io/hba/adsa/him_code/sequence.h	1.2"

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
 *  Header Module Name:  SEQUENCE.H
 *
 *  Version:      2.0    (Supports multiple arrays of sequencer code)
 *
 *  Description:  Master sequencer code header, includes all files containing
 *                variations of AIC-7770 sequencer code. Sequencer code arrays
 *                are labeled seq_00 to seq_15, with corresponding constant
 *                definitions SEQ_00 to SEQ_15. Each array is contained in a
 *                separate file with names seq_00.h to seq_15.h. SEQUENCE.H
 *                must include seq_00.h, other arrays may be included depending
 *                on what features are to be supported. The code in seq_00.h
 *                supports C, D & E parts in internal SCB mode ONLY! Other
 *                sequencer code is needed for swapping, wide, etc. The array
 *                seq_exist[] is used to determine which sequencer codesets
 *                are available to the HIM.
 *
 *  IMPORTANT:    seq_00[] must be large enough to hold a maximum size code             
 *                array (1792 bytes, or 0x700). This will allow the HIM to
 *                "swap" sequencer codesets for BIOS calls in swapping mode.
 *
 *  History:      8-2-93, CSF
 *
 *                02/??/93   V1.0 pilot release. (One array, seq_code[])
 *
 ****************************************************************************/


/* Include sequencer codesets, seq_00.h is mandatory!!! */

#if !defined( _EX_SEQ00_ )
#include "seq_00.h"
#endif

#if !defined( _EX_SEQ01_ )
#include "seq_01.h"
#endif

#if !defined( _EX_SEQ02_ )
#include "seq_02.h"
#endif
/*
#include "seq_03.h"
#include "seq_04.h"
#include "seq_05.h"
#include "seq_06.h"
#include "seq_07.h"
#include "seq_08.h"
#include "seq_09.h"
#include "seq_10.h"
#include "seq_11.h"
#include "seq_12.h"
#include "seq_13.h"
#include "seq_14.h"
#include "seq_15.h"
*/

/* Array indicates existence of sequencer codesets */

int E_SeqExist[16] =
{
   #if defined (SEQ_00)
   (int)   sizeof(E_Seq_00),
   #else
      0,
   #endif
   #if defined (SEQ_01)
   (int)   sizeof(E_Seq_01),
   #else
      0,
   #endif
   #if defined (SEQ_02)
   (int)   sizeof(E_Seq_02),
   #else
      0,
   #endif
   #if defined (SEQ_03)
      sizeof(E_Seq_03),
   #else
      0,
   #endif
   #if defined (SEQ_04)
      sizeof(E_Seq_04),
   #else
      0,
   #endif
   #if defined (SEQ_05)
      sizeof(E_Seq_05),
   #else
      0,
   #endif
   #if defined (SEQ_06)
      sizeof(E_Seq_06),
   #else
      0,
   #endif
   #if defined (SEQ_07)
      sizeof(E_Seq_07),
   #else
      0,
   #endif
   #if defined (SEQ_08)
      1,
   #else
      0,
   #endif
   #if defined (SEQ_09)
      1,
   #else
      0,
   #endif
   #if defined (SEQ_10)
      1,
   #else
      0,
   #endif
   #if defined (SEQ_11)
      1,
   #else
      0,
   #endif
   #if defined (SEQ_12)
      1,
   #else
      0,
   #endif
   #if defined (SEQ_13)
      1,
   #else
      0,
   #endif
   #if defined (SEQ_14)
      1,
   #else
      0,
   #endif
   #if defined (SEQ_15)
      1,
   #else
      0,
   #endif
};

