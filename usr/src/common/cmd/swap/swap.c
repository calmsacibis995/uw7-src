/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

#ident	"@(#)swap:swap.c	1.10.5.11"
#ident	"$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	Swap administrative interface.
 *	Used to add/delete/list swap devices. 
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/errno.h>
#include	<sys/param.h>
#include	<sys/swap.h>
#include	<sys/sysmacros.h>
#include	<sys/mkdev.h>
#include	<sys/stat.h>
#include	<sys/uadmin.h>
#include	<fcntl.h> 
#include	<unistd.h>
#include	<sys/file.h>
#include	<errno.h>

#define LFLAG 1
#define DFLAG 2
#define AFLAG 3
#define SFLAG 4
#define CFLAG 5

#define BUFSIZE 1024

char *prognamep;
void usage(void);
int swap_usage(void);
off_t find_size(char *);
int list(void);
int delete(char *, int);
int add(char *, int, int, boolean_t);

int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	register int c, flag = 0;
	register int ret;
	register off_t s_offset, length;
	char *pathname;

	prognamep = argv[0];
	if (argc < 2) {
		usage();
		exit(1);
	}

	while ((c = getopt(argc, argv, "clsd:a:")) != EOF) {
		char *char_p;
		switch (c) {
		case 'l': 	/* list all the swap devices */
			if (argc != 2 || flag ) {
				usage();
				exit(1);
			}
			flag |= LFLAG;
			ret = list();
			break;
		case 's':
			if (argc != 2 || flag) {
				usage();
				exit(1);
			}
			flag |= SFLAG;
			ret = swap_usage();
			break;
		case 'd':
			if (argc < 3 || argc > 4 || flag ) {
				usage();
				exit(1);
			}
			flag |= DFLAG;
			pathname = optarg;
			if (argc > 3) {
				if ((s_offset =
				      strtol(argv[optind], &char_p, 10)) < 0 
				    || *char_p != '\0' )
					exit(1);
			} else
				s_offset = 0;
			ret = delete(pathname, s_offset);
			break;

		case 'c':	/* configure all swap devices */
			if (argc < 2 || argc > 3 || flag) {
				usage();
				exit(1);
			}
			flag |= CFLAG;
			if (argc == 2)
				ret = swap_conf("/etc/swaptab");
			else
				ret = swap_conf(argv[2]);
			break;

		case 'a':
			if (argc < 3 || argc > 5 || flag) {
				usage();
				exit(1);
			}
			flag |= AFLAG;
			pathname = optarg;
			if (argc > 4) {
				if ((s_offset =
				      strtol(argv[optind++], &char_p, 10)) < 0 
				    || *char_p != '\0' )
					exit(1);
			} else
				s_offset = 0;
			if (argc > 3) {
				if ((length =
				      strtol(argv[optind], &char_p, 10)) < 1 
				    || *char_p != '\0' )
					exit(1);
			} else
				length = find_size(pathname);
			ret = add(pathname, s_offset, length, B_FALSE);
			break;
		case '?':
			usage();
			exit(1);
		}
	}
	if (!flag) {
		usage();
		exit(1);
	}
	return ret;
}


void
usage(void)
{
	fprintf(stderr, "Usage:\t%s -l\n", prognamep);
	fprintf(stderr, "\t%s -s\n", prognamep);
	fprintf(stderr, "\t%s -c [ <file name> ]\n", prognamep);
	fprintf(stderr, "\t%s -d <file name> [ <low block> ]\n", prognamep);
	fprintf(stderr,
		"\t%s -a <file name> [ [ <low block> ] <nbr of blocks> ]\n",
		prognamep);
}

int
swap_conf(char *fname)
{
	FILE	*fp;
	char	pathname[BUFSIZE];
	char	snum[BUFSIZE];
	char	buf[BUFSIZE];
	ulong_t swaplo;
	size_t swapsz;
	int	err;

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "open of file %s failed\n", fname);
		return(2);
	}

	while (fgets(buf, BUFSIZE, fp)) {
		if (*buf == '#')
			continue;
		err = sscanf(buf, "%s %u %s", pathname, &swaplo, snum);
		if (err < 3) {
			fprintf(stderr, "%s in incorrect format\n", fname);
			return 2;
		}
		if (snum[0] == '-')
			swapsz = find_size(pathname);
		else
			swapsz = strtoul(snum, NULL, 10);

		if (swapsz)
			add(pathname, swaplo, swapsz, B_TRUE);
	}
	fclose(fp);
	return 0;
}

int
swap_usage(void)
{
	uint_t pagesize = sysconf(_SC_PAGESIZE);
	uint_t ratio;
	swapusage_t usage;

	ratio = pagesize / UBSIZE;

	/*
	 * obtain allocation/reservation statistics information from
	 * swapctl(SC_GETUSAGE, ...)
	 */
	if (swapctl(SC_GETUSAGE, &usage) == -1) {
		fprintf(stderr,"SC_GETUSAGE failed\n");
		perror(prognamep);
		return(2);
	}

        (void) printf(
           "\ntotal: %lu allocated + %lu reserved = %lu blocks used, %lu blocks available\n",
	   usage.stu_allocated * ratio,
	   (usage.stu_used - usage.stu_allocated) * ratio,
	   usage.stu_used * ratio,
	   (usage.stu_max - usage.stu_used) * ratio);
	
	return (0);
}

int
list(void)
{
	register struct swaptable 	*st;
	register struct swapent	*swapent;
	register int	i;
	struct stat statbuf;
	char		*path;
	int		length, num, error=0;
	ushort 		type = 0;

retry:
	if ((num = swapctl(SC_GETNSWP, NULL)) == -1) {
		fprintf(stderr,"SC_GETNSWP failed\n");
		perror(prognamep);
		return(2);
	}
	if (num == 0) {
		fprintf(stderr,"No swap device configured:swap(SC_LIST)\n");
		return(1);
	}

	if ((st = (swaptbl_t *) malloc(num * sizeof(swapent_t) + sizeof(int))) == NULL) {
		fprintf(stderr,"Malloc fails:swap(SC_LIST) aborted! Please try later.\n");
		perror(prognamep);
		return(2);
	}
	if ((path = (char *) malloc(num * MAXPATHLEN)) == NULL) {
		fprintf(stderr,"Malloc fails:swap(SC_LIST) aborted! Please try later.\n");
		perror(prognamep);
		return(2);
	}
	swapent = st->swt_ent;
	for (i = 0 ; i < num ; i++, swapent++) {
		swapent->ste_path = path;
		path += MAXPATHLEN;
	}

	st->swt_n = num;
	if ((num = swapctl(SC_LIST, st)) == -1) {
		fprintf(stderr,"SC_LIST failed\n");
		perror (prognamep);
		return(2);
	}

        printf("%-35s dev  swaplo blocks   free\n", "path");

	swapent = st->swt_ent;
	for (i = 0;  i < num  ;  i++, swapent++) {
	    strlen (swapent->ste_path) < 35 ? 
	         printf("%-35s", swapent->ste_path) : printf("%s", swapent->ste_path);
		if (stat(swapent->ste_path, &statbuf) < 0)
			printf(" ?,? ");
		else {
			type = (statbuf.st_mode & (S_IFBLK | S_IFCHR));
			printf("%2d,%-2d",
				type ? major(statbuf.st_rdev) : major(statbuf.st_dev),
				type ? minor(statbuf.st_rdev) : minor(statbuf.st_dev));
		}
		printf(" %6d %6d %6d", 
			swapent->ste_start,
			swapent->ste_pages << DPPSHFT,
			swapent->ste_free << DPPSHFT);
		if(swapent->ste_flags & ST_INDEL)
			printf(" INDEL\n");
		else
			printf("\n");
	}
	return 0;

}

int
delete(char *path, int offset)
{
	register swapres_t	*si;
	swapres_t		swpi;

	si = &swpi;
	si->sr_name = path;
	si->sr_start = offset;

	if (swapctl(SC_REMOVE, si) < 0){
		fprintf(stderr,"SC_REMOVE failed\n");
		perror (prognamep);
		return(2);
	}
	return 0;
}

int
add(char *path, int offset, int cnt, boolean_t is_conf)
{
	register swapres_t	*si;
	swapres_t		swpi;

	si = &swpi;
	si->sr_name = path;
	si->sr_start = offset;
	si->sr_length = cnt;

	if (swapctl(SC_ADD, si) < 0){
		if (is_conf && errno == EEXIST)
			return 0;
		fprintf(stderr,"SC_ADD failed\n");
		perror (prognamep);
		return(2);
	}
	return 0;
}


off_t
find_size(char *path)
{
	int		fd;
	struct stat	statb;

	if ((fd = open(path, O_RDONLY)) < 0) {
		perror(path);
		exit(1);
	}
	if (fstat(fd, &statb) < 0) {
		perror(path);
		exit(1);
	}
	close(fd);
	if (statb.st_size == 0) {
		fprintf(stderr, "%s: Can't determine size of device '%s';\n",
				prognamep, path);
		fprintf(stderr, "\tspecify size explicitly.\n");
		exit(1);
	}
	return statb.st_size / 512;
}
