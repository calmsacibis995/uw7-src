#ident	"@(#)mapkey.c	1.6"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 *	L000	29Apr97		rodneyh@sco.com
 *	- Change to clear current lock states when loading a new keymap file.
 *
 */

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <stdio.h>

#define DFLT "/usr/lib/keyboard/keys";   

int compat_mode = 0;
int uslmap, scomap = 0;
int kern = 0;
#ifdef DEBUG
int dmp = -1;
#endif

main( Narg, ppCarg )
int Narg;
char **ppCarg;
{
   static struct key_dflt mykeymap;
   extern int optind;
   extern char *optarg;
   FILE *pF;
   int arg, base,i,j, kdfd, pervt, errflg ;
   char *mapfile;

   base = 10; 
   kern = pervt = errflg = 0;
   mapfile=DFLT;

   while( EOF != (arg = getopt( Narg, ppCarg, "USVdoxl:")) ){
      switch(arg){
         case 'd':
            kern = 1;
            break;
         case 'o':
            base = 8;
            break;
         case 'x':
            base = 16;
            break;
         case 'V':
            pervt = 1;
            break;
         case 'S':
            scomap = 1;
            break;
         case 'U':
            uslmap = 1;
            break;
#ifdef DEBUG
         case 'l':
            dmp = atoi(optarg);
            break;
#endif
         default:
            fprintf(stderr,"unknown option -%s", ppCarg[0]);
            fprintf(stderr,"usage: %s [-doxVSU] [file]\n",ppCarg[0]);
            exit(1);
         }
   }
   if ( uslmap && scomap ) {
         fprintf(stderr,"-U and -S cannot be used simultaneously");
         fprintf(stderr,"usage: %s [-doxVSU] [file]\n",ppCarg[0]);
         errflg++;

   }
   if (ioctl(0, KIOCINFO, 0) == -1 ) {
      fprintf(stderr,"mapkey can only be run from a virtual terminal\n");
      fprintf(stderr,"on a graphics workstation.\n");
      fprintf(stderr,"usage: %s [-doxVSU] [file]\n",ppCarg[0]);
      errflg++;
      
   }
   if( kern ){
      if( optind < Narg ){
         fprintf(stderr,"unknown option -%s", ppCarg[0]);
         fprintf(stderr,"usage: %s [-doxVSU] [file]\n",ppCarg[0]);
         errflg++;
      }
   } else {
      if(geteuid() != 0 ) {
         fprintf(stderr,"not super-user\n");
         errflg++;
      }
      if( optind < Narg ){
         mapfile = ppCarg[optind++];
         if (optind < Narg ) {
            fprintf(stderr,"unknown option -%s", ppCarg[optind]);
            fprintf(stderr,"usage: %s [-doxVSU] [file]\n", ppCarg[0]);
            errflg++;
         }
      }
      if( 0 == (pF = fopen(mapfile,"r"))){
         fprintf(stderr,"can't open %s\n",mapfile);
         errflg++;
      }
   }
   if ( errflg ) 
		exit( errflg );
   kdfd = 0;
   if (compat_mode = ioctl(kdfd, WS_GETXXCOMPAT, 0) == -1 ) {
        fprintf(stderr,"mapkey: WS_GETXXCOMPAT ioctl failed - error=%d",
			errno);
        exit (1);
   }
   if ( compat_mode ) {
      if (errflg = ioctl(kdfd, WS_CLRXXCOMPAT, 0) == -1 ) {
           fprintf(stderr,"mapkey: WS_CLRXXCOMPAT ioctl failed - error=%d",
			errno);
           exit (1);
      }
   }
   if( kern ){
      if (pervt) {
         if (errflg = ioctl(kdfd,GIO_KEYMAP,&mykeymap.key_map) == -1) {
            fprintf(stderr,"mapkey: GIO_KEYMAP ioctl failed - error=%d",errno);
            exit (1);
         }
      } else {
             mykeymap.key_direction = KD_DFLTGET;
             if (errflg = ioctl(kdfd, KDDFLTKEYMAP, &mykeymap) == -1) {
                fprintf(stderr,"mapkey: KDDFLTKEYMAP ioctl failed - error=%d",errno);
                exit (1);
             }
      }
      fprintmap(stdout, base, &mykeymap.key_map);
   } else {
      fparsemap(pF,&mykeymap.key_map);
      fclose(pF);

      if (pervt) {
         if (ioctl(kdfd,PIO_KEYMAP,&mykeymap.key_map) == -1) {
          fprintf(stderr,"mapkey: error in PIO_KEYMAP ioctl");

         exit (1);
         }

	 /* L000 begin
	  *
	  * Clear the lockstate on the active VT only
	  */
         if(ioctl(kdfd, KDSETLED, 0) == -1){

		 fprintf(stderr,"mapkey: error in KDSETLED ioctl");
		 exit (1);

         }	/* End L000 */

      } else {
         mykeymap.key_direction = KD_DFLTSET;
         if (ioctl(kdfd,KDDFLTKEYMAP,&mykeymap) == -1) {
         fprintf(stderr,"mapkey: error in KDDFLTKEYMAP ioctl");
         exit (1);
         }

	 /* L000 begin
	  *
	  * Clear the lockstate on all VT's
	  */
         if(ioctl(kdfd, KDSETLCK, 0) == -1){

		 fprintf(stderr,"mapkey: error in KDSETLED ioctl");
		 exit (1);

         }	/* End L000 */

      }
   if ( compat_mode ) 
      if (compat_mode = ioctl(kdfd, WS_SETXXCOMPAT, 0) == -1 ) {
         fprintf(stderr,"mapkey: WS_SETXXCOMPAT ioctl failed - error=%d",
			errno);
        exit (1);
      }
   }
   exit (0);
}
