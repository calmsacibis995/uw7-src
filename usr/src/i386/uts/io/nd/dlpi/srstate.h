#ident "@(#)srstate.h	11.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#ifndef _DLPI_SRSTATE_H
#define _DLPI_SRSTATE_H

#define LOCK_RT(rt) LOCK((rt)->lock_rt, plstr)
#define UNLOCK_RT(rt, sp) UNLOCK((rt)->lock_rt, sp)

/* Valid Values for the state field */
#define SR_GOT_STE	1
#define SR_GOT_ROUTE	2

/*  Valid values for the r_list field */
#define R_DISCO		1
#define R_INUSE		2

#endif	/* _DLPI_SRSTATE_H */

