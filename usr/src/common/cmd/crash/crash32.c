#ident	"@(#)crash:common/cmd/crash/crash32.c	1.1"

/*
 * 32-bit stubs for 64-bit calls, linked into crash32, which can
 * then be used to examine most Gemini dumps on an older UnixWare
 * machine which lacks 64-bit support in its kernel or libc.so.1;
 * or even on an OpenServer system which lacks pread()
 * (but will not print out the "%ll"s properly).
 */

#include <sys/types.h>
#include <errno.h>
#include "crash.h"

ssize_t
pread64(int fd, void *buffer, size_t size, off64_t off)
{
	if (highlong(off)) {
		errno = EINVAL;
		return (ssize_t)(-1);
	}
	if (lseek(fd, (off32_t)off, 0) == -1)
		return (ssize_t)(-1);
	return (ssize_t)read(fd, buffer, size);
}

void *
mmap64(void *addr, size_t size, int prot, int map, int fd, off64_t off)
{
	if (highlong(off)) {
		errno = EINVAL;
		return (void *)(-1);
	}
	return (void *)mmap(addr, size, prot, map, fd, (off32_t)off);
}
