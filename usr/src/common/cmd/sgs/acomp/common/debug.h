#ident	"@(#)acomp:common/debug.h	55.1.2.3"
/* debug.h */


/* Definitions to support debugging output. */


extern int db_linelevel;	/* line number debug level */
extern int db_symlevel;		/* symbol info debug level */
extern int db_abbreviations;	/* generate Dwarf2 abbreviation table */
extern int db_curline;		/* current debugger line */
extern int db_format;		/* debugging information format */
extern int db_name_lookup;	/* Dwarf 2 feature */
extern int db_addr_ranges;	/* Dwarf 2 feature */

extern void db_s_file();
extern void db_e_file();
extern void db_s_cwd();
extern void db_reg_incdir();
extern int db_reg_file();
extern void db_begf();
extern void db_s_fcode();
extern void db_e_fcode();
extern void db_endf();
extern void db_s_block();
extern void db_e_block();
extern void db_symbol();
extern void db_lineno();
extern void db_sue();
extern void db_sy_clear();
extern void db_s_inline();
extern void db_s_inline_expr();
extern void db_e_inline();

/* There's a tie-in between the way the "cc" command passes
** the -ds, -dl flags, how main.c sets the db_linelevel and
** db_symlevel flags, and how debug.c/elfdebug.c use them.
*/
#define	DB_LEVEL2	0	/* No -ds, -dl */
#define	DB_LEVEL0	1	/* -ds, -dl set */
#define	DB_LEVEL1	2	/* Not supported by "cc". */

/* Values to select the format of debugging information.  The 
** acomp or c++be options -d1 or -d2 will select Dwarf 1 or
** Dwarf 2, respectively.  The current default is Dwarf 1.
*/
#define DB_DWARF_1	0
#define DB_DWARF_2	1

#define IS_DWARF_1()	(! db_format)
#define IS_DWARF_2()	(db_format == DB_DWARF_2)

/* Values to specify the language being compiled through this
** code generator.
*/
#ifdef __STDC__
typedef enum language {Unknown_lang, C_lang, Cplusplus_lang,
		       End_lang} language_type;
#else
typedef int language_type;
#define Unknown_lang	0x0
#define C_lang		0x1
#define Cplusplus_lang	0x2
#define End_lang	0x3
#endif

/* These select different behavior if function-at-a-time
** compilation.
*/
#ifdef FAT_ACOMP

#define	DB_S_FILE(s)	cg_q_str(db_s_file, s)
#define	DB_E_FILE()	cg_q_call(db_e_file)
#define	DB_S_CWD(s)	cg_q_str(db_s_cwd, s)
#define DB_SET_LANG(s)	db_set_lang(s)
#define	DB_REG_INCDIR(s)	db_reg_incdir(s)
#define	DB_REG_FILE(s)	db_reg_file(s)
#define	DB_BEGF(sid)	cg_q_sid(db_begf, sid)
#define	DB_S_FCODE()	cg_q_call(db_s_fcode)
#define	DB_E_FCODE()	cg_q_call(db_e_fcode)
#define	DB_S_BLOCK()	cg_q_call(db_s_block)
#define	DB_E_BLOCK()	cg_q_call(db_e_block)
#define	DB_SYMBOL(sid)	cg_q_sid(db_symbol, sid)
#define	DB_ENDF()	cg_q_call(db_endf)

#ifdef  DBLINE
#define	DB_LINENO()	cg_mk_lineno()
#else
#define	DB_LINENO()	cg_q_call(db_lineno)
#endif

#define	DB_SUE(t)	cg_q_type(db_sue, t)
#define	DB_SY_CLEAR(sid) cg_q_sid(db_sy_clear, sid)

#else	/* ! FAT_ACOMP */

#define DB_S_FILE(s)	db_s_file(s)
#define DB_E_FILE()	db_e_file()
#define DB_S_CWD(s)	db_s_cwd(s)
#define DB_SET_LANG(s)	db_set_lang(s)
#define DB_REG_INCDIR(s)	db_reg_incdir(s)
#define DB_REG_FILE(s)	db_reg_file(s)
#define DB_BEGF(sid)	db_begf(sid)
#define DB_S_FCODE()	db_s_fcode()
#define DB_E_FCODE()	db_e_fcode()
#define DB_ENDF()	db_endf()
#define DB_S_BLOCK()	db_s_block()
#define DB_E_BLOCK()	db_e_block()
#define DB_SYMBOL(sid)	db_symbol(sid)
#define DB_LINENO()	db_lineno()
#define DB_SUE(t)	db_sue(t)
#define DB_SY_CLEAR(sid) db_sy_clear(sid)

#endif	/* def FAT_ACOMP */
