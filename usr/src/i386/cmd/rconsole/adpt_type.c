/*	copyright	"%c%"	*/

#ident	"@(#)rconsole:i386/cmd/rconsole/adpt_type.c	1.3"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <ctype.h>

extern int errno;
main( argc, argv )
int argc;
char **argv;
{
   extern int optind;
   extern char *optarg;
   int fd, exit_code, adp_type;
   struct kd_disparam kd_d; 
   struct kd_vdctype kd_v; 

   (void)setlocale(LC_ALL,"");
   (void)setcat("uxadpt_type");
   (void)setlabel("UX:adpt_type");

   errno = 0;
   if ((fd=open("/dev/video", 2)) == -1 ) {
	   exit(0); /* non-integral console */
   }
   if (ioctl(fd,KDDISPTYPE, &kd_d)  == -1 ) {
      	   pfmt(stderr,MM_ERROR,":1:ioctl KDDSIPPARAM failed, errno %d\n", errno);
	   exit(99); 	/* error */
   }
   switch (kd_d.type) {
	case KD_MONO:
		/* fprintf(stderr,"KD_MONO\n"); */
		exit_code=1;
		break;;
	case KD_EGA:
		/* fprintf(stderr,"KD_EGA\n"); */
		exit_code=9;
		break;;
	case KD_CGA:
		/* fprintf(stderr,"KD_CGA\n"); */
		exit_code=2;
		break;;
	case KD_VGA:
		/* fprintf(stderr,"KD_VGA\n"); */
                if (ioctl(fd,KDVDCTYPE, &kd_v)  == -1 ) {
      	           pfmt(stderr,MM_ERROR,":2:ioctl KDVDCTYPE failed, errno %d\n", errno);
	           exit(98);
                }
                switch(kd_v.dsply ) {
                      case KD_MULTI_C:
                      case KD_STAND_C:
		           /*fprintf(stderr,"KD_MULTI_C\n"); */
		           exit_code=3;
		           break;;
                      case KD_MULTI_M:
                      case KD_STAND_M:
		           /* fprintf(stderr,"KD_MULTI_M\n"); */
		           exit_code=4;
		           break;;
	              default:
		           /* fprintf(stderr,"kd_v.dsply=%d\n",kd_v.dsply);*/
		           exit_code=5;
		           break;;
                }
		break;;
	default:
		/*fprintf(stderr,"adp_type=%d\n",kd_d.type);*/
		exit_code=10;
		break;;
	}
   exit(exit_code);
}
