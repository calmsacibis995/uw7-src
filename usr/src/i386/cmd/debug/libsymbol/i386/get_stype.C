/* $Copyright: $
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
#ident	"@(#)debugger:libsymbol/i386/get_stype.C	1.9"

#include "Fund_type.h"
#include "TYPE.h"
#include "Itype.h"
#include "Tag.h"

// machine dependent routine

int
TYPE::get_Stype(Stype& stype)
{
    switch( _form )
    {
    case TF_user:
	Tag ut_tag;
	ut_tag = symbol.tag();
	
	switch( ut_tag )
	{
	case t_pointertype:
	case t_reftype:
		stype = Saddr;	// for   register T *p;
				// ...   eval p
		break;
	case t_enumtype:
		stype = Sint4;
		break;
	case t_structuremem:
		Attribute *attrPtr;
		if( (attrPtr=symbol.attribute(an_type)) && 
		               attrPtr->form==af_fundamental_type )
		{
			TYPE t = attrPtr->value.fund_type;
			return t.get_Stype(stype);
		}
		stype = SINVALID;
		return 0;
	default:
		stype = SINVALID;
		return 0;
	}
	break;
    case TF_fund:
	return ::get_Stype(stype, ft);
    default:
	stype = SINVALID;
	return 0;
    }
    return 1;
}

int
get_Stype(Stype &stype, Fund_type ft)
{
	switch (ft)
	{
	case ft_char:
	case ft_schar:   stype = Schar;		break;
	case ft_boolean:
	case ft_uchar:   stype = Suchar;	break;
	case ft_short:   stype = Sint2;		break;
	case ft_sshort:  stype = Sint2;		break;
	case ft_ushort:  stype = Suint2;	break;
	case ft_int:     stype = Sint4;		break;
	case ft_sint:    stype = Sint4;		break;
	case ft_uint:    stype = Suint4;	break;
	case ft_long:    stype = Sint4;		break;
	case ft_slong:   stype = Sint4;		break;
	case ft_ulong:   stype = Suint4;	break;
#if LONG_LONG
	case ft_long_long:    stype = Sint8;		break;
	case ft_slong_long:    stype = Sint8;		break;
	case ft_ulong_long:    stype = Suint8;		break;
#endif
	case ft_pointer: stype = Saddr;		break;
	case ft_sfloat:  stype = Ssfloat;	break;
	case ft_lfloat:  stype = Sdfloat;	break;
	case ft_xfloat:  stype = Sxfloat;	break;
	case ft_void:    stype = Saddr;	 	break;
	case ft_string:  stype = Sdebugaddr;	break;
	case ft_none:
	default:	stype = SINVALID; return 0;
	}
	return 1;
}
