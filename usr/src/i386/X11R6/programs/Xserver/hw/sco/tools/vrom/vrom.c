/*
 *	@(#)vrom.c 11.3
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1997.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *	SCO MODIFICATION HISTORY
 *
 *	S001, 28-Oct-97, davidw
 *	- Double VB_MAX to 0x10000 - 64K vrom size.  New Chips 65550
 *	chipsets in Toshiba Tecra 730XCDT have larger bios sizes.
 * 	S000, 14-Dec-95, kylec
 *	- Create
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

#define VB_OFFSET 0xC0000
#define VB_MAX 	  0x10000		/* S001 */
#define VB_2K 	  0x800
#define VB_512    0x200
#define VB_OK 	  0
#define VB_FAIL   1


VB_load(unsigned char *bios, int *size)
{
    int mem_fd, i;
    unsigned char buf[3];

    *size = 0;
    mem_fd = open("/dev/mem", O_RDONLY);
    if (mem_fd == -1)
    {
        perror("/dev/mem");
        return VB_FAIL;
    }

    /* Find start of video bios */
    for (i = 0; i < VB_MAX; i += VB_2K)
    {
        if (lseek(mem_fd, VB_OFFSET + i, SEEK_SET) == -1)
        {
            perror("lseek /dev/mem");
            break;
        }

        if (read(mem_fd, (void*)buf, 3) == -1)
        {
            perror("read /dev/mem");
            break;
        }
        
        if (buf[0] == 0x55 && buf[1] == 0xAA)
        {
            unsigned char c = buf[2];
            if (lseek(mem_fd, VB_OFFSET + i, SEEK_SET) == -1)
            {
                perror("lseek /dev/mem");
                break;
            }
            *size = c * VB_512;
            while (c--)
            {
                if (read(mem_fd, (void*)bios, VB_512) == -1)
                {
                    perror("read /dev/mem");
                    *size = 0;
                    break;
                }

                bios += VB_512;
            }
            break;
        }
    }

    close(mem_fd);

    if (*size > 0)
        return VB_OK;
    else
        return VB_FAIL;
}


int VB_csum(unsigned char *vb, int sz)
{
    int i, csum = 0;

    for (i=0; i<sz; i++)
        csum += vb[i];

    if ((csum & 0XFF) == 0)
        return VB_OK;
    else
        return VB_FAIL;
}


main (int argc, char **argv)
{
    int size;
    unsigned char vb[VB_MAX];

    if ((VB_load(vb, &size) != VB_OK) ||
        (VB_csum(vb, size) != VB_OK))
        exit(VB_FAIL);

    write(1, vb, size);
    exit(VB_OK);
}
