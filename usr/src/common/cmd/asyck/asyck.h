#ifndef _IO_ASY_ASYC_ASYC_H	/* wrapper symbol for kernel use */
#define _IO_ASY_ASYC_ASYC_H	/* subject to change without notice */

#ident	"@(#)asyck.h	1.2"
#ident	"$Header$"

/* 
 * Public definitions, types and prototypes, for the serial port 
 * driver "asyc". Included and used in space.c 
 */

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */


#endif /* _KERNEL_HEADERS */

#ifdef MERGE386 
#ifdef _KERNEL_HEADERS

#include <util/merge/merge386.h>/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/merge386.h>	/* REQUIRED */

#endif
#endif

/* 
 * Public types 
 * Used in ioctl(2) arguments, space.c tunables and as program constants.
 * 
 */ 

typedef ushort_t	ioaddr_t; 


/* 
 * UART type: UART chipset version. 
 */

typedef 
enum { NOTYPE=0, NS8250, NS16450, NS16550, ASY16650, ASY16750 }
uart_type_t; 

/* 
 * UART state: 
 *
 * HWRESET:	Uninitialised (reset) state after POST.
 * INIT:	Set by asyc driver to known (9600,N,8,1) state.
 * SYSCON:	Used by system for console (cmn_err(3C)) IO.
 * KDBCON:	Used by kdb(1M) and other (kernel) debuggers for IO.
 * TTYINIT:	Available as general serial device (for ttymon(1) etc.).
 * HWFAILED:	UART failed tests/failed in use: present but unusable.
 * 
 */

typedef ushort_t uart_state_t; 
 
#define NOSTATE 0x0000	/* state/presence unknown					*/
#define HWRESET 0x0001	/* present, does ICR echo (Presence 1)		*/
#define INIT 	0x0002	/* passed all presence tests				*/
#define SYSCON 	0x0004	/* used as system console (bootstring)		*/
#define KDBCON 	0x0008	/* used as kdb(1M) console	(via newterm)	*/
#define TTYDEV	0x0010	/* used as STREAMS terminal device			*/
#define HWERR 	0x0020	/* failed data loopback, not usable at all	*/
#define RSRCERR	0x0040 	/* resources failure - not usable terminal	*/
#define ABSENT 	0x8000	/* Device not present at address specified	*/

/* 
 * Space.c definitions
 *
 * UART data: define operating parameters for UART. 
 * UART tune: tune features per UART
 * 
 */

/* 
 * conspec_t 
 * Define the serial protocol to use with a serial console device.
 * IO specifies the port to use, bauddiv is the (macro generated)
 * clock divisor and ctldata is the (macro  generated) data word 
 * specification (data bits, stop bits, parity).
 */

typedef 
struct _conspec { 
	ioaddr_t	iobase; 
	ushort_t	bauddiv; 
	uchar_t		ctldata; 
	/* Private console data */
	uchar_t		syscon_icr; 
} conspec_t; 

/* 
 * uart_data_t 
 * Used in space.c tio allow tunable override of the initial control 
 * parameters of a port (except those inherent in the mminor port 
 * flavour selected, the UART type (determines feature set used) and 
 * the compatibility mode of the driver (mimic OS5 incorrect treatment 
 * of DCD input of modem port). 
 */

typedef 
struct _UART_data { 
	ioaddr_t		iobase; 		/* IO base address	*/
	ushort_t		irqnum; 		/* IRQ (PIC) line 	*/
	uart_type_t		uart; 			/* UART type		*/
	uart_state_t	status;			/* Current state	*/
	int				rxfifo;			/* Rx FIFO trigger level*/
	int				txfifo;			/* Rx FIFO trigger level*/
	struct termios  *termp; 		/* Initial flags values	*/
	struct termiox  *termxp; 		/* Initial HW flow 	*/
	ulong_t			compat;			/* Compatibility mode	*/
} uart_data_t;

/* 
 * Baud rate generation 
 * 8250 compatible UARTs generate Baud Rate B of 
 * 	B = Freq(Clk) / ( 16 * (Baud_Rate_Divisor))
 * PCs use a 1.8432 MHz xtal.   
 * Allow Space.c to overwrite antries in the Baud Rate table to allow 
 * (1) Non standard (ie. not supported by stty(1)) Baud Rates 
 * (2) Alias stty(1) baud rate to new Baud rate (eg. use 110 Baud entry 
 * for 57600 Baud - allows application workarounds).
 * 
 * MKDIV2(Baud,Clock)	: Defines Divisor to give Baud with UART Clock Freq.
 * MKDIV1(Baud) 	: Defines Divisor to give Baud with standard PC.
 * MKBD(Baud,Div) 	: Defines a Baud Rate table entry.
 * 
 * Note that the limit on the minimum Baud rate is set by the divisor size
 * of 16 bits. Hence definition of min Baud.
 */

#define MKDIV(b)		(1843200/(16*(b)))		
#define MKBD(b) 		{(b),MKDIV(b)} 
#define MK_BAUD(b) 		(1843200/(16*(b))) 
#define MIN_BAUD		(1843200/(16*65535))	

/* 
 * Extra definitions for Baud rates above 38400 
 * B0 - B38400 are defined in termio.h
 * Used to set very  high Baud rates in space.c  (for console).
 */

#define B57600	0x00E0000F		
#define B115200	0x00F0000F

/* 
 * asyc(7) ioctl definition.
 * 
 * Added ioctl(2) calls to 
 * (1) Return operating statistics for the UART hardware, to give aid 
 * 		to tuning communications links,
 * (2) Get the hardware type and driver configuration of the serial port, 
 * (3) Set UART specific features, (not supported on all UART flavors) or 
 *		not yet implemented in termio/termiox.
 */

/* 
 * ioctl(2) ID prefix 	("AsY") 
 */

#define ASY	(('A'<< 24)|('s'<< 16)|('Y' << 8))

/* Get driver operating statistics 	*/

#define ASY_GETSTATS	(ASY|2)	

/* 
 * Asynchronous port driver (asyc) statistics data.
 * Can be used to get data for debugging/tuning.
 */ 

typedef 
struct asyc_stats { 

							/* Operating counts	*/
	int	rxdataisr; 			/* Rx Data ISR requests 	*/
	int	rawrxchars; 		/* Raw received chars 		*/
	int	spclrxchars; 		/* special control chars 	*/
	int	txdatasvcreq; 		/* Tx Data service request	*/
	int	isrmodint; 			/* Modem Status service req	*/
	int	isrnointid; 		/* No ISR ID bits set. 		*/

							/* Flow control data	*/
	int	rcvdvstart; 		/* Received a VSTART/<IXANY>	*/
	int	rcvdvstop;			/* Received a VSTOP character	*/ 
	int	rcvdixany;			/* Received a VSTOP character	*/ 

							/* Resource failures	*/
	int	droprxcread; 		/* Dropped as !CREAD 		*/
	int	droprxopen; 		/* Dropped as !ISOPEN		*/
	int	droprxnobuf; 		/* No STREAMS buffer free 	*/ 
	int	nobufchars; 		/* Chars lost no STREAMS buffer	*/

							/* Errors		*/
	int	ierrcnt; 			/* Got a PERR|FERR|BREAK	*/
	int	softrxovrn1; 		/* Buffer 1 ISR full: drop char	*/
	int	softrxovrn2; 		/* Buffer 2 ISR full: drop char	*/
	int	rxoverrun;	 		/* Receive overrun error	*/
	int	rxperror; 			/* Parity error detected 	*/
	int	rxfrmerror; 		/* Frame error detected 	*/
	int	rxbrkdet; 			/* BREAK condition detected	*/

							/* Data I/O counts	*/ 
	int	rxrealdata; 		/* Valid data character				*/
	int	txdatachars;		/* Data characters sent 		*/
	int	txflowchars;		/* Flow control characters sent 		*/
	int	nointrpending; 		/* IRQ with no IPEND bit	*/

							/* Operating states	*/
	int	vstop_xhre_spin;	/* VSTOP XOFF && ~THRE		*/
	int	vstart_xhre_spin;	/* VSTART XOFF && ~THRE		*/
	int	restart_xhre_spin; 	/* RESTART_OUTPUT && ~THRE	*/
	int	rsteotpend; 		/* Cleared EOTPEND (lost IRQ)	*/
	int	badcarrsts; 		/* Bad CARR_ON status detected	*/
	int	nomsrdeltas;		/* MSR IRQ but bad MSR state	*/

							/* Transmit data 	*/
	int	notxttstate; 		/* BUSY/TIMEOUT/TTSTOP		*/
	int	notxhwfc; 			/* HW flow control blocked tx	*/
	int	notxswfc; 			/* SW flow control blocked tx	*/
	int	notxnodata; 		/* Tx ran out of data to send	*/
	int	notxqlockinuse; 	/* Cant get queue lock		*/

							/* OS/STREAMS data	*/
	int	upstream_charcnt;	/* Characters sent upstream	*/
	int	upstream_calls; 	/* Calls send STREAMS buffers	*/
	int	txstrqempty; 		/* Call to get next Tx failed	*/

							/* Sample timestamp 	*/	
	ulong_t	sample_time;	/* Relative time in (usec) 	*/

} astats_t ; 

/*
 * ASY_SETUARTMODE allows the user to tune various UART/driver  
 * operating parameters (per port) to improve various performance 
 * characteristics.
 * 
 * 		Rx FIFO trigger level 		Lost chars vs. IRQ overhead
 * 		Tx FIFO trigger level 		Burst overruns vs. IRQ overhead
 * 		Rx ISR buffer high water 	Remote flow control sensitivity	
 * 		Rx ISR buffer low water 	Remote flow control sensitivity	
 * 		Background asycpoll periodicity. Overhead vs. response time	
 * 		Compatibility mode (treatment of DCD).	API style (DCD value)
 * 
 */

#define ASY_SETUARTMODE (ASY|3)

typedef struct _setasyc  { 
	int		set_rxfifo; 
	int		set_txfifo; 
	int		set_rxlowat;
	int		set_rxhiwat;
	ulong_t	set_pollfreq; 
	int		set_compat; 	
} setasyc_t; 

/* 
 * Get current UART operating mode (driver configuration data) 
 * and UART hardware setup data.
 */

#define ASY_GETUARTDATA	(ASY|1)	

/* 
 * uart_data_t returns the UART setup parameters used by the driver. 
 * (Most parameters can be tuned at the space.c/ioctl/bootstring level).
 * uscan_data: IO,IRQ,Rx/Tx FIFO triggers,compatibility mode
 * The setud_t returns the data set as default or by previous 
 * ASY_SETUARTMODE ioctl(2) calls.
 */

typedef 
struct _getasydat { 
	uart_data_t		udata; 
	setasyc_t		asydrv; 		
} getasydat_t ; 	

#if defined(__cplusplus)
        }
#endif

#endif /* _IO_ASY_ASYC_ASYC_H */
