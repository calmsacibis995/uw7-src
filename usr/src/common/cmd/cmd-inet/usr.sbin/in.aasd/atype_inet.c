#ident "@(#)atype_inet.c	1.3"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
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

/*
 * This file contains code used to handle the INET address type.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <syslog.h>
#include <aas/aas.h>
#include <aas/aas_proto.h>
#include <aas/aas_util.h>
#include "aasd.h"
#include "atype.h"

static int inet_parse(FILE *cf, AasAddr *addr_ret, int *line, int *init);
static int inet_compare(AasAddr *addr1, AasAddr *addr2);
static int inet_validate(AasAddr *addr);
static char *inet_show(AasAddr *addr);

AddressType atype_inet = {
	"INET",
	AAS_ATYPE_INET,
	inet_parse,
	inet_compare,
	inet_validate,
	inet_show
};

/*
 * get_address -- read an address from the given file
 * An inet address is read from the file and returned in addr.
 * The address is terminated by a space, tab, newline, or dash.
 * The terminating character is put back in the stream (with ungetc).
 * If a string of more than 63 characters is read, get_address stops
 * reading and indicates an error.  The return value is 1 if ok or 0 if not.
 */

static int
get_address(FILE *cf, struct in_addr *addr)
{
	char buf[64];
	char *p, *ep;
	int c;

	p = buf;
	ep = buf + sizeof(buf) - 1;
	while ((c = getc(cf)) != EOF && c != ' ' && c != '\t' && c != '\n'
	    && c != '-') {
		if (p == ep) {
			return 0;
		}
		*p++ = c;
	}

	ungetc(c, cf);

	*p = '\0';

	/*
	 * Convert the string to an IP address.
	 */
	
	return inet_aton(buf, addr);
}

static int
inet_parse(FILE *cf, AasAddr *addr_ret, int *line, int *init)
{
	int c;
	char *p;
	static struct in_addr addr;
	struct in_addr addr2;
	static ulong next, last;
	static int range;
	static int newline;
	extern char *config_file;

	/*
	 * See if this is the first call.  If so, initialize stuff.
	 */
	
	if (*init) {
		range = 0;
		newline = 1;
		*init = 0;
	}

	/*
	 * If we're doing a range, return the next address
	 */

	if (range) {
		addr.s_addr = htonl(next);
		if (next == last) {
			range = 0;
		}
		else {
			next++;
		}
		addr_ret->addr = &addr;
		addr_ret->len = sizeof(addr);
		return 1;
	}

	for (;;) {
		/*
		 * Skip white space
		 */
		while ((c = getc(cf)) == ' ' || c == '\t')
			;
		if (c == EOF) {
			report(LOG_ERR, "%s line %d: EOF encountered in pool entry.",
				config_file, *line);
			return -1;
		}
		/*
		 * Check for the end of the entry.  A line that begins with
		 * a closing brace ends the entry.
		 */
		if (newline) {
			(*line)++;
			if (c == '}') {
				/*
				 * Skip the rest of the line.
				 */
				while ((c = getc(cf)) != EOF && c != '\n')
					;
				return 0;
			}
			newline = 0;
		}
		if (c == '\n') {
			newline = 1;
			continue;
		}
		/*
		 * Skip the rest of the line if a '#' is found.
		 */
		if (c == '#') {
			while ((c = getc(cf)) != '\n' && c != EOF)
				;
			newline = 1;
			continue;
		}
		ungetc(c, cf);
		/*
		 * Attempt to read an address
		 */
		if (!get_address(cf, &addr)) {
			report(LOG_ERR, "%s line %d: invalid address.",
				config_file, *line);
			return -1;
		}
		/*
		 * Check for a '-', which indicates a range.
		 */
		if ((c = getc(cf)) == '-') {
			if (!get_address(cf, &addr2)) {
				report(LOG_ERR, "%s line %d: invalid address.",
					config_file, *line);
				return -1;
			}
			next = ntohl(addr.s_addr) + 1;
			last = ntohl(addr2.s_addr);
			if (next > last) {
				report(LOG_ERR, "%s line %d: invalid range.", config_file, *line);
				return -1;
			}
			range = 1;
		}
		else {
			ungetc(c, cf);
		}
		addr_ret->addr = &addr;
		addr_ret->len = sizeof(addr);
		return 1;
	}
}

static int
inet_compare(AasAddr *addr1, AasAddr *addr2)
{
	return memcmp(addr1->addr, addr2->addr, sizeof(struct in_addr));
}

static int
inet_validate(AasAddr *addr)
{
	return addr->len == sizeof(struct in_addr);
}

static char *
inet_show(AasAddr *addr)
{
	struct in_addr in;

	memcpy(&in, addr->addr, sizeof(in));
	return inet_ntoa(in);
}
