#ident	"@(#)ktool:i386at/ktool/scodb/make_info.c	1.1"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/scodb/dbg.h>
#include	<sys/scodb/stunv.h>

extern int errno;

ldsv_usage(char *name) {
	fprintf(stderr, "usage: %s [ -v ] -o <output file> [ -l <kernel> ] idef-files...\n", name);
	exit(1);
}

main(c, v)
	char **v;
{
	int r, stun_size, vari_size, lsym_size;
	int cc;
	int vers;
	char magic[IDF_MAGICL];
	FILE *f_in, *f_out, *fopen();
	int verbose = 0, debug = 0;
	char *outfile_name = NULL;
	char *kernel_name = NULL;
	extern char *optarg;
	extern int optind;

	while ((r = getopt(c, v, "dvo:l:")) != -1)
		switch (r) {
			case 'd':
				++debug;
				break;

			case 'v':
				verbose = 1;
				break;

			case 'o':
				if (outfile_name)
					ldsv_usage(v[0]);
				outfile_name = optarg;
				break;

			case 'l':
				if (kernel_name)
					ldsv_usage(v[0]);
				kernel_name = optarg;
				break;

			case '?':
			default:
				ldsv_usage(v[0]);
		}
	
	if (outfile_name == NULL)
		ldsv_usage(v[0]);

	v += optind;
	if (!*v--) {
		fprintf(stderr, "no input files!\n");
		ldsv_usage(v[0]);
	}

	if ((f_out = fopen(outfile_name, "w")) == NULL)
		pferror(1, "Error %d opening output file \"%s\"",
			errno, outfile_name);

	while (*++v) {
		if ((f_in = fopen(*v, "r")) == NULL) {
			pferror(0, "Can't open input file \"%s\"", *v);
			continue;
		}
		if (debug) {
			printf("%s: ", *v);
			fflush(stdout);
		}
		cc = fread(magic, sizeof magic, 1, f_in);
		if (cc != 1) {
		inco:
			if (!debug)
				fprintf(stderr, "%s: ", *v);
			fprintf(stderr, "incomplete idef file\n");
			goto nextf;
		}
		else if (memcmp(magic, IDF_MAGIC, IDF_MAGICL)) {
			if (!debug)
				fprintf(stderr, "%s: ", *v);
			fprintf(stderr, "made with old version of `stv' program (no magic number present)\n");
			goto nextf;
		}
		cc = fread(&vers, sizeof vers, 1, f_in);
		if (cc != 1)
			goto inco;
		if (vers != IDF_VERSION) {
			if (!debug)
				fprintf(stderr, "%s: ", *v);
			fprintf(stderr, "made with wrong version of `stv' program (%d instead of %d)\n", vers, IDF_VERSION);
			goto nextf;
		}
		r_stuns(f_in);
		r_varis(f_in);
		if (debug)
			putchar('\n');
	nextf:	fclose(f_in);
	}

	seekover_usym_header(f_out);

	stun_size = d_cstuns(f_out);
	vari_size = d_cvaris(f_out);

	if (kernel_name)
		lsym_size = write_local_symbols(kernel_name, f_out);
	else
		lsym_size = 0;

	write_usym_header(f_out, stun_size, vari_size, lsym_size, verbose);

	fclose(f_out);
}

seekover_usym_header(FILE *fp)
{
	fseek(fp, sizeof(struct scodb_usym_info), 0);
}

write_usym_header(FILE *fp, int stun_size, int vari_size, int lsym_size,
		  int verbose)
{
	struct scodb_usym_info info;

	info.magic = USYM_MAGIC;
	info.stun_offset = sizeof(info);
	info.stun_size = stun_size;
	info.vari_offset = info.stun_offset + stun_size;
	info.vari_size = vari_size;
	info.lineno_offset = 0;
	info.lineno_size = 0;

	if (lsym_size) {
		/*
		 * Local symbol table
		 */
		info.lsym_offset = info.vari_offset + vari_size;
		info.lsym_size = lsym_size;
	} else {
		info.lsym_offset = 0;
		info.lsym_size = 0;
	}

	if (verbose) {
		fprintf(stderr, "stun: offset %d, size %d\n",
			info.stun_offset, info.stun_size);
		fprintf(stderr, "vari: offset %d, size %d\n",
			info.vari_offset, info.vari_size);
		if (lsym_size)
			fprintf(stderr, "lsym: offset %d, size %d\n",
				info.lsym_offset, info.lsym_size);
	}

	rewind(fp);
	fwrite(&info, sizeof(info), 1, fp);
}
