#ident	"@(#)check_uart.c	15.1"

#include <fcntl.h>

main( argc, argv )
	int argc;
	char *argv[];
{
	exit( open( argv[1], O_RDONLY|O_NDELAY, 0 ) );
}
