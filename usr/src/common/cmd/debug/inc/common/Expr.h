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
#ifndef EXPR_H
#define EXPR_H
#ident	"@(#)debugger:inc/common/Expr.h	1.13"


// Class: Expr
//	Expr is a language independent container class for language expressions.
//	(It should be language independent, but there are a few things that
//		are really C++ specific - user_derived_type, create_type for EH)
//	Expr objects are created for any command that needs to
//		- evaluate an expression (print, set, if, while, dump,
//			and stop expressions),
//		- get the value of a symbol (symbols), or
//		- get the type of an expression or symbol
//			(whatis, symbols, exception events)
//	Most Expr objects are evaluated and deleted immediately,
//		but for stop expressions they must be kept around
//		for the life of the event
//
// Public Member Functions:
//	Expr 	- constructors; non-copy constructor is used to instantiate
//		  an expr object.  This constructor initializes the string
//		  expression representation and the langauge discriminate.
//	eval	- evaluates an expression by invoking the eval member function
//		  of the object pointed to by ParsedRep.  
//		  A return value of zero 
//		  indicates the evaluation failed, while non-zero indicates
//		  the evaluation was sucessful.  Upon sucessful return, the 
//		  result of the evaluation is available through calls to rvalue
//		  and lvalue.
//	rvalue	- returns, in its parameter rval, the rvalue of a previous 
//		  expression evaluation.  If no
//		  rvalue is available, then the function call returns zero.
//		  Otherwise, non-zero is returned.
//	lvalue	- returns, in its parameter lval, the lvalue of a previous 
//		  expression evaluation. If no
//		  lvalue is available, then the function call returns zero.
//		  Otherwise, non-zero is returned.
//	string	- returns a pointer to the character string representation
//		  of the expression.
//	triggerList - traverses the parsed expression represention and 
//		  returns a list of TriggerItem objects.  The objects provide 
//		  the location and (possibibly null) value of each of the
//		  data items that can affect the value of the expression.
//	exprIsTrue - evaluates the expression and returns a non-zero 
//		  value if the expression value constitutes a true value in
//		  the expression's source langauge.  Otherwise, the function
//		  returns zero.
//

// Assumptions and restrictions:
//	Expr should be used as the base class for a language dependent class.
//		new instances of Expr should be created using the function new_expr,
//		which will check for the current language, rather than using new Expr
//		directly, or allocating an Expr on the stack.
//	The Rvalue returned by the function rvalue is deleted when the Expr class
//		is destroyed.  A copy should be made if it is needed after that.
//	If is the responsibility of the calling function to ensure that interrupts
//		are allowed while the expression is being evaluated, printed, etc.

#include "Iaddr.h"
#include "Language.h"
#include "ParsedRep.h"
#include "Symbol.h"

class  Frame;
class  Rvalue;
struct Place;
class  Symbol;
class  Value;
class  ProcObj;
class  Resolver;
class  Context;
class  TYPE;
class  Vector;
class  Buffer;

// flags used in flags field
#define	E_IS_EVENT	0x01
#define E_IS_SYMBOL	0x02
#define E_IS_TYPE	0x04	// called from whatis, may be type name without a value
#define E_VERBOSE	0x80
#define E_OBJ_STOP_EXPR	0x10	// evaluating an object expression for StopLoc,
				// return information in a vector
#define MAX_QUALIFIERS	4
#define NULL_QUALIFIER ((char *)-1)

class Expr
{
protected:
	const char	*estring;	// expression text
	Symbol		symbol;
	ProcObj		*pobj;
	Frame		*frame;
	Language	lang;		
	ParsedRep	*etree;		// parsed representation of expression
	Value		*value;
	int		flags;		// event or no-resolve expr
	Vector		**pvector;	// holds info. for overloaded or virtual functions

	virtual int 	parse(Resolver *) = 0;	// language dependent
	virtual Value 	*eval();	// language dependent
	Context		*get_qualified_context(char *[], int nqualifiers);
	int		get_func_qualifier(char *fname, Context *);
	int		get_frame_context(long nbr, Context *);
	int		get_block_context(long nbr, Context *);
	void		reset_context(Context *, ProcObj *, Frame *);

public:
			Expr(Language l, const char *e, ProcObj* pobj, int isEvent = 0, int isType = 0);
			Expr(Language l, const char *e, ProcObj *pobj, ParsedRep *);
			Expr(Language l, Symbol&, ProcObj* pobj);
	virtual		~Expr();

	// Member access methods
	const char 	*string() { return estring; }

	// General expression evaluation methods
	int 		eval(ProcObj *pobj, Iaddr pc=~0,Frame *frame=0, int verbose=0);
	int		rvalue(Rvalue *&rval);
	int		lvalue(Place &lval);
	int		print_type(int multiple);
	int		print_symbol_type(Buffer *);
	int		print_type(Buffer *);
	virtual void	use_derived_type() = 0;
#if EXCEPTION_HANDLING
	virtual TYPE	*create_user_type(Frame *frame) = 0;
	virtual TYPE	*create_program_type(Frame *frame, int flags, int &err) = 0;
#endif

	// Event interface methods
	int		triggerList(Iaddr pc, List &valueList);
	int		exprIsTrue(ProcObj *pobj, Frame *frame)
				{ return etree->exprIsTrue(lang, pobj, frame); }
	Expr		*copyEventExpr(List&, List&,  ProcObj*);
	void		objectStopExpr(Vector **v)
				{ pvector = v; flags |= E_OBJ_STOP_EXPR; }
};

Expr	*new_expr(const char *e, ProcObj *pobj, int isEvent = 0, int isType = 0,
			Language l = UnSpec);
Expr	*new_expr(const char *e, ProcObj *pobj, ParsedRep *, Language l = UnSpec);
Expr	*new_expr(Symbol&, ProcObj *pobj, Language l = UnSpec);

#endif
