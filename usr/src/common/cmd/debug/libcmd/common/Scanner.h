#ifndef Scanner_h
#define Scanner_h
#ident	"@(#)debugger:libcmd/common/Scanner.h	1.8"
//
// NAME
//	Scanner, Token
//
// ABSTRACT
//	Lexical analyzer for the command language
//
// DESCRIPTION
//	The Scanner accepts input a line at a time, and produces
//	an list of Token pointers.
//	All data items are private.  Scanner is a friend of Token.
//

#include "string.h"
#include "Link.h"

enum TokType
{
	NoTok = 0,
	DEC,
	HEX,
	OCT,
	CHARCONST,
	STRING,
	NAME,
	AMPERSAND,
	ARROW,
	AT,
	BACKSLASH,
	BANG,
	BAR,
	COLON,
	COMMA,
	DEBUG_NAME, 	// %name - register or built-in
	DOLLAR,
	DOT,
	DQUOTE,	// double quote when escaped
	EQUAL,
	GRAVE,
	GREATER,
	GREATEREQUAL,
	HAT,
	LBRACK,
	LCURLY,
	LESS,
	LPAREN,
	MINUS,
	NL,
	NON_ASCII_SEQ,	// sequence of non-ascii chars
	PERCENT,
	PLUS,
	POUND,
	QUESTION,
	RBRACK,
	RCURLY,
	RPAREN,
	SEMI,
	SLASH,
	SQUOTE,	// single quote, when escaped
	STAR,
	TILDE,
	USER_NAME,	// $name = user-defined name
	WHITESPACE,

	BADNUMBER,
	BADTOKEN,
	BADSTRING,
	BADCHAR
};

class Token;
class Buffer;

extern char *fmtstring( const char * );
extern Token *find_alias( const char * );
extern void make_alias( const char *, Token * );
extern void rm_alias( const char * );

extern void print_tok_list( Token *, Buffer * );
extern void print_alias( const char * );

class Token : public Link
{
	TokType	 _op;	
	short	 _line;	// line number (usually 1)
	short	 _pos;	// charater offset (zero based)
	union
	{
		unsigned long	 _val;	// the "value", if decimal
		char	*_str;	// the "value" for C style identifiers;
				// for strings and character
				// constants, the string without
				// quotes and with
				// escape sequences resolved
	} _value;
	char	*_raw;		// raw value as typed for strings, 
				// character constants and hex or octal
				// numbers;
				// for hex or octal number, we preserve
				// the string form in case they are
				// part of a file name (0010.c) and 
				// reprinting in ASCII would lose 
				// leading 0s or case information 
				// (0xab vs 0xAb)

	friend class Scanner;
public:
		 Token(TokType op, int line, int pos)
			{ _op = op; _line = line; _raw = 0;
			  _pos = pos; _value._val = 0; }
		 Token(Token *t);
		~Token();

	Token	*next();
	Token	*prev();

	TokType		op()	{ return _op; }
	int		line()	{ return _line; }
	int		pos()	{ return _pos; }
	unsigned long	val();
	char		*str()	{ return _value._str; }
	char		*raw()	{ return _raw; }
	char		*resolved_string();
	void		set_str(char *s) { _value._str = s; }
	void		makenum(char *s);

	Token		*alias() { return (_op == NAME)
					? find_alias( _value._str ) : 0;
				 }

	char	*print();
	Token	*subst( Token * );
	Token	*clone();

#if DEBUG
	char	*dump();
#endif
};

class Scanner
{
	Token	*token_head;	// the list of Token pointers
	Token	*token_tail;	// the last Token in the list
	int	 lineno;	// line number within the current command
public:
		 Scanner()	{ lineno = 0; token_head = 0; token_tail = 0; }
		~Scanner()	{ clear(); }

	void	 scan( const char * );
	void	 clear();
	Token	*token()	{ return token_head; }
};

#endif /* Scanner_h */
