#ident "@(#)syssignal.c	1.2"

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#include "ntp_stdlib.h"

#if defined(NTP_POSIX_SOURCE) && !defined(SYS_WINNT) && !defined(VMS)
#include <errno.h>

extern int errno;

void
signal_no_reset(sig, func)
int sig;
void (*func)();
{
    int n;
    struct sigaction vec;

    vec.sa_handler = func;
    sigemptyset(&vec.sa_mask);
#ifdef _SEQUENT_
    vec.sa_flags = SA_RESTART;
#else
    vec.sa_flags = 0;
#endif

    while (1) {
        n = sigaction(sig, &vec, NULL);
	if (n == -1 && errno == EINTR) continue;
	break;
    }
    if (n == -1) {
	perror("sigaction");
        exit(1);
    }
}

#else
RETSIGTYPE
signal_no_reset(sig, func)
int sig;
RETSIGTYPE (*func)();
{
    signal(sig, func);

}
#endif

