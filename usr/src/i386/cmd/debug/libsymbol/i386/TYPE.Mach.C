/* $Copyright:	$
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

#ident	"@(#)debugger:libsymbol/i386/TYPE.Mach.C	1.5"

#include "Fund_type.h"
#include "TYPE.h"
#include "Interface.h"

int
TYPE::isBitFieldSigned()
{
    	Fund_type ft;
    	if( !fund_type(ft) )
    	{
		printe(ERR_internal, E_ERROR,
			"TYPE::isBitFldSigned", __LINE__);
		return 0;
	}

	switch (ft)
	{ 
	case ft_schar:
	case ft_sshort:
	case ft_sint:
	case ft_slong:
#if LONG_LONG
	case ft_slong_long:
#endif
		return 1;
	case ft_char:
	case ft_boolean:
	case ft_short:
	case ft_int:
	case ft_long:
	case ft_uchar:
	case ft_ushort:
	case ft_uint:
	case ft_ulong:
#if LONG_LONG
	case ft_long_long:
	case ft_ulong_long:
#endif
		return 0;
	case ft_pointer:
	case ft_sfloat:
	case ft_lfloat:
	case ft_xfloat:
	case ft_none:
	default:
		printe(ERR_internal, E_ERROR, "TYPE::isBitFldSigned", __LINE__);
		return 0;
	}
	// not reached
}
