/*
 * File mca_data.h
 *
 * @(#) mca_data.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _mca_data_h
#define _mca_data_h

typedef struct mca_prod_s mca_prod_t;
struct mca_prod_s
{
    u_short	id;
    const char	*name;
    mca_prod_t	*next;
};

typedef struct mca_vend_s mca_vend_t;
struct mca_vend_s
{
    const char	*key;
    const char	*name;
    mca_prod_t	*prod;
    mca_vend_t	*next;
};

extern mca_vend_t	*mca_vendors;

int load_mca_data(void);

#endif	/* _mca_data_h */

