#ident	"@(#)alint:common/tables2.h	1.3"
#define ARGS	50
#define NSZ	2048	/* symbol table size	*/
#define STRSIZ	113
#define STACKSZ 128
#define TYSZ	500

extern STYPE *tary;
extern STYPE tarychunk0[];

extern ATYPE (* atyp)[];
extern char *(* strp)[];

extern int args_alloc;
extern char *curname;

extern void arg_expand();
extern STAB *find();
extern STYPE *tget();
extern void cleanup();
extern void build_lsu();
extern int eq_struct();
extern ENT *tyfind();
extern char *getstr();
extern char *l_hash();
