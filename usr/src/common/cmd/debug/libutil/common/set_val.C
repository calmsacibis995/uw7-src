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
#ident	"@(#)debugger:libutil/common/set_val.C	1.14"

#include "Interface.h"
#include "Parser.h"
#include "Expr.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Interface.h"
#include "str.h"
#include "utility.h"
#include "global.h"
#include <string.h>

#include "Buffer.h"
#include "Dbgvarsupp.h"

void
expand_name_rhs(Exp *exp, Buffer *buf)
{
	buf->clear();
	if ((*exp)[1]) 
	{
		printe(ERR_concat, E_ERROR);
		return;
	}
	char *p = (*exp)[0];
	if (p[0] == '%' || p[0] == '$' || p[0] == '"')
		buf->add(p);
	else
	{
		buf->add('"');
		buf->add(p);
		buf->add('"');
	}
	return;
}

void
expand_string_list_rhs(ProcObj * pobj, Exp *exp, int report_errors,
	Buffer *buf)
{
	buf->clear();
	buf->add('"');
	char * p;
	for ( int j = 0; p = (*exp)[j]; j++) 
	{
		if (p[0] == '%' || p[0] == '$')
		{
			// This is reinstantiated for each variable on the rhs.
			// This is estimated to be less costly than
			// doing it once outside the loop since vars
			// are less often used in the set command.
			var_table.set_context(
			      pobj, pobj ? pobj->curframe() : 0, 1, 1, 1);
			var_table.Find(p);
			char * value = var_table.Value();
			if (!value) 
			{
		 		if (report_errors)
					printe(ERR_eval_fail_expr, 
						E_ERROR, p);
				buf->clear();
				return;
			}
			buf->add(value);
		}
		else if (p[0] == '"')
		{
			int len = strlen(p);
			p[len-1] = 0;
			buf->add(p+1);
			p[len-1] = '"';
		}
		else
		{
			if (report_errors) 
				printe(ERR_string_req, E_ERROR);
			buf->clear();
			return;
		}
	}
	buf->add('"');
	return;
}

void
expand_expr_rhs(Exp *exp, Buffer *buf)
{
	buf->clear();
	char * p;
	for ( int j = 0; p = (*exp)[j]; j++) 
	{
		buf->add(p);
		buf->add(' ');
	}
	return;
}

int
set_val(Proclist * procl, char *lval, Exp *exp, int verbose)
{
	int 		single = 1;
	ProcObj		*pobj;
	plist		*list;
	plist		*list_head = 0;
	int		ret = 1;
	Buffer		*buf = buf_pool.get();
	Buffer		*exprbuf = 0;

	if (procl)
	{
		single = 0;
		list_head = list = proglist.proc_list(procl);
		pobj = list++->p_pobj;
	}
	else
	{
		pobj = proglist.current_object();
	}
	sigrelse(SIGINT);
	do
	{

		Process	*proc = 0;
		// we only stop all if we have a thread -
		// the lower-level print routines catch whether
		// the current object needs to be stopped - for
		// threads we must also have entire process stopped
		if (pobj && pobj->obj_type() == pobj_thread)
		{
			proc = pobj->process();
			if (!proc->stop_all())
			{
				ret = 0;
				proc = 0;
				continue;
			}
		}
		char * rhs = (*exp)[0];

		// if lval is predefined string debug var...
		if (lval && lval[0] == '%')
		{
			// name vars
			// This allows the user to omit quotes for
			// names that the expression evaluator would
			// normally choke on.

			dbg_vtype	vtype = dbg_var(lval);
			switch(vtype) 
			{
				case Dis_mode_v:
				case Follow_v:
				case Frame_num_v:
				case Func_v:
				case Lang_v:
				case Mode_v:
				case Proc_v:
				case Program_v:
				case Redir_v:
				case Stack_bounds_v:
				case Thread_change_v:
				case Thread_v:
				case Verbose_v:
				case Wait_v:
					expand_name_rhs(exp, buf);
					rhs = (char *)*buf;
					break;
				case Glob_path_v:
				case List_file_v:
				case Path_v:
				case Prompt_v:
					// string_list vars
					expand_string_list_rhs(pobj,
						exp,1, buf);
					rhs = (char *)*buf;
					break;
				default:
					break;
			}
		}
		else if (lval && lval[0] == '$')
		{
			// Allow string_list
			expand_string_list_rhs(pobj, exp, 0, buf);
			rhs = (char *)*buf;
			// but, if not, let expr evaluator try
			if (!rhs || !*rhs)
			{
				expand_expr_rhs(exp, buf);
				rhs = (char *)*buf;
			}
		}

		else if ((*exp)[1])
		{
			expand_expr_rhs(exp, buf);
			rhs = (char *)*buf;
		}

		if (!rhs || !*rhs) 
		{
			if (proc)
				proc->restart_all();
			ret = 0;
			continue;
		}

		char	*e;
		if (lval)
		{
			if (!exprbuf)
				exprbuf = buf_pool.get();
			// MORE - for now, C dependent
			exprbuf->clear();
			exprbuf->add(lval);
			exprbuf->add(" = ");
			exprbuf->add(rhs);
			e = (char *)*exprbuf;
		}
		else
			e = rhs;
		Expr *expr = new_expr(e, pobj);
		if (!expr->eval(pobj, ~0, 0, verbose))
		{
			ret = 0;
		}
		delete expr;
		if (proc)
			if (!proc->restart_all())
				ret = 0;
	}
	while(!single && ((pobj = list++->p_pobj) != 0)
		&& !prismember(&interrupt, SIGINT));
	sighold(SIGINT);
	if (exprbuf)
		buf_pool.put(exprbuf);
	buf_pool.put(buf);
	if (list_head)
		proglist.free_plist(list_head);
	return ret;
}
