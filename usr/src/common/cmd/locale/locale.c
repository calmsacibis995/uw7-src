#ident	"@(#)locale:locale.c	1.1"
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <pfmt.h>
#include <langinfo.h>

/* Until library support these */
#ifndef CHARCLASS
#define CHARCLASS 0x1000
#define DATECMD_FMT 0x1001
#define QUITEXPR 0x1002
#define QUITSTR 0x1003
#endif

#define MY_LC_COLLATE	0
#define MY_LC_CTYPE	1
#define	MY_LC_MESSAGES	2
#define MY_LC_MONETARY	3
#define	MY_LC_NUMERIC	4
#define	MY_LC_TIME	5
#define	BEGIN_KYWD	6
#define abday BEGIN_KYWD+0
#define abmon BEGIN_KYWD+1
#define alpha BEGIN_KYWD+2
#define alt_digits BEGIN_KYWD+3
#define am_pm BEGIN_KYWD+4
#define blank BEGIN_KYWD+5
#define charclass BEGIN_KYWD+6
#define cntrl BEGIN_KYWD+7
#define currency_symbol BEGIN_KYWD+8
#define d_fmt BEGIN_KYWD+9
#define d_t_fmt BEGIN_KYWD+10
#define datecmd_fmt BEGIN_KYWD+11
#define day BEGIN_KYWD+12
#define decimal_point BEGIN_KYWD+13
#define digit BEGIN_KYWD+14
#define era BEGIN_KYWD+15
#define era_d_fmt BEGIN_KYWD+16
#define era_d_t_fmt BEGIN_KYWD+17
#define era_t_fmt BEGIN_KYWD+18
#define my_frac_digits BEGIN_KYWD+19
#define graph BEGIN_KYWD+20
#define my_grouping BEGIN_KYWD+21
#define my_int_curr_symbol BEGIN_KYWD+22
#define my_int_frac_digits BEGIN_KYWD+23
#define line BEGIN_KYWD+24
#define lower BEGIN_KYWD+25
#define mon BEGIN_KYWD+26
#define my_mon_decimal_point BEGIN_KYWD+27
#define my_mon_grouping BEGIN_KYWD+28
#define my_mon_thousands_sep BEGIN_KYWD+29
#define my_n_cs_precedes BEGIN_KYWD+30
#define my_n_sep_by_space BEGIN_KYWD+31
#define my_n_sign_posn BEGIN_KYWD+32
#define my_negative_sign BEGIN_KYWD+33
#define noexpr BEGIN_KYWD+34
#define nostr BEGIN_KYWD+35
#define my_p_cs_precedes BEGIN_KYWD+36
#define my_p_sep_by_space BEGIN_KYWD+37
#define my_p_sign_posn BEGIN_KYWD+38
#define my_positive_sign BEGIN_KYWD+39
#define print BEGIN_KYWD+40
#define punct BEGIN_KYWD+41
#define space BEGIN_KYWD+42
#define t_fmt BEGIN_KYWD+43
#define t_fmt_ampm BEGIN_KYWD+44
#define thousands_sep BEGIN_KYWD+45
#define tolower BEGIN_KYWD+46
#define toupper BEGIN_KYWD+47
#define upper BEGIN_KYWD+48
#define xdigit BEGIN_KYWD+49
#define yesexpr BEGIN_KYWD+50
#define yesstr BEGIN_KYWD+51
#define	charmap	BEGIN_KYWD+52
#define quitstr BEGIN_KYWD+53
#define quitexpr BEGIN_KYWD+54


#define PROCESSED	0x1
#define	TITLE	0x2
#define NOOUTPUT 0x4
#define NOQUOTE	0x8

/* This array must be sorted in "strcmp" order.  See lookup(). */
struct kywd {
	char *name;
	int category;
	int sw_val;
	int output;
} locale_keywords[] = {
{ "LC_COLLATE", MY_LC_COLLATE, MY_LC_COLLATE, 0 },
{ "LC_CTYPE", MY_LC_CTYPE, MY_LC_CTYPE, 0 },
{ "LC_MESSAGES", MY_LC_MESSAGES, MY_LC_MESSAGES, 0 },
{ "LC_MONETARY", MY_LC_MONETARY, MY_LC_MONETARY, 0 },
{ "LC_NUMERIC", MY_LC_NUMERIC, MY_LC_NUMERIC, 0 },
{ "LC_TIME", MY_LC_TIME, MY_LC_TIME, 0 },
{ "abday", MY_LC_TIME, abday, 0 },
{ "abmon", MY_LC_TIME, abmon, 0 },
{ "alpha", MY_LC_CTYPE, alpha, NOOUTPUT },
{ "alt_digits", MY_LC_TIME, alt_digits, 0 },
{ "am_pm", MY_LC_TIME, am_pm, 0 },
{ "blank", MY_LC_CTYPE, blank, NOOUTPUT },
{ "charclass", MY_LC_CTYPE, charclass, 0 },
{ "charmap", BEGIN_KYWD, charmap, 0},
{ "cntrl", MY_LC_CTYPE, cntrl, NOOUTPUT },
{ "currency_symbol", MY_LC_MONETARY, currency_symbol, 0 },
{ "d_fmt", MY_LC_TIME, d_fmt, 0 },
{ "d_t_fmt", MY_LC_TIME, d_t_fmt, 0 },
{ "datecmd_fmt", MY_LC_TIME, datecmd_fmt, 0 },
{ "day", MY_LC_TIME, day, 0 },
{ "decimal_point", MY_LC_NUMERIC, decimal_point, 0 },
{ "digit", MY_LC_CTYPE, digit, NOOUTPUT },
{ "era", MY_LC_TIME, era, 0 },
{ "era_d_fmt", MY_LC_TIME, era_d_fmt, 0 },
{ "era_d_t_fmt", MY_LC_TIME, era_d_t_fmt, 0 },
{ "era_t_fmt", MY_LC_TIME, era_t_fmt, 0 },
{ "frac_digits", MY_LC_MONETARY, my_frac_digits, NOQUOTE },
{ "graph", MY_LC_CTYPE, graph, NOOUTPUT },
{ "grouping", MY_LC_NUMERIC, my_grouping, 0 },
{ "int_curr_symbol", MY_LC_MONETARY, my_int_curr_symbol, 0 },
{ "int_frac_digits", MY_LC_MONETARY, my_int_frac_digits, NOQUOTE },
{ "line", MY_LC_CTYPE, line, NOOUTPUT },
{ "lower", MY_LC_CTYPE, lower, NOOUTPUT },
{ "mon", MY_LC_TIME, mon, 0 },
{ "mon_decimal_point", MY_LC_MONETARY, my_mon_decimal_point, 0 },
{ "mon_grouping", MY_LC_MONETARY, my_mon_grouping, 0 },
{ "mon_thousands_sep", MY_LC_MONETARY, my_mon_thousands_sep, 0 },
{ "n_cs_precedes", MY_LC_MONETARY, my_n_cs_precedes, NOQUOTE },
{ "n_sep_by_space", MY_LC_MONETARY, my_n_sep_by_space, NOQUOTE },
{ "n_sign_posn", MY_LC_MONETARY, my_n_sign_posn, NOQUOTE },
{ "negative_sign", MY_LC_MONETARY, my_negative_sign, 0 },
{ "noexpr", MY_LC_MESSAGES, noexpr, 0 },
{ "nostr", MY_LC_MESSAGES, nostr, 0 },
{ "p_cs_precedes", MY_LC_MONETARY, my_p_cs_precedes, NOQUOTE },
{ "p_sep_by_space", MY_LC_MONETARY, my_p_sep_by_space, NOQUOTE },
{ "p_sign_posn", MY_LC_MONETARY, my_p_sign_posn, NOQUOTE },
{ "positive_sign", MY_LC_MONETARY, my_positive_sign, 0 },
{ "print", MY_LC_CTYPE, print, NOOUTPUT },
{ "punct", MY_LC_CTYPE, punct, NOOUTPUT },
{ "quitexpr", MY_LC_MESSAGES, quitexpr, 0 },
{ "quitstr", MY_LC_MESSAGES, quitstr, 0 },
{ "space", MY_LC_CTYPE, space, NOOUTPUT },
{ "t_fmt", MY_LC_TIME, t_fmt, 0 },
{ "t_fmt_ampm", MY_LC_TIME, t_fmt_ampm, 0 },
{ "thousands_sep", MY_LC_NUMERIC, thousands_sep, 0 },
{ "tolower", MY_LC_CTYPE, tolower, NOOUTPUT },
{ "toupper", MY_LC_CTYPE, toupper, NOOUTPUT },
{ "upper", MY_LC_CTYPE, upper, NOOUTPUT },
{ "xdigit", MY_LC_CTYPE, xdigit, NOOUTPUT },
{ "yesexpr", MY_LC_MESSAGES, yesexpr, 0 },
{ "yesstr", MY_LC_MESSAGES, yesstr, 0 }
};

#define	ABIT	0x1
#define	CBIT	0x2
#define	KBIT	0x4
#define	MBIT	0x8
#define	ERROR	0x10

unsigned int flags = 0;
struct lconv *lc;

void
kcprint(struct kywd *kp)
{
	if((flags & CBIT) && kp->category < BEGIN_KYWD &&
	  !(locale_keywords[kp->category].output & TITLE)) {
		printf("%s\n",locale_keywords[kp->category].name);
		locale_keywords[kp->category].output |= TITLE;
	}
	if(flags & KBIT) {
		printf("%s",kp->name);
		if(kp->output & NOOUTPUT)
			putchar('\n');
		else if(kp->output & NOQUOTE)
			printf("=");
		else
			printf("=\"");
	}
}

/* This routine assumes that there are only ascii characters in spchar and
that no part of a multibyte character looks like an ascii character that is not
in fact the ascii character itself */
void *
qoutput(char *str, char *spchar)
{
	while(*str) {
		if(strchr(spchar,*str) != NULL) {
			putchar('\\');
		}
		putchar(*str);
		str++;
	}
}

void
nlprint(int nptrs,...)
{
	va_list ap;
	va_start(ap, nptrs);
	while(nptrs-- > 0) {
			qoutput(nl_langinfo(va_arg(ap,int)), ";\"\\");
			
		if(nptrs > 0)
			putchar(';');
	}
	if(flags & KBIT)
		putchar('"');
	putchar('\n');
	va_end(ap);
}
		
void
grpprint(char *grp)
{
	if(*grp == '\0') {
		printf("-1\n");
		return;
	}
	for(;;) {
		if(*grp == CHAR_MAX) {
			printf("-1\n");
			return;
		}
		printf("\\x%x",*grp);
		grp++;
		if(*grp == '\0')
			break;
		printf("; ");
	}
	putchar('\n');

}

void
swkeyword(struct kywd *pk) 
{
	int i;
	switch(pk->sw_val) {
		case MY_LC_COLLATE:
			return;
		case MY_LC_CTYPE:
		case MY_LC_NUMERIC:
		case MY_LC_MONETARY:
		case MY_LC_MESSAGES:
		case MY_LC_TIME:
			for(i = BEGIN_KYWD; i < sizeof(locale_keywords)/sizeof(struct kywd); i++) {
				if(locale_keywords[i].category == pk->sw_val &&
					!(locale_keywords[i].output & PROCESSED)) 
						swkeyword(&locale_keywords[i]);
			}
			return;
	}
	kcprint(pk);
	switch(pk->sw_val) {
		case abday:
			nlprint(7,ABDAY_1,ABDAY_2,ABDAY_3,ABDAY_4,ABDAY_5,ABDAY_6,ABDAY_7);
			break;
		case abmon:
			nlprint(12,ABMON_1,ABMON_2,ABMON_3,ABMON_4,ABMON_5,ABMON_6,ABMON_7,ABMON_8,ABMON_9,ABMON_10,ABMON_11,ABMON_12);
			break;
		case alpha:
			break;
		case alt_digits:
			nlprint(1,ALT_DIGITS);
			break;
		case am_pm:
			nlprint(1,AM_STR,PM_STR);
			break;
		case blank:
			break;
		case charclass:
			nlprint(1,CHARCLASS);
			break;
		case charmap:
			nlprint(1,CODESET);
			break;
		case cntrl:
			break;
		case currency_symbol:
			nlprint(1,CRNCYSTR);
			break;
		case d_fmt:
			nlprint(1,D_FMT);
			break;
		case d_t_fmt:
			nlprint(1,D_T_FMT);
			break;
		case datecmd_fmt:
			nlprint(1,DATECMD_FMT);
			break;
		case day:
			nlprint(7,DAY_1,DAY_2,DAY_3,DAY_4,DAY_5,DAY_6,DAY_7);
			break;
		case decimal_point:
			nlprint(1,RADIXCHAR);
			break;
		case digit:
			break;
		case era:
			nlprint(1,ERA);
			break;
		case era_d_fmt:
			nlprint(1,ERA_D_FMT);
			break;
		case era_d_t_fmt:
			nlprint(1,ERA_D_T_FMT);
			break;
		case era_t_fmt:
			nlprint(1,ERA_T_FMT);
			break;
		case my_frac_digits:
			if(lc->frac_digits == CHAR_MAX)
				printf("-1\n");
			else
				printf("\\x%x\n",lc->frac_digits);
			break;
		case graph:
			break;
		case my_grouping:
			grpprint(lc->grouping);
			break;
		case my_int_curr_symbol:
			qoutput(lc->int_curr_symbol,";\"\\");
			if(flags & KBIT)
				putchar('"');
			putchar('\n');
			break;
		case my_int_frac_digits:
			if(lc->int_frac_digits == CHAR_MAX)
				printf("-1\n");
			else
				printf("\\x%x\n",lc->int_frac_digits);
			break;
		case line:
			break;
		case lower:
			break;
		case mon:
			nlprint(12,MON_1,MON_2,MON_3,MON_4,MON_5,MON_6,MON_7,MON_8,MON_9,MON_10,MON_11,MON_12);
			break;
		case my_mon_decimal_point:
			qoutput(lc->mon_decimal_point,";\"\\");
			if(flags & KBIT)
				putchar('"');
			putchar('\n');
			break;
		case my_mon_grouping:
			grpprint(lc->mon_grouping);
			break;
		case my_mon_thousands_sep:
			printf("\"%s\"\n",lc->mon_thousands_sep);
			if(flags & KBIT)
				putchar('"');
			putchar('\n');
			break;
		case my_n_cs_precedes:
			if(lc->n_cs_precedes == CHAR_MAX)
				printf("-1\n");
			else
				printf("\\x%x\n",lc->n_cs_precedes);
			break;
		case my_n_sep_by_space:
			if(lc->n_sep_by_space == CHAR_MAX)
				printf("-1\n");
			else
				printf("\\x%x\n",lc->n_sep_by_space);
			break;
		case my_n_sign_posn:
			if(lc->n_sign_posn == CHAR_MAX)
				printf("-1\n");
			else
				printf("\\x%x\n",lc->n_sign_posn);
			break;
		case my_negative_sign:
			qoutput(lc->negative_sign,";\"\\");
			if(flags & KBIT)
				putchar('"');
			putchar('\n');
			break;
		case noexpr:
			nlprint(1,NOEXPR);
			break;
		case nostr:
			nlprint(1,NOSTR);
			break;
		case my_p_cs_precedes:
			if(lc->p_cs_precedes == CHAR_MAX)
				printf("-1\n");
			else
				printf("\\x%x\n",lc->p_cs_precedes);
			break;
		case my_p_sep_by_space:
			if(lc->p_sep_by_space == CHAR_MAX)
				printf("-1\n");
			else
				printf("\\x%x\n",lc->p_sep_by_space);
			break;
		case my_p_sign_posn:
			if(lc->p_sign_posn == CHAR_MAX)
				printf("-1\n");
			else
				printf("\\x%x\n",lc->p_sign_posn);
			break;
		case my_positive_sign:
			qoutput(lc->positive_sign,";\"\\");
			if(flags & KBIT)
				putchar('"');
			putchar('\n');
			break;
		case print:
			break;
		case punct:
			break;
		case quitexpr:
			nlprint(1,QUITEXPR);
			break;
		case quitstr:
			nlprint(1,QUITSTR);
			break;
		case space:
			break;
		case t_fmt:
			nlprint(1,T_FMT);
			break;
		case t_fmt_ampm:
			nlprint(1,T_FMT_AMPM);
			break;
		case thousands_sep:
			nlprint(1,THOUSEP);
			break;
		case tolower:
			break;
		case toupper:
			break;
		case upper:
			break;
		case xdigit:
			break;
		case yesexpr:
			nlprint(1,YESEXPR);
			break;
		case yesstr:
			nlprint(1,YESSTR);
			break;
	}
	pk->output |= PROCESSED;
}


void
var_print(char *name, char *lcall, char *lang)
{
	char *val;

	if(lcall != NULL) {
		printf("%s=\"",name);
		qoutput(lcall,"$`\"\\\n");
		printf("\"\n");
		return;
	}
	if((val = getenv(name)) == NULL || *val == '\0') {
		if(lang != NULL) {
			printf("%s=\"%s",name);
			qoutput(lang,"$`\"\\\n");
			printf("\"\n");
		}
		else
			printf("%s=\"C\"\n",name);
		return;
	}
	printf("%s=",name);
	qoutput(val, ",|&;<>()$`'\\\" \t\n*?[#~=%");
	putchar('\n');
}

struct kywd *
lookup(char *name)
{
	int i,c;
	for(i=0;i<sizeof(locale_keywords)/sizeof(struct kywd);i++) {
		if((c = strcmp(name,locale_keywords[i].name)) == 0)
			return(&locale_keywords[i]);
		if(c < 0)
			break;
	}
	return(NULL);
}

int
main(int argc, char *argv[])
{

	int c,j;
	struct kywd *pk;
	char *lcall, *lang;
	int err = 0;

	(void) setlocale(LC_ALL,"");
	(void) setcat("localedef");
	(void) setlabel("UX:locale");

	while((c = getopt(argc,argv,"amck")) != EOF) {

		switch(c) {

			case 'c':	flags |= CBIT;
						break;
			case 'a':	flags |= ABIT;
						break;
			case 'm':	flags |= MBIT;
						break;
			case 'k':	flags |= KBIT;
						break;
			default:
			case '?':	flags |= ERROR;
						break;
		}
	}

	if((flags & ERROR) || ((flags & MBIT) && (flags & ~MBIT)) 
	 ||((flags & ABIT) && (flags & ~ABIT))
	 ||((flags & (ABIT|MBIT)) && (optind != argc))
	 ||((flags & (CBIT|KBIT)) && (optind == argc))) {

		pfmt(stderr,MM_ERROR,":148:Usage error\n");
		pfmt(stderr,MM_ACTION,":150:Usage:\nlocale [ -a | -m ]\nlocale [ -ck ] name ...\n");
		exit(1);
	}
	
	if(flags == 0 && argc == optind) {
		if((lcall = getenv("LC_ALL")) == NULL || *lcall == '\0')
			lcall = NULL;
		if((lang = getenv("LANG")) == NULL || *lang == '\0') {
			lang = NULL;
			printf("LANG=\n");
		}
		else {
			printf("LANG=");
			qoutput(lang, ",|&;<>()$`'\\\" \t\n*?[#~=%");
			putchar('\n');
		}
		var_print("LC_COLLATE",lcall,lang);
		var_print("LC_CTYPE",lcall,lang);
		var_print("LC_MESSAGES",lcall,lang);
		var_print("LC_MONETARY",lcall,lang);
		var_print("LC_NUMERIC",lcall,lang);
		var_print("LC_TIME",lcall,lang);
		if(lcall == NULL)
			printf("LC_ALL=\n");
		else {
			printf("LC_ALL=",lcall);
			qoutput(lcall, ",|&;<>()$`'\\\" \t\n*?[#~=%");
			putchar('\n');
		}
		return(0);
	}

		
	if(flags & ABIT) {
		(void) system("find /usr/lib/locale  -name *LC_TIME | /bin/cut -f5 -d/");
		return(0);
	}

	if(flags & MBIT) {
		(void) system("find /usr/lib/localedef/charmaps -print");
		return(0);
	}
	for(j=c=optind;c< argc;c++) {
		if((pk = lookup(argv[c])) == NULL) {
			pfmt(stderr,MM_ERROR,":143:Unknown keyword or category: %s\n",argv[c]);
			err++;
		}
		else
			argv[j++] = (char *) pk;
	}
	argc = j;
	if((lc = localeconv()) == NULL) {
		pfmt(stderr,MM_ERROR,":85:localeconv failed in current locale\n");
		return(1);
	}

	for(c = optind;c < argc;c++) 
		for(j=c; j< argc;j++) 
			if(!(((struct kywd *) argv[j])->output & PROCESSED) 
			&& ((struct kywd *) argv[j])->category == 
			   ((struct kywd *) argv[c])->category)
				swkeyword((struct kywd *) argv[j]);

	return(err);
}
