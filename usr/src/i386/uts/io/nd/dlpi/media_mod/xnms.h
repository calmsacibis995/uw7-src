#ident "@(#)xnms.h	7.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1995.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#ifndef _XNMS_H
#define _XNMS_H

void	bcopy(void *, void *, int);
int	qaddl(void *, int);
int	qincl(void *);
int	kstrlen(const char *s);

#ifdef DEBUG
#   define STATIC
#   define CMN_ERR		cmn_err
#   define ASSERT(EX) \
	if (!(EX)) \
	  cmn_err(CE_PANIC,"Assert fails file=%s, line=%d", __FILE__, __LINE__)
#else
#   define STATIC	static
#   define CMN_ERR		0 && cmn_err
#   define ASSERT(x)
#endif

#endif	/* _XNMS_H */

