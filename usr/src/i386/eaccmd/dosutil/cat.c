/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/cat.c	1.2.1.4"
#ident  "$Header$"
/*	@(#) cat.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
#include	<stdio.h>
#include	"dosutil.h"
#ifdef	INTL
#include	<ctype.h>						/*L002*/
#endif

extern int Bps;

/*	cat()  --  dump a DOS file into a Unix file.  If the file is not
 *		ASCII printable, there won't be <cr><lf> conversion, unless
 *		flag is set to MAP.
 *		clustno :  starting cluster of DOS file
 *		nbytes  :  length of the DOS file in bytes
 *		outfile :  file stream into which to write the file
 *		flag  	:  RAW if no <cr><lf> conversion necessary
 *		 	   MAP if <cr><lf> conversion always required
 *			   UNKNOWN if file should be checked for conversion
 *
 */

cat(clustno,nbytes,outfd,flag)
unsigned clustno;
long nbytes;
int outfd;
int flag;
{
	unsigned clustsize, count;
	FILE *outfile;
	int n;

	clustsize = frmp->f_sectclust * Bps;

	if (flag == UNKNOWN)	/* M001 */
		flag = (canprint(clustno,nbytes) ? MAP : RAW);

#ifdef DEBUG
	fprintf(stderr,"DEBUG cat() %ld bytes starting from cluster %u\t",
			nbytes,clustno);
	fprintf(stderr,"flag = %d\n",flag);
#endif

	while (goodclust(clustno)){
		if (!readclust(clustno,buffer)){
			sprintf(errbuf,"cluster %u unreadable",clustno);
			fatal(errbuf,1);
		}
		count   = min(clustsize,nbytes);

		if (flag == RAW) {
			if ((n = write(outfd, buffer, count)) == -1) {
				sprintf(errbuf, 
				"write to a unix file failed: %s",
				 strerror(errno));	
				fatal(errbuf,1);
			}
		}
		else {/* (flag == MAP) */
				fwrcvt(buffer,1,count,outfd);
		}

		nbytes -= count;
		clustno = nextclust(clustno);
	}
	if (nbytes != 0)
		fatal("ERROR internal inconsistency in DOS disk",1);
}



/*	fwrcvt()  --  write out a DOS text file, stripping the CR character.
 *		inbuf    :  input buffer
 *		dummy    :  not used; for compatibility with fwrite()
 *		count    :  number of bytes in input buffer
 *		outfile  :  file stream into which to write the file
 */

static fwrcvt(inbuf,dummy,count,outfd)
char *inbuf;
int dummy;
unsigned count;
int outfd;
{
	static int crseen;

	while (count-- > 0) {			/* M001 begin */
		switch(*inbuf) {
		case CR:
			if (crseen) {
				if (write(outfd,"\r",1) == -1){
					sprintf(errbuf, 
					"write to a unix file failed: %s",
				 	strerror(errno));	
					fatal(errbuf,1);
				}
			}
			else
				crseen = TRUE;
			break;
		case DOSEOF:
			if (crseen) {
				if (write(outfd,"\r",1) == -1) {
					sprintf(errbuf, 
					"write to a unix file failed: %s",
				 	strerror(errno));	
					fatal(errbuf,1);
				}
				crseen = FALSE;
			}
			return;
		case '\n':
			crseen = FALSE;
			if (write(outfd,"\n",1) == -1) {
				sprintf(errbuf, 
				"write to a unix file failed: %s",
			 	strerror(errno));	
				fatal(errbuf,1);
			}
			break;
		default:
			if (crseen) {
				if (write(outfd,"\r",1) == -1) {
					sprintf(errbuf, 
					"write to a unix file failed: %s",
			 		strerror(errno));	
					fatal(errbuf,1);
				}
				crseen = FALSE;
			}
			if (write(outfd, inbuf, 1) == -1) {
				sprintf(errbuf, 
				"write to a unix file failed: %s",
			 	strerror(errno));	
				fatal(errbuf,1);
			}
		}				/* M001 end */
		inbuf++;
	}
}


/*	canprint()  --  returns TRUE if a DOS file is printable. Only
 *		the first cluster of the file is examined.  The last
 *		byte of the file is allowed to be DOSEOF.  Printable
 *		characters are:		0x07  -  0x0d
 *					0x20  -  0x7e
 *
 *		start :  starting cluster of the DOS file.
 *		nbytes: size of the file
 */

static canprint(start,nbytes)
unsigned start;
long nbytes;
{
	char *c;
	unsigned chkbytes;

	if (!readclust(start,buffer)){
		sprintf(errbuf,"cluster %u unreadable",start);
		fatal(errbuf,1);
	}
	chkbytes = min(frmp->f_sectclust * Bps, nbytes);

	for (c = buffer; c < buffer + chkbytes; c++){

		if (*c == DOSEOF)
			return( (c - buffer + 1) < nbytes ? FALSE : TRUE);

#ifdef INTL
		if (!isprint(*c) && !isspace(*c))		/*L002*/
#else
		if ((*c < 0x07) || (*c > 0x7e) || 
		    ((*c > 0x0d) && (*c < 0x20)))
#endif
			return(FALSE);
	}
	return(TRUE);
}
