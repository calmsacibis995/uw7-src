#ident	"@(#)autodetect:autodetect.c	1.10.1.3"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 * L000		28 April 97	georgep@sco.com
 * - added support for detecting the Data General Audobon machine,
 *   detection code courtesy of hamilton@dg-rtp.dg.com.
 * - the detection code has been glued in as a free standing function
 *   at is not integrated into autodetect scheme, this should be revisited
 *   when an Audobon is available to to test on, for this delta, the
 *   changes have been integrated "blind".
 */

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/corollary.h>
#include <sys/cbus.h>

#define M_FLG_SRGE		1       /* sig scattered in a range of memory */
#define MAX_SIGNATURE_LEN	10
#define MAX_DESCRIPTION_LEN	40
#define EBDA_PTR        0x40e   /* pointer to base of EBDA segment      */
#define BMEM_PTR        0x413   /* pointer to installed base memory in kbyte */

struct machconfig {
        char    *sigaddr;       /* machine signature location   */
        size_t   range;         /* signature search range       */
        unsigned char   sigid[MAX_SIGNATURE_LEN]; /* signature to match */
        unsigned char   m_flag;      /* search style flag */
        unsigned char   name[MAX_DESCRIPTION_LEN]; /* desc. of the machine */
	int	rval;		/* return value */
};

#define NULL	0

struct machconfig mconf[] = {
	(char *)0xf0000, 0xffff, "_MP_", M_FLG_SRGE, "PC+MP", 7,
	(char *)0xfffe0000, 0xffff, "Corollary", M_FLG_SRGE, "Cbus", 0,
        (char *)0xfffe4, 0x100, "COMPAQ", M_FLG_SRGE, "Compaq", 1,
	(char *)0xf0000, 0xffff, "EBI2", M_FLG_SRGE, "AST Manhattan", 2,
	(char *)0xfe000, 0x100, "ACER", 0, "Acer Frame3000MP", 3,
	(char *)0xf4000, 0xffff, "ACER", 0, "Acer Altos", 3,
	(char *)0xf0000, 0xffff, "Tricord", M_FLG_SRGE, "Tricord MP", 4,
	(char *)0xfe076,0x200, "Advanced L", M_FLG_SRGE, "ALR machine", 5,
        (char *)0xfffe0000, 0x100, "OLIVETTI", M_FLG_SRGE, "Olivetti", 8,
        (char *)0x0, 0x0, "DG", 0, "NUMALiiNE", 10	/* L000 */
};
#define NSIGS   (sizeof(mconf)/sizeof(struct machconfig))

extern int errno;

void	*bufp;
int	fdmem, fdpmem;
#ifdef DEBUG
int	wfd;
#endif

char *
membrk(char *s1, char *s2, int n1, int n2)
{
        char    *os = s1;
        int     n;

        for (n = n1 - n2 ; n >= 0; n--) {
                if (memcmp(s1++, s2, n2) == 0) {
                        return(s1);
                }
        }
        return(0);
}

void
cleanup()
{
	if (fdpmem != -1)
		close(fdpmem);

	if (fdmem != -1)
		close(fdmem);

#ifdef DEBUG
	if (wfd != -1)
		close(wfd);
#endif

	if (bufp != NULL)
		free(bufp);

	return;

}

main(argc, argv)
int argc;
char *argv[]; 
{
	int i;
	int pcmp_ret = 0;

	bufp = NULL;

	if ((fdpmem = open("/dev/pmem", O_RDONLY)) == -1)
		printf("auto_detect:/dev/pmem open fails! errno=%d\n", errno);

	if ((fdmem = open("/dev/mem", O_RDONLY)) == -1)
		printf("auto_detect:/dev/mem open fails! errno=%d\n", errno);

	if (fdmem == -1 && fdpmem == -1)
		printf("auto_detect:Cannot open /dev/[p]mem. Operation aborted");

#ifdef DEBUG
	if ((wfd = open("./outfile", O_RDWR|O_CREAT)) == -1)
		printf("auto_detect:./outfile open fails! errno=%d\n", errno);
#endif

	/* L000
         * Search for Audobon first.  This is a complete hack and
	 * will be tidied up when an Audobon is available to test on.
	 * For now the DG supplied code has been integrated in "as-is"
         * to reduce the risk of introducing bugs.
	 */
	if (on_audubon(fdmem) == 0) return(10);

	if ((pcmp_ret = look_pcmp()) != -1)
		return(pcmp_ret);

        for (i = 0; i < NSIGS; i++) {

		if ((bufp = malloc(mconf[i].range)) == NULL) {
			printf("auto_detect:malloc for i=%d fails.\n", i);
			cleanup();
			return(-1);
		}

		if (lseek(fdpmem, mconf[i].sigaddr, 0) != -1) {
			if (read(fdpmem, bufp, mconf[i].range) != -1)
				goto search;
		}

		if (lseek(fdmem, mconf[i].sigaddr, 0) == -1) {
			printf("auto_detect:lseek fails for i=%d.\n", i);
			cleanup();
			return(-1);
		} else {
			if (read(fdmem, bufp, mconf[i].range) == -1) {
				printf("auto_detect:read fails for i=%d.\n", i);
				cleanup();
				return(-1);
			}
		}

search:

#ifdef DEBUG
		write(wfd, bufp, mconf[i].range);
		write(wfd, "\n\n", 2);
#endif

		if (mconf[i].m_flag & M_FLG_SRGE) {

			if ((membrk((void *)bufp, (void *)mconf[i].sigid,
			     mconf[i].range, strlen(mconf[i].sigid))) != 0)  {	/* found the footprint */

				if (strcmp("Corollary", mconf[i].sigid) == 0) {
#ifdef DEBUG
					printf("auto_detect:Find %s\n", 
						mconf[i].name);
#endif
					iscorollary(fdpmem, fdmem, 
							argc, argv, i);
					/* NOTREACHED */

				} else {	/* !Corollary */
					if (strcmp("OLIVETTI",
					   (char *)&(mconf[i].sigid[0])) == 0) {

						uchar_t mach_type, *add = 0xffffd;

						lseek(fdmem, add, SEEK_SET);
						read(fdmem, &mach_type, 1);
						if (mach_type != 0x71) {
							printf("\nOlivetti PSM for ");
							printf("line 50xx only ");
							printf("supports 5050 model ");
							printf("(Pentium, APIC).\n\n");
							exit(-1);
						} else {
#ifdef DEBUG
							printf("auto_detect:Find %s\n", mconf[i].name);
#endif
							cleanup();
							return(mconf[i].rval);
						}

					} else {	/* !OLIVETTI */
						if (strcmp("COMPAQ", mconf[i].sigid) != 0) {
#ifdef DEBUG
							printf("auto_detect:Find %s\n", mconf[i].name);
#endif
							cleanup();
							return(mconf[i].rval);
						} else {	/* !COMPAQ */
							if (*(char *)bufp == 'E') {
#ifdef DEBUG
								printf("auto_detect:Find %s MP\n", mconf[i].name);
#endif
								cleanup();
								return(mconf[i].rval);
							} else {
#ifdef DEBUG
								printf("auto_detect:Find %s UP\n", mconf[i].name);
#endif
								break;
							}
						}
					}	/* !OLIVETTI */
				}	/* !Corollary */
                        }	/* found the footprint */
                } else {	/* !M_FLG_SRGE */
                        if (memcmp((void *)bufp, (void *)mconf[i].sigid,
                            strlen(mconf[i].sigid)) == 0) {

				if (strcmp("COMPAQ", mconf[i].sigid) != 0) {
#ifdef DEBUG
					printf("auto_detect:Find %s\n", mconf[i].name);
#endif
					cleanup();
					return(mconf[i].rval);
				} else {
					if (*(char *)bufp == 'E') {
#ifdef DEBUG
						printf("auto_detect:Find %s MP\n", mconf[i].name);
#endif
						cleanup();
						return(mconf[i].rval);
					 } else {
#ifdef DEBUG
						printf("auto_detect:Find %s UP\n", mconf[i].name);
#endif
						break;
					}
				}
                        }	/* found the footprint */
                }	/* !M_FLG_SRGE */

		free(bufp);
		bufp = NULL;
        }

	cleanup();
	return(-1);
}

unsigned print_debug;

unsigned char *
get_struct(fd1, fd2, location, size)
int fd1, fd2;
unsigned location;
unsigned size;
{
	int		good_fd;
	unsigned char	*mem;

	if (lseek(fd1, location, 0) != -1) {
		good_fd = fd1;
	} else if (lseek(fd2, location, 0) != -1) {
		good_fd = fd2;
	} else {
		printf("auto_detect:lseek fails for seek=0x%x.\n", location);
		cleanup();
		exit(-1);
	}

	mem = (unsigned char *)malloc(size);
	
	if (mem == NULL) {
		printf("auto_detect:malloc for size=%d fails.\n", size);
		cleanup();
		exit(-1);
	}

	if (read(good_fd, mem, size) != size) {
		printf("auto_detect: read failed\n", size);
		cleanup();
		exit(-1);
	}

	return mem;
}

free_struct(ptr)
unsigned *ptr;
{
	free(ptr);
}

iscorollary(fd1, fd2, argc, argv, indx)
int fd1, fd2;
int argc;
char *argv[];
int indx;
{
	unsigned char		*rrd_ram;
	unsigned		corollary_string = 0xdeadbeef;
	struct configuration	*config_ptr;
	int			type;

	rrd_ram = get_struct(fd1, fd2, RRD_RAM, 0x8000);

	if (argc == 2)
		if (strcmp(argv[1], "-d") == 0)
			print_debug = 1;

	/* 
	 * search the ram for the info check word 
	 */
	config_ptr = (struct configuration *)
		membrk(rrd_ram, &corollary_string, 0x8000, sizeof(int));

	if (config_ptr == 0) {
		printf("autodetect: can't find identifier\n");
		free_struct(rrd_ram);
		cleanup();
	}

	config_ptr = (struct configuration *)((char *)config_ptr - 1);

	type = corollary_find_processors(config_ptr, indx);

	free_struct(rrd_ram);
	cleanup();
	exit(type);
}

int
corollary_find_processors(cptr, indx)
struct configuration	*cptr;
int indx;
{
	struct ext_cfg_header		*ptr_header;
	struct ext_memory_board		*mem_ptr = NULL;
	struct oem_rom_information	*oem_ptr = NULL;
	struct ext_cfg_override		*cfg_ptr = NULL;
	char				*ptr_source;
	int				type;

	ptr_source = (char *)cptr;

	ptr_source += sizeof(configuration);
	ptr_header = (struct ext_cfg_header *)ptr_source;

	if (*(unsigned *)ptr_source == EXT_CHECKWORD) {

		do {
			ptr_source += sizeof(struct ext_cfg_header);

			switch (ptr_header->ext_cfg_checkword) {
			case EXT_MEM_BOARD:
#ifdef DEBUG
				printf("found EXT_MEM_BOARD\n");
#endif
				mem_ptr = (struct ext_memory_board *)ptr_source;
				break;
			case EXT_CHECKWORD:
				break;
			case EXT_VENDOR_INFO:
				oem_ptr = (struct oem_rom_information *)ptr_source; 
				break;
			case EXT_CFG_OVERRIDE:
#ifdef DEBUG
				printf("found EXT_CFG_OVERRIDE\n");
#endif
				cfg_ptr = (struct ext_cfg_override *)ptr_source;
				break;
			case EXT_ID_INFO:
#ifdef DEBUG
				printf("found EXT_ID_INFO\n");
#endif
				type = corollary_read_ext_ids(
					(struct ext_id_info *)ptr_source);

				if (oem_ptr->oem_number == CBUS_OEM_IBM_MCA)
					type = 103;
				else if (oem_ptr->oem_number == CBUS_OEM_OLIVETTI_PCI)
					type = 104;
				return type;
			case EXT_CFG_END:
				break;
			default:
				break;
			}
			
			ptr_source += ptr_header->ext_cfg_length;
			ptr_header = (struct ext_cfg_header *)ptr_source;

		} while (ptr_header->ext_cfg_checkword != EXT_CFG_END);
	}

	/*
	 * This is a pre-XM ROM.
	 */

	/* return the mconf table index value. */
	return mconf[indx].rval;
}

char *cpu_type[] = {
	"None",
	"386",
	"486",
	"Pentium"
};

unsigned cpu_type_num = sizeof(cpu_type) / sizeof(char *);


char *io_type[] = {
	"No I/O ",
	"SIO    ",
	"SCSI   ",
	"SYM    ",
	"Bridge ",
	"Bridge ",
	"----   ",
	"----   ",
	"----   ",
	"Memory "
};

unsigned io_type_num = sizeof(io_type) / sizeof(char *);

#define IS_SYMMETRIC	(ELEMENT_HAS_CBC | ELEMENT_HAS_APIC)

/*
 * read in the extended id information table.  filled in
 * by some of our C-bus licensees, and _all_ of the C-bus2
 * machines.
 */
int
corollary_read_ext_ids(p, indx)
struct ext_id_info *p;
int indx;
{
	register int i;
	char *s;
	unsigned all_sym = 1;
	unsigned any_sym = 0;

	if (print_debug) {
		printf("\nid\ttype\tattr\tfunct\tattr     start    ");
		printf("size     feature\n");
		printf(  "--\t----\t----\t-----\t----     -----    ");
		printf("----     -------\n");
	}

	for (i = 0; i < SMP_MAX_IDS && p->id != 0x7f; i++, p++) {

		if (i == 0)
			continue;

		if ((p->pm == 0) && (p->io_function == IOF_INVALID_ENTRY))
			continue;

		if (print_debug) {
			printf("0x%x\t", p->id);

			if (p->proc_type < cpu_type_num)
				s = cpu_type[p->proc_type];
			else
				s = "UNKNOWN";

			printf("%s\t", s);

			if (p->proc_attr & PA_CACHE_ON) {
				printf("+cache\t");
			} else {
				if (p->id)
					printf("-cache\t");
				else
					printf("      \t");
			}

			if (p->io_function < io_type_num)
				s = io_type[p->io_function];
			else
				s = "UNKNOWN";

			printf("%s\t", s);

			printf("%08x ", p->io_attr);

			printf("%08x ", p->pel_start);

			printf("%08x ", p->pel_size);

			if (p->pel_features & IS_SYMMETRIC)
				printf("SYMMETRIC");
			else
				printf("         ");

			printf("\n");
		}

		if (p->id) {
			if ((p->pel_features & IS_SYMMETRIC) == 0)
				all_sym = 0;
			else
				any_sym = 1;
		}
	}

	if (print_debug)
		printf("\nMachine type: ");

	if (all_sym) {
		if (print_debug)
			printf("ALL SYMMETRIC\n");
		return 101;
	}
	
	if (any_sym) {
		if (print_debug)
			printf("MIXED\n");
		return 102;
	}
		
	if (print_debug)
		printf("CBUS OR CBUS XM\n");

	/* return the mconf table index value */
	return mconf[indx].rval;
}
int look_pcmp()
{
	vaddr_t addr;
	char sigaddr[2];
	int ebda_base = 0, bmem_base = 0;
	int ret_code = 0, search = 0;

	/*
         * PC+MP floating pointer signature "_MP_" search order:
	 *	1. First 1k bytes of EBDA (Extended BIOS data Area)	
	 *	   EBDA pointer is defined at 2-byte location (40:0Eh)
	 *	   Standard EBDA segment located at 639K	
	 *	   i.e., (40:0Eh) = C0h; (40:0Fh) = 9F;	
	 *	2. Last 1k bytes of the base memory if EBDA undefined	
	 *	   i.e., 639k-640k for system with 640k base memory
	 *	3. ROM space 0F0000h-0FFFFFFh if nothing found in RAM
	 */
	addr = EBDA_PTR;
	while (search < 2){
		if (lseek(fdpmem, addr, 0) != -1) {
			if ((ret_code = read(fdpmem, sigaddr, 2)) == -1)
				printf("auto_detect:read fails for pcmp:sigaddr.\n");
		}
		else if ((ret_code = lseek(fdmem, addr, 0)) == -1)
			printf("auto_detect:lseek fails for pcmp:addr.\n");
	 	else {
			if ((ret_code = read(fdmem, sigaddr, 2)) == -1)
				printf("auto_detect:read fails for pcmp:sigaddr.\n");
		}
		if (ret_code == -1){
			cleanup();
			return(-1);
		}
		if(search++ == 0){
			ebda_base = (int) ((*(unsigned short *)sigaddr) << 4);
			if (ebda_base > 639*1024 || ebda_base < 510*1024)
				ebda_base = 0;		/* EBDA undefined */
			addr = BMEM_PTR;
		}
		else{
			bmem_base = (int) (((*(unsigned short *)sigaddr)-1) 
						* 1024);
			if (bmem_base > 639*1024 || bmem_base < 510*1024)
				bmem_base = 0;	/* Base memory undefined */
		}
	}

	if (ebda_base && pcmp_find_mp_fptr(ebda_base, ebda_base+1023))
		return(6);
	else if (bmem_base && pcmp_find_mp_fptr(bmem_base, bmem_base+1023))
		return(7);
	else
		return(-1);

}
/*
 * int 
 * pcmp_find_mp_fptr(uint_t begin, uint_t end)
 *	This routine search PCMP floating pointer structure
 *	from physical address <begine> to <end>.
 *
 * Calling/Exit State:
 *	None.
 */
int
pcmp_find_mp_fptr(uint_t begin, uint_t end)
{
	uint_t	*vbegin;
	struct pcmp_fptr {
		char sig[4];
		int paddr;
		char len, rev;
		char checksum;
		char mp_feature_byte[5];
	} *fp, *endp;
	size_t sz;
	int ret_code = 0;
		
	sz = end - begin + 1;
	if ((vbegin = malloc(sz)) == NULL) {
		printf("auto_detect:malloc for pcmp:vbegin fails.\n");
		cleanup();
		return(0);
	}
	if (lseek(fdpmem, begin, 0) != -1) {
		if ((ret_code = read(fdpmem, vbegin, sz)) == -1)
			printf("auto_detect:read fails for pcmp:vbegin.\n");
	}
	else if ((ret_code = lseek(fdmem, begin, 0)) == -1)
		printf("auto_detect:lseek fails for pcmp:begin.\n");
	 else {
		if ((ret_code = read(fdmem, vbegin, sz)) == -1)
			printf("auto_detect:read fails for pcmp:vbegin.\n");
	}
	if (ret_code == -1){
		free(vbegin);
		cleanup();
		return(0);
	}
	fp = (struct pcmp_fptr *)vbegin;
	endp = (struct pcmp_fptr *)(vbegin + sz - sizeof(*fp));

	for (fp; fp <= (struct pcmp_fptr *)endp; fp++) {
		if (fp->sig[0] == '_' && fp->sig[1] == 'M' &&
		    fp->sig[2] == 'P' && fp->sig[3] == '_'){
			free(vbegin);
			return(1);
		}
	}
	free(vbegin);
	return(0);
}

/* L000 
 *
 * Function that detects whether we are runniing on an Audubon VCS.
 * Return:
 *
 *	less than zero:		not an Audubon
 *	equal zero;		an Audubon
 *	greater than zero	could not tell
 *
 * The only argument is a file descriptor that is expected to
 * allow read access to /dev/mem.
 *
 * The general strategy is to locate the MPSpec floating pointer
 * structure, and use the floating pointer to locate the MP tables.
 * If the MP table header contains the Audubon signature, indication that
 * the platform is an Audubon node, and an extended MP entry of
 * of type  192 (the MC configuration table pointer) exists, then the
 * platform is an Audubon VCS.
 */

#define NO -1
#define YES 0
#define UNKNOWN 1

#define MBSIZE (64*1024)
static unsigned char mb[MBSIZE];
static unsigned entrylen();

static int memfd;
static int cumulative_status = NO;

#define K 1024

/*
 * MP Spec floating pointer and MPtable header definitions
 */
struct fphdr {
	char		sig[4];
	unsigned	mpptr;
	unsigned char	len;
	unsigned char	rev;
	unsigned char	sum;
	unsigned char	features[5];
};

struct mphdr {
	char		sig[4];
	short		baselen;
	unsigned char	rev;
	unsigned char	sum;
	char		oem[8];
	char		product[12];
	unsigned	oemtab;
	short		oemtabsiz;
	short		nentry;
	unsigned	lapicaddr;
	short		etl;
	unsigned char	etlsum;
	char		reserved;
};

static struct fphdr * search();
static void physread();

static int on_audubon(fd)
 unsigned fd; {
	struct fphdr *fp;
	struct mphdr *hp;
	unsigned mpp, etl, i;

	memfd = fd;
	if( (fp = search(639*K,K)) || (fp = search(15*64*K,64*K)) ) {
		mpp = fp->mpptr;
		physread( mpp, sizeof(struct mphdr) );
		hp = (struct mphdr *) mb;
		if( strncmp("AUDUBON",&hp->product, 7) )
			return cumulative_status;
		etl = hp->etl;
		if( ! etl )
			return cumulative_status;
		physread( mpp + hp->baselen, etl );
		for( i = 0; i < etl; i += entrylen(mb[i]) ) {
			if( mb[i] == 192 )
				return YES;
		}
	}
	return cumulative_status;
}


/*
 * Search a memory range for the MP Spec floating pointer
 */
static struct fphdr * search( base, len )
 unsigned base, len; {
	unsigned char *cp, sum;
	unsigned i;
	struct fphdr *fp;

	physread( base, len );
	for( cp = mb; cp < mb+MBSIZE; cp += sizeof(*fp) ) {
		fp = (struct fphdr *) cp;
		if( strncmp("_MP_",fp->sig, 4) )
			continue;

		if( fp->rev != 4 )
			continue;
		if( fp->len != 1 )
			continue;

		sum = 0;
		for( i = 0; i < sizeof(*fp); i++ )
			sum += cp[i];
		if( sum )
			continue;
		return fp;
	}
	return (struct fphdr *) 0;
}



/*
 * Read a range of physical memory.
 * To avoid having to mess with allocation and deallocation,
 * uses the same buffer for all reads, so reading a structure from
 * memory has the side effect of invalidating pointers into the last
 * structure read.
 */
static void physread( base, len )
 unsigned base, len; {

	if( lseek(memfd, base, 0) == -1 ) {
		cumulative_status = UNKNOWN;
		return;
	}

	if( read(memfd,mb,len) != len ) {
		cumulative_status = UNKNOWN;
		return;
	}
}



/*
 * Length of an extended MP table entry of given type
 */
static unsigned entrylen(c)
 unsigned char c; {
	
	switch( c ) {
	 case 128:
		return 20;

	 case 129:
		return 8;

	 case 130:
		return 8;

	 case 192:
		return 16;

	 default:
		return 0xffffffff;
	}
}
