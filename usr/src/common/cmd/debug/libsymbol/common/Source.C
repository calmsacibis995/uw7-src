#ident	"@(#)debugger:libsymbol/common/Source.C	1.5"
#include	"Source.h"
#include	"Lineinfo.h"
#include	"FileEntry.h"
#include	"utility.h"
#include	<string.h>

Source::Source()
{
	lineinfo = 0;
	file_table = 0;
	ss_base = 0;
}

Source::Source(const Source& source )
{
	lineinfo = source.lineinfo;
	ss_base = source.ss_base;
	file_table = source.file_table;
}

Source&
Source::operator=( const Source & source )
{
	lineinfo = source.lineinfo;
	ss_base = source.ss_base;
	file_table = source.file_table;
	return *this;
}

const FileEntry *
Source::get_values(long index, long &line, Iaddr *stmt_start)
{
	LineEntry	*pcinfo = &lineinfo->addrpart[index];
	line = pcinfo->linenum;
	if (stmt_start)
		*stmt_start = pcinfo->addr + ss_base;
	if (pcinfo->file_entry)
		return pcinfo->file_entry;
	return &file_table->files[1];
}

const FileEntry *
Source::pc_to_stmt( Iaddr pc, long & line, int slide, Iaddr *stmt_start )
{
	int		first,middle,last;
	int		found;
	long		entrycount;
	LineEntry	*pcinfo;

	if (stmt_start)
		*stmt_start = 0;
	if ( lineinfo == 0 )
	{
		line = 0;
		return 0;
	}
	entrycount = lineinfo->entrycount;
	pcinfo = lineinfo->addrpart;
	pc -= ss_base;
	found = 0;
	first = 0;
	last = (int)entrycount - 1;
	if (pc < pcinfo[first].addr)
	{
		if (slide > 0)
		{
			return get_values(first, line, stmt_start);
		}
		else
		{
			line = 0;
			return 0;
		}
	}
	else if (pc > pcinfo[last].addr)
	{
		if (slide < 0)
		{
			return get_values(first, line, stmt_start);
		}
		else
		{
			line = 0;
			return 0;
		}
	}
	while (first <= last)
	{
		middle = ( first + last ) / 2;
		if ( pcinfo[middle].addr == pc )
		{
			found = 1;
			break;
		}
		else if ( pcinfo[middle].addr > pc )
		{
			last = middle - 1;
		}
		else
		{
			first = middle + 1;
		}
	}
	if ( found )
	{
		return get_values(middle, line, stmt_start);
	}
	else if (slide == 0)
	{
		line = 0;
		return 0;
	}
	else if ( slide < 0 )
	{
		return get_values(last, line, stmt_start);
	}
	else 
	{
		return get_values(last+1, line, stmt_start);
	}
}

int
Source::check_file(const FileEntry *fentry, const char *file_name)
{
	if (!file_name || !fentry || !fentry->file_name)
		return 1;

	return check_file_name(fentry->file_name, file_name);
}

void
Source::stmt_to_pc( long line, const char *file, Iaddr & pc, int slide )
{
	int		first, middle, last;
	int		found;
	long		entrycount;
	LineEntry 	*stmtinfo;

	if ( lineinfo == 0 )
	{
		pc = 0;
		return;
	}
	entrycount = lineinfo->entrycount;
	stmtinfo = lineinfo->linepart;
	found = 0;
	first = 0;
	last = (int)entrycount - 1;
	if (line < stmtinfo[first].linenum)
	{
		if (slide > 0 && check_file(stmtinfo[first].file_entry, file))
			pc = stmtinfo[first].addr + ss_base;
		else 
			pc = 0;

		return;
	}
	else if (line > stmtinfo[last].linenum)
	{
		if (slide < 0 && check_file(stmtinfo[last].file_entry, file))
			pc = stmtinfo[last].addr + ss_base;
		else 
			pc = 0;
		return;
	}
	while (first <= last)
	{
		middle = ( first + last ) / 2;
		if ( stmtinfo[middle].linenum == line )
		{
			// search backwards for first entry with that linenumber
			while (stmtinfo[middle-1].linenum == line)
				--middle;
			// search forwards for match on file
			while (stmtinfo[middle].linenum == line)
			{
				if (check_file(stmtinfo[middle].file_entry, file))
				{
					found = 1;
					break;
				}
				middle++;
			}
			break;
		}
		else if ( stmtinfo[middle].linenum > line )
		{
			last = middle - 1;
		}
		else
		{
			first = middle + 1;
		}
	}
	if ( found )
	{
		// find first entry for that line number here
		pc = stmtinfo[middle].addr + ss_base;
	}
	else if (slide == 0)
	{
		pc = 0;
	}
	else if ( slide < 0 )
	{
		// find first entry for that line number here
		if (check_file(stmtinfo[last].file_entry, file))
			pc = stmtinfo[last].addr + ss_base;
		else
			pc = 0;
	}
	else
	{
		if (check_file(stmtinfo[last].file_entry, file))
			pc = stmtinfo[last+1].addr + ss_base;
		else
			pc = 0;
	}
	return;
}
	
const FileEntry *
Source::find_header(const char *header)
{
	if (!file_table || !file_table->files)
		return 0;
	FileEntry *fentry = file_table->files;
	for (int i = 0; i < file_table->filecount; i++, fentry++)
	{
		if (check_file_name(fentry->file_name, header))
			return fentry;
	}
	return 0;
}
	
const FileEntry *
Source::find_header(int index)
{
	if (file_table && file_table->files && index > 0
		&& index < file_table->filecount)
		return &file_table->files[index];
	return 0;
}
	
const FileEntry *
Source::primary_file_entry()
{
	if (!file_table || !file_table->files)
		return 0;
	// Dwarf2 indexing is 1-based - 0 entry is null
	return &file_table->files[1];
}

