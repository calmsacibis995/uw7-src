#ident	"@(#)unix_conv:common/convert.c	2.17.2.3"

#include <stdio.h>
#include <signal.h>

#include "old.a.out.h"
#include "old.ar.h"
#include "5.0.ar.h"

#include <ar.h>
#include <aouthdr.h>
#include <filehdr.h>
#include <scnhdr.h>
#include <linenum.h>
#include <reloc.h>
#include <syms.h>

#include <sys/types.h>
#include <sys/stat.h>
#include "paths.h"

#define	EVEN(x)	(((x) & 01) ? ((x) + 1) : (x))

void abort();        /* interrupt handler, defined below */
char *tempnam();/* C library routine for creating temp file name */
long time(); 	/* C library routine which gives you the time of day */
extern long sgetl();
extern int sputl();

long genarc(), xgenarc();	/* routines for converting archives 
					   and object files.  the routines 
					   return the number of bytes in the 
					   converted file.  the converted file
					   is assumed to be in tmpfil.  a 
					   returned value of 0 implies that 
					   something has gotten screwed up. 
					*/
long genparc();		/* convert a 5.0 archive to a 6.0 archive file */

long gencpy(); /* routine for copying a file which does not (or can not)
		  need to be converted.  the tmpfil file is made a copy of
		  the infil input file.  */


/* input/output/temporary files */

char *infilname, *outfilname, *tmpfilname, *nm_newarc, *nm_member, *xfilname;

FILE	*infil, *outfil, *tmpfil, *newarc, *member, *xfil;

char buffer[BUFSIZ]; /* file copy buffer */


#define MAXSYMS 2500 /* maximum number of symbols per object file */

struct {
	short sym;
	short adj;
	} patch[MAXSYMS];


int arcflag = 0;	/* flag indicates that an archive is being processed */
int flag5 = 0;		/* when set, use 5.0 convert behavior */
int xflag = 0;		/* when set, convert a xenix archive into 6.0 format */


/* main program for convert 

   UNIX 5.0 transition tool for converting pre 5.0 UNIX archives and object
   files to the 5.0 format.
	
   usage is 'convert infile outfile'

   input file infile is interpreted as a pre 5.0 archive or object file and
   output file outfile is generated as an equivalent 5.0  archive or object
   file.

   Update for 6.0 -----------------------

   UNIX 6.0 transition tool for converting 5.0 archive files to 6.0 archive
   files.  The usage is now:

   convert [-5 | -x] infile outfile

   where the -5 option allows for the 5.0 behavior (as is reasonable)
   and the -x option allows for conversion of XENIX archives.

*/

main ( argc, argv )

int argc;
char * argv[];

{
	extern int	stat( );
	unsigned int in_magic; /* magic number of input file */

	long bytes_out;
	int  bytes_in;
	unsigned short  fmode;
	struct stat  statrec;


	/* trap for interrupts */

	if ((signal(SIGINT, SIG_IGN)) == SIG_DFL)
		(void) signal(SIGINT, abort);
	if ((signal(SIGHUP, SIG_IGN)) == SIG_DFL)
		(void) signal(SIGHUP, abort);
	if ((signal(SIGQUIT, SIG_IGN)) == SIG_DFL)
		(void) signal(SIGQUIT, abort);
	if ((signal(SIGTERM, SIG_IGN)) == SIG_DFL)
		(void) signal(SIGTERM, abort);


	if (strcmp(argv[1], "-5") == 0)
	{
		flag5 = 1;
		argv++;
		argc--;
	} else if (strcmp(argv[1], "-x") == 0) {
		xflag = 1;
		argv++;
		argc--;
	}

	if (argc != 3) {
		fprintf(stderr,"usage: convert [-5 | -x] infile outfile\n");
		exit(1);
	}

	infilname = argv[1]; /* get input file name from command line */
	outfilname = argv[2];/* get output file name from command line*/

	if (strcmp(infilname,outfilname) == 0) {
		/* input cannot be the same as output */
		fprintf(stderr,
		   "convert: input file cannot be the same as output file\n");
		exit(1);
	}

	if ((infil = fopen(infilname,"r")) == NULL) 
		/* can't get the input file */
		readerr(infilname);
	if (stat(infilname, &statrec) != 0) {
		fprintf(stderr,"convert: can not obtain mode of file %s",
		    infilname);
		exit(1);
	}
	fmode=statrec.st_mode;

	/*
	* Check for accidental overwrite of possibly precious file
	*/
	if (access(outfilname, 0) == 0)
	{
		fprintf(stderr,
			"convert: output file \"%s\" already exists!\n",
			outfilname);
		exit(1);
	}
	if ((outfil = fopen(outfilname,"w")) == NULL) 
		/* can't write onto the output file */
		writerr(outfilname);

	tmpfilname = tempnam(TMPDIR,"conv");
	if ((tmpfil = fopen(tmpfilname,"w")) == NULL)
		/* can't write onto the temporary file */
		writerr(tmpfilname);


	/* let's get down to business: are we dealing with an archive or
	   an object file?  answer: look at the 'magic number' */

	if (fread(&in_magic,1,sizeof(in_magic),infil) != sizeof(in_magic))
		readerr(infilname);
	in_magic &= 0177777;
	fseek(infil,0L,0);

	switch (in_magic) {

		case OARMAG:			/* archive input */
			if (!flag5 && !xflag)
			{
				fprintf(stderr, "convert: \"%s\": must specify -5 or -x option to convert pre-5.0 archive\n", infilname);
				abort();
			}
			arcflag++;
			if (xflag) {
				xfilname = tempnam(TMPDIR, "conv");
				if ((xfil = fopen(xfilname, "w")) == NULL)
					writerr(xfilname);
				bytes_out = xgenarc(infil);
				if ((xfil = fopen(xfilname, "r")) == NULL)
					readerr(xfilname);
				fclose(tmpfil);
				if ((tmpfil = fopen(tmpfilname, "w")) == NULL)
					writerr(tmpfilname);
				bytes_out = genparc(xfil);
			} else
				bytes_out = genarc(infil);
			break;

		default:			/* questionable input */
			/*
			* See if it is a 5.0 archive to be converted
			*/
			if (fread(buffer, 1, sizeof(char) * SARMAG5, infil)
				!= sizeof(char) * SARMAG5)
			{
				readerr(infilname);
			}
			rewind(infil);
			if (strncmp(buffer, ARMAG5, SARMAG5) == 0)
			{
				if (flag5)
				{
					fprintf(stderr, "convert: \"%s\": cannot convert 5.0 archive with -5 option\n", infilname);
					abort();
				}
				bytes_out = genparc(infil);
			}
			else
				bytes_out = gencpy(infil, infilname);
			break;
		};


	/* copy converted output, which resides in the tmpfil file to the
	   outfil file */

	fclose(infil);
	if ((outfil = fopen(outfilname,"w")) == NULL)
		writerr(outfilname);

	/* always begin at the beginning */
	fclose(tmpfil);
	if ((tmpfil = fopen(tmpfilname,"r")) == NULL)
		/* can't read the temporary file */
		readerr(tmpfilname);

	while (bytes_out) {
		bytes_in = fread(buffer,1,BUFSIZ,tmpfil);
		bytes_out -= (long)fwrite(buffer,1,bytes_in,outfil);
		if (bytes_in < BUFSIZ && bytes_out > 0)
			writerr(outfilname);
	};

	/* we are all done, so clean up! */

	fclose(outfil);
	chmod(outfilname, fmode);
	fclose(tmpfil);
	unlink(tmpfilname);
	if (xflag) {
		fclose(xfil);
		unlink(xfilname);
	}
	exit(0);
}


/* interrupt handler: delete all the temp files, unlink the output
   and then abort with an error code of 1 */

void
abort ()
{
	if (infil)
		fclose(infil);		/* close all files */
	if (outfil)
		fclose(outfil);
	if (tmpfil)
		fclose(tmpfil);
	if (newarc)
		fclose(newarc);
	if (member)
		fclose(member);
	if (xfil)
		fclose(xfil);
	if (tmpfilname)
		unlink(tmpfilname);	/* delete temp file and output */
	if (outfilname)
		unlink(outfilname);
	if (nm_newarc)
		unlink(nm_newarc);
	if (nm_member)
		unlink(nm_member);
	if (xfilname)
		unlink(xfilname);
	exit(1);	/* abort! */
}


readerr(filname) /* input read error, give the user a reason and abort */

char *filname;

{
	fprintf(stderr,"convert: input error on file %s ",filname);
	perror(" ");
	abort();
}


writerr(filname) /* output write error, give the user a reason and abort */

char *filname;

{
	fprintf(stderr,"convert: output error on file %s ",filname);
	perror(" ");
	abort();
}

long
genarc (arcfile) /* procedure to convert a pre 5.0 archive into
		   a 5.0 archive format.  the converted file
		   is written onto the temp file tmpfil.  the
		   genarc function returns the number of bytes
		   in the converted file tmpfil. */

FILE *arcfile;

{
	long arc_size; /* size of converted archive file on output */
	long mem_size; /* size of an archive member file */
	unsigned short magic_nbr;
	short incr;

	struct ar_hdr5  new_header;
	struct oar_hdr oldf_header;
	struct arf_hdr5 newf_header;

	/* set up temporary files */

	nm_newarc = tempnam(TMPDIR,"conv");
	nm_member = tempnam(TMPDIR,"conv");

	if ((newarc = fopen(nm_newarc,"w")) == NULL)
		writerr(nm_newarc);

	/* skip past the magic number in the input file: arcfile */
	fseek(arcfile,(long)sizeof(int),0);
		
	/* set up the new archive header */
	strncpy(new_header.ar_magic,ARMAG5,SARMAG5);
	strncpy(new_header.ar_name,infilname,sizeof(new_header.ar_name));
	sputl(time(NULL),new_header.ar_date);
	sputl(0L,new_header.ar_syms); /* recreate archive without symbols */

		/* a word to the wise is sufficient */

	fprintf(stderr,"convert: warning, archive symbol table not created\n");
	fprintf(stderr,"         execute 'ar ts %s' to generate symbol table\n",
			outfilname);

	if ((arc_size = fwrite(&new_header,1,sizeof(new_header),newarc)) != 
		sizeof(new_header))
			writerr(nm_newarc);

	/* now process each archive member in turn */

	while (fread(&oldf_header,1,sizeof(oldf_header),arcfile) ==
			sizeof(oldf_header)) {

		/* translate header data for each member */
		strncpy(newf_header.arf_name,oldf_header.ar_name,
				sizeof(oldf_header.ar_name));
		sputl(oldf_header.ar_date,newf_header.arf_date);
		sputl((long)oldf_header.ar_uid,newf_header.arf_uid);
		sputl((long)oldf_header.ar_gid,newf_header.arf_gid);
		sputl(oldf_header.ar_mode,newf_header.arf_mode);

		/* prepare the member for conversion */
		
		if ((member = fopen(nm_member,"w")) == NULL)
			writerr(nm_member);
		
		mem_size = oldf_header.ar_size;

		if (mem_size & 1) { /* ar expects members to be evenly sized */
			mem_size++;
			incr = 1;
		} else
			incr = 0;

		while (mem_size >= BUFSIZ) {
			if (fread(buffer,1,BUFSIZ,arcfile) != BUFSIZ)
				readerr(infilname);
			if (fwrite(buffer,1,BUFSIZ,member) != BUFSIZ)
				writerr(nm_member);
			mem_size -= BUFSIZ;
		};
		if (mem_size) {
			if (fread(buffer,1,mem_size,arcfile) != mem_size)
				readerr(infilname);
			if (fwrite(buffer,1,mem_size,member) != mem_size)
				writerr(nm_member);
		};
		
		/* now perform the actual conversion */

		fclose(member);
		if ((member = fopen(nm_member,"r")) == NULL)
			readerr(nm_member);

		if (oldf_header.ar_size < sizeof(magic_nbr))
			goto just_copy;			/* don't bother */
		if (fread(&magic_nbr,1,sizeof(magic_nbr),member) !=
			sizeof(magic_nbr))
			readerr(nm_member);
		
		fseek(member,0L,0);
	
		switch (magic_nbr) {
			default:
			just_copy:
				mem_size = gencpy(member,newf_header.arf_name);
				break;
		};


		fclose(member);

		/* now let's put the sucker back into the new archive */

		sputl((mem_size-incr),newf_header.arf_size); /* finish up */
		
		if (fwrite(&newf_header,1,sizeof(newf_header),newarc) !=
			sizeof(newf_header)) writerr(nm_newarc);

		arc_size += mem_size + sizeof(newf_header);

		/* put the new converted member into the new archive */

		fclose(tmpfil);
		if ((tmpfil = fopen(tmpfilname,"r")) == NULL)
			readerr(tmpfilname);
		while (mem_size = fread(buffer,1,BUFSIZ,tmpfil))
			fwrite(buffer,1,mem_size,newarc);
	};

	/* copy new archive file to tmpfil */

	fclose(newarc);
	fclose(tmpfil);
	if ((newarc = fopen(nm_newarc,"r")) == NULL)
		readerr(nm_newarc);
	if ((tmpfil = fopen(tmpfilname,"w")) == NULL)
		writerr(tmpfilname);

	while (mem_size = fread(buffer,1,BUFSIZ,newarc))
		fwrite(buffer,1,mem_size,tmpfil);

	/* time to clean up */

	fclose(newarc);
	unlink(nm_newarc);
	fclose(member);
	unlink(nm_member);

	return(arc_size);
}

long
xgenarc (arcfile) /* procedure to convert a pre 5.0 archive into
		     a 5.0 archive format.  the converted file
		     is written onto the temp file xfil.  the
		     xgenarc function returns the number of bytes
		     in the converted file xfil. */

FILE *arcfile;

{
	long arc_size; /* size of converted archive file on output */
	long mem_size; /* size of an archive member file */
	unsigned short magic_nbr;
	short incr;

	struct ar_hdr5  new_header;
	struct xar_hdr oldf_header;
	struct arf_hdr5 newf_header;

	/* set up temporary files */

	nm_newarc = tempnam(TMPDIR,"conv");
	nm_member = tempnam(TMPDIR,"conv");

	if ((newarc = fopen(nm_newarc,"w")) == NULL)
		writerr(nm_newarc);

	/* skip past the magic number in the input file: arcfile */
	fseek(arcfile,(long)sizeof(short),0);
		
	/* set up the new archive header */
	strncpy(new_header.ar_magic,ARMAG5,SARMAG5);
	strncpy(new_header.ar_name,infilname,sizeof(new_header.ar_name));
	sputl(time(NULL),new_header.ar_date);
	sputl(0L,new_header.ar_syms); /* recreate archive without symbols */

		/* a word to the wise is sufficient */

	fprintf(stderr,"convert: warning, archive symbol table not created\n");
	fprintf(stderr,"         execute 'ar ts %s' to generate symbol table\n",
			outfilname);

	if ((arc_size = fwrite(&new_header,1,sizeof(new_header),newarc)) != 
		sizeof(new_header))
			writerr(nm_newarc);

	/* now process each archive member in turn */

	while (fread(&oldf_header,1,sizeof(oldf_header),arcfile) ==
			sizeof(oldf_header)) {

		/* if the member has a name of "__.SYMDEF" then we can
		   delete it from the new archive.  this special member
		   was used with earlier versions of random access
		   libraries.  it has no application or use in the new
		   archive format. */


		if (strcmp(oldf_header.ar_name,"__.SYMDEF") == 0) {
			fseek(arcfile,EVEN(oldf_header.ar_size),1); /* skip */
			continue; /* and go on to the next archive member */
		};

		/* translate header data for each member */
		strncpy(newf_header.arf_name,oldf_header.ar_name,
				sizeof(oldf_header.ar_name));
		sputl(oldf_header.ar_date,newf_header.arf_date);
		sputl((long)oldf_header.ar_uid,newf_header.arf_uid);
		sputl((long)oldf_header.ar_gid,newf_header.arf_gid);
		sputl((long)oldf_header.ar_mode,newf_header.arf_mode);

		/* prepare the member for conversion */
		
		if ((member = fopen(nm_member,"w")) == NULL)
			writerr(nm_member);
		
		mem_size = oldf_header.ar_size;

		if (mem_size & 1) { /* ar expects members to be evenly sized */
			mem_size++;
			incr = 1;
		} else
			incr = 0;

		while (mem_size >= BUFSIZ) {
			if (fread(buffer,1,BUFSIZ,arcfile) != BUFSIZ)
				readerr(infilname);
			if (fwrite(buffer,1,BUFSIZ,member) != BUFSIZ)
				writerr(nm_member);
			mem_size -= BUFSIZ;
		};
		if (mem_size) {
			if (fread(buffer,1,mem_size,arcfile) != mem_size)
				readerr(infilname);
			if (fwrite(buffer,1,mem_size,member) != mem_size)
				writerr(nm_member);
		};
		
		/* now perform the actual conversion */

		fclose(member);
		if ((member = fopen(nm_member,"r")) == NULL)
			readerr(nm_member);

		/* just copy the member */
		mem_size = gencpy(member,newf_header.arf_name);

		fclose(member);

		/* now let's put the sucker back into the new archive */

		sputl((mem_size-incr),newf_header.arf_size); /* finish up */
		
		if (fwrite(&newf_header,1,sizeof(newf_header),newarc) !=
			sizeof(newf_header)) writerr(nm_newarc);

		arc_size += mem_size + sizeof(newf_header);

		/* put the new converted member into the new archive */

		fclose(tmpfil);
		if ((tmpfil = fopen(tmpfilname,"r")) == NULL)
			readerr(tmpfilname);
		while (mem_size = fread(buffer,1,BUFSIZ,tmpfil))
			fwrite(buffer,1,mem_size,newarc);
	};

	/* copy new archive file to xfil */

	fclose(newarc);
	fclose(xfil);
	if ((newarc = fopen(nm_newarc,"r")) == NULL)
		readerr(nm_newarc);
	if ((xfil = fopen(xfilname,"w")) == NULL)
		writerr(xfilname);

	while (mem_size = fread(buffer,1,BUFSIZ,newarc))
		fwrite(buffer,1,mem_size,xfil);

	/* time to clean up */

	fclose(newarc);
	unlink(nm_newarc);
	fclose(member);
	unlink(nm_member);
	fclose(xfil);

	return(arc_size);
}


#define NSYMS	2000	/* max number of symbols in random access directory */

/*
* Convert a 5.0 archive to a 6.0 archive.  Write output to ``tmpfil''.
*/
long
genparc(arcfile)
	FILE *arcfile;
{
	char sym_buf[NSYMS * (SYMNMLEN + 1)];	/* room for string table */
	char sym_off[NSYMS][sizeof(long) / sizeof(char)];	/* offsets */
	char num_syms[sizeof(long) / sizeof(char)];	/* num sym in dir. */
	char hdr_buf[(sizeof(struct ar_hdr) + 1) * sizeof(char)];
	struct ar_hdr5 old_hdr;		/* 5.0 entire archive header */
	struct arf_hdr5 old_fhdr;	/* 5.0 archive member header */
	struct ar_sym5 old_syms[NSYMS];	/* 5.0 random access directory */
	long off_pair[NSYMS][2];	/* map from 5.0 to 6.0 offsets */
	struct ar_hdr new_fhdr;		/* 6.0 archive member header */
	long nsyms, diff, size;
	int lastoff;
	register char *sp;
	register int i, j;

	if (fread((char *)&old_hdr, sizeof(old_hdr), 1, arcfile) != 1)
		readerr(infilname);
	/*
	* write out archive magic string
	*/
	if (fwrite(ARMAG, sizeof(char), SARMAG, tmpfil) != SARMAG)
		writerr(tmpfilname);
	for (i = 0; i < sizeof(long) / sizeof(char); i++)
		num_syms[i] = old_hdr.ar_syms[i];
	nsyms = sgetl(num_syms);
	if (nsyms > NSYMS)
	{
		fprintf(stderr,
			"convert: \"%s\" too many symbols in directory\n",
			infilname);
		abort();
	}
	if (fread(old_syms, sizeof(old_syms[0]), nsyms, arcfile) != nsyms)
		readerr(infilname);
	if (nsyms > 0)		/* Create a new symbol directory */
	{
#ifdef FLEXNAMES
		int trunc_cnt = 0;	/* num. of poss. truncated symbols */
#endif

		/*
		* length of the offset table
		*/
		size = (nsyms + 1) * (sizeof(long) / sizeof(char));
		/*
		* generate string table
		*/
		sp = sym_buf;
		for (j = 0; j < nsyms; j++)
		{
			i = 0;
			while (i < SYMNMLEN && old_syms[j].sym_name[i] != ' ')
				*sp++ = old_syms[j].sym_name[i++];
#ifdef FLEXNAMES
			if (i >= SYMNMLEN)
				trunc_cnt++;
#endif
			*sp++ = '\0';
		}
#ifdef FLEXNAMES
		/*
		* note the number of possibly truncated
		* symbols in the directory
		*/
		if (trunc_cnt > 0)
			fprintf(stderr, "Note: %d of %ld symbols possibly truncated in archive symbol table\n",
				trunc_cnt, nsyms);
#endif
		/*
		* set size to the actual length of the directory file
		*/
		size += (sp - sym_buf);
		if (size & 01)		/* archive assumes even length */
		{
			size++;
			*sp++ = '\0';
		}
		/*
		* write out header for symbol directory
		*/
		if (sprintf(hdr_buf, "%-16s%-12ld%-6u%-6u%-8o%-10ld%-2s",
			"/               ", time((long *)0), 0, 0, 0,
			size, ARFMAG) != sizeof(new_fhdr))
		{
			fprintf(stderr,
				"convert: internal header creation error\n");
			abort();
		}
		(void)strncpy((char *)&new_fhdr, hdr_buf, sizeof(new_fhdr));
		if (fwrite((char *)&new_fhdr, sizeof(new_fhdr), 1, tmpfil) != 1)
			writerr(tmpfilname);
		/*
		* set size to the distance to the beginning of
		* the first actual archive member's header.
		*/
		size += (SARMAG * sizeof(char)) + sizeof(new_fhdr);
		/*
		* write out the symbol directory, skipping the offsets table
		*/
		if (fwrite(num_syms, sizeof(num_syms), 1, tmpfil) != 1 ||
			fseek(tmpfil, sizeof(sym_off[0]) * nsyms, 1) != 0 ||
			fwrite(sym_buf, sizeof(char), sp - sym_buf, tmpfil)
			!= sp - sym_buf)
		{
			writerr(tmpfilname);
		}
	}
	else
		size = sizeof(char) * SARMAG;
	/*
	* for each archive file member, create a new header, copy the
	* contents, and change the optional pad to a \n, remember mapping
	* from old header offsets to new header offsets.
	*/
	lastoff = 0;
	while (fread((char *)&old_fhdr, sizeof(old_fhdr), 1, arcfile) == 1)
	{
		char	ustr[12];  /* string used to hold uid in decimal */
		char	gstr[12];  /* string used to hold gid in decimal */
		off_pair[lastoff][0] = ftell(arcfile) - sizeof(old_fhdr);
		off_pair[lastoff][1] = ftell(tmpfil);
		lastoff++;
		diff = sgetl(old_fhdr.arf_size);
		sprintf(ustr, "%u", sgetl(old_fhdr.arf_uid));
		sprintf(gstr, "%u", sgetl(old_fhdr.arf_gid));
		if (sprintf(hdr_buf, "%-16s%-12ld%-6.6s%-6.6s%-8o%-10ld%-2s",
			strcat(old_fhdr.arf_name, "/"),
			sgetl(old_fhdr.arf_date), ustr,
			gstr,sgetl(old_fhdr.arf_mode),
			diff, ARFMAG) != sizeof(new_fhdr))
		{
			fprintf(stderr,
				"convert: internal header creation error\n");
			abort();
		}
		(void)strncpy((char *)&new_fhdr, hdr_buf, sizeof(new_fhdr));
		if (fwrite((char *)&new_fhdr, sizeof(new_fhdr), 1, tmpfil) != 1)
			writerr(tmpfilname);
		size += sizeof(new_fhdr);
		j = diff;
		while ((i = fread(buffer, sizeof(char),
			j > BUFSIZ ? BUFSIZ : j, arcfile)) == BUFSIZ)
		{
			j -= fwrite(buffer, sizeof(char), i, tmpfil);
		}
		if (j >= BUFSIZ)
			readerr(infilname);
		if (fwrite(buffer, sizeof(char), i, tmpfil) != i)
			writerr(tmpfilname);
		size += diff;
		if (diff & 01)		/* eat pad also */
		{
			if (fread(buffer, sizeof(char), 1, arcfile) != 1)
				readerr(infilname);
			buffer[0] = '\n';
			if (fwrite(buffer, sizeof(char), 1, tmpfil) != 1)
				writerr(tmpfilname);
			size++;
		}
	}
	/*
	* Rewrite over the table of offsets in the archive symbol
	* directory.  Assume that off_pair is in sorted order.
	*/
	if (fseek(tmpfil,
		SARMAG * sizeof(char) + sizeof(new_fhdr) + sizeof(long), 0)
		!= 0)
	{
		fprintf(stderr, "convert: cannot seek back to offsets table\n");
		abort();
	}
	j = 0;
	for (i = 0; i < nsyms; i++)
	{
		long off = sgetl(old_syms[i].sym_ptr);

		for (off_pair[lastoff][0] = off; off_pair[j][0] != off; j++)
			;
		if (j >= lastoff)
		{
			fprintf(stderr, "convert: cannot find old offset!\n");
			abort();
		}
		sputl(off_pair[j][1], sym_off[i]);
	}
	/*
	* Now, insert the offsets table.
	*/
	if (fwrite(sym_off, sizeof(sym_off[0]), nsyms, tmpfil) != nsyms)
		writerr(tmpfilname);
	/*
	* Check for extra stuff - just to be sure
	*/
	if (fread(buffer, sizeof(char), BUFSIZ, arcfile) != 0)
	{
		fprintf(stderr,
			"convert: extra chars at end of 5.0 archive file %s\n",
			infilname);
		abort();
	}
	return (size);
}




long
gencpy(cpyfile, cpyname) /* routine for copying a file which does not 
		   	    need to be converted.  the tmpfil file is made
		    	    a copy of the infil input file.  */

FILE *cpyfile;
char *cpyname;

{
	long cpy_size;
	int bytes_in;

	cpy_size = 0;
	
	fclose(tmpfil); /* start with a clean file */
	if ((tmpfil = fopen(tmpfilname,"w")) == NULL)
		writerr(tmpfilname);

	while (bytes_in = fread(buffer,1,BUFSIZ,cpyfile)) {
		fwrite(buffer,1,bytes_in,tmpfil);
		cpy_size = cpy_size + bytes_in;
		}

	/* warn the user */
#if u3b || M32
	if(strcmp(infilname, cpyname) == 0)
#endif
	fprintf(stderr,"convert: warning, contents of file %s not modified\n",
			cpyname);

	return(cpy_size);
}
