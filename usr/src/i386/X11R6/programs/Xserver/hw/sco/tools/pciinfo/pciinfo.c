/*
 *	@(#)pciinfo.c	11.1
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1994.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *	SCO MODIFICATION HISTORY
 *
 * S009, 30-Oct-95, kylec
 * 	Add -n option to detect nth instance 
 * S008, 15-Mar-95, davidw
 *	Add additional error message when /dev/pci doesn't open.
 * S007, 12-Aug-94, rogerv
 *	Added "x" flag for debugging printfs.  Changed "while" loop to
 *	"for" loop for robustness.  Put in 2 patches, one for the
 *	15,000 PCI buses everest kernel boot fix, and one to make sure
 *	that the program terminates even if PCI_SEARCH_BUS doesn't.
 * S006, 09-Aug-94, rogerv
 *	Corrected checking of pci bus ioctl return value to look for
 *	(-1) for an error.
 * S005, 08-Aug-94, rogerv
 *	Rewritten to emulate the everest pci bus ioctls for tbird
 *	(agaII defined), and use the ioctls for everest (agaII not
 *	defined). This gives access to pci bus configuration space
 *	without	going through the PCI Bios v2.0 (int1AH) interfaces.
 * S004, 25-Jul-94, davidw
 *	p - present status option added
 * S003, 24-Feb-94, staceyc
 * 	B, D, and W options added
 * S002, 17-Feb-94, staceyc
 * 	options and usage added
 * S001, 16-Feb-94, staceyc
 * 	fixed class wierdness, need to add options in next rev
 * S000, 14-Feb-94, staceyc
 * 	created - mostly derived from Buck's vesainfo
 */

#include <stdio.h>

#ifdef agaII
#include "pci.h"

/* Includes needed to make sysi86() call to enable this process to do I/O. */
#include  <sys/types.h>
#include <sys/param.h>
#include <sys/tss.h>
#include <sys/immu.h>
#include <sys/region.h>
#include <sys/proc.h>
#include <sys/v86.h>
#include <sys/sysi86.h>

#else /* agaII */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "pcix.h"

#endif /* agaII */

#include "pcidefs.h"


int vbopts = 0; /* vbios options */

#ifndef agaII
static int pci_fd;  /* file descriptor for pci device file */
/* int ioctl(int fildes, int request, arg); */
#endif /* agaII */

static char *program_name;

static void
ReadConfigSpace(pci_config_header *pci_config, long Offset, char type_to_read,
             struct pci_headerinfo pci_headerinfo)
{
  struct pci_configdata configdata ;

  configdata.reg = Offset ;
  configdata.data = 0 ;
  switch (type_to_read)
    {
      case 'd':
        configdata.size = sizeof(long) ;
        break ;
      case 'b':
        configdata.size = sizeof(char) ;
        break ;
      case 'w':
        configdata.size = sizeof(short) ; 
        break ;
    } 
}


static void
LoadPCIStats(pci_config_header *pci_config,
             struct pci_headerinfo pci_headerinfo)
{
    int i;
    struct pci_configdata configdata;
    
    /*
     * First, get the vendor id and device id from PCIbus configuration
     * space for this device...
     */
    configdata.info = pci_headerinfo.info;
    /* should only have to copy device info once */
    configdata.reg = PCI_VENDOR_DEVICE_ID_OFFSET;
    configdata.size = sizeof(unsigned long);
    configdata.data = 0; /* insure that value is valid */
#ifdef agaII
    if (pci_read_config_dword(&configdata) != 1)
    {
#else /* agaII */
    if (ioctl(pci_fd, PCI_READ_CONFIG, &configdata) == -1)
    {
#endif /* agaII */
        fprintf(stderr,
                "%s: can't read vendor and device id's for PCI device:\n",
                program_name);
        fprintf(stderr, "    slot=%x, function=%x, bus=%x.\n",
                pci_headerinfo.info.slotnum,
                pci_headerinfo.info.funcnum,
                pci_headerinfo.info.busnum);
    }
    pci_config->vendor_id =
        (unsigned short) configdata.data & 0x0ffff;
    pci_config->device_id =
        (unsigned short) ((configdata.data >> 16) &
                          0x0ffff);
    /*
     * Status and command registers next...
     */
    configdata.reg = PCI_STATUS_COMMAND_OFFSET;
    configdata.size = sizeof(unsigned long);
    configdata.data = 0;
#ifdef agaII
    if (pci_read_config_dword(&configdata) != 1)
    {
#else /* agaII */
    if (ioctl(pci_fd, PCI_READ_CONFIG, &configdata) == -1)
    {
#endif /* agaII */
        fprintf(stderr,
                "%s: can't read command and status registers for PCI device:\n",
                program_name);
        fprintf(stderr, "    slot=%x, function=%x, bus=%x.\n",
                pci_headerinfo.info.slotnum,
                pci_headerinfo.info.funcnum,
                pci_headerinfo.info.busnum);
    }
    pci_config->command = 
        (unsigned short) configdata.data & 0x0ffff;
    pci_config->status = 
        (unsigned short) ((configdata.data >> 16) &
                          0x0ffff);
    /*
     * Get class and revision next...
     */
    configdata.reg = PCI_REV_CLASS_OFFSET;
    configdata.size = sizeof(unsigned long);
    configdata.data = 0;
#ifdef agaII
    if (pci_read_config_dword(&configdata) != 1)
    {
#else /* agaII */
    if (ioctl(pci_fd, PCI_READ_CONFIG, &configdata) == -1)
    {
#endif /* agaII */
        fprintf(stderr,
                "%s: can't read class and revision id for PCI device:\n",
                program_name);
        fprintf(stderr, "    slot=%x, function=%x, bus=%x\n",
                pci_headerinfo.info.slotnum,
                pci_headerinfo.info.funcnum,
                pci_headerinfo.info.busnum);
    }
    pci_config->class_revision = configdata.data;
    /*
     * Cache line size, latency timer, header type, and bist next...
     */
    configdata.reg = PCI_CACHE_LINE_ETC_OFFSET;
    configdata.size = sizeof(unsigned long);
    configdata.data = 0;
#ifdef agaII
    if (pci_read_config_dword(&configdata) != 1)
    {
#else /* agaII */
    if (ioctl(pci_fd, PCI_READ_CONFIG, &configdata) == -1)
    {
#endif /* agaII */
        fprintf(stderr, "%s: can't read cache line size, latency timer, header type,\n",
                program_name);
        fprintf(stderr, "  and BIST for PCI device:\n");
        fprintf(stderr, "    slot=%x, function=%x, bus=%x.\n",
                pci_headerinfo.info.slotnum,
                pci_headerinfo.info.funcnum,
                pci_headerinfo.info.busnum);
    }
    pci_config->cache_line_size = (unsigned char) configdata.data & 0x0ff;
    pci_config->latency_timer = (unsigned char) (configdata.data >> 8) &
        0x0ff;
    pci_config->header_type = (unsigned char) (configdata.data >> 16) &
        0x0ff;
    pci_config->bist = (unsigned char) (configdata.data >> 24) &
        0x0ff;
    /*
     * Now the Base Address Registers...
     */
    for (i = 0; i < BASE_ADDRESS_DWORDS; i++)
    {
        configdata.reg = PCI_BASE_REGISTERS_OFFSET +
            (i * sizeof(unsigned long));
        configdata.size = sizeof(unsigned long);
        configdata.data = 0;
#ifdef agaII
        if (pci_read_config_dword(&configdata) != 1)
        {
#else  /* agaII */
        if (ioctl(pci_fd, PCI_READ_CONFIG, &configdata) == -1)
        {
#endif /* agaII */
            fprintf(stderr,
                    "%s: can't read base address register %d for PCI device:\n",
                    program_name, i);
            fprintf(stderr, "    slot=%x, function=%x, bus=%x.\n",
                    pci_headerinfo.info.slotnum,
                    pci_headerinfo.info.funcnum,
                    pci_headerinfo.info.busnum);
        }
        pci_config->base_address[i] = configdata.data;
    }
    /*
     * Next, get the expansion ROM base address...
     */
    configdata.reg = PCI_EXP_ROM_BASE_ADDR_OFFSET;
    configdata.size = sizeof(unsigned long);
    configdata.data = 0;
#ifdef agaII
    if (pci_read_config_dword(&configdata) != 1)
    {
#else /* agaII */
    if (ioctl(pci_fd, PCI_READ_CONFIG, &configdata) == -1)
    {
#endif /* agaII */
        fprintf(stderr, "%s: can't read expansion ROM base address for PCI device:\n",
                program_name);
        fprintf(stderr, "    slot=%x, function=%x, bus=%x\n",
                pci_headerinfo.info.slotnum,
                pci_headerinfo.info.funcnum,
                pci_headerinfo.info.busnum);
    }
    pci_config->exp_rom_base_addr = configdata.data;
    
    /*
     * Last, get interrupt line, interrupt pin, min_gnt, and max_lat...
     */
    configdata.reg = PCI_INTERRUPT_LINE_ETC_OFFSET;
    configdata.size = sizeof(unsigned long);
    configdata.data = 0;
#ifdef agaII
    if (pci_read_config_dword(&configdata) != 1)
    {
#else  /* agaII */
    if (ioctl(pci_fd, PCI_READ_CONFIG, &configdata) == -1)
    {
#endif /* agaII */
        fprintf(stderr,
                "%s: can't read interrupt line, interrupt pin, min_gnt,\n",
                program_name);
        fprintf(stderr, "  and max_lat for PCI device:\n");
        fprintf(stderr, "    slot=%x, function=%x, bus=%x.\n",
                pci_headerinfo.info.slotnum,
                pci_headerinfo.info.funcnum,
                pci_headerinfo.info.busnum);
    }
    pci_config->interrupt_line = (unsigned char) configdata.data & 0x0ff;
    pci_config->interrupt_pin = (unsigned char) (configdata.data >> 8) &
        0x0ff;
    pci_config->min_gnt = (unsigned char) (configdata.data >> 16) &
        0x0ff;
    pci_config->max_lat = (unsigned char) (configdata.data >> 24) &
        0x0ff;
}
            
static void
PrintPCIStats(
              pci_config_header *pci_config,
              int bflag)
{
    int count;
    
    if (bflag)
    {
        for (count = 0; count < BASE_ADDRESS_DWORDS; ++count)
            printf("0x%.8X\n", pci_config->base_address[count]);
        return;
    }
    
    printf("                 device id: 0x%.8X\n", pci_config->device_id);
    printf("                 vendor id: 0x%.8X\n", pci_config->vendor_id);
    printf("                    status: 0x%.8X\n", pci_config->status);
    printf("                   command: 0x%.8X\n", pci_config->command);
    printf("            class_revision: 0x%.8X\n",
           pci_config->class_revision);
    printf("                      bist: 0x%.8X\n", pci_config->bist);
    printf("               header type: 0x%.8X\n", pci_config->header_type);
    printf("             latency timer: 0x%.8X\n",
           pci_config->latency_timer);
    printf("           cache line size: 0x%.8X\n",
           pci_config->cache_line_size);
    for (count = 0; count < BASE_ADDRESS_DWORDS; ++count)
        printf("         base address[0x%.1X]: 0x%.8X\n", count,
               pci_config->base_address[count]);
    printf("expansion ROM base address: 0x%.8X\n",
           pci_config->exp_rom_base_addr);
    printf("                   max_lat: 0x%.8X\n", pci_config->max_lat);
    printf("                   min_gnt: 0x%.8X\n", pci_config->min_gnt);
    printf("             interrupt pin: 0x%.8X\n",
           pci_config->interrupt_pin);
    printf("            interrupt line: 0x%.8X\n",
           pci_config->interrupt_line);
    printf("\n");
}
            
static void
Usage(
      char *name)
{
    fprintf(stderr,
            "\n%s - PCI BIOS v2.0 query program\n\n", name);
    fprintf(stderr,
            "usage: %s [-d device_id] [-v vendor_id] [-n index] [-c class] [-o v86opts]\n", name);
    fprintf(stderr,
            "-d device_id        print information for device_id\n");
    fprintf(stderr,
            "-v vendor_id        print information for vendor_id\n");
    fprintf(stderr,
            "-n index            print information for nth device with matching vendor_id and device_id\n");
    fprintf(stderr,
            "-c class            print information for class\n");
    fprintf(stderr,
            "-b                  print only the %d base address dword registers\n",
            BASE_ADDRESS_DWORDS);
    fprintf(stderr,
            "-p                  print only the PCI BIOS present registers\n");
    fprintf(stderr,
            "-q                  print no output\n");
    fprintf(stderr,
            "-B offset           print the byte at byte offset in configuration space\n");
    fprintf(stderr,
            "-W offset           print the word at byte offset in configuration space\n");
    fprintf(stderr,
            "-D offset           print the dword at byte offset in configuration space\n");
    fprintf(stderr,
            "-o v86_options      options to be passed to v86bios code\n\n");
    fprintf(stderr,
            "-d, -v, and -c can be used simultaneously, only matching configuration\n");
    fprintf(stderr,
            "information will be displayed.  device_id, vendor_id, and class are\n");
    fprintf(stderr,
            "all numbers and can be specified in decimal, octal, or hexadecimal.\n");
    fprintf(stderr,
            "%s with no options prints all configuration information for all PCI\n", name);
    fprintf(stderr,
            "devices.  %s returns 0 if at least one matching configuration section\n",
            name);
    fprintf(stderr,
            "is found and 1 if none are found or an error occurs.\n");
}
                            
#define CHECK_DEVICE 0x1
#define CHECK_VENDOR 0x2
#define CHECK_CLASS  0x4
#define CHECK_INDEX  0x8
#define ALL_OK (CHECK_DEVICE | CHECK_VENDOR | CHECK_CLASS | CHECK_INDEX)
                            
main(
     int argc,
     char **argv)
{
    int index, status, print_ok;
    int maxindex;
    unsigned int class;
    struct pci_busdata pci_busdata; /* save data from PCI_PRESENT bios
                                       function */
    pci_config_header pci_config;   
    struct pci_headerinfo pci_headerinfo;
    unsigned char *pci_config_p;
    int c;
    unsigned int device_id, vendor_id, search_class, index_id;
    unsigned int Boffset, Woffset, Doffset;
    static int dflag, vflag, cflag, bflag, pflag, qflag, nflag;
    static int Bflag, Wflag, Dflag;
    static int xflag = 0;  /* debug flag */
    int xsearch_bus;  /* for debug only */
    int device_found = 1;
    extern char *optarg;
    extern int optind;
    
    program_name = argv[0];
    
    pci_config_p = (unsigned char *)&pci_config;
    
    while ((c = getopt(argc, argv, "n:xpqbd:v:c:o:B:W:D:h")) != -1)
    {
        switch (c)
        {
          case 'd' :
            dflag = 1;
            device_id = strtol(optarg, 0, 0);
            break;
          case 'n' :
            nflag = 1;
            index_id = strtol(optarg, 0, 0);
            break;
          case 'o' :
            vbopts = strtol(optarg, 0, 0);
            break;
          case 'v' :
            vflag = 1;
            vendor_id = strtol(optarg, 0, 0);
            break;
          case 'c' :
            cflag = 1;
            search_class = strtol(optarg, 0, 0);
            break;
          case 'b' :
            bflag = 1;
            break;
          case 'p' :
            pflag = 1;
            break;
          case 'q' :
            qflag = 1;
            break;
          case 'x' :
            xflag = 1;
            break;
          case 'B' :
            Bflag = 1;
            Boffset = strtol(optarg, 0, 0);
            if (Boffset >= sizeof(pci_config))
            {
                fprintf(stderr, "%s: byte offset too large.\n",
                        program_name);
                Usage(program_name);
                exit(1);
            }
            break;
          case 'W' :
            Wflag = 1;
            Woffset = strtol(optarg, 0, 0);
            if (Woffset >= (sizeof(pci_config) - sizeof(short)))
            {
                fprintf(stderr, "%s: word offset too large.\n",
                        program_name);
                Usage(program_name);
                exit(1);
            }
            break;
          case 'D' :
            Dflag = 1;
            Doffset = strtol(optarg, 0, 0);
            if (Doffset >= (sizeof(pci_config) - sizeof(long)))
            {
                fprintf(stderr, "%s: dword offset too large.\n",
                        program_name);
                Usage(program_name);
                exit(1);
            }
            break;
          case 'h' :
          case '?' :
            Usage(program_name);
            exit(1);
            break;
        }
    }
    
    if (nflag && !dflag && !vflag)
        exit(1);
    
    if (qflag)
        bflag = 0;
    if (Bflag || Wflag || Dflag)
        qflag = 1;
    
    pci_busdata.bus_present = 0;
#ifdef agaII
    if (pci_bus_present(&pci_busdata) == 0) {
#else /* agaII */
    /*
     * First, open pci device file, then check for bus present.
     */
    if ((pci_fd = open("/dev/X/pci", O_RDONLY)) == -1) {
        fprintf(stderr, "%s: unable to open /dev/X/pci\n", program_name);
        perror(program_name);
        fprintf(stderr, "%s: This computer does not have a PCI bus.\n",
                program_name);
        exit(1);
    }
    if ((ioctl(pci_fd, PCI_BUS_PRESENT, &pci_busdata) == -1) ||
        (pci_busdata.bus_present == 0)) {
#endif /* agaII */
        fprintf(stderr, "%s: pci_bus_present failed, no PCI bus found\n",
                program_name);
        exit(1);
    }
    /*
     * Patch for the everest ../boot/boot/bootsup.s bug that didn't
     * mask off the high order bytes...when fixed boots get out
     * there (this change is marked "L023" in bootsup.s), then
     * this can be removed.  But Beta releases did not have this fix.
     */
    pci_busdata.businfo.numbuses &= 0x00ff; /* bug: mask off high byte */
    pci_busdata.businfo.mechanism &= 0x00ff; /* bug: mask off high byte */
    
    if (pflag) {
        
        printf("               Present status: 0x%.2X\n", 
               pci_busdata.bus_present);
        printf("           Hardware mechanism: 0x%.2X\n", 
               pci_busdata.businfo.mechanism);
        printf("         Number of PCI buses : 0x%.2X\n", 
               pci_busdata.businfo.numbuses);
        exit(0);
    }
    /*
     * At this point, we know that there is at least one PCI
     * bus in this system.  Now, get the information about the
     * devices on the bus...
     * Note: for tbird, the names of the PCI access functions are
     *       the same as the current PCI device ioctls for everest.
     *
     * First, enable this process to do I/O to get the configuration
     * data.
     */
#ifdef agaII
    if (sysi86(SI86V86, V86SC_IOPL, 0x3000) < 0) {
        fprintf(stderr, "%s: cannot get I/O privileges\n",
                program_name);
        perror(program_name);
        exit(1);
    }
#endif /* agaII */

    maxindex = pci_busdata.businfo.numbuses *
        ((pci_busdata.businfo.mechanism == 1) ?
         PCI_DEVS_MECH1 :
         PCI_DEVS_MECH2) * PCI_MAX_FUNCS;
    if (xflag)
        printf("main(): maxindex %d\n", maxindex);
    for (index = 0; index < maxindex; index++) {
        pci_headerinfo.vend_id = 0x0ffff;
        /* find any device */
        pci_headerinfo.dev_id = 0x0ffff;
        pci_headerinfo.base_class = 0x0ffff;
        pci_headerinfo.sub_class = 0x0ffff;
        pci_headerinfo.index = index;

#ifdef agaII
        if ((xsearch_bus = pci_search_bus(pci_busdata,
            &pci_headerinfo)) != -1) {
#else /* agaII */
        if ((xsearch_bus = ioctl(pci_fd, PCI_SEARCH_BUS, &pci_headerinfo)) != -1) {
#endif /* agaII */
            if (xflag)
                printf("main(): pci_search_bus %d, index %d\n",
                       xsearch_bus, index);
            if (pci_headerinfo.device_present != 1) {
                if (xflag)
                    printf("main(): got device_present %d, index %d\n",
                           pci_headerinfo.device_present,
                           index);
                break;
            } 
            else 
            {
                /*
                 * Found device on bus, and have device data for
                 * it...Now it is time to get the configuration
                 * data for the device.
                 */
                if (xflag)
                    printf("main(): got a device, device_present is %d, index %d, slot=%x, function=%x, bus=%x.\n",
                           pci_headerinfo.device_present,
                           index, 
                           pci_headerinfo.info.slotnum,
                           pci_headerinfo.info.funcnum,
                           pci_headerinfo.info.busnum);
                /*
                 * Patch for everest pci driver problem
                 * where it keeps reporting the same
                 * device over and over again when it
                 * has actually run out of devices...
                 */
                if (pci_headerinfo.info.busnum >=
                    pci_busdata.businfo.numbuses)
                    break;
                
                
                LoadPCIStats(&pci_config, pci_headerinfo);
                if (Bflag)
                    ReadConfigSpace(&pci_config, Boffset, 
                                    'b', pci_headerinfo);
                else if (Dflag)
                    ReadConfigSpace(&pci_config, Doffset, 
                                    'd', pci_headerinfo);
                else if (Wflag)
                    ReadConfigSpace(&pci_config, Woffset,
                                    'w', pci_headerinfo);
                print_ok = 0;
                if (vflag) {
                    if (pci_config.vendor_id == vendor_id)
                        print_ok |= CHECK_VENDOR;
                } else
                    print_ok |= CHECK_VENDOR;
                if (cflag) {
                    if ((pci_config.class_revision >>
                         24) ==
                        search_class)
                        print_ok |= CHECK_CLASS;
                } else
                    print_ok |= CHECK_CLASS;
                if (dflag) {
                    if (pci_config.device_id == device_id)
                        print_ok |= CHECK_DEVICE;
                } else
                    print_ok |= CHECK_DEVICE;
                
                if (nflag)
                {
                    if (vflag && dflag &&
                        (print_ok == (CHECK_DEVICE |
                                      CHECK_CLASS |
                                      CHECK_VENDOR))) {
                        struct pci_devicedata device_data;
                        device_data.vend_id = vendor_id;
                        device_data.dev_id = device_id;
                        device_data.index = index_id;
                        if ((ioctl(pci_fd,
                                   PCI_FIND_DEVICE,
                                   &device_data) >= 0) &&
                            (device_data.device_present == 1) &&
                            (device_data.info.slotnum == pci_headerinfo.info.slotnum))
                        {
                            print_ok |= CHECK_INDEX;
                        }
                    }
                }
                else
                    print_ok |= CHECK_INDEX;
                
                if (print_ok == ALL_OK) {
                    if (! qflag)
                        PrintPCIStats(&pci_config,
                                      bflag);
                    if (Bflag) 
                        printf("0x%.2X\n",
                               pci_config_p[Boffset]);
                    if (Wflag) {
                        unsigned short *p = (unsigned
                                             short *)
                            (&pci_config_p[Woffset]);
                        printf("0x%.4X\n", *p);
                    }
                    if (Dflag) {
                        unsigned long *p = (unsigned
                                            long *)
                            (&pci_config_p[Doffset]);
                        printf("0x%.8X\n", *p);
                    }
                    device_found = 0;
                }
            }
        }
        else 
        {                       /* error encountered... */
            fprintf(stderr,
                    "%s: PCI bus device search failed, possible access method error:\n",
                    program_name);
            fprintf(stderr,
                    "    access method is %d, should be 1 or 2.\n",
                    pci_busdata.businfo.mechanism);
            perror(program_name);
            exit(1);
        }
    } /* end while */
    return (device_found);
}
