#ident	"@(#)lprof:bblk/common/dwarf.c	1.4"
/*
* dwarf.c - handle DWARF I and II line number information
*/
#include <dwarf2.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bblk.h"

enum e_d1 { /* DWARF I states */
	D1_SIZE,
	D1_START,
	D1_LINENO,
	D1_CHAR,
	D1_ADDR,
	D1__TOTAL
};

enum e_d2 { /* DWARF II states */
	D2_SIZE,
	D2_VERSION,
	D2_PRO_SIZE,
	D2_MIN_INST,
	D2_DEF_IS_STMT,
	D2_LINE_BASE,
	D2_LINE_RANGE,
	D2_OPCODE_BASE,
	D2_OP_COUNTS,
	D2_INC_DIRS_STR,
	D2_FILES_STR,
	D2_FILES_INDEX,
	D2_FILES_TIME,
	D2_FILES_SIZE,
	D2_OPCODE,
	D2_LENGTH,
	D2_SUBOPCODE,
	D2_ADV_LINE,
	D2_SEQUENCE,
	D2_SET_FILE,
	D2_2ADDR,
	D2_4ADDR,
	D2__TOTAL
};

static const char exnumbmsg[] = ":1649:expecting a number instead of %s\n";
static const char exzeromsg[] = ":1650:expecting a zero instead of %s\n";
static const char exbytemsg[] = ":1651:expecting a byte instead of %s\n";
static const char exstrmsg[] =
	":1652:expecting a string constant instead of %s\n";

static unsigned long
uvalue(void) /* return unsigned integer value at curptr */
{
	unsigned long ret;
	unsigned char *p;

	ret = strtoul((char *)curptr, (char **)&p, 0);
	if (p == curptr)
		error(exnumbmsg, (char *)curptr);
	return ret;
}

static long
svalue(void) /* return signed integer value at curptr */
{
	unsigned char *p;
	long ret;

	ret = strtol((char *)curptr, (char **)&p, 0);
	if (p == curptr)
		error(exnumbmsg, (char *)curptr);
	return ret;
}

static void
zero(void) /* verify that integer at curptr is a zero */
{
	if (uvalue() != 0)
		error(exzeromsg, (char *)curptr);
}

static unsigned char *
endquote(void) /* return pointer to end of string constant at curptr */
{
	unsigned char *p = curptr;

	if (*p != '"')
		error(exstrmsg, (char *)p);
	while (*++p != '"') {
		if (*p == '\\')
			p++;
	}
	return p;
}

static int
nextoperand(void) /* advance curptr to next operand, if any */
{
	unsigned char *p = curptr;

	if (*p == '"')
		p = endquote();
	while (*p != ',') {
		if (*p == '\0' || chtab[*p++] & CH_CMT)
			return 0;
	}
	while (chtab[*++p] & CH_WSP)
		;
	curptr = p;
	return 1;
}

static int
sequence(unsigned long *res, int issign) /* construct "LEB128" value */
{
	unsigned char *p = curptr, *e;
	static int shift;
	unsigned byte;

	for (;;) {
		byte = strtoul((char *)p, (char **)&e, 0);
		if (p == e || byte > UCHAR_MAX)
			error(exbytemsg, (char *)p);
		if (res != 0) {
			if (shift == 0)
				*res = 0;
			*res |= (byte & 0x7f) << shift;
		}
		if ((byte & 0x80) == 0) {
			if (issign && byte & 0x40) /* sign extend it */
				*res |= -(1 << (7 + shift));
			byte = 0;
			shift = 0; /* reset for next sequence */
			break;
		}
		shift += 7;
		while (chtab[*e] & CH_WSP)
			e++;
		if (*e != ',')
			break;
		while (chtab[*++e] & CH_WSP)
			;
		p = e;
	}
	curptr = e;
	return byte;
}

static void
d2line(enum e_dot item) /* process .debug_line directive "item" operands */
{
	static int line_base, line_range, opcode_base, didfiles, op_counts;
	static unsigned long bbline = 1, set_file = 1, adv_line;
	static enum e_d2 state = D2_SIZE;
	unsigned char *p;
	int opcode;

	do {
		switch (state) {
		default:
			error(":1653:unknown DWARF II .debug_line state (%d)\n",
				(int)state);
			/*NOTREACHED*/
		case D2_SIZE:
			if (item != DOT_4BYTE)
				goto wrongsize;
			state = D2_VERSION;
			break;
		case D2_VERSION:
			if (item != DOT_2BYTE)
				goto wrongsize;
			state = D2_PRO_SIZE;
			if (uvalue() != 2)
				error(":1654:unexpected DWARF II version\n");
			break;
		case D2_PRO_SIZE:
			if (item != DOT_4BYTE)
				goto wrongsize;
			state = D2_MIN_INST;
			break;
		case D2_MIN_INST:
			if (item != DOT_BYTE)
				goto wrongsize;
			state = D2_DEF_IS_STMT;
			break;
		case D2_DEF_IS_STMT:
			if (item != DOT_BYTE)
				goto wrongsize;
			state = D2_LINE_BASE;
			break;
		case D2_LINE_BASE:
			if (item != DOT_BYTE)
				goto wrongsize;
			state = D2_LINE_RANGE;
			line_base = svalue();
			break;
		case D2_LINE_RANGE:
			if (item != DOT_BYTE)
				goto wrongsize;
			state = D2_OPCODE_BASE;
			line_range = svalue();
			break;
		case D2_OPCODE_BASE:
			if (item != DOT_BYTE)
				goto wrongsize;
			state = D2_OP_COUNTS;
			opcode_base = svalue();
			op_counts = opcode_base - 1;
			break;
		case D2_OP_COUNTS:
			if (item != DOT_BYTE)
				goto wrongsize;
			if (--op_counts == 0)
				state = D2_INC_DIRS_STR;
			break;
		case D2_INC_DIRS_STR:
			if (item == DOT_BYTE) {
				zero();
				state = D2_FILES_STR;
				break;
			}
			if (item != DOT_STRING)
				goto wrongsize;
			p = endquote();
			if (p - curptr == 1)
				state = D2_FILES_STR;
			curptr = p + 1;
			break;
		case D2_FILES_STR:
			if (item == DOT_BYTE) {
				zero();
				state = D2_OPCODE;
				break;
			}
			if (item != DOT_STRING)
				goto wrongsize;
			p = endquote();
			state = (p - curptr == 1) ? D2_OPCODE : D2_FILES_INDEX;
			curptr = p + 1;
			break;
		case D2_FILES_INDEX:
			if (item != DOT_BYTE)
				goto wrongsize;
			if (sequence(0, 0) == 0)
				state = D2_FILES_TIME;
			break;
		case D2_FILES_TIME:
			if (item != DOT_BYTE)
				goto wrongsize;
			if (sequence(0, 0) == 0)
				state = D2_FILES_SIZE;
			break;
		case D2_FILES_SIZE:
			if (item != DOT_BYTE)
				goto wrongsize;
			if (sequence(0, 0) == 0)
				state = didfiles ? D2_OPCODE : D2_FILES_STR;
			break;
		case D2_OPCODE:
			if (item != DOT_BYTE)
				goto wrongsize;
			switch (opcode = uvalue()) {
			case 0: /* start of extended opcode */
				state = D2_LENGTH;
				break;
			case DW_LNS_copy:
			case DW_LNS_negate_stmt:
			case DW_LNS_set_basic_block:
			case DW_LNS_const_add_pc:
				break;
			case DW_LNS_advance_line:
				state = D2_ADV_LINE;
				break;
			case DW_LNS_advance_pc:
			case DW_LNS_set_column:
				state = D2_SEQUENCE;
				break;
			case DW_LNS_set_file:
				state = D2_SET_FILE;
				break;
			case DW_LNS_fixed_advance_pc:
				state = D2_2ADDR;
				break;
			default: /* special opcode */
				opcode -= opcode_base;
				bbline += line_base + opcode % line_range;
				addline(bbline, set_file);
				break;
			}
			break;
		case D2_LENGTH:
			if (item != DOT_BYTE)
				goto wrongsize;
			if (sequence(0, 0) == 0)
				state = D2_SUBOPCODE;
			break;
		case D2_SUBOPCODE:
			if (item != DOT_BYTE)
				goto wrongsize;
			switch (uvalue()) {
			default:
				error(":1655:unknown DWARF II subopcode\n");
				/*NOTREACHED*/
			case DW_LNE_end_sequence:
				state = D2_OPCODE;
				break;
			case DW_LNE_set_address:
				state = D2_4ADDR;
				break;
			case DW_LNE_define_file:
				state = D2_FILES_STR;
				didfiles = 1;
				break;
			}
			break;
		case D2_ADV_LINE:
			if (item != DOT_BYTE)
				goto wrongsize;
			if (sequence(&adv_line, 1) == 0) {
				state = D2_OPCODE;
				bbline += adv_line;
				addline(bbline, set_file);
			}
			break;
		case D2_SEQUENCE:
			if (item != DOT_BYTE)
				goto wrongsize;
			if (sequence(0, 0) == 0)
				state = D2_OPCODE;
			break;
		case D2_SET_FILE:
			if (item != DOT_BYTE)
				goto wrongsize;
			if (sequence(&set_file, 0) == 0)
				state = D2_OPCODE;
			break;
		case D2_2ADDR:
			if (item != DOT_2BYTE)
				goto wrongsize;
			state = D2_OPCODE;
			break;
		case D2_4ADDR:
			if (item != DOT_4BYTE)
				goto wrongsize;
			state = D2_OPCODE;
			break;
		}
	} while (nextoperand());
	return;
wrongsize:;
	error(":1656:initialization directive in .debug_line has wrong size\n");
}

void
nbyte(enum e_dot item) /* process data initialization directive "item" */
{
	static enum e_d1 state = D1_SIZE;

	if (curscn == SCN_DEBUG_LINE) {
		d2line(item);
		return;
	}
	if (curscn != SCN_LINE)
		return;
	/*
	* Pretty much ignore everything except D1_LINENO:
	*	D1_SIZE->D1_START->D1_LINENO->D1_CHAR->D1_ADDR->D1_LINENO
	*/
	do {
		switch (state) {
		default:
			error(":1657:unknown DWARF I .line state (%d)\n",
				(int)state);
			/*NOTREACHED*/
		case D1_SIZE:
			if (item != DOT_4BYTE)
				goto wrongsize;
			state = D1_START;
			break;
		case D1_START:
			if (item != DOT_4BYTE)
				goto wrongsize;
			state = D1_LINENO;
			break;
		case D1_LINENO:
			if (item != DOT_4BYTE)
				goto wrongsize;
			state = D1_CHAR;
			addline(uvalue(), 1);
			break;
		case D1_CHAR:
			if (item != DOT_2BYTE)
				goto wrongsize;
			state = D1_ADDR;
			break;
		case D1_ADDR:
			if (item != DOT_4BYTE)
				goto wrongsize;
			state = D1_LINENO;
			break;
		}
	} while (nextoperand());
	return;
wrongsize:;
	error(":1658:initialization directive in .line has wrong size\n");
}
