#ident	"@(#)nas:i386/inst386.h	1.13"

/*
* i386/inst386.h - i386 assembler instruction information
*
* Included only by "i386/inst386.c"
*/

#define FGEN(mnem, e0,e1,e2, isz, mn, mx, chk, gen, osz, fl, cl, gl) \
	{{{(const Uchar *)mnem, gen, fl, isz}, mn, mx, {e0,e1,e2}, osz, chk, cl, gl}}
#define GEN(mnem, e0,e1,e2, isz, mn, mx, chk, gen, osz, fl, cl, gl) \
	 FGEN(mnem, e0,e1,e2, isz, mn, mx, chk, gen, osz, fl|(mx ? IF_VARSIZE : 0), cl, gl)


#define chk_nop 0			/* nullary instructions need none */

	/*
	* Base instructions based on 80386
	* Programmer's Reference Manual.
	*/
	/* Instructions in this table, at least the first few, have been
	** ordered based on usage statistics.  The actual instruction table
	** is built lazily, and missing instructions are added by looking
	** for them linearly through this table.
	*/
/* BEGIN:  ordered part */
GEN("movl",	0x00,0x00,0x00, 2, 2, 2, chk_mov, gen_list,	4, IF_RSIZE, cl_mov, gl_mov),
GEN("pushl",	0x00,0x00,0x00, 1, 1, 1, chk_push, gen_list,	4, 0, cl_push, gl_push),
GEN("cmpl",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_cmp),
GEN("popl",	0x00,0x00,0x00, 1, 1, 1, chk_pop, gen_list, 4, 0, cl_pop, gl_pop),
GEN("call",	0x00,0x00,0x00, 2, 1, 1, chk_jmp, gen_list,	0, IF_STAR|IF_KRELOC|IF_CALL, cl_jmp, gl_call),
GEN("call0",	0x00,0x00,0x00, 2, 1, 1, chk_jmp, gen_list,	0, IF_STAR|IF_KRELOC|IF_CALL, cl_jmp, gl_call),
GEN("call1",	0x00,0x00,0x00, 2, 1, 1, chk_jmp, gen_list,	0, IF_STAR|IF_KRELOC|IF_CALL, cl_jmp, gl_call),
GEN("jmp",	0x00,0x00,0x00, 2, 1, 1, chk_jmp, gen_jmp,	0, IF_STAR, cl_jmp, gl_jmp),
GEN("addl",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_add),
GEN("andl",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_and),
GEN("imull",	0x00,0x00,0x00, 2, 1, 3, chk_imul, gen_imul, 4, IF_RSIZE, 0, gl_imul),
GEN("testl",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, IF_RSIZE, cl_test, gl_test),
GEN("xorl",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_xor),
GEN("subl",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_sub),
GEN("orl",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_or),
GEN("leal",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, 0, cl_lea, gl_lea),
/* END:  ordered part */

GEN("aaa",	0x37,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("aad",	0xD5,0x0A,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("aam",	0xD4,0x0A,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("aas",	0x3F,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("adc",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_adc),
GEN("adcl",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_adc),
GEN("adcw",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 2, IF_RSIZE, cl_ar2, gl_adc),
GEN("adcb",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 1, IF_RSIZE, cl_ar2, gl_adc),

GEN("add",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_add),
GEN("addw",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 2, IF_RSIZE, cl_ar2, gl_add),
GEN("addb",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 1, IF_RSIZE, cl_ar2, gl_add),

GEN("and",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_and),
GEN("andw",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 2, IF_RSIZE, cl_ar2, gl_and),
GEN("andb",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 1, IF_RSIZE, cl_ar2, gl_and),

GEN("arpl",	0x63,0x00,0x00, 2, 2, 2, 0, gen_list,	2, IF_RSIZE, cl_arpl, gl_arpl),

GEN("bound",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, IF_RSIZE, cl_bound, gl_bound),
GEN("boundl",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, IF_RSIZE, cl_bound, gl_bound),
GEN("boundw",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	2, IF_RSIZE, cl_bound, gl_bound),

GEN("bsf",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE, cl_bsfr, gl_bsf),
GEN("bsfl",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE, cl_bsfr, gl_bsf),
GEN("bsfw",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	2, IF_RSIZE, cl_bsfr, gl_bsf),

GEN("bsr",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE, cl_bsfr, gl_bsr),
GEN("bsrl",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE, cl_bsfr, gl_bsr),
GEN("bsrw",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	2, IF_RSIZE, cl_bsfr, gl_bsr),

GEN("bt",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_bit, gl_bt),
GEN("btl",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_bit, gl_bt),
GEN("btw",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_bit, gl_bt),

GEN("btc",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_bit, gl_btc),
GEN("btcl",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_bit, gl_btc),
GEN("btcw",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_bit, gl_btc),

GEN("btr",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_bit, gl_btr),
GEN("btrl",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_bit, gl_btr),
GEN("btrw",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_bit, gl_btr),

GEN("bts",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_bit, gl_bts),
GEN("btsl",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_bit, gl_bts),
GEN("btsw",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_bit, gl_bts),

GEN("call2",	0x00,0x00,0x00, 2, 1, 1, chk_jmp, gen_list,	0, IF_STAR|IF_KRELOC|IF_CALL, cl_jmp, gl_call),
GEN("lcall",	0x9A,0x00,0x00, 2, 1, 2, chk_ljmp, gen_ljmp,	0, IF_STAR|IF_CALL, cl_ljmp, gl_lcall),
GEN("lcall0",	0x9A,0x00,0x00, 2, 1, 2, chk_ljmp, gen_ljmp,	0, IF_STAR|IF_CALL, cl_ljmp, gl_lcall),
GEN("lcall1",	0x9A,0x00,0x00, 2, 1, 2, chk_ljmp, gen_ljmp,	0, IF_STAR|IF_CALL, cl_ljmp, gl_lcall),
GEN("lcall2",	0x9A,0x00,0x00, 2, 1, 2, chk_ljmp, gen_ljmp,	0, IF_STAR|IF_CALL, cl_ljmp, gl_lcall),

GEN("cbtw",	0x66,0x98,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("cwtl",	0x98,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("clc",	0xF8,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("cld",	0xFC,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("cli",	0xFA,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("clts",	0x0F,0x06,0x00, 2, 0, 0, chk_nop, gen_nop,	0, IF_P5_0F_PREFIX, 0, 0),
GEN("cmc",	0xF5,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("cmp",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_cmp),
GEN("cmpw",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 2, IF_RSIZE, cl_ar2, gl_cmp),
GEN("cmpb",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 1, IF_RSIZE, cl_ar2, gl_cmp),

GEN("cmps",	0xA7,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, 0, 0, 0),
GEN("cmpsl",	0xA7,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, 0, 0, 0),
GEN("scmp",	0xA7,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, 0, 0, 0),
GEN("scmpl",	0xA7,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, 0, 0, 0),
GEN("cmpsw",	0xA7,0x00,0x00, 1, 0, 1, chk_str, gen_str, 2, 0, 0, 0),
GEN("scmpw",	0xA7,0x00,0x00, 1, 0, 1, chk_str, gen_str, 2, 0, 0, 0),
GEN("cmpsb",	0xA6,0x00,0x00, 1, 0, 1, chk_str, gen_str, 1, 0, 0, 0),
GEN("scmpb",	0xA6,0x00,0x00, 1, 0, 1, chk_str, gen_str, 1, 0, 0, 0),

GEN("cwtd",	0x66,0x99,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("cltd",	0x99,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("daa",	0x27,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("das",	0x2F,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("dec",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list, 4, IF_RSIZE, cl_incdec, gl_dec),
GEN("decb",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list, 1, IF_RSIZE, cl_incdec, gl_dec),
GEN("decw",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list, 2, IF_RSIZE, cl_incdec, gl_dec),
GEN("decl",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list, 4, IF_RSIZE, cl_incdec, gl_dec),

GEN("div",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 4,	IF_RSIZE, cl_unary, gl_div),
GEN("divb",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 1,	IF_RSIZE, cl_unary, gl_div),
GEN("divw",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 2,	IF_RSIZE, cl_unary, gl_div),
GEN("divl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 4,	IF_RSIZE, cl_unary, gl_div),

FGEN("enter",	0x00,0x00,0x00, 4, 2, 2, 0, gen_ent, 0, 0, cl_enter, 0),

GEN("hlt",	0xF4,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("idiv",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 4,	IF_RSIZE, cl_unary, gl_idiv),
GEN("idivb",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 1,	IF_RSIZE, cl_unary, gl_idiv),
GEN("idivw",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 2,	IF_RSIZE, cl_unary, gl_idiv),
GEN("idivl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 4,	IF_RSIZE, cl_unary, gl_idiv),

GEN("imul",	0x00,0x00,0x00, 2, 1, 3, chk_imul, gen_imul, 4, IF_RSIZE, 0, gl_imul),
GEN("imulw",	0x00,0x00,0x00, 2, 1, 3, chk_imul, gen_imul, 2, IF_RSIZE, 0, gl_imul),
GEN("imulb",	0x00,0x00,0x00, 2, 1, 3, chk_imul, gen_imul, 1, IF_RSIZE, 0, gl_imul),

GEN("in",	0x00,0x00,0x00, 1, 1, 1, chk_inout, gen_list,	4, IF_BASE_DX, cl_inout, gl_in),
GEN("inl",	0x00,0x00,0x00, 1, 1, 1, chk_inout, gen_list,	4, IF_BASE_DX, cl_inout, gl_in),
GEN("inw",	0x00,0x00,0x00, 1, 1, 1, chk_inout, gen_list,	2, IF_BASE_DX, cl_inout, gl_in),
GEN("inb",	0x00,0x00,0x00, 1, 1, 1, chk_inout, gen_list,	1, IF_BASE_DX, cl_inout, gl_in),

GEN("inc",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list, 4, IF_RSIZE, cl_incdec, gl_inc),
GEN("incb",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list, 1, IF_RSIZE, cl_incdec, gl_inc),
GEN("incw",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list, 2, IF_RSIZE, cl_incdec, gl_inc),
GEN("incl",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list, 4, IF_RSIZE, cl_incdec, gl_inc),

GEN("ins",	0x6D,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, IF_NOSEG, 0, 0),
GEN("insl",	0x6D,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, IF_NOSEG, 0, 0),
GEN("insw",	0x6D,0x00,0x00, 1, 0, 1, chk_str, gen_str, 2, IF_NOSEG, 0, 0),
GEN("insb",	0x6C,0x00,0x00, 1, 0, 1, chk_str, gen_str, 1, IF_NOSEG, 0, 0),

GEN("int",	0x00,0x00,0x00, 1, 1, 1, chk_int, gen_list,	0, 0, cl_int, gl_int),

GEN("into",	0xCE,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("iret",	0xCF,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("jcxz",	0xE3,0x00,0x00, 2, 1, 1, chk_pcr, gen_pc8,	0, 0, 0, 0),
/* first byte of encoding is byte (short) form; long form is short + 0x10 */
GEN("ja",	0x77,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jae",	0x73,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jb",	0x72,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jbe",	0x76,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jc",	0x72,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("je",	0x74,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jg",	0x7F,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jge",	0x7D,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jl",	0x7C,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jle",	0x7E,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jna",	0x76,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jnae",	0x72,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jnb",	0x73,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jnbe",	0x77,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jnc",	0x73,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jne",	0x75,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jng",	0x7E,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jnge",	0x7C,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jnl",	0x7D,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jnle",	0x7F,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jno",	0x71,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jnp",	0x7B,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jns",	0x79,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jnz",	0x75,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jo",	0x70,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jp",	0x7A,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jpe",	0x7A,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jpo",	0x7B,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("js",	0x78,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),
GEN("jz",	0x74,0x00,0x00, 2, 1, 1, chk_pcr, gen_pcr,	0, 0, 0, 0),

GEN("ljmp",	0xEA,0x00,0x00, 2, 1, 2, chk_ljmp, gen_ljmp,	0, IF_STAR, cl_ljmp, gl_ljmp),

GEN("lahf",	0x9F,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("lar",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_lar, gl_lar),
GEN("larl",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_lar, gl_lar),
GEN("larw",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_lar, gl_lar),

GEN("lea",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, 0, cl_lea, gl_lea),
GEN("leaw",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	2, 0, cl_lea, gl_lea),

GEN("leave",	0xC9,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("lgdt",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	0, IF_P5_0F_PREFIX, cl_grp7, gl_lgdt),
GEN("lidt",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	0, IF_P5_0F_PREFIX, cl_grp7, gl_lidt),

GEN("lds",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, IF_RSIZE, cl_lseg, gl_lds),
GEN("ldsl",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, IF_RSIZE, cl_lseg, gl_lds),
GEN("ldsw",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	2, IF_RSIZE, cl_lseg, gl_lds),
GEN("lss",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_lseg, gl_lss),
GEN("lssl",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_lseg, gl_lss),
GEN("lssw",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_lseg, gl_lss),
GEN("les",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, IF_RSIZE, cl_lseg, gl_les),
GEN("lesl",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, IF_RSIZE, cl_lseg, gl_les),
GEN("lesw",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	2, IF_RSIZE, cl_lseg, gl_les),
GEN("lfs",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_lseg, gl_lfs),
GEN("lfsl",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_lseg, gl_lfs),
GEN("lfsw",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_lseg, gl_lfs),
GEN("lgs",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_lseg, gl_lgs),
GEN("lgsl",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_lseg, gl_lgs),
GEN("lgsw",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_lseg, gl_lgs),

GEN("lldt",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_grp6, gl_lldt),
GEN("lmsw",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	2, IF_P5_0F_PREFIX, cl_grp7a, gl_lmsw),

GEN("lock",	0xF0,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, IF_PREFIX_PSEUDO_OP, 0, 0),

GEN("lods",	0xAD,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, 0, 0, 0),
GEN("slod",	0xAD,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, 0, 0, 0),
GEN("lodsl",	0xAD,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, 0, 0, 0),
GEN("slodl",	0xAD,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, 0, 0, 0),
GEN("lodsw",	0xAD,0x00,0x00, 1, 0, 1, chk_str, gen_str, 2, 0, 0, 0),
GEN("slodw",	0xAD,0x00,0x00, 1, 0, 1, chk_str, gen_str, 2, 0, 0, 0),
GEN("lodsb",	0xAC,0x00,0x00, 1, 0, 1, chk_str, gen_str, 1, 0, 0, 0),
GEN("slodb",	0xAC,0x00,0x00, 1, 0, 1, chk_str, gen_str, 1, 0, 0, 0),

GEN("loop",	0xE2,0x00,0x00, 1, 1, 1, chk_pcr, gen_pc8,	0, 0, 0, 0),
GEN("loope",	0xE1,0x00,0x00, 1, 1, 1, chk_pcr, gen_pc8,	0, 0, 0, 0),
GEN("loopz",	0xE1,0x00,0x00, 1, 1, 1, chk_pcr, gen_pc8,	0, 0, 0, 0),
GEN("loopne",	0xE0,0x00,0x00, 1, 1, 1, chk_pcr, gen_pc8,	0, 0, 0, 0),
GEN("loopnz",	0xE0,0x00,0x00, 1, 1, 1, chk_pcr, gen_pc8,	0, 0, 0, 0),

GEN("lsl",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_lar, gl_lsl),
GEN("lsll",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_lar, gl_lsl),
GEN("lslw",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_lar, gl_lsl),

GEN("ltr",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_grp6, gl_ltr),

GEN("mov",	0x00,0x00,0x00, 2, 2, 2, chk_mov, gen_list,	4, IF_RSIZE, cl_mov, gl_mov),
GEN("movw",	0x00,0x00,0x00, 2, 2, 2, chk_mov, gen_list,	2, IF_RSIZE, cl_mov, gl_mov),
GEN("movb",	0x00,0x00,0x00, 2, 2, 2, chk_mov, gen_list,	1, IF_RSIZE, cl_mov, gl_mov),

GEN("movs",	0xA5,0x00,0x00, 1, 0, 1, chk_str, gen_str,	4, 0, 0, 0),
GEN("movsl",	0xA5,0x00,0x00, 1, 0, 1, chk_str, gen_str,	4, 0, 0, 0),
GEN("smov",	0xA5,0x00,0x00, 1, 0, 1, chk_str, gen_str,	4, 0, 0, 0),
GEN("smovl",	0xA5,0x00,0x00, 1, 0, 1, chk_str, gen_str,	4, 0, 0, 0),
GEN("movsw",	0xA5,0x00,0x00, 1, 0, 1, chk_str, gen_str,	2, 0, 0, 0),
GEN("smovw",	0xA5,0x00,0x00, 1, 0, 1, chk_str, gen_str,	2, 0, 0, 0),
GEN("movsb",	0xA4,0x00,0x00, 1, 0, 1, chk_str, gen_str,	1, 0, 0, 0),
GEN("smovb",	0xA4,0x00,0x00, 1, 0, 1, chk_str, gen_str,	1, 0, 0, 0),

GEN("movsbw",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	0, IF_P5_0F_PREFIX, cl_movxbw, gl_movsbw),
GEN("movswl",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	0, IF_P5_0F_PREFIX, cl_movxwl, gl_movswl),
GEN("movsbl",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	0, IF_P5_0F_PREFIX, cl_movxbl, gl_movsbl),
GEN("movzbw",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	0, IF_P5_0F_PREFIX, cl_movxbw, gl_movzbw),
GEN("movzwl",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	0, IF_P5_0F_PREFIX, cl_movxwl, gl_movzwl),
GEN("movzbl",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	0, IF_P5_0F_PREFIX, cl_movxbl, gl_movzbl),

GEN("mul",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 4,	IF_RSIZE, cl_unary, gl_mul),
GEN("mulb",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 1,	IF_RSIZE, cl_unary, gl_mul),
GEN("mulw",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 2,	IF_RSIZE, cl_unary, gl_mul),
GEN("mull",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 4,	IF_RSIZE, cl_unary, gl_mul),

GEN("neg",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 4,	IF_RSIZE, cl_unary, gl_neg),
GEN("negb",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 1,	IF_RSIZE, cl_unary, gl_neg),
GEN("negw",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 2,	IF_RSIZE, cl_unary, gl_neg),
GEN("negl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 4,	IF_RSIZE, cl_unary, gl_neg),

GEN("nop",	0x90,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("not",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 4,	IF_RSIZE, cl_unary, gl_not),
GEN("notb",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 1,	IF_RSIZE, cl_unary, gl_not),
GEN("notw",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 2,	IF_RSIZE, cl_unary, gl_not),
GEN("notl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list, 4,	IF_RSIZE, cl_unary, gl_not),

GEN("or",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_or),
GEN("orw",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 2, IF_RSIZE, cl_ar2, gl_or),
GEN("orb",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 1, IF_RSIZE, cl_ar2, gl_or),

GEN("out",	0x00,0x00,0x00, 1, 1, 1, chk_inout, gen_list,	4, IF_BASE_DX, cl_inout, gl_out),
GEN("outl",	0x00,0x00,0x00, 1, 1, 1, chk_inout, gen_list,	4, IF_BASE_DX, cl_inout, gl_out),
GEN("outw",	0x00,0x00,0x00, 1, 1, 1, chk_inout, gen_list,	2, IF_BASE_DX, cl_inout, gl_out),
GEN("outb",	0x00,0x00,0x00, 1, 1, 1, chk_inout, gen_list,	1, IF_BASE_DX, cl_inout, gl_out),

GEN("outs",	0x6F,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, 0, 0, 0),
GEN("outsl",	0x6F,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, 0, 0, 0),
GEN("outsw",	0x6F,0x00,0x00, 1, 0, 1, chk_str, gen_str, 2, 0, 0, 0),
GEN("outsb",	0x6E,0x00,0x00, 1, 0, 1, chk_str, gen_str, 1, 0, 0, 0),

GEN("pop",	0x00,0x00,0x00, 1, 1, 1, chk_pop, gen_list, 4, 0, cl_pop, gl_pop),
GEN("popw",	0x00,0x00,0x00, 1, 1, 1, chk_pop, gen_list, 2, 0, cl_pop, gl_pop),

GEN("popa",	0x61,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("popal",	0x61,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("popaw",	0x66,0x61,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("popf",	0x9D,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("popfl",	0x9D,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("popfw",	0x66,0x9D,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("push",	0x00,0x00,0x00, 1, 1, 1, chk_push, gen_list,	4, 0, cl_push, gl_push),
GEN("pushw",	0x00,0x00,0x00, 1, 1, 1, chk_push, gen_list,	2, 0, cl_push, gl_push),

GEN("pusha",	0x60,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("pushal",	0x60,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("pushaw",	0x66,0x60,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("pushf",	0x9C,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("pushfl",	0x9C,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("pushfw",	0x66,0x9C,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("rcl",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_rcl),
GEN("rcll",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_rcl),
GEN("rclw",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	2, 0, cl_shift, gl_rcl),
GEN("rclb",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	1, 0, cl_shift, gl_rcl),
GEN("rcr",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_rcr),
GEN("rcrl",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_rcr),
GEN("rcrw",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	2, 0, cl_shift, gl_rcr),
GEN("rcrb",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	1, 0, cl_shift, gl_rcr),
GEN("rol",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_rol),
GEN("roll",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_rol),
GEN("rolw",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	2, 0, cl_shift, gl_rol),
GEN("rolb",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	1, 0, cl_shift, gl_rol),
GEN("ror",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_ror),
GEN("rorl",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_ror),
GEN("rorw",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	2, 0, cl_shift, gl_ror),
GEN("rorb",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	1, 0, cl_shift, gl_ror),

GEN("rep",	0xF3,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, IF_PREFIX_PSEUDO_OP, 0, 0),
GEN("repe",	0xF3,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, IF_PREFIX_PSEUDO_OP, 0, 0),
GEN("repz",	0xF3,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, IF_PREFIX_PSEUDO_OP, 0, 0),
GEN("repne",	0xF2,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, IF_PREFIX_PSEUDO_OP, 0, 0),
GEN("repnz",	0xF2,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, IF_PREFIX_PSEUDO_OP, 0, 0),

GEN("ret",	0xC3,0x00,0x00, 1, 0, 1, chk_ret, gen_ret,	0, 0, 0, 0),
GEN("lret",	0xCB,0x00,0x00, 1, 0, 1, chk_ret, gen_ret,	0, 0, 0, 0),

GEN("sahf",	0x9E,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("sal",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_sal),
GEN("sall",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_sal),
GEN("salw",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	2, 0, cl_shift, gl_sal),
GEN("salb",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	1, 0, cl_shift, gl_sal),
GEN("sar",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_sar),
GEN("sarl",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_sar),
GEN("sarw",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	2, 0, cl_shift, gl_sar),
GEN("sarb",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	1, 0, cl_shift, gl_sar),
GEN("shl",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_shl),
GEN("shll",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_shl),
GEN("shlw",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	2, 0, cl_shift, gl_shl),
GEN("shlb",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	1, 0, cl_shift, gl_shl),
GEN("shr",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_shr),
GEN("shrl",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	4, 0, cl_shift, gl_shr),
GEN("shrw",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	2, 0, cl_shift, gl_shr),
GEN("shrb",	0x00,0x00,0x00, 2, 1, 2, chk_shift, gen_list,	1, 0, cl_shift, gl_shr),

GEN("sbb",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_sbb),
GEN("sbbl",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_sbb),
GEN("sbbw",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 2, IF_RSIZE, cl_ar2, gl_sbb),
GEN("sbbb",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 1, IF_RSIZE, cl_ar2, gl_sbb),

GEN("scas",	0xAF,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, IF_NOSEG, 0, 0),
GEN("scasl",	0xAF,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, IF_NOSEG, 0, 0),
GEN("scal",	0xAF,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, IF_NOSEG, 0, 0),
GEN("ssca",	0xAF,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, IF_NOSEG, 0, 0),
GEN("sscal",	0xAF,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, IF_NOSEG, 0, 0),
GEN("scasw",	0xAF,0x00,0x00, 1, 0, 1, chk_str, gen_str, 2, IF_NOSEG, 0, 0),
GEN("scaw",	0xAF,0x00,0x00, 1, 0, 1, chk_str, gen_str, 2, IF_NOSEG, 0, 0),
GEN("sscaw",	0xAF,0x00,0x00, 1, 0, 1, chk_str, gen_str, 2, IF_NOSEG, 0, 0),
GEN("scasb",	0xAE,0x00,0x00, 1, 0, 1, chk_str, gen_str, 1, IF_NOSEG, 0, 0),
GEN("scab",	0xAE,0x00,0x00, 1, 0, 1, chk_str, gen_str, 1, IF_NOSEG, 0, 0),
GEN("sscab",	0xAE,0x00,0x00, 1, 0, 1, chk_str, gen_str, 1, IF_NOSEG, 0, 0),

GEN("seta",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_seta),
GEN("setae",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_setnc),
GEN("setb",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_setc),
GEN("setbe",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_setna),
GEN("setc",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_setc),
GEN("sete",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_setz),
GEN("setg",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc1, gl_setg),
GEN("setge",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc1, gl_setnl),
GEN("setl",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc1, gl_setl),
GEN("setle",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc1, gl_setng),
GEN("setna",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_setna),
GEN("setnae",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_setc),
GEN("setnb",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_setnc),
GEN("setnbe",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_seta),
GEN("setnc",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_setnc),
GEN("setne",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_setnz),
GEN("setng",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc1, gl_setng),
GEN("setnl",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc1, gl_setnl),
GEN("setnle",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc1, gl_setg),
GEN("setno",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_setno),
GEN("setnp",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc1, gl_setnp),
GEN("setns",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc1, gl_setns),
GEN("setnz",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_setnz),
GEN("seto",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_seto),
GEN("setp",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc1, gl_setp),
GEN("setpe",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc1, gl_setp),
GEN("setpo",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc1, gl_setnp),
GEN("sets",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc1, gl_sets),
GEN("setz",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list, 1, IF_RSIZE|IF_P5_0F_PREFIX, cl_setcc0, gl_setz),

GEN("sgdt",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	0, IF_P5_0F_PREFIX, cl_grp7, gl_sgdt),
GEN("sidt",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	0, IF_P5_0F_PREFIX, cl_grp7, gl_sidt),

GEN("shld",	0x00,0x00,0x00, 3, 2, 3, chk_shxd, gen_shxd, 4, IF_RSIZE|IF_P5_0F_PREFIX, 0, gl_shld),
GEN("shldl",	0x00,0x00,0x00, 3, 2, 3, chk_shxd, gen_shxd, 4, IF_RSIZE|IF_P5_0F_PREFIX, 0, gl_shld),
GEN("shldw",	0x00,0x00,0x00, 3, 2, 3, chk_shxd, gen_shxd, 2, IF_RSIZE|IF_P5_0F_PREFIX, 0, gl_shld),
GEN("shrd",	0x00,0x00,0x00, 3, 2, 3, chk_shxd, gen_shxd, 4, IF_RSIZE|IF_P5_0F_PREFIX, 0, gl_shrd),
GEN("shrdl",	0x00,0x00,0x00, 3, 2, 3, chk_shxd, gen_shxd, 4, IF_RSIZE|IF_P5_0F_PREFIX, 0, gl_shrd),
GEN("shrdw",	0x00,0x00,0x00, 3, 2, 3, chk_shxd, gen_shxd, 2, IF_RSIZE|IF_P5_0F_PREFIX, 0, gl_shrd),

GEN("sldt",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_grp6, gl_sldt),
GEN("smsw",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	2, IF_P5_0F_PREFIX, cl_grp7a, gl_smsw),

GEN("stc",	0xF9,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("std",	0xFD,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("sti",	0xFB,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("stos",	0xAB,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, IF_NOSEG, 0, 0),
GEN("stosl",	0xAB,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, IF_NOSEG, 0, 0),
GEN("ssto",	0xAB,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, IF_NOSEG, 0, 0),
GEN("sstol",	0xAB,0x00,0x00, 1, 0, 1, chk_str, gen_str, 4, IF_NOSEG, 0, 0),
GEN("stosw",	0xAB,0x00,0x00, 1, 0, 1, chk_str, gen_str, 2, IF_NOSEG, 0, 0),
GEN("sstow",	0xAB,0x00,0x00, 1, 0, 1, chk_str, gen_str, 2, IF_NOSEG, 0, 0),
GEN("stosb",	0xAA,0x00,0x00, 1, 0, 1, chk_str, gen_str, 1, IF_NOSEG, 0, 0),
GEN("sstob",	0xAA,0x00,0x00, 1, 0, 1, chk_str, gen_str, 1, IF_NOSEG, 0, 0),

GEN("str",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_grp6, gl_str),

GEN("sub",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_sub),
GEN("subw",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 2, IF_RSIZE, cl_ar2, gl_sub),
GEN("subb",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 1, IF_RSIZE, cl_ar2, gl_sub),

GEN("test",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	4, IF_RSIZE, cl_test, gl_test),
GEN("testw",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	2, IF_RSIZE, cl_test, gl_test),
GEN("testb",	0x00,0x00,0x00, 2, 2, 2, 0, gen_list,	1, IF_RSIZE, cl_test, gl_test),

GEN("verr",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_grp6, gl_verr),
GEN("verw",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_grp6, gl_verw),

GEN("wait",	0x9B,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("xchg",	0x00,0x00,0x00, 1, 2, 2, 0, gen_list,	4, IF_RSIZE, cl_xchg, gl_xchg),
GEN("xchgl",	0x00,0x00,0x00, 1, 2, 2, 0, gen_list,	4, IF_RSIZE, cl_xchg, gl_xchg),
GEN("xchgw",	0x00,0x00,0x00, 1, 2, 2, 0, gen_list,	2, IF_RSIZE, cl_xchg, gl_xchg),
GEN("xchgb",	0x00,0x00,0x00, 1, 2, 2, 0, gen_list,	1, IF_RSIZE, cl_xchg, gl_xchg),

GEN("xlat",	0xD7,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("xor",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 4, IF_RSIZE, cl_ar2, gl_xor),
GEN("xorw",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 2, IF_RSIZE, cl_ar2, gl_xor),
GEN("xorb",	0x00,0x00,0x00, 2, 2, 2, chk_ar2, gen_list, 1, IF_RSIZE, cl_ar2, gl_xor),

	/*
	* Pseudo instructions.
	*/
GEN("addr16",	0x67,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, IF_PREFIX_PSEUDO_OP, 0, 0),
GEN("data16",	0x66,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, IF_PREFIX_PSEUDO_OP, 0, 0),

GEN("clr",	0x00,0x00,0x00, 2, 1, 1, 0, gen_clr,	4, 0, cl_clr, gl_clr),
GEN("clrl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_clr,	4, 0, cl_clr, gl_clr),
GEN("clrw",	0x00,0x00,0x00, 2, 1, 1, 0, gen_clr,	2, 0, cl_clr, gl_clr),
GEN("clrb",	0x00,0x00,0x00, 2, 1, 1, 0, gen_clr,	1, 0, cl_clr, gl_clr),

GEN("esc",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	2, IF_RSIZE, cl_esc, gl_esc),

	/*
	* 80387 Instructions.
	*/
GEN("fld",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fld0, gl_fld),
GEN("flds",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fld1, gl_flds),
GEN("fldl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fld1, gl_fldl),
GEN("fldt",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fld1, gl_fldt),

GEN("fst",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fld0, gl_fst),
GEN("fsts",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fst1, gl_fsts),
GEN("fstl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fst1, gl_fstl),
GEN("fstp",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fld0, gl_fstp),
GEN("fstps",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fstp1, gl_fstps),
GEN("fstpl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fstp1, gl_fstpl),
GEN("fstpt",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fstp1, gl_fstpt),

GEN("fxch",	0x00,0x00,0x00, 2, 0, 1, chk_fxch, gen_fxch,	0, 0, 0, gl_fxch),

GEN("fild",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fld1, gl_fild),
GEN("fildl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fld1, gl_fildl),
GEN("fildll",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fld1, gl_fildll),

GEN("fist",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fst1, gl_fist),
GEN("fistl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fst1, gl_fistl),
GEN("fistp",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fstp1, gl_fistp),
GEN("fistpl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fstp1, gl_fistpl),
GEN("fistpll",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fstp1, gl_fistpll),

GEN("fbld",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fld1, gl_fbld),

GEN("fbstp",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fstp1, gl_fbstp),

GEN("fadd",	0xde,0xc1,0x00, 2, 0, 2, 0, gen_optnop,	8, IF_RSIZE, cl_fop, gl_fadd),
GEN("fadds",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_faddmul, gl_fadds),
GEN("faddl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_faddmul, gl_faddl),
GEN("faddp",	0xde,0xc1,0x00, 2, 0, 2, 0, gen_optnop,	8, IF_RSIZE, cl_fopp, gl_faddp),
GEN("fiadd",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list,	0, 0, cl_faddmul, gl_fiadd),
GEN("fiaddl",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list,	0, 0, cl_faddmul, gl_fiaddl),

GEN("fsub",	0xde,0xe1,0x00, 2, 0, 2, 0, gen_optnop,	8, IF_RSIZE, cl_fop, gl_fsub),
GEN("fsubs",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fsub, gl_fsubs),
GEN("fsubl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fsub, gl_fsubl),
GEN("fsubp",	0xde,0xe1,0x00, 2, 0, 2, 0, gen_optnop,	8, IF_RSIZE, cl_fopp, gl_fsubp),
GEN("fisub",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list,	0, 0, cl_fsub, gl_fisub),
GEN("fisubl",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list,	0, 0, cl_fsub, gl_fisubl),

GEN("fsubr",	0xde,0xe9,0x00, 2, 0, 2, 0, gen_optnop,	8, IF_RSIZE, cl_fop, gl_fsubr),
GEN("fsubrs",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fsub, gl_fsubrs),
GEN("fsubrl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fsub, gl_fsubrl),
GEN("fsubrp",	0xde,0xe9,0x00, 2, 0, 2, 0, gen_optnop,	8, IF_RSIZE, cl_fopp, gl_fsubrp),
GEN("fisubr",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list,	0, 0, cl_fsub, gl_fisubr),
GEN("fisubrl",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list,	0, 0, cl_fsub, gl_fisubrl),

GEN("fmul",	0xde,0xc9,0x00, 2, 0, 2, 0, gen_optnop,	8, IF_RSIZE, cl_fop, gl_fmul),
GEN("fmuls",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_faddmul, gl_fmuls),
GEN("fmull",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_faddmul, gl_fmull),
GEN("fmulp",	0xde,0xc9,0x00, 2, 0, 2, 0, gen_optnop,	8, IF_RSIZE, cl_fopp, gl_fmulp),
GEN("fimul",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list,	0, 0, cl_faddmul, gl_fimul),
GEN("fimull",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list,	0, 0, cl_faddmul, gl_fimull),

GEN("fdiv",	0xde,0xf1,0x00, 2, 0, 2, 0, gen_optnop,	8, IF_RSIZE, cl_fop, gl_fdiv),
GEN("fdivs",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fdiv, gl_fdivs),
GEN("fdivl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fdiv, gl_fdivl),
GEN("fdivp",	0xde,0xf1,0x00, 2, 0, 2, 0, gen_optnop,	8, IF_RSIZE, cl_fopp, gl_fdivp),
GEN("fidiv",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list,	0, 0, cl_fdiv, gl_fidiv),
GEN("fidivl",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list,	0, 0, cl_fdiv, gl_fidivl),

GEN("fdivr",	0xde,0xf9,0x00, 2, 0, 2, 0, gen_optnop,	8, IF_RSIZE, cl_fop, gl_fdivr),
GEN("fdivrs",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fdiv, gl_fdivrs),
GEN("fdivrl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fdiv, gl_fdivrl),
GEN("fdivrp",	0xde,0xf9,0x00, 2, 0, 2, 0, gen_optnop,	8, IF_RSIZE, cl_fopp, gl_fdivrp),
GEN("fidivr",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list,	0, 0, cl_fdiv, gl_fidivr),
GEN("fidivrl",	0x00,0x00,0x00, 1, 1, 1, 0, gen_list,	0, 0, cl_fdiv, gl_fidivrl),

GEN("fsqrt",	0xD9,0xFA,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fscale",	0xD9,0xFD,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fprem",	0xD9,0xF8,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fprem1",	0xD9,0xF5,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("frndint",	0xD9,0xFC,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fxtract",	0xD9,0xF4,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fabs",	0xD9,0xE1,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fchs",	0xD9,0xE0,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("fcom",	0xD8,0xD1,0x00, 2, 0, 1, 0, gen_optnop,	0, 0, cl_fcom0, gl_fcom),
GEN("fcoms",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fcom, gl_fcoms),
GEN("fcoml",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fcom, gl_fcoml),

GEN("fcomp",	0xD8,0xD9,0x00, 2, 0, 1, 0, gen_optnop,	0, 0, cl_fcom0, gl_fcomp),
GEN("fcomps",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fcom, gl_fcomps),
GEN("fcompl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fcom, gl_fcompl),

GEN("fcompp",	0xDE,0xD9,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("ficom",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fcom, gl_ficom),
GEN("ficoml",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fcom, gl_ficoml),
GEN("ficomp",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fcom, gl_ficomp),
GEN("ficompl",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fcom, gl_ficompl),

GEN("ftst",	0xD9,0xE4,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("fucom",	0xDD,0xE1,0x00, 2, 0, 1, 0, gen_optnop,	0, 0, cl_fcom0, gl_fucom),

GEN("fucomp",	0xDD,0xE9,0x00, 2, 0, 1, 0, gen_optnop,	0, 0, cl_fcom0, gl_fucomp),

GEN("fucompp",	0xDA,0xE9,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("fxam",	0xD9,0xE5,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("fcos",	0xD9,0xFF,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fsin",	0xD9,0xFE,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fsincos",	0xD9,0xFB,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("fptan",	0xD9,0xF2,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fpatan",	0xD9,0xF3,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("f2xm1",	0xD9,0xF0,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fyl2x",	0xD9,0xF1,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fyl2xp1",	0xD9,0xF9,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("fldz",	0xD9,0xEE,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fld1",	0xD9,0xE8,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fldpi",	0xD9,0xEB,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fldl2t",	0xD9,0xE9,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fldl2e",	0xD9,0xEA,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fldlg2",	0xD9,0xEC,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fldln2",	0xD9,0xED,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("finit",	0x9B,0xDB,0xE3, 3, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fninit",	0xDB,0xE3,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("fldcw",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fcntl, gl_fldcw),
GEN("fstcw",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	0, 0, cl_fcntl, gl_fstcw),
GEN("fnstcw",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fcntl, gl_fnstcw),

GEN("fstsw",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	0, 0, cl_fstsw, gl_fstsw),
GEN("fnstsw",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fstsw, gl_fnstsw),

GEN("fclex",	0x9B,0xDB,0xE2, 3, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fnclex",	0xDB,0xE2,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("fsave",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	0, 0, cl_fsave, gl_fsave),
GEN("fnsave",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fsave, gl_fnsave),
GEN("frstor",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fsave, gl_frstor),

GEN("fldenv",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fcntl, gl_fldenv),
GEN("fstenv",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	0, 0, cl_fcntl, gl_fstenv),
GEN("fnstenv",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fcntl, gl_fnstenv),

GEN("fincstp",	0xD9,0xF7,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fdecstp",	0xD9,0xF6,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("ffree",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	0, 0, cl_fld0, gl_ffree),

GEN("fnop",	0xD9,0xD0,0x00, 2, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

GEN("fsetpm",	0x9B,0xDB,0xE4, 3, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),
GEN("fwait",	0x9B,0x00,0x00, 1, 0, 0, chk_nop, gen_nop,	0, 0, 0, 0),

/* i486 new instructions */
GEN("bswap",	0x00,0x00,0x00, 2, 1, 1, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_bswap, gl_bswap),

GEN("cmpxchg",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_xadd, gl_cmpxchg),
GEN("cmpxchgl",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_xadd, gl_cmpxchg),
GEN("cmpxchgw",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_xadd, gl_cmpxchg),
GEN("cmpxchgb",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	1, IF_RSIZE|IF_P5_0F_PREFIX, cl_xadd, gl_cmpxchg),

GEN("xadd",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_xadd, gl_xadd),
GEN("xaddl",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	4, IF_RSIZE|IF_P5_0F_PREFIX, cl_xadd, gl_xadd),
GEN("xaddw",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	2, IF_RSIZE|IF_P5_0F_PREFIX, cl_xadd, gl_xadd),
GEN("xaddb",	0x00,0x00,0x00, 3, 2, 2, 0, gen_list,	1, IF_RSIZE|IF_P5_0F_PREFIX, cl_xadd, gl_xadd),

GEN("invd",	0x0F,0x08,0x00, 2, 0, 0, 0, gen_nop,	0, IF_P5_0F_PREFIX, 0, 0),
GEN("wbinvd",	0x0F,0x09,0x00, 2, 0, 0, 0, gen_nop,	0, IF_P5_0F_PREFIX, 0, 0),

GEN("invlpg",	0x00,0x00,0x00, 3, 1, 1, 0, gen_list,	0, IF_P5_0F_PREFIX, cl_invlpg, gl_invlpg),

/* P5 new instructions */
GEN("cmpxchg8b", 0x0,0x00,0x00, 3, 1, 1, 0, gen_list,	0, IF_P5_0F_PREFIX, cl_cmpxchg8b, gl_cmpxchg8b),
GEN("wrmsr",	0x0F,0x30,0x00, 2, 0, 0, 0, gen_nop,	0, IF_P5_0F_PREFIX, 0, 0),
GEN("rdtsc",	0x0F,0x31,0x00, 2, 0, 0, 0, gen_nop,	0, IF_P5_0F_PREFIX, 0, 0),
GEN("rdmsr",	0x0F,0x32,0x00, 2, 0, 0, 0, gen_nop,	0, IF_P5_0F_PREFIX, 0, 0),
GEN("cpuid",	0x0F,0xA2,0x00, 2, 0, 0, 0, gen_nop,	0, IF_P5_0F_PREFIX, 0, 0),
GEN("rsm",	0x0F,0xAA,0x00, 2, 0, 0, 0, gen_nop,	0, IF_P5_0F_PREFIX, 0, 0),

/* P6 new instructions */

GEN("UD2",	0x0F,0x0B,0x00, 2, 0, 0, 0, gen_nop,	0, IF_P5_0F_PREFIX, 0, 0),
GEN("rdpmc",	0x0F,0x33,0x00, 2, 0, 0, 0, gen_nop,	0, IF_P5_0F_PREFIX, 0, 0),
GEN("sysenter",	0x0F,0x34,0x00, 2, 0, 0, 0, gen_nop,	0, IF_P5_0F_PREFIX, 0, 0),
GEN("sysexit",	0x0F,0x35,0x00, 2, 0, 0, 0, gen_nop,	0, IF_P5_0F_PREFIX, 0, 0),

GEN("cmovaw",	0x0F,0x47,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovaew",	0x0F,0x43,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovbw",	0x0F,0x42,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovbew",	0x0F,0x46,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovcw",	0x0F,0x42,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovew",	0x0F,0x44,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovgw",	0x0F,0x4F,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovgew",	0x0F,0x4D,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovlw",	0x0F,0x4C,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovlew",	0x0F,0x4E,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovnaw",	0x0F,0x46,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovnaew",	0x0F,0x42,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovnbw",	0x0F,0x43,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovnbew",	0x0F,0x47,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovncw",	0x0F,0x43,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovnew",	0x0F,0x45,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovngw",	0x0F,0x4E,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovngew",	0x0F,0x4C,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovnlw",	0x0F,0x4D,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovnlew",	0x0F,0x4F,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovnow",	0x0F,0x41,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovnpw",	0x0F,0x4B,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovnsw",	0x0F,0x49,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovnzw",	0x0F,0x45,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovow",	0x0F,0x40,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovpw",	0x0F,0x4A,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovpew",	0x0F,0x4A,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovpow",	0x0F,0x4B,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovsw",	0x0F,0x48,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),
GEN("cmovzw",	0x0F,0x44,0x00, 3, 2, 2, 0, gen_cmov,2, IF_RSIZE, cl_cmov, 0),

GEN("cmoval",	0x0F,0x47,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovael",	0x0F,0x43,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovbl",	0x0F,0x42,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovbel",	0x0F,0x46,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovcl",	0x0F,0x42,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovel",	0x0F,0x44,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovgl",	0x0F,0x4F,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovgel",	0x0F,0x4D,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovll",	0x0F,0x4C,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovlel",	0x0F,0x4E,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovnal",	0x0F,0x46,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovnael",	0x0F,0x42,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovnbl",	0x0F,0x43,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovnbel",	0x0F,0x47,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovncl",	0x0F,0x43,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovnel",	0x0F,0x45,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovngl",	0x0F,0x4E,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovngel",	0x0F,0x4C,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovnll",	0x0F,0x4D,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovnlel",	0x0F,0x4F,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovnol",	0x0F,0x41,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovnpl",	0x0F,0x4B,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovnsl",	0x0F,0x49,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovnzl",	0x0F,0x45,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovol",	0x0F,0x40,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovpl",	0x0F,0x4A,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovpel",	0x0F,0x4A,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovpol",	0x0F,0x4B,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovsl",	0x0F,0x48,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),
GEN("cmovzl",	0x0F,0x44,0x00, 3, 2, 2, 0, gen_cmov,4, IF_RSIZE, cl_cmov, 0),

GEN("fcomi",	0xDB,0xF1,0x00, 2, 0, 1, 0, gen_optnop,	0, 0, cl_fcomi0, gl_fcomi),
GEN("fcompi",	0xDF,0xF1,0x00, 2, 0, 1, 0, gen_optnop,	0, 0, cl_fcomi0, gl_fcompi),
GEN("fucomi",	0xDB,0xE9,0x00, 2, 0, 1, 0, gen_optnop,	0, 0, cl_fcomi0, gl_fucomi),
GEN("fucompi",	0xDF,0xE9,0x00, 2, 0, 1, 0, gen_optnop,	0, 0, cl_fcomi0, gl_fucompi),

GEN("fcmovb",	0xDA,0xC0,0x00, 2, 2, 2, 0, gen_list,   0, 0, cl_fcmov, gl_fcmovb),
GEN("fcmove",	0xDA,0xC8,0x00, 2, 2, 2, 0, gen_list,   0, 0, cl_fcmov, gl_fcmove),
GEN("fcmovbe",	0xDA,0xD0,0x00, 2, 2, 2, 0, gen_list,   0, 0, cl_fcmov, gl_fcmovbe),
GEN("fcmovu",	0xDA,0xD8,0x00, 2, 2, 2, 0, gen_list,   0, 0, cl_fcmov, gl_fcmovu),
GEN("fcmovnb",	0xDB,0xC0,0x00, 2, 2, 2, 0, gen_list,   0, 0, cl_fcmov, gl_fcmovnb),
GEN("fcmovne",	0xDB,0xC8,0x00, 2, 2, 2, 0, gen_list,   0, 0, cl_fcmov, gl_fcmovne),
GEN("fcmovnbe",	0xDB,0xD0,0x00, 2, 2, 2, 0, gen_list,   0, 0, cl_fcmov, gl_fcmovnbe),
GEN("fcmovnu",	0xDB,0xD8,0x00, 2, 2, 2, 0, gen_list,   0, 0, cl_fcmov, gl_fcmovnu),
