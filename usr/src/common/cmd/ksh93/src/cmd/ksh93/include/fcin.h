#ident	"@(#)ksh93:src/cmd/ksh93/include/fcin.h	1.1"
#pragma prototyped
#ifndef fcgetc
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * Fast character input with sfio text streams and strings
 *
 */

#include	<sfio.h>

typedef struct _fcin
{
	Sfio_t		*_fcfile;	/* input file pointer */
	unsigned char	*fcbuff;	/* pointer to input buffer */
	unsigned char	*fclast;	/* pointer to end of input buffer */
	unsigned char	*fcptr;		/* pointer to next input char */
	unsigned char	fcchar;		/* saved character */
	void (*fcfun)(Sfio_t*,const char*,int);	/* advance function */
} Fcin_t;

#define fcfile()	(_Fcin._fcfile)
#define fcgetc(c)	(((c=fcget()) || (c=fcfill())), c)
#define	fcget()		((int)(*_Fcin.fcptr++))
#define	fcpeek(n)	((int)_Fcin.fcptr[n])
#define	fcseek(n)	((char*)(_Fcin.fcptr+=(n)))
#define fcfirst()	((char*)_Fcin.fcbuff)
#define fcsopen(s)	(_Fcin._fcfile=(Sfio_t*)0,_Fcin.fcbuff=_Fcin.fcptr=(unsigned char*)(s))
#define fcsave(x)	(*(x) = _Fcin)
#define fcrestore(x)	(_Fcin = *(x))
extern int		fcfill(void);
extern int		fcfopen(Sfio_t*);
extern int		fcclose(void);
void			fcnotify(void(*)(Sfio_t*,const char*,int));

extern Fcin_t		_Fcin;		/* used by macros */

#endif /* fcgetc */
