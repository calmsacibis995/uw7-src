#ident	"@(#)pcintf:bridge/p_version.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)p_version.c	6.5	LCC);	/* Modified: 23:22:42 7/12/91 */

/***********************************************************************

	Copyright (c) 1989 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

***********************************************************************/

#include "sysconfig.h"

#include <malloc.h>
#include <string.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "const.h"
#include "dossvr.h"
#include "log.h"
#include "table.h"
#include "version.h"


#if defined(JANUS) && defined(FAST_LSEEK)
	/* Fast LSEEK code must be enabled statically for Merge 386 */
unsigned short bridge_ver_flags = V_FAST_LSEEK;
#else
unsigned short bridge_ver_flags = 0;
#endif	/* JANUS && FAST_LSEEK */

char *bridge_version = NULL;
char server_version[256];

LOCAL char	*feature		PROTO((char *));
LOCAL int	feature_exists		PROTO((char *, char *));

extern int rlPidTable;


void get_version_string(bridge_str, count, addr)
char *bridge_str;
struct output *addr;
{
	sprintf(server_version, "F=1,PCI=%d.%d.%d,LCSTBL=%s",
		VERS_MAJOR, VERS_MINOR, VERS_SUBMINOR, unix_table_name);
	if (count) {
		bridge_ver_flags = 0;
		if (bridge_version)
			free(bridge_version);
		if (bridge_version = malloc(count)) {
			strcpy(bridge_version, bridge_str);
			log("server_version=\"%s\"\n", server_version);
			log("bridge_version=\"%s\"\n", bridge_version);
#ifdef FAST_LSEEK
			if (feature_exists("FAST_LSEEK", NULL)) {
				bridge_ver_flags |= V_FAST_LSEEK;
				strcat(server_version, ",FAST_LSEEK=1.0");
			}
#endif /* FAST_LSEEK */
#ifdef FAST_FIND
			if (feature_exists("FAST_FIND", NULL)) {
				bridge_ver_flags |= V_FAST_FIND;
				strcat(server_version, ",FAST_FIND=1.0");
			}
#endif /* FAST_FIND */
			if (feature_exists("ERR_FILTER", NULL))
				bridge_ver_flags |= V_ERR_FILTER;
			if (feature_exists("PID_TABLE", NULL))
				rlPidTable = TRUE;
			log("bridge_ver_flags=%#x\n", (int)bridge_ver_flags & 0xffff);
			addr->hdr.res = SUCCESS;
		}
		else
			addr->hdr.res = INSUFFICIENT_MEMORY;
	}
	else
		addr->hdr.res = DATA_INVALID;
	strcpy(addr->text, server_version);
	addr->hdr.t_cnt = (unsigned short)strlen(server_version) + 1;
	return;
}



#if defined(MIN)
#	undef MIN
#endif
#define	MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define	MAX_VER_SIZE	32

char *feature(token)
	char *token;
{
	register char *f;
	register int n;
	register int l;

	static char version[MAX_VER_SIZE];

	if (!token)
		return NULL;
	f = bridge_version;
	l = strlen(token);
	while (f && (strncmp(f, token, (n = strcspn(f, "="))) || n != l))
		if (f = strchr(f, ','))
			f++;
	if (f) {
		f += n + 1;
		if (n = strcspn(f, ",")) {
			strncpy(version, f, MIN(n, MAX_VER_SIZE));
			version[MAX_VER_SIZE - 1] = '\0';
			return version;
		}
	}
	return f;
}



int feature_exists(token, version)
	char *token;
	char *version;
{
	char *c;

	if (!(c = feature(token)))
		return 0;	/* the feature isn't listed */
	else if (!version)
		return 1;	/* caller doesn't care what version */
	else
		return !strcmp(c, version);
}
