#ident	"@(#)dstime:dstime/dst_adtodc.c	1.2"

#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <pfmt.h>
#include <priv.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/rtc.h>
#include <sys/sysi86.h>
#include <sys/types.h>
#include <sys/uadmin.h>
#include <time.h>
#include <unistd.h>
#include "timem.h"

#define UNHEXIZE(A)	(((((A)>>4)&0xF)*10)+((A)&0xF))

char *prog;

void usage();

main(int argc, char **argv)
{
	int i, err;
	struct rtc_t clk;
	struct tm tmp;
	time_t utc;
	time_t timbuf;
	time_t adjustment;
	time_t correction;
	char buf[BUFSIZ];

	/*
	 * Clear working privileges.
	 */
	(void)procprivc(CLRPRV, PRIVS_W, (priv_t)0);

	prog = strrchr(argv[0], '/');
	if (prog++ == NULL) {
		prog = argv[0];
	}
	(void)sprintf(buf, "UX:%s", prog);
	/*
	 * Set locale and error message info.
	 */
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel(buf);

	if (argc != 3) {
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		usage();
	}

	adjustment = (time_t)strtol(*++argv, (char **)NULL, 10);
	correction = (time_t)strtol(*++argv, (char **)NULL, 10);
	if ((long)adjustment == LONG_MAX || (long)adjustment == LONG_MIN ||
	    (long)correction == LONG_MAX || (long)correction == LONG_MIN) {
		pfmt(stderr, MM_ERROR, ":1191:Bad range\n");
		exit(1);
	}

	if (sysi86(RTODC, &clk, 0) < 0) {
		exit(1);
	}

	tmp.tm_sec  = UNHEXIZE(clk.rtc_sec);
	tmp.tm_min  = UNHEXIZE(clk.rtc_min);
	tmp.tm_hour = UNHEXIZE(clk.rtc_hr);
	tmp.tm_mday = UNHEXIZE(clk.rtc_dom);
	tmp.tm_mon  = UNHEXIZE(clk.rtc_mon) - 1; /* 0 based */
	tmp.tm_year = UNHEXIZE(clk.rtc_yr);
	if (tmp.tm_year < (EPOCH_YEAR - BASE_YEAR)) {
		tmp.tm_year += 100;
	}
	if ((timbuf = (time_t)norm_tm(&tmp)) == LONG_MIN) {
		exit(1);
	}
	timbuf += adjustment;
	/*
	 * Convert to GMT.
	 */
	utc = timbuf + correction;

	/*
	 * uadmin(A_CLOCK, ...) and stime() both require SYSOPS privilege.
	 * CAUTION: do not insert here any calls
	 * that do not require SYSOPS privilege.
	 */
	(void)procprivc(SETPRV, SYSOPS_W, (priv_t)0);
		uadmin(A_CLOCK, correction, 0);
		i = stime(&utc); err = errno;
	(void)procprivc(CLRPRV, PRIVS_W, (priv_t)0);
	exit(i ? 1 : 0);
}

void
usage()
{

	pfmt(stderr, MM_ACTION|MM_NOGET,
		"Usage: %s adjustment correction\n", prog);
	exit(1);
}
