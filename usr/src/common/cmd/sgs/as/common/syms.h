#ident	"@(#)nas:common/syms.h	1.2"
/*
* common/syms.h - common assembler symbol header
*
* Depends on:
*	<libelf.h> - if FORMAT == ELF
*	"common/as.h"
*/

#if FORMAT == ELF		/* values from <libelf.h> */
   enum	/* symbol bindings */
   {
	Bind_Local	= STB_LOCAL,
	Bind_Global	= STB_GLOBAL,
	Bind_Weak	= STB_WEAK,
	Bind_Temporary	= STB_NUM	/* not a valid binding */
   };
#else
   enum	/* symbol bindings */
   {
	Bind_Local,
	Bind_Global,
	Bind_Weak,
	Bind_Temporary
   };
#endif

#if FORMAT == ELF		/* values from <libelf.h> */
   enum	/* symbol types */
   {
	SymTy_None	= STT_NOTYPE,
	SymTy_Object	= STT_OBJECT,
	SymTy_Function	= STT_FUNC,
	SymTy_Section	= STT_SECTION,
	SymTy_File	= STT_FILE
   };
#else
   enum	/* symbol types */
   {
	SymTy_None,
	SymTy_Object,
	SymTy_Function,
	SymTy_Section,
	SymTy_File
   };
#endif

enum	/* symbol kinds */
{
	SymKind_Regular,	/* regular symbol */
	SymKind_Dot,		/* the special symbol . (dot) */
	SymKind_Common,		/* a common symbol */
	SymKind_Set,		/* a .set symbol */
	SymKind_GOT		/* global offset table base */
};

struct t_syms_	/* symbol information */
{
	const Uchar	*sym_name;	/* nul-terminated symbol name */
	Section		*sym_sect;	/* non-zero => (also) section */
	Section		*sym_defn;	/* non-zero => symbol defined */
	Expr		*sym_uses;	/* this symbol's exprs */
	union
	{
		Expr	*sym_expr;	/* symbol address expression */
		Operand	*sym_oper;	/* .set operand */
	} addr;
	union
	{
		Expr	*sym_expr;	/* size as expression */
		Ulong	sym_value;	/* size as value */
	} size;
	union
	{
		Expr	*sym_expr;	/* type as expression */
		Ulong	sym_value;	/* type as value */
	} type;
	union				/* only for common symbols */
	{
		Expr	*sym_expr;	/* alignment as expression */
		Ulong	sym_value;	/* alignment as value */
	} align;
	size_t		sym_nlen;	/* strlen(sym_name) */
	size_t		sym_index;	/* nontemporary 1-based index */
	Ulong		sym_line;	/* line number of the definition */
	Ushort		sym_file;	/* file number of the definition */
	Uchar		sym_kind;	/* SymKind_* value */
	Uchar		sym_exty;	/* its type as an expr: ExpTy_* */
	Uchar		sym_mods;	/* address value expr modifiers */
	Uchar		sym_bind;	/* Bind_* value */
	Uchar		sym_refd;	/* nonzero => somehow referenced */
	Uchar		sym_flags;	/* internal flags */
};

#ifdef __STDC__
void	initsyms(void);				/* predefined symbols */
Symbol	*lookup(const Uchar *, size_t);		/* lookup or enter name */
void	label(const Uchar *, size_t);		/* define label */
void	walksyms(void);				/* walk thru all symbols */
void	gensyms(void);				/* make obj file symtab */
void	bindsym(Symbol *, int);			/* binding for symbol */
void	commsym(Symbol *, int, Expr *, Expr *);	/* note symbol as common */
void	bsssym(Symbol *, int, Ulong, Ulong);	/* define symbol in bss */
void	sizesym(Symbol *, Expr *);		/* set size for symbol */
void	typesym(Symbol *, Expr *);		/* set type for symbol */
void	opersym(Symbol *, Operand *);		/* handle .set symbol */
Uchar	*savestr(const Uchar *, size_t);	/* stash string contents */
void	printsymbol(const Symbol *);		/* output entry contents */
#else
void	initsyms(), label();
Symbol	*lookup();
void	walksyms(), gensyms();
void	bindsym(), sizesym(), typesym(), commsym(), bsssym(), opersym();
Uchar	*savestr();
void	printsymbol();
#endif
