/*
 * File pci_data.h
 *
 * @(#) pci_data.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _pci_data_h
#define _pci_data_h

typedef struct pci_prod_s pci_prod_t;
struct pci_prod_s
{
    u_short	id;
    u_char	rev;
    const char	*name;
    pci_prod_t	*next;
};

typedef struct pci_vend_s pci_vend_t;
struct pci_vend_s
{
    u_short	id;
    const char	*name;
    pci_prod_t	*prod;
    pci_vend_t	*next;
};

extern pci_vend_t	*pci_vendors;

int load_pci_data(void);

#endif	/* _pci_data_h */

