#ident	"@(#)pdi.cmds:diskcfg.c	1.27.3.1"
#ident	"$Header$"

/*
 * Disk driver configuration utility
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1990 INTERACTIVE Systems Corporation
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


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/vtoc.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include "scsicomm.h"
#include "diskcfg.h"

#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include <sys/p_sysi86.h>

#include <locale.h>

#define DCFG_MODE	0644
#define DCFG_OWN	0	/* root */
#define DCFG_GRP	3	/* sys  */

#define WR_SCSI		0
#define WR_NON		1

char	rmbuf[256];

static char	*progname,
		*root;
static char temp_dir[L_tmpnam];
static char iformat[50];

extern int	opterr,
			optind;
extern char	*optarg;

extern int Debug = FALSE;

extern void		error();
static struct diskdesc	*read_input();
static void		read_Config();
static void		strip_quotes();
static void		set_defaults();
static void		get_values();
static void		read_Master();
static int		make_System_loadable();
static void		build_System_files();
static void		update_configuration();
static void		update_resmgr();

char *p_index[]= {"1st", "2nd", "3rd", "4th"};
char *p_byte[]= {"0", "1", "2", "3", "4"};

main(argc, argv)
int	argc;
char	*argv[];
{
	struct diskdesc	*adapters;		/* Chain of adapter descriptions */
	int	arg;
	char	*label;

	opterr = 0;	/* turn off getopt error messages */

	progname = argv[0];

	root = NULL;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxdisksetup");
	label = (char *) malloc(strlen(progname)+1+3);
	sprintf(label, "UX:%s", progname);
	(void) setlabel(label);
	
	while ((arg = getopt(argc,argv,"SR:")) != EOF) {

		switch (arg) {
		case 'S' : /* Turn on Debug messages */
			Debug = TRUE;
			break;
		case 'R' : /* set the root variable */
			root = strdup(optarg);
			break;
		case '?' : /* Incorrect argument found */
			error(":427:usage: %s [-S] [-R root] [inputfile]\n", progname);
			break;
		}
	}

/*
 *	If there is an arguement left, make it the input file
 *	instead of stdin.
 */
	if (optind < argc)
		(void)freopen(argv[optind], "r", stdin);

	if ((adapters = read_input()) == NULL)
		return (NORMEXIT);

	/*
	 * Check if user have sufficient privilege to execute the idinstall
	 * program
	 */
	if ( access(IDINSTALL, X_OK) ) {
		if ( Debug )
			fprintf(stderr,"Errno set by access(IDINSTALL, X_OK) is %d.\n",errno);
		switch(errno) {
			case EPERM:
			case EACCES:
				error(":428:You do not have sufficient privilege to use a required program -- %s.\n", IDINSTALL);
			default:
				error(":429:Cannot access a required program -- %s\n", IDINSTALL);
		}
	}

	read_Master(adapters);

	read_Config(adapters);

	make_System_loadable(adapters);

	build_System_files(adapters);

	update_configuration(adapters);

	return (NORMEXIT);
}

static struct diskdesc *
read_input()
{
	char		*ptr;			/* String pointer */
	unsigned short	lineno;			/* Input line number */
	int		retval;			/* Return value from scanf */
	struct diskdesc	*adapters,		/* Chain of adapter descriptions */
			*diskdesc,		/* Current description */
			*tmpdesc;		/* Description for searching */
	struct diskdesc	**nextlink;		/* Next link in adapter chain */
	char ibuf[256];

	adapters = NULL;
	nextlink = &adapters;
	lineno = 1;

	for (;;) {
		if ((diskdesc = (struct diskdesc *) malloc(sizeof(struct diskdesc))) == NULL)
			error(":430:Insufficient memory\n");

		diskdesc->next = NULL;
		diskdesc->occurence = 1;

		retval = scanf("%8s \"%64[^\"]\" %4s %1s %d %lx %hu %hu %hu %hu %hu %lx %lx %lx %lx",
			       diskdesc->name, diskdesc->fullname,
			       diskdesc->type, diskdesc->configure,
			       &diskdesc->unit, &diskdesc->equip,
			       &diskdesc->dma1, &diskdesc->dma2,
			       &diskdesc->ipl, &diskdesc->ivect,
			       &diskdesc->ishare,
			       &diskdesc->sioaddr, &diskdesc->eioaddr,
			       &diskdesc->smemaddr, &diskdesc->ememaddr);
		if (retval == EOF)
			return (adapters);

		if (retval != ARGSPERLINE)
			error(":431:Invalid field %d in line %u of input\n", retval+1, lineno);

		/*
		 *  pick up the optional input value for bind_cpu.
		 *  this is used to setup CPU_BIND in the rm database.
		 *  -1 indicates no value for CPU_BIND.
		 */
		diskdesc->bind_cpu = -1;
		retval = scanf("%[^\n]\n",ibuf);
		if (retval == 1)
			diskdesc->bind_cpu = atoi(ibuf);

		for (ptr = diskdesc->type; *ptr != '\0'; ptr++)
			if (isupper(*ptr))
				*ptr = toupper(*ptr);
		if (!DEVICE_IS_SCSI(diskdesc))
			error(":432:Unrecognized adapter type %s in line %u of input\n", diskdesc->type, lineno);

		if (strchr(VALIDCONF, diskdesc->configure[0]) == NULL)
			error(":433:Unrecognized configuration flag %c in line %u of input\n", diskdesc->configure[0], lineno);
		if (islower(diskdesc->configure[0]))
			diskdesc->configure[0] = toupper(diskdesc->configure[0]);

		if (diskdesc->ishare < MINISHARE || diskdesc->ishare > MAXISHARE)
			error(":434:Unrecognized interrupt sharing code %u in line %u of input\n", diskdesc->ishare, lineno);

		if (diskdesc->sioaddr > diskdesc->eioaddr)
			error(":435:Starting I/O address %x exceeds ending I/O address %x in line %u of input\n",
				diskdesc->sioaddr, diskdesc->eioaddr, lineno);

		if (diskdesc->smemaddr > diskdesc->ememaddr)
			error(":436:Starting memory address %x exceeds ending memory address %x in line %u of input\n",
				diskdesc->smemaddr, diskdesc->ememaddr, lineno);

		*nextlink = diskdesc;			/* Add to end of chain */
		nextlink = &diskdesc->next;		/* Point to next link */
		lineno++;
	}
}

/*
 * Extract prefix from the Master file for each adapter.
 */
static void
read_Master(adapters)
struct diskdesc	*adapters;
{
	FILE	*cfgfp;		/* pipe to idinstall */
	unsigned short	found;		/* Found previous mdevice match? */
	struct diskdesc	*diskdesc;	/* Description for searching */
	char	command[PATH_MAX+PATH_MAX+10],
			cmd_line[PATH_MAX+PATH_MAX+31],
			tname[MAXNAMELEN+1],
			ibuffer[BUFSIZ];	/* a temp input buffer */

	if (root == NULL) {
		sprintf(command,"%s -G -m",IDINSTALL);
	} else {
		sprintf(command,"%s -G -m -R %s/etc/conf",IDINSTALL,root);
	}

	for (diskdesc = adapters; diskdesc != NULL; diskdesc = diskdesc->next) {

		sprintf(cmd_line,"%s %s 2>/dev/null",command,diskdesc->name);

		if ((cfgfp = popen(cmd_line, "r")) == NULL) 
			error(":437:Cannot read Master entry for module %s.\n", diskdesc->name);

		found = 0;
		do
			if (fgets(ibuffer,BUFSIZ,cfgfp) != NULL) {
				if (sscanf(ibuffer, "%14s %8s", tname, diskdesc->prefix) == 2)
					if (EQUAL(diskdesc->name, tname)) {
						found++;
					}
			}
		while (!found && !feof(cfgfp) && !ferror(cfgfp));

		if (!found)
			error(":438:No Master entry for module %s.\n", diskdesc->name);

		if (pclose(cfgfp) == -1)
			error(":439:Problem closing pipe to %s.\n", cmd_line);

	}
}

/*
 * Extract configuration information from the disk.cfg file for each adapter.
 *
 * we only bother to do this for the devices we are configuring in.
 */
static void
read_Config(adapters)
struct diskdesc	*adapters;
{
	FILE	*cfgfp;		/* fp to disk.cfg file */
	struct diskdesc	*diskdesc;	/* Description for searching */
	char	basename[PATH_MAX+sizeof(PACKD)+1],
			cfgname[PATH_MAX+sizeof(PACKD)+MAXNAMELEN+sizeof(CFGNAME)+3],
			tname[MAXNAMELEN+1],
			ibuffer[BUFSIZ];	/* a temp input buffer */


	if (root == NULL)
		(void) sprintf(basename, "%s", PACKD);
	else
		(void) sprintf(basename, "%s%s", root, PACKD);

	(void)sprintf(iformat, "%%%d[^=]=%%%d[^\n]", MAX_FIELD_NAME_LEN, BUFSIZ-1);

	for (diskdesc = adapters; diskdesc != NULL; diskdesc = diskdesc->next) {

		if (!CONFIGURE(diskdesc))
			continue;

		sprintf(cfgname,"%s/%s/%s",basename,diskdesc->name,CFGNAME);
		if ((cfgfp = fopen(cfgname, "r")) == NULL) 
			error(":440:Cannot open file %s for reading.\n", cfgname);

		set_defaults(diskdesc);

		get_values(diskdesc, cfgfp);

		if (fclose(cfgfp) == -1)
			error(":441:Problem closing file %s\n", cfgname);

	}
}

static void
set_defaults(diskdesc)
struct diskdesc *diskdesc;
{
	diskdesc->devtype = strdup(D_DEVTYPE);
	diskdesc->caps = strdup(D_CAPS);
	diskdesc->memaddr2 = ((D_MEMADDR2 == 0) ? 0 : diskdesc->smemaddr + D_MEMADDR2);
	diskdesc->ioaddr2 = ((D_IOADDR2 == 0) ? 0 : diskdesc->sioaddr + D_IOADDR2);
	diskdesc->maxsec = D_MAXSEC;
	diskdesc->drives = D_DRIVES;
	diskdesc->delay = D_DELAY;
	diskdesc->baseminor = ((diskdesc->occurence == 1) ? D_BASEMINOR : 0);
	diskdesc->defsecsiz = D_DEFSECSIZ;
	diskdesc->hbaid = D_HBAID;
	diskdesc->maxxfer = D_MAXXFER;
}

static void
get_values(diskdesc, cfgfp)
struct diskdesc *diskdesc;
FILE *cfgfp;
{
	int		field_number();
	long 	tlong;
	char	ibuffer[BUFSIZ],
			tname[MAX_FIELD_NAME_LEN+1],
			rbuffer[BUFSIZ],
			tbuffer[BUFSIZ];

	do
		if (fgets(ibuffer,BUFSIZ,cfgfp) != NULL) {
			if (sscanf(ibuffer, iformat, tname, rbuffer) == 2) {
				strip_quotes(rbuffer,tbuffer);
				switch (field_number(tname)) {
					case DEVTYPE:
						diskdesc->devtype = strdup(tbuffer);
						if (Debug)
							(void)fprintf(stderr,"DEVTYPE is %s for %s\n", diskdesc->devtype, diskdesc->name);
						break;
					case CAPS:
						diskdesc->caps = strdup(tbuffer);
						if (Debug)
							(void)fprintf(stderr,"CAPS is %s for %s\n", diskdesc->caps, diskdesc->name);
						break;
					case MEMADDR2:
						tlong = strtol(tbuffer, (char**)NULL, 0);
						diskdesc->memaddr2 = ((tlong == 0) ? tlong : diskdesc->smemaddr + tlong);
						if (Debug)
							(void)fprintf(stderr,"MEMADDR2 is 0x%lx for %s\n", tlong, diskdesc->name);
						break;
					case IOADDR2:
						tlong = strtol(tbuffer, (char**)NULL, 0);
						diskdesc->ioaddr2 = ((tlong == 0) ? tlong : diskdesc->sioaddr + tlong);
						if (Debug)
							(void)fprintf(stderr,"IOADDR2 is 0x%lx for %s\n", tlong, diskdesc->name);
						break;
					case MAXXFER:
						diskdesc->maxxfer = strtoul(tbuffer, (char**)NULL, 0);
						if (Debug)
							(void)fprintf(stderr,"MAXXFER is 0x%lx for %s\n", diskdesc->maxxfer, diskdesc->name);
						break;
					case MAXSEC:
						diskdesc->maxsec = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
						if (Debug)
							(void)fprintf(stderr,"MAXSEC is %hu for %s\n", diskdesc->maxsec, diskdesc->name);
						break;
					case DRIVES:
						diskdesc->drives = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
						if (Debug)
							(void)fprintf(stderr,"DRIVES is %hu for %s\n", diskdesc->drives, diskdesc->name);
						break;
					case DELAY:
						diskdesc->delay = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
						if (Debug)
							(void)fprintf(stderr,"DELAY is %hu for %s\n", diskdesc->delay, diskdesc->name);
						break;
					case BASEMINOR:
						if (diskdesc->occurence == 1)
							diskdesc->baseminor = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
						else
							diskdesc->baseminor = 0;
						if (Debug)
							(void)fprintf(stderr,"BASEMINOR is %hu for %s\n", diskdesc->baseminor, diskdesc->name);
						break;
					case DEFSECSIZ:
						diskdesc->defsecsiz = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
						if (Debug)
							(void)fprintf(stderr,"DEFSECSIZ is %hu for %s\n", diskdesc->defsecsiz, diskdesc->name);
						break;
					case HBAID:
						diskdesc->hbaid = (unsigned short)strtol(tbuffer, (char**)NULL, 0);
						if (Debug)
							(void)fprintf(stderr,"HBAID is %hu for %s\n", diskdesc->hbaid, diskdesc->name);
						break;
				}
			}
		}
	while (!feof(cfgfp) && !ferror(cfgfp));

	if (ferror(cfgfp))
		error(":442:Error occurred while reading %s file for module %s.\n", CFGNAME, diskdesc->name);

}

static void
strip_quotes(input,output)
char *input, *output;
{
	register char *i, *j;

	for (i=input, j=output; i[0]; i++) {
		if ( i[0] != '"' ) {
			j[0] = i[0];
			j++;
		}
	}
	j[0] = '\0';
}

static int
field_number(name)
char *name;
{
	register int index;

	for (index = 0; index < NUM_FIELDS; index++)
		if (EQUAL(name,field_tbl[index].name))
			return((int)field_tbl[index].tag);

	return(-1);
}

static void
write_SCSI_System_line(diskdesc,sdevfp,sdevname)
struct diskdesc	*diskdesc;
FILE *sdevfp;
char *sdevname;
{
	if (fprintf(sdevfp, "%s\t%c\t%d\t%u\t%u\t%u\t%lx\t%lx\t%lx\t%lx\t%hd",
		    diskdesc->name, diskdesc->configure[0], diskdesc->unit,
			SDI_IPL, diskdesc->ishare, diskdesc->ivect,
	    	diskdesc->sioaddr, diskdesc->eioaddr,
	    	diskdesc->smemaddr, diskdesc->ememaddr,
			diskdesc->dma1 ? diskdesc->dma1 : -1 ) < 0)
		error(":449:Problem writing to file %s.\n", sdevname);
	if (diskdesc->bind_cpu != -1) {
		if (fprintf(sdevfp, "\t%d\n", diskdesc->bind_cpu) < 0)
			error(":449:Problem writing to file %s.\n", sdevname);
	} else {
		if (fprintf(sdevfp, "\n") < 0)
			error(":449:Problem writing to file %s.\n", sdevname);
	}
}

/*
 * Build all the System files.  We build them for each adapter.
 */
static void
build_System_files(adapters)
struct diskdesc	*adapters;
{
	char	sdevname[L_tmpnam+MAXNAMELEN+2+sizeof(SYSTEM)];
	int		temp, written_static_yet, first_time;
	int	rv;
	struct diskdesc *diskdesc;
	FILE		*sdevfp;

	if (mkdir(tmpnam(temp_dir), S_IRWXU) != 0)
		error(":450:Cannot create temporary directory %s.\n", temp_dir);

	if (rv = RMopen(O_RDWR))
		error("%s: RMopen() failed, errno=%s\n", progname, rv);

	for (diskdesc=adapters; diskdesc != NULL; diskdesc = diskdesc->next) {


		(void) sprintf(sdevname, "%s/%s", temp_dir, diskdesc->name);

		if (access(sdevname,F_OK) != 0) {
			first_time = TRUE;
			diskdesc->Install_path = strdup(sdevname);
			if (mkdir(sdevname, S_IRWXU) != 0)
				error(":450:Cannot create temporary directory %s.\n", sdevname);
		} else {
			first_time = FALSE;
			diskdesc->Install_path = NULL;
		}

		(void) sprintf(sdevname, "%s/%s/%s", temp_dir, diskdesc->name, SYSTEM);
		if ((sdevfp = fopen(sdevname, "a")) == NULL) 
			error(":452:Cannot open file %s for append.\n", sdevname);

		if ( first_time ) {
			if (fprintf(sdevfp, "$version 2\n") < 0)
				error(":449:Problem writing to file %s.\n", sdevname);
		}

		if (first_time && !diskdesc->loadable) {
			if (fprintf(sdevfp,"$static\n") < 0)
				error(":449:Problem writing to file %s.\n", sdevname);
		}

		if (DEVICE_IS_SCSI(diskdesc)) {
			write_SCSI_System_line(diskdesc,sdevfp,sdevname);
			update_resmgr(WR_SCSI,diskdesc);
		}

		if (fclose(sdevfp) != 0)
			error(":441:Problem closing file %s.\n", sdevname);

	}

	RMclose();

	sprintf(rmbuf,"/etc/conf/bin/idconfupdate 1>/tmp/mse.log 2>&1");
	if (system(rmbuf) == -1) 
		error(":453:Error %d trying to exec %s.\n", errno, rmbuf);

}

/*
 * Make all the decisions about loadable System files.
 *
 * We do this for each adapter.
 */
static int
make_System_loadable(adapters)
struct diskdesc	*adapters;
{
        struct diskdesc *diskdesc;
        char *not_loadable_name = NULL;

        for (diskdesc=adapters; diskdesc != NULL; diskdesc = diskdesc->next) {
                if (CONFIGURE(diskdesc)) {
                        if (diskdesc->unit == 0) {
                                diskdesc->loadable = FALSE;
                                not_loadable_name = diskdesc->name;
                        } else {
                                diskdesc->loadable = TRUE;
                        }
                } else {
                        diskdesc->loadable = TRUE;  /* ul92-09927 */
                }
        }

        if (not_loadable_name) {
                for (diskdesc=adapters;
                     diskdesc != NULL;
                     diskdesc = diskdesc->next) {
                        if (!strcmp(not_loadable_name, diskdesc->name))
                                diskdesc->loadable = FALSE;

                }
        }
}

/*
 * Update configuration for each adapter
 */
static void
update_configuration(adapters)
struct diskdesc	*adapters;
{
	int	idretval;
	struct diskdesc	*diskdesc;
	char	command[PATH_MAX+PATH_MAX+26],
			cmd_line[PATH_MAX+PATH_MAX+PATH_MAX+42];

	if (root == NULL) {
		sprintf(command,"%s -u -e",IDINSTALL);
	} else {
		sprintf(command,"%s -u -e -R %s/etc/conf",IDINSTALL,root);
	}

	for (diskdesc = adapters; diskdesc != NULL; diskdesc = diskdesc->next) {
		if ( diskdesc->Install_path == NULL )
			continue;

		if ( Debug ) {
			sprintf(cmd_line,"cd %s;%s %s",
				diskdesc->Install_path,command,diskdesc->name);
		} else {
			sprintf(cmd_line,"cd %s;%s %s 2>/dev/null",
				diskdesc->Install_path,command,diskdesc->name);
		}

		idretval = system(cmd_line);

		if ( idretval == -1 )
			error(":455:Cannot fork shell to execute %s.\n", cmd_line);

		if ( WEXITSTATUS(idretval) != 0 )
			error(":456:Unable to update configuration for %s.\n", diskdesc->name);

		(void) rmdir(diskdesc->Install_path);
	}
	(void) rmdir(temp_dir);
}

#define	MAX_NAME_LEN	20

static void
update_resmgr(func, diskdesc)
int func;
struct diskdesc	*diskdesc;
{
	static	char	modname[MAX_NAME_LEN] = "";
	static	int	r_inst;

	int ret, errno;
	int newkey = 0;
	rm_key_t key;
	
	if (strcmp(diskdesc->name, modname) != 0)	{
		strncpy(modname, diskdesc->name, MAX_NAME_LEN);
		r_inst = 0;
	}

	(void)RMbegin_trans(0, RM_READ);
	errno = RMgetbrdkey(diskdesc->name, r_inst, &key);
	(void)RMend_trans(0);
		
	if (strcmp(diskdesc->configure, "Y") == 0) {
		if (errno == ENOENT)	{
			RMnewkey(&key); /* RMnewkey does implicit begin, not end*/
			RMend_trans(key); 
			newkey = 1;
		}

		/*
		 * BOOTHBA
		 */
		(void)RMbegin_trans(key, RM_RDWR);
		if (!newkey)
			(void)RMdelvals(key, CM_BOOTHBA);
		if (diskdesc->unit == 0) {
			sprintf(rmbuf, "%d", diskdesc->unit);
			if (RMputvals(key, CM_BOOTHBA, rmbuf))
				error(":458:RMputvals() %s failed\n", "CM_BOOTHBA");
		}

		/*
		 * UNIT
		 */
		if (!newkey && RMdelvals(key, CM_UNIT))
			error(":457:RMdelvals() %s failed\n", "CM_UNIT");
		sprintf(rmbuf, "%d", diskdesc->unit);
		if (RMputvals(key, CM_UNIT, rmbuf))
			error(":458:RMputvals() %s failed\n", "CM_UNIT");

		/*
		 * IPL
		 */
		if (!newkey && RMdelvals(key, CM_IPL))
			error(":457:RMdelvals() %s failed\n", "CM_IPL");
		sprintf(rmbuf, "%d", (func != WR_NON ? SDI_IPL : 0) );
		if (RMputvals(key, CM_IPL, rmbuf))
			error(":458:RMputvals() %s failed\n", "CM_IPL");

		/*
		 * ITYPE
		 */
		if (!newkey && RMdelvals(key, CM_ITYPE))
			error(":457:RMdelvals() %s failed\n", "CM_ITYPE");
		sprintf(rmbuf, "%d", (func != WR_NON ? diskdesc->ishare : 0) );
		if (RMputvals(key, CM_ITYPE, rmbuf))
			error(":458:RMputvals() %s failed\n", "CM_ITYPE");

		/*
		 * IRQ
		 */
		if (!newkey && RMdelvals(key, CM_IRQ))
			error(":457:RMdelvals() %s failed\n", "CM_IRQ");
		sprintf(rmbuf, "%u", (func != WR_NON ? diskdesc->ivect : 0) );
		if (RMputvals(key, CM_IRQ, rmbuf))
			error(":458:RMputvals() %s failed\n", "CM_IRQ");

		/*
		 * IOADDR
		 */
		if (!newkey && RMdelvals(key, CM_IOADDR))
			error(":457:RMdelvals() %s failed\n", "CM_IOADDR");
		sprintf(rmbuf, "%lx %lx ", diskdesc->sioaddr, diskdesc->eioaddr);
		if (RMputvals(key, CM_IOADDR, rmbuf))
			error(":458:RMputvals() %s failed\n", "CM_IOADDR");

		/*
		 * MEMADDR
		 */
		if (!newkey && RMdelvals(key, CM_MEMADDR))
			error(":457:RMdelvals() %s failed\n", "CM_MEMADDR");
		sprintf(rmbuf, "%x %x", diskdesc->smemaddr, diskdesc->ememaddr);
		if (RMputvals(key, CM_MEMADDR, rmbuf))
			error(":458:RMputvals() %s failed\n", "CM_MEMADDR");

		/*
		 * DMAC
		 */
		if (!newkey && RMdelvals(key, CM_DMAC))
			error(":457:RMdelvals() %s failed\n", "CM_DMAC");
		sprintf(rmbuf, "%hd", (diskdesc->dma1 ? diskdesc->dma1 : -1));
		if (RMputvals(key, CM_DMAC, rmbuf))
			error(":458:RMputvals() %s failed\n", "CM_DMAC");

		/*
		 * BINDCPU
		 */
		if (!newkey)
			RMdelvals(key, CM_BINDCPU);
		if (diskdesc->bind_cpu != -1) {
			sprintf(rmbuf, "%d", diskdesc->bind_cpu);
			if (RMputvals(key, CM_BINDCPU, rmbuf))
				error(":458:RMputvals() %s failed\n", "CM_BINDCPU");
		}

		if (newkey) {
			/*
			 * MODNAME
			 */
			sprintf(rmbuf, "%s", modname);
			if (RMputvals(key, CM_MODNAME, rmbuf))
				error(":458:RMputvals() %s failed\n", "CM_MODNAME");


			/*
			 * BRDBUSTYPE
			 */
			sprintf(rmbuf, "%s", "unk");
			if (RMputvals(key, CM_BRDBUSTYPE, rmbuf))
				error(":458:RMputvals() %s failed\n", "CM_BRDBUSTYPE");
			/*
			 * ENTRYTYPE
			 */
			sprintf(rmbuf, "%d", CM_ENTRY_DEFAULT);
			if (RMputvals(key, CM_ENTRYTYPE, rmbuf))
				error(":458:RMputvals() %s failed\n", "CM_ENTRYTYPE");
		}
		r_inst++;
	} else {
		(void) RMdelkey(key); /* RMdelkey does begin/delkey/end */
		return;
	}
	(void)RMend_trans(key);
}
