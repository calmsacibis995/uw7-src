/*		copyright	"%c%" 	*/


#ident	"@(#)setclk:i386/cmd/setclk/setclk.c	1.4.12.2"

/***************************************************************************
 * Command : setclk
 * Inheritable Privileges : P_SYSOPS
 *       Fixed Privileges : None
 * Notes:
 *
 ***************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <priv.h>
#include <sys/sysi86.h>
#include <sys/uadmin.h>
#include <sys/rtc.h>
#include <errno.h>
#include <time.h>

#define	dysize(A) (((A)%4)? 365: 366)	/* number of days per year */
#define unhexize(A)	(((((A)>>4)&0xF)*10)+((A)&0xF))

main()
{
	struct rtc_t clk;
	struct tm tmp;
	time_t timbuf;
	time_t tzone;
	const char *msg1 = "RTODC not implemented in system\n";
	const char *msg2 = "unable to set time from the Real Time Clock\n";
	const char *msg3 = "\n\t\tTime of Day Clock needs Restoring:\n";
	const char *msg4 = "\t\tChange using \"date\" utility\n";

	if (sysi86(RTODC, &clk, 0) < 0) {
		if (errno == EINVAL) {
			printf("%s%s", msg1, msg2);
			exit(0);
		}
		printf("%s%s", msg3, msg4);
		exit(0);
	}

	tmp.tm_sec  = unhexize(clk.rtc_sec);
	tmp.tm_min  = unhexize(clk.rtc_min);
	tmp.tm_hour = unhexize(clk.rtc_hr);
	tmp.tm_mday = unhexize(clk.rtc_dom);
	tmp.tm_mon  = unhexize(clk.rtc_mon) - 1; /* 0 based */
	tmp.tm_year = unhexize(clk.rtc_yr);
	tmp.tm_isdst = -1;
	if (tmp.tm_year < 70)
		tmp.tm_year += 100;
	timbuf = mktime(&tmp);

	if (tmp.tm_isdst == 0)
		tzone = timezone;
	else
		tzone = altzone;

	uadmin(A_CLOCK, tzone, 0);
	sysi86(STIME, timbuf);
}
