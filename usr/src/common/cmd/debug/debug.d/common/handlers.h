/*
 * $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/* Connects sig_handle.c with the C++ handlers */

#ifndef _Handlers_h
#define _Handlers_h

#ident	"@(#)debugger:debug.d/common/handlers.h	1.4"

#include <signal.h>

typedef void SIG_FUNC_TYP(int);
typedef SIG_FUNC_TYP *SIG_TYP;

#ifdef __cplusplus
extern "C" {
#endif

SIG_TYP poll_handler();
SIG_TYP inform_handler();
SIG_TYP fault_handler();
SIG_TYP internal_error_handler();
SIG_TYP suspend_handler();
SIG_TYP usr2_handler();

#ifdef __cplusplus
}
#endif


#endif
