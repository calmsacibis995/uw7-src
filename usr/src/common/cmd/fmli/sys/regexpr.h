/*		copyright	"%c%" 	*/

#ident	"@(#)fmli:sys/regexpr.h	1.1"
#ifndef _REGEXPR_H
#define _REGEXPR_H
#define	NBRA 9
extern char	*braslist[NBRA];
extern char	*braelist[NBRA];
extern int nbra, regerrno, reglength;
extern char *loc1, *loc2, *locs;
#ifdef	__STDC__
extern int step(const char *string, const char *expbuf); 
extern int advance(const char *string, const char *expbuf);
extern char *compile(const char *instring, char *expbuf, char *endbuf);
#else
extern int step(); 
extern int advance();
extern char *compile();
#endif /* __STDC__ */
#endif /* _REGEXPR_H */
