#pragma ident	"@(#)mmisc:tools/MSGTools/mkcatdefs/catio.c	1.1"
static char sccsid[] = "@(#)22	1.6  com/cmd/msg/catio.c, bos, bos320 5/29/91 10:23:05";
/*
 * COMPONENT_NAME:  (CMDMSG) Message Catalogue Facilities
 *
 * FUNCTIONS: descopen, descgets, descclose, descset, descerrck
 *
 * ORIGINS: 27
 *
 * IBM CONFIDENTIAL -- (IBM Confidential Restricted when
 * combined with the aggregated modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 1988, 1989, 1991
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */
/*
 * @OSF_COPYRIGHT@
 */

#include <stdio.h>
FILE *descfil;

/*
 * EXTERNAL PROCEDURES CALLED: None
 */


/*
 * NAME: descopen
 *
 * FUNCTION: Perform message utility input operations
 *
 * EXECUTION ENVIRONMENT:
 *	User Mode.
 *
 * NOTES: The routines in this file are used to provide a mechanism by which
 *     	  the routines in gencat can get the next input character without
 *  	  worrying about details such as if the input is coming from a file
 *  	  or standard input, etc.
 *
 * RETURNS : 0 - successful open
 *          -1 - open failure
 */


/*   Open a catalog descriptor file and save file descriptor          */ 
int 
#ifdef _NO_PROTO
descopen(filnam)
char *filnam;
#else
descopen(char *filnam) 
#endif /* _NO_PROTO */
			/*
			  filnam - pointer to message catalog name
			*/
{
	if ((descfil = fopen(filnam,"r")) == 0) 
		return (-1);
	
return (0);
}



/*
 * NAME: descgets
 *
 * FUNCTION: Read message catalog
 *
 * EXECUTION ENVIRONMENT:
 *	User mode.
 *
 * NOTES: Read the next line from the opened message catalog descriptor file.
 *
 * RETURNS: Pointer to message buffer -- scccessful
 *          NULL pointer -- error or end-of-file
 */

char 
#ifdef _NO_PROTO
*descgets(buff, bufsize)
char *buff;
int bufsize;
#else
*descgets(char *buff, int bufsize)
#endif /* _NO_PROTO */
			/*
			  buff - pointer to message buffer
			  bufsize - size of message buffer in bytes
			*/
{
	char *str;

	str = fgets(buff, bufsize, descfil);
	buff[bufsize-1] = '\0';        /* terminate in case length exceeded */
	return (str);
}



/*
 * NAME: descclose
 *
 * FUNCTION: Close message catalog
 *
 * EXECUTION ENVIRONMENT:
 *	User mode.
 *
 * NOTES: Close the message catalog descriptor file.
 *
 * RETURNS: None
 */


void
#ifdef _NO_PROTO
descclose()
#else
descclose()
#endif /* _NO_PROTO */
{
	fclose(descfil);
	descfil = 0;
}



/*
 * NAME: descset
 *
 * FUNCTION: Establish message catalog file descriptor
 *
 * EXECUTION ENVIRONMENT:
 *	User mode.
 *
 * NOTES: Set the file descriptor to be used for message catalog access.
 *        
 *
 * RETURNS: None
 */


void
#ifdef _NO_PROTO
descset(infil)
FILE *infil;
#else
descset(FILE *infil)
#endif /* _NO_PROTO */
{
	descfil = infil;
}


/*
 * NAME: descerrck
 *
 * FUNCTION: Check I/O stream status
 *
 * EXECUTION ENVIRONMENT:
 *	User mode.
 *
 * RETURNS: 0 - no error encountered
 *         -1 - error encountered
 */
int
#ifdef _NO_PROTO
descerrck()
#else
descerrck()
#endif /* _NO_PROTO */
{
	if (ferror(descfil))
		return(-1);
	else    return(0);
}

