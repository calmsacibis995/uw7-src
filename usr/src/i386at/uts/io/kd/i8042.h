#ifndef _IO_KD_I8042_H	/* wrapper symbol for kernel use */
#define _IO_KD_I8042_H	/* subject to change without notice */

#ident	"@(#)i8042.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef AT_KEYBOARD

/*
 *
 *	Copyright (C) The Santa Cruz Operation, 1988-1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 *
 */

/*
 *  Intel 8042 definitions.
 *
 *  REFERENCES
 *	IBM Personal System/2 Model 80 Technical Reference,
 *	Chapter 4 (System Board I/O Controllers),
 *	"Keyboard/Auxiliary Device Controller" (section 1).
 *	First edition (April 1987),
 *	IBM 84X1508 (a.k.a. S84X-1508-00).
 *
 *	8042 Technical Reference Manual.
 *	Undated, COMPAQ.
 *
 *  MODIFICATION HISTORY
 *	14 April 1988	scol!blf
 *		- Created.
 *	05apr89		scol!hughd
 *		- brought into 3.2 MC from 2.3 MC: for AT also
 *	21jul89		scol!howardf
 *		- commented out "#ifdef Q_PS"s for Compaq keyboard mouse etc.
 */

extern void	on8042intr(int);
extern int	getc8042(int);
extern int	send8042(int, unsigned char, unsigned char *);
extern int	xmit8042(int, unsigned char, unsigned char *);
extern void	reset8042(void /* no args */);
extern void	drain8042(int);
extern void	able8042(int, int, int);

/*
 * Some general notes:
 *
 * The data byte (I8042_DATA) should be read only when I42S_OBF is set.
 * If I42S_ABF is set, the value in I8042_DATA came from the AUX device.
 * (Note: I42S_ABF is valid iff. I42S_OBF is set.)
 *
 * Neither I8042_CTRL nor I8042_DATA should be written unless if either
 * I42S_IBF or I42S_OBF is set.
 *
 * The devices connected to the 8042 should be disabled before sending
 * a command that generates a reply sent to the host CPU via I8042_DATA.
 *
 * On the PS/2, an external latch holds the (level-triggered) IRQs
 * until I8042_DATA is read.
 */

#define I8042_CTRL	0x64	/* R/W (1 byte): Status/Command		*/
#define I8042_DATA	0x60	/* R/W (1 byte): Input/Output data	*/

/*
 * Command byte (send command I42X_WCB to change)
 */

/***			0x80	** reserved				*/
#define I42C_KTM	0x40	/* Keyboard Translate Mode (1=IBM XT)	*/
#define I42C_DAD	0x20	/* Disable AUX Device (mouse)		*/
#define I42C_DKB	0x10	/* Disable KeyBoard device		*/

/***			0x08	** reserved				*/
#define I42C_SF		0x04	/* "System Flag" (0=cold boot, 1=reset)	*/
#define I42C_AIE	0x02	/* AUX device Interrupt enable		*/
#define I42C_KIE	0x01	/* KeyBoard device Interrupt enable	*/

/*
 * Status byte (read host CPU I/O port I8042_CTRL)
 *
 *	Note: The hi-order nibble may be either nibble of
 *	the 8042 input port, depending on what commands
 *	have been sent to the 8042.
 */

#define I42S_PED	0x80	/* Parity Error Detected		*/
#define I42S_TOE	0x40	/* general Time Out Error		*/
#define I42S_ABF	0x20	/* AUX output Buffer Full
				 *  (valid iff. I42S_OBF is set)
				 */
#define I42S_INB	0x10	/* Inhibit (0=password lock mode)	*/

#define I42S_CDF	0x08	/* Command/Data Flag (0=cmd, 1=data)	*/
#define I42S_SF		0x04	/* "System Flag"			*/
#define I42S_IBF	0x02	/* Input Buffer Full			*/
#define I42S_OBF	0x01	/* Output Buffer Full			*/

/*
 * Output port
 *	host CPU read:	Send 8042 command 0xD0
 *	host CPU write: Send 8042 command 0xD1
 */

#define I8042_OUT	0xD0	/* 8042 RAM output port address		*/

#define I42O_KDO	0x80	/* Keyboard Data Out			*/
#define I42O_KCO	0x40	/* Keyboard Clock Out			*/
#define I42O_IRQ12	0x20	/* IRQ 12 (AUX interrupt)		*/
#define I42O_IRQ01	0x10	/* IRQ  1 (Keyboard interrupt)		*/
#define I42O_ACO	0x08	/* AUX Clock Out			*/
#define I42O_ADO	0x04	/* AUX Data Out				*/
#define I42O_A20	0x02	/* A20 gate (enable address wraparound)	*/
#define I42O_SR		0x01	/* System (host CPU) Reset (inverted!)	*/

/*
 * Input port
 *	host CPU read:	Send 8042 command 0xC0 (I42X_RIP)
 *
 *	Note: the 8042 can place either nibble of this port
 *	in the hi-order nibble of the status byte.
 */

#define I8042_IN	0xD1	/* 8042 RAM input port address		*/

/***			0x80	** not connected			*/
/***			0x40	** not connected			*/
/***			0x20	** not connected			*/
/***			0x10	** not connected			*/
#define I42I_LCK	0x08	/* keyboard LoCKed			*/
#define I42I_FUSE	0x04	/* keyboard/AUX pwr line FUSE (0=open)	*/
#define I42I_ADI	0x02	/* AUX Data In				*/
#define I42I_KDI	0x01	/* Keyboard Data In			*/

/*
 * Commands
 *
 *	0x00 - 0x1F	host CPU read indirect RAM
 *	0x20 - 0x3F	host CPU read 8042 RAM directly
 *	0x40 - 0x5f	host CPU write indirect RAM
 *	0x60 - 0x7F	host CPU write 8042 RAM directly
 *		The 8042 RAM address for the "indirect" commands
 *		is the lo-order 5 bits of 8042 RAM byte 0x2B.
 *
 *	0xF0 - 0xFF	pulse output port
 *		A truely baroque set of commands; see the references.
 */

#define I42X_RCB	0x20	/* Read 8042 Command Byte		*/
#define I42X_WCB	0x60	/* Write 8042 Command Byte		*/

/* #ifndef Q_PS */
# define I42X_SSC	0xA3	/* System Speed Control (not in PS/2)	*/
# define I42X_TSC	0xA4	/* Toggle 8042 Speed Control		*/
# define I42X_RSC	0xA5	/* Read Speed Control			*/
/* #else */
/***			0xA3	** not used				*/
# define I42X_TPW	0xA4	/* Test PassWord (0xFA=password loaded)	*/
# define I42X_LPW	0xA5	/* Load PassWord			*/
# define I42X_EPW	0xA6	/* Enable PassWord			*/
# define I42X_DAD	0xA7	/* Disable AUX Device			*/
# define I42X_EAD	0xA8	/* Enable AUX Device			*/
# define I42X_AIT	0xA9	/* AUX Interface Test (0x00=ok)		*/
/* #endif */

#define I42X_TST	0xAA	/* 8042 Self Test			*/
#define I42X_KIT	0xAB	/* Keyboard Interface Test (0x00=ok)	*/

#define I42X_DKB	0xAD	/* Disable KeyBoard			*/
#define I42X_EKB	0xAE	/* Enable KeyBoard			*/
#define I42X_RIP	0xC0	/* Read Input Port (see defines above)	*/

/* #ifdef Q_PS */
# define I42X_PHI	0xC1	/* Poll HI byte of input port		*/
# define I42X_PLO	0xC2	/* Poll LO byte of input port		*/
/* #endif */

#define I42X_ROP	0xD0	/* Read Output Port (see defines above)	*/
#define I42X_WOP	0xD1	/* Write Output Port			*/

/* #ifdef Q_PS */
# define I42X_WKB	0xD2	/* Write Keyboard output Buffer		*/
# define I42X_WAB	0xD3	/* Write AUX output Buffer		*/
# define I42X_WAD	0xD4	/* Write to AUX Device			*/
/* #endif */

#define I42X_RTI	0xE0	/* Read Test Inputs (T1=bit 1, T0=bit 0)
				 *  Note: The pins are wired differently
				 *  on the PS/2 and AT.
				 */
#endif /* AT_KEYBOARD */
#if defined(__cplusplus)
	}
#endif

#endif /* _IO_KD_I8042_H */
