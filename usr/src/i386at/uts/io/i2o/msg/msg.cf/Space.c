#ident  "@(#)Space.c	1.4"
#ident	"$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 *
 */

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/conf.h>
#include "config.h"

/*
 *	Determines if we go into the debugger at load time or not.
 *		1 = enter the debugger
 *		0 = bypass 
 */
unsigned long I2O_MESSAGE_DEBUG = 0;

/*
 *	Variable needed to declare whether you want the PULL/PUSH model or not.
 *		1 = on, PULL/PUSH model will be in effect
 *		0 = off, PUSH/PUSH model in effect, this is the I2O model
 */
unsigned long PULL_PUSH = 0;

/*
 *	Identifies time to wait for executive messages to finish at init time.
 *		Value is in hundreths of a second.
 *		Default value is 12000 which is 120 seconds
 */
unsigned long EXECUTIVE_TIMEOUT = 12000;

/*
 *	Identifies wait time for an LCT_NOTIFY message to finish at init time.
 *		Value is in hundreths of a second.
 *		Default value is 6000 which is 60 seconds
 */
unsigned long LCT_TIMEOUT = 6000;

/*
 *	Identifies time to wait for an valid message buffer on the request 
 *      free FIFO at init time.
 *		Value is in hundreths of a second.
 *		Default value is 300 which is 3 seconds
 */
unsigned long BUFFER_GET_TIMEOUT = 300;

/*
 *	Number of outbound buffers allocated for the reply queue.
 *		Default value is 100
 */
unsigned long OUTBOUND_BUFFER_NUMBER = 100;

/*
 *	Number of inbound buffers allocated for the request queue.
 *        **note** This is PULL/PUSH mode only, the IOP allocates request 
 *                 buffers in PUSH/PUSH mode.
 *		Default value is 100
 */
unsigned long INBOUND_BUFFER_NUMBER = 100;

/*
 *	Size of outbound message frames in 32 bit word increments.
 *		Default value is 32, 32 * 4 = 128 bytes
 */
unsigned long OUTBOUND_FRAME_SIZE = 32;

/*
 *	Value stamped into in-core resmgr for the IPL value of any transports
 *	discovered during boot. This is only used if the message layer stamps
 *	the MODNAME and discovers that the IPL is also missing.
 */

unsigned long ISL_IPL = 5; 



