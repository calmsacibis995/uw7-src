#ident "@(#)ndstat.h	11.1"
#ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#define LIST_SHORT	0
#define LIST_NORMAL	1
#define LIST_LONG	2
#define LIST_SAP	3
#define LIST_SR		4

struct dlpi_stats *get_hw_independent_stats(int);

enum type {MDI=0,
        ODIDLPI,
        LLI
};

extern enum type type; 
extern int mdionly, odidlpionly, llionly;
