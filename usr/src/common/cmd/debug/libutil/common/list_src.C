#ident	"@(#)debugger:libutil/common/list_src.C	1.23"

#include "utility.h"
#include "SrcFile.h"
#include "ProcObj.h"
#include "Interface.h"
#include "Location.h"
#include "Proglist.h"
#include "global.h"
#include "Process.h"
#include "RegExp.h"
#include "FileEntry.h"


static int	fprintn(ProcObj *pobj, SrcFile *, long, const FileEntry *, int is_user_set);
static int	check_srcfile(SrcFile *, long, long &);
static int	dore(ProcObj *, SrcFile *, const char *, int, const FileEntry *);
static SrcFile *check_fentry(ProcObj *, const FileEntry *&, int &is_user_set,
			int do_delete, Symbol &scope);

int 
list_src(Proclist *procl, int count, char *cnt_var, Location *l, 
	const char *re, int mode)
{
	int		ret = 1;
	int 		single = 1;
	ProcObj		*pobj;
	plist		*list;
	plist		*list_head = 0;
	int		src_is_user_set = 0;

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
	// if a pobj is specified in the location, it overrides
	if (l)
	{
		ProcObj	*l_pobj;
		if (!l->get_pobj(l_pobj))
		{
			return 0;
		}
		if (l_pobj)
		{
			pobj = l_pobj;
			single = 1;
		}
	}
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		if (list_head)
			proglist.free_plist(list_head);
		return 0;
	}
	sigrelse(SIGINT);
	do {
		SrcFile		*sf;
		char		*file;
		char		*header;
		char		*func;
		const FileEntry	*fentry = 0;
		long		line;
		Frame		*f;
		long		lcount;

		sf = 0;
		if (prismember(&interrupt, SIGINT))
			break;

		if (!single)
			printm(MSG_list_header, pobj->obj_name(), pobj->prog_name());
		if (!pobj->state_check(E_DEAD))
		{
			ret = 0;
			continue;
		}
		if (cnt_var)
		{
			if (!parse_num_var(pobj, pobj->curframe(),
				cnt_var, count))
			{
				ret = 0;
				continue;
			}
		}
		lcount = (count > 0) ? count : num_line;
		if (l)
		{
			Symtab	*stab = 0;
			Symbol	scope;
			int	do_delete = 0;
			f = pobj->curframe();

			if (l->get_file(pobj, f, file, header, scope) == 0 ||
				l->get_symtab(pobj, f, stab) == 0)
			{
				ret = 0;
				continue;
			}
			if (!file) 
			{
				// list on running process allowed
				// only where fully qualified locaction
				// is given
				if (!pobj->state_check(E_RUNNING|E_DEAD))
				{
					ret = 0;
					continue;
				}
			}
			switch(l->get_type())
			{
				case lk_none:
				case lk_addr:
				default:
					printe(ERR_bad_list_loc, E_ERROR);
					ret = 0;
					goto out;
				case lk_fcn:
					if ((l->get_func(pobj, f, func) == 0) || 
						(func == 0))
					{
						ret = 0;
						continue;
					}
					if ((fentry = find_fcn(pobj, stab,
						scope, func, line )) == 0)
					{
						ret = 0;
						continue;
					}
					break;
				case lk_stmt:
					if (l->get_line(pobj, f, 
						(unsigned long &)line) == 0)
					{
						ret = 0;
						continue;
					}
					if (!scope.isnull())
						fentry = find_file_entry(pobj, scope, file, header);
					else if (stab)
					{
						printe(ERR_bad_loc, E_ERROR);
						return 0;
					}
					else if (header)
					{
						// for now, just assume this is the
						// name of a file not found in the
						// debugging information.  Path search
						// and querying about header files
						// will be done later, in check_fentry
						fentry = new FileEntry(header);
						src_is_user_set = 1;
						do_delete = 1;
					}
					else
						fentry = pobj->curr_src(src_is_user_set);
			}
			if ((sf = check_fentry(pobj, fentry, src_is_user_set,
					do_delete, scope)) == 0
				|| !check_srcfile(sf, line, lcount))
			{
				ret = 0;
				continue;
			}
			pobj->set_current_stmt(fentry, line, src_is_user_set);
			pobj->set_current_comp_unit(scope);
		}
		else
		{
			Symbol scope = pobj->curr_comp_unit();
			if (!pobj->state_check(E_RUNNING))
			{
				ret = 0;
				continue;
			}
			fentry = pobj->curr_src(src_is_user_set);
			if ((sf = check_fentry(pobj, fentry, src_is_user_set,
						0, scope)) == 0)
			{
				ret = 0;
				continue;
			}
			if (re)
			{
				long	tmp = 1;
				// make sure we have current line
				if (!check_srcfile(sf, pobj->current_line(), tmp))
				{
					ret = 0;
					continue;
				}
				if (!dore(pobj, sf, re, mode, fentry))
				{
					ret = 0;
					continue;
				}
				if (count <= 0)
				{
					long currline = pobj->current_line();
					printm(MSG_line_src, currline,
						sf->line((int)currline));
					continue;
				}
			}
			if (!check_srcfile(sf, pobj->current_line(), lcount))
			{
				ret = 0;
				continue;
			}
		}
		if (!fprintn(pobj, sf, lcount, fentry, src_is_user_set))
		{
			ret = 0;
		}
		if (!single)
			printm(MSG_newline);
	} 
	while(!single && ((pobj = list++->p_pobj) != 0));
out:
	if (list_head)
		proglist.free_plist(list_head);
	sighold(SIGINT);
	return ret;
}

// Look up the SrcFile for the given FileEntry.  If the file is not found
// and the file was set by the user (through setting %list_file or something
// like list foo.h@1, as opposed to the current location determined by debug)
// the debugger will try again, assuming the name may be a header file name,
// and querying the user)
static SrcFile *
check_fentry(ProcObj *pobj, const FileEntry *&fentry, int &is_user_set,
		int do_delete, Symbol &scope)
{
	SrcFile	*sf;
	if (!fentry)
	{
		// could be stopped in a function compiled w/o -g
		current_loc(pobj, pobj->curframe(), scope, &fentry);
	}		
	if (fentry == 0 || fentry->file_name == 0 || *fentry->file_name == 0)
	{
		printe(ERR_no_cur_src_obj, E_ERROR, pobj->obj_name());
		return 0;
	}
	if ((sf = find_srcfile(pobj, fentry)) != 0)
		return sf;

	if (is_user_set)
	{
		const FileEntry *nentry = find_file_entry(pobj, scope, 0, fentry->file_name);
		if (!nentry)
		{
			printe(ERR_no_source, E_ERROR, fentry->file_name);
			if (do_delete)
				delete fentry;
			return 0;
		}
		if (do_delete)
			delete fentry;
		fentry = nentry;
		is_user_set = 0;
		if ((sf = find_srcfile(pobj, fentry)) != 0)
			return sf;
	}
	printe(ERR_no_source, E_ERROR, fentry->file_name);
	return 0;
}

static int
check_srcfile(SrcFile *sf, long line, long &num)
{
	long	hi;

	if (line <= 0)
	{
		printe(ERR_bad_line_number, E_ERROR);
		return 0;
	}
	if ((num = sf->num_lines(line, num, hi)) == 0)
	{
		if (hi == 0)
			printe(ERR_no_lines, E_ERROR, sf->filename());
		else
			printe(ERR_only_n_lines, E_ERROR, hi,
				sf->filename());
		return 0;
	}
	return 1;
}

// Print count lines.
static int
fprintn(ProcObj *pobj, SrcFile *sf, long count, const FileEntry *fentry, int is_user_set)
{
	long	firstline, lastline;

	// firstline would be zero for a file compiled w/o -g - 
	// no line number info
	if ((firstline = pobj->current_line()) == 0)
		firstline = 1;
	lastline = firstline + count - 1;

	while (firstline <= lastline)
	{
		if (prismember(&interrupt, SIGINT))
			break;
		printm(MSG_line_src, firstline, sf->line((int)firstline));
		firstline++;
	}
	pobj->set_current_stmt(fentry, lastline, is_user_set);

	return count;
}

// regular expression parsing and searching
static int
dore(ProcObj *pobj, SrcFile *sf, const char *nre, int forward, const FileEntry *fentry)
{
	long 			cline, tline;
	static RegExp		*rex;

	if (!fentry)
	{
		printe(ERR_no_cur_src_obj, E_ERROR, pobj->obj_name());
		return 0;
	}

	if (*nre != '\0')
	{
		if (!rex)
			rex = new RegExp;
		if (rex->re_compile(nre, 0, 0) == 0)
		{
			printe(ERR_bad_regex, E_ERROR, nre);
			return 0;
		}
	}
	else if (!rex)
	{
		printe(ERR_no_previous_re, E_ERROR);
		return 0;
	}

	tline = cline = pobj->current_line();
	do {
		long	hi;
		if (prismember(&interrupt, SIGINT))
			break;
		if (forward)
		{
			if (sf->num_lines(tline, 2, hi) == 1)
				// last line
				tline = 1;
			else
				tline += 1;
		}
		else
		{
			if (tline == 1)
				// go to last line
				tline = sf->num_lines(0, 0, hi);
			else
				tline -= 1;
		}
		if (rex->re_execute(sf->line((int)tline)))
		{
			pobj->set_current_stmt(fentry, tline, 0);
			return 1;
		}
	} while (tline != cline);
	printe(ERR_no_re_match, E_WARNING);
	return 0;
}
