/*		copyright	"%c%" 	*/

#ident	"@(#)xcpxinstall:i386/xcpcmd/xinstall/fixshlib.c	1.1"
#ident  "$Header$"
/*
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#ifndef __STDC__
#define const	/*nothing*/
#define size_t	unsigned
#define UCHAR_MAX 255
#endif

static const char cantopen_str[] = "%s:cannot open ";
static const char error_str[] = "error";
static size_t nextoffset;	/* file offset for next read into buffer */
static char *progname;		/* invocation name */
static char *filename;		/* current file name */

main(argc,argv)int argc;char**argv;
{
	char name[20];
	int shlib_num, i, fd, libname_len;
	char buf[300], shdr[40];		/* size of COFF section header */
	struct shlib_name {
		unsigned int sz;
		unsigned int ofset;
	} shl_name;
	struct scnhdr {
		char		s_name[8];	/* section name */
		long		s_paddr;	/* physical address, aliased s_nlib */
		long		s_vaddr;	/* virtual address */
		long		s_size;		/* section size */
		long		s_scnptr;	/* file ptr to raw data for section */
		long		s_relptr;	/* file ptr to relocation */
		long		s_lnnoptr;	/* file ptr to line numbers */
		unsigned short	s_nreloc;	/* number of relocation entries */
		unsigned short	s_nlnno;	/* number of line number entries */
		long		s_flags;	/* flags */
	} shr;
	unsigned short us[10];	/* size of COFF file header */
	short magic;		/* UNIX-style magic number */


	progname = argv[0];
	filename = argv[1];

	if ((fd = open(filename, O_RDWR)) < 0)
	{
	 	fprintf(stderr, cantopen_str, progname);
		perror(filename);
	
		exit(2);
	}
	strcpy(name,".lib");
	
	/*
	* In COFF, the actual file header looks like an array of 10
	* unsigned shorts.  The second unsigned short is the number
	* of sections and the ninth is the size of the optional
	* header.  The first object in the optional header is the
	* UNIX-style magic number.  The section headers (each 40
	* bytes) begin after the optional header.  The first 8 bytes
	* of each header is its name.
	*
	* If given a particular section to look through, search for
	* an exact match.  Otherwise, look for the first section whose
	* name somewhere contains "data".  Once a particular section
	* header is found, the file offset is a four-byte value 20
	* bytes into the header.
	*/
	if (lseek(fd, 0L, 0) != 0)
	{
	seek_err:;
		perror(error_str);
		return;
	}
	if (read(fd, (char *)us, sizeof(us)) != sizeof(us))
	{
	read_err:;
		fprintf(stderr, "%s:read ", progname);
		perror(error_str);
		return;
	}
	if (us[1] < 1)	/* no section headers */
	{
	magic_err:;
		fprintf(stderr, "%s:unknown file type--", progname);
		perror("possibly bad magic");
		return;
	}
	if (us[8] != 0)	/* optional header exists */
	{
		if (us[8] < sizeof(short))
			goto magic_err;
		if (read(fd, (char *)&magic, sizeof(magic)) != sizeof(magic))
			goto read_err;
		switch (magic)
		{
		default:
			goto magic_err;
		case 0407:
		case 0410:
		case 0411:
		case 0413:
		case 0401:
		case 0405:
		case 0437:
			break;
		}
		if (lseek(fd, (long)(sizeof(us) + us[8]), 0)
			!= sizeof(us) + us[8])
		{
			goto seek_err;
		}
	}
	/*
	* Check out each section header.
	*/
	do
	{
		if (read(fd, &shr, sizeof(shr)) != sizeof(shr))
			goto read_err;

		shr.s_name[8] = '\0';	/* guarantee null-termination */
		if (name != 0)	/* looking for exact match */
		{
			if (strcmp(name, shr.s_name) == 0) {
			    shlib_num=shr.s_paddr;
			    printf("num=%d name=%s\n",shlib_num, shr.s_name);
			    goto match;
			}
		}
	} while (--us[1] != 0);
	fprintf(stderr, "%s:no %s section found\n", progname,
		name == 0 ? "matching" : name);
	return;
match:;
	memcpy((char *)&nextoffset, &shr.s_scnptr, sizeof(long));
	if (lseek(fd, nextoffset, 0) != nextoffset)
		goto seek_err;
	for (i =0; i<=shlib_num;i++){
		read(fd,&shl_name,sizeof(shl_name));
		libname_len = (shl_name.sz - shl_name.ofset ) * 4;
		read(fd,buf, libname_len);
		printf("%s size=%d\n", buf, libname_len);
		if (strcmp(buf, "/shlib/libnsl_s") != 0) continue;
		lseek(fd, -libname_len, SEEK_CUR);
		strcpy(buf,"/shlib/libNSL_s");
		write(fd,buf,libname_len);
		
	}
}
