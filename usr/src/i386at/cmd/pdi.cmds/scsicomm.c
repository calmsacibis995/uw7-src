#ident	"@(#)pdi.cmds:scsicomm.c	1.8.6.1"

/*  "error()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_ERROR>.
 *
 *  "warning()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_WARNING>.
 */


#include	<sys/types.h>
#include	<sys/mkdev.h>
#include	<sys/stat.h>
#include	<sys/sdi_edt.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<sys/sysi86.h>
#include	<sys/vtoc.h>
#include	<sys/sd01_ioctl.h>
#include	<string.h>
#include	<stdio.h>
#include	"mirror.h"
#include	"scsicomm.h"
#include 	<ctype.h>
#include	<pfmt.h>
#include	<locale.h>

extern int	errno;
extern int	Debug;

/*
 * read the next line from a file, skipping over white space
 * and comments and place each field into a character array. 
 * Returns the number of fields read or EOF.
 */

int
ParseLine(array,fp, numfields)
char array[][MAXFIELD];
FILE *fp;
int numfields;
{
	int	i, j;
	char	ch;

	/*  inititalize the array to null strings  */
	for (i=0; i<numfields; i++) 
		array[i][0] = '\0';

	/*  skip over comment lines , reading the remainder of the line  */
	while ((ch = getc(fp)) == '#') {
		fscanf(fp,"%*[^\n]%*[\n]");
	}
	if (ungetc(ch,fp) == EOF)
		return(EOF);

	j = 0;
	for (i=0; i<numfields; i++) {
		if(fscanf(fp,"%s",array[i]) == EOF) {
			j = 0;
			break;
		}
		j++;
	}
	/*  read the remainder of the line  */
	fscanf(fp,"%*[^\n]%*[\n]");
	if (j == 0) {
		return(EOF);
	} else {
		if (j != numfields) {
			errno = 0;
			warning(":367:Number of columns incorrect in file.\n");
		}
		return(j);
	}
}

/*
 *  Routine to read in the pdsector and VTOC.
 *	Inputs:  devname - pointer to the device name
 *	         vtocptr - pointer to where to put the VTOC
 *	Return:  1 if pdsector VTOC was read in
 *		 0 otherwise.
 */

int
rd_vtoc(dpart,vbuf)
char	*dpart;		/* Disk partition name */
struct	vtoc_ioctl	*vbuf;	/* buf for VTOC data */
{
	int fd;

	/* Open Slice 0 */
	if ((fd = open(dpart,O_RDONLY)) == -1) {
		return 0;
	}

	/* Get VTOC */
	if (ioctl(fd, V_READ_VTOC, vbuf) == -1) {
		return 0;
	}

	close(fd);
	return 1;
}


/*
 *  Routine to print out error information.
 *	Inputs:  message - pointer to the string of the error message
 *	         data1...data5 - pointers to additional arguments
 *
 *	NOTE:  This routine does not return.  It exits.
 */

void
error(message, data1, data2, data3, data4, data5)
char	*message;	/* Message to be reported */
long	data1;		/* Pointer to arg	 */
long	data2;		/* Pointer to arg	 */
long	data3;		/* Pointer to arg	 */
long	data4;		/* Pointer to arg	 */
long	data5;		/* Pointer to arg	 */
{
	(void) fflush(stdout);
	if (message[0] == ':')
		(void) pfmt(stderr, MM_ERROR,
			message, data1, data2, data3, data4, data5);
	else 	{
		(void) fprintf(stderr, "ERROR: ");
		(void) fprintf(stderr, message, data1, data2, data3, data4, data5);
	}
	if (errno)
		pfmt(stderr, MM_ERROR,
			":374:system call error is: %s\n", strerror(errno));

	exit(ERREXIT);
}

/*
 *  Routine to print out warning information.
 *	Inputs:  message - pointer to the string of the warning message
 *	         data1...data5 - pointers to additional arguments
 */

void
warning(message, data1, data2, data3, data4, data5)
char	*message;	/* Message to be reported */
long	data1;		/* Pointer to arg	 */
long	data2;		/* Pointer to arg	 */
long	data3;		/* Pointer to arg	 */
long	data4;		/* Pointer to arg	 */
long	data5;		/* Pointer to arg	 */
{
	(void) fflush(stdout);
	if (message[0] == ':')
		(void) pfmt(stderr, MM_WARNING,
			message, data1, data2, data3, data4, data5);
	else 	{
		(void) fprintf(stderr, "WARNING: ");
		(void) fprintf(stderr, message, data1, data2, data3, data4, data5);
	}
	if (errno) {
		pfmt(stderr, MM_ERROR,
			":374:system call error is: %s\n", strerror(errno));
		errno = 0;
	}
}

void
nowarning(message, data1, data2, data3, data4, data5)
char	*message;	/* Message to be reported */
long	data1;		/* Pointer to arg	 */
long	data2;		/* Pointer to arg	 */
long	data3;		/* Pointer to arg	 */
long	data4;		/* Pointer to arg	 */
long	data5;		/* Pointer to arg	 */
{
	return;
}

/*
 *  Return the name of the block device given the name of the
 *  character device.
 *	Inputs:  rpart - pointer to the name of the character partition.
 *	         bpart - address of where to put the block device name
 *	Return:  1 if constructed the block device name
 *		 0 otherwise.
 */

int
get_blockdevice(rpart, bpart)
char	*rpart, *bpart;
{
	char	*ptr;
	struct	stat	statbuf;

	/*  Verify rpart is a character device special file.  */
	if (stat(rpart, &statbuf) == -1)
	{
		return(0);
	}

	if ((statbuf.st_mode & S_IFMT) != S_IFCHR)
	{
		return(0);
	}

	strcpy(bpart, rpart);
	if ((ptr = strchr(rpart,'r')) == NULL)
	{
		return(0);
	}
	strcpy(strchr(bpart, 'r'), ++ptr);
	return(1);
}

/*
 *  Swap bytes in a 16 bit data type.
 *	Inputs:  data - long containing data to be swapped in low 16 bits
 *	Return:  short containing swapped data
 */

short
scl_swap16(data)
unsigned long	data;
{
	unsigned short	i;

	i = ((data & 0x00FF) << 8);
	i |= ((data & 0xFF00) >> 8);

	return (i);
}

/*
 *  Swap bytes in a 24 bit data type.
 *	Inputs:  data - long containing data to be swapped in low 24 bits
 *	Return:  long containing swapped data
 */

long
scl_swap24(data)
unsigned long	data;
{
	unsigned long	i;

	i = ((data & 0x0000FF) << 16);
	i |= (data & 0x00FF00);
	i |= ((data & 0xFF0000) >> 16);

	return (i);
}

/*
 *  Swap bytes in a 32 bit data type.
 *	Inputs:  data - long containing data to be swapped
 *	Return:  long containing swapped data
 */

long
scl_swap32(data)
unsigned long	data;
{
	unsigned long	i;

	i = ((data & 0x000000FF) << 24);
	i |= ((data & 0x0000FF00) << 8);
	i |= ((data & 0x00FF0000) >> 8);
	i |= ((data & 0xFF000000) >> 24);

	return (i);
}

