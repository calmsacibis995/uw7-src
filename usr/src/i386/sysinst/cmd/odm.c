#ident	"@(#)odm.c	15.1"
/*  inetinst
 *
 *  ODM_ODI.C  is specifically for ODI drivers.  How ODI drivers
 *  are normally configured is by passing a command line to a lower
 *  support level from the HSM.
 *
 *  ODM (for ODI) replaces the command-line in the HSM so that, at
 *  at initialization, the correct config parameters are set.
 *  
 *  This replaces the need to use the IDTOOLS to configure a driver.
 *  ODM takes as an argument the name of a file (in the current directory)
 *  that is a built loadable ODI module (built using idbuild -M from the
 *  Driver.o and space.c for the driver).
 */

#include <fcntl.h>
#include <stdio.h>
#include <nlist.h>
#include <unistd.h>
#include <libelf.h>
#include <sys/types.h>
#include <sys/stat.h>

#define ODM_BUFSZ 256	/*  Size of an ODI cmdLine buffer for ODM  */

/*
 *  This struct has been added because of current problems with the
 *  ODI header files.  (as of 2/3/94)
 */
typedef struct kludge {
        /*
         * all modifiable fields in this struct are protected by bd_lock.
         */
        major_t         bd_major;       /* major number for device */
        ulong_t         bd_io_start;    /* start of I/O base address */
        ulong_t         bd_io_end;      /* end of I/O base address */
        paddr_t         bd_mem_start;   /* start of base mem address */
        paddr_t         bd_mem_end;     /* start of base mem address */
        int             bd_irq_level;   /* interrupt request level */
        int             bd_max_saps;    /* max service access points (minors) */
        char            *bd_cmdLine;    /* configuration command line */
} my_config_t;


main(int argc, char *argv[])
{
	char		*progname;		/* buffer hold cmd name */
	char		*cmdbuf;		/* buffer hold cmdLine */
	char		*filename;		/* name of dlpi_eth driver */
	char		sym_config[48];		/* name of config symbol */
	char		sym_cmdLine[48];	/* name of cmdLine symbol */
	char		my_cmdLine[ODM_BUFSZ];	/* ODI cmdLine string */
	my_config_t	my_config;		/* ODI config structure */
	struct nlist	odm_nlist[3];		/* nlist for locating config */
	int 		odm_fd;			/* fd - loadable module */
	major_t         major=0;          	/* major number for device */
        ulong_t         io_start=0;       	/* start of I/O base address */
        ulong_t         io_end=0;         	/* end of I/O base address */
        paddr_t         mem_start=0;      	/* start of base mem address */
        paddr_t         mem_end=0;        	/* start of base mem address */
        int             irq_level=0;      	/* interrupt request level */
	Elf		*elf;			/* ELF Pointer */
	Elf_Scn		*elf_scn;		/* ELF Section */
	Elf32_Ehdr	*elf_ehdr;		/* ELF Header */
	Elf32_Shdr	*elf_shdr;		/* ELF Section Header */
	Elf_Data	*elf_data;		/* ELF Data */
	int		c;			/* For parsing command line */
	int		Cflag=0;		/* Command line for ODI */
	int		Pflag=0;		/* Port (I/O) address */
	int		Mflag=0;		/* Mem address */
	int		Iflag=0;		/* IRQ */
	int		Dflag=0;		/* DLPI Driver */
	int		Oflag=0;		/* ODI Driver */

	progname = (char *)strdup(argv[0]);
	opterr = 0;
	while((c = getopt(argc, argv, "c:p:m:i:od")) != -1) {
	switch (c) {
		case '?':
			usage(progname);
			break;
		case 'o':			/* This is an ODI driver */
			if (Dflag) {
				usage(progname);
				break;
			}
			Oflag++;
			break;
		case 'd':			/*  This is a DLPI driver */
			if (Oflag) {
				usage(progname);
				break;
			}
			Dflag++;
			break;
		case 'i':			/*  IRQ Level  */
			if (Iflag) {
				usage(progname);
				break;
			}
			/* irq_level = atoi(optarg); */
			if (!sscanf(optarg, "%d", &irq_level)) {
				usage(progname);
				break;
			}
			Iflag++;
			break;
		case 'p':			/*  Port (I/O) Address  */
			if (Pflag) {
				usage(progname);
				break;
			}
			if (sscanf(optarg, "%x %x", &io_start, &io_end)!=2) {
				usage(progname);
				break;
			}
			Pflag++;
			break;
		case 'm':			/*  Memory Address  */
			if (Mflag) {
				usage(progname);
				break;
			}
			if (sscanf(optarg, "%x %x", &mem_start, &mem_end)!=2) {
				usage(progname);
				break;
			}
			Mflag++;
			break;
		case 'c':			/*  ODI Command line */
			Cflag++;
			cmdbuf = (char *)strdup(optarg);
			break;
		default:
			usage(progname);
			break;
	}
	}

	/*
	 *  Make sure we have an argument - the driver to be modified.
	 */
        if (optind == argc) {
                usage(progname);
        } else
	filename = (char *)strdup(argv[optind]);

	/*
	 *  From the command line argument, determine the module and
	 *  symbol(s) we want to change.
	 *  The two symbols are the bd_config structure and the cmdLine
	 *  that the hsm passes down.
	 */
	strcpy(sym_config, filename);
	strcat(sym_config, "config");
	strcpy(sym_cmdLine, filename);
	strcat(sym_cmdLine, "_0_cmdLine");

	/*
	 *  Catch some impossible combinations here
	 */
	if (Dflag && Cflag) {		/* No cmd line for DLPI Drivers */
		usage(progname);
 	}
	if (Oflag && !Cflag) { 		/*  Must have cmd line for ODI */
		usage(progname);
	}
	if (Pflag && (io_start >= io_end)) {	/* Start must be < end! */
		usage(progname);
	}
	if (Mflag && (mem_start >= mem_end)) {	/* Start must be < end! */
		usage(progname);
	}

	/*
	 *  Assemble the nlist struct we're going to use to get the
	 *  address of the config structure in the driver.
	 */
	odm_nlist[0].n_name = sym_config;
	odm_nlist[1].n_name = sym_cmdLine;
	odm_nlist[2].n_name = "";

	/* 
	 *  Perform the nlist on the driver
	 */
	if (nlist(filename, odm_nlist) < 0) {
		fprintf(stderr, "Couldn't nlist %s\n", filename);
		exit(1);
	}

	/*
	 *  Open the driver so that we can perform the necessary
	 *  surgery.
	 */
	if ((odm_fd = open(filename, O_RDWR)) < 0) {
		fprintf(stderr, "Couldn't open %s\n", filename);
		exit(2);
	}

	/*
	 *  Now we have to go and get the ELF section in which the
	 *  config symbol we want is located.
	 */
	elf = elf_begin(odm_fd, ELF_C_RDWR, NULL);
	elf_scn = elf_getscn(elf, (size_t)odm_nlist[0].n_scnum);

	/*
	 *  Now that we've got the section, get the header and make
	 *  sure that what we got was a .data section.
	 */
	elf_shdr =elf32_getshdr(elf_scn);
	if (elf_shdr->sh_type != SHT_PROGBITS)
	{
		fprintf(stderr, "Not a .data section\n");
	}

	/*
	 *  Retrieve the data from the .data section
	 */
	elf_data = 0;
	if ((elf_data = elf_getdata(elf_scn, elf_data)) == 0 ||
	    elf_data->d_size == 0) {
		fprintf(stderr, "elf_getdata failed\n");
	}

	/*
	 *  Copy the command line and config struct into our
	 *  private work area.
	 */
	memcpy(my_cmdLine, (off_t)(elf_data->d_buf) +
	    (off_t)odm_nlist[1].n_value, 80);

	memcpy(&my_config, (off_t)(elf_data->d_buf) +
	    (off_t)odm_nlist[0].n_value, sizeof(my_config));

	fprintf(stderr, "BEFORE: <%s>\n", my_cmdLine);
	fprintf(stderr, "       IRQ=%d, MEM=%x %x, IO=%x %x\n",
		my_config.bd_irq_level, my_config.bd_mem_start,
		my_config.bd_mem_end, my_config.bd_io_start, 
		my_config.bd_io_end);

	/*
	 *  Copy the command line that was passed us
	 */
	strcpy(my_cmdLine, cmdbuf);

	/*
	 *  Update the bd_config struct with the values passed on
	 *  the command line.
	 */
	if (Iflag) {
		my_config.bd_irq_level = irq_level;
	}
	if (Pflag) {
		my_config.bd_io_start = io_start;
		my_config.bd_io_end = io_end;
	}
	if (Mflag) {
		my_config.bd_mem_start = mem_start;
		my_config.bd_mem_end = mem_end;
	}

	
	/*
	 *  Update the cmdLine array and the bd_config structure in the
	 *  HSM with the indicated values and copy it back into the .data
	 *  section we are working with.
	 */
	memcpy((off_t)(elf_data->d_buf) + (off_t)odm_nlist[1].n_value,
		&my_cmdLine, sizeof(my_cmdLine));
	memcpy((off_t)(elf_data->d_buf) + (off_t)odm_nlist[0].n_value,
		&my_config, sizeof(my_config));

	fprintf(stderr, "AFTER: <%s>\n", my_cmdLine);
	fprintf(stderr, "       IRQ=%d, MEM=%x %x, IO=%x %x\n",
		my_config.bd_irq_level, my_config.bd_mem_start,
		my_config.bd_mem_end, my_config.bd_io_start, 
		my_config.bd_io_end);


	/*
	 *  Now write the .data section back into the ELF file
	 */
	elf_update(elf, ELF_C_WRITE);
	elf_end(elf);

	exit(0);
}

usage(char *progname)
{
        fprintf(stderr, "%s: Usage: %s -{d|o} -i irq_level -p \"port_start port_end\" -m \"mem_start mem_end\" -c cmd_line\n", progname, progname);
        exit(1);
}

