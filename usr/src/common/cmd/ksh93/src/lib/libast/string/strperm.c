#ident	"@(#)ksh93:src/lib/libast/string/strperm.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * apply file permission expression expr to perm
 *
 * each expression term must match
 *
 *	[ugoa]*[-&+|=]?[rwxst0-7]*
 *
 * terms may be combined using ,
 *
 * if non-null, e points to the first unrecognized char in expr
 */

#include <ast.h>
#include <ls.h>
#include <modex.h>

int
strperm(const char* aexpr, char** e, register int perm)
{
	register char*	expr = (char*)aexpr;
	register int	c;
	register int	typ;
	register int	who;
	int		num;
	int		op;

	for (;;)
	{
		op = num = who = typ = 0;
		for (;;)
		{
			switch (c = *expr++)
			{
			case 'u':
				who |= S_ISVTX|S_ISUID|S_IRWXU;
				continue;
			case 'g':
				who |= S_ISVTX|S_ISGID|S_IRWXG;
				continue;
			case 'o':
				who |= S_ISVTX|S_IRWXO;
				continue;
			case 'a':
				who = S_ISVTX|S_ISUID|S_ISGID|S_IRWXU|S_IRWXG|S_IRWXO;
				continue;
			default:
				if (c >= '0' && c <= '7') c = '=';
				expr--;
				/*FALLTHROUGH*/
			case '=':
			case '+':
			case '|':
			case '-':
			case '&':
				op = c;
				for (;;)
				{
					switch (c = *expr++)
					{
					case 'r':
						typ |= S_IRUSR|S_IRGRP|S_IROTH;
						continue;
					case 'w':
						typ |= S_IWUSR|S_IWGRP|S_IWOTH;
						continue;
					case 'X':
						if (op != '+' || !S_ISDIR(perm) && !(perm & (S_IXUSR|S_IXGRP|S_IXOTH))) continue;
						/*FALLTHROUGH*/
					case 'x':
						typ |= S_IXUSR|S_IXGRP|S_IXOTH;
						continue;
					case 's':
						typ |= S_ISUID|S_ISGID;
						continue;
					case 't':
						typ |= S_ISVTX;
						continue;
					case '=':
					case '+':
					case '|':
					case '-':
					case '&':
					case ',':
					case 0:
						if (who) typ &= who;
						switch (op)
						{
						default:
							if (who) perm &= ~who;
							else perm = 0;
							/*FALLTHROUGH*/
						case '+':
						case '|':
							perm |= typ;
							typ = 0;
							break;
						case '-':
							perm &= ~typ;
							typ = 0;
							break;
						case '&':
							perm &= typ;
							typ = 0;
							break;
						}
						switch (c)
						{
						case '=':
						case '+':
						case '|':
						case '-':
						case '&':
							op = c;
							typ = 0;
							continue;
						}
						if (c) break;
						/*FALLTHROUGH*/
					default:
						if (c < '0' || c > '7')
						{
							if (e) *e = expr - 1;
							if (typ)
							{
								if (who)
								{
									typ &= who;
									perm &= ~who;
								}
								perm |= typ;
							}
							return(perm);
						}
						num = (num <<= 3) | (c - '0');
						if (*expr < '0' || *expr > '7')
						{
							typ |= modei(num);
							num = 0;
						}
						continue;
					}
					break;
				}
				break;
			}
			break;
		}
	}
}
