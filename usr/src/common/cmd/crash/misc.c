#ident	"@(#)crash:common/cmd/crash/misc.c	1.2.2.2"

/*
 * This file contains code for the crash functions:  ?, help, redirect,
 * and quit, as well as the command interpreter and redirect().
 * It also contains the command processing for nm and vtop functions.
 */

#include <sys/param.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/user.h>
#include <setjmp.h>
#include <locale.h>
#include "crash.h"

extern int opipe;				/* open pipe flag */
static char *rediromode;			/* used by redirect() */
extern struct func *functab;
static char aliases[] =
"alias:\tuniquely identifiable initial substrings are acceptable aliases\n";

/* returns argcnt, and args contains all arguments */
int
getcmd()
{
	register char		*p;
	int			n;
	FILE			*ofp;
	FILE			*ifp;
	static char		line[LINESIZE];

	if(bp)
		ifp = bp;
	else {
		ifp = stdin;
		(void) fprintf(stdout,"> ");
	}
	rediromode = "a";
	(void) fflush(stdout);
	(void) fflush(ifp);
	if ((p = fgets(line, sizeof(line), ifp)) == (char *)0) {
		fprintf(fp,"\n");
		exit(0);
		/* NOTREACHED */
	}
	if ((n = strlen(line)) > 0) {
		if (line[--n] == '\n')
			line[n] = '\0';
		else
			while ((n = getc(ifp)) != EOF && n != '\n')
				;
	}

	argcnt = 0;
	args[0] = (char *)0;
	optarg = (char *)0;

	while (*p && isspace(*p))
		p++;

	switch (*p) {
	case '|':
	case '>':
		error("missing command: \"%c\" unexpected\n", *p);
		/* NOTREACHED */
	case '!':
		while (*++p && isspace(*p))
			;
		if (*p == '\0')
			p = "exec ${SHELL:-/bin/sh}";
		(void) system(p);
		/* FALLTHRU */
	case '\0':
		return;
	}

	for (;;) {
		if (argcnt >= NARGS-1) {
			prerrmes("excess arguments ignored: %s\n", p);
			break;
		}

		args[argcnt++] = p;

		switch (*p) {
		case '>':
			args[--argcnt] = (char *)0;
			if (*(p+1) != '>')
				rediromode = "w";
			else {
				rediromode = "a";
				p++;
			}
			do {
				p++;
			} while (*p && isspace(*p));
			if (*p == '\0') {
				error("no filename after \">\"\n");
				/* NOTREACHED */
			}
			for (optarg = p; *p && !isspace(*p); p++)
				;
			break;

		case '!':
		case '|':
			args[--argcnt] = (char *)0;
			do {
				p++;
			} while (*p && isspace(*p));
			if (*p == '\0') {
				error("no shell command after \"!\" or \"|\"\n");
				/* NOTREACHED */
			}
			if (rp != (FILE *)0 || optarg != (char *)0) {
				error("cannot use pipe with redirected output\n");
				/* NOTREACHED */
			}
			ofp = fp;
			fp = (FILE *)0;
			opipe = 1;
			pipesig = signal(SIGPIPE, sigint);
			fp = popen(p, "w");
			if (fp == (FILE *)0) {
				(void) signal(SIGPIPE, pipesig);
				opipe = 0;
				fp = ofp;
				error("cannot open pipe\n");
				/* NOTREACHED */
			}
			args[argcnt] = (char *)0;
			return;

		default:
			n = 0;
			do {
				if (*p == '(')
					n++;
				else if (*p == ')' && n > 0)
					n--;
			} while (*++p != '\0' && (n || ! isspace(*p)));
			break;
		}
		if (*p == '\0')
			break;		/* last argument */
		*p++ = '\0';
		while (*p && isspace(*p))
			p++;
		if (*p == '\0')
			break;		/* trailing spaces */
	}
	args[argcnt] = (char *)0;
	if (optarg)
		redirect();
}


/* get arguments for ? function */
int
getfuncs()
{
	int c;

	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	prfuncs();
}

/* print all function names in columns */
int
prfuncs()
{
	int i;
	struct func *ff;
	char tempbuf[20];

	for(i = 0; i < tabsize; i++) {
		ff = functab + i;
		if(*ff->description != '(')
			fprintf(fp,"%-19s",ff->name);
		else {
			tempbuf[0] = 0;
			strcat(tempbuf,ff->name);
			strcat(tempbuf," ");
			strcat(tempbuf,ff->description);
			strcat(tempbuf," ");
			fprintf(fp,"%-19s",tempbuf);
		}
		if ((i + 1) % 4 == 0)
			fprintf(fp,"\n");
	}
	fprintf(fp,"\n");
}

/* get arguments for help function */
int
gethelp()
{
	int c, od;
	int all = 0;
	struct func *ff;
	extern char aliases[], helphelp[], odhelp[];

	optind = 1;
	while((c = getopt(argcnt,args,"w:e")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'e' :	all = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		if(all)
			longjmp(syn,0);
		do {
			od = prhelp(args[optind++]);
			fputs(aliases,fp);
			if(od)
				fputs(odhelp,fp);
			if(args[optind])
				fputs("\n",fp);
		}while(args[optind]);
	}
	else if(all) {
		fputs(helphelp,fp);
		fputs(aliases,fp);
		fputs(odhelp,fp);
		fputs("\n",fp);
		for(ff = functab; ff->name; ff++) {
			if (*ff->description == '(')
				continue;
			prhelp(ff->name);
			fputs("\n",fp);
		}
	}
	else {
		prhelp("help");
		fputs(aliases,fp);
		fputs("\n",fp);
		fputs(helphelp,fp);
	}
}

/* print function information */
int
prhelp(string)
char *string;
{
	int found = 0;
	int len;
	struct func *ff,*aa;
	extern int getod();

	for (ff = functab; ff->name; ff++) {
		if (strcmp(ff->name,string) == 0) {
			found = 1;
			break;
		}
	}
	if (!found) {
		len = strlen(string);
		for (ff = functab; ff->name; ff++) {
			if (found && ff->call == aa->call)
				continue;
			if (strncmp(ff->name,string,len) == 0) {
				found++;
				aa = ff;
			}
		}
		ff = aa;
	}
	if (found != 1) {
		error("%s is an %s command name\n",string,
			found? "ambiguous": "unrecognized");
	}
	aa = ff;
	if(*ff->description == '(') {
		for(aa = functab;aa->name != NULL;aa++)
			if((aa->call == ff->call) && (*aa->description != '('))
				break;
		if(aa->name == NULL)
			aa = ff;
	}
	fprintf(fp,"%s %s\n\t%s\n",ff->name,aa->syntax,aa->description);
	found = 0;
	for(aa = functab;aa->name != NULL;aa++) {
		if((aa->call == ff->call) && (strcmp(aa->name,ff->name) != 0)) {
			if(!found)
				fprintf(fp,"alias:\t");
			fprintf(fp,"%s ",aa->name);
			found = 1;
		}
	}
	if(found)
		fprintf(fp,"\n");
	return ff->call == getod;
}

/* terminate crash session */
int
getquit()
{
	if(rp)
		fclose(rp);
	exit(0);
}

/* get arguments for redirect function */
int
getredirect()
{
	int c;
	int close = 0;

	optind = 1;
	while((c = getopt(argcnt,args,"w:c")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'c' :	close = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) { 
		if(args[optind+1])
			longjmp(syn,0);
		prredirect(args[optind],1);
	}
	else prredirect(NULL,close);
}

/* print results of redirect function */
int
prredirect(string,close)
char *string;
int close;
{
	static char *strduped;
	extern char *outfile;

	if(close && rp) {
		if(fp == rp)
			fp = stdout;
		fclose(rp);
		rp = NULL;
		if(strduped) {
			free(strduped);
			strduped = NULL;
		}
		outfile = "stdout";
	}
	if(string) {
		if(!(rp = fopen(string,"a")))
			error("cannot open outfile %s\n",string);
		if (fp == stdout)
			fp = rp;
		outfile = strduped = strdup(string);
		/* which might leave NULL there */
	}
	else
		string = outfile;
	if(string) {
		fprintf(fp,"outfile = %s\n",string);
		if(fp == rp)
			fprintf(stdout,"outfile = %s\n",string);
	}
}

/* file redirection for all commands */
redirect(void)
{
	int i;
	FILE *ofp;

	ofp = fp;
	if (opipe || (fp != stdout && fp != rp && fp != (FILE *)0)) {
		fprintf(stdout,"file redirection option ignored\n");
		fflush(stdout);
	}
	else if(fp = fopen(optarg,rediromode)) {
		fprintf(fp,"\n> ");
		for(i=0;i<argcnt;i++)
			fprintf(fp,"%s ",args[i]);
		fprintf(fp,"\n");
	}
	else {
		fp = ofp;
		error("cannot open outfile %s\n",optarg);
	}
}

/* get arguments for nm, ds, sym, symval, ts function: print in symtab.c */
getnm(void)
{
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) != EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind]) {
		do prnm(strcon(args[optind++],'h'));
		while (args[optind]);
	}
	else
		prnm_all();
}

/* get arguments for vtop function, and print the results */
getvtop(void)
{
	int proc = Cur_proc;
	vaddr_t vaddr;
	phaddr_t paddr, faddr;
	int first = 1;
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:s:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 's' :	proc = setproc();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (!args[optind])
		longjmp(syn,0);

	while (args[optind]) {
		vaddr = strcon(args[optind++],'h');
		if ((paddr = cr_vtop(vaddr, proc, NULL, B_TRUE)) == -1)
			continue;
		if (first) {
			fprintf(fp," VIRTUAL  PHYSICAL%s\n",
				active? "": "   DUMPLOC");
			first = 0;
		}
		fprintf(fp, "%8x %9llx", vaddr, paddr);
		if (!active) {
			switch (faddr = cr_ptof(paddr, NULL)) {
			case -1: fprintf(fp, "   (hole)");	break;
			case  0: fprintf(fp, "   (zero)");	break;
			default: fprintf(fp, " %9llx", faddr);	break;
			}
		}
		fprintf(fp, "\n");
	}
}

char helphelp[] =
"\n"
"New features of this crash: compatible with UnixWare Gemini UP and MP\n"
"Command Line Processing: supports shell-like use of | > and >> (-w redundant)\n"
"Argument Processing: supports numbers, symbols, structure fields, expressions,\n"
"         indirections and saved values e.g. \"od $a=*l.procp+1c\": saved value\n"
"         $$ is the last argument evaluated, assign $a through $z as you go\n"
"Process Slot Arguments: \"idle\" selects idle context, \"#pid\" selects by pid\n"
"addstruct: add structure from hdrfile to size:offset table /usr/lib/crash .so\n"
"defproc: \"idle\" as procslot selects idle context; optional lwp or engine arg\n"
"dis:     -p|-sproc for physical address or address from non-default process\n"
"engine:  -b to bind crash to the engine, or unbind if no engine arg given\n"
"help:    -e for syntax, description and aliases for every supported command\n"
"hexmode: \"hexmode on\" to make other commands expect and display hexadecimal\n"
"ldt:     no procslot argument, use \"defproc\" to change proc,lwp if necessary\n"
"nm:      no args shows all symbols; new display; ds, sym, symval, ts synonyms\n"
"od:      -n for symbols and -ffieldspec for structures: \"help od\" explains\n"
"pcb:     -e for every lwp of every process (-ei addr: every int from addr up)\n"
"plocal:  -e for every engine\'s plocal structure, or engine arg to select\n"
"proc:    -f includes ps args in place of the command name shown by default\n"
"queue:   -ss for all read sides, -sss for all both sides, -s address allowed\n"
"resmgr:  \"resmgr [modname...]\" shows common fields, \"resmgr -f\" shows full\n"
"search:  \"abcd\" allowed, address and length optional, skips holes, new display\n"
"size:    -f for field offsets also, no -x now this always shows hexadecimal\n"
"stack, trace: -e for every lwp of every process, -t trysp if default sp fails\n"
"user:    -e for every lwp of every process, shows ubp u_procp u_lwpp together.\n";
