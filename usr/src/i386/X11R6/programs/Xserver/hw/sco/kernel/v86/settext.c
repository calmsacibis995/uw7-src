#ident "@(#)settext.c 11.1"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "v86.h"

main (int argc, char **argv)
{
  intregs_t regs;
  int fd_v86, mode;

  memset((void*)&regs, 0, sizeof(regs));
  regs.type = V86BIOS_INT;
  regs.entry.intval = V86_VIDEO_BIOS; /* video bios */
  regs.eax.byte.ah = 0;         /* set video mode */
  mode = 3;                     /* default: text mode 3 */

  if (argc > 1)
    mode = atoi(argv[1]);       /* override mode */

  regs.eax.byte.al = mode;

  if ((mode < 0) || (mode > 15))
    {
      printf("Invalid mode: %d\n", mode);
      printf("Usage: %s [0-15]\n", argv[0]);
      exit(1);
    }
    
    
  fd_v86 = open(V86_DEVICE, O_RDWR);
  if (fd_v86 == -1)
    {
      perror(V86_DEVICE);
      exit(1);
    }
  if (ioctl(fd_v86, V86_CALLBIOS, &regs) < 0)
    {
      perror("BIOS error");
      close(fd_v86);
      exit(1);
    }

  /* Now query the current mode */
  memset((void*)&regs, 0, sizeof(regs));
  regs.type = V86BIOS_INT;
  regs.entry.intval = V86_VIDEO_BIOS; /* video bios */
  regs.eax.byte.ah = 15;        /* query mode */

  if (ioctl(fd_v86, V86_CALLBIOS, &regs) < 0)
    {
      perror("BIOS error");
      close(fd_v86);
      exit(1);
    }
  else
    {
      printf("Video mode: %d\n", regs.eax.byte.al);
      printf("Columns: %d\n", regs.eax.byte.ah);
      printf("Active page: %d\n", regs.ebx.byte.bl);
    }

  close(fd_v86);
  exit(0);

}
