/*
 * File hw_isa.h
 * Information handler for isa
 *
 * @(#) hw_isa.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_isa_h
#define _hw_isa_h

#include <sys/pnp.h>

extern const char	* const callme_isa[];
extern const char	short_help_isa[];

int	have_isa(void);
void	report_isa(FILE *out);
void	SmallRes(FILE *out, u_long resNum, const PnP_TagReq_t *tagReq);
void	LargeRes(FILE *out, u_long resNum, const PnP_TagReq_t *tagReq);

#endif	/* _hw_isa_h */

