#ident	"@(#)fur:common/cmd/fur/fur.h	1.3.2.8"
#ident	"$Header:"

#ifndef _FUR_H
#define _FUR_H

#include <sys/types.h>
#include <libelf.h>
#include <sys/elf_386.h>
#ifndef NOT_INTEMU
#include "intemu.h"
#endif

#ifdef __STDC__
void error(const char *, ...);
Elf_Data * myelf_getdata(Elf_Scn *, Elf_Data *, const char *);
Elf32_Sym *findsymbyoff(Elf32_Addr);
long symchg(Elf32_Addr off, int text_ndx, Elf32_Sym *firstsym, Elf32_Sym *end);
Elf32_Sym *findsym(char *, Elf32_Sym *, Elf32_Sym *, int, char *);
struct text_info *gettextinfo(Elf32_Sym *, char *, char *);
void updaterels(Elf_Data *, Elf_Data *, int, int);
void chktextrels(Elf_Data *, Elf_Data *, int, int, int, int);
#else
void error();
Elf32_Sym *findsym();
Elf32_Sym *findsymbyoff();
long symchg();
struct text_info *gettextinfo();
void usage();
Elf_Data *myelf_getdata();
void updaterels();
void chktextrels();
#endif

/* keeps track of functions as the are moved */
struct text_info {
	Elf32_Addr ti_curaddr;		/* symtab input address */
	Elf32_Addr ti_newaddr;		/* symtab output address */
	Elf32_Addr ti_usesize;		/* size to use for function*/
	char *ti_filename;
	char *ti_data;			/* input buffer ptr to function body */
};

/* structure to keep track of sections in file */
struct section {
	Elf_Scn *sec_scn;		/* scn pointer */
	Elf32_Shdr *sec_shdr;	/* section header */
	Elf_Data *sec_data;		/* data associated with section */
	int foundname;			/* used in cleanup_symbol_table */
};

extern struct section *esections;

struct flowcount {
	unsigned long callcount;
	unsigned long firstcount;
	unsigned long secondcount;
};
struct blockcount {
	unsigned long callcount;
	unsigned long firstblock;
	unsigned long firstcount;
	unsigned long secondcount;
};

#ifdef DECLARE_GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

struct codeblock {
	Elf_Data *relsec;
	Elf_Data *textsec;
	Elf_Data *symtab;
	Elf_Data *strtab;
	struct codeblock *next;
/*	int symtab_offset;*/
/*	int strtab_offset;*/
/*	int blockno_symno;*/
/*	int funcno_symno;*/
};

#define EH_TRYBLOCK -2
#define EH_FUNCTION -1
struct eheither {
	Elf32_Addr addr;
	Elf32_Addr len;
	ulong type;
	ulong fill3;
	ulong fill4;
	ulong fill5;
	ulong fill6;
};

struct ehfunc_entry {
	Elf32_Addr addr;
	Elf32_Addr len;
	ulong type;
	ulong fill3;
	ulong fill4;
	ulong fill5;
	ulong rangetable;
};

struct ehtry {
	Elf32_Addr start_addr;
	Elf32_Addr len;
	ulong type;
	ulong fill3;
	ulong fill4;
	ulong fill5;
	ulong fill6;
};

struct ehregion {
	Elf32_Addr offset;
	Elf32_Addr num;
};

extern long loc;
extern char *p_data;

Elf32_Shdr Shdr;
EXTERN Elf32_Addr *Nops;
EXTERN int Nnops;

EXTERN int Sym_index;
EXTERN int Text_sym;
EXTERN int Keeprebuild;
EXTERN char *Keepblocks;
EXTERN int Found_pseudo;

#define pop_addr() (Nstack ? Stack[--Nstack] : NO_SUCH_ADDR)

EXTERN uint_t *Starts;
EXTERN int Nstack, Sstack;
EXTERN Elf32_Addr *Stack;

EXTERN Elf32_Addr *Data;
EXTERN int Ndata;

EXTERN int Pass2;
EXTERN int Text_ready;
EXTERN int Allprologues;
EXTERN int Allepilogues;
EXTERN int Allblocks;
EXTERN int Flowblocks;
EXTERN char *Someprologues;
EXTERN char *Someepilogues;
EXTERN char *Someblocks;
EXTERN struct codeblock Prologue;
EXTERN struct codeblock Epilogue;
EXTERN struct codeblock Perblock;

/* A block is made up of 5 (easy) pieces:
	preprocess code (user added)
	the original code to the block (minus, possibly, the final instruction)
	postprocess code (user added)
	the final instruction (if it is a jump, call or ret instruction)
	a corrective jump (if the code is moved to a point that warrants such)
*/

#define LONGCODE_PREFIX	0x0f

#define JMPCODE 0xeb
#define LONG_JMPCODE 0xe9

#define JO	0x0
#define JNO	0x1
#define JB	0x2
#define JAE	0x3
#define JE	0x4
#define JNE	0x5
#define JBE	0x6
#define JA	0x7
#define JS	0x8
#define JNS	0x9
#define JPE	0xA
#define JPO	0xB
#define JL	0xC
#define JGE	0xD
#define JNG	0xE
#define JG	0xF

#define REVERSE(CODE) (CODE ^= 0x1)

#define isret(INST, INST2) (((INST) == 0xc3) || ((INST) == 0xc2) || ((INST) == 0xca) || ((INST) == 0xcb))
#define isnearret(INST, INST2) (((INST) == 0xc3) || ((INST) == 0xc2))
#define isscjump(INST, INST2) (((INST) >> 4) == 7)
#define islcjump(INST, INST2) (((INST) == 0xf) && (((INST2) >> 4) == 8))
#define iscjump(INST, INST2) (isscjump(INST, INST2) || islcjump(INST, INST2))
#define isljump(INST, INST2) ((INST) == 0xe9)
#define issjump(INST, INST2) ((INST) == 0xeb)
#define isjump(INST, INST2) (isljump(INST, INST2) || issjump(INST, INST2))

#define islcall(INST, INST2) ((INST) == 0xe8)
#define isscall(INST, INST2) (0)
#define iscall(INST, INST2) (islcall(INST, INST2) || isscall(INST, INST2))

#define isimmpush(INST, INST2) ((INST) == 0x68)
#define TESTPOP(INST, INST2, ADDR) isimmpop(INST, INST2, GET1(Text_data->d_buf, (Elf32_Addr) (ADDR) + 2))
#define isimmpop(INST, INST2, INST3) (((INST) == 0x83) && ((INST2) == 0xc4) && ((INST3) == 0x4))
#define stripop(INST) (((INST) >> 3) & 0x7)
#define isijump(INST, INST2) (((INST) == 0xff) && ((stripop(INST2) == 4) || (stripop(INST2) == 5)))
#define isicall(INST, INST2) (((INST) == 0xff) && ((stripop(INST2) == 2) || (stripop(INST2) == 3)))
#define iscallgate(INST, INST2) ((INST) == 0x9a)

#define islonginst(INST, INST2) (islcall(INST, INST2) || isljump(INST, INST2)|| islcjump(INST, INST2))
#define issub(INST, INST2) (((INST) == 0x2d) || (((INST) == 0x81) && (stripop(INST2) == 5)))

/*unchar Reverse[] = { JNO, JO, JAE, JB, JNE, JE, JA, JBE, JNS, JS, JPO, JPE, JGE, JL, JG, JNG };*/

/* fur goes through multiple phases and the flag numbers can be reused
** if the usage of flags doesn't overlap.  The phases are:
	decode
	insertion preparation
	ordering
	code generation

	It is the responsibility of the code reusing a flag number to clear
	it before reusing.  For example, BEENHERE should always be cleared
	before use
*/

/* Flags that are used from decode on */
#define FUNCTION_START			(1<<0)
#define START_NOP				(1<<1)
#define ENTRY_POINT				(1<<2)
#define DATA_BLOCK				(1<<3)

/* Flags that are used in decode only */
#define ARITH_ON_ADDRESS		(1<<4)

#define DECODE_STABLE_FLAGS		(FUNCTION_START|START_NOP|ENTRY_POINT|DATA_BLOCK)
/* Flags that are used from order establishment on */
#define LOOP_HEADER				(1<<4)
#define LOW_USAGE				(1<<5)
#define INLINE_CALL				(1<<6)
#define GEN_TEXT				(1<<7)

#define ORDER_STABLE_FLAGS		(DECODE_STABLE_FLAGS|LOOP_HEADER|LOW_USAGE|INLINE_CALL|GEN_TEXT)

/* Flags that are used in order establishment only */
#define PLACED					(1<<8)
#define PLACED_THIS_TRACE		(1<<9)
#define REDUCIBLE_OKAY			(1<<10)
#define IN_CYCLE				(1<<11)
#define HAS_EFFECT_FLAG			(1<<12)
#define FLOW_BLOCK				(1<<13)

/* Flags that are used in inlining analysis */
#define BEENHERE_ON_PATH		(1<<8)
#define BEENHERE_NCALLS			(1<<9)
#define GEN_THIS_BLOCK			(1<<10)

/* Flags that are used in code generation*/
#define REVERSE_END_CJUMP		(1<<8)
#define CORRECTIVE_LONG_JUMP	(1<<9)
#define JUMP_GROWN				(1<<10)
#define JUMP_SHRUNK				(1<<11)
#define GROUPED					(1<<12)

/* Flag always left free for use */
#define BEENHERE				(1<<15)

/*
** The next four structures logically form one structure.  However,
** on different executions of fur, only certain members of the structure
** are necessary.  For example, if statistics are not available, the
** frequency members are always zero.  Therefore, the structure is
** split into five separate pieces, hidden by the macros below.
*/
struct block_std {
	Elf32_Addr start_addr;
	Elf32_Addr end_addr;
	int funcno;
	unchar end_type;
	unchar end_instlen;
	ushort flags;
	Elf32_Addr jump_target; /* goes from address to block in makeblocks */
};

struct block_change {
	Elf32_Addr new_start_addr;
	Elf32_Addr new_end_addr;
	unchar new_end_instlen;
	unchar new_end_type;
	ushort align;
	ulong order;
	Elf32_Addr corrective_jump_target;
	Elf32_Addr fallthrough_target;
	Elf32_Rel *rel;
	Elf32_Rel *endrel;
};

struct block_insertion {
	struct codeblock *beg;
	struct codeblock *end;
};

struct block_stats {
	ulong freq;
	ulong ffreq; /* fall through frequency */
	ulong tfreq; /* taken frequency */
	ulong call_order;
};

struct block_gen {
	char *textbuf;
	short buflen;
	short genlen;
	Elf32_Rel *rel;
	int rellen;
};

#define START_ADDR(BLOCK) (Blocks_std[(BLOCK)].start_addr)
#define SET_START_ADDR(BLOCK, ADDR) (Blocks_std[(BLOCK)].start_addr = (ADDR))
#define FUNCNO(BLOCK) (Blocks_std[(BLOCK)].funcno)
#define SET_FUNCNO(BLOCK, FUNCNO) (Blocks_std[(BLOCK)].funcno = (FUNCNO))
#define START_SYM(BLOCK) (Funcs[Blocks_std[(BLOCK)].funcno])
#define END_TYPE(BLOCK) (Blocks_std[(BLOCK)].end_type)
#define SET_END_TYPE(BLOCK, TYPE) (Blocks_std[(BLOCK)].end_type = (TYPE))
#define JCC_OP(BLOCK) (((Newtext[ENDPOS(BLOCK)] == LONGCODE_PREFIX) ? Newtext[ENDPOS(BLOCK) + 1] : Newtext[ENDPOS(BLOCK)]) & 0xf)

#define END_INSTLEN(BLOCK) (Blocks_std[(BLOCK)].end_instlen)
#define SET_END_INSTLEN(BLOCK, INSTLEN) (Blocks_std[(BLOCK)].end_instlen = (INSTLEN))
#define FLAGS(BLOCK) (Blocks_std[(BLOCK)].flags)
#define SET_FLAGS(BLOCK, FLAGS) (Blocks_std[(BLOCK)].flags = (FLAGS))
#define ADD_FLAGS(BLOCK, FLAGS) (Blocks_std[(BLOCK)].flags |= (FLAGS))
#define DEL_FLAGS(BLOCK, FLAGS) (Blocks_std[(BLOCK)].flags &= ~(FLAGS))
#define KEEP_FLAGS(BLOCK, FLAGS) (Blocks_std[(BLOCK)].flags &= (FLAGS))
#define JUMP_TARGET(BLOCK) (Blocks_std[(BLOCK)].jump_target)
#define SET_JUMP_TARGET(BLOCK, TARGET) (Blocks_std[(BLOCK)].jump_target = (TARGET))
#define END_ADDR(BLOCK) (Blocks_std[(BLOCK)].end_addr)
#define SET_END_ADDR(BLOCK, ADDR) (Blocks_std[(BLOCK)].end_addr = (ADDR))

#define IS_FUNCTION_START(BLOCK) (FLAGS(BLOCK) & FUNCTION_START)
#define IS_LOOP_HEADER(BLOCK) (FLAGS(BLOCK) & LOOP_HEADER)
#define IN_LOW_USAGE_FUNC(BLOCK) (!(Func_info[FUNCNO(BLOCK)].flags & FUNC_PLACED))
#define IN_CHANGED_FUNC(BLOCK) (!(Func_info[FUNCNO(BLOCK)].flags & FUNC_CHANGED))

#define NEW_END_INSTLEN(BLOCK) (Blocks_change[(BLOCK)].new_end_instlen)
#define SET_NEW_END_INSTLEN(BLOCK, INSTLEN) (Blocks_change[(BLOCK)].new_end_instlen = (INSTLEN))
#define ADDTO_NEW_END_INSTLEN(BLOCK, INSTLEN) (Blocks_change[(BLOCK)].new_end_instlen += (INSTLEN))
#define SUBTRACT_FROM_NEW_END_INSTLEN(BLOCK, INSTLEN) (Blocks_change[(BLOCK)].new_end_instlen -= (INSTLEN))
#define NEW_END_TYPE(BLOCK) (Blocks_change[(BLOCK)].new_end_type)
#define SET_NEW_END_TYPE(BLOCK, TYPE) (Blocks_change[(BLOCK)].new_end_type = (TYPE))
#define FUNCALIGN(BLOCK) (IS_FUNCTION_START(BLOCK) && !(Func_info[FUNCNO(BLOCK)].flags & FUNC_DONT_ALIGN))
#define ALIGNMENT(BLOCK) (((BLOCK == Special_align1) || (BLOCK == Special_align2)) ? 1024 : (IS_LOOP_HEADER(BLOCK) ? Loopalign : (FUNCALIGN(BLOCK) ? Funcalign : 0)))
#define ORDER(BLOCK) (Blocks_change[(BLOCK)].order)
#define SET_ORDER(BLOCK, ORDER) (Blocks_change[(BLOCK)].order = (ORDER))
#define ADDTO_ORDER(BLOCK, ORDER) (Blocks_change[(BLOCK)].order += (ORDER))
#define CORRECTIVE_JUMP_TARGET(BLOCK) (Blocks_change[(BLOCK)].corrective_jump_target)
#define SET_CORRECTIVE_JUMP_TARGET(BLOCK, TARGET) (Blocks_change[(BLOCK)].corrective_jump_target = (TARGET))
#define NEW_START_ADDR(BLOCK) (Blocks_change[(BLOCK)].new_start_addr)
#define SET_NEW_START_ADDR(BLOCK, ADDR) (Blocks_change[(BLOCK)].new_start_addr = (ADDR))
#define ADDTO_NEW_START_ADDR(BLOCK, ADDR) (Blocks_change[(BLOCK)].new_start_addr += (ADDR))
#define NEW_END_ADDR(BLOCK) (Blocks_change[(BLOCK)].new_end_addr)
#define SET_NEW_END_ADDR(BLOCK, ADDR) (Blocks_change[(BLOCK)].new_end_addr = (ADDR))
#define ADDTO_NEW_END_ADDR(BLOCK, ADDR) (Blocks_change[(BLOCK)].new_end_addr += (ADDR))
#define REL(BLOCK) (Blocks_change[(BLOCK)].rel)
#define SET_REL(BLOCK, REL) (Blocks_change[(BLOCK)].rel = (REL))
#define ENDREL(BLOCK) (Blocks_change[(BLOCK)].endrel)
#define SET_ENDREL(BLOCK, ENDREL) (Blocks_change[(BLOCK)].endrel = (ENDREL))

#define INSERTION_AT_BEGINNING(BLOCK) (Blocks_insertion[(BLOCK)].beg)
#define SET_INSERTION_AT_BEGINNING(BLOCK, BEGINNING) (Blocks_insertion[(BLOCK)].beg = (BEGINNING))
#define INSERTION_AT_END(BLOCK) (Blocks_insertion[(BLOCK)].end)
#define SET_INSERTION_AT_END(BLOCK, END) (Blocks_insertion[(BLOCK)].end = (END))
#define TEST_INSERTION_AT_BEGINNING(BLOCK) (Blocks_insertion && Blocks_insertion[(BLOCK)].beg)
#define SET_TEST_INSERTION_AT_BEGINNING(BLOCK, BEGINNING) (Blocks_insertion && Blocks_insertion[(BLOCK)].beg = (BEGINNING))
#define TEST_INSERTION_AT_END(BLOCK) (Blocks_insertion && Blocks_insertion[(BLOCK)].end)
#define SET_TEST_INSERTION_AT_END(BLOCK, END) (Blocks_insertion && Blocks_insertion[(BLOCK)].end = (END))

#define FREQ(BLOCK) (Blocks_stats[(BLOCK)].freq)
#define SET_FREQ(BLOCK, FREQ) (Blocks_stats[(BLOCK)].freq = (FREQ))
#define ADDTO_FREQ(BLOCK, FREQ) (Blocks_stats[(BLOCK)].freq += (FREQ))
#define FFREQ(BLOCK) (Blocks_stats[(BLOCK)].ffreq)
#define SET_FFREQ(BLOCK, FFREQ) (Blocks_stats[(BLOCK)].ffreq = (FFREQ))
#define ADDTO_FFREQ(BLOCK, FFREQ) (Blocks_stats[(BLOCK)].ffreq += (FFREQ))
#define TFREQ(BLOCK) (Blocks_stats[(BLOCK)].tfreq)
#define SET_TFREQ(BLOCK, TFREQ) (Blocks_stats[(BLOCK)].tfreq = (TFREQ))
#define ADDTO_TFREQ(BLOCK, TFREQ) (Blocks_stats[(BLOCK)].tfreq += (TFREQ))
#define CALL_ORDER(BLOCK) (Blocks_stats[(BLOCK)].call_order)
#define SET_CALL_ORDER(BLOCK, ORDER) (Blocks_stats[(BLOCK)].call_order = (ORDER))

#define GENBUF(BLOCK) (Blocks_gen[(BLOCK)].textbuf)
#define SET_GENBUF(BLOCK, GENBUF) (Blocks_gen[(BLOCK)].textbuf = (GENBUF))
#define SGENBUF(BLOCK) (Blocks_gen[(BLOCK)].buflen)
#define SET_SGENBUF(BLOCK, SGENBUF) (Blocks_gen[(BLOCK)].buflen = (SGENBUF))
#define GENLEN(BLOCK) (Blocks_gen[(BLOCK)].genlen)
#define SET_GENLEN(BLOCK, GENLEN) (Blocks_gen[(BLOCK)].genlen = (GENLEN))
#define ADDTO_GENLEN(BLOCK, GENLEN) (Blocks_gen[(BLOCK)].genlen += (GENLEN))
#define GENREL(BLOCK) (Blocks_gen[(BLOCK)].rel)
#define SET_GENREL(BLOCK, GENREL) (Blocks_gen[(BLOCK)].rel = (GENREL))
#define SGENREL(BLOCK) (Blocks_gen[(BLOCK)].rellen)
#define SET_SGENREL(BLOCK, SGENREL) (Blocks_gen[(BLOCK)].rellen = (SGENREL))
#define TEXTBUF(BLOCK) ((FLAGS(BLOCK) & GEN_TEXT) ? GENBUF(BLOCK) : (((char *) Text_data->d_buf) + START_ADDR(BLOCK)))

#define PREPOS(BLOCK) (NEW_START_ADDR(BLOCK))
#define CODEPOS(BLOCK) (PREPOS(BLOCK) + PRECODELEN(BLOCK))
#define POSTPOS(BLOCK) (CODEPOS(BLOCK) + CODELEN(BLOCK))
#define ENDPOS(BLOCK) (POSTPOS(BLOCK) + POSTCODELEN(BLOCK))
#define CORRPOS(BLOCK) (ENDPOS(BLOCK) + NEW_END_INSTLEN(BLOCK))

#define PRECODE(BLOCK) ((unchar *) INSERTION_AT_BEGINNING(BLOCK)->textsec->d_buf)
#define PRECODELEN(BLOCK) (TEST_INSERTION_AT_BEGINNING(BLOCK) ? INSERTION_AT_BEGINNING(BLOCK)->textsec->d_size : 0)
#define PRERELSEC(BLOCK) ((unchar *) INSERTION_AT_BEGINNING(BLOCK)->relsec->d_buf)
#define PRERELSIZE(BLOCK) ((TEST_INSERTION_AT_BEGINNING(BLOCK) && INSERTION_AT_BEGINNING(BLOCK)->relsec) ? INSERTION_AT_BEGINNING(BLOCK)->relsec->d_size : 0)
#define PREBLOCKNO_SYMNO(BLOCK) (INSERTION_AT_BEGINNING(BLOCK)->blockno_symno)
#define PREFUNCNO_SYMNO(BLOCK) (INSERTION_AT_BEGINNING(BLOCK)->funcno_symno)
#define PRESYMTAB(BLOCK) ((Elf32_Sym *) INSERTION_AT_BEGINNING(BLOCK)->symtab->d_buf)

#define POSTCODE(BLOCK) ((unchar *) INSERTION_AT_END(BLOCK)->textsec->d_buf)
#define POSTCODELEN(BLOCK) (TEST_INSERTION_AT_END(BLOCK) ? INSERTION_AT_END(BLOCK)->textsec->d_size : 0)
#define POSTRELSEC(BLOCK) ((unchar *) INSERTION_AT_END(BLOCK)->relsec->d_buf)
#define POSTRELSIZE(BLOCK) ((TEST_INSERTION_AT_END(BLOCK) && INSERTION_AT_END(BLOCK)->relsec) ? INSERTION_AT_END(BLOCK)->relsec->d_size : 0)
#define POSTBLOCKNO_SYMNO(BLOCK) (INSERTION_AT_END(BLOCK)->blockno_symno)
#define POSTFUNCNO_SYMNO(BLOCK) (INSERTION_AT_END(BLOCK)->funcno_symno)
#define POSTSYMTAB(BLOCK) ((Elf32_Sym *) INSERTION_AT_END(BLOCK)->symtab->d_buf)

#define CODELEN(BLOCK) (((FLAGS(BLOCK) & GEN_TEXT) ? GENLEN(BLOCK) : (END_ADDR(BLOCK) - START_ADDR(BLOCK))) - END_INSTLEN(BLOCK))
#define HAS_EFFECT(BLOCK) (FLAGS(BLOCK) & HAS_EFFECT_FLAG)
#define COMPUTE_HAS_EFFECT(BLOCK) (Force || \
	Blocks_insertion || \
	(Func_info[FUNCNO(BLOCK)].flags & FUNC_UNTOUCHABLE) || \
	(!(FLAGS(BLOCK) & START_NOP) && (CODELEN(BLOCK) || (END_TYPE(BLOCK) != END_JUMP) || (JUMP_TARGET(BLOCK) == NO_SUCH_ADDR) || (JUMP_TARGET(BLOCK) == BLOCK) || (FLAGS(BLOCK) & ENTRY_POINT))))

#define ORIGENDPOS(BLOCK) (END_ADDR(BLOCK) - END_INSTLEN(BLOCK))

#define FALLTHROUGH(BLOCK) (Blocks_change[BLOCK].fallthrough_target)
#define SET_FALLTHROUGH(BLOCK, FALLTHROUGH) (Blocks_change[BLOCK].fallthrough_target = FALLTHROUGH)

struct end {
	Elf32_Addr end_addr;
	unchar end_type;
	unchar end_instlen;
	unchar beenhere;
	Elf32_Addr jump_addr;
};

EXTERN struct special {
	struct codeblock code;
	char *func;
	char *file;
	char found;
	char epilogue;
	ulong block;
} *Special;
EXTERN int Nspecial;
EXTERN Elf_Data Nulldata;
EXTERN int Special_align1;
EXTERN int Special_align2;

struct jumptable {
	Elf32_Addr	base_address;
	Elf32_Addr	ijump_end_addr;
	Elf32_Addr	tabstart;
	Elf32_Addr	*dsts;
	int tablen;
};

EXTERN struct jumptable *DecodeJumpTables;
EXTERN int NDecodeJumpTables;
EXTERN struct jumptable *JumpTables;
EXTERN int NJumpTables;

EXTERN char 		Shortname[13];
EXTERN char 		*Functionfile;
EXTERN unchar		*Newtext;
EXTERN Elf32_Rel	*Rel;
EXTERN int			Srel;
EXTERN int			Nrel;

/* For detection of switches in PIC code */
EXTERN int			Rodata_sec;

EXTERN int			Canonical;

EXTERN char			*Sizes;
EXTERN int			Nsizes;
EXTERN int			Force;
EXTERN char 		*Orderfile;
EXTERN char			*LinkerOption;
EXTERN int			Loopratio;
EXTERN int			LowUsageRatio;
EXTERN int			Forcedis;
EXTERN int			InlineCriteria;
EXTERN int			InlineCallRatio;
EXTERN int			Loopalign;
EXTERN int			Funcalign;
EXTERN int			NoDataAllowed;
/* do not assume that function symbols are actually text */
EXTERN int			Funcistext;
EXTERN int			SafetyCheck;
EXTERN int			Textalign;
EXTERN int			Checkinstructions;
EXTERN int			Numfuncalign;
EXTERN int			Forcecontiguous;
EXTERN int			Viewfreq;
EXTERN int			Metrics;
EXTERN int			No_warnings;
EXTERN int			Existwarnings;
EXTERN char 		*Freqfile;
EXTERN char 		*Mergefile;
EXTERN Elf32_Rel	*Textrel;
EXTERN Elf32_Rel	*Endtextrel;
EXTERN Elf_Data 	*Eh_data;
EXTERN int			Eh_index;
EXTERN int			Eh_sym;
EXTERN Elf_Data 	*Eh_other;
EXTERN Elf_Data 	*Eh_reldata;
EXTERN Elf_Data 	*Text_data;
EXTERN int	 		Text_index;
EXTERN Elf32_Ehdr	*Ehdr;
EXTERN int			Nfound_0f;
EXTERN struct found_0f *Found_0f;
struct found_0f {
	int block;
	Elf32_Addr offset;
};

EXTERN ulong		*Names;
EXTERN char			**Fnames; /* Local symbol's file name */
EXTERN Elf32_Sym	*Origsymtab;
EXTERN Elf32_Sym	*Symstart;	/* Elf's symbol table */
EXTERN Elf_Data		*Sym_data;
EXTERN Elf32_Sym	**Funcs;
EXTERN int			Nfunc_order;
EXTERN int			*Func_order;
struct func_info {
#define FUNC_CHANGED			(1<<0)
#define DUP_NAME				(1<<1)
#define FUNC_PLACED				(1<<2)
#define FUNC_EXPANDED			(1<<3)
#define EXPANSION_IN_PROGRESS	(1<<4)
#define FUNC_SAVE_ORDER			(1<<5)
#define FUNC_UNTOUCHABLE		(1<<6)
#define FUNC_FOUND				(1<<7)
#define FUNC_ENTRY_POINT		(1<<8)
#define FUNC_DONT_ALIGN			(1<<9)
#define FUNC_GROUPED			(1<<10)
#define FUNC_BEENHERE			(1<<11)
	ushort flags;
	ushort clen;
	ushort placecount;
	ulong start_block;
	ulong end_block;
	ulong order_start;
	ulong ncalls;
	ulong group;
	int *map;
};
EXTERN struct func_info *Func_info;
struct groupinfo {
	Elf32_Addr addr;
	Elf32_Rel *rel;
	int block;
	int start;
	int end;
};
EXTERN struct groupinfo *Groupinfo;
EXTERN int Ngroups;
EXTERN int			Nfuncs;
EXTERN ulong		*Nf_map;
EXTERN Elf32_Sym	**Nonfuncs;
EXTERN int			Nnonfuncs;
EXTERN Elf_Data		*Str_data;

EXTERN int *Flowblockno;
EXTERN int Nflow;
EXTERN struct block_std			*Blocks_std;
EXTERN struct block_insertion	*Blocks_insertion;
EXTERN struct block_stats		*Blocks_stats;
EXTERN struct block_change		*Blocks_change;
EXTERN struct block_gen			*Blocks_gen;
EXTERN int			Ndecodeblocks;
EXTERN int			Nblocks;
EXTERN int			Sblocks;
EXTERN int			*Order;
EXTERN int			Norder;
EXTERN struct end	*Ends;
EXTERN int			Nends;
EXTERN int						Ntextblocks;

EXTERN struct codeblock **New;
EXTERN int Nnew;
EXTERN int Growsym;
EXTERN int Growstr;

#define MAXADDR			0xffffffff
#define NO_SUCH_ADDR	0xffffffff
#define BLOCKNO_SYMNO	(1<<23)-1
#define FUNCNO_SYMNO	(1<<23)-2

#define LONG_CJUMP_LEN	6
#define SHORT_CJUMP_LEN	2
#define JUMP_LEN		5

/* Values for end_type */
#define END_CJUMP			0
#define END_JUMP			1
#define END_RET				2
#define END_FALL_THROUGH	3
/* Do we consider calls the end of a block?  Currently, I say "yes".  This is
* because the function that is being called could lead to an exit of the
* program.  Of course, the code fix-up is like that of a FALL_THROUGH.
*/
#define END_CALL			4
/* When the -KPIC option is used to the compiler, a call is made to the
** next statement.  This really is not a call, it's just an assignment
** to a register done in an odd way.
*/
#define END_PSEUDO_CALL		5
#define END_IJUMP			6
#define END_PUSH			7
#define END_POP				8
#define N_END_TYPES END_POP+1

#define N_JCC				16

EXTERN char *prog;				/* program name */
EXTERN struct section *esections;		/* list of section in target file */
EXTERN int Debug;
EXTERN int			TestPushPop;

#ifdef __STDC__
void usage(char *);
#else
void usage();
typedef enum { B_FALSE, B_TRUE } boolean_t;
#endif

#define BOOL(EXPR) (!!(EXPR))
static void *Ret;
void *out_of_memory();

#define ALIGN(a, b) ((b == 0) ? (a) : ((((a) +(b) -1) / (b)) * (b)))

#define MALLOC(X) ((Ret = (void *) malloc(X)) ? Ret : out_of_memory())
#define REALLOC(S, X) ((Ret = (void *) realloc(S, X)) ? Ret : out_of_memory())
#define REZALLOC(S, X) ((Ret = (void *) realloc(S, X)) ? memset(Ret, '\0', X) : out_of_memory())
#define CALLOC(S, X) ((Ret = (void *) calloc(S, X)) ? Ret : out_of_memory())
#define NAME(SYM) (((char *) Str_data->d_buf) + (SYM)->st_name)
#define FNAME(SYM) (Fnames[(SYM)-Symstart] ? Fnames[(SYM)-Symstart] : "")
#define FULLNAME(SYM) FNAME(SYM), (Fnames[(SYM)-Symstart] ? "@" : ""), NAME(SYM)
#define CURADDR(SYM) ((SYM)->st_value)

/* Not portable yet */
#define GET4(BUF, ADDR) (*((ulong *) (((char *) (BUF)) + (ADDR))))
#define PUT4(BUF, ADDR, VALUE) (*((ulong *) (((char *) (BUF)) + (ADDR))) = (VALUE))
#define ADD4(BUF, ADDR, ADDEND) PUT4(BUF, ADDR, (GET4(BUF, ADDR) + (ADDEND)))
#define GET2(BUF, ADDR) (*((ushort *) (BUF) + (ADDR)))
#define PUT2(BUF, ADDR, VALUE) (*((ushort *) (((char *) (BUF)) + (ADDR))) = (VALUE))
#define GET1(BUF, ADDR) (*((unchar *) (BUF) + (ADDR)))
#define PUT1(BUF, ADDR, VALUE) (((unchar *) (BUF))[ADDR] = (VALUE))

/* Prototypes */
void checkinstructions();
void checkdata();
int target_erratum_condition(Elf32_Addr addr, int block, int gen, int align);
void push_addr(Elf32_Addr addr);
void mark_entry_point(Elf32_Addr start_addr);
int blockstart_by_addr(Elf32_Addr addr);
void get_func_info();
int getblocks();
void find_block_reloc();
void fixup_symbol_table();
void cleanup_symbol_table();
void nopfill(unchar *buf, unchar *end);
void fill_in_text();
void fixup_jumps();
void fixup_pic_jump_tables();
void fixup_other_relocs();
void read_elf(struct codeblock *block, char *filename);
void add_new_symbols_and_strings();
void fixup_new_code_reloc(int symstart, int strstart);
int invalfunction(char *str, int i);
void post_new_code(struct codeblock *block);
void setup_insertion();
void find_0f();
ulong calc_maxfunc();
ulong calc_lue(int new);
ulong calc_jumps();
ulong checksum(int funcstart, int funcend);
int find_name(char *name);
Elf32_Addr get_refto(Elf32_Rel *rel, caddr_t sec_data, Elf32_Sym **ppsym, Elf32_Sym *symstart);
int comp_relocations(const void *v1, const void *v2);
int comp_orig_addr(const void *v1, const void *v2);
int func_by_addr(Elf32_Addr addr);
void param_set(int i, int val, int precedence);
void param_set_env();
void param_set_var_val(const char *str, int precedence);
void bad_opcode();
int comp_new_addr(const void *v1, const void *v2);
int comp_names(const void *v1, const void *v2);
void sort_by_name();
void errexit(const char *fmt, ...);

#define NOP				0x90

struct nop {
	int len;
	unchar *nops;
};
struct param {
	const char *name;
	int *val;
	int precedence;
};
#ifndef DECLARE_GLOBALS
extern struct nop Noptable[];
extern struct param Params[];
extern int Nparams;
extern char *ENDtable[];
extern char *JCCtable[];
#else
struct param Params[] = {
	{ "DEBUG", &Debug, 0 },
	{ "LOOPRATIO", &Loopratio, 0 },
	{ "LOOPALIGN", &Loopalign, 0 },
	{ "FUNCALIGN", &Funcalign, 0 },
	{ "TEXTALIGN", &Textalign, 0 },
	{ "SAFETY_CHECK", &SafetyCheck, 0 },
	{ "NUMFUNCALIGN", &Numfuncalign, 0 },
	{ "FORCE_CONTIGUOUS", &Forcecontiguous, 0 },
	{ "CHECK_INSTRUCTIONS", &Checkinstructions, 0 },
	{ "EXIST_WARNINGS", &Existwarnings, 0 },
	{ "FUNC_IS_TEXT", &Funcistext, 0 },
	{ "LOW_USAGE_RATIO", &LowUsageRatio, 0 },
	{ "FORCE_DIS", &Forcedis, 0 },
	{ "INLINE_CRITERIA", &InlineCriteria, 0 },
	{ "INLINE_CALL_RATIO", &InlineCallRatio, 0 },
	{ "NO_DATA_ALLOWED", &NoDataAllowed, 0 },
};
int Nparams = sizeof(Params) / sizeof(struct param);

#define L3eax	0x8d, 0x40, 0			/* 3 byte: reg+disp8 */
#define L3esi	0x8d, 0x76, 0
#define L3edi	0x8d, 0x7f, 0
#define L4eax	0x8d, 0x44, 0x20, 0		/* 4 byte: modR/M+SIB+disp8 */
#define L4esi	0x8d, 0x74, 0x26, 0
#define L4edi	0x8d, 0x7c, 0x27, 0
#define L6eax	0x8d, 0x80, 0, 0, 0, 0		/* 6 byte: reg+disp32 */
#define L6esi	0x8d, 0xb6, 0, 0, 0, 0
#define L6edi	0x8d, 0xbf, 0, 0, 0, 0
#define L7eax	0x8d, 0x84, 0x20, 0, 0, 0, 0	/* 7 byte: modR/M+SIB+disp32 */
#define L7esi	0x8d, 0xb4, 0x26, 0, 0, 0, 0
#define L7edi	0x8d, 0xbc, 0x27, 0, 0, 0, 0

static const unchar onebyte[] = { NOP /*, 0xf8 */ };

static const unchar twobyte[] = { /* 0x3b, 0xff, */ 0x8b, 0xf6, 0x8b, 0xff, 0x8b, 0xc0 };

static const unchar threebyte[] =
{
	L3eax,	L3esi,	/* next base is %edi */
	L3eax,	L3edi,	/* next base is %esi */
	L3edi,	L3esi,	/* otherwise */
/*	0x83, 0xc6, 0,*/
/*	0x83, 0xc7, 0*/
};
static const unchar lea4[6][4] =
{
	{L4eax},	{L4esi},	/* next base is %edi */
	{L4eax},	{L4edi},	/* next base is %esi */
	{L4edi},	{L4esi},	/* otherwise */
};

static const unchar fivebyte[] =
	{ 0x3d, 0, 0, 0, 0 };

static const unchar sixbyte[] =
{
	L6eax,	L6esi,	/* next base is %edi */
	L6eax,	L6edi,	/* next base is %esi */
	L6edi,	L6esi, /* otherwise */
/*	0x81, 0xff, 0, 0, 0, 0*/
};
static const unchar lea7[6][7] =
{
	{L7eax},	{L7esi},	/* next base is %edi */
	{L7eax},	{L7edi},	/* next base is %esi */
	{L7edi},	{L7esi},	/* otherwise */
};
static const unchar lea8[6][8] =
{
	{L4edi, L4esi},	{L4esi, L4eax},	/* next base is %edi */
	{L4esi, L4edi},	{L4edi, L4eax},	/* next base is %esi */
	{L4edi, L4esi},	{L4esi, L4edi},	/* otherwise */
};
static const unchar lea10[6][10] =
{
	{L3edi, L7esi},	{L3esi, L7eax},	/* next base is %edi */
	{L3esi, L7edi},	{L3edi, L7eax},	/* next base is %esi */
	{L3edi, L7esi},	{L3esi, L7edi},	/* otherwise */
};
static const unchar lea11[6][11] =
{
	{L4edi, L7esi},	{L4esi, L7eax},	/* next base is %edi */
	{L4esi, L7edi},	{L4edi, L7eax},	/* next base is %esi */
	{L4edi, L7esi},	{L4esi, L7edi},	/* otherwise */
};
static const unchar lea12[6][12] =
{
	{L6edi, L6esi},	{L6esi, L6eax},	/* next base is %edi */
	{L6esi, L6edi},	{L6edi, L6eax},	/* next base is %esi */
	{L6edi, L6esi},	{L6esi, L6edi},	/* otherwise */
};
static const unchar lea13[6][13] =
{
	{L6edi, L7esi},	{L6esi, L7eax},	/* next base is %edi */
	{L6esi, L7edi},	{L6edi, L7eax},	/* next base is %esi */
	{L6edi, L7esi},	{L6esi, L7edi},	/* otherwise */
};
static const unchar lea14[6][14] =
{
	{L7edi, L7esi},	{L7esi, L7eax},	/* next base is %edi */
	{L7esi, L7edi},	{L7edi, L7eax},	/* next base is %esi */
	{L7edi, L7esi},	{L7esi, L7edi},	/* otherwise */
};
const struct nop Noptable[] =
{
	0, (unchar *) NULL,
	sizeof(onebyte), (unchar *) onebyte,
	sizeof(twobyte), (unchar *) twobyte,
	sizeof(threebyte), (unchar *) threebyte,
	sizeof(lea4), (unchar *) lea4,
/*	sizeof(fivebyte), (unchar *) fivebyte,*/
	0, (unchar *) NULL,
	sizeof(sixbyte), (unchar *) sixbyte,
	sizeof(lea7), (unchar *) lea7,
	sizeof(lea8), (unchar *) lea8,
	0, (unchar *) NULL,
	sizeof(lea10), (unchar *) lea10,
	sizeof(lea11), (unchar *) lea11,
	sizeof(lea12), (unchar *) lea12,
	sizeof(lea13), (unchar *) lea13,
	sizeof(lea14), (unchar *) lea14
};
char *ENDtable[] = {
	"CJUMP",
	"JUMP",
	"RET",
	"FALL_THROUGH",
	"CALL",
	"PSEUDO_CALL",
	"IJUMP",
	"PUSH",
	"POP",
};
char *JCCtable[] = {
	"JO",
	"JNO",
	"JB",
	"JAE",
	"JE",
	"JNE",
	"JBE",
	"JA",
	"JS",
	"JNS",
	"JP",
	"JNP",
	"JL",
	"JGE",
	"JLE",
	"JG"
};
#endif

#endif /* _FUR_H */
