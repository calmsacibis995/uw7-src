#ident	"@(#)pcintf:bridge/p_chmod.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)p_chmod.c	6.6	LCC);	/* Modified: 11:40:30 1/7/92 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef RLOCK
#	include <fcntl.h>
#	include <rlock.h>
#endif

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "dossvr.h"


void
pci_chmod(in, out)
struct input
	*in;				/* Request packet */
struct output
	*out;				/* Response packet */
{
char
	fileName[MAX_FN_TOTAL];			/* Name of file to chmod */
struct stat
	fileStat;			/* Unix file info */
int
	dosAttr = 0,			/* Returned DOS file attribute */
	newMode,			/* New Unix file modes */
#ifdef RLOCK
	vdescriptor,
#endif
	ret;

	out->hdr.stat = NEW;
	out->hdr.res = SUCCESS;

	/* Massage MS-DOS pathname */
	if (cvt_fname_to_unix(MAPPED, (unsigned char *)in->text,
	    (unsigned char *)fileName)) {
		out->hdr.res = ACCESS_DENIED;
		return;
	}

	if (stat(fileName, &fileStat) < 0) {
		err_handler(&out->hdr.res, CHMOD, fileName);
		return;
	}

	/* mode != 0 ==> set modes otherwise get modes */
	if (in->hdr.mode != 0) {
#ifdef RLOCK
		if ((vdescriptor = open_file(fileName, O_RDONLY, SHR_RDWRT,
		    in->hdr.pid, in->hdr.req)) < 0) {
			out->hdr.res = (unsigned char)-vdescriptor;
			return;
		}
#endif
		/* note: do not complain about archive bit */
		if (in->hdr.attr & (SYSTEM | VOLUME_LABEL | SUB_DIRECTORY)) {
			out->hdr.res = ACCESS_DENIED;
#ifdef RLOCK
			close_file(vdescriptor, 0);
#endif
			return;
		}

		if (in->hdr.attr & READ_ONLY)
			newMode = fileStat.st_mode & ~ALL_WRITE;
		else
			newMode = fileStat.st_mode | O_WRITE;

		ret = chmod(fileName, newMode);
#ifdef RLOCK
		close_file(vdescriptor, 0);
#endif
		if (ret < 0) {
			err_handler(&out->hdr.res, CHMOD, fileName);
			return;
		}
	} else
		out->hdr.attr = attribute(&fileStat, fileName);

	return;
}
