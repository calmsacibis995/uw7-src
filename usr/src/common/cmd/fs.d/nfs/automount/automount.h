/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)automount.h	1.2"
#ident	"$Header$"

#include <synch.h>

#define MXHOSTNAMELEN	64
#define MAXNETNAMELEN	255
#define MAXFILENAMELEN	255
#define CACHE_CL_SIZE	10

#define	FH_HASH_SIZE	8
#define MNTTYPE_NFS	"nfs"

/*
 * General queue structure 
 */
struct q {
	struct q	*q_next;
#define	q_head	q_next
	struct q	*q_prev;
#define	q_tail	q_prev
};

#define	INSQUE(head, ptr) my_insque(&(head), &(ptr)->q)
#define	REMQUE(head, ptr) my_remque(&(head), &(ptr)->q)
#define HEAD(type, head) ((type *)(head.q_head))
#define NEXT(type, ptr)	((type *)(ptr->q.q_next))
#define	TAIL(type, head) ((type *)(head.q_tail))
#define PREV(type, ptr)	((type *)(ptr->q.q_prev))
	
/*
 * Types of filesystem entities (vnodes)
 * We support only one level of DIR; everything else is a symbolic LINK
 */
enum vn_type { VN_DIR, VN_LINK};

struct avnode {
	struct q q;
	mutex_t		vn_mutex;	/* lock to protect struct's variables */
	int		vn_valid;	/* is this avnode valid? */
	int		vn_count;	/* reference count */
	nfs_fh		vn_fh;		/* fhandle */
	struct fattr	vn_fattr;	/* file attributes */
	enum vn_type	vn_type;	/* type of avnode */
	caddr_t		vn_data;	/* avnode private data */
};
struct avnode *fhtovn();		/* lookup avnode given fhandle */

/*
 * Structure describing a host/filesystem/dir tuple in a NIS map entry
 */
struct mapfs {
	struct mapfs	*mfs_next;	/* next in entry */
	int 	mfs_ignore;		/* ignore this entry */
	char	*mfs_host;		/* host name */
	char	*mfs_dir;		/* dir to mount */
	char	*mfs_subdir;		/* subdir of dir */
	int     mfs_penalty;            /* penalty for mounting from this host */
};

/*
 * NIS entry - lookup of name in DIR gets us this
 */
struct mapent {
	char	*map_root;
	char	*map_mntpnt;
	char	*map_mntopts;
	struct mapfs	*map_fs;
	struct mapent	*map_next;
};
struct mapent *getmapent();

/*
 * Everthing we know about a mounted filesystem
 * Can include things not mounted by us (fs_mine == 0)
 */
struct filsys {
	struct q q;			/* next in q */
	mutex_t  fs_mutex;		/* lock to protect struct's variables */
	int	fs_death;		/* time when no longer valid */
	int	fs_mine;		/* 1 if we mounted this fs */
	int	fs_present;		/* for checking unmounts */
	int 	fs_unmounted;		/* 1 if unmounted OK */
	char	*fs_type;		/* type of filesystem */
	char	*fs_host;		/* host name */
	char	*fs_dir;		/* dir of host mounted */
	char	*fs_mntpnt;		/* local mount point */
	char	*fs_opts;		/* mount options */
	dev_t	fs_mntpntdev;		/* device of mntpnt */
	dev_t	fs_mountdev;		/* device of mount */
	struct nfs_args	fs_nfsargs;	/* nfs mount args */
	struct filsys	*fs_rootfs;	/* root for this hierarchy */
	nfs_fh	fs_rootfh;		/* file handle for nfs mount */
	int	fs_mflags;		/* mount flags */
};
struct q fs_q;
struct q tmpq;
struct filsys *already_mounted(), *alloc_fs();

/*
 * Structure for recently referenced links
 */
struct link {
	struct q q;			/* next in q */
	struct avnode	link_vnode;	/* space for avnode */
	struct autodir	*link_dir;	/* dir which we are part of */
	char		*link_name;	/* this name in dir */
	struct filsys	*link_fs;	/* mounted file system */
	char		*link_path;	/* dir within file system */
	long		link_death;	/* time when no longer valid */
};
struct link *makelink();
struct link *findlink();

/*
 * Descriptor for each directory served by the automounter 
 */
struct autodir {
	struct q q;			/* next in q */
	rwlock_t	dir_rwlock;	/* lock to protect dir_head changes */
	struct avnode	dir_vnode;	/* avnode */
	char		*dir_name;	/* mount point */
	char		*dir_map;	/* name of map for dir */
	char		*dir_opts;	/* default mount options */
	int		dir_remove;	/* remove mount point */
	struct q	dir_head;	/* queue of links for this autodir */
};
struct q dir_q;

/*
 * This structure is used to build a list of
 * mnttab structures from /etc/mnttab.
 */
struct mntlist {
	struct mnttab  *mntl_mnt;
	struct mntlist *mntl_next;
};

/*
 * This structure is used to build an array of
 * hostnames with associated penalties to be
 * passed to the nfs_cast procedure
 */
struct host_names {
	char *host;
	int  penalty;
};

/*
 * Structure to hold cache client handles
 */
struct cache_client {
	char	host[MXHOSTNAMELEN+1];
	int	in_use;
	time_t	time_valid;
	mutex_t	mutex;
	CLIENT	*cl;
};

char self[64];		/* my hostname */
char tmpdir[200];	/* real name of /tmp */

time_t time_now;	/* try to set every 60 seconds */
int mount_timeout;	/* max seconds to wait for mount */
int max_link_time;	/* seconds to keep link around */
int nomounts;		/* don't do any mounts - for cautious servers */
nfsstat lookup(), nfsmount();

/*
 * Definitions of NFS specific flags
 */
#define MNTOPT_SUID	"suid"		/* Set uid allowed */
#define MNTOPT_INTR	"intr"		/* Allow NFS ops to be interrupted */
#define MNTOPT_NOINTR	"nointr"	/* Don't allow interrupted ops */
#define MNTOPT_PORT	"port"		/* NFS server IP port number */
#define MNTOPT_SECURE	"secure"	/* Secure (AUTH_DES) mounting */
#define MNTOPT_KERB	"kerberos"	/* Secure (AUTH_Kerb) mounting */
#define MNTOPT_RSIZE	"rsize"		/* Max NFS read size (bytes) */
#define MNTOPT_WSIZE	"wsize"		/* Max NFS write size (bytes) */
#define MNTOPT_TIMEO	"timeo"		/* NFS timeout (1/10 sec) */
#define MNTOPT_RETRANS	"retrans"	/* Max retransmissions (soft mnts) */
#define MNTOPT_ACTIMEO	"actimeo"	/* Attr cache timeout (sec) */
#define MNTOPT_ACREGMIN	"acregmin"	/* Min attr cache timeout (files) */
#define MNTOPT_ACREGMAX	"acregmax"	/* Max attr cache timeout (files) */
#define MNTOPT_ACDIRMIN	"acdirmin"	/* Min attr cache timeout (dirs) */
#define MNTOPT_ACDIRMAX	"acdirmax"	/* Max attr cache timeout (dirs) */
#define MNTOPT_NOAC	"noac"		/* Don't cache attributes at all */
#define MNTOPT_NOCTO	"nocto"		/* No close-to-open consistency */
#define MNTOPT_BG	"bg"		/* Do mount retries in background */
#define MNTOPT_FG	"fg"		/* Do mount retries in foreground */
#define MNTOPT_RETRY	"retry"		/* Number of mount retries */
#define MNTOPT_DEV	"dev"		/* Device id of mounted fs */
#define MNTOPT_MAP	"map"		/* Automount map */
#define MNTOPT_DIRECT	"direct"	/* Automount   direct map mount */
#define MNTOPT_INDIRECT	"indirect"	/* Automount indirect map mount */
#define MNTOPT_IGNORE	"ignore"	/* Ignore this entry */

#define	CONT	0
#define	EXIT	1

#define	WORK	0
#define	PASS	1
#define	FAIL	2

#define ZERO_LINK(link) {			\
	link->link_death = 0;			\
	link->link_fs = (struct filsys *)0;	\
	}

#ifdef DEBUG

#define	MUTEX_LOCK(mutex) {						\
	fprintf(stderr, "%d: Locking Mutex %x ... \n", myid, mutex);	\
	mutex_lock(mutex);						\
	fprintf(stderr, "%d: Locked Mutex %x\n", myid, mutex);		\
	}

#define	MUTEX_UNLOCK(mutex) {						\
	fprintf(stderr, "%d: Unlocking Mutex %x ... \n", myid, mutex);	\
	mutex_unlock(mutex);						\
	fprintf(stderr, "%d: Unlocked Mutex %x\n", myid, mutex);	\
	}

#define	RW_RDLOCK(lock) {						\
	fprintf(stderr, "%d: Read Locking %x ... \n", myid, lock);	\
	rw_rdlock(lock);						\
	fprintf(stderr, "%d: Read Locked %x\n", myid, lock);		\
	}

#define	RW_WRLOCK(lock) {						\
	fprintf(stderr, "%d: Write Locking %x ... \n", myid, lock);	\
	rw_wrlock(lock);						\
	fprintf(stderr, "%d: Write Locked %x\n", myid, lock);		\
	}

#define	RW_UNLOCK(lock) {						\
	fprintf(stderr, "%d: Unlocking Lock %x ... \n", myid, lock);	\
	rw_unlock(lock);						\
	fprintf(stderr, "%d: Unlocked Lock %x\n", myid, lock);		\
	}

#else

#define	MUTEX_LOCK	mutex_lock
#define	MUTEX_UNLOCK	mutex_unlock
#define	RW_RDLOCK	rw_rdlock
#define	RW_WRLOCK	rw_wrlock
#define	RW_UNLOCK	rw_unlock

#endif

