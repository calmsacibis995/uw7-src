#ifndef _Transcript_h
#define _Transcript_h

#ident	"@(#)debugger:gui.d/common/Transcript.h	1.3"

#include <stddef.h>


// Class that maintains a buffer of text lines.
// The number of lines is limited.  When the
// buffer grows beyond the limit, the oldest
// lines are discarded.

class Transcript {
	char	*string;
	size_t	*lines;
	size_t	last;
	size_t	bytes_used;
	size_t	total_bytes;
	void	getmemory(size_t sz);
	size_t 	discard_old();
public:
		Transcript();
		~Transcript();
	void	add(char *);
	char 	*get_string() { return (bytes_used ? string : ""); }
	void	clear();
	size_t	size()  { return bytes_used; }
};

#endif
