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
#ifndef TYPE_h
#define TYPE_h
#ident	"@(#)debugger:inc/common/TYPE.h	1.15"

#include "Symbol.h"
#include "Itype.h"
#include "Fund_type.h"
#include "Attribute.h"
#include "Language.h"

class	Frame;
class	ProcObj;
class	Place;
class	Buffer;
struct	a_type;

enum Type_form
{
	TF_fund,  // char, short, int, unsigned int, ...
	TF_user,   // ptr, array, struct, enum, ...
	TF_C_lang,	// language-specific type info in derived type
};

class TYPE
{
protected:
	Type_form	_form;
	union
	{
		Fund_type	ft;   	  // meaningful iff form == TF_fund.
		a_type		*c_type;  // meaningful iff form == TF_C_lang
	};

	Symbol			symbol; // meaningful iff form == TF_user.

public:
			TYPE()  { null(); }
			TYPE(Fund_type fundtype) { _form = TF_fund; 
						ft = fundtype; }
			TYPE(const Symbol &sym)	{ _form = TF_user;
						symbol = sym; }
			TYPE(a_type *ptr)  { _form = TF_C_lang; c_type = ptr; }

	virtual		~TYPE();

	TYPE		&operator=(Fund_type);	// init as a fundamental type
	TYPE		&operator=(Symbol &);	// init as a user defined type
	TYPE		&operator=(a_type *tptr)
				{ _form = TF_C_lang;
				  c_type = tptr;
				  return *this; }

	void		null()  { _form = TF_fund; ft = ft_none; } // make null.
	int		isnull() { return _form == TF_fund && ft == ft_none; }

	Type_form	form() { return _form; }
	int		fund_type(Fund_type&) const;	// return 1 iff form TF_fund.
	int		user_type(Symbol&) const;	// return 1 iff form TF_user.
	int		is_C_type(a_type *&) const;	// return 1 iff form TF_C_lang.
	
	// language specific routines
	virtual int	get_base_type(TYPE *);
	virtual int	get_Stype(Stype& stype);
	virtual int	operator==(TYPE&);
	virtual int	size();			// size in bytes
	virtual TYPE	*clone();		// make a copy, including derived types
	virtual TYPE	*clone_type();	// make a null copy of appropriate derived type
	virtual	int	is_assignable(TYPE *);
	int		print(Buffer *result, ProcObj *pobj);

	int		operator!=(TYPE& t) { return !(this->operator==(t)); }

	// General type query rtns
	int		isClass()
				{ return (_form==TF_user && 
					(symbol.tag()==t_structuretype
						|| symbol.tag()==t_classtype)); }
	int		isStruct()
				{ return (_form==TF_user && 
					symbol.tag()==t_structuretype); }
	int		isBitFieldSigned();		// machine dependent

#if DEBUG
	void dumpTYPE();
#endif
};

#if DEBUG
    void dumpAttr(Attribute *AttrPtr, int indent);
#endif

int get_Stype(Stype &stype, Fund_type);

// Language specific comparison functions
int CC_type_compare(Language, Symbol &, Symbol &);

#endif
