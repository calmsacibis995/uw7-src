/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/dosformat.c	1.3.5.1"
#ident  "$Header$"

/*
 * Revision History:
 *
 * L000		peters		17 Sep 97
 *	- The calculation of 'sectors_per_fat' should round upwards,
 *	  not truncate!
 *	- When writing the first FAT field, check 'fat16' to determine
 *	  whether 16 bits (rather than 12) should be written.
 *	- Previously this program would only allow drives up to 32Mb
 *	  to be formatted, since the number of sectors is kept in a short
 *	  (DOS 3.3).  To cope with larger drives, the extensions provided
 *	  in later format BPB's (DOS 5.0) have been implemented.
 */

#include	<sys/types.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<sys/errno.h>
#include	<sys/param.h>
#include	<sys/vtoc.h>
#include	<sys/fdisk.h>
#include	<time.h>
#include	<fcntl.h>
#include	<signal.h>
#include	<string.h>
#include	"MS-DOS.h"
#include	"MS-DOS_boot.h"

static char	*mkfsopts[] = {
#define V_FLAG		0
	"v",
#define Q_FLAG		1
	"q",
#define DOS3_FLAG	2				/* L000 */
	"3",						/* L000 */
#define DOS5_FLAG	3				/* L000 */
	"5",						/* L000 */
	NULL
};

static struct	{
	int		sectors_per_cluster;
	int		total_sectors;
	int		sectors_per_fat;
	int		root_dir_ent;
	unsigned	media_descriptor;
}
drive_table[] = {{1, 320, 1, 64, 0xFE},
		{2, 640, 1, 112, 0xFF},
		{1, 360, 2, 64, 0xFC},
		{2, 720, 2, 112, 0xFD},
		{1, 2400, 7, 224, 0xF9},
		{2, 1440, 3, 112, 0xF9},
		{1, 2880, 9, 224, 0XF0},
		{1, 1232, 2, 192, 0XFE},
		{1, 1, 0, 0, 0XF8},
		{0, 0, 0, 0}};

static struct	table_struct	odev;
static struct	disk_parms	parms_struct;
static uchar_t	sector_buffer[MAX_SECTOR_SIZE];
static char	usage[80];
static char	device[MAXPATHLEN];
static char	zero[MAX_SECTOR_SIZE];
static char	dbuffer[20];
static int 	dos_version;
static int	label_volume = 0;
char	*cmd;

/* signal handler */
static void
interrupt(int signum)
{
	(void) fprintf(stderr, "\ndosformat: Signal caught %d  - Terminating\n\tDisk is unusable\n", signum);
	exit(2);
}

/*
 * check type of file system.
 * Currently we support dos3 or dos5.
 */
static void
checktype(void)
{
	int ispart, fd;
	unsigned int	 len;
	struct	absio	absbuf;
	struct	mboot	masterboot;
	char partdev[MAXPATHLEN];

	len = strlen(device);
	if (device[len-2] == 'p' && device[len-1] >= '1' && 
	    device[len-1] <= '9') {
		(void) strcpy(partdev, device);
		partdev[len - 1] = '0';
    		ispart = B_TRUE;
	} else {
		ispart = B_FALSE;
	} 

#ifdef DEBUG
(void) fprintf(stderr, "getting masterboot from \"%s\"\n", partdev);
#endif
	if (ispart) {
		absbuf.abs_sec = 0;
		absbuf.abs_buf = (char *)&masterboot;
		fd = open(partdev, O_RDONLY);
		/* read partition table from disk with new ioctl */
		if (fd != -1 && ioctl(fd, V_RDABS, &absbuf) != -1) {
			int i;
			struct	ipart	*ptp;
			for (i = 0, ptp = (struct ipart *)&masterboot.parts;
					i < FD_NUMPART; ++i, ++ptp) {
#ifdef DEBUG
(void) fprintf(stderr, "part %u: (%u,%u,%u)\n", i, ptp->systid,
 ptp->relsect, ptp->numsect);
#endif
				if (ptp->systid != UNUSED 
				   && ptp->relsect == parms_struct.dp_pstartsec
				   && ptp->numsect == parms_struct.dp_pnumsec) {
					switch (ptp->systid) {
					case DOSOS12:
					case DOSOS16:
						dos_version = 3;
						break;
					case DOSHUGE:
						dos_version = 5;
						break;
					default:
						continue;
					}
					break;
				}
			}
			close(fd);
		}
   	}
} 

/*
 * Get the BIOS information and stores them into odev. 
 * 
 * We could eliminate this function.  However, this tells us
 * that we have written the boot sector properly. 
 *
 */ 
static int 
get_bios_info(int handle)
{
#ifdef DEBUG
	FILE	*output;
	char	identifier[30];

	output = stderr;
	(void) strcpy(identifier, "get_bios_info(): DEBUG - ");
#endif


	odev.handle = handle;

	odev.bytes_per_sector =  parms_struct.dp_secsiz;
	/* Read partial boot sector */
	if (read_sector(handle, (long)0, (char *)sector_buffer) == -1)
		return(-1);

	if (sector_buffer[0] != 0xE9 && sector_buffer[0] != 0xEB) {
		(void) fprintf(stderr, "Non MS-DOS disk\n");
		return(-1);
	}

	/* Get all pertinent info from boot sector */
	odev.bytes_per_sector = getushort(&sector_buffer[SECTOR_SIZE]);
	odev.sectors_per_cluster = getubyte(&sector_buffer[SECTORS_PER_CLUSTER]);
	odev.reserved_sectors = getushort(&sector_buffer[RESERVED_SECTORS]);
	odev.number_of_fats = getubyte(&sector_buffer[NUMBER_OF_FATS]);
	odev.root_dir_ent = getushort(&sector_buffer[ROOT_DIR_ENT]);
	odev.total_sectors = getushort(&sector_buffer[TOTAL_SECTORS]);
	odev.sectors_per_fat = getushort(&sector_buffer[SECTORS_PER_FAT]);

	odev.sectors_in_root = (odev.root_dir_ent * BYTES_PER_DIR) / odev.bytes_per_sector;
	odev.root_base_sector = odev.reserved_sectors + (odev.number_of_fats * odev.sectors_per_fat);

	/* Handle DOS 5.0 partitions > 32Mb */
	if (odev.total_sectors == 0)
		odev.total_sectors = getulong(&sector_buffer[HUGE_SECTORS]);

	/* We must support 2 FAT entry sizes 16 and 12 bit */
	if (4095 > TOTAL_CLUSTERS)
		odev.fat16 = 0;
	else
		odev.fat16 = 1;

	odev.media_descriptor = getubyte(&sector_buffer[MEDIA_DESCRIPTOR]);
#ifdef DEBUG
	/* Display data if requested */
	(void) fprintf(output, "%sBytes per sector = %d\n", identifier, odev.bytes_per_sector);
	(void) fprintf(output, "%sSectors per cluster = %d\n", identifier, odev.sectors_per_cluster);
	(void) fprintf(output, "%sReserved sectors = %d\n", identifier, odev.reserved_sectors);
	(void) fprintf(output, "%sNumber of FATs = %d\n", identifier, odev.number_of_fats);
	(void) fprintf(output, "%sRoot directory entries = %d\n", identifier, odev.root_dir_ent);
	(void) fprintf(output, "%sTotal sectors on volume = %d\n", identifier, odev.total_sectors);
	(void) fprintf(output, "%sSectors per FAT = %d\n", identifier, odev.sectors_per_fat);
	(void) fprintf(output, "%sfat16 = %d\n", identifier, odev.fat16);
	(void) fprintf(output, "%sMedia Descriptor = %x\n", identifier, odev.media_descriptor);
	(void) fprintf(output, "%sBase sector of root directory = %ld\n", identifier, odev.root_base_sector);
	(void) fprintf(output, "%sSectors in root directory = %ld\n", identifier, odev.sectors_in_root);

#endif
	return(0);
}

/* 
 * Read a given sector in the buffer specified by bufp.
 */
static int
read_sector(int device_handle, long sector, char *bufp)
{
	int	bytes_read;
	long	offset;

#ifdef DEBUG
(void) fprintf(stderr, "read_sector(): DEBUG - Reading sector %ld\n", sector);
#endif

	/* Calculate the actual displacement into the media */
	offset = odev.bytes_per_sector * sector;

	/* Seek to the appropriate address for this sector */
	if (lseek(device_handle, offset, 0) == -1) {
		(void) fprintf(stderr, "read_sector(): Failed to lseek to offset %ld\n", offset);
		perror("	Reason");
		return(-1);
	}

	/* Do the actual read. We should never have short blocks.  */
	if ((bytes_read = read(device_handle, bufp, odev.bytes_per_sector)) != odev.bytes_per_sector) {
		(void) fprintf(stderr, "read_sector(): Read error got %d vs %d\n", bytes_read, odev.bytes_per_sector);
		return(-1);
	}

#ifdef DEBUG
	(void) fprintf(stderr, "read_sector(): DEBUG - Read of sector %ld complete - Location: %ld - No errors\n", sector, offset);
#endif
	return(bytes_read);
}

/* 
 * Write buffer at the specified offset.
 */
static int
write_bsect(int handle, char *bufp, uint_t offset, uint_t size)
{

#ifdef DEBUG
(void)fprintf(stderr, "write_bsect(): size %d, offset %d\n",size, offset);
#endif
	/*
	 * seek to the appropriate address for this sector
	 */
	if (lseek(handle, offset, 0) == -1) {
		(void) fprintf(stderr, "write_bsect(): Failed to lseek to offset %d\n", offset);
		return(-1);
	}
	if (write(handle, bufp, size ) != size ) {
		(void) fprintf(stderr, "write_bsect(): write error, Is the disk full?\n");
		return(-1);
	}
	return(0);
}

/*
 *  Convert a unix filename to a DOS filename.
 */
static void
unix2dos(char *ubufp, char *dbufp)
{
	int i; 
	unsigned int ulen;
	int j = 0;
	int dotfound;

	ulen = strlen(ubufp);

#ifdef DEBUG
(void) fprintf(stderr, "ulen =  %d ubufp=  %s\n",ulen, ubufp);
#endif
	/*
	 * Parse the name string looking for an invalid character.
	 */
	for (i = 0; i < ulen; i++) {
		switch (ubufp[i]) {
		case '"':
		case '\\':
		case '/':
		case '[':
		case ']':
		case ':':
		case '*':
		case '<':
		case '>':
		case '|':
		case '+':
		case '=':
		case ';':
		case '\'':
		case '?':
			break;
		case '.':
			if (i != 0) 
				dbufp[j++] = ubufp[i];
			dotfound = i;
			break;
		default:
			dbufp[j++] = ubufp[i];
			break;
		}
	}


	ulen = strlen(dbufp);
#ifdef DEBUG
(void) fprintf(stderr,"ulen = %d dbufp =  %s dotfound =  %d\n",ulen, dbufp, dotfound);
#endif
	if (dotfound == 0) {
		if  (ulen < 8) {
			dbufp[ulen+1] = '\0';
		} else
			dbufp[8] = '\0';
		return;
	}
		  
	if (dotfound != 8) {
		if (dotfound > 8) {
			dbufp[8] = '.'; 
			j = 9;
		} else
			j = dotfound + 1;
		i = (dotfound + 3) > ulen ? ulen : (dotfound + 3);
#ifdef DEBUG
(void)fprintf(stderr, "ulen = %d dotfound = %d i = %d j = %d\n",ulen, dotfound, i, j);
#endif
		for (;dotfound < i; j++)
			dbufp[j] = dbufp[++dotfound]; 
#ifdef DEBUG
(void) fprintf(stderr, "ulen = %d dotfound = %d i = %d j = %d\n",ulen, dotfound, i, j);
#endif
		dbufp[j] ='\0'; 
	} else {
		j = (dotfound + 4) > ulen ? ulen : (dotfound + 4);
		dbufp[j] ='\0'; 
	}

	
#ifdef DEBUG
(void) fprintf(stderr, "unix2dos(): DEBUG - label name = \"%s\"\n", dbufp);
#endif
	return;
}

/*
 * make_label(handle, buffer)
 * 	Creates volume label for the specified handle.
 */
static int
make_label(int handle, char *bufp)
{
	uint_t	len;
	int	i, j;
	long	right_now;
	uint_t	offset;
	struct	tm	*time_ptr;

	/*
	 * Now the user may have just hit <RETURN>.
	 * If so, then the length of the reply is one.
	 * We just ignore the rest of this code.
	 */
	len = strlen(bufp);
	if (len < 2)
		return(0);
	unix2dos(bufp, dbuffer);

#ifdef DEBUG
	(void) fprintf(stderr, "make_label(): DEBUG - Label = \"%s\"\n", dbuffer);
#endif

	if (dos_version == 5) {
		int size; 
		/*
		 * Read in boot sector
		 */
		lseek(handle, 0, 0);
		size = parms_struct.dp_secsiz;
		if (read(handle, zero, size) != parms_struct.dp_secsiz) {
			(void) fprintf(stderr, "%s: Error - Failed to read boot sector\n", cmd);
			perror("	Reason");
		}

		if (label_volume)
			(void) strncpy(&zero[VOLUME_LABEL], dbuffer, 11);
		else
			(void) strncpy(&zero[VOLUME_LABEL], "NO NAME    ", 11);
		if (odev.fat16)
			(void) strncpy(&zero[FILESYS_TYPE], "FAT16   ", 8);
		else
			(void) strncpy(&zero[FILESYS_TYPE], "FAT12   ", 8);

		/*
		 * Write out patched boot sector
		 */
		if (write_bsect(handle, zero, 0, size) == -1) { 
			(void) fprintf(stderr, "%s: Error - Failed to write patched boot sector\n", cmd);
			perror("	Reason");
		}

		return(0);
	}

	/*
	 * Get current time
	 */
	right_now = time((long *) 0);
	time_ptr = localtime(&right_now);

	/*
	 * put this file
	 */
	if (read_sector(handle, odev.root_base_sector, (char *)sector_buffer) == -1) {
		(void) fprintf(stderr, "%s: Error - Failed to read boot sector\n", cmd);
		perror("	Reason");
		exit(1);
	}
	if (sector_buffer[0] != 0xE5 && sector_buffer[0] != 0x00) { 
		(void) fprintf(stderr, "%s: boot sector has invalid infomation\n", cmd);
		exit(1);
	}
	/* write file name */
	for (i = 0; i < 11; i ++) 
		sector_buffer[i] = ' ';
	for (i = 0; i < 8; i ++) {
		if (dbuffer[i] != '.')
			sector_buffer[i] = dbuffer[i];
		else 
			break;
	}
	
	if (dbuffer[i] == '.') {
		for (j = 8; j < 11 && dbuffer[++i] != '\0'; j++) 
			sector_buffer[j] = dbuffer[i];
	}
#ifdef DEBUG
	fprintf(stderr, "sector_buffer %s\n", sector_buffer);
#endif

	/* Next is attribute */
	sector_buffer[FILE_ATTRIBUTE] = LABEL;

	/* Next is time */
	sector_buffer[TIME + 1] = ((time_ptr->tm_hour & 0x1F) << 3);
	sector_buffer[TIME + 1] |= ((time_ptr->tm_min & 0x38) >> 3);
	sector_buffer[TIME] = ((time_ptr->tm_min & 0x07) << 5);

	/* Next is date */
	sector_buffer[DATE + 1] = (((time_ptr->tm_year - 80) & 0x7F) << 1);
	sector_buffer[DATE + 1] |= (((time_ptr->tm_mon + 1) & 0x08) >> 3);
	sector_buffer[DATE] = (((time_ptr->tm_mon + 1) & 0x07) << 5);
	sector_buffer[DATE] |= (time_ptr->tm_mday & 0x1F);

	/* Size is zero - 4 bytes */
	sector_buffer[FILE_SIZE] = '\0';
	sector_buffer[FILE_SIZE + 1] = '\0';
	sector_buffer[FILE_SIZE + 2] = '\0';
	sector_buffer[FILE_SIZE + 3] = '\0';

	sector_buffer[STARTING_CLUSTER] = '\0';
	sector_buffer[STARTING_CLUSTER + 1] = '\0';


	offset = odev.root_base_sector * odev.bytes_per_sector;
	if (write_bsect(handle, (char *)sector_buffer, offset, odev.bytes_per_sector ) == -1) {
		(void) fprintf(stderr, "%s: make_label(): Failed to update directory, sector %ld\n\tDisk may be unusable\n", cmd, odev.root_base_sector);
		exit(1);
	}

	return(0);
}

/*
 * Calculate the free space.
 */
static long
free_space(void)
{
	long	cluster;
	int	fat_offset;
	long	fat_entry;
	long	free_clusters = 0;


	/* Loop over each cluster in the FAT */
	for (cluster = 2; cluster < TOTAL_CLUSTERS; cluster++) {
#ifdef DEBUG_FREE
(void) fprintf(stderr, "free_space(): DEBUG - Fat cluster %ld\n", cluster);
#endif

		/*
		 * Determine which sector in the FAT to use 
		 * Displacement is the number of BITS (NOT BYTES) 
		 * into the FAT that we can expect to find our 
		 * next entry in the cluster list for this file.
		 *
		 * if we are using 12 bit FAT entries then we 
		 * have some math to do here.
		 */
		if (odev.fat16 == 0) {
			/* 
			 * If our current cluster is even - keep 
			 * low order 12 bits, otherwise high-order
			 * 12 bits.
			*/
			fat_offset = (cluster * 3) / 2;

#ifdef DEBUG_FREE
(void) fprintf(stderr, "free_space(): DEBUG - Fat offset = %d\n", fat_offset);
#endif
	
			if ((cluster / 2) * 2 == cluster) { /* Even */
				fat_entry = (*(odev.our_fat + fat_offset) + 
					*(odev.our_fat + fat_offset + 1)) & 0xFFF0;

#ifdef DEBUG_FREE
(void) fprintf(stderr, "free_space(): DEBUG - Cluster %ld is even %x %x\n", cluster, (unsigned) *(odev.our_fat + fat_offset), (unsigned) *(odev.our_fat + fat_offset + 1));
#endif
			}
			else {
				fat_entry = ((*(odev.our_fat + fat_offset) + *(odev.our_fat + fat_offset + 1)) & 0x0FFF) >> 4;

#ifdef DEBUG_FREE
(void) fprintf(stderr, "free_space(): DEBUG - Cluster %ld is odd %x %x\n", cluster, *(odev.our_fat + fat_offset), *(odev.our_fat +fat_offset + 1));
#endif
			}
		} else {
			fat_offset = cluster * 2;
			fat_entry = *(odev.our_fat + fat_offset + 1) + *(odev.our_fat + fat_offset);
		}

#ifdef DEBUG_FREE
(void) fprintf(stderr, "free_space(): DEBUG - cluster = %ld fat_entry = %x\n", cluster, fat_entry);
#endif

		/* Check FAT entry */
		if (fat_entry == 0) {
			free_clusters++;
		}
	}

	/*
	 * return free_space
	 */
#ifdef DEBUG
	(void) fprintf(stderr, "free_space(): DEBUG - Free clusters %ld\n", free_clusters);
#endif
	return((free_clusters * odev.sectors_per_cluster) * odev.bytes_per_sector);
}

int
main(int argc, char **argv)
{
	char	buffer[30];
	int	dindex;
	int	handle;
	int	i, c;
	char	*options, *value;
	int	dos5;			
	int 	nentry;
	int	quiet = 0;
	int	silent = 0;
	int	hard_drive = 0;
	uint_t	size;
	uint_t	offset;
	union io_arg	io_arg_union;


	if (cmd = strrchr(argv[0], '/'))
		cmd++;
	else
		cmd = argv[0];

	if (strcmp(cmd, "mkfs") == 0) {
	    	(void) strcpy(usage, "Usage: %s -F dosfs [-o specific_options] special\n");
		quiet = 1;
	} 
		(void) strcpy(usage, "Usage: %s [-fqv2] drive\n");

	while ((c = getopt(argc, argv, "fqv2o:")) != EOF)
		switch(c) {
		case 'f':
			quiet = 1;
			break;

		case 'q':
			silent = 1;
			break;

		case 'v':
			label_volume = 1;
			break;

		case '2':
			break;

		case 'o':
			/*
			 * dosfs mkfs specific options
			 * This is the only way the generic mkfs command
			 * can pass options to the dosfs mkfs.
			 */


			options = optarg;
			while (*options != '\0') {
				switch (getsubopt(&options, mkfsopts, &value)) {
				case V_FLAG:
					label_volume = 1;
					break;
				case Q_FLAG:
					silent = 1;
					break;
				case DOS3_FLAG:
					dos_version = 3;
					break;
				case DOS5_FLAG:
					dos_version = 5;
					break;
				default:
					(void) fprintf(stderr,
					    "%s: illegal -o suboption: %s\n",
						cmd, value);
					(void) fprintf(stderr, usage, cmd); 
					exit(1);
				}
			}
			break;

		default:
			(void) fprintf(stderr, usage, cmd);
			exit(1);
		}

	/*
	 * The device must be a special file name and not A: etc.
	 * If the passed device is not in the form /dev/...
	 * then error.  
	 */

	(void) strcpy(device, argv[optind]);
	if ((strlen(device) < (uint_t)6) || (strncmp(device, "/dev/", 5) != 0)) {
		(void) fprintf(stderr, "%s: Device must be a special file name such as /dev/rdsk/f03ht\n", cmd);
		exit(1);
	}


	/*
	 * Prompt before we begin - if requested
	 */
	if (quiet == 0) {
		(void) printf("Insert new diskette for %s\n", device);
		(void) printf("and press <RETURN> when ready ");
		(void) fflush(stdout);

		(void)fgets(buffer, 20, stdin);
	}

	if (silent == 0) {
		(void) printf("\nFormatting...");
		(void) fflush(stdout);
	}

	/*
	 * Issue "interrupted" message, when signals are caught
	 */
	(void) signal(SIGHUP, interrupt);
	(void) signal(SIGINT, interrupt);
	(void) signal(SIGQUIT, interrupt);

	if ((handle = open(device, O_RDWR)) == -1) {
	   (void) fprintf(stderr, "\n%s: open for %s failed - ", cmd,
                          device);
	    perror("	Reason");
	   if (errno == ENXIO ) {
	      (void) fprintf(stderr, "   An example of device to be used for an unformatted disk is: /dev/rdsk/f13ht \n");
	      (void) fprintf(stderr, "   for a 3.25\" high density disk with boot block, in drive 1.\n");
	   }
	   exit(1);
	}

	/*
	 * Get parameters for this device
	 */
	if (ioctl(handle, V_GETPARMS, &parms_struct) == -1) {
		(void) fprintf(stderr, "%s: Error - V_GETPARMS failed\n", cmd);
		perror("	Reason");
		exit(1);
	}

#ifdef DEBUG
	(void) fprintf(stderr, "dp_type: %c\n", parms_struct.dp_type);
	(void) fprintf(stderr, "dp_heads: %u\n", parms_struct.dp_heads);
	(void) fprintf(stderr, "dp_cyls: %u\n", parms_struct.dp_cyls);
	(void) fprintf(stderr, "dp_sectors: %u\n", parms_struct.dp_sectors);
	(void) fprintf(stderr, "dp_secsiz: %u\n", parms_struct.dp_secsiz);
	(void) fprintf(stderr, "dp_pnumsec: %u\n", parms_struct.dp_pnumsec);
	(void) fprintf(stderr, "dp_pstartsec: %u\n", parms_struct.dp_pstartsec);
#endif

	/*
	 * Locate this device type in our table of 
	 * known formats
	 */
	dindex = -1;
	for (i = 0; drive_table[i].total_sectors; i++) {
		if (drive_table[i].total_sectors == parms_struct.dp_pnumsec){
			dindex = i;
			break;
		} else if (drive_table[i].media_descriptor == 0xf8) {
			hard_drive = 1;
			dindex = i;
			break;
		}
	}
	


	/*
	 * Make sure we found this type
	 */
	if (dindex == -1) {
		(void) fprintf(stderr, "Invalid media type\n");
		exit(1);
	}

	/* 
	 * Determine whether this is a DOS 3.3 or DOS 5.0 partition
	 */
	if (hard_drive && !dos_version) { 
		checktype();
	}
	dos5 = (dos_version == 5);

	/* 
	 * Used for calculating number of sectors per FAT.
 	 * Currently we support 2 FAT -- entry size 16 and 12.
	 * For simplicity we use size 16 (2 bytes).  
	 */ 
	nentry = parms_struct.dp_secsiz/2;
	/* 
	 * Now set hard drive parameters
	 */
	if (hard_drive) {
		drive_table[dindex].total_sectors = parms_struct.dp_pnumsec;
		/* Total_sectors in the boot sector is 2 bytes. */
		if (drive_table[dindex].total_sectors > 65535 && !dos5)
			drive_table[dindex].total_sectors = 65535;
		drive_table[dindex].sectors_per_fat = 
			(drive_table[dindex].total_sectors+nentry-1)/nentry;
		drive_table[dindex].root_dir_ent = ROOT_DIR_ENTRY;
	}

	/*
	 * Format the volume, track by track
	 */
	for (i = 0; i < parms_struct.dp_heads * 
		(unsigned int)parms_struct.dp_cyls; i++) {
		io_arg_union.ia_fmt.start_trk = i;
		io_arg_union.ia_fmt.num_trks = 1;
		io_arg_union.ia_fmt.intlv = 2;

		(void) ioctl(handle, V_FORMAT, &io_arg_union);
	}

	/*
	 * Load sector zero
	 * Sector zero known as the boot sector, contains critical
	 * information such as OEM identification, BIOS, and Bootstrap
	 * routine.
	 */
	if (lseek(handle, 0, 0) == -1) {
		(void) fprintf(stderr, "%s: Failed to seek to sector zero\n",
							cmd);
		exit(1);
	}

	/*
	 * Load up boot code.
	 */
	if (sizeof(Bootcode) > MAX_SECTOR_SIZE) {
		(void) fprintf(stderr, "%s: boot sector is bigger than %d\n", 
				cmd, (int)MAX_SECTOR_SIZE);
	}
	for (i = 0; i < sizeof(Bootcode); i++) {
		zero[i] = Bootcode[i];
	}

	/* 
	 * Load up the BIOS information.
	 */
	if (dos5) {
		if (drive_table[dindex].total_sectors <= 65535) {
			putushort(&zero[TOTAL_SECTORS], drive_table[dindex].total_sectors);
		} else {
			putushort(&zero[TOTAL_SECTORS], 0);
			putulong(&zero[HUGE_SECTORS], drive_table[dindex].total_sectors);
		}
		putubyte(&zero[XTD_BOOT_SIG], EXBOOTSIG );
	} else {
		putushort(&zero[TOTAL_SECTORS], drive_table[dindex].total_sectors);
	}

	putushort(&zero[SECTOR_SIZE], parms_struct.dp_secsiz);
	putubyte(&zero[SECTORS_PER_CLUSTER], drive_table[dindex].sectors_per_cluster);
	putushort(&zero[RESERVED_SECTORS], 0x01);
	putubyte(&zero[NUMBER_OF_FATS], 0x02);
	putushort(&zero[ROOT_DIR_ENT], drive_table[dindex].root_dir_ent);
	putubyte(&zero[MEDIA_DESCRIPTOR], drive_table[dindex].media_descriptor);
	putushort(&zero[SECTORS_PER_FAT], drive_table[dindex].sectors_per_fat);
	putushort(&zero[SECTORS_PER_TRACK], parms_struct.dp_sectors);
	putushort(&zero[NUMBER_OF_HEADS], parms_struct.dp_heads);


	/*
	 * Write out sector zero (boot sector).
	 */
	if (write(handle, zero, (unsigned) parms_struct.dp_secsiz) != parms_struct.dp_secsiz) {
		(void) fprintf(stderr, "%s: Error - Failed to write patched boot sector\n", cmd);
		perror("	Reason");
	}


	/*
	 * Now we must create the root directory and FATs
	 * add_device read the boot sector and initialize 
	 * table_struct.
	 */
	if (get_bios_info(handle) == -1) 
		exit(1);


	/*
	 * load our FAT into memory
         */
#ifdef DEBUG
        (void) fprintf(stderr, "main(): DEBUG - Attempting to malloc %ld bytes (%ld * %ld)\n", odev.bytes_per_sector * odev.sectors_per_fat, odev.bytes_per_sector, odev.sectors_per_fat);
#endif

        /*
	 * malloc our working copy of the FAT
         */
	size = odev.bytes_per_sector * odev.sectors_per_fat;
	/* 
	 * Zero indicates that this cluster is available.
	 * Make each cluster available.
	 */
        if ((odev.our_fat = calloc( 1, (size_t)size)) == NULL) { 
		(void) fprintf(stderr, "open_device(): Error - Failed to malloc FAT space\n");
                exit(1);
        }

#ifdef DEBUG
 	(void) fprintf(stderr, "open_device(): DEBUG - malloc of %ld bytes is successful\n", odev.bytes_per_sector * odev.sectors_per_fat); 
#endif


	/*
	 * The first two elements, which refer to cluster 0 or 1 holds:
	 * 	First byte contains media_description and remaining 
	 *	16 or 24 bits (depends of fat entry size)are set to 1.
	 */
	
	*(odev.our_fat) = drive_table[dindex].media_descriptor;
	*(odev.our_fat + 1) = (char) 0xFF;
	*(odev.our_fat + 2) = (char) 0xFF;
	if (odev.fat16)
		*(odev.our_fat + 3) = (char) 0xFF;
	/*
	 * Write out updated FATs to disk
	 */
	offset =  odev.bytes_per_sector * odev.reserved_sectors;
	size = odev.bytes_per_sector * odev.sectors_per_fat;
	for (i=0; i < odev.number_of_fats; i++) {
		if (write_bsect(handle, odev.our_fat, offset, size) == -1) {
			(void) fprintf(stderr, "%s: Error - Failed to write out FATs\n\tDisk may be unusable\n", cmd);
			exit(1);
		}
		offset =  odev.bytes_per_sector * (odev.reserved_sectors + odev.sectors_per_fat);
	}

	/*
	 * Set up the ROOT directory
	 */
	size = odev.bytes_per_sector;
	for (i = 0; i < size; i++)
		sector_buffer[i] = '\0';
	for (i = odev.root_base_sector; i < odev.root_base_sector + odev.sectors_in_root; i++) {

		/*
	 	 * Calculate the actual displacement into the media
	 	 */
		offset = odev.bytes_per_sector * i;
		if (write_bsect(handle, (char *)sector_buffer, offset, size) == -1) {
			(void) fprintf(stderr, "%s: Error - write of root dir failed\n\tDisk may be unusable\n", cmd);
			exit(1);
		}
	}

	if (silent == 0) {
		(void) printf("Format complete\n");
		(void) fflush(stdout);
	}

	/*
	 * If user wants to label the volume, do it now
	 */
	if (label_volume) {
		(void) printf("\nVolume label (11 characters, <RETURN> for none)? ");
		if (fgets(buffer, 20, stdin) != NULL) 
			(void) make_label(handle, buffer);
	}


	if (silent == 0) {
		long	bad;
		long 	fspace;

		(void) printf("\n%10ld bytes total disk space\n", odev.bytes_per_sector * (odev.total_sectors - (odev.root_base_sector + odev.sectors_in_root)));

		fspace = free_space();
		bad = ((odev.bytes_per_sector * (odev.total_sectors - (odev.root_base_sector + odev.sectors_in_root))) - fspace);

		if (bad)
			(void) printf("%10ld bytes in bad sectors\n", bad);

		(void) printf("%10ld bytes available on disk\n", fspace);
		(void) fflush(stdout);
	}


	(void) close(handle);
	free(odev.our_fat);
	exit(0);	/* NOTREACHED */
}
