#ident	"@(#)mp.cmds:common/cmd/dispadmin/fcdispadmin.c	1.1.1.1"
#ident  "$Header$"

/***************************************************************************
 * Command: FCdispadmin
 *
 * Inheritable Privileges: P_FCHAR,P_SYSOPS
 *       Fixed Privileges: None
 *
 * Notes: This file contains the class specific code implementing
 * 	  the times-sharing dispadmin sub-command.
 *
 ***************************************************************************/

#include	<stdio.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/priocntl.h>
#include	<sys/fcpriocntl.h>
#include	<sys/param.h>
#include	"time_res.h"
#include	<sys/fc.h>

#define	BASENMSZ	16

extern int	errno;
extern char	*basename();
extern void	fatalerr();
extern long	hrtconvert();

static void	get_fcdptbl(), set_fcdptbl();

static char usage[] =
"usage:	dispadmin -l\n\
	dispadmin -c FC -g [-r res]\n\
	dispadmin -c FC -s infile\n";

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
			if (strcmp(optarg, "FC") != 0)
				fatalerr("error: %s executed for %s class, \
%s is actually sub-command for FC class\n", cmdpath, optarg, cmdpath);
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

		printf("FC\t(Time Sharing)\n");
		exit(0);

	} else if (gflag) {
		if (lflag || sflag)
			fatalerr(usage);

		if (rflag == 0)
			res = 1000;

		get_fcdptbl(res);
		exit(0);

	} else if (sflag) {
		if (lflag || gflag || rflag)
			fatalerr(usage);

		set_fcdptbl(infile);
		exit(0);

	} else {
		fatalerr(usage);
	}
	/* NOTREACHED */
}


/*
 * Procedure: get_fcdptbl
 *
 * Notes: Retrieve the current fc_dptbl from memory, convert the time
 *	  quantum values to the resolution specified by res and write
 *	  the table to stdout. 
 *
 * Restrictions: priocntl(2): <none>
 */
static void
get_fcdptbl(res)
ulong	res;
{
	int		i;
	int		fcdpsz;
	pcinfo_t	pcinfo;
	pcadmin_t	pcadmin;
	fcadmin_t	fcadmin;
	fcdpent_t	*fc_dptbl;
	hrtime_t	hrtime;

	strcpy(pcinfo.pc_clname, "FC");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1)
		fatalerr("%s: Can't get FC class ID, priocntl system \
call failed with errno %d\n", basenm, errno);

	pcadmin.pc_cid = pcinfo.pc_cid;
	pcadmin.pc_cladmin = (char *)&fcadmin;
	fcadmin.fc_cmd = FC_GETDPSIZE;

	if (priocntl(0, 0, PC_ADMIN, &pcadmin) == -1)
		fatalerr("%s: Can't get fc_dptbl size, priocntl system \
call failed with errno %d\n", basenm, errno);

	fcdpsz = fcadmin.fc_ndpents * sizeof(fcdpent_t);
	if ((fc_dptbl = (fcdpent_t *)malloc(fcdpsz)) == NULL)
		fatalerr("%s: Can't allocate memory for fc_dptbl\n", basenm);

	fcadmin.fc_dpents = fc_dptbl;

	fcadmin.fc_cmd = FC_GETDPTBL;
	if (priocntl(0, 0, PC_ADMIN, &pcadmin) == -1)
		fatalerr("%s: Can't get fc_dptbl, priocntl system call \
call failed with errno %d\n", basenm, errno);

	printf("# Fixed Class Dispatcher Configuration\n");
	printf("RES=%ld\n\n", res);
	printf("# fc_quantum    PRIORITY LEVEL\n");

	for (i = 0; i < fcadmin.fc_ndpents; i++) {
		if (res != HZ) {
			hrtime.hrt_secs = 0;
			hrtime.hrt_rem = fc_dptbl[i].fc_quantum;
			hrtime.hrt_res = HZ;
			if (hrtnewres(&hrtime, res, HRT_RNDUP) == -1)
				fatalerr("%s: Can't convert to requested \
resolution\n", basenm);
			if ((fc_dptbl[i].fc_quantum = hrtconvert(&hrtime))
			    == -1)
				fatalerr("%s: Can't express time quantum in \
requested resolution,\ntry coarser resolution\n", basenm);
		}
		printf("%10d #   %3d\n", fc_dptbl[i].fc_quantum, i);
	}
}


/*
 * Procedure: set_fcdptbl
 *
 * Notes: Read the fc_dptbl values from infile, convert the time
 *	  quantum values to HZ resolution, do a little sanity checking
 *	  and overwrite the table in memory with the values from the
 *	  file.
 *
 * Restrictions: priocntl(2): <none>
 */
static void
set_fcdptbl(infile)
char	*infile;
{
	int		i;
	int		nfcdpents;
	char		*tokp;
	pcinfo_t	pcinfo;
	pcadmin_t	pcadmin;
	fcadmin_t	fcadmin;
	fcdpent_t	*fc_dptbl;
	int		linenum;
	ulong		res;
	hrtime_t	hrtime;
	FILE		*fp;
	char		buf[512];
	int		wslength;

	strcpy(pcinfo.pc_clname, "FC");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1)
		fatalerr("%s: Can't get FC class ID, priocntl system \
call failed with errno %d\n", basenm, errno);

	pcadmin.pc_cid = pcinfo.pc_cid;
	pcadmin.pc_cladmin = (char *)&fcadmin;
	fcadmin.fc_cmd = FC_GETDPSIZE;

	if (priocntl(0, 0, PC_ADMIN, &pcadmin) == -1)
		fatalerr("%s: Can't get fc_dptbl size, priocntl system \
call failed with errno %d\n", basenm, errno);

	nfcdpents = fcadmin.fc_ndpents;
	if ((fc_dptbl =
	    (fcdpent_t *)malloc(nfcdpents * sizeof(fcdpent_t))) == NULL)
		fatalerr("%s: Can't allocate memory for fc_dptbl\n", basenm);

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
	 * non-blank, non-comment lines to fill the table (fc_ndpents lines).
	 * We assume that any non-blank, non-comment line is data for the
	 * table and fail if we find more or less than we need.
	 */
	for (i = 0; i < fcadmin.fc_ndpents; i++) {

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

		if (res != HZ) {
			hrtime.hrt_secs = 0;
			hrtime.hrt_rem = atol(tokp);
			hrtime.hrt_res = res;
			if (hrtnewres(&hrtime, HZ, HRT_RNDUP) == -1)
				fatalerr("%s: Can't convert specified \
resolution to ticks\n", basenm);
			if((fc_dptbl[i].fc_quantum = hrtconvert(&hrtime)) == -1)
				fatalerr("%s: fc_quantum value out of \
valid range; line %d of input,\ntable not overwritten\n", basenm, linenum);
		} else {
			fc_dptbl[i].fc_quantum = atol(tokp);
		}
		if (fc_dptbl[i].fc_quantum <= 0)
			fatalerr("%s: fc_quantum value out of valid range; \
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

	fcadmin.fc_dpents = fc_dptbl;
	fcadmin.fc_cmd = FC_SETDPTBL;
	if (priocntl(0, 0, PC_ADMIN, &pcadmin) == -1)
		fatalerr("%s: Can't set fc_dptbl, priocntl system call \
failed with errno %d\n", basenm, errno);
}
