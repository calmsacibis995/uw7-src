#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/misc.h	1.21"
#endif

#ifndef _MISC_H
#define _MISC_H

typedef char *			ADDR;

/*
 *	useful defines
 */
#define DIMENSION(x)		(sizeof(x)/sizeof(x[0]))
#define ELEMENT(x)		(x *)MALLOC(sizeof(x))
#define ARRAY(x, n)		(x *)MALLOC(sizeof(x) * (n))
#define MATCH(p, q)		(strcmp((p), (q)) == 0)
#define MATCHN(p, q, n)		(strncmp((p), (q), (int)(n)) == 0)
#define MIN(i, j)		((i) < (j) ? (i) : (j))
#define MAX(i, j)		((i) > (j) ? (i) : (j))
#define ABS(i)			((i) > 0 ? (i) : -(i))
#define BCOPY(p, q, n)		(bcopy((char *)(p), (char *)(q), (int)(n)))
#define STRNDUP(p, n)		(strncpy((char *) calloc((n+1), sizeof(char)), (char *)(p), (int)(n)))
#define CONCAT(p, q) \
	strcat( (char *)strcpy( (char *)calloc(strlen(p) + strlen(q) +1, sizeof(char)), (char *)p), (char *)q)  
#define SWITCH(p)		{ char *_s_ = p;  if (!_s_) { ;
#define CASE(p)			} else if (MATCH(_s_, p)) {
#define DEFAULT			} else {
#define ENDSWITCH		} }

#if defined(__STDC__)
# define Concat(x,y) x ## y
# define Concatq(x,y)   Quote(x ## y)
# define Quote(x) #x
#else
# define Concat(x,y) x/**/y
# define Concatq(x,y)   Quote(x/**/y)
# define Lq(x) "x
# define Rq(x) x"
# define Quote(x) Rq(Lq(x))
#endif

#define ERROR(x)		(void)fprintf x
/*
 *	debugging stuff
 */
#ifdef DEBUG
#define debug(x)		(void)fprintf x
#define trace(x)		(void)fprintf(stderr,\
					"%s: line = %d, file = %s\n",\
					x, __LINE__, __FILE__)
#else
#define debug(x)
#define trace(x)
#endif

/*
 *	externs in misc.c
 */
extern int		bcopy();

#if	defined(_WSMcomm_h)
extern Widget		CreateCaption(String, String, Widget );
extern void		AddToCaption( Widget, Widget );
extern void		BusyPeriod(Widget, Boolean);
#endif

#endif
