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

//
// Class: ParsedRep 
//
// Member Functions (public):
//      eval	- pure virtual function called to invoke the appropriate
//		  derived class eval function.
//      triggerList - is called to handle a stop expression.  It traverses the
//                parsed expression represention and returns a list of Value
//                objects.  The value objects provide the the location and
//                (possibly null) value of each of the  expression's data
//                objects.
//	eventIsTrue -
//	clone
//
#ifndef PARSEDREP_H
#define PARSEDREP_H
#ident	"@(#)debugger:inc/common/ParsedRep.h	1.11"

#include "List.h"
#include "ProcObj.h"

class	Place;
class	Value;
class	Resolver;
class	Vector;
class	Buffer;

class ParsedRep
{
public:
	ParsedRep	*next;
		ParsedRep() { next = 0; }
	virtual	~ParsedRep() { delete next; }

//
// Interface routines required by all language expression classes
//
	virtual	Value	*eval(Language, ProcObj *, Frame *, int, Vector **) = 0;
	virtual ParsedRep *clone() = 0;	// make deep copy
	virtual	int	triggerList(Language, ProcObj *, Resolver *, List &, Value*&) = 0;
	virtual	int	exprIsTrue(Language, ProcObj *, Frame *) = 0;
	virtual int	getTriggerLvalue(Place&) = 0;
	virtual int	getTriggerRvalue(Rvalue *&) = 0;
	virtual ParsedRep *copyEventExpr(List&, List&, ProcObj*) = 0;
	virtual int	print_type(ProcObj *, const char *) = 0;
	virtual int	print_type(Buffer *, ProcObj *) = 0;
	virtual int	print_symbol_type(Buffer *, ProcObj *) = 0;
};

#endif /* PARSEDREP_H */
