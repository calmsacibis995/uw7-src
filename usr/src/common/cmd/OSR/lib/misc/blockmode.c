#ident	"@(#)OSRcmds:lib/misc/blockmode.c	1.1"
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>
#include <errno.h>
#include "../../include/osr.h"

static char fdstat[2048];

int
blockopen(const char *path, int oflag, mode_t mode)
{
	int	fd;
	fd = open(path, oflag, mode);
	if (fd < 0) return (-1);
	fdstat[fd] = 0;
}

off_t
blocklseek(int fildes, off_t offset, int whence)
{
	unsigned long	oldoffset = offset;

	if (fdstat[fildes])
		offset = (unsigned long) offset << SCTRSHFT;
	if (offset < oldoffset) {
		errno = EINVAL;
		return (-1);
	}

	return (lseek(fildes, offset, whence));
}

ssize_t
blockread(int fildes, void *buf, size_t nbyte)
{
	size_t	oldnbyte = nbyte;

	if (fdstat[fildes])
		nbyte = (unsigned long) nbyte << SCTRSHFT;
	if (nbyte < oldnbyte) {
		errno = EINVAL;
		return (-1);
	}

	return (read(fildes, buf, nbyte));
}

ssize_t
blockwrite(int fildes, void *buf, size_t nbyte)
{
	size_t	oldnbyte = nbyte;

	if (fdstat[fildes])
		nbyte = (unsigned long) nbyte << SCTRSHFT;
	if (nbyte < oldnbyte) {
		errno = EINVAL;
		return (-1);
	}

	return (write(fildes, buf, nbyte));
}

int
blockfcntl(int fd, int cmd, char *arg)
{
	struct stat	blockstat;
	int		fbmode;
	int		ftype;

	switch (cmd) {
		case F_GETBMODE:
			if (fstat(fd, &blockstat) != 0) {
				errno=EBADF;
				return(-1);
			}
			/* Must be block or character special device */
			if ((ftype = (blockstat.st_mode & S_IFMT)) != S_IFCHR &&
					ftype != S_IFBLK) {
				errno=EINVAL;
				return(-1);
			}
			fbmode = (int) (fdstat[fd] & 1);
			if ((char *)memcpy(arg, &fbmode, sizeof(fbmode)) != arg) {
				errno=EFAULT;
				return(-1);
			}
			break;
		case F_SETBMODE:
			if (fstat(fd, &blockstat) != 0) {
				errno=EBADF;
				return(-1);
			}
			if ((char *)memcpy(&fbmode, arg, sizeof(fbmode)) != (char *)&fbmode) {
				errno=EFAULT;
				return(-1);
			}
			/* Must be block or character special device */
			if ((ftype = (blockstat.st_mode & S_IFMT)) != S_IFCHR &&
					ftype != S_IFBLK) {
				errno=EINVAL;
				return(-1);
			}
			/* The mode changed, so reset the offset */
			if (lseek(fd, 0, SEEK_SET) < 0)
				return (-1);
			/* Set the flag */
			if (fbmode)
				fdstat[fd] = 1;
			else
				fdstat[fd] = 0;
			break;
		default:
			return (-1);
	}
	return (0);
}
