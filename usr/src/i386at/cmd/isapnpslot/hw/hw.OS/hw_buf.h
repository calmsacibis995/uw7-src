/*
 * File hw_buf.h
 * Information handler for buf
 *
 * @(#) hw_buf.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_buf_h
#define _hw_buf_h

extern const char	* const callme_buf[];
extern const char	short_help_buf[];

int	have_buf(void);
void	report_buf(FILE *out);

#endif	/* _hw_buf_h */

