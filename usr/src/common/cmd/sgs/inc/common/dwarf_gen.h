#ifndef DWARF_GEN_H
#define DWARF_GEN_H
#ident	"@(#)sgs-inc:common/dwarf_gen.h	1.4"

/* Routines used by acomp, c++fe, and c++be to generate debugging information
** (either Dwarf1 or Dwarf2) in assembler.
** The routines are
**	dw2_out_loc_desc - generate a Dwarf2 location description
** and
**	dwarf_out:
**	Generate formatted output based on character string.
**	dwarf_out is printf-like, except only recognized characters
**	are supported, including those output literally.  Escapes are:
**	L[dt]		output debug/type label (label follows)
**	LD		output end label corresponding to Ld
**	Lc		output label of current debug info entry
**	LC		output end label corresponding to Lc
**	L[lh]		output low/high pc label (label follows)
**	L[BE]		output begin/end text label
**	L[be]		output begin/end line numbers labels
**	L[pP]		output begin/end labels for .debug_line prologue
**	Ln		output label for line number -- label name uses name
**                      generated in last call to .n
**	LH		output function high pc label (c++be)
**	Li		output text label for inlined function instance
**	LI		output debug label for concrete inlined instance tree
**			label has form ..Dx.y to avoid conflicts with labels
**			generated by the front end
**	LS		output label for block scope entry
**	Ls		output label for function entry sibling,- number follows (c++fe/c++be)
**	LL		output text label (number follows)
**	Lf		output function start label -- label name uses name
**	 			generated in last call to .f
**	B		output byte pseudo-op
**	C		output assembly comment delimiter and string
**	H		output unaligned halfword pseudo-op
**	S[dlpnat]	set section to debug/line/previous/pub_names/aranges/text
**	W		output unaligned word pseudo-op
**	b		output string as a bunch of hex-valued bytes + NUL
**	d		output decimal value
**	s		output string
**	x		output hex value
**	l[su]		output LEB128 signed/unsigned value. - Dwarf 2
**	l[SU]		output LEB128 signed/unsigned value for long long. - Dwarf 2
**	.d		output assembler expression that assigns selected
**			label the value of ".".
**	.[CD]		output assembler expression that assigns end label for
**			current debug info entry or selected one the value ".".
**	.[hl]		output assembler expression that assigns hi/lo pc label
**			for current debug info entry the value of ".".
**	.[BE]		output assembler expression that assigns begin/end text
**			label the value of ".".
**	.[be]		same for begin/end line numbers
**			label of the .debug_abbrev section the value of ".".
**							- Dwarf 2
**	.n		output assembler expression to assign line number label
**			the next label number.
**	.N		just allocate the next label number for subsequent use.
**							- Dwarf 2
**	.f		output assembler expression to assign function start
**			label the next label number.
**	=		output assembler expression that makes type (acomp) or
**			temporary (c++fe) label in next arg equivalent to
**			current debug info entry's label
**	'		on recursive call, take next character literally
**	literal chars:  \t SP \n : ; -
**	On a recursive call, any character is permitted, and is taken
**	to be literal.
*/

/* Before inclusion of this file, these macros must be defined:
** DB_CURINFOLAB	- int	- label of current entry
** DB_CURSIBLING	- int	- label of current sibling entry
** IS_DWARF_2()		- int or boolean
** COMMENTS_IN_DEBUG	- int or boolean
** ERROR_FUNCTION	- void (*)(const char *, ...)
** ADDR_SIZE		- int	- size of address on target machine
** DBOUT		- FILE * - output file
*/

#include <stdio.h>
#include <stdarg.h>
#include <dwarf2.h>

/* Little Endian Base 128 (LEB128) related constants   - Dwarf 2
*/
#ifndef LEB128_bits
#define LEB128_bits	7
#endif
#define LEB128_mask	((1 << LEB128_bits) - 1)
#define LEB128_high_bit	(1 << (LEB128_bits - 1))
#define LEB128_cont_bit (1 << LEB128_bits)

/* Define strings that introduce data generation pseudo-ops. */

#ifndef	DB_BYTE
#define	DB_BYTE	".byte"
#endif
#ifndef	DB_2BYTE
#define	DB_2BYTE ".2byte"
#endif
#ifndef	DB_4BYTE
#define	DB_4BYTE ".4byte"
#endif
#ifndef	DB_DOTSET
#define	DB_DOTSET ".set"
#endif
#ifndef	DB_DOTALIGN
#define	DB_DOTALIGN ".align"
#endif
#ifndef	DB_DOTSTRING
#define	DB_DOTSTRING ".string"
#endif
#ifndef SECTION
#define SECTION		".section"	/* choose file section */
#endif
#ifndef COMMENTSTR
#define COMMENTSTR	"#"
#endif

/* Section names.
** The first three are simply the section names.
** The last is the full directive that resets to the
** previous section.
*/
#ifndef	DB_DOTDEBUG
#define	DB_DOTDEBUG ".debug"		/* Most debug info. - Dwarf 1 */
#endif
#ifndef	DB_DOTLINE
#define	DB_DOTLINE ".line"		/* Line number information - Dwarf 1 */
#endif
#ifndef	DB_DOTDBG_INFO
#define	DB_DOTDBG_INFO ".debug_info"	/* Most debug info. - Dwarf 2 */
#endif
#ifndef	DB_DOTDBG_LINE
#define	DB_DOTDBG_LINE ".debug_line"	/* Line number information - Dwarf 2 */
#endif
#ifndef	DB_DOTDBG_ABBR
#define	DB_DOTDBG_ABBR ".debug_abbrev"	/* Debug abbreviation info. - Dwarf 2 */
#endif
#ifndef DB_DOTDBG_NAME
#define DB_DOTDBG_NAME ".debug_pubnames" /* Debug lookup by name - Dwarf 2 */
#endif
#ifndef DB_DOTDBG_ADDR
#define DB_DOTDBG_ADDR ".debug_aranges" /* Debug address ranges - Dwarf 2 */
#endif
#ifndef	DB_DOTPREVIOUS
#define	DB_DOTPREVIOUS "\t.previous\n"	/* Back to previous section. */
#endif
#ifndef DB_DOTTEXT
#define DB_DOTTEXT	"\t.text\n"	/* set to .text section. */
#endif

/* Label names. */
#ifndef LAB_DEBUG
#define LAB_DEBUG	"..D%d"		/* general debug label */
#endif
#ifndef LAB_SIBLING
#define LAB_SIBLING	"..S%d"		/* general sibling label */
#endif
#ifndef LAB_FUNCNO
#define LAB_FUNCNO	"..FS%d"	/* function start label */
#endif
#ifndef LAB_TYPE
#define LAB_TYPE	"..T%d"		/* general type label */
#endif
#ifndef LAB_LAST
#define LAB_LAST	"..D%d.e"	/* debug end of block */
#endif
#ifndef LAB_PCLOW
#define	LAB_PCLOW	"..D%d.pclo"	/* debug-associated low pc */
#endif
#ifndef LAB_PCHIGH
#define	LAB_PCHIGH	"..D%d.pchi"	/* debug-associated high pc */
#endif
#ifndef LAB_BLOCK
#define	LAB_BLOCK	"..B%d"		/* block entry label */
#endif
#ifndef	LAB_TEXTBEGIN
#define LAB_TEXTBEGIN	"..text.b"	/* beginning of text label */
#endif
#ifndef LAB_TEXTEND
#define LAB_TEXTEND	"..text.e"	/* end of text label */
#endif
#ifndef	LAB_LINEBEGIN
#define	LAB_LINEBEGIN	"..line.b"	/* start of line numbers */
#endif
#ifndef	LAB_LINEEND
#define	LAB_LINEEND	"..line.e"	/* end of line numbers */
#endif
#ifndef	LAB_ABBRBEGIN
#define	LAB_ABBRBEGIN	"..abbr.b"	/* start of Dwarf 2 abbrev. table */
#endif
#ifndef	LAB_INFOBEGIN
#define	LAB_INFOBEGIN	"..dbg_info.b"	/* start of Dwarf 2 .debug_info sectionn */
#endif
#ifndef	LAB_INFOEND
#define	LAB_INFOEND	"..dbg_info.e"	/* end of Dwarf 2 .debug_info section */
#endif
#ifndef LAB_NAMEBEGIN
#define LAB_NAMEBEGIN	"..dbg_name.b"	/* start of Dwarf 2 .debug_pubname section. */
#endif
#ifndef LAB_NAMEEND
#define LAB_NAMEEND	"..dbg_name.e"	/* end of Dwarf 2 .debug_pubname section. */
#endif
#ifndef LAB_LINENO
#define LAB_LINENO	"..LN%d"	/* line number label */
#endif
#ifndef LAB_LP_START
#define LAB_LP_START	"..LN.b"	/* start of statement program */
#endif
#ifndef LAB_LP_END
#define LAB_LP_END	"..LN.e"	/* end of statement program */
#endif
#ifndef LAB_FHIGH
#define	LAB_FHIGH	"..FE%d.pchi"	/* high pc for function */
#endif
#ifndef LAB_CONCRETE_INSTANCE
#define LAB_CONCRETE_INSTANCE	"..D%d.%d"
#endif
#ifndef LAB_INLINE
#define LAB_INLINE	"..IN%d"	/* inline label */
#endif

static void dwarf_out(const char * fmt, ...)
{
    va_list ap;
    static int recursive = 0;
    static int next_lab_no = 0;

    va_start(ap, fmt);

    ++recursive;			/* mark a recursive call */

    /* Walk the format string, decide what to do. */
    for (;;) {
	char * s;
	int i;
	switch( *fmt++ ){
	case 0:
	    /* End of string. */
	    va_end(ap);
	    --recursive;		/* back out one level */
	    return;
	case 'b':
	    s = va_arg(ap, char *);

#ifdef	DB_USEBYTE
	{
	    /* Output string a a set of bytes, with preceding length. */
	    int charno;

	    if (COMMENTS_IN_DEBUG)
		dwarf_out("C", s);

	    for (charno = 0; ; ++charno) {
		if ((charno % 8) == 0) {
		    if (charno != 0)
			putc('\n', DBOUT);
		    fprintf(DBOUT, "\t%s\t", DB_BYTE);
		}
		else
		    putc(',', DBOUT);
		fprintf(DBOUT, "%#x", s[charno]);
		if (s[charno] == 0)
		    break;		/* quit after seeing NUL */
	    }
	    putc('\n', DBOUT);
	}
#else
	    /* Output string as .string. */
	    fprintf(DBOUT, "\t%s\t\"%s\"", DB_DOTSTRING, s);
#endif
	    break;
	case 'd':
	    i = va_arg(ap, int);
	    fprintf(DBOUT, "%d", i);
	    break;
	case 'C':
	    /* Output comment string if -1s is set.  Output newline always. */
	    s = va_arg(ap, char *);
	    if (COMMENTS_IN_DEBUG)
		fprintf(DBOUT, "\t%s %s", COMMENTSTR, s);
	    putc('\n', DBOUT);
	    break;
	case 's':
	    s = va_arg(ap, char *);
	    fputs(s, DBOUT);
	    break;
	case 'x':
	    i = va_arg(ap, int);
	    fprintf(DBOUT, "%#x", i);
	    break;
	case 'l':
	    /* Little Endian Base 128 (LEB128) format - signed or unsigned. */
	    {
		int is_signed;
		unsigned int byte;
#if MAX_INTEGER_VALUE > LONG_MAX
		long long ll;
#endif

		switch( *fmt++ ){
		case 's':
		    is_signed = 1;
		    goto leb128;
		case 'u':
		    is_signed = 0;
leb128: ;
		    i = va_arg(ap, int);
		    fprintf(DBOUT, "%s\t", DB_BYTE);
		    byte = i & LEB128_mask;
		    i = (is_signed ? i >> LEB128_bits
				   : ((unsigned int) i) >> LEB128_bits);
		    while ( is_signed ? ( ! (   (i == 0 || i == -1)
                                          && ((i & LEB128_high_bit) == (byte & LEB128_high_bit))))
			           : i != 0) {
			byte |= LEB128_cont_bit;
			fprintf(DBOUT, "%#x,", byte);
			byte = i & LEB128_mask;
			i >>= LEB128_bits;
		    }  /* while */
		    fprintf(DBOUT, "%#x", byte);
		    break;
#if MAX_INTEGER_VALUE > LONG_MAX
		case 'S':
		    is_signed = 1;
		    goto ll_leb128;
		case 'U':
		    is_signed = 0;
ll_leb128: ;
		    ll = va_arg(ap, long long);
		    fprintf(DBOUT, "%s\t", DB_BYTE);
		    byte = ll & LEB128_mask;
		    ll = (is_signed ? ll >> LEB128_bits
				   : ((unsigned long long) ll) >> LEB128_bits);
		    while ( is_signed ? ( ! (   (ll == 0 || ll == -1)
                                          && ((ll & LEB128_high_bit) == (byte & LEB128_high_bit))))
			           : i != 0) {
			byte |= LEB128_cont_bit;
			fprintf(DBOUT, "%#x,", byte);
			byte = ll & LEB128_mask;
			ll >>= LEB128_bits;
		    }  /* while */
		    fprintf(DBOUT, "%#x", byte);
		    break;
#endif
		default:
		fmt -= 2; goto badfmt;
		}  /* switch */
	    }
	    break;

	case '\'':
	    /* quote character */
	    ++fmt;			/* skip to (past) next char, print it */
	    /*FALLTHRU*/
	/* literal chars */
	case ' ':
	case '\t':
	case '\n':
	case ':':
	case ';':
	case '-':
	    putc(fmt[-1], DBOUT);
	    break;
	case 'B':
	    fputs(DB_BYTE, DBOUT);
	    break;
	case 'H':
	    fputs(DB_2BYTE, DBOUT);
	    break;
	case 'W':
	    fputs(DB_4BYTE, DBOUT);
	    break;
	case 'L':
	    /* labels */
	    switch( *fmt++ ){
	    case 'c':			/* current entry */
		i = DB_CURINFOLAB;
		s = LAB_DEBUG;
		break;
	    case 'C':			/* end label, current entry */
		i = DB_CURINFOLAB;
		s = LAB_LAST;
		break;
	    case 'd':			/* debug label */
		i = va_arg(ap, int);
		s = LAB_DEBUG;
		break;
	    case 'D':			/* end label, debug label */
		i = va_arg(ap,int);
		s = LAB_LAST;
		break;
	    case 'p':			/* start of .debug_line prologue */
		s = LAB_LP_START;
		break;
	    case 'P':			/* end of .debug_line prologue */
		s = LAB_LP_END;
		break;
	    case 'l':			/* low pc */
		i = va_arg(ap,int);
		s = LAB_PCLOW;
		break;
	    case 'h':			/* high pc */
		i = va_arg(ap,int);
		s = LAB_PCHIGH;
		break;
	    case 'H':			/* function high pc */
		i = va_arg(ap,int);
		s = LAB_FHIGH;
		break;
	    case 'n':			/* line number label */
		i = next_lab_no;
		s = LAB_LINENO;
		break;
	    case 'f':			/* function start label */
		i = next_lab_no;
		s = LAB_FUNCNO;
		break;
	    case 't':			/* type label */
		i = va_arg(ap, int);
		s = LAB_TYPE;
		break;
	    case 'B':			/* beginning of text label */
		s = LAB_TEXTBEGIN;
		break;
	    case 'E':
		s = LAB_TEXTEND;	/* end of text label */
		break;
	    case 'b':			/* beginning of line numbers */
		s = LAB_LINEBEGIN;
		break;
	    case 'e':
		s = LAB_LINEEND;	/* end of line numbers */
		break;
	    case 'L':			/* .text label */
		i = va_arg(ap, int);
		s = LABFMT;		/* use .text label format */
		break;
	    case 'i':
		i = va_arg(ap, int);
		s = LAB_INLINE;
		break;
	    case 'I':
		i = va_arg(ap, int);
		s = LAB_CONCRETE_INSTANCE;
		fprintf(DBOUT, s, i, va_arg(ap, int));
		goto skip_label;
	    case 'S':			/* block entry */
		i = va_arg(ap, int);
		s = LAB_BLOCK;
		break;
	    case 's':			/* sibling */
		i = va_arg(ap, int);
		s = LAB_SIBLING;
		break;
	    default:
		fmt -= 2; goto badfmt;
	    }
	    fprintf(DBOUT, s, i);
skip_label:
	    break;
	case '.':
	    /* Output label-setting directive. */
	    switch( *fmt++ ){
	    case 'd':			/* selected debug label */
		i = va_arg(ap, int);
		dwarf_out("Ld:\n", i);
		break;
	    case 'D':
		i = va_arg(ap, int);	/* end label for selected label */
		dwarf_out("LD:\n", i);
		break;
	    case 'C':
		i = DB_CURINFOLAB;	/* end label for current entry */
		dwarf_out("LD:\n", i);
		break;
	    case 'B':			/* begin of text */
		dwarf_out("LB:\n");
		if (IS_DWARF_2()) {
		    /* For Dwarf 2, the statement address changes are
		       made in terms of an address delta from the last;
		       place a .LNn label along with the beginning of
		       text label. */
		    dwarf_out(".n");
		}  /* if */
		break;
	    case 'l':			/* lo pc */
		dwarf_out("Ll:\n", DB_CURINFOLAB);
		break;
	    case 'h':			/* hi pc */
		dwarf_out("Lh:\n", DB_CURINFOLAB);
		break;
	    case 'E':			/* end of text */
		dwarf_out("LE:\n");
		break;
	    case 'b':			/* begin of line numbers */
		dwarf_out("Lb:\n");
		break;
	    case 'e':			/* end of line numbers */
		dwarf_out("Le:\n");
		break;
	    case 'n':			/* line number label */
		i = ++next_lab_no;
		dwarf_out("Ln:\n", i);
		goto save_label_values;
	    case 'N':			/* just bump line label number */
		i = ++next_lab_no;
save_label_values:
		if (IS_DWARF_2()) {
		    /* Dwarf 2 - Advance the label numbers. */
		    dw2_prev_line_label = dw2_curr_line_label;
		    dw2_curr_line_label = i;
		}  /* if */
		break;
	    case 'f':			/* function start label */
		i = ++next_lab_no;
		dwarf_out("Lf:\n", i);
		break;
	    default:
		fmt -= 2; goto badfmt;
	    }
	    break;
	case '=':
	    i = va_arg(ap, int);
	    dwarf_out("\ts Lt,Ld\n", DB_DOTSET, i, DB_CURINFOLAB);
	    break;
	case 'S':
	    /* Set location counter to section. */
	    switch( *fmt++ ){
	      const char * section_name;
	    case 'd':
		section_name = db_dbg_sec_name;
		goto output_section_chg;
	    case 'l':
		section_name = db_line_sec_name;
		goto output_section_chg;
	    case 'n':
		section_name = DB_DOTDBG_NAME;
		goto output_section_chg;
	    case 'a':
		section_name = DB_DOTDBG_ADDR;
output_section_chg:
		fprintf(DBOUT, "\t%s\t%s\n", SECTION, section_name);
		break;
	    case 'p':
		fputs(DB_DOTPREVIOUS, DBOUT);
		break;
	    case 't':
		fputs(DB_DOTTEXT, DBOUT);
		break;
	    default:
		fmt -= 2; goto badfmt;
	    }
	    break;
	default:
	    fmt -= 1;
	    if (recursive) {		/* treat as literal if recursive call */
		putc(*fmt++, DBOUT);
		break;
	    }
badfmt: ;
	    fprintf(stderr, "dwarf_out:  ??fmt:  %s", fmt);
	}
    }
    /*NOTREACHED*/
} /* dwarf_out */

static int dw2_size_LEB128(int signed_flag, int value)
/*
Return the number of bytes required to express "value" as an LEB128
representation.  "signed_flag" of zero denotes an unsigned LEB128 is
requested, "signed_flag" not equal zero denotes a signed LEB128 number.
*/
{
    unsigned int byte;
    int size = 0;

    byte = value & LEB128_mask;
    value = (signed_flag != 0 ? value >> LEB128_bits
			      : ((unsigned int) value) >> LEB128_bits);
    while ( signed_flag != 0 ? ( ! (   (value == 0 || value == -1)
                                    && ((value & LEB128_high_bit) == (byte & LEB128_high_bit))))
			     : value != 0) {
	size++;
	byte = value & LEB128_mask;
	value >>= LEB128_bits;
    }  /* while */
    return ++size;
}  /* dw2_size_LEB128 */

/*    Dwarf 2
   Routine to generate a location decription based on a variable argument
   list.  Up to 3 passes will be made at the argument list; one to calculate
   the size of the "block" to be generated, one to output the block data,
   and if debugging is on for debug info generation, a third pass to 
   emit a comment to describe the contents of the block.

   "at_comment" is the comment string for the attribute entry being generated.
   "block_type" is the block format to be generated (implies size and format
   of the length field).  The remaining arguments are the opcodes and data
   in the order to be emitted.  The variable portion of the argument list
   is denoted by an opcode of zero, which does not match any existing Dwarf
   2 opcodes.
*/
static void dw2_out_loc_desc(const char *at_comment, int block_type, int first_op, ...)
{
    int op_code, work1;
    int count = 0;
    va_list ap;
    char *block_op_comment;
    char *block_format;
    int dot_byte_done = 0;

    /* First pass to calculate the block data length. */
    va_start(ap, first_op);
    op_code = first_op;

    while (op_code > 0) {
	count++;			/* count 1 byte for the opcode */
	switch (op_code) {

	case DW_OP_addr:		/* opcode followed by address const */
	    count += ADDR_SIZE;
	    goto bypass_1_data;

	case DW_OP_const1u:		/* opcode followed by 1 byte constant */
	case DW_OP_const1s:
	case DW_OP_pick:
	case DW_OP_deref_size:
	case DW_OP_xderef_size:
	    count++;
	    goto bypass_1_data;

	case DW_OP_const2u:		/* opcode followed by 2 byte constant */
	case DW_OP_const2s:
	case DW_OP_skip:
	case DW_OP_bra:
	    count += 2;
	    goto bypass_1_data;

	case DW_OP_const4u:		/* opcode followed by 4 byte constant */
	case DW_OP_const4s:
	    count += 4;
	    goto bypass_1_data;

	case DW_OP_const8u:		/* opcode followed by 8 byte constant */
	case DW_OP_const8s:
	    count += 8;
	    goto bypass_2_data;

	case DW_OP_bregx:		/* opcode followed by UNSIGNED LEB */
					/* constant and SIGNED LEB constant */
	    count += dw2_size_LEB128( 0 /* unsigned*/, va_arg(ap, int));
	    count += dw2_size_LEB128( 1 /* signed*/, va_arg(ap, int));
	    break;

	case DW_OP_constu:		/* opcodes followed by UNSIGNED */
	case DW_OP_plus_uconst:		/* LEB128 constant */
	case DW_OP_regx:	
	case DW_OP_piece:
	    count += dw2_size_LEB128( 0 /*unsigned*/, va_arg(ap, int));
	    break;

	case DW_OP_SCO_reg_pair:	/* opcode followed by two UNSIGNED LEB */
					/* constants */
	    count += dw2_size_LEB128( 0 /* unsigned */, va_arg(ap, int));
	    count += dw2_size_LEB128( 0 /* unsigned */, va_arg(ap, int));
	    break;

	case DW_OP_consts:		/* opcodes followed by SIGNED */
	case DW_OP_breg0:		/* LEB128 constant */
	case DW_OP_breg1:
	case DW_OP_breg2:
	case DW_OP_breg3:
	case DW_OP_breg4:
	case DW_OP_breg5:
	case DW_OP_breg6:
	case DW_OP_breg7:
	case DW_OP_breg8:
	case DW_OP_breg9:
	case DW_OP_breg10:
	case DW_OP_breg11:
	case DW_OP_breg12:
	case DW_OP_breg13:
	case DW_OP_breg14:
	case DW_OP_breg15:
	case DW_OP_breg16:
	case DW_OP_breg17:
	case DW_OP_breg18:
	case DW_OP_breg19:
	case DW_OP_breg20:
	case DW_OP_breg21:
	case DW_OP_breg22:
	case DW_OP_breg23:
	case DW_OP_breg24:
	case DW_OP_breg25:
	case DW_OP_breg26:
	case DW_OP_breg27:
	case DW_OP_breg28:
	case DW_OP_breg29:
	case DW_OP_breg30:
	case DW_OP_breg31:
	case DW_OP_fbreg:
	    count += dw2_size_LEB128( 1 /* signed*/, va_arg(ap, int));
	    break;

bypass_2_data: ;
	    va_arg(ap,int);
bypass_1_data: ;
	    va_arg(ap,int);
	    break;

	}  /* switch */
	op_code = va_arg(ap, int);
    }  /* while */
    va_end();

    /* Second pass - emit the block data. */
    va_start(ap, first_op);
    op_code = first_op;
    switch (block_type) {
    case DW_FORM_block1:
	block_op_comment = "1";
	block_format = "\tB\tx";
	dot_byte_done = 1;
	break;
    case DW_FORM_block2:
	block_op_comment = "2";
	block_format = "\tH\tx";
	break;
    case DW_FORM_block4:
	block_op_comment = "4";
	block_format = "\tW\tx";
	break;
    case DW_FORM_block:
	block_op_comment = "";
	block_format = "\tlu";
	dot_byte_done = 1;
	break;
    default:
	ERROR_FUNCTION("dwarf2_out_loc_desc: DWARF 2 - only \"block\" forms allowed");
    }  /* switch */
    dwarf_out(block_format, count);

    while (op_code > 0) {
	/* output the opcode */
	dwarf_out( ((dot_byte_done == 1) ? ",x" : "\n\tB\tx"), op_code);
	dot_byte_done = 1;

	switch (op_code) {

	case DW_OP_addr:		/* opcode followed by address const */
	    /* This has not been completely thought through.  FOR NOW, handle
	       symbol names that have been passed in as a string.  May well
	       need to reference various label types in the debugging
	       sections. 
	    */
	    dwarf_out(";\tW\ts", va_arg(ap, char *));
	    dot_byte_done = 0;
	    break;

	case DW_OP_const1u:		/* opcode followed by 1 byte constant */
	case DW_OP_const1s:
	case DW_OP_pick:
	case DW_OP_deref_size:
	case DW_OP_xderef_size:
	    dwarf_out( ((dot_byte_done == 1) ? ",d" : ";\tB\td"), va_arg(ap, int));
	    dot_byte_done = 1;
	    break;

	case DW_OP_const2u:		/* opcode followed by 2 byte constant */
	case DW_OP_const2s:
	case DW_OP_skip:
	case DW_OP_bra:
	    dwarf_out("\n\tH\td", va_arg(ap, int));
	    dot_byte_done = 0;
	    break;

	case DW_OP_const4u:		/* opcode followed by 4 byte constant */
	case DW_OP_const4s:
	    dwarf_out(";\tW\td", va_arg(ap, int));
	    dot_byte_done = 0;
	    break;

	case DW_OP_const8u:		/* opcode followed by 8 byte constant */
	case DW_OP_const8s:
	    work1 = va_arg(ap, int);	/* avoid order of param eval problems */
	    dwarf_out(";\tW\td,d", work1, va_arg(ap, int));
	    dot_byte_done = 0;
	    break;

	case DW_OP_bregx:		/* opcode followed by UNSIGNED LEB */
					/* constant and SIGNED LEB constant */
	    work1 = va_arg(ap, int);	/* avoid order of param eval problems */
	    dwarf_out("\n\tlu;\tls", work1, va_arg(ap, int));
	    dot_byte_done = 1;
	    break;

	case DW_OP_constu:		/* opcodes followed by UNSIGNED */
	case DW_OP_plus_uconst:		/* LEB128 constant */
	case DW_OP_regx:	
	case DW_OP_piece:
	    dwarf_out("\n\tlu", va_arg(ap, int));
	    dot_byte_done = 1;
	    break;

	case DW_OP_SCO_reg_pair:	/* opcode followed by two UNSIGNED LEB's */
	    dwarf_out("\n\tlu;", va_arg(ap, int));
	    dwarf_out("\tlu", va_arg(ap, int));
	    dot_byte_done = 1;
	    break;

	case DW_OP_consts:		/* opcodes followed by SIGNED */
	case DW_OP_breg0:		/* LEB128 constant */
	case DW_OP_breg1:
	case DW_OP_breg2:
	case DW_OP_breg3:
	case DW_OP_breg4:
	case DW_OP_breg5:
	case DW_OP_breg6:
	case DW_OP_breg7:
	case DW_OP_breg8:
	case DW_OP_breg9:
	case DW_OP_breg10:
	case DW_OP_breg11:
	case DW_OP_breg12:
	case DW_OP_breg13:
	case DW_OP_breg14:
	case DW_OP_breg15:
	case DW_OP_breg16:
	case DW_OP_breg17:
	case DW_OP_breg18:
	case DW_OP_breg19:
	case DW_OP_breg20:
	case DW_OP_breg21:
	case DW_OP_breg22:
	case DW_OP_breg23:
	case DW_OP_breg24:
	case DW_OP_breg25:
	case DW_OP_breg26:
	case DW_OP_breg27:
	case DW_OP_breg28:
	case DW_OP_breg29:
	case DW_OP_breg30:
	case DW_OP_breg31:
	case DW_OP_fbreg:
	    dwarf_out(";\tls", va_arg(ap, int));
	    dot_byte_done = 1;
	    break;

	}  /* switch */
	op_code = va_arg(ap, int);
    }  /* while */
    va_end();


    /* Third pass - only need if doing debugging comments.. */
    if (COMMENTS_IN_DEBUG) {
	va_start(ap, first_op);
	op_code = first_op;
	fprintf(DBOUT, "\t%s %s DW_FORM_block%s", COMMENTSTR, at_comment,
		block_op_comment);

	while (op_code > 0) {
	    /* First get the name of the opcode. */
	    count = 0;			/* assume no digit in part of name. */
	    switch (op_code) {

	    case DW_OP_addr:	block_op_comment = "addr"; break;
	    case DW_OP_const1u:	block_op_comment = "const1u"; break;
	    case DW_OP_const1s: block_op_comment = "const1s"; break;
	    case DW_OP_pick:	block_op_comment = "pick"; break;
	    case DW_OP_deref_size:	block_op_comment = "deref_size"; break;
	    case DW_OP_xderef_size:	block_op_comment = "xderef_size"; break;
	    case DW_OP_const2u:	block_op_comment = "const2u"; break;
	    case DW_OP_const2s:	block_op_comment = "const2s"; break;
	    case DW_OP_skip:	block_op_comment = "skip"; break;
	    case DW_OP_bra:	block_op_comment = "bra"; break;
	    case DW_OP_const4u:	block_op_comment = "const4u"; break;
	    case DW_OP_const4s:	block_op_comment = "const4s"; break;
	    case DW_OP_const8u:	block_op_comment = "const8u"; break;
	    case DW_OP_const8s:	block_op_comment = "const8s"; break;
	    case DW_OP_bregx:	block_op_comment = "bregx"; break;
	    case DW_OP_constu:	block_op_comment = "constu"; break;
	    case DW_OP_plus_uconst:	block_op_comment = "plus_uconst"; break;
	    case DW_OP_regx:	block_op_comment = "regx"; break;	
	    case DW_OP_piece:	block_op_comment = "piece"; break;
	    case DW_OP_consts:	block_op_comment = "consts"; break;

	    case DW_OP_breg0:
	    case DW_OP_breg1:
	    case DW_OP_breg2:
	    case DW_OP_breg3:
	    case DW_OP_breg4:
	    case DW_OP_breg5:
	    case DW_OP_breg6:
	    case DW_OP_breg7:
	    case DW_OP_breg8:
	    case DW_OP_breg9:
	    case DW_OP_breg10:
	    case DW_OP_breg11:
	    case DW_OP_breg12:
	    case DW_OP_breg13:
	    case DW_OP_breg14:
	    case DW_OP_breg15:
	    case DW_OP_breg16:
	    case DW_OP_breg17:
	    case DW_OP_breg18:
	    case DW_OP_breg19:
	    case DW_OP_breg20:
	    case DW_OP_breg21:
	    case DW_OP_breg22:
	    case DW_OP_breg23:
	    case DW_OP_breg24:
	    case DW_OP_breg25:
	    case DW_OP_breg26:
	    case DW_OP_breg27:
	    case DW_OP_breg28:
	    case DW_OP_breg29:
	    case DW_OP_breg30:
	    case DW_OP_breg31:
		block_op_comment = "breg";
	        count = 1;		/* signal digit needed */
		work1 = op_code - DW_OP_breg0;
		break;

	    case DW_OP_fbreg:	block_op_comment = "fbreg"; break;
	    case DW_OP_deref:	block_op_comment = "deref"; break;
	    case DW_OP_dup:	block_op_comment = "dup"; break;
	    case DW_OP_drop:	block_op_comment = "drop"; break;
	    case DW_OP_over:	block_op_comment = "over"; break;
	    case DW_OP_swap:	block_op_comment = "swap"; break;
	    case DW_OP_rot:	block_op_comment = "rot"; break;
	    case DW_OP_xderef:	block_op_comment = "xderef"; break;
	    case DW_OP_abs:	block_op_comment = "abs"; break;
	    case DW_OP_and:	block_op_comment = "and"; break;
	    case DW_OP_div:	block_op_comment = "div"; break;
	    case DW_OP_minus:	block_op_comment = "minus"; break;
	    case DW_OP_mod:	block_op_comment = "mod"; break;
	    case DW_OP_mul:	block_op_comment = "mul"; break;
	    case DW_OP_neg:	block_op_comment = "neg"; break;
	    case DW_OP_not:	block_op_comment = "not"; break;
	    case DW_OP_or:	block_op_comment = "or"; break;
	    case DW_OP_plus:	block_op_comment = "plus"; break;
	    case DW_OP_shl:	block_op_comment = "shl"; break;
	    case DW_OP_shr:	block_op_comment = "shr"; break;
	    case DW_OP_shra:	block_op_comment = "shra"; break;
	    case DW_OP_xor:	block_op_comment = "xor"; break;
	    case DW_OP_eq:	block_op_comment = "eq"; break;
	    case DW_OP_ge:	block_op_comment = "ge"; break;
	    case DW_OP_gt:	block_op_comment = "gt"; break;
	    case DW_OP_le:	block_op_comment = "le"; break;
	    case DW_OP_lt:	block_op_comment = "lt"; break;
	    case DW_OP_ne:	block_op_comment = "ne"; break;

	    case DW_OP_lit0:
	    case DW_OP_lit1:
	    case DW_OP_lit2:
	    case DW_OP_lit3:
	    case DW_OP_lit4:
	    case DW_OP_lit5:
	    case DW_OP_lit6:
	    case DW_OP_lit7:
	    case DW_OP_lit8:
	    case DW_OP_lit9:
	    case DW_OP_lit10:
	    case DW_OP_lit11:
	    case DW_OP_lit12:
	    case DW_OP_lit13:
	    case DW_OP_lit14:
	    case DW_OP_lit15:
	    case DW_OP_lit16:
	    case DW_OP_lit17:
	    case DW_OP_lit18:
	    case DW_OP_lit19:
	    case DW_OP_lit20:
	    case DW_OP_lit21:
	    case DW_OP_lit22:
	    case DW_OP_lit23:
	    case DW_OP_lit24:
	    case DW_OP_lit25:
	    case DW_OP_lit26:
	    case DW_OP_lit27:
	    case DW_OP_lit28:
	    case DW_OP_lit29:
	    case DW_OP_lit30:
	    case DW_OP_lit31:
		block_op_comment = "lit";
	        count = 1;		/* signal digit needed */
		work1 = op_code - DW_OP_lit0;
		break;

	    case DW_OP_reg0:
	    case DW_OP_reg1:
	    case DW_OP_reg2:
	    case DW_OP_reg3:
	    case DW_OP_reg4:
	    case DW_OP_reg5:
	    case DW_OP_reg6:
	    case DW_OP_reg7:
	    case DW_OP_reg8:
	    case DW_OP_reg9:
	    case DW_OP_reg10:
	    case DW_OP_reg11:
	    case DW_OP_reg12:
	    case DW_OP_reg13:
	    case DW_OP_reg14:
	    case DW_OP_reg15:
	    case DW_OP_reg16:
	    case DW_OP_reg17:
	    case DW_OP_reg18:
	    case DW_OP_reg19:
	    case DW_OP_reg20:
	    case DW_OP_reg21:
	    case DW_OP_reg22:
	    case DW_OP_reg23:
	    case DW_OP_reg24:
	    case DW_OP_reg25:
	    case DW_OP_reg26:
	    case DW_OP_reg27:
	    case DW_OP_reg28:
	    case DW_OP_reg29:
	    case DW_OP_reg30:
	    case DW_OP_reg31:
		block_op_comment = "reg";
	        count = 1;		/* signal digit needed */
		work1 = op_code - DW_OP_reg0;
		break;

	    case DW_OP_nop:	block_op_comment = "nop"; break;
	    }  /* switch */
	    fprintf(DBOUT, " DW_OP_%s", block_op_comment);
	    if (count) fprintf(DBOUT, "%d", work1);

	    /* Now output data associated with an opcode. */

	    switch (op_code) {

	    case DW_OP_addr:		/* opcode followed by address const */
		fprintf(DBOUT, " %s", va_arg(ap, char *));
		break;

	    case DW_OP_const1u:		/* opcode followed by 1 byte constant */
	    case DW_OP_const1s:
	    case DW_OP_pick:
	    case DW_OP_deref_size:
	    case DW_OP_xderef_size:
	    
	    case DW_OP_const2u:		/* opcode followed by 2 byte constant */
	    case DW_OP_const2s:
	    case DW_OP_skip:
	    case DW_OP_bra:

	    case DW_OP_const4u:		/* opcode followed by 4 byte constant */
	    case DW_OP_const4s:

	    case DW_OP_constu:		/* opcodes followed by UNSIGNED */
	    case DW_OP_plus_uconst:	/* LEB128 constant */
	    case DW_OP_regx:	
	    case DW_OP_piece:

	    case DW_OP_consts:		/* opcodes followed by SIGNED */
	    case DW_OP_breg0:		/* LEB128 constant */
	    case DW_OP_breg1:
	    case DW_OP_breg2:
	    case DW_OP_breg3:
	    case DW_OP_breg4:
	    case DW_OP_breg5:
	    case DW_OP_breg6:
	    case DW_OP_breg7:
	    case DW_OP_breg8:
	    case DW_OP_breg9:
	    case DW_OP_breg10:
	    case DW_OP_breg11:
	    case DW_OP_breg12:
	    case DW_OP_breg13:
	    case DW_OP_breg14:
	    case DW_OP_breg15:
	    case DW_OP_breg16:
	    case DW_OP_breg17:
	    case DW_OP_breg18:
	    case DW_OP_breg19:
	    case DW_OP_breg20:
	    case DW_OP_breg21:
	    case DW_OP_breg22:
	    case DW_OP_breg23:
	    case DW_OP_breg24:
	    case DW_OP_breg25:
	    case DW_OP_breg26:
	    case DW_OP_breg27:
	    case DW_OP_breg28:
	    case DW_OP_breg29:
	    case DW_OP_breg30:
	    case DW_OP_breg31:
	    case DW_OP_fbreg:
		fprintf(DBOUT, " %d", va_arg(ap, int));
		break;

	    case DW_OP_const8u:		/* opcode followed by 8 byte constant */
	    case DW_OP_const8s:

	    case DW_OP_bregx:		/* opcode followed by UNSIGNED LEB */
					/* constant and SIGNED LEB constant */
		work1 = va_arg(ap, int);
		fprintf(DBOUT, " %d %d", work1, va_arg(ap, int));
		break;
	    }  /* switch */
	    op_code = va_arg(ap, int);
	}  /* while */
	va_end();
    }  /* if */

    /* finish the current assembly source line. */
    fprintf(DBOUT, "\n");

}  /* dw2_out_loc_desc */

#endif /* DWARF_GEN_H */