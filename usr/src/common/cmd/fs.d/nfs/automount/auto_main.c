/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)auto_main.c	1.3"
#ident	"$Header$"

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#define _NSL_RPC_ABI
#include <rpc/rpc.h>
#include <syslog.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "nfs_prot.h"
#include <netinet/in.h>
#include <sys/mnttab.h>
#include <sys/mntent.h>
#include <sys/mount.h>
#include <netdb.h>
#include <sys/signal.h>
#include <sys/file.h>
#include <sys/select.h>
#include <setjmp.h>
#include <nfs/nfs_clnt.h>
#include <sys/poll.h>
#include <netconfig.h>
#include <locale.h>
#include <pfmt.h>
#define NFSCLIENT
#include <nfs/mount.h>
#include "automount.h"

#define DEFAULT_MASTER_MAP	"/etc/auto.master"
#define THREAD_STACK_SIZE	46000
#define TOT_THREADS		25
#define	MIN_THREADS		4
#define	MAX_THREADS		8

int minthreads = MIN_THREADS;
int maxthreads = MAX_THREADS;
int am_thread_stack_size = THREAD_STACK_SIZE;

void set_timeout();
void getmapbyname();
void *auto_run_parallel();
void *auto_run_main();
struct netconfig *loopback_trans();
struct mntlist *mkmntlist();
extern void catch();
extern void check_mnttab();

int maxwait = 60;
int mount_timeout = 30;
int max_link_time = 5*60;
int pingtime = 15;

dev_t	tmpdev;
pid_t	right_pid;
time_t	right_time;
struct cache_client	cache_cl[CACHE_CL_SIZE];
struct mntlist		*current_mounts;

int trace;		/* For debugging purposes */
int verbose;		/* More verbose with err messages */
int syntaxok = 1;	/* OK to log map syntax errs to console */

mutex_t	host_mutex;
mutex_t	subnet_mutex;
mutex_t	nconf_mutex;
mutex_t	dupreq_mutex;
mutex_t	lookup_mutex;
mutex_t	unmount_mutex;
mutex_t	auto_mutex;
cond_t	unmount_cond;
rwlock_t	fh_rwlock;
rwlock_t	fsq_rwlock;
rwlock_t	tmpq_rwlock;

#define SPAWN_THREAD() {					\
	if ((is_creating == 0) &&				\
	    (num_in_dispatch == num_serving) &&			\
	    (num_serving < maxthreads)) {			\
		is_creating = 1;				\
		MUTEX_UNLOCK(&auto_mutex);			\
		if (thr_create(NULL, am_thread_stack_size, auto_run_parallel,	\
			       NULL, THR_DETACHED, NULL) != 0) {		\
			syslog(LOG_ERR,	gettxt(":188",				\
			       "Thread %d: WARNING: cannot create thread: %s"),	\
			       myid, strerror(errno));				\
			MUTEX_LOCK(&auto_mutex);		\
		} else {					\
			MUTEX_LOCK(&auto_mutex);		\
			num_serving++;				\
		}						\
		is_creating = 0;				\
	}							\
}

main(argc, argv)
	int argc;
	char *argv[];
{
	SVCXPRT *xprt, *newxprt;
	int fd, newfd;
	struct t_bind *tres = NULL;
	struct t_info tinfo;
	extern void nfs_program_2();
	struct netconfig *nconf;
	struct nfs_args args;
	struct autodir *dir, *dir_next;
	int pid, bad;
	int command_line_maps = 0;
	int master_map_not_specified = 0;
	char *master_map = NULL;
	struct stat sb;
	struct knetconfig knconf;
	extern int trace;
	char pidbuf[64];
	struct stat stbuf;
	int num_fd, i;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxnfscmds");
	(void) setlabel("UX:automount");

	argc--;
	argv++;

	openlog("automount", LOG_PID | LOG_NOWAIT | LOG_CONS, LOG_DAEMON);
	(void) umask(0);
	(void) setbuf(stdout, (char *)NULL);
	(void) gethostname(self, sizeof self);
	(void) strcpy(tmpdir, "/tmp_mnt");

	(void) ns_setup();

	while (argc && argv[0][0] == '-') switch (argv[0][1]) {
	case 'n':
		nomounts++;
		argc--;
		argv++;
		break;
	case 'f':
		if ((argc < 2) || (argv[1][0] == '-')) {
			pfmt(stderr, MM_ERROR, ":170:bad master file name\n");
			usage();
		}
		master_map = argv[1];
		argc -= 2;
		argv += 2;
		break;
	case 'M':
		if ((argc < 2) || (argv[1][0] != '/')) {
			pfmt(stderr, MM_ERROR, ":171:bad mount directory\n");
			usage();
		}
		(void) strcpy(tmpdir, argv[1]);
		argc -= 2;
		argv += 2;
		break;
	case 't':	/* timeout */
		if (argc < 2) {
			pfmt(stderr, MM_ERROR, ":172:bad timeout value\n");
			usage();
		}
		if (argv[0][2]) {
			set_timeout(argv[0][2], atoi(argv[1]));
		} else {
			char *s;

			for (s = strtok(argv[1], ","); s;
			     s = strtok(NULL, ",")) {
				if (*(s+1) != '=') {
					pfmt(stderr, MM_ERROR, 
					     ":172:bad timeout value\n");
					usage();
				}
				set_timeout(*s, atoi(s+2));
			}
		}
		argc -= 2;
		argv += 2;
		break;
	case 'T':
		trace++;
		argc--;
		argv++;
		break;
	case 'D':
		if (argv[0][2])
			(void) putenv(&argv[0][2]);
		else {
			if (argc < 2) {
				pfmt(stderr, MM_ERROR,
				     ":174:bad environment variable assignment\n");
				usage();
			} else {
			
				(void) putenv(argv[1]);
				argc--;
				argv++;
			}
		}
		argc--;
		argv++;
		break;
	case 'v':
		verbose++;
		argc--;
		argv++;
		break;
	case 'i':
		if ((argc < 2) || (argv[1][0] == '-')) {
			pfmt(stderr, MM_ERROR,
			     ":253:bad minimum threads value\n");
			usage();
		}
		minthreads = atoi(argv[1]);
		if (minthreads < 0 || minthreads > TOT_THREADS) {
			pfmt(stderr, MM_WARNING,
			     ":254:invalid number of minimum threads, set to default %d\n", MIN_THREADS);
			minthreads = MIN_THREADS;
		}
		argc -= 2;
		argv += 2;
		break;
	case 'a':
		if ((argc < 2) || (argv[1][0] == '-')) {
			pfmt(stderr, MM_ERROR,
			     ":255:bad maximum threads value\n");
			usage();
		}
		maxthreads = atoi(argv[1]);
		if (maxthreads < 0 || maxthreads > TOT_THREADS) {
			pfmt(stderr, MM_WARNING,
			     ":256:invalid number of maximum threads, set to default %d\n", MAX_THREADS);
			maxthreads = MAX_THREADS;
		}
		argc -= 2;
		argv += 2;
		break;
	default:
		usage();
	}

	num_fd = maxthreads;

	/*
	 * Get process id and start time.
	 */
	right_pid = getpid();
	right_time = time((long *)0);

	/*
	 * Initialize all global locks.
	 */
	mutex_init(&host_mutex, USYNC_THREAD, NULL);
	mutex_init(&subnet_mutex, USYNC_THREAD, NULL);
	mutex_init(&nconf_mutex, USYNC_THREAD, NULL);	
	mutex_init(&dupreq_mutex, USYNC_THREAD, NULL);
	mutex_init(&lookup_mutex, USYNC_THREAD, NULL);
	mutex_init(&auto_mutex, USYNC_THREAD, NULL);
	mutex_init(&unmount_mutex, USYNC_THREAD, NULL);
	for (i = 0; i < CACHE_CL_SIZE; i++)
		 mutex_init(&cache_cl[i].mutex, USYNC_THREAD, NULL);

	cond_init(&unmount_cond, USYNC_THREAD, NULL);

	rwlock_init(&fh_rwlock, USYNC_THREAD, NULL);
	rwlock_init(&fsq_rwlock, USYNC_THREAD, NULL);
	rwlock_init(&tmpq_rwlock, USYNC_THREAD, NULL);

	/*
	 * Read /etc/mnttab and determine current mounts of system.
	 */
	read_mnttab();
	current_mounts = mkmntlist();
	if (current_mounts == NULL) {
		pfmt(stderr, MM_ERROR,
		     ":175:cannot establish current mounts\n");
		exit (1);
	}

	/*
	 * Get mountpoints and maps off the command line.
	 */
	while (argc >= 2) {
		command_line_maps = 1;
		if (argv[0][0] != '/') {
			pfmt(stderr, MM_ERROR,
			     ":176:directory must be full path name\n");
			usage();
		} else {
			if (argc >= 3 && argv[2][0] == '-') {
				dirinit(argv[0], argv[1], argv[2]+1, 0);
				argc -= 3;
				argv += 3;
			} else {
				dirinit(argv[0], argv[1], "rw", 0);
				argc -= 2;
				argv += 2;
			}
		}
	}

	if (argc)
		usage();

	if (getenv("CPU") == NULL) {
		char buf[16], str_cpu[32];
		int len;
		FILE *f;

		if ((f = popen("uname -p", "r")) != NULL) {
			(void) fgets(buf, 16, f);
			(void) pclose(f);
			if (len = strlen(buf))
				buf[len - 1] = '\0';
			(void) sprintf(str_cpu, "CPU=%s", buf);
			(void) putenv(str_cpu);
		}
	}
	
	if ((master_map == NULL) && (!command_line_maps)) {
		master_map_not_specified = 1;
		master_map = strdup(DEFAULT_MASTER_MAP);
		if (master_map == NULL) {
			pfmt(stderr, MM_ERROR, ":180:no memory\n");
			exit (1);
		}
	}		
	if (master_map != NULL)
		/* the third parameter is for type of map.
		 * 0 = master map, 1 = direct map
		 */
		(void) getmapbyname(master_map, "rw", 0);

	if (master_map_not_specified && master_map)
		free(master_map);

	/*
	 * Free the current mount list - don't need it anymore.
	 */
	freemntlist(current_mounts);

	/*
	 * Remove -null map entries.
	 */
	for (dir = HEAD(struct autodir, dir_q); dir; dir = dir_next) {
	    	dir_next = NEXT(struct autodir, dir);
		if (strcmp(dir->dir_map, "-null") == 0) {
			REMQUE(dir_q, dir);
			if (dir->dir_name) 
				free(dir->dir_name);
			if (dir->dir_map) 
				free(dir->dir_map);
			if (dir->dir_opts) 
				free(dir->dir_opts);
			free(dir);
		}
	}

	/*
	 * If there are no entries initialized, exit.
	 */
	if (HEAD(struct autodir, dir_q) == NULL) {
		pfmt(stderr, MM_ERROR, ":177:no map entries found\n");
		exit(1);
	}

	/*
	 * Register automount service with RPC.  Create arbitrary number
	 * of file descriptors (num_fd) and register service so that
	 * automounter can poll at more than one file descriptor.
	 * Set file descriptor to O_NONBLOCK so that threads do not block
	 * in getmsg() waiting for input.
	 */
	nconf = loopback_trans();
	if (nconf == (struct netconfig *) NULL) {
		pfmt(stderr, MM_ERROR, ":178:no transport available\n");
		exit(1);
	}

	fd = t_open(nconf->nc_device, O_RDWR, &tinfo);
        if (fd == -1) {
		pfmt(stderr, MM_ERROR, ":179:cannot open connection for %s\n", 
		     nconf->nc_netid);
		exit(1);
	}
	fcntl(fd, F_SETFL, O_NONBLOCK);

	tres = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR);
	if (tres == NULL) {
		pfmt(stderr, MM_ERROR, ":180:no memory\n");
		t_close(fd);
		exit(1);
        }
	tres->qlen = 8;		/* Chosen Arbitrarily */
        tres->addr.len = 0;

	xprt = svc_tli_create(fd, nconf, tres, 0, 0);
	if (xprt == (SVCXPRT *) NULL) {
		pfmt(stderr, MM_ERROR, ":181:cannot create server handle\n");
		t_close(fd);
		exit(1);
	}
	if (!svc_reg(xprt, NFS_PROGRAM, NFS_VERSION, nfs_program_2,
		     (struct netconfig *)0)) {
		pfmt(stderr, MM_ERROR, ":182:cannot register service\n");
		t_close(fd);
		exit(1);
	}

	for (i = 1; i < num_fd; i++) {
		if ((newfd = dup(fd)) == -1) {
			syslog(LOG_ERR, 
			       gettxt(":239", "cannot %s: %m"), "dup");
			break;
		}

		newxprt = svc_tli_create(newfd, nconf, tres, 0, 0);
		if (newxprt == (SVCXPRT *) NULL) {
			syslog(LOG_ERR,
			       gettxt(":183", "cannot create server handle"));
			break;
		}

		if (!svc_reg(newxprt, NFS_PROGRAM, NFS_VERSION, nfs_program_2,
			     (struct netconfig *)0)) {
			syslog(LOG_ERR,
			       gettxt(":184", "cannot register service"));
			break;
		}
	}
	t_free((char *) tres, T_BIND);

	if (mkdir_r(tmpdir) < 0) {
		pfmt(stderr, MM_ERROR, ":240:cannot create %s: %s\n",
		     tmpdir, strerror(errno));
		exit(1);
	}
	if (stat(tmpdir, &stbuf) < 0) {
		pfmt(stderr, MM_ERROR, ":241:cannot %s %s: %s\n",
		     "stat", tmpdir, strerror(errno));
		exit(1);
	}
	tmpdev = stbuf.st_dev;

	/*
	 *  Fork the automount daemon here
	 */
	switch (pid = fork()) {
	case -1:
		pfmt(stderr, MM_ERROR, ":40:%s failed: %s\n",
		     "fork", strerror(errno));
		exit(1);
	case 0:
		/* child */
		(void) setsid();
		sigset(SIGTERM, catch);
		sigset(SIGHUP,  check_mnttab);
		sigset(SIGCHLD, SIG_IGN); /* no zombies */
		auto_run();
		syslog(LOG_ERR, gettxt(":185", "auto_run returned"));
		exit(1);
	}

	/* parent */
	args.flags = NFSMNT_INT + NFSMNT_TIMEO +
		     NFSMNT_HOSTNAME + NFSMNT_RETRANS;
	args.addr = &xprt->xp_ltaddr;

	if (stat(nconf->nc_device, &sb) < 0) {
		pfmt(stderr, MM_ERROR, ":33:%s failed for %s: %s\n",
		     "stat", nconf->nc_device, strerror(errno));
		exit(1);
	}
	knconf.knc_semantics = nconf->nc_semantics;
	knconf.knc_protofmly = nconf->nc_protofmly;
	knconf.knc_proto = nconf->nc_proto;
	knconf.knc_rdev = sb.st_rdev;
	args.flags |= NFSMNT_KNCONF;
	args.knconf = &knconf;

	args.timeo = (mount_timeout + 5) * 10;
	args.retrans = 5;

	bad = 1;
	/*
	 * Mount the daemon at its mount points.
	 * Start at the end of the list because that's
	 * the first on the command line.
	 */
	for (dir = TAIL(struct autodir, dir_q); dir;
	     dir = PREV(struct autodir, dir)) {
		(void) sprintf(pidbuf, "(pid%d@%s)", pid, dir->dir_name);
		if (strlen(pidbuf) >= (size_t) HOSTNAMESZ-1)
			(void) strcpy(&pidbuf[HOSTNAMESZ-3], "..");
		args.hostname = pidbuf;
		args.fh = (caddr_t)&dir->dir_vnode.vn_fh; 

		if (mount("", dir->dir_name, MS_RDONLY | MS_DATA, MNTTYPE_NFS, 
			  &args, sizeof(args)) < 0) {
			syslog(LOG_ERR,
			       gettxt(":94", "%s: %s failed for %s: %m"),
			       "main", "mount", dir->dir_name);
		} else {
			/*
			 * If at least one mount is successful,
			 * don't kill the daemon.
			 */
			bad = 0;
		}
	}

	if (bad)
		(void) kill(pid, SIGKILL);
	exit(bad);
	/*NOTREACHED*/
}


void
set_timeout(letter, t)
	char letter ; int t;
{
	if (trace > 2)
		fprintf(stderr, "set_timeout(%c, %d)\n", letter, t);

	if (t <= 1) {
		pfmt(stderr, MM_ERROR, ":172:bad timeout value\n");
		usage();
	}
	switch (letter) {
	case 'm':
		mount_timeout = t;
		if (trace > 2)
			fprintf(stderr, "automount: mount_timeout = %d\n", t);
		break;
	case 'l':
		max_link_time = t;
		if (trace > 2)
			fprintf(stderr, "automount: max_link_time = %d\n", t);
		break;
	case 'w':
		maxwait = t;
		if (trace > 2)
			fprintf(stderr, "automount: maxwait = %d\n", t);
		break;
        case 'p':
		pingtime = t;
		if (trace > 2)
			fprintf(stderr, "automount: pingtime = %d\n", t);
		break;
	default:
		pfmt(stderr, MM_ERROR, ":173:bad timeout switch\n");
		usage();
		break;
	}
}


/*
 * Get a netconfig entry for loopback transport.
 */
struct netconfig *
loopback_trans()
{
#ifdef LOOPBACK
	struct netconfig *nconf;
	NCONF_HANDLE *nc;

	nc = setnetconfig();
	if (nc == NULL)
		return (NULL);

	while (nconf = getnetconfig(nc)) {
		if (nconf->nc_flag & NC_VISIBLE &&
		    nconf->nc_semantics == NC_TPI_CLTS &&
		    strcmp(nconf->nc_protofmly, NC_LOOPBACK) == 0) {
			nconf = getnetconfigent(nconf->nc_netid);
			break;
		}
	}

	endnetconfig(nc);
	return (nconf);
#else
	/* XXX
	 * For some reason a loopback mount will not work
	 * with "ticlts". It does work with "udp" so
	 * until the "ticlts" problem is sorted out
	 * stick with udp loopback.
	 */
	return (getnetconfigent("udp"));
#endif
}

jmp_buf	jmp_env;
struct pollfd	pollset[FD_SETSIZE];
int		svc_workmap[FD_SETSIZE];
sigset_t	set;
int	num_serving = 1;
int	num_in_dispatch = 0;
int	is_creating = 0;
int	is_timeout = 0;
time_t	time_timeout;
int	dtbsize;
int	poll_timeout = 60 * 1000;
int	wait_timeout = 60;

/*
 * catch() is the SIGTERM signal handler.
 */
void
catch()
{
	longjmp(jmp_env, 1);
}


/*
 * Description:
 *	Cleans up by unmounting automounter and real
 *	mountpoints before exiting.
 * Call From:
 *	catch (through longjmp from the THR_BOUND thread)
 * Entry/Exit:
 *	No locks held on entry or exit.
 */
void
windup()
{
	struct autodir *dir, *next_dir;
	struct filsys *fs, *fs_next;
	struct stat stbuf;
	struct fattr *fa;
	int retry, remove;
	extern rwlock_t fh_rwlock;
	thread_t myid = thr_self();	

	if (trace > 1)
		fprintf(stderr, "%d: windup: received SIGTERM\n", myid);

	/*
	 * The automounter has received a SIGTERM.
	 * Set number of threads to one so that no new
	 * threads can be created.  Increment is_timeout
	 * so that polling threads do no execute the do_timeout()
	 * routine.
	 */
	MUTEX_LOCK(&auto_mutex);
	is_timeout++;
	minthreads = maxthreads = 1;
	MUTEX_UNLOCK(&auto_mutex);

	/*
	 *  Check for symlink mount points and change them
	 *  into directories to prevent the umount system
	 *  call from following them.
	 */
	RW_WRLOCK(&fh_rwlock);
	for (dir = HEAD(struct autodir, dir_q); dir;
	     dir = NEXT(struct autodir, dir)) {
		RW_WRLOCK(&dir->dir_rwlock);
		if (dir->dir_vnode.vn_type == VN_LINK) {
			dir->dir_vnode.vn_type = VN_DIR;
			fa = &dir->dir_vnode.vn_fattr;
			fa->type = NFDIR;
			fa->mode = NFSMODE_DIR + 0555;
		} else {
			dir->dir_vnode.vn_valid = 0;
		}
		RW_UNLOCK(&dir->dir_rwlock);
	}
	RW_UNLOCK(&fh_rwlock);

	/*
	 * Unmount all automounter mount points.
	 */
tryagain:
	retry = 0;
	for (dir = HEAD(struct autodir, dir_q); dir; dir = next_dir) {
		next_dir = NEXT(struct autodir, dir);

		/*  
		 *  This lstat is a kludge to force the kernel
		 *  to flush the attr cache.  Make sure the
		 *  link does become a directory (S_IFDIR).
		 *  If it was a direct mount point (symlink) 
		 *  the kernel needs to do a getattr to find 
		 *  out that it has changed back into a directory.
		 */
		RW_WRLOCK(&dir->dir_rwlock);
		if (dir->dir_vnode.vn_valid) {
			if (lstat(dir->dir_name, &stbuf) < 0) {
				syslog(LOG_ERR,
				       gettxt(":94", "%s: %s failed for %s: %m"),
				       "windup", "lstat", dir->dir_name);
			}
			while ((stbuf.st_mode & S_IFMT) != S_IFDIR) {
				sleep(1);
				if (lstat(dir->dir_name, &stbuf) < 0)
					break;
			}
		}

		dir->dir_vnode.vn_valid = 0;

		if (umount(dir->dir_name) < 0) {
			syslog(LOG_ERR,
			       gettxt(":94", "%s: %s failed for %s: %m"),
			       "windup", "umount", dir->dir_name);
			retry++;
		} else {
			if (dir->dir_remove)
				safe_rmdir(dir->dir_name);
			REMQUE(dir_q, dir);
		}
		RW_UNLOCK(&dir->dir_rwlock);
	}

	if (retry) {
		sleep(1);
		goto tryagain;
	}

	/*
	 * Unmount all the real mount points.
	 */
	remove = 1;
	while (remove) {
		remove = 0;
		RW_WRLOCK(&fsq_rwlock);
		RW_WRLOCK(&tmpq_rwlock);
		tmpq.q_head = tmpq.q_tail = NULL;
		for (fs = HEAD(struct filsys, fs_q); fs; fs = fs_next) {
			fs_next = NEXT(struct filsys, fs);

			if (fs->fs_mine && fs == fs->fs_rootfs) {
				remove = 1;
				do_remove(fs);
				break;
			}
		}
		RW_UNLOCK(&tmpq_rwlock);
		RW_UNLOCK(&fsq_rwlock);

		if (remove)
			do_unmount(EXIT);
	}

	syslog(LOG_ERR, gettxt(":187", "exiting"));
	exit(0);
	/*NOTREACHED*/
}


/*
 * Description:
 *	Sets concurrency and spawns off THR_BOUND thread.
 * Call From:
 *	main 
 * Entry/Exit:
 *	No locks held on entry or exit.
 */
auto_run()
{
	/*
	 * Only the thread running auto_run_main will capture and
	 * handle these signals.
	 * Any threads created in auto_run_parallel will ignore them.
	 */
	sigemptyset(&set);
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGTERM);

	dtbsize = _rpc_dtbsize();

	time_now = time((long *)0);
	time_timeout = time_now + maxwait;

	/*
	 * Set number of LWPS used for multiplexed threads.
	 */
	if (thr_setconcurrency(maxthreads) != 0)
		pfmt(stderr, MM_WARNING, ":40:%s failed: %s\n",
		     "thr_setconcurrency", strerror(errno));

        /*
         * Create a THR_BOUND thread to catch signals safely.
         */
	thr_create(NULL, am_thread_stack_size, auto_run_main,
		   NULL, THR_BOUND, NULL);

	thr_exit(NULL);
	/* NOTREACHED */
}


/*
 * Description:
 *	This is a THR_BOUND sleeping thread that justs updates
 *	time_now.  It is the only thread that catches signals
 *	SIGTERM and SIGHUP.
 * Call From:
 *	spawn off by auto_run 
 * Entry/Exit:
 *	No locks held on entry or exit.
 */
/* ARGSUSED */
void *
auto_run_main(arg)
	void *arg;
{
	/*
	 * Set longjump for SIGTERM catch.
	 */
	if (setjmp(jmp_env)) {
		windup();
	}

        /*
         * Create threads to poll for requests.
         */
        thr_create(NULL, am_thread_stack_size, auto_run_parallel, 
		   NULL, THR_DETACHED, NULL);

	for (;;) {
		time_now = time((long *)0);
		sleep(60);
	}
}


/*
 * Description:
 *	These are THR_DETACHED polling threads that polls for 
 *	requests and services them.  It spawns off another
 *	polling thread if all other threads are busy.
 * Call From:
 *	first thread created by auto_run_main, others are created
 *	by auto_run_parallel itself
 * Entry/Exit:
 *	No locks held on entry or exit.
 */
/* ARGSUSED */
void *
auto_run_parallel(arg)
	void *arg;
{
	int		read_fds, n;
	int		i, fds_found;
	thread_t	myid = thr_self();

	/*
	 * Block signals SIGHUP and SIGTERM.
	 */
	thr_sigsetmask(SIG_BLOCK, &set, NULL);

	if (trace > 1)
		fprintf(stderr, "%d: num_serving=%d\n", myid, num_serving);

	/*
	 * Loop to poll for requests, work on a request, or do_timeouts.
	 */
	MUTEX_LOCK(&auto_mutex);
	for (;;) {
		read_fds = _rpc_select_to_poll(dtbsize, &svc_fdset, pollset);
		MUTEX_UNLOCK(&auto_mutex);

		n = poll(pollset, read_fds, poll_timeout);

		switch (n) {
		case -1:
		case 0:
			MUTEX_LOCK(&auto_mutex);
                        if ((is_timeout == 0) && (time_timeout <= time_now)) {
                                is_timeout++;
                                MUTEX_UNLOCK(&auto_mutex);
                                do_timeouts();
                                MUTEX_LOCK(&auto_mutex);
				time_now = time((long *)0);
                                time_timeout = time_now + wait_timeout;
                                is_timeout--;
                        }
			if (num_serving > minthreads) {
                                num_serving--;
				MUTEX_UNLOCK(&auto_mutex);
				thr_exit(NULL);
			}
			continue;
		default:
			MUTEX_LOCK(&auto_mutex);
			fds_found = 0;
			for (i = 0; i < dtbsize && fds_found == 0; i++) {
				register struct pollfd *p = &pollset[i];
				
				if (svc_workmap[i] == 0 && p->revents) {
					fds_found++;
					
					if (p->revents & POLLNVAL) {
						FD_CLR(p->fd, &svc_fdset);
					} else {
						svc_workmap[i] = 1;
						num_in_dispatch++;
						SPAWN_THREAD();
						MUTEX_UNLOCK(&auto_mutex);
						svc_getreq_common(p->fd);
						MUTEX_LOCK(&auto_mutex);
						num_in_dispatch--;
						svc_workmap[i] = 0;
					}
				}
			}
		}
	}
}

usage()
{
	pfmt(stderr, MM_ACTION, ":247:\n\tUsage: automount [-nTv] [-t sub_options] [-M directory] \n\t\t[-D n=s] [-f file] [ dir map [-mount_options] ]\n");
	exit(1);
	/*NOTREACHED*/
}
