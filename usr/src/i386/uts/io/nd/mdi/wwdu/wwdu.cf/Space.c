#ident "@(#)Space.c	10.1"
#ident "$Header$"              //gem
/*
 *	@(#) Space.c 7.2 94/10/26 SCOINC
 *
 *      Copyright (C) The Santa Cruz Operation, 1994.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/*
 * System V STREAMS TCP - Release 3.0
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 * All Rights Reserved.
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code.
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies.
 */

/*
 *      Copyright (C) 1991-1994 The Santa Cruz Operation, Inc.
 *      All Rights Reserved.
 *
 *      The copyright above and this notice must be preserved in all
 *      copies of this source code.  The copyright above does not
 *      evidence any actual or intended publication of this source
 *      code.
 *
 *      This is unpublished proprietary trade secret source code of
 *      The Santa Cruz Operation, Inc..  This source code may not be
 *      copied, disclosed, distributed, demonstrated or licensed except
 *      as expressly authorized by The Santa Cruz Operation, Inc..
 */

/*
 * LLI Driver for the IBM Token Ring BUSMASTER Adapter.
 * CONTENTS: the space.c file.
 */


#define WWDU_0_HWADDR           0x00,0x00,0x00,0x00,0x00,0x00
#define WWDU_1_HWADDR           0x00,0x00,0x00,0x00,0x00,0x00
#define WWDU_2_HWADDR           0x00,0x00,0x00,0x00,0x00,0x00
#define WWDU_3_HWADDR           0x00,0x00,0x00,0x00,0x00,0x00


/* PCI Datarate */
//gem2#define WWDU_0_DATARATE   8
//gem2#define WWDU_1_DATARATE   8
//gem2#define WWDU_2_DATARATE   8
//gem2#define WWDU_3_DATARATE   8

/* Token Ring Frame Size */
//gem2#define WWDU_0_FRAMESZ   4096
//gem2#define WWDU_1_FRAMESZ   4096
//gem2#define WWDU_2_FRAMESZ   4096
//gem2#define WWDU_3_FRAMESZ   4096

/* Full-Duplex operation */
//gem2#define WWDU_0_ENABLEFDX  0
//gem2#define WWDU_1_ENABLEFDX  0
//gem2#define WWDU_2_ENABLEFDX  0
//gem2#define WWDU_3_ENABLEFDX  0



//thc #define MB_WARN_TICKS  (5*60*HZ) /* ticks between "no buffers" warnings,*/
                                      /* to disable the warnings, set to 0    */


/*
 * hardware addres vector
 */
unsigned char wwduhwad[][6] = {
       {WWDU_0_HWADDR},
       {WWDU_1_HWADDR},
       {WWDU_2_HWADDR},
       {WWDU_3_HWADDR},
};



//gem2short   wwdudatarate [] = {
//gem2        WWDU_0_DATARATE,
//gem2        WWDU_1_DATARATE,
//gem2        WWDU_2_DATARATE,
//gem2        WWDU_3_DATARATE 
//gem2};

//gem2unsigned short  wwduframesz [] = {
//gem2        WWDU_0_FRAMESZ,
//gem2        WWDU_1_FRAMESZ,
//gem2        WWDU_2_FRAMESZ,
//gem2        WWDU_3_FRAMESZ
//gem2};


//gem2unsigned short  wwduenablefdx [] = {
//gem2        WWDU_0_ENABLEFDX,
//gem2        WWDU_1_ENABLEFDX,
//gem2        WWDU_2_ENABLEFDX,
//gem2        WWDU_3_ENABLEFDX
//gem2};

