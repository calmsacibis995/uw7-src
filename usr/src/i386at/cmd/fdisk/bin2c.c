#ident	"@(#)fdisk:i386at/cmd/fdisk/bin2c.c	1.2.1.1"
#ident	"$Header$"

#include <stdio.h>
#include <libelf.h>
#include <fcntl.h>

extern void elf_eoj();

#define BOOTSZ		446	/* size of boot code in master boot block */
unsigned	char	bootcod[BOOTSZ];

main(argc, argv)
int	argc;
char	*argv[];
{
	int	i;
	FILE	*fdout;
	Elf *elf;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	char *cmd;
	char *infile, *ofile;
	int fd;
	unsigned int size;
	int kind;
	unsigned short pnum;
	int	j;
	unsigned int	ch;

	cmd = argv[0];
	infile = argv[1];
	ofile = argv[2];

	if (argc != 3) {
		fprintf(stderr, "usage: %s infile outfile\n", cmd);
		exit (1);
	}

	if ((elf_version(EV_CURRENT)) == EV_NONE) {
		fprintf(stderr, "%s: ELF Access library out of date\n", cmd);
		exit (1);
	}

	if ((fd = open(infile, O_RDONLY)) == -1) {
		perror(infile);
		exit (1);
	}

	if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
		fprintf(stderr, "%s: Can't Elf Begin %s (%s)\n",
			cmd, infile, elf_errmsg(-1));
		elf_eoj(NULL, fd, 1);
	}

	if ((ehdr = elf32_getehdr(elf)) == NULL) {
		fprintf(stderr, "%s: (%s) Can't get Elf Header (%s)\n",
			cmd, infile, elf_errmsg(-1));
		elf_eoj(elf, fd, 1);
	}

	if (((kind = elf_kind(elf)) != ELF_K_ELF) &&
			(kind != ELF_K_COFF)) {
		fprintf(stderr, "%s: %s not a valid binary file\n",
			cmd, infile);
		elf_eoj(elf, fd, 1);
	}

	if ((phdr = elf32_getphdr(elf)) == NULL) {
		fprintf(stderr, "%s: Can get Program Header for %s (%s)\n",
			cmd, infile, elf_errmsg(-1));
		elf_eoj(elf, fd, 1);
	}

	for (pnum = 0; pnum < ehdr->e_phnum; ++pnum) {
		if ((phdr->p_type == PT_LOAD) &&
			(phdr->p_filesz != 0))
				break;
		++phdr;
	}

	if (pnum >= ehdr->e_phnum) {
		fprintf(stderr, "%s: unable to find program header for %s\n",
			cmd, infile);
		elf_eoj(elf, fd, 1);
	}

	size = phdr->p_filesz;
	if( size > BOOTSZ) {
		fprintf(stderr, 
			"%s: size of text code (%d) is greater than maximum size %d\n",
                        infile, size, BOOTSZ);
		elf_eoj(elf, fd, 1);
	}


	if ((lseek(fd, phdr->p_offset, 0L)) == -1L) {
		fprintf(stderr, "%s: seek error on %s\n",
			cmd, infile);
		elf_eoj(elf, fd, 1);
	}

	if ((read(fd, bootcod, size)) != size) {
		fprintf(stderr, "%s: read error on %s\n",
			cmd, infile);
		elf_eoj(elf, fd, 1);
	}

	if ((fdout = fopen(ofile, "w")) == NULL) {
		fprintf(stderr, "%s: Cannot open %s\n",
			cmd, ofile);
		elf_eoj(elf, fd, 1);
	}


	fprintf(fdout, "#ident	\"@(#)fdisk:i386at/cmd/fdisk/bin2c.c	1.2.1.1\"\n");
	fprintf(fdout, "/*\n");
	fprintf(fdout, " *	This file is generated from the assembly language\n");
	fprintf(fdout, " *	code in the file \"bootstrap.s\"\n");
	fprintf(fdout, " */\n\n\n");
	fprintf(fdout, "unsigned char Bootcod[] = {\n");
	i = 0;
	while(i < BOOTSZ){
		fprintf(fdout, "   ");
		for(j=0; j<10 && i < BOOTSZ; i++, j++){
			ch = (unsigned int) bootcod[i];
			fprintf(fdout, "0x%02x, ", ch);
		}
		fprintf(fdout, "\n");
	}
	fprintf(fdout, "};\n");
	fclose(fdout);
	elf_eoj(elf, fd, 0);
}

void
elf_eoj(e, fd, x)
Elf *e;
int fd;
int x;
{
	if (e)
		(void)elf_end(e);
	if (fd != -1)
		(void)close(fd);
	exit (x);
}
