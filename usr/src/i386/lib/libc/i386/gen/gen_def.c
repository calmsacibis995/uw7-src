#ident	"@(#)libc-i386:gen/gen_def.c	1.2"
/*
 * Contains the definition of
 * the pointer to the imported symbols 
 * end and environ and the functions exit() and _cleanup()
 */

 int (* _libc_end) = 0;

 void (* _libc__cleanup)() = 0;

 char ** (* _libc_environ) = 0;
