/*
 * File hw_eisa.h
 * Information handler for eisa
 *
 * @(#) hw_eisa.h 65.1 97/06/11 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_eisa_h
#define _hw_eisa_h

extern const char	* const callme_eisa[];
extern const char	short_help_eisa[];

int	have_eisa(void);
void	report_eisa(FILE *out);

/*
 * The following are shared with ISA PnP
 */

const char	*eisa_cfg_name(const u_char *eisa_id);
void		parse_eisa_cfg(const char *cfg_name);
const char	*eisa_vendor_key(const u_char *eisa_id);
const char	*eisa_vendor_name(const char *vendor_key);
const char	*eisa_product_name(const char *vendor_key, u_short prod_id);

#endif	/* _hw_eisa_h */

