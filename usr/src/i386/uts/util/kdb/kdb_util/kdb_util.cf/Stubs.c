#ident	"@(#)kern-i386:util/kdb/kdb_util/kdb_util.cf/Stubs.c	1.6.2.1"
#ident	"$Header$"

#ifndef NODEBUGGER

#include <sys/ksynch.h>
#include <sys/types.h>

static char empty_commands[2];
char *kdbcommands = empty_commands;

extern fspin_t debug_count_mutex;

void kdb_init(void)
{
	FSPIN_INIT(&debug_count_mutex);
}

#ifndef MODSTUB
void kdb_printf() {}
boolean_t kdb_check_aborted() { return B_FALSE; }
#endif

#endif /* NODEBUGGER */
