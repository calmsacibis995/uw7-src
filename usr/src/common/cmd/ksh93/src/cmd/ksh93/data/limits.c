#ident	"@(#)ksh93:src/cmd/ksh93/data/limits.c	1.1"
#pragma prototyped

#include	<ast.h>
#include	"ulimit.h"

#define size_resource(a,b) ((a)|((b)<<11))	

/*
 * This is the list of resouce limits controlled by ulimit
 * This command requires getrlimit(), vlimit(), or ulimit()
 */

#ifndef _no_ulimit 
const Shtable_t shtab_limits[] =
{
	{"time(seconds)       ",	size_resource(1,RLIMIT_CPU)},
#   ifdef RLIMIT_FSIZE
	{"file(blocks)        ",	size_resource(512,RLIMIT_FSIZE)},
#   else
	{"file(blocks)        ",	size_resource(1,2)},
#   endif /* RLIMIT_FSIZE */
	{"data(kbytes)        ",	size_resource(1024,RLIMIT_DATA)},
	{"stack(kbytes)       ",	size_resource(1024,RLIMIT_STACK)},
	{"memory(kbytes)      ",	size_resource(1024,RLIMIT_RSS)},
	{"coredump(blocks)    ",	size_resource(512,RLIMIT_CORE)},
	{"nofiles(descriptors)",	size_resource(1,RLIMIT_NOFILE)},
	{"vmemory(kbytes)     ",	size_resource(1024,RLIMIT_VMEM)}
};

const char e_unlimited[] = "unlimited";

#endif
