#ifndef Object_h
#define Object_h
#ident	"@(#)debugger:inc/common/Object.h	1.10"

// Information for a single object file - provides file
// format level interface to object sections and segments.
//
// Objects remain in existence even after their associated
// processes die.  This saves work if the same a.out is reinvoked
// or a shared library is used by more than 1 process.
// 
// Object file format specific functions are provided by derived
// classes.

#include "Iaddr.h"
#include "List.h"
#include "Machine.h"
#include <sys/types.h>

class Symtable;

enum File_format {
	ff_none	= 0,
	ff_elf,
	ff_coff,
#ifdef USES_SVR3_CORE
	ff_coff_core
#endif
};

enum Sect_type {
	s_interp = 0,
	s_dynamic,
	s_lib,	// static shared lib info, if available
	s_notes,	// for ELF core files
	s_symtab,
	s_strtab,
	s_debug,	// DWARFI debugging section
	s_line,		// DWARFI line number section
	s_plt,	// procedure linkage table, if available
	s_got,	// global offset table, if available
	s_dwarf2_info,	// DWARFII debugging section
	s_abbreviation,	// DWARFII abbreviation tables
	s_dwarf2_line,	// DWARFII line number section
	s_last	// must be last
};

struct Sectinfo {
	void		*data;
	Iaddr		vaddr;
	long		offset;
	long		size;
	struct Sectinfo *next;
};

enum mem_type {
	mt_read,
	mt_map,
};

struct mem_map {
	mem_type	m_type;
	void 		*m_start;
	long		m_size;
};

// segment types
#define	SEG_EXEC	1
#define	SEG_WRITE	2
#define	SEG_READ	4
#define	SEG_LOAD	8

#define SEG_PROT_MASK(x)	(x & (SEG_EXEC|SEG_WRITE|SEG_READ))

struct Seginfo {
        long	offset;
        Iaddr	vaddr;
	long	mem_size;
	long	file_size;
	int	seg_flags;
};

// values for flags
#define O_SHARED	1
#define O_CORE		2
#define O_DEBUG		4

class Object {
protected:
	int		fdobj;
	dev_t		device;
	ino_t		inumber;
	time_t		obj_time;
	File_format	file_form;
	int		flags;
	Iaddr		start;
	Seginfo 	*seginfo;
	List		memlist; // list of mapped objects
	Sectinfo	*sections[s_last];
	Symtable 	*symtable;
	int		foreign;
	int		read_data(void *&dest, long offset, long size,
				int nodelete);
	int		map_data(void *&dest, long offset, long size);
	friend		int locate_obj(int fd, Object *&obj);
	friend 		Object *find_object( int fdobj, const char *exec_name = 0);
public:
			Object( int fdobj, dev_t, ino_t, time_t );
	virtual		~Object();
	int		fd()	{ return fdobj; }
	Iaddr		start_addr() { return start; }
	File_format	file_format()  { return file_form; }
	Symtable	*get_symtable() { return symtable; }
	char		**get_stsl_names();
	int		getsect( Sect_type, Sectinfo *);
	int		is_shared() { return(flags & O_SHARED); }
	int		has_debug_info() { return(flags & O_DEBUG); }
	int		is_core() { return(flags & O_CORE); }
	int		read(Iaddr from, void *to, int len);
	int		is_foreign() { return foreign; }
	virtual Seginfo *get_seginfo( int &count, int &shared );
};

// cfront 2.1 requires base class name in derived class constructor,
// 1.2 forbids it
#ifdef __cplusplus
#define OBJECT	Object
#else
#define OBJECT
#endif

extern char	*check_shell_script(int fd);

#endif	/* Object_h */
