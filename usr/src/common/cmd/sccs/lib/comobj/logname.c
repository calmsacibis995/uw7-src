#ident	"@(#)sccs:lib/comobj/logname.c	6.6"
# include	<pwd.h>
# include	<sys/types.h>
# include	<macros.h>
# include	<ccstypes.h>


char	*logname()
{
	struct passwd *getpwuid();
	struct passwd *log_name;
	uid_t	getuid();
	uid_t uid;

	uid = getuid();
	log_name = getpwuid(uid);
	endpwent();
	if (! log_name)
		return(0);
	else
		return(log_name->pw_name);
}
