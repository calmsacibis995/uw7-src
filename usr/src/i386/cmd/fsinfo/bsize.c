#ident	"@(#)fsinfo:i386/cmd/fsinfo/bsize.c	1.1.2.1"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/fs/s5ino.h>
#include <sys/fs/s5param.h>
#include <sys/stat.h>
#include <sys/fs/s5filsys.h>
#include <sys/fs/s5dir.h>

s5bsize(fd, fs)
int fd;
struct filsys *fs;
{
	return fs->s_type;
}
