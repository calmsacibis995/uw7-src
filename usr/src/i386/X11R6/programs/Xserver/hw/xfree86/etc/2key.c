/*
 * $XFree86: xc/programs/Xserver/hw/xfree86/etc/2key.c,v 3.2 1995/01/28 16:03:47 dawes Exp $
 *
 * Enable/disable the 2-key VT switching sequences for Esix SVR4
 * Note that is program *only* works for Esix SVR4.  To use this program
 * to turn off the 2-key switching for the VT the X server is running on,
 * add the line 'VTInit "2key off"' to your XF86Config file.
 *
 * Usage:
 *         2key on|off
 *
 * David Dawes <dawes@physics.su.oz.au>  October 1992
 *
 */
/* $XConsortium: 2key.c /main/2 1995/11/12 20:16:27 kaleb $ */

#include <stdio.h>
#include <sys/types.h>
#include <sys/kd.h>

extern int errno;

main(argc, argv)

int  argc;
char *argv[];

{
  if (argc == 2)
  {
    if (!strcmp(argv[1], "on"))
    {
      if (ioctl(0, KDENA2KEYSW) < 0)
      {
        fprintf(stderr, "%s: KDENA2KEYSW error (errno=%d)\n", argv[0], errno);
        exit(1);
      }
      exit(0);
    }
    if (!strcmp(argv[1], "off"))
    {
      if (ioctl(0, KDDIS2KEYSW) < 0)
      {
        fprintf(stderr, "%s: KDDIS2KEYSW error (errno=%d)r\n", argv[0], errno);
        exit(1);
      }
      exit(0);
    }
  }
  fprintf(stderr, "usage: %s on|off\n", argv[0]);
  exit(2);
}
