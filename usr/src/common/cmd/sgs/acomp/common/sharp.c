#ident	"@(#)acomp:common/sharp.c	52.37"
/* sharp.c */

/* This module processes lines to the compiler that begin with #.
** These lines are passed to sh_proc() with a pointer to a NUL
** terminated line with the # excluded; the NUL replaces the
** new-line that was in the input text.
*/

#include "p1.h"
#include "acgram.h"
#include <string.h>
#include <unistd.h>
#ifdef	MERGED_CPP
#include "interface.h"
#endif



static void do_ident();
static void do_cgfname();
static void do_FILE();
static void do_LINE();
static void do_pragma();
static void do_pr_incdir();
static void do_pr_cwd();



static int sh_passedfile = 0;		/* 0:  no filename passed to CG;
					** 1:  filename from line directive
					** 2:  filename from #file
					** 2 takes precedence over 1; only
					** max. one of either.
					*/

#ifdef	IEEE_PRAGMA
static void do_pr_strict_ieeefp();
#endif
						
#ifdef	PACK_PRAGMA
static void do_pr_pack();
#endif

#ifdef	WEAK_PRAGMA
static void do_pr_weak();
#endif

#ifdef ASM_PRAGMA
static void do_pr_asm_opt();
#endif

#ifdef SECTION_MAP_PRAGMA
static void do_pr_section_map();
#endif

#ifdef NVLT
static void do_pr_nvlt();
#endif

static void do_pr_inline();

#ifndef LINT
#ifdef FIXED_FRAME
static void do_pr_fixed_frame();
#endif
#endif

static void do_pr_int_to_unsigned();

/* Compare string in sh_yytext (returned from sh_yylex()) against s. */
#define	STRINGIS(s) \
	(sh_yyleng == sizeof(s)-1 && strncmp(s, sh_yytext, sizeof(s)-1) == 0)

#ifdef	MERGED_CPP
    static int sh_yyleng;
    static char * sh_yytext;
#define	sh_yylex() lx_sh_yylex(&sh_yytext, &sh_yyleng)
#else
    extern char yytext[];
    extern int yyleng;
#define sh_yyleng yyleng
#define sh_yytext yytext
#define sh_yylex() yylex()
#endif


#ifdef MERGED_CPP

void
sh_proc(code, tp)
int code;
Token * tp;
/* Take stuff from # line seen by CPP.  code is one of
**	PP_IDENT	saw #ident, get token list:  string
**	PP_LINE		saw # (line information) or #line; token list:  file
**	PP_VFILE	saw #file, token list:  file
**	PP_PRAGMA	saw #pragma, get token list:  rest of line
*/
{

    lx_s_sharp(tp);

    switch( code ){
    case PP_IDENT:	do_ident(); break;
    case PP_LINE:	do_LINE(); break;
    case PP_VFILE:	do_FILE(); break;
    case PP_PRAGMA:	do_pragma(); break;
    default:
	cerror(gettxt(":284","sh_proc() got bad code %d from cpp"), code);
    }
    lx_e_sharp();
    return;
}

#else /* ndef MERGED_CPP */

void
sh_proc()
{
    /* We could get one of the following types of line:
    **
    **		# nnn filename
    **		# ident "string"
    **		# file "string"
    */
    switch( sh_yylex() ){
    case L_INT_CONST:
    {
	int newlineno = atoi(sh_yytext);
	char * filename = NULL;

	if (sh_yylex() == L_STRING) {
	    do_cgfname(1);
	    sh_yytext[sh_yyleng-1] = '\0';
	    filename = sh_yytext+1;
	}
	er_filename(filename, newlineno); /* Set error file, line number */
	(void) record_start_of_src_file(PP_CURSEQNO(), newlineno, filename,
					0 /* FALSE */);
	break;
    }
    case L_IDENT:
    {
	/* expect "directive" */
	void (*dodirective)();

	if (STRINGIS("ident"))
	    dodirective = do_ident;
	else if (STRINGIS("file"))
	    dodirective = do_FILE;
	else if (STRINGIS("pragma"))
	    dodirective = do_pragma;
	else {
	    UERROR(gettxt(":285","unknown control: \"%s\""), sh_yytext);
	    dodirective = (void (*)()) 0;
	}

	if (dodirective)
	    dodirective();
	break;
    }
    default:
	break;				/* didn't recognize */
    }
    return;
}

#endif

static void
do_ident()
/* Process #ident. */
{
    if (sh_yylex() != L_STRING || sh_yytext[0] == 'L')
	UERROR(gettxt(":286","string literal expected after #ident"));
    else {
	char savechar = sh_yytext[sh_yyleng];
	sh_yytext[sh_yyleng] = '\0';
	cg_ident(sh_yytext);
	sh_yytext[sh_yyleng] = savechar;
    }
    return;
}


static void
do_cgfname(level)
int level;
/* The current token is a string literal and is assumed to be a
** filename.  Pass the first such string that is at a higher level
** than the current level in passedfile.
*/
{
    char savechar;

    if (level <= sh_passedfile)
	return;

    /* end points to a NUL or " */
    savechar = sh_yytext[sh_yyleng-1];
    sh_yytext[sh_yyleng-1] = '\0';
    cg_filename(sh_yytext+1);		/* pass non-"-quoted string */
    DB_S_FILE(sh_yytext+1);		/* notify debug info - if necessary. */
    sh_yytext[sh_yyleng-1] = savechar;
    sh_passedfile = level;
    return;
}


static void
do_LINE()
/* The next token should be a string literal with the effective
** filename.
*/
{
    if (sh_yylex() == L_STRING && sh_yytext[0] != 'L') {
	do_cgfname(1);			/* this is a level 1 filename */
	LX_GETCURLINE();
    }
    else
	UERROR(gettxt(":287","string literal expected after #line"));
    return;
}


static void
do_FILE()
/* The next token should be a string literal with the effective
** filename.
*/
{
    if (sh_yylex() == L_STRING && sh_yytext[0] != 'L')
	do_cgfname(2);			/* this is a level 2 filename */
    else
	UERROR(gettxt(":288","string literal expected after #file"));
    return;
}

static void
do_pragma()
/* Process pragmas. */
{
    int tokencode = sh_yylex();

    if (tokencode != L_IDENT)
	goto vcheck;			/* no comment about unrecognized */
    
    /*CONSTANTCONDITION*/
    if (0)
	/*EMPTY*/ ;
#ifdef	SECTION_MAP_PRAGMA
    else if (STRINGIS("section_map"))
	do_pr_section_map();
#endif    
#ifdef	PACK_PRAGMA
    else if (STRINGIS("pack"))
	do_pr_pack();
#endif
#ifdef	WEAK_PRAGMA
    else if (STRINGIS("weak"))
	do_pr_weak();
#endif
    else if (STRINGIS("int_to_unsigned"))
	do_pr_int_to_unsigned();
#ifdef IEEE_PRAGMA
    else if (STRINGIS("strict_ieeefp"))
	do_pr_strict_ieeefp();
#endif
#ifdef ASM_PRAGMA
    else if (STRINGIS("asm"))
	do_pr_asm_opt();
#endif
#ifdef NVLT
    else if (STRINGIS("nvlt"))
	do_pr_nvlt();
#endif
    else if (STRINGIS("Cplusplus_inline"))
	do_pr_inline(0 /* Not FE generated function */);
    else if (STRINGIS("compiler_generated_inline"))
	do_pr_inline(1 /* FE generated function */);
    else if (STRINGIS("include_directory"))
	do_pr_incdir();
    else if (STRINGIS("working_directory"))
	do_pr_cwd();
#ifndef LINT
#ifdef FIXED_FRAME
	/* Turn fixed_frame on or off at on a per function basis
	** Syntax is #pragma fixed_frame [ON|OFF] */
    else if (STRINGIS("fixed_frame"))
	do_pr_fixed_frame();
#endif
#endif

/* Add your favorite pragma here.... */
    else
	goto vcheck;
    return;

/* Check whether a warning is appropriate here. */
vcheck:;
    if (verbose) {
	if (tokencode != 0)
	    WERROR(gettxt(":289","unrecognized #pragma ignored: %.*s"), sh_yyleng, sh_yytext);
	else
	    WERROR(gettxt(":290","no tokens follow \"#pragma\""));
    }
    return;
}

#ifdef	SECTION_MAP_PRAGMA

static void
do_pr_section_map()
/* Process #pragma strict_ieeefp.  Syntax:
**	#pragma section_map section newname
**
**	section must be text, data, data1, rodata, rodata1
*/
{
    char  *section, *name;
    char   savechar;

    if (sh_yylex() != L_IDENT)
	goto err;
    savechar = sh_yytext[sh_yyleng];
    sh_yytext[sh_yyleng] = '\0';
    section = st_nlookup(sh_yytext, (unsigned int) sh_yyleng+1);
    sh_yytext[sh_yyleng] = savechar;

    if (sh_yylex() != L_STRING)
	goto err;
    sh_yytext[sh_yyleng-1] = '\0';
    name = st_nlookup(&sh_yytext[1], (unsigned int) sh_yyleng-1);
    sh_yytext[sh_yyleng-1] = '\"';

    /* Must be at end of tokens. */
    if (sh_yylex() != 0)
	goto err;
#ifndef LINT
    if (!strcmp(section, "text"))
	cg_section_map(PROG, name);
    else if (!strcmp(section, "data")) {
	cg_section_map(DATA, name);
	cg_section_map(ADATA, name);
    }
    else if (!strcmp(section, "data1")) {
	cg_section_map(ISTRNG, name);
	cg_section_map(STRNG, name);
    }
    else if (!strcmp(section, "rodata"))
	cg_section_map(CDATA, name);
    else if (!strcmp(section, "rodata1"))
	cg_section_map(CSTRNG, name);
    else
	goto err;	
#endif
    return;

err:;
    WERROR(gettxt(":1554","Ignoring malformed #pragma section_map"));
    return;
}
#endif


#ifdef	IEEE_PRAGMA

static void
do_pr_strict_ieeefp()
/* Process #pragma strict_ieeefp.  Syntax:
**	#pragma strict_ieeefp(mode)
** Mode must be on, off, or ommitted.
*/
{
    int mode = IEEEFP_DEFAULT;
    if (sh_yylex() != L_LP)
	goto err;
    switch( sh_yylex() ){
    case L_RP:
	break;
    case L_IDENT:
	if (STRINGIS("on"))
	    mode = IEEEFP_ON;
	else if (STRINGIS("off"))
	    mode = IEEEFP_OFF;
	else
	    goto err;
	if (sh_yylex() != L_RP)
	    goto err;
	break;
    default:
	goto err;
    }
    /* Must be at end of tokens. */
    if (sh_yylex() != 0)
	goto err;
    SET_IEEEFP_MODE(mode);
    return;

err:;
    WERROR(gettxt(":291","ignoring malformed #pragma strict_ieeefp(mode)"));
    return;
}
#endif

#ifdef	PACK_PRAGMA

static void
do_pr_pack()
/* Process #pragma pack.  Syntax:
**	#pragma pack(n)
** n must be power of 2, <= PACK_PRAGMA, or omitted
*/
{
    char packstring[100];

    if (sh_yylex() != L_LP)
	goto err;
    switch( sh_yylex() ){
    case L_RP:
	packstring[0] = '\0';
	break;
    case L_INT_CONST:
	strncpy(packstring, sh_yytext, sh_yyleng);
	packstring[sh_yyleng] = '\0';
	if (sh_yylex() != L_RP)
	    goto err;
	break;
    default:
	goto err;
    }
    /* Must be at end of tokens. */
    if (sh_yylex() != 0)
	goto err;
    Pack_align = packstring[0] ? Pack_string(packstring) : Pack_default;
    return;

err:;
    WERROR(gettxt(":292","ignoring malformed #pragma pack(n)"));
    return;
}

BITOFF
Pack_string(s)
char * s;
/* Interpret string s as a decimal number:  pack alignment.
** Generate warning if bad value, return current value.
** Otherwise set new value.
*/
{
    int tempalign = atoi(s);
    /* must be power of 2 */
    if (   tempalign <= 0
	|| tempalign > PACK_PRAGMA
	|| (tempalign & (tempalign-1)) != 0
    ) {
	WERROR(gettxt(":293","bad #pragma pack value: %d"), tempalign);
	tempalign = Pack_align;
    }
    else
	tempalign *= SZCHAR;		/* actual value is bits */
    
    return( tempalign );
}

#endif

#ifdef WEAK_PRAGMA

/* The symbol is defined to be 2 printf-like strings, the first for the
**	#pragma weak <symbol>
** form, the second for the
**	#pragma weak <symbol> = <value>
** form.
*/

static void
do_pr_weak()
/* Process a #pragma weak ... */
{
    char * s1;				/* first name string */
    char * s2;				/* second name string */
    int form;				/* form of pragma */
    char savechar;
#ifndef LINT
    const char * s;
    static const char * const weakstring[2] = { WEAK_PRAGMA };
    static int firsttime = 1;
#endif

    if (sh_yylex() != L_IDENT)
	goto badweak;
    savechar = sh_yytext[sh_yyleng];
    sh_yytext[sh_yyleng] = '\0';
    s1 = st_nlookup(sh_yytext, (unsigned int) sh_yyleng+1);
					/* save name string */
    sh_yytext[sh_yyleng] = savechar;

    form = 0;				/* assume first form */
    
    switch( sh_yylex() ){
    case L_EQ:				/* trying for second form */
	if (sh_yylex() != L_IDENT)
	    goto badweak;
	savechar = sh_yytext[sh_yyleng];
	sh_yytext[sh_yyleng] = '\0';
	s2 = st_nlookup(sh_yytext, (unsigned int) sh_yyleng+1);
	sh_yytext[sh_yyleng] = savechar;
	if (sh_yylex() != 0)
	    goto badweak;		/* terminates wrong */
	form = 1;			/* second form of pragma */
	break;
    case 0:
	break;
    default:
	goto badweak;
    }

#ifndef LINT
    /* For first time, make sure we've done beginning-of-file stuff. */
    if (firsttime) {
	firsttime = 0;
	cg_begfile();
    }

    /* Interpret the appropriate string and drop in references to the
    ** strings in the pragma's use.
    */
    for (s = weakstring[form];; ++s ) {
	switch (*s) {
	case 0:
	    return;			/* done */
	case '%':
	    ++s;
	    switch (*s) {
	    case '%':
		putchar('%');
		break;
	    case '1':			/* output first identifier */
		fputs(s1, stdout);
		break;
	    case '2':
		if (form == 0)
		    cerror(gettxt(":294","pragma weak:  %%2 in single ident version"));
		fputs(s2, stdout);
		break;
	    default:
		cerror(gettxt(":295","pragma weak:  unknown %%%c"), *s);
	    }
	    break;
	default:
	    putchar(*s);		/* output the character */
	}
	/* s points at last character processed */
    }
    /* NOTREACHED */
#else /* def LINT */
    return;
#endif

badweak:;
    WERROR(gettxt(":296","ignoring malformed #pragma weak symbol [=value]"));
    return;
}

#endif	/* def WEAK_PRAGMA */

static void
do_pr_int_to_unsigned()
/* Look for "#pragma int_to_unsigned <ident>.  <ident> must
** exist in the symbol table and must be for a function
** returning unsigned.  Mark the entry with the SY_AMBIG bit.
*/
{
    char savechar;
    char * s1;
    SX sid;

    if (sh_yylex() != L_IDENT)
	goto badprag;
    savechar = sh_yytext[sh_yyleng];
    sh_yytext[sh_yyleng] = '\0';
    s1 = st_nlookup(sh_yytext, (unsigned int) sh_yyleng+1);
					/* save name string */
    sh_yytext[sh_yyleng] = savechar;

    if (sh_yylex() != 0)
	goto badprag;
    
    sid = sy_lookup(s1, SY_NORMAL, SY_LOOKUP);
    if (sid != SY_NOSYM) {
	/* Must have type function-returning-unsigned. */
	T1WORD t = SY_TYPE(sid);
	if (   !TY_ISFTN(t)
	    || TY_EQTYPE((t=TY_DECREF(t)), TY_UINT) == 0
	    )
	    WERROR(gettxt(":297","must have type \"function-returning-unsigned\": %s"), s1);
	else
	    SY_FLAGS(sid) |= SY_AMBIG;
    }
    return;

badprag:
    WERROR(gettxt(":298","ignoring malformed #pragma int_to_unsigned symbol"));
    return;
}

#ifndef LINT
#ifdef FIXED_FRAME

extern set_function_fixed_frame_flag();

	/* Turn fixed_frame on or off at on a per function basis */
static void
do_pr_fixed_frame()
{
    int tokencode;

    tokencode = sh_yylex();

    if(tokencode != L_IDENT)
	goto badprag;
    if(STRINGIS("ON"))
	set_function_fixed_frame_flag(1);	
    else if(STRINGIS("OFF"))
	set_function_fixed_frame_flag(0);	
    else goto badprag;
    return;
badprag:
    WERROR(gettxt(":1555","malformed #pragma %s"), "fixed_frame");
    return;
}
#endif
#endif

#ifdef ASM_PRAGMA
static void
do_pr_asm_opt()
/* Look for "#pragma asm [full_optimization|partial_optimization] <ident>.  
   <ident> must exist in the symbol table and must be for an asm.
   Mark the entry with either SY_ASM_FULL_OPT or SY_ASM_PARTIAL_OPT
*/
{
    char savechar;
    char * s1;
    SX sid;
    int sy_flag;
    int tokencode = sh_yylex();

    /* find optimization type */
    if (tokencode != L_IDENT)
	goto badprag;		
    if (STRINGIS("full_optimization"))
	sy_flag = SY_ASM_FULL_OPT;
    else if (STRINGIS("partial_optimization"))
	sy_flag = SY_ASM_PARTIAL_OPT;
    else
	goto badprag;

    /* find function name */
    if (sh_yylex() != L_IDENT)
	goto badprag;
    savechar = sh_yytext[sh_yyleng];
    sh_yytext[sh_yyleng] = '\0';
    s1 = st_nlookup(sh_yytext, (unsigned int) sh_yyleng+1);
    sh_yytext[sh_yyleng] = savechar;

    /* check that no tokens follow function name */
    if (sh_yylex() != 0)
	goto badprag;
    
    /* check function name has been previously declared */
    sid = sy_lookup(s1, SY_NORMAL, SY_LOOKUP);
    if (sid != SY_NOSYM && TY_ISFTN(SY_TYPE(sid))) {
        SY_FLAGS(sid) |= sy_flag;
	return;
    }
    else
        WERROR(gettxt(":299","%s must be declared function"), s1);

badprag:
    WERROR(gettxt(":1555","ignoring malformed #pragma %s"), "asm");
    return;
}
#endif	/* ASM_PRAGMA */

#ifdef NVLT

extern nvltflag;

	/* Turn nvltflag on or off at a given point in the file.
	** Syntax is #pragma nvlt [ON|OFF] */
static void
do_pr_nvlt()
{
    int tokencode = sh_yylex();

    if(tokencode != L_IDENT)
	goto badprag;
    if(STRINGIS("ON"))
	nvltflag |= 1;
    else if(STRINGIS("OFF"))
	nvltflag &= 2;
    else goto badprag;
    return;
badprag:
    WERROR(gettxt(":1555","ignoring malformed #pragma %s"), "nvlt");
    return;
}
#endif

static void
do_pr_inline(fe_generated)
int	fe_generated;
/* Look for #pragma inline <ident>.  <ident> must exist in the symbol
** table and must be for a function.  Mark the entry with the 
** SY_CPLUSPLUS_INLINE or SY_CPP_FE_INLINE bit.
*/
{
    char savechar;
    char * s1;
    SX sid;

    if (sh_yylex() != L_IDENT)
	goto badprag;
    savechar = sh_yytext[sh_yyleng];
    sh_yytext[sh_yyleng] = '\0';
    s1 = st_nlookup(sh_yytext, (unsigned int) sh_yyleng+1);
					/* save name string */
    sh_yytext[sh_yyleng] = savechar;

    if (sh_yylex() != 0)
	goto badprag;
    
    sid = sy_lookup(s1, SY_NORMAL, SY_LOOKUP);
    if (sid != SY_NOSYM && TY_ISFTN(SY_TYPE(sid))) {
#ifdef IN_LINE
#ifndef LINT
	/* Pass the C++ inline pragma to the inline routines and data.
	   The function has already been processed.
	*/
	extern void mark_cpp_inline();

	mark_cpp_inline(sid, fe_generated);
#endif  /* LINT */
#endif  /* IN_LINE */
	return;
    }
    else
        WERROR(gettxt(":299","%s must be declared function"), s1);

badprag:
    WERROR(gettxt(":1555","ignoring malformed #pragma %s"), 
	   (fe_generated ? "compiler_generated_inline" : "Cplusplus_inline"));
    return;
}

static void
do_pr_incdir()

{
    char savechar;

    if (sh_yylex() == L_STRING && sh_yytext[0] != 'L') {
	savechar = sh_yytext[sh_yyleng - 1];
	sh_yytext[sh_yyleng - 1] = '\0';
#ifndef LINT
	DB_REG_INCDIR(sh_yytext + 1);	/* pass non-"-quoted string */
#endif
	record_incdir(sh_yytext + 1, sh_yyleng - 2);
	sh_yytext[sh_yyleng - 1] = savechar;

	if (sh_yylex() == 0)
	    return;
    }  /* if */
    
    WERROR(gettxt(":1555","ignoring malformed #pragma %s"), 
	   "include_directory");
    return;
}

static void 
do_pr_cwd()
{
    char savechar;

    if (sh_yylex() == L_STRING && sh_yytext[0] != 'L') {
	savechar = sh_yytext[sh_yyleng - 1];
	sh_yytext[sh_yyleng - 1] = '\0';
#ifndef LINT
	DB_S_CWD(sh_yytext + 1);	/* pass non-"-quoted string */
#endif
	sh_yytext[sh_yyleng - 1] = savechar;

	if (sh_yylex() == 0)
	    return;
    }  /* if */
    
    WERROR(gettxt(":1555","ignoring malformed #pragma %s"), 
	   "working_directory");
    return;
}
