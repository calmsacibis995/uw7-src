#ident	"@(#)fstyp:i386/cmd/fstyp/bsize.c	1.1"
#ident	"$Header$"

#include <sys/types.h>
#include <sys/fs/s5ino.h>
#include <sys/fs/s5param.h>
#include <sys/stat.h>
#include <sys/fs/s5filsys.h>
#include <sys/fs/s5dir.h>

s5bsize(fd, sblock)
int fd;
struct filsys *sblock;
{
	return sblock->s_type;
}
