/*
 * terminal setup
 */
#include <termio.h>
#include <sys/ioctl.h>

static struct termio tio;
static struct termio orig;

ioctlon()
{
	ioctl(0, TCGETA, &orig);
	ioctl(0, TCGETA, &tio);
	tio.c_lflag &= ~ECHO;
	tio.c_cc[VTIME] = 0;
	tio.c_cc[VMIN] = 1;
	ioctl(0, TCSETAW, &tio);
}

ioctloff()
{
	ioctl(0, TCSETAW, &orig);
}
