/*		copyright	"%c%" 	*/

#ident	"@(#)ipc:ipcs.c	1.12.8.12"
#ident "$Header$"

/***************************************************************************
 * Command: ipcs
 * Inheritable Privileges: P_MACREAD,P_DACREAD
 *       Fixed Privileges: P_DEV
 * Notes:
 *
 ***************************************************************************/
/*
 * ipcs - IPC status
 *
 * Examine and print certain things about
 * message queues, semaphores and shared memory.
 *
 * As of SVR4, IPC information is obtained via msgctl, semctl and shmctl
 * to the extent possible.  /dev/kmem is used only to obtain configuration
 * information and to determine the IPC identifiers present in the system.
 * This change ensures that the information in each msgid_ds, semid_ds or
 * shmid_ds data structure that we obtain is complete and consistent.
 * More importantly, for SVR4.1, it ensures that the user is checked
 * for proper access to the objects.
 * For example, the shm_nattch field of a shmid_ds data structure is
 * only guaranteed to be meaningful when obtained via shmctl; when read
 * directly from /dev/kmem, it may contain garbage.
 * If the user supplies an alternate corefile (using -C), no attempt is
 * made to obtain information using msgctl/semctl/shmctl.
 *
 * In SVR4.1, the display of IPC objects are affected by the presence of
 * ACLs and levels.  At present, only system V IPC objects are handled.
 * Xenix IPC objects are not affected.  Note further that SVR4.0 allows
 * the display of all objects; SVR4.1 only objects to which the user has
 * read access.
 *
 * In the base, /dev/kmem is protected by DAC.  Therefore,
 * this program must always run with the setgid bit on.
 * With MAC installed, the program must in addition have
 * a fixed P_DEV privilege to override the private state
 * of /dev/kmem.
 *
 * Configuration:
 *	crw-r-----	root	kmem	SYS_PUBLIC /dev/kmem (private state)
 *	-r-xr-sr-x	bin	kmem	SYS_PUBLIC /usr/bin/ipcs
 *							P_DEV      (fixed)
 *							P_MACREAD  (inher)
 *							P_MACWRITE (inher)
 */

#include <sys/types.h>
#include <priv.h>
#include <malloc.h>
#include <errno.h>
#include <acl.h>
#include <sys/ipcsec.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <dshm.h>
#include <sys/vnode.h> 
#include <sys/param.h>
#include <sys/var.h>
#define _KERNEL
#include <sys/fs/xnamnode.h>
#undef _KERNEL
#include <sys/sd.h>
#include <fcntl.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <mac.h>
#include <sys/ksym.h>
#include <nlist.h>

#define		SYS5	0
#define		SYS3	1

#define	TIME	0
#define	MSGDIR	1
#define	SEMDIR	2
#define	SHMDIR	3
#define	MSGINFO	4
#define	SEMINFO	5
#define	SHMINFO	6
#define XNAMNODE 7
#define VAR	8
#define SDTAB	9
#define DSHMINFO 10
#define DSHMDIR 11
/*
 * Given an index into an IPC table (message table, shared memory
 * table or semaphore table) determine the corresponding IPC
 * identifier (msgid, shmid or semid).  This requires knowledge of
 * the table size, the corresponding ipc_perm structure (for the
 * sequence number contained therein) and the undocumented method
 * by which the kernel assigns new identifiers.
 */
#define IPC_ID(tblsize, index, permp)	((index) + (tblsize)*(permp)->seq)

struct nlist nl[] = {		/* name list entries for IPC facilities */
	{"time", 0, 0, 0, 0, 0},
	{"msgdir", 0, 0, 0, 0, 0},
	{"semdir", 0, 0, 0, 0, 0},
	{"shmdir", 0, 0, 0, 0, 0},
	{"msginfo", 0, 0, 0, 0, 0},
	{"seminfo", 0, 0, 0, 0, 0},
	{"shminfo", 0, 0, 0, 0, 0},
	{"xnamtable", 0, 0, 0, 0, 0},
	{"v", 0, 0, 0, 0, 0},
	{"sdtab", 0, 0, 0, 0, 0},
	{"dshminfo", 0, 0, 0, 0, 0},
	{"dshmdir", 0, 0, 0, 0, 0},
	{NULL}
};
char	chdr[] = "T     ID     KEY        MODE        OWNER    GROUP",
				/* common header format */
	chdr2[] = "  CREATOR   CGROUP",
				/* c option header format */
	*name = "/stand/unix",	/* name list file */
	*mem = "/dev/kmem",	/* memory file */
	*usage = "usage:  ipcs [-abcdmopqstX] [-C corefile] [-N namelist]",
	*usage_mac="usage:  ipcs [-abcdmopqstX[z|Z]] [-C corefile] "
                   "[-N namelist]",
	opts[] = "abcdmopqstC:N:X", /* allowable options for getopt */
	opts_mac[] = "abcdmopqstzC:N:XZ"; /* allowable getopt options on b2*/
extern char	*optarg;	/* arg pointer for getopt */
int		bflg,		/* biggest size:
					segsz on m; qbytes on q; nsems on s */
		aflg,		/* "all" flag */
		cflg,		/* creator's login and group names */
		dflg,		/* DSHM status */
		mflg,		/* shared memory status */
		oflg,		/* outstanding data:
					nattch on m; cbytes, qnum on q */
		pflg,		/* process id's: lrpid, lspid on q;
					cpid, lpid on m */
		qflg,		/* message queue status */
		sflg,		/* semaphore status */
		tflg,		/* times: atime, ctime, dtime on m;
					ctime, rtime, stime on q;
					ctime, otime on s */
		Cflg,		/* user supplied corefile */
		Nflg,		/* user supplied namelist */
		Xflg,		/* print XENIX IPC also */
		zflg,		/* display level alias name */
		Zflg,		/* display fully qualified level name */

		err;		/* option error count */
extern int	optind;		/* option index for getopt */

/*
 * print level information
 *	-1: don't print level
 *	LVL_ALIAS: print alias name, if it exists
 *	LVL_FULL: print fully qualified level name
 */
int	lvlformat;
#define	SHOWLVL()	(lvlformat != -1)


/*
 * Procedure:     main
 *
 * Restrictions:
                 fprintf: none
                 getopt: none
                 open(2): P_MACREAD
                 lvlipc(2): none
                 printf: none
                 ctime: none
                 msgctl(2): none
                 shmctl(2): none
                 semctl(2): none
		 dshm_control(2): none
*/

int getinfo();
void hp();
void tp();
void macp();
void reade();
void lseeke();

main(argc, argv)
int	argc;		/* arg count */
char	*argv[];	/* arg vector */
{
	register int	i,	/* loop control */
			md,	/* memory file file descriptor */
			o,	/* option flag */
			n,	/* table size */
			id;	/* IPC identifier */
	gid_t		rgid;	/* to save real gid */
	gid_t		egid;	/* to save effective gid */
	time_t		time;	/* date in memory file */

	struct ipcdir	shmdir;		/* shared memory ID directory */
	struct ipcdirent *mds = NULL;	/* shared memory ID entries */
	struct shminfo shminfo;		/* shared memory information */

	struct ipcdir	dshmdir;		/* DSHM ID directory */
	struct ipcdirent *dshmds = NULL;	/* DSHM ID entries */
	struct dshminfo dshminfo;		/* DSHM information */

	struct ipcdir	msgdir;		/* message queue ID directory */
	struct ipcdirent *qds = NULL;	/* message queue ID entries */
	struct msginfo msginfo;		/* message information */
	struct msqid_ds *dsbuf;

	struct ipcdir	semdir;		/* semaphore ID directory */
	struct ipcdirent *sds = NULL;	/* semaphore ID entries */
	struct seminfo seminfo;		/* semaphore information */

	struct var v;		/* tunable parameters for kernel */
	struct xnamnode xnamnode;	/* in-core node for IFNAM files */
	struct xnamnode *pxnamnode; /* in-core node pointer for IFNAM files */
	/* head of table of in-core nodes for IFNAM files */
	struct xnamnode *xnamtable[XNAMTBLSIZE];
	struct ipc_perm ipcperm;	/* simulated permissions for XENIX */
	struct xsd sd;		/* XENIX shared data structure */
	struct xsd *psd;	/* pointer to XENIX shared data structure */
	boolean_t msgflg = B_FALSE;
	boolean_t semflg = B_FALSE;
	boolean_t shmflg = B_FALSE;
	boolean_t dshmflg = B_FALSE;
	boolean_t xnamflg = B_FALSE;
	char *opt_str=opts;
	char *usage_str=usage;
        u_long tmp;
	int nl_i = 0;

	/*
	 * Set up the maximum allowable set of privileges and
	 * turn off all working privileges.
	 * Turn off effective gid until needed.
	 */
	rgid = getgid();
	egid = getegid();
	if (setegid(rgid) != 0) {
		fprintf(stderr,
			"UX:ipcs: ERROR: abort to enforce least privilege\n");
		exit(1);
	}

	/* Go through the options and set flags. */
	while ((o = getopt(argc, argv, opts_mac)) != EOF)
		switch (o) {
		case 'a':
			aflg = bflg = cflg = oflg = pflg = tflg = 1;
			break;
		case 'b':
			bflg = 1;
			break;
		case 'c':
			cflg = 1;
			break;
		case 'C':
			mem = optarg;
			Cflg = 1;
			break;
		case 'm':
			mflg = 1;
			break;
		case 'd':
			dflg = 1;
			break;
		case 'N':
			name = optarg;
			Nflg = 1;
			break;
		case 'o':
			oflg = 1;
			break;
		case 'p':
			pflg = 1;
			break;
		case 'q':
			qflg = 1;
			break;
		case 's':
			sflg = 1;
			break;
		case 't':
			tflg = 1;
			break;
		case 'X':
      			Xflg = 1;
			break;
		case 'z':
      			zflg = 1;
			break;
		case 'Z':
      			Zflg = 1;
			break;
		case '?':
			err++;
			break;
		}

	if (zflg)
		lvlformat = LVL_ALIAS;
	else if (Zflg)
		lvlformat = LVL_FULL;
	else
		lvlformat = -1;

	/* establish up front if the enhanced security package is installed */
	if ((lvlipc(IPC_MSG, 0, MAC_GET, (level_t *)-1) == -1)
	&&  (errno == ENOPKG)) {	/* this is the base system */
		if (SHOWLVL()) {
			fprintf(stderr,"UX:ipcs: ERROR: illegal option -Z or");
			fprintf(stderr,"-z\nUX:ipcs: ERROR: system service ");
			fprintf(stderr,"not installed\n%s\n", usage_str);
			exit(1);
		}
        }
	else {
		opt_str = opts_mac;
		usage_str = usage_mac;
		if (aflg && lvlformat == -1)  lvlformat = LVL_FULL;
        }
	if (err || (optind < argc)) {
   	        fprintf(stderr, "%s\n", usage_str);
		exit(1);
	}
	/*
	 * The z and Z options are mutually exclusive.
	 */
	if (zflg && Zflg) {
		fprintf(stderr,
		"UX:ipcs: ERROR: invalid combination of options -Z & -z\n");
		fprintf(stderr, "%s\n", usage_str);
		exit(1);
	}

	if ((mflg + qflg + sflg + dflg) == 0)
		mflg = qflg = sflg = dflg = 1;

	if (Cflg) {
		/* Check out namelist file if using alternate corefile;
		   Otherwise, use getksym() for running kernel */

	        if (Nflg) {
		     if (nlist(name, nl) < 0) {
			 fprintf(stderr, "ipcs: no namelist file found\n");
			 exit(1);
		     }
		}
		else   	{
		     while (nl[nl_i].n_name != NULL) { 
			if(getksym(nl[nl_i].n_name,
				   (unsigned long *) &nl[nl_i].n_value,
				   &tmp)
			   == -1){ 
			     fprintf(stderr, "ipcs: incorrect namelist file\n");
			     exit(1);
			}
			nl_i++;
		     }
		}
	}

		(void)procprivl(CLRPRV,MACREAD_W,(priv_t)0);
	if (!Cflg) {
		(void)setegid(egid);
		if ((md = open(mem, O_RDONLY)) < 0) {
			fprintf(stderr, "ipcs:  no memory file\n");
			exit(1);
		}
		if (setegid(rgid) != 0) {
			fprintf(stderr,
			"UX:ipcs: ERROR: abort to enforce least privilege\n");
			exit(1);
		}
	} else {
		if ((md = open(mem, O_RDONLY)) < 0) {
			fprintf(stderr, "ipcs:  no memory file\n");
			exit(1);
		}
	}
		(void)procprivl(SETPRV,MACREAD_W,(priv_t)0);

	if (getinfo(md, TIME, &time, sizeof(time)) != 0) {
		fprintf(stderr, "ipcs: cannot get kernel information\n");
		exit(1);
	}
	printf("IPC status from %s as of %s", mem, ctime(&time));

        if (Xflg) {
		if (getinfo(md,XNAMNODE, xnamtable, sizeof(xnamtable)) == 0)
			xnamflg = B_TRUE;
		(void) getinfo(md,VAR, &v, sizeof(v));
	}

        msgctl(-1, IPC_STAT, dsbuf);

	/* Print Message Queue status report. */
	if (qflg) {
		if (getinfo(md, MSGINFO, &msginfo, sizeof msginfo) == 0) {
			printf("%s%s%s%s%s%s%s\nMessage Queues:\n", chdr,
				cflg ? chdr2 : "",
				oflg ? " CBYTES  QNUM" : "",
				bflg ? " QBYTES" : "",
				pflg ? " LSPID LRPID" : "",
				tflg ? "   STIME    RTIME    CTIME " : "",
				SHOWLVL() ? "   LEVEL" : "");
			if (getinfo(md, MSGDIR, &msgdir, sizeof msgdir) == 0)
				n = msgdir.ipcdir_nents;
			else
				n = 0;
			qds = malloc(sizeof *qds * n);
			if (qds == NULL) {
				fprintf(stderr,"ipcs: Cannot get memory\n");
				exit(1);
			}
			lseeke(md, (long)msgdir.ipcdir_entries, 0);
			reade(md, qds, sizeof *qds * n);
			msgflg = B_TRUE;
		} else {
			printf("Message Queue facility not in system.\n");
			n = 0;
		}
		for (i = 0; i < n; i++) {
			struct msqid_ds ds;
			if (!qds[i].ipcd_ent)
				continue;
			id = SLOT_TO_IPCID(i, qds[i].ipcd_seq, msginfo.msgmni);
			if (!Cflg && msgctl(id, IPC_STAT, &ds) < 0)
				continue;
			hp('q', "SRrw-rw-rw-", &ds.msg_perm, id, SYS5);
			if (oflg)
				printf("%7u%6u", ds.msg_cbytes, ds.msg_qnum);
			if (bflg)
				printf("%7u", ds.msg_qbytes);
			if (pflg)
				printf("%6u%6u", ds.msg_lspid, ds.msg_lrpid);
			if (tflg) {
				tp(ds.msg_stime);
				tp(ds.msg_rtime);
				tp(ds.msg_ctime);
			}
			if (SHOWLVL())
				macp('q', id);
			printf("\n");
		}
	}
	if (qds != NULL)
		free(qds);

	/* Print Shared Memory status report. */
	if (mflg) {
		if (getinfo(md, SHMINFO, &shminfo, sizeof shminfo) == 0) {
			if (oflg || bflg || tflg || !qflg || !msgflg)
				printf("%s%s%s%s%s%s%s\n", chdr,
					cflg ? chdr2 : "",
					oflg ? " NATTCH" : "",
					bflg ? "  SEGSZ" : "",
					pflg ? "  CPID  LPID" : "",
					tflg ? "   ATIME    DTIME    CTIME " : "",
					SHOWLVL() ? "   LEVEL" : "");
			printf("Shared Memory:\n");
			if (getinfo(md, SHMDIR, &shmdir, sizeof shmdir) == 0)
				n = shmdir.ipcdir_nents;
			else
				n = 0;
			mds = malloc(sizeof *mds * n);
			if (mds == NULL) {
				fprintf(stderr,"ipcs: Cannot get memory\n");
				exit(1);
			}
			lseeke(md, (long)shmdir.ipcdir_entries, 0);
			reade(md, mds, sizeof *mds * n);
			shmflg = B_TRUE;
		} else {
			printf("Shared Memory facility not in system.\n");
			n = 0;
		}
		for (i = 0; i < n; i++) {
			struct shmid_ds ds;
			if (!mds[i].ipcd_ent)
				continue;
			id = SLOT_TO_IPCID(i, mds[i].ipcd_seq, shminfo.shmmni);
			if (!Cflg && shmctl(id, IPC_STAT, &ds) < 0)
				continue;
			hp('m', "--rw-rw-rw-", &ds.shm_perm, id, SYS5);
			if (oflg)
				printf("%7u", ds.shm_nattch);
			if (bflg)
				printf("%7d", ds.shm_segsz);
			if (pflg)
				printf("%6u%6u", ds.shm_cpid, ds.shm_lpid);
			if (tflg) {
				tp(ds.shm_atime);
				tp(ds.shm_dtime);
				tp(ds.shm_ctime);
			}
			if (SHOWLVL())
				macp('m', id);
			printf("\n");
		}
		if (mds != NULL)
			free(mds);

		if (Xflg) {
			/* handle XENIX system 3 shared data */
			if ((pxnamnode = xnamtable[1]) != 0) {
                                if ((oflg || bflg || tflg || !qflg ||
                                        !msgflg) && !shmflg)
                                        printf("%s%s%s%s\n", chdr,
                                                cflg ? chdr2 : "",
                                                oflg ? " NATTCH" : "",
                                                bflg ? "   SEGSZ" : "");
                                printf("XENIX Shared Memory (3.0):\n");
			} else if (!xnamflg){
                                printf("XENIX Shared Memory (3.0) facility not\
in system.\n");
			} else
                                printf("XENIX Shared Memory (3.0):\n");
			while (pxnamnode)
			{
                                lseeke(md, pxnamnode, 0);
                                reade(md, &xnamnode, sizeof(xnamnode));
                                ipcperm.cuid = ipcperm.uid = xnamnode.x_uid;
                                ipcperm.cgid = ipcperm.gid = xnamnode.x_gid;
                                ipcperm.mode = xnamnode.x_mode;
                                psd = (struct xsd *) xnamnode.x_un.xsd;
                                lseeke(md, psd, 0);
                                reade(md, &sd, sizeof(sd));
                                if (!(sd.xsd_flags & SDI_LOCKED))
                                        ipcperm.mode |= 01000;
                                hp('m', "--rw-rw-rw-", &ipcperm, 0, SYS3);
                                if (oflg)
                                        printf("%7u", xnamnode.x_vnode.v_count);
                                if (bflg)
                                        printf("%8u", sd.xsd_len + 1);
                                printf("\n");
                                pxnamnode = xnamnode.x_next;
			}
		}
	}

	/* Print DSHM status report. */
	if (dflg) {
		if (getinfo(md, DSHMINFO, &dshminfo, sizeof dshminfo) == 0) {
			if (oflg || bflg || tflg || !qflg || !msgflg)
				printf("%s%s%s%s%s%s%s\n", chdr,
				       cflg ? chdr2 : "",
				       oflg ? " NATTCH" : "",
				       bflg ? "  SEGSZ" : "",
				       pflg ? "  CPID  LPID" : "",
				       tflg ? "   ATIME    DTIME    CTIME "
				       : "", 
				       SHOWLVL() ? "   LEVEL" : "");
			printf("Dynamic Shared Memory:\n");
			if (getinfo(md, DSHMDIR, &dshmdir, sizeof dshmdir) == 0)
				n = dshmdir.ipcdir_nents;
			else
				n = 0;
			dshmds = malloc(sizeof(*dshmds) * n);
			if (dshmds == NULL) {
				fprintf(stderr,"ipcs: Cannot get memory\n");
				exit(1);
			}
			lseeke(md, (long)dshmdir.ipcdir_entries, 0);
			reade(md, dshmds, sizeof *dshmds * n);
			dshmflg = B_TRUE;
		} else {
			printf("Dynamic Shared Memory facility "
			       "not available on this system.\n");
			n = 0;
		}
		for (i = 0; i < n; i++) {
			struct dshmid_ds ds;
			if (!dshmds[i].ipcd_ent)
				continue;
			id = SLOT_TO_IPCID(i, dshmds[i].ipcd_seq,
					   dshminfo.dshmmni); 
			if (!Cflg && dshm_control(id, IPC_STAT, &ds) < 0)
				continue;
			hp('d', "--rw-rw-rw-", &ds.dshm_perm, id, SYS5);
			if (oflg)
				printf("%7u", ds.dshm_nattch);
			if (bflg)
				printf("%7d", ds.dshm_mapsize);
			if (pflg)
				printf("%6u%6u", ds.dshm_cpid, ds.dshm_lpid);
			if (tflg) {
				tp(ds.dshm_atime);
				tp(ds.dshm_dtime);
				tp(ds.dshm_ctime);
			}
			if (SHOWLVL())
				macp('m', id);
			printf("\n");
		}
		if (dshmds != NULL)
			free(dshmds);

	}

	/* Print Semaphore facility status. */
	if (sflg) {
		if (getinfo(md, SEMINFO, &seminfo, sizeof seminfo) == 0) {
			if (bflg || tflg || (!qflg || !msgflg) &&
				(!mflg || !shmflg))
				printf("%s%s%s%s%s\n", chdr,
					cflg ? chdr2 : "",
					bflg ? " NSEMS" : "",
					tflg ? "   OTIME    CTIME " : "",
					SHOWLVL() ? "   LEVEL" : "");
			printf("Semaphores:\n");
			if (getinfo(md, SEMDIR, &semdir, sizeof semdir) == 0)
				n = semdir.ipcdir_nents;
			else
				n = 0;
			sds = malloc(sizeof *sds *n);
			if (sds == NULL) {
				fprintf(stderr,"ipcs: Cannot get memory\n");
				exit(1);
			}
			lseeke(md, (long)semdir.ipcdir_entries, 0);
			reade(md, sds, sizeof *sds * n);
			semflg = B_TRUE;
		} else {
			printf("Semaphore facility not in system.\n");
			n = 0;
		}
		for (i = 0; i < n; i++) {
			struct semid_ds ds;
			if (!sds[i].ipcd_ent)
				continue;
			id = SLOT_TO_IPCID(i, sds[i].ipcd_seq, seminfo.semmni);
			if (!Cflg && semctl(id, 0, IPC_STAT, &ds) < 0)
				continue;
			hp('s', "--ra-ra-ra-", &ds.sem_perm, id, SYS5);
			if (bflg)
				printf("%6u", ds.sem_nsems);
			if (tflg) {
				tp(ds.sem_otime);
				tp(ds.sem_ctime);
			}
			if (SHOWLVL())
				macp('s', id);
			printf("\n");
		}
                if (Xflg) {
                        if ((pxnamnode = xnamtable[0]) != 0) {
                        /* handle system 3 semaphores */
                                if ((bflg || tflg || (!qflg || !msgflg)
                                        && (!mflg || (!shmflg &&
                                        getinfo(md,SDTAB,&tmp,1) != 0))) &&
                                        !semflg)
                                                printf("%s%s%s\n", chdr,
                                                        cflg ? chdr2 : "",
                                                        bflg ? " NSEMS" : "");
                                printf("XENIX Semaphores (3.0):\n");
                        } else if (!xnamflg) {
                                printf("XENIX Semaphore (3.0) facility  not \
in system.\n");
                        } else
                                printf("XENIX Semaphores (3.0):\n");
			while (pxnamnode)
			{
                                lseeke(md, pxnamnode, 0);
                                reade(md, &xnamnode, sizeof(xnamnode));
                                ipcperm.cuid = ipcperm.uid = xnamnode.x_uid;
                                ipcperm.cgid = ipcperm.gid = xnamnode.x_gid;
                                ipcperm.mode = xnamnode.x_mode;
                                ipcperm.key = 0;
                                hp('s', "--ra-ra-ra-", &ipcperm, 0, SYS3);
                                if (bflg)
                                        printf("%6u", 1);
                                printf("\n");
                                pxnamnode = xnamnode.x_next;
			}
		}
	}
	exit(0);
	/*NOTREACHED*/
}

/*
 * Procedure:     lseeke
 *
 * Restrictions:
                 perror: none
                 fprintf: none
*/
/*
**	lseeke - lseek with error exit
*/

void
lseeke(f, o, w)
int	f,	/* fd */
	w;	/* whence */
long	o;	/* offset */
{
	if (lseek(f, o, w) == -1) {
		perror("ipcs:  seek error");
		exit(1);
	}
}

/*
 * Procedure:     reade
 *
 * Restrictions:
                 read(2): none
                 perror: none
                 fprintf: none

*/
/*
**	reade - read with error exit
*/

void
reade(f, b, s)
int	f;	/* fd */
size_t	s;	/* size */
void	*b;	/* buffer address */
{
	if (read(f, (char *)b, s) != s) {
		perror("ipcs:  read error");
		exit(1);
	}
}

/*
 * Procedure:     hp
 *
 * Restrictions:
                 printf: none
                 aclipc(2): none
                 getpwuid:  none
                 getgrgid:  P_MACREAD
*/
/*
**	hp - common header print
*/

void
hp(type, modesp, permp, id, sys3)
char				type,	/* facility type */
				*modesp;/* ptr to mode replacement characters */
register struct ipc_perm	*permp;	/* ptr to permission structure */
int				id;	/* facility ID number */
int				sys3;	/* system 5 vs. system 3 */
{
	register int		i;	/* loop control */
	register struct group	*g;	/* ptr to group group entry */
	register struct passwd	*u;	/* ptr to user passwd entry */
	int			ipctype;/* IPC type */

	/* get the IPC definition type */
	switch(type) {
	case 'q':
		ipctype = (int)IPC_MSG;
		break;
	case 's':
		ipctype = (int)IPC_SEM;
		break;
	case 'm':
		ipctype = (int)IPC_SHM;
		break;
	}

        if (sys3){
		printf("%c%s%s", type, "    x	  ", "xenix    ");
	}
	else {
		printf("%c%7d%s%#8.8x ", type, id,
			permp->key ? " " : " 0x", permp->key);
	}
	for (i = 02000; i; modesp++, i >>= 1)
		printf("%c", ((int)permp->mode & i) ? *modesp : '-');
	if (sys3) {
		printf("%c", ' ');
	} else {
		printf("%c", (aclipc(ipctype, id, ACL_CNT, 0, 0) > NACLBASE)
			? '+' : ' ');
	}
	if ((u = getpwuid(permp->uid)) == NULL)
		printf("%9d", permp->uid);
	else
		printf("%9.8s", u->pw_name);
	(void)procprivl(CLRPRV,MACREAD_W,(priv_t)0);

	if ((g = getgrgid(permp->gid)) == NULL)
		printf("%9d", permp->gid);
	else
		printf("%9.8s", g->gr_name);

	(void)procprivl(SETPRV,MACREAD_W,(priv_t)0);

	if (cflg) {
		if ((u = getpwuid(permp->cuid)) == NULL)
			printf("%9d", permp->cuid);
		else
			printf("%9.8s", u->pw_name);

		(void)procprivl(CLRPRV,MACREAD_W,(priv_t)0);
		if ((g = getgrgid(permp->cgid)) == NULL)
			printf("%9d", permp->cgid);
		else
			printf("%9.8s", g->gr_name);
		(void)procprivl(SETPRV,MACREAD_W,(priv_t)0);
	}
}

/*
 * Procedure:     macp
 *
 * Restrictions:
                 lvlipc(2): none
                 fprintf: none
                 lvlout: none
                 printf: none
*/
/*
**	macp - Mandatory Access Control level printout
**
**	Notes:
**		1. Pre-allocate buffer to store level name.  This should
**		   save a lvlout call in most cases.  Double size
**		   dynamically if level won't fit in buffer.
*/

#define	INITBUFSIZ	512		/* initial buffer size */

void
macp(type, id)
	char	type;
	int	id;
{
	static char	lvl_buf[INITBUFSIZ];
	static char	*lvl_name = lvl_buf;
	static int	lvl_namesz = INITBUFSIZ;
	level_t		lid;			/* level identifier */
	int		ipctype;		/* IPC type */
	char		*ipcstr;		/* IPC string message */

	/* get the IPC definition type */
	switch(type) {
	case 'q':
		ipctype = (int)IPC_MSG;
		ipcstr = "message queue";
		break;
	case 's':
		ipctype = (int)IPC_SEM;
		ipcstr = "semaphore";
		break;
	case 'm':
		ipctype = (int)IPC_SHM;
		ipcstr = "shared memory";
		break;
	}

	/* get the lid */
	if (lvlipc(ipctype, id, MAC_GET, &lid) == -1) {
		fprintf(stderr,
		"UX:ipcs: ERROR: cannot get level for %s id %d\n",
			ipcstr, id);
		return;
	}

	while (lvlout(&lid, lvl_name, lvl_namesz, lvlformat) == -1) {
		if ((lvlformat==LVL_FULL) && (errno==ENOSPC)) {
			char *tmp_name;
			if ((tmp_name = malloc(lvl_namesz*2))
			==  (char *)NULL) {
				fprintf(stderr,
	"UX:ipcs: ERROR: no memory to print level for %s id %d\n",
					ipcstr, id);
				return;
			}
			lvl_namesz *= 2;
			if (lvl_name != lvl_buf)
				free(lvl_name);
			lvl_name = tmp_name;
		} else {
			fprintf(stderr,
"UX:ipcs: ERROR: cannot translate level to text format for %s id %d\n",
				ipcstr, id);
			return;
		}
	}

	printf("   ");		/* line up level */
	printf("%s", lvl_name);
}

/*
 * Procedure:     tp
 *
 * Restrictions:
                 localtime:  none
                 printf:  none
*/
/*
**	tp - time entry printer
*/

void
tp(time)
time_t	time;	/* time to be displayed */
{
	register struct tm *t;	/* ptr to converted time */

	if (time) {
		t = localtime(&time);
		printf(" %2d:%2.2d:%2.2d", t->tm_hour, t->tm_min, t->tm_sec);
	} else
		printf(" no-entry");
}

int
getinfo(fd, index, buf, buflen)
int fd, index;
char *buf;
size_t buflen;
{

	struct mioc_rksym rks;
	if (Cflg) {
		if (nl[index].n_value) {
			lseeke(fd, (long)nl[index].n_value, 0);
			reade(fd, buf, buflen);
			return(0);
		}
		return(-1);
	}

	rks.mirk_symname = nl[index].n_name;
	rks.mirk_buf = buf;
	rks.mirk_buflen = buflen;
	return(ioctl(fd,MIOC_READKSYM,&rks));
}
