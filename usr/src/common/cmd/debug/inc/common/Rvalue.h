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
#ifndef RVALUE_H
#define RVALUE_H
#ident	"@(#)debugger:inc/common/Rvalue.h	1.10"

#include "Vector.h"
#include "Itype.h"
#include "Fund_type.h"

class ProcObj;
class Buffer;
class Value;
class TYPE;

#define DEBUGGER_FORMAT 'z'
#define DEBUGGER_BRIEF_FORMAT ((char) -1)
#define PRINT_SIZE	1024	// max width of single formatted print
				// expression

class Rvalue
{
protected:
	Vector  raw_bytes;
	ProcObj	*_pobj;

		// the following conversion routines do the conversion in place
	int	cvt_to_SINT(Fund_type, Fund_type);
	int	cvt_to_UINT(Fund_type, Fund_type);
	int	cvt_to_FP(Fund_type, Fund_type);
	int	cvt_to_STR(Fund_type, Fund_type);

	void	set_value(Itype &val, Stype);
	int	assign(Value *dest);

		Rvalue(Rvalue&);
		Rvalue() { _pobj = 0; }
		Rvalue(void *, int, ProcObj *pobj = 0);
public:
	virtual	~Rvalue() {}

	virtual	Rvalue *clone();

	void	reinit(void *p, int n) { raw_bytes.clear().add(p, n); }

		// SINVALID if can't get as Itype member.
	Stype	get_Itype(Itype&);
	Stype	get_Itype(Itype&, Fund_type ft);

		// access functions
	void	*dataPtr()		{ return raw_bytes.ptr(); }
	int	isnull()		{ return raw_bytes.size() == 0;   }
	void	null()	{ raw_bytes.clear(); _pobj = 0;}
	ProcObj	*proc()	{ return _pobj; }

			// language specific functions
	virtual void	print(Buffer *, ProcObj * pobj = 0, char format = 0,
				char *format_str = 0, int verbose = 0);
	virtual TYPE	*type();
	virtual	int	size();
	virtual int	get_Stype(Stype& stype);

	int		get_Stype(Stype& stype, Fund_type);
};

#endif
