/*
 * File pcmcia_data.h
 *
 * @(#) pcmcia_data.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _pcmcia_data_h
#define _pcmcia_data_h

typedef struct pcmcia_prod_s pcmcia_prod_t;
struct pcmcia_prod_s
{
    u_short		id;
    const char		*name;
    pcmcia_prod_t	*next;
};

typedef struct pcmcia_vend_s pcmcia_vend_t;
struct pcmcia_vend_s
{
    const char		*key;
    const char		*name;
    pcmcia_prod_t	*prod;
    pcmcia_vend_t	*next;
};

extern pcmcia_vend_t	*pcmcia_vendors;

int load_pcmcia_data(void);

#endif	/* _pcmcia_data_h */

