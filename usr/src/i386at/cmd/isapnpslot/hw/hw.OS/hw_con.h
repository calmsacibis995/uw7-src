/*
 * File hw_con.h
 * Information handler for Console
 *
 * @(#) hw_con.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_con_h
#define _hw_con_h

extern const char	* const callme_con[];
extern const char	short_help_con[];

int	have_con(void);
void	report_con(FILE *out);

#endif	/* _hw_con_h */

