#ident	"@(#)OSRcmds:i486/i486.c	1.1"

#define PSRINFO_STRINGS

#include <sys/prosrfs.h>
#include <libgen.h>

main(int argc, char **argv)
{
	processor_info_t	pinfo;
	char	buffer[20];
	int	entries;
	char	*name;
	int	cmdtyp;
	int	realtyp;

	_processor_info(0, &pinfo);
	strcpy(buffer, pinfo.pi_processor_type);

	entries=sizeof(pfs_chip_map)/sizeof(char *);

	name=basename(argv[0]);

	cmdtyp=realtyp=0;

	for (entries -=1; entries > 0; entries--) {
		if (strcmp(name, pfs_chip_map[entries]) == 0)
			cmdtyp = entries;
		if (strcmp(buffer, pfs_chip_map[entries]) == 0)
			realtyp = entries;
	}

	if (cmdtyp <= realtyp)
		return (0);
	else
		return (255);
}
