#ident	"@(#)kern-i386:util/kdb/kdb_util/extn.c	1.3"
/* @(#)dis_extn.c	10.2 */

#include <util/types.h>
#include <util/kdb/db_as.h>
#include <util/kdb/kdb_util/dis.h>
#include <util/kdb/kdb_util/structs.h>

/*
 *	This file contains those global variables that are used in more
 *	than one source file. This file is meant to be an
 *	aid in keeping track of the variables used.  Note that in the
 *	other source files, the global variables used are declared as
 *	'static' to make sure they will not be referenced outside of the
 *	containing source file.
 */

unsigned short	curbyte;	/* for storing the results of 'getbyte()' */
unsigned short	cur1byte;	/* for storing the results of 'get1byte()' */
unsigned short	cur2bytes;	/* for storing the results of 'get2bytes()' */
#ifdef AR32WR
	unsigned long	cur4bytes;		/* for storing the results of 'get4bytes()' */
#else
	long	cur4bytes;
#endif
char	bytebuf[4];

as_addr_t loc;		/* byte location in section being disassembled	*/
			/* IMPORTANT: remember that loc is incremented	*/
			/* only by the getbyte routine			*/
char	mneu[NLINE];	/* array to store mnemonic code for output	*/
