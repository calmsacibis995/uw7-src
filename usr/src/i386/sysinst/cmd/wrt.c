#ident	"@(#)wrt.c	15.1"

#include	<stdio.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<stdlib.h>
#include	<limits.h>
#include	<unistd.h>
#include	<sys/types.h>

int tape_fd;

#define	GSIZE	5120

int block_size = GSIZE;
char *device="/dev/rmt/ctape1";
int dflag, Cflag, sflag, vflag;
#define	_512K	524288

unsigned char magic[] = { 0x19, 0x9e, 'T','L' };

main(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	int option;
	int i;

	dflag = sflag = vflag = Cflag = 0;
	while((option = getopt(argc, argv, "svd:C:")) != EOF) {
		switch(option) {

		case 's':	if (dflag) {
					fprintf(stderr,"wrt: -s cannot be used with -d\n");
					exit(1);
				}
				sflag = 1;
				break;

		case 'd':	if (sflag) {
					fprintf(stderr,"wrt: -d cannot be used with -s\n");
					exit(1);
				}
				device = optarg;
				dflag = 1;
				break;

		case 'v':	vflag = 1;
				break;

		case 'C':	Cflag = 1;
				block_size = atoi(optarg);
				switch(optarg[strlen(optarg)-1]) {
					case 'k':
					case 'K': block_size *= 1024;
						  break;
					case 'b':
					case 'B': block_size *= 512;
						  break;

					default:  break;
				}
				if (block_size < 512 || block_size > _512K) {
					fprintf(stderr,"wrt: Invalid block size.  Must be between 512 and 512K\n");
					exit(1);
				}
			        break;

		default:	fprintf(stderr,"wrt: unknown option %c\n",option);
				exit(1);
		}
	}
	if (optind == argc) {
		fprintf(stderr,"wrt: You must supply a filename\n");
		exit(1);
	}
	if (sflag) tape_fd = fileno(stdout);
	else {
		tape_fd = open(device,O_RDWR|O_CREAT|O_TRUNC,0666);
		if (tape_fd < 0) {
			fprintf(stderr,"wrt: Cannot open %s for output\n",device);
			exit(1);
		}
	}
	if (vflag) fprintf(stderr,"wrt: Using %s for output\n",sflag?"stdout":device);
	for(i = optind; i < argc; i++) {
		write_tape(argv[i]);
	}
	close(tape_fd);
}

int write_tape(char *name)
{
	int fd,cnt,i;
	char *buf;
	unsigned long filesize;
	int am;

	buf = malloc(block_size);
	fd = open(name,O_RDONLY);
	if (fd < 0) {
		fprintf(stderr,"wrt: cannot open %s for input.  Skipping...\n",name);
		return -1;
	}
	filesize=lseek(fd,0L,2);
	if (vflag) fprintf(stderr,"wrt: Writing %ld bytes from %s\n",filesize,name);
	lseek(fd,0L,0);
	i = 0;
	memcpy(buf,magic,4);
	memcpy(buf+4,&filesize,4);
	write(tape_fd,buf,512);
	while((cnt = read(fd,buf,GSIZE))) {
		am = GSIZE;
		if (cnt < GSIZE) {
			memset(buf+cnt,-1,GSIZE-cnt);
			am = cnt / 512;
			am += (cnt % 512) ? 1 : 0;
			am *= 512;
			if (vflag) fprintf(stderr,"wrt: Writing %d bytes in last block\n",am);
		}
		write(tape_fd,buf,am );
	}
	close(fd);
	free(buf);
	return 0;
}
