/*		copyright	"%c%" 	*/


%{
#ident	"@(#)pos_localedef:common/cmd/pos_localedef/colltbl/parse.y	1.1.9.1"

/* Preprocessor Statements */
#include "colltbl.h"

#define SZ_COLLATE	256
#define ORD_LST		1
#define PAR_LST		2
#define BRK_LST		3

/* External Variables */
extern char	codeset[];
extern int	curprim;
extern int	cursec;

static int	ordtype = ORD_LST;
static unsigned char	begrng = 0;
static int	cflag = 0;
static int	oflag = 0;


/* Redefined Error Message Handler */
void yyerror(text)
char *text;
{
	error(YYERR, text);
}
%}

%union {
	char	*sval;
}

/* Token Types */
%token		ELLIPSES
%token	<sval>	ID
%token		IS
%token		CODESET
%token		ORDER
%token		SUBSTITUTE
%token		SEPARATOR
%token	<sval>	STRING
%token	<sval>	SYMBOL
%token		WITH

%type	<sval>	symbol
%type	<sval>	error
%%
collation_table : statements
		  {
			if (!cflag || !oflag)
				error(PRERR,"codeset or order statement not specified");
		  }
		;

statements 	: statement
		| statements statement
		;

statement	: codeset_stmt
		  {
			if (cflag)
				error(PRERR, "multiple codeset statements seen");
			cflag++;
		  }
		| order_stmt
		  {
			if (oflag)
				error(PRERR, "multiple order statements seen");
			oflag++;
		  }
		| substitute_stmt
		| error 
		  {
			error(EXPECTED,"codeset, order or substitute statement");
		  }
		;

codeset_stmt	: CODESET ID
		  {
			if (strlen($2) >= 50)
				error(TOO_LONG,"file name",$2);
			strcpy(codeset, $2);
		  }
		;

substitute_stmt	: SUBSTITUTE STRING WITH STRING
		  {
			substitute($2, $4);
		  }
		;

order_stmt	: ORDER IS order_list
		;

order_list	: order_element
		| order_list SEPARATOR order_element
		;

order_element	: symbol
		| lparen sub_list rparen
		  {
			ordtype = ORD_LST;
		  }
		| lbrace sub_list rbrace
		  {
			ordtype = ORD_LST;
		  }
		| error 
		  {
			error(INVALID, "order element", $1);
		  }
		;

lparen		: '('
		  {
			ordtype = PAR_LST;
			++curprim;
			cursec = 1;
		  }
	  	;

rparen		: ')'
		;

lbrace		: '{'
		  {
			ordtype = BRK_LST;
			++curprim;
			cursec = 0;
		  }
	  	;

rbrace		: '}'
		;

sub_list	: sub_element
		| sub_list SEPARATOR sub_element
		| error 
		  {
			error(INVALID, "list", "inter-filed");
		  }
		;

sub_element	: symbol
		;

symbol		: SYMBOL
		  {
			if (strlen($1) == 1)
				begrng = *$1;
			else
				begrng = 0;
			mkord($1, ordtype);
		  }
		| ELLIPSES SEPARATOR SYMBOL
		  {
			static char	tarr[2];
			int	i, n;

			if (begrng == 0 || strlen($3) != 1 || (unsigned char)*$3 <= begrng)
				error(PRERR, "bad list range");
			n = (int) (unsigned char)*$3 - begrng;

			for(i=0; i<n; i++) {
				begrng++;
				tarr[0] = begrng;
				mkord(tarr, ordtype);
			}
		  }
		;
