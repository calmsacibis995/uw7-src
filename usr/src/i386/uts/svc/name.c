#ident	"@(#)kern-i386:svc/name.c	1.11.8.3"
#ident	"$Header$"

#include <acc/dac/acl.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kma.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pmem.h>
#include <proc/cg.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <proc/procset.h>
#include <proc/resource.h>
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/ucontext.h>
#include <proc/unistd.h>
#include <proc/user.h>
#include <svc/bootinfo.h>
#include <svc/errno.h>
#include <svc/limitctl.h>
#include <svc/psm.h>
#include <svc/sysconfig.h>
#include <svc/systeminfo.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/utsname.h>
#include <svc/memory.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>


STATIC char	*version = VERSION;	/* VERSION and RELEASE are defined at */
STATIC char	*release = RELEASE;	/* compile time, on the command line */

extern char	architecture[];		/* from name.cf/Space.c */
extern char	hostname[];		/* from name.cf/Space.c */
extern char	initfile[];             /* from name.cf/Space.c */
extern char	osbase[];		/* from name.cf/Space.c */
extern char	osprovider[];		/* from name.cf/Space.c */
extern char	o_architecture[];	/* from name.cf/Space.c */
extern char	o_hw_provider[];	/* from name.cf/Space.c */
extern char	o_machine[];		/* from name.cf/Space.c */
extern char	o_sysname[];		/* from name.cf/Space.c */
extern char	*o_bustype;		/* from name.cf/Space.c */
extern char	*bustypes;		/* from name.cf/Space.c */
extern char	*scodate;		/* kernel timestamp from sco.c */
extern int	exec_ncargs;		/* from proc.cf/Space.c */

/*
 *+ Reader/Writer spin lock which protects the writable member(s)
 *+ of the utsname structure, the secure rpc domain (srpc_domain)
 *+ and the fully-qualified host name (hostname).  Currently, only
 *+ utsname.nodename is writable; other fields of the utsname
 *+ structure are constant once the system is initialized.
 */
STATIC LKINFO_DECL(uname_lockinfo, "SU::uname_lock", 0);

STATIC rwlock_t uname_lock;

/*
 * void inituname(void)
 *	Initialize uname and related info.
 *
 * Calling/Exit State:
 *	One time initialization routine called during system startup.
 *
 * Remarks:
 *	The uname_lock is used to protect the writable strings
 *	utsname.nodename, srpc_domain and hostname.
 */
void
inituname(void)
{
	char *p;
	extern void identify_cpu(void);

	/*
	 * Get the release and version of the system.
	 */
	if (release[0] != '\0') {
		strncpy(utsname.release, release, SYS_NMLN-1);
		utsname.release[SYS_NMLN-1] = '\0';
		strncpy(xutsname.release, release, XSYS_NMLN-1);
		xutsname.release[XSYS_NMLN-1] = '\0';
	}
	if (version[0] != '\0') {
		strncpy(utsname.version, version, SYS_NMLN-1);
		utsname.version[SYS_NMLN-1] = '\0';
		strncpy(xutsname.version, version, XSYS_NMLN-1);
		xutsname.version[XSYS_NMLN-1] = '\0';
	}

	/*
	 * Initialize the machine name from information determined
	 * by identify_cpu().  Note that we're calling identify_cpu()
	 * redundantly here (chip_detect() will call it again later)
	 * because we need this for possible use in the copyright
	 * and title banner.
	 */
	identify_cpu();
	strncpy(utsname.machine, l.cpu_fullname, SYS_NMLN-1);
	utsname.machine[SYS_NMLN-1] = '\0';
	strncpy(xutsname.machine, l.cpu_abbrev, XSYS_NMLN-1);
	xutsname.machine[XSYS_NMLN-1] = '\0';

	/*
	 * Initialize the bustypes string (defaulting to the value
	 * assigned in name.cf/Space.c) for use by systeminfo.
	 */
	if ((p = bs_getval("BUSTYPES")) != NULL)
		bustypes = p;

	/*
	 * Get the INITFILE from the boot parameters, if specified.
	 * Put it in 'initfile' for later use by '/sbin/init'.
	 */
	if ((p = bs_getval("INITFILE")) != NULL)
		bcopy(p, initfile, SYS_NMLN);

	/*
	 * Initialize the o_bustype string (defaulting to the value
	 * assigned in name.cf/Space.c) for use by scoinfo.
	 */
	if (bootinfo.machflags & MC_BUS)
		o_bustype = "MCA";
	else if (bootinfo.machflags & EISA_IO_BUS)
		o_bustype = "EISA";
	else if (bootinfo.machflags & AT_BUS)
		o_bustype = "ISA";

	RW_INIT(&uname_lock, PROC_HIER_BASE, PL1, &uname_lockinfo, KM_NOSLEEP);
}

/*
 * void printuname(const char *)
 *	Parameterize the supplied string with fields from the
 *	utsname structure (and related strings) and print it.
 *
 * Calling/Exit State:
 *	One time routine called during system startup
 *	to print the title[] strings.
 */

void
printuname(const char *fmt)
{
	char	*str;
	int	c;

	while ((c = *fmt++) != '\0') {
		if (c != '%') {
			cmn_err(CE_CONT, "%c", c);
			continue;
		}

		switch (c = *fmt++) {

		case 'a':	str = architecture;	break;	/* %a */
		case 'i':	str = initfile;		break;	/* %i */
		case 'm':	str = utsname.machine;	break;	/* %m */
		case 'n':	str = utsname.nodename;	break;	/* %n */
		case 'r':	str = utsname.release;	break;	/* %r */
		case 's':	str = utsname.sysname;	break;	/* %s */
		case 'v':	str = utsname.version;	break;	/* %v */
		case '%':	str = "%";		break;	/* %% */
		case '\0':	str = "%"; --fmt;	break;	/* %<NUL> */

		default:
			cmn_err(CE_CONT, "%%%c", c);
			continue;
		}
		cmn_err(CE_CONT, "%s", str);
	}
	cmn_err(CE_CONT, "\n");
}

/*
 * void
 * si_add_bustype(const char *bustype)
 *	Add a new bustype to the bustypes string.
 *
 * Calling/Exit State:
 *	Can sleep.
 */
void
si_add_bustype(const char *bustype)
{
	char *new_bustypes;
	size_t cur_len = strlen(bustypes);
	size_t new_len = cur_len + strlen(bustype) + 2;
	static size_t alloc_len;

	new_bustypes = kmem_alloc(new_len, KM_SLEEP);
	strcpy(new_bustypes, bustypes);
	new_bustypes[cur_len] = ',';
	strcpy(new_bustypes + cur_len + 1, bustype);
	if (alloc_len)
		kmem_free(bustypes, alloc_len);
	bustypes = new_bustypes;
	alloc_len = new_len;
}

/* Enhanced Application Compatibility Support */
/*
** Check the code in "svc/sco.c" when making any implementation changes to
** avoid breaking the SCO-compatible equivalent of this function.
*/
/* End Enhanced Application Compatibility Support */

struct sysconfiga {
	int which;
};

/*
 * int sysconfig(struct sysconfiga *uap, rval_t *rvp)
 *	Undocumented _sysconfig(2) system call handler.
 *	Return various system configuration parameters.
 *
 * Calling/Exit State:
 *	No special locking issues here.  This function does not
 *	do much!
 */
int
sysconfig(struct sysconfiga *uap, rval_t *rvp)
{
	switch (uap->which) {

	case _CONFIG_CLK_TCK:			/* clock ticks per second */
		rvp->r_val1 = HZ;
		break;

	case _CONFIG_NGROUPS:			/* max supplementary groups */
		rvp->r_val1 = ngroups_max;
		break;

	case _CONFIG_OPEN_FILES:		/* max open files (soft lim) */
		rvp->r_val1 = u.u_rlimits->rl_limits[RLIMIT_NOFILE].rlim_cur;
		break;

	case _CONFIG_CHILD_MAX:			/* max processes per real uid */
		rvp->r_val1 = v.v_maxup;
		break;

	case _CONFIG_POSIX_VER:			/* current POSIX version */
		rvp->r_val1 = _POSIX_VERSION;
		break;
	
	case _CONFIG_PAGESIZE:			/* logical page size */
		rvp->r_val1 = PAGESIZE;
		break;

	case _CONFIG_XOPEN_VER:			/* current XOPEN version */
		rvp->r_val1 = _XOPEN_VERSION;
		break;

	case _CONFIG_NACLS_MAX:			/* max entries in ACL */
                rvp->r_val1 = acl_getmax();
                break;

	case _CONFIG_NPROC:			/* max processes system-wide */
		rvp->r_val1 = v.v_proc;
		break;

	case _CONFIG_NENGINE:			/* # engines in system */
                rvp->r_val1 = Nengine;
		break;

	case _CONFIG_NENGINE_ONLN:		/* # engines online now */
                rvp->r_val1 = nonline;
		break;

	case _CONFIG_ARG_MAX:			/* max length of exec args*/
		rvp->r_val1 = exec_ncargs;
		break;

	case _CONFIG_NCGS_CONF:			/* number of CGs in system */
		rvp->r_val1 = Ncg;
		break;

	case _CONFIG_NCGS_ONLN:			/* number of CGs online now */
		rvp->r_val1 = NcgonlineU;
		break;

	case _CONFIG_MAX_ENG_PER_CG:		/* max engines per CG */
		rvp->r_val1 = cg_maxengines();
		break;

	case _CONFIG_TOTAL_MEMORY:		/* total memory */
		rvp->r_val1 = btop64(global_memsize.tm_global.tm_total);
		break;

	case _CONFIG_USEABLE_MEMORY:		/* user + system memory */
		rvp->r_val1 = btop64(global_memsize.tm_global.tm_useable);
		break;

	case _CONFIG_GENERAL_MEMORY:		/* user only memory */
		rvp->r_val1 = btop64(global_memsize.tm_global.tm_general);
		break;

	case _CONFIG_DEDICATED_MEMORY:		/* dedicated memory */
		rvp->r_val1 = btop64(global_memsize.tm_global.tm_dedicated);
		break;

	case _CONFIG_CACHE_LINE:		/* memory cache line size */
		rvp->r_val1 = (int)kma_cache_line();
		break;

#ifndef MINI_KERNEL		
	case _CONFIG_SYSTEM_ID:			/* unique system ID */
	{
		int 	error;
		int	id;

		if ((error = license_system_id(&id)))
			return EINVAL;
		else
			rvp->r_val1 = id;
		break;
	}
#endif	

	default:
		return EINVAL;
	}
	
	return 0;
}


struct uname {
	struct utsname *cbuf;
};

/*
 * int nuname(struct uname *uap, rval_t *rvp)
 *	New uname system call.  Supports larger fields.
 *
 * Calling/Exit State:
 *	No spin locks can be held by the caller.  This
 *	function can block (via copyout).
 */
/* ARGSUSED */
int
nuname(struct uname *uap, rval_t *rvp)
{
        struct utsname *buf = uap->cbuf;
	char name[SYS_NMLN];

	ASSERT(KS_HOLD0LOCKS());

	if (copyout(utsname.sysname, buf->sysname, strlen(utsname.sysname) + 1))
		return EFAULT;

	getutsname(utsname.nodename, name);
	if (copyout(name, buf->nodename, strlen(name) + 1))
		return EFAULT;

	if (copyout(utsname.release, buf->release, strlen(utsname.release) + 1))
		return EFAULT;

	if (copyout(utsname.version, buf->version, strlen(utsname.version) + 1))
		return EFAULT;

	if (copyout(o_machine, buf->machine, strlen(o_machine) + 1))
		return EFAULT;

	rvp->r_val1 = 1;
	return 0;
}

struct systeminfoa {
	int command;
	char *buf;
	long count;
};

/*
 * STATIC int strout(char *str, struct systeminfoa *uap, rval_t *rvp)
 *	Service routine for the systeminfo() function below.
 *
 * Calling/Exit State:
 *	This function can block (via copyout, subyte), no locks can
 *	be held by the caller.
 */
STATIC int
strout(char *str, struct systeminfoa *uap, rval_t *rvp)
{
        int strcnt, getcnt;

	ASSERT(KS_HOLD0LOCKS());

	strcnt = strlen(str);
        getcnt = (strcnt >= uap->count) ? uap->count : strcnt + 1;

        if (copyout(str, uap->buf, getcnt))
                return EFAULT;

        if (strcnt >= uap->count && subyte(uap->buf + uap->count - 1, 0) < 0)
                return EFAULT;

        rvp->r_val1 = strcnt + 1;
        return 0;
}

/*
 * int systeminfo(struct systeminfoa *uap, rval_t *rvp)
 *	Sysinfo(2) system call handler.  Get and set system
 *	information strings.
 *
 * Calling/Exit State:
 *	This function can block (via copyout/copyin).  No spin
 *	locks can be held by the caller.
 */
/* ARGSUSED */
int
systeminfo(struct systeminfoa *uap, rval_t *rvp)
{
	int error;
	char name[SYS_NMLN];
	int userlim;

	ASSERT(KS_HOLD0LOCKS());

	switch (uap->command) {

	case SI_RELEASE:
		error = strout(utsname.release, uap, rvp);
		break;

	case SI_VERSION:
		error = strout(utsname.version, uap, rvp);
		break;

	case SI_HW_SERIAL:
		error = strout(os_hw_serial, uap, rvp);
		break;

	case SI_SRPC_DOMAIN:
		getutsname(srpc_domain, name);
		error = strout(name, uap, rvp);
		break;

	case SI_INITTAB_NAME:
                error = strout(initfile, uap, rvp);
                break;

	case __O_SI_ARCHITECTURE:
		error = strout(o_architecture, uap, rvp);
		break;

	case SI_ARCHITECTURE:
		error = strout(architecture, uap, rvp);
		break;

	case SI_BUSTYPES:
		error = strout(bustypes, uap, rvp);
		break;

	case __O_SI_HOSTNAME:
		/* UW2 returned local hostname, not fully-qualified name */
		getutsname(utsname.nodename, name);
		error = strout(name, uap, rvp);
		break;

	case SI_HOSTNAME:
		/* as of Gemini we return the fully-qualified hostname */
		getutsname(hostname, name);
		error = strout(name, uap, rvp);
		break;

	case __O_SI_HW_PROVIDER:
		error = strout(o_hw_provider, uap, rvp);
		break;

	case SI_HW_PROVIDER:
		error = strout(os_platform_name, uap, rvp);
		break;

	case SI_KERNEL_STAMP:
		error = strout(scodate, uap, rvp);
		break;

	case __O_SI_MACHINE:
		error = strout(o_machine, uap, rvp);
		break;

	case SI_MACHINE:
		error = strout(utsname.machine, uap, rvp);
		break;

	case SI_OS_BASE:
		error = strout(osbase, uap, rvp);
		break;

	case SI_OS_PROVIDER:
		error = strout(osprovider, uap, rvp);
		break;

	case __O_SI_SYSNAME:
		error = strout(o_sysname, uap, rvp);
		break;

	case SI_SYSNAME:
		error = strout(utsname.sysname, uap, rvp);
		break;

	case SI_USER_LIMIT:
		limit(L_GETUSERLIMIT, &userlim);
		if (userlim <= 0)
			error = strout("unlimited", uap, rvp);
		else {
			numtos(userlim, name);
			error = strout(name, uap, rvp);
		}
		break;

	case SI_SET_HOSTNAME:
	{
		size_t len;
		char *p;

		if (pm_denied(u.u_lwpp->l_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}

		if ((error = copyinstr(uap->buf, name, SYS_NMLN, &len)) != 0)
			break;

		/* 
		 * Must be non-NULL string and string must be less
		 * than SYS_NMLN chars.
		 */
		if (len < 2 || (len == SYS_NMLN && name[SYS_NMLN-1] != '\0')) {
			error = EINVAL;
			break;
		}

		setutsname(hostname, name);

		/*
		 * The nodename fields of utsname and xutsname will store only
		 * the first component of what might be a fully-qualified name.
		 */
		for (p = name; *p != '\0'; p++)
			if (*p == '.') {
				*p = '\0';
				break;
			}

		setutsname(utsname.nodename, name);
		name[sizeof(xutsname.nodename) - 1] = '\0';	/* protection */
		setutsname(xutsname.nodename, name);
		rvp->r_val1 = len;
		break;
	}

	case SI_SET_SRPC_DOMAIN:
	{
		size_t len;

		if (pm_denied(u.u_lwpp->l_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}
		if ((error = copyinstr(uap->buf, name, SYS_NMLN, &len)) != 0)
			break;
		/*
		 * If string passed in is longer than length
		 * allowed for domain name, fail.
		 */
		if (len == SYS_NMLN && name[SYS_NMLN-1] != '\0') {
			error = EINVAL;
			break;
		}

		/*
		 * Update the global srpc_domain variable.
		 */
		setutsname(srpc_domain, name);
		rvp->r_val1 = len;
		break;
	}

	default:
		error = EINVAL;
		break;
	}

	return error;
}

/*
 * void getutsname(char *utsnp, char *name)
 *	Return a consistent copy of an element of the utsname
 *	structure, or other related string.
 *
 * Calling/Exit State:
 *	This function acquires the uname_lock in read mode to
 *	obtain a consistent snapshot of the utsname element.
 *	Callers	must be aware of lock hierarchy.  This function
 *	does not block.
 *
 * Remarks:
 *	The passed-in character array 'name' must be large enough to
 *	contain the string.  A size of SYS_NMLN will always suffice.
 *
 *	This function should be used by other kernel components which
 *	need to get a copy of an entry in the utsname structure, or
 *	a related string such as srpc_domain or hostname.
 *
 *	Strings that don't change during operation of the system can
 *	be referenced directly without lock protection, so in such
 *	cases getutsname need not and should not be used.
 */
void
getutsname(char *utsnp, char *name)
{
	pl_t pl;

	pl = RW_RDLOCK(&uname_lock, PL1);
	strcpy(name, utsnp);
	RW_UNLOCK(&uname_lock, pl);
}

/*
 * void setutsname(char *utsnp, char *newname)
 *	Atomically update a member of the utsname structure
 *	or other related string.
 *
 *	The address of the string to be updated is passed in
 *	via 'utsnp'; the new name to be copied in is supplied
 *	via 'newname'.
 *
 * Calling/Exit State:
 *	This function acquires the uname_lock in write mode.
 *	The caller must be aware of lock hierarchy.
 *	
 * Remarks:
 *	The caller must assure that 'newname' is not larger
 *	than the space allocated for 'utsnp'.
 */
void
setutsname(char *utsnp, char *newname)
{
	pl_t	pl;

	pl = RW_WRLOCK(&uname_lock, PL1);
	strcpy(utsnp, newname);
	RW_UNLOCK(&uname_lock, pl);
}
