#ident	"@(#)mp.cmds:common/cmd/dispadmin/fpdispadmin.c	1.3.1.1"

/***************************************************************************
 * Command: FPdispadmin
 *
 * Inheritable Privileges: P_FPIME,P_SYSOPS
 *       Fixed Privileges: None
 *
 * Notes: This file contains the class specific code implementing
 * 	  the fixed-time dispadmin sub-command.
 *
 ***************************************************************************/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/priocntl.h>
#include	<sys/fppriocntl.h>
#include	<sys/param.h>
#include	"time_res.h"
#include	<sys/fpri.h>

#define BASENMSZ	16
#define MATCH 		0
#define FP_TQINF_STRING	"FP_TQINF"

extern int	errno;
extern char	*basename();
extern void	fatalerr();
extern long	hrtconvert();

static void	get_fpdptbl(), set_fpdptbl();

static char usage[] =
"usage:	dispadmin -l\n\
	dispadmin -c FP -g [-r res]\n\
	dispadmin -c FP -s infile\n";

static char	basenm[BASENMSZ];
static char	cmdpath[256];

/*
 * Procedure: main - process options and call appropriate subroutines
 */
main(argc, argv)
int	argc;
char	**argv;
{
	extern char	*optarg;

	int		c;
	int		lflag, gflag, rflag, sflag;
	ulong		res;
	char		*infile;

	strcpy(cmdpath, argv[0]);
	strcpy(basenm, basename(argv[0]));
	lflag = gflag = rflag = sflag = 0;
	while ((c = getopt(argc, argv, "lc:gr:s:")) != -1) {
		switch (c) {

		case 'l':
			lflag++;
			break;

		case 'c':
			if (strcmp(optarg, "FP") != 0)
				fatalerr("error: %s executed for %s class, \
%s is actually sub-command for FP class\n", cmdpath, optarg, cmdpath);
			break;

		case 'g':
			gflag++;
			break;

		case 'r':
			rflag++;
			res = strtoul(optarg, (char **)NULL, 10);
			break;

		case 's':
			sflag++;
			infile = optarg;
			break;

		case '?':
			fatalerr(usage);

		default:
			break;
		}
	}

	if (lflag) {
		if (gflag || rflag || sflag)
			fatalerr(usage);

		printf("FP\t(Fixed Priority)\n");
		exit(0);

	} else if (gflag) {
		if (lflag || sflag)
			fatalerr(usage);

		if (rflag == 0)
			res = 1000;

		get_fpdptbl(res);
		exit(0);

	} else if (sflag) {
		if (lflag || gflag || rflag)
			fatalerr(usage);

		set_fpdptbl(infile);
		exit(0);

	} else {
		fatalerr(usage);
	}
	/* NOTREACHED */
}


/*
 * Procedure: get_fpdptbl
 *
 * Notes: Retrieve the current fp_dptbl from memory, convert the time
 *	  quantum values to the resolution specified by res and write
 *	  the table to stdout.
 *
 * Restrictions: priocntl(2): <none>
 */
static void
get_fpdptbl(res)
ulong	res;
{
	int		i;
	int		fpdpsz;
	pcinfo_t	pcinfo;
	pcadmin_t	pcadmin;
	fpadmin_t	fpadmin;
	fpdpent_t	*fp_dptbl;
	hrtime_t	hrtime;

	strcpy(pcinfo.pc_clname, "FP");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1)
		fatalerr("%s: Can't get FP class ID, priocntl system \
call failed with errno %d\n", basenm, errno);

	pcadmin.pc_cid = pcinfo.pc_cid;
	pcadmin.pc_cladmin = (char *)&fpadmin;
	fpadmin.fp_cmd = FP_GETDPSIZE;

	if (priocntl(0, 0, PC_ADMIN, &pcadmin) == -1)
		fatalerr("%s: Can't get fp_dptbl size, priocntl system \
call failed with errno %d\n", basenm, errno);

	fpdpsz = fpadmin.fp_ndpents * sizeof(fpdpent_t);
	if ((fp_dptbl = (fpdpent_t *)malloc(fpdpsz)) == NULL)
		fatalerr("%s: Can't allocate memory for fp_dptbl\n", basenm);

	fpadmin.fp_dpents = fp_dptbl;

	fpadmin.fp_cmd = FP_GETDPTBL;
	if (priocntl(0, 0, PC_ADMIN, &pcadmin) == -1)
		fatalerr("%s: Can't get fp_dptbl, priocntl system call \
failed with errno %d\n", basenm, errno);

	printf("# Fixed Priority Dispatcher Configuration\n");
	printf("RES=%ld\n\n", res);
	printf("# TIME QUANTUM                    PRIORITY\n");
	printf("# (fp_quantum)                      LEVEL\n");

	for (i = 0; i < fpadmin.fp_ndpents; i++) {
		if (fp_dptbl[i].fp_quantum == FP_TQINF) {
			(void)printf("%10s                    #      %3d\n",
		    FP_TQINF_STRING, i);
			continue;
		}

		if (res != HZ) {
			hrtime.hrt_secs = 0;
			hrtime.hrt_rem = fp_dptbl[i].fp_quantum;
			hrtime.hrt_res = HZ;
			if (hrtnewres(&hrtime, res, HRT_RNDUP) == -1)
				fatalerr("%s: Can't convert to requested \
resolution\n", basenm);
			if((fp_dptbl[i].fp_quantum = hrtconvert(&hrtime)) == -1)
				fatalerr("%s: Can't express time quantum in \
requested resolution,\ntry coarser resolution\n", basenm);
		}
		printf("%10ld                    #      %3d\n",
		    fp_dptbl[i].fp_quantum, i);
	}
}


/*
 * Procedure: set_fpdptbl
 *
 * Notes: Read the fp_dptbl values from infile, convert the time
 *	  quantum values to HZ resolution, do a little sanity checking
 *	  and overwrite the table in memory with the values from the
 *	  file.
 *
 * Restrictions: priocntl(2): <none>
 */
static void
set_fpdptbl(infile)
char	*infile;
{
	int		i;
	int		nfpdpents;
	char		*tokp;
	pcinfo_t	pcinfo;
	pcadmin_t	pcadmin;
	fpadmin_t	fpadmin;
	fpdpent_t	*fp_dptbl;
	int		linenum;
	ulong		res;
	hrtime_t	hrtime;
	FILE		*fp;
	char		buf[512];
	int		wslength;

	strcpy(pcinfo.pc_clname, "FP");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1)
		fatalerr("%s: Can't get FP class ID, priocntl system \
call failed with errno %d\n", basenm, errno);

	pcadmin.pc_cid = pcinfo.pc_cid;
	pcadmin.pc_cladmin = (char *)&fpadmin;
	fpadmin.fp_cmd = FP_GETDPSIZE;

	if (priocntl(0, 0, PC_ADMIN, &pcadmin) == -1)
		fatalerr("%s: Can't get fp_dptbl size, priocntl system \
call failed with errno %d\n", basenm, errno);

	nfpdpents = fpadmin.fp_ndpents;
	if ((fp_dptbl =
	    (fpdpent_t *)malloc(nfpdpents * sizeof(fpdpent_t))) == NULL)
		fatalerr("%s: Can't allocate memory for fp_dptbl\n", basenm);

	if ((fp = fopen(infile, "r")) == NULL)
		fatalerr("%s: Can't open %s for input\n", basenm, infile);

	linenum = 0;

	/*
	 * Find the first non-blank, non-comment line.  A comment line
	 * is any line with '#' as the first non-white-space character.
	 */
	do {
		if (fgets(buf, sizeof(buf), fp) == NULL)
			fatalerr("%s: Too few lines in input table\n",basenm);
		linenum++;
	} while (buf[0] == '#' || buf[0] == '\0' ||
	    (wslength = strspn(buf, " \t\n")) == strlen(buf) ||
	    strchr(buf, '#') == buf + wslength);

	if ((tokp = strtok(buf, " \t")) == NULL)
		fatalerr("%s: Bad RES specification, line %d of input file\n",
		    basenm, linenum);
	if (strlen(tokp) > (size_t) 4) {
		if (strncmp(tokp, "RES=", 4) != 0)
			fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
		if (tokp[4] == '-')
			fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
		res = strtoul(&tokp[4], (char **)NULL, 10);
	} else if (strlen(tokp) == 4) {
		if (strcmp(tokp, "RES=") != 0)
			fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
		if ((tokp = strtok(NULL, " \t")) == NULL)
			fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
		if (tokp[0] == '-')
			fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
		res = strtoul(tokp, (char **)NULL, 10);
	} else if (strlen(tokp) == 3) {
		if (strcmp(tokp, "RES") != 0)
			fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
		if ((tokp = strtok(NULL, " \t")) == NULL)
			fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
		if (strlen(tokp) > (size_t) 1) {
			if (strncmp(tokp, "=", 1) != 0)
				fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
			if (tokp[1] == '-')
				fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
			res = strtoul(&tokp[1], (char **)NULL, 10);
		} else if (strlen(tokp) == 1) {
			if ((tokp = strtok(NULL, " \t")) == NULL)
				fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
			if (tokp[0] == '-')
				fatalerr("%s: Bad RES specification, \
line %d of input file\n", basenm, linenum);
			res = strtoul(tokp, (char **)NULL, 10);
		}
	} else {
		fatalerr("%s: Bad RES specification, line %d of input file\n",
		    basenm, linenum);
	}

	/*
	 * The remainder of the input file should contain exactly enough
	 * non-blank, non-comment lines to fill the table (fp_ndpents lines).
	 * We assume that any non-blank, non-comment line is data for the
	 * table and fail if we find more or less than we need.
	 */
	for (i = 0; i < fpadmin.fp_ndpents; i++) {

		/*
		 * Get the next non-blank, non-comment line.
		 */
		do {
			if (fgets(buf, sizeof(buf), fp) == NULL)
				fatalerr("%s: Too few lines in input table\n",
				    basenm);
			linenum++;
		} while (buf[0] == '#' || buf[0] == '\0' ||
		    (wslength = strspn(buf, " \t\n")) == strlen(buf) ||
		    strchr(buf, '#') == buf + wslength);

		if ((tokp = strtok(buf, " \t")) == NULL)
			fatalerr("%s: Too few values, line %d of input file\n",
			    basenm, linenum);

		if ( strcmp(tokp,FP_TQINF_STRING) == MATCH) {
			(void)printf("Strings match on line %d\n",i);
			fp_dptbl[i].fp_quantum=FP_TQINF;
		} else {
			if (res != HZ) {
				hrtime.hrt_secs = 0;
				hrtime.hrt_rem = atol(tokp);
				hrtime.hrt_res = res;
				if (hrtnewres(&hrtime, HZ, HRT_RNDUP) == -1)
					fatalerr("%s: Can't convert specified \
	resolution to ticks\n", basenm);
				if((fp_dptbl[i].fp_quantum = hrtconvert(&hrtime)) == -1)
					fatalerr("%s: fp_quantum value out of \
	valid range; line %d of input,\ntable not overwritten\n", basenm, linenum);
			} else {
				fp_dptbl[i].fp_quantum = atol(tokp);
			}
		}

		if (fp_dptbl[i].fp_quantum <= 0 && fp_dptbl[i].fp_quantum != FP_TQINF)
			fatalerr("%s: fp_quantum value out of valid range; \
line %d of input,\ntable not overwritten\n", basenm, linenum);

		if ((tokp = strtok(NULL, " \t")) != NULL && tokp[0] != '#')
			fatalerr("%s: Too many values, line %d of input file\n",
			    basenm, linenum);
	}

	/*
	 * We've read enough lines to fill the table.  We fail
	 * if the input file contains any more.
	 */
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (buf[0] != '#' && buf[0] != '\0' &&
		    (wslength = strspn(buf, " \t\n")) != strlen(buf) &&
		    strchr(buf, '#') != buf + wslength)
			fatalerr("%s: Too many lines in input table\n",
				basenm);
	}

	fpadmin.fp_dpents = fp_dptbl;
	fpadmin.fp_cmd = FP_SETDPTBL;
	
	if (priocntl(0, 0, PC_ADMIN, &pcadmin) == -1)
		fatalerr("%s: Can't set fp_dptbl, priocntl system call \
failed with errno %d\n", basenm, errno);
}
