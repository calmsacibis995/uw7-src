#ident	"@(#)kern-i386:util/kdb/kdb/dbcon.c	1.13.2.1"
#ident	"$Header$"

/*
 * kernel debugger console interface routines
 */

#include <io/conssw.h>
#include <util/cmn_err.h>	/* for VA_LIST */
#include <util/kdb/kdebugger.h>
#include <util/param.h>
#include <util/types.h>

#define EOT     0x04    /* ascii eot character */

#define XOFF    0x13
#define XON     0x11

#define INTR	0x03	/* ^C interrupt character */
#define INTR2	0x7F	/* DEL interrupt character */

extern void db_flush_input(void);
extern void db_brk_msg(int);
extern int strlen(const char *);
extern int db_msg_pending;
static int pending_c = -1;
extern void _watchdog_hook(void);

uint_t db_nlines = 24;
uint_t db_lines_left = 24;

void dbprintf(const char *fmt, ...);
static void dbprintn(llong_t, int, int, int, int, int);

#define GETC_SPIN(c) { \
		while (((c) = DBG_GETCHAR()) == -1) \
			_watchdog_hook(); \
	}

/*
 * dbgets reads a line from the debugging console using polled i/o
 */

char *
dbgets(char *buf, int count)
{
    int c;
    int i;
    extern void _watchdog_hook(void);

    db_lines_left = db_nlines;	/* reset pagination */
    kdb_output_aborted = B_FALSE;

    count--;
    for (i = 0; i < count; ) {
	if ((c = pending_c) != -1)
	    pending_c = -1;
	else
	    GETC_SPIN(c);
	if (c == INTR || c == INTR2)
	    continue;
	DBG_PUTCHAR(c);
	if (c == '\r') {
	    DBG_PUTCHAR(c = '\n');
	}
	else if (c == '\b') {                /* backspace */
	    DBG_PUTCHAR(' ');
	    DBG_PUTCHAR('\b');
	    if (i > 0)
		    i--;
	    continue;
	}
	else if (c == EOT && i == 0)         /* ctrl-D */
	    return NULL;
	buf[i++] = (char)c;
	if (c == '\n')
	    break;
    }
    buf[i] = '\0';
    return (buf);
}


void
dbputc(int chr)
{
	int c2;

	if (kdb_output_aborted)
		return;

	/* Just in case... */
	if (db_nlines < 2)
		db_nlines = 24;
	if (db_lines_left < 2)
		db_lines_left = 2;

	if (chr == '\n') {
		if (--db_lines_left == 1) {
			db_lines_left = db_nlines + 1;
			dbprintf("\n[MORE]---");
			if (kdb_output_aborted)
				return;
		        GETC_SPIN(c2);
			dbprintf("\r         ");
			if (kdb_output_aborted)
				return;
			if (c2 == INTR || c2 == INTR2)
				pending_c = c2;
			chr = '\r';
		}
		if (pending_c == -1) {
			if ((pending_c = DBG_GETCHAR()) == XOFF) {
				while ((pending_c = DBG_GETCHAR()) != XON) {
					if (pending_c == INTR ||
					    pending_c == INTR2)
						break;
				}
				if (pending_c == XON)
					pending_c = -1;
			}
		}
		if (pending_c == INTR || pending_c == INTR2) {
			pending_c = -1;
			dbg_putc_count = 0;
			kdb_output_aborted = B_TRUE;
			db_flush_input();
		}
		DBG_PUTCHAR('\r');
	}
	DBG_PUTCHAR(chr);
}


void
db_xprintf(const char *fmt, VA_LIST ap)
{
	int width, prec, base, ljust, zeropad;
	int c;
	char size;
	char *s;
	llong_t arg;

loop:
	while ((c = *fmt++) != '%') {
		if (c == '\0')
			return;
		dbputc(c);
		if (kdb_output_aborted)
			return;
	}
	if (*fmt == '%') {
		dbputc('%');
		fmt++;
		goto loop;
	}
	prec = width = ljust = zeropad = 0;
	while (*fmt == '-' || *fmt == '+' || *fmt == '#' || *fmt == '0') {
		if (*fmt == '-')
			ljust = 1;
		else if (*fmt == '0')
			zeropad = 1;
		/* for compatibility, we allow (but ignore) other flags */
		++fmt;
	}
	if (*fmt == '*') {
		width = VA_ARG(ap, int);
		++fmt;
	} else {
		while ('0' <= *fmt && *fmt <= '9')
			width = width * 10 + (*fmt++ - '0');
	}
	if (*fmt == '.') {
		if (*++fmt == '*') {
			prec = VA_ARG(ap, int);
			++fmt;
		} else {
			while ('0' <= *fmt && *fmt <= '9')
				prec = prec * 10 + (*fmt++ - '0');
		}
	}
	if ((c = size = *fmt++) == 'l' || c == 'L')
		c = *fmt++;

	/*
	 * Historically, db_xprintf ignored the case of the format character.
	 * For the sake of compatibility with code that may have used %D and
	 * so on, we continue to recognize some upper-case equivalents.
	 * These are marked "compat" below.
	 */
	switch (c) {
	case 'D':						/* compat */
		c = 'd';
		/*FALLTHRU*/
	case 'd':
	case 'u':
	case 'U':						/* compat */
		base = 10;
		break;
	case 'o':
	case 'O':						/* compat */
		base = 8;
		break;
	case 'p':
		dbputc('0');
		dbputc('x');
		/*FALLTHRU*/
	case 'x':
	case 'X':
		base = 16;
		break;
	case 'B':
		base = 2;
		break;
	case 's':
	case 'S':						/* compat */
		s = VA_ARG(ap, char *);
		if (prec == 0 || prec > strlen(s))
			prec = strlen(s);
		width -= prec;
		if (!ljust) {
			while (width-- > 0) {
				dbputc(' ');
				if (kdb_output_aborted)
					return;
			}
		}
		while (prec-- > 0) {
			dbputc(*s++);
			if (kdb_output_aborted)
				return;
		}
		if (ljust) {
			while (width-- > 0) {
				dbputc(' ');
				if (kdb_output_aborted)
					return;
			}
		}
		goto loop;
	case 'c':
	case 'C':						/* compat */
		if (!ljust) {
			while (width-- > 1) {
				dbputc(' ');
				if (kdb_output_aborted)
					return;
			}
		}
		dbputc(VA_ARG(ap, int));
		if (ljust) {
			while (width-- > 1) {
				dbputc(' ');
				if (kdb_output_aborted)
					return;
			}
		}
		goto loop;
	default:
		/* unknown format -- print it */
		dbputc(c);
		goto loop;
	}

	/*
	 * Here we want to print a numeric argument.  Fetch the argument,
	 * taking into account its size and the desire for sign extension.
	 */
	if (size == 'L')
		arg = VA_ARG(ap, llong_t);
	else if (c == 'd')
		arg = VA_ARG(ap, long);
	else
		arg = VA_ARG(ap, ulong_t);

	dbprintn(arg, base, (c == 'd'), width, ljust, zeropad);
	goto loop;
}

void
dbprintf(const char *fmt, ...)
{
	VA_LIST	ap;

	if (db_msg_pending)
		db_brk_msg(1);

	VA_START(ap, fmt);

	db_xprintf(fmt, ap);
}

static void
dbprintn(llong_t n, int b, int sflag, int width, int ljust, int zeropad)
{
	ullong_t nn = n;
	boolean_t minus = B_FALSE;
	int i, c;
	char d[NBBY * sizeof(n)];	/* worst case: ~0 in binary */

	if (sflag && n < 0) {
		minus = B_TRUE;
		nn = -n;
	}
	for (i = 0;;) {
		d[i++] = (char)(nn % b);
		nn = nn / b;
		if (nn == 0)
			break;
	}

	width -= i + minus;

	if (!ljust) {
		if (zeropad) {
			if (minus) {
				dbputc('-');
				minus = B_FALSE;
			}
			c = '0';
		} else
			c = ' ';
		while (width-- > 0) {
			dbputc(c);
			if (kdb_output_aborted)
				return;
		}
	}

	if (minus)
		dbputc('-');

	while (i-- > 0)
		dbputc("0123456789ABCDEF"[d[i]]);

	if (ljust) {
		while (width-- > 0) {
			dbputc(' ');
			if (kdb_output_aborted)
				return;
		}
	}
}

#ifdef DEBUG

/*
 * void
 * dbprint_test(void)
 *	Test printing of various argument sizes and formats.
 *
 * Calling/Exit State:
 *	None.
 *
 * Discussion:
 *	Intended to be called only from a kernel debugger.
 */
void
dbprint_test(void)
{
	uchar_t uc = 'A';
	uchar_t ub = 0x18u;
	uchar_t sb = 0x81u;
	ulong_t ul = 0x12345678u;
	ulong_t sl = 0xEDCBA987u;
	ulong_t hl = 0x80000000u;
	ullong_t ull = 0x0807060504030201u;
	ullong_t sll = 0x8070605040302010u;
	ullong_t hll = 0x8000000000000000u;

	debug_printf("uc  = [%c]\n", uc);
	debug_printf("sb  = [%x]\n", sb);
	debug_printf("ul  = [%x]\n", ul);
	debug_printf("sl  = [%08X]\n", sl);
	debug_printf("sl  = [%o]\n", sl);
	debug_printf("hl  = [%d]\n", hl);
	debug_printf("hl  = [%u]\n", hl);
	debug_printf("ul  = [%u]\n", ul);
	debug_printf("sl  = [%u]\n", sl);
	debug_printf("sl  = [%B]\n", sl);
	debug_printf("ull = [%Lx]\n", ull);
	debug_printf("sll = [%Lx]\n", sll);
	debug_printf("sll = [%Lo]\n", sll);
	debug_printf("ull = [%Lu]\n", ull);
	debug_printf("sll = [%Lu]\n", sll);
	debug_printf("hll = [%Ld]\n", hll);
	debug_printf("hll = [%Lu]\n", hll);
	debug_printf("sb  = [%d] = [%-+0#*.*d]\n", sb, 5, 4, sb);
	debug_printf("ull = [%Ld] = [%*.*Ld]\n", ull, 20, 1, ull);
	debug_printf("sl  = [%p]\n", sl);
	debug_printf("str = [%3.3s]\n", "string");
}

#endif /* DEBUG */
