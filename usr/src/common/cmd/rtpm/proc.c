#ident	"@(#)rtpm:proc.c	1.9.1.1"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <ftw.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/pid.h>
#include <sys/class.h>
#include <sys/procfs.h>
#include <sys/cred.h>
#include <sys/exec.h>
#include <sys/engine.h>
#include <sys/clock.h>
#include <sys/dl.h>
#include <sys/ksym.h>
#include <vm/as.h>
#include <locale.h>
#include <deflt.h>
#include <pfmt.h>
#include <sys/procset.h>
#include <sys/bind.h>
#include <sys/mman.h>
#include <sys/immu.h>
#undef ERR
#include <curses.h>
#include <signal.h>
#include <setjmp.h>


#ifdef DEBUG
#include <assert.h>
#else
#define assert(x)
#endif
#ifndef TRUE
#define	TRUE	1
#define	FALSE	0
#endif

#ifndef BILLION
#define BILLION 1000000000
#endif

#define SANITY_CHECKS


#ifdef SANITY_CHECKS
#define SANCHECK(x)	(x)
#else
#define SANCHECK(x)	(1)	/* always succeed */
#endif


#include <mas.h>
#include <metreg.h>
#include "rtpm.h"

/*
 * flag to indicate whether curses has been started
 */
extern int in_curses;

extern int scr_rows, scr_cols;

/*
 * metric descriptor ret from mas_open
 */

extern int md;

#if 0
#ifndef LOGNAME_MAX
#define LOGNAME_MAX 32	/* max number of chars in login that will be printed */
#endif
#endif

#define MAX(x,y) ((x) > (y) ? (x) : (y))

/* * Macros for use with mmap, KADDR(p) converts a user address to a 
 * kernel address, UADDR(p) converts a kernel address to a user address
 */


#define UADDR(p,size,loc) ((p == 0 ? (void *) 0 : convert_address((unsigned long) p, size, loc)))  

/* chunk size of proc_list allocations */

#define INC_SIZE	100

/* Structure for storing user info */

struct udata {
	uid_t	uid;		/* numeric user id */
	char	name[LOGNAME_MAX]; /* login name, may not be null terminated */
};

typedef struct _pinfo {
	uid_t	uid;   
	dev_t	ttydev;
	pid_t	pid;   
	long	size;  
	char	psargs[PRARGSZ]; 
	ulong_t	nlwp; 
	timestruc_t start;
	char	lwp_sname;  
	lwpid_t	lwp_lwpid;  
	processorid_t lwp_onpro;
	int lwp_pri;
	float	lwp_seconds;
	char lwp_name[PRFNSZ];  
	char lwp_clname[8];	
} pinfo_t;

/* udata and devl granularity for structure allocation */
#define UDQ	50

/* Pointer to user data */
struct udata *ud;
int	nud = 0;	/* number of valid ud structures */
int	maxud = 0;	/* number of ud's allocated */

pinfo_t *info_p;	/* process information structure from /proc */


void getdev();

static int	read_one_lwp(struct lwp *lwp, pinfo_t *p);
static void	map_page(int sig, siginfo_t *siginfo, ucontext_t *context);
static lwp_t 	*prchoose(proc_t *p, caddr_t kp);
static struct proc_list *get_list_node();
static struct proc_list *allocate_list_nodes();
static void	release_list_node(struct proc_list *p);
static char	*gettty();
static void	clean_proc();
static void	read_lwps(struct lwp *first_lwp, caddr_t kfirst_lwp, 
			  proc_t *p, caddr_t kp);
static void	print_proc_hdr(int  row, int col );
static void	print_proc_entry( struct proc_list *pl_p, int row, int col );
static void 	prcom( struct proc_list *pl_p, uid_t puid, int row, int col );
static int 	psread(int fd, char *bp, unsigned int bs);
static int 	getunam(uid_t puid);

/*
 * /etc/ps_data stores information for quick access by ps
 * and whodo.
 */

int	ndev;			/* number of devices */
int	maxdev;			/* number of devl structures allocated */

#define USER_ONLY	1
#define SYS_ONLY	2

int print_proc_flags = 0;
int ps_sorted = 0;
int psscroll = 0;
int pscanscroll = 0;
int pscnt = 0;
int psrow = 0;
int pscol = 0;

#define DNSIZE	14
struct devl {			/* device list	 */
	char	dname[DNSIZE];	/* device name	 */
	dev_t	dev;		/* device number */
} *devl;



struct proc_list {
	struct proc_list *nxt;	
	int inuse;
	int print_me;
	float savetime;
	float tdiff;
	pinfo_t pinfo;
} proot;   /* dummy start node to eliminate special cases */

struct proc_list *proc_freelist;

#define NFD	17

extern int need_header;

struct lwpstat lwp_count = { 0,0,0,0,0,0,0,0,0 };


/* 
 * Defines and structure for the needed kernel symbols.
 */

#define CLASS 0
#define NCLASS 1
#define PRACTIVE 2
#define KSTART 3
#define ENGINE 4
#define TIMER 5

struct {
	char *name;
	unsigned long value;
} kaddr[] = {
	{ "class", 0 },
	{ "nclass", 0 },
	{ "practive", 0 },
	{ "_start", 0 },
	{ "engine", 0 },
	{ "timer_resolution", 0 },
};
#define KADDR_SIZE  (sizeof(kaddr)/sizeof(*kaddr))

static proc_t **pp_firstproc;
static class_t	*class_table;
static struct engine *engine;	/* engine array base */
static int timer_resolution;
static int km;
static caddr_t	mapbase;
static int numclass;

static jmp_buf jmp_env;
static int jmp_count;

static int maxproc;

static void sortps( void );


/* structs for use with read() */

static proc_t proc_buf;
static proc_t proc_next_buf;
static proc_t proc_next_prev_buf;
static struct pid pid_buf;
static struct as as_buf;
static struct execinfo execinfo_buf;
static struct cred cred_buf;
static struct lwp lwp_buf;
static struct lwp lwp_next_buf;
static struct lwp lwp_next_prev_buf;
static char clname_buf[PRCLSZ];
static char lname_buf[PRCLSZ];

static int max_struct_size = 0;

typedef struct map_info {
  unsigned long kernel_page_addr;
  caddr_t user_page_addr;

  struct map_info *bucket_next;
  struct map_info *bucket_prev;
  struct map_info *age_next;
  struct map_info *age_prev;
} map_info_t;


#define HASH_SIZE 1001
#define MAX_MAPPED_PAGES 500

map_info_t *map_table[HASH_SIZE];

map_info_t agelist_start;
map_info_t agelist_end;

int page_count = 0;

caddr_t convert_address(unsigned long p, unsigned long size, void *loc );
map_info_t *get_map_info(unsigned long page_start);
void put_map_info(map_info_t *mi);
map_info_t *remove_map_info(map_info_t *mi);


caddr_t
convert_address(unsigned long p, unsigned long size, void *loc) {
  unsigned long offset = p % NBPP;
  unsigned long page_start = p - offset;
  caddr_t addr;

  map_info_t *mi = get_map_info(page_start);

  if (!mi) {
    if (page_count >= MAX_MAPPED_PAGES) {
      mi = remove_map_info(agelist_start.age_next);
    }
    else {
      mi = malloc(sizeof(map_info_t));

      if (!mi) {
	if (in_curses) {
	  endwin();
	}
	fprintf(stderr, gettxt("RTPM:7", "out of memory\n"));
	exit( 1 );
      }
    }

    mi->kernel_page_addr = page_start;
    addr = mmap(0, (2 + (max_struct_size/NBPP)) * NBPP, PROT_READ, 
		MAP_SHARED, km, page_start);

    if (addr == (caddr_t) -1) {
      free(mi);

      if (loc == 0) {
	return 0;
      }

      lseek(km, p, SEEK_SET);
      read(km, loc, size);

      return loc;
    }
    mi->user_page_addr = (void *) addr;

    put_map_info(mi);
  }

  return mi->user_page_addr + offset;
}


map_info_t *
get_map_info(unsigned long page_start)
{
  unsigned long index = page_start % HASH_SIZE;
  map_info_t *curr = map_table[index];

  while (curr) {
    if (curr->kernel_page_addr == page_start) {
      /* move to end of age list */
      curr->age_prev->age_next = curr->age_next;
      curr->age_next->age_prev = curr->age_prev;
      curr->age_next = &agelist_end;
      curr->age_prev = agelist_end.age_prev;
      curr->age_prev->age_next = curr;
      curr->age_next->age_prev = curr;
      
      return curr;
    }
    curr = curr->bucket_next;
  }
  
  return 0;
}


void
put_map_info(map_info_t *mi) 
{
  int index = mi->kernel_page_addr % HASH_SIZE;

  mi->bucket_prev = 0;
  mi->bucket_next = map_table[index];

  if (map_table[index]) {
    map_table[index]->bucket_prev = mi;
  }
  map_table[index] = mi;

  mi->age_prev = agelist_end.age_prev;

  mi->age_next = &agelist_end;
  mi->age_prev->age_next = mi;
  agelist_end.age_prev = mi;

  page_count++;
}


map_info_t *
remove_map_info(map_info_t *mi)
{
  int index = mi->kernel_page_addr % HASH_SIZE;

  if (mi->bucket_prev) {
    mi->bucket_prev->bucket_next = mi->bucket_next;
  }
  else {
    map_table[index] = mi->bucket_next;
  }

  if (mi->bucket_next) {
    mi->bucket_next->bucket_prev = mi->bucket_prev;
  }

  mi->age_next->age_prev = mi->age_prev;
  mi->age_prev->age_next = mi->age_next;
  
  page_count--;

  return mi;
}



int
initproc(void) {	
	int	i;
	extern int errno;

	proot.nxt = NULL;

	max_struct_size = sizeof(proc_t);
	max_struct_size = MAX(max_struct_size, sizeof(struct pid));
	max_struct_size = MAX(max_struct_size, sizeof(struct as));
	max_struct_size = MAX(max_struct_size, sizeof(struct execinfo));
	max_struct_size = MAX(max_struct_size, sizeof(struct cred));
	max_struct_size = MAX(max_struct_size, sizeof(struct lwp));
	max_struct_size = MAX(max_struct_size, sizeof(lname_buf));
	
	agelist_start.age_next = &agelist_end;
	agelist_end.age_prev = &agelist_start;

	/* cast through (void *) to keep lint happy */
	maxproc = *((int *) (void *) mas_get_met(md, PROCMAX, 0, 0));

	proc_freelist = malloc(3 * maxproc * sizeof(struct proc_list));	

	if (!proc_freelist) {
	  if (in_curses) {
	    endwin();
	  }
	  fprintf(stderr, gettxt("RTPM:7", "out of memory\n"));
	  exit( 1 );
	}

	for (i = 0; i < 3 * maxproc - 1; i++) {
		proc_freelist[i].nxt = proc_freelist + (i + 1);
	}
	proc_freelist[3 * maxproc - 1].nxt = 0;	

	for (i = 0; i < KADDR_SIZE; i++) {
		unsigned long type;
		if (getksym(kaddr[i].name, &kaddr[i].value, &type) == -1 &&
		    errno == ENOMATCH) {
			return(1);
		}
	}

	if ((km = open("/dev/kmem", 0)) == -1) {
		return(2);
	}

	pp_firstproc = (proc_t **) UADDR(kaddr[PRACTIVE].value, 0, 0);

	class_table = (class_t *) UADDR(kaddr[CLASS].value, 0, 0);
	engine = *((struct engine **) UADDR(kaddr[ENGINE].value, 0, 0));
	timer_resolution = *((int *) UADDR(kaddr[TIMER].value, 0, 0));
	numclass = *((int *) UADDR(kaddr[NCLASS].value, 0, 0));

	return(0);
}


set_ps_option_user() {
	if( print_proc_flags == USER_ONLY )
		return(0);
	print_proc_flags = USER_ONLY;
	ps_sorted = 0;
	sortps();
	return(1);
}

set_ps_option_sys() {
	if( print_proc_flags == SYS_ONLY )
		return(0);
	print_proc_flags = SYS_ONLY;
	ps_sorted = 0;
	sortps();
	return(1);
}

set_ps_option_all() {
	if( !print_proc_flags )
		return(0);
	print_proc_flags = 0;
	ps_sorted = 0;
	sortps();
	return(1);
}

/*
 * insert - add entries to list of procs / lwps
 */
int
insert( pinfo_t *pinfo )
{
	struct proc_list *proc_p, *ptmp_p;
	int foundit;

	foundit = 0;

	for (proc_p = &proot; proc_p->nxt != 0; proc_p = proc_p->nxt) {
		if( proc_p->nxt->pinfo.pid < pinfo->pid ||
		   (proc_p->nxt->pinfo.pid == pinfo->pid && 
		    proc_p->nxt->pinfo.lwp_lwpid < pinfo->lwp_lwpid)) {
			continue;
		}

/*
 *		check both pid and start time, since it's 
 *		possible the pid could have been re-used 
 *		since we last looked
 */
		if( proc_p->nxt->pinfo.pid == pinfo->pid
		   && proc_p->nxt->pinfo.lwp_lwpid
		   == pinfo->lwp_lwpid
		   && proc_p->nxt->pinfo.start.tv_sec 
		   == pinfo->start.tv_sec
		   && proc_p->nxt->pinfo.start.tv_nsec
		   == pinfo->start.tv_nsec ) {
			foundit = 1;
		}
		break;
	}
	if( !foundit ) {
/*
 *		didn't find it, add a new one
 */
		ptmp_p = get_list_node();
		ptmp_p->nxt = proc_p->nxt;
		proc_p->nxt = ptmp_p;
		ptmp_p->savetime = 0.0;
		ptmp_p->inuse = 0;
		ptmp_p->print_me = 0;
	}
	proc_p = proc_p->nxt;
	memcpy( &(proc_p->pinfo), pinfo, sizeof( pinfo_t ));
	if( !proc_p->inuse ) {
		proc_p->tdiff = pinfo->lwp_seconds - proc_p->savetime;

		if( proc_p->tdiff > 0.0 ) {
			proc_p->tdiff += 0.005;	/* round up by 5 ms */
		}
		proc_p->savetime = pinfo->lwp_seconds;

		switch( pinfo->lwp_sname ) {
		case 'S':	lwp_count.sleep++; break;
		case 'R':	lwp_count.run++; break;
		case 'Z':	lwp_count.zombie++; break;
		case 'T':	lwp_count.stop++; break;
		case 'I':	lwp_count.idle++; break;
		case 'O':	lwp_count.onproc++; break;
		default:	lwp_count.other++; break;
		}
		lwp_count.total++;
	}
	proc_p->inuse = 1;

	return(0);
}


/*
 * read_proc: walk down the list of active processes and build 
 * a pinfo structure.
 */

int 
read_proc() {
	proc_t	*p;
	caddr_t kp;

	clean_proc();
	memset((char *) &lwp_count, 0, sizeof(lwp_count));

	kp = (caddr_t) *pp_firstproc;
	p = (proc_t *) UADDR(*pp_firstproc, sizeof(proc_buf), &proc_buf);

	while (p != 0) {
		pinfo_t ps;
		lwp_t	*p_lwp;
		struct pid *p_pid = UADDR(p->p_pidp, sizeof(pid_buf), 
					  &pid_buf);
		struct as *p_as = UADDR(p->p_as, sizeof(as_buf), &as_buf);
		struct execinfo *ei = UADDR(p->p_execinfo, 
					    sizeof(execinfo_buf),
					    &execinfo_buf);
					    
		struct cred *cred = UADDR(p->p_cred, sizeof(cred_buf),
					  &cred_buf);
		proc_t *next_p;
		caddr_t knext_p;

		memset((char *) &ps, 0, sizeof(ps));

		ps.nlwp = p->p_nlwp;

		ps.pid = p_pid->pid_id;
		ps.size = (p_as == 0) ? 0 : p_as->a_size/PAGESIZE;
		ps.start = p->p_start;
		ps.ttydev = p->p_cttydev;
		
		if (cred != 0) {
			ps.uid = cred->cr_uid;
		}

		if (ei != 0)  {
			memcpy(&ps.psargs, ei->ei_psargs, 
			      min(PSARGSZ, PRARGSZ-1));
		}

		if (p->p_flag & P_DESTROY) {
			/* Process is about to die */
			ps.lwp_sname = 'Z';
			ps.lwp_lwpid = 0;
		}
		else if ((p_lwp = prchoose(p, kp)) == 0) {
			/* Process isn't fully created */
			ps.lwp_sname = 'I';
			ps.lwp_lwpid = 0;
		}
		else {
			read_one_lwp(p_lwp, &ps);
		}

		lwp_count.nproc++;

/*
 *	        KLUDGE found to be 1
 */
		info_p = &ps;

		read_lwps(UADDR(p->p_lwpp, sizeof(lwp_buf), &lwp_buf), 
			  (caddr_t) p->p_lwpp, p, kp);

		knext_p = (caddr_t) p->p_next;
		next_p = UADDR(p->p_next, sizeof(proc_next_buf), 
			       &proc_next_buf);


		if (next_p == 0 || 
		    next_p->p_prev != (proc_t *) kp ||
		    ((proc_t *) UADDR(next_p->p_prev, 
				      sizeof(proc_next_prev_buf),
				      &proc_next_prev_buf))->p_next != 
		    (proc_t *) knext_p) {
		  p = 0;
		}
		else {
		  if (next_p == &proc_next_buf) {
		    p = &proc_buf;
		    memcpy(&proc_buf, &proc_next_buf, sizeof(proc_buf));
		  }
		  else {
		    p = next_p;
		  }
		  kp = knext_p;
		}
	}

	return(0);
}


print_proc( int row, int col ) {
	struct proc_list *p;
	int i;

	psrow = row;
	pscol = col;
	if( need_header & METS ) {
		print_proc_hdr( row, col ) ;
	}
	row++;
	sortps();
	move(row,col);
	set_default_color(0);
	for( i = row; i < scr_rows-1; i++ )
		color_clrtoeol( i, 0 );
	assert( psscroll >= 0 );
	for( i = psscroll; (i < (pscnt-1)) && (row < scr_rows-3) ; i++ ) {
		for( p = proot.nxt ; p ; p = p->nxt ) {
			assert( p->print_me <= pscnt );
			if( p->print_me == (i+1) ) {
				print_proc_entry( p, row++, col );
				assert( p->print_me == (i+1));
				break;
			}
		}
		assert( p );
	};
	pscanscroll = 0;
	if( !psscroll && i == (pscnt-1) ) { 
		for( p = proot.nxt ; p ; p = p->nxt ) {
			assert( p->print_me <= pscnt );
			if( p->print_me == (i+1) ) {
				print_proc_entry( p, row++, col );
				assert( p->print_me == (i+1));
				break;
			}
		}
	} else {
		set_message_color(0);
		move( row, pscol + 2 );
		if( psscroll ) {
			if( i < (pscnt-1) ) {
				printw(gettxt("RTPM:627", "<-   more   ->"));
				pscanscroll = 1;
			} else 
				printw(gettxt("RTPM:628", "<-   more"));
		} else if( i < (pscnt-1) ) {
			printw(gettxt("RTPM:629", "     more   ->"));
			pscanscroll = 1;
		}
	}
	return(1);
}


int
scroll_ps( int incr ) {
	int psrows = scr_rows - psrow - 3;

	if(incr > 0 && (!pscanscroll || (pscnt - psrows - psscroll)< incr))
		return(1);
	if( incr < 0 && (psscroll + incr) < 0 )
		return(1);
	psscroll += incr;
	print_proc( psrow, pscol );
	print_uname();
	print_time();
	return(0);
}


static void
sortps( void ) {
	struct proc_list *p, *tmp;
	float max;

	if( ps_sorted )
		return;

	ps_sorted = 1;
	pscnt = 0;
	for( p = proot.nxt ; p ; p = p->nxt ) {
		if( p->inuse && p->tdiff > 0.0 ) {
			switch( print_proc_flags ) {
			case 0:
				p->print_me = -1;
				break;
			case USER_ONLY:
				if( strcmp( p->pinfo.lwp_clname, "SYS" ))
					p->print_me = -1;
				else
					p->print_me = 0;
				break;
			case SYS_ONLY:
				if(!strcmp( p->pinfo.lwp_clname, "SYS" ))
					p->print_me = -1;
				else
					p->print_me = 0;
				break;
			}
		} else {
			p->print_me = 0;
		}
	}
	do {
		max = 0.0;
		tmp = NULL;
		for( p = proot.nxt ; p ; p = p->nxt ) {
			if( p->inuse && p->print_me < 0
			  && p->tdiff > max ) {
				max = p->tdiff;
				tmp = p;
			}
		}
		if( tmp ) {
			tmp->print_me = ++pscnt;
		}
	} while( tmp );
}


static void
clean_proc() {
	struct proc_list *p, *tmp;

	ps_sorted = 0;
	pscnt = 0;
	psscroll = 0;
	for( p = &proot ; p->nxt ; p = p->nxt ) {
		if( p->nxt->inuse ) {
			p->nxt->inuse = 0;
			p->nxt->print_me = 0;
			continue;
		}
		tmp = p->nxt;
		p->nxt = p->nxt->nxt;
		release_list_node(tmp);
	}
}


static void
print_proc_entry( struct proc_list *pl_p, int row, int col ) {
	info_p = &(pl_p->pinfo);
	prcom( pl_p, info_p->uid, row, col );
}

/*
 * Procedure:     readata
 *
 * Restrictions:
 *               open(2): MACREAD
 *               stat(2): None
 *               pfmt: None
 *               strerror: None
 * Notes:
 *
 * readata reads in the open devices (terminals) and stores 
 * info in the devl structure.
 */
static char	psfile[] = "/etc/ps_data";

int readata()
{
	struct stat sbuf1, sbuf2;
	int fd, i;

	if( (fd = open(psfile, O_RDONLY) ) < 0 )
		return(0);

	if (fstat(fd, &sbuf1) < 0
	  || sbuf1.st_size == 0
	  || stat("/etc/passwd", &sbuf2) == -1
	  || sbuf1.st_mtime <= sbuf2.st_mtime
	  || sbuf1.st_mtime <= sbuf2.st_ctime) {
		(void) close(fd);
		return 0;
	}

	/* Read /dev data from psfile. */
	if (psread(fd, (char *) &ndev, sizeof(ndev)) == 0)  {
		(void) close(fd);
		return 0;
	}

	if ((devl = (struct devl *)malloc(ndev * sizeof(*devl))) == NULL) {
		if( in_curses )
			endwin();
		fprintf(stderr, gettxt("RTPM:7", "out of memory\n"));
		exit(1);
	}
	if (psread(fd, (char *)devl, ndev * sizeof(*devl)) == 0)  {
		(void) close(fd);
		return 0;
	}

	/* See if the /dev information is out of date */
	for (i=0; i<ndev; ++i)
		if (devl[i].dev == PRNODEV) {
			char buf[DNSIZE+5];
			strcpy(buf, "/dev/");
			strcat(buf, devl[i].dname);
			if (stat(buf, &sbuf2) == -1 ||
			    sbuf1.st_mtime <= sbuf2.st_mtime ||
			    sbuf1.st_mtime <= sbuf2.st_ctime) {
				(void) close(fd);
				return 0;	/* Out of date */
			}
		}

	/* Read /etc/passwd data from psfile. */
	if (psread(fd, (char *) &nud, sizeof(nud)) == 0)  {
		(void) close(fd);
		return 0;
	}

	if ((ud = (struct udata *)malloc(nud * sizeof(*ud))) == NULL) {
		if( in_curses )
			endwin();
		fprintf(stderr, gettxt("RTPM:7", "out of memory\n"));
		exit(1);
	}
	if (psread(fd, (char *)ud, nud * sizeof(*ud)) == 0)  {
		(void) close(fd);
		return 0;
	}

	(void) close(fd);
	return 1;
}

/*
 * Procedure:     getdev
 *
 * Restrictions:
 *               ftw: None
 *               pfmt: None
 *               strerror: None
 * Notes:  getdev() uses ftw() to pass pathnames under /dev to gdev()
 * along with a status buffer.
 */
void getdev()
{
	int	gdev();
	int	rcode;

	ndev = 0;
	rcode = ftw("/dev", gdev, 17);

	if(rcode) {
		if( in_curses )
			endwin();
		fprintf(stderr,gettxt("RTPM:626", "ftw() failed\n"));
		exit(1);
	}
	return;
}

/*
 * Procedure:     gdev
 *
 * Restrictions:
 *               pfmt: None
 *
 * gdev() puts device names and ID into the devl structure for character
 * special files in /dev.  The "/dev/" string is stripped from the name
 * and if the resulting pathname exceeds DNSIZE in length then the highest
 * level directory names are stripped until the pathname is DNSIZE or less.
 */
int gdev(objptr, statp, numb)
	char	*objptr;
	struct stat *statp;
	int	numb;
{
	register int	i;
	int	leng, start;
	static struct devl ldevl[2];
	static int	lndev, consflg;

	/* Make sure there is room and be ready for syscon & systty. */
	while (ndev + lndev >= maxdev) {
		maxdev += UDQ;
		devl = (struct devl *) ((devl == NULL) ?
					malloc(sizeof(struct devl ) * maxdev) :
					realloc(devl, sizeof(struct devl ) * maxdev));
		if (devl == NULL) {
			if( in_curses )
				endwin();
			fprintf(stderr, gettxt("RTPM:7", "out of memory\n"));
			exit(1);
		}
	}

	switch (numb) {

	case FTW_F:	
		if ((statp->st_mode & S_IFMT) == S_IFCHR) {
			/*
			 * Save systty & syscon entries if the console
			 * entry hasn't been seen.
			 */
			if (!consflg
			  && (strcmp("/dev/systty", objptr) == 0
			    || strcmp("/dev/syscon", objptr) == 0)) {
				(void) strncpy(ldevl[lndev].dname,
				  &objptr[5], DNSIZE);
				ldevl[lndev].dev = statp->st_rdev;
				lndev++;
				return 0;
			}

			leng = strlen(objptr);
			/* Strip off /dev/ */
			if (leng < DNSIZE + 4)
				(void) strcpy(devl[ndev].dname, &objptr[5]);
			else {
				start = leng - DNSIZE - 1;

				for (i = start; i < leng && (objptr[i] != '/');
				  i++)
					;
				if (i == leng )
					(void) strncpy(devl[ndev].dname,
					  &objptr[start], DNSIZE);
				else
					(void) strncpy(devl[ndev].dname,
					  &objptr[i+1], DNSIZE);
			}
			devl[ndev].dev = statp->st_rdev;
			ndev++;
			/*
			 * Put systty & syscon entries in devl when console
			 * is found.
			 */
			if (strcmp("/dev/console", objptr) == 0) {
				consflg++;
				for (i = 0; i < lndev; i++) {
					(void) strncpy(devl[ndev].dname,
					  ldevl[i].dname, DNSIZE);
					devl[ndev].dev = ldevl[i].dev;
					ndev++;
				}
				lndev = 0;
			}
		}
		return 0;

	case FTW_D:
		/* Record /dev/ subdirectories except /dev/fd/. */
		if ((strlen(objptr) < (unsigned)(DNSIZE + 4)) &&
		    (strcmp(objptr, "/dev/fd") != 0)) {
			(void) strcpy(devl[ndev].dname,
				      objptr[4] ? objptr+5 : "");
			devl[ndev].dev = PRNODEV;
			ndev++;
		}
		return 0;

	case FTW_DNR:
	case FTW_NS:
		return 0;

	default:
		return 1;
	}
}

/*
 * Procedure:     getpasswd
 *
 * Restrictions:
                 getpwent: none
                 endpwgent: None
                 pfmt: None
 * Notes
 * Get the passwd file data into the ud structure.
 */
void
getpasswd()
{
	struct passwd *pw;

	ud = NULL;
	nud = 0;
	maxud = 0;

	while ((pw = getpwent()) != NULL) {
		while (nud >= maxud) {
			maxud += UDQ;
			ud = (struct udata *) ((ud == NULL) ? 
			  malloc(sizeof(struct udata ) * maxud) : 
			  realloc(ud, sizeof(struct udata ) * maxud));
			if (ud == NULL) {
				
				if( in_curses )
					endwin();
				fprintf(stderr, gettxt("RTPM:7", "out of memory\n"));
				exit(1);
			}
		}
		/*
		 * Copy fields from pw file structure to udata.
		 */
		ud[nud].uid = pw->pw_uid;
		(void) strncpy(ud[nud].name, pw->pw_name, LOGNAME_MAX);
		nud++;
	}
	endpwent();
}

/*
 * gettty returns the user's tty number or ? if none.
 * ip = where the search left off last time.
 */
static char *
gettty(register int *ip)
{
	register int	i;

	if (info_p->ttydev != PRNODEV && *ip >= 0) {
		for (i = *ip; i < ndev; i++) {
			if (devl[i].dev == info_p->ttydev) {
				*ip = i + 1;
				return devl[i].dname;
			}
		}
	}
	*ip = -1;
	return gettxt("RTPM:630", "?");
}

/*
 *  read_lwps: walk down the list of lwps, and build an lwpsinfo structure
 *  for each one with read_one_lwp
 *
 */

static void
read_lwps(struct lwp *first_lwp, caddr_t kfirst_lwp, proc_t *p, caddr_t kp)
{
	struct lwp *lwp = first_lwp;
	caddr_t klwp = kfirst_lwp;
	ushort_t count = 0;

	while (lwp != 0) { 
		struct lwp *next_lwp;
		caddr_t knext_lwp;

		if (read_one_lwp(lwp, info_p) != 0) {
			break;
		}
		
		insert( info_p );
		count++;

		knext_lwp = (caddr_t) lwp->l_next;
		next_lwp = UADDR(lwp->l_next, sizeof(lwp_next_buf), 
				 &lwp_next_buf);

		/*
		if (next_lwp == 0 || 
		    next_lwp->l_prev != (struct lwp *) KADDR(lwp) ||
		    ((struct lwp *) UADDR(next_lwp->l_prev))->l_next !=
		     (struct lwp *) KADDR(next_lwp)||
		    next_lwp->l_procp != (proc_t *) KADDR(p) ||
		    count >= p->p_nlwp) {
			lwp = 0;
		}
		*/
		if (next_lwp == 0 || 
		    next_lwp->l_prev != (struct lwp *) klwp ||
		    ((struct lwp *) UADDR(next_lwp->l_prev, 
					  sizeof(lwp_next_prev_buf), 
					  &lwp_next_prev_buf))->l_next !=
		     (struct lwp *) knext_lwp ||
		    next_lwp->l_procp != (proc_t *) kp ||
		    count >= p->p_nlwp) {
			lwp = 0;
		}
		else {
			klwp = (caddr_t) lwp->l_next;
			if (next_lwp == &lwp_next_buf) {
			  lwp = &lwp_buf;
			  memcpy(&lwp_buf, &lwp_next_buf, sizeof(lwp_buf));
			}
			else {
			  lwp = next_lwp;
			}
		}
	}
}


/*
 *  Create an lwpsinfo structure for an lwp. 
 */

static int
read_one_lwp(struct lwp *lwp, pinfo_t *p)
{
	char	*clname;

	if (!SANCHECK(lwp->l_cid < numclass)) {
		return(1);
	}

	switch (lwp->l_stat) {
	      case SONPROC:	p->lwp_sname = 'O'; break;
	      case SRUN:	p->lwp_sname = 'R'; break;
	      case SSLEEP:	p->lwp_sname = 'S'; break;
	      case SSTOP:	p->lwp_sname = 'T'; break;
	      case SIDL:	p->lwp_sname = 'I'; break;
	      default:		p->lwp_sname = '?'; break;
	}

	if (!SANCHECK(p->lwp_sname != '?')) {
		return(1);
	}

	p->lwp_pri = lwp->l_pri;
	p->lwp_lwpid = lwp->l_lwpid;

	clname = UADDR((off_t) class_table[lwp->l_cid].cl_name,
		       sizeof(clname_buf), clname_buf);
	memcpy(&p->lwp_clname, clname, 
	       min(strlen(clname), (size_t) (PRCLSZ-1)));
	
	if (lwp->l_name) {
	  char *lname = UADDR(lwp->l_name, sizeof(lname_buf), lname_buf);
		memcpy(p->lwp_name, lname,
		       min(strlen(lname), (size_t) (PRFNSZ-1)));
	}

	if (lwp->l_eng == 0) {
		p->lwp_onpro = PBIND_NONE;
	}
	else {
		p->lwp_onpro = (lwp->l_eng - engine);
	}

	p->lwp_seconds = (float) lwp->l_utime * timer_resolution/BILLION + 
			 (float) lwp->l_stime * timer_resolution/BILLION;
	
	return 0;
}


/*
 * Procedure:     prcom
 *
 *
 * Notes:
 * Print info about the process.
 */
static void 
prcom( struct proc_list *pl_p, uid_t puid, int row, int col ) {
	extern float tot_tix;	/* elapsed cpu time */
	float  difftime = pl_p->tdiff;
	register char	*tp;
	long	tm;
	int	i;
	char *s;
	char buf[256];

	set_default_color( 0 );
	/*
	 * Get current terminal.  If none ("?") and 'a' is set, don't print
	 * info.  If 't' is set, check if term is in list of desired terminals
	 * and print it if it is.
	 */

	move( row, col );

	printw("%3.0f ",difftime*100*100/tot_tix);	/* %cpu */
	i = 0;
	tp = gettty(&i);

	printw("%c ", info_p->lwp_sname);		/* S */

	if ((i = getunam(puid)) >= 0)			/* USER */
		printw("%8.8s", ud[i].name);
	else
		printw("%8.8ld", puid);

	printw("%6ld", info_p->pid);			/* PID */
	printw("%6ld", info_p->lwp_lwpid);	/* LWPID */

	if (info_p->lwp_onpro == PBIND_NONE)	/* CPU */
		printw("%6s", "-");
	else
		printw("%4d", info_p->lwp_onpro);
	printw("%4d", info_p->lwp_pri);		/* PRI */

	tm = (long) (info_p->lwp_seconds);	/* CPU_TIME */

	printw(" %4ld:%.2ld", tm / 60, tm % 60); 
	{
		printw(".%02d", (int) (100 * (info_p->lwp_seconds - tm)));
	}
	printw("%7d", (unsigned long) info_p->size);	/* SZ */
	printw(" %-7.14s", tp);				/* TTY */
	sprintf( buf, "%s", info_p->psargs);
	if ( info_p->nlwp > 1 && *(info_p->lwp_name) &&
	    *(info_p->lwp_name) != ' ') {
		s = info_p->lwp_name;
		sprintf( &buf[strlen(buf)], "[%s]", s);
	}
	buf[19 + scr_cols - 80] = '\0';
	printw(" %s", buf );
}

static void
print_proc_hdr(int  row, int col ) {
	move( row, col );

	set_label_color( 0 );
	switch( print_proc_flags ) {
	case SYS_ONLY:
		printw(gettxt("RTPM:622", "%%%%%% S     USER   PID LWPID CPU PRI    CPUTIME   SIZE TTY     SYSCMD[LWP]"));
		break;
	case USER_ONLY:	
		printw(gettxt("RTPM:623", "%%%%%% S     USER   PID LWPID CPU PRI    CPUTIME   SIZE TTY     USERCMD[LWP]"));
		break;
	default:	
		printw(gettxt("RTPM:624", "%%%%%% S     USER   PID LWPID CPU PRI    CPUTIME   SIZE TTY     CMD[LWP]"));
		break;

	}
	set_default_color( 0 );
}

/*
 * For full command listing (-f flag) print user name instead of number.
 * Search table of userid numbers and if puid is found, return the
 * corresponding name.  Otherwise search /etc/passwd.
 */
static int 
getunam(uid_t puid)
{
	register int i;
	for (i = 0; i < nud; i++)
		if (ud[i].uid == puid)
			return i;
	return -1;
}

static int 
psread(int fd, char *bp, unsigned int bs)
{
	if (read(fd, bp, bs) != bs) {
		return 0;
	}
	return 1;
}


/*
 * prchoose(): Choose a "representative" lwp for a process.
 * Lifted from prsubr.c with minor modification.
 */

lwp_t *
prchoose(proc_t *p, caddr_t kp)
{
	lwp_t *lwp;
	caddr_t klwp;
	lwp_t *l_onproc = NULL;	/* running on processor */
	lwp_t *l_run = NULL;	/* runnable, on disp queue */
	lwp_t *l_sleep = NULL;	/* sleeping */
	lwp_t *l_susp = NULL;	/* suspended stop */
	lwp_t *l_jstop = NULL;	/* jobcontrol stop */
	lwp_t *l_req = NULL;	/* requested stop */
	lwp_t *l_istop = NULL;	/* event-of-interest stop */
	lwp_t *next_lwp;
	caddr_t knext_lwp;
	ushort_t count = 0;

	/* Note:  SIDL LWPs are ignored.  If there are no */
	/* non-SIDL LWPs, this function returns NULL. */

	klwp = (caddr_t) p->p_lwpp;
	lwp = UADDR(p->p_lwpp, sizeof(lwp_buf), &lwp_buf);

	/*
	if (lwp == 0 || lwp->l_procp != (proc_t *) KADDR(p)) {
		return 0;
	}
	*/

	if (lwp == 0 || lwp->l_procp != (proc_t *) kp) {
		return 0;
	}
	
	do {
		if (!SANCHECK(lwp->l_stat == SSLEEP ||
			      lwp->l_stat == SRUN ||
			      lwp->l_stat == SONPROC ||
			      lwp->l_stat == SSTOP ||
			      lwp->l_stat == SIDL)) {
			break;
		}

		switch (lwp->l_stat) {
		case SSLEEP:
			if ((lwp->l_flag & (L_NWAKE|L_PRSTOPPED))
			  == (L_NWAKE|L_PRSTOPPED)) {
				if (l_req == NULL)
					l_req = lwp;
			} else if (l_sleep == NULL)
				l_sleep = lwp;
			break;
		case SRUN:
			if (l_run == NULL)
				l_run = lwp;
			break;
		case SONPROC:
			if (l_onproc == NULL)
				l_onproc = lwp;
			break;
		case SSTOP:
			switch (lwp->l_whystop) {
			case PR_SUSPENDED:
				if (l_susp == NULL)
					l_susp = lwp;
				break;
			case PR_JOBCONTROL:
				if (l_jstop == NULL)
					l_jstop = lwp;
				break;
			case PR_REQUESTED:
				if (l_req == NULL)
					l_req = lwp;
				break;
			default:
				if (l_istop == NULL)
					l_istop = lwp;
				break;
			}
			break;
		case SIDL:
			break;
		}

		count++;

		knext_lwp = (caddr_t) lwp->l_next;
		next_lwp = UADDR(lwp->l_next, sizeof(lwp_next_buf),
				 &lwp_next_buf);

		/*
		if (next_lwp == 0 || 
		    next_lwp->l_prev != (struct lwp *) KADDR(lwp) ||
		    ((struct lwp *) UADDR(next_lwp->l_prev))->l_next != 
		     (struct lwp *) KADDR(next_lwp) ||
		    next_lwp->l_procp != (proc_t *) KADDR(p) ||
		    count >= p->p_nlwp) {
			lwp = 0;
		}
		*/
		if (next_lwp == 0 || 
		    next_lwp->l_prev != (struct lwp *) klwp ||
		    ((struct lwp *) UADDR(next_lwp->l_prev,
					  sizeof(lwp_next_prev_buf),
					  &lwp_next_prev_buf))->l_next != 
		     (struct lwp *) next_lwp ||
		    next_lwp->l_procp != (proc_t *) kp ||
		    count >= p->p_nlwp) {
			lwp = 0;
			klwp = 0;
		}
		else {
		        klwp = knext_lwp;
			if (next_lwp == &lwp_next_buf) {
			  lwp = &lwp_buf;
			  memcpy(&lwp_buf, &lwp_next_buf, sizeof(lwp_buf));
			}
			else {
			  lwp = next_lwp;
			}
		}
	} while (lwp);

	if (l_onproc)
		lwp = l_onproc;
	else if (l_run)
		lwp = l_run;
	else if (l_sleep)
		lwp = l_sleep;
	else if (l_jstop)
		lwp = l_jstop;
	else if (l_istop)
		lwp = l_istop;
	else if (l_req)
		lwp = l_req;
	else if (l_susp)
		lwp = l_susp;

	return lwp;
}




static struct proc_list *
get_list_node()
{
	struct proc_list *p;

	if (!proc_freelist) {
		proc_freelist = allocate_list_nodes();
		if (!proc_freelist) {
			if (in_curses) {
				endwin();
			}
			fprintf(stderr, gettxt("RTPM:7", "out of memory\n"));
			exit( 1 );
		}
	}		

	p = proc_freelist;
	proc_freelist = proc_freelist->nxt;
	p->nxt = 0; /* in case caller uses p->nxt w/o setting it */

	return(p);
}


static struct proc_list *
allocate_list_nodes()
{
	struct proc_list *p;
	int	i;

	p = malloc(INC_SIZE * sizeof(struct proc_list));

	if (!p) {
		return(NULL);
	}

	for (i = 0; i < INC_SIZE - 1; i++) {
		p[i].nxt = p + (i + 1);
	}
	p[INC_SIZE - 1].nxt = 0;
	return(p);
}


static void
release_list_node(struct proc_list *p)
{
	p->nxt = proc_freelist;
	proc_freelist = p;
}


