/*		copyright	"%c%" 	*/


#ident	"@(#)gcore:i386/cmd/gcore/gcore.c	1.1.11.1"
#ident  "$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
*/

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
**	Note: below, where we set the
**	Elf Header for the core file, are a couple of
**	Machine Specific lines of code, commented by
**		Machine Specific.
*/

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/user.h>
#include <sys/sysmacros.h>
#include <sys/procfs.h>
#include <sys/elf.h>
#include <sys/mman.h>
#include <sys/utsname.h>

#define	TRUE	1
#define	FALSE	0

/* Error returns from Pgrab() */
#define	G_NOPROC	(-1)	/* No such process */
#define	G_ZOMB		(-2)	/* Zombie process */
#define	G_PERM		(-3)	/* No permission */
#define	G_BUSY		(-4)	/* Another process has control */
#define	G_SYS		(-5)	/* System process */
#define	G_SELF		(-6)	/* Process is self */
#define	G_STRANGE	(-7)	/* Unanticipated error, perror() was called */
#define	G_INTR		(-8)	/* Interrupt received while grabbing */


typedef struct {
	Elf32_Word namesz;
	Elf32_Word descsz;
	Elf32_Word type;
	char	   name[8];
} Elf32_Note;

#define CF_T_PRSTATUS	10
#define CF_T_FPREG	12
#define CF_T_PRPSINFO	13
#define CF_T_PRCRED	14
#define CF_T_UTSNAME	15
#define CF_T_LWPSTATUS	16
#define CF_T_LWPSINFO	17

static	void	alrm();
static	pid_t	getproc();
static	int	dumpcore();
static	int	grabit();
static	int	elfnote(int, void *, int);
static	int	isprocdir();
static	int	Pgrab();
static	int	Ioctl();
static	int	openprocfile();

static char *	command = NULL;		/* name of command ("gcore") */
static char *	filename = "core";	/* default filename prefix */
static char *	procdir = "/proc";	/* default PROC directory */
static int	timeout = FALSE;	/* set TRUE by SIGALRM catcher */
static long	buf[4096];		/* big multi-purpose buffer */
static int	readsize;		/* size for reading address space */
static pstatus_t prstat;		/* prstatus info */
static int	openerr;		/* error code from openprocfile() */

static char	mapname[PATH_MAX];
static char	asname[PATH_MAX];
static char	ctlname[PATH_MAX];
static char	statusname[PATH_MAX];

/* file descriptors for callees of main; main cleans these up */
static int	fds[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
#define as_fd		(fds[0])
#define ctl_fd		(fds[1])
#define lwpinfo_fd	(fds[2])
#define lwpstat_fd	(fds[3])
#define map_fd		(fds[4])
#define prcred_fd	(fds[5])
#define prstat_fd	(fds[6])
#define psinfo_fd	(fds[7])
#define status_fd	(fds[8])
#define core_fd		(fds[9])

main(argc, argv)
	int argc;
	char **argv;
{
	int opt;
	int errflg = 0;

	command = argv[0];

	while ((opt = getopt(argc, argv, "o:p:")) != EOF) {
		switch (opt) {
		case 'o':		/* filename prefix (default "core") */
			filename = optarg;
			break;
		case 'p':		/* alternate /proc directory */
			procdir = optarg;
			break;
		default:
			errflg = 1;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (errflg || argc <= 0) {
		(void) fprintf(stderr,
			"usage: %s [-o filename] [-p procdir] pid ...\n",
			command);
		exit(2);
	}

	if (!isprocdir(procdir)) {
		(void) fprintf(stderr,
			"%s: %s is not a PROC directory\n",
			command, procdir);
		exit(2);
	}

	/* catch alarms */
	(void) sigset(SIGALRM, alrm);

	/*
	 * Determine an appropriate size limit for reading process address
	 * space.  We prefer to read page-aligned, page-sized chunks so
	 * that an attempt to read a segment that extends beyond the end
	 * of the underlying mapped object will get as much data as
	 * possible before terminating with a read error.
	 */
	if ((readsize = PAGESIZE) > sizeof buf)
		readsize = sizeof buf;

	/*
	 * Loop through processid arguments.
	 */
	while (--argc >= 0) {
		int i;
		pid_t pid;
		char *pdir;

		/*
		 * Close all of our callees' file descriptors.
		 * Doing it here simplifies their logic.
		 */
		for (i = 0; i < sizeof fds / sizeof fds[0]; i++) {
			if (fds[i] >= 0) {
				(void) close(fds[i]);
				fds[i] = -1;
			}
		}

		if ((pid = getproc(*argv++, &pdir)) < 0
			|| grabit(pdir, pid) < 0
			|| dumpcore(pdir, pid) != 0) {
			++errflg;
		}
	}

	exit (errflg != 0);
	/* NOTREACHED */
}

static pid_t		/* get process id and /proc directory */
getproc(path, pdirp)	/* return pid on success, -1 on failure */
	register char * path;	/* number or /proc/nnn */
	char ** pdirp;		/* points to /proc directory on success */
{
	register char * name;
	register pid_t pid;
	char *next;

	if ((name = strrchr(path, '/')) != NULL)	/* last component */
		*name++ = '\0';
	else {
		name = path;
		path = procdir;
	}

	pid = strtol(name, &next, 10);
	if (isdigit(*name) && pid >= 0 && *next == '\0') {
		if (strcmp(procdir, path) != 0
		 && !isprocdir(path)) {
			(void) fprintf(stderr,
				"%s: %s is not a PROC directory\n",
				command, path);
			pid = -1;
		}
	} else {
		(void) fprintf(stderr, "%s: invalid process id: %s\n",
			command, name);
		pid = -1;
	}

	if (pid >= 0)
		*pdirp = path;
	return pid;
}

static int
grabit(dir, pid)		/* take control of an existing process */
	char * dir;
	pid_t pid;
{
	int gcode;

	gcode = Pgrab(dir, pid);

	if (gcode >= 0)
		return gcode;
	
	if (gcode == G_INTR)
		return -1;

	(void) fprintf(stderr, "%s: %s.%d not dumped", command, filename, pid);
	switch (gcode) {
	case G_NOPROC:
		(void) fprintf(stderr, ": %d: No such process", pid);
		break;
	case G_ZOMB:
		(void) fprintf(stderr, ": %d: Zombie process", pid);
		break;
	case G_PERM:
		(void) fprintf(stderr, ": %d: Permission denied", pid);
		break;
	case G_BUSY:
		(void) fprintf(stderr, ": %d: Process is traced", pid);
		break;
	case G_SYS:
		(void) fprintf(stderr, ": %d: System process", pid);
		break;
	case G_SELF:
		(void) fprintf(stderr, ": %d: Cannot dump self", pid);
		break;
	}
	(void) fputc('\n', stderr);

	return -1;
}

/*ARGSUSED*/
static void
alrm(sig)
	int sig;
{
	timeout = TRUE;
}

	
/* 
 * 
 *      Following is the format of the core file that will be dumped:
 *
 *
 *      *********************************************************
 *      *                                                       *
 *      *               Elf header                              *
 *      *********************************************************
 *      *                                                       *
 *      *                                                       *
 *      *               Program header:                         *
 *      *                                                       *
 *      *                       One entry per note section.     *
 *      *                                                       *
 *      *                       One entry for each region of    *
 *      *                       memory in the address space     *
 *      *                       with different permissions.     *
 *      *                                                       *
 *      *********************************************************
 *      *                                                       *
 *      *               Note sections:                          *
 *      *                                                       *
 *      *                       For a process with N LWPs       *
 *      *                       there will be N+1 note          *
 *      *                       sections (a note section per    *
 *      *                       LWP and a process-wide note     *
 *      *                       section).                       *
 *      *                                                       *
 *      *********************************************************
 *      *                                                       *
 *      *               Dump of the address space.              *
 *      *                                                       *
 *      *********************************************************
 *
 * Reads the address space from as_fd, which must be opened by caller.
 * May leave one or more global fds open, to be closed by caller.
 */
static int
dumpcore(pdir, pid)
	char *pdir;		/* proc directory */
	pid_t pid;		/* process-id */
{
	int nsegments;		/* current number of segments */
	Elf32_Ehdr ehdr;	/* ELF header */
	Elf32_Phdr *v;		/* ELF program header */
	psinfo_t psstat;
	prcred_t prcred;
	struct utsname u_name;
	lwpsinfo_t lwpsinfo;
	lwpstatus_t lwpstat;
	ulong hdrsz;
	off_t poffset;
	int nhdrs;
	int i;
	int size, count, ncount;
	char cname[PATH_MAX],prstatname[PATH_MAX],prcredname[PATH_MAX];
	char psinfoname[PATH_MAX],lwpstatname[PATH_MAX],lwpinfoname[PATH_MAX];
	struct stat statbuf;
	char * bp = (char *)&buf[0];	/* pointer to big buffer */
	prmap_t *pdp = (prmap_t *)bp;
	int ret, index = 0, lwp_i;
	int partial = 0;

	/*
	 * Fetch the memory map and look for text, data, and stack.
	 */
	(void) sprintf(mapname, "%s/%d/map", pdir, pid); 
	if (stat(mapname, &statbuf) == -1) {
		(void) fprintf(stderr, "stat of /proc map file %s failed, errno %d\n",
			mapname, errno);
		return -1;
	}

	if ((nsegments = (statbuf.st_size/sizeof(prmap_t))) <= 0) {
		(void) fprintf(stderr,"dumpcore(): file %s contains no segments\n",
			mapname);
		return -1;
	}

	if (nsegments >= (sizeof(buf)/sizeof(prmap_t))) {
		(void) fprintf(stderr, "dumpcore(): too many segments\n");
		return -1;
	}

	if ((map_fd = openprocfile(mapname)) < 0)
		return openerr;

	for (i=0; i<nsegments; i++, pdp++)
		if ((ret = read(map_fd,pdp,sizeof(prmap_t))) != sizeof(prmap_t)){
			(void) fprintf(stderr, "dumpcore(): read of map file failed: read return %d\n", ret);
			return -1;
		}

	pdp = (prmap_t *) &buf[0]; /* reset pointer to prmap_t area */

	(void) sprintf(psinfoname, "%s/%d/psinfo", pdir, pid); 
	if ((psinfo_fd = openprocfile(psinfoname)) < 0)
		return openerr;

	if ((ret = read(psinfo_fd, &psstat, sizeof(psinfo_t))) != sizeof(psinfo_t)) {
		(void) fprintf(stderr, "dumpcore(): read of psinfo file %s failed\n",
			psinfoname);
		return -1;
	}

	/*
	 *  The total number of note sections we will need will be one more
	 *  than the number of LWP's in the process (one note section for
	 *  process-wide info and one note section per LWP).  Therefore,
	 *  the number of entries in the program header is the total number
	 *  of regions of memory that have different protections in the
	 *  address space plus the total number of note sections.
	 */
	nhdrs = nsegments + psstat.pr_nlwp + 1;
	hdrsz = nhdrs * sizeof(Elf32_Phdr);

	v = (Elf32_Phdr *)calloc(nhdrs, sizeof(Elf32_Phdr));

	(void) memset(&ehdr, 0, sizeof(Elf32_Ehdr));
	ehdr.e_ident[EI_MAG0] = ELFMAG0;
	ehdr.e_ident[EI_MAG1] = ELFMAG1;
	ehdr.e_ident[EI_MAG2] = ELFMAG2;
	ehdr.e_ident[EI_MAG3] = ELFMAG3;
	ehdr.e_ident[EI_CLASS] = ELFCLASS32;
	ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr.e_ident[EI_VERSION] = EV_CURRENT;

	ehdr.e_type = ET_CORE;		/* Machine Specific */
	ehdr.e_machine = EM_386;	/* Machine Specific */

        ehdr.e_version = EV_CURRENT;
        ehdr.e_phoff = sizeof(Elf32_Ehdr);
        ehdr.e_ehsize = sizeof(Elf32_Ehdr);
        ehdr.e_phentsize = sizeof(Elf32_Phdr);
        ehdr.e_phnum = (Elf32_Half)nhdrs;

	/*
	 * Create the core dump file.
	 */
	(void) sprintf(cname, "%s.%d", filename, pid);
	if ((core_fd = creat(cname, 0666)) < 0) {
		perror(cname);
		return -1;
	}

	/*
	 * Write out elf header.
	 */
	if (write(core_fd, &ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
		perror("write");
		return -1;
	}	

	/*
	 * Initialize the entry for process-wide note section.
	 */
	poffset = sizeof(Elf32_Ehdr) + hdrsz;
	v[index].p_type = PT_NOTE;
        v[index].p_flags = PF_R;
        v[index].p_offset = poffset;
        v[index].p_filesz = (sizeof(Elf32_Note) * 4) + 
		roundup(sizeof(pstatus_t), sizeof(Elf32_Word)) +
		roundup(sizeof(psinfo_t), sizeof(Elf32_Word)) +
		roundup(sizeof(prcred_t), sizeof(Elf32_Word)) +
		roundup(sizeof(struct utsname), sizeof(Elf32_Word));
	poffset += v[index].p_filesz;
        index++;

	/*
	 * Initialize the program header entries for the per-lwp note sections.
	 */
	for (lwp_i = 0; lwp_i < psstat.pr_nlwp; lwp_i++) {
                v[index].p_type = PT_NOTE;
                v[index].p_flags = PF_R;
                v[index].p_offset = poffset;
                v[index].p_filesz =
                        (sizeof (Elf32_Note) * 2) +
                         roundup(sizeof (lwpstatus_t), sizeof (Elf32_Word)) +
                         roundup(sizeof (lwpsinfo_t), sizeof (Elf32_Word));
                poffset += v[index].p_filesz;
                index++;
	}

	/*
	 * Initialize the program header information.
	 */
	for (i = index; i < nhdrs; i++, pdp++) {
		vaddr_t naddr;		
		v[i].p_type = PT_LOAD;
		v[i].p_vaddr = (Elf32_Word) pdp->pr_vaddr;
		naddr = pdp->pr_vaddr;
		while ((naddr += PAGESIZE) < (pdp->pr_vaddr + pdp->pr_size))
		           ;
		size = naddr - pdp->pr_vaddr;
		v[i].p_memsz = size;
		if (pdp->pr_mflags & MA_WRITE)
			v[i].p_flags |= PF_W;
		if (pdp->pr_mflags & MA_READ)
			v[i].p_flags |= PF_R;
		if (pdp->pr_mflags & MA_EXEC)
			v[i].p_flags |= PF_X;
		if ((pdp->pr_mflags & (MA_WRITE|MA_EXEC)) != MA_EXEC) {
			v[i].p_offset = poffset;
			v[i].p_filesz = size;
			poffset += size;
		}	
	}

	/*
	 * Write out program header.
	 */
	if (write(core_fd, (char *)v, hdrsz) != hdrsz) {
		perror("write");
		return -1;
	}	

	(void) sprintf(prstatname,"%s/%d/status",pdir,pid);
	(void) sprintf(prcredname,"%s/%d/cred",pdir,pid);

	if ((prstat_fd = openprocfile(prstatname)) < 0)
		return openerr;

	if ((prcred_fd = openprocfile(prcredname)) < 0)
		return openerr;
	
	if (read(prstat_fd, &prstat, sizeof(pstatus_t)) != sizeof(pstatus_t)) {
		(void) fprintf(stderr, "dumpcore(): read of status file %s failed\n",
			prstatname);
		return -1;
        }
	if (read(prcred_fd, &prcred, sizeof(prcred_t)) != sizeof(prcred_t)) {
		(void) fprintf(stderr, "dumpcore(): read of cred file %s failed\n",
			prcredname);
		return -1;
        }

	if (uname(&u_name) < 0) {
		perror("uname");
		return -1;
	}

	/*
	 * Write the note sections for the process-wide data
	 * (pstatus, psinfo, credentials, utsname).
	 */
	if (elfnote(CF_T_PRSTATUS, &prstat, sizeof(pstatus_t)) < 0)
		return -1;
	if (elfnote(CF_T_PRPSINFO, &psstat, sizeof(psinfo_t)) < 0)
		return -1;
	if (elfnote(CF_T_PRCRED, &prcred, sizeof(prcred_t)) < 0)
		return -1;
	if (elfnote(CF_T_UTSNAME, &u_name, sizeof(struct utsname)) < 0)
		return -1;

	/*
	 * Dump the note sections for the per-lwp data.
	 */
	for (lwp_i = 0; lwp_i < psstat.pr_nlwp; lwp_i++) {
		(void) sprintf(lwpstatname,"%s/%d/lwp/%d/lwpstatus",pdir,pid,lwp_i+1);
		if ((lwpstat_fd = openprocfile(lwpstatname)) < 0)
			return openerr;

		if ((ret = read(lwpstat_fd, &lwpstat, sizeof(lwpstatus_t)))
				!= sizeof(lwpstatus_t)) {
			(void) fprintf(stderr,"dumpcore(): read of lwpstatus file %s failed\n",
				lwpstatname);
			return -1;
		}		       
		(void) sprintf(lwpinfoname, "%s/%d/lwp/%d/lwpsinfo",pdir,pid,lwp_i+1);
		if ((lwpinfo_fd = openprocfile(lwpinfoname)) < 0)
			return openerr;

		if ((ret = read(lwpinfo_fd, &lwpsinfo, sizeof(lwpsinfo_t)))
				!= sizeof(lwpsinfo_t)) {
			(void) fprintf(stderr,"dumpcore(): read of lwpsinfo file %s failed\n",
				lwpinfoname);
			return -1;
		}		       
                if (elfnote(CF_T_LWPSTATUS,&lwpstat,sizeof(lwpstatus_t)) < 0)
			return -1;
                if (elfnote(CF_T_LWPSINFO,&lwpsinfo,sizeof(lwpsinfo_t)) < 0)
			return -1;
        }

	/*
	 * Dump data and stack.
	 * p_offset and p_filesz will be zero for segments not to be dumped.
	 * If a segment can be read only partially, adjust the recorded
	 * file offsets to reflect the amount actually dumped.  (This can
	 * occur, for example, if an mmap'd file segment is larger than
	 * the underlying file.  It is not a common situation.)
	 */
	for (i = psstat.pr_nlwp + 1; i < nhdrs; i++) {
		if (v[i].p_filesz == 0)
			continue;

		(void) lseek(as_fd, v[i].p_vaddr, SEEK_SET);

		/* after partial segment, adjust remaining file offsets */
		if (partial)
			v[i].p_offset = lseek(core_fd, 0L, SEEK_CUR);

		/* first read to page boundary, subsequently page-aligned */
		ncount = readsize - (v[i].p_vaddr % readsize);

		for (count = v[i].p_filesz; count > 0; ) {
			if (ncount > count)
				ncount = count;

			if ((ncount = read(as_fd, buf, ncount)) <= 0)
				break;

			if (write(core_fd, buf, ncount) != ncount) {
				perror("write");
				return -1;
			}

			count -= ncount;
			ncount = readsize;
		}

		/* if complete segment not dumped, must adjust header info */
		if (count > 0) {
			if ((v[i].p_filesz -= count) == 0)
				v[i].p_offset = 0;
			++partial;
		}
	}
		
	/*
	 * If one or more segments were only partially dumped, go back and
	 * rewrite the headers to reflect the amounts actually dumped.
	 */
	if (partial) {
		if (lseek(core_fd, sizeof(Elf32_Ehdr), SEEK_SET) != sizeof(Elf32_Ehdr)
			|| write(core_fd, (char *)v, hdrsz) != hdrsz) {
			perror("write");
			return -1;
		}
	}

	(void) fprintf(stderr, "%s: %s.%d dumped\n", command, filename, pid);
	return 0;
}


static int
elfnote(type, ptr, size)
	int type;
	void *ptr;
	int size;
{
	Elf32_Note note;		/* ELF note */

	(void) memset(&note, 0, sizeof(Elf32_Note)); 
	(void) strcpy(note.name,"CORE");
	note.type = type;
	note.namesz = 8;
	note.descsz = roundup(size, sizeof(Elf32_Word));

	if (write(core_fd, (char *)&note, sizeof(Elf32_Note)) != sizeof(Elf32_Note)
		|| write(core_fd, (char *)ptr, note.descsz) != note.descsz) {
		perror("write");
		return -1;
	}

	return 0;
}

 

static int
isprocdir(dir)	/* return TRUE iff dir is a PROC directory */
	char *dir;
{
	/* this is accomplished by doing a stat on the directory */
	/* and checking the st_fstype entry of the stat structure */
	/* to see if the file system type is "proc".              */

	struct stat stat1;	/* dir  */
	char * path = (char *)&buf[0];
	register char * p;

	/* make a copy of the directory name without trailing '/'s */
	if (dir == NULL)
		(void) strcpy(path, ".");
	else {
		(void) strcpy(path, dir);
		p = path + strlen(path);
		while (p > path && *--p == '/')
			*p = '\0';
		if (*path == '\0')
			(void) strcpy(path, ".");
	}

	/* stat the directory */
	if (stat(path, &stat1) != 0)
		return FALSE;

	/* check to see if the directory is a "proc" directory */
	if (strcmp(stat1.st_fstype, "proc"))
		return FALSE;		/* not a "proc" directory */
	else
		return TRUE;		/* it is a "proc" directory */
}

/*
 * Grab existing process.
 * Return negative error code on failure, 0 on success.
 * As a side effect, may leave one or more of the global
 * file descriptors as_fd, status_fd and ctl_fd open to
 * the process's /proc/pid/as, /proc/pid/status and
 * /proc/pid/ctl files, respectively.  It is the caller's
 * responsibility to close these file descriptors.
 */
static int
Pgrab(pdir, pid)
	char *pdir;			/* /proc directory */
	register pid_t pid;		/* UNIX process ID */
{
	int saverr;

again:	/* Come back here if we lose it in the Window of Vulnerability */
	if (as_fd >= 0) {
		(void) close(as_fd);
		as_fd = -1;
	}
	if (status_fd >= 0) {
		(void) close(status_fd);
		status_fd = -1;
	}
	if (ctl_fd >= 0) {
		(void) close(ctl_fd);
		ctl_fd = -1;
	}

	/* generate the /proc/pid filename */
	(void) sprintf(asname, "%s/%d/as", pdir, pid);

	/* Request exclusive open to avoid grabbing someone else's	*/
	/* process and to prevent others from interfering afterwards.	*/
	if ((as_fd = open(asname, (O_RDWR|O_EXCL))) < 0) {
		switch (errno) {
		case EBUSY:
			return G_BUSY;
		case ENOENT:
			return G_NOPROC;
		case EACCES:
		case EPERM:
			return G_PERM;
		default:
			perror("Pgrab open()");
			return G_STRANGE;
		}
	}

	/* Make sure the file descriptor is not one of 0, 1, or 2 */
	if (as_fd <= 2) {
		int dfd = fcntl(as_fd, F_DUPFD, 3);
		(void) close(as_fd);
		as_fd = dfd;

		if (dfd < 0) {
			perror("Pgrab fcntl()");
			return G_STRANGE;
		}
	}

	/* ---------------------------------------------------- */
	/* We are now in the Window of Vulnerability (WoV).	*/
	/* The process may exec() a setuid/setgid or unreadable	*/
	/* object file between the open() and the PCSTOP.	*/
	/* We will get EBADF in this case and must start over.	*/
	/* ---------------------------------------------------- */

	/*
	 * Get the process's status.
	 */
	(void) sprintf(statusname, "%s/%d/status", pdir, pid);
	if ((status_fd = openprocfile(statusname)) < 0)
		return openerr;

	if (read(status_fd, &prstat, sizeof(pstatus_t)) != sizeof(pstatus_t)) {
		(void) fprintf(stderr, "Pgrab - read failed for status file %s\n", statusname);
		return G_STRANGE;
	}

	/*
	 * If the process is a system process, we can't dump it.
	 */
	if (prstat.pr_flags & PR_ISSYS)
		return G_SYS;

	/*
	 * We can't dump ourself.
	 */
	if (pid == getpid()) {
		/*
		 * Verify that the process is really ourself:
		 * Set a magic number, read it through the
		 * /proc file and see if the results match.
		 */
		long magic1 = 0;
		long magic2 = 2;

		if (lseek(as_fd, (long)&magic1, SEEK_SET) == (long)&magic1
		 && read(as_fd, (char *)&magic2, sizeof(magic2)) == sizeof(magic2)
		 && magic2 == 0
		 && (magic1 = 0xfeedbeef)
		 && lseek(as_fd, (long)&magic1, SEEK_SET) == (long)&magic1
		 && read(as_fd, (char *)&magic2, sizeof(magic2)) == sizeof(magic2)
		 && magic2 == 0xfeedbeef) {
			return G_SELF;
		}
	}

	/*
	 * If the process is already stopped or has been directed
	 * to stop via /proc, there is nothing more to do.
	 */
	if (prstat.pr_lwp.pr_flags & (PR_ISTOP|PR_DSTOP))
		return 0;

	(void) sprintf(ctlname, "%s/%d/ctl", pdir, pid);
	if ((ctl_fd = open(ctlname, O_WRONLY)) < 0) {
		switch (errno) {
		case EBUSY:
			return G_BUSY;
		case ENOENT:
			return G_NOPROC;
		case EACCES:
		case EPERM:
			return G_PERM;
		default:
			perror("Pgrab open()");
			return G_STRANGE;
		}
	}

	/*
	 * Mark the process run-on-last-close so
	 * it runs even if we die from SIGKILL.
	 */
	if (Ioctl(ctl_fd, PR_RLC, 1) == -1) {
		if (errno == EBADF)	/* WoV */
			goto again;

		if (errno == ENOENT)	/* Don't complain about zombies */
			return G_ZOMB;

		perror("Pgrab PR_RLC");
		return G_STRANGE;
	}
	
	/*
	 * Direct the process to stop.
	 * Set an alarm to avoid waiting forever.
	 */
	timeout = FALSE;
	saverr = 0;
	(void) alarm(2);
	if (Ioctl(ctl_fd, PCSTOP, 0) == 0)
		(void) alarm(0);
	else {
		saverr = errno;
		(void) alarm(0);
		if (saverr == EINTR
		 && timeout
		&& (lseek(status_fd, 0, SEEK_SET) != -1)
		&& (read(status_fd, &prstat, sizeof(pstatus_t))
		    != sizeof(pstatus_t))) {
			saverr = errno;
			(void) fprintf(stderr, "Pgrab - read failed for status file %s\n",
				statusname);
			timeout = FALSE;
		}
	}
	if (saverr) {
		int rc;

		switch (saverr) {
		case EBADF:		/* we lost control of the the process */
			goto again;
		case EINTR:		/* timeout or user typed DEL */
			rc = G_INTR;
			break;
		case ENOENT:
			rc = G_ZOMB;
			break;
		default:
			perror("Pgrab PCSTOP");
			rc = G_STRANGE;
			break;
		}
		if (!timeout || saverr != EINTR) {
			return rc;
		}
	}

	/* re-read status file to ensure process is stopped */
	(void) lseek(status_fd, 0, SEEK_SET); /* reset status file */
	if (read(status_fd, &prstat, sizeof(pstatus_t))
	   != sizeof(pstatus_t)) {
		(void) fprintf(stderr, "Pgrab - re-read of status file %s failed\n",
		 statusname);
	}

	/*
	 * Process should either be stopped via /proc or
	 * there should be an outstanding stop directive.
	 */
	if ((prstat.pr_lwp.pr_flags & (PR_ISTOP|PR_DSTOP)) == 0) {
		(void) fprintf(stderr, "Pgrab: process is not stopped\n");
		return G_STRANGE;
	}

	return 0;
}

static int
Ioctl(fd, request, type)
	int fd;
	ulong request;
{
	ulong_t cmd[2];
	int len;

	if (type == 1) {
		cmd[0] = PCSET;
		cmd[1] = request;
		len = 2*sizeof(long);
	} else {
		cmd[0] = request;
		len = sizeof(long);
	}
	return (write(fd, (char *)cmd, len) == len ? 0 : -1);
}

static int
openprocfile(filename)
char *filename;
{
        int fdname;
  
        openerr=0;
	if ((fdname = open(filename, O_RDONLY)) < 0) {
		switch (errno) {
		case EBUSY:
			openerr = G_BUSY;
			break;
		case ENOENT:
			openerr = G_NOPROC;
			break;
		case EACCES:
		case EPERM:
			openerr = G_PERM;
			break;
		default:
			perror("Pgrab open()");
			openerr = G_STRANGE;
			break;
		}
	}
	return fdname;
}
