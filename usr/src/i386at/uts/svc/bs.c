#ident	"@(#)kern-i386at:svc/bs.c	1.20.12.2"
#ident	"$Header$"


/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/*
 * Parse the bootstrap arguments and set kernel variables accordingly.
 */

#include <fs/vfs.h>
#include <io/conf.h>
#include <io/conssw.h>
#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <mem/pmem.h>
#include <mem/vmparam.h>
#include <mem/vm_hat.h>
#include <proc/cred.h>
#include <svc/bootinfo.h>
#include <svc/cpu.h>
#include <svc/copyright.h>
#include <svc/psm.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/mod/mod_obj.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

#define equal(a,b)	(strcmp(a, b) == 0)

int bs_lexcon(const char **, conssw_t **, minor_t *, char **);

/* boot string variables */
#define	MAXPARMS	2		/* number of device parms */

extern char *kernel_name;
extern char *startupmsg, *rebootmsg, *automsg;
extern char *title[MAXTITLE];
extern uint_t ntitle;
extern boolean_t title_changed;
extern char *copyright[MAXCOPYRIGHT];
extern uint_t ncopyright;
extern boolean_t copyright_changed;
extern char *mem_real_msg;
extern char *mem_useable_msg;
extern char *mem_general_msg;
extern char *mem_dedicated_msg;
extern char *sdi_devicenames;
extern char *sdi_lunsearch;
extern char *rm_resmgr;
extern boolean_t rm_invoke_dcu;
extern char *initstate;
extern char *pci_scan;
extern char *psmname;
extern char *cm_bootarg[];
extern uint_t cm_bootarg_count;
extern uint_t cm_bootarg_max;
extern int putbuf_only;
extern boolean_t ignore_machine_check;
extern boolean_t disable_cache;
extern boolean_t disable_pge;
extern int pdi_timeout;
#ifdef USE_GDB
extern boolean_t use_gdb;
#endif /* USE_GDB */

#ifndef MINI_KERNEL
extern boolean_t disable_verify;
#endif

extern boolean_t disable_copy_mtrrs; 

struct bootinfo bootinfo; /* TEMP */

char *bootstring;
int bootstring_size;	/* bytes reserved for bootstring */
int bootstring_actual;  /* actual bytes occupied by bootstring */
STATIC boolean_t bs_relocated = B_FALSE;/* TRUE post relocation of bootstring */

/*
 * void
 * bs_reloc()
 *      map the bootstring into kernel virtual,
 * 	make the page read only.
 *
 * Calling/Exit State:
 *      None
 */
void 
bs_reloc()
{
	/*
	 * map the BKI into kernel virtual BKI.
	 */
	bootstring = (char *) physmap0((paddr_t) bootstring, bootstring_size);

	bs_relocated = B_TRUE;

        /* remove write perm from bootstring page */
	ASSERT(bootstring_size == PAGESIZE);
	ASSERT(((ulong_t)bootstring & PAGEOFFSET) == 0);

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		PG_CLRW(kvtol2ptep64(bootstring));
	} else
#endif /* PAE_MODE */
	{
		PG_CLRW(kvtol2ptep(bootstring));
	}
	TLBSflush1((vaddr_t)bootstring);
}

/*
 * void
 * bs_scan(int (*func)(const char *, const char *, void *), void *arg)
 *	Scan boot parameters, allowing a function to process each one.
 *
 * Calling/Exit State:
 *	None
 */
void
bs_scan(int (*func)(const char *, const char *, void *), void *arg)
{
	const char *s;
	const char *v;
	const char *ebootstring = bootstring + bootstring_actual;
	
	/*
	 * Parse the bootstrap args passed in bootstring.
	 */
	for (s = bootstring; s < ebootstring;) {
		v = s + strlen(s) + 1;
		if ((*func)(s, v, arg) != 0)
			break;
		s = v + strlen(v) + 1;
	}
}

struct getval_arg {
	const char *param;
	const char *value;
};

STATIC int
bs_getval_scan(const char *param, const char *val, void *varg)
{
	struct getval_arg *arg = varg;

	if (equal(param, arg->param)) {
		arg->value = val;
		return 1;
	}

	return 0;
}

/*
 * char *
 * bs_getval(const char *param)
 *	Return the value of the specified boot parameter.
 *
 * Calling/Exit State:
 *	Returns a constant string; no locks required.
 */
char *
bs_getval(const char *param)
{
	struct getval_arg getval_arg;

	getval_arg.param = param;
	getval_arg.value = NULL;
	bs_scan(bs_getval_scan, &getval_arg);
	/*
	 * The bootstring is relocated during system initialisation.
	 * To prevent subtle failures (from using the returned pointer
	 * post-relocation) we allocate memory (never freed) for
	 * pre-relocation callers of this routine [ e.g. PSMs ].
	 */
		
	if (getval_arg.value && (!bs_relocated)) {
		char *s;
		
		s = os_alloc(strlen(getval_arg.value) + 1);
		if (s != NULL)
			(void) strcpy(s, getval_arg.value);
		
		return s;
	}
	
	return (char *)getval_arg.value;
}

/*
 * STATIC int
 * bs_doarg(const char *s, const char *p, void *dummy)
 *	Determine argument and further analyze parameters
 *
 * Calling/Exit State:
 *	Returns	non-zero to abort scan
 *
 *	Currently, we only care about arguments that have the following form:
 *
 *	  case 0 BOOTPROG=<kernel_name>
 *	  case 1 ROOTFS[TYPE]=<fstype>
 *	  case 2 TITLE=<msg>			(may be multiple)
 *	  case 3 COPYRIGHT=<msg>		{may be multiple}
 *	  case 4 STARTUPMSG=<msg>
 *	  case 5 REBOOTMSG=<msg>
 *	  case 6 AUTOMSG=<msg>
 * 	  case 7 CONSOLE=<device>(<minor>[,<paramstr>])
 *        case 9 DEVICENAMES=scsi_device_name,scsi_device_name,...
 *	  case 10 TZ_OFFSET=<seconds>
 *	  case 11 LUNSEARCH=[(C:B,T,L),...]
 *	  case 12 DCU=<YES|NO>
 *	  case 13 RESMGR=<filename>
 *	  case 14 INITSTATE=<str>
 *	  case 15 PCISCAN=<str>
 *	  case 16 RESMGR:<modname>:[<instance>:]<param>=<value>
 *	  case 17 CONMSGS=<YES|NO>
 *	  case 18 IGNORE_MACHINE_CHECK=<YES|NO>
 *	  case 19 DISABLE_CACHE=<YES|NO>
 *	  case 20 DISABLE_PGE=<YES|NO>
 *	  case 21 PDI_TIMEOUT=<YES|NO>
 *	  case 22 PSM=<str>
 *	  case 23 USE_GDB=<YES|NO>
 *	  case 24 RMEM=<msg>
 *	  case 25 UMEM=<msg>
 *	  case 26 PMEM=<msg>
 *	  case 27 DMEM=<msg>
 *	  case 28 HDPARM<n>=<parmstr>
 *	  case 29 DISABLE_VERIFY=<YES|NO>
 *	  case 30 DISABLE_COPY_MTRRS=<YES|NO>
 *
 *	Note that '<>' surround user supplied values; '[]' surround
 *	optional extensions; "parms" are words containing numeric characters;
 *	"devices" are device mnemonics (from bdevsw/cdevsw d_name); and all
 *	other symbols are literal.
 *
 */
STATIC int
bs_doarg(const char *s, const char *p, void *dummy)
{
	int n, i;
	major_t maj;
	long parms[MAXPARMS];
	static const char kprefix[] = "/stand/";

	/* case 0 */
	if (equal(s, "BOOTPROG")) {
		kernel_name = calloc(strlen(p) + sizeof kprefix);
		strcpy(kernel_name, kprefix);
		strcat(kernel_name, p);
		return 0;			/* arg was handled */
	}

	/* case 1 */
	if (equal(s, "ROOTFS") || equal(s, "ROOTFSTYPE")) {
		strncpy(rootfstype, p, ROOTFS_NAMESZ);
		return 0;			/* arg was handled */
	}

	/* case 2 */
	if (equal(s, "TITLE")) {
		if (!title_changed) {
			title_changed = B_TRUE;
			ntitle = 0;
		}
		if (ntitle < MAXTITLE) {
			char *m = calloc(strlen(p) + 1);
			strcpy(m, p);
			title[ntitle++] = m;
		}
		return 0;			/* arg was handled */
	}

	/* case 3 */
	if (equal(s, "COPYRIGHT")) {
		if (!copyright_changed) {
			copyright_changed = B_TRUE;
			ncopyright = 0;
		}
		if (ncopyright < MAXCOPYRIGHT) {
			char *m = calloc(strlen(p) + 1);
			strcpy(m, p);
			copyright[ncopyright++] = m;
		}
		return 0;			/* arg was handled */
	}

	/* case 4 */
	if (equal(s, "STARTUPMSG")) {
		startupmsg = calloc(strlen(p) + 1);
		strcpy(startupmsg, p);
		return 0;			/* arg was handled */
	}

	/* case 5 */
	if (equal(s, "REBOOTMSG")) {
		rebootmsg = calloc(strlen(p) + 1);
		strcpy(rebootmsg, p);
		return 0;			/* arg was handled */
	}

	/* case 6 */
	if (equal(s, "AUTOMSG")) {
		automsg = calloc(strlen(p) + 1);
		strcpy(automsg, p);
		return 0;			/* arg was handled */
	}

	/* case 7: console device override */
	if (equal(s, "CONSOLE")) {
		(void) bs_lexcon((const char **)&p,
				 &consswp, &consminor, &consparamp);
		return 0;			/* arg was handled */
	}

	/* case 9: PDI device names */
	if (equal(s, "DEVICENAMES")) {
		sdi_devicenames = calloc(strlen(p) + 1);
		strcpy(sdi_devicenames, p);
		return 0;			/* arg was handled */
	}

	/* case 10: timezone correction */
	if (equal(s, "TZ_OFFSET")) {
		long correction;
		extern time_t c_correct;

		if (*bs_lexnum(p, &correction) == '\0') {
			c_correct = correction;
			return 0;		/* arg was handled */
		}
	}

	/* case 11: PDI limited lun searching */
	if (equal(s, "LUNSEARCH")) {
		sdi_lunsearch = calloc(strlen(p) + 1);
		strcpy(sdi_lunsearch, p);
		return 0;			/* arg was handled */
	}

	/* case 12: DCU=<YES|NO> */

	if (equal(s, "DCU") && (p[0] == 'Y' || p[0] == 'y')) {
		rm_invoke_dcu = B_TRUE;
		return 0;			/* arg was handled */
	}

	/* case 13: RESMGR=<filename> */

	if (equal(s, "RESMGR")) {
		rm_resmgr = calloc(strlen(p) + 1);
		strcpy(rm_resmgr, p);
		return 0;			/* arg was handled */
	}

	/* case 14: INITSTATE=<str> */

	if (equal(s, "INITSTATE")) {
		initstate = calloc(strlen(p) + 1);
		strcpy(initstate, p);
		return 0;			/* arg was handled */
	}

	/* case 15: PCISCAN=<str> */

	if (equal(s, "PCISCAN")) {
		pci_scan = calloc(strlen(p) + 1); /* arg was handled */
		strcpy(pci_scan, p);
		return 0;
	}

	/*
	 * case 16: RESMGR:<modname>:[<instance>:]<param>=<value> 
	 *
	 * This needs to be parsed before the other arguments because
	 * the keyword is delimited by ':' instead of '='.
	 *
	 * XXX - There may be case problems with <modname> and <param>.
	 * XXX - Should be accessed directly from rm_init().
	 */
	if (strncmp(s, "RESMGR:", 7) == 0) {
		size_t n = strlen(s + 7);
		if (cm_bootarg_count >= cm_bootarg_max)
			return 0;
		cm_bootarg[cm_bootarg_count] = calloc(n + strlen(p) + 2);
		strcpy(cm_bootarg[cm_bootarg_count], s + 7);
		cm_bootarg[cm_bootarg_count][n] = '=';
		strcpy(cm_bootarg[cm_bootarg_count++] + n + 1, p);
		return 0;			/* arg was handled */
	}

	/* case 17: CONMSGS=<YES|NO> */

	if (equal(s, "CONMSGS") && (p[0] == 'N' || p[0] == 'n')) {
		putbuf_only = B_TRUE;
		return 0;			/* arg was handled */
	}

	/* case 18 IGNORE_MACHINE_CHECK=<YES|NO> */

	if (equal(s, "IGNORE_MACHINE_CHECK") && (p[0] == 'Y' || p[0] == 'y')) {
		ignore_machine_check = B_TRUE;
		return 0;			/* arg was handled */
	}

	/* case 19 DISABLE_CACHE=<YES|NO> */

	if (equal(s, "DISABLE_CACHE") && (p[0] == 'Y' || p[0] == 'y')) {
		disable_cache = B_TRUE;
		return 0;			/* arg was handled */
	}

	/* case 20 DISABLE_PGE=<YES|NO> */

	if (equal(s, "DISABLE_PGE") && (p[0] == 'Y' || p[0] == 'y')) {
		disable_pge = B_TRUE;
		return 0;			/* arg was handled */
	}

	/* case 21 PDI_TIMEOUT=<YES|NO> */

	if (equal(s, "PDI_TIMEOUT"))  {
		if (p[0] == 'Y' || p[0] == 'y')
			pdi_timeout = B_TRUE;
		else
			pdi_timeout = B_FALSE;
		return 0;			/* arg was handled */
	}

	/* case 22 PSM=<str> */

	if (equal(s, "PSM"))  {
		psmname = calloc(strlen(p) + 1);
		strcpy(psmname, p);
		return 0;			/* arg was handled */
	}

#ifdef USE_GDB
	/* case 23: USE_GDB=<YES|NO> */

	if (equal(s, "USE_GDB")) {
	  	if (p[0] == 'y' || p[0] == 'Y')
			use_gdb = B_TRUE;
		else
			use_gdb = B_FALSE;
		return 0;
	}
#endif /* USE_GDB */

	/* case 24 RMEM=<msg> */

	if (equal(s, "RMEM")) {
		char *m = calloc(strlen(p) + 1);
		strcpy(m, p);
		mem_real_msg = m;
		return 0;			/* arg was handled */
	}

	/* case 25 UMEM=<msg> */

	if (equal(s, "UMEM")) {
		char *m = calloc(strlen(p) + 1);
		strcpy(m, p);
		mem_useable_msg = m;
		return 0;			/* arg was handled */
	}

	/* case 26 PMEM=<msg> */

	if (equal(s, "PMEM")) {
		char *m = calloc(strlen(p) + 1);
		strcpy(m, p);
		mem_general_msg = m;
		return 0;			/* arg was handled */
	}

	/* case 27 DMEM=<msg> */

	if (equal(s, "RMEM")) {
		char *m = calloc(strlen(p) + 1);
		strcpy(m, p);
		mem_dedicated_msg = m;
		return 0;			/* arg was handled */
	}

 	/* case 28 HDPARM<n>=<parmstr> */

	if (equal(s, "HDPARM0") || equal(s, "HDPARM1")) {
		extern char *hdparmstr[];
		int unit = s[6] - '0';

		hdparmstr[unit] = calloc(strlen(p) + 1);
		strcpy(hdparmstr[unit], p);
		return 0;			/* arg was handled */
	}

#ifndef MINI_KERNEL
	/* case 29 DISABLE_VERIFY=<YES|NO> */

	if (equal(s, "DISABLE_VERIFY") && (p[0] == 'Y' || p[0] == 'y')) {
		disable_verify = B_TRUE;
		return 0;			/* arg was handled */
	}
#endif	

	/* case 30 DISABLE_COPY_MTRRS */

	if (equal(s, "DISABLE_COPY_MTRRS") && (p[0] == 'Y' || p[0] == 'y')) {
		disable_copy_mtrrs = B_TRUE;
		return 0;			
	}

	
	/* case 8: block device overrides */

	n = bs_lexparms(p, parms, MAXPARMS);

	/* unknown argument */

	return 0;
}

/*
 * void
 * bootarg_parse(void)
 *
 *	Parse the bootstrap string and set rootdev and dumpdev.
 *
 * Calling/Exit State:
 *
 *	Called from the initialization process when the system is
 *	still running on the boot processor.  No return value.
 */
void
bootarg_parse(void)
{
	bs_scan(bs_doarg, NULL);
}


STATIC void (*boot_putc)(int chr);

/* ARGSUSED */
STATIC void bc_nop(minor_t minor) {}

/* ARGSUSED */
STATIC dev_t
bc_open(minor_t minor, boolean_t syscon, const char *params)
{
	return 0;
}

/* ARGSUSED */
STATIC int
bc_putc(minor_t minor, int chr)
{
	boot_putc(chr);
	return 1;
}

STATIC conssw_t bootcons = {
	bc_open,	/* open */
	(void (*)())bc_nop,	/* close */
	bc_putc,	/* putc */
	0,		/* getc */
	bc_nop,		/* suspend */
	bc_nop,		/* resume */
	-1		/* cpu */
};

vaddr_t ksym_base;

/*
 * STATIC int
 * bs_bootinfo(const char *s, const char *p, void *dummy)
 *      Synthesize the remnant of the bootinfo structure which
 *      ***STILL*** remains in the kernel.
 *
 * Calling/Exit State:
 *      Called from the boot engine before page tables are built.
 */

STATIC int
bs_bootinfo(const char *s, const char *p, void *dummy)
{
	static struct modobj _modobj_kern;
	long val;
	uint_t symtab, symtabsz, strtab, strtabsz, symentsz, globidx;
	paddr_t base;
	extern struct modobj *mod_obj_kern;
	extern size_t mod_obj_size;
	extern char _symtab_sz[];
	extern char _end[];
	
	/* TEMP: Construct fake bootinfo from corresponding new parameters */

	if (equal(s, "BOOTDEV")) {
		if (p[0] == 'f')
			bootinfo.bootflags |= BF_FLOPPY;
		return 0;
	}
	if (equal(s, "BUSTYPES")) {
		while (*p) {
			if (strncmp(p, "ISA", 3) == 0)
				bootinfo.machflags |= AT_BUS;
			else if (strncmp(p, "EISA", 4) == 0)
				bootinfo.machflags |= EISA_IO_BUS;
			else if (strncmp(p, "MCA", 3) == 0)
				bootinfo.machflags |= MC_BUS;
			if ((p = strchr(p, ',')) == NULL)
				break;
			bootinfo.machflags |= BUS_BRIDGED;
			++p;
		}
		return 0;
	}
	if (equal(s, "KSYM"))  {
		p = bs_lexnum(p, &val) + 1;
		symtab = val;
		p = bs_lexnum(p, &val) + 1;
		symtabsz = val;
		p = bs_lexnum(p, &val) + 1;
		strtab = val;
		p = bs_lexnum(p, &val) + 1;
		strtabsz = val;
		p = bs_lexnum(p, &val) + 1;
		symentsz = val;
		(void)bs_lexnum(p, &val);
		globidx = val;

		mod_obj_kern = &_modobj_kern;
		_modobj_kern.md_symspace = (char *)symtab;
		_modobj_kern.md_symsize = symtabsz;
		_modobj_kern.md_strings = (char *)strtab;
		_modobj_kern.md_symentsize = symentsz;
		_modobj_kern.md_nsyms = symtabsz / symentsz;
		_modobj_kern.md_globidx = globidx;
		_modobj_kern.md_space = (char *) kvbase;
		_modobj_kern.md_space_size = (vaddr_t) _end - kvbase;

		mod_obj_size = (size_t)_symtab_sz;
		return 0;
	}
	return 0;
}

/*
 * STATIC int
 * bs_bootcons(const char *s, const char *p, void *dummy)
 *	Get the addresses of the boot loader's get and put
 *	functions.
 *
 * Calling/Exit State:
 *	Called from the boot engine before page tables are built.
 *
 *	Not really part of bootinfo, but needs to be done early...
 */

STATIC int
bs_bootcons(const char *s, const char *p, void *dummy)
{
	if (equal(s, "BOOTCONS")) {
		p = bs_lexnum(p, (long *)&boot_putc);
		if (*p == ',')
			(void)bs_lexnum(p + 1, (long *)&bootcons.cn_getc);
		console_openchan(&conschan, &bootcons, 0, NULL, B_TRUE);
		PHYS_PRINT("bs_bootcons: console open!\n");
	}

	return 0;
}
/*
 * STATIC void
 * strlshift(char *s)
 *	left shift the string one char, overwriting *s
 *	use ovbcopy to guarantee correctness.
 *
 * Calling/Exit State:
 *	none.
 */
STATIC void
strlshift(char *s)
{
	/* actually strlen(s+1) + 1 */
	ovbcopy(s+1, s, strlen(s));
}

/*
 * STATIC void
 * bs_fixup()
 *	Change delimiting occurences of '=' & '\n' to '\0' in the
 *	bootstring.
 *
 * Calling/Exit State:
 *	Called from the boot engine before page tables are built.
 */
STATIC void
bs_fixup()
{
	char *s = bootstring;
	boolean_t value = B_FALSE; /* TRUE if examining rhs of "NAME=value" */
	
	while(*s) {
		switch (*s) {
                case '\n':
                        if (s>bootstring && *(s-1) == '\\') {
				/* escaped \n; delete '\\' */
				strlshift(s-1);
				continue;
                        } else {
				if (!value) {
					/* not part of value; delete */
					strlshift(s);
					continue;
				} else {
					/* delimiting '\n' */
					*s='\0';
					value = B_FALSE;
				}
			}
                        break;

                case '=':
                        if (!value) { /* genuine = i.e. not part of value */
                                *s='\0';
                                value = B_TRUE;
                        }
                        break;
                }
                s++;
        }

	bootstring_actual = s - bootstring;

}

/*
 * void
 * make_bootinfo(void)
 *
 * Calling/Exit State:
 *	Called from the boot engine before page tables are built.
 */
void
make_bootinfo(void)
{
	bs_fixup();
	bs_scan(bs_bootcons, NULL);
	bs_scan(bs_bootinfo, NULL);
}

/*
 * int
 * bs_lexparms(const char *s, long *parms, int maxparms)
 *	Extract numeric parameters, delimited by parens, from an argument string
 *
 * Calling/Exit State:
 *		s = input string pointer
 *	    parms = pointer to array of longs
 *	 maxparms = size of parms array
 *
 *	returns	0 = error
 *		n = number of parameters found
 */
int
bs_lexparms(const char *s, long *parms, int maxparms)
{
	char tmp[500];
	char *p;
	int n = 0;

	strncpy(tmp, s, sizeof tmp - 1);
	tmp[sizeof tmp - 1] = '\0';
	
	if ((p = strchr(tmp, '(')) == (char *)0)
		return 0;
	*p = '\0';			/* delimit anything prior to '(' */
	do {
		p++;
		if (maxparms-- == 0)
			return 0;
		p = bs_lexnum((const char *)p, &parms[n]); /* extract number */
		if (p == (char *)0)  		/* no parm error */
			return 0;
		if (*p != ','  &&  *p != ')')
			return 0;
		n++;
	} while (*p == ',');			/* while more parms */
	return n;
}


/*
 * int
 * bs_lexcon(const char **strp, conssw_t **cswp, minor_t *mp, char **paramstrp)
 *	Parse a console device string
 *
 * Calling/Exit State:
 *	Called with no locks held, at console open time.
 *
 * Description:
 *	Parses a string of the form:
 *
 *		<devname>(<minor>[,<paramstr>])
 *
 *	where <devname> is a console-capable device name
 *	      <minor> is the minor number of that device (in decimal)
 *	      <paramstr> is an optional, device-specific, parameter string,
 *			which may include nested, paired parentheses
 *
 *	The input string at *strp does not have to be terminated
 *	at the end of this construct; *strp will be left pointing
 *	to the first character after the closing parenthesis.
 *
 *	*cswp will be set to the conssw_t structure for the named device.
 *
 *	*mp will be set to the value of <minor>.
 *
 *	If paramstrp is non-NULL, *paramstrp will be set to a privately-
 *	allocated copy of <paramstr>.
 *
 *	Returns non-zero if the string was successfully parsed.
 */
int
bs_lexcon(const char **strp, conssw_t **cswp, minor_t *mp, char **paramstrp)
{
	const char *s;
	const char *s2;
	char *p;
	int nest = 0;
	int i;
	long num;

	if ((s = strchr(*strp, '(')) == (char *)0)
		return 0;
	/* find device name in constab */
	for (i = 0;; i++) {
		if (i == conscnt)
			return 0;
		for (s2 = *strp, p = constab[i].cn_name; s2 != s; ++s2, ++p) {
			if (*s2 != *p)
				break;
		}
		if (s2 == s)
			break;
	}
	*cswp = constab[i].cn_consswp;
	++s;
	/* extract minor number */
	if ((s = bs_lexnum(s, &num)) == 0)
		return 0;
	*mp = (minor_t)num;
	/* if no parameter string, return now */
	if (*s != ')' && *s++ != ',')
		return 0;
	/* parse parameter string; find terminating matching paren */
	for (s2 = s, nest = 0; *s2 != ')' || nest != 0; ++s2) {
		if (*s2 == '\0')
			return 0;
		if (*s2 == '(')
			++nest;
		else if (*s2 == ')')
			--nest;
	}
	if (paramstrp != NULL) {
		/* allocate memory for parameter string and copy it */
		p = consmem_alloc(s2 - s + 1, 0);
		if (p == NULL)
			return 0;
		bcopy(s, p, s2 - s);
		p[s2 - s] = '\0';
		*paramstrp = p;
	}
	*strp = s2 + 1;
	return 1;
}

/*
 * char *
 * bs_lexnum(const char *p, long *parm)
 *	Extract number from string (works with binary, octal, decimal, or hex)
 *
 * Calling/Exit State:
 *	     p = pointer to input string
 *	  parm = pointer to value storage
 *
 *	returns 0 = error
 *		else pointer to next non-numeric character in input string
 *
 *	binary constants are preceded by a '0b'
 *	octal constants are preceded by a '0'
 *	hex constants are preceded by a '0x'
 *	all others are assumed decimal
 */
char *
bs_lexnum(const char *p, long *parm)
{
	char *q;
	ulong_t v;

	v = strtoul(p, &q, 0);
	if (q == p)
		return (char *)0;
	*parm = (long)v;
	return q;
}
