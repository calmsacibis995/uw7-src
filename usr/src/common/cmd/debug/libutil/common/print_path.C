#ident	"@(#)debugger:libutil/common/print_path.C	1.8"
#include	"utility.h"
#include	"ProcObj.h"
#include	"Proglist.h"
#include	"Interface.h"
#include	"SrcFile.h"
#include 	"Parser.h"
#include 	"global.h"
#include	"FileEntry.h"

int
print_path( Proclist * procl, const char *fname, const char *compilation_unit )
{
	int single = 1;
	ProcObj	*pobj;
	plist	*list;
	plist	*list_head = 0;
	int	ret = 1;

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
	do
	{
		SrcFile		*srcfile;
		const FileEntry *fentry;
		if (compilation_unit)
			fentry = find_file_entry(pobj, 0, compilation_unit, fname, E_NONE);
		else
			fentry = find_file_entry(pobj, 0, fname, 0, E_NONE);
		if ((srcfile = find_srcfile(pobj, fentry)) == 0)
		{
			printe(ERR_no_source, E_ERROR, fname);
			ret = 0;
			continue;
		}
		printm(MSG_source_file, srcfile->filename());
	}
	while(!single && ((pobj = list++->p_pobj) != 0));
	if (list_head)
		proglist.free_plist(list_head);
	return ret;
}
