/*
 * File hw_pci.h
 * Information handler for pci
 *
 * @(#) hw_pci.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_pci_h
#define _hw_pci_h

extern const char	* const callme_pci[];
extern const char	short_help_pci[];

int	have_pci(void);
void	report_pci(FILE *out);

#endif	/* _hw_pci_h */

