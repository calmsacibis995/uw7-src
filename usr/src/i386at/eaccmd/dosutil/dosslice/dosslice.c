#ident	"@(#)eac:i386at/eaccmd/dosutil/dosslice/dosslice.c	1.1"

/*
	This program checks for DOS partitions on a hard disk
	and makes sure there is a slice set up for each.  It also
	makes sure there is a node file with the right permissions.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vtoc.h>
#include <sys/fdisk.h>
#include <fcntl.h>
#include <pwd.h>

				/* DOS slice assignments as per disksetup */
#define DOSSLICE	5	/* primary dos slice (first encounter) */
#define ALTDOSSL_1	14	/* secondary dos slice (second encounter) */
#define ALTDOSSL_2	15	/* secondary dos slice (third encounter) */
#define TRUE		1
#define FALSE		0

struct mboot	mboot;		/* master boot block */
struct ipart	*fdp;		/* fdisk partition table pointer */

struct  disk_parms   dp;        /* device parameters */
struct  pdinfo	pdinfo;		/* physical device info area */
struct  vtoc	vtoc;		/* table of contents */
int		vtoc_changed = 0;
int		disk_fd;
int		disk_no = -1;
char		disk_name[40];
int		slice;
char		slice_name[40];
struct absio	absio;
struct stat	statb;
dev_t		dev;
char		answer[80];
struct passwd	*pw;
int		mode;
char		*cmdname;

#define VALID_DOSSLICE( X ) (vtoc.v_part[(X)].p_tag == V_OTHER && \
			     vtoc.v_part[(X)].p_flag == (V_UNMNT | V_VALID))

void
main(argc, argv)
	int	argc;
	char	*argv[];
{
	static	char	options[] = "s";
	extern	int	optind;
	extern	char	*optarg;
	int	c, i;
	int	doscnt = 0;
	int	newdoscnt = 0;
	int	silent = 0;
	char	*buf;

	cmdname = argv[0];
	while ( (c=getopt(argc, argv, options)) != EOF ) {
		switch (c) {
		case 's':
			silent = 1;
			break;
		case '?':
		default:
			usage();
			exit(1);
		}
	}

	if(argc > optind+1) {
		usage();
		exit(1);
	}
	if (argc == optind+1) {
		if (strcmp(argv[optind], "0") == 0)
			disk_no = 0;
		else if (strcmp(argv[optind], "1") == 0)
			disk_no = 1;
		else {
			usage();
			exit(1);
		}
	} else {
		if(silent) {
			usage();
			exit(1);
		}
	}

	while (disk_no < 0) {
		printf("For which disk drive would you like to set up a DOS slice?\n");
		printf("(0 or 1)? ");
		gets(answer);
		if (answer[1] == '\0' && (answer[0] == '0' || answer[0] == '1')) {
			disk_no = answer[0] - '0';
			printf("\n");
			break;
		}
		printf("\n\007Please enter 0 or 1\n\n");
	}
	sprintf(disk_name, "/dev/rdsk/%ds0", disk_no);

	if ((disk_fd = open(disk_name, O_RDWR)) == -1) {
		fprintf(stderr,"%s: open of %s failed\n", cmdname, disk_name);
		perror(disk_name);
		exit(1);
	}

	if (fstat(disk_fd, &statb) == -1) {
		fprintf(stderr,"%s: stat of %s failed\n", cmdname, disk_name);
		perror(disk_name);
		exit(1);
	}

	dev = statb.st_rdev;

	absio.abs_sec = 0;
	absio.abs_buf = (char *)&mboot;
	if (ioctl(disk_fd, V_RDABS, &absio) == -1) {
		fprintf(stderr, "%s: Can't get partition information from disk %d\n",
				cmdname, disk_no);
		exit(1);
	}

	if (ioctl(disk_fd, V_GETPARMS, &dp) == -1) {
		fprintf(stderr,"%s: GETPARMS on %s failed\n", cmdname, disk_name);
		perror(disk_name);
		exit(1);
	}

	if ((buf=(char *)malloc(dp.dp_secsiz)) == NULL) {
		fprintf(stderr,"%s: malloc of buffer failed\n", cmdname);
		perror(disk_name);
		exit(3);
	}

	if ( (lseek(disk_fd, dp.dp_secsiz*VTOC_SEC, 0) == -1) ||
    	   (read(disk_fd, buf, dp.dp_secsiz) == -1)) {
		fprintf(stderr,"%s: unable to read pdinfo structure on %s\n", cmdname, disk_name);
		perror(disk_name);
		exit(4);
	}
	memcpy((char *)&pdinfo, buf, sizeof(pdinfo));
	/* check for valid pdinfo struct - valid version id's are
	 * V_VERSION_1, V_VERSION_2, V_VERSION_3, and V_VERSION.
	 */
	if ((pdinfo.sanity != VALID_PD) || (pdinfo.version < V_VERSION_1 && 
		pdinfo.version > V_VERSION)) {
		fprintf(stderr,"%s: invalid pdinfo block found on %s\n", cmdname, disk_name);
		usage();
		exit(5);
	}
	memcpy((char *)&vtoc, &buf[pdinfo.vtoc_ptr%dp.dp_secsiz], sizeof(vtoc));

	if ((vtoc.v_sanity != VTOC_SANE) || (vtoc.v_version < V_VERSION_1 && 
		vtoc.v_version > V_VERSION)) {
		fprintf(stderr,"%s: invalid VTOC found on %s\n", cmdname, disk_name);
		usage();
		exit(6);
	}

	/* Count the DOS partitions */
	fdp = (struct ipart *)mboot.parts;
	for (doscnt = 0, i = 0; i < FD_NUMPART; i++, fdp++) {
		if (fdp->systid == DOSOS12 ||
		    fdp->systid == DOSOS16 ||
		    fdp->systid == DOSHUGE ||
		    fdp->systid == EXTDOS)
			doscnt++;
	}
	if (doscnt == 0) {
		fprintf(stderr, "There is no DOS partition on disk %d.\n", disk_no);
		fprintf(stderr, "You must first run \"fdisk %s\"", disk_name);
		fprintf(stderr, " to create a DOS partition.\n");
		exit(1);
	}

	if((vtoc_changed = assign_dos()) == -1) {
		exit(5);
	}

	if(!silent)
		fprintf(stderr, "DOS partitions found on disk %d: %d\n", disk_no, doscnt);
		/* put new vtoc into the same sector */
	if (vtoc_changed) {

		*((struct vtoc *)&buf[pdinfo.vtoc_ptr%dp.dp_secsiz]) = vtoc;
		if ( (lseek(disk_fd, dp.dp_secsiz*VTOC_SEC, 0) == -1) ||
    	   	(write(disk_fd, buf, dp.dp_secsiz) == -1)) {
			fprintf(stderr,"%s: unable to write pdinfo structure on %s\n", cmdname, disk_name);
			perror(disk_name);
			exit(4);
		}
	}

	/* Create the nodes if they don't already exist */
	mk_dosnode( DOSSLICE, silent );
	mk_dosnode( ALTDOSSL_1, silent );
	mk_dosnode( ALTDOSSL_2, silent );

	/* Make sure vtoc change takes effect */
	if (vtoc_changed && ioctl(disk_fd, V_REMOUNT, NULL) == -1) {
		if(!silent)
		printf(
"You must reboot the system before you can use the DOS partition.\n");
	}

	exit(0);
}

usage() {
	fprintf(stderr, "Usage:  %s  [-s] [0|1]\n\t-s silent\n", cmdname);
}

/* this routines assigns the DOS partitions into the UNIX vtoc for use
 * by VPIX. The first DOS partition is assigned to slot 5 with secondary
 * partitions assigned to slots 14 (ALTDOSSL_1) and 15 (ALTDOSSl_2) if
 * they aren't already assigned to a UNIX slice.
 */

assign_dos(){

int i;
int first = TRUE;
int changed = 0;

/* see if a DOS partition was allocated */
	fdp = (struct ipart *)mboot.parts;
	for (i = 0; i < FD_NUMPART; i++, ++fdp) {
		if (fdp->systid == DOSOS12 ||
		    fdp->systid == DOSOS16 ||
		    fdp->systid == DOSHUGE ||
		    fdp->systid == EXTDOS)
			if (first == TRUE &&
				(!vtoc.v_part[DOSSLICE].p_tag ||
				  vtoc.v_part[DOSSLICE].p_tag == V_OTHER)) {
				if (partcmp(&vtoc.v_part[DOSSLICE], V_OTHER,
					V_UNMNT|V_VALID, fdp->relsect,
					fdp->numsect) == 0) {
					vtoc.v_part[DOSSLICE].p_tag = V_OTHER;
					vtoc.v_part[DOSSLICE].p_flag = V_UNMNT | V_VALID;
					vtoc.v_part[DOSSLICE].p_start = fdp->relsect;
					vtoc.v_part[DOSSLICE].p_size = fdp->numsect;
					changed++;
				}
				first = FALSE;

			/* vtoc slots 14 and 15 are secondary DOS
			 * partions although UNIX has precedence
			 * over these slots
			 */
			} else 
			if ((!vtoc.v_part[ALTDOSSL_1].p_tag) ||
			     (vtoc.v_part[ALTDOSSL_1].p_tag == V_OTHER)) {
				if (partcmp(&vtoc.v_part[ALTDOSSL_1], V_OTHER,
				    V_UNMNT|V_VALID, fdp->relsect,
				    fdp->numsect) == 0) {
					vtoc.v_part[ALTDOSSL_1].p_tag = V_OTHER;
					vtoc.v_part[ALTDOSSL_1].p_flag = V_UNMNT | V_VALID;
					vtoc.v_part[ALTDOSSL_1].p_start = fdp->relsect;
					vtoc.v_part[ALTDOSSL_1].p_size = fdp->numsect;
					changed++;
				}
			} else 
			if ((!vtoc.v_part[ALTDOSSL_2].p_tag) ||
			     (vtoc.v_part[ALTDOSSL_2].p_tag == V_OTHER)) {
				if (partcmp(&vtoc.v_part[ALTDOSSL_2], V_OTHER,
				    V_UNMNT|V_VALID, fdp->relsect,
				    fdp->numsect) == 0) {
					vtoc.v_part[ALTDOSSL_2].p_tag = V_OTHER;
					vtoc.v_part[ALTDOSSL_2].p_flag = V_UNMNT | V_VALID;
					vtoc.v_part[ALTDOSSL_2].p_start = fdp->relsect;
					vtoc.v_part[ALTDOSSL_2].p_size = fdp->numsect;

					changed++;
				}
			} else {
				fprintf(stderr,"\n%s: No slots available in vtoc for DOS partition \"%d\" on drive %s\n", cmdname, i+1, disk_name);
				return(-1);
			}

	}
	return(changed);
}

mk_dosnode( slice, silent )
int slice;
int silent;
{
	if(!VALID_DOSSLICE( slice )) {
		return;
	}

	sprintf(slice_name, "/dev/rdsk/%ds%d", disk_no, slice);

	if (access(slice_name, 0) == -1) {
		if (mknod(slice_name, S_IFCHR|0666, dev + slice) == -1) {
			perror(slice_name);
			exit(1);
		}
	}

	/* Make sure the permissions are right */
	if( !silent) {
		printf("Slice: %s\n", slice_name);
		printf("Do you want this DOS partition to be public (writeable by everyone)? ");
		gets(answer);
	} else {
		answer[0] = 'Y';
	}

	if (toupper(answer[0]) == 'Y') {
		if ((pw = getpwnam("root")) == NULL) {
			fprintf(stderr, "Can't find password file entry for root\n");
			exit(1);
		}
		mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
	} else {
		printf(
"The DOS partition will be made private (readable/writeable by one user).\n");
		for (;;) {
			printf("Which user will be allowed access? ");
			gets(answer);
			if ((pw = getpwnam(answer)) != NULL)
				break;
			printf("\nNo such user: %s\n\n", answer);
			setpwent();
		}
		mode = S_IRUSR|S_IWUSR;
	}
	if (chown(slice_name, pw->pw_uid, pw->pw_gid) == -1
	 || chmod(slice_name, mode) == -1) {
		perror(slice_name);
		exit(1);
	}
	endpwent();
}

partcmp(part, tag, flag, start, size)
struct partition *part;
{
	if((part->p_tag == tag) &&
	   (part->p_flag == flag) &&
	   (part->p_start == start) &&
	   (part->p_size == size))
		return(1);

	return(0);
}
