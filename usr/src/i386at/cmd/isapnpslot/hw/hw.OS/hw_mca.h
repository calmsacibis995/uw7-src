/*
 * File hw_mca.h
 * Information handler for mca
 *
 * @(#) hw_mca.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_mca_h
#define _hw_mca_h

extern const char	* const callme_mca[];
extern const char	short_help_mca[];

int	have_mca(void);
void	report_mca(FILE *out);

#endif	/* _hw_mca_h */

