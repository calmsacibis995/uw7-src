/*		copyright	"%c%" 	*/

#ident	"@(#)sco:argtype.c	1.2"

/*
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*	MODIFICATION HISTORY
 *
 *	S000	sco!kai		Oct 19, 1989	
 *	- can't just try to read 10 bytes and bail with an error message.
 *	  use st_size as an upper limit. if *any* kind of error, remain silent
 *	  and return "UNKNOWN"
 */
/* Enhanced Application Compatibility Support */
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "argtype.h"

/* Miscellaneous magic numbers */
#define PORTAR
#include "ar.h"			/* pick up ARMAG */
#include "filehdr.h"		/* pick up I386MAGIC */
#include <sys/x.out.h>		/* pick up ARCMAGIC and X_MAGIC */ /* S000 */
/* #include <sys/relsym86.h>" */	/* pick up MTHEADER */
#define MTHEADR 0x80    /* module header, usually first in a rel file */


argtype( char *arg )
{
	struct stat sb;

	if ( *arg == '-' )		/* arg is an option flag */
		return(OPT);
	if ( stat( arg, &sb ) < 0 )	/* arg doesn't exist in file system */
		return(CREAT);
	return( filtype( arg , sb.st_size) );	/* S000 */
}

filtype( char *arg , off_t size)	/* S000 */
{
	FILE *fp;
	int ret;

	if ((fp = fopen( arg, "r" )) == NULL) /* open file to see what it is */
	{   perror(arg);
	    exit(52);
	}
	ret = chk_file( fp, size);	/* S000 */
	fclose(fp);
	return( ret );
}

chk_file( FILE *fp, off_t size)	/* S000 */
{

	unsigned char buf[10];
	register unsigned short *ip;
	register int rlen,chksum;

	if(size < sizeof(unsigned short))	/* S000 */
		return(UNKNOWN);
	if((unsigned)fread( buf, sizeof(char), size < 10 ? size : 10, fp) < (size < 10 ? size : 10))	/* S000 */
		return(UNKNOWN);
	ip = (unsigned short *)buf;
	switch( *ip )
	{
	    case I386MAGIC:	
		return(COFF);
		break;
	    case X_MAGIC:
		return(XOUT);
	    case ARCMAGIC:
		return(XARCH);
	    default:
		if ( size >= 8 && strncmp( ARMAG, buf, 8 ) == 0 )    /* S000 */
		    return(UARCH);
		if ((buf[0] & 0xff) != MTHEADR) /* check for 86REL */
		    return(UNKNOWN);
		rlen = 3 + (buf[1] | (buf[2] << 8));/* record length */
		if ( rlen < 5 || rlen > 1024 )
		    return(UNKNOWN);
		fseek(fp, 0L, 0);
		chksum = 0; 
		if(rlen > size)		/* S000 */
			return(UNKNOWN);
		while (rlen--) {
		    chksum += fgetc(fp);	/* get checksum */
		}
		if (chksum & 0xff) {
		    return(UNKNOWN);		/* bad checksum */
		}
		return(OMF);
	}
}
/* End Enhanced Application Compatibility Support */
