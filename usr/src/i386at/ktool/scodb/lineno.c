#ident	"@(#)ktool:i386at/ktool/scodb/lineno.c	1.1"

/*
 * lineno - extract line number information for scodb.
 *
 * Usage:	lineno [-v | -V] <kernel binary> <usym file>
 *
 * This program extracts the line number information from the kernel
 * binary specified, and then writes it into the usym file specified.
 * The usym file is assumed to already have been created by the ? program
 * and to contain the structure/union definitions.  The line number definitions
 * are added to the end of the file, and the header information in the
 * usym file update to reflect the fact.
 *
 * The -v and -V options are for verbosity.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "libelf.h"
#include "dwarf.h"
#include <syms.h>
#include <sys/scodb/dbg.h>
#include <sys/scodb/stunv.h>
#include <sys/scodb/sent.h>

extern int errno;
char *binary_name = NULL;
char *usym_name = NULL;

struct scodb_usym_info usym_info;		/* header of usym file */
struct scodb_lineno *lineno_buf;	/* space for building scodb line
					   number table */
int lineno_count;			/* count of line numbers */

int verbose = 0;

int usym_fd, elf_fd, line_index = -1;
off_t usym_size;

Elf *elf_file;
Elf32_Ehdr *elf_ehdr;
Elf_Scn *elf_scn = NULL;
Elf32_Shdr *elf_shdr;
Elf_Data *elf_line_data;

static		unsigned char	*data_ptr;

static		unsigned long	get_long(), peek_long();
static		unsigned short	get_short(), peek_short();
static		unsigned char	get_byte();
static		char	*get_string();

main(argc, argv)
char **argv;
{
	int arg;

	parse_args(argc, argv);

	if (elf_version(EV_CURRENT) == EV_NONE) {
		fprintf(stderr, "ELF version error\n");
		exit(1);
	}

	/*
	 * Try to open the kernel binary
	 */

	if ((elf_fd = open(binary_name, O_RDONLY)) < 0) {
		fprintf(stderr, "Error %d opening kernel binary %s\n",
			errno, binary_name);
		exit(1);
	}

	/*
	 * Try to open the usym file.
	 */

	if ((usym_fd = open(usym_name, O_RDWR)) < 0) {
		fprintf(stderr, "Error %d opening usym file %s\n",
			errno, usym_name);
		exit(1);
	}

	read_usym_info();

	elf_file = elf_begin(elf_fd, ELF_C_READ, (Elf *)0);
	elf_ehdr = elf32_getehdr(elf_file);

	/*
	 * Look for the .line section
	 */

	line_index = find_section(elf_file, ".line");

	if (line_index < 0) {
		if (verbose & 2)
			fprintf(stderr, "Cannot find .line section\n");
		exit(1);
	}

	if ((elf_line_data = elf_getdata(elf_scn, NULL)) == NULL) {
		fprintf(stderr, "Cannot get .line data\n");
		exit(1);
	}

	get_line(elf_line_data);

	if (verbose)
		printf("%d line numbers processed\n", lineno_count);

	write_usym_info();
}

/*
 * Read in the header from the usym file
 */
 
read_usym_info()
{
	int n;

	n = sizeof(struct scodb_usym_info);
	if (read(usym_fd, &usym_info, n) != n) {
		perror("Cannot read usym header\n");
		exit(1);
	}

	if (usym_info.magic != USYM_MAGIC) {
		fprintf(stderr, "Bad magic number on usym file\n");
		exit(1);
	}

	if (usym_info.lineno_offset != 0 || usym_info.lineno_size != 0) {
		fprintf(stderr, "Usym file already includes line number information (offset %d, size %d)\n", usym_info.lineno_offset, usym_info.lineno_size);
		exit(1);
	}
		
	if ((usym_size = lseek(usym_fd, 0, SEEK_END)) == -1) {
		fprintf(stderr, "Cannot seek to end of usymfile\n");
		exit(1);
	}
}

/*
 * Write out the line number information to the end of the file,
 * and update the header to reflect the fact that the line numbers
 * are present
 */

write_usym_info()
{
	int n;

	/*
	 * First write out the updated header
	 */

	usym_info.lineno_offset = usym_size;
	usym_info.lineno_size = lineno_count * sizeof(struct scodb_lineno);

	if (lseek(usym_fd, 0, SEEK_SET) == -1) {	
		perror("Cannot seek to start of usymfile");
		exit(1);
	}

	if (write(usym_fd, &usym_info, sizeof(usym_info)) != sizeof(usym_info)) {
		perror("Cannot write header");
		exit(1);
	}

	/*
	 * Seek to the end of the file and write out the line numbers
	 */

	if (lseek(usym_fd, 0, SEEK_END) == -1) {	
		perror("Cannot seek to end of usymfile");
		exit(1);
	}
	
	n = sizeof(struct scodb_lineno) * lineno_count;
	if (write(usym_fd, lineno_buf, n) != n) {
		perror("Cannot write line number information\n");
		exit(1);
	}
}

/* 
 * Find the section named by the "name" argument.  Returns the index number
 * of the section.
 */

find_section(Elf *elf_file, char *name)
{
	int i;
	char *secname;

	i = 1;
	elf_scn = NULL;

	while ((elf_scn = elf_getscn(elf_file, i)) != NULL) {
		elf_shdr = elf32_getshdr(elf_scn);
		secname = elf_strptr(elf_file, elf_ehdr->e_shstrndx, elf_shdr->sh_name);
		if (!strcmp(secname, name))
			return(i);
		i++;
	}
	return(-1);
}

usage(char *name)
{
	fprintf(stderr, "Usage: %s [-v | -V] <kernel binary> <usymfile>\n", name);
	exit(1);
}

/*
 * Set up the pointer used by the get_byte(), peek_short() etc. routines
 * below.
 */

void
set_data_ptr(unsigned char *p)
{
	data_ptr = p;
}

/*
 * Get 1 byte of data from the offset specified.  Update the offset.
 */

unsigned char 
get_byte(unsigned *offset)
{
	char 	*ptr;

	ptr = (char *)((unsigned)data_ptr + *offset);
	*offset += 1;
	return *ptr;
}

/*
 * Get 2 bytes of data from the offset specified.  Update the offset.
 */

unsigned short
get_short(unsigned *offset)
{
	short x;
	unsigned char    *p = (unsigned char *)&x;
	char *ptr;

	ptr = (char *)((unsigned)data_ptr + *offset);

	*p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr;
        *offset += 2;
        return x;

}

/*
 * Get 2 bytes of data from the offset specified.  Do not update the offset.
 */

unsigned short
peek_short(unsigned offset)
{
	short x;
	unsigned char    *p = (unsigned char *)&x;
	char *ptr;

	ptr = (char *)((unsigned)data_ptr + offset);

	*p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr;
        return x;

}

/*
 * Get 4 bytes of data from the offset specified.  Update the offset.
 */

unsigned long
get_long(unsigned *offset)
{
	long 	x;
	unsigned char	*p = (unsigned char *)&x;
	char *ptr;

	ptr = (char *)((unsigned)data_ptr + *offset);

	*p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr;
        *offset += 4;
        return x;
}

/*
 * Get 4 bytes of data from the offset specified.  Do not update the offset.
 */

unsigned long
peek_long(unsigned offset)
{
	long 	x;
	unsigned char	*p = (unsigned char *)&x;
	char *ptr;

	ptr = (char *)((unsigned)data_ptr + offset);

	*p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr;
        return x;
}

/*
 * Get a string of data from the offset specified.  Update the offset.
 */

char *
get_string(unsigned *offset)
{
	char *ptr;

	ptr = (char *)((unsigned)data_ptr + *offset);
	*offset += strlen(ptr) + 1;
	return ptr;
}

/*
 * Process line number information.
 *
 * ELF line number information looks like this:
 *
 * length of line number information for 1st source file (4 bytes)
 * base address of 1st source file (4 bytes)
 *
 *	line number (4 bytes)
 *	statement number (2 bytes - currently 0xffff)
 *	address relative to base address of source file (4 bytes)
 *
 *	...remaining line numbers for 1st source file...
 *
 * length of line number information for 2nd source file (4 bytes)
 * base address of 2nd source file (4 bytes)
 *
 * ...line numbers for 2nd source file...
 */

get_line(Elf_Data *data)
{
	int size, length;
	unsigned line, old_pcval, pcval, base_address, delta;
	short stmt;
	unsigned offset = 0, end_offset;
	struct scodb_lineno *lineno_p;

	set_data_ptr(data->d_buf);
	size = data->d_size;
	pcval = 0;

	/*
	 * Allocate space for the scodb format line number table.  We know
	 * the total size of the ELF line number table, and we know that
	 * each ELF line number entry occupies 10 bytes (4 + 4 + 2).  We also
	 * know the size of each entry of the scodb line number table.  We can
	 * therefore calculate the amount of memory required for the scodb
	 * table based on these values.  Note that this will not be precise,
	 * but is guaranteed to be always a bit more than we require.  This
	 * is because the ELF table is split up into a number of sections,
	 * each of which has a header containing a length and a base
	 * address.  These additional headers are not present in the scodb
	 * table, which is just a simple array, and by their additional size
	 * will therefore ensure that the size we calculate for the scodb
	 * table is at least what is required.
	 */

	length = size * sizeof(struct scodb_lineno) / (4 + 4 + 2);

	lineno_buf = (struct scodb_lineno *) malloc(length);
	lineno_p = lineno_buf;
	lineno_count = 0;

	while (offset < size) {
		length = get_long(&offset);
		base_address = get_long(&offset);
		length -= (4 + 4);
		end_offset = offset + length;

		while (offset < end_offset) {
			line = get_long(&offset);
			stmt = get_short(&offset);
			delta = get_long(&offset);
			old_pcval = pcval;
			pcval = base_address + delta;

			if (old_pcval > pcval)
				fprintf(stderr, "Warning - line numbers not in ascending order\n");
	
			lineno_p->l_addr = pcval;
			lineno_p->l_lnno = line;

			lineno_p++;
			lineno_count++;

			if (verbose & 2)
				(void)printf("%-12ld\t0x%lx\n", line, pcval);
		}
	}
}

parse_args(int argc, char **argv)
{
	int c;

	if (argc < 3)
		usage(argv[0]);

	while ((c = getopt(argc, argv, "Vv")) != EOF) {
		switch (c) {
		case 'v':
			verbose |= 1;
			break;
		case 'V':
			verbose |= 2;
		}
	}

	if (argc - optind < 2)
		usage(argv[0]);

	binary_name = argv[optind];
	usym_name = argv[optind+1];
}
