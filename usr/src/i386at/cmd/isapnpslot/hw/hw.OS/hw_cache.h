/*
 * File hw_cache.h
 * Information handler for cache
 *
 * @(#) hw_cache.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_cache_h
#define _hw_cache_h

extern const char	* const callme_cache[];
extern const char	short_help_cache[];

int	have_cache(void);
void	report_cache(FILE *out);

#endif	/* _hw_cache_h */

