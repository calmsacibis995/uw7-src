/*      Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.      */
/*      Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.        */
/*        All Rights Reserved   */

/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.      */
/*      The copyright notice above does not evidence any        */
/*      actual or intended publication of such source code.     */


/*
 * This file contains the code for the functionality specific to the
 * Media Vision Pro Audio Spectrum 16 card.
 */

#ident	"@(#)kern-pdi:io/hba/zl5380/mvpas16.c	1.1"

#ifdef  _KERNEL_HEADERS

#include <util/types.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include "zl5380.h"
#include "mvpas16.h"
#include <io/ddi.h>

#elif defined(_KERNEL)

#include <sys/types.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include "zl5380.h"
#include "mvpas16.h"
#include <sys/ddi.h>

#endif  /* _KERNEL_HEADERS */

/*
 * Local Functions
 */

int mv_pas16adapter(int base);
void mv_pas16initialize(zl5380ha_t *zl5380ha);

/*
 * mv_pas16initialize(zl5380ha_t *zl5380ha)
 *
 * Description:
 *	Initializes the MV PAS16 config. registers
 *
 * Returns:
 * 	None
 */
void
mv_pas16initialize(zl5380ha_t *zl5380ha)
{	
        unsigned char temp;
	int	zl5380base;
        int mvpas16_irq_table[MV_PAS16_IRQ_TABLE_LENGTH] =
        { 0,  0,  1,  2,  3,  4,  5,  6, 0,  0,  7,  8,  9,  0, 10, 11 };

	zl5380base = zl5380ha->ha_base + ZL5380_IO_ADDRESS_OFFSET;

        CSDR = zl5380base;
        ODR = zl5380base;
        ICR = zl5380base + 0x01;
        MR = zl5380base + 0x02;
        TCR = zl5380base + 0x03;
        CSCR = zl5380base + 0x2000;
        SER = zl5380base + 0x2000;
        BSR = zl5380base + 0x2001;
        SDSR = zl5380base + 0x2001;
        IDR = zl5380base + 0x2002;
        SDTR = zl5380base + 0x2002;
        RPIR = zl5380base + 0x2003;
        SDIR = zl5380base + 0x2003;

        (void) inb (RPIR);      /* Clear Interrupts     */

        /* Configure and Enable the interrupt on the card       */
        temp = inb (zl5380ha->ha_base + MV_IO_CONFIGURATION_REGISTER_3);
        temp = (temp & MV_IO_CONFIGURATION_REGISTER_3_MASK) |
                (mvpas16_irq_table[zl5380ha->ha_vect] << 
			MV_PAS16_IRQ_SHIFT_COUNT);
        outb (zl5380ha->ha_base + MV_IO_CONFIGURATION_REGISTER_3, temp);
        outb (zl5380ha->ha_base + MV_SYSTEM_CONFIGURATION_REGISTER_4,
                MV_SYSTEM_CONFIGURATION_REGISTER_4_DATA);

	return;
}

/*
 * mv_pas16adapter(int baseaddress)
 *
 * Description:  
 *	Checks if a MV PAS16 card is present at the given address
 *
 * Returns: 1 if it is a MV PAS16 card, else 0
 */

int
mv_pas16adapter(int baseaddress)
{
        unsigned int    temp;
        unsigned char   board_id;

        /* Write board id and I/O base address into Master Decode Register */
        temp = baseaddress + ZL5380_IO_ADDRESS_OFFSET;
        switch (baseaddress) {

        case BASE_ADDR_1:       board_id = BOARD_ID_1;
                                break;

        case BASE_ADDR_2:       board_id = BOARD_ID_2;
                                break;

        case BASE_ADDR_3:       board_id = BOARD_ID_3;
                                break;

        case BASE_ADDR_4:       board_id = BOARD_ID_4;
                                break;

        default         :       return 0;

        }

        outb (MV_MASTER_DECODE_REGISTER, board_id);
        outb (MV_MASTER_DECODE_REGISTER, baseaddress >>
                MV_MASTER_DECODE_REGISTER_SHIFT_COUNT);

        return(((inb (temp) == 0)		/* CSDR */
                && (inb (temp + 0x02 )  == 0)	/* MR   */
                && (inb (temp + 0x03)  == 0))); /* TCR  */

}


