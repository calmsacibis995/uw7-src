#ident	"@(#)debugger:libutil/common/set_path.C	1.2"

#include	"global.h"
#include	"utility.h"
#include	<string.h>

// build path string from global and local paths
char *
set_path( const char *path, const char * string )
{
	char	*p2 = 0;

	if ( string == 0 ) 
	{
		if (path)
		{
			p2 = new char[ strlen(path) + 1 ];
			strcpy( p2, path );
		}
	}
	else if ( path == 0 )
	{
		p2 = new char[ strlen(string) + 1 ];
		strcpy( p2, string );
	}
	else
	{
		int	len = strlen(path);
		p2 = new char[  len + strlen(string) + 2 ];
		strcpy( p2, path );
		strcpy( p2+len, ":" );
		strcpy( p2+len+1, string );
	}
	return p2;
}
