#ident	"@(#)kern-i386:util/kdb/kdb/dbmisc.c	1.6.3.1"
#ident	"$Header$"

#include <util/types.h>
#include <util/kdb/kdebugger.h>
#include <util/kdb/kdb/debugger.h>
#include <util/kdb/db_as.h>
#include <util/plocal.h>

extern int	db_cur_size;

extern void sprintf();

static void stuffrow(char *, const uchar_t *);

void
db_dump(as_addr_t addr, ulong_t count)
{
	paddr64_t	from;		/* first address to dump from */
	paddr64_t	to;		/* last address to dump from */
	paddr64_t	start, end;	/* range of dump (from, to rounded to 16-bytes) */
	union u_val{
		u_char	c_val;
		u_short	s_val;
		u_long	l_val;
		ullong_t	ll_val;
	}val;
	char	row[17];
	int	i;

	from = addr.a_addr;
	to = from + count - 1;
	to -= (to % db_cur_size);
	from -= (from % db_cur_size);

	start = (from & ~15);
	end = (to | 15) - (db_cur_size - 1);

	row[16] = '\0';

	for (i = 0, addr.a_addr = start;;) {
		if (addr.a_addr < from || addr.a_addr > to) {
			dbprintf("%.*s", 2 * db_cur_size, "................");
			stuffrow(row + i, (uchar_t *)"....");
		} else if (db_read(addr, &val, db_cur_size) == -1) {
			dbprintf("%.*s", 2 * db_cur_size, "????????????????");
			stuffrow(row + i, (uchar_t *)"????");
		} else {
			switch (db_cur_size) {
			case 1:
				dbprintf("%2x", (uchar_t)val.c_val);
				break;
			case 2:
				dbprintf("%4x", (ushort_t)val.s_val);
				break;
			case 4:
				dbprintf("%8x", (ulong_t)val.l_val);
				break;
			case 8:
				dbprintf("%16Lx", (ullong_t)val.ll_val);
				break;
			}
			stuffrow(row + i, (uchar_t *)&val);
		}
		if ((i += db_cur_size) < 16) {
			dbprintf(" ");
		} else {
			if(PAE_ENABLED())
				dbprintf("   %09Lx  %s\n", (ullong_t)addr.a_addr & ~15, row);
			else
				dbprintf("    %08x  %s\n", (ulong_t)addr.a_addr & ~15, row);
			i = 0;
		}
		if ((addr.a_addr += db_cur_size) > end)
			break;
		if (kdb_output_aborted)
			break;
	}
}

/*
 *	char	*row_p;		- pointer into array for row of ASCII chars
 *	uchar_t	*data;		- char(s) to stuff in row
 */
static void
stuffrow(char *row_p, const uchar_t *data)
{
	int	n = db_cur_size;

	while (n-- > 0) {
		if (*data < ' ' || *data > 127) {
			*row_p++ = '.';
			++data;
		} else
			*row_p++ = (char)*data++;
	}
}


#define MAXERRS	10		/* maximum # of errors allowed before	*/
				/* abandoning this disassembly as a	*/
				/* hopeless case			*/

/*
 *	db_dis(addr, count)
 *
 *	disassemble `count' instructions starting at `addr'
 */

void
db_dis(as_addr_t addr, uint_t count)
{
	extern char mneu[], *sl_name;
	extern int errlev;
	long l;

	errlev = 0;

	while (count--) {
		sprintf(mneu, "0x%x:", addr.a_addr);
		if (addr.a_as == AS_KVIRT) {
			if ((l = adrtoext(addr.a_addr)) == 0)
				sprintf(mneu, "%s:", sl_name);
			else if (l != -1)
				sprintf(mneu, "%s+%x:", sl_name, l);
		}
		dbprintf("%-15s ", mneu);
		addr = dis_dot(addr, (db_cur_size == 2));
		if (errlev >= MAXERRS) {
			dbprintf("dis: too many errors\n");
			dbprintf("\tdisassembly terminated\n");
			break;
		}
		dbprintf("%-36s", mneu);
		prassym((void (*)())dbprintf);
		dbprintf("\n");
		if (kdb_output_aborted)
			break;
	}
}
