#ident	"@(#)pdi.cmds:tapecntl.c	1.9.1.5"
#ident	"$Header$"

/* This program will be used by the tape utilities shell scripts to provide
*  basic control functions for tape erase, retension, rewind, reset, and
*  tape positioning via file-mark count. The tapecntl function is defined by
*
*	Name: tapecntl	- tape control
*
*	Synopsis: tapecntl	[ options ] [ arg ]
*
*	Options:  -C		read compression characteristics
*		  -a		position tape to End-of-Data
*		  -b		read block length limits
*		  -l		load
*		  -u		unload
*		  -e		erase
*		  -t		retension
*		  -r		reset
*		  -v		set variable length block mode
*		  -w		rewind
*		  -c n		set compression characteristics
*				Bit 0 = Compression, Bit 1 = Decompression
*		  -d n		set density to that specified by code n
*				(SCSI-2 spec table 9-22)
*		  -f n		set fixed block length to n bytes
*		  -p n		position - position to filemark n
*/

#include <fcntl.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/st01.h>
#include <sys/tape.h>
#include <stdio.h>
#include <ctype.h>
#include <pfmt.h>
#include <locale.h>

static char INVLENMSG[] =
	":1:Invalid block size. Must be in multiple of 512 bytes.\n";
static char ERAS[] =":2:Erase function failure.\n";
static char WRPRT[] = ":3:Write protected cartridge.\n";
static char CHECK[] =
	":4:Please check tape or cables are installed properly.\nOperation may not be valid on this controller type.\n";
static char RET[] = ":5:Retension function failure.\n";
static char CHECK1[] =
	":6:Please check equipment thoroughly before retry.\n";
static char LOADMSG[] =":7:Load function failure.\n";
static char UNLOADMSG[] =":8:Unload function failure.\n";
static char BLKLENMSG[] =
	":9:Read block length limits function failure.\n";
static char BLKSETMSG[] =
	":10:Set block length function failure.\n";
static char NOSUPPORTMSG[] =
	":11:Function not supported on this controller type.\n";
static char DENSITYMSG[] =":12:Set tape density function failure.\n";
static char REW[] = ":13:Rewind function failure.\n";
static char POS[] = ":14:Positioning function failure.\n";
static char BMEDIA[] =
	":15:No Data Found - End of written media or bad tape media suspected.\n";
static char RESET[] = ":16:Reset function failure.\n";	
static char OPN[] = ":17:Device open failure.\n";
static char USAGE[] =
	":18:Usage:\ntapecntl [-Cabelrtuvw] [-c compression_characteristics]\n\t[-d SCSI_density_code_in_decimal] [-f arg] [-p arg] [device]\n";

static char USAGE2[] =
	":19:tapecntl:\n\t-C\tread compression characteristics\n\t-a\tposition tape to End-of-Data\n\t-b\tread block length limits\n\t-e\terase\n\t-l\tload\n\t-r\treset\n\t-t\tretension\n\t-u\tunload\n\t-v\tset variable length block mode\n\t-w\trewind\n\t-c n\tset compression characteristics to n\n\t-d n\tset density to that specified by SCSI density\n\t\tcode n -- specify n in decimal\n\t-f n\tset fixed block length to n bytes\n\t-p n\tposition - space n filemarks forward\n";

static char COMPRESSMSG[] = ":26:Compression not supported.\n";

static char COMPRESS2MSG[] = ":27:Compression not supported, or invalid parameter specified\n";

void exit();

main(argc,argv)
int argc;
char **argv;
{
	int	c,tp,arg,length;
	extern char *optarg;
	extern int optind;
	extern int errno;

	char	*device;
	struct blklen bl;

	char	stat_buf[6];
	int	blen = 0;
	int	load = 0;
	int	unload = 0;
	int	erase = 0;
	int	retension = 0;
	int	reset = 0;
	int	variable = 0;
	int	fixed = 0;
	int	rewind = 0;
	int	position = arg = 0;
	int	append = 0;
	int	density = 0;
	int	setCompression = 0;				/* LXXX */
	int	getCompression = 0;				/* LXXX */

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxtapecntl");
	(void)setlabel("UX:tapecntl");

	if (argc < 2)
	{
		pfmt(stderr, MM_ACTION, USAGE2);
		exit(1);
	}
	signal(SIGINT,SIG_DFL);

	device = "/dev/rmt/ntape1";

	while(( c = getopt(argc,argv,"bluetrvf:wp:ad:?c:C")) != EOF)
		switch ( c ) {

		case 'a':
			append = 1;
			break;

		case 'b':
			blen = 1;
			break;

		case 'l':
			load = 1;
			break;

		case 'u':
			unload = 1;
			break;

		case 'e':
			erase = 1;
			break;

		case 't':
			retension = 1;
			break;

		case 'r':
			reset = 1;
			break;

		case 'v':
			variable = 1;
			break;

		case 'f':
			fixed = 1;
			length = atoi(optarg);
			break;

		case 'w':
			rewind = 1;
			break;

		case 'p':
			position = 1;
			arg = atoi(optarg);
			break;

		case 'd':
			density = 1;
			arg = atoi(optarg);
			break;

		case '?':
			pfmt(stderr, MM_ACTION, USAGE);
			exit(1);
		
		case 'C':
			getCompression = 1;
			break;

		case 'c':
			setCompression = 1;
			arg = atoi(optarg);
			break;
		}

	if ( optind + 1 == argc )
		device = argv[optind];
	else if ( optind + 1 < argc ) {
		pfmt(stderr, MM_ACTION, USAGE);
		exit(1);
	}
	if (optind == 1) {
		pfmt(stderr, MM_ACTION, USAGE2);
		exit(1);
	}

	if ( append ) {
		if((tp = open(device, O_RDONLY))<0) { 
			pfmt(stderr, MM_ERROR, OPN);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(4);
  		}
		if(ioctl(tp,T_EOD,NULL,0)<0) {
			pfmt(stderr, MM_ERROR, POS);
			if(errno == ENXIO) {
				pfmt(stderr, MM_ERROR, CHECK);
				exit(1);
			} else
		  	if(errno == EIO) {
				pfmt(stderr, MM_ERROR, BMEDIA);
				exit(2);
		  	}
		   	close (tp);
			exit(1);
		}
		close (tp);
		exit(0);
	}

	if ( blen ) {
		if((tp = open(device, O_RDONLY))<0) { 
			pfmt(stderr, MM_ERROR, OPN);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(4);
  		}
		if(ioctl(tp,T_RDBLKLEN,&bl,sizeof(struct blklen))<0) {
		   	close (tp);
			pfmt(stderr, MM_ERROR, BLKLENMSG);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(1);
		}
		pfmt(stdout, MM_NOSTD,
			":20:Block length limits:\n");
		pfmt(stdout, MM_NOSTD,
			":21:\tMaximum block length = %d\n", bl.max_blen);
		pfmt(stdout, MM_NOSTD,
			":22:\tMinimum block length = %d\n", bl.min_blen);
		close (tp);
		exit(0);
	}

	if ( variable ) {
		if((tp = open(device, O_RDONLY))<0) { 
			pfmt(stderr, MM_ERROR, OPN);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(4);
  		}
		bl.max_blen = 0;
		bl.min_blen = 0;
		if(ioctl(tp,T_WRBLKLEN,&bl,sizeof(struct blklen))<0) {
			pfmt(stderr, MM_ERROR,
				":23:set for variable length block mode failed: %s\n",strerror(errno));
		   	if (errno == ENXIO)
				pfmt(stderr, MM_ERROR, BLKSETMSG);
			if (errno == EINVAL)
				pfmt(stderr, MM_ERROR, NOSUPPORTMSG);
			else
				pfmt(stderr, MM_ERROR, CHECK);
		   	close (tp);
			exit(1);
		}
		close (tp);
		exit(0);
	}


	if ( fixed ) {
		if (length%512) {
			pfmt(stderr, MM_ERROR, INVLENMSG);
			exit(4);
		}
		if((tp = open(device, O_RDONLY))<0) { 
			pfmt(stderr, MM_ERROR, OPN);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(4);
  		}
		bl.max_blen = length;
		bl.min_blen = length;
		if(ioctl(tp,T_WRBLKLEN,&bl,sizeof(struct blklen))<0) {
			pfmt(stderr, MM_ERROR,
				":24:set for fixed length block mode failed: %s\n",strerror(errno));
		   	if (errno == ENXIO)
				pfmt(stderr, MM_ERROR, BLKSETMSG);
			if (errno == EINVAL)
				pfmt(stderr, MM_ERROR, NOSUPPORTMSG);
			else
				pfmt(stderr, MM_ERROR, CHECK);
		   	close (tp);
			exit(1);
		}
		close (tp);
		exit(0);
	}

	if ( load ) {
		if((tp = open(device, O_RDONLY))<0) { 
			pfmt(stderr, MM_ERROR, OPN);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(4);
  		}
		if(ioctl(tp,T_LOAD,stat_buf,6)<0) {
			close (tp);
			pfmt(stderr, MM_ERROR, LOADMSG);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(1);
		}
		close (tp);
		exit(0);
	}
	if ( unload ) {
		if((tp = open(device, O_RDONLY))<0) { 
			pfmt(stderr, MM_ERROR, OPN);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(4);
  		}
		if(ioctl(tp,T_UNLOAD,stat_buf,6)<0) {
		   	close (tp);
			pfmt(stderr, MM_ERROR, UNLOADMSG);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(1);
		}
		close (tp);
		exit(0);
	}
	if ( erase ) {
		if((tp = open(device, O_RDWR))<0) {
		    	if(errno == EACCES) {
				pfmt(stderr, MM_ERROR, WRPRT);
				exit(3);
		      	} else {
				pfmt(stderr, MM_ERROR, OPN);
				pfmt(stderr, MM_ERROR, CHECK);
				exit(4);
		      	}
  		}
		if(ioctl(tp,T_ERASE,0,0)<0) {
		   	close (tp);
			pfmt(stderr, MM_ERROR, ERAS);
		   	if(errno == ENXIO) {
				pfmt(stderr, MM_ERROR, CHECK);
				exit(1);
		      	} else {
				pfmt(stderr, MM_ERROR, CHECK1);
				exit(2);
		      	}
		}
		close (tp);
		exit(0);
	}

	if ( retension ) {
		if((tp = open(device, O_RDONLY))<0) { 
			pfmt(stderr, MM_ERROR, OPN);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(4);
  		}
		if(ioctl(tp,T_RETENSION,0,0)<0)  {
			close (tp);
			pfmt(stderr, MM_ERROR, RET);
			if(errno == ENXIO) {
				pfmt(stderr, MM_ERROR, CHECK);
				exit(1);
			} else {
				pfmt(stderr, MM_ERROR, CHECK1);
				exit(2);
			}
		}
		close (tp);
		exit(0);
	}
	if ( reset ) {
		if((tp = open(device, O_RDONLY))<0) { 
			pfmt(stderr, MM_ERROR, OPN);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(4);
  		}
		if (ioctl(tp,T_RST,0,0)<0) {
			close (tp);
			pfmt(stderr, MM_ERROR, RESET);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(1);
		}
		close (tp);
		exit(0);
	}
	if ( rewind ) {
		if((tp = open(device, O_RDONLY))<0) { 
			pfmt(stderr, MM_ERROR, OPN);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(4);
  		}
		if(ioctl(tp,T_RWD,0,0)<0) {
			close (tp);
			pfmt(stderr, MM_ERROR, REW);
	         	if(errno == ENXIO) {
				pfmt(stderr, MM_ERROR, CHECK);
				exit(1);
		   	} else {
				pfmt(stderr, MM_ERROR, CHECK1);
				exit(2);
		   	}
		}
		close (tp);
		exit(0);
	}
	if ( position ) {
		int	cmd;

		if((tp = open(device, O_RDONLY))<0) { 
			pfmt(stderr, MM_ERROR, OPN);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(4);
  		}
		if (arg < 0) {
			cmd = T_SFB;
			arg = (-1) * arg;
		}
		else {
			cmd = T_SFF;
		}
		if(ioctl(tp,cmd,arg,0)<0) {
			close (tp);
			pfmt(stderr, MM_ERROR, POS);
			if(errno == ENXIO) {
				pfmt(stderr, MM_ERROR, CHECK);
				exit(1);
			} else
		  	if(errno == EIO) {
				pfmt(stderr, MM_ERROR, BMEDIA);
				exit(2);
		  	}
		}
		close (tp);
		exit(0);
	}
	if ( density ) {
		if((tp = open(device, O_RDONLY))<0) { 
			pfmt(stderr, MM_ERROR, OPN);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(4);
  		}
		if(ioctl(tp,T_STD,arg,sizeof(arg))<0) {
		   	close (tp);
			pfmt(stderr, MM_ERROR, DENSITYMSG);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(1);
		}
		pfmt(stdout, MM_NOSTD,
			":25:New tape density code = %d (%#x)\n", arg, arg);
		close (tp);
		exit(0);
	}

	if ( getCompression ) {
		if ((tp = open(device, O_RDONLY)) < 0) {
			pfmt(stderr, MM_ERROR, OPN);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(4);
		}
		if (ioctl(tp,T_GETCOMP,&arg,sizeof(arg))<0) {
			close(tp);
			pfmt(stderr, MM_ERROR, COMPRESSMSG);
			exit(1);
		}
		pfmt(stdout, MM_NOSTD,
			":28:Tape Compression = %d\nTape Decompression = %d\n", 
			arg&1 ? 1 : 0, arg&2 ? 1 : 0);
		close(tp);
		exit(0);
	}

	if ( setCompression ) {
		if ((tp = open(device, O_RDONLY)) < 0) {
			pfmt(stderr, MM_ERROR, OPN);
			pfmt(stderr, MM_ERROR, CHECK);
			exit(4);
		}
		if (ioctl(tp,T_SETCOMP,arg,0) < 0) {
			close(tp);
			pfmt(stderr, MM_ERROR, COMPRESS2MSG);
			exit(1);
		}
		close(tp);
		exit(0);
	}

	return(0);
}
