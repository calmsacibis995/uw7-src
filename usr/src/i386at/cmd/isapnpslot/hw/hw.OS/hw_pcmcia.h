/*
 * File hw_pcmcia.h
 * Information handler for pcmcia
 *
 * @(#) hw_pcmcia.h 62.1 97/03/09 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_pcmcia_h
#define _hw_pcmcia_h

extern const char	* const callme_pcmcia[];
extern const char	short_help_pcmcia[];

int	have_pcmcia(void);
void	report_pcmcia(FILE *out);

/*
 * Intel 82365SL PCIC
 */

#define PCIC_ADDR_1	0x3E0	/* and 0x3E1 */
#define PCIC_ADDR_2	0x3E2	/* and 0x3E3 */

#define PCIC_SLOTS	4			/* Slots per device */
#define PCIC_ADDRS	(256/PCIC_SLOTS)	/* Addresses per slot */

#define PCIC_REVISION			0x00
#define PCIC_INTERFACE_STATUS		0x01
#define PCIC_POWER_CONTROL		0x02
#define PCIC_INTERRUPT			0x03
#define PCIC_STATUS_CHANGE_INTR		0x05
#define	PCIC_WINDOW_ENABLE		0x06
#define PCIC_IO_CONTROL			0x07
#define PCIC_IO_0_START_LOW		0x08
#define PCIC_IO_0_START_HIGH		0x09
#define PCIC_IO_0_STOP_LOW		0x0A
#define PCIC_IO_0_STOP_HIGH		0x0B
#define PCIC_MEM_0_START_LOW		0x10
#define PCIC_MEM_0_START_HIGH		0x11
#define PCIC_MEM_0_STOP_LOW		0x12
#define PCIC_MEM_0_STOP_HIGH		0x13
#define PCIC_MEM_0_OFFSET_LOW		0x14
#define PCIC_MEM_0_OFFSET_HIGH		0x15

#endif	/* _hw_pcmcia_h */

