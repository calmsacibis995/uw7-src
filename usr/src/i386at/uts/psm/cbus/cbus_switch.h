#ifndef _PSM_CBUS_CBUS_SWITCH_H
#define _PSM_CBUS_CBUS_SWITCH_H

#ident	"@(#)kern-i386at:psm/cbus/cbus_switch.h	1.1"
#ident	"$Header$"

/*++

Copyright (c) 1992-1997  Corollary Inc.

Header Name:

    cbus_switch.h

Abstract:

    this module provides the definition for the
	various function switch tables.

--*/

typedef struct _cbus_functions_t {
	ms_bool_t		(*Present)();
	void			(*InitializeCpu)();
	void			(*EnableInterrupt)();
	void			(*DisableInterrupt)();
	void			(*EoiInterrupt)();
	void			(*IpiCpu)();
	ms_rawtime_t	(*TimeGet)();
	void			(*StartCpu)();
	void			(*OffLinePrep)();
	void			(*OffLineSelf)();
	void			(*ShowState)();
	ms_bool_t		(*ServiceTimer)();
	void			(*ServiceXint)();
	void			(*ServiceSpurious)();
	void			(*PokeLed)();
} CBUS_FUNCTIONS_T, *PCBUS_FUNCTIONS;

typedef struct _cbus_switch_t {
	int				(*Present)();
	void			(*ParseRrd)();
	void			(*Setup)();
	void			(*InitializeCpu)();
	void			(*EnableInterrupt)();
	void			(*DisableInterrupt)();
	void			(*EoiInterrupt)();
	void			(*IpiCpu)();
	ms_rawtime_t	(*TimeGet)();
	void			(*StartCpu)();
	void			(*OffLinePrep)();
	void			(*OffLineSelf)();
	void			(*ShowState)();
	ms_bool_t		(*ServiceTimer)();
	void			(*ServiceXint)();
	void			(*ServiceSpurious)();
	void			(*PokeLed)();
} CBUS_SWITCH_T, *PCBUS_SWITCH;

#endif // _PSM_CBUS_CBUS_SWITCH_H
