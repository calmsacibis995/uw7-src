#ident	"@(#)nas:common/sect.h	1.7"
/*
* common/sect.h - common assembler section header
*
* Depends on:
*	<libelf.h> - if FORMAT == ELF
*	"common/as.h"
*/

	/* section attribute bits */
#if FORMAT == ELF		/* values from <libelf.h> */
#  define Attr_Alloc	SHF_ALLOC
#  define Attr_Exec	SHF_EXECINSTR
#  define Attr_Write	SHF_WRITE
#else
#  define Attr_Alloc	0x1
#  define Attr_Exec	0x2
#  define Attr_Write	0x4
#endif

#if FORMAT == ELF		/* values from <libelf.h> */
   enum	/* builtin section types */
   {
	SecTy_Null	= SHT_NULL,
	SecTy_Progbits	= SHT_PROGBITS,
	SecTy_Symtab	= SHT_SYMTAB,
	SecTy_Strtab	= SHT_STRTAB,
	SecTy_RelocA	= SHT_RELA,
	SecTy_Hash	= SHT_HASH,
	SecTy_Dynamic	= SHT_DYNAMIC,
	SecTy_Note	= SHT_NOTE,
	SecTy_Nobits	= SHT_NOBITS,
	SecTy_Reloc	= SHT_REL,
	SecTy_Dynsym	= SHT_DYNSYM,
	SecTy_Delay_Rel	= SHT_DELAY_REL
   };
#else
   enum	/* builtin section types */
   {
	SecTy_Null,
	SecTy_Progbits,
	SecTy_Symtab,
	SecTy_Strtab,
	SecTy_RelocA,
	SecTy_Hash,
	SecTy_Dynamic,
	SecTy_Note,
	SecTy_Nobits,
	SecTy_Reloc,
	SecTy_Dynsym,
	SecTy_Delay_Rel
   };
#endif

/* Flags associated with the Code struct code_impdep field; defined here
** so as to be visible to common/syms.c and common/sect.c.
*/
#ifdef P5_ERR_41
#define CODE_P5_BR_LABEL	0x10	/* label associated with this code */
#define CODE_P5_0F_NOT_8X	0x20	/* instruction is a known 0f !8x
					   prefix instruction. */
#define CODE_NO_CC_PAD		0x40	/* force this padding code sequence
					   to contain nops which DO NOT
					   change the condition code. */
#define CODE_PREFIX_PSEUDO_OP	0x80	/* this "code" instruction is a
					   prefix pseudo op associated
					   with the following instruction. */
#endif

enum	/* section content kinds */
{
	CodeKind_Unset,		/* as yet unused */
	CodeKind_RelSym,	/* relocation off symbol */
	CodeKind_RelSec,	/* relocation off section */
	CodeKind_FixInst,	/* fixed-sized instruction */
	CodeKind_VarInst,	/* variable-sized instruction */
	CodeKind_Expr,		/* expression value */
	CodeKind_Align,		/* alignment request */
	CodeKind_Backalign,	/* alignment at label */
	CodeKind_Skipalign,	/* the padding location for Backalign */
	CodeKind_Pad,		/* skip bytes */
	CodeKind_Zero,		/* fill zero bytes */
	CodeKind_String		/* string value */
};

enum	/* common CodeKind_Expr target forms */
{
	CodeForm_Float,		/* single precision floating */
	CodeForm_Double,	/* double precision floating */
	CodeForm_Extended,	/* double extended floating */
	CodeForm_Integer	/* integer binary value */
};

typedef struct t_more_	More;	/* defined below */

struct t_more_	/* extended information for align/pad/string codes */
{
	union
	{
		const Uchar	*more_str;	/* for CodeKind_String */
		Code		*more_code;	/* for Pad/Backalign */
	} data;
	Code			*more_prev;	/* previous code */
	Ulong			more_line;	/* line of directive */
	Ushort			more_file;	/* file of directive */
};

struct t_code_	/* each unit of a section's contents */
{
	Code			*code_next;	/* next code for sect */
	union
	{
		More		*code_more;	/* for Pad/String/align */
		const Inst	*code_inst;	/* for CodeKind_...Inst */
		Symbol		*code_sym;	/* for CodeKind_RelSym */
		Section		*code_sec;	/* for CodeKind_RelSec */
		Uint		code_form;	/* for CodeKind_Expr */
	} info;
	union
	{
		Oplist		*code_olst;	/* only for instructions */
		Expr		*code_expr;	/* expression value */
		const Ulong	*code_align;	/* for Align/Backalign */
		Ulong		code_skip;	/* for String/Zero/Pad */
		Ulong		code_setadd;	/* incoming addend and... */
		long		code_addend;	/* outgoing (SecTy_RelocA) */
	} data;
	Ulong			code_addr;	/* section-relative */
	Ushort			code_size;	/* not if code_skip used */
	Uchar			code_kind;	/* CodeKind_* value */
	Uchar			code_impdep;	/* for implementations */
};

struct t_sect_	/* section information */
{
	Uchar		*sec_data;	/* final contents */
	Section		*sec_next;	/* next section in list */
	Section		*sec_relo;	/* matching relocation info */
	Symbol		*sec_sym;	/* symbol for section name */
	Code		*sec_code;	/* list of contents */
	Code		*sec_last;	/* last of list */
	Code		*sec_prev;	/* next-to-last of list */
	Code		*sec_lastpad;	/* most recent padding code */
	union
	{
		Expr	*sec_expr;	/* attribute as expression */
		Ulong	sec_value;	/* attribute as value */
	} attr;
	union
	{
		Expr	*sec_expr;	/* type as expression */
		Ulong	sec_value;	/* type as value */
	} type;
	Ulong		sec_align;	/* current max alignment */
	Ulong		sec_size;	/* current size of section */
	size_t		sec_index;	/* 1-based index for section */
	Ushort		sec_impdep;	/* for implementations */
	Uchar		sec_flags;	/* internal flags */
};

#ifdef __STDC__
void	initsect(void);
void	setsect(Symbol *, Expr *, Expr *);	/* define/set section */
Section	*relosect(Section *, int);		/* create matching reloc */
Section	*cursect(void);				/* return current sect */
void	prevsect(void);				/* toggle cur/prev sects */
int	stacksect(Section *);			/* push/pop sect stack */
void	walksect(void);				/* walk thru all sects */
void	gensect(void);				/* generate all contents */
int	sectoptalign(Section *, const Ulong *);	/* test alignment and fix */
void	sectalign(Section *, const Ulong *, Uint); /* add alignment request */
void	sectbackalign(Section *, const Symbol *, const Ulong *, Uint);
void	sectzero(Section *, Ulong);		/* add zero-valued bytes */
void	sectpad(Section *, Eval *);		/* add padding bytes */
#ifdef P5_ERR_41
void	sectinsertpad(Section *, Code *, Ulong);  /* insert padding bytes */
#endif
void	sectexpr(Section *, Expr *, Ulong, int);/* add expression value */
void	sectstr(Section *, Expr *);		/* add string value */
void	sectvinst(Section *, const Inst *, Oplist *);	/* add var-inst */
void	sectfinst(Section *, const Inst *, Oplist *);	/* add fix-inst */
Code	*sectrelsec(Section *, int, Ulong, Section *);	/* section based */
Code	*sectrelsym(Section *, int, Ulong, Symbol *);	/* symbol based */
void	recalc_sect_addrs(Section *, Code *, Ulong, char *);
						/* recalc code_addr */

		/* implementation provides */
void	obssectattr(Section *, const Expr *);	/* handle obsolete attrs */
int	validalign(Ulong);			/* verify alignment value */

#ifdef P5_ERR_41
void	chk_P5_0F_issues(Code *);	/* flag all 0f !8x instructions and 
					   each statement following a call */
int	pentium_bug(Section *);		/* Check for possible Pentium erratum 41
				           candidates and insert padding/nop */
void	P5_err_report_stats(const char *);	/* report padding info. */
#endif
#else
void	initsect(), setsect(), prevsect();
int	stacksect();
Section	*relosect(), *cursect();
void	walksect(), gensect();
int	sectoptalign();
void	sectalign(), sectbackalign(), sectzero(), sectpad();
#ifdef P5_ERR_41
void	sectinsertpad();
#endif
void	sectexpr(), sectstr(), sectvinst(), sectfinst();
Code	*sectrelsec(), *sectrelsym();
void	recalc_sect_addrs();

void	obssectattr();
int	validalign();

#ifdef P5_ERR_41
void	chk_P5_0F_issues();
int	pentium_bug();
void	P5_err_report_stats();
#endif
#endif
