#ident	"@(#)debugger:libutil/common/functions.C	1.16"
// print out list of functions 
// only used by the gui
// 
// by default, prints all functions, global and local, for 
// all files in the current object (a.out, libc.so.1, etc.)
//
// if an object is given, it looks there instead
// if a filename is given, only the functions defined in that
// file are listed (local or global)
// if source_only is non-zero, only functions from files having
// statement information are listed

#include "utility.h"
#include "global.h"
#include "Symbol.h"
#include "Source.h"
#include "Symtab.h"
#include "ProcObj.h"
#include "Proctypes.h"
#include "Frame.h"
#include "Proglist.h"
#include "Interface.h"
#include "Tag.h"
#include "NewHandle.h"
#include "RegExp.h"
#include "FileEntry.h"
#include "Vector.h"
#include <signal.h>
#include <string.h>
#include <stdlib.h>

class FuncList
{
	char		**list;
	int		count;
	int		size;
	PatternMatch	*patmatch;
	int		has_pattern;
	Vector		*vector;
public:
			FuncList();
			~FuncList();
	int		set_pattern(const char *pattern);
	void		add(const char * name, const char *filename = 0,
						const char *header = 0);
	int		add_file(const char *name);
	void		printall();
	void		clear();
	void		init();
};

#define LISTENT	200

FuncList::FuncList()
{
	// initialize list with default number of entries
	// we never shrink list
	if ((list = (char **)calloc(LISTENT, 
		sizeof(char *))) == 0)
	{
		newhandler.invoke_handler();
		return;
	}
	size = LISTENT;
	count = 0;
	has_pattern = 0;
	patmatch = 0;
	vector = vec_pool.get();
	vector->clear();
}

FuncList::~FuncList()
{
	clear();
	free(list);
	delete patmatch;
	if (vector)
		vec_pool.put(vector);
	vector = 0;
}

void
FuncList::init()
{
	has_pattern = 0;
	vector = vec_pool.get();
	vector->clear();
}

void
FuncList::clear()
{
	register char	**lptr = list;
	while(*lptr)
	{
		delete(*lptr);
		*lptr = 0;
		lptr++;
	}
	count = 0;
	if (vector)
		vec_pool.put(vector);
}

// a list of files is kept to ensure that the same file is not
// scanned twice (once with debugging info and once without)
int
FuncList::add_file(const char *filename)
{
	const char	**ptr = (const char **)vector->ptr();
	int		count = vector->size()/sizeof(char **);
	for (int i = 0; i < count; i++, ptr++)
	{
		if (strcmp(filename, *ptr) == 0)
			return 0;
	}
	vector->add(&filename, sizeof(const char *));
	return 1;
}

void
FuncList::add(const char *name, const char *filename, const char *header)
{
	char	*sym;
	int	len;

	if (has_pattern && !patmatch->match(name))
		return;

	len = strlen(name) + 1;

	if (filename)
		len += strlen(filename) + 1;
	if (header)
		len += strlen(header) + 1;
	sym = new char[len];

	if (filename && header)
		sprintf(sym, "%s@%s@%s", filename, header, name);
	else if (filename)
		sprintf(sym, "%s@%s", filename, name);
	else
		strcpy(sym, name);
	
	if (count >= (size - 1))
	{
		// always leave a null entry at end
		if ((list = (char **)realloc((char *)list,
			(sizeof(char *) * (size + LISTENT)))) == 0)
		{
			size = 0;
			newhandler.invoke_handler();
		}
		memset(list+size, 0, sizeof(char *) * LISTENT);
		size += LISTENT;
	}
	list[count++] = sym;
}

int
FuncList::set_pattern(const char *pattern)
{
	if (!patmatch)
		patmatch = new PatternMatch;
	if ((has_pattern = patmatch->compile(pattern)) == 0)
		printe(ERR_bad_regex, E_ERROR, pattern);
	return has_pattern;
}

int
fcmp(const void *s1, const void *s2)
{
	return strcmp(*(char **)s1, *(char **)s2);
}

void
FuncList::printall()
{
	char		*symentry, **lptr;
	char		*last = 0;

	if (!count)
		return;

	qsort(list, count, sizeof(char *), (int (*)(const void *,
		const void *))fcmp);

        lptr = list;
	symentry = *lptr;
	for ( ; symentry; symentry = *(++lptr))
	{
		if (prismember(&interrupt, SIGINT))
			break;
		if (last && (strcmp(last, symentry) == 0))
			continue;
		printm(MSG_function, symentry);
		last = symentry;
	}
	vector->clear();
}

static void 
search_file(Symbol &fsymb, ProcObj *pobj, FuncList *funclist, int source_only, int add_globals)
{
	Source		source;
	const char	*fname;
	const char	*sname;

	fname = fsymb.name();
	if ((source_only && !fsymb.has_attribute(an_lineinfo))
		|| !funclist->add_file(fname))
	{
		return;
	}
	Symbol	s = fsymb.child();
	for(; !s.isnull(); s = s.sibling())
	{
		if (prismember(&interrupt, SIGINT))
			break;
		if (!s.isEntry() || !s.has_attribute(an_lopc))
			continue;
		if (s.has_attribute(an_external))
		{
			// global function - also handled in search through
			// global symbol table in search_symtab
			if (!add_globals && !s.isInlined())
				continue;
			sname = pobj->symbol_name(s);
			if (sname && *sname)
				funclist->add(sname);
		}
		else
		{
			// static function
			sname = pobj->symbol_name(s);
			if (sname && *sname)
			{
				long		file;
				long		line;
				const char	*sym_header = 0;
				if (s.file_and_line(file, line) && file > 1
					&& fsymb.file_table(source))
				{
					// static function from header, location must
					// include header file name
					const FileEntry *fentry = source.find_header(file);
					sym_header = fentry->file_name;
				}
				funclist->add(sname, fname, sym_header);
			}
		}
	}
}

static void 
search_symtab(Symtab *stp, ProcObj *pobj, FuncList *funclist, int source_only)
{
	Symbol	sym = stp->first_symbol();
	for(; !sym.isnull(); sym = sym.sibling())
	{
		if (prismember(&interrupt, SIGINT) ||
			(!sym.isSourceFile()))
			break;
		search_file(sym, pobj, funclist, source_only,
					/*add_globals=*/source_only);
	}

	// if not looking only for symbols for which we have debug info,
	// use global list to get globals
	if (!source_only)
	{
		const char	*sname;
		NameEntry	*ne;
		ne = stp->first_global();
		for(; ne; ne = stp->next_global())
		{
			sym = stp->global_symbol(ne);
			if (!sym.isnull() && sym.isGlobalSub())
			{
				sname = pobj->symbol_name(sym);
				if (sname && *sname)
					funclist->add(sname);
			}
		}
	}
}

// local new handler
static void
functions_memory_handler()
{
	// restore old handler before printing message
	// in case it generates another memory error
	newhandler.restore_old_handler();
	printe(ERR_memory_allocation, E_ERROR);
	newhandler.return_to_saved_env(1);
}

int
functions(Proclist *procl, const char *filename, const char *object,
	const char *pattern, int source_only)
{
	ProcObj		*pobj;
	static FuncList	*funclist;
	int		ret = 1;
	int		single = 1;
	plist		*list;
	plist		*list_head = 0;

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
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		if (list_head)
			proglist.free_plist(list_head);
		return 0;
	}

	if (!funclist)
		funclist = new FuncList();
	else
		funclist->init();
	if (pattern && !funclist->set_pattern(pattern))
	{
		ret = 0;
		goto out1;
	}

	newhandler.install_user_handler(functions_memory_handler);
	if (setjmp(*newhandler.get_jmp_buf()) != 0)
		// longjmp returns here
		goto out2;

	sigrelse(SIGINT);
	do
	{
		printm(MSG_function_header, pobj->obj_name(), pobj->prog_name());
		if (filename)
		{
			Symbol	file;
			if (!pobj->find_source(filename, file))
			{
				printe(ERR_no_source_info, E_ERROR, filename);
				ret = 0;
				continue;
			}
			search_file(file, pobj, funclist, source_only,
					/*add_globals=*/ 1);
		}
		else
		{
			Symtab	*stp;

			if (!object)
			{
				if ((stp = pobj->find_symtab(
					pobj->curframe()->pc_value())) == 0)
				{
					printe(ERR_no_sym_info, E_ERROR,
						pobj->obj_name());
					ret = 0;
					continue;
				}
			}
			else
			{
				if ((stp = pobj->find_symtab(object)) == 0)
				{
					printe(ERR_cant_find_symtab, E_ERROR,
						object);
					ret = 0;
					continue;
				}
			}
			search_symtab(stp, pobj, funclist, source_only);
		}
		funclist->printall();
		if (prismember(&interrupt, SIGINT))
			break;
	}
	while (!single && ((pobj = list++->p_pobj) != 0));

	sighold(SIGINT);
out2:
	newhandler.restore_old_handler();
out1:
	funclist->clear();
	if (list_head)
		proglist.free_plist(list_head);
	return ret;
}
