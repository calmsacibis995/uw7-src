/*		copyright	"%c%" 	*/

#ident	"@(#)regex.h	1.2"
#ident	"$Header$"


#if	defined(__STDC__)

int		match ( char * , char * );
size_t		replace ( char ** , char * , char * , int );

#else

int		match();
size_t		replace();

#endif
