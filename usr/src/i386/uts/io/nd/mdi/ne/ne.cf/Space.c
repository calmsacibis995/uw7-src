#ident "@(#)Space.c	5.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/* Define the # of RX and TX descriptors per board */
#define NE3200_RX_DESCS_PER_BOARD	16
#define NE3200_TX_DESCS_PER_BOARD	16

int ne_numRX = NE3200_RX_DESCS_PER_BOARD;     /* # of RX descs per board */
int ne_numTX = NE3200_TX_DESCS_PER_BOARD;     /* # of TX descs per board */
