#ident	"@(#)kern-i386:util/kdb/kdb/dbag.c	1.4.3.1"
#ident	"$Header$"

#include <util/kdb/db_as.h>
#include <util/kdb/kdb/debugger.h>
#include <util/kdb/kdebugger.h>
#include <util/param.h>
#include <util/types.h>

#define RETVAL_FAILED				(0x0000)
#define RETVAL_OK				(0x0001)
#define RETVAL_INVALIDATE_CURRENT_NUMBER	(0x0002)
#define RETVAL_ZERO_CURRENT_NUMBER		(0x0004)
#define RETVAL_ZERO_FIELD			(0x0008)
#define RETVAL_ZERO_REPEAT			(0x0010)

#define RETVAL_NORMAL_OK \
	(RETVAL_OK | RETVAL_INVALIDATE_CURRENT_NUMBER | \
	 RETVAL_ZERO_FIELD | RETVAL_ZERO_REPEAT)

#define RETVAL_MODIFIER_OK (RETVAL_OK | RETVAL_INVALIDATE_CURRENT_NUMBER)

STATIC unsigned int current_number, quit_building_current_number;

/*
 * The following format_registers are used to return values from some
 * format procedures:
 */

#define REG_REPEAT_COUNT		(255)
#define REG_CURRENT_SIZE		(254)
#define REG_FIELD_ARGUMENT		(253)
#define REG_LAST_INSTRUCTION_SIZE	(252)

/* The following are inclusive */
#define REG_STACK_BOTTOM		(249)
#define REG_STACK_TOP			(200)

#define repeat_count	(format_registers[REG_REPEAT_COUNT])
#define current_size	(format_registers[REG_CURRENT_SIZE])
#define field_argument	(format_registers[REG_FIELD_ARGUMENT])

STATIC unsigned long format_registers[256];

/* ARGSUSED */
STATIC int
ag_digit(as_addr_t *addr, int arg)
{
	if (quit_building_current_number) {
		quit_building_current_number = 0;
		current_number = 0;
	}

	current_number *= 10;
	current_number += arg;

	return RETVAL_OK;
}

/* ARGSUSED */
STATIC int
ag_fieldarg(as_addr_t *addr, int arg)
{
	field_argument = current_number;

	return (RETVAL_OK | RETVAL_ZERO_CURRENT_NUMBER);
}

/* ARGSUSED */
STATIC int
ag_repeat(as_addr_t *addr, int arg)
{
	repeat_count = current_number;

	return (RETVAL_OK | RETVAL_ZERO_CURRENT_NUMBER);
}

/*
 * Dot to Register
 */
/* ARGSUSED */
STATIC int
ag_mD2R(as_addr_t *addr, int arg)
{
	format_registers[current_number] = addr->a_addr;

	return RETVAL_NORMAL_OK;
}

/*
 * Register to Dot
 */
/* ARGSUSED */
STATIC int
ag_mR2D(as_addr_t *addr, int arg)
{
	addr->a_addr = format_registers[current_number];

	return RETVAL_NORMAL_OK;
}

/*
 * Number to Register
 */
/* ARGSUSED */
STATIC int
ag_mN2R(as_addr_t *addr, int arg)
{
	format_registers[current_number] = field_argument;

	return RETVAL_NORMAL_OK;
}

/*
 * Register to Current Number
 */
/* ARGSUSED */
STATIC int
ag_mR2N(as_addr_t *addr, int arg)
{
	current_number = format_registers[current_number];

	return (RETVAL_OK | RETVAL_ZERO_REPEAT | RETVAL_ZERO_FIELD |
		RETVAL_INVALIDATE_CURRENT_NUMBER);
}

/*
 * Number to Dot
 */
/* ARGSUSED */
STATIC int
ag_mN2D(as_addr_t *addr, int arg)
{
	addr->a_addr = current_number;

	return RETVAL_NORMAL_OK;
}

/*
 * Indirect pointer of dot to dot.
 */
/* ARGSUSED */
STATIC int
ag_indirect(as_addr_t *addr, int arg)
{
	unsigned long naddr;

	bcopy(&format_registers[REG_STACK_TOP],
	      &format_registers[REG_STACK_TOP+1],
	      (REG_STACK_BOTTOM - REG_STACK_TOP) *
	      sizeof(format_registers[0]));

	format_registers[REG_STACK_TOP] = addr->a_addr;
	if (db_read(*addr, &naddr, sizeof naddr) == -1)
		return RETVAL_FAILED;
	addr->a_addr = naddr;

	return RETVAL_NORMAL_OK;
}

/* ARGSUSED */
STATIC int
ag_pop(as_addr_t *addr, int arg)
{
	addr->a_addr = format_registers[REG_STACK_TOP];
	bcopy(&format_registers[REG_STACK_TOP+1],
	      &format_registers[REG_STACK_TOP],
	      (REG_STACK_BOTTOM - REG_STACK_TOP) *
	      sizeof(format_registers[0]));

	return RETVAL_NORMAL_OK;
}

STATIC int
ag_add(as_addr_t *addr, int arg)
{
	int t = current_number ? current_number : 1;

	addr->a_addr += (arg * t);

	return RETVAL_NORMAL_OK;
}

/* ARGSUSED */
STATIC void
ag_char(unsigned char ch, int arg)
{
	/* Someday maybe handle meta characters */

	if (arg != 2) {
		if (arg) {
			if (ch == 0x7F) {
				ch = '?';
				dbprintf("^");
			} else if (ch < ' ') {
				ch += '@';
				dbprintf("^");
			}
		} else {
			if (ch < ' ' || ch >= 0x7F) {
				ch = '.';
			}
		}
	}

	dbprintf("%c", ch);
}

/* ARGSUSED */
STATIC int
ag_string(as_addr_t *addr, int arg)
{
	char ch;
	unsigned int max_field = field_argument;

	while (!field_argument || max_field--) {
		if (db_read(*addr, &ch, sizeof ch) == -1)
			return RETVAL_FAILED;

		if (!ch)
			break;

		ag_char(ch, current_number);
		addr->a_addr += sizeof ch;
	}

	return RETVAL_NORMAL_OK;
}

/* ARGSUSED */
STATIC int
ag_ch(as_addr_t *addr, int arg)
{
	ag_char(arg, 2);

	return RETVAL_NORMAL_OK;
}

/* ARGSUSED */
STATIC int
ag_character(as_addr_t *addr, int arg)
{
	char ch;

	if (db_read(*addr, &ch, sizeof ch) == -1)
		return RETVAL_FAILED;

	ag_char(ch, current_number);
	addr->a_addr += sizeof ch;

	return RETVAL_NORMAL_OK;
}

/* ARGSUSED */
STATIC int
ag_findsym(as_addr_t *addr, int arg)
{
	char *name;
	u_long our_addr;
	u_long sym_addr;
	
	if (db_read(*addr, &our_addr, sizeof our_addr) == -1)
		return RETVAL_FAILED;
	if ((name = findsymname(our_addr, NULL)) != NULL) {
		dbprintf("%s", name);
		if ((sym_addr = findsymaddr(name)) != our_addr)
			dbprintf("+0x%lx", our_addr - sym_addr);
	} else {
		dbprintf("0x%08lx", our_addr);
	}

	addr->a_addr += sizeof our_addr;

	return RETVAL_NORMAL_OK;
}

/* ARGSUSED */
STATIC int
ag_curaddr(as_addr_t *addr, int arg)
{
	char *name;
	u_long sym_addr;
	
	if (!current_number) {
		if ((name = findsymname(addr->a_addr, NULL)) != NULL) {
			dbprintf("%s", name);
			if ((sym_addr = findsymaddr(name)) != addr->a_addr)
				dbprintf("+0x%x", addr->a_addr - sym_addr);
		}
	} else {
		dbprintf("%08lx", addr->a_addr);
	}

	return RETVAL_NORMAL_OK;
}

/* ARGSUSED */
STATIC int
ag_instruction(as_addr_t *addr, int arg)
{
	unsigned long last_address = addr->a_addr;
	extern char mneu[];

	*addr = dis_dot(*addr,0);

	format_registers[REG_LAST_INSTRUCTION_SIZE] =
		(addr->a_addr - last_address);

	dbprintf("%-.*s", current_number, mneu);

	return RETVAL_NORMAL_OK;
}

/* ARGSUSED */
STATIC int
ag_modifier(as_addr_t *addr, int arg)
{
	current_size = arg;

	return RETVAL_MODIFIER_OK;
}

STATIC int
ag_number(as_addr_t *addr, int arg)
{
	union u_val {
		u_char	c_val;
		u_short	s_val;
		u_int	i_val;
		u_long	l_val;
	} val;
	static char dbprformat[] = "%0*..";
	char *p = &dbprformat[3];
	unsigned int cs;
	
	cs = current_size;
	if (current_size == sizeof(val.c_val) ||
	    current_size == sizeof(val.s_val) ||
	    current_size == sizeof(val.i_val) ||
	    current_size == sizeof(val.l_val))
		cs = current_size;
	else
		cs = sizeof(unsigned int);

	if (db_read(*addr, &val, cs) == -1)
		return RETVAL_FAILED;

	if (cs == sizeof(val.l_val))
	    *p++ = 'l';

	*p++ = (char)arg;

	*p++ = '\0';

	if (cs == sizeof(val.i_val))
		dbprintf(dbprformat, current_number, val.i_val);
	else if (cs == sizeof(val.s_val))
		dbprintf(dbprformat, current_number, val.s_val);
	else if (cs == sizeof(val.c_val))
		dbprintf(dbprformat, current_number, val.c_val);
	else if (cs == sizeof(val.l_val))
		dbprintf(dbprformat, current_number, val.l_val);

	addr->a_addr += cs;

	return RETVAL_NORMAL_OK;
}

/*
 * types:
 *	hex					x
 *	dec					d
 *	oct					o
 *	unsigned dec				u
 *
 * modifiers:
 *	byte					B
 *	half (short)				H
 *	int					I
 *	long					L
 *
 * modifiers:
 *	digits to display in field.		[0-9]*\/
 *	number of bytes in number.		[0-9]
 *	repeat count				[0-9]*;
 *
 *	character norm/ctrlprt			c
 *	dot (symbolic address/hex address)	a
 *	instruction				i
 *	newline/space/tab			n t
 *	symbol					p
 *	dot (increment/decrement)		+-
 *	string norm/ctrlprt			s
 *
 *	move dot to register			m
 *	move register to dot			M
 *	move number to register			r
 *	move register to current number		R
 *	move number to dot			.
 *	indirect to dot				*
 *
 *	grouping				(.*)
 *	quote a character			\\.
 *	quoted string string			".*"
 *
 */

struct fdumptypes {
	unsigned char symbol;		/* character for each function */
	int (*func)();			/* func rets # of bytes to inc addr */
	int arg;			/* arg to func (if useful) */
} fdumptypes[] = {
	{ '0', ag_digit,	0,	}, /* build a number */
	{ '1', ag_digit,	1,	}, /* build a number */
	{ '2', ag_digit,	2,	}, /* build a number */
	{ '3', ag_digit,	3,	}, /* build a number */
	{ '4', ag_digit,	4,	}, /* build a number */
	{ '5', ag_digit,	5,	}, /* build a number */
	{ '6', ag_digit,	6,	}, /* build a number */
	{ '7', ag_digit,	7,	}, /* build a number */
	{ '8', ag_digit,	8,	}, /* build a number */
	{ '9', ag_digit,	9,	}, /* build a number */

	{ ';', ag_repeat,	0,	}, /* number to repeat count */
	{ '/', ag_fieldarg,	0,	}, /* user arg  */

	{ 'm', ag_mD2R,		0,	}, /* dot to register */
	{ 'M', ag_mR2D,		0,	}, /* register to dot */
	{ 'r', ag_mN2R,		0,	}, /* number to register */
	{ 'R', ag_mR2N,		0,	}, /* register to number */
	{ '.', ag_mN2D,		0,	}, /* number to dot */

	{ '*', ag_indirect,	0,	}, /* indirect to dot */
	{ '^', ag_pop,		0,	}, /* pop last indirect */

	{ '-', ag_add,		-1,	}, /* decrement dot */
	{ '+', ag_add,		1,	}, /* increment dot */

	{ ' ', ag_ch,		' ',	}, /* one space */
	{ 'n', ag_ch,		'\n',	}, /* print newline */
	{ 't', ag_ch,		'\t',	}, /* tab */

	{ 'a', ag_curaddr,	0,	}, /* symbolic dot */
	{ 'c', ag_character,	0,	}, /* one character */
	{ 's', ag_string,	0,	}, /* disp chars up to NUL */

	{ 'B', ag_modifier,	sizeof(unsigned char),	},
	{ 'H', ag_modifier,	sizeof(unsigned short),	},
	{ 'I', ag_modifier,	sizeof(unsigned int),	},
	{ 'L', ag_modifier,	sizeof(unsigned long),	},

	{ 'd', ag_number,	'd',	}, /* decimal */
	{ 'o', ag_number,	'o',	}, /* octal */
	{ 'u', ag_number,	'u',	}, /* unsigned decimal */
	{ 'x', ag_number,	'x',	}, /* hexidecimal */

	{ 'i', ag_instruction,	0,	}, /* instruction */

	{ 'p', ag_findsym, 	0,	}, /* display symbol at dot */

	{ '\0', NULL, 0, },
};

void
db_fdump(as_addr_t *addr, ulong_t count, char *fstring)
{
	int i, retval;
	unsigned int r, quote_character;
	char *string;
	static int grouping;

	quit_building_current_number = 0;
	current_number = 0;
	field_argument = 0;
	repeat_count = 0;

	while (count--) {

		string = fstring;

		while (*string) {
			if (kdb_output_aborted)
				return;

			quote_character = 0;

			if (*string == '\\') {
				++string;
				if (*string == '\0') {
					quit_building_current_number = 0;
					current_number = 0;
					field_argument = 0;
					repeat_count = 0;
					++quote_character;
					break;
				}

				i = (sizeof(fdumptypes) /
				     sizeof(fdumptypes[0])) - 1;
			} else if (*string == '"') {
				unsigned char sbuff[256];
				int sindex = 0;

				++string;
				while (*string != '"') {
					if (*string) {
						sbuff[sindex++] = *string++;
						if (sindex >=
						    sizeof(sbuff)) {
							dbprintf("?bad "
								 "string?\n");
							return;
						}
					} else {
						dbprintf("?bad string?\n");
						return;
					}
					if (kdb_output_aborted)
						return;
				}

				sbuff[sindex++] = '\0';

				if (repeat_count < 2)
					r = 1;
				else
					r = repeat_count;
				while (r--) {
					dbprintf("%s", sbuff);
					if (kdb_output_aborted)
						return;
				}

				repeat_count = 0;
				quit_building_current_number = 0;
				current_number = 0;
				field_argument = 0;

				++string;
				continue;
			} else if (*string == '(') {
				unsigned char gbuff[256];
				int gindex = 0;

				if (grouping) {
					dbprintf("?can't group while "
						 "grouping?\n");
					return;
				}

				++string;
				while (*string != ')') {
					if (*string) {
						gbuff[gindex++] = *string++;
						if (gindex >=
						    sizeof(gbuff)-1) {
							dbprintf("?bad "
								 "grouping?\n");
							return;
						}
					} else {
						dbprintf("?bad grouping?\n");
						return;
					}
				}

				gbuff[gindex++] = '\\';
				gbuff[gindex++] = '\0';
				grouping++;
				db_fdump(addr, repeat_count ?
						repeat_count : 1UL,
					 (char *)gbuff);
				grouping = 0;
				quit_building_current_number = 0;
				current_number = 0;
				field_argument = 0;
				repeat_count = 0;
				++string;
				continue;
			} else {
				for (i=0; fdumptypes[i].func; ++i) {
					if (fdumptypes[i].symbol == *string)
						break;
				}
			}

			if (repeat_count < 2)
				r = 1;
			else
				r = repeat_count;

			while (r--) {
				if (fdumptypes[i].func) {
					retval = fdumptypes[i].func(addr,
							fdumptypes[i].arg);
					if (!(retval & RETVAL_OK)) {
						dbprintf("????????");
						return;
					}
				} else {
					dbprintf("%c", *string);
					retval = RETVAL_NORMAL_OK;
				}
				if (kdb_output_aborted)
					break;
			}

			if (retval & RETVAL_INVALIDATE_CURRENT_NUMBER)
				quit_building_current_number = 1;

			if (retval & RETVAL_ZERO_CURRENT_NUMBER) {
				quit_building_current_number = 0;
				current_number = 0;
			}

			if (retval & RETVAL_ZERO_FIELD)
				field_argument = 0;

			if (retval & RETVAL_ZERO_REPEAT)
				repeat_count = 0;

			++string;
		}

		if (!quote_character)
			dbprintf("\n");
	}
}
