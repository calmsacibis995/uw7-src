#ifndef Stmt_h
#define Stmt_h
#ident	"@(#)debugger:inc/common/Stmt.h	1.2"

struct Stmt {
	long	line;
	short	pos;
	char	first_instr;	// non-zero if at first instr of stmt

		Stmt()	{ line = 0; pos = 0; first_instr = 0; }

	int	is_unknown()	{	return ( line == 0 );	}
	void	unknown()	{	line = 0; pos = 0;
					first_instr = 0;	}
	int	operator==( Stmt & st )	{ return (line == st.line) &&
						 (pos == st.pos);	}
	int	at_first_instr()	{ return(first_instr != 0); }
};

#endif

// end of Stmt.h
