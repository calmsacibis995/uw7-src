/*
 * File hw_rom.h
 * Information handler for rom
 *
 * @(#) hw_rom.h 67.1 97/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_rom_h
#define _hw_rom_h

extern const char	* const callme_rom[];
extern const char	short_help_rom[];

int	have_rom(void);
void	report_rom(FILE *out);

#endif	/* _hw_rom_h */

