/*		copyright	"%c%" 	*/

#ident	"@(#)mp.cmds:i386/cmd/sysdef/sysdef.c	1.3.1.2"
#ident  "$Header$"

/***************************************************************************
 * Command: sysdef
 * Inheritable Privileges: P_DEV,P_SYSOPS,P_MACREAD,P_DACREAD 
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/
	/*
	** This command can now print the value of data items
	** from (1) the file (/unix is default), and (2) from
	** /dev/kmem.  If the read is from /dev/kmem, we can
	** also print the value of BSS symbols, (e.g., nadvertise).
	** The logic to support this is: if read is from file,
	** (1) find the section number of .bss, (2) look through
	** nlist for symbols that are in .bss section and zero
	** the n_value field.  At print time, if the n_value field
	** is non-zero, print the info.
	**
	** This protects us from trying to read a bss symbol from
	** the file and, possibly, droping core.
	**
	** When reading from /dev/kmem, the n_value field is the
	** seek address, and the contents are read from that address.
	**
	** NOTE: when reading from /dev/kmem, the actual, incore
	** values will be printed, for example: the current nodename
	** will be printed, and as mentioned above, nadvertise (number
	** of current advertised resources), etc.
	**
	** the cmn line usage is: sysdef -i -n namelist -m master.d directory
	** (-i for incore.)
	*/
#include	<stdio.h>
#include	<nlist.h>
#include	<sys/types.h>
#include	<sys/sysmacros.h>
#include	<sys/sysi86.h>
#include	<sys/var.h>
#include	<sys/tuneable.h>
#include	<sys/ipc.h>
#include	<sys/msg.h>
#include	<sys/sem.h>
#include	<sys/shm.h>
#include	<sys/fcntl.h>
#include	<sys/utsname.h>
#include	<sys/resource.h>
#include	<sys/conf.h>
#include	<sys/stat.h>
#include	<sys/signal.h>
#include	<sys/hrtcntl.h>
#include	<sys/priocntl.h>
#include	<sys/procset.h>
#include	<ctype.h>
#include	<errno.h>
#include	<acl.h>
/*Needed for inline swap code*/
#include 	<sys/swap.h>

#include	<libelf.h>
#include	<sys/elf_386.h>
#include	<sys/param.h>
#include	<sys/ksym.h>

extern char *ctime();
extern char *strcat();
extern char *strcpy();
extern char *strncpy();
extern char *strncat();
extern size_t strlen();
extern void *malloc();
extern void *calloc();
extern void *realloc();

extern char *optarg;
extern int optind;

#define ONLBE(d)	(0)
#define KVIOBASE	0x60000

static	struct	var	v;
struct  tune	tune;
static	int	aclmax;
static	struct	msginfo	minfo;
static	struct	seminfo	sinfo;
static	struct	shminfo	shinfo;

static	int 	incore = 0; /* 0 == read values from /dev/kmem, 1 == from file */
static	int 	bss;	    /* if read from file, don't read bss symbols */
static	char	*os ="/stand/unix";
static	char	*mem = "/dev/kmem";
static	char 	*rlimitnames[] = {
	"cpu time",
	"file size",
	"heap size",
	"stack size",
	"core file size",
	"file descriptors",
	"mapped memory"
};

static	long	strthresh;
static	int	nstrpush, strmsgsz, strctlsz;
static	int	nadvertise, nrcvd, nrduser, nsndd, minserve, maxserve, maxgdp,
	rfsize, rfs_vhigh, rfs_vlow, nsrmount, nremote, nlocal, rcache_time;
static	int	hrtimes_size, itimes_size,
	aio_size, min_aio_servers, max_aio_servers, aio_server_timeout;
static	short	naioproc, ts_maxupri;
static	int	ncsize;
static	long	sfs_ninode;
static	int	adt_bsize, adt_nbuf, adt_nlvls;
static	char 	sys_name[10], intcls[10];
static	unsigned int	nlsize, lnsize;
static	int	sysfile, memfile;

static	void	memseek(), getnlist();
static 	int	getinfo();

static	struct nlist	*nl, *nlptr;

static	int 	com2cons, do387cr3, do386b1, maxminor, nclass,
 	ninode, ninode, nstrphash, piomaxsz, putbufsz,
	vs, tu, aclp, msginfo, seminfo,
	shminfo, FLckinfo, utsnm,
	pnstrpush, pstrthresh, pstrmsgsz, pstrctlsz, pmaxgdp,
	pnadvertise, pnrcvd, pnrduser, pnsndd, pminserve,
	pmaxserve, prfsize, prfs_vhigh, prfs_vlow, pnsrmount,
	pnremote, pnlocal, prcache_time, endnm,
	phrtimes_size, pitimes_size, pnaiosys, pminaios, 
	pmaxaios, paiotimeout, pnaioproc, pts_maxupri, 
	psys_name, pinitclass, prlimits,
	pncsize, pninode, psfsninode,
	padtbsize, padtnbuf, padtnlvls;

#define ADDR	0	/* index for _addr array */
#define OPEN	1	/* index for open routine */
#define CLOSE	2	/* index for close routine */
#define READ	3	/* index for read routine */
#define WRITE	4	/* index for write routine */
#define IOCTL	5	/* index for ioctl routine */
#define STRAT	6	/* index for strategy routine */
#define MMAP	7	/* index for mmap routine */
#define SEGMAP	8	/* index for segmap routine */
#define POLL	9	/* index for poll routine */
#define SIZE	10	/* index for size routine */

#define EQ(x,y)		(strcmp(x, y)==0)  

#define	MAXI	300
#define	MAXL	MAXI/11+10
#define EXPAND	99

static	struct	link {
	char	*l_cfnm;	/* config name from master table */
	int l_funcidx;		/* index into name list structure */
	unsigned int l_soft :1;	/* software driver flag from master table */
	unsigned int l_dtype:1;	/* set if block device */
	unsigned int l_used :1;	/* set when device entry is printed */
} *ln, *lnptr;

	/* ELF Items */
static	Elf *elfd = NULL;
static	Elf32_Ehdr *ehdr = NULL;

/*
 * Procedure:     main
 *
 * Restrictions:
                 getopt: none
                 fprintf: none
                 open: none
                 stat(2): none
                 opendir: none
                 printf: none
                 fgets: none
                 sscanf: none
                 fclose: none
                 ctime: none
                 acl(2): none
*/
/*This global variable is needed for the inline swap code*/

static	char *prognamep;

main(argc, argv)
	int	argc;
	char	**argv;
{

	struct	utsname utsname;
	struct rlimit rlimit[RLIM_NLIMITS];
	Elf_Scn *scn;
	Elf32_Shdr *shdr;
	char *name;
	int ndx;
	int i;

	prognamep = argv[0];
	while ((i = getopt(argc, argv, "im:n:?")) != EOF) {
		switch (i) {
		case 'i':
			incore++;
			break;
		case 'n':
			os = optarg;
			break;
		default:
			fprintf(stderr,"usage: %s [-i -n namelist]\n", argv[0]);
			exit (1);
		}
	}


	if (incore) {
		if((memfile = open(mem,O_RDONLY)) < 0) {
			fprintf(stderr,"cannot open %s: %s\n", mem, strerror(errno));
			exit(1);
		}
	}
	else {

		/*
		**	Use libelf to read both COFF and ELF namelists
		*/
		if((sysfile = open(os,O_RDONLY)) < 0) {
			fprintf(stderr,"cannot open %s: %s\n",os, strerror(errno));
			exit(1);
		}
	
        	if ((elf_version(EV_CURRENT)) == EV_NONE) {
                	fprintf(stderr, "ELF Access Library out of date\n");
				exit (1);
        	}
		
        	if ((elfd = elf_begin (sysfile, ELF_C_READ, NULL)) == NULL) {
                	fprintf(stderr, "Unable to elf begin %s (%s)\n",
				os, elf_errmsg(-1));
			exit (1);
        	}
	
		if ((ehdr = elf32_getehdr(elfd)) == NULL) {
			fprintf(stderr, "%s: Can't read Exec header (%s)\n",
				os, elf_errmsg(-1));
			exit (1);
		}
	
		if ( (((elf_kind(elfd)) != ELF_K_ELF) &&
				((elf_kind(elfd)) != ELF_K_COFF))
				|| (ehdr->e_type != ET_EXEC) )
		{
			fprintf(stderr, "%s: invalid file\n", os);
			elf_end(elfd);
			exit (1);
		}

		/*
		**	If this is a file read, look for .bss section
		*/

		ndx = 1;
		scn = NULL;
		while ((scn = elf_nextscn(elfd, scn)) != NULL) {
			if ((shdr = elf32_getshdr(scn)) == NULL) {
				fprintf(stderr, "%s: Error reading Shdr (%s)\n",
					os, elf_errmsg(-1));
				exit (1);
			}
			name =
			elf_strptr(elfd, ehdr->e_shstrndx, (size_t)shdr->sh_name);
			if ((name) && ((strcmp(name, ".bss")) == 0)) {
				bss = ndx;
			}
			ndx++;
		}
	} /* (!incore) */

	uname(&utsname);
	printf("*\n* %s Kernel Configuration\n*\n",utsname.machine);

	nlsize = MAXI;
	lnsize = MAXL;
	nl=(struct nlist *)(calloc(nlsize, sizeof(struct nlist)));
	ln=(struct link *)(calloc(lnsize, sizeof(struct link)));
	nlptr = nl;
	lnptr = ln;

	/*
	endnm = setup("");

	getnlist();
	*/

	/* easy stuff */
	printf("*\n* Tunable Parameters\n*\n");
	nlptr = nl;
	vs = setup("v");
	tu = setup("tune");
	utsnm = setup("utsname");
	prlimits = setup("sysdef_rlimits");
	endnm = msginfo = setup("msginfo");
	aclp = setup("aclmax");
	pnstrpush = setup("nstrpush");
	pstrthresh = setup("strthresh");
	pstrmsgsz = setup("strmsgsz");
	pstrctlsz = setup("strctlsz");
	pnadvertise = setup("nadvertise");
	pmaxgdp = setup("maxgdp");
	pnrcvd = setup("nrcvd");
	pnrduser = setup("nrduser");
	pnsndd = setup("nsndd");
	pminserve = setup("minserve");
	pmaxserve = setup("maxserve");
	prfsize = setup("rfsize");
	prfs_vhigh = setup("rfs_vhigh");
	prfs_vlow = setup("rfs_vlow");
	pnsrmount = setup("nsrmount");
	seminfo = setup("seminfo");
	shminfo = setup("shminfo");
	pnremote = setup("nremote");
	pnlocal = setup("nlocal");
	prcache_time = setup("rc_time");
	phrtimes_size = setup("hrtimes_size");
	pitimes_size = setup("itimes_size");
	pnaiosys = setup("aio_size");
	pminaios = setup("min_aio_servers");
	pmaxaios = setup("max_aio_servers");
	paiotimeout = setup("aio_server_timeout");
	pnaioproc = setup("naioproc");
	pts_maxupri = setup("ts_maxupri");
	psys_name = setup("sys_name");
	pinitclass = setup("intcls");
	pncsize = setup("ncsize");
	pninode = setup("ninode");
	psfsninode = setup("sfs_ninode");
	padtbsize = setup("adt_bsize");
	padtnbuf = setup("adt_nbuf");
	padtnlvls = setup("adt_nlvls");
 	com2cons = setup("com2cons");
 	do386b1 = setup("do386b1");
 	do387cr3 = setup("do387cr3");
 	maxminor = setup("maxminor");
 	nclass = setup("nclass");
 	nstrphash = setup("nstrphash");
 	piomaxsz = setup("piomaxsz");
 	putbufsz = setup("putbufsz");

	setup("");

	if(!incore) {
		getnlist();

		for(nlptr = &nl[vs]; nlptr != &nl[endnm]; nlptr++) {
			if(nlptr->n_value == 0) {
				fprintf(stderr, "namelist error\n");
				exit(1);
			}
		}
	}
	(void) getinfo(vs,&v,sizeof(v));
	printf("%6d	number of buffer headers allocated at a time (NBUF)\n",
		v.v_buf);
	printf("%6d	maximum kilobytes for buffer cache (BUFHWM)\n",v.v_bufhwm);
	printf("%6d	entries in callout table (NCALL)\n",v.v_call);
	printf("%6d	entries in proc table (NPROC)\n",v.v_proc);
	printf("%6d	maximum global priority in sys class (MAXCLSYSPRI)\n",v.v_maxsyspri);
	printf("%6d	processes per user id (MAXUP)\n",v.v_maxup);
	printf("%6d	hash slots for buffer cache (NHBUF)\n",v.v_hbuf);
	printf("%6d	buffers for physical I/O (NPBUF)\n",v.v_pbuf);
	printf("%6d	auto update time limit in seconds (NAUTOUP)\n",
		v.v_autoup);
	(void) getinfo(tu,&tune,sizeof(tune));
	printf("%6d  page stealing low water mark (GPGSLO)\n", tune.t_gpgslo);
	printf("%6d  fsflush run rate (FSFLUSHR)\n", tune.t_fsflushr);
	if (getinfo(aclp, &aclmax, sizeof(aclmax)) == 0) {
		printf("%6d  maximum number of ACL entries (ACLMAX)\n", aclmax);
	}

	if (getinfo(nclass,&nclass,sizeof(nclass)) == 0)
 		printf("%6d  number of scheduler classes (NCLASS)\n", nclass);
 
 
 	printf("*\n* i386 Specific Tunables\n*\n");
	if (getinfo(com2cons, &com2cons, sizeof(com2cons)) == 0)
 		printf("%6d  Alternate console switch (COM2CONS)\n", com2cons);
 
	if (getinfo(do386b1,&do386b1,sizeof(do386b1)) == 0)
 		printf("%6d  80836 B1 stepping bug work around (DO386B1)\n", do386b1);
 
	if (getinfo(do387cr3,&do387cr3,sizeof(do387cr3)) == 0)
 		printf("%6d  80387 errata #21 work around (DO387CR3)\n", do387cr3);
 
	if (getinfo(maxminor,&maxminor,sizeof(maxminor)) == 0)
 		printf("%6d  maximum value for major and minor numbers (MAXMINOR)\n", maxminor);
 
	if (getinfo(nstrphash,&nstrphash,sizeof(nstrphash)) == 0)
 		printf("%6d  size of internal hash table (NSTRPHASH)\n", nstrphash);
 
	if (getinfo(piomaxsz,&piomaxsz,sizeof(piomaxsz)) == 0)
 		printf("%6d  size of virtual kernel address space for raw hard disk I/O (PIOMAXSZ)\n", piomaxsz);
 
	if (getinfo(putbufsz,&putbufsz,sizeof(putbufsz)) == 0)
 		printf("%6d  size of buffer to record system messages (PUTBUFSZ)\n", putbufsz);

	printf("*\n* Utsname Tunables\n*\n");

	printf("%8s  release (REL)\n",utsname.release);
	printf("%8s  node name (NODE)\n",utsname.nodename);
	printf("%8s  system name (SYS)\n",utsname.sysname);
	printf("%8s  version (VER)\n",utsname.version);

	printf("*\n* Process Resource Limit Tunables (Current:Maximum)\n*\n");
	(void) getinfo(prlimits,rlimit,sizeof(rlimit));
	for (i = 0; i < RLIM_NLIMITS; i++) {
		if (rlimit[i].rlim_cur == RLIM_INFINITY)
			printf("Infinity:");
		else
			printf("0x%8.8x:", rlimit[i].rlim_cur);
		if (rlimit[i].rlim_max == RLIM_INFINITY)
			printf("Infinity");
		else
			printf("0x%8.8x", rlimit[i].rlim_max);
		printf("\t%s\n", rlimitnames[i]);
	}

	printf("*\n* File System Tunables\n*\n");
	if (getinfo(pncsize,&ncsize,sizeof(ncsize)) == 0) {
		printf("%6d	directory name lookup cache size (ncsize)\n",
			ncsize);
	}
	if (getinfo(pninode,&ninode,sizeof(ninode)) == 0) {
		printf("%6d	number of s5 inodes (NINODE)\n", ninode);
	}
	if (getinfo(psfsninode,&sfs_ninode,sizeof(sfs_ninode)) == 0) {
		printf("%6d	number of sfs inodes (SFSNINODE)\n",
			sfs_ninode);
	}

	printf("*\n* Streams Tunables\n*\n");
	if (getinfo(pnstrpush,&nstrpush,sizeof(nstrpush)) ==0) {
		printf("%6d	maximum number of pushes allowed (NSTRPUSH)\n",
			nstrpush);
	}
	if (getinfo(pstrthresh,&strthresh,sizeof(strthresh)) == 0) {
		if (strthresh) {
			printf("%6ld	streams threshold in bytes (STRTHRESH)\n",
				strthresh);
		}
		else {
			printf("%6ld	no streams threshold (STRTHRESH)\n",
				strthresh);
		}
	}
	if (getinfo(pstrmsgsz,&strmsgsz,sizeof(strmsgsz)) == 0) {
		printf("%6d	maximum stream message size (STRMSGSZ)\n",
			strmsgsz);
	}
	if (getinfo(pstrctlsz,&strctlsz,sizeof(strctlsz)) == 0) {
		printf("%6d	max size of ctl part of message (STRCTLSZ)\n",
			strctlsz);
	}

	printf("*\n* RFS Tunables\n*\n");
	if (getinfo(pnadvertise, &nadvertise, sizeof(nadvertise)) == 0) {
		printf("%6d	entries in advertise table (NADVERTISE)\n",
			nadvertise);
	}
	if (getinfo(pnrcvd, &nrcvd, sizeof(nrcvd)) == 0) {
		printf("%6d	receive descriptors (NRCVD)\n",
			nrcvd);
	}
	if (getinfo(pnrduser, &nrduser, sizeof(nrduser)) == 0) {
		printf("%6d	maximum number of rd_user structures (NRDUSER)\n",
			nrduser);
	}
	if (getinfo(pnsndd, &nsndd, sizeof(nsndd)) == 0) {
		printf("%6d	send descriptors (NSNDD)\n",
			nsndd);
	}
	if (getinfo(pminserve, &minserve, sizeof(minserve)) == 0) {
		printf("%6d	minimum number of server processes (MINSERVE)\n",
			minserve);
	}
	if (getinfo(pmaxserve, &maxserve, sizeof(maxserve)) == 0) {
		printf("%6d	maximum number of server processes (MAXSERVE)\n",
			maxserve);
	}
	if (getinfo(pmaxgdp, &maxgdp, sizeof(maxgdp)) == 0) {
		printf("%6d	maximum number of remote systems with active mounts (MAXGDP)\n",
			maxgdp);
	}
	if (getinfo(prfsize, &rfsize, sizeof(rfsize)) == 0) {
		printf("%6d	size of static RFS administrative storage area (RFHEAP)\n",
			rfsize);
	}
	if (getinfo(prfs_vhigh, &rfs_vhigh, sizeof(rfs_vhigh)) == 0) {
		printf("%6d	latest compatible RFS version (RFS_VHIGH)\n",
			rfs_vhigh);
	}
	if (getinfo(prfs_vlow, &rfs_vlow, sizeof(rfs_vlow)) == 0) {
		printf("%6d	earliest compatible RFS version (RFS_VLOW)\n",
			rfs_vlow);
	}
	if (getinfo(pnsrmount, &nsrmount, sizeof(nsrmount)) == 0) {
		printf("%6d	entries in server mount table (NSRMOUNT)\n",
			nsrmount);
	}
	if (getinfo(prcache_time, &rcache_time, sizeof(rcache_time)) == 0) {
		printf("%6d	max interval for turning off RFS caching (RCACHE_TIME)\n",
			rcache_time);
	}
	if (getinfo(pnremote, &nremote, sizeof(nremote)) == 0) {
		printf("%6d	minimum number of RFS buffers (NREMOTE)\n",
			nremote);
	}
	if (getinfo(pnlocal, &nlocal, sizeof(nlocal)) == 0) {
		printf("%6d	minimum number of local buffers (NLOCAL)\n",
			nlocal);
	}
	if (getinfo(msginfo,&minfo,sizeof(minfo)) == 0)
		{
		printf("*\n* IPC Messages\n*\n");
		printf("%6d	entries in msg map (MSGMAP)\n",minfo.msgmap);
		printf("%6d	max message size (MSGMAX)\n",minfo.msgmax);
		printf("%6d	max bytes on queue (MSGMNB)\n",minfo.msgmnb);
		printf("%6d	message queue identifiers (MSGMNI)\n",minfo.msgmni);
		printf("%6d	message segment size (MSGSSZ)\n",minfo.msgssz);
		printf("%6d	system message headers (MSGTQL)\n",minfo.msgtql);
		printf("%6u	message segments (MSGSEG)\n",minfo.msgseg);
		}

	if (getinfo(seminfo,&sinfo,sizeof(sinfo)) == 0)
		{
		printf("*\n* IPC Semaphores\n*\n");
		printf("%6d	entries in semaphore map (SEMMAP)\n",sinfo.semmap);
		printf("%6d	semaphore identifiers (SEMMNI)\n",sinfo.semmni);
		printf("%6d	semaphores in system (SEMMNS)\n",sinfo.semmns);
		printf("%6d	undo structures in system (SEMMNU)\n",sinfo.semmnu);
		printf("%6d	max semaphores per id (SEMMSL)\n",sinfo.semmsl);
		printf("%6d	max operations per semop call (SEMOPM)\n",sinfo.semopm);
		printf("%6d	max undo entries per process (SEMUME)\n",sinfo.semume);
		printf("%6d	semaphore maximum value (SEMVMX)\n",sinfo.semvmx);
		printf("%6d	adjust on exit max value (SEMAEM)\n",sinfo.semaem);
		}

	if (getinfo(shminfo,&shinfo,sizeof(shinfo)) == 0)
		{
		printf("*\n* IPC Shared Memory\n*\n");
		printf("%6d	max shared memory segment size (SHMMAX)\n",shinfo.shmmax);
		printf("%6d	min shared memory segment size (SHMMIN)\n",shinfo.shmmin);
		printf("%6d	shared memory identifiers (SHMMNI)\n",shinfo.shmmni);
		printf("%6d	max attached shm segments per process (SHMSEG)\n",shinfo.shmseg);
		}

	
	printf("*\n* High Resolution Timer Tunables\n*\n");
	if (getinfo(phrtimes_size, &hrtimes_size, sizeof(hrtimes_size)) == 0) {
		printf("%6d	max number of timer structures for real-time clock (HRTIME)\n", hrtimes_size);
	}
	if (getinfo(pitimes_size, &itimes_size, sizeof(itimes_size)) == 0) {
		printf("%6d	max number of timer structures for processes special clocks (HRVTIME)\n", itimes_size);
	}

	if (getinfo(pts_maxupri, &ts_maxupri, sizeof(ts_maxupri)) == 0) {
		printf("*\n* Time Sharing Scheduler Tunables\n*\n");
		printf("%d	maximum time sharing user priority (TSMAXUPRI)\n", ts_maxupri);
	}

	if (getinfo(psys_name, &sys_name, sizeof(sys_name)) == 0) {
		printf("%s	system class name (SYS_NAME)\n", sys_name);
	}

	if (getinfo(pinitclass, &intcls, sizeof(intcls)) == 0) {
		printf("%s	class of init process (INITCLASS)\n", intcls);
	}

	if (getinfo(pnaiosys, &aio_size, sizeof(aio_size)) == 0) {
		printf("*\n* Async I/O Tunables\n*\n");
		printf("%6d	outstanding async system calls(NAIOSYS)\n", aio_size);
	}
	if (getinfo(pminaios, &min_aio_servers, sizeof(min_aio_servers)) == 0) {
		printf("%6d	minimum number of servers (MINAIOS)\n", min_aio_servers);
	}
	if (getinfo(pmaxaios, &max_aio_servers, sizeof(max_aio_servers)) == 0) {
		printf("%6d	maximum number of servers (MAXAIOS)\n", max_aio_servers);
	}
	if (getinfo(paiotimeout, &aio_server_timeout, sizeof(aio_server_timeout)) == 0) {
		printf("%6d	number of secs an aio server will wait (AIOTIMEOUT)\n", aio_server_timeout);
	}
	if (getinfo(pnaioproc, &naioproc, sizeof(naioproc)) == 0) {
		printf("%6d	number of async requests per process (NAIOPROC)\n", naioproc);
	}


	if (getinfo(padtbsize, &adt_bsize, sizeof(adt_bsize)) == 0
		&& getinfo(padtnbuf, &adt_nbuf, sizeof(adt_nbuf)) == 0) {
		printf("*\n* Audit Tunables\n*\n");
		printf("%6d	size of audit buffer(s) (ADT_BSIZE)\n", adt_bsize);
		printf("%6d	number of audit buffers (ADT_NBUF)\n", adt_nbuf);
		if (getinfo(padtnlvls, &adt_nlvls, sizeof(adt_nlvls)) == 0) {
			printf("%6d	number of object level table entries (ADT_NLVLS)\n", adt_nlvls);
		}
	}

	if (elfd)
		elf_end(elfd);
	exit(0);
}

/*
 * Procedure:     setup
 *
 * Restrictions:
                 fprintf: none
*/
/*
 * setup - add an entry to a namelist structure array
 */
static	int
setup(nam)
	char	*nam;
{
	int idx;

	if(nlptr >= &nl[nlsize]) { 
		if ((nl=(struct nlist *)realloc(nl,(nlsize+EXPAND)*(sizeof(struct nlist)))) == NULL) {
			fprintf(stderr, "Namelist space allocation failed\n");
			exit(1);
		}
		nlptr=&nl[nlsize];
		nlsize+=EXPAND;
	}

	nlptr->n_name = (char *)malloc((unsigned int)(strlen(nam)+1));	/* initialize pointer to next string */
	strcpy(nlptr->n_name,nam);	/* move name into string table */
	nlptr->n_type = 0;
	nlptr->n_value = 0;
	idx = nlptr++ - nl;
	return(idx);
}


/*
 * Procedure:     memseek
 *
 * Restrictions:
                 fseek: none
                 fprintf: none
*/
void
memseek(sym)
int	sym;
{
	Elf_Scn *scn;
	Elf32_Shdr *eshdr;
	long eoff;

	if ((scn = elf_getscn(elfd, nl[sym].n_scnum)) == NULL) {
		fprintf(stderr, "%s: Error reading Scn %d (%s)\n",
			os, nl[sym].n_scnum, elf_errmsg(-1));
		exit (1);
	}

	if ((eshdr = elf32_getshdr(scn)) == NULL) {
		fprintf(stderr, "%s: Error reading Shdr %d (%s)\n",
			os, nl[sym].n_scnum, elf_errmsg(-1));
		exit (1);
	}

	eoff = (long)(nl[sym].n_value - eshdr->sh_addr + eshdr->sh_offset);

	if ((lseek(sysfile, eoff, 0)) < 0) {
		fprintf(stderr, "%s: lseek error (in memseek)\n", os);
		exit (1);
	}
}

/*
 * Procedure:     getnlist
 *
 * Restrictions:
*/
/*
**	filter out bss symbols if the reads are from the file
*/
void
getnlist()
{
	register struct nlist *p;

	nlist(os, nl);

		/*
		**	The nlist is done.  If any symbol is a bss
		**	and we are not reading from incore, zero
		**	the n_value field.  (Won't be printed if
		**	n_value == 0.)
		*/
	for (p = nl; p->n_name && p->n_name[0]; p++) {
		if (p->n_scnum == bss) {
			p->n_value = 0;
		}
	}
}

/*
 * Procedure:     getinfo
 *
 * Restrictions:
 *		ioctl(2): none
 *		read(2): none
*/
/*
**	get information from kernel
*/
static int
getinfo(index, buf, buflen)
int index;
void *buf;
size_t buflen;
{
	struct mioc_rksym rks;

	if(incore) {
		rks.mirk_symname = nl[index].n_name;
		rks.mirk_buf = buf;
		rks.mirk_buflen = buflen;
		return(ioctl(memfile,MIOC_READKSYM,&rks));
	}
	if(nl[index].n_value) {
		memseek(index);
		if(read(sysfile,buf,buflen) == buflen)
			return(0);
	}
	return(-1);
}
