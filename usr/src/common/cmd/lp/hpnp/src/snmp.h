/*		copyright	"%c%"	*/
#ident	"@(#)snmp.h	1.2"
/*
 * (c)Copyright Hewlett-Packard Company 1991.  All Rights Reserved.
 * (c)Copyright 1983 Regents of the University of California
 * (c)Copyright 1988, 1989 by Carnegie Mellon University
 * 
 *                          RESTRICTED RIGHTS LEGEND
 * Use, duplication, or disclosure by the U.S. Government is subject to
 * restrictions as set forth in sub-paragraph (c)(1)(ii) of the Rights in
 * Technical Data and Computer Software clause in DFARS 252.227-7013.
 *
 *                          Hewlett-Packard Company
 *                          3000 Hanover Street
 *                          Palo Alto, CA 94304 U.S.A.
 */

#define BUFSZ 1024
#define GETONE    "/usr/sbin/getone"	/* Command to obtain SNMP MIB info */
#define COMMUNITY "public"		/* default community name */
/*
 * The following are the paths into the HP printer specific MIB that obtain
 * various bits of status about the printer.
 */
#define SNMP_ACCESS "HP.2.4.3.5.7.0"
#define SNMP_ONLINE "HP.2.3.9.1.1.2.1.0"
#define SNMP_PRINTING " HP.2.4.3.1.2.0"
#define SNMP_PAPER " HP.2.3.9.1.1.2.2.0"
#define SNMP_INTERVENTION " HP.2.3.9.1.1.2.3.0"

int check_access(char *);
