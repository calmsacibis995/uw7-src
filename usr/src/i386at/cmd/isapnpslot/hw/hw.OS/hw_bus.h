/*
 * File hw_bus.h
 * Information handler for bus
 *
 * @(#) hw_bus.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_bus_h
#define _hw_bus_h

extern const char	* const callme_bus[];
extern const char	short_help_bus[];

int	have_bus(void);
void	report_bus(FILE *out);

#endif	/* _hw_bus_h */

