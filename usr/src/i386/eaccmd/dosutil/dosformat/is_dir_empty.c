/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/is_dir_empty.c	1.1.1.2"
#ident  "$Header$"

/*
	Pass this routine a handle and a displacement,
	indicating a directory entry for an MS-DOS sub directory,
	and it will return a 1 if the sub directory is empty,
	a zero if not, and a 0 on error.
*/

#include	"MS-DOS.h"

#include	<stdio.h>

is_dir_empty(handle, disp)
int	handle;
long	disp;
{
	int	index;
	long	sector;

	/*
		Locate this handle in our device table
	*/
	if ((index = lookup_device(handle)) == -1) {
		(void) fprintf(stderr, "is_dir_empty(): Error - Handle %d not found in device table\n", handle);
		return(-1);
	}

	/*
		Is it really a sub directory?
	*/
	if (sector_buffer[FILE_ATTRIBUTE + disp] != SUB_DIRECTORY)
		return(0);

	/*
		Determine first sector of the sub directory's
		data space.
	*/
	sector = CLUS_2_SECT(GET_CLUS(disp));

	if (read_sector(handle, sector) == -1)
		return(0);

	/*
		For the directory to be considered empty,
		there must be no more than 2 entries ("." and
		"..").
	*/
	if (sector_buffer[0] != '.' || sector_buffer[1] != ' ' || sector_buffer[BYTES_PER_DIR] != '.' || sector_buffer[BYTES_PER_DIR + 1] != '.' || sector_buffer[BYTES_PER_DIR + 2] != ' ') {
		(void) fprintf(stderr, "is_dir_empty(): Error - Corrupted directory entry found\n");
		return(0);
	}

	switch(sector_buffer[BYTES_PER_DIR * 2]) {
		case 0x00:
		case 0xE5:
			return(1);

		default:
			return(0);
	}
}
