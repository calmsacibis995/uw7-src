/*
 * File eisa_data.h
 *
 * @(#) eisa_data.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _eisa_data_h
#define _eisa_data_h

typedef struct eisa_prod_s eisa_prod_t;
struct eisa_prod_s
{
    u_short	id;
    const char	*name;
    eisa_prod_t	*next;
};

typedef struct eisa_vendor_s eisa_vend_t;
struct eisa_vendor_s
{
    const char	*key;
    const char	*name;
    eisa_prod_t	*prod;
    eisa_vend_t	*next;
};

extern eisa_vend_t	*eisa_vendors;

int load_eisa_data(void);

#endif	/* _eisa_data_h */

