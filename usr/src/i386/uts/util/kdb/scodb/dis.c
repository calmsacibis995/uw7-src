#ident	"@(#)kern-i386:util/kdb/scodb/dis.c	1.1.1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1994.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Modification History:
 *
 *	L000		scol!nadeem	22apr92
 *	- added "^" pattern matching expression to disassembly search strings.
 *	  This allows easy searching of a specific instruction address.
 *	L001		scol!nadeem	23apr92
 *	- from scol!hughd 15oct91 - 3.2.4b DS cannot handle the structure copy
 *	  in opnds() without Segmentation violation: workaround with
 *	  intermediate variable.
 *	- fix bug which was causing a panic when disassembling mov instructions
 *	  via the CRn registers.
 *	L002		scol!nadeem	14may92
 *	- fix disassembly bug where any instruction whose operands are the
 *	  eax register and a memory location were being disassembled with
 *	  parentheses around the memory location, eg: "mov eax,(freemem)".
 *	  The parentheses were removed as they do not match assembler syntax.
 *	- added a new unassembly mode "immed" which prints out an '&'
 *	  before every symbol being accessed in immediate mode.
 *	- make the "u mode" command display an explanation of each mode.
 *	- make "u mode help" synonymous with "u mode list".
 *	L003		scol!nadeem	15may92
 *	- fix bug whereby "movw %ax,address" was being disassembled
 *	  incorrectly (in the spl routines).  This was occuring because
 *	  the disassembler was mistakenly thinking that the 16-bit operand
 *	  prefix of this instruction indicated the size of the address,
 *	  whereas it actually indicated the size of data to be transferred
 *	  from the address.
 *	L004	scol!nadeem	12jun92
 *	- fix bug introduced when scodb was changed to deal with both old and
 *	  new symbol table format.  When in disassembly mode, a search for a
 *	  string would not terminate when reaching the next symbol.
 *	L005	scol!nadeem	31jul92
 *	- added support for static text symbols.  Added a new option
 *	  "u mode static" which displays the disassembly listing using
 *	  static as well as global symbols.  This will be turned on by
 *	  default.
 *	L006	scol!nadeem	23jul93
 *	- display 8-bit values in signed form, so that "incl FC(%ebp)" is
 *	  properly displayed as "incl -4(%ebp)".
 *	L007	scol!nadeem	11aug93
 *	- add support for displaying source code line numbers during
 *	  disassembly.  For each disassembled location, determine if
 *	  there is a line number associated with it, and if so display it
 *	  alongside the instruction.
 *	T008	scol!nadeem from scocan!larryp	9nov93
 *	- correct the outputting of addresses in "symbol+offset" form such that
 *	  in a restricted field the symbol is truncated rather than the
 *	  offset.  So, for example, you would now get "a_long_symbol_na+14c"
 *	  instead of the confusing "a_long_symbol_name+1".
 *	L009	scol!nadeem & scol!hughd	23mar94
 *	- half the "set" instructions were wrongly disassembled, r/m8 missed
 *	- fixed compiler warnings
 *	L010	nadeem@sco.com	7apr94
 *	- fixed bug in disassembly of "mov dr/cr/tr" whereby the general
 *	  register was not displayed correctly.
 *	- added vi-style ^D and ^U commands to the unassembly listing to scroll
 *	  the unassembly down or up by half a screen.
 *	- cleaned up the indentation in c_unasm().
 *	L011	nadeem@sco.com	26sep94	
 *	- support for printing out the effective address of a call instruction
 *	  during single-stepping (eg: call 14(%eax) <htnamei>).
 */

/*
*	To test out the disassembler, compile this file
*	with -DSTDALONE.
*
*	Standalone support is at end of the file.
*/

#include	"sys/reg.h"
#include	"dbg.h"
#include	"dis.h"
#include	"sent.h"
#include	"histedit.h"

#define		PUTCHAR(b, c)		(**(b) = (c), ++*(b))
#define		PUTST(b, s)		(strcpy(*(b), (s)), \
						*(b) += strlen(s))

#define		P_LPAR(b)		PUTCHAR((b), '(' /*)*/)
#define		P_RPAR(b)		PUTCHAR((b), /*(*/ ')')

#define		P_DOLLAR(b)		PUTCHAR((b), '$')	/* L002 */

#define		INW	7		/* width for opcode	     */
#define		DNBPL	5		/* prefered # bytes per line */
#define		MXNBPL	7		/* max # bytes per line      */
#define		LNW	(MXNBPL*3+2)	/* line width		     */

#define		LINECOL	60		/* column at which to display source
					   code line numbers.  L007 */

STATIC struct {
	char	*um_name;
	int	um_val;
	char	*um_help;					/* v L002 v */
} umodes[] = {
{ "default",	UDEFAULT,   "set default mode." },
{ "args",	UM_ARGS,    "display '@' before function arguments." },
{ "binary",	UM_BINARY,  "print instruction bytes."  },
{ "disp",	UM_DISP,    "print #(reg) as (#+reg)." },
{ "intel",	UM_INTEL,   "display operands as <destination, source>." },
{ "reg",	UM_REGPCT,  "display '%' before register names." },
{ "sym",	UM_SYMONLY, "symbolic output." },
{ "immed",	UM_IMMED,   "display '$' before immediate operands." },
{ "static",	UM_STATIC,  "display static text symbols." },	/* L005 */
};								/* ^ L002 ^ */

NOTSTATIC int umode = UDEFAULT;

#define		SZ_32		0
#define		SZ_16		1

#ifdef STDALONE
# include	<stdio.h>
# ifdef _16
#  define	DF_ASZ		SZ_16
#  define	DF_OSZ		SZ_16
# else
#  define	DF_ASZ		SZ_32
#  define	DF_OSZ		SZ_32
# endif
# define	pnzb		d_pnzb
# define	prstb		d_prstb
# define	pn		d_pn
# define	symname		d_symname
# define	getbyte		d_getbyte
# define	getmem		d_getmem
# define	una		d_una
#else
# define	DF_ASZ		SZ_32
# define	DF_OSZ		SZ_32
#endif

STATIC int umode_symflag;	/* flag passed to symname() to indicate whether
				 * it should display local symbols - L005
				 */

extern int *REGP;						/* L011 */
extern char *symname();						/* L011 */

#ifndef STDALONE

/*
*	unassemble command
*/
NOTSTATIC
c_unasm(c, v)
	int c;
	char **v;
{
	long oldoff, offt, off;
	unsigned long mi_off = 0xFFFFFFFF;
	int i, r, ls = 0, searchdir, dodir, exact = 0;
	int mx_row = 0, cur_row, dis_nlines, dis_dir;		/* L010 */
	char cs, cx, *mdn, *pinst();
	char sybuf[DBIBFL];
	struct instr inst;
	struct sent *findsym();
	struct sent *se;
	extern char *scodb_error;
	extern int *REGP;
	extern int scodb_nlines;

	++v;
	if (!strcmp(*v, "exact")) {
		exact = 1;
		++v;
	}

	/*
	*	"please print modes"
	*		or
	*	"please set modes"
	*/
	if (!strcmp(*v, "mode")) {
		if (!*++v) {
			/*
			*	print modes that are set
			*/
			printf("Unassembly mode:\n");
			r = 0;
			for (i = 0;i < NMEL(umodes);i++)
				if (i && (umode & umodes[i].um_val)) {
					++r;
					printf("\t%s\n", umodes[i].um_name);
				}
			if (!r)
				printf("\tnone set\n");
			return DB_CONTINUE;
		}
		while (*v) {
			if ((!strcmp(*v, "help") || !strcmp(*v, "list")) &&
			    !ls++) {				/* L002 */
				/*
				*	print all possible modes
				*/
				printf("Unassembly modes:\n");
				for (i = 0;i < NMEL(umodes);i++)
					printf("\t%s\t- %s\n",
						umodes[i].um_name,
						umodes[i].um_help); /* L002 */
				goto nex;
			}
			if (**v == UNEGATE) {
				r = 1;
				mdn = *v + 1;
			}
			else {
				r = 0;
				mdn = *v;
			}
			for (i = 0;i < NMEL(umodes);i++)
				if (!strcmp(mdn, umodes[i].um_name)) {
					if (r)
						umode &= ~umodes[i].um_val;
					else if (i == 0)
						umode = umodes[i].um_val;
					else
						umode |= umodes[i].um_val;
					goto nex;
				}
			printf("Do not recognize unassembly mode \"%s\"\n", mdn);
			return DB_ERROR;
		nex:	++v;
		}
		return DB_CONTINUE;
	}
	inst.in_seg = REGP[T_CS];
	if (!getaddrv(v, &inst.in_seg, &off)) {
		perr();
		return DB_ERROR;
	}
	if (exact)
		inst.in_off = off;
	else if (ioff(inst.in_seg, off, &inst.in_off) < 0) {
		badaddr(inst.in_seg, inst.in_off);
		return DB_ERROR;
	}
	sybuf[0] = '\0';

	dis_nlines = 1;						/* L010v */
	dis_dir = 0;
	for (;;) {
		while (dis_nlines-- > 0) {
			if (dis_dir == -1) {
				r = ioff(inst.in_seg, oldoff - 1,
					 &inst.in_off);
				if (r != 1) {
					dobell();
					break;
				}
				putchar('\r');
				if (cur_row == 0 || oldoff <= mi_off) {
					insline();
					if (mx_row < (scodb_nlines - 1))
						++mx_row;
				}
				else
					up();
			} else
			if (dis_dir == 1)
				putchar('\n');

			oldoff = inst.in_off;
			cur_row = p_row();
			if (inst.in_off < mi_off)
				mi_off = inst.in_off;
			if (cur_row > mx_row)
				mx_row = cur_row;
			if (!disi(&inst)) {
				badaddr(inst.in_seg, inst.in_off);
				return DB_ERROR;
			}
			pinst(inst.in_seg, oldoff, &inst, 0, 0);
		}

		/* loop to process keyboard commands */

		for (;;) {
			dis_nlines = 1;
			dis_dir = 1;				/* L010^ */
			cx = getrchr();
			if (quit(cx))
				goto done;
			switch (cx) {
			case '/':
			case '?':
				putchar('\n');
				insline();
				cs = sybuf[0];
				getistre("Search string: ", sybuf, 1);
				if (sybuf[0] == '\0')
					sybuf[0] = cs;
				up();
				delline();
				up();
				if (mx_row > (scodb_nlines - 3))
					mx_row = (scodb_nlines - 3);
				searchdir = (cx == '/');
				dodir = searchdir;
			srch:	{
				int soff, ooff, pl, i;
				char *s;

				/*
				 * If the user entered a null search string,
				 * abort the search and read a new command.
				 */

				if (sybuf[0] == '\0') {
					dobell();
					break;
				}
				soff = inst.in_off;
				se = findsym(oldoff);

				if (dodir) {
					/*
					 * disassemble  silently forward looking
					 * for the string
					 */
					se = sent_prev(se);	/* L004 */
					i = 0;			/* L010 */
					for (;;) {
						ooff = inst.in_off;
						if (ooff >= se->se_vaddr)
							goto notfnd;
						if (!disi(&inst))
							goto notfnd;
						s = pinst(inst.in_seg, ooff, &inst, 1, &pl);
						i++;		/* L010 */
						if (findst(s, sybuf, pl))
							break;
					}

					dis_nlines = i;		/* L010v */
					dis_dir = 1;
					inst.in_off = soff;	/* L010^ */
				} else {
					/*
					 * disassemble silently backwards
					 * looking for the string
					 */
					ooff = inst.in_off = oldoff;
					for (i = 0;;i++) {
						if (i >= 50)
							goto notfnd;
						if (ooff < se->se_vaddr)
							goto notfnd;
						r = ioff(inst.in_seg, ooff - 1, &inst.in_off);
						if (r != 1)
							goto notfnd;
						ooff = inst.in_off;
						disi(&inst);
						s = pinst(inst.in_seg, ooff, &inst, 1, &pl);
						if (findst(s, sybuf, pl))
							break;
					}

					dis_nlines = i + 1;	/* L010 */
					dis_dir = -1;		/* L010 */
				}

				/* found at instr starting at inst.in_off */

				goto nxi;
			notfnd:					/* L009 */
				inst.in_off = soff;
				dobell();
				}
				break;

			case 'N':
			case 'n':
				if (sybuf[0] == '\0') {
					/* no prev */
					dobell();
					break;
				}
				dodir = (cx == 'N') ? !searchdir : searchdir;
				goto srch;

			case ' ':
			case '\n':
			case '\r':
			case '\t':	/* all NEXT instr */
			case 'j':
			case '+':
				dis_dir = 1;			/* L010v */
				dis_nlines = 1;
				goto nxi;

			case 'k':
			case '-':
			case 'l':
			case '\b':	/* all LAST instr */
				dis_dir = -1;
				dis_nlines = 1;
				goto nxi;
			case '\004':	 /* ^D */
				dis_dir = 1;
				dis_nlines = scodb_nlines / 2;
				goto nxi;
			case '\025': 	/* ^U */
				dis_dir = -1;
				dis_nlines = scodb_nlines / 2;
				goto nxi;			/* L010^ */
			default:
				dobell();
				break;
			} /* switch */
		 } /* keyboard loop */
	nxi:	;
	} /* unassembly loop */
done:	for (i = p_row();i <= mx_row;i++)
		putchar('\n');
	return DB_CONTINUE;
}

/*
*	find substring "sus" in string "str"
*/
STATIC
findst(str, sus, strl)
	register char *str, *sus;
	int strl;
{
	int l;
	register char *es;

	l = strlen(sus);
	if (*sus == '^') {					/* v L000 v */
		return (!strncmp(str, sus + 1, l - 1));
	} else {						/* ^ L000 ^ */
		es = str + strl - l;
		while (str <= es) {
			if (*str == *sus && !strncmp(str + 1, sus + 1, l - 1))
				return 1;
			++str;
		}
		return 0;
	}
}

/*
*	find the instruction boundary at or before this seg:off
*
*	do this by finding the symbol before seg:off, disassembling
*	up to seg:off.
*
*	returns:	1	found the boundary
*			0	did not find a boundary
*			-1	dis failed
*
*	in 1/0 case, *newoff has a valid value;
*	in -1 error case, *newoff has offending address.
*/
NOTSTATIC
ioff(seg, off, newoff)
	long seg, off, *newoff;
{
	long ofx;
	struct instr inst;
	struct sent *se, *findsym();

	se = findsym(off);
	if (se) {
		inst.in_off = se->se_vaddr;
		inst.in_seg = seg;
		while (inst.in_off <= off) {
			ofx = inst.in_off;
			if (!disi(&inst)) {	/* dis failed? */
				*newoff = inst.in_off;
				return -1;
			}
		}
		*newoff = ofx;
		return 1;
	}
	else {	/* can't tell. */
		*newoff = off;
		return 0;
	}
}

#endif /* STDALONE */

/*************************************************************
*	forward declarations - tables are at end of file
*/
extern struct opmap	opmap[];	/* 1 byte opcode instructions */
extern struct opmap	opmap2b[];	/* 2 byte opcode instructions */
extern struct opmap	opgroup[][8];	/* group instructions */
extern struct operand	implic[][2];	/* implicit operands */
extern struct operand	modrm32[];	/* 32 bit ModR/M operands */
extern struct operand	modrm16[];	/* 16 bit ModR/M operands */
extern short		addrt[];
extern unsigned char	opndsz[];				/* L009 */

extern int		regs2[5][8][3];

extern long		sib32_scale[];	/* for SIB: scale */
extern long		sib32_index[];	/* for SIB: index */
extern long		sib32_base[];	/* for SIB: base */
extern char		*regn_gr[];	/* general registers */
extern char		*regn_xr[];	/* other registers */
/*************************************************************/

/*
*	the only thing that inst should be given with is the seg/off
*/
NOTSTATIC
disi(inst)
	register struct instr *inst;
{
	unsigned char	c;
	unsigned char	modrm;
	int		osz = DF_OSZ;
	int		asz = DF_ASZ;
	int		twob = 0;
	struct opmap	*grp;
	struct opmap	*op;
	register int	r;

	inst->in_flag = 0;
	inst->in_len = 0;
	inst->in_nopnd = 0;

	/*
	*	this is only a loop if a prefix, segment override,
	*	2-byte opcode or opnd/addr size is found
	*/
	for (;;) {
		if (!db_getbyte(inst->in_seg, inst->in_off, &c))
			return 0;
		++inst->in_off;
		inst->in_buf[inst->in_len++] = c;
		if (twob)
			op = &opmap2b[c];
		else
			op = &opmap[c];
		switch (op->op_flags & CODE) {
			case C_PREFIX:
				switch (c) {
					case 0x66:
						osz	= !osz;
						break;
						
					case 0x67:
						asz	= !asz;
						break;

					case 0xF0:
					case 0xF2:
					case 0xF3:
						inst->in_flag |= I_PRFX;
						inst->in_prefix = c;
						break;
				}
				break;

			case C_2BYTE:
				twob	= 1;
				inst->in_opcde = inst->in_len - 1;
				break;

			case C_SEG:
				inst->in_flag |= I_SEGOVR;
				inst->in_segovr = (op->op_flags & SEGMENT) >> SEGSHF;
				break;

			case C_COPROC:
				inst->in_opcde = inst->in_len - 1;
				r = coproc(inst);
				return r;

			case C_NOSUCH:
				inst->in_opcde = inst->in_len - 1;
				r = nosuch(inst);
				return r;

			case C_NORM:
				if (!twob)
					inst->in_opcde = inst->in_len - 1;
				r = show(inst, op, NULL, osz, asz, 0, 0);
				if (op->op_flags & C_CALL)
					inst->in_flag |= I_CALL;
				else if (op->op_flags & C_JT)
					inst->in_flag |= I_JUMP;
				return r;

			case C_GP:
				if (!twob)
					inst->in_opcde = inst->in_len - 1;
				r = (op->op_flags & GROUP) >> GRPSHF;
				if (!db_getbyte(inst->in_seg, inst->in_off, &modrm))
					return 0;
				++inst->in_off;
				inst->in_buf[inst->in_len++] = modrm;
				grp	= op;
				op	= &opgroup[r][MRM_REG(modrm)];
				r = show(inst, op, grp, osz, asz, 1, modrm);
				if (op->op_flags & C_CALL)
					inst->in_flag |= I_CALL;
				return r;
		}
	}
}

STATIC
show(inst, op, grp, osz, asz, havmrm, modrm)
	register struct instr *inst;
	register struct opmap *op, *grp;
	int osz, asz, havmrm, modrm;
{
	int i, j;
	struct opmap on;

	if (grp) {
		on.op_flags = op->op_flags;
		on.op_flags |= (grp->op_flags & (C_GP|GRP_FLAGS|CODE_FLAGS));
		j = 0;
		for (i = 0;i < 3 && grp->op_opnds[i];i++)
			on.op_opnds[j++] = grp->op_opnds[i];
		for (i = 0;i < 3 && op->op_opnds[i];i++)
			on.op_opnds[j++] = op->op_opnds[i];
		on.op_opnds[j] = 0;
		on.op_name = op->op_name;
		op = &on;
	}
	else {
		for (j = 0;j < 3 && j < op->op_opnds[j];j++)
			;
	}
	if (!opnds(inst, havmrm, modrm, op, osz, asz))
		return 0;
	inst->in_opcn = op->op_name;
	return 1;
}

/*
*	coprocessor instructions are not supported
*/
STATIC
coproc(inst)
	struct instr *inst;
{
	inst->in_flag	= 0;
	inst->in_prefix	= 0;
	inst->in_segovr	= 0;
	inst->in_nopnd	= 0;
	inst->in_opcn	= "<coproc esc>";
	return 1;
}

STATIC
nosuch(inst)
	struct instr *inst;
{
	inst->in_flag	= 0;
	inst->in_prefix	= 0;
	inst->in_segovr	= 0;
	inst->in_nopnd	= 0;
	inst->in_opcn	= "<unknown>";
	return 1;
}

STATIC
clearcont(cn)
	register long cn[];
{
	cn[0]=cn[1]=cn[2]=cn[3]=cn[4]=cn[5]=cn[6]=cn[7]=cn[8]=cn[9]=0;
}

STATIC
opnds(inst, havmrm, modrm, op, osz, asz)
	register struct instr *inst;
	int havmrm;
	unsigned char modrm;
	register struct opmap *op;
	int osz, asz;
{
	unsigned char *s;
	int a, i, n, o, low = 0;
	int rg, rx, rt, rn;
	register struct operand *ond;

	ond = &inst->in_opnd[0];
	clearcont(ond->oa_cont);
	for (i = 0;i < 3 && (n = op->op_opnds[i]);i++) {
		ond->oa_flag = 0;
		if (n & IMPLICIT) {
			struct operand *tmp;			/* v L001 v */
			tmp = &implic[(n >> 8) - 1][osz];
			*ond = *tmp;	/* structure copy */	/* ^ L001 ^ */
			if (ond->oa_flag & OF_REG) {
				if (ond->oa_flag & OFR_SPEC) {
					ond->oa_flag &= ~OFR_SPEC;
					if (op->op_flags & C_B)
						ond->oa_cont[0] = GR_AL;
					else	/* ? GR_AX ? */
						ond->oa_cont[0] = GR_EAX;
				}
				switch (ond->oa_cont[0] & REGSIZ) {
					case LO:
					case HI:
						low = I_SFB;
						break;
					case X:
						if (low == 0 || I_SFW < low)
							low = I_SFW;
						break;
					case EX:
						if (low == 0)
							low = I_SFL;
				}
			}
		}
		else {
			a = addrt[(n & ADDRESS) - 1];
			if (a & (IMMED|OFFSET)) {		/* v L002 v */
				if (a & OFFSET) {
					n &= ~OPERAND;
					n |= Ov;
					ond->oa_flag |= OFI_ADDR;
				}				/* ^ L002 ^ */

				ond->oa_flag |= OF_IMMED;
				if ((n & OPERAND) == Ox) {
					if (op->op_flags & C_B)
						n = Ob;
					else
						n = Ov;
				}
				n = (n & OPERAND) >> OP_SHF;
								/* v L003 v */
				o = (a & OFFSET) ? OSIZ(asz, n) & 0x7F
						 : OSIZ(osz, n) & 0x7F;
								/* ^ L003 ^ */

				s = &inst->in_buf[inst->in_len];
				if (!db_getmem(inst->in_seg, inst->in_off, s, o))
					return 0;
				inst->in_off += o;
				inst->in_len += o;
				if (op->op_opnds[1] == OF_NONE)
					low = o << 4;
				if (a & RELIP)
					ond->oa_flag |= OFI_RELIP;
				if (op->op_flags & C_JT)
					ond->oa_flag |= OFI_JMCL;
				pbytes(s, o, ond->oa_cont, &ond->oa_flag, (OSIZ(osz, n) & O_PTRT));
			}
			else if (a & MRM) {	/* has ModR/M byte */
				if (havmrm++ == 0) {
					if (!db_getbyte(inst->in_seg, inst->in_off, &modrm))
						return 0;
					++inst->in_off;
					inst->in_buf[inst->in_len++] = modrm;
				}
 				/*
 				*	for suffixes
 				*/
				if (op->op_flags & C_B)
					low = I_SFB;
				else if (osz) {
					if (low == 0 || I_SFW < low)
						low = I_SFW;
				}
				else {
					if (low == 0)
						low = I_SFL;
				}
				if (a & (REG|MOD)) {
					if (a & REG) {
						rt = ((a & REG) >> RSHF) - 1;
						rn = MRM_REG(modrm);
						n = (n & OPERAND) >> OP_SHF;
						o = OSIZ(osz, n);
						rx = RXSIZ(o);
						ond->oa_cont[0] = ond->oa_cont[rx];
					}
					else {
						rt = ((a & MOD) >> MSHF) - 1;
						rn = MRM_RM(modrm);  /* L010 */
						if (op->op_flags & C_B)	/* byte form - byte size */
							rx = 0;
						else if (osz)	/* not long */
							rx = 1;
						else	/* long */
							rx = 2;
					}
					rg = regs2[rt][rn][rx];
					ond->oa_flag |= OF_REG;
					ond->oa_cont[0] = rg;
				}
				else if (a & SPECIAL) {	/* similar */
					int k;
					struct operand *mo;

					if (asz == 0)
						mo = &modrm32[(MRM_MOD(modrm) << 3) | MRM_RM(modrm)];
					else
						mo = &modrm16[(MRM_MOD(modrm) << 3) | MRM_RM(modrm)];
					*ond = *mo;
					if (op->op_flags & C_I)
						ond->oa_flag |= OF_INDIR;
					if (ond->oa_flag & OF_REG) {
						/*
						*	find register size
						*/
						n = (n & OPERAND) >> OP_SHF;
						o = OSIZ(osz, n) & 0x7F;
						rx = RXSIZ(o);
						ond->oa_cont[0] = ond->oa_cont[rx];
					}
					else {	/* OF_MEM */
						k = 0;
						if (!memproc(inst, ond, mo, &k, MRM_MOD(modrm)))
							return 0;
						ond->oa_flag &= ~OFM_NX;
						ond->oa_flag |= k;
					}
				}
				/* no others */
			}
		}
		++ond;
		++inst->in_nopnd;
	}
	if (op->op_flags & (C_F|C_B))
		inst->in_flag |= low;
	return 1;
}

/*
*	process OF_MEM
*/
STATIC
memproc(inst, on, newon, cnm, mod)
	struct instr *inst;
	register struct operand *on;
	register struct operand *newon;
	register int *cnm;
	int mod;
{
	int no, j, gotsib = 0;
	long cn;
	struct operand nxo;
	unsigned char sib;

	no = newon->oa_flag & OFM_NX;	/* number of cont[] */
	for (j = 0;j < no;j++) {
		cn = newon->oa_cont[j];
		if (cn & C_REG) 	/* register */
			on->oa_cont[(*cnm)++] = cn;
		else if (cn & C_DISP) {	/* displacement */
			on->oa_cont[(*cnm)++] = cn;
			if (!rddisp(inst, &on->oa_cont[(*cnm)++], cn&C_DISPM))
				return 0;
		}
		else {			/* cn & C_SIB */
			if (!db_getbyte(inst->in_seg, inst->in_off, &sib))
				return 0;
			++inst->in_off;
			inst->in_buf[inst->in_len++] = sib;
			gotsib = 1;
		}
	}
	if (gotsib) {
		get_sib32(sib, mod, &nxo);
		if (!memproc(inst, on, &nxo, cnm, 0))
			return 0;
	}
	return 1;
}

/*
*	put n bytes from buf into cont
*		also set the OFI_ amount in *flg
*/
STATIC
pbytes(buf, n, cont, flg, isptr)
	register unsigned char buf[];
	register int n;
	register long cont[];
	int *flg, isptr;
{
	cont[1] = 0;
	*flg &= ~OFI_SIZE;
	if (isptr)
		*flg |= OFIS_PTR;

	cont[0] = buf[0];
	if (n == 1) {
		if (buf[0] & 0x80)	/* high bit set - sign extend. */
			cont[0] |= 0xFFFFFF00;
		*flg |= OFI_SZ8;
		return;
	}

	cont[0] |= (buf[1]<<8);
	if (n == 2) {
		if (buf[1] & 0x80)	/* high bit set - sign extend. */
			cont[0] |= 0xFFFF0000;
		*flg |= OFI_SZ16;
		return;
	}

	cont[0] |= (buf[2] << 16) | (buf[3] << 24);
	if (n == 4) {
		*flg |= OFI_SZ32;
		return;
	}

	cont[1] |= buf[4] | buf[5] << 8;
	if (n == 6) {
		*flg |= OFI_SZ48;
		return;
	}
	cont[1] |= (buf[6] << 16) | (buf[7] << 24);
	*flg |= OFI_SZ64;
	return;
}

/*
*	read a displacement
*/
STATIC
rddisp(inst, cn, n)
	struct instr *inst;
	register long *cn;
	int n;
{
	register unsigned char *s;

	s = &inst->in_buf[inst->in_len];
	if (!db_getmem(inst->in_seg, inst->in_off, s, n))
		return 0;
	inst->in_off += n;
	inst->in_len += n;
	*cn = 0;
	*cn |= s[0];
	if (n == 1)
		return 1;
	*cn |= s[1] << 8;
	if (n == 2)
		return 1;
	*cn |= (s[2] << 16) | (s[3] << 24);
	return 1;
}

STATIC
get_sib32(sib, mod, op)
	int sib, mod;
	struct operand *op;
{
	register int nx;
	long scale, index, base;

	scale	= sib32_scale[SIB_SCALE(sib)];	/* SS		*/
	index	= sib32_index[SIB_INDEX(sib)];	/* INDEX	*/
	base	= sib32_base [SIB_BASE(sib)];	/* BASE		*/

	if (base == 0 && mod != 0)
		base = GR_EBP;
	nx = 0;
	if (mod == 0 && base == /*GR_EBP*/0) {
		op->oa_cont[nx] = C_DISP32;
		++nx;
	}
	if (base) {
		op->oa_cont[nx] = C_REG|base;
		++nx;
	}
	if (index) {
		op->oa_cont[nx] = C_REG|scale|index;
		++nx;
	}
	op->oa_flag = OF_MEM|nx;
}

STATIC char *symp, symp2[64];

/*
*	print out an instruction
*/
NOTSTATIC
char *
pinst(seg, off, inst, dontprint, stl)
	long seg, off;
	register struct instr *inst;
	int dontprint, *stl;
{
	register int i;
	register int l = 0, n = 0;
	int il, ni, np, line;					/* L007 */
	char *symname(), *s;
	static char obuf[DBIBFL*2];
	char *obufp = obuf;

	symp = NULL;
	symp2[0] = 0;

	/*
	*	print out bytes of the instruction
	*
	*	we will print up to MXNBPL bytes on a line, but
	*	if a line would have more than that we will print
	*	only DNBPL of them.
	*/

	umode_symflag = (umode & UM_STATIC) ? 0 : SYM_GLOBAL;	/* L005 */

 	prstb_sym(&obufp, symname(off, umode_symflag), 24);	/* L005 T008 */
 	PUTCHAR(&obufp, ' ');
	if (umode & UM_BINARY) {
		il = inst->in_len;
		ni = 0;
		do {
			l = 0;
			if (ni) {
				PUTCHAR(&obufp, '\n');
				prstb(&obufp, "", 25);
			}
			np = il - ni;
			if (np > MXNBPL)
				np = DNBPL;
			for (i = 0;i < np && (i + ni) < il;i++) {
				PUTCHAR(&obufp, ' ');
				pnzb(&obufp, inst->in_buf[i + ni], 2);
				l += 3;
			}
			ni += i;
		} while (ni != il);
		i = LNW - l - 1;
		while (i-- > 0)
			PUTCHAR(&obufp, ' ');
	}

	/*
	*	put any prefix to the instruction
	*/
	if (inst->in_flag & I_PRFX) {
		s = opmap[inst->in_prefix].op_name;
		PUTST(&obufp, s);
		PUTCHAR(&obufp, ' ');
	}

	/*
	*	the instruction name itself
	*/
	if (!(s = inst->in_opcn)) {
		s = "???";
		l = 3;
	}
	else
		l = strlen(s);
	PUTST(&obufp, s);

	/*
	*	suffix to the instruction name
	*/
	switch (inst->in_flag & I_SFX) {
		case I_SFB:
			PUTCHAR(&obufp, 'b');
			++l;
			break;
		case I_SFW:
			PUTCHAR(&obufp, 'w');
			++l;
			break;
		case I_SFL:
			PUTCHAR(&obufp, 'l');
			++l;
			break;
	}

	for (i = INW;i > l;i--)
		PUTCHAR(&obufp, ' ');

	/*
	*	operands to the instruction
	*/
	if (umode & UM_INTEL) {
		for (i = 0;i < inst->in_nopnd;i++) {
			if (n++)
				PUTCHAR(&obufp, ',');
			pond(&obufp, inst, &inst->in_opnd[i], off);
		}
	}
	else {
		for (i = inst->in_nopnd;i;i--) {
			if (n++)
				PUTCHAR(&obufp, ',');
			pond(&obufp, inst, &inst->in_opnd[i-1], off);
		}
	}

	if ((line = db_search_lineno(off)) != 0) {		/* L007v */
		char *linestr = " ; line XXXXXXXXXXX";
		int col;

		col = obufp - obuf;

		while (col++ < LINECOL)
			PUTCHAR(&obufp, ' ');

		pnd(linestr + 8, line);
		PUTST(&obufp, linestr);
	}							/* L007^ */

	/*
	*	possible symbol/address values
	*/
	if (symp && (umode & UM_SYMONLY) == 0) {
		PUTCHAR(&obufp, ' ');
		PUTCHAR(&obufp, '<');
		PUTST(&obufp, symp);
		PUTCHAR(&obufp, '>');
	}
	if (symp2[0] && (umode & UM_SYMONLY) == 0) {
		PUTCHAR(&obufp, ' ');
		PUTCHAR(&obufp, '<');
		PUTST(&obufp, symp2);
		PUTCHAR(&obufp, '>');
	}

	*obufp = '\0';
	if (!dontprint)
		printf("%s", obuf);
	if (stl)
		*stl = obufp - obuf;
	return obuf;
}

STATIC
pond(opt, inst, oa, off)
	register char **opt;
	struct instr *inst;
	register struct operand *oa;
	long off;
{
	char *s;

	switch (oa->oa_flag & OF_TYP) {
		case OF_IMMED:
			if (oa->oa_flag & OF_INDIR)
				P_LPAR(opt);
			pimm(	opt,
				off,
				inst->in_len,
				oa->oa_flag,
				oa->oa_cont[0],
				oa->oa_cont[1],
				0);
			if (oa->oa_flag & OF_INDIR)
				P_RPAR(opt);
			break;

		case OF_REG:
			preg(opt, off, inst->in_len, oa->oa_cont[0]);
			break;

		case OF_MEM:
			if (inst->in_flag & I_SEGOVR) {
				if (umode & UM_REGPCT)
					PUTCHAR(opt, CREG);
				s = regn_xr[inst->in_segovr + 1];
				PUTST(opt, s);
				PUTCHAR(opt, ':');
			}
			pmem(opt, off, inst->in_len, oa);
			break;
	}
}

/*
*	register name
*/
#define		SCHARS	"cdt"
STATIC
preg(opt, off, ilen, fl)
	register char **opt;
	long off;
	int ilen, fl;
{
	int i;
	register char *n;

	if (umode & UM_REGPCT)
		PUTCHAR(opt, CREG);
	if ((i = (fl & _XR_X)) >= _CR) {	/* _CR|_DR|_TR - L001 */
		PUTCHAR(opt, SCHARS[(i - _CR) >> _XRSHF]);
		PUTCHAR(opt, 'r');
		*opt += pn(*opt, REGN(fl));
	}
	else {
		/* don't print 'e' if an XR */
		if ((fl & REGSIZ) == EX && ((fl & _XR_X) == 0))
			PUTCHAR(opt, 'e');
		if (fl & _GR_X)
			n = regn_gr[GRN(fl)];
		else
			n = regn_xr[XRN(fl)];
		PUTST(opt, n);
		if (!n[1]) {
			switch (fl & REGSIZ) {
				case LO:
					PUTCHAR(opt, 'l');
					break;

				case HI:
					PUTCHAR(opt, 'h');
					break;

				case X:
				case EX:
					PUTCHAR(opt, 'x');
					break;
			}
		}
	}
}

/*
*	print an immediate value
*		immediates come sign extended, so if only the original
*		value is wanted...
*/
STATIC
pimm(opt, off, ilen, fl, c0, c1, md)
	register char **opt;
	long off;
	int ilen, fl, c0, c1, md;
{
	int x, sz;
	char *symname(), *s, bf[9];

	if (fl & OFI_JMCL) {

		/*
		 * Jump or Call
		 */

		x = c0;
		if (fl & OFI_RELIP)
			x += ilen + off;
		if ((umode & UM_SYMONLY) && (s = symname(x, umode_symflag))) {
								/* L005 */
			PUTST(opt, s);
			return 0;
		}
		else {
			*opt += pn(*opt, c0);
			if (symp) {
				strcpy(symp2, symp);
				symp = 0;
			}
			symp = symname(x, umode_symflag);	/* L005 */
		}
	}
	else {
		sz = fl & OFI_SIZE;
		if (fl & OFIS_PTR) {
			if (sz == OFI_SZ32) {		/* 16:16 *//* L009v */
				pnzb(opt, ((unsigned)c0 & 0xFFFF0000) >> 16, 4);
				PUTCHAR(opt, ':');
				pnzb(opt, (c0 & 0x0FFFF), 4);
			}
			else if (sz == OFI_SZ48) {	/* 16:32 */
				pnzb(opt, c1, 4);
				PUTCHAR(opt, ':');
				pnzb(opt, c0, 8);
			}
			else if (sz == OFI_SZ64) {	/* [32,32] */
				PUTCHAR(opt, '['	/*]*/);
				pnzb(opt, c0, 8);
				PUTCHAR(opt, ',');
				pnzb(opt, c1, 8);
				PUTCHAR(opt, /*[*/	']');
			}
		}
		else {

			if ((umode & UM_IMMED) && (fl & OF_IMMED) &&
			    !(fl & OFI_ADDR))
				P_DOLLAR(opt);

			if (umode & UM_SYMONLY) {		/* v L002 v */

				/*
				 * If requested, output a '$' before all
				 * symbols which are being accessed in
				 * immediate mode.
				 */

				if (s = symname(c0, umode_symflag | 1)) {
					PUTST(opt, s);
				} else {
					/*
					 * Display 8-bit index offsets in
					 * signed format.
					 *			   L006v
					 */
					if ((fl & OFI_ADDR) && sz == OFI_SZ8) {
						if (c0 & 0x80) {
							**opt = '-';
							(*opt)++;
							c0 = (~c0 + 1) & 0xff;
						}
					}			/* L006^ */
					*opt += pn(*opt, c0);
				}				/* ^ L002 ^ */
				return 0;
			}
			else {
				if (symp) {
					strcpy(symp2, symp);
					symp = 0;
				}
				symp = symname(c0, umode_symflag | 1);
								/* L005 */
				*opt += pn(*opt, c0);
			}
		}
	}
	/*
	*	reach here: not a symbol
	*/
	if (md)
		P_LPAR(opt);
	return 1;
}

STATIC
pmem(opt, off, ilen, oa)
	register char **opt;
	long off;
	int ilen;
	register struct operand *oa;
{
	char scal;
	int ncont, i, nb, npn = 0, pp = 0, ps;
	long cont, disp;

	ncont = oa->oa_flag & OFM_NX;
	/*
	*	if there are 3 cont's, the
	*	first a DISP and the third a REG==BP
	*	and the DISP is positive,
	*	then it's an argument to the function.
	*/
	if (umode & UM_ARGS) {
		if (ncont == 3 &&
		     (oa->oa_cont[0] & CTYPE)		== C_DISP	&&
		    ((cont = oa->oa_cont[2]) & CTYPE)	== C_REG	&&
		    (cont & _GR_X)			== _GR_BP	&&
		    (disp = oa->oa_cont[1]) > 0				&&
		    (disp < UMNARG*4)
		) {
			PUTCHAR(opt, '@');
			*opt += pn(*opt, (disp - 4) / 4);
			return;
		}
	}

	for (i = 0;i < ncont;i++) {
		cont = oa->oa_cont[i];
		switch (cont & CTYPE) {
			case C_REG:
				if (!pp++)
					P_LPAR(opt);
				if (npn++)
					PUTCHAR(opt, '+');
				preg(opt, off, ilen, cont & REGISTER);
				scal = ((cont & CR_SCALE) >> SCALE_SHF);
				if (scal > 1) {
					PUTCHAR(opt, '*');
					*opt += pn(*opt, scal);
					++npn;
				}
				break;

			case C_DISP:
				/*
				*	A displacement off of a register.
				*/
				nb = cont & C_DISPM;
				disp = oa->oa_cont[++i];
				switch (nb) {	
					case 1:
						disp &= 0xFF;
						break;
					case 2:
						disp &= 0xFFFF;
						break;
				}

				/*
				*	we want pimm() to P_LPAR() if
				*	(UM_DISP and it's not a symbol)
				*/
				ps = pimm(opt, off, ilen, OFI_ADDR|(nb<<OFI_SHF),
					disp, 0, umode & UM_DISP);
				if (umode & UM_DISP) {
					/*
					*	pimm() P_LPAR()s for us
					*/
					++pp;
					if (i+1 != ncont)
						PUTCHAR(opt, '+');
				}
				else if (i+1 != ncont) {
					++pp;
					P_LPAR(opt);
				}
				break;
		}
	}
	if (pp)
		P_RPAR(opt);
}

struct opmap *
getopm(inp)
	struct instr *inp;
{
	unsigned char *s = IN_OPCODE(inp);

	if (*s == 0x0F) {
		++s;
		return &opmap2b[*s];
	}
	else
		return &opmap[*s];
}

/*
 * Routine to print out the effective address of an indirect call
 * instruction (eg: "call 14(%eax)").
 */

print_call_eaddr(instp)						/* L011v */
struct instr *instp;
{
	int addr;
	struct operand *oa;
	char *s;

	/*
	 * Do nothing if the instruction is not a call.
	 */

	if (!(instp->in_flag & I_CALL) || instp->in_nopnd != 1)
		return;

	oa = &instp->in_opnd[0];

	if (!(oa->oa_flag & OF_MEM))
		return;

	/*
	 * Determine if the form of the instruction is a 32-bit
	 * register plus non-scaled displacement.
	 */

	if ((oa->oa_flag & OFM_NX) == 3 &&
	    (oa->oa_cont[0] & CTYPE) == C_DISP &&
	    ((oa->oa_cont[2] & (CTYPE|CR_SCALE|EX)) == (C_REG|EX))) {

		/* obtain register value */

		switch (oa->oa_cont[2] & _GR_X) {
		case _GR_A:
			addr = REGP[T_EAX];
			break;
		case _GR_B:
			addr = REGP[T_EBX];
			break;
		case _GR_C:
			addr = REGP[T_ECX];
			break;
		case _GR_D:
			addr = REGP[T_EDX];
			break;
		case _GR_DI:
			addr = REGP[T_EDI];
			break;
		case _GR_SI:
			addr = REGP[T_ESI];
			break;
		case _GR_BP:
			addr = REGP[T_EBP];
			break;
		case _GR_SP:
			addr = REGP[T_ESP];
			break;
		default:
			return;
		}

		/*
		 * Add in the displacement and go indirect if required.
		 * If there is a valid symbol at the address, then
		 * print it out.
		 */

		addr += oa->oa_cont[1];

		if ((oa->oa_flag & OF_INDIR) && !db_getlong(REGP[T_CS], addr, &addr))
				return;

		if ((s = symname(addr, 2)) != NULL)
			printf(" <%s>", s);
	}
}								/* L011^ */

/*_____________________________________________________________________________*
*                                                                              *
*                       Tables for disassembler use                            *
*                                                                              *
*                                                                              *
*       Some tables have a page number reference.                              *
*       These refer to:                                                        *
*              80386 Programmer's Reference Manual                             *
*              1987 Issue                                                      *
*              Intel Order Number: 230985-001                                  *
*              ISBN 1-55512-022-9                                              *
*                                                                              *
*_____________________________________________________________________________*/







/*
*	page A-4,5
*	"One-Byte Opcode Map"
*		the first 0x100 opcodes
*/
STATIC struct opmap opmap[0x100] = {
/*--------------------------------------------------------------*/
/*-opcode  type           name          op1     op2     op3  ---*/
/*--------------------------------------------------------------*/
/* 00 */ { C_NORM|C_B,    "add",	A_E|Ob,	A_G|Ob,	0	},
/* 01 */ { C_NORM|C_F,    "add",	A_E|Ov,	A_G|Ov,	0	},
/* 02 */ { C_NORM|C_B,    "add",	A_G|Ob,	A_E|Ob,	0	},
/* 03 */ { C_NORM|C_F,    "add",	A_G|Ov,	A_E|Ov,	0	},
/* 04 */ { C_NORM|C_B,    "add",	I_AL,	A_I|Ob,	0	},
/* 05 */ { C_NORM|C_F,    "add",	I_eAX,	A_I|Ov,	0	},
/* 06 */ { C_NORM|C_F,    "push",	I_ES,	0,	0	},
/* 07 */ { C_NORM|C_F,    "pop",	I_ES,	0,	0	},
/* 08 */ { C_NORM|C_B,    "or",		A_E|Ob,	A_G|Ob,	0	},
/* 09 */ { C_NORM|C_F,    "or",		A_E|Ov,	A_G|Ov,	0	},
/* 0A */ { C_NORM|C_B,    "or",		A_G|Ob,	A_E|Ob,	0	},
/* 0B */ { C_NORM|C_F,    "or",		A_G|Ov,	A_E|Ov,	0	},
/* 0C */ { C_NORM|C_B,    "or",		I_AL,	A_I|Ob,	0	},
/* 0D */ { C_NORM|C_F,    "or",		I_eAX,	A_I|Ov,	0	},
/* 0E */ { C_NORM|C_F,    "push",	I_CS,	0,	0	},
/* 0F */ { C_2BYTE,       0,		0,	0,	0	},
/* 10 */ { C_NORM|C_B,    "adc",	A_E|Ob,	A_G|Ob,	0	},
/* 11 */ { C_NORM|C_F,    "adc",	A_E|Ov,	A_G|Ov,	0	},
/* 12 */ { C_NORM|C_B,    "adc",	A_G|Ob,	A_E|Ob,	0	},
/* 13 */ { C_NORM|C_F,    "adc",	A_G|Ov,	A_E|Ov,	0	},
/* 14 */ { C_NORM|C_B,    "adc",	I_AL,	A_I|Ob,	0	},
/* 15 */ { C_NORM|C_F,    "adc",	I_eAX,	A_I|Ov,	0	},
/* 16 */ { C_NORM|C_F,    "push",	I_SS,	0,	0	},
/* 17 */ { C_NORM|C_F,    "pop",	I_SS,	0,	0	},
/* 18 */ { C_NORM|C_B,    "sbb",	A_E|Ob,	A_G|Ob,	0	},
/* 19 */ { C_NORM|C_F,    "sbb",	A_E|Ov,	A_G|Ov,	0	},
/* 1A */ { C_NORM|C_B,    "sbb",	A_G|Ob,	A_E|Ob,	0	},
/* 1B */ { C_NORM|C_F,    "sbb",	A_G|Ov,	A_E|Ov,	0	},
/* 1C */ { C_NORM|C_B,    "sbb",	I_AL,	A_I|Ob,	0	},
/* 1D */ { C_NORM|C_F,    "sbb",	I_eAX,	A_I|Ov,	0	},
/* 1E */ { C_NORM|C_F,    "push",	I_DS,	0,	0	},
/* 1F */ { C_NORM|C_F,    "pop",	I_DS,	0,	0	},
/* 20 */ { C_NORM|C_B,    "and",	A_E|Ob,	A_G|Ob,	0	},
/* 21 */ { C_NORM|C_F,    "and",	A_E|Ov,	A_G|Ov,	0	},
/* 22 */ { C_NORM|C_B,    "and",	A_G|Ob,	A_E|Ob,	0	},
/* 23 */ { C_NORM|C_F,    "and",	A_G|Ov,	A_E|Ov,	0	},
/* 24 */ { C_NORM|C_B,    "and",	I_AL,	A_I|Ob,	0	},
/* 25 */ { C_NORM|C_F,    "and",	I_eAX,	A_I|Ov,	0	},
/* 26 */ { C_SEG|S_ES,    0,		0,	0,	0	},
/* 27 */ { C_NORM,        "daa",	0,	0,	0	},
/* 28 */ { C_NORM|C_B,    "sub",	A_E|Ob,	A_G|Ob,	0	},
/* 29 */ { C_NORM|C_F,    "sub",	A_E|Ov,	A_G|Ov,	0	},
/* 2A */ { C_NORM|C_B,    "sub",	A_G|Ob,	A_E|Ob,	0	},
/* 2B */ { C_NORM|C_F,    "sub",	A_G|Ov,	A_E|Ov,	0	},
/* 2C */ { C_NORM|C_B,    "sub",	I_AL,	A_I|Ob,	0	},
/* 2D */ { C_NORM|C_F,    "sub",	I_eAX,	A_I|Ov,	0	},
/* 2E */ { C_SEG|S_CS,    0,		0,	0,	0	},
/* 2F */ { C_NORM,        "das",	0,	0,	0	},
/* 30 */ { C_NORM|C_B,    "xor",	A_E|Ob,	A_G|Ob,	0	},
/* 31 */ { C_NORM|C_F,    "xor",	A_E|Ov,	A_G|Ov,	0	},
/* 32 */ { C_NORM|C_B,    "xor",	A_G|Ob,	A_E|Ob,	0	},
/* 33 */ { C_NORM|C_F,    "xor",	A_G|Ov,	A_E|Ov,	0	},
/* 34 */ { C_NORM|C_B,    "xor",	I_AL,	A_I|Ob,	0	},
/* 35 */ { C_NORM|C_F,    "xor",	I_eAX,	A_I|Ov,	0	},
/* 36 */ { C_SEG|S_SS,    0,		0,	0,	0	},
/* 37 */ { C_NORM,        "aaa",	0,	0,	0	},
/* 38 */ { C_NORM|C_B,    "cmp",	A_E|Ob,	A_G|Ob,	0	},
/* 39 */ { C_NORM|C_F,    "cmp",	A_E|Ov,	A_G|Ov,	0	},
/* 3A */ { C_NORM|C_B,    "cmp",	A_G|Ob,	A_E|Ob,	0	},
/* 3B */ { C_NORM|C_F,    "cmp",	A_G|Ov,	A_E|Ov,	0	},
/* 3C */ { C_NORM|C_B,    "cmp",	I_AL,	A_I|Ob,	0	},
/* 3D */ { C_NORM|C_F,    "cmp",	I_eAX,	A_I|Ov,	0	},
/* 3E */ { C_SEG|S_CS,    0,		0,	0,	0	},
/* 3F */ { C_NORM,        "aas",	0,	0,	0	},
/* 40 */ { C_NORM|C_F,    "inc",	I_eAX,	0,	0	},
/* 41 */ { C_NORM|C_F,    "inc",	I_eCX,	0,	0	},
/* 42 */ { C_NORM|C_F,    "inc",	I_eDX,	0,	0	},
/* 43 */ { C_NORM|C_F,    "inc",	I_eBX,	0,	0	},
/* 44 */ { C_NORM|C_F,    "inc",	I_eSP,	0,	0	},
/* 45 */ { C_NORM|C_F,    "inc",	I_eBP,	0,	0	},
/* 46 */ { C_NORM|C_F,    "inc",	I_eSI,	0,	0	},
/* 47 */ { C_NORM|C_F,    "inc",	I_eDI,	0,	0	},
/* 48 */ { C_NORM|C_F,    "dec",	I_eAX,	0,	0	},
/* 49 */ { C_NORM|C_F,    "dec",	I_eCX,	0,	0	},
/* 4A */ { C_NORM|C_F,    "dec",	I_eDX,	0,	0	},
/* 4B */ { C_NORM|C_F,    "dec",	I_eBX,	0,	0	},
/* 4C */ { C_NORM|C_F,    "dec",	I_eSP,	0,	0	},
/* 4D */ { C_NORM|C_F,    "dec",	I_eBP,	0,	0	},
/* 4E */ { C_NORM|C_F,    "dec",	I_eSI,	0,	0	},
/* 4F */ { C_NORM|C_F,    "dec",	I_eDI,	0,	0	},
/* 50 */ { C_NORM|C_F,    "push",	I_eAX,	0,	0	},
/* 51 */ { C_NORM|C_F,    "push",	I_eCX,	0,	0	},
/* 52 */ { C_NORM|C_F,    "push",	I_eDX,	0,	0	},
/* 53 */ { C_NORM|C_F,    "push",	I_eBX,	0,	0	},
/* 54 */ { C_NORM|C_F,    "push",	I_eSP,	0,	0	},
/* 55 */ { C_NORM|C_F,    "push",	I_eBP,	0,	0	},
/* 56 */ { C_NORM|C_F,    "push",	I_eSI,	0,	0	},
/* 57 */ { C_NORM|C_F,    "push",	I_eDI,	0,	0	},
/* 58 */ { C_NORM|C_F,    "pop",	I_eAX,	0,	0	},
/* 59 */ { C_NORM|C_F,    "pop",	I_eCX,	0,	0	},
/* 5A */ { C_NORM|C_F,    "pop",	I_eDX,	0,	0	},
/* 5B */ { C_NORM|C_F,    "pop",	I_eBX,	0,	0	},
/* 5C */ { C_NORM|C_F,    "pop",	I_eSP,	0,	0	},
/* 5D */ { C_NORM|C_F,    "pop",	I_eBP,	0,	0	},
/* 5E */ { C_NORM|C_F,    "pop",	I_eSI,	0,	0	},
/* 5F */ { C_NORM|C_F,    "pop",	I_eDI,	0,	0	},
/* 60 */ { C_NORM|C_F,    "pusha",	0,	0,	0	},
/* 61 */ { C_NORM|C_F,    "popa",	0,	0,	0	},
/* 62 */ { C_NORM|C_F,    "bound",	A_G|Ov,	A_M|Oa,	0	},
/* 63 */ { C_NORM,        "arpl",	A_E|Ow,	A_R|Ow,	0	},
/* 64 */ { C_SEG|S_FS,    0,		0,	0,	0	},
/* 65 */ { C_SEG|S_GS,    0,		0,	0,	0	},
/* 66 */ { C_PREFIX,      0,		0,	0,	0	},
/* 67 */ { C_PREFIX,      0,		0,	0,	0	},
/* 68 */ { C_NORM|C_F,    "push",	A_I|Ov,	0,	0	},
/* 69 */ { C_NORM|C_F,    "imul",	A_G|Ov,	A_E|Ov,	A_I|Ov	},
/* 6A */ { C_NORM|C_F,    "push",	A_I|Ob,	0,	0	},
/* 6B */ { C_NORM,        "imulb",	A_G|Ov,	A_E|Ov,	A_I|Ob	},
/* 6C */ { C_NORM|C_F,    "ins",	I_ESDI,	I_DX,	0	},
/* 6D */ { C_NORM|C_F,    "ins",	I_ESDI,	I_DX,	0	},
/* 6E */ { C_NORM|C_F,    "outs",	I_DX,	I_DSSI,	0	},
/* 6F */ { C_NORM|C_F,    "outs",	I_DX,	I_DSSI,	0	},
/* 70 */ { C_NORM|C_JT,   "jo",		A_J|Ob,	0,	1	},
/* 71 */ { C_NORM|C_JT,   "jno",	A_J|Ob,	0,	2	},
/* 72 */ { C_NORM|C_JT,   "jb",		A_J|Ob,	0,	3	},
/* 73 */ { C_NORM|C_JT,   "jnb",	A_J|Ob,	0,	4	},
/* 74 */ { C_NORM|C_JT,   "je",		A_J|Ob,	0,	5	},
/* 75 */ { C_NORM|C_JT,   "jne",	A_J|Ob,	0,	6	},
/* 76 */ { C_NORM|C_JT,   "jbe",	A_J|Ob,	0,	7	},
/* 77 */ { C_NORM|C_JT,   "jnbe",	A_J|Ob,	0,	8	},
/* 78 */ { C_NORM|C_JT,   "js",		A_J|Ob,	0,	9	},
/* 79 */ { C_NORM|C_JT,   "jns",	A_J|Ob,	0,	10	},
/* 7A */ { C_NORM|C_JT,   "jp",		A_J|Ob,	0,	11	},
/* 7B */ { C_NORM|C_JT,   "jnp",	A_J|Ob,	0,	12	},
/* 7C */ { C_NORM|C_JT,   "jl",		A_J|Ob,	0,	13	},
/* 7D */ { C_NORM|C_JT,   "jnl",	A_J|Ob,	0,	14	},
/* 7E */ { C_NORM|C_JT,   "jle",	A_J|Ob,	0,	15	},
/* 7F */ { C_NORM|C_JT,   "jnle",	A_J|Ob,	0,	16	},
/* 80 */ { C_GP|G_1|C_B,  0,		A_E|Ob,	A_I|Ob,	0	},
/* 81 */ { C_GP|G_1|C_F,  0,		A_E|Ov,	A_I|Ov,	0	},
/* 82 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 83 */ { C_GP|G_1|C_B,  0,		A_E|Ov,	A_I|Ob,	0	},
/* 84 */ { C_NORM|C_B,    "test",	A_E|Ob,	A_G|Ob,	0	},
/* 85 */ { C_NORM|C_F,    "test",	A_E|Ov,	A_G|Ov,	0	},
/* 86 */ { C_NORM|C_B,    "xchg",	A_E|Ob,	A_G|Ob,	0	},
/* 87 */ { C_NORM|C_F,    "xchg",	A_E|Ov,	A_G|Ov,	0	},
/* 88 */ { C_NORM|C_B,    "mov",	A_E|Ob,	A_G|Ob,	0	},
/* 89 */ { C_NORM|C_F,    "mov",	A_E|Ov,	A_G|Ov,	0	},
/* 8A */ { C_NORM|C_B,    "mov",	A_G|Ob,	A_E|Ob,	0	},
/* 8B */ { C_NORM|C_F,    "mov",	A_G|Ov,	A_E|Ov,	0	},
/* 8C */ { C_NORM|C_F,    "mov",	A_E|Ow,	A_S|Ow,	0	},
/* 8D */ { C_NORM|C_F,    "lea",	A_G|Ov,	A_M|Ov,	0	},
/* 8E */ { C_NORM|C_F,    "mov",	A_S|Ow,	A_E|Ow,	0	},
/* 8F */ { C_NORM|C_F,    "pop",	A_E|Ov,	0,	0	},
/* 90 */ { C_NORM,        "nop",	0,	0,	0	},
/* 91 */ { C_NORM|C_F,    "xchg ",	I_eAX,	I_eCX,	0	},
/* 92 */ { C_NORM|C_F,    "xchg",	I_eAX,	I_eDX,	0	},
/* 93 */ { C_NORM|C_F,    "xchg",	I_eAX,	I_eBX,	0	},
/* 94 */ { C_NORM|C_F,    "xchg",	I_eAX,	I_eSP,	0	},
/* 95 */ { C_NORM|C_F,    "xchg",	I_eAX,	I_eBP,	0	},
/* 96 */ { C_NORM|C_F,    "xchg",	I_eAX,	I_eSI,	0	},
/* 97 */ { C_NORM|C_F,    "xchg",	I_eAX,	I_eDI,	0	},
/* 98 */ { C_NORM,        "cbw",	0,	0,	0	},
/* 99 */ { C_NORM,        "cwd",	0,	0,	0	},
/* 9A */ { C_NORM|C_JT|C_CALL, "lcall",	A_I|Op,	0,	0	},
/* 9B */ { C_NORM,        "wait",	0,	0,	0	},
/* 9C */ { C_NORM|C_F,    "pushf",	0,	0,	0	},
/* 9D */ { C_NORM|C_F,    "popf",	0,	0,	0	},
/* 9E */ { C_NORM,        "sahf",	0,	0,	0	},
/* 9F */ { C_NORM,        "lahf",	0,	0,	0	},
/* A0 */ { C_NORM|C_F,    "mov",	I_AL,	A_O|Ob,	0	},
/* A1 */ { C_NORM|C_F,    "mov",	I_eAX,	A_O|Ov,	0	},
/* A2 */ { C_NORM|C_F,    "mov",	A_O|Ob,	I_AL,	0	},
/* A3 */ { C_NORM|C_F,    "mov",	A_O|Ov,	I_eAX,	0	},
/* A4 */ { C_NORM|C_F,    "movs",	I_DSSI,	I_ESDI,	0	},
/* A5 */ { C_NORM|C_F,    "movs",	I_DSSI,	I_ESDI,	0	},
/* A6 */ { C_NORM|C_F,    "cmps",	I_DSSI,	I_ESDI,	0	},
/* A7 */ { C_NORM|C_F,    "cmps",	I_DSSI,	I_ESDI,	0	},
/* A8 */ { C_NORM|C_B,    "test",	I_AL,	A_I|Ob,	0	},
/* A9 */ { C_NORM|C_F,    "test",	I_eAX,	A_I|Ov,	0	},
/* AA */ { C_NORM|C_F,    "stos",	I_ESDI,	I_AL,	0	},
/* AB */ { C_NORM|C_F,    "stos",	I_ESDI,	I_eAX,	0	},
/* AC */ { C_NORM|C_F,    "lods",	I_AL,	I_DSSI,	0	},
/* AD */ { C_NORM|C_F,    "lods",	I_eAX,	I_DSSI,	0	},
/* AE */ { C_NORM|C_F,    "scas",	I_AL,	I_DSSI,	0	},
/* AF */ { C_NORM|C_F,    "scas",	I_eAX,	I_DSSI,	0	},
/* B0 */ { C_NORM|C_B,    "mov",	I_AL,	A_I|Ob,	0	},
/* B1 */ { C_NORM|C_B,    "mov",	I_CL,	A_I|Ob,	0	},
/* B2 */ { C_NORM|C_B,    "mov",	I_DL,	A_I|Ob,	0	},
/* B3 */ { C_NORM|C_B,    "mov",	I_BL,	A_I|Ob,	0	},
/* B4 */ { C_NORM|C_B,    "mov",	I_AH,	A_I|Ob,	0	},
/* B5 */ { C_NORM|C_B,    "mov",	I_CH,	A_I|Ob,	0	},
/* B6 */ { C_NORM|C_B,    "mov",	I_DH,	A_I|Ob,	0	},
/* B7 */ { C_NORM|C_B,    "mov",	I_BH,	A_I|Ob,	0	},
/* B8 */ { C_NORM|C_F,    "mov",	I_eAX,	A_I|Ov,	0	},
/* B9 */ { C_NORM|C_F,    "mov",	I_eCX,	A_I|Ov,	0	},
/* BA */ { C_NORM|C_F,    "mov",	I_eDX,	A_I|Ov,	0	},
/* BB */ { C_NORM|C_F,    "mov",	I_eBX,	A_I|Ov,	0	},
/* BC */ { C_NORM|C_F,    "mov",	I_eSP,	A_I|Ov,	0	},
/* BD */ { C_NORM|C_F,    "mov",	I_eBP,	A_I|Ov,	0	},
/* BE */ { C_NORM|C_F,    "mov",	I_eSI,	A_I|Ov,	0	},
/* BF */ { C_NORM|C_F,    "mov",	I_eDI,	A_I|Ov,	0	},
/* C0 */ { C_GP|G_S|G_2|C_B,  0,	A_E|Ob,	A_I|Ob,	0	},
/* C1 */ { C_GP|G_S|G_2|C_F,  0,	A_E|Ov,	A_I|Ob,	0	},
/* C2 */ { C_NORM,        "ret",	A_I|Ow,	0,	0	},
/* C3 */ { C_NORM,        "ret",	0,	0,	0	},
/* C4 */ { C_NORM,        "les",	A_G|Ov,	A_M|Op,	0	},
/* C5 */ { C_NORM,        "lds",	A_G|Ov,	A_M|Op,	0	},
/* C6 */ { C_NORM|C_B,    "mov",	A_E|Ob,	A_I|Ob,	0	},
/* C7 */ { C_NORM|C_F,    "mov",	A_E|Ov,	A_I|Ov,	0	},
/* C8 */ { C_NORM,        "enter",	A_I|Ow,	A_I|Ob,	0	},
/* C9 */ { C_NORM,        "leave",	0,	0,	0	},
/* CA */ { C_NORM,        "ret",	A_I|Ow,	0,	0	},
/* CB */ { C_NORM,        "ret",	0,	0,	0	},
/* CC */ { C_NORM,        "int3",	0,	0,	0	},
/* CD */ { C_NORM,        "int",	A_I|Ob,	0,	0	},
/* CE */ { C_NORM,        "into",	0,	0,	0	},
/* CF */ { C_NORM,        "iret",	0,	0,	0	},
/* D0 */ { C_GP|G_S|G_2|C_B,  0,	A_E|Ob,	I_1,	0	},
/* D1 */ { C_GP|G_S|G_2|C_F,  0,	A_E|Ov,	I_1,	0	},
/* D2 */ { C_GP|G_S|G_2|C_B,  0,	A_E|Ob,	I_CL,	0	},
/* D3 */ { C_GP|G_S|G_2|C_F,  0,	A_E|Ov,	I_CL,	0	},
/* D4 */ { C_NORM,        "aam",	0,	0,	0	},
/* D5 */ { C_NORM,        "aad",	0,	0,	0	},
/* D6 */ { C_NOSUCH,      0,		0,	0,	0	},
/* D7 */ { C_NORM,        "xlat",	0,	0,	0	},
/* D8 */ { C_COPROC,      0,		0,	0,	0	},
/* D9 */ { C_COPROC,      0,		0,	0,	0	},
/* DA */ { C_COPROC,      0,		0,	0,	0	},
/* DB */ { C_COPROC,      0,		0,	0,	0	},
/* DC */ { C_COPROC,      0,		0,	0,	0	},
/* DD */ { C_COPROC,      0,		0,	0,	0	},
/* DE */ { C_COPROC,      0,		0,	0,	0	},
/* DF */ { C_COPROC,      0,		0,	0,	0	},
/* E0 */ { C_NORM|C_JT,   "loopne",	A_J|Ob,	0,	0	},
/* E1 */ { C_NORM|C_JT,   "loope",	A_J|Ob,	0,	0	},
/* E2 */ { C_NORM|C_JT,   "loop",	A_J|Ob,	0,	0	},
/* E3 */ { C_NORM|C_JT,   "jcxz",	A_J|Ob,	0,	17	},
/* E4 */ { C_NORM|C_B,    "in",		I_AL,	A_I|Ob,	0	},
/* E5 */ { C_NORM|C_F,    "in",		I_eAX,	A_I|Ob,	0	},
/* E6 */ { C_NORM|C_B,    "out",	A_I|Ob,	I_AL,	0	},
/* E7 */ { C_NORM|C_F,    "out",	A_I|Ob,	I_eAX,	0	},
/* E8 */ { C_NORM|C_JT|C_CALL,  "call",	A_J|Ov,	0,	0	},
/* E9 */ { C_NORM|C_JT,   "jmp",	A_J|Ov,	0,	0	},
/* EA */ { C_NORM|C_JT,   "jmp",	A_A|Op,	0,	0	},
/* EB */ { C_NORM|C_JT,   "jmp",	A_J|Ob,	0,	0	},
/* EC */ { C_NORM|C_F,    "in",		I_AL,	I_DX,	0	},
/* ED */ { C_NORM|C_F,    "in",		I_eAX,	I_DX,	0	},
/* EE */ { C_NORM|C_F,    "out",	I_DX,	I_AL,	0	},
/* EF */ { C_NORM|C_F,    "out",	I_DX,	I_eAX,	0	},
/* F0 */ { C_PREFIX,      "lock",	0,	0,	0	},
/* F1 */ { C_NOSUCH,      0,		0,	0,	0	},
/* F2 */ { C_PREFIX,      "repnz",	0,	0,	0	},
/* F3 */ { C_PREFIX,      "rep",	0,	0,	0	},
/* F4 */ { C_NORM,        "hlt",	0,	0,	0	},
/* F5 */ { C_NORM,        "cmc",	0,	0,	0	},
/* F6 */ { C_GP|G_U|G_3|C_B,  0,	A_E|Ob,	0,	0	},
/* F7 */ { C_GP|G_U|G_3|C_F,  0,	A_E|Ov,	0,	0	},
/* F8 */ { C_NORM,        "clc",	0,	0,	0	},
/* F9 */ { C_NORM,        "stc",	0,	0,	0	},
/* FA */ { C_NORM,        "cli",	0,	0,	0	},
/* FB */ { C_NORM,        "sti",	0,	0,	0	},
/* FC */ { C_NORM,        "cld",	0,	0,	0	},
/* FD */ { C_NORM,        "std",	0,	0,	0	},
/* FE */ { C_GP|G_C|G_4,  0,		0,	0,	0	},
/* FF */ { C_GP|C_I|G_5,  0,		0,	0,	0	},
};

/*
*	page A-6,7
*	"Two-Byte Opcode Map (first byte is 0FH)"
*		2-byte opmap
*		given that a C_2BYTE has been found, the following can
*		be used to decode the instruction further:
*/
STATIC struct opmap opmap2b[0x100] = {
/*--------------------------------------------------------------*/
/*-opcode  type           name          op1     op2     op3  ---*/
/*--------------------------------------------------------------*/
/* 00 */ { C_GP|G_6,      0,		0,	0,	0	},
/* 01 */ { C_GP|G_7,      0,		0,	0,	0	},
/* 02 */ { C_NORM,        "lar",	A_G|Ov,	A_E|Ow,	0	},
/* 03 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 04 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 05 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 06 */ { C_NORM,        "clts",	0,	0,	0	},
/* 07 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 08 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 09 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 0A */ { C_NOSUCH,      0,		0,	0,	0	},
/* 0B */ { C_NOSUCH,      0,		0,	0,	0	},
/* 0C */ { C_NOSUCH,      0,		0,	0,	0	},
/* 0D */ { C_NOSUCH,      0,		0,	0,	0	},
/* 0E */ { C_NOSUCH,      0,		0,	0,	0	},
/* 0F */ { C_NOSUCH,      0,		0,	0,	0	},
/* 10 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 11 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 12 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 13 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 14 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 15 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 16 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 17 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 18 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 19 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 1A */ { C_NOSUCH,      0,		0,	0,	0	},
/* 1B */ { C_NOSUCH,      0,		0,	0,	0	},
/* 1C */ { C_NOSUCH,      0,		0,	0,	0	},
/* 1D */ { C_NOSUCH,      0,		0,	0,	0	},
/* 1E */ { C_NOSUCH,      0,		0,	0,	0	},
/* 1F */ { C_NOSUCH,      0,		0,	0,	0	},
/* 20 */ { C_NORM|C_F,    "mov",	A_C|Od,	A_R|Od,	0	},
/* 21 */ { C_NORM|C_F,    "mov",	A_D|Od,	A_R|Od,	0	},
/* 22 */ { C_NORM|C_F,    "mov",	A_R|Od,	A_C|Od,	0	},
/* 23 */ { C_NORM|C_F,    "mov",	A_R|Od,	A_D|Od,	0	},
/* 24 */ { C_NORM|C_F,    "mov",	A_T|Od,	A_R|Od,	0	},
/* 25 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 26 */ { C_NORM|C_F,    "mov",	A_R|Od,	A_T|Od,	0	},
/* 27 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 28 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 29 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 2A */ { C_NOSUCH,      0,		0,	0,	0	},
/* 2B */ { C_NOSUCH,      0,		0,	0,	0	},
/* 2C */ { C_NOSUCH,      0,		0,	0,	0	},
/* 2D */ { C_NOSUCH,      0,		0,	0,	0	},
/* 2E */ { C_NOSUCH,      0,		0,	0,	0	},
/* 2F */ { C_NOSUCH,      0,		0,	0,	0	},
/* 30 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 31 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 32 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 33 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 34 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 35 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 36 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 37 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 38 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 39 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 3A */ { C_NOSUCH,      0,		0,	0,	0	},
/* 3B */ { C_NOSUCH,      0,		0,	0,	0	},
/* 3C */ { C_NOSUCH,      0,		0,	0,	0	},
/* 3D */ { C_NOSUCH,      0,		0,	0,	0	},
/* 3E */ { C_NOSUCH,      0,		0,	0,	0	},
/* 3F */ { C_NOSUCH,      0,		0,	0,	0	},
/* 40 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 41 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 42 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 43 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 44 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 45 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 46 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 47 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 48 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 49 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 4A */ { C_NOSUCH,      0,		0,	0,	0	},
/* 4B */ { C_NOSUCH,      0,		0,	0,	0	},
/* 4C */ { C_NOSUCH,      0,		0,	0,	0	},
/* 4D */ { C_NOSUCH,      0,		0,	0,	0	},
/* 4E */ { C_NOSUCH,      0,		0,	0,	0	},
/* 4F */ { C_NOSUCH,      0,		0,	0,	0	},
/* 50 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 51 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 52 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 53 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 54 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 55 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 56 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 57 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 58 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 59 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 5A */ { C_NOSUCH,      0,		0,	0,	0	},
/* 5B */ { C_NOSUCH,      0,		0,	0,	0	},
/* 5C */ { C_NOSUCH,      0,		0,	0,	0	},
/* 5D */ { C_NOSUCH,      0,		0,	0,	0	},
/* 5E */ { C_NOSUCH,      0,		0,	0,	0	},
/* 5F */ { C_NOSUCH,      0,		0,	0,	0	},
/* 60 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 61 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 62 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 63 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 64 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 65 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 66 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 67 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 68 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 69 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 6A */ { C_NOSUCH,      0,		0,	0,	0	},
/* 6B */ { C_NOSUCH,      0,		0,	0,	0	},
/* 6C */ { C_NOSUCH,      0,		0,	0,	0	},
/* 6D */ { C_NOSUCH,      0,		0,	0,	0	},
/* 6E */ { C_NOSUCH,      0,		0,	0,	0	},
/* 6F */ { C_NOSUCH,      0,		0,	0,	0	},
/* 70 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 71 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 72 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 73 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 74 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 75 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 76 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 77 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 78 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 79 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 7A */ { C_NOSUCH,      0,		0,	0,	0	},
/* 7B */ { C_NOSUCH,      0,		0,	0,	0	},
/* 7C */ { C_NOSUCH,      0,		0,	0,	0	},
/* 7D */ { C_NOSUCH,      0,		0,	0,	0	},
/* 7E */ { C_NOSUCH,      0,		0,	0,	0	},
/* 7F */ { C_NOSUCH,      0,		0,	0,	0	},
/* 80 */ { C_NORM|C_JT,   "jo",		A_J|Ov,	0,	1	},
/* 81 */ { C_NORM|C_JT,   "jno",	A_J|Ov,	0,	2	},
/* 82 */ { C_NORM|C_JT,   "jb",		A_J|Ov,	0,	3	},
/* 83 */ { C_NORM|C_JT,   "jnb",	A_J|Ov,	0,	4	},
/* 84 */ { C_NORM|C_JT,   "je",		A_J|Ov,	0,	5	},
/* 85 */ { C_NORM|C_JT,   "jne",	A_J|Ov,	0,	6	},
/* 86 */ { C_NORM|C_JT,   "jbe",	A_J|Ov,	0,	7	},
/* 87 */ { C_NORM|C_JT,   "jnbe",	A_J|Ov,	0,	8	},
/* 88 */ { C_NORM|C_JT,   "js",		A_J|Ov,	0,	9	},
/* 89 */ { C_NORM|C_JT,   "jns",	A_J|Ov,	0,	10	},
/* 8A */ { C_NORM|C_JT,   "jp",		A_J|Ov,	0,	11	},
/* 8B */ { C_NORM|C_JT,   "jnp",	A_J|Ov,	0,	12	},
/* 8C */ { C_NORM|C_JT,   "jl",		A_J|Ov,	0,	13	},
/* 8D */ { C_NORM|C_JT,   "jnl",	A_J|Ov,	0,	14	},
/* 8E */ { C_NORM|C_JT,   "jle",	A_J|Ov,	0,	15	},
/* 8F */ { C_NORM|C_JT,   "jnle",	A_J|Ov,	0,	16	},
/* 90 */ { C_NORM,        "seto",	A_E|Ob,	0,	0	},
/* 91 */ { C_NORM,        "setno",	A_E|Ob,	0,	0	},
/* 92 */ { C_NORM,        "setb",	A_E|Ob,	0,	0	},
/* 93 */ { C_NORM,        "setnb",	A_E|Ob,	0,	0	},
/* 94 */ { C_NORM,        "setz",	A_E|Ob,	0,	0	},
/* 95 */ { C_NORM,        "setnz",	A_E|Ob,	0,	0	},
/* 96 */ { C_NORM,        "setbe",	A_E|Ob,	0,	0	},
/* 97 */ { C_NORM,        "setnbe",	A_E|Ob,	0,	0	},
/* 98 */ { C_NORM,        "sets",	A_E|Ob,	0,	0	},	/*L009*/
/* 99 */ { C_NORM,        "setns",	A_E|Ob,	0,	0	},	/*L009*/
/* 9A */ { C_NORM,        "setp",	A_E|Ob,	0,	0	},	/*L009*/
/* 9B */ { C_NORM,        "setnp",	A_E|Ob,	0,	0	},	/*L009*/
/* 9C */ { C_NORM,        "setl",	A_E|Ob,	0,	0	},	/*L009*/
/* 9D */ { C_NORM,        "setnl",	A_E|Ob,	0,	0	},	/*L009*/
/* 9E */ { C_NORM,        "setle",	A_E|Ob,	0,	0	},	/*L009*/
/* 9F */ { C_NORM,        "setnle",	A_E|Ob,	0,	0	},	/*L009*/
/* A0 */ { C_NORM|C_F,    "push",	I_FS,	0,	0	},
/* A1 */ { C_NORM|C_F,    "pop",	I_FS,	0,	0	},
/* A2 */ { C_NOSUCH,      0,		0,	0,	0	},
/* A3 */ { C_NORM|C_F,    "bt",		A_E|Ov,	A_G|Ov,	0	},
/* A4 */ { C_NORM,        "shld",	A_E|Ov,	A_G|Ov,	A_I|Ob	},
/* A5 */ { C_NORM,        "shld",	A_E|Ov,	A_G|Ov,	I_CL	},
/* A6 */ { C_NOSUCH,      0,		0,	0,	0	},
/* A7 */ { C_NOSUCH,      0,		0,	0,	0	},
/* A8 */ { C_NORM|C_F,    "push",	I_GS,	0,	0	},
/* A9 */ { C_NORM|C_F,    "pop",	I_GS,	0,	0	},
/* AA */ { C_NOSUCH,      0,		0,	0,	0	},
/* AB */ { C_NORM|C_F,    "bts",	A_E|Ov,	A_G|Ov,	0	},
/* AC */ { C_NORM,        "shrd",	A_E|Ov,	A_G|Ov,	A_I|Ob	},
/* AD */ { C_NORM,        "shrd",	A_E|Ov,	A_G|Ov,	I_CL	},
/* AE */ { C_NOSUCH,      0,		0,	0,	0	},
/* AF */ { C_NORM|C_F,    "imul",	A_G|Ov,	A_E|Ov,	0	},
/* B0 */ { C_NOSUCH,      0,		0,	0,	0	},
/* B1 */ { C_NOSUCH,      0,		0,	0,	0	},
/* B2 */ { C_NORM,        "lss",	A_M|Op,	0,	0	},
/* B3 */ { C_NORM|C_F,    "btr",	A_E|Ov,	A_G|Ov,	0	},
/* B4 */ { C_NORM,        "lfs",	A_M|Op,	0,	0	},
/* B5 */ { C_NORM,        "lgs",	A_M|Op,	0,	0	},
/* B6 */ { C_NORM,        "movzx",	A_G|Ov,	A_E|Ob,	0	},
/* B7 */ { C_NORM,        "movzx",	A_G|Ov,	A_E|Ow,	0	},
/* B8 */ { C_NOSUCH,      0,		0,	0,	0	},
/* B9 */ { C_NOSUCH,      0,		0,	0,	0	},
/* BA */ { C_GP|G_8,      0,		A_E|Ov,	A_I|Ob,	0	},
/* BB */ { C_NORM|C_F,    "btc",	A_E|Ov,	A_G|Ov,	0	},
/* BC */ { C_NORM|C_F,    "bsf",	A_G|Ov,	A_E|Ov,	0	},
/* BD */ { C_NORM|C_F,    "bsr",	A_G|Ov,	A_E|Ov,	0	},
/* BE */ { C_NORM,        "movsx",	A_G|Ov,	A_E|Ob,	0	},
/* BF */ { C_NORM,        "movsx",	A_G|Ov,	A_E|Ow,	0	},
/* C0 */ { C_NOSUCH,      0,		0,	0,	0	},
/* C1 */ { C_NOSUCH,      0,		0,	0,	0	},
/* C2 */ { C_NOSUCH,      0,		0,	0,	0	},
/* C3 */ { C_NOSUCH,      0,		0,	0,	0	},
/* C4 */ { C_NOSUCH,      0,		0,	0,	0	},
/* C5 */ { C_NOSUCH,      0,		0,	0,	0	},
/* C6 */ { C_NOSUCH,      0,		0,	0,	0	},
/* C7 */ { C_NOSUCH,      0,		0,	0,	0	},
/* C8 */ { C_NOSUCH,      0,		0,	0,	0	},
/* C9 */ { C_NOSUCH,      0,		0,	0,	0	},
/* CA */ { C_NOSUCH,      0,		0,	0,	0	},
/* CB */ { C_NOSUCH,      0,		0,	0,	0	},
/* CC */ { C_NOSUCH,      0,		0,	0,	0	},
/* CD */ { C_NOSUCH,      0,		0,	0,	0	},
/* CE */ { C_NOSUCH,      0,		0,	0,	0	},
/* CF */ { C_NOSUCH,      0,		0,	0,	0	},
/* D0 */ { C_NOSUCH,      0,		0,	0,	0	},
/* D1 */ { C_NOSUCH,      0,		0,	0,	0	},
/* D2 */ { C_NOSUCH,      0,		0,	0,	0	},
/* D3 */ { C_NOSUCH,      0,		0,	0,	0	},
/* D4 */ { C_NOSUCH,      0,		0,	0,	0	},
/* D5 */ { C_NOSUCH,      0,		0,	0,	0	},
/* D6 */ { C_NOSUCH,      0,		0,	0,	0	},
/* D7 */ { C_NOSUCH,      0,		0,	0,	0	},
/* D8 */ { C_NOSUCH,      0,		0,	0,	0	},
/* D9 */ { C_NOSUCH,      0,		0,	0,	0	},
/* DA */ { C_NOSUCH,      0,		0,	0,	0	},
/* DB */ { C_NOSUCH,      0,		0,	0,	0	},
/* DC */ { C_NOSUCH,      0,		0,	0,	0	},
/* DD */ { C_NOSUCH,      0,		0,	0,	0	},
/* DE */ { C_NOSUCH,      0,		0,	0,	0	},
/* DF */ { C_NOSUCH,      0,		0,	0,	0	},
/* E0 */ { C_NOSUCH,      0,		0,	0,	0	},
/* E1 */ { C_NOSUCH,      0,		0,	0,	0	},
/* E2 */ { C_NOSUCH,      0,		0,	0,	0	},
/* E3 */ { C_NOSUCH,      0,		0,	0,	0	},
/* E4 */ { C_NOSUCH,      0,		0,	0,	0	},
/* E5 */ { C_NOSUCH,      0,		0,	0,	0	},
/* E6 */ { C_NOSUCH,      0,		0,	0,	0	},
/* E7 */ { C_NOSUCH,      0,		0,	0,	0	},
/* E8 */ { C_NOSUCH,      0,		0,	0,	0	},
/* E9 */ { C_NOSUCH,      0,		0,	0,	0	},
/* EA */ { C_NOSUCH,      0,		0,	0,	0	},
/* EB */ { C_NOSUCH,      0,		0,	0,	0	},
/* EC */ { C_NOSUCH,      0,		0,	0,	0	},
/* ED */ { C_NOSUCH,      0,		0,	0,	0	},
/* EE */ { C_NOSUCH,      0,		0,	0,	0	},
/* EF */ { C_NOSUCH,      0,		0,	0,	0	},
/* F0 */ { C_NOSUCH,      0,		0,	0,	0	},
/* F1 */ { C_NOSUCH,      0,		0,	0,	0	},
/* F2 */ { C_NOSUCH,      0,		0,	0,	0	},
/* F3 */ { C_NOSUCH,      0,		0,	0,	0	},
/* F4 */ { C_NOSUCH,      0,		0,	0,	0	},
/* F5 */ { C_NOSUCH,      0,		0,	0,	0	},
/* F6 */ { C_NOSUCH,      0,		0,	0,	0	},
/* F7 */ { C_NOSUCH,      0,		0,	0,	0	},
/* F8 */ { C_NOSUCH,      0,		0,	0,	0	},
/* F9 */ { C_NOSUCH,      0,		0,	0,	0	},
/* FA */ { C_NOSUCH,      0,		0,	0,	0	},
/* FB */ { C_NOSUCH,      0,		0,	0,	0	},
/* FC */ { C_NOSUCH,      0,		0,	0,	0	},
/* FD */ { C_NOSUCH,      0,		0,	0,	0	},
/* FE */ { C_NOSUCH,      0,		0,	0,	0	},
/* FF */ { C_NOSUCH,      0,		0,	0,	0	},
};

/*
*	page A-8
*	"Opcodes determined by bits 5,4,3 of modR/M byte"
*		"group" opcodes
*/
STATIC struct opmap opgroup[8][8] = {
/*--------------------------------------------------------------*/
/*-opcode  type           name          op1     op2     op3  ---*/
/*--------------------------------------------------------------*/
	 { /* GROUP 1						*/
/* 00 */ { C_NORM|C_F,    "add",	0,	0,	0	},
/* 01 */ { C_NORM|C_F,    "or",		0,	0,	0	},
/* 02 */ { C_NORM|C_F,    "adc",	0,	0,	0	},
/* 03 */ { C_NORM|C_F,    "sbb",	0,	0,	0	},
/* 04 */ { C_NORM|C_F,    "and",	0,	0,	0	},
/* 05 */ { C_NORM|C_F,    "sub",	0,	0,	0	},
/* 06 */ { C_NORM|C_F,    "xor",	0,	0,	0	},
/* 07 */ { C_NORM|C_F,    "cmp",	0,	0,	0	},
	 },
	 { /* GROUP 2						*/
/* 00 */ { C_NORM|C_F,    "rol",	0,	0,	0	},
/* 01 */ { C_NORM|C_F,    "ror",	0,	0,	0	},
/* 02 */ { C_NORM|C_F,    "rcl",	0,	0,	0	},
/* 03 */ { C_NORM|C_F,    "rcr",	0,	0,	0	},
/* 04 */ { C_NORM|C_F,    "shl",	0,	0,	0	},
/* 05 */ { C_NORM|C_F,    "shr",	0,	0,	0	},
/* 06 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 07 */ { C_NORM|C_F,    "sar",	0,	0,	0	},
	 },
	 { /* GROUP 3						*/
/* 00 */ { C_NORM|C_F,    "test",	A_I|Ox,	0,	0	},
/* 01 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 02 */ { C_NORM|C_F,    "not",	0,	0,	0	},
/* 03 */ { C_NORM|C_F,    "neg",	0,	0,	0	},
/* 04 */ { C_NORM|C_F,    "mul",	I_xA,	0,	0	},
/* 05 */ { C_NORM|C_F,    "imul",	I_xA,	0,	0	},
/* 06 */ { C_NORM|C_F,    "div",	I_xA,	0,	0	},
/* 07 */ { C_NORM|C_F,    "idiv",	I_xA,	0,	0	},
	 },
	 { /* GROUP 4						*/
/* 00 */ { C_NORM|C_B,    "inc",	A_E|Ob,	0,	0	},
/* 01 */ { C_NORM|C_B,    "dec",	A_E|Ob,	0,	0	},
/* 02 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 03 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 04 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 05 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 06 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 07 */ { C_NOSUCH,      0,		0,	0,	0	},
	 },
	 { /* GROUP 5						*/
/* 00 */ { C_NORM|C_F,    "inc",	A_E|Ov,	0,	0	},
/* 01 */ { C_NORM|C_F,    "dec",	A_E|Ov,	0,	0	},
/* 02 */ { C_NORM|C_JT|C_CALL,  "call",	A_E|Ov,	0,	0	},
/* 03 */ { C_NORM|C_JT|C_CALL,  "call",	A_E|Op,	0,	0	},
/* 04 */ { C_NORM|C_JT,   "jmp",	A_E|Ov,	0,	0	},
/* 05 */ { C_NORM|C_JT,   "jmp",	A_E|Op,	0,	0	},
/* 06 */ { C_NORM|C_F,    "push",	A_E|Ov,	0,	0	},
/* 07 */ { C_NOSUCH,      0,		0,	0,	0	},
	 },
	 { /* GROUP 6						*/
/* 00 */ { C_NORM,        "sldt",	A_E|Ow,	0,	0	},
/* 01 */ { C_NORM,        "str",	A_E|Ow,	0,	0	},
/* 02 */ { C_NORM,        "lldt",	A_E|Ow,	0,	0	},
/* 03 */ { C_NORM,        "ltr",	A_E|Ow,	0,	0	},
/* 04 */ { C_NORM,        "verr",	A_E|Ow,	0,	0	},
/* 05 */ { C_NORM,        "verw",	A_E|Ow,	0,	0	},
/* 06 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 07 */ { C_NOSUCH,      0,		0,	0,	0	},
	 },
	 { /* GROUP 7						*/
/* 00 */ { C_NORM,        "sgdt",	A_M|Os,	0,	0	},
/* 01 */ { C_NORM,        "sidt",	A_M|Os,	0,	0	},
/* 02 */ { C_NORM,        "lgdt",	A_M|Os,	0,	0	},
/* 03 */ { C_NORM,        "lidt",	A_M|Os,	0,	0	},
/* 04 */ { C_NORM,        "smsw",	A_E|Ow,	0,	0	},
/* 05 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 06 */ { C_NORM,        "lmsw",	A_E|Ow,	0,	0	},
/* 07 */ { C_NOSUCH,      0,		0,	0,	0	},
	 },
	 { /* GROUP 8						*/
/* 00 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 01 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 02 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 03 */ { C_NOSUCH,      0,		0,	0,	0	},
/* 04 */ { C_NORM|C_F,    "bt",		0,	0,	0	},
/* 05 */ { C_NORM|C_F,    "bts",	0,	0,	0	},
/* 06 */ { C_NORM|C_F,    "btr",	0,	0,	0	},
/* 07 */ { C_NORM|C_F,    "btc",	0,	0,	0	},
	 },
};

/*
*	page A-1,2
*	"CODES FOR ADDRESSING METHOD"
*		address types
*/
STATIC short addrt[12] = {
/* A 1 */	IMMED,
/* C 2 */	MRM|R_CT,
/* D 3 */	MRM|R_DB,
/* E 4 */	MRM|SPECIAL,
/* G 5 */	MRM|R_GN,
/* I 6 */	IMMED,
/* J 7 */	IMMED|RELIP,
/* M 8 */	MRM|SPECIAL,	/* mem only */
/* O 9 */	OFFSET,
/* R A */	MRM|M_GN,
/* S B */	MRM|R_SG,
/* T C */	MRM|R_TS,
};

/*
*	page A-2
*	"CODES FOR OPERAND TYPE"
*		operand size, depending on opsize override
*/
STATIC unsigned char opndsz[8] = {				/* L009 */
/* a */	O_SSZ(8,4),		/* 32,32	16,16 : in memory	*/
/* b */	O_SSZ(1,1),		/*     8	    8 : byte		*/
/* c */	O_SSZ(2,1),		/*    16            8 : word/byte	*/
/* d */	O_SSZ(4,4),		/*    32	   32 : dword		*/
/* p */	O_SSZ(6,4)|O_PTRT,	/* 16:32	16:16 : pointer		*/
/* s */	O_SSZ(6,6)|O_PTRT,	/* 16:32	16:32 : pseudo-descript	*/
/* v */	O_SSZ(4,2),		/*    32	   16 : dword/word	*/
/* w */	O_SSZ(2,2),		/*    16           16 : word		*/
};








/*
*	implicit operands
*/
STATIC struct operand implic[][2] = {
	{	{OF_REG,		{GR_AL }	},
		{OF_REG,		{GR_AL }	} },
	{	{OF_REG,		{GR_AH }	},
		{OF_REG,		{GR_AH }	} },
	{	{OF_REG,		{GR_EAX}	},
		{OF_REG,		{GR_AX }	} },
	{	{OF_REG,		{GR_BL }	},
		{OF_REG,		{GR_BL }	} },
	{	{OF_REG,		{GR_BH }	},
		{OF_REG,		{GR_BH }	} },
	{	{OF_REG,		{GR_EBX}	},
		{OF_REG,		{GR_BX }	} },
	{	{OF_REG,		{GR_CL }	},
		{OF_REG,		{GR_CL }	} },
	{	{OF_REG,		{GR_CH }	},
		{OF_REG,		{GR_CH }	} },
	{	{OF_REG,		{GR_ECX}	},
		{OF_REG,		{GR_CX }	} },
	{	{OF_REG,		{GR_DL }	},
		{OF_REG,		{GR_DL }	} },
	{	{OF_REG,		{GR_DH }	},
		{OF_REG,		{GR_DH }	} },
	{	{OF_REG,		{GR_DX }	},
		{OF_REG,		{GR_DX }	} },
	{	{OF_REG,		{GR_EDX}	},
		{OF_REG,		{GR_DX }	} },
	{	{OF_REG,		{XR_CS }	},
		{OF_REG,		{XR_CS }	} },
	{	{OF_REG,		{XR_DS }	},
		{OF_REG,		{XR_DS }	} },
	{	{OF_REG,		{XR_ES }	},
		{OF_REG,		{XR_ES }	} },
	{	{OF_REG,		{XR_FS }	},
		{OF_REG,		{XR_FS }	} },
	{	{OF_REG,		{XR_GS }	},
		{OF_REG,		{XR_GS }	} },
	{	{OF_REG,		{XR_SS }	},
		{OF_REG,		{XR_SS }	} },
	{	{OF_REG,		{GR_EBP}	},
		{OF_REG,		{GR_BP }	} },
	{	{OF_REG,		{GR_EDI}	},
		{OF_REG,		{GR_DI }	} },
	{	{OF_REG,		{GR_ESI}	},
		{OF_REG,		{GR_SI }	} },
	{	{OF_REG,		{GR_ESP}	},
		{OF_REG,		{GR_SP }	} },
	{	{OF_REG|OFR_SPEC,	{0     }	},
		{OF_REG|OFR_SPEC,	{0     }	} },
	{	{OF_IMMED|OFI_SZ8,	{1     }	},
		{OF_IMMED|OFI_SZ8,	{1     }	} },
	{	{OF_REG,		{XR_FL }	},
		{OF_REG,		{XR_FL }	} },
	{	{OF_MEM|1,		{C_REG|GR_ESI}	},
		{OF_MEM|1,		{C_REG|GR_SI }	} },
	{	{OF_MEM|1,		{C_REG|GR_EDI}	},
		{OF_MEM|1,		{C_REG|GR_DI }	} },
};

/*
*	page 17-6
*	"Table 17-3.  32-Bit Addressing Forms with the ModR/M Byte"
*		There is apparently an error on this page; as
*		on first thought, 00 100 should be just C_SIB
*		but it appears that 00 100 is just like 10 100 ...
*/
STATIC struct operand modrm32[] = {
/*00 000*/	{ OF_MEM|1,	{C_REG|GR_EAX	 		       } },
/*00 001*/	{ OF_MEM|1,	{C_REG|GR_ECX	 		       } },
/*00 010*/	{ OF_MEM|1,	{C_REG|GR_EDX	 		       } },
/*00 011*/	{ OF_MEM|1,	{C_REG|GR_EBX	 		       } },
/*00 100*/	{ OF_MEM|1,	{C_SIB		 		       } },
/*00 101*/	{ OF_MEM|1,	{C_DISP32    	 		       } },
/*00 110*/	{ OF_MEM|1,	{C_REG|GR_ESI	 		       } },
/*00 111*/	{ OF_MEM|1,	{C_REG|GR_EDI	 		       } },
/*01 000*/	{ OF_MEM|2,	{C_DISP8,     C_REG|GR_EAX	       } },
/*01 001*/	{ OF_MEM|2,	{C_DISP8,     C_REG|GR_ECX	       } },
/*01 010*/	{ OF_MEM|2,	{C_DISP8,     C_REG|GR_EDX	       } },
/*01 011*/	{ OF_MEM|2,	{C_DISP8,     C_REG|GR_EBX	       } },
/*01 100*/	{ OF_MEM|2,	{C_SIB,       C_DISP8     	       } },
/*01 101*/	{ OF_MEM|2,	{C_DISP8,     C_REG|GR_EBP	       } },
/*01 110*/	{ OF_MEM|2,	{C_DISP8,     C_REG|GR_ESI	       } },
/*01 111*/	{ OF_MEM|2,	{C_DISP8,     C_REG|GR_EDI	       } },
/*10 000*/	{ OF_MEM|2,	{C_DISP32,    C_REG|GR_EAX	       } },
/*10 001*/	{ OF_MEM|2,	{C_DISP32,    C_REG|GR_ECX	       } },
/*10 010*/	{ OF_MEM|2,	{C_DISP32,    C_REG|GR_EDX	       } },
/*10 011*/	{ OF_MEM|2,	{C_DISP32,    C_REG|GR_EBX	       } },
/*10 100*/	{ OF_MEM|2,	{C_SIB,       C_DISP32    	       } },
/*10 101*/	{ OF_MEM|2,	{C_DISP32,    C_REG|GR_EBP	       } },
/*10 110*/	{ OF_MEM|2,	{C_DISP32,    C_REG|GR_ESI	       } },
/*10 111*/	{ OF_MEM|2,	{C_DISP32,    C_REG|GR_EDI	       } },
/*11 000*/	{ OF_REG,	{GR_AL,	      GR_AX,	   GR_EAX      } },
/*11 001*/	{ OF_REG,	{GR_CL,	      GR_CX,	   GR_ECX      } },
/*11 010*/	{ OF_REG,	{GR_DL,	      GR_DX,	   GR_EDX      } },
/*11 011*/	{ OF_REG,	{GR_BL,	      GR_BX,	   GR_EBX      } },
/*11 100*/	{ OF_REG,	{GR_AH,       GR_SP,	   GR_ESP      } },
/*11 101*/	{ OF_REG,	{GR_CH,       GR_BP,	   GR_EBP      } },
/*11 110*/	{ OF_REG,	{GR_DH,       GR_SI,	   GR_ESI      } },
/*11 111*/	{ OF_REG,	{GR_BH,       GR_DI,	   GR_EDI      } },
};

/*
*	page 17-5
*	"Table 17-2.  16-Bit Addressing Forms with the ModR/M Byte"
*/
STATIC struct operand modrm16[] = {
/*00 000*/	{ OF_MEM|2,	{C_REG|GR_BX, C_REG|GR_SI	       } },
/*00 001*/	{ OF_MEM|2,	{C_REG|GR_BX, C_REG|GR_DI	       } },
/*00 010*/	{ OF_MEM|2,	{C_REG|GR_BP, C_REG|GR_SI	       } },
/*00 011*/	{ OF_MEM|2,	{C_REG|GR_BP, C_REG|GR_DI	       } },
/*00 100*/	{ OF_MEM|1,	{C_REG|GR_SI			       } },
/*00 101*/	{ OF_MEM|1,	{C_REG|GR_DI			       } },
/*00 110*/	{ OF_MEM|1,	{C_DISP16			       } },
/*00 111*/	{ OF_MEM|1,	{C_REG|GR_BX			       } },
/*01 000*/	{ OF_MEM|3,	{C_DISP8,     C_REG|GR_BX, C_REG|GR_SI } },
/*01 001*/	{ OF_MEM|3,	{C_DISP8,     C_REG|GR_BX, C_REG|GR_DI } },
/*01 010*/	{ OF_MEM|3,	{C_DISP8,     C_REG|GR_BP, C_REG|GR_SI } },
/*01 011*/	{ OF_MEM|3,	{C_DISP8,     C_REG|GR_BP, C_REG|GR_DI } },
/*01 100*/	{ OF_MEM|2,	{C_DISP8,     C_REG|GR_SI	       } },
/*01 101*/	{ OF_MEM|2,	{C_DISP8,     C_REG|GR_DI	       } },
/*01 110*/	{ OF_MEM|2,	{C_DISP8,     C_REG|GR_BP	       } },
/*01 111*/	{ OF_MEM|2,	{C_DISP8,     C_REG|GR_BX	       } },
/*10 000*/	{ OF_MEM|3,	{C_DISP16,    C_REG|GR_BX, C_REG|GR_SI } },
/*10 001*/	{ OF_MEM|3,	{C_DISP16,    C_REG|GR_BX, C_REG|GR_DI } },
/*10 010*/	{ OF_MEM|3,	{C_DISP16,    C_REG|GR_BP, C_REG|GR_SI } },
/*10 011*/	{ OF_MEM|3,	{C_DISP16,    C_REG|GR_BP, C_REG|GR_DI } },
/*10 100*/	{ OF_MEM|2,	{C_DISP16,    C_REG|GR_SI	       } },
/*10 101*/	{ OF_MEM|2,	{C_DISP16,    C_REG|GR_DI	       } },
/*10 110*/	{ OF_MEM|2,	{C_DISP16,    C_REG|GR_BP	       } },
/*10 111*/	{ OF_MEM|2,	{C_DISP16,    C_REG|GR_BX	       } },
/*11 000*/	{ OF_REG,	{GR_AL,       GR_AX,       GR_EAX      } },
/*11 001*/	{ OF_REG,	{GR_CL,       GR_CX,       GR_ECX      } },
/*11 010*/	{ OF_REG,	{GR_DL,       GR_DX,       GR_EDX      } },
/*11 011*/	{ OF_REG,	{GR_BL,       GR_BX,       GR_EBX      } },
/*11 100*/	{ OF_REG,	{GR_AH,       GR_SP,       GR_ESP      } },
/*11 101*/	{ OF_REG,	{GR_CH,       GR_BP,       GR_EBP      } },
/*11 110*/	{ OF_REG,	{GR_DH,       GR_SI,       GR_ESI      } },
/*11 111*/	{ OF_REG,	{GR_BH,       GR_DI,       GR_EDI      } },
};


/*
*	OF_REG:
*/

int regs2[5][8][3] = {
/* GENERAL */ { { GR_AL,	GR_AX, 	GR_EAX	},
		{ GR_CL,	GR_CX, 	GR_ECX	},
		{ GR_DL,	GR_DX, 	GR_EDX	},
		{ GR_BL,	GR_BX, 	GR_EBX	},
		{ GR_AH,	GR_SP, 	GR_ESP	},
		{ GR_CH,	GR_BP, 	GR_EBP	},
		{ GR_DH,	GR_SI, 	GR_ESI	},
		{ GR_BH,	GR_DI, 	GR_EDI	} },
/* CONTROL */ { { CR_0,		CR_0,	CR_0	},
		{ CR_1,		CR_1,	CR_1	},
		{ CR_2,		CR_2,	CR_2	},
		{ CR_3,		CR_3,	CR_3	},
		{ CR_4,		CR_4,	CR_4	},
		{ CR_5,		CR_5,	CR_5	},
		{ CR_6,		CR_6,	CR_6	},
		{ CR_7,		CR_7,	CR_7	} },
/* DEBUG */   { { DR_0,		DR_0,	DR_0	},
		{ DR_1,		DR_1,	DR_1	},
		{ DR_2,		DR_2,	DR_2	},
		{ DR_3,		DR_3,	DR_3	},
		{ DR_4,		DR_4,	DR_4	},
		{ DR_5,		DR_5,	DR_5	},
		{ DR_6,		DR_6,	DR_6	},
		{ DR_7,		DR_7,	DR_7	} },
/* SEGMENT */ { { XR_ES,	XR_ES,	XR_ES	},
		{ XR_CS,	XR_CS,	XR_CS	},
		{ XR_SS,	XR_SS,	XR_SS	},
		{ XR_DS,	XR_DS,	XR_DS	},
		{ XR_FS,	XR_FS,	XR_FS	},
		{ XR_GS,	XR_GS,	XR_GS	} },
/* TEST */    { { TR_0,		TR_0,	TR_0	},
		{ TR_1,	      	TR_1,	TR_1	},
		{ TR_2,		TR_2,	TR_2	},
		{ TR_3,		TR_3,	TR_3	},
		{ TR_4,		TR_4,	TR_4	},
		{ TR_5,		TR_5,	TR_5	},
		{ TR_6,		TR_6,	TR_6	},
		{ TR_7,		TR_7,	TR_7	} }
};

/*
*	page 17-7
*	"Table 17-4.  32-Bit Addressing Forms with the SIB Byte"
*		The following three arrays implement the table.
*/

/*
*	page 17-7
*		index by SIB_SCALE
*/
STATIC long sib32_scale[] = {
	CR_SC1,	CR_SC2,	CR_SC4,	CR_SC8,
};

/*
*	page 17-7
*		index by SIB_INDEX
*/
STATIC long sib32_index[] = {
	GR_EAX,	GR_ECX,	GR_EDX,	GR_EBX,
	0,	GR_EBP,	GR_ESI,	GR_EDI,
};

/*
*	page 17-7
*		index by SIB_BASE
*		There is apparently an error on page 17-7, for in the "NOTES"
*		section at the base of the page, the 5'th index (i.e, index ==
*		5) states [*]:
*			[*] means a disp32 with no base if MOD==00, [ESP]
*			otherwise.  This provides the following addressing
*			modes:
*				disp32[index]		(MOD==00)
*				disp8[EBP][index]	(MOD==01)
*				disp32[EBP][index]	(MOD==10)
*		This is rather cryptic.  I don't remember the problem
*		right now. -- NHI Wed Feb  7 17:11:00 PST 1990
*/
STATIC long sib32_base[] = {
	GR_EAX,	GR_ECX,	GR_EDX,	GR_EBX,
	GR_ESP,	0,	GR_ESI,	GR_EDI,
	/* 0 if SS is 0, GR_EBP otherwise */
	/* SS or MOD ? */
};

/*
*	register names, without pre- or suf- fix
*/
STATIC char *regn_gr[] = {
	"a",
	"b",
	"c",
	"d",
	"bp",
	"sp",
	"di",
	"si",
};

STATIC char *regn_xr[] = {
	"eip",
	"fl",
	"cs",
	"ds",
	"es",
	"fs",
	"gs",
	"ss",
};

#ifdef STDALONE


/*_____________________________________________________________________________*
*                                                                              *
*                                                                              *
*           Standalone test program.                                           *
*           Compile with -DSTDALONE to include.                                *
*           Fully self-contained.                                              *
*_____________________________________________________________________________*/

d_pnzb(bp, n, nd)
	register char **bp;
	register int n, nd;
{
#define		HEXDIGS	"0123456789ABCDEF"
 	for (--nd;nd >= 0;nd--)
 		*(*bp)++ = HEXDIGS[(n >> (4 * nd)) & 0xF];
}

d_prstb(bp, s, l)
	register char **bp;
	register char s[];
{
	register int i;

	for (i = 0;i < l && s[i];i++)
		*(*bp)++ = s[i];
	for (;i < l;i++)
		*(*bp)++ = ' ';
}

d_pn(s, l)
	register char *s;
	register long l;
{
	register int nc;
	char c;

	if (l == 0)
		*s++ = '0';
	else {
		nc = 0;
		while ((l & 0xF0000000) == 0) {
			l <<= 4;
			nc++;
		}
		for (;nc < 8;nc++) {
			c = (l & 0xF0000000) >> 28;
			if (c > 9)
				*s++ = c - 10 + 'A';
			else
				*s++ = c + '0';
			l <<= 4;
		}
	}
	*s = '\0';
}

char *
d_symname(v) {
	static char bf[80];

	sprintf(bf, "<%x>", v);
	return bf;
}

int ifd;

d_getbyte(seg, off, bt)
	char *bt;
{
	int r;
	extern int errno;

	lseek(ifd, off, 0);
	r = read(ifd, bt, 1);
	if (r == 1)
		return 1;
	if (r < 0)
		fprintf(stderr, "getbyte(%x) failed: %d\n", off, errno);
	return 0;
}

d_getmem(seg, off, bf, len)
	char *bf;
{
	int r;
	extern int errno;

	lseek(ifd, off, 0);
	r = read(ifd, bf, len);
	if (r == len)
		return 1;
	if (r < 0)
		fprintf(stderr, "getmem(%x, %x) failed: %d\n", off, len, errno);
	return 0;
}

usage(s)
	char *s;
{
	fprintf(stderr, "usage: dx [-o start_offset] [-l length] [file]\n");
	if (s)
		fprintf(stderr, "\t%s\n", s);
	exit(1);
}

int pflag = 0;

main(c, v)
	char **v;
{
	int id;

	int r;
	char *s, *fn;
	long off = 0;
	long len = -1;	/* everything */
	long strtol();
	extern int optind;
	extern char *optarg;

	while ((r = getopt(c, v, "l:o:p")) != -1)
		switch (r) {
			case 'l':
				len = strtol(optarg, &s, 16);
				if ((len == 0 && s == optarg) || *s)
					usage("argument must be integral");
				break;

			case 'o':
				off = strtol(optarg, &s, 16);
				if ((off == 0 && s == optarg) || *s)
					usage("argument must be integral");
				break;

			case 'p':
				++pflag;
				break;

			case '?':
				usage("unknown argument");
		}
	switch (c - optind) {
		case 0:
			id = 0;
			break;
		
		case 1:
			id = open(v[optind], 0);
			if (id < 0) {
				fprintf(stderr, "can't open ");
				perror(v[optind]);
				exit(2);
			}
			break;
		
		default:
			usage("only one file, please");
	}
	una(id, off, len);
	return 0;
}

/*
*	unassemble, starting at `off', for `len' bytes, or
*	everything if `len' < 0
*/
d_una(fd, off, len) {
	int boff;
	struct instr inst;

	ifd = fd;
	inst.in_seg = 0;
	inst.in_off = off;
	boff = off;
#define		DONE(ofst)	(len >= 0 && ((ofst - boff) > len))
	while (!DONE(off)) {
		if (!disi(&inst))
			break;
		if (DONE(inst.in_off))
			break;
		if (pflag)
			showinst(&inst);
		pinst(0, off, &inst, 0, 0);
		putchar('\n');
		off = inst.in_off;
	}
}

/*
*	show stuff about instruction
*/
showinst(ip)
	struct instr *ip;
{
	int i, j, k;
	struct operand *op;

	printf("instruction:\n");
	printf("\tflag\t%x\n", ip->in_flag);
	printf("\tlen\t%x\n", ip->in_len);
	printf("\topcn\t%s\n", ip->in_opcn);
	printf("\tinstr\t");
		for (i = 0;i < ip->in_len;i++)
			printf("%02X ", ip->in_buf[i]);
		printf("\n");
	printf("\tnopnd\t%d\n", ip->in_nopnd);
	for (i = 0;i < ip->in_nopnd;i++) {
		op = &ip->in_opnd[i];
		printf("\t\tflag\t%x\n", op->oa_flag);
		switch (op->oa_flag & OF_TYP) {
			case OF_REG:
				printf("\t\treg\n");
				j = 3;
				break;

			case OF_IMMED:
				printf("\t\timmed\n");
				j = 1;
				break;

			case OF_MEM:
				printf("\t\tmem\n");
				j = op->oa_flag & OFM_NX;
				break;
		}
		if (j) {
			printf("\t\t");
			for (k = 0;k < j;k++)
				printf("\t%x", op->oa_cont[k]);
			printf("\n");
		}
	}
}

#endif
