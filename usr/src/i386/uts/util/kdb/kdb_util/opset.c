#ident	"@(#)kern-i386:util/kdb/kdb_util/opset.c	1.5.4.1"
#ident	"$Header$"

#include <util/types.h>
#include <util/param.h>
#include <util/kdb/kdebugger.h>

typedef long		L_INT;
typedef unsigned long	ADDR;

void prdiff(long, void (*)());
extern void prhex(long , void (*)());

char *sl_name;

/*
 *	UNIX debugger
 *
 *		Instruction printing routines.
 *		MACHINE DEPENDENT.
 *		Tweaked for i386.
 */

/* prassym(): symbolic printing of disassembled instruction */
void
prassym(void (*prf)())
{
	int cnt, jj;
	unsigned long value;
	long diff;
	register char *os;
	char *pos;

	extern	char	mneu[];		/* in dis/extn.c */

	/* depends heavily on format output by disassembler */
	cnt = 0;
	os = mneu;	/* instruction disassembled by dis_dot() */
	while(*os != '\t' && *os != ' ' && *os != '\0')
		os++;		/* skip over instr mneumonic */
	while (*os) {
		while(*os == '\t' || *os == ',' || *os == ' ')
			os++;
		value = jj = 0;
		pos = os;
		switch (*os) {
		/*
		** This counts on disassembler not lying about
		** lengths of displacements.
		*/
		case '$':
			pos++;
			/* FALLTHROUGH */

		case '0':
			value = strtoul(pos,&pos,0);
			jj = (value != 0);
			if (*pos != '(')
				break;
			/* FALLTHROUGH */

		case '(':
			while (*pos != ')')
				pos++;
			os = pos;
			break;

		case '+':
		case '-':
			while(*os != '\t' && *os != ' ' && *os != '\0')
				os++;
			if ((os[0] == ' ' || os[0] == '\t') && os[1] == '<') {
				char *cp;

				value = strtoul(os + 2, &cp, 16);
				if (*cp == '>')
					jj = 1;
				os = cp;
			}
			if (value == 0) /* probably a .o, unrelocated displacement*/
				jj = 0;
			break;
		}
		if (jj > 0 && (diff = adrtoext((ADDR) value)) != -1) {
			if (cnt < 0) {
				(*prf)(" [-,");
				cnt = 1;
			} else if (cnt++ == 0)
				(*prf)(" [");
			else
				(*prf)(",");
			(*prf)("%s", sl_name);
			prdiff(diff, prf);
		} else if (cnt > 0)
			(*prf)(",-");
		else
			--cnt;
		while(*os != '\t' && *os != ',' && *os != ' ' && *os != '\0')
			os++;
	} /* for */
	if (cnt > 0)
		(*prf)("]");
}

void
prdiff(long diff, void (*prf)())
{
	if (diff) {
		(*prf)("+");
		prhex(diff, prf);
	}
}

int
adrtoext(vaddr_t val)
{
	vaddr_t	addr;
	int	valid;

	sl_name = findsyminfo(val, &addr, &valid);
	if (!valid)
		sl_name = "";
	if (*sl_name == '\0')
		return -1;	/* not found, or found nameless object */
	return(val - addr);
}
