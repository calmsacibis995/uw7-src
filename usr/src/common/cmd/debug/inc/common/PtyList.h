#ifndef _PtyList_h
#define _PtyList_h
#ident	"@(#)debugger:inc/common/PtyList.h	1.2"

//
// Header for Linked list of Pseudo Terminal records.
//

#include "Link.h"
#include <stdio.h>

class PtyInfo : public Link {
	int	count;
	int	pty; 		   // pty file descriptor
	char	*_name;
public:
		PtyInfo();
		~PtyInfo();

	// Access functions
	int	pt_fd()		{ return pty; }
	int	refcount()	{ return count; }
	char	*name()		{ return _name; }
	void	bump_count()	{ count++; }
	int	dec_count()	{ count--; return count; }
	int	is_null()	{ return (pty < 0); }
};

extern PtyInfo	*first_pty;
extern PtyInfo	*setup_childio();
extern void	redirect_childio(int fd);

#endif // _PtyList_h
