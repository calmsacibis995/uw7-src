#ident	"@(#)ksh93:src/lib/libast/include/tm.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * time conversion support definitions
 */

#ifndef _TM_H
#define _TM_H

#if _DLL_INDIRECT_DATA && !_DLL
#define tm_data		(*_tm_data_)
#define tm_info		(*_tm_info_)
#else
#define tm_data		_tm_data_
#define tm_info		_tm_info_
#endif

#include <ast.h>
#include <times.h>

#define tmset(z)	do{if(!tm_info.zone||(z)&&tm_info.zone!=(z)||!(z)&&tm_info.zone!=tm_info.local)tminit(z);}while(0)

#define TM_ADJUST	(1<<0)		/* local doesn't do leap secs	*/
#define TM_LEAP		(1<<1)		/* do leap seconds		*/
#define TM_UTC		(1<<2)		/* universal coordinated ref	*/

#define TM_DST		(-60)		/* default minutes for DST	*/
#define TM_LOCALZONE	(25 * 60)	/* use local time zone offset	*/
#define TM_MAXLEAP	1		/* max leap secs per leap	*/

/*
 * these indices must agree with tm_dform[]
 */

#define TM_MONTH_3	0
#define TM_MONTH	12
#define TM_DAY_3	24
#define TM_DAY		31
#define TM_TIME		38
#define TM_DATE		39
#define TM_DEFAULT	40
#define TM_MERIDIAN	41

#define TM_UT		43
#define TM_DT		47
#define TM_SUFFIXES	51
#define TM_PARTS	55
#define TM_HOURS	62
#define TM_DAYS		66
#define TM_LAST		69
#define TM_THIS		72
#define TM_NEXT		75
#define TM_EXACT	78
#define TM_NOISE	81

#define TM_NFORM	85

typedef struct				/* leap second info		*/
{
	time_t		time;		/* the leap second event	*/
	int		total;		/* inclusive total since epoch	*/
} Tm_leap_t;

typedef struct				/* time zone info		*/
{
	char*		type;		/* type name			*/
	char*		standard;	/* standard time name		*/
	char*		daylight;	/* daylight or summertime name	*/
	short		west;		/* minutes west of GMT		*/
	short		dst;		/* add to tz.west for DST	*/
} Tm_zone_t;

typedef struct				/* tm library readonly data	*/
{
	char**		format;		/* default TM_* format strings	*/
	char*		lex;		/* format lex type classes	*/
	char*		digit;		/* output digits		*/
	short*		days;		/* days in month i		*/
	short*		sum;		/* days in months before i	*/
	Tm_leap_t*	leap;		/* leap second table		*/
	Tm_zone_t*	zone;		/* alternate timezone table	*/
} Tm_data_t;

typedef struct				/* tm library global info	*/
{
	char*		deformat;	/* TM_DEFAULT override		*/
	int		flags;		/* flags			*/
	char**		format;		/* current format strings	*/
	Tm_zone_t*	date;		/* timezone from last tmdate()	*/
	Tm_zone_t*	local;		/* local timezone		*/
	Tm_zone_t*	zone;		/* current timezone		*/
} Tm_info_t;

typedef struct tm Tm_t;

extern Tm_data_t	tm_data;
extern Tm_info_t	tm_info;

extern time_t		tmdate(const char*, char**, time_t*);
extern Tm_t*		tmfix(Tm_t*);
extern char*		tmfmt(char*, size_t, const char*, time_t*);
extern char*		tmform(char*, const char*, time_t*);
extern int		tmgoff(const char*, char**, int);
extern void		tminit(Tm_zone_t*);
extern time_t		tmleap(time_t*);
extern int		tmlex(const char*, char**, char**, int, char**, int);
extern Tm_t*		tmmake(time_t*);
extern char*		tmpoff(char*, const char*, int, int);
extern time_t		tmtime(Tm_t*, int);
extern Tm_zone_t*	tmtype(const char*, char**);
extern int		tmword(const char*, char**, const char*, char**, int);
extern Tm_zone_t*	tmzone(const char*, char**, const char*, int*);

#endif
