#ident	"@(#)elfmark:cmd/sgs/elfmark/i386/elfmark.c	1.2"

#include <sys/types.h>
#include <elf.h>
#include <elfid.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <pfmt.h>


static int
fixit(const char *pathname, uint_t target)
{
	int		fd;
	Elf32_Ehdr	ehdr;

	if ((fd = open(pathname, O_RDWR)) == -1)
	{
		pfmt(stderr, MM_ERROR, ":1641:cannot open %s: %s\n",
			pathname, strerror(errno));
		return 0;
	}
	if (read(fd, (char *)&ehdr, sizeof(Elf32_Ehdr)) !=
		sizeof(Elf32_Ehdr))
	{
		pfmt(stderr, MM_ERROR, 
			":1642:cannot read ELF header from %s\n",
				pathname);
		close(fd);
		return 0;
	}
	if ((strncmp((char *)ehdr.e_ident, ELFMAG, SELFMAG) != 0) ||
		(ehdr.e_ident[EI_CLASS] != ELFCLASS32) ||
		(ehdr.e_ident[EI_DATA] != ELFDATA2LSB) ||
		(ehdr.e_ident[EI_VERSION] != EV_CURRENT) ||
		(ehdr.e_machine != EM_386))
	{
		pfmt(stderr, MM_ERROR, 
			":1643:%s is not an ELF file or is of wrong type\n",
			pathname);
		close(fd);
		return 0;
	}
	ehdr.e_flags = target;
	if (lseek(fd, 0, SEEK_SET) == -1)
	{
		pfmt(stderr, MM_ERROR, ":1644:cannot seek in %s\n",
			pathname);
		close(fd);
		return 0;
	}
	if (write(fd, (char *)&ehdr, sizeof(Elf32_Ehdr)) !=
		sizeof(Elf32_Ehdr))
	{
		pfmt(stderr, MM_ERROR, 
			":1645:cannot write ELF header to %s\n",
			pathname);
		close(fd);
		return 0;
	}
	close(fd);
	return 1;
}

static void
usage()
{
	pfmt(stderr, MM_ACTION, 
		":1646:usage: elfmark -t osr5|udk|none file...\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	const char	*label = "UX:elfmark";
	int		retcode = 0;
	int		c;
	uint_t		target = 0;
	uchar_t		target_seen = 0;

	setlocale(LC_ALL, "");
	setcat("uxcds");
	setlabel(label);

	while((c = getopt(argc, argv, "t:")) != EOF)
	{
		switch(c)
		{
		case 't':
			target_seen = 1;
			if (strcmp(optarg, "osr5") == 0 ||
				strcmp(optarg, "osrvr") == 0 ||
				strcmp(optarg, "openserver") == 0 ||
				strcmp(optarg, "OSR5") == 0 ||
				strcmp(optarg, "OSRVR") == 0 ||
				strcmp(optarg, "OPENSERVER") == 0)
				target = OSR5_FIX_FLAG;
			else if (strcmp(optarg, "udk") == 0 ||
				strcmp(optarg, "UDK") == 0)
				target = UDK_FIX_FLAG;
			else if (strcmp(optarg, "none") == 0 ||
				strcmp(optarg, "NONE") == 0)
				target = 0;
			else
				usage();
			break;
		default:
			usage();
			break;
		}
	}

	if (!target_seen || (optind >= argc))
		usage();

	for(; optind < argc; optind++)
	{
		char	*pathname = argv[optind];
		if (fixit(pathname, target) == 0)
		{
			pfmt(stderr, MM_ERROR, 
				":1647:fixup of %s failed\n",
				pathname);
			retcode++;
		}
		else
			fprintf(stdout, "%s\n", pathname);
	}
	return(retcode);
}
