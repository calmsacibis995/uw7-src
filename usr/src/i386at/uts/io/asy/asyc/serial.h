#ifndef _IO_ASY_ASYC_SERIAL_H	/* wrapper symbol for kernel use */
#define _IO_ASY_ASYC_SERIAL_H	/* subject to change without notice */

#ident	"@(#)serial.h	1.12"
#ident 	"$Header$"

/* 
 * Private header file. Contains various definitions used only by the 
 * serial port driver "asyc". _Not_ a public header.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#include <util/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */

#ifdef MERGE386 
#include <sys/merge386.h>	/* REQUIRED */
#endif

/*
 * Control register bit definitions for NS 8250 compatible 
 * UART family. Currently includes 
 * 8250 	Obsolete
 * 16450 	Current 8250: no new features
 * 16550 	Added Rx/Tx 16B buffers, copes with higher speeds
 * 
 * Above the 16550 UART part the features are manufacturer dependent 
 * (apart from the 16550 compatible set) 
 * 16650 	
 *		ST16C650[A]		: Startech 	: 32B buffers, Auto RTS/CTS flow control 
 *		TL16C750		: TI		: 64B buffers, Auto RTS/CTS flow control 	
 */

/*
 * Register offsets (from the data register)
 */

#define	DAT		0	/* receive/transmit data */
#define	ICR		1	/* interrupt control register */

/* Aliases: data register is also Baud rate clock divisor register	*/
#define	DLL		0	/* divisor latch (lsb) */
#define	DLH		1	/* divisor latch (msb) */


#define	ISR		2	/* interrupt status register */
#define	LCR		3	/* line control register */
#define	MCR		4	/* modem control register */
#define	LSR		5	/* line status register */
#define	MSR		6	/* modem status register */
#define	SCR		7	/* scratch data register	*/

#define IDAT(a)	((a)+DAT)	/* data register 	*/
#define	IICR(a)	((a)+ICR)	/* interrupt control register */
#define	IISR(a) ((a)+ISR)	/* interrupt status register */
#define IFCR(a)	IISR(a)		/* FIFO control register	*/
#define	ILCR(a) ((a)+LCR)	/* line control register */
#define	IMCR(a)	((a)+MCR)	/* modem control register */
#define	ILSR(a) ((a)+LSR)	/* line status register */
#define	IMSR(a) ((a)+MSR)	/* modem status register */
#define	ISCR(a) ((a)+SCR)	/* modem status register */

/*
 * Line Control Register 
 */

#define	LCR_WLS0	0x01	/* word length select bit 0 */	
#define	LCR_WLS1	0x02	/* word length select bit 2 */	
#define	LCR_STB		0x04	/* number of stop bits */
#define	LCR_PEN		0x08	/* parity enable */
#define	LCR_EPS		0x10	/* even parity select */
#define	LCR_SETBREAK	0x40	/* break key */
#define	LCR_DLAB	0x80	/* divisor latch access bit */
#define LCR_RXLEN	0x03    /* # of data bits per received/xmitted character */


/*
 * Line Status Register 
 */

#define	LSR_RCA		0x01	/* data ready */
#define	LSR_OVRRUN	0x02	/* overrun error */
#define	LSR_PARERR	0x04	/* parity error */
#define	LSR_FRMERR	0x08	/* framing error */
#define	LSR_BRKDET 	0x10	/* a break has arrived */
#define	LSR_XHRE	0x20	/* tx hold reg is now empty */
#define	LSR_XSRE	0x40	/* tx shift reg is now empty */
#define	LSR_RFBE	0x80	/* rx FIFO Buffer error */

/*
 * Interrupt Status Register 
 */

#define	ISR_MSTATUS	0x00		/* RS-232 line interrupt */
#define	ISR_NOINTRPEND	0x01	/* Interrupt pending bit */
#define	ISR_TxRDY	0x02		/* Transmitter interrupt */
#define	ISR_RxRDY	0x04		/* Receiver ready interrupt */
#define	ISR_ERROR_INTR	0x08	/* Error interrupt */
#define	ISR_FFTMOUT	0x0c		/* FIFO Timeout */
#define	ISR_RSTATUS	0x06		/* Receiver Line status */
#define ISR_64E		0x20		/* Set when '750 64B FIFO enabled*/
#define ISR_FEN		0xC0		/* Bits set when FIFOs enabled	*/

/*
 * FIFO control register 
 */
#define	FCR_FEN		0x01	/* Enable FIFOs 		*/
#define	FCR_RRST	0x02	/* Reset RxFIFO (self clr)	*/
#define	FCR_TRST	0x04	/* Reset TxFIFO (self clr)	*/
#define	FCR_DMA		0x08	/* DMA mode select		*/
#define FCR_64E		0x20	/* Enable '750 UART 64B FIFO	*/
#define	FCR_RTRL	0x40	/* RxFIFO trigger lsb		*/
#define	FCR_RTRH	0x80	/* RxFIFO trigger msb		*/

#define FCR_FE1	(FCR_FEN|FCR_RRST|FCR_TRST) 

/*
 * Interrupt Enable Register 
 */
#define	ICR_RIEN	0x01	/* Received Data Ready */
#define	ICR_TIEN	0x02	/* Tx Hold Register Empty */
#define	ICR_SIEN	0x04	/* Receiver Line Status */
#define	ICR_MIEN	0x08	/* Modem Status */

/*
 * Modem Control Register 
 */
#define	MCR_DTR		0x01	/* Data Terminal Ready 		*/
#define	MCR_RTS		0x02	/* Request To Send 		*/
#define	MCR_OUT1	0x04	/* Aux output - not used 	*/
#define	MCR_OUT2	0x08	/* turns intr to 386 on/off 	*/	
#define	MCR_ASY_LOOP	0x10	/* loopback for diagnostics 	*/
#define	MCR_LBK		0x10	/* loopback for diagnostics 	*/

/* 
 * UART specific controls (not implemented over 550/650/750)
 */

#define MCR750_AFC	0x20	/* TI16C750 Auto flow control	*/

#define MCR650_INTA	0x20	/* EXAR ST16C650[A] 		*/
#define MCR650_IRRT	0x40	/* EXAR ST16C650[A] 		*/
#define MCR650_CLKSEL	0x80	/* EXAR ST16C650[A] 		*/

/*
 * Modem Status Register 
 */
#define	MSR_DCTS	0x01	/* Delta Clear To Send */
#define	MSR_DDSR	0x02	/* Delta Data Set Ready */
#define	MSR_DRI		0x04	/* Trail Edge Ring Indicator */
#define	MSR_DDCD	0x08	/* Delta Data Carrier Detect */
#define	MSR_CTS		0x10	/* Clear To Send */
#define	MSR_DSR		0x20	/* Data Set Ready */
#define	MSR_RI		0x40	/* Ring Indicator */
#define	MSR_DCD		0x80	/* Data Carrier Detect */

#define DELTAS(x) 	((x)&(MSR_DCTS|MSR_DDSR|MSR_DRI|MSR_DDCD))
#define STATES(x) 	((x)(MSR_CTS|MSR_DSR|MSR_RI|MSR_DCD))


#define FIFOEN450	0x8f	/* 450 fifo enabled, Rx fifo level at 1 */
#define FIFOEN550	0x87	/* 550 fifo enabled, Rx fifo level at 14 */
#define	TRLVL1		0x07	/* 550 fifo enabled, Rx fifo level at 1	*/
#define	TRLVL2		0x47	/* 550 fifo enabled, Rx fifo level at 4	*/
#define	TRLVL3		0x87	/* 550 fifo enabled, Rx fifo level at 8	*/
#define	TRLVL4		0xc7	/* 550 fifo enabled, Rx fifo level at 14 */

/*
 * asyc_flags : internal driver state 
 */

#define CONDEV		0x00000001	/* Device is system/kdb console 	*/
#define ISTOP		0x00000002	/* Input flow blocked (HW/SW)		*/
#define	OSTOP		0x00000004	/* Transmit blocked by flow control	*/

#define ISTOPPEND	0x00000008	/* Input flow blocker SW			*/
#define IGOPEND		0x00000010	/* Input flow unblock SW			*/
#define IXOFPEND	(ISTOPPEND|IGOPEND)

#define OBRKPEND	0x00000020	/* Output BREAK state pending		*/ 
#define UARTPEND	0x00000040	/* Set UART parameters pending		*/ 
#define EOTXPEND	0x00000080	/* End of transmit, timed asyc_idle	*/

#define ABUSY		0x00001000	/* Tx in progress					*/
#define ASTOP		0x00002000	/* Tx blocked (flow control) 		*/
#define AIDLE		0x00004000	/* Tx Idling (no data)				*/
#define ATIMED		0x00008000	/* Tx in EOTx timeout 				*/
#define ABREAK		0x00010000	/* Tx in BREAK state				*/

#define OBLK_SWFC	0x00000100	/* Tx blocked by IXON && VSTOP		*/
#define OBLK_HWFC	0x00000200	/* Tx blocked by [CTS|DSR][XON]		*/
#define OBLK_MSTP	0x00000400	/* Tx blocked by [CTS|DSR][XON]		*/
#define OSTOPPEND	(OBLK_SWFC|OBLK_MSTP|OBLK_HWFC)

#define IDLETASK	(UARTPEND|OBRKPEND|IXOFPEND)

/* 
 * L998
 * itimeout(D3) cannot be called from IPL 9 interrupt routines. It may call
 * kmem_zalloc() and this may call blist_alloc() which will require a lock. 
 * asycintr() may have interrupted a kmem_giveback_daemon() execution at 
 * IPL 8, and the lock may be taken, causing the system to hang. 
 * Thus, scheduling timed/asynchronous events at IPL 9 is done by setting a 
 * flag in the asyc_t flags field; the next time that the poll routine is 
 * called (this is initiated at xxstart(D2) time, and is TO_PERIODIC, with
 * a rate of 20ms, the delay count is decremented. When the count reaches 
 * 0, the requested event is called.
 */

#define DOIDLETOUT	0x00100000	/* Replace idle itimeout IPL9 call		*/
#define DOSHUPTOUT	0x00200000	/* Replace SIGHUP itimeout IPL9 call	*/

/* 
 * Input buffer and flow control settings. 
 * At input buffer free space < IBUF_LOWAT, assert input flow control.
 * When free space goes above IBUF_HIWAT, turn flow control off
 * (Hysteresis to prevent oscillating since SW flow control wastes 
 * bandwidth by sending VSTART/VSTOP bytes.
 */

#define	ISIZE	512					/* Input ring buffer size 	*/

/*
 * Output Buffer 
 * If we are using a separate ISR output buffer (as opposed to 
 * using the STREAMS data blocks themselves as accessed by the 
 * ISR, then the size (in bytes) of the buffer is set by the 
 * OBUFSZ define below. 1 buffer per UART port.
 */

#define OBUFSZ	256					/* Output ring buffer size 	*/

/* 
 * Exclusive open flag (stored in strtty.t_dstat) 
 * Set if open(2d) otype & FEXCL
 */

#define EXCL_OTYPE		1

#define NTOUTS 			17

/*
 * Define variable to hold EFLAGS register when calling the 
 * asyc_[disable|restore]_irqsvc routines. 32b
 */
typedef unsigned int eflags_t;

/*
 * 2 buffers: active and idle.
 * 	active is written to by asycintr() when valid data received. 
 * 	asycpoll() checks count in active buffer. If >0 it swaps 
 *	buffers and sends newly idle to asyc_inchar() for processing.
 *	asyc_inchar() takes each character, does any input processing 
 * 	specified, and sends upstream, returning the empty buffer to 
 *	be used by asycpoll(). 
 *	Most variables are only written by a single entity. 
 * 
 * 	Active buffer should always have iget == 0. iput == chars
 * 	received. Process buffer contents from ibuf[0..(iput - 1)].
 *	When (iget == iput) reset them both to 0. Buffer swaps can 
 *	verify that buffer to activate is processed (!iput && !iget).
 *
 *	Output buffering is implementable as two levels, depending 
 *  on whether to use a separate output buffer which is used 
 *	by the ISR only, filled as a background task. This allows Tx 
 * 	streaming even with small STREAMS data packets. 
 *
 */

typedef
struct asyc_aux {
	ushort_t	ibuf1[ISIZE];		/* Data & LSR , WR by ISR	*/ 
	ushort_t	ibuf2[ISIZE];	

	ushort_t	iput1;				/* Next free ibuf1[x]		*/
	ushort_t	iget1;				/* Unprocessed ibuf1[x] 	*/	

	ushort_t	iput2;				/* Valid iff active 		*/
	ushort_t	iget2;				/* Empty <=> iput == iget	*/

	boolean_t	ibuf1_active; 		/* current active buffer	*/

	ushort_t	ibuf_lowat;			/* Disable flow control		*/
	ushort_t	ibuf_hiwat;			/* Enable flow control		*/

	ushort_t	oput;				/* Offset to free obuf[]	*/
	ushort_t	oget;				/* Offset to next obuf[]	*/

	uchar_t		obuf[OBUFSZ];		/* ISR output data buffer 	*/

	ushort_t	obuf_lowat;			/* Reload obuf if less data	*/
	ushort_t	obuf_hiwat;			/* Stop loading above this	*/

	int			txtout; 			/* Tx chain timeout polls	*/
	int			txtoval[NTOUTS];	/* Tx chain TxRDY timeout 	*/
	int			txtchar; 			/* Character Tx period (us) */
	toid_t		idletoid;			/* Last char itimeout id	*/

	int			obrkidx;			/* BREAK obuf array offset	*/

	eflags_t	efl; 				/* Base CPU EFLAGS register	*/

} asycbuf_t ;

/*
 * Output buffer manipulation macros.
 *
 * Functions depend upon which output buffering strategy is used: ISR 
 * separate output buffer/use STREAMS buffers in output ISR process.
 *	
 * Routines / Macros 
 *	
 *	OBUFCNT(ap)		: Get bytes in OP buffer 
 *	OBOFF(s,e)		: Get bytes between s(tart) and e(nd) locations. 
 * 	
 */

#define OBUFCNT(ap)	( ( (ap)->oput >= (ap)->oget ) ?	\
		((ap)->oput - (ap)->oget ) : (OBUFSZ + (ap)->oput - (ap)->oget))

#define OBOFF(s,e)	(((e) >= (s)) ? ((e) - (s)) : (OBUFSZ + (e) -(s)))

/* 
 * TXTOFF is the timeout offset (in asycpoll period units [20000 us])
 * Allows for itimeout(D3) invocation holdoff latency and rounding down 
 * losses in calculation of periods, and gives non-zero value at high baud.
 */

#define TXTOFF	2 

/* 
 * UART FIFO sizes.
 */ 
 
#define U550_FIFOSZ	16
#define U650_FIFOSZ	32
#define U750_FIFOSZ	64	

/* 
 * asyc_debug definition flags : set to display various options 
 *
 * Display functons	
 */

#define DSP_LCRCTL	0x1				/* LCR modifications	*/
#define DSP_DRVCTL	0x2 			/* Driver control 		*/
#define DSP_TYPE	0x4 			/* Display uarttype		*/
#define DSP_IOCTL	0x8 			/* Display ioctl(2) calls	*/
#define DSP_MSRSTS	0x10			/* Display MSR reg states	*/
#define DSP_FIFO	0x20			/* Display FIFO control 	*/
#define DSP_FUNCS	0x40			/* Display routine names 	*/
#define DSP_TCHAR	0x80			/* Display Tx data		*/
#define DSP_TTYOVR	0x100			/* Display TTY override	*/
#define DSP_DCU		0x200			/* DCU control	*/
#define DSP_RDPIT	0x400			/* Read PIT timer 	*/
#define DSP_EOTXDAT	0x800			/* End of Tx data handling	*/
#define DSP_XOFF	0x1000			/* XOFF flow control	*/
#define DSP_IFC		0x2000			/* Input flow control	*/
#define DSP_OFC		0x4000			/* Output flow control	*/
#define DSP_TXSMB	0x8000			/* */
#define DSP_SPACE	0x10000			/* */
#define DSP_PROCS	0x20000			/* asycproc functions	*/
#define DSP_DBGPROC	0x40000			/* Debug asycproc functions	*/
#define DSP_RFTL	0x80000			/* Debug asycproc functions	*/
#define DSP_FLWCTL	0x100000		/* Flow control 	*/
#define DSP_XFUNCS	0x200000		/* Display odd function calls	*/
#define DSP_CHRPROC	0x400000		/* Display input char processing	*/
#define DSP_SPCH	0x800000		/* Display special characeter	*/

/* 
 * Debug actions (top nibble)
 */

#define START_WDOG	0x10000000		/* Start watchdog (EISA only)	*/
#define DSP_DOGETC	0x20000000		/* Start watchdog (EISA only)	*/

/* 
 * For debugging console stuff, set error codes in 32b ecode.
 * Use crash(1M) or kdb(1M) to view. Or sdb on /dev/kmem.
 */

#define BAD_CONS_MINOR 		0x000C0001
#define BAD_PORT_STATE 		0x000C0002
#define SETCONSOLE_FAILED 	0x000C0004
#define BAD_CONS_PARMS		0x000C0008
#define FIXCNMAP_FAILED) 	0x000C0010

/* 
 * Terminal special characters 
 * These characters cause special functions to be performed on 
 * system/debugger serial consoles.
 */

/* Character to enter kernel debugger	*/
#define DEBUGGER_CHAR	('K' - '@')	/* Ctrl-K */

/* Character to panic the box	*/
#define PANIC_CHAR	('_' - '@')	/* Ctrl-hyphen */

/* Character to reboot, via "init 6" 	*/
#define REBOOT_CHAR	('\\' - '@')	/* Ctrl-backslash */

/* 
 * Char to input C-list Macro
 */

#define PutInChar( tp, c) if (( tp)->t_in.bu_ptr != NULL) \
{ \
	/* Put a char into the input c-list */ \
	*( tp->t_in.bu_ptr++) = c; \
	if ( --tp->t_in.bu_cnt == 1) \
		iasy_input( tp, L_BUF); \
} else { \
	cmn_err(CE_WARN,"PutInChar(): NULL t_in.bu_bp");  \
} 

/*

/*
 * Asychronous configuration Structures 
 */

typedef
struct asyc {
	int		flags;			

	/* Register IO addresses, all derivable from base 	*/

	ulong_t		asyc_dat;
	ulong_t		asyc_icr;
	ulong_t		asyc_isr;
	ulong_t		asyc_lcr;
	ulong_t		asyc_mcr;
	ulong_t		asyc_lsr;
	ulong_t		asyc_msr;


	/* IRQ cookie - cm_intr_attach()/cm_intr_detach() */
	/* uart_scantbl/DCU contains IRQ number	*/
 
	ulong_t			asyc_vect;
	void			*asyc_irqmagic; 

	/* ISR buffer data structure	*/
	struct asyc_aux *asyc_bp;


	/* Minor device type, store minor / port / open (EXCL)	*/
	minor_t			asyc_dev;

	/* UART HW type	*/
	uart_type_t		asyc_utyp;	

	/* DOIDLETOUT code 	*/
	long			ticks; 				/* Delay to asyc_idle_wrapper	*/
	clock_t			endtime;
	struct strtty  	*tp;

	/* DOSHUPTOUT code 	*/
	struct strtty	*sighup_tp; 		/* Call iasy_hup(sighup_tp) ASAP	*/

#ifdef MERGE386
	struct mrg_com_data mrg_data;
	int		asyc_mrg_state;
#endif /* MERGE386 */

	uchar_t		rxfifo;		/* RxFIFO trigger IRQ  count 	*/
	uchar_t		txfifo; 	/* Tx burst size/FIFO fill cnt	*/

	/* Tx timeout count */
	uint_t		txtout;		/* Tx timeout (in 20ms ticks)	*/

	/* 
	 * Saved Modem Status Register state.
	 */

	uchar_t		asyc_msrlaststate;

	ulong_t		compat; 	/* Compatibility mode */
	ulong_t		pollfreq;	/* May be tunable in future	*/

	astats_t	*asyc_stats; 	/* Operating statistics	*/

} asyc_t ;

/* Status of the com port attachment (asyc_mrg_state) */

#define	ASYC_MRG_DETACH		0x00
#define	ASYC_MRG_ATTACH		0x01

/* 
 * Locking/ISR exclusion.
 *
 * Because the interrupt routine executes at IPL 9 it cannot be blocked 
 * by the splhi(D3) call. ( splhi(D3) contains an explicit ASSERT to 
 * test for attempts to set SPL levels above PL_HI (8) ). The only method 
 * to protect critical code is to use the CPU cli/sti opcodes to clear/set 
 * the IF bit in the eflags register: this prevents any interrupt from 
 * being serviced. To correctly restore the CPU state we save the eflags 
 * value and restore it. 
 * The asyc_disable_irqsvc/asyc_restore_irqsvc are inlined into the code 
 * when the ASYC_CLI/STI_BASE(ap) macros are used. They need to be declared
 * in the same module as used to avoid C making them extern routines by 
 * default.
 */ 

asm int 
asyc_disable_irqsvc ( void )
{ 
	pushf
	pop		%eax
	cli 
} 

asm void 
asyc_restore_irqsvc ( int efl )
{ 
%mem	efl; 	
	movl	efl, %eax
	pushl	%eax
	popfl

}

#define ASYC_CLI_BASE(x)	{(x)->efl = asyc_disable_irqsvc(); }  
#define ASYC_STI_BASE(x)	{ asyc_restore_irqsvc((x)->efl); }
/*
 * For console routines, no other data allocated.
 */
typedef int	flags_t	; 

#define ASYC_CLI(f)		{(f) = asyc_disable_irqsvc(); }  
#define ASYC_STI(f)		{ asyc_restore_irqsvc((f)); }

/* 
 * Baud rate generation 
 * 8250 compatible UARTs generate Baud Rate B of 
 * 	B = Freq(Clk) / ( 16 * (Baud_Rate_Divisor))
 * PCs use a 1.8432 MHz xtal.   
 * Allow Space.c to overwrite antries in the Baud Rate table to allow 
 * (1) Non standard (ie. not supported by stty(1)) Baud Rates 
 * (2) Alias stty(1) baud rate to new Baud rate (eg. use 110 Baud entry 
 * for 57600 Baud - allows application workarounds).
 */

typedef 
struct _line_speed { 
	uint_t		baudrate; 
	ushort_t	clockdiv; 	
} line_speed_t; 

#define MAXBAUDRATE 115200		/* Used to create divisors */ 

/*
 * Used to calculate max char time
 */

#define	MAX_CHAR_K1		12000000uL

#define TST_BYTE1	0x55
#define TST_BYTE2	0xAA

/*
 * Timeout decrement value (prevents timeouts = ~0 due to 
 * 20ms units)
 */

#define TX_TOUT_POLL_DEC 	100


#define ENABLE_IRQS(io)	(outb(IICR(io),(ICR_RIEN|ICR_TIEN|ICR_SIEN|ICR_MIEN)))

#define DCD_HOLD_DLY	10			/* Allow DCD to settle after sample ?? */

/*
 * Rx FIFO trigger level tuning definitions.
 */

typedef ushort_t ftl_t ; 		/* FIFO trigger level (<65535)	*/

typedef ushort_t  *ftltb_t; 	/* FIFO trigger level table		*/

typedef struct _uf { 
	uart_type_t		uart;				/* UART h/w model	*/
	int				nftl; 				/* Number of trigger levels	*/
	ftltb_t			pftlab; 			/* Address of trigger level table	*/
	int				nbrf; 				/* FIFO size (B)		*/
} uf_t ; 

/*
 * Create ut_t entries with th is macro.
 */

#define _MKUT(uart,rfsz,ftltab)	 { (uart), sizeof((ftltab)), (ftltab), rfsz } 

typedef enum { 
	TUNE_OFF, 				/* Disabled	*/
	TUNE_IDLE, 					/* Enabled, inactive 	*/
	TUNE_INIT, 					/* Device to tune set, initialising	*/
	TUNE_LOGGING, 				/* Collecting data	*/
	TUNE_ANALYSE,				/* ?? Deciding !	*/
	TUNE_SETRL					/* Get results and tune	*/
} rftlstate_t;  

/* 
 * Tunables 
 *	Use this macro to validate. 
 *		Eg. MK_VALID_VAR(asyc_ibhiwat, 100,  asyc_iblowat, 90)
 */

#define MK_VALID_VAR(v,Vmax,Vmin,Vdef)	\
	{ 	if (((v) < (Vmin)) || ((v) > (Vmax))) (v) = (Vdef); 	} 

/*
 * Input ISR buffer pointer bump macro. Increment modulo bufsize.
 */

#define	INEXT(x)	(((x) < (ushort_t)(ISIZE - 1)) ? ((x) + 1) : 0)

#define	ONEXT(x)	(((x) < (ushort_t)(OBUFSZ - 1)) ? ((x) + 1) : 0)
#define	OLAST(x)	(((x) > (ushort_t)0) ? ((x) - 1) : (OBUFSZ - 1))

#define TMAX(a,b)	((a>b)?((a)+1):(b))

/* 
 * Transmitter states 
 *	TXBUSY		: data in TxFIFO | THR | TSR & sent down wire.
 *	TTBLOCKED	: HW or SW flow controlled or upstream M_STOP message.
 *	TXIDLE		: TSRE && THRE, ie. UART tx empty, line in SPACE state.
 *	TXSPIN		: Waiting for BREAK state to complete. 
 * 
 * Note that the UART tries to stream transmits, so should remain in 
 * TXBUSY state for entire transmit data, unless a BREAK or DELAY request
 * is embedded, or a TCSETA ioctl is issued, all of which require a TXIDLE
 * condition to process. This is found by delaying a characters time from 
 * receipt of the THRE TxRDY IRQ. The asyc_doidle() routine then calls the
 * requisite routine. The BREAK processing adds a \000 character to the Tx 
 * data required to drain prior to BREAK, and the asyc_txsvc routine makes 
 * this dummy data the last byte in the FIFO data block. The TxRDY associated 
 * with that block is called as the \0 is being copied into the TSR and 
 * being transmitted, and the BREAK state can thus be started at any time 
 * during the character transmit (ie. the interupt processing latency for 
 * the TxRDY should be less than the \0 transmit time, and BREAK extends the 
 * line MARK state from an \0 character into a BREAK condition.
 * 
 */

#define TXBUSY(tp)		((tp)->t_state & BUSY)
#define TXBLOCKED(tp)	((tp)->t_state & TTSTOP)
#define TXSPIN(tp)		((tp)->t_state & TIMEOUT)
#define TXIDLE(tp)		(!((tp)->t_state & (BUSY|TIMEOUT|TTSTOP)))


/* 
 * TXSVC_REQD(tp) used to filter calls to asyc_txsvc() from the asycintr
 * routine (THRE: always call the routine), asycpoll() non-timeout call
 * (depends what state ??), and T_OUTPUT (if TXIDLE(tp) as well). Set to 
 * B_TRUE for now. 
 */

#define TXSVC_REQD	(OBUFCNT(ap) || (EVENTS_WAITING(tp)))

#define IBUFFREE(ap)	(((ap)->ibuf1_active) ? 					\
							(ISIZE - ((ap)->iput1 - (ap)->iget1)) : 	\
							(ISIZE - ((ap)->iput2 - (ap)->iget2)))

/*
 * 	GET_LSR(lsr) gets the current LSR and saves the associated data
 * 	if it indicates an error : the PE/FE/BRK bits are then cleared.
 * 	Necessary because reading the LSR (holds the state of the top 
 * 	byte of the FIFO data (if LSR_RCA is set)) will clear the error 
 * 	bits OE/PE/FE and the BRKDET bit, and the received data state is 
 *	lost, ie. rubbish UART hardware.
 */

#define GET_LSR(lsr)											\
{																\
	ushort_t	idat;											\
	lsr = inb(asyc->asyc_lsr);									\
	if (lsr & (LSR_OVRRUN|LSR_PARERR|LSR_FRMERR|LSR_BRKDET)) { 	\
		idat = inb(asyc->asyc_dat) & 0xff; 						\
		idat |= (lsr << 8); 									\
		if (ap->ibuf1_active)									\
			ap->ibuf1[ap->iput1++] = idat; 						\
		else													\
			ap->ibuf2[ap->iput2++] = idat; 						\
	}															\
}

/* 
 * Define a logical binary XOR operator 
 */

#define XOR(a,b)	(((a) || (b)) && !((a) && (b)))

/* 
 * Define macros for testing the setting of various operating modes in the 
 * space.c uart_scantbl[]. 
 */

#define SCOMODEM_COMPAT(unit)	(uart_scantbl[(unit)].compat & SCOMODEM)
#define TIOCM_DIS_HWFC(unit)	(uart_scantbl[(unit)].compat & TIOCMDFC)

/* 
 * Text processing macros for bootstring processing.
 */

#define SKIP_WS(p)	\
{	\
	while ((p) && (*(p)))	\
		if ((*(p) == ' ') || (*(p) == '\t'))	\
			++(p);	\
		else 		\
			break;	\
}

#define FIND_KEY(p)	\
{	\
	while ((p) && (*(p)))	\
	if ((*(p) != 'B')&&(*(p) != 'P')&&(*(p)!= 'C'))	\
		++(p);	\
	else		\
		break;	\
}

#define FIND_SEP(p)	\
{	\
	while ((p) && (*(p))) 	\
	if (*(p) != '|')		\
		++(p);				\
	else 					\
		break;				\
}

#if defined(__cplusplus)
        }
#endif

#endif /* _IO_ASY_ASYC_SERIAL_H */
