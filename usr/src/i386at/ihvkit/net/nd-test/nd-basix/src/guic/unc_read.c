#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define PERMS	0666

extern int errno;
main(int argc, char *argv[])
{
	int in_fd, out_fd;
	int l_len,bit_shift,mloop,bytes_read,loop;
	unsigned char buf[1024];
	unsigned char lsb;
	unsigned char tchar;

	if (argc == 1) {
		in_fd = 0;
		out_fd = 1;	
	}
	else if (argc == 2 || argc == 3) {
		if (argc == 2) 
			out_fd = 1;	
		else 
			if ( (out_fd = creat(argv[2], PERMS)) == -1 ) {
				(void)printf("Error creating %s in mode %03o (ERROR:%d).\n", argv[2], PERMS, errno);
				return(5);
			}
			
		if ( (in_fd = open(argv[1], O_RDONLY, 0)) == -1 ) {
			(void)printf("Error opening %s(ERROR:%d).\n", argv[1],errno);
			return(1);
		}
	}
	else {
		(void)printf("\nUsage: %s [<compressed-journal>] [<out-file>]\n", argv[0]);
		return(6);
	}

	bit_shift=1;
	while(1) {
		if ( ((int)read(in_fd, &l_len, sizeof(l_len))) != sizeof(l_len)) {
			if (errno != 0) {
				(void)printf("Error while reading %d bytes(ERROR:%d).\n",
					sizeof(l_len),errno);
				return(2);
			}
			else
				break;
		}
		if ( (bytes_read = (int)read(in_fd, buf, l_len)) < l_len) {
			(void)printf("Error while reading %d bytes(ERROR:%d).\n",
				l_len,errno);
			return(3);
		}
/* Uncompressing. */
		bit_shift = 1;
		for(mloop=0;mloop<bytes_read;mloop++) {
			tchar = (unsigned char)buf[mloop];
			for(loop=1;loop<=bit_shift;loop++) {
				lsb = tchar << 7;
				tchar = ( tchar >> 1) | lsb;
			}
			buf[mloop] = tchar;
			if (bit_shift == 7)
				bit_shift = 1;
			else
				bit_shift++;
		}
		if ( (int)write(out_fd, buf, bytes_read) != bytes_read) {
			(void)printf("Error while writing %d bytes(ERROR:%d).\n",
				bytes_read,errno);
			return(2);
		}
	}
	if ( close(in_fd) != 0) {
		(void)printf("Error while closing %s(ERROR:%d).\n",
			argv[2],errno);
		return(2);
	}
	if ( close(out_fd) != 0) {
		(void)printf("Error while closing %s(ERROR:%d).\n",
			argv[2],errno);
		return(5);
	}
	return(0);
}
