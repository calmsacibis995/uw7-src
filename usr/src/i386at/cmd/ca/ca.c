#ident	"@(#)ca:ca/ca.c	1.3.1.1"

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/nvm.h>
#include <sys/cm_i386at.h>
#include <sys/ca.h>
#include <sys/eisa.h>

/* size of the data in eisa cmos */

/* extern int strlen(); */
extern int strncmp();
extern int strcmp();

static char data[EISA_BUFFER_SIZE];

static char *memory_type[] = {
	"System (base or extended)",
	"Expanded",
	"Virtual",
	"Other"
};

static char *memory_decode[] = {
	"20 Bits",
	"24 Bits",
	"32 Bits"
};

static char *width[] = {
	"Byte",
	"Word",
	"Double Word"
};

static char *dma_timing[] = {
	"ISA Compatible",
	"Type A",
	"Type B",
	"Type C (BURST)",
};

main(int argc, char **argv)
{
	extern char *optarg;
	extern int optind, opterr, optopt;
	char *bustypestr = NULL;
	int errcnt = 0;
	int bustype;
	char option;
	int slot = -1;
	struct cadata nvm;
	int fd;
	int bflag = 0;		/* bustype flag */
	int sflag = 0;		/* slot flag */


	/*
	 * Get option letter from argument vector.
	 */
	while ((option = getopt(argc, argv, "b:s:")) != EOF) {
		switch (option) {
		case 'b':
			if (bustypestr != (char *)NULL) {
				fprintf(stderr, "Multiple use of -b\n");
				errcnt++;
			}
			bustypestr = optarg;
			if (strcmp(bustypestr, "eisa") == 0)
				bustype = CM_BUS_EISA;
			else if (strcmp(bustypestr, "mca") == 0)
				bustype = CM_BUS_MCA;
			bflag++;
			break;
		case 's':
			if (slot != -1) {
				fprintf(stderr, "Multiple use of -s\n");
				errcnt++;
                        }
                        slot = atoi(optarg);
			sflag++;
			break;
		default:
			errcnt++;
			break;
		}
	}
	argc += optind;
	argv += optind;
	/*
	 * If bustype or slot number is not specified, then exit.
	 */
	if (errcnt || !(bflag) || !(sflag)) {
		fprintf(stderr, "Usage: ca [-b bustype] [-s slot number]\n");
		exit(errcnt);
	}

#ifdef DEBUG
	printf("bustype=0x%x, bustypestr=%s slot=%d\n", 
		bustype, bustypestr, slot);
#endif /* DEBUG */

	if ((fd = open("/dev/ca", O_RDONLY)) == -1) {
		perror("open failed");
		exit(1);
	}

	nvm.ca_busaccess = slot;
	nvm.ca_buffer = data;
	nvm.ca_size = 0;

	if (ioctl(fd, CA_EISA_READ_NVM, &nvm) != -1) {
		ca_eisa_parse_nvm(nvm.ca_busaccess, nvm.ca_buffer, nvm.ca_size);
	} else {
		perror("ioctl failed");
		exit(1);
	}
#ifdef DEBUG
	printf("ca_size=0x%x\n", nvm.ca_size);
#endif /* DEBUG */
	exit(0);
}

/* This section displays whatever is returned by the EISA CMOS Query. */

int
ca_eisa_parse_nvm(ulong_t slotnum, char *data, int length)
{
	int findex = 0;		/* function index number */
	NVM_SLOTINFO *slot = (NVM_SLOTINFO *)data;
	NVM_FUNCINFO *function;
#ifdef NOTYET
	eisa_funcinfo_t *ef;
#endif /* NOTYET */
	int c;

	printf("\nSlot %d :\n", slotnum);

	while (slot < (NVM_SLOTINFO *)(data + length)) {
		printf("\nBoard id : %c%c%c %x %x\n", 
			(slot->boardid[0] >> 2 &0x1f) + 64, 
			((slot->boardid[0] << 3 | 
				slot->boardid[1] >> 5) & 0x1f) + 64, 
			(slot->boardid[1] &0x1f) + 64, 
			slot->boardid[2], 
			slot->boardid[3]);
		printf("Revision : %x\n", slot->revision);
		printf("Number of functions : %x\n", slot->functions);
		printf("Function info : %x\n", *(unsigned char *) &slot->fib);
		printf("Checksum : %x\n", slot->checksum);
		printf("Duplicate id info : %x\n",
		       *(unsigned short *) &slot->dupid);

		function = (NVM_FUNCINFO *)(slot + 1);
#ifdef NOTYET
		ef = (eisa_funcinfo_t *)(slot + 1);
#endif /* NOTYET */

		while (function < (NVM_FUNCINFO *)slot + slot->functions) {

			printf("\n\tFunction number %d :\n", findex++);
#ifdef NOTYET 
			printf("Type String = %s\n", (char *)ef->type);
#endif /* NOTYET */

			if (*(unsigned char *)&function->fib) {

			    printf("\n\tBoard id : %c%c%c %x %x\n", 
				(function->boardid[0] >> 2 &0x1f) + 64, 
				((function->boardid[0] << 3 | 
				       function->boardid[1] >> 5) & 0x1f) + 64, 
				(function->boardid[1] & 0x1f) + 64, 
				function->boardid[2], 
				function->boardid[3]);
			    printf("\tDuplicate id info : %x\n",
				   *(unsigned short *) &function->dupid);

			    printf("\tFunction info : %x\n",
				   *(unsigned char *) &function->fib);
			    if (function->fib.type)
				printf("\tType;sub-type : %s\n", 
							function->type);

			    if (function->fib.data) {
				unsigned char *free_form_data = 
					function->u.freeform + 1;
				unsigned char *free_form_end = 
					free_form_data + *function->u.freeform;

				printf("\tFree Form Data :\n\t");
				for (; free_form_data < free_form_end; free_form_data++)
					printf("%u ", *free_form_data);
				printf("\n");

			    } else if (function->fib.memory) {
				NVM_MEMORY *memory = function->u.r.memory;

				printf("\tMemory info :\n");
				while (memory < function->u.r.memory + NVM_MAX_MEMORY) {
				    printf("\t\tMemory Section %d:\n", 
					memory - function->u.r.memory + 1);
				    printf("\t\t\tLogical Characteristics:\n");
				    if (memory->config.write)
					printf("\t\t\t\tRead/Write\n");
				    else
					printf("\t\t\t\tRead Only\n");

				    if (memory->config.cache)
					printf("\t\t\t\tCached\n");

				    printf("\t\t\t\tType is %s\n", memory_type[memory->config.type]);
				    if (memory->config.share)
					printf("\t\t\t\tShared\n");

				    printf("\t\t\tPhysical Characteristics:\n");
				    printf("\t\t\t\tData Path Width: %s\n", 
					width[memory->datapath.width]);
				    printf("\t\t\t\tData Path Decode: %s\n", 
					memory_decode[memory->datapath.decode]);
				    printf("\t\t\tBoundaries:\n");
				    printf("\t\t\t\tStart address: %lx\n", 
					*(long *)memory->start * 256);
				    printf("\t\t\t\tSize: %lx\n", 
					(long)(memory->size * 1024));
				    if (memory->config.more) 
					memory++;
				    else 
					break;

				} /* end while */
			    }

			    if (function->fib.irq) {
				NVM_IRQ *irq = function->u.r.irq;

				printf("\tIRQ info :\n");
				while (irq < function->u.r.irq + NVM_MAX_IRQ) {
				    printf("\t\tInterrupt Request Line: %d\n", irq->line);
#ifdef NOTYET 
			for (c = 0; c < EISA_MAX_IRQ; c++) {
				printf("IRQ line: %d\n", ef->eisa_irq[c].line);
				    if (ef->eisa_irq[c].trigger)
					printf("\t\tLevel-triggerred\n");
				    else
					printf("\t\tEdge-triggerred\n");

				    if (ef->eisa_irq[c].share)
					printf("\t\tShareable\n");
			}
#endif /* NOTYET */
				    if (irq->trigger)
					printf("\t\tLevel-triggerred\n");
				    else
					printf("\t\tEdge-triggerred\n");

				    if (irq->share)
					printf("\t\tShareable\n");

				    if (irq->more) 
					irq++;
				    else 
					break;
				}	
			    }

			    if (function->fib.dma) {
				NVM_DMA *dma = function->u.r.dma;

				printf("\tDMA info :\n");
				while (dma < function->u.r.dma + NVM_MAX_DMA) {
				    printf("\t\tDMA Device %d:\n", 
						dma - function->u.r.dma + 1);
				    printf("\t\t\tChannel Number: %d\n", 
						dma->channel);
				    printf("\t\t\tTransfer Size is %s\n", 
						width[dma->width]);
				    printf("\t\t\tTransfer Timing is %s\n", 
						dma_timing[dma->timing]);
				    if (dma->share)
					printf("\t\t\tShareable\n");

				    if (dma->more) 
					dma++;
				    else 
					break;
				}
			    }

			    if (function->fib.port) {
				NVM_PORT *port = function->u.r.port;

				printf("\tPort info :\n");
				while (port < function->u.r.port + NVM_MAX_PORT) {
				    printf("\t\tPort Address: %x\n", 
						port->address);
				    printf("\t\tSequential Ports: %d\n", 
						port->count);
				    if (port->share)
					printf("\t\tShareable\n");

				    if (port->more) 
					port++;
				    else 
					break;
				}
			    }

			    if (function->fib.init) {
				unsigned char *init = function->u.r.init;

				printf("\tInit info :\n\t");
				while (init < function->u.r.init + NVM_MAX_INIT)
				    printf("%u ", *init++);
				printf("\n");
			    }
			} /* end fib */

			function++;
#ifdef NOTYET
			ef++;
#endif /* NOTYET */
		}
		slot = (NVM_SLOTINFO *)function;
	}
}




