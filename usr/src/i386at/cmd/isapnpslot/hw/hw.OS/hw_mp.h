/*
 * File hw_mp.h
 * Information handler for mp
 *
 * @(#) hw_mp.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_mp_h
#define _hw_mp_h

extern const char	* const callme_mp[];
extern const char	short_help_mp[];

int	have_mp(void);
void	report_mp(FILE *out);

#endif	/* _hw_mp_h */

