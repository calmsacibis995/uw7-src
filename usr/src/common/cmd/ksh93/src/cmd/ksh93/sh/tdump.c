#ident	"@(#)ksh93:src/cmd/ksh93/sh/tdump.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * shell parse tree dump
 *
 */

#include	"defs.h"
#include	"shnodes.h"
#include	"path.h"
#include	"io.h"

static int p_comlist(const struct dolnod*);
static int p_arg(const struct argnod*);
static int p_comarg(const struct comnod*);
static int p_redirect(const struct ionod*);
static int p_switch(const struct regnod*);
static int p_tree(const union anynode*);
static int p_string(const char*);

static Sfio_t *outfile;

int sh_tdump(Sfio_t *out, const union anynode *t)
{
	outfile = out;
	return(p_tree(t));
}
/*
 * print script corresponding to shell tree <t>
 */
static int p_tree(register const union anynode *t)
{
	if(!t)
		return(sfputl(outfile,-1));
	if(sfputl(outfile,t->tre.tretyp)<0)
		return(-1);
	switch(t->tre.tretyp&COMMSK)
	{
		case TTIME:
		case TPAR:
			return(p_tree(t->par.partre)); 
		case TCOM:
			return(p_comarg((struct comnod*)t));
		case TSETIO:
		case TFORK:
			if(sfputu(outfile,t->fork.forkline)<0)
				return(-1);
			if(p_tree(t->fork.forktre)<0)
				return(-1);
			return(p_redirect(t->fork.forkio));
		case TIF:
			if(p_tree(t->if_.iftre)<0)
				return(-1);
			if(p_tree(t->if_.thtre)<0)
				return(-1);
			return(p_tree(t->if_.eltre));
		case TWH:
			if(t->wh.whinc)
			{
				if(p_tree((union anynode*)(t->wh.whinc))<0)
					return(-1);
			}
			else
			{
				if(sfputl(outfile,-1)<0)
					return(-1);
			}
			if(p_tree(t->wh.whtre)<0)
				return(-1);
			return(p_tree(t->wh.dotre));
		case TLST:
		case TAND:
		case TORF:
		case TFIL:
			if(p_tree(t->lst.lstlef)<0)
				return(-1);
			return(p_tree(t->lst.lstrit));
		case TARITH:
			if(sfputu(outfile,t->ar.arline)<0)
				return(-1);
			return(p_arg(t->ar.arexpr));
		case TFOR:
			if(p_tree(t->for_.fortre)<0)
				return(-1);
			if(p_string(t->for_.fornam)<0)
				return(-1);
			return(p_tree((union anynode*)t->for_.forlst));
		case TSW:
			if(p_arg(t->sw.swarg)<0)
				return(-1);
			return(p_switch(t->sw.swlst));
		case TFUN:
			if(sfputu(outfile,t->funct.functline)<0)
				return(-1);
			if(p_string(t->funct.functnam)<0)
				return(-1);
			if(p_tree(t->funct.functtre)<0)
				return(-1);
			return(p_tree((union anynode*)t->funct.functargs));
		case TTST:
			if(sfputu(outfile,t->tst.tstline)<0)
				return(-1);
			if((t->tre.tretyp&TPAREN)==TPAREN)
				return(p_tree(t->lst.lstlef)); 
			else
			{
				if(p_arg(&(t->lst.lstlef->arg))<0)
					return(-1);
				if((t->tre.tretyp&TBINARY))
					return(p_arg(&(t->lst.lstrit->arg)));
			}
	}
}

static int p_arg(register const struct argnod *arg)
{
	register int n;
	struct fornod *fp;
	while(arg)
	{
		if((n = strlen(arg->argval)) || arg->argflag)
			fp=0;
		else
		{
			fp=(struct fornod*)arg->argchn.ap;
			n = strlen(fp->fornam)+1;
		}
		sfputu(outfile,n+1);
		if(fp)
			sfputc(outfile,0);
		sfwrite(outfile,fp?fp->fornam:arg->argval,n);
		sfputc(outfile,arg->argflag);
		if(fp)
			p_tree(fp->fortre);
		arg = arg->argnxt.ap;
	}
	return(sfputu(outfile,0));
}

static int p_redirect(register const struct ionod *iop)
{
	while(iop)
	{
		sfputl(outfile,iop->iofile);
		p_string(iop->ioname);
		if(iop->iodelim)
		{
			p_string(iop->iodelim);
			sfputl(outfile,iop->iosize);
			sfseek(sh.heredocs,iop->iooffset,SEEK_SET);
			sfmove(sh.heredocs,outfile, iop->iosize,-1);
		}
		else
			sfputu(outfile,0);
		iop = iop->ionxt;
	}
	return(sfputl(outfile,-1));
}

static int p_comarg(const register struct comnod *com)
{
	p_redirect(com->comio);
	p_arg(com->comset);
	if(!com->comarg)
		sfputl(outfile,-1);
	else if(com->comtyp&COMSCAN)
		p_arg(com->comarg);
	else
		p_comlist((struct dolnod*)com->comarg);
	return(sfputu(outfile,com->comline));
}

static int p_comlist(const struct dolnod *dol)
{
	register char *cp, *const*argv;
	register int n;
	argv = dol->dolval+ARG_SPARE;
	while(cp = *argv)
		argv++;
	n = argv - (dol->dolval+1);
	sfputl(outfile,n);
	argv = dol->dolval+ARG_SPARE;
	while(cp  = *argv++)
		p_string(cp);
	return(sfputu(outfile,0));
}

static int p_switch(register const struct regnod *reg)
{
	while(reg)
	{
		sfputl(outfile,reg->regflag);
		p_arg(reg->regptr);
		p_tree(reg->regcom);
		reg = reg->regnxt;
	}
	return(sfputl(outfile,-1));
}

static int p_string(register const char *string)
{
	register size_t n=strlen(string);
	if(sfputu(outfile,n+1)<0)
		return(-1);
	return(sfwrite(outfile,string,n));
}
