#ident	"@(#)debugger:libutil/common/symbols.C	1.27"

#include "utility.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Proctypes.h"
#include "Frame.h"
#include "Interface.h"
#include "Buffer.h"
#include "Msgtab.h"
#include "Symbol.h"
#include "Symtab.h"
#include "global.h"
#include "Tag.h"
#include "NameList.h"
#include "Iaddr.h"
#include "Parser.h"
#include "Rvalue.h"
#include "Source.h"
#include "Expr.h"
#include "Language.h"
#include "Dbgvarsupp.h"
#include "RegExp.h"
#include "NewHandle.h"
#include "str.h"
#include "CCnames.h"
#if EXCEPTION_HANDLING
#include "Exception.h"
#endif

#include <signal.h>
#include <string.h>
#include <stdlib.h>

struct SymEntry
{
	Symbol		*sym;
	const char	*location;
	long		line;
			SymEntry(Symbol &s, const char *loc, long l) {
				sym = new Symbol(s); location = loc;
				line = l; }
			~SymEntry() { delete sym; }
};

int
cmp(SymEntry **sym1, SymEntry **sym2)
{
	return(strcmp((*sym1)->sym->name(), (*sym2)->sym->name()));
}


class SymList
{
	SymEntry	**list;
	int		count;
	int		size;
public:
			SymList();
			~SymList();
	void		add(Symbol&, const char *location, long line);
	int		printall(ProcObj *, int mode);
	void		clear();
};

#define LISTENT	200

SymList::SymList()
{
	// initialize list with default number of entries
	// we never shrink list
	if ((list = (SymEntry **)calloc(LISTENT, 
		sizeof(SymEntry *))) == 0)
	{
		newhandler.invoke_handler();
		return;
	}
	size = LISTENT;
	count = 0;
}

SymList::~SymList()
{
	clear();
	free(list);
}

void
SymList::clear()
{
	register SymEntry	**lptr = list;
	while(*lptr)
	{
		delete(*lptr);
		*lptr = 0;
		lptr++;
	}
	count = 0;
}

void
SymList::add(Symbol &s, const char *loc, long line)
{
	SymEntry	*sym = new SymEntry(s, loc, line);
	
	if (count >= (size - 1))
	{
		// always leave a null entry at end
		if ((list = (SymEntry **)realloc((char *)list,
			(sizeof(SymEntry *) * (size + LISTENT)))) == 0)
		{
			size = 0;
			newhandler.invoke_handler();
		}
		memset(list+size, 0, sizeof(SymEntry *) * LISTENT);
		size += LISTENT;
	}
	list[count++] = sym;
}

static int
print_sym(ProcObj *pobj, Symbol s, const char *location, 
	long line, int mode, Buffer *buf1, Buffer *buf2)
{
	char	line_str[6];
	char	*l;
	int	bad_val = 0;
	int	assumed;
	
	if (line)
	{
		sprintf(line_str, "%d", line);
		l = line_str;
	}
	else
		l = "";
	assumed = s.type_assumed(1);

	Expr *exp = 0;
	const char *sym_name = s.name();
	if (!sym_name || !*sym_name)
		sym_name = "<unnamed>";

	buf1->clear();
	buf2->clear();
	if (mode & SYM_VALUES)
	{
		Rvalue *rval;
		char   *val;
		// get type and expression first so any error messages come
		// out on a separate line and don't mess up the output
		exp = new_expr(s, pobj);
		if (!exp->eval(pobj) || !exp->rvalue(rval))
		{
			bad_val = 1;
			val = (char *)Mtable.format(ERR_evaluation);
		}
		else
		{
			rval->print(buf1, pobj, DEBUGGER_BRIEF_FORMAT);
			val = buf1->size() ? (char *)*buf1 : "";
		}
		if (mode & SYM_TYPES)
		{
			char	*type;
			exp->print_symbol_type(buf2);
			type = buf2->size() ? (char *)*buf2 : "";

			if (assumed)
				printm(MSG_symbol_type_val_assume, 
					sym_name, location, l,
					type, val);
			else
				printm(MSG_symbol_type_val, sym_name, 
					location, l,
					type, val);
		}
		else
		{
			if (assumed)
				printm(MSG_symbol_val_assume, sym_name, location, l, val);
			else
				printm(MSG_symbol_val, sym_name, location, l, val);
		}
	}
	else if (mode & SYM_TYPES)
	{
		char	*type;
		exp = new_expr(s, pobj);
		exp->print_symbol_type(buf2);
		type = (char *)*buf2;

		if (assumed)
			printm(MSG_symbol_type_assume, sym_name, 
				location, l, type);
		else
			printm(MSG_symbol_type, sym_name, location, 
				l, type);
	}
	else
		printm(MSG_symbol, s.name(), location, l);
	delete exp;
	return (!bad_val);
}

int
SymList::printall(ProcObj *pobj, int mode)
{
	int		ret = 1;
	SymEntry	*symentry, **lptr;
	Buffer		*buf1;
	Buffer		*buf2;

	if (!count)
		return 1;

	buf1 = buf_pool.get();
	buf2 = buf_pool.get();

	qsort(list, count, sizeof(SymEntry *), (int (*)(const void *,
		const void *))cmp);

        lptr = list;
	symentry = *lptr;
	for ( ; symentry; symentry = *(++lptr))
	{
		if (!print_sym(pobj, *(symentry->sym),
			symentry->location, symentry->line,
			mode, buf1, buf2))
			ret = 0;
		if (prismember(&interrupt, SIGINT))
			break;
	}
	buf_pool.put(buf1);
	buf_pool.put(buf2);
	return ret;
}


// print out global names from the a.out, or from a shared object

static int
get_globals(ProcObj *pobj, const char *obj_name,  int mode, PatternMatch *patmatch, SymList *list)
{
	const char	*s_name;
	NameEntry	*np;
	Symtab		*stp;
	Symbol		sym;
	int		ret = 1;
	Buffer		*buf1 = 0;
	Buffer		*buf2 = 0;

	if (!list)
	{
		buf1 = buf_pool.get();
		buf2 = buf_pool.get();
	}

	if (!obj_name)
	{
		Iaddr	pc = pobj->curframe()->pc_value();
		if ((stp = pobj->find_symtab(pc)) == 0)
		{
			printe(ERR_no_sym_info, E_ERROR, 
				pobj->obj_name());
		}
		else
			obj_name = pobj->object_name(pc);
	}
	else 
	{
		if ((stp = pobj->find_symtab(obj_name)) == 0)
			printe(ERR_cant_find_symtab, E_ERROR, 
				obj_name);
	}
	if (!stp)
	{
		if (buf1)
		{
			buf_pool.put(buf1);
			buf_pool.put(buf2);
		}
		return 0;
	}

	for(np = stp->first_global(); np; np = stp->next_global())
	{
		s_name = np->name();
		if (!s_name || (patmatch && !patmatch->match(s_name)))
			continue;
		sym = stp->global_symbol(np);

		if (list)
			list->add(sym, obj_name, 0);
		else 
			if (!print_sym(pobj, sym, obj_name, 0,
				mode, buf1, buf2))
				ret = 0;
		if (prismember(&interrupt, SIGINT))
			break;
	}
	if (buf1)
	{
		buf_pool.put(buf1);
		buf_pool.put(buf2);
	}
	return ret;
}

// print out the local names visible from the current scope

static char *file_loc;

static const char *
find_inline_name(Symbol scope)
{
	while (!scope.isnull())
	{
		if (scope.tag() == t_inlined_sub)
		{
			Symbol abstract = scope.arc(an_abstract);
			if (abstract.isnull())
				return 0;
			CC_parse_name nm(abstract);
			return nm.full_name();
		}
		scope = scope.parent();
	}
	return 0;
}

static int
inner_names(ProcObj * pobj, Symbol& inner_scope, int mode, 
	PatternMatch *patmatch, const char *filename, SymList *symlist)
{
	Symbol		scope, s;
	const char	*s_name;
	Tag		tag;
	int		flevel = 0;
	Source		source;
	const char	*funcname;
	const char	*fname;
	const char	*location;
	const char	*scope_name = 0;

	// find the current source file - needed to print the location
	for(scope = inner_scope; !scope.isnull(); scope = scope.parent())
	{
		tag = scope.tag(); // returns t_none if no tag.
		if (tag == t_sourcefile)
		{
			scope.source(source);
			fname = scope.name();
			break;
		}
		else if (IS_ENTRY(tag))
			funcname = scope.name();
	}

	// if printing locals, work from the inner scope out
	// if printing file statics only, scope is already set at the
	// file level from the preceeding for loop
	if (mode&SYM_LOCALS)
		scope = inner_scope;
	else if (scope.isnull())
	{
		printe(ERR_no_cur_src_obj, E_WARNING, pobj->obj_name());
		return 0;
	}

	for(; !scope.isnull(); scope = scope.parent())
	{
		long	line = 0;
		tag = scope.tag(); // returns t_none if no tag.

		if (tag == t_sourcefile && !(mode&SYM_FILES))
			break;

		// the tag must be one of these if it has a scope
		switch (tag)
		{
		case t_sourcefile:
			flevel = 1;
			break;

		case t_entry:
		case t_subroutine:
		case t_extlabel:
			break;

		case t_block:
		case t_try_block:
		case t_catch_block:
			if (current_context_language(pobj) == CPLUS)
				scope_name = find_inline_name(scope.parent());
			break;

		case t_inlined_sub:
			if (current_context_language(pobj) == CPLUS)
				scope_name = find_inline_name(scope);
			break;

		// don't need to scan class/struct/union members
		case t_classtype:
		case t_structuretype:
		case t_uniontype:
			continue;

		default:
			printe(ERR_unexpected_tag, E_WARNING, tag);
		    	continue;
		}

		s = scope.child();
		if (s.isnull())
			goto finish_scope;

		// The location for a file symbol is the file name
		// For a local, it's the function name and line
		// where the line appears only if the symbol is declared in
		// in a inner block.  The location is the same for each
		// symbol in that scope
		if (flevel)
		{
			if (filename && (strcmp(filename, fname) != 0))
			{
				// file symbols requested in different
				// file
				scope = pobj->first_file();
				for(; !scope.isnull(); 
					scope = pobj->next_file())
				{
					fname = scope.name();
					if (strcmp(fname, filename) == 0)
						break;
				}
				if (scope.isnull())
				{
					printe(ERR_no_source, E_ERROR, 
						filename);
					return 0;
				}
				s = scope.child();
				if (s.isnull())
					break;
			}
			location = file_loc
				= new char[strlen(fname) + 1];
			strcpy(file_loc, fname);
		}
		else
		{
			if (scope_name)
				location = scope_name;
			else
				location = funcname;
			Iaddr	lowpc = scope.pc(an_lopc);

			source.pc_to_stmt(lowpc, line);
		}

		// loop through the scope's children, which are the variables
		while(!s.isnull() && !prismember(&interrupt, SIGINT))
		{
			tag = s.tag(); 

			switch(tag)
			{
			default:
				break;

			case t_variable:
			case t_subroutine:
				// global variables and functions are covered
				// by get_globals
				if (s.has_attribute(an_external))
					break;
				// FALL-THROUGH
			case t_argument:
			case t_label:
			case t_entry:
				if (((s_name = s.name()) == 0) 
					|| (patmatch && !patmatch->match(s_name)))
					break;
				symlist->add(s, location, line);
				break;

			}
			s = s.sibling();
		}
finish_scope:
		if (scope.tag() == t_inlined_sub)
			scope_name = 0;
	}
	return 1;
}

static int
print_debug_vars(ProcObj *pobj, int mode, PatternMatch *patmatch )
{
	char	*name;
	int	ret = 1;
	char	*l = "";
	char	*loc = "debugger";
	char	*bad_val = (char *)Mtable.format(ERR_evaluation);

	var_table.set_context( pobj, pobj ? pobj->curframe() : 0, 
		0, mode&SYM_BUILT_IN, mode&SYM_USER);

	for (var_table.First(); name = var_table.Name(); var_table.Next())
	{
		if (patmatch && !patmatch->match(name))
			continue;
		if (mode & SYM_VALUES)
		{
			char	*value = 0;
			Buffer	*buf = 0;
#if EXCEPTION_HANDLING
			if (strcmp(name, "%eh_object") == 0)
			{
				if (pobj && pobj->get_eh_info()
					&& pobj->get_eh_info()->is_obj_valid())
				{
					// special handling for %eh_object, since it is
					// more like a program symbol than the other
					// debugger symbols
					Expr	*exp = 0;
					Rvalue	*rval;

					buf = buf_pool.get();
					buf->clear();
					exp = new_expr(name, pobj);
					if (exp->eval(pobj) && exp->rvalue(rval))
					{
						rval->print(buf, pobj, DEBUGGER_BRIEF_FORMAT);
						value = buf->size() ? (char *)*buf : "";
					}
					delete exp;
				}
			}
			else
#endif
				value = var_table.Value();
			if (!value)
			{
				value = bad_val;
				ret = 0;
			}
			if (mode & SYM_TYPES)	
				printm(MSG_symbol_type_val, name, loc, l, "", value);
			else
				printm(MSG_symbol_val, name, loc, l, value);
			if (buf)
				buf_pool.put(buf);
		}
		else if (mode & SYM_TYPES)	
			printm(MSG_symbol_type, name, loc, l, "");
		else
			printm(MSG_symbol, name, loc, l);
		if (prismember(&interrupt, SIGINT))
		{
			break;
		}
	}
	return ret;
}

// local new handler
static void
symbols_memory_handler()
{
	// restore old handler before printing message
	// in case it generates another memory error
	newhandler.restore_old_handler();
	printe(ERR_memory_allocation, E_ERROR);
	newhandler.return_to_saved_env(1);
}

int
symbols(Proclist *procl, const char *obj, const char *pattern, 
	const char *file, int mode)
{
	Iaddr	pc;
	Symtab	*symtab;
	Symbol	top_scope;
	int	single = 1;
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head = 0;
	int	ret = 1;
	static SymList	*symlist;
	int	stopped;
	PatternMatch	*patmatch = 0;
	
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
	if (!pobj && (mode & (SYM_FILES|SYM_LOCALS|SYM_GLOBALS)))
		// print debug vars even with no pobj
	{
		printe(ERR_no_proc, E_ERROR);
		if (list_head)
			proglist.free_plist(list_head);
		return 0;
	}
	if (pattern)
	{
		patmatch = new PatternMatch;
		if (!patmatch->compile(pattern))
		{
			delete patmatch;
			printe(ERR_bad_regex, E_ERROR, pattern);
			if (list_head)
				proglist.free_plist(list_head);
			return 0;
		}
	}
	newhandler.install_user_handler(symbols_memory_handler);
	if (setjmp(*newhandler.get_jmp_buf()) != 0)
	{
		// longjmp returns here
		if (list_head)
			proglist.free_plist(list_head);
		return 0;
	}
	sigrelse(SIGINT);
	do
	{
		Msg_id	msgid;
		Frame	*frame = 0;
		Process	*proc = pobj ? pobj->process() : 0;
		stopped = 0;
		char	*obj_name = 0, *filename = 0;

		if (mode & (SYM_FILES|SYM_LOCALS|SYM_GLOBALS))
		{
			if (!pobj->state_check(E_RUNNING|E_DEAD) ||
				!proc->stop_all())
			{
				ret = 0;
				continue;
			}
			stopped = 1;
		}
		if ((mode & (SYM_VALUES|SYM_TYPES)) == (SYM_VALUES|SYM_TYPES))
			msgid = MSG_sym_type_val_header;
		else if (mode & SYM_TYPES)
			msgid = MSG_sym_type_header;
		else if (mode & SYM_VALUES)
			msgid = MSG_sym_val_header;
		else
			msgid = MSG_sym_header;
		if (pobj)
			printm(msgid, pobj->obj_name(), pobj->prog_name());
		else
			printm(msgid, "(no process)", "(no program)");

		if (mode & (SYM_BUILT_IN|SYM_USER))
		{
			if (!print_debug_vars(pobj, mode, patmatch))
				ret = 0;
			if (prismember(&interrupt, SIGINT))
			{
				if (stopped)
					if (!proc->restart_all())
						ret = 0;
				break;
			}
		}
		if (pobj)
			frame =  pobj->curframe();
		if (obj)
		{
			if ((obj_name = parse_str_var(pobj, 
				frame, (char *)obj)) == 0)
			{
				if (stopped)
					proc->restart_all();
				ret = 0;
				continue;
			}
			obj_name = makestr(obj_name);
		}
		if (mode & (SYM_FILES|SYM_LOCALS))
		{
			pc = frame->pc_value();
			if (file)
			{
				if ((filename = parse_str_var(pobj, 
					frame, (char *)file)) == 0)
				{
					if (stopped)
						proc->restart_all();
					ret = 0;
					continue;
				}
				filename = makestr(filename);
			}
			if ((symtab = pobj->find_symtab(pc)) == 0)
			{
				printe(ERR_no_sym_info, E_ERROR,
					pobj->obj_name());
				if (stopped)
					proc->restart_all();
				ret = 0;
				continue;
			}
			if (!symlist)
				// symlist collects symbols in 
				// alphabetical order
				symlist = new SymList;
			top_scope = symtab->find_scope(pc);
			if (!inner_names(pobj, top_scope, mode,
				patmatch, filename, symlist))
				ret = 0;
			if (mode & SYM_GLOBALS)
			{
				if (!get_globals(pobj, obj_name,
					mode, patmatch, symlist))
					ret = 0;
			}
			if (!symlist->printall(pobj, mode))
				ret = 0;
			symlist->clear();
			delete file_loc;
			file_loc = 0;
		}
		else if (mode & SYM_GLOBALS)
		{
			// globals are already in alphabetical order
			if (!get_globals(pobj, obj_name, mode,
				patmatch, 0))
				ret = 0;
		}
		if (stopped && !proc->restart_all())
			ret = 0;

		delete obj_name;
		delete filename;
		if (prismember(&interrupt, SIGINT))
			break;
	}
	while(!single && ((pobj = list++->p_pobj) != 0));

	sighold(SIGINT);
	newhandler.restore_old_handler();
	delete patmatch;
	if (list_head)
		proglist.free_plist(list_head);
	return ret;
}
