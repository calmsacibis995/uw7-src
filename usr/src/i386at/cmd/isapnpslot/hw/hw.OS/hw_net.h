/*
 * File hw_net.h
 * Information handler for net
 *
 * @(#) hw_net.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_net_h
#define _hw_net_h

extern const char	* const callme_net[];
extern const char	short_help_net[];

int	have_net(void);
void	report_net(FILE *out);

#endif	/* _hw_net_h */

