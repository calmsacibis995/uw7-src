#ident	"@(#)pcintf:pkg_lcs/lcs.h	1.1.1.2"
/* SCCSID(@(#)lcs.h	7.3	LCC)	* Modified: 20:38:36 6/12/91 */
/*
 *  Include file for the LCS (Character set) library
 */


/*
 *  lcs_set_options() mode bit definitions
 */

#define LCS_MODE_NO_MULTIPLE	0x0001	/* Don't translate into multiple */
#define LCS_MODE_STOP_XLAT	0x0002	/* Stop translation on untranslatable */
#define LCS_MODE_USER_CHAR	0x0004	/* Use default_char for nonexact */
#define LCS_MODE_UPPERCASE	0x0008	/* Perform uppercase translation */
#define LCS_MODE_LOWERCASE	0x0010	/* Perform lowercase translation */


extern int lcs_errno;

/*
 *  Errors returned by lcs_translate_string() and lcs_translate_block
 */

#define LCS_ERR_NOTFOUND	(-1)		/* Table was not found */
#define LCS_ERR_BADTABLE	(-2)		/* Bad table file */
#define LCS_ERR_NOTABLE		(-3)		/* No translation tables set */
#define LCS_ERR_NOSPACE		(-4)		/* Insufficient space in out */
#define LCS_ERR_STOPXLAT	(-5)		/* Translation was stopped */
#define LCS_ERR_INPUT_SPLIT	(-6)		/* Second byte not present */


/*
 *  Defininiton of the statistic variables
 */

extern int lcs_exact_translations;
extern int lcs_multiple_translations;
extern int lcs_best_single_translations;
extern int lcs_user_default_translations;
extern int lcs_input_bytes_processed;
extern int lcs_output_bytes_processed;

/*
 *  Define types
 */

typedef unsigned short lcs_char;
typedef unsigned char *lcs_tbl;

/*
 *  Define primitive macros
 */

#define lcs_ascii(c)	((lcs_char)(0x2000 | ((lcs_char)(c) & 0x00ff)))



#ifndef NO_LCS_EXTERNS
#	if defined(__STDC__)

extern lcs_tbl	lcs_get_table(char *, char *);
extern int	lcs_release_table(lcs_tbl);
extern int	lcs_set_tables(lcs_tbl, lcs_tbl);
extern void	lcs_set_options(short, char, short);
extern int	lcs_translate_string(char *, int, char *);
extern int	lcs_translate_block(char *, int, char *, int);
extern lcs_char	lcs_tolower(lcs_char);
extern lcs_char	lcs_toupper(lcs_char);
extern int	lcs_islower(lcs_char);
extern int	lcs_isupper(lcs_char);
extern int	lcs_convert_in(lcs_char *, int, char *, int);
extern int	lcs_convert_out(char *, int, lcs_char *, int);

#	else

extern lcs_tbl	lcs_get_table();
extern lcs_char	lcs_tolower();
extern lcs_char	lcs_toupper();

#	endif
#endif

#ifndef NULL
#if defined(MSDOS) && (defined(M_I86CM) || defined(M_I86LM) || defined(M_I86HM))
#define NULL	0L
#else
#define NULL	0
#endif
#endif
