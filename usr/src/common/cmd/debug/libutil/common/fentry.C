#ident	"@(#)debugger:libutil/common/fentry.C	1.3"

#include "FileEntry.h"
#include "Interface.h"
#include "ProcObj.h"
#include "Source.h"
#include "Symbol.h"
#include "utility.h"
#include "global.h"
#include "str.h"
#include <signal.h>

int
FileEntry::compare(Symbol &sym) const
{
	const char *sym_name = sym.name();
	if (file_name && sym_name)
	{
		// compiler is inconsistent about putting out "./"-qualified
		// path names - name in .debug_line is file.c, name in
		// .debug_info is ./file.c
		if (sym_name[0] == '.' && sym_name[1] == '/')
			return (strcmp(sym_name+2, file_name) == 0);
		return (strcmp(sym_name, file_name) == 0);
	}
	return 0;
}

const char *
FileEntry::get_qualified_name(Symbol &sym) const
{
	if (file_name)
	{
		if (compare(sym))
			return sym.name();

		const char *sym_name = sym.name();
		if (!sym_name)
			return file_name;
		return sf(strlen(sym_name) + strlen(file_name) + 1,
				"%s@%s", sym_name, file_name);
	}
	else
		return sym.name();
}

static const FileEntry *
do_search(ProcObj *pobj, Symbol &scope, const char *header_name)
{
	sigrelse(SIGINT);
	Source lineinfo;
	for (scope = pobj->first_file(); !scope.isnull();
			scope = pobj->next_file())
	{
		if (prismember(&interrupt, SIGINT))
		{
			sighold(SIGINT);
			return 0;
		}

		if (!scope.file_table(lineinfo))
			continue;

		const FileEntry *fentry = lineinfo.find_header(header_name);
		if (fentry)
		{
			sighold(SIGINT);
			return fentry;
		}
	}
	sighold(SIGINT);
	printe(ERR_no_source_info, E_ERROR, header_name);
	return 0;
}

const FileEntry *
find_file_entry(ProcObj *pobj, Symbol &scope, const char *file_name,
	const char *header_name, Severity sev)
{
	Source	lineinfo;
	if (header_name)
	{
		const FileEntry	*fentry;
		if (!scope.isnull() && scope.file_table(lineinfo)
			&& (fentry = lineinfo.find_header(header_name)) != 0)
			return fentry;

		if (file_name)
		{
			printe(ERR_no_source_info, E_ERROR, header_name);
			return 0;
		}

		if (!query(QUERY_search_for_header, 1, header_name))
			return 0;
		return do_search(pobj, scope, header_name);
	}

	if (scope.isnull())
	{
		if (sev != E_NONE)
			printe(ERR_no_cur_src_obj, sev, pobj->obj_name());
		return 0;
	}
	if (!scope.file_table(lineinfo))
	{
		// probably file compiled w/o -g, create temporary file entry so
		// list, etc. can find file even without line number info.
		if (sev != E_NONE)
			printe(ERR_no_source_info, sev, scope.name());
		return pobj->create_fentry(scope.name());
	}
	return lineinfo.primary_file_entry();
}

const FileEntry *
find_file_entry(ProcObj *proc, Symtab *stab, const char *file_name,
	const char *header_name, Severity sev)
{
	Symbol	scope = find_compilation_unit(proc, stab, file_name);

	return find_file_entry(proc, scope, file_name, header_name, sev);
}
