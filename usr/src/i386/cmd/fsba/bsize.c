#ident	"@(#)fsba:i386/cmd/fsba/bsize.c	1.1"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/fs/s5ino.h>
#include <sys/fs/s5param.h>
#include <sys/stat.h>
#include <sys/fs/s5filsys.h>
#include <sys/fs/s5dir.h>
#include "fsba.h"

s5bsize(fd, super)
int fd;
struct filsys *super;
{
	return super->s_type;
}
