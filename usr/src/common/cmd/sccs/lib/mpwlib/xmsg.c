#ident	"@(#)sccs:lib/mpwlib/xmsg.c	6.3.1.1"
# include	"../../hdr/defines.h"
# include	<errno.h>


/*
	Call fatal with an appropriate error message
	based on errno.  If no good message can be made up, it makes
	up a simple message.
	The second argument is a pointer to the calling functions
	name (a string); it's used in the manufactured message.
*/
int
xmsg(file,func)
char *file, *func;
{
	register char *str;
	char d[FILESIZE];
	extern int errno;
	int	fatal();

	switch (errno) {
	case ENFILE:
		return(fatal(":247:no file (ut3)"));
	case ENOENT:
		return(fatal(":242:`%s' nonexistent (ut4)",file));
	case EACCES:
		str = d;
		copy(file,str);
		file = str;
		return(fatal(":243:directory `%s' unwritable (ut2)",dname(file)));

	case ENOSPC:
		return(fatal(":244:no space! (ut10)"));
	case EFBIG:
		return(fatal(":245:write error (ut8)"));
	default:
		return(fatal(":246:errno = %d, function = `%s' (ut11)",errno,
			func));
	}
}
