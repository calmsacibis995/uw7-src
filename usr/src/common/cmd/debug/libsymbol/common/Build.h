#ident	"@(#)debugger:libsymbol/common/Build.h	1.8"

#ifndef Build_h
#define Build_h

#include <stddef.h>

#ifndef OFFSET_T
typedef unsigned long offset_t;
#define OFFSET_T
#endif // OFFSET_T

// Build -- the base class for Coffbuild, Dwarfbuild, and Elfbuild.
// Provides some virtual functions to simplify algorithms in class Evaluator.

struct Syminfo;
struct Attribute;
class Protorec;
class Lineinfo;
struct FileEntry;

class Build
{
    public:
	void	buildNameAttrs(Protorec&, const char *);  // handle names that might
						   // be mangled (C++).

	virtual int        get_syminfo( offset_t offset, Syminfo &, int new_file ) = 0;
	virtual Attribute *make_record( offset_t offset, int want_file = 0 ) = 0;
	virtual offset_t   globals_offset() = 0;
	virtual Lineinfo  *line_info( offset_t offset, const FileEntry * = 0 ) = 0;
	virtual int	   out_of_range( offset_t ) = 0;
	virtual	Attribute *find_record(offset_t offset);
};

#define WANT_FILE	1

char	*demangle_name(const char *);

#endif /* Build_h */
