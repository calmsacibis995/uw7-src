#ident	"@(#)debugger:libsymbol/common/Protoline.h	1.5"
#ifndef Protoline_h
#define Protoline_h

#include	"Vector.h"
#include	"Iaddr.h"

struct Lineinfo;
struct FileEntry;

class Protoline {
	Vector		vector;
	Iaddr		last_addr;
	long		last_line;
	int		count;
	char		addr_sorted;
	char		line_sorted;
public:
			Protoline()
				{
					count = 0;
					addr_sorted = line_sorted = 1;
					last_addr = 0;
					last_line = 0;
				}
			~Protoline()	{}
	Protoline &	add_line( Iaddr, long line_num, const FileEntry * = 0 );
	Protoline &	drop_line();
	Lineinfo *	put_line(Iaddr);
};

#endif /* Protoline_h */
