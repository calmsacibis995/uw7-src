/*		copyright	"%c%" 	*/

#ident	"@(#)fs.cmds:i386/cmd/fs.d/df.c	1.34.18.1"
#ident  "$Header$"

/***************************************************************************
 * Command: df
 * Inheritable Privileges: P_COMPAT,P_MACREAD,P_DACREAD,P_DEV
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/vfstab.h>
#include <sys/mnttab.h>
#include <sys/param.h>		/* for MAXNAMELEN - used in devnm() */
#include <dirent.h> 	
#include <fcntl.h>
#include <sys/wait.h>
/* new includes added for security */
#include	<mac.h>
#include <errno.h>
#include <priv.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define MNTTAB	"/etc/mnttab"
#define VFSTAB	"/etc/vfstab"
#define DEVNM	"/etc/devnm.path"
#define DEVNM_MAX	20	/*max # of dev paths to search in devnm.path*/
#define MAX_OPTIONS	20	/* max command line options */
#define BLOCK_FRAG	2	/* original statement was *512/1024 - but this
				 * caused overflow to occur early when the 512
				 * was enacted - this buys us time */

static char *argp[MAX_OPTIONS];	/* command line for specific module*/
static int argpc;
static int status;
static char path[BUFSIZ];
static int k_header = 0;
static int header = 0;
static int eb_flg = 0;
static int posix;

#define DEVLEN 1024	/* for devnm() */
static struct stat S;
static struct stat Sbuf;
static char *devnm();
static char *basename();
static int is_remote();
static void mnterror(int);
static void mk_cmdline(char *, char *, int, char *, char *);
static void print_statvfs(struct mnttab *, struct statvfs *, int);
static void exec_specific(char *);
static void echo_cmdline(char **, int, char *);
static void build_path(char *, char *);
static int dsearch(DIR *,dev_t);


/*
 * Procedure:     main
 *
 * Restrictions:
                 getopt: none
                 setlocale: none
                 stat(2): none
                 pfmt: none
                 strerror: none
                 printf: none
                 fopen: P_MACREAD for MNTTAB and VFSTAB
                 getmntent: P_MACREAD for MNTTAB
                 statvfs(2): none
                 fclose: none
                 getmntany: P_MACREAD for MNTTAB
                 getvfsspec: P_MACREAD for VFSTAB
 * Generic DF 
 *
 * This is the generic part of the df code. It is used as a
 * switchout mechanism and in turn executes the file system
 * type specific code located at /usr/lib/fs/FSType. 
 *
 */

static int	mac_install;	/* defines whether security MAC is installed */
static level_t	level;		/* new MAC security level ceiling for mounted file system */
static const char badopen[] = ":4:Cannot open %s: %s\n";
static const char badstatvfs[] = ":250:statvfs() on %s failed: %s\n";
static const char badstat[] = ":5:Cannot access %s: %s\n";
static const char otherfs[] = ":251:%s mounted as a %s file system\n";

static const char mnttab[] = MNTTAB;
static const char vfstab[] = VFSTAB;
static char posix_var[] = "POSIX2";

/*
 * Common code used to call print_statvfs() with the
 * appropriate flag based on options given on the command line.
 */
#define	PRINT_OPT()						\
		/*						\
		 * P_flg related code below is part of		\
		 * POSIX.2 command compliance support		\
		 */						\
		if (P_flg && k_flg) {				\
			print_statvfs(&mountb, &statbuf, 'Pk');	\
			continue;				\
		} else if (P_flg && !k_flg) {			\
			print_statvfs(&mountb, &statbuf, 'P');	\
			continue;				\
		}						\
		if (g_flg) {					\
			print_statvfs(&mountb, &statbuf, 'g');	\
			continue;				\
		}						\
		if (k_flg) {					\
			print_statvfs(&mountb, &statbuf, 'k');	\
			continue;				\
		}						\
		/* 						\
		 * i_flg related code below is part of the	\
	         * Enhanced Application Compatibility Support	\
		 */						\
		if (v_flg && !i_flg) {				\
			print_statvfs(&mountb, &statbuf, 'v');	\
			continue;				\
		} else if (v_flg && i_flg) {			\
			print_statvfs(&mountb, &statbuf, 'vi');	\
			continue;				\
		} else if (!v_flg && i_flg) {			\
			print_statvfs(&mountb, &statbuf, 'i');	\
			continue;				\
		}						\
		/* End						\
		 * Enhanced Application Compatibility Support	\
		 */						\
		if (t_flg) {					\
			print_statvfs(&mountb, &statbuf, 't');	\
			continue;				\
		}						\
		if (b_flg) {					\
			print_statvfs(&mountb, &statbuf, 'b');	\
		}						\
		if (e_flg) {					\
			print_statvfs(&mountb, &statbuf, 'e');	\
		}						\
		if (n_flg) {					\
			print_statvfs(&mountb, &statbuf, 'n');	\
		}						\
		if (b_flg || n_flg || e_flg)			\
			continue;				\
		if (F_flg) {					\
			print_statvfs(&mountb, &statbuf, 'x');	\
			continue;				\
		}						\
		print_statvfs(&mountb, &statbuf, 'x');


main (argc, argv)
int argc;
char *argv[];

{
	int arg;			/* argument from getopt() */
	int usgflg, b_flg, e_flg, V_flg, o_flg, k_flg;
	int t_flg, g_flg, n_flg, l_flg, f_flg, F_flg, v_flg;
	int P_flg = 0;			/* POSIX.2 print format option */
/* Enhanced Application Compatibility Support */
	int i_flg = 0;
/* End Enhanced Application Compatibility Support */

	int i, j;

	mode_t mode;

	char	 *res_name, *s, *opt;
	int	 errcnt = 0;
	int	 exitcode = 0;
	int	 notfound=1;
	int	 res_found=0;

	char *FSType = NULL;		/* FSType */
	char *oargs;			/* FSType specific argument */
	char options[MAX_OPTIONS];	/* options for specific module */

	char *usage = ":1356:Usage:\ndf [-F FSType] [-beglnPtV | -k | -iv] [current_options] [-o specific_options] [directory | special | resource...]\n";

	FILE *fp, *fp2;
	struct mnttab mountb;
	struct mnttab mm;
	struct statvfs statbuf;
	struct vfstab	vfsbuf;
	struct stat stbuf;

	if (getenv(posix_var) != 0)	{
		posix = 1;
	} else	{
		posix = 0;
	}

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");


	usgflg = b_flg = e_flg = V_flg = k_flg = eb_flg = 0;
	t_flg = g_flg = n_flg = l_flg = f_flg = v_flg = 0; 


	F_flg = o_flg = 0;
	(void)strcpy(options, "-");

	/* see if the command called is devnm */

	s = basename(argv[0]);
	if(!strncmp(s,"devnm",5)) {
		(void)setlabel("UX:devnm");

		if (argc == 1){
			(void)pfmt(stderr, MM_ERROR, ":1357:No mountpoint specified\n");
			(void)pfmt(stderr,MM_ERROR, ":1358:Usage: devnm [mountpoint-name ...]\n");
		}
		while(--argc) {
			if(stat(*++argv, &S) == -1) {
				(void)pfmt(stderr, MM_ERROR, badstat, *argv,
					strerror(errno));
				errcnt++;
				continue;
			}
			res_name = devnm();
			if(res_name[0] != '\0')
				(void)printf("%s %s\n", res_name, *argv);
			else {
				(void)pfmt(stderr, MM_ERROR, ":253:%s not found\n",
					*argv);
				errcnt++;
			}
		}
		exit(errcnt);
	}
	(void)setlabel("UX:df");

	/* establish upfront if the enhanced security package is installed */
	if ((lvlproc(MAC_GET, &level) == -1) && (errno ==ENOPKG))
		mac_install = 0;	
	else
		mac_install = 1;

	/* if init is running df, then we need MACREAD, otherwise we don't want it*/

	if (mac_install && level)
		(void)procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);

	/* open mnttab and vfstab */

	if (( fp = fopen(MNTTAB, "r")) == NULL){
		(void)pfmt(stderr, MM_ERROR, badopen, mnttab, strerror(errno));
		exit(2);
	}

	if (( fp2 = fopen(VFSTAB, "r")) == NULL){
		(void)pfmt(stderr, MM_ERROR, badopen, vfstab, strerror(errno));
		exit(2);
	}

	if (mac_install && level)
		(void)procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);
	/* 
 	* If there are no arguments to df then the generic 
 	* determines the file systems mounted from /etc/mnttab
 	* and does a statvfs on them and reports on the generic
 	* superblock information
 	*/

	if (argc == 1) {		/* no arguments or options */

		while (( i = getmntent(fp, &mountb)) == 0 ) {
			if ((j = statvfs(mountb.mnt_mountp, &statbuf)) != 0){
				(void)pfmt(stderr, MM_ERROR, badstatvfs,
					mountb.mnt_mountp, strerror(errno));
				exitcode=1;
				continue;
			}
			print_statvfs(&mountb, &statbuf, 'x');
 		}
		if (i > 0 ) {
			mnterror(i);
			exit(1);
		}
		exit(exitcode);
	}

	/* One or more options or arguments */
	/* Process the Options */ 

	while ((arg = getopt(argc,argv,"F:o:?beikVtgnlfvP")) != -1) {

		switch(arg) {
		case 'v':       /* print verbose output */
			 v_flg = 1;
			 break;
		case 'b':	/* print kilobytes free */
			b_flg = 1;
			(void)strcat(options, "b");
			break;
		case 'e':	/* print file entries free */
			e_flg = 1;
			(void)strcat(options, "e");
			break;
		case 'V':	/* echo complete command line */
			V_flg = 1;
			(void)strcat(options, "V");
			break;
		case 't':	/* full listing with totals */
			t_flg = 1;
			(void)strcat(options, "t");
			break;
		case 'g':	/* print entire statvfs structure */
			g_flg = 1;
			(void)strcat(options, "g");
			break;
		case 'n':	/* print FSType name */
			n_flg = 1;
			(void)strcat(options, "n");
			break;
		case 'l':	/* report on local File systems only */
			l_flg = 1;
			(void)strcat(options, "l");
			break;
		case 'F':	/* FSType specified */
			if (F_flg) {
				(void)pfmt(stderr, MM_ERROR, ":254:More than one FSType specified\n");
				(void)pfmt(stderr, MM_ACTION, usage);
				exit(2);
			}
			F_flg = 1;
			FSType = optarg;
			if (( i = strlen(FSType)) > 8 ) {
				(void)pfmt(stderr, MM_ERROR, ":255:FSType %s exceeds 8 characters\n", FSType);
				exit(2);
			}
			break;
		case 'o':	/* FSType specific arguments */
			o_flg = 1;
			oargs = optarg;
			break;
		case 'f':	/* perform actual count on free list */
			f_flg = 1;
			(void)strcat(options, "f");
			break;
		case 'k':	/* new format */
			k_flg = 1;
			(void)strcat(options, "k");
			break;

		/* Enhanced Application Compatibility Support */
		case 'i':	/* print inode info */
			i_flg = 1;
			(void)strcat(options, "i");
			break;
		/* End Enhanced Application Compatibility Support */

		case 'P':	/* POSIX.2 print formatting option */
			P_flg = 1;
			(void)strcat(options, "P");
			break;

		case '?':	/* print usage message */
			usgflg = 1;
			(void)strcat(options, "?");
		}
	}
	/* mutually check of k flag and other flags */
	if (k_flg && (opt = strpbrk(options, "Fobetnfvi")) != NULL)
	{
		(void)pfmt(stderr, MM_ERROR, ":1351:invalid combination of options -k with -%.1s\n",opt);
		(void)pfmt(stderr, MM_ACTION, usage);
		exit(2);
	}
	if (v_flg || i_flg) {
		if ((opt = strpbrk(options, "Fobektgnf")) != NULL) {
			if (i_flg)
				(void)pfmt(stderr, MM_ERROR, ":1350:invalid combination of options -i with -%.1s\n",opt);
			else	
				(void)pfmt(stderr, MM_ERROR, ":1352:invalid combination of options -v with -%.1s\n",opt);
			(void)pfmt(stderr, MM_ACTION, usage);
			exit(2);
		}
	}
	if (posix && P_flg) {
		V_flg = 0;
	}
	if (P_flg && !k_flg) {
		(void)pfmt(stdout, MM_NOSTD,
		    ":1365:Filesystem          512-blocks  Used     Available  Capacity  Mounted on\n");
	} else if (P_flg && k_flg) {
		(void)pfmt(stdout, MM_NOSTD,
		    ":1366:Filesystem          1024-blocks Used     Available  Capacity  Mounted on\n");
	/* i_flg related code below is part of the 
	   Enhanced Application Compatibility Support */
	} else if (v_flg && !i_flg)
		(void)pfmt(stdout, MM_NOSTD,
		    ":1367:Mount Dir  Filesystem           blocks      used     avail  %%used\n");
	else if (v_flg && i_flg)
		(void)pfmt(stdout, MM_NOSTD,
		    ":1124:Mount Dir  Filesystem        blocks     used    avail %%used iused  ifree %%iused\n");
	else if (!v_flg && i_flg)
		(void)pfmt(stdout, MM_NOSTD,
			":1078:Mount Dir  Filesystem            iused     ifree    itotal %%iused\n");
	/* End Enhanced Application Compatibility Support */

	if ((!F_flg) && (usgflg)) {
		(void)pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		(void)pfmt(stderr, MM_ACTION, usage);
		exit(2);
	}
	if (b_flg && e_flg) {
		eb_flg++;
	}

	/* if no arguments (only options): process mounted file systems only */
	if (optind == argc) {
		if (V_flg) {
			while ((i = getmntent(fp, &mountb)) == 0 ) {
				if ((F_flg) && (strcmp(FSType, mountb.mnt_fstype) != 0))
					continue;
				if ((l_flg) && (is_remote(mountb.mnt_fstype)))
					continue;
				mk_cmdline(mountb.mnt_fstype,
						options,
						o_flg,
						oargs,
						mountb.mnt_special);
		  		echo_cmdline(argp, argpc, mountb.mnt_fstype);
		 	}
			if (i > 0 ) {
				mnterror(i);
				exit(1);
			}
		 	exit(0);
		}
		if (f_flg || o_flg) {	/* specific df */
			while ((i = getmntent(fp, &mountb)) == 0 ) {
				if ((F_flg) && ( strcmp(FSType, mountb.mnt_fstype) != 0))
					continue;
				mk_cmdline(mountb.mnt_fstype,
						options,
						o_flg,
						oargs,
						mountb.mnt_special);
				build_path(mountb.mnt_fstype, path);
				exec_specific(mountb.mnt_fstype); 
			}
			if (i > 0 ) {
				mnterror(i);
				exit(1);
			}
			exit(0);
		} /* end specific df */

		if ( F_flg && usgflg ) {
			mk_cmdline(FSType,
					options,
					o_flg,
					oargs,
					NULL);
			build_path(FSType, path);
			exec_specific(FSType);
			exit(0); 	/* only one usage mesg required */
		}
		/* f or o flags are not set */
		while (( i = getmntent(fp, &mountb)) == 0 ) {
			if ((F_flg) && (strcmp(FSType, mountb.mnt_fstype) != 0))
				continue;
	    		if ((l_flg) && (is_remote(mountb.mnt_fstype))) 
				continue;
			j = statvfs(mountb.mnt_mountp, &statbuf);
			if (j != 0){
				(void)pfmt(stderr, MM_ERROR, badstatvfs,
					mountb.mnt_mountp, strerror(errno)); 
				exitcode=1;
				continue;
			}
			PRINT_OPT();
 		}
		if (i > 0 ) {
			mnterror(i);
			exit(1);
		}
		exit(exitcode);

	}  /* end case of no arguments */

	/* arguments could be mounted/unmounted file systems */
	for (; optind < argc; optind++) {

		(void)fclose(fp);
		(void)fclose(fp2);
		if (( fp = fopen(MNTTAB, "r")) == NULL){
			(void)pfmt(stderr, MM_ERROR, badopen, mnttab, strerror(errno));
			exit(2);
		}
		if (( fp2 = fopen(VFSTAB, "r")) == NULL){
			(void)pfmt(stderr, MM_ERROR, badopen, vfstab, strerror(errno));
			exit(2);
		}

		mntnull(&mm);
		mm.mnt_special = argv[optind];

		/* case argument is special from mount table */
		i = getmntany(fp, &mountb, &mm);
		if (i == 0){

			if ((l_flg) && (is_remote(mountb.mnt_fstype)))
				continue;
			if ((F_flg)&&(strcmp(mountb.mnt_fstype, FSType) != 0)){
				(void)pfmt(stderr, MM_WARNING, otherfs,
					mm.mnt_special, mountb.mnt_fstype);
				exitcode=1;
				continue;
			}
			if (V_flg) {
				if (!F_flg) 
					FSType = mountb.mnt_fstype;
				mk_cmdline(FSType,
						options,
						o_flg,
						oargs,
						mountb.mnt_special);
				echo_cmdline(argp, argpc, FSType);
				continue;
			}
			if (f_flg || o_flg) {
				if (!F_flg) 
					FSType = mountb.mnt_fstype;
				mk_cmdline(FSType,
						options,
						o_flg,
						oargs,
						mountb.mnt_special);
				build_path(FSType, path);
				exec_specific(FSType);
				continue;
			}
			j = statvfs(mountb.mnt_mountp, &statbuf);
			if (j != 0) {
				(void)pfmt(stderr, MM_ERROR, badstatvfs,
					mountb.mnt_mountp, strerror(errno)); 
				exitcode=1;
				continue;
			}
			if (F_flg && usgflg) {
				mk_cmdline(FSType,
					options,
					o_flg,
					oargs,
					mountb.mnt_special);
				exec_specific(FSType);
				exit(0);
			}
			PRINT_OPT();
			continue;
		}

		/* perform a stat(2) to determine file type */

		/* stat fails */
		i = stat(argv[optind], &stbuf);
		if (i == -1) {
			(void)pfmt(stderr, MM_ERROR, badstat, argv[optind],
				strerror(errno));
			exitcode=1;
		 	continue;
		}

		/* if block or character device */

		if ((( mode = ( stbuf.st_mode & S_IFMT )) == S_IFBLK) || 
			( mode == S_IFCHR )) {

			/* check if the device exists in vfstab */
			i = getvfsspec(fp2, &vfsbuf, argv[optind]);
			if (i != 0) {
				if (!F_flg) {
					(void)pfmt(stderr, MM_ERROR, ":257:FSType cannot be identified\n");
					exit(2);	
				}
				mk_cmdline(FSType, 
					options,
					o_flg,
					oargs,
					argv[optind]);
				if (V_flg) {
					echo_cmdline(argp, argpc, FSType);
					continue;
				}
				build_path(FSType, path);
				exec_specific(FSType);
				continue;
			}

			/* device exists in vfstab */
			if (!F_flg) 
				FSType = vfsbuf.vfs_fstype;
			if ( g_flg || n_flg || l_flg ){
				(void)pfmt(stderr, MM_ERROR, ":258:Options g, n or l not supported for unmounted FSTypes\n");
				exit(2);
			}
			mk_cmdline(FSType, 
					options,
					o_flg,
					oargs,
					vfsbuf.vfs_special);
			if (V_flg) {
				echo_cmdline(argp, argpc, FSType);
				continue;
			}
			build_path(FSType, path);
			exec_specific(FSType);
			continue;

		} /* end: block or character device */
		/* argument is a path */

		j = statvfs(argv[optind], &statbuf);
		if (j != 0) {
			(void)pfmt(stderr, MM_ERROR, badstatvfs, mountb.mnt_mountp,
				strerror(errno));
			exitcode=1;
			continue;
		}
		if ((F_flg)&&(strcmp(statbuf.f_basetype, FSType) != 0)){
			(void)pfmt(stderr, MM_WARNING, otherfs, argv[optind],
				statbuf.f_basetype);
			exitcode=1;

			continue;
		}
		if (V_flg) {
			mk_cmdline(statbuf.f_basetype,
					options,
					o_flg,
					oargs,
					argv[optind]);
			echo_cmdline(argp, argpc, statbuf.f_basetype);
			continue;
		}
		if (f_flg || o_flg) {
			mk_cmdline(statbuf.f_basetype,
					options,
					o_flg,
					oargs,	
					argv[optind]);
			build_path(statbuf.f_basetype, path);
			exec_specific(statbuf.f_basetype);
			continue;
		}
		/* rest handled by generic */
		mountb.mnt_mountp = argv[optind];	/* mount pt is file */
		i = stat(argv[optind], &S);
		if (i == -1) {
			(void)pfmt(stderr, MM_ERROR, badstat, argv[optind],
				strerror(errno));
			exitcode=1;
		 	continue;
		}
		res_name = devnm();
		/* Even if the resource name is found here, we may not
		   have the correct mountpoint(in the case where a path
		   was given below a mountpoint, ie, /usr is the mntpt,
		   but the argument given was /usr/include/sys.)
		*/
		   
		if(res_name[0] != '\0') {
			res_found++;
		}
			(void)fclose(fp);
			fp = fopen(MNTTAB, "r");
			if (fp == NULL) {
				(void)pfmt(stderr, MM_ERROR, badopen, mnttab,
					strerror(errno));
				exit(2);
			}
			mntnull(&mm);
			mm.mnt_mountp = argv[optind];
			/* case argument is mountpoint from mount table */
			/* if argument is a path below a mountpoint, then sat
			   each entry in the mttab and check if the device no.
			   matches.  
			*/
			i = getmntany(fp, &mountb, &mm);
			if (i != 0){
				if (i < 0) {
					(void)fclose(fp);
					fp = fopen(MNTTAB, "r");
					if (fp == NULL){
						(void)pfmt(stderr, MM_ERROR, badopen,
							mnttab, strerror(errno));
						exit(2);
					}
					mntnull(&mountb);
					i = stat(argv[optind], &S);
					if (i == -1) {
						(void)pfmt(stderr, MM_ERROR, badstat,
							argv[optind], strerror(errno));
						exitcode=1;
		 				continue;
					}

					while (getmntent(fp,&mountb) == 0) {
						i=stat(mountb.mnt_mountp,&Sbuf);
						if ( i<0 )  {
							(void)pfmt(stderr, MM_ERROR, badstat,
								argv[optind], strerror(errno));
							exitcode=1;
		 					continue;
						
						}
						if (S.st_dev == Sbuf.st_dev) {
							notfound=0;
							break;
						}
					}
					/* argument may be a path with a mounted
					   resource under it. ie, arg=/mnt/var,
					   where /mnt is mounted via rfs and /var
					   is a mounted file system.  In this case
					   stat will not return the same device
					   numbers so a match will never be found.
					   Since we have the info from statvfs print
					   it anyway.
					*/

					if (notfound) {
						mountb.mnt_mountp=argv[optind];
						if (res_found) {
							mountb.mnt_special=res_name; 
							res_found=0;
						}
						else {
							mountb.mnt_special="***************"; 
						}
					}
				}
			}

		mountb.mnt_fstype  = statbuf.f_basetype;
		if ((l_flg) && (is_remote(mountb.mnt_fstype))) 
			continue;
		PRINT_OPT();

	}	/* end: for all arguments */
	return(exitcode);
}	/* end main */




/*
 * Procedure:     echo_cmdline
 *
 * Restrictions:
                 printf: none

*/
static void
echo_cmdline(argp, argpc, fstype)
char *argp[];
int argpc;
char *fstype;
{
	int i;
	(void)printf("%s", argp[0]);
	if (fstype != NULL )	
		(void)printf(" -F %s", fstype);
	for( i= 1; i < argpc; i++) 
	        (void)printf(" %s", argp[i]);
	(void)printf("\n");

}

/*
 * Procedure:     mnterror
 *
 * Restrictions:
                 pfmt: None
*/
static void
mnterror(flag)
	int	flag;
{
	switch (flag) {
	case MNT_TOOLONG:
		(void)pfmt(stderr, MM_ERROR, ":259:Line in mnttab exceeds %d characters\n",
			MNT_LINE_MAX-2);
		break;
	case MNT_TOOFEW:
		(void)pfmt(stderr, MM_ERROR, ":260:Line in mnttab has too few entries\n");
		break;
	case MNT_TOOMANY:
		(void)pfmt(stderr, MM_ERROR, ":261:Line in mnttab has too many entries\n");
		break;
	}
}

/* function to generate command line to be passed to specific */
static void
mk_cmdline(fstype, options, o_flg, oargs, argument)
char *fstype;
char *options;
int  o_flg;
char *oargs;
char *argument;
{


	argpc = 0;
	argp[argpc++] = "df";
	if (strcmp(options, "-") != 0)
		argp[argpc++] = options;
	if (o_flg) {
		argp[argpc++] = "-o";
		argp[argpc++] = oargs;
	}
	argp[argpc++] = argument;
	argp[argpc] = NULL;
}


/*
 * Procedure:     print_statvfs
 *
 * Restrictions:
                 pfmt: none
*/
static void
print_statvfs(mountb, statbuf, flag)
struct mnttab *mountb;
struct statvfs *statbuf;
int flag;
{
	ulong_t	physblks;

	fsblkcnt_t	TotalBlocks,
			UsedBlocks,
			AvailBlocks,
			Capacity;

/* Enhanced Application Compatibility Support */
	fsfilcnt_t TotalInodes=0, FreeInodes=0, UsedInodes=0;

/* End Enhanced Application Compatibility Support */

	physblks=statbuf->f_frsize/512;
        TotalBlocks = statbuf->f_blocks*physblks;
        AvailBlocks = statbuf->f_bavail*physblks;
        UsedBlocks = TotalBlocks - AvailBlocks;
        Capacity = TotalBlocks ? ceil (100.0 * (double) UsedBlocks / (double) TotalBlocks) : 0;

	switch(flag) {

        case 'v':
		
		(void)pfmt(stdout, MM_NOSTD,
			":1466:%-10.10s %-17.17s %9Ld %9Ld %9Ld %4Ld%%\n",
                        mountb->mnt_mountp,
                        mountb->mnt_special,
                        TotalBlocks,
                        UsedBlocks,
                        AvailBlocks,
			Capacity);
                break;

        case 'P':
		
		(void)pfmt(stdout, MM_NOSTD,
			":1467:%-19s %-11Lu %-8Lu %-8Lu   %2Lu%%       %s\n",
                        mountb->mnt_special,
                        TotalBlocks,
                        UsedBlocks,
                        AvailBlocks,
			Capacity,
                        mountb->mnt_mountp);
                break;

        case 'Pk':
		
		(void)pfmt(stdout, MM_NOSTD,
			":1468:%-19s %-11Lu %-8Lu %-8Lu   %2Lu%%       %s\n",
                        mountb->mnt_special,
			TotalBlocks/BLOCK_FRAG,
			UsedBlocks/BLOCK_FRAG,
			AvailBlocks/BLOCK_FRAG,
			Capacity,
                        mountb->mnt_mountp);
                break;

	/* Enhanced Application Compatibility Support */
	case 'vi':
		TotalInodes = statbuf->f_files;
		FreeInodes = statbuf->f_favail;
		UsedInodes = statbuf->f_files - statbuf->f_favail;
		(void)pfmt(stdout, MM_NOSTD,
			":1469:%-8.8s %-17.17s %8Ld %8Ld %8Ld %3Ld%% %6Ld %6Ld %3Ld%%\n",
			mountb->mnt_mountp, 
			mountb->mnt_special,
			TotalBlocks,
			UsedBlocks,
			AvailBlocks,
			Capacity,
			UsedInodes,
			FreeInodes,
			TotalInodes ? (100 * UsedInodes / TotalInodes) : 0);

		break;
	
	case 'i':
		TotalInodes = statbuf->f_files;
		FreeInodes = statbuf->f_favail;
		UsedInodes = statbuf->f_files - statbuf->f_favail;

		(void)pfmt(stdout, MM_NOSTD,
			":1470:%-10.10s %-17.17s %9Ld %9Ld %9Ld %4Ld%%\n",
			mountb->mnt_mountp, 
			mountb->mnt_special,
			UsedInodes,
			statbuf->f_favail,
			TotalInodes,
			TotalInodes ? (100 * UsedInodes / TotalInodes) : 0);
		break;
	/* End Enhanced Application Compatibility Support */

	case 'g':
		(void)pfmt(stdout, MM_NOSTD,
			":1373:%-18s (%-19s):  %6lu block size  %7lu frag size\n",
			mountb->mnt_mountp, 
			mountb->mnt_special,
			statbuf->f_bsize, 
			statbuf->f_frsize);
		(void)pfmt(stdout, MM_NOSTD,
			":1471:%7Lu total blocks  %7Lu free blocks  %7Lu available\n", 
			statbuf->f_blocks*physblks, 
			statbuf->f_bfree*physblks,
			statbuf->f_bavail < statbuf->f_blocks ? statbuf->f_bavail*physblks : 0);
		(void)pfmt(stdout, MM_NOSTD,
			":1472:%7Ld total files   %7Ld free files   %7Ld available\n", 
			statbuf->f_files,
			statbuf->f_ffree,
			statbuf->f_favail);

		(void)pfmt(stdout, MM_NOSTD,
			":1376:%7lu filesys id %32s\n", statbuf->f_fsid,statbuf->f_fstr);
		(void)pfmt(stdout, MM_NOSTD,
			":1377:%7s fstype   0x%8.8lX flag       %7ld filename length\n\n", 
			statbuf->f_basetype,
			statbuf->f_flag,
			statbuf->f_namemax);

		break;

	case 't':
		(void)pfmt(stdout, MM_NOSTD,
			":1473:%-19s (%-19s):  %8Ld blocks%8Ld files\n", 
			mountb->mnt_mountp, 
			mountb->mnt_special,
			AvailBlocks, 
			statbuf->f_favail);
		(void)pfmt(stdout, MM_NOSTD,
			":1474:                                    total:  %8Ld blocks%8Ld files\n",
			TotalBlocks,
			statbuf->f_files);
		break;

	case 'b':
		if (eb_flg) {
			(void)pfmt(stdout, MM_NOSTD,
				":1475:%-19s (%-19s):  %8Ld kilobytes\n", 
				mountb->mnt_mountp, 
				mountb->mnt_special,
				(AvailBlocks/BLOCK_FRAG));
		}
		else {
			if (!header) {
				(void)pfmt(stdout, MM_NOSTD, ":1381:Filesystem                            avail\n");
				header++;
			}
			(void)pfmt(stdout, MM_NOSTD, ":1476:%-19s                   %-8Ld\n", 
				mountb->mnt_special,
				(AvailBlocks/BLOCK_FRAG));
		}
		break;

	case 'e':
		if (eb_flg) {
			(void)pfmt(stdout, MM_NOSTD,
				":1477:%-19s (%-19s):  %8Ld files\n", 
				mountb->mnt_mountp, 
				mountb->mnt_special,
				statbuf->f_favail);
		}
		else {
			if (!header) {
				(void)pfmt(stdout, MM_NOSTD, ":1384:Filesystem                ifree\n");
				header++;
			}
			(void)pfmt(stdout, MM_NOSTD,
				":1478:%-19s       %-8Ld\n", 
				mountb->mnt_special, statbuf->f_favail);
		}
		break;

	case 'n':
		(void)pfmt(stdout, MM_NOSTD,
			":273:%-19s: %-10s\n",
			mountb->mnt_mountp,
			statbuf->f_basetype);
		break;
	case 'k':
		if (!k_header) {
			(void)pfmt(stdout, MM_NOSTD,
				":274:filesystem          kbytes   used     avail    capacity  mounted on\n");
			k_header = 1;
		} 
		(void)pfmt(stdout, MM_NOSTD,
			":1479:%-19s %-8Lu %-8Lu %-8Lu %2Lu%%       %s\n",
			mountb->mnt_special,
			TotalBlocks/BLOCK_FRAG,
			UsedBlocks/BLOCK_FRAG,
			AvailBlocks/BLOCK_FRAG,
			Capacity,
			mountb->mnt_mountp);
		break;

        default:	
		(void)pfmt(stdout, MM_NOSTD,
			":1480:%-19s (%-19s):%8Ld blocks%8Ld files\n", 
			mountb->mnt_mountp, 
			mountb->mnt_special,
			AvailBlocks,
			statbuf->f_favail);
		break;
	}
}

static void
build_path(FSType, path)
char *FSType;
char *path;
{

	(void)strcpy(path, "/usr/lib/fs/");
	(void)strcat(path, FSType );
	(void)strcat(path, "/df");
}


/*
 * Procedure:     exec_specific
 *
 * Restrictions:
                 pfmt: None
                 strerror: None
                 execvp(2): P_MACREAD
*/
static void
exec_specific(FSType)
char *FSType;
{
int  pid,c_ret;

	switch(pid = fork()) {
	case (pid_t)-1:
		(void)pfmt(stderr, MM_ERROR, ":277:fork() failed: %s\n",
			strerror(errno));
		exit(2);
		/*NOTREACHED*/
	case 0:	
		if (mac_install && level)
			(void)procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);
		if (execvp(path, argp) == -1) {
			if (errno == EACCES) {
				(void)pfmt(stderr, MM_ERROR, ":278:Cannot execute %s: %s\n",
					path, strerror(errno));
				exit(2);
			}
			(void)pfmt(stderr, MM_ERROR, ":279:Operation not applicable for FSType %s\n", FSType);
			exit(2);
		}
		exit(2);
		
	default:
		if (wait(&status) == pid) {
			if ((c_ret=WHIBYTE(status)) != 0){
				exit(c_ret);
			}
		}
	} 
}

/* code used by devnm */
static char *
basename(s)
char *s;
{
	int n = 0;

	while(*s++ != '\0') n++;
	while(n-- > 0)
		if(*(--s) == '/') 
			return(++s);
	return(--s);
}

static struct dirent *dbufp;

/*
 * Procedure:     devnm
 *
 * Restrictions:
                 opendir: none
                 chdir(2): none
								 getcwd: none
*/
static char *
devnm()
{
	int i,j;
	static dev_t fno;
	static struct devs {
		char devdir[DEVLEN];
		DIR *dfd;
	} devd[DEVNM_MAX];
	static int devnm_cnt = 0;
	FILE *devnm_fd;

	static char devnam[DEVLEN];
	static char cwd[MAXPATHLEN];

	devnam[0] = '\0';

	/*
	** read in the DEVNM file if we never read it before	
	*/
	if(!devnm_cnt) {
		if((devnm_fd = fopen(DEVNM, "r")) == (FILE *)NULL ) {
			(void)pfmt(stderr, MM_ERROR, badopen, DEVNM, strerror(errno));
			exit(2);
		}

		while(fgets(devd[devnm_cnt].devdir, DEVLEN, devnm_fd) != NULL) {
			/* get rid of new-line */
			devd[devnm_cnt].devdir[strlen(devd[devnm_cnt].devdir) - 1] = '\0';
			devd[devnm_cnt].dfd = opendir(devd[devnm_cnt].devdir);
			if(devnm_cnt++ >= DEVNM_MAX)
				break;
		}
	}
	
	fno = S.st_dev;

	(void)getcwd(cwd, MAXPATHLEN - 1);
	for(i = 0; i < DEVNM_MAX; i++) {
		   j=chdir(devd[i].devdir);
		   if ((j == 0) && (dsearch(devd[i].dfd,fno))) {
			(void)strcpy(devnam, devd[i].devdir);
			(void)strcat(devnam,"/");
			(void)strncat(devnam,dbufp->d_name,MAXNAMELEN);
			(void)chdir(cwd);
			return(devnam);
		}
	}
	(void)chdir(cwd);
	return(devnam);

}


/*
 * Procedure:     dsearch
 *
 * Restrictions:
                 stat(2): none
                 pfmt: none
                 strerror: none
*/
static int
dsearch(ddir,fno)
DIR *ddir;
dev_t fno;
{
	int i;

	rewinddir(ddir);
	while((dbufp=readdir(ddir)) != (struct dirent *)NULL) {
		if(!dbufp->d_ino) continue;
		i=stat(dbufp->d_name, &S);
		if(i == -1) {
			(void)pfmt(stderr, MM_ERROR, badstat,dbufp->d_name,strerror(errno));
			return(0);
		}
		if((fno != S.st_rdev) 
		|| ((S.st_mode & S_IFMT) != S_IFBLK)
		|| (strcmp(dbufp->d_name,"swap") == 0)
		|| (strcmp(dbufp->d_name,"pipe") == 0)
			) continue;
		return(1);
	}
	return(0);
}
static int
is_remote(fstype)
	char	*fstype;
{
	if(strcmp(fstype, "rfs") == 0) 
		return 1;
	if(strcmp(fstype, "nfs") == 0) 
		return 1;
	if(strcmp(fstype, "nucfs") == 0) 
		return 1;
	if(strcmp(fstype, "nucam") == 0) 
		return 1;
	return 0;
}
