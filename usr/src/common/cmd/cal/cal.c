#ident	"@(#)cal:cal.c	1.3.3.2"
#include <sys/types.h>
#include <sys/euc.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <pfmt.h>
#include <stdio.h>

#define	 WEEKNAME	"%a"
#define	 MONNAMEF	"%B"
#define	 MONNAMEA	"%b"
#define	 WEEKLEN	50

eucwidth_t	wp;
short int 	eucw1,eucw2,eucw3;
short int 	scrw1,scrw2,scrw3;
char	dayw[WEEKLEN];
char	monw[WEEKLEN];
char	string[450];
extern struct tm *localtime();
extern long time();
struct tm *thetime;
long timbuf;
main(argc, argv)
char *argv[];
{
	register y, i, j;
	int m;
	void	weekedit();
	void	monedit();


	(void)setlocale(LC_ALL,"");
	(void)setcat("uxue");
	(void)setlabel("UX:cal");
	(void)getwidth(&wp);
	eucw1 = wp._eucw1;
	eucw2 = wp._eucw2+1;
	eucw3 = wp._eucw3+1;
	scrw1 = wp._scrw1;
	scrw2 = wp._scrw2;
	scrw3 = wp._scrw3;

	timbuf = time(&timbuf);
	thetime = localtime(&timbuf);

	if (argc>1 && strcmp(argv[1],"--") == 0){
		argc--;
		argv++;
	}
	switch(argc) {
	case 1:
		m = thetime->tm_mon + 1;
		y = thetime->tm_year + 1900;
		break;
	case 2:
		goto xlong;
	case 3:
		m = number(argv[1]);
		y = number(argv[2]);
		break;
	default:
		pfmt(stderr,MM_ACTION,":58:usage: cal [ [month] year ]\n");
		exit(0);
	}

/*
 *	print out just month
 */

	if(m<1 || m>12)
		goto badarg;
	if(y<1 || y>9999)
		goto badarg;
	thetime->tm_mon = m-1;
	memset(monw, ' ', sizeof(monw));
	(void)strftime(monw,sizeof(monw),MONNAMEF,thetime);
	printf("   %s %u\n", monw, y);
	(void)weekedit();
	printf("%s\n", dayw);
	cal(m, y, string, 24);
	for(i=0; i<6*24; i+=24)
		pstr(string+i, 24);
	exit(0);

/*
 *	print out complete year
 */

xlong:
	y = number(argv[1]);
	if(y<1 || y>9999)
		goto badarg;
	(void)weekedit();
	printf("\n\n\n");
	printf("				%u\n", y);
	printf("\n");
	for(i=0; i<12; i+=3) {
		for(j=0; j<6*72; j++)
			string[j] = '\0';
		(void)monedit(i);
		printf("%s", &monw[0]);
		(void)monedit(i+1);
		printf(" %s", &monw[0]);
		(void)monedit(i+2);
		printf(" %s\n", &monw[0]);
		printf("%s  %s  %s\n", dayw, dayw, dayw);
		cal(i+1, y, string, 72);
		cal(i+2, y, string+23, 72);
		cal(i+3, y, string+46, 72);
		for(j=0; j<6*72; j+=72)
			pstr(string+j, 72);
	}
	printf("\n\n\n");
	exit(0);

badarg:
	pfmt(stderr,MM_ERROR,":59:Bad argument\n");
}

number(str)
char *str;
{
	register n, c;
	register char *s;

	n = 0;
	s = str;
	while(c = *s++) {
		if(c<'0' || c>'9')
			return(0);
		n = n*10 + c-'0';
	}
	return(n);
}

pstr(str, n)
char *str;
{
	register i;
	register char *s;

	s = str;
	i = n;
	while(i--)
		if(*s++ == '\0')
			s[-1] = ' ';
	i = n+1;
	while(i--)
		if(*--s != ' ')
			break;
	s[1] = '\0';
	printf("%s\n", str);
}

char	mon[] = {
	0,
	31, 29, 31, 30,
	31, 30, 31, 31,
	30, 31, 30, 31,
};

cal(m, y, p, w)
char *p;
{
	register d, i;
	register char *s;

	s = p;
	d = jan1(y);
	mon[2] = 29;
	mon[9] = 30;

	switch((jan1(y+1)+7-d)%7) {

	/*
	 *	non-leap year
	 */
	case 1:
		mon[2] = 28;
		break;

	/*
	 *	1752
	 */
	default:
		mon[9] = 19;
		break;

	/*
	 *	leap year
	 */
	case 2:
		;
	}
	for(i=1; i<m; i++)
		d += mon[i];
	d %= 7;
	s += 3*d;
	for(i=1; i<=mon[m]; i++) {
		if(i==3 && mon[m]==19) {
			i += 11;
			mon[m] += 11;
		}
		if(i > 9)
			*s = i/10+'0';
		s++;
		*s++ = i%10+'0';
		s++;
		if(++d == 7) {
			d = 0;
			s = p+w;
			p = s;
		}
	}
}

/*
 *	return day of the week
 *	of jan 1 of given year
 */

jan1(yr)
{
	register y, d;

/*
 *	normal gregorian calendar
 *	one extra day per four years
 */

	y = yr;
	d = 4+y+(y+3)/4;

/*
 *	julian calendar
 *	regular gregorian
 *	less three days per 400
 */

	if(y > 1800) {
		d -= (y-1701)/100;
		d += (y-1601)/400;
	}

/*
 *	great calendar changeover instant
 */

	if(y > 1752)
		d += 3;

	return(d%7);
}

void
weekedit()
{
	int	i,j,k,l,len;
	char	buf[20];
	
	memset(dayw, ' ', sizeof(dayw));
	for( i=0,j=0; i<7; ++i){
		thetime->tm_wday=i;
		(void)strftime(buf,sizeof(buf),WEEKNAME,thetime);
		len = strlen(buf);
		for( k=0,l=0; k<2;){
			if (!wp._multibyte || ISASCII(buf[l]) ){
				if(len == 1){
					++j;
					++k;
				}
				if(buf[l] == '\0' || buf[l] == '\n'){
					buf[l] = ' ';
				}
				dayw[j]= buf[l];
				j += 1;
				l += 1;
				k += 1;
			}
			else if(ISSET2(buf[l] & 0xFF)){
				if(len == eucw2){
					++j;
					++k;
				}
				(void)strncpy(&dayw[j], &buf[l], eucw2);
				j += eucw2;
				l += eucw2;
				k += scrw2;
			}
			else if( ISSET3(buf[l] & 0xFF)){ 
				if(len == eucw3){
					++j;
					++k;
				}
				(void)strncpy(&dayw[j], &buf[l], eucw3);
				j += eucw3;
				l += eucw3;
				k += scrw3;
			}
			else{   
				if(len == eucw1 && scrw1 < 2){
					++j;
					++k;
				}
				(void)strncpy(&dayw[j], &buf[l], eucw1);
				j += eucw1;
				l += eucw1;
				k += scrw1;
			}
		}
		j += 1;
	}
	dayw[j--]= '\0';
	return;
}

void
monedit(mon)
int	mon;
{
	int	k,l;
	int	startp;
	char	buf[20];
	
	memset(monw, ' ', sizeof(monw));
	thetime->tm_mon=mon;
	(void)strftime(buf,sizeof(buf),MONNAMEA,thetime);
	k =0;
	l =0;
	while( buf[l] != '\0' && buf[l] != '\n' ){
		if ( !wp._multibyte || ISASCII(buf[l])){
			++k;
			++l;
		}
		else if(ISSET2(buf[l] & 0xFF)){
			l += eucw2;
			k += scrw2;
		}
		else if( ISSET3(buf[l] & 0xFF)){ 
			l += eucw3;
			k += scrw3;
		}
		else{   
			l += eucw1;
			k += scrw1;
		}
	}
		
	startp = 10 - k / 2;
	(void)strncpy(&monw[startp], buf, l);
	monw[22+l-k]= '\0';
	return;
}
