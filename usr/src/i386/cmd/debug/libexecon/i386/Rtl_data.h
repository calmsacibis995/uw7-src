#ifndef Rtl_data_h
#define Rtl_data_h
#ident	"@(#)debugger:libexecon/i386/Rtl_data.h	1.6"

// Machine dependent description of dynamic linking access
// routines

#include "Iaddr.h"
#include <link.h>

class	Process;
class	Procctl;
class	Seglist;

class	Rtl_data {
	Iaddr		r_debug_addr;
	Iaddr		rtld_base( Process *, Proclive * );
	const char	*ld_so;
	Iaddr		rtld_addr;
	r_debug		rdebug;
	const char	*alternate_ld_so;
	friend class	Seglist;
public:
			Rtl_data(const char *, const char *);
			~Rtl_data() { delete (void *)ld_so; 
					delete (void *)alternate_ld_so;
					}
	int		find_alternate_rtld(Proclive *, Seglist *);
	int		find_r_debug( Process *, Proclive *, Seglist * );
	int		find_link_map( int, const char *, Seglist *, 
				Iaddr & , Process *);
};

#endif
