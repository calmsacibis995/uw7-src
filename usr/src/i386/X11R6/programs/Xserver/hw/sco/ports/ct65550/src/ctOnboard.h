/*
 *	@(#)ctOnboard.h	11.1	10/22/97	12:34:58
 *	@(#) ctOnboard.h 59.1 96/11/04 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ifndef _CT_ONBOARD_H
#define _CT_ONBOARD_H

#ident "@(#) $Id: ctOnboard.h 59.1 96/11/04 "

#define	CT_MEM_ALLOC(pScreen, w, h, st)	CT(OnboardAlloc)(	\
						(pScreen),	\
						(w), (h), (st),	\
						__FILE__, __LINE__)

#define	CT_MEM_FREE(pScreen, ptr)	CT(OnboardFree)(	\
						(pScreen),	\
						(ptr),		\
						__FILE__, __LINE__)

#define	CT_MEM_OFFSET(ptr, x, y)	CT(OnboardOffset)((ptr), (x), (y))
#define	CT_MEM_VADDR(ptr)		CT(OnboardVAddr)((ptr))
#define	CT_MEM_LOCK(ptr)		CT(OnboardLock)((ptr))
#define	CT_MEM_UNLOCK(ptr)		CT(OnboardUnlock)((ptr))

#define CT_MEM_AVAILABLE(pScreen)	CT(OnboardAvailable)((pScreen))

extern Bool CT(OnboardInit)();
extern void CT(OnboardClose)();

extern void *CT(OnboardAlloc)();
extern void CT(OnboardFree)();
extern void CT(OnboardLock)();
extern void CT(OnboardUnlock)();
extern unsigned long CT(OnboardOffset)();
extern CT_PIXEL *CT(OnboardVAddr)();
extern int CT(OnboardAvailable)();

#endif /* _CT_ONBOARD_H */
