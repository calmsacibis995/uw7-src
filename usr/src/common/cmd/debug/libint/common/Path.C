#ident	"@(#)debugger:libint/common/Path.C	1.2"

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */


#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include "Path.h"
#include "str.h"


// Canonicalize a pathname, compressing /./, //, and resolving /../.
// Resulting pathnames start are absolute, but symbolic links
// are not resolved (different from realpath).
// Allocates space for resulting name.  Returns 0 if the
// current working directory can't be obtained.


char		*current_dir;
static int	cwd_len;

int
init_cwd()
{
	char	buf[PATH_MAX+1];

	if (getcwd(buf, PATH_MAX) == 0)
	{
		if (current_dir)
		{
			delete current_dir;
			current_dir = 0;
		}
		return 0;
	}
	cwd_len = strlen(buf);
	delete current_dir;
	current_dir = new char[cwd_len + 1];
	strcpy(current_dir, buf);
	return 1;
}

char *
pathcanon(const char *raw)
{
	register char	*s, *d;
	char		*modcanon;
	char		canon[PATH_MAX+1];
	register char	*limit = canon + PATH_MAX;

	if (raw == 0 || current_dir == 0) 
	{
		return (0);
	}

	// If the path in raw is not already absolute, convert it to 
	// that form. In any case, initialize canon with the 
	// absolute form of raw.  Make sure that none of the operations 
	// overflow the corresponding buffers.
	// The code below does the copy operations by hand so that it 
	// can easily keep track of whether overflow is about to occur.
	s = (char *)raw;
	d = canon;
	if (*s != '/') 
	{
		// Relative; prepend the working directory.
		strcpy(canon, current_dir);
		d += cwd_len;
		// Add slash to separate working directory from 
		// relative part.
		if (d < limit && d[-1] != '/')
			*d++ = '/';
		modcanon = d;
	} 
	else
		modcanon = canon;
	while (d < limit && *s)
		*d++ = *s++;

	// Add a trailing slash to simplify the code below.
	s = "/";
	while (d < limit && (*d++ = *s++))
		continue;
	// Canonicalize the path.  The strategy is to update in place,
	// with d pointing to the end of the canonicalized portion and 
	// s to the * current spot from which we're copying.  
	// This works because * canonicalization doesn't increase path 
	// length.
	// Note also that the path has had a slash added at its end.
	// This greatly simplifies the treatment of boundary conditions.
	d = s = modcanon;
	while (d < limit && *s) 
	{
		register char  *t; 
		if (((*d++ = *s++) != '/') || (d == (canon + 1)))
			continue;
		t = d - 2;
		switch (*t) 
		{
		case '/':
			// Found // in the name.
			d--;
			continue;
		case '.': 
			switch (*--t) 
			{
			case '/':
				// Found /./ in the name.
				d -= 2;
				continue;
			case '.': 
				if (*--t == '/') 
				{
					// Found /../ in the name.
					while (t > canon && *--t != '/')
						continue;
					d = t + 1;
				}
				continue;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}

	// Remove the trailing slash that was added above.
	if (*(d - 1) == '/' && d > canon + 1)
		d--;
	*d = '\0';
	return(makestr(canon));
}
