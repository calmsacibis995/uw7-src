#ident	"@(#)debugger:libsymbol/common/Syminfo.h	1.2"
#ifndef Syminfo_h
#define Syminfo_h

#ifndef OFFSET_T
typedef unsigned long offset_t;
#define OFFSET_T
#endif // OFFSET_T

enum sym_bind	{ sb_none, sb_local, sb_global, sb_weak };

enum sym_type	{ st_none, st_object, st_func, st_section, st_file };

struct Syminfo {
	long		name;
	long		lo,hi;
	unsigned char	bind,type;
	offset_t	sibling,child;
	int		resolved;
};

#endif /* Syminfo_h */
