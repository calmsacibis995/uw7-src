/*
 * File hw_rom.c
 * Information handler for rom
 *
 * @(#) hw_rom.c 67.2 97/11/07 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <sys/bootinfo.h>
#include <sys/stat.h>
#include <malloc.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include "hw_rom.h"
#include "hw_util.h"

const char	* const callme_rom[] =
{
    "rom",
    "bios",
    NULL
};

const char	short_help_rom[] = "System ROM statistics";

#define ROM_512		512
#define ROM_2K		(2 * 1024)
#define ROM_32K		(32 * 1024)
#define ROM_128K	(128 * 1024)

static u_char	biosBuf[ROM_128K];

static void	find_adapter_roms(FILE *out, u_long *next);
static int	bios_read(u_long addr, u_long *size);
static u_char	bios_sum(u_long size);
static void	show_adapter_bios(FILE *out, u_long addr, u_long size);
static int	show_system_bios(FILE *out, u_long next);
static void	bios_strings(FILE *out, u_long size);

int
have_rom(void)
{
    return 1;	/* I HOPE we always have at least a BIOS ROM */
}

void
report_rom(FILE *out)
{
    u_long	next;

    report_when(out, "ROM");

    find_adapter_roms(out, &next);
    show_system_bios(out, next);
}

/*
 * We search from 0xc0000 - 0xe0000 in 2k blocks for:
 *	offset 0x0000	0x55
 *	offset 0x0001	0xaa
 *	offset 0x0002	ROM length in 512 bytes
 *	offset 0x0003	entry by FAR CALL
 *	The ROM byte checksum must be zero
 * and from 0xe0000 - 0xefff in 64k blocks for:
 *	offset 0x0000	0x55
 *	offset 0x0001	0xaa
 *	offset 0x0002	unused
 *	offset 0x0003	entry by FAR CALL
 *	The ROM byte checksum must be zero
 */

static void
find_adapter_roms(FILE *out, u_long *next)
{
    u_long	size;
    u_long	addr;

    *next = 0;

    for  (addr = 0xc0000LU; addr <= 0xe0000LU; addr += ROM_2K)
    {
	int	err;

	if ((err = bios_read(addr, &size)) != 0)
	{
	    if (err == -1)
		continue;

	    break;
	}

	if ((addr + size) > *next)
	    *next = addr + size;

	show_adapter_bios(out, addr, size);
    }
}

/*
 * Return:
 *	0	Successful
 *	-1	Not a valid ROM
 *	else	fatal error
 */

int
bios_read(u_long addr, u_long *size)
{
    /*
     * Read the ROM into our buffer
     */

    if (read_mem(addr, biosBuf, 3) == -1)
    {
	debug_print("BIOS read fail at 0x%x: %s", addr, strerror(errno));
	return errno;
    }
    
    if ((biosBuf[0] != 0x55U) || (biosBuf[1] != 0xAAU))
	return -1;

    if (addr < 0xe0000LU)
	*size = (u_long)biosBuf[2] * ROM_512;
    else
	*size = ROM_32K;

    if (*size > sizeof(biosBuf))
    {
	debug_print("Adapter BIOS at 0x%x is too large: %gKb\n",
					addr,
					(double)*size / 1024);
	return -1;
    }

    if (read_mem(addr, biosBuf, *size) == -1)
    {
	debug_print("BIOS read fail at 0x%x: %s", addr, strerror(errno));
	return errno;
    }

    return 0;
}

static u_char
bios_sum(u_long size)
{
    u_long	addr;
    u_char	csum = 0;

    for (addr = 0; addr < size; addr++)
        csum += biosBuf[addr];

    return csum ? ENOENT : 0;
}

static void
show_adapter_bios(FILE *out, u_long addr, u_long size)
{
    u_char	sum;

    fprintf(out, "\n    Adapter BIOS ROM\n");
    fprintf(out, "\tAddress:        0x%x - 0x%x\n", addr, addr + size - 1);
    fprintf(out, "\tSize:           %gKb\n", (double)size / 1024);

    sum = bios_sum(size);
    fprintf(out, "\tCheckSum:       0x%2.2x  %s\n",
				    sum,
				    sum ? "* * Invalid * *" : "(As expected)");

    if (verbose)
	bios_strings(out, size);
}

static int
show_system_bios(FILE *out, u_long next)
{
    u_long	addr;
    u_long	size;
    char	*msg;
    u_char	*tp;

    if (next > 0xe0000UL)
    {
	addr = next;
	size = 0x100000UL - next;
    }
    else
    {
	addr = 0xe0000UL;
	size = 128 * 1024;
    }

    fprintf(out, "\n    System BIOS ROM\n");
    fprintf(out, "\tAddress:        0x%x - 0x%x\n", addr, addr + size - 1);
    fprintf(out, "\tSize:           %gKb\n", (double)size / 1024);

    if (size > sizeof(biosBuf))
    {
	debug_print("System BIOS is too large: %gKb\n", (double)size / 1024);
	return ENOMEM;
    }

    if (read_mem(addr, biosBuf, size) == -1)
    {
	debug_print("BIOS read fail at 0x%x: %s", addr, strerror(errno));
	return errno;
    }
    
    /*
     * FFFFh:0005h contains the ROM BIOS Release Date in ASCII
     * (e.g., '06/10/85').  Later versions of the Compaq have
     * this release date shifted by 1 byte, to start at
     * FFFFh:0006h.
     */

    tp = &biosBuf[0xffff5 - addr];
    if (!isprint(*tp))
	++tp;

    fprintf(out, "\tBIOS Date:      %.8s\n", tp);

    /*
     * The System Identification byte is located at FFFFh:000Eh.
     *  The submodel and revision bytes can be obtained by
     * calling Int 15h, service C0h.
     *
     *                      System Identification Byte
     *
     *                              BIOS      Model     Submodel
     *          Product             Date      Byte      Byte      Revision
     *           PC                 04/24/81  FF        --        --
     *           PC                 10/19/81  FF        --        --
     *           PC                 10/27/82  FF        --        --
     *           PC XT              11/8/82   FE        --        --
     *           PC XT              1/10/86   FB        00        01
     *           PC XT              5/9/86    FB        00        02
     *           PCjr               6/1/83    FD        --        --
     *           AT                 1/10/84   FC        --        --
     *           AT                 6/10/85   FC        00        01
     *           AT                 11/15/85  FC        01        00
     *           PS/2 Model 25      6/26/87   FA        01        00
     *           PS/2 Model 30      9/2/86    FA        00        00
     *           PS/2 Model 30      12/12/86  FA        00        01
     *           PS/2 Model 50      2/13/87   FC        04        00
     *           PS/2 Model 60      2/13/87   FC        05        00
     *           PS/2 Model 80      3/30/87   F8        00        00
     *           PS/2 Model 80      10/07/87  F8        01        00
     *           XT-286             4/21/86   FC        02        00
     *           PC Convertible     9/13/85   F9        00        00
     *
     * ZDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD?
     * 3 ROM BIOS    Z model byte                                            3
     * 3 copyright   3    Z submodel byte          machine                   3
     * 3   date      3    3    Z revision                                    3
     * CDDDDDDDDDDEDDDDEDDDDEDDDDEDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD4
     * 3          3 00 3 00 3 00 3 AT&T 6300, Olivetti PC                    3
     * 3          3 2D 3 -- 3 -- 3 Compaq PC        (4.77mHz original)       3
     * 3          3 30 3 -- 3 -- 3 Sperry PC        (built by Mitsubishi)    3
     * 3          3 86 3 -- 3 -- 3 HP-110 portable PC                        3
     * 3          3 9A 3 -- 3 -- 3 Compaq Plus      (XT compatible)          3
     * 3 03/30/87 3 F8 3 00 3 00 3 PS/2 Model 80  8580-041 (16mhz)  (-071?)  3
     * 3 08/28/87 3 F8 3 ?? 3 ?? 3 PS/2 Model 80-071  16mHz  8580            3
     * 3 10/07/87 3 F8 3 01 3 00 3 PS/2 Model 80  8580-111/311 (20mhz)       3
     * 3 09/17/87 3 F8 3 01 3 01 3 PS/2 Model 80-111  20mHz  8580            3
     * 3 11/21/89 3    3    3    3 PS/2 Model 80-Axx                         3
     * 3 04/11/88 3 F8 3 04 3 02 3 PS/2 Model 70-121 8570-121, 8570-E61      3
     * 3 04/11/88 3 F8 3 09 3 02 3 PS/2 Model 70 desktop                     3
     * 3 01/18/89 3 F8 3 0B 3 00 3 PS/2 Model 70 Portable                    3
     * 3 01/18/89 3    3    3    3 PS/2 Model 73                             3
     * 3 01/29/88 3    3    3    3 PS/2 Model 70                             3
     * 3 03/17/89 3    3    3    3 PS/2 Model 70-061                         3
     * 3 03/17/89 3    3    3    3 PS/2 Model 70-121                         3
     * 3 02/20/89 3    3    3    3 PS/2 Model 70-A21                         3
     * 3 02/20/89 3    3    3    3 PS/2 Model 70-A61                         3
     * 3 10/02/89 3    3    3    3 PS/2 Model 70-B21                         3
     * 3 12/01/89 3    3    3    3 PS/2 Model 70-A61 --> B61                 3
     * 3 09/09/88 3 F8 3 0B 3 01 3 PS/2 8573-???                             3
     * 3       ?  3 F8 3 0C 3 00 3 PS/2 8555-031/061                         3
     * 3 02/20/89 3 F8 3 0D 3  ? 3 PS/2 Model 70-A21                         3
     * 3 06/22/88 3 F8 3 0D 3 00 3 PS/2 Model 70 8570-A21                    3
     * 3 09/13/85 3 F9 3 00 3 00 3 PC Convertible laptop                     3
     * 3 09/02/86 3 FA 3 00 3 00 3 PS/2 Model 30 8530-021                    3
     * 3 12/12/86 3 FA 3 00 3 00 3 PS/2 Model 30 8530-021                    3
     * 3 02/05/87 3    3    3    3 PS/2 Model 30 8530-021                    3
     * 3 08/25/88 3    3    3    3 PS/2 Model 30 8530-E21                    3
     * 3 05/16/88 3    3    3    3 PS/2 Model 30 8530-E21                    3
     * 3 06/28/89 3    3    3    3 PS/2 Model 30 8530-Exx                    3
     * 3 06/26/87 3 FA 3 01 3 00 3 PS/2 Model 25 8525                        3
     * 3 01/10/86 3 FB 3 00 3 00 3 XT-2 (early)                              3
     * 3 01/10/86 3 FB 3 00 3 01 3 XT Model 089   (101-key keyboard          3
     * 3 05/09/86 3 FB 3 01 3 02 3 XT-2 (revised) (640k m'bd, 101 key k'bd   3
     * 3 01/10/84 3 FC 3 -- 3 -- 3 AT Model 099 (original 6mHz)              3
     * 3 06/10/85 3 FC 3 00 3 01 3 AT Model 5170-239 6mHz (6.6 max governor) 3
     * 3 11/15/85 3 FC 3 01 3 00 3 AT Model 5170-339 8mHz (8.6 max governor) 3
     * 3          3 FC 3 01 3 00 3 Compaq 386/16                             3
     * 3          3 FC 3 01 3 03 3 some Phoenix 386 BIOS                     3
     * 3          3 FC 3 01 3 81 3 some Phoenix 386 BIOS                     3
     * 3 04/21/86 3 FC 3 02 3 00 3 XT/286                                    3
     * 3 02/13/87 3 FC 3 04 3 00 3 PS/2 Model 50 8550-021                    3
     * 3 12/22/86 3 FC 3 05 3 00 3 PS/2 Model 60 8560                        3
     * 3 02/13/87 3 FC 3 05 3 00 3 PS/2 Model 60 8560                        3
     * 3          3 FC 3 00 3    3 7531/2 Industrial AT                      3
     * 3          3 FC 3 06 3    3 7552 "Gearbox"                            3
     * 3 04/18/88 3 FC 3 04 3 03 3 PS/2 50Z  8550-031/061                    3
     * 3 01/24/90 3 FC 3 01 3 00 3 Compaq Deskpro 80386/25e                  3
     * 3 10/02/89 3 FC 3 02 3 00 3 Compaq Deskpro 386s, 386SX, 16mHz         3
     * 3 08/25/88 3 FC 3 09 3 00 3 8530-Exx (286)                            3
     * 3 06/01/83 3 FD 3 -- 3 -- 3 PCjr                                      3
     * 3 11/08/82 3 FE 3 -- 3 -- 3 XT, Portable PC, XT/370, 3270PC           3
     * 3 04/24/81 3 FF 3 -- 3 -- 3 PC-0   (original)(16k motherboard)        3
     * 3 10/19/81 3 FF 3 -- 3 -- 3 PC-1             (64k motherboard)        3
     * 3 08/16/82 3 FF 3 -- 3 -- 3 PC, XT, XT/370   (256k motherboard)       3
     * 3 10/27/82 3 FF 3 -- 3 -- 3 PC with HD/EGA BIOS upgrade chipset       3
     * 3 02/08/90 3    3    3    3 PS/2 Model 65                             3
     * 3 11/02/88 3    3    3    3 PS/2 Model 55SX                           3
     * 3 02/07/89 3    3    3    3 PS/2 Model 73-031                         3
     * @DDDDDDDDDDADDDDADDDDADDDDADDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDY
     */

    tp = &biosBuf[0xffffe - addr];
    switch (tp[0])
    {
	case 0xff:
	    msg = "PC";
	    break;

	case 0xfe:
	case 0xfb:
	    msg = "PC XT";
	    break;

	case 0xfd:
	    msg = "PCjr";
	    break;

	case 0xfc:
	    msg = "PC/XT-286";
	    break;

	case 0xfa:
	    msg = "PS/2 Model 25/30";
	    break;

	case 0xf8:
	    msg = "PS/2 Model 80";
	    break;

	case 0xf9:
	    msg = "PC Convertible";
	    break;

	default:
	    msg = "other";
    }

    fprintf(out, "\tBIOS Catagory:  ");
    if (verbose)
	fprintf(out, "0x%2.2x  ", tp[0]);
    fprintf(out, "IBM %s\n", msg);

    /*
     * ##
     * To identify the submodel you need to do  an int 15h,
     * AH=0C0h (Return System Configuration Parameters).  This
     * call is not supported in early XT and AT BIOSes.
     */

    /*
     * Strings
     */

    if (verbose)
	bios_strings(out, size);
    return 0;
}

/*
 * Print strings that are interesting:
 *	Are at least 10 characters long
 *	Contain at least one space character
 *	Has at least 5 letters
 */

#define MIN_INTERESTING_LEN	10
#define MIN_INTERESTING_ALPHA	5

static void
bios_strings(FILE *out, u_long size)
{
    u_long	addr;
    int		first = 1;

    for (addr = 0; addr < size; ++addr)
    {
	u_long	n;
	int	hasSpace = 0;
	int	alphaCnt = 0;
	u_long	len;

	for (n = addr; (n < size) && isprint(biosBuf[n]); ++n)
	{
	    if (biosBuf[n] == ' ')
		hasSpace = 1;
	    if (isalpha(biosBuf[n]))
		++alphaCnt;
	}

	len = n - addr;
	if (hasSpace &&
	    (len >= MIN_INTERESTING_LEN) &&
	    (alphaCnt >= MIN_INTERESTING_ALPHA))
	{
	    while (isspace(biosBuf[addr]))
	    {
		++addr;
		--len;
	    }

	    while (len && isspace(biosBuf[addr+len-1]))
		--len;

	    if (len >= MIN_INTERESTING_LEN)
	    {
		if (first)
		{
		    fprintf(out, "\n\tMessages found in the BIOS ROM:\n");
		    first = 0;
		}

		fprintf(out, "\t    %.*s\n", len, &biosBuf[addr]);
	    }
	}

	addr = n;
    }
}

