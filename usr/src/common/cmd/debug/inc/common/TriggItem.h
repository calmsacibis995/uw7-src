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
#ifndef TRIGGERITEM_H
#define TRIGGERITEM_H

#ident	"@(#)debugger:inc/common/TriggItem.h	1.7"

// TriggerItem is used to pass information about event expression
// between the expression evaluator and the event handler
// When a triggerItem is created, Place and Rvalue structures
// are allocated if possible.

#include "Place.h"
#include "Rvalue.h"
#include "Frame.h"
#include "ParsedRep.h"
#include "Iaddr.h"

#define NULL_SCOPE ((Iaddr)-1)

// values for flags field
#define T_REINIT_LVAL	1
#define T_FOREIGN	2	

class ProcObj;

class TriggerItem
{
	ParsedRep *node;	// ptr to root of expression (sub) tree ...
				//   ...describing this triggerItem
	int flags;

    public:
	Iaddr	scope;		// if local variable,start address of ...
				//   ... enclosing routine; otherwise null
	ProcObj	*pobj;		// item's process
	FrameId	frame;		// stack frame if automatic and ...
				//   ... active (on the stack); otherwise null
	TriggerItem()
		{	scope = NULL_SCOPE;
			pobj = 0;
			frame.null();
			node = 0;
			flags = 0;
		}
	TriggerItem(TriggerItem &t)
		{	pobj = t.pobj;
			scope = t.scope;
			frame = t.frame;
			node = t.node;
			flags = t.flags ;
		}

	~TriggerItem() {}

	void copyContext(TriggerItem &ti)
		{
			pobj = ti.pobj;
			frame = ti.frame;
			scope = ti.scope;
		}
	void setGlobalContext(ProcObj *l)
		{
			pobj = l;
			frame.null();
			scope = NULL_SCOPE;
		}
	ParsedRep* getNode()
		{ return node; }
	void setNode(ParsedRep *e)
		{ node = e; }
	int  reinitOnChange()
		{ return (flags & T_REINIT_LVAL); }
	void setReinit()
		{ flags |= T_REINIT_LVAL; }
	int  isForeign()
		{ return (flags & T_FOREIGN); }
	void setForeign()
		{ flags |= T_FOREIGN; }
	int getTriggerLvalue( Place& lval)
		{ return node->getTriggerLvalue(lval); }
	int getTriggerRvalue(Rvalue *&rval)
		{ return node->getTriggerRvalue(rval); }
#if DEBUG
	void	dump();
#endif

};

#endif /* TRIGGERITEM_H */
	
