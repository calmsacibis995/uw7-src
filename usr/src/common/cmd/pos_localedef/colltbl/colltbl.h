/*	copyright	"%c%"	*/

#ident	"@(#)pos_localedef:common/cmd/pos_localedef/colltbl/colltbl.h	1.1.5.2"

/* Diagnostic mnemonics */
#define WARNING		0
#define ERROR		1
#define ABORT		2

enum errtype {
	GEN_ERR,
	DUPLICATE,
	EXPECTED,
	ILLEGAL,
	TOO_LONG,
	INSERTED,
	NOT_FOUND,
	NOT_DEFINED,
	TOO_MANY,
	INVALID	,
	BAD_OPEN,
	NO_SPACE,
	NEWLINE,
	REGERR,
	CWARN,
	YYERR,
	PRERR
};

/* Diagnostics Functions and Subroutines */
void	error();
void	regerr();
void	usage();
