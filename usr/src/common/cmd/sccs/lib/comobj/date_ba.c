#ident	"@(#)sccs:lib/comobj/date_ba.c	6.4"
# include	"../../hdr/defines.h"
# define DO2(p,n,c)	*p++ = ((char)(((n>=100)?(n-100):(n))/10) + '0'); *p++ = ( (char) (n%10) + '0'); *p++ = c;


char *
date_ba(bdt,adt)
long *bdt;
char *adt;
{
	register struct tm *lcltm;
	register char *p;
	struct tm *localtime();

	lcltm = localtime(bdt);
	p = adt;
	DO2(p,lcltm->tm_year,'/');
	DO2(p,(lcltm->tm_mon + 1),'/');
	DO2(p,lcltm->tm_mday,' ');
	DO2(p,lcltm->tm_hour,':');
	DO2(p,lcltm->tm_min,':');
	DO2(p,lcltm->tm_sec,0);
	return(adt);
}
