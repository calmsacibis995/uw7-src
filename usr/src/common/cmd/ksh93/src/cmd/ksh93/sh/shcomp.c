#ident	"@(#)ksh93:src/cmd/ksh93/sh/shcomp.c	1.2"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * shell script to shell binary converter
 *
 */

#include	"defs.h"
#include	"shnodes.h"
#include	"path.h"
#include	"io.h"

#define CNTL(x)	((x)&037)
#define VERSION	2
static const char id[] = "\n@(#)shcomp (AT&T Bell Laboratories) 12/28/93\0\n";
static const char header[6] = { CNTL('k'),CNTL('s'),CNTL('h'),0,VERSION,0 };

main(int argc, char *argv[])
{
	Sfio_t *in, *out;
	union anynode *t;
	char *cp;
	int n, nflag=0, vflag=0, dflag=0;
	error_info.id = argv[0];
	while(n = optget(argv, "Dnv [infile [outfile]]" )) switch(n)
	{
	    case 'D':
		dflag=1;
		break;
	    case 'v':
		vflag=1;
		break;
	    case 'n':
		nflag=1;
		break;
	    case ':':
		error(2, opt_arg);
		break;
	    case '?':
		error(ERROR_usage(2), opt_arg);
		break;
	}
	sh_init(argc,argv);
	argv += opt_index;
	argc -= opt_index;
	if(error_info.errors || argc>2)
		error(ERROR_usage(2),optusage((char*)0));
	if(cp= *argv)
	{
		argv++;
		if((n=path_open(cp,path_get(cp))) < 0)
			n = path_open(cp,"");
		if(n < 0)
			error(ERROR_system(1),gettxt(e_open_id,e_open),cp);
		in = sh_iostream(n);
	}
	else
		in = sfstdin;
	if(cp= *argv)
	{
		if(!(out = sfopen(NIL(Sfio_t*),cp,"w")))
			error(ERROR_system(1),gettxt(e_create_id,e_create),cp);
	}
	else
		out = sfstdout;
	if(dflag)
		sh_onoption(SH_DICTIONARY|SH_NOEXEC);
	if(nflag)
		sh_onoption(SH_NOEXEC);
	if(vflag)
		sh_onoption(SH_VERBOSE);
	if(!dflag)
		sfwrite(out,header,sizeof(header));
	sh.inlineno = 1;
	while(1)
	{
		stakset(NIL(char*),0);
		if(t = (union anynode*)sh_parse(in,0))
		{
			if(!dflag && sh_tdump(out,t) < 0)
				error(ERROR_exit(1),gettxt(":210","dump failed"));
		}
		else if(sfeof(in))
			break;
		if(sferror(in))
			error(ERROR_system(1),gettxt(":211","I/O error"));
	}
	if(in!=sfstdin)
		sfclose(in);
	if(out!=sfstdout)
		sfclose(out);
	return(0);
}
