/*
 * File hw_cpu.h
 * Information handler for cpu
 *
 * @(#) hw_cpu.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_cpu_h
#define _hw_cpu_h

extern const char	* const callme_cpu[];
extern const char	short_help_cpu[];

int	have_cpu(void);
void	report_cpu(FILE *out);

int	show_num_cpu(FILE *out);

#endif	/* _hw_cpu_h */

