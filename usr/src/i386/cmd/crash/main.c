#ident	"@(#)crash:i386/cmd/crash/main.c	1.10.5.3"

#include <sys/param.h>
#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/user.h>
#include <setjmp.h>
#include <locale.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/vmparam.h>
#include "crash.h"

char *namelist = "/stand/unix";
char *dumpfile = "/dev/mem";
char *lmodpath = (char *)NULL;

void makefunctab(void);
void add_shlib(char *libname, struct func **ftab, int *ftabsize);
extern struct offstable *loc_offstab[];

extern FILE *fp;	/* output file pointer */
extern FILE *rp;	/* redirect file pointer */
FILE *bp;	/* batch file pointer */

extern jmp_buf jmp;	/* jump on error or signal */
       jmp_buf syn;	/* jump on command syntax error */

/* allow the directory /usr/lib/crash and the CRASH_LIBS environment
   variable to be a list of shared libraries containing additional commands.
   Load the libraries and include their functions into the command table.
*/
char lib_dir[] = "/usr/lib/crash";

#ifdef LD_WEAK_REFERENCE_BUG
int No_PSM_version_defined = 1;
#endif

/* function calls */
extern int getas(),getbufhdr(),getbuffer(),getdis(),
	getinode(),getlcks(),getpage(),
	getmap(),getvfs(),getnm(),getod(),getpcb(),
	getproc(),getqrun(),getqueue(),getquit(),getptbl(),
	getsnode(),getstack(),getstat(),getstream(),getstrstat(),
	gettrace(),gettty(),getuser(),getvar(),getvnode(),getvtop(),
	getfuncs(),getbase(),gethelp(),getsearch(),getdeflwp(),
	getsearch(),getfile(),getdefproc(),getredirect(),
	getresmgr(),getsize(),getvfssw(),getlinkblk(),
	getpty(),getclass(),gettsdptbl(),get_vxfs_inode(),
	getdispq(),gettslwp(),get_sfs_inode(),gettest(),
	getfpriv(),getabuf(),getlidcache(),getplocal(),
	getcglocal(),
	getaddstruct(),gethexmode(),
	/*
	 * i386 specifics are following
	 */
	getldt(),getgdt(),getidt(),getpanic(),getengine(),getcg();

#ifdef NOTDEF
extern int getcallout(),gethrt(),getkmastat(),get_cdfs_inode(),
	getprnode(),getrtdptbl(),getrtproc();
#endif

/* function table */
struct func *functab;
struct func loc_functab[] = {
	"abuf","[-wlogfile] [-b|-c|-d|-o|-x]",
		getabuf,"audit buffer data",
	"addstruct","[-wfilename.so] structname [dephdrfile...] hdrfile",
		getaddstruct,"add structure to size and offset table",
	"as","[-wfilename] [-f] [-e | proc[-last]|#pid|procaddr...]",
		getas,"address space structures",
	"b"," ",
		getbuffer,"(buffer)",
	"base","[-wfilename] expression...",
		getbase,"numeric base conversions",
	"buf"," ",
		getbufhdr,"(bufhdr)",
	"buffer","[-wfilename] [-x|-d|-o|-c|-b|-i] hdraddr",
		getbuffer,"buffer data",
	"bufhdr","[-wfilename] [-f] [hdraddr...]",
		getbufhdr,"buffer headers",
#ifdef NOTDEF
	"c"," ",
		getcallout,"(callout)",
	"callout","[-wfilename]",
		getcallout,"callout table",
#endif
	"cg","[-wfilename] [-b] [cg]",
		getcg,"display or change default context cg",
	"cglocal","[-wfilename] [-e | eng]",
		getcglocal,"cpu group local data",
	"class","[-wfilename] [clslot[-last]...]",
		getclass,"class table",
	"deflwp", "[-wfilename] [-c | lwp | idleng]",
		getdeflwp,"display or change default context lwp",
	"defproc","[-wfilename] [-c | proc|#pid|idle [lwp|idleng]]",
		getdefproc,"display or change default context process",
	"dis","[-wfilename] [-a] (-c | [-p|-sproc] address) [count]",
		getdis,"disassembler",
	"dispq","[-wfilename] [-g] [-l] [priority[-last]...]",
		getdispq, "dispatch queues",
	"ds"," ",
		getnm,"(nm)",
	"engine","[-wfilename] [-b] [eng]",
		getengine,"display or change default context engine",
	"f"," ",
		getfile,"(file)",
	"file","[-wfilename] [-f] [fileaddr...]",
		getfile,"file table",
	"filepriv","[-wfilename] [-n]",
		getfpriv,"privilege table",
	"fprv"," ",
		getfpriv, "(filepriv)",
	"fs"," ",
		getvfssw,"(vfssw)",
	"gdt","[-wfilename] [-e] [gdtslot [count]]",
		getgdt,"global descriptor table",
	"help","[-wfilename] [-e | command...]",
		gethelp,"command syntax, description and aliases",
	"hexmode","[-wfilename] [off|on]",
		gethexmode,"avoid decimal and octal",
#ifdef NOTDEF
	"hrt","[-wfilename]",
		gethrt,"high resolution timers",
#endif
	"i"," ",
		getinode,"(inode)",
	"idt","[-wfilename] [-e] [idtslot [count]]",
		getidt,"interrupt descriptor table",
	"inode","[-wfilename] [-e] [-f] [-l] [-r] [slot[-last]|inodeaddr...]",
		getinode,"s5 inode table",
#ifdef NOTDEF
	"kmastat","[-wfilename]",
		getkmastat,"kernel memory allocator statistics",
#endif
	"lck","[-wfilename] [filockaddr...]",
		getlcks,"record lock tables",
	"ldt","[-wfilename] [-e] [ldtslot [count]]",
		getldt,"local descriptor table",
	"lidcache","[-wfilename]",
		getlidcache,"lid cache",
	"linkblk","[-wfilename] [linkinfoaddr...]",
		getlinkblk,"linkblk table",
	"m"," ",
		getvfs,"(vfs)",
	"map","[-wfilename] mapaddr...",
		getmap,"resource allocation maps",
	"mount"," ",
		getvfs,"(vfs)",
	"nm","[-wfilename] [address...]",
		getnm,"symbol address translation",
	"od","[-wf] [-ffieldspec] [-x|-d|-o|-c|-a] [-l|-h|-n|-t|-b] [-p|-sproc] addr [cnt]",
		getod,"display values",
	"p"," ",
		getproc,"(proc)",
	"page","[-wfilename] [-e] [pfn[-last]|pageaddr...]",
		getpage,"page structure",
	"panic","[-wfilename]",
		getpanic,"system messages, panic registers and trace",
	"pcb","[-wfilename] [-e] ([-k|-u] [proc|#pid|idle [lwp|eng]] | [-i trysp])",
		getpcb,"process control block (saved registers)",
#ifdef NOTDEF
	"pcinode", "[-wfilename] [-e] [-f] [-l] [-r] [slot[-last]|cdfs_inodeaddr...]",
		get_cdfs_inode,"cdfs inode table",
#endif
	"plocal","[-wfilename] [-e | eng]",
		getplocal,"processor local data",
#ifdef NOTDEF
	"prnode","[-wfilename] [-f] [proc[-last]|#pid|prnodeaddr...]",
		getprnode,"proc node",
#endif
	"proc","[-wfilename] [-f[ -n]] [proc[-last]|#pid|procaddr...]",
		getproc,"process table",
	"ps"," ",
		getproc,"(proc)",
	"ptbl","[-wfilename] ( [-e] | [-sproc] | [-p] ptbladdr [count] )",
		getptbl,"page tables",
	"pty","[-wfilename] [-e] [-f] [-l] [-h] [-s] [-ttype] [slot[-last]|pt_ttysaddr...]",
		getpty,"pty structure",
	"q"," ",
		getquit,"(quit)",
	"qrun","[-wfilename]",
		getqrun,"servicable streams queues",
	"queue","[-wfilename] [-f|-s[s[s]]] [queueaddr...]",
		getqueue,"allocated streams queues",
	"quit"," ",
		getquit,"exit",
	"rd"," ",
		getod,"(od)",
	"redirect","[-wfilename] [-c | filename]",
		getredirect,"output redirection",
	"resmgr","[-wfilename] [-f] [modname...]",
		getresmgr,"resource manager database",
#ifdef NOTDEF
	"rtdptbl","[-wfilename] [priority[-last]...]",
		getrtdptbl, "real time dispatcher parameter table",
	"rtproc","[-wfilename]",
		getrtproc,"real time process table",
#endif
	"s"," ",
		getstack,"(stack)",
	"search","[-wfilename] pattern [-mmask] [-p|-sproc] [address [length]]",
		getsearch,"memory search",
	"si"," ",
		get_sfs_inode,"(sinode)",
	"sinode","[-wfilename] [-e] [-f] [-l] [-r] [slot[-last]|inodeaddr...]",
		get_sfs_inode,"sfs/ufs inode table",
	"size","[-wfilename] [-f] structname...",
		getsize,"structure size and field offsets",
	"snode","[-wfilename] [-f] [slot[-last]|snodeaddr...]",
		getsnode,"special node",
	"stack","[-wfilename] [-ttrysp] [-e | proc|#pid|idle [lwp|idleng]]",
		getstack,"kernel stack values",
	"stat","[-wfilename]",
		getstat,"system information",
	"stream","[-wfilename] [-f] [shinfoaddr...]",
		getstream,"allocated streams table slots",
	"strstat","[-wfilename]",
		getstrstat,"streams statistics",
	"symval"," ",
		getnm,"(nm)",
	"t"," ",
		gettrace,"(trace)",
	"test","[-wfilename] [debuglevel]",
		gettest, "display or change debuglevel",
	"trace","[-wfilename] [-ttrysp] [-e | proc|#pid|idle [lwp|idleng]]",
		gettrace,"kernel stack trace",
	"ts"," ",
		getnm,"(nm)",
	"tsdptbl","[-wfilename] [priority[-last]...]",
		gettsdptbl, "time sharing dispatcher parameter table",
	"tslwp","[-wfilename]",
		gettslwp,"time sharing hash table",
	"tsproc"," ",
		gettslwp,"(tslwp)",
	"tty","[-wfilename] [-e] [-f] [-l] [-ttype] [slot[-last]|strttyaddr...]",
		gettty,"streams tty structure",
	"u"," ",
		getuser,"(user)",
	"user","[-wfilename] [-e | proc|#pid|idle [lwp|idleng]]",
		getuser,"uarea",
	"v"," ",
		getvar,"(var)",
	"var","[-wfilename]",
		getvar,"tunable system variables",
	"vfs","[-wfilename] [vfsaddr...]",
		getvfs,"mounted vfs list",
	"vfssw","[-wfilename] [slot[-last]|swaddr...]",
		getvfssw,"virtual file system switch table",
	"vi"," ",
		get_vxfs_inode,"(vxinode)",
	"vinode"," ",
		get_vxfs_inode,"(vxinode)",
	"vnode","[-wfilename] vnodeaddr...",
		getvnode,"vnode structure",
	"vtop","[-wfilename] [-sproc] address...",
		getvtop,"virtual to physical address",
	"vxinode","[-wfilename] [-e] [-f] [-l] [-r] [vx_inodeaddr...]",
		get_vxfs_inode,"vxfs inode table",
	"?","[-wfilename]",
		getfuncs,"list available commands",
	"!cmd"," ",
		NULL,"escape to shell",
	NULL,NULL,NULL,NULL
};

char *args[NARGS];		/* argument array */
int argcnt;			/* argument count */
char *outfile;			/* output file for redirection */
char *batchfile;		/* batch file */
int tabsize;			/* size of function table */

/* main program with call to functions */
main(argc,argv)
int argc;
char **argv;
{
	char line[] = "\n================================================================================\n";
	struct func *a,*f;
	int c,i,found;
	int arglength;
	char *options = "D:d:n:w:m:b:";
	char usage[] = "usage: crash [-d dumpfile] [-n namelist] [-m modpath] [-b batchfile]\n";

#ifndef KVBASE_IS_VARIABLE
	kvbase = KVBASE;
#endif

	bp = NULL;
	(void)setlocale(LC_ALL, "");
	if (setjmp(jmp))
		exit(1);
	fp = stdout;
	outfile = "stdout";

	while((c = getopt(argc,argv,options)) !=EOF) {
		switch(c) {
			case 'D' : 	debugmode = atoi(optarg);
					setbuf(stdout,NULL);
					setbuf(stderr,NULL);
					break;

			case 'd' :	dumpfile = optarg;
				 	break;

			case 'n' : 	namelist = optarg; 
					break;

			case 'm' :	lmodpath = optarg;
					break;
			
			case 'w' : 	outfile = optarg;
					if (!(rp = fopen(outfile,"a")))
						fatal("cannot open outfile %s\n",
							outfile);
					break;

			case 'b' : 	batchfile = optarg;
					if (!(bp = fopen(batchfile,"r")))
						fatal("cannot open batchfile %s\n",
							batchfile);
					break;

			default  :	fatal(usage); 
					break;
		}
	}

	if (rp)
		fp = rp;
	/* not error message, but traditionally goes to both rp and stdout */
	prerrmes("dumpfile = %s, namelist = %s, outfile = %s\n",
			dumpfile, namelist, outfile);

	addoffstab(loc_offstab);
	makefunctab();

	(void)signal(SIGSEGV, sigint);	/* don't dump core */
	(void)signal(SIGSYS, sigint);	/* even on OSr5 */

	cr_open(O_RDONLY|O_LARGEFILE, dumpfile, namelist, lmodpath);
	adjustoffstab();

	/* set break signal handling */
	(void)signal(SIGINT, intsig = sigint);

	pr_context();	/* show Cur_eng Nengine Cur_proc Cur_lwp */
	setjmp(jmp);

	for (;;) {
		cr_free(ALLTMPBUFS);
		getcmd();
		if(argcnt == 0)
			continue;
		if (rp) {
			fp = rp;
			if(bp){
				fprintf(fp,line);
				fprintf(fp,"\t\t\t OUTPUT OF:");
			} else 
				fprintf(fp,"\n> ");
			for (i = 0;i<argcnt;i++)
				fprintf(fp,"%s ",args[i]);
			if(bp)
				fprintf(fp,line);
			else
				fprintf(fp,"\n");
		}
		found = 0;
		for (f = functab; f->name; f++) {
			if(!strcmp(f->name,args[0])) {
				found = 1;
				break;
			}
		}
		if (!found) {
			arglength = strlen(args[0]);
			for (f = functab; f->name; f++) {
				if (found && f->call == a->call)
					continue;
				if (!strncmp(f->name,args[0],arglength)) {
					found++;
					a = f;
				}
			}
			f = a;
		}
		if (found != 1) {
			error("%s is an %s command name\n",args[0],
				found? "ambiguous": "unrecognized");
		}
		if (setjmp(syn)) {
			opterr = 0;
			while(getopt(argcnt,args,"") != EOF);
			opterr = 1;
			a = f;
			if (*f->syntax == ' ') {
				for (a = functab;a->name;a++)
					if((a->call == f->call)
					&& (*a->syntax != ' '))
						break;
				if (a->name == NULL)
					a = f;
			}
			error("usage: %s %s\n",f->name,a->syntax);
		}
		(*(f->call))();
		fflush(fp);
		resetfp();
	}
}

void
add_shlib(char *libname, struct func **ftab, int *ftabsize )
{
	struct func *lftab, *nftab;
	struct offstable **offstab;
	void *dl_handle;
	int ltabsize;
	int diff;

	dl_handle = dlopen(libname, RTLD_LAZY );
	if (!dl_handle) {
		prerrmes("library %s: %s\n", libname, dlerror());
		return;
	}
	lftab = dlsym( dl_handle, "functab" );
	offstab = dlsym( dl_handle, "offstab" );
	if (!lftab && !offstab) {
		dlclose(dl_handle);
		prerrmes("library %s has neither function nor offset table\n",
			libname );
		return;
	}
	if (offstab)
		addoffstab(offstab);
	if (!lftab || !ftab)
		return;
	/* count number of commands in new table */
	for (ltabsize = 0; lftab[ltabsize].name; ltabsize++)
		;
	nftab = realloc(*ftab, (*ftabsize + ltabsize) * sizeof (struct func));
	if (!nftab) {
		prerrmes("cannot extend function table\n");
		return;
	}
	*ftab = nftab;
	/* insert new commands into function table. If names clash,
	 * old entry is overwritten both lists are assumed to be sorted.
	 */
	for (; lftab->name; ltabsize--, lftab++) {
		while (nftab->name && nftab->name[0] != '?') {
			diff = strcmp( lftab->name, nftab->name);
			if (diff <= 0)
				break;
			nftab++;
		}
		if (diff) {
			memmove(nftab+1, nftab,
				(*ftabsize-(nftab-*ftab)) * sizeof *nftab);
			(*ftabsize)++;
		}
		memcpy(nftab, lftab, sizeof *lftab);
	}
}


static void
makefunctab()
{
	char *liblist;
	struct func *ftab;
	char *libname;
	int ftabsize;
	DIR *usr_lib_crash = NULL;
	struct dirent *dirent;

	ftab = malloc(sizeof loc_functab);
	ftabsize = sizeof loc_functab / sizeof (struct func);
	if (!ftab) {
		prerrmes("cannot extend function table\n");
		ftab = loc_functab;
	} else {
		memcpy(ftab, &loc_functab, sizeof(loc_functab));
		usr_lib_crash = opendir(lib_dir);

		if (usr_lib_crash) {
			while (dirent = readdir(usr_lib_crash)) {
				int name_len = strlen(dirent->d_name);
				/* only process entries with a .so suffix */
				if (name_len < 3 || 
				    strcmp(dirent->d_name + name_len - 3, 
					   ".so"))
					continue;
				libname = malloc(strlen(lib_dir) + 1 + 
						name_len + 1);
				if (!libname)
					continue;
				strcpy(libname, lib_dir);
				strcat(libname, "/" );
				strcat(libname, dirent->d_name );
				add_shlib(libname, &ftab, &ftabsize);
				free(libname);
			}
			closedir( usr_lib_crash );
		}

		/* add libraries in "CRASH_LIBS" environment variable */
		if ((liblist = getenv("CRASH_LIBS")) != NULL)
			if ((liblist = strdup(liblist)) == NULL)
				prerrmes("cannot extend function table\n");
		if (liblist) {
			for (libname = strtok(liblist, ":"); libname;
						libname = strtok( 0, ":" ))
				add_shlib( libname, &ftab, &ftabsize );
			free(liblist);
		}
	}

	functab = ftab;
	tabsize = ftabsize - 1;
}
