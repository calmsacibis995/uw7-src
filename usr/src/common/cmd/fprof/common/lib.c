/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved. */
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved. */
/*	  All Rights Reserved */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc. */
/*	The copyright notice above does not evidence any */
/*	actual or intended publication of such source code. */
#ident	"@(#)fprof:common/lib.c	1.7"

#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/seg.h>
#include <sys/proc.h>
#include <sys/tss.h>
#include <sys/ksym.h>
#include <sys/elf.h>
#include <sys/mman.h>
#include <sys/sysmacros.h>
#include <dlfcn.h>
#include <link.h>
#include <limits.h>
#include <unistd.h>
#include <sys/inline.h>
#include <sys/procfs.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "fprof.h"

#define OUTSIZE 4096
#ifndef CFG_FILE
#define CFG_FILE "/usr/ccs/lib/fprof.cfg"
#endif

static char *Header, *HeaderCur;
static char Tracebuf[PATH_MAX];
static char *Tracefile = Tracebuf;
static ulong Out[OUTSIZE], *End = Out + OUTSIZE - 56, *Walk = Out;
static ulong BytesWritten;
static int Init;
static int Outfd = -1;
static struct r_debug *Db;
static char *Progname = NULL;
static struct logcontrol Control;

static void (**Rt_event)();

static ulong *Lboltp;
static const volatile timestruc_t *mapped_hrtimep;	/* must be == 0 at start */

/* Write to the log file and reset the pointer */
static void
logflush()
{
	_write(Outfd, (char *) Out, ((char *) Walk) - (char *) Out);
	BytesWritten += (char *) Walk - (char *) Out;
	Walk = Out;
}

#define OUT(INFO) *Walk++ = (INFO)

#define HRLBOLT ((((mapped_hrtimep->tv_sec & 0x3ff) * 100) + (mapped_hrtimep->tv_nsec / (1000000000 / 100))) << 14)
#define LBOLT (*Lboltp << 14)
#define ASMTIMER() ((Lboltp ? LBOLT : HRLBOLT) | (!Control.accurate ? 0 : asmtimer()))

#define BUFFLUSH() do {\
		OUT(0);\
		OUT(BUFTIME);\
		OUT(2);\
		OUT(ASMTIMER());\
		logflush();\
		OUT(ASMTIMER());\
		OUT(0);\
		OUT(2);\
	} while(0)

#define OUTDONE(INFO) do {\
	OUT(INFO);\
	if (Walk >= End) {\
		BUFFLUSH();\
	};\
	} while(0)

/*
** If the command was kicked off by fprof ProfileCommandName will
** be set (full path).  Otherwise, use /proc.
*/
get_progname()
{
	int fd;
	struct psinfo psinfo;
	char buf[25];
	int len;

	if (!(Progname = getenv("ProfileCommandName"))) {
		sprintf(buf, "/proc/%d/psinfo", getpid());
		if ((fd = open(buf, O_RDONLY)) < 0) {
			sprintf(buf, "Cannot read from process %d", getpid());
			perror(buf);
			Progname = "NULL";
			return;
		}
		if (read(fd, &psinfo, sizeof(struct psinfo)) < sizeof(struct psinfo)) {
			sprintf(buf, "Cannot read from process %d", getpid());
			perror(buf);
			Progname = "NULL";
			close(fd);
			return;
		}
		Progname = malloc(((len = strcspn(psinfo.pr_psargs, " ")) + 1) * sizeof(char));
		strncpy(Progname, psinfo.pr_psargs, len);
		Progname[len] = '\0';
		close(fd);
	}
}

/*
** Read the link map and write it out.  The format is null terminated
** string for the name, followed by the text size (this will be used
** as a sanity check at read time), followed by its address.  Terminate
** the list with a zero-length name.  Pad out to a long boundary.
*/
static void
map_write()
{
	char *p;
	ulong *place;
	char *name;
	long start;
	struct link_map *map;
	struct stat st;

	BUFFLUSH();
	HeaderCur += sprintf(HeaderCur, "Map=%x\n", BytesWritten + (Walk - Out) * sizeof(ulong));
	OUT(0);
	OUT(MAPTIME);
	place = Walk;
	OUT(0);
	p = (char *) Walk;
	for (map = Db->r_map; map; map = map->l_next) {
		if (!map->l_name) {
			name = Progname;
			start = 0;
		}
		else {
			name = map->l_name;
			if (!map->l_tstart)
				start = map->l_addr;
			else
				start = map->l_tstart;
		}
		strcpy(p, name);
		p += strlen(p) + 1;
		p = LALIGN(p);
		*((long *) p) = start;
		p += sizeof(long);
		st.st_size = 0;
		stat(name, &st);
		*((long *) p) = st.st_size;
		p += sizeof(long);
	}

	Walk = (ulong *) p;
	OUT(0);
	OUT(p - (char *) place);
	*place = p - (char *) place;
	logflush();
}

static caddr_t 
get_hrt_mapping(void)
{
	void *ret_addr = (caddr_t)(-1);
	unsigned long hrtaddr = 0;
	unsigned long hrtinfo;
	int sysdat_fd;
	caddr_t sysdat_pageaddr = NULL;

	/* 
	 * this function is entered only once -- the first time that a call 
	 * is made to gettimeofday() by any thread in the process. 
	 * if we can successfully map hrtrestime, then return the mapped
	 * address; otherwise return (-1);
 	 */

	if ((getksym("hrestime", &hrtaddr, &hrtinfo) == -1) ||
	    ((sysdat_fd = open("/dev/sysdat", O_RDONLY)) == -1)) {
		return(ret_addr);
	}
	if (((sysdat_pageaddr = mmap(0, PAGESIZE, PROT_READ, MAP_SHARED, 
				sysdat_fd, (off_t)hrtaddr)) != NULL) &&
            (sysdat_pageaddr != (void *)(-1))) {
		ret_addr = (caddr_t)sysdat_pageaddr + (hrtaddr % PAGESIZE);
	}
	(void)_close(sysdat_fd);
	return(ret_addr);
}

static void
get_lbolt_mapping()
{
	ulong value = 0, info = STT_FUNC;
	int fd;

	if (getksym("lbolt", &value, &info) < 0)
		return;
	if ((fd = open("/dev/kmem", O_RDONLY)) < 0)
		return;
	if ((Lboltp = (ulong *) mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, value & ~0xfff)) != (ulong *) -1)
		Lboltp = (ulong *) (((caddr_t) Lboltp) + (value & 0xfff));
	else
		Lboltp = NULL;
	close(fd);
}

/*
** Read variables and set up data structures.
*/
static void
loginit()
{
	register int i;
	int pagesize;
	void *handle;
	struct utsname un;
	FILE *fp;

	Init = 1;
	Control.inlogue = 1;
	Control.nolog = 0;
	Control.accurate = 0;
	Control.on = 1;
	Control.mark = 0;
	sprintf(Tracefile, "/tmp/out.%d", getpid());
	if ((fp = fopen(CFG_FILE, "r"))) {
		char buf[100];
		char *p;

		while (fgets(buf, 100, fp)) {
			if (!(p = strchr(buf, '=')))
				continue;
			*p++ = '\0';
			p[strlen(p) - 1] = '\0';
			if (strcmp(buf, "Logging") == 0)
				Control.nolog = strcmp(p, "off") == 0;
			else if (strcmp(buf, "Accuracy") == 0)
				Control.accurate = strcmp(p, "accurate") == 0;
			else if (strcmp(buf, "StartState") == 0)
				Control.on = strcmp(p, "off") != 0;
			else if (strcmp(buf, "LogPrefix") == 0)
				sprintf(Tracefile, "%s.%d", p, getpid());
		}
	}
	if (getenv("_FprofLogging")) {
		if (strcmp(getenv("_FprofLogging"), "off") == 0) {
			Control.nolog = 1;
			return;
		}
		Control.nolog = 0;
	}
	if (getenv("_FprofAccuracy"))
		Control.accurate = strcmp(getenv("_FprofAccuracy"), "accurate") == 0;
	if (getenv("_FprofStartState"))
		Control.on = strcmp("_FprofStartState", "off") != 0;

	if (getenv("_FprofLogPrefix"))
		sprintf(Tracefile, "%s.%d", getenv("_FprofLogPrefix"), getpid());

	if (getenv("_FprofLogFile")) {
		putenv("_FprofLogging=off");
		strcpy(Tracefile, getenv("_FprofLogFile"));
	}
	if ((Outfd = open(Tracefile, O_TRUNC|O_CREAT|O_RDWR, 0777)) < 0) {
		perror("open");
		exit(1);
	}
	lockf(Outfd, F_LOCK, HEADER_SIZE);
	fcntl(Outfd, F_SETFD, 1);

	atexit(logflush);

	if (Control.accurate) {
		if (!setperm()) {
			fprintf(stderr, "Logging failed: root privilege is required to read accurate timer.\nSet Accuracy parameter to 'normal' or become root.\n");
			exit(1);
		}
	}

	get_lbolt_mapping();

	pagesize = sysconf(_SC_PAGESIZE);

	if (!Lboltp && (mapped_hrtimep = (timestruc_t *)get_hrt_mapping()) == (timestruc_t *)(-1)) {
		perror("Reading Timer");
		exit(1);
	}

	handle = (void *) _dlopen(NULL, RTLD_LAZY);

	Db = dlsym(handle, "_r_debug");
	if (Rt_event = dlsym(handle, "_rt_event"))
		*Rt_event = map_write;
	get_progname();
	_write(Outfd, Out, HEADER_SIZE); /* Create header page */
	BytesWritten = HEADER_SIZE;
	if ((HeaderCur = Header = (char *) mmap(NULL, HEADER_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, Outfd, 0)) == (char *) -1) {
		perror("mmap");
		exit(1);
	}
	uname(&un);
	HeaderCur += sprintf(HeaderCur, "flow\nVersion=1\nCompiled=0\nControl=%x\nNode=%s\nTime=%x\nAccurateTimeStamp=%d\n", &Control, un.nodename, time(0), Control.accurate);
	map_write();
	Control.inlogue = 0;
}

/* Must be fast!  Output an address followed by the time */
_prologue(ulong addr)
{
	ulong x;

	if (!Init)
		loginit();

	if (Control.nolog || Control.inlogue || !Control.on)
		return;

	Control.inlogue = 1;
	if (Control.mark) {
		OUT(0);
		OUT(MARKLOG);
		Control.mark = 0;
	}

	OUT(addr);
	OUTDONE(ASMTIMER() & ~(0x80000000));
	Control.inlogue = 0;
}

/* Must be fast!  Output an address followed by the time */
_epilogue(ulong addr)
{
	if (Control.nolog || Control.inlogue || !Control.on)
		return;

	Control.inlogue = 1;
	if (Control.mark) {
		OUT(0);
		OUT(MARKLOG);
		Control.mark = 0;
	}

	OUT(addr);
	OUTDONE(ASMTIMER() | 0x80000000);
	Control.inlogue = 0;
}
