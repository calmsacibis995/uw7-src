#ident	"@(#)pdi.cmds:prtvtoc.c	1.3.12.1"

/* The prtvtoc command has three functions, the primary function is to   */
/* display the contents of the pdinfo, the vtoc and the alternates table */
/* as a source of system information. The second purpose for the command */
/* is to output the contents of the vtoc to in a format for later use    */
/* the edvtoc command. The third purpose is output the contents of the   */
/* contents of the pdinfo, vtoc and alternates table into the            */
/* /etc/partitions file for use with the mkpart command. The format of   */
/* be the same format as used in the 3.2 releases			 */

#include <stdio.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/vtoc.h>
#include <sys/fdisk.h>
#include <sys/stat.h>
#include <sys/alttbl.h>
#include <sys/mkdev.h>

#include <errno.h>
#include <locale.h>
#include <pfmt.h>
#include "readxedt.h"
#include "info.h"

char    *devname;		/* Name of device */
int	devfd;			/* Device file descriptor */
FILE	*vtocfile;
struct  disk_parms dp;		/* Device parameters */
struct  pdinfo pdinfo;		/* Physical device info area */
struct  vtoc_ioctl vtoc;	/* Table of contents */
struct  alt_info alttbl;	/* Alternate sector and track tables */
char    *buf;			/* Buffer used to read in disk structs. */
struct  absio absio;

extern int	*alts_fd;
int Debug=0;

void
main(argc, argv)
int	argc;
char	*argv[];
{
	static char     options[] = "aef:p";
	extern int	optind;
	extern char	*optarg;
	int	create_partsfile = 0;	/* flag to create /etc/partition */
	int	create_vtocfile = 0;	/* flag to create file for edvtoc */
	int 	prt_alts = 0;		/* flag to print alternates info */
	int 	prt_pdinfo = 0;		/* flag to print pdinfo */
	struct stat statbuf;
	int c, i;
	int	nsect;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxprtvtoc");
	(void) setlabel("UX:prtvtoc");
	
	while ( (c=getopt(argc, argv, options)) != EOF ) {
		switch (c) {
		case 'a':
			prt_alts++;
			break;
		case 'e':
			create_partsfile++;
			break;
		case 'f':
			if ((vtocfile = fopen(optarg, "w")) == NULL) {
				(void) pfmt(stderr, MM_ERROR,
					":1:unable to open/create %s file\n",optarg);
				exit(1);
			}
			create_vtocfile++;
			break;
		case 'p':
			prt_pdinfo++;
			break;
		default:
			(void) pfmt(stderr, MM_ERROR,
				":2:Invalid option '%s'\n",argv[optind]);
			giveusage();
			exit(1);
		}
	}

		/* get the last argument -- device stanza */
	if (argc != optind+1) {
		(void) pfmt(stderr, MM_ERROR,
			":3:Missing disk device name\n");
		giveusage();
		exit(1);
	}
	devname = argv[optind];
	if (stat(devname, &statbuf)) {
		(void) pfmt(stderr, MM_ERROR,
			":4:invalid device %s, stat failed\n",devname);
		giveusage();
		exit(1);
	}
	if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
		(void) pfmt(stderr, MM_ERROR,
			":5:device %s is not character special\n",devname);
		giveusage();
		exit(1);
	}

	if ((devfd=open(devname, O_RDONLY)) == -1) {
		(void) pfmt(stderr, MM_ERROR,
			":7:open of %s failed\n", devname);
		(void) pfmt(stderr, MM_ERROR|MM_NOGET,
			"%s\n", strerror(errno));
		exit(2);
	}
	alts_fd = &devfd;

	if (ioctl(devfd, V_GETPARMS, &dp) == -1) {
		(void) pfmt(stderr, MM_ERROR,
			":8:GETPARMS on %s failed\n", devname);
		(void) pfmt(stderr, MM_ERROR|MM_NOGET,
			"%s\n", strerror(errno));
		exit(2);
	}
	nsect = (sizeof(alttbl)+ dp.dp_secsiz - 1)/dp.dp_secsiz;
	if ((buf=(char *)malloc(nsect*dp.dp_secsiz)) == NULL) {
		(void) pfmt(stderr, MM_ERROR,
			":9:malloc of buffer failed\n");
		(void) pfmt(stderr, MM_ERROR|MM_NOGET,
			"%s\n", strerror(errno));
		exit(3);
	}
	if (ioctl(devfd, V_READ_PDINFO, &pdinfo) == -1) {
		(void) pfmt(stderr, MM_ERROR,
			":10:reading pdinfo failed\n");
		(void) pfmt(stderr, MM_ERROR,
			":11:unable to read pdinfo structure.\n");
		(void) pfmt(stderr, MM_ERROR|MM_NOGET,
			"%s\n", strerror(errno));
 		exit(4);
	}
	if (ioctl(devfd, V_READ_VTOC, &vtoc) == -1) {
		(void)pfmt(stderr, MM_ERROR, ":62:reading vtoc failed: %s\n", strerror(errno));
		exit(6);
	}

	absio.abs_buf = buf;
	absio.abs_sec = dp.dp_pstartsec + (pdinfo.alt_ptr/dp.dp_secsiz);
	while(nsect > 0) {
		if (ioctl(devfd, V_RDABS, &absio) == -1) {
			(void) pfmt(stderr, MM_ERROR,
				":14:reading alternates table failed\n");
			giveusage();
			exit(7);
		}
		nsect--;
		absio.abs_buf += dp.dp_secsiz;
		absio.abs_sec += 1;
	}
	absio.abs_buf = buf;
	memcpy((char *)&alttbl, buf, sizeof(alttbl));

	/* Either write vtoc file or write vtoc info to screen */
	if (!create_vtocfile && !prt_pdinfo && !prt_alts && !create_partsfile)
		for (i=0; i < vtoc.v_nslices; i++) 
			if (vtoc.v_slices[i].p_flag & V_VALID) {
				(void) pfmt(stdout, MM_NOSTD,
					":15:slice %d:\t",i);
				printslice(&vtoc.v_slices[i]);
			}
	
	if (create_vtocfile) {
		fprintf(vtocfile,"#SLICE	TAG 	FLAGS	START	SIZE\n");
		for (i=0; i < vtoc.v_nslices; i++) 
			writeslice(i);
		fclose(vtocfile);
	}

	if (prt_pdinfo) {
		(void) pfmt(stdout, MM_NOSTD,
			":16:\tDevice %s\n",devname);
		(void) pfmt(stdout, MM_NOSTD,
			":17:device type:\t\t%d (",dp.dp_type);
		if (dp.dp_type == DPT_WINI)
			(void) pfmt(stdout, MM_NOSTD,
				":18:DPT_WINI");
		else
			if (dp.dp_type == DPT_SCSI_HD)
				(void) pfmt(stdout, MM_NOSTD,
					":19:DPT_SCSI_HD");
			else
				(void) pfmt(stdout, MM_NOSTD,
					":20:DPT_SCSI_OD");
		(void) pfmt(stdout, MM_NOSTD,
			":21:)\n");
		(void) pfmt(stdout, MM_NOSTD,
			":22:cylinders:\t\t%ld\t\theads:\t\t%ld\n",pdinfo.cyls,pdinfo.tracks);
		(void) pfmt(stdout, MM_NOSTD,
			":23:sectors/track:\t\t%ld\t\tbytes/sector:\t%ld\n",pdinfo.sectors,pdinfo.bytes);
		(void) pfmt(stdout, MM_NOSTD,
			":24:number of partitions:\t%d",vtoc.v_nslices);
		(void) pfmt(stdout, MM_NOSTD,
			":25:\t\tsize of alts table:\t%d\n", pdinfo.alt_len);
		(void) pfmt(stdout, MM_NOSTD, ":63:media stamp:\t\t\"%.12s\"\n",
			pdinfo.serial);
	}

	if (prt_alts) {
		/* messages for print_altsec() are in disksetup.str */
		setcat("uxdisksetup");
		if (!print_altsec()) {
			setcat("uxprtvtoc");
			if (alttbl.alt_sanity == ALT_SANITY &&
					alttbl.alt_version == ALT_VERSION) {
				printalts(&alttbl.alt_sec, 1);
				printalts(&alttbl.alt_trk, 0);
			}
			else
				(void) pfmt(stderr, MM_ERROR,
				":14:reading alternates table failed\n");
		} else
			setcat("uxprtvtoc");

	}

	if (create_partsfile) {
		write_partsfile();
	}
	close(devfd);
	exit(0);
}

writeslice(slice)
int slice;
{
	fprintf(vtocfile,"%2d	0x%x	0x%x	%ld	%ld\n",slice,
		vtoc.v_slices[slice].p_tag, vtoc.v_slices[slice].p_flag, 
		vtoc.v_slices[slice].p_start,vtoc.v_slices[slice].p_size);
}

printalts(altptr, sectors)
struct alt_table	*altptr;
int			sectors;
{
	int i, j;


	if (sectors)
		(void) pfmt(stdout, MM_NOSTD,
			":26:\nALTERNATE SECTOR TABLE: %d alternates available, %d used\n",
			altptr->alt_reserved, altptr->alt_used);
	else
		(void) pfmt(stdout, MM_NOSTD,
			":27:\nALTERNATE TRACK TABLE: %d alternates available, %d used\n",
			altptr->alt_reserved, altptr->alt_used);

	if (altptr->alt_used > 0) {
		if (sectors)
			(void) pfmt(stdout, MM_NOSTD,
				":28:\nAlternates are assigned for the following bad sectors:\n");
		else
			(void) pfmt(stdout, MM_NOSTD,
				":29:\nAlternates are assigned for the following bad tracks:\n");
		for (i = j = 0; i < (int)altptr->alt_used; ++i) {
			if (altptr->alt_bad[i] == -1)
				continue;
			(void) pfmt(stdout, MM_NOSTD,
				":30:\t%ld -> %ld", altptr->alt_bad[i],
				sectors ? altptr->alt_base + i
					: altptr->alt_base / (daddr_t)dp.dp_sectors + i);
			if ((++j % 3) == 0)
				(void) pfmt(stdout, MM_NOSTD|MM_NOGET,
					"\n");
		}
		(void) pfmt(stdout, MM_NOSTD|MM_NOGET,
			"\n");
	}
	if (altptr->alt_used != altptr->alt_reserved) {
		if (sectors)
			(void) pfmt(stdout, MM_NOSTD,
				":31:\nThe following sectors are available as alternates:\n");
		else
			(void) pfmt(stdout, MM_NOSTD,
				":32:\nThe following tracks are available as alternates:\n");
		for (i = altptr->alt_used, j = 0; i < (int)altptr->alt_reserved; ++i) {
			if (altptr->alt_bad[i] == -1)
				continue;
			(void) pfmt(stdout, MM_NOSTD,
				":33:\t\t%ld",
				sectors ? altptr->alt_base + i
					: altptr->alt_base / (daddr_t)dp.dp_sectors + i);
			if ((++j % 4) == 0)
				(void) pfmt(stdout, MM_NOSTD|MM_NOGET,
					"\n");
		}
		(void) pfmt(stdout, MM_NOSTD|MM_NOGET,
			"\n");
	}
}


/*  Printslice prints a single slice entry.  */
printslice(v_p)
struct partition *v_p;
{
	daddr_t cylstart, cylsecs;
	double cyllength;

	switch(v_p->p_tag) {
	case V_BOOT:	(void) pfmt(stdout, MM_NOSTD,
				":34:BOOT\t\t");
			break;
	case V_ROOT:	(void) pfmt(stdout, MM_NOSTD,
				":35:ROOT\t\t");
			break;
	case V_SWAP:	(void) pfmt(stdout, MM_NOSTD,
				":36:SWAP\t\t");
			break;
	case V_USR:	(void) pfmt(stdout, MM_NOSTD,
				":37:USER\t\t");
			break;
	case V_BACKUP:	(void) pfmt(stdout, MM_NOSTD,
				":38:DISK\t\t");
			break;
	case V_STAND:	(void) pfmt(stdout, MM_NOSTD,
				":39:STAND\t\t");
			break;
	case V_HOME:	(void) pfmt(stdout, MM_NOSTD,
				":40:HOME\t\t");
			break;
	case V_DUMP:	(void) pfmt(stdout, MM_NOSTD,
				":41:DUMP\t\t");
			break;
	case V_VAR:	(void) pfmt(stdout, MM_NOSTD,
				":42:VAR\t\t");
			break;
	case V_ALTS:	(void) pfmt(stdout, MM_NOSTD,
				":43:ALTERNATES\t");
			break;
	case V_ALTTRK:	(void) pfmt(stdout, MM_NOSTD,
				":44:ALT TRACKS\t");
			break;
	case V_ALTSCTR:	(void) pfmt(stdout, MM_NOSTD,
				":45:ALT SEC/TRK\t");
			break;
	case V_MANAGED_1:	(void) pfmt(stdout, MM_NOSTD,
					":46:VOLPUBLIC\t");
				break;
	case V_MANAGED_2:	(void) pfmt(stdout, MM_NOSTD,
					":47:VOLPRIVATE\t");
				break;
	case V_OTHER:	(void) pfmt(stdout, MM_NOSTD,
				":48:NONUNIX\t\t");
			break;
	default:	(void) pfmt(stdout, MM_NOSTD,
				":49:unknown 0x%x\t",v_p->p_tag);
			break;
	}

	(void) pfmt(stdout, MM_NOSTD,
		":50:permissions:\t");
	if (v_p->p_flag & V_VALID)
		(void) pfmt(stdout, MM_NOSTD,
			":51:VALID ");
	if (v_p->p_flag & V_UNMNT)
		(void) pfmt(stdout, MM_NOSTD,
			":52:UNMOUNTABLE ");
	if (v_p->p_flag & V_RONLY)
		(void) pfmt(stdout, MM_NOSTD,
			":53:READ ONLY ");
	if (v_p->p_flag & V_OPEN)
		(void) pfmt(stdout, MM_NOSTD,
			":54:(driver open) ");
	if (v_p->p_flag & ~(V_VALID|V_OPEN|V_RONLY|V_UNMNT))
		(void) pfmt(stdout, MM_NOSTD,
			":55:other stuff: 0x%x",v_p->p_flag);
	cylsecs = dp.dp_heads*dp.dp_sectors;
	cylstart = v_p->p_start / cylsecs;
	cyllength = (float)v_p->p_size / (float)cylsecs;
	(void) pfmt(stdout, MM_NOSTD,
		":56:\n\tstarting sector:\t%ld (cyl %ld)\t\tlength:\t%ld (%.2f cyls)\n",
		v_p->p_start, cylstart, v_p->p_size, cyllength);
}

/*
 * Giveusage ()
 * Give a (not so) concise message on how to use mkpart.
 */
giveusage()
{
	(void) pfmt(stderr, MM_ACTION,
		":57:Usage: prtvtoc [-a] [-e] [-p] [-f filename] raw-device\n");
	if (devfd)
		close(devfd);
}


write_partsfile()
{
#define PARTFILE   "/etc/partitions"	/* partitions file for mkpart */
	FILE	*partfile;		/* /etc/partition file */
	int 	partfd;			/* descriptor to creat/open partfile */
	char diskname[7];
	char ptag[10], pname[10];
	char *str1, *str2, *str3;
	int i, ctlnum, drivnum, targnum;

	if (((partfd=open(PARTFILE,O_CREAT|O_WRONLY|O_APPEND,0644)) == NULL) ||
	   ((partfile = fdopen(partfd,"ab")) == NULL)) {
		(void) pfmt(stderr, MM_ERROR,
			":58:opening /etc/partitions");
		(void) pfmt(stderr, MM_ERROR|MM_NOGET,
			"%s\n", strerror(errno));
		exit(9);
	}
	if (devname[10] == 'c')
		if (sscanf(devname,"/dev/rdsk/c%1dt%1dd%1d",&ctlnum,&targnum,&drivnum) != 3) {
			(void) pfmt(stderr, MM_ERROR,
				":59:can't parse dev to create /etc/partitions file\n");
			exit(10);
		}
		else
			sprintf(diskname,"disk%d%d%d",ctlnum,targnum,drivnum);
	else
		if (devname[10] == '0')
			sprintf(diskname,"disk0");
		else
			sprintf(diskname,"disk01");
			

	/* Now we write out the drive definition to the partition file. */
	fprintf(partfile,
	    "%s:\n    heads = %d, cyls = %d, sectors = %d, bpsec = %d,\n",
 	    diskname,(int)dp.dp_heads,dp.dp_cyls,dp.dp_sectors,dp.dp_secsiz);
	fprintf(partfile,
		"    vtocsec = %d, altsec = %ld, boot = \"/etc/boot\", device = \"%s\"",
		VTOC_SEC, pdinfo.alt_ptr/(daddr_t)dp.dp_secsiz, devname);

	/* Write out any bad sectors */
	if (alttbl.alt_sec.alt_used != 0) {
		fprintf(partfile,",\n    badsec = ( %ld",alttbl.alt_sec.alt_bad[0]);
		for (i=1; i < (int)alttbl.alt_sec.alt_used; i++)
			fprintf(partfile,",\n               %ld",alttbl.alt_sec.alt_bad[i]);
		fprintf(partfile,")");
	}

	/* Write out any bad tracks */
	if (alttbl.alt_trk.alt_used != 0) {
		fprintf(partfile,",\n    badtrk = ( %ld",alttbl.alt_trk.alt_bad[0]);
		for (i=1; i < (int)alttbl.alt_trk.alt_used; i++)
			fprintf(partfile,",\n               %ld",alttbl.alt_trk.alt_bad[i]);
		fprintf(partfile,")");
	}
	fprintf(partfile,"\n\n");

	for (i=1; i < vtoc.v_nslices; i++) {
		if (vtoc.v_slices[i].p_flag & V_VALID) {
			switch(vtoc.v_slices[i].p_tag) {
			   case V_BOOT: sprintf(ptag,"BOOT");
					sprintf(pname,"reserved");
					break;
			   case V_ROOT: sprintf(ptag,"ROOT");
					sprintf(pname,"root");
				        break;
			   case V_SWAP:	sprintf(ptag,"SWAP");
					sprintf(pname,"swap");
					break;
			   case V_USR:	sprintf(ptag,"USR");
					if (i == 3)
						sprintf(pname,"usr");
					else
						if (i == 4)
							sprintf(pname,"home");
						else
							sprintf(pname,"home%d",i);
					break;
			   case V_BACKUP: sprintf(ptag,"DISK");
					sprintf(pname,"disk");
					break;
			   case V_STAND: sprintf(ptag,"STAND");
					sprintf(pname,"stand");
					break;
			   case V_HOME:	sprintf(ptag,"HOME");
					sprintf(pname,"home%d",i);
					break;
			   case V_DUMP:	sprintf(ptag,"DUMP");
					sprintf(pname,"dump");
					break;
			   case V_VAR:	sprintf(ptag,"VAR");
					sprintf(pname,"var");
					break;
			   case V_ALTS:	sprintf(ptag,"ALTS");
					sprintf(pname,"alts");
					break;
			   case V_ALTTRK: sprintf(ptag,"ALTTRK");
					sprintf(pname,"trkalt");
					break;
			   case V_ALTSCTR:	sprintf(ptag,"ALTSCTR");
					sprintf(pname,"altsctr");
					break;
			   case V_MANAGED_1:	sprintf(ptag,"VOLPUBLIC");
					sprintf(pname,"volpublic");
					break;
			   case V_MANAGED_2:	sprintf(ptag,"VOLPRIVATE");
					sprintf(pname,"volprivate");
					break;
			   case V_OTHER: sprintf(ptag,"OTHER");
					sprintf(pname,"dos");
					break;
			   default:	printf(ptag,"UNKNOWN");	
					sprintf(pname,"unknown");
					break;
			}
			fprintf(partfile,
			"%s:\n\tpartition = %d, start = %ld, size = %ld,\n",
			pname,i,vtoc.v_slices[i].p_start, vtoc.v_slices[i].p_size);
			fprintf(partfile, "\ttag = %s,", ptag);
			if (vtoc.v_slices[i].p_flag & V_UNMNT)
				fprintf(partfile,
					" perm = NOMOUNT, perm = VALID\n\n");
			else
                                if (vtoc.v_slices[i].p_flag & V_RONLY)
                                        fprintf(partfile," perm = RO, perm = VALID\n\n");
		    		else
                                        fprintf(partfile," perm = VALID\n\n");
		}
	}
	fclose(partfile);
	close(partfd);
	(void) pfmt(stdout, MM_NOSTD,
		":60:Successfully created/updated /etc/partitions file.\n");
}
