#ident	"@(#)debugger:libcmd/common/Scanner.C	1.14"
#include "Scanner.h"
#include "Buffer.h"
#include "Interface.h"
#include "Input.h"
#include "str.h"
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Alias maintains a linked list of aliases
// The aliases are stored in alphabetical order

struct Alias : public Link
{
	char   *name;	// alias name
	Token  *tlist;	// list of tokens making up the alias

		Alias() { name = 0; tlist = 0; }
	       ~Alias();
	Alias  *next() { return (Alias *)Link::next(); }
	Alias  *prev() { return (Alias *)Link::prev(); }
};

static Alias *alias_table;

// alias -r name removes the alias from the table and
// reclaims the token list

Alias::~Alias()
{
	if ( alias_table == this )
		alias_table = next();
	unlink();
	delete tlist;
}

// add a new alias to the table, or replace the list of tokens
// if the alias is already in the table

void
make_alias( register const char *name, register Token *tlist )
{
	register Alias *p, *pp, *np;
	pp = 0;
	for ( p = alias_table ; p ; pp = p, p = p->next() )
	{
		if ( p->name == name )
		{
			delete p->tlist;
			p->tlist = tlist->clone();
			return;
		}
		else if ( strcmp( p->name, name ) > 0 )
		{
			break;
		}
	}
	np = new Alias;
	np->name = (char *)name;
	np->tlist = tlist->clone();
	if ( pp )
	{
		np->append(pp);
	}
	if ( alias_table == p )
	{
		alias_table = np;
		if ( p )
			np->prepend(p);
	}
}

Token *
find_alias( register const char *name )
{
	register Alias *p;
	if ( name )
	{
		for ( p = alias_table ; p ; p = p->next() )
		{
			if ( p->name == name )
				return p->tlist;
		}
	}
	return 0;
}

// print the list of tokens making up the alias
// if no name is given, print all the aliases

void
print_alias( register const char *name )
{
	Buffer	*buf = buf_pool.get();
	if ( name )
	{
		register Token *tl;
		if ((tl = find_alias( name )) != 0)
		{
			print_tok_list(tl, buf);
			printm(MSG_alias, name, (char *)*buf);
		}
		else
			printe(ERR_no_alias, E_WARNING, name);
	}
	else
	{
		// print all aliases
		register Alias *p;
		for ( p = alias_table ; p ; p = p->next() )
		{
			print_tok_list(p->tlist, buf);
			printm(MSG_alias, p->name, (char *)*buf);
		}
	}
	buf_pool.put(buf);
}

void
rm_alias( register const char *name )
{
	register Alias *p;
	if ( name )
	{
		for ( p = alias_table ; p ; p = p->next() )
		{
			if ( p->name == name )
			{
				delete p;
				return;
			}
		}
		printe(ERR_no_alias, E_NONE, name);
	}
}

// convert a list of tokens into a character string

void
print_tok_list( register Token *t, Buffer *buf )
{
	buf->clear();

	while ( t )
	{
		buf->add(t->print());
		t = t->next();
	}
	return;
}

// convert non-printing characters into printable
// escape sequences

static char *
FmtByte(int c)
{	static char buf[8];

	switch( c &= 0xFF ){
	case '\0'	: return "\\0";	
	case '\a'	: return "\\a";
	case '\b'	: return "\\b";	
	case '\f'	: return "\\f";	
	case '\n'	: return "\\n";	
	case '\r'	: return "\\r";	
	case '\t'	: return "\\t";	
	case '\v'	: return "\\v";	
	case ' '	: return " ";	
	case '\''	: return "\\\'";
	case '\"'	: return "\\\"";
	case '\\'	: return "\\\\";
	}
	sprintf( buf, isascii(c)&&isprint(c)?"%c":"\\%03o", c );
 	return buf;
}

// produce a printable (escape sequences expanded)
// version of the character string

// sbuf is used by several functions in the scanner, especially
// Scanner::scan.  Since scan is called frequently, we want to
// avoid the overhead of constantly retrieving and freeing
// one of our pool buffers.

static Buffer	sbuf;

char *
fmtstring(const char *r)
{
	register const unsigned char *raw = (const unsigned char *)r;

	sbuf.clear();

	sbuf.add( '"' );
	if( raw )
	{
		while( *raw )
			sbuf.add( FmtByte( *raw++ ));
	}
	sbuf.add( '"' );
	return (char *)sbuf;
}

static char *
backslash( char *str )	// convert backslash escapes in place
{
	register unsigned char *p = (unsigned char *)str;
	register unsigned char *q = p;
	register int	c;
	int		i, n;
	// the escape sequences recognized are as given in
	// the ANSI C standard

	while ( *p )
	{
		if ( *p == '\\' )
		{
			p++;	
			c = *p++;
			switch ( c )
			{
			default: 	*q++ = c;	break;
			case 'a':	*q++ = '\a';	break;
			case 'v':	*q++ = '\v';	break;
			case 'n':	*q++ = '\n';	break;
			case 't':	*q++ = '\t';	break;
			case 'b':	*q++ = '\b';	break;
			case 'r':	*q++ = '\r';	break;
			case 'f':	*q++ = '\f';	break;

			// octal escapes look like \d, \dd, or \ddd
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				for ( i = 3, n = 0; i; --i )
				{
					if ( c < '0' || c > '7' )
					{
						p--;
						break;
					}
					n = (n<<3) + (c - '0');
					c = *p++;
				}
				*q++ = n;
				break;

			// hex escapes are \xd... (no limit on digits)
			case 'x':
				for (c = *p++, n = 0; isxdigit(c); c = *p++)
				{
					n = (n<<4);
					n += isdigit(c) ? (c - '0')
						: 10 + (islower(c) ? (c - 'a')
						: (c - 'A'));
				}
				*q++ = n;
				p--;
				break;
			}
		} else
			*q++ = *p++;
	}
	*q = 0;
	return str;
}

#if DEBUG

// create a printable version (for debugging) version
// of the Token, including the Token type

char *
Token::dump()
{
	register char *name = 0;

	sbuf.clear();

	if ( !this )
		return "(Token *) 0";

	char xbuf[32];
	sprintf(xbuf, "%2d:%-3d ", _line, _pos);
	sbuf.add(xbuf);

	switch( _op )
	{
		case DEC:	name = "DEC";	break;
		case HEX:	name = "HEX";	break;
		case OCT:	name = "OCT";	break;
		case CHARCONST:	name = "CHARCONST"; break;
		case STRING:	name = "STRING"; break;
		case NAME:	name = "NAME";	break;
		case NL:	name = "NL";	break;
		case NON_ASCII_SEQ: name = "NON_ASCII_SEQ"; break;
		case WHITESPACE:name = "WS";	break;
		case NoTok:	name = "NoTok";	break;
		case BADTOKEN:	name = "BADTOKEN"; break;
		case BADSTRING:	name = "BADSTRING"; break;
		case BADNUMBER:	name = "BADNUMBER"; break;
		case BADCHAR:	name = "BADCHAR"; break;
		default:			break;
	}

	if (name)
	{
		sprintf(xbuf, "%-12s ", name);
		sbuf.add(xbuf);
	}

	sbuf.add ( print() );
	return (char *)sbuf;
}
#endif

// create a printable version of the Token, without
// the Token type - this is mainly for printing aliases
// and expressions

char *
Token::print()
{
	static char val[32];

	if ( !this )
		return "";

	switch( _op )
	{
		case DEC:
			sprintf(val, "%lu", _value._val);
			return val;

		case BADCHAR:
			// never saw a closing '
			sbuf.clear();
			sbuf.add('\'');
			if (_raw && *_raw)
				sbuf.add(_raw);
			else
				sbuf.add("\\0");
			return (char *)sbuf;

		case BADSTRING:
			// never saw a closing "
			sbuf.clear();
			sbuf.add('\"');
			sbuf.add(_raw);
			return (char *)sbuf;
			
		case CHARCONST:
			sbuf.clear();
			sbuf.add('\'');
			if (_raw && *_raw)
				sbuf.add(_raw);
			else
				sbuf.add("\\0");
			sbuf.add('\'');
			return (char *)sbuf;

		case STRING:
			sbuf.clear();
			sbuf.add('"');
			sbuf.add(_raw);
			sbuf.add('"');
			return (char *)sbuf;

		case HEX:
		case OCT:
		case BADTOKEN:
		case BADNUMBER:
		case NON_ASCII_SEQ:
			return _raw;

		case DEBUG_NAME:
		case NAME:
		case USER_NAME:
			return _value._str;

		case AMPERSAND:	return "&";
		case ARROW:	return "->";
		case AT:	return "@";
		case BACKSLASH:	return "\\";
		case BANG:	return "!";
		case BAR:	return "|";
		case COLON:	return ":";
		case COMMA:	return ",";
		case DOLLAR:	return "$";
		case DOT:	return ".";
		case DQUOTE:	return "\"";
		case EQUAL:	return "=";
		case GRAVE:	return "`";
		case GREATER:	return ">";
		case GREATEREQUAL:	return ">=";
		case HAT:	return "^";
		case LBRACK:	return "[";
		case LCURLY:	return "{";
		case LESS:	return "<";
		case LPAREN:	return "(";
		case MINUS:	return "-";
		case NL:	return "\\n";
		case PERCENT:	return "%";
		case PLUS:	return "+";
		case POUND:	return "#";
		case QUESTION:	return "?";
		case RBRACK:	return "]";
		case RCURLY:	return "}";
		case RPAREN:	return ")";
		case SEMI:	return ";";
		case SLASH:	return "/";
		case SQUOTE:	return "\'";
		case STAR:	return "*";
		case TILDE:	return "~";
		case WHITESPACE:return " ";

		case NoTok:
		default:
			sprintf(val, "<op=%d>", _op);
			return val;
	}
}

char *
Token::resolved_string()	
{ 
	switch( _op )
	{
		case STRING:
		case CHARCONST:
		case BADCHAR:
		case BADSTRING:
			if (!_value._str)
				_value._str = ::str(backslash(_raw));
			return _value._str;
		default:
			return _value._str; 
	}
}

Token::Token(Token *t)
{	_op = t->_op;
	_line = t->_line;
	_pos = t->_pos; 
	_raw = t->_raw ? makestr(t->_raw) : 0;
	_value._val = t->_value._val;
}

Token::~Token()
{
	delete _raw;
	delete next();
}

Token *
Token::next()
{
	return (Token *)Link::next();
}

Token *
Token::prev()
{
	return (Token *)Link::prev();
}

// create a complete copy of a Token list

Token *
Token::clone()
{
	register Token *oldt = this;
	Token *prev = 0;
	Token *head = 0;

	while( oldt )
	{
		register Token *newt = new Token(oldt);
		if ( !head )
			head = newt;
		else
			newt->append( prev );
		prev = newt;
		oldt = oldt->next();
	}

	return head;
}

// replace the Token in its linked list
// with the list of Tokens in tl
// aliases that take positional arguments will
// use the original positional in place of the aliased token

Token *
Token::subst( register Token *tl )
{
	Token *head = 0;
	Token *oldt = tl;
	Token *backp = prev();
	Token *nextp = next();
	Token *savep = nextp;
	Token *lastp = 0;
	long  maxpos = 0;

	for( ; oldt; oldt = oldt->next())
	{
		register Token *newt;
		if (oldt->op() == DOLLAR && oldt->next())
		{
			int i;
			Token	*actual = savep;
			while(actual && actual->op() == WHITESPACE)
				actual = actual->next();

			switch(oldt->next()->op())
			{
			case POUND:
				// $# - number of actual parameters
				i = 0;
				oldt = oldt->next();
				while(actual && actual->op() != NL
					&& actual->op() != POUND)
				{
					i++;
					// treat stream of tokens as one
					// parameter until we hit whitespace
					while(actual && 
						actual->op() != WHITESPACE
						&& actual->op() != NL 
						&& actual->op() != POUND
						&& actual->op() != LCURLY)
						actual = actual->next();

					// now skip whitespace
					while(actual &&
						actual->op() == WHITESPACE)
						actual = actual->next();
				}
				nextp = actual;
				maxpos = i;
				newt = new Token(DEC, _pos, _line);
				newt->_value._val = i;
				if ( !head )
					head = newt;
				else
					newt->append( lastp );
				lastp = newt;
				continue;
			case STAR:
				// $*  - add all arguments
				// separated by single whitespace
				oldt = oldt->next();
				while(actual && actual->op() != NL && 
					actual->op() != POUND)
				{

					Token *lastws = 0;
					while (actual->op() == WHITESPACE)
					{
						// suppress multiple whitespace
						lastws = actual;
						actual = actual->next();
					}
					if (lastws)
						actual = lastws;
					else if (actual->op() == LCURLY)
					{
						// new argument, insert whitespace
						newt = new Token(WHITESPACE, _pos, _line);
						if ( !head )
							head = newt;
						else
							newt->append( lastp );
						lastp = newt;
					}
					newt = new Token(actual);
					if ( !head )
						head = newt;
					else
						newt->append( lastp );
					newt->_pos = _pos;
					newt->_line = _line;
					lastp = newt;
					actual = actual->next();
				}
				nextp = actual;
				maxpos = INT_MAX;
				continue;
			case DEC:
				// positional parameter
				long  position;
				oldt = oldt->next();
				position = oldt->val();
				for(i = 1; i < position; i++)
				{
					// treat stream of tokens as one
					// parameter until we hit whitespace
					while(actual && 
						actual->op() != WHITESPACE
						&& actual->op() != NL 
						&& actual->op() != POUND
						&& actual->op() != LCURLY)
						actual = actual->next();

					// now skip whitespace
					while(actual &&
						actual->op() == WHITESPACE)
						actual = actual->next();
				}
				if (!actual)
					break;
				while(actual && 
					actual->op() != WHITESPACE
					&& actual->op() != NL 
					&& actual->op() != POUND
					&& actual->op() != LCURLY)
				{
					newt = new Token(actual);
					if ( !head )
						head = newt;
					else
						newt->append( lastp );
					lastp = newt;
					newt->_pos = _pos;
					newt->_line = _line;
					actual = actual->next();
				}
				if (position > maxpos)
				{
					maxpos = position;
					nextp = actual;
				}
				continue;
			default:
				break;
			} // end switch
		}
		newt = new Token(oldt);
	
		if ( !head )
			head = newt;
		else
			newt->append( lastp );
		lastp = newt;
		newt->_pos = _pos;
		newt->_line = _line;
	}
	unlink();
	head->ljoin( backp );
	lastp->rjoin( nextp );
	return head;
}

// convert a name token into an integer-valued token
// this is needed to handle options with numeric arguments, like -c3,
// where the token was scanned as a NAME, value "c3"
// if it isn't a valid number, don't change it, the parser will
// and put out an error message
void
Token::makenum(char *in)
{
	int	base;
	unsigned char *s2;
	unsigned char *s = (unsigned char *)in;
	long	value;

	if (*s == '0')
	{
		++s;
		base = 8;
	}
	else
		base = 10;

	if (base == 8 && (*s == 'x' || *s == 'X'))
	{
		base = 16;
		++s;
	}
	value = strtoul((const char *)s, (char **)&s2, base);
	if (s2 && !*s2)
	{
		if (base == 10)
		{
			_op = DEC;
			_value._val = value;
		}
		else
		{
			_raw = makestr((char *)s);
			_value._val = 0;
			if (base == 8)
				_op = OCT;
			else
				_op = HEX;
		}
	}
}

unsigned long
Token::val()
{
	switch(_op)
	{
	default:
		return(_value._val);
	case HEX:
		return(strtoul(_raw, 0, 16));
	case OCT:
		return(strtoul(_raw, 0, 8));
	}
}

void
Scanner::clear()
{
	token_head = 0;
	token_tail = 0;
	lineno = 1;
}

// Once EOF is returned by GETNEXT, pushback will not backup.  EOF will
// continue to be returned.
#define GETNEXT()	((line && *line) ? (++tokbegin, *line++) : (line=0,EOF))
#define NEXT()		((line && *line) ? *line : EOF)
#define PUSHBACK()	((line) ? (--tokbegin, --line) : 0)

// scan tokenizes an entire line and creates a linked list of tokens
// any invalid tokens are given an invalid token code and are dealt
// with in the parser - tokens not recognized here may be language
// specific and ok in an expression

void
Scanner::scan( const char *l )
{
	extern int PromptLen();
	int tokbegin = PromptLen();

	const unsigned char	*line = (const unsigned char *)l;

	clear();
	sbuf.clear();

	for (;;)
	{
		register int base = 0;
		register int c;

		c = GETNEXT();
		if( c == EOF )
		{
			return;
		}

		register Token *t = new Token(NoTok, lineno, tokbegin);
		if ( token_head )
			t->append( token_tail );
		else
			token_head = t;
		token_tail = t;

		switch( c )
		{
		case ' ':
		case '\v':
		case '\r':
		case '\t':
			while ( isspace(c) )
			{
				// newline is a space character, but is
				// a different token
				if ( c == '\n' )
					break;
				// bump to the next tab stop
				else if ( c == '\t' )
				{
					if (tokbegin & 7)
						tokbegin = (tokbegin+7)&~7;
				}
				c = GETNEXT();
			}
			PUSHBACK();
			t->_op = WHITESPACE;
			break;

		case '\n':
			++lineno;
			tokbegin = PromptLen();
			t->_op = NL;
			break;

		// convert \n into whitespace and continue to tokenize
		// the next line of input.  Dealing with the backslash
		// here means the parser doesn't have to.
		case '\\':
			c = GETNEXT();
			if ( c == '\n' )
			{
				t->_op = WHITESPACE;
				++lineno;
				tokbegin = PromptLen();
				if (GETNEXT() != EOF)
					PUSHBACK();
				else
				{
					InputPrompt = MORE_PROMPT;
					if ((line = 
						(const unsigned char *)GetLine()) == 0)
					{
						// special case of \nEOF
						printe(ERR_unfinished_cmd,
							E_ERROR);
						delete token_head;
						token_head = 0;
						return;
					}
					if (InputEcho())
						printm(MSG_input_line, Sprompt, (const char *)line);
				}
			}
			else
			{
				t->_op = BACKSLASH;
				PUSHBACK();
			}
			break;

		case '&':	t->_op = AMPERSAND;	break;
		case '@':	t->_op = AT;		break;
		case '!':	t->_op = BANG;		break;
		case '|':	t->_op = BAR;		break;
		case ':':	t->_op = COLON;		break;
		case ',':	t->_op = COMMA;		break;
		case '.':	t->_op = DOT;		break;
		case '=':	t->_op = EQUAL;		break;
		case '`':	t->_op = GRAVE;		break;
		case '^':	t->_op = HAT;		break;
		case '[':	t->_op = LBRACK;	break;
		case '{':	t->_op = LCURLY;	break;
		case '<':	t->_op = LESS;		break;
		case '(':	t->_op = LPAREN;	break;
		case '+':	t->_op = PLUS;		break;
		case '#':	t->_op = POUND;		break;
		case '?':	t->_op = QUESTION;	break;
		case ']':	t->_op = RBRACK;	break;
		case '}':	t->_op = RCURLY;	break;
		case ')':	t->_op = RPAREN;	break;
		case ';':	t->_op = SEMI;		break;
		case '/':	t->_op = SLASH;		break;
		case '*':	t->_op = STAR;		break;
		case '~':	t->_op = TILDE;		break;

		case '>':
			c = GETNEXT();
			if (c == '=')
				t->_op = GREATEREQUAL;
			else
			{
				PUSHBACK();
				t->_op = GREATER;
			}
			break;
		case '-':
			c = GETNEXT();
			if (c == '>')
				t->_op = ARROW;
			else
			{
				PUSHBACK();
				t->_op = MINUS;
			}
			break;

		case '%':
		case '$':
		{
			int	curop = c;
			c = GETNEXT();
			if (!isalpha(c) && (c != '_'))
			{
				PUSHBACK();
				t->_op = (curop == '$') ? DOLLAR : PERCENT;
				break;
			}
			sbuf.add(curop);
			do
			{
				sbuf.add(c);
				c = GETNEXT();
			} while( isalnum(c) || (c == '_'));
			PUSHBACK();
			t->_value._str = str((char *)sbuf);
			sbuf.clear();
			t->_op = (curop == '$') ? USER_NAME: DEBUG_NAME;
			break;
		}

		case '\'':
		case '"':
		{
			// if previous token was a backslash,
			// we handle quotes as just regular characters
			if (t->prev() && t->prev()->op() == BACKSLASH)
			{
				t->_op = (c == '\'') ? SQUOTE : DQUOTE;
				break;
			}
			// strings and character constants are handled the
			// same way here, since character constants may
			// be multi-byte escape sequences
			int quote = c;
			c = GETNEXT();
			while ( c && c != EOF && c != quote )
			{
				if ( c == '\\' )
				{
					c = GETNEXT();
					if (c == '\n')
					{
						// we process \\n here
						++lineno;
						tokbegin = PromptLen();
						InputPrompt = MORE_PROMPT;
						if ((line = 
							(const unsigned char *)GetLine()) == 0)
						{
							// special case of \nEOF
							printe(ERR_unfinished_cmd, E_ERROR);
							delete token_head;
							token_head = 0;
							return;
						}
						if (InputEcho())
							printm(MSG_input_line,
								Sprompt, (const char *)line);
					}
					else 
					{
						sbuf.add('\\');
						sbuf.add(c);
					}
				}
				else if ((NEXT() == EOF) && (c != quote))
				{
					// bad string or char constant
					// check if last char read
					// is a delimiter so we 
					// don't lose context
					switch(c)
					{
						default:
							break;
						case '\n':
						case ';':
						case '#':
						case '{':
						case '[':
						case '(':
							PUSHBACK();
							break;
					}
					break;
				}
				else
					sbuf.add(c);
				c = GETNEXT(); 
			}
			t->_raw = makestr((char *)sbuf);
			sbuf.clear();

			if ( c == quote )
			{
				t->_op = (quote == '"') ? STRING : CHARCONST;
			}
			else
			{
				t->_op = (quote == '"') ? BADSTRING
					: BADCHAR;
			}
			break;
		}

		case '0':
			if ( base == 0 )
				base = 8;
			/* fall through */
		case '1':	case '2':	case '3':
		case '4':	case '5':	case '6':
		case '7':	case '8':	case '9':
			sbuf.add(c);
			if ( base == 0 )
				base = 10;
			c = GETNEXT();
			if ( base == 8 && (c == 'x' || c == 'X') )
			{
				base = 16;
				sbuf.add(c);
				c = GETNEXT();
			}

			while( isxdigit(c) )
			{
				if ( base != 16 && isalpha(c))
					break;
				sbuf.add(c);
				c = GETNEXT();
			}
			PUSHBACK();

			char	*p1, *p2;
			unsigned long	ul;
			p1 = (char *)sbuf;
			errno = 0;
			ul = strtoul( p1, &p2, base );
			if ( p2 - p1 + 1 != sbuf.size()
				|| (ul == ULONG_MAX && errno == ERANGE))
			{
				t->_op = BADNUMBER;
				t->_raw = makestr(p1);
			}
			else
			{
				if (base == 10)
				{
					t->_op = DEC;
					t->_value._val = ul;
				}
				else
				{
					t->_raw = makestr(p1);
					if (base == 8)
						t->_op = OCT;
					else
						t->_op = HEX;
				}
			}
			sbuf.clear();
			break;

		default:
			if ( !isalpha(c) && (c != '_') )
			{
				if (!isascii(c))
				{
					// sequence of one or more
					// non-ascii chars - could
					// be multibyte chars
					do {
						sbuf.add(c);
						c = GETNEXT();
					} while(!isascii(c));
					PUSHBACK();
					t->_op = NON_ASCII_SEQ;
				}
				else
				{
					sbuf.add(c);
					t->_op = BADTOKEN;
				}
				t->_raw = makestr((char *)sbuf);
				sbuf.clear();
				break;
			}

			// identifiers are C specific
			// this does not accept general assembly language
			// identifier syntax - like '$' or '.' - in names,
			// or other forms from other languages

			while( isalnum(c) || (c == '_'))
			{
				sbuf.add(c);
				c = GETNEXT();
			}
			PUSHBACK();
			t->_value._str = str((char *)sbuf);
			sbuf.clear();
			t->_op = NAME;
			break;
		}
	}
}
