/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/sarutil.c	1.7.2.1"
#ident "$Header$"


/* sarutil.c
 * 
 * Commonly used low level functions.  Most important are get_item()
 * and sar_error().
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../sa.h"
#include "sar.h"


static char *errors[] = {
	"",				/* SARERR_BLANK - used internally */
	"Illegal output mode.",
	"Illegal output field.",
	"Initialization error.",
	"File system names mismatch.",
	"KMA pool sizes mismatch.",
	"Couldn't invoke sadc.",
	"Error reading input.",
	"Couldn't open input file.",
	"Illegal argument to -s",
	"Illegal argument to -e",
	"Illegal argument to -i",
	"usage:\n sar [-P 0,1...|ALL] [-ubdycwaqvtmpgrkAR] [-o file] t [n]\n sar [-P 0,1...|ALL] [-ubdycwaqvtmpgrkAR] [-s time] [-e time] [-i sec] [-f file]",
	"usage:\n hwsar [-P 0,1...|ALL] [-N 0,1,...|ALL] [-H meterlist] [-ubdycwaqvtmpgrkAR] [-o file] t [n]\n sar [-P 0,1...|ALL] [-N 0,1,...|ALL] [-H meterlist] [-ubdycwaqvtmpgrkAR] [-s time] [-e time] [-i sec] [-f file]",
	"Unknown argument",
	"Memory error",
	"Inconsistent INIT record",
	"CG count mismatch.",
};



/* get_item(void *buffer, int bufsize, int item_size, FILE *infile)
 * buffer - memory block for incoming data.
 * bufsize - size of buffer
 * item_size - size of data item in the data file
 * infile - input file stream
 *
 * Retrieves a single data item from the input file.  bufsize
 * is the expected size of the data item, item_size is the size
 * read from the current record's reader.  The smaller number
 * of bytes is read, and any extra data in the file skipped.
 *
 * WARNING: If bufsize > item_size, the memory in the buffer
 * after the first item_size bytes are left untouched, they
 * are not 'cleared' or initialized.  The routine calling 
 * get_item is responsible for initializing this memory if
 * necessary.
 * 
 */


int
get_item(void *buffer, int bufsize, int item_size, FILE *infile)
{
	int   to_read = MIN(bufsize, item_size);
	
	if (fread(buffer, to_read, 1, infile) != 1) {
		sar_error(SARERR_READ);
	}
	
	if (to_read < item_size) {
		if (skip(infile, item_size - to_read) == -1) {
			sar_error(SARERR_READ);
		}
	}
	return(0);
}

int
diff_ratio_p(int x, int y)
{
	if (y == 0) {
		sarerrno = SARERR_OUTBLANK;
		return(-1);
	}
	else {
		return(x/y);
	}
}

diff_percent_p(x, y)
{  
	if (y == 0) {
		sarerrno = SARERR_OUTBLANK;
		return(-1);
	}
	else {
		return(100 * x/y);
	}
}


void
sar_error(int errcode)
{
	if (errcode == SARERR_USAGE) {
		fprintf(stderr, "%s\n", errors[errcode - 1]);
	}
	else {
		fprintf(stderr, "sar: %s\n", errors[errcode - 1]);
		if (errcode == SARERR_ILLARG_UNKNOWN) {
			fprintf(stderr, "%s\n", errors[errcode - 1]);
		}
	}
	exit(errcode);
}
