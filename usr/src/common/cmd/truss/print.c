/*		copyright	"%c%" 	*/

#ident	"@(#)truss:common/cmd/truss/print.c	1.9.12.1"
#ident  "$Header$"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/uio.h>
#ifdef RFS_SUPPORT
#include <sys/nserve.h>
#include <sys/rf_sys.h>
#endif /* RFS_SUPPORT */
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/ulimit.h>
#include <sys/utsname.h>
#include <stropts.h>
#include <sys/mod.h>
#include <sys/cguser.h>
#include <sys/processor.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <sys/systeminfo.h>
#include "pcontrol.h"
#include "print.h"
#include "ramdata.h"
#include "systable.h"
#include "proto.h"
#include "machdep.h"

/*
 * Each prt_xxx routine in this file is responsible for `printing'
 * an argument or return value of a particular type.  The routine
 * returns 1 if it actually printed something, and 0 otherwise.
 * By `printing', we really mean formatting the argument for printing
 * and appending it to the sys_string buffer.  Actual printing of the
 * resulting string takes place elsewhere.
 *
 * Almost all of the routines return 1; the exception is for special
 * cases such as hidden arguments (which aren't printed) and arguments
 * of long long types that must be constructed during successive calls
 * and printed once both words have been obtained.
 */

/*
 * Function prototypes for static routines in this module.
 */
#if	defined(__STDC__)

static	int	prt_nov( int , int );
static	int	prt_oct( int , int );
static	int	prt_hex( int , int );
static	int	prt_hhx( int , int );
static	int	prt_dex( int , int );
static	int	prt_stg( int , int );
static	int	prt_rst( int , int );
static	int	prt_rlk( int , int );
static	int	prt_ioc( int , int );
static	int	prt_ioa( int , int );
static	int	prt_fcn( int , int );
static	int	prt_uts( int , int );
static	int	prt_msc( int , int );
static	int	prt_msf( int , int );
static	int	prt_sec( int , int );
static	int	prt_sef( int , int );
static	int	prt_shc( int , int );
static	int	prt_shf( int , int );
static	int	prt_sfs( int , int );
static	int	prt_opn( int , int );
static	int	prt_sig( int , int );
static	int	prt_six( int , int );
static	int	prt_act( int , int );
static	int	prt_smf( int , int );
#ifdef RFS_SUPPORT
static	int	prt_rfs( int , int );
static	int	prt_rv1( int , int );
static	int	prt_rv2( int , int );
static	int	prt_rv3( int , int );
#endif /* RFS_SUPPORT */
static	int	prt_plk( int , int );
static	int	prt_mtf( int , int );
static	int	prt_mft( int , int );
static	int	prt_iob( int , int );
static	int	prt_iov( int , int );
static	int	prt_wop( int , int );
static	int	prt_spm( int , int );
static	int	prt_mpr( int , int );
static	int	prt_mty( int , int );
static	int	prt_mcf( int , int );
static	int	prt_mc4( int , int );
static	int	prt_mc5( int , int );
static	int	prt_mad( int , int );
static	int	prt_ulm( int , int );
static	int	prt_rlm( int , int );
static	int	prt_cnf( int , int );
static	int	prt_inf( int , int );
static	int	prt_ptc( int , int );
static	int	prt_fui( int , int );
static	void	grow( int );
static	CONST char *	mmap_protect( int );
static	CONST char *	mmap_type( int );
static 	int	prt_boo(int, int);
static 	int	prt_mdc(int, int);
static 	int	prt_mdt(int, int);
static	int	prt_ihx(int, int);
static	int	prt_idt(int, int);
static	int	prt_dshc(int, int);
static	int	prt_dshf(int, int);
static	int	prt_cgl(int, int);
static	int	prt_cgh(int, int);
static	int	prt_cgs(int, int);
static	int	prt_cgf(int, int);
static	int	prt_cgp(int, int);
static	int	prt_dlx(int, int);
static	int	prt_dhx(int, int);
static	int	prt_dc2(int, int);
static	int	prt_hx2(int, int);
static	int	prt_wst(int, int);

#else	/* defined(__STDC__) */

static	int	prt_nov();
static	int	prt_oct();
static	int	prt_hex();
static	int	prt_hhx();
static	int	prt_dex();
static	int	prt_stg();
static	int	prt_rst();
static	int	prt_rlk();
static	int	prt_ioc();
static	int	prt_ioa();
static	int	prt_fcn();
static	int	prt_uts();
static	int	prt_msc();
static	int	prt_msf();
static	int	prt_sec();
static	int	prt_sef();
static	int	prt_shc();
static	int	prt_shf();
static	int	prt_sfs();
static	int	prt_opn();
static	int	prt_sig();
static	int	prt_six();
static	int	prt_act();
static	int	prt_smf();
#ifdef RFS_SUPPORT
static	int	prt_rfs();
static	int	prt_rv1();
static	int	prt_rv2();
static	int	prt_rv3();
#endif /* RFS_SUPPORT */
static	int	prt_plk();
static	int	prt_mtf();
static	int	prt_mft();
static	int	prt_iob();
static	int	prt_iov();
static	int	prt_wop();
static	int	prt_spm();
static	int	prt_mpr();
static	int	prt_mty();
static	int	prt_mcf();
static	int	prt_mc4();
static	int	prt_mc5();
static	int	prt_mad();
static	int	prt_ulm();
static	int	prt_rlm();
static	int	prt_cnf();
static	int	prt_inf();
static	int	prt_ptc();
static	int	prt_fui();
static	void	grow();
static	CONST char *	mmap_protect();
static	CONST char *	mmap_type();
static	int	prt_boo();
static	int	prt_mdc();
static	int	prt_mdt();
static	int	prt_ihx();
static	int	prt_idt();
static	int	prt_dshc();
static	int	prt_dshf();
static	int	prt_cgl();
static	int	prt_cgh();
static	int	prt_cgs();
static	int	prt_cgf();
static	int	prt_cgp();
static	int	prt_dlx();
static	int	prt_dhx();
static	int	prt_dc2();
static	int	prt_hx2();
static	int	prt_wst();

#endif	/* defined(__STDC__) */

#define GROW(nb) if (sys_leng+(nb) >= sys_ssize) grow(nb)

/*ARGSUSED*/
static int
prt_nov(val, raw)	/* print nothing */
int val;
int raw;
{
	return 0;
}

/*ARGSUSED*/
int
prt_dec(val, raw)	/* print as decimal */
int val;
int raw;
{
	GROW(12);
	sys_leng += sprintf(sys_string+sys_leng, "%d", val);
	return 1;
}

/*ARGSUSED*/
static int
prt_oct(val, raw)	/* print as octal */
int val;
int raw;
{
	GROW(12);
	sys_leng += sprintf(sys_string+sys_leng, "%#o", val);
	return 1;
}

/*ARGSUSED*/
static int
prt_hex(val, raw)	/* print as hexadecimal */
int val;
int raw;
{
	GROW(10);
	sys_leng += sprintf(sys_string+sys_leng, "0x%.8X", val);
	return 1;
}

/*ARGSUSED*/
static int
prt_hhx(val, raw)	/* print as hexadecimal (half size) */
int val;
int raw;
{
	GROW(10);
	sys_leng += sprintf(sys_string+sys_leng, "0x%.4X", val);
	return 1;
}

/*ARGSUSED*/
static int
prt_dex(val, raw)	/* print as decimal if small, else hexadecimal */
int val;
int raw;
{
	return (val & 0xff000000)? prt_hex(val, 0) : prt_dec(val, 0);
}

static int
prt_stg(val, raw)	/* print as string */
int val;
int raw;
{
	register char * s = raw? NULL : fetchstring((long)val, 400);

	if (s == NULL)
		return prt_hex(val, 0);

	GROW((int)strlen(s)+2);
	sys_leng += sprintf(sys_string+sys_leng, "\"%s\"", s);
	return 1;
}

static int
prt_rst(val, raw)	/* print as string returned from syscall */
int val;
int raw;
{
	register char * s = (raw || Errno)? NULL : fetchstring((long)val, 400);

	if (s == NULL)
		return prt_hex(val, 0);

	GROW((int)strlen(s)+2);
	sys_leng += sprintf(sys_string+sys_leng, "\"%s\"", s);
	return 1;
}

static int
prt_rlk(val, raw)	/* print contents of readlink() buffer */
int val;		/* address of buffer */
int raw;
{
	register char * s = (raw || Errno || Rval1 <= 0)? NULL :
				fetchstring((long)val, (Rval1>400)?400:Rval1);

	if (s == NULL)
		return prt_hex(val, 0);

	GROW((int)strlen(s)+2);
	sys_leng += sprintf(sys_string+sys_leng, "\"%s\"", s);
	return 1;
}

static int
prt_ioc(val, raw)	/* print ioctl code */
int val;
int raw;
{
	register CONST char * s = raw? NULL : ioctlname(val);

	if (s == NULL)
		return prt_hex(val, 0);

	outstring(s);
	return 1;
}

static int
prt_ioa(val, raw)	/* print ioctl argument */
int val;
int raw;
{
	register CONST char * s;

	switch(sys_args[1]) {	/* cheating -- look at the ioctl() code */

	/* streams ioctl()s */
	case I_LOOK:
		return prt_rst(val, raw);
	case I_PUSH:
	case I_FIND:
		return prt_stg(val, raw);
	case I_LINK:
	case I_UNLINK:
	case I_SENDFD:
		return prt_dec(val, 0);
	case I_SRDOPT:
		if (raw || (s = strrdopt(val)) == NULL)
			return prt_dec(val, 0);
		outstring(s);
		return 1;
	case I_SETSIG:
		if (raw || (s = strevents(val)) == NULL)
			return prt_hex(val, 0);
		outstring(s);
		return 1;
	case I_FLUSH:
		if (raw || (s = strflush(val)) == NULL)
			return prt_dec(val, 0);
		outstring(s);
		return 1;

	/* tty ioctl()s */
	case TCSBRK:
	case TCXONC:
	case TCFLSH:
	case TCDSET:
		return prt_dec(val, 0);

	default:
		return prt_hex(val, 0);
	}

	/*NOTREACHED*/
}

static int
prt_fcn(val, raw)	/* print fcntl code */
int val;
int raw;
{
	register CONST char * s = raw? NULL : fcntlname(val);

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_uts(val, raw)	/* print utssys code */
int val;
int raw;
{
	register CONST char * s = raw? NULL : utscode(val);

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_msc(val, raw)	/* print msgsys command */
int val;
int raw;
{
	register CONST char * s = raw? NULL : msgcmd(val);

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_msf(val, raw)	/* print msgsys flags */
int val;
int raw;
{
	register CONST char * s = raw? NULL : msgflags(val);

	if (s == NULL)
		return prt_oct(val, 0);

	outstring(s);
	return 1;
}

static int
prt_sec(val, raw)	/* print semsys command */
int val;
int raw;
{
	register CONST char * s = raw? NULL : semcmd(val);

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_sef(val, raw)	/* print semsys flags */
int val;
int raw;
{
	register CONST char * s = raw? NULL : semflags(val);

	if (s == NULL)
		return prt_oct(val, 0);

	outstring(s);
	return 1;
}

static int
prt_shc(val, raw)	/* print shmsys command */
int val;
int raw;
{
	register CONST char * s = raw? NULL : shmcmd(val);

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_shf(val, raw)	/* print shmsys flags */
int val;
int raw;
{
	register CONST char * s = raw? NULL : shmflags(val);

	if (s == NULL)
		return prt_oct(val, 0);

	outstring(s);
	return 1;
}

static int
prt_sfs(val, raw)	/* print sysfs code */
int val;
int raw;
{
	register CONST char * s = raw? NULL : sfsname(val);

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

#ifdef RFS_SUPPORT
static int
prt_rfs(val, raw)	/* print rfsys code */
int val;
int raw;
{
	register CONST char * s = raw? NULL : rfsysname(val);

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}
#endif /* RFS_SUPPORT */

static int
prt_opn(val, raw)	/* print open code */
int val;
int raw;
{
	register CONST char * s = raw? NULL : openarg(val);

	if (s == NULL)
		return prt_oct(val, 0);

	outstring(s);
	return 1;
}

static int
prt_sig(val, raw)	/* print signal name plus flags */
int val;
int raw;
{
	register CONST char * s = raw? NULL : sigarg(val);

	if (s == NULL)
		return prt_hex(val, 0);

	outstring(s);
	return 1;
}

static int
prt_six(val, raw)	/* print signal name, masked with SIGNO_MASK */
int val;
int raw;
{
	register CONST char * s = raw? NULL : sigarg(val & SIGNO_MASK);

	if (s == NULL)
		return prt_hex(val, 0);

	outstring(s);
	return 1;
}

static int
prt_act(val, raw)	/* print signal action value */
int val;
int raw;
{
	register CONST char * s;

	if (raw)
		s = NULL;
	else if (val == (int)SIG_DFL)
		s = "SIG_DFL";
	else if (val == (int)SIG_IGN)
		s = "SIG_IGN";
	else if (val == (int)SIG_HOLD)
		s = "SIG_HOLD";
	else
		s = NULL;

	if (s == NULL)
		return prt_hex(val, 0);

	outstring(s);
	return 1;
}

static int
prt_smf(val, raw)	/* print streams message flags */
int val;
int raw;
{
	switch (val) {
	case 0:
		return prt_dec(val, 0);
	case RS_HIPRI:
		if (raw)
			return prt_hhx(val, 0);
		outstring("RS_HIPRI");
		return 1;
	default:
		return prt_hhx(val, 0);
	}

	/*NOTREACHED*/
}

#ifdef RFS_SUPPORT
static int
prt_rv1(val, raw)	/* print RFS verification argument */
int val;
int raw;
{
	register CONST char * s = NULL;

	switch (val) {
	case V_SET:
		s = "V_SET";
		break;
	case V_CLEAR:
		s = "V_CLEAR";
		break;
	case V_GET:
		s = "V_GET";
		break;
	}

	if (raw || s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_rv2(val, raw)	/* print RFS version argument */
int val;
int raw;
{
	register CONST char * s = NULL;

	switch (val) {
	case VER_CHECK:
		s = "VER_CHECK";
		break;
	case VER_GET:
		s = "VER_GET";
		break;
	}

	if (raw || s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

/*ARGSUSED*/
static int
prt_rv3(val, raw)	/* print RFS tuneable argument */
int val;
int raw;
{
	register CONST char * s = NULL;

#ifndef SVR3
	switch (val) {
	case T_NSRMOUNT:
		s = "T_NSRMOUNT";
		break;
	case T_NADVERTISE:
		s = "T_NADVERTISE";
		break;
	}
#endif

	if (raw || s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}
#endif /* RFS_SUPPORT */

static int
prt_plk(val, raw)	/* print plock code */
int val;
int raw;
{
	register CONST char * s = raw? NULL : plockname(val);

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_mtf(val, raw)	/* print mount flags */
int val;
int raw;
{
	register CONST char * s = raw? NULL : mountflags(val);

	if (s == NULL)
		return prt_hex(val, 0);

	outstring(s);
	return 1;
}

static int
prt_mft(val, raw)	/* print mount file system type */
int val;
int raw;
{
	if (val >= 0 && val < 256)
		return prt_dec(val, 0);

	if (raw)
		return prt_hex(val, 0);

	return prt_stg(val, raw);
}

static int
prt_iob(val, raw)	/* print contents of read() or write() I/O buffer */
register int val;	/* address of I/O buffer (sys_args[1]) */
int raw;
{
	register process_t *Pr = &Proc;
	register int fdp1 = sys_args[0]+1;
	register int nbyte = (Pr->why.pr_lwp.pr_what==SYS_write ||
			      Pr->why.pr_lwp.pr_what==SYS_pwrite) ?
				      sys_args[2] : (Errno? 0 : Rval1);
	register int elsewhere = FALSE;		/* TRUE iff dumped elsewhere */
	char buffer[IOBSIZE];
	int nb;

	iob_buf[0] = '\0';

	if (Pr->why.pr_lwp.pr_why == PR_SYSEXIT && nbyte > IOBSIZE) {
		switch (Pr->why.pr_lwp.pr_what) {
		case SYS_read:
		case SYS_pread:
			elsewhere = prismember(&readfd, fdp1);
			break;
		case SYS_write:
		case SYS_pwrite:
			elsewhere = prismember(&writefd, fdp1);
			break;
		}
	}

	if (nbyte <= 0 || elsewhere)
		return prt_hex(val, 0);

	nb = nbyte>IOBSIZE? IOBSIZE : nbyte;

	if (Pread(Pr, (ulong_t)val, buffer, nb) != nb)
		return prt_hex(val, 0);

	iob_buf[0] = '"';
	showbytes(buffer, nb, iob_buf+1);
	(void)strcat(iob_buf,
		     (nb == nbyte)?
		     (CONST char *)"\"" : (CONST char *)"\"..");
	if (raw)
		return prt_hex(val, 0);

	outstring(iob_buf);
	return 1;
}

#define IOVSIZE 8
static int
prt_iov(val, raw)	/* print contents of readv() or writev() I/O buffer */
int val;		/* address of I/O buffer (sys_args[1]) */
int raw;
{
	process_t *Pr = &Proc;
	int fdp1 = sys_args[0]+1;
	int veclen = sys_args[2];
	int printbytes, availbytes, i;
	struct iovec vec[16];
	char buf[2*IOVSIZE+IOVSIZE*7], *p = buf;

	if ((Pr->why.pr_lwp.pr_what==SYS_readv && Errno==0 &&
	     prismember(&readfd,fdp1)) ||
	    (Pr->why.pr_lwp.pr_what==SYS_writev &&
	     prismember(&writefd,fdp1)) ||
	    raw || veclen <= 0 || veclen > 16) {
		return prt_hex(val, 0);
	}

	if (Pread(Pr, (ulong_t)val, vec, veclen * sizeof *vec) !=
	    veclen * sizeof *vec) {
		return prt_hex(val, 0);
	}

	availbytes = 0;
	if (Pr->why.pr_lwp.pr_what == SYS_writev)
		for (i=0; i<veclen; ++i) {
			if (vec[i].iov_len < 0)
				return prt_hex(val, 0);
			availbytes += vec[i].iov_len;
		}
	else if (!Errno)
		availbytes = Rval1;

	printbytes = (availbytes > IOVSIZE) ? IOVSIZE : availbytes;

	p += sprintf(p, "{ ");
	for (i=0; i<veclen; ++i) {
		char buffer[IOVSIZE];
		int nb;

		if (printbytes <= 0) {
			*p++ = '.';
			*p++ = '.';
			break;
		}

		nb = vec[i].iov_len;
		if (nb > printbytes)
			nb = printbytes;

		if (nb <= 0 ||
		    Pread(Pr, (ulong_t)vec[i].iov_base, buffer, nb) != nb)
			p += sprintf(p, "0x%.8X", (int)vec[i].iov_base);
		else {
			*p++ = '"';
			showbytes(buffer, nb, p);
			p += nb * 2;
			*p++ = '"';
			if (vec[i].iov_len > nb && availbytes > nb) {
				/* could have printed more */
				*p++ = '.';
				*p++ = '.';
			}
		}
		p += sprintf(p, ", %d, ", vec[i].iov_len);
		printbytes -= nb;
		availbytes -= nb;
	}
	p += sprintf(p, "}");
	outstring(buf);
	return 1;
}

static int
prt_wop(val, raw)	/* print waitsys() options */
int val;
int raw;
{
	register CONST char * s = raw? NULL : woptions(val);

	if (s == NULL)
		return prt_oct(val, 0);

	outstring(s);
	return 1;
}

/*ARGSUSED*/
static int
prt_spm(val, raw)	/* print sigprocmask argument */
int val;
int raw;
{
	register CONST char * s = NULL;

	if (!raw) {
		switch (val) {
		case SIG_BLOCK:		s = "SIG_BLOCK";	break;
		case SIG_UNBLOCK:	s = "SIG_UNBLOCK";	break;
		case SIG_SETMASK:	s = "SIG_SETMASK";	break;
		}
	}

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static CONST char *
mmap_protect(arg)
register int arg;
{
	register char * str = code_buf;

	if (arg & ~(PROT_READ|PROT_WRITE|PROT_EXEC))
		return (char *)NULL;
	
	if (arg == PROT_NONE)
		return "PROT_NONE";

	*str = '\0';
	if (arg & PROT_READ)
		(void) strcat(str, "|PROT_READ");
	if (arg & PROT_WRITE)
		(void) strcat(str, "|PROT_WRITE");
	if (arg & PROT_EXEC)
		(void) strcat(str, "|PROT_EXEC");
	return (CONST char *)(str+1);
}

static CONST char *
mmap_type(arg)
register int arg;
{
	register char * str = code_buf;

	switch (arg&MAP_TYPE) {
	case MAP_SHARED:
		(void) strcpy(str, "MAP_SHARED");
		break;
	case MAP_PRIVATE:
		(void) strcpy(str, "MAP_PRIVATE");
		break;
	default:
		(void) sprintf(str, "%d", arg&MAP_TYPE);
		break;
	}

	arg &= ~(_MAP_NEW|MAP_TYPE);

	if (arg & ~(MAP_FIXED|MAP_RENAME|MAP_NORESERVE))
		(void) sprintf(str+strlen(str), "|0x%X", arg);
	else {
		if (arg & MAP_FIXED)
			(void) strcat(str, "|MAP_FIXED");
		if (arg & MAP_RENAME)
			(void) strcat(str, "|MAP_RENAME");
		if (arg & MAP_NORESERVE)
			(void) strcat(str, "|MAP_NORESERVE");
	}

	return (CONST char *)str;
}

static int
prt_mpr(val, raw)	/* print mmap()/mprotect() flags */
int val;
int raw;
{
	register CONST char * s = raw? NULL : mmap_protect(val);

	if (s == NULL)
		return prt_hhx(val, 0);

	outstring(s);
	return 1;
}

static int
prt_mty(val, raw)	/* print mmap() mapping type flags */
int val;
int raw;
{
	register CONST char * s = raw? NULL : mmap_type(val);

	if (s == NULL)
		return prt_hhx(val, 0);

	outstring(s);
	return 1;
}

/*ARGSUSED*/
static int
prt_mcf(val, raw)	/* print memcntl() function */
int val;
int raw;
{
	register CONST char * s = NULL;

	if (!raw) {
		switch (val) {
		case MC_SYNC:		s = "MC_SYNC";		break;
		case MC_LOCK:		s = "MC_LOCK";		break;
		case MC_UNLOCK:		s = "MC_UNLOCK";	break;
		case MC_ADVISE:		s = "MC_ADVISE";	break;
		case MC_LOCKAS:		s = "MC_LOCKAS";	break;
		case MC_UNLOCKAS:	s = "MC_UNLOCKAS";	break;
		}
	}

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_mc4(val, raw)	/* print memcntl() (fourth) argument */
int val;
int raw;
{
	register char * s = NULL;

	if (val == 0)
		return prt_dec(val, 0);

	if (raw)
		return prt_hhx(val, 0);

	switch (sys_args[2]) { /* cheating -- look at memcntl func */
	case MC_SYNC:
		if ((val & ~(MS_ASYNC|MS_INVALIDATE)) == 0) {
			*(s = code_buf) = '\0';
			if (val & MS_ASYNC)
				(void) strcat(s, "|MS_ASYNC");
			if (val & MS_INVALIDATE)
				(void) strcat(s, "|MS_INVALIDATE");
		}
		break;

	case MC_LOCKAS:
	case MC_UNLOCKAS:
		if ((val & ~(MCL_CURRENT|MCL_FUTURE)) == 0) {
			*(s = code_buf) = '\0';
			if (val & MCL_CURRENT)
				(void) strcat(s, "|MCL_CURRENT");
			if (val & MCL_FUTURE)
				(void) strcat(s, "|MCL_FUTURE");
		}
		break;
	}

	if (s == NULL)
		return prt_hhx(val, 0);

	outstring(++s);
	return 1;
}

static int
prt_mc5(val, raw)	/* print memcntl() (fifth) argument */
int val;
int raw;
{
	register char * s;

	if (val == 0)
		return prt_dec(val, 0);

	if (raw || (val & ~VALID_ATTR))
		return prt_hhx(val, 0);

	s = code_buf;
	*s = '\0';
	if (val & SHARED)
		strcat(s, "|SHARED");
	if (val & PRIVATE)
		strcat(s, "|PRIVATE");
	if (val & PROT_READ)
		(void) strcat(s, "|PROT_READ");
	if (val & PROT_WRITE)
		(void) strcat(s, "|PROT_WRITE");
	if (val & PROT_EXEC)
		(void) strcat(s, "|PROT_EXEC");

	if (*s == '\0')
		return prt_hhx(val, 0);

	outstring(++s);
	return 1;
}

static int
prt_mad(val, raw)	/* print madvise() argument */
int val;
int raw;
{
	register CONST char * s = NULL;

	if (!raw) {
		switch (val) {
		case MADV_NORMAL:	s = "MADV_NORMAL";	break;
		case MADV_RANDOM:	s = "MADV_RANDOM";	break;
		case MADV_SEQUENTIAL:	s = "MADV_SEQUENTIAL";	break;
		case MADV_WILLNEED:	s = "MADV_WILLNEED";	break;
		case MADV_DONTNEED:	s = "MADV_DONTNEED";	break;
		}
	}

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_ulm(val, raw)	/* print ulimit() argument */
int val;
int raw;
{
	register CONST char * s = NULL;

	if (!raw) {
		switch (val) {
		case UL_GFILLIM:	s = "UL_GFILLIM";	break;
		case UL_SFILLIM:	s = "UL_SFILLIM";	break;
		case UL_GMEMLIM:	s = "UL_GMEMLIM";	break;
		case UL_GDESLIM:	s = "UL_GDESLIM";	break;
		}
	}

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_rlm(val, raw)	/* print get/setrlimit() argument */
int val;
int raw;
{
	register CONST char * s = NULL;

	if (!raw) {
		switch (val) {
		case RLIMIT_CPU:	s = "RLIMIT_CPU";	break;
		case RLIMIT_FSIZE:	s = "RLIMIT_FSIZE";	break;
		case RLIMIT_DATA:	s = "RLIMIT_DATA";	break;
		case RLIMIT_STACK:	s = "RLIMIT_STACK";	break;
		case RLIMIT_CORE:	s = "RLIMIT_CORE";	break;
		case RLIMIT_NOFILE:	s = "RLIMIT_NOFILE";	break;
		case RLIMIT_VMEM:	s = "RLIMIT_VMEM";	break;
		}
	}

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_cnf(val, raw)	/* print sysconfig code */
int val;
int raw;
{
	register CONST char * s = raw? NULL : sconfname(val);

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_inf(val, raw)	/* print systeminfo code */
int val;
int raw;
{
	register CONST char * s = NULL;

	if (!raw) {
		switch (val) {
		case __O_SI_SYSNAME:	  s = "__O_SI_SYSNAME";		break;
		case __O_SI_HOSTNAME:	  s = "__O_SI_HOSTNAME";	break;
		case SI_RELEASE:	  s = "SI_RELEASE";		break;
		case SI_VERSION:	  s = "SI_VERSION";		break;
		case __O_SI_MACHINE:	  s = "__O_SI_MACHINE";		break;
		case __O_SI_ARCHITECTURE: s = "__O_SI_ARCHITECTURE";	break;
		case SI_HW_SERIAL:	  s = "SI_HW_SERIAL";		break;
		case __O_SI_HW_PROVIDER:  s = "__O_SI_HW_PROVIDER";	break;
		case SI_SRPC_DOMAIN:	  s = "SI_SRPC_DOMAIN";		break;
		case SI_INITTAB_NAME:	  s = "SI_INITTAB_NAME";	break;
		case SI_ARCHITECTURE:	  s = "SI_ARCHITECTURE";	break;
		case SI_BUSTYPES:	  s = "SI_BUSTYPES";		break;
		case SI_HOSTNAME:	  s = "SI_HOSTNAME";		break;
		case SI_HW_PROVIDER:	  s = "SI_HW_PROVIDER";		break;
		case SI_KERNEL_STAMP:	  s = "SI_KERNEL_STAMP";	break;
		case SI_MACHINE:	  s = "SI_MACHINE";		break;
		case SI_OS_BASE:	  s = "SI_OS_BASE";		break;
		case SI_OS_PROVIDER:	  s = "SI_OS_PROVIDER";		break;
		case SI_SYSNAME:	  s = "SI_SYSNAME";		break;
		case SI_USER_LIMIT:	  s = "SI_USER_LIMIT";		break;
		case __O_SI_SET_HOSTNAME: s = "__O_SI_SET_HOSTNAME";	break;
		case SI_SET_HOSTNAME:	  s = "SI_SET_HOSTNAME";	break;
		case SI_SET_SRPC_DOMAIN:  s = "SI_SET_SRPC_DOMAIN";	break;
		}
	}

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_ptc(val, raw)	/* print pathconf code */
int val;
int raw;
{
	register CONST char * s = raw? NULL : pathconfname(val);

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_fui(val, raw)	/* print fusers() input argument */
int val;
int raw;
{
	register CONST char * s = raw? NULL : fuiname(val);

	if (s == NULL)
		return prt_hhx(val, 0);

	outstring(s);
	return 1;
}

/*ARGSUSED*/
static int
prt_boo(val, raw)	/* print boolean_t */
int val;
int raw;
{
	if(raw)
		return prt_dec(val, 0);

	if(val == 0)
		outstring("B_FALSE");
	else
		outstring("B_TRUE");
	return 1;
}

static int
prt_mdc(val, raw)	/* print command type from modadm */
int val;
int raw;
{
	if(raw || val != MOD_C_MREG)
		return prt_dec(val, 0);

	outstring("MOD_C_MREG");
	return 1;
}

static char *mdttab[] = {
	"MOD_TY_NONE",
	"MOD_TY_CDEV",
	"MOD_TY_BDEV",
	"MOD_TY_STR",
	"MOD_TY_FS",
	"MOD_TY_SDEV",
	"MOD_TY_MISC"
};

#define NMDT	(sizeof(mdttab)/sizeof(char *))

static int
prt_mdt(val, raw)	/* print module type from modadm */
int val;
int raw;
{
	if(raw || val < 0 || val >= NMDT)
		return prt_dec(val, 0);

	outstring(mdttab[val]);
	return 1;
}

/*ARGSUSED*/
static int
prt_ihx(val, raw)	/* print *val in hex */
int val;
int raw;
{
	return prt_hex(fetchval(val), 0);
}

static int
prt_idt(val, raw)	/* print idtype_t */
int val;
int raw;
{
	if (raw)
		return prt_dec(val, 0);

	outstring(idtypename(val));
	return 1;
}

static int
prt_dshc(val, raw)	/* print dshm command */
int val;
int raw;
{
	register CONST char * s = raw? NULL : dshmcmd(val);

	if (s == NULL)
		return prt_dec(val, 0);

	outstring(s);
	return 1;
}

static int
prt_dshf(val, raw)	/* print dshm flags */
int val;
int raw;
{
	register CONST char * s = raw? NULL : dshmflags(val);

	if (s == NULL)
		return prt_oct(val, 0);

	outstring(s);
	return 1;
}

static int dxhalves = 0;
static long long dx = 0;

static int
outdx(void)
{
	GROW(18);

	if (dx < -0xFFFFFF || dx > 0xFFFFFF)
		sys_leng += sprintf(sys_string+sys_leng, "0x%.16LX", dx);
	else
		sys_leng += sprintf(sys_string+sys_leng, "%Ld", dx);

	dxhalves = 0;
	dx = 0;
	return 1;
}

/*ARGSUSED*/
static int
prt_dlx(val, raw)	/* print low-order word of long long DEX */
int val;
int raw;
{
	dx |= (long long)val & ULONG_MAX;
	return (++dxhalves == 2)? outdx() : 0;
}

/*ARGSUSED*/
static int
prt_dhx(val, raw)	/* print high-order word of long long DEX */
int val;
int raw;
{
	dx |= (long long)val << LONG_BIT;
	return (++dxhalves == 2)? outdx() : 0;
}

static int cghalves = 0;
static cgid_t cgid = 0;

static int
outcgid(int raw)
{
	register CONST char * s = NULL;

	if (!raw) {
		switch (cgid) {
		case CG_NONE:		s = "CG_NONE";		break;
		case CG_CURRENT:	s = "CG_CURRENT";	break;
		case CG_DEFAULT:	s = "CG_DEFAULT";	break;
		case CG_QUERY:		s = "CG_QUERY";		break;
		case CG_BAD:		s = "CG_BAD";		break;
		}
	}

	if (s == NULL) {
		GROW(18);
		sys_leng += sprintf(sys_string+sys_leng, "0x%.16LX", cgid);
	} else
		outstring(s);

	cghalves = 0;
	cgid = 0;
	return 1;
}

static int
prt_cgl(val, raw)	/* print low-order word of long long cgid_t */
int val;
int raw;
{
	cgid |= (cgid_t)val & ULONG_MAX;
	return (++cghalves == 2)? outcgid(raw) : 0;
}

static int
prt_cgh(val, raw)	/* print high-order word of long long cgid_t */
int val;
int raw;
{
	cgid |= (cgid_t)val << LONG_BIT;
	return (++cghalves == 2)? outcgid(raw) : 0;
}

static int
prt_cgs(val, raw)	/* print cg_ids selector */
int val;
int raw;
{
	if (!raw && val == CG_ONLINE) {
		outstring("CG_ONLINE");
		return 1;
	}

	return prt_hex(val, 0);
}

static int
prt_cgf(val, raw)	/* print cg_bind flags */
int val;
int raw;
{
	register CONST char * s = NULL;

	if (!raw) {
		switch (val) {
		case CGBIND_NOW:	s = "CGBIND_NOW";	break;
		case CGBIND_FORK:	s = "CGBIND_FORK";	break;
		case CGBIND_EXEC:	s = "CGBIND_EXEC";	break;
		}
	}

	if (s == NULL)
		return prt_hex(val, 0);

	outstring(s);
	return 1;
}

static int
prt_cgp(val, raw)	/* print cg_processors selector */
int val;
int raw;
{
	if (!raw && val == P_ONLINE) {
		outstring("P_ONLINE");
		return 1;
	}

	return prt_hex(val, 0);
}

/*ARGSUSED*/
static int
prt_dc2(val, raw)	/* print rval2 in decimal */
int val;
int raw;
{
	outstring("  [ ");
	(void) prt_dec(val, 0);
	outstring(" ]");
	return 1;
}

/*ARGSUSED*/
static int
prt_hx2(val, raw)	/* print rval2 in hexadecimal */
int val;
int raw;
{
	outstring("  [ ");
	(void) prt_hex(val, 0);
	outstring(" ]");
	return 1;
}

/*ARGSUSED*/
static int
prt_wst(val, raw)	/* print rval2 from wait() */
int val;
int raw;
{
	outstring("  [ ");

	if (val & ~0xffff)
		(void) prt_hex(val, 0);
	else
		(void) prt_hhx(val, 0);

	outstring(" ]");
	return 1;
}

void
outstring(s)
register CONST char * s;
{
	register int len = strlen(s);

	GROW(len);
	(void) strcpy(sys_string+sys_leng, s);
	sys_leng += len;
}

static void
grow(nbyte)	/* reallocate format buffer if necessary */
register int nbyte;
{
	while (sys_leng+nbyte >= sys_ssize) {
		sys_string = realloc(sys_string, sys_ssize *= 2);
		if (sys_string == NULL)
			abend("cannot reallocate format buffer", 0);
	}
}


/* array of pointers to print functions, one for each format */

int (* CONST Print[])() = {
	prt_nov,	/* NOV -- no value */
	prt_dec,	/* DEC -- print value in decimal */
	prt_oct,	/* OCT -- print value in octal */
	prt_hex,	/* HEX -- print value in hexadecimal */
	prt_dex,	/* DEX -- print value in hexadecimal if big enough */
	prt_stg,	/* STG -- print value as string */
	prt_ioc,	/* IOC -- print ioctl code */
	prt_fcn,	/* FCN -- print fcntl code */
	PRT_SYS,	/* S?? -- print sys??? code */
	prt_uts,	/* UTS -- print utssys code */
	prt_opn,	/* OPN -- print open code */
	prt_sig,	/* SIG -- print signal name plus flags */
	prt_act,	/* ACT -- print signal action value */
#ifdef RFS_SUPPORT
	prt_rfs,	/* RFS -- print rfsys code */
	prt_rv1,	/* RV1 -- print RFS verification argument */
	prt_rv2,	/* RV2 -- print RFS version argument */
	prt_rv3,	/* RV3 -- print RFS tuneable argument */
#else
	prt_nov,
	prt_nov,
	prt_nov,
	prt_nov,
#endif /* RFS_SUPPORT */
	prt_msc,	/* MSC -- print msgsys command */
	prt_msf,	/* MSF -- print msgsys flags */
	prt_sec,	/* SEC -- print semsys command */
	prt_sef,	/* SEF -- print semsys flags */
	prt_shc,	/* SHC -- print shmsys command */
	prt_shf,	/* SHF -- print shmsys flags */
	prt_plk,	/* PLK -- print plock code */
	prt_sfs,	/* SFS -- print sysfs code */
	prt_rst,	/* RST -- print string returned by syscall */
	prt_smf,	/* SMF -- print streams message flags */
	prt_ioa,	/* IOA -- print ioctl argument */
	prt_six,	/* SIX -- print signal, masked with SIGNO_MASK */
	prt_mtf,	/* MTF -- print mount flags */
	prt_mft,	/* MFT -- print mount file system type */
	prt_iob,	/* IOB -- print contents of I/O buffer */
	prt_hhx,	/* HHX -- print value in hexadecimal (half size) */
	prt_wop,	/* WOP -- print waitsys() options */
	prt_spm,	/* SPM -- print sigprocmask argument */
	prt_rlk,	/* RLK -- print readlink buffer */
	prt_mpr,	/* MPR -- print mmap()/mprotect() flags */
	prt_mty,	/* MTY -- print mmap() mapping type flags */
	prt_mcf,	/* MCF -- print memcntl() function */
	prt_mc4,	/* MC4 -- print memcntl() (fourth) argument */
	prt_mc5,	/* MC5 -- print memcntl() (fifth) argument */
	prt_mad,	/* MAD -- print madvise() argument */
	prt_ulm,	/* ULM -- print ulimit() argument */
	prt_rlm,	/* RLM -- print get/setrlimit() argument */
	prt_cnf,	/* CNF -- print sysconfig() argument */
	prt_inf,	/* INF -- print systeminfo() argument */
	prt_ptc,	/* PTC -- print pathconf/fpathconf() argument */
	prt_fui,	/* FUI -- print fusers() input argument */
	prt_nov,	/* HID -- hidden argument, nothing printed */
	prt_iov,	/* IOV -- print contents of iovec buffer */
	prt_boo,	/* BOO -- print boolean_t */
	prt_mdc,	/* MDC -- print command type from modadm */
	prt_mdt,	/* MDT -- print module type from modadm */
	prt_ihx,	/* IHX -- print *val as hex */
	prt_idt,	/* IDT -- print idtype_t */
	prt_dshc,	/* DSHC -- print DSHM command */
	prt_dshf,	/* DSHF -- print DSHM flags */
	prt_cgl,	/* CGL -- print low-order word of long long cgid_t */
	prt_cgh,	/* CGH -- print high-order word of long long cgid_t */
	prt_cgs,	/* CGS -- print cg_ids selector */
	prt_cgf,	/* CGF -- print cg_bind flags */
	prt_cgp,	/* CGP -- print cg_processors selector */
	prt_dlx,	/* DLX -- print low-order word of long long DEX */
	prt_dhx,	/* DHX -- print high-order word of long long DEX */
	prt_dc2,	/* DC2 -- print rval2 in decimal */
	prt_hx2,	/* HX2 -- print rval2 in hexadecimal */
	prt_wst,	/* WST -- print rval2 from wait() */
};
