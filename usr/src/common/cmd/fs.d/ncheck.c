/*		copyright	"%c%" 	*/

#ident	"@(#)fs.cmds:common/cmd/fs.d/ncheck.c	1.5.11.9"
#ident  "$Header$"


#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/vfstab.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <priv.h>

#define MAX_OPTIONS	20	/* max command line options */

char *argp[MAX_OPTIONS];	/* command line for specific module*/
int argpc;
char path[BUFSIZ];

void	echo_cmdline();
void	mk_cmdline();

/*
 * Procedure:     main
 *
 * Notes:
 *
 * Generic NCHECK 
 *
 * This is the generic part of the ncheck code. It is used as a
 * switchout mechanism and in turn executes the file system
 * type specific code located at /usr/lib/fs/FSType. 
 */

main (argc, argv)
int argc;
char *argv[];

{
	int arg;			/* argument from getopt() */
	int i_flg;			/* current options */
	int F_flg, o_flg, V_flg, usgflg;/* generic flags */
	int i;

	extern char *optarg;		/* getopt(3c) specific */
	extern int optind;

	char *FSType = NULL;		/* FSType */
	char *oargs = NULL;		/* FSType specific argument */
	char *iargs = NULL;		/* arguments for option i */
	char *cmdname = NULL; 		/* command name or path */
	char *special;			/* Special device */
	char options[MAX_OPTIONS];	/* options for specific module */

	char *usage = "Usage:\n"
	"ncheck [-F FSType] [-V] [current_options] [-o specific_options] [special ...]\n"; 

	FILE *fp;
	struct vfstab	vfsbuf;

	i_flg =  0; 
	F_flg = V_flg = o_flg = usgflg =  0;
	cmdname = argv[0];	
	strcpy(options, "-");

	/* open VFSTAB */

	fp = fopen(VFSTAB, "r");

	if ( fp == NULL){
		fprintf(stderr, "%s: cannot open %s\n", cmdname, VFSTAB);
		exit(2);
	}

	/* Process the Options (if any) */ 
	while ((arg = getopt(argc,argv,"V?F:o:i:as")) != -1) {
		switch(arg) {
		case 'a':	/* allows printing of names . and ..*/
			strcat(options, "a");
			break;
		case 's':	/*limits report to specials & SetUID/GID files */
			strcat(options, "s");
			break;
		case 'V':	/* echo complete command line */
			V_flg = 1;
			break;
		case 'F':	/* FSType specified */
			if (F_flg) {
				fprintf(stderr, "%s: more than one FSType specified\n", cmdname);
				fprintf(stderr, usage);
				exit(2);
			}
			F_flg = 1;
			FSType = optarg;
			if (( i = strlen(FSType)) > 8 ) {
				fprintf(stderr, "FSType name %s exceeds 8 characters\n", FSType);
			}
			break;
		case 'o':	/* FSType specific arguments */
			o_flg = 1;
			oargs = optarg;
			break;
		case 'i':	/* limit report to files whose i-numbers follow */
			i_flg = 1;
			iargs = optarg;
			break;
		case '?':	/* print usage message */
			usgflg = 1;
			strcat(options, "?");
		}
	}
	if (usgflg) {
		if (F_flg) {
			mk_cmdline( options,
				o_flg,
				oargs,
				i_flg,
				iargs,
				argv[optind]);
			build_path(FSType, path);
			exec_specific(path);
   		} else {
			fprintf(stderr, usage);
		}
		exit(2);
	}


	/* 
 	* If there are no device to ncheck then the generic 
	* reads the VFSTAB and executes the specific module of
	* each entry which has a numeric fsckpass field.
 	*/

	if (optind == argc) {	
		while (( i = getvfsent(fp, &vfsbuf)) == 0) {
			if (vfsbuf.vfs_fsckpass == NULL)
				continue;
			if (!isnumber(vfsbuf.vfs_fsckpass))
				continue;
			if (vfsbuf.vfs_fstype == NULL)
				continue;
			if ((F_flg) && FSType && (strcmp(FSType, vfsbuf.vfs_fstype) != 0))
				continue;
			mk_cmdline( options,
					o_flg,
					oargs,
					i_flg,
					iargs,
					vfsbuf.vfs_fsckdev);
			build_path(vfsbuf.vfs_fstype, path);
			if (V_flg)  {
				echo_cmdline(argp, argpc, vfsbuf.vfs_fstype);
				continue;
			}
			exec_specific(path);
		}
		exit(0);
	}
	/* special provided */
	for (; optind < argc; optind++ ) {
		/* What can we find in vfstab ? */
		rewind(fp);
		while ((( i = getvfsent(fp, &vfsbuf)) == 0)  &&
		 	!(vfsbuf.vfs_special && 
		 	  strcmp(vfsbuf.vfs_special, argv[optind]) == 0) &&
		 	!(vfsbuf.vfs_fsckdev && 
			  strcmp(vfsbuf.vfs_fsckdev, argv[optind]) == 0) &&
		 	!(vfsbuf.vfs_mountp && 
		 	  strcmp(vfsbuf.vfs_mountp, argv[optind]) == 0))
				continue;
		if (i == 0) {		/* an entry matches */
			/* If F_flg set, use supplied FSType, though 
			   it might be bogus. Otherwise, use what came
			   from vfstab */
			if (!F_flg) 
				FSType = vfsbuf.vfs_fstype;
			special = vfsbuf.vfs_fsckdev;
		} else { /* No match. If F_flg not set, we're in trouble*/
			if (!F_flg) {
				fprintf(stderr, 
					"%s: FSType for %s cannot be identified\n",
					cmdname,argv[optind]);
				continue;
			}
			special = argv[optind];
		}
		mk_cmdline( options, o_flg, oargs, i_flg, iargs, special);
		build_path(FSType, path);
		if (V_flg) {
			echo_cmdline(argp, argpc, FSType);
			continue;
		}
		exec_specific(path);
	}

	exit(0);
}	/* end main */


/*
 * Procedure:     echo_cmdline
 *
 */

void	echo_cmdline(argp, argpc, fstype)
char *argp[];
int argpc;
char *fstype;
{
	int i;
	printf("ncheck ");
	if (fstype != NULL)
		printf("-F %s ", fstype);
	for( i= 1; i < argpc; i++) 
	        printf("%s ", argp[i]);
	printf("\n");

}


/*
 * Procedure:     mk_cmdline
 *
 *
 * Notes:
 *
 * function to generate command line to be passed to specific. 
 */

void	mk_cmdline(options, o_flg, oargs, i_flg, iargs, argument)
char *options;
int  o_flg;
char *oargs;
int i_flg;
char *iargs;
char *argument;
{


	argpc = 0;
	argp[argpc++] = "ncheck";
	if (strcmp(options, "-") != 0)
		argp[argpc++] = options;
	if (i_flg) {
		argp[argpc++] = "-i";
		argp[argpc++] = iargs;
	}
	if (o_flg) {
		argp[argpc++] = "-o";
		argp[argpc++] = oargs;
	}
	argp[argpc++] = argument;
	argp[argpc] = NULL;
}



/*
 * Procedure:     build_path
 *
*/

int
build_path(FSType, path)
char *FSType;
char *path;
{
	strcpy(path, "/usr/lib/fs/");
	strcat(path, FSType );
	strcat(path, "/ncheck");
 	return 0;	
}


/*
 * Procedure:     exec_specific
 *
 */

int
exec_specific(path)
char *path;
{
	int status;
	pid_t pid;
	int ret;

	switch(pid = fork()) {
	case -1:
		fprintf(stderr, "ncheck: cannot fork process\n");
		exit(2);
		break;

	case 0:	

		if ((execvp(path, argp)) == -1) {
			fprintf(stderr, "ncheck: cannot execute %s\n", path);
			/* This is not an error. Some FS have no ncheck */
			exit(0);
		}
		exit(2);
		break;
		
	default:
		if (wait(&status) == pid) {
			ret = WHIBYTE(status);
			if (ret > 0)
				exit(ret);
		}

	} 
}

isnumber(s)
char *s;
{
	register c;

	while(c = *s++)
		if(!isdigit(c))
			return(0);
	return(1);
}
