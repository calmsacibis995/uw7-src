#ifndef _PSM_CBUS_CBUS_INCLUDES_H
#define _PSM_CBUS_CBUS_INCLUDES_H

#ident	"@(#)kern-i386at:psm/cbus/cbus_includes.h	1.2"
#ident	"$Header$"

/*++

Copyright (c) 1992-1997  Corollary Inc.

Header Name:

    cbus_includes.h

Abstract:

    this header provides a list of all includes files
	used by the Corollary PSM.  this header file,
    and only this header file, is included in the
    source files.

--*/

#include <svc/psm.h>
#include <psm/toolkits/psm_inline.h>
#include <psm/toolkits/psm_assert.h>
#include <psm/toolkits/psm_time/psm_time.h>
#include <psm/toolkits/psm_i8254/psm_i8254.h>
#include <psm/toolkits/psm_i8259/psm_i8259.h>
#include <psm/toolkits/psm_mc146818/psm_mc146818.h>

#include <cbus.h>
#include <cbus_II.h>
#include <cbus_switch.h>
#include <cbus_tables.h>
#include <cbus_externs.h>

#endif // _PSM_CBUS_CBUS_INCLUDES_H
