/*
 * File hw_ram.h
 * Information handler for ram
 *
 * @(#) hw_ram.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_ram_h
#define _hw_ram_h

extern const char	* const callme_ram[];
extern const char	short_help_ram[];

int	have_ram(void);
void	report_ram(FILE *out);

#endif	/* _hw_ram_h */

