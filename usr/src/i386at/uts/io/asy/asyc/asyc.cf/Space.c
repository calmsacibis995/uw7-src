/*	Copyright (c) 1997 Santa Cruz Operation Ltd. All Rights Reserved. */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF SCO Ltd.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)Space.c	1.26"
#ident 	"$Header$"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/stream.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/termio.h>
#include <sys/termiox.h>
#include <sys/asyc.h>
#include <sys/inline.h>
#include <sys/cmn_err.h>
#include <sys/stropts.h>
#include <sys/strtty.h>
#include <sys/debug.h>
#include <sys/asyc.h> 
#include <sys/eucioctl.h>
#include <sys/ddi.h>
#include "config.h"

/*
 * Fix for itimeout(D3) PANIC
 */

int asyc_use_itimeout = 0;

/*
 * asyc_debug 
 * Functionality under production code is undefined.
 * NB. DON'T SET ANY BITS IN THE TOP NIBBLE : THE DEBUG DRIVER
 * GENERATES VARIOUS DEBUG EVENTS FOR THESE BITS.
 */

int	asyc_debug = 0; 

/* 
 * Tuneable driver configuration parameters.
 *
 * asyc_pollfreq (usecs) : period of asycpoll invocation.
 * If read data is lost due to ibuf overruns, decrease this time.
 * If low data rate, increase period to reduce system load.
 */

int asyc_pollfreq = 20000 ; 

/* 
 * Flow control low/high water points : units % of input buffer size 
 *
 * For Buffer Free > High Water -> Assert Flow Control 
 * For Buffer Free < Low Water -> Disassert FLow control 
 * 
 * The values should both be in the range 10% - 90% and the low 
 * water mark should be less than the high water mark, else default 
 * values will be selected.
 */

uint_t asyc_iblowat = 65 ; 
uint_t asyc_ibhiwat = 75 ; 

/*
 * ISR Output buffer hysteresis, % of buffer size.
 *
 * Determine whether to reload the ISR obuf[] with STREAMS data at 
 * each asycpoll() invocation (each asyc_pollfreq).
 *
 */

uint_t	asyc_obhiwat = 90 ;
uint_t	asyc_oblowat = 60 ;


/* 
 * MAX_SCAN_DEVS / asyc_nscdevs
 * Number of devices to scan for. This sets the default devices which can 
 * be specified as a console device without using the IO address override. 
 * The order of entries in the uart_scantbl array determines the console 
 * device (see boot(4) and Sassign(4)) mapping from IO address to minor 
 * number (eg. 1st entry in uart_scantbl[] is device 0, minors 0 to 3 incl.
 * 
 * Note that some PCs may have non UART hardware at the addresses used by 
 * COM3 and COM4 and thus the driver only searches for COM1 and COM2 UARTs
 * by default. Further devices can be set in the dcu(1M) after installation
 * and these can be used as serial ports with no changes to this file. If a
 * port is to be used as a console device it should be added to the scan 
 * table. 
 */ 

#define MAX_SCAN_DEVS 2		/* Valid entries in uart_scantbl[]	*/

int asyc_nscdevs = MAX_SCAN_DEVS; 

/* 
 * unsigned int asyc_sminor 
 * Standard serial ports base minor number. This can be adjusted to 
 * compensate for other serial driver hardware allocating a range
 * of minor numbers which conflict with the asyc(7) driver's default minors. 
 * (The asyc driver creates 4 minor nodes for each standard PC serial port.
 * numbered consecutively from <asyc_sminor>. Eg. COM1 is usually assigned 
 * minors 0 - 3, with <asyc_sminor> set to 0. 
 */

uint_t	 asyc_sminor = 0;		/* Starting minor number */

/* 
 * uart_scantbl[] 
 * Holds the data used to make the scan for UART devices, prior to the 
 * dcu(1M) information being available. Needed to assign minor/unit 
 * mapping for the console device specification.
 * 
 * NB. The table may contain any number of entries: only the first 
 * MAX_SCAN_DEVS entries will be used.
 * 
 * NB. For non-standard configurations the driver may overwrite the in-core
 * entry with bootstring specified (boot(4)) console setup data.
 * 
 * NB. The user can setup some of the fields in the array below to override
 * default values (eg. initial serial line data transfer parameters like 
 * Baud rate, number of stop bits) or to force the driver to use a UART 
 * identity (eg. the UART type field (set to NOTYPE) is usually set by the 
 * driver from the results of various tests: it can be forced to treat the 
 * UART as (say) a NS16550 UART by setting the field to NS16550). 
 * 
 * NB. Since there is only 1 termio[sx] entry per device and 4 nodes per
 * device the mode parameters should not set any modes that are not applicable
 * to all device node semantics, ie. the XXt node sets the CLOCAL bit in 
 * t_cflag, whereas XXs, XXh and XXm nodes do not. All node semantic 
 * parameters (ie. the modes that set the behaviour that identify the node 
 * type, usually modem/terminal and flow control related) are overwritten 
 * on opening. Hence setting CLOCAL for port 1 will not effect the actual 
 * setting of the mode when opening tty00s. 
 * Node identity overrides are used for 
 *  t_cflag		CLOCAL, CREAD, HUPCL 
 *	t_iflag		IXON, IXOFF 
 *	x_hflag		RTSXOFF, CTSXON
 * 
 * Also, if default termio[sx] settings are used they should be complete, 
 * the driver will not attempt to fill in missing fields. So if a port is 
 * set with IGNBRK on, the driver will not provide a default baud rate etc.
 * Tunables should set Baud rate, Data size, Stop bits as a minimum. 
 *
 */ 

typedef struct termios termios_t; 
typedef struct termiox termiox_t; 

/*
 * Set the desired initial parameter set in the ttab/xtab tables.
 * Set the device entry in uart_scantbl to the desired ttab/xtab 
 * address. 
 * Distribution has both the ports set to the standard default flags 
 * and parameters (96,N,8,1, XON/XOFF, IGNBRK)
 */


struct termios ttab[] = { 

/* 
 * { c_iflag, c_oflag, c_cflag, c_lflag, c_cc[] }
 * { InputMode, OutputMode, ControlMode, LocalMode, ControlCharsTable } 
 * Field definitions are in /usr/include/sys/termios.h
 */ 

/* ttab[0]	*/
{ IXON|IXOFF|IGNPAR|IGNBRK, 0, CREAD|CS8|B9600|HUPCL, 0, {0} },

/* ttab[1]	*/
{ IGNPAR|IGNBRK, 0, CREAD|CS8|B38400|HUPCL, 0, {0} },

/* ttab[2]	*/
{ IXON|IXOFF|IGNPAR|IGNBRK, 0, CREAD|CS7|B9600|HUPCL|PARENB|CLOCAL, 0, {0} },

}; 

struct termiox xtab[] = { 

/*
 * { x_hflag, x_cflag, x_rflag[], x_sflag } 
 * { HardwareFlowControlMode, ClockMode, SpareTable, SpareFlag
 */

/* xtab[0]	*/
{ CTSXON|RTSXOFF , 0, {0} , 0 } ,  

}; 

/* 
 * If a device is to be used as a console/debugger device it should 
 * appear in this table.
 */

uart_data_t	uart_scantbl[] = { 
/* { IO, IRQ, NOTYPE, NOSTATE, RFTL, TFTL, INIT , INITX , CP }, */
{ 0x3f8, 4, NOTYPE, NOSTATE, 8, 16,(struct termios *)0, (struct termiox *)0, 0},
{ 0x2f8, 3, NOTYPE, NOSTATE, 8, 16,(struct termios *)0, (struct termiox *)0, 0},
{ 0x3e8, 5, NOTYPE, NOSTATE, 0, 0, (struct termios *)0, (struct termiox *)0, 0},
{ 0x2e8, 9, NOTYPE, NOSTATE, 0, 0, (struct termios *)0, (struct termiox *)0, 0},
}; 

/* 
 * uart_scantbl[] entries are of the form: 
 * { IO, IRQ, UART, US, RF, TF, IT, IX }
 * where 
 * 
 * IO = UART base IO location. Standard PC values are:
 * 	COM1	0x3f8  
 * 	COM2	0x2f8  
 * 	COM3	0x3e8  
 * 	COM4	0x2e8  
 * 
 * IRQ = UART Interrupt line. Standard PC values are:
 * 
 * 	COM1	IRQ4
 * 	COM2	IRQ3
 * 	COM3	No standard 	
 * 	COM4	No standard 
 * 
 * UART = UART device type. The asyc(7) driver requires that a UART be 
 *	  compatible with the National Semiconductor 8250. All standard 
 *	  PC UARTs are compatible, but some use more modern devices with 
 *	  extra features that the driver can take advantage of, to improve
 *	  data trasnfer rate and system load. The devices that the asyc(7) 
 *	  driver knows about are:
 * 
 * 	NS8250	: NS 8250 (and compatible) - Standard UART
 * 	NS16450	: NS 16450 (and compatible) - Faster standard UART 
 *	NS16550 : NS 16550 (and compatible) - 16450 + 16B buffers
 *	NS16650 : NS 16650 (and compatible) - 16450 + 32B buffers + AutoFlowCtl
 * 	TL16750 : TI 16750 (and compatible) - 16450 + 64B buffers + AutoFlowCtl
 *	NOTYPE	: Driver determines type.
 * 
 * RF - Rx FIFO trigger level to set (UART HW dependent). The higher values
 * 		allowed by the H/W are more efficient (lower CPU load) but have 
 *		a greater risk of losing data. The converse applies.
 * 
 * TF - Tx FIFO fill level (UART HW dependent). This is the number of bytes
 *		that the driver loads into the Tx FIFO: these bytes should be sent 
 *		back to back (small intercharacter delay). Higher values are more
 *		efficient if the receiver can keep pace. 
 * 
 * IT - Pointer to initial termios settings structure
 *		(Loads t_cflag, t_iflag, t_oflag and t_cc[] settings)
 *		Sets all terminal processing options not miplicitly specified by 
 *		the node minor. 
 * 
 * IX - Pointer to initial termiox settings structure
 *		Sets all terminal processing options not miplicitly specified by 
 *		the node minor. 
 *
 * Compatibility mode
 * 	Sets various operation options. 
 *
 *		SCOMODEM 	: 	Treats DCD input in MODEM ports as SCO OSr5, ODT3.0 
 *						ie. ignores DCD state after open(2) completes. The 
 *						open(2) waits until DCD is set.
 *
 *		TIOCMDFC	:	Issuing TIOCMSET, TIOCM_BIC or TIOCM_BIS ioctl(2)s 
 *						will disable input hardware flow control (if set). 
 *						Since these ioctl(2)s allow user setting of the 
 *						input flow control lines, cannot guarantee control
 *						state. Clearing flags allows TCGETX to verify state.
 *
 */ 

/*
 * Limit size of byte copy from STREAMS buffers to ISR output buffer. 
 * This occurs at CLI priority so may need to be limited if locks out 
 * IRQs for too long.
 */

uint_t		asyc_rldcnt = 200; 

