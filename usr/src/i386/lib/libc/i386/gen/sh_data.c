#ident	"@(#)libc-i386:gen/sh_data.c	1.4"

#ifdef DSHLIB
char	**_environ;
#else
#pragma weak environ = _environ
char	**_environ = 0;
#endif
