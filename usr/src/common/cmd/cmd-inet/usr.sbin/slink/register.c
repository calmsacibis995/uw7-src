#ident "@(#)register.c,v 6.6 1994/02/06 12:47:59 stevea Exp - STREAMware TCP/IP source"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
/*      SCCS IDENTIFICATION        */
#ifdef SS
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <fcntl.h>
#include <sys/serial.h>
#include <sys/oemserial.h>
#include <sys/cp.h>
#include "proto.h"

/*
 * use the serialization mechanism to initialize the slink activation
 * state. snverify(real, demo, bad) will setup a pointer to one of the
 * 3 routines passed in, snstate() returns the serialization state, and
 * snchk() actuall calls real(), demo(), or bad() depending on the
 * serial number/activation key. We'll use this info later.
 */
static int slink_snstate;

real()
{
	slink_snstate = CP_REAL_SS;
}

demo()
{
	slink_snstate = CP_DEMO_SS;
}

bad()
{
	slink_snstate = CP_WRONG_SS;
}

int
Register()
{
	int fd;
	struct strioctl si;
	int sys_as, my_as;

	if ((fd = open(_PATH_SS, O_RDONLY)) < 0)
		return (-1);
	si.ic_cmd = CPI_GETKERNAS;
	si.ic_timout = 0;
	si.ic_len = 0;
	si.ic_dp = (char *) (&sys_as);
	if (ioctl(fd, I_STR, &si) < 0) {
		error(0, "ERROR: Cannot obtain Operating System activation state.\n");
		return (-1);
	}
	/*
	 * make the calls to the libserial functions that deal with
	 * setting up activation state
	 */
	snverify(real, demo, bad);
	my_as = snstate();
	snchk();

	/* test the activation state, instead of the serialization state */
	if (slink_snstate == CP_WRONG_SS) {
		/* this is a hard error, sorry */
		error(0, "ERROR: Invalid activation state.\n\tTCP terminated.\n");
		return (-1);
	} else if (my_as == sys_as) {
		/* valid serialization state & states match - go for it */
		si.ic_cmd = CPI_REGISTER;
		si.ic_timout = 0;
		si.ic_len = CP_PAIRLEN;
		si.ic_dp = &snstring[DISTOFF];
		if (ioctl(fd, I_STR, &si) < 0) {
			/* this is a hard error, sorry */
			error(0, "ERROR: Unable to register with CPD.\n\tTCP terminated.\n");
			return (-1);
		}
	} else {
		/*
		 * valid serialization states, but OS and TCP states don't
		 * match. Warn the user and proceed with startup.
		 */
		error(0, "WARNING: Activation state mismatch.\n\tTCP Startup proceeding\n");
	}
	return (0);
}
#endif /* SS */
