#ident	"@(#)pdi.cmds:addbadsec.c	1.5"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */


#include <stdio.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/vtoc.h>
#include <sys/stat.h>
#include <sys/alttbl.h>
#include <sys/mkdev.h>
#include "badsec.h"

char    *devname;		/* name of device */
int	devfd;			/* device file descriptor */
struct  disk_parms   dp;        /* device parameters */
struct  pdinfo	pdinfo;		/* physical device info area */
struct  vtoc	vtoc;		/* table of contents */
char    *buf;			/* buffer used to read in disk structs. */
char    errstring[] = "ADDBADSEC error";

extern	struct	badsec_lst *badsl_chain;
extern	int	badsl_chain_cnt;
extern	struct	badsec_lst *gbadsl_chain;
extern	int	gbadsl_chain_cnt;

extern int	*alts_fd;

void
main(argc, argv)
int	argc;
char	*argv[];
{
	static char     options[] = "a:f:";
	extern int	optind;
	extern char	*optarg;
	char		*nxtarg;
	minor_t 	minor_val;
	struct stat 	statbuf;
	int 		c, i;
	FILE		*badsecfd = NULL;
	struct badsec_lst *blc_p;

	while ( (c=getopt(argc, argv, options)) != EOF ) {
		switch (c) {
		case 'a':
			nxtarg = optarg;
			for (;*nxtarg != '\0';)
				add_gbad(strtol(nxtarg, &nxtarg, 0));
			break;
		case 'f':
			if ((badsecfd = fopen(optarg, "r")) == NULL) {
				fprintf(stderr,"addbadsec: unable to open %s file\n",optarg);
				exit(1);
			}
			break;
		default:
			fprintf(stderr,"Invalid option '%s'\n",argv[optind]);
			giveusage();
			exit(1);
		}
	}

		/* get the last argument -- device stanza */
	if (argc != optind+1) {
		fprintf(stderr,"Missing disk device name\n");
		giveusage();
		exit(1);
	}
	devname = argv[optind];

	if (stat(devname, &statbuf)) {
		fprintf(stderr,"addbadsec invalid device %s, stat failed\n",devname);
		giveusage();
		exit(1);
	}
	if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
		fprintf(stderr,"addbadsec: device %s is not character special\n",devname);
		giveusage();
		exit(1);
	}
	minor_val = minor(statbuf.st_rdev);
	if ((minor_val % V_NUMPAR) != 0) {
		fprintf(stderr,"addbadsec: device %s is not a slice 0 device\n",devname);
		giveusage();
		exit(1);
	}
	if ((devfd=open(devname, O_RDONLY)) == -1) {
		fprintf(stderr,"addbadsec: open of %s failed\n", devname);
		perror(errstring);
		exit(2);
	}
	alts_fd = &devfd;

	if (ioctl(devfd, V_GETPARMS, &dp) == -1) {
		fprintf(stderr,"addbadsec: GETPARMS on %s failed\n", devname);
		perror(errstring);
		exit(2);
	}

	if ((buf=(char *)malloc(dp.dp_secsiz)) == NULL) {
		fprintf(stderr,"addbadsec: malloc of buffer failed\n");
		perror(errstring);
		exit(3);
	}

	if ( (lseek(devfd, dp.dp_secsiz*VTOC_SEC, 0) == -1) ||
    	   (read(devfd, buf, dp.dp_secsiz) == -1)) {
		fprintf(stderr,"addbadsec: unable to read pdinfo structure.\n");
		perror(errstring);
		exit(4);
	}
	memcpy((char *)&pdinfo, buf, sizeof(pdinfo));
	if ((pdinfo.sanity != VALID_PD) || (pdinfo.version != V_VERSION)) {
		fprintf(stderr,"addbadsec: invalid pdinfo block found.\n");
		giveusage();
		exit(5);
	}
	memcpy((char *)&vtoc, &buf[pdinfo.vtoc_ptr%dp.dp_secsiz], sizeof(vtoc));
	if ((vtoc.v_sanity != VTOC_SANE) || (vtoc.v_version != V_VERSION)) {
		fprintf(stderr,"addbadsec: invalid VTOC found.\n");
		giveusage();
		exit(6);
	}

	if (badsecfd)
		rd_gbad(badsecfd);

#ifdef ADDBAD_DEBUG
	printf("\n main: Total bad sectors found= %d\n", gbadsl_chain_cnt);
	for (blc_p=gbadsl_chain; blc_p; blc_p=blc_p->bl_nxt) {
		for (i=0; i<blc_p->bl_cnt; i++)
			printf(" badsec=%d ", blc_p->bl_sec[i]);
	}
	printf("\n");
#endif

	if (updatebadsec())
		wr_altsctr();
	else {
		updatealts();
		wr_alts();
	}

	if (ioctl(devfd, V_ADDBAD, NULL) == -1) {
		fprintf(stderr, "Warning: V_ADDBAD io control failed. System must be re-booted\n");
		fprintf(stderr,"for alternate sectors to be usable.\n");
		exit(90);
	}
	sync();

	fclose(badsecfd);
	close(devfd);
	exit(0);
}

/*
 * Giveusage ()
 * Give a (not so) concise message on how to use mkpart.
 */
giveusage()
{
	fprintf(stderr,"addbadsec [-a sector] [-f filename] raw-device\n");
	if (devfd)
		close(devfd);
}


/*
 *	read in the additional growing bad sectors 
 */
rd_gbad(badsecfd)
FILE	*badsecfd;
{
	int	badsec_entry;
	int	status;

	status = fscanf(badsecfd, "%d", &badsec_entry);
	while (status!=EOF) {
		add_gbad(badsec_entry);
		status = fscanf(badsecfd, "%d", &badsec_entry);
	}
}

add_gbad(badsec_entry)
int	badsec_entry;
{
	struct badsec_lst *blc_p;

	if (!gbadsl_chain) {
		blc_p = (struct badsec_lst *)malloc(BADSLSZ);
		if (!blc_p) {
			fprintf(stderr,"Unable to allocate memory for additional bad sectors\n");
			exit(20);
		}
		gbadsl_chain = blc_p;
		blc_p->bl_cnt = 0;
		blc_p->bl_nxt = 0;
	}
	for (blc_p = gbadsl_chain; blc_p->bl_nxt; )
		blc_p = blc_p->bl_nxt;
				
	if (blc_p->bl_cnt == MAXBLENT) {
		blc_p->bl_nxt = (struct badsec_lst *)malloc(BADSLSZ);
		if (!blc_p->bl_nxt) {
			fprintf(stderr,"Unable to allocate memory for additional bad sectors\n");
			exit(20);
		}
		blc_p = blc_p->bl_nxt;
		blc_p->bl_cnt = 0;
		blc_p->bl_nxt = 0;


	}
	blc_p->bl_sec[blc_p->bl_cnt++] = badsec_entry;
	gbadsl_chain_cnt++;

}
