#ident	"@(#)localedef:main.c	1.1"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <pfmt.h>
#include <stdarg.h>
#include <errno.h>
#include "_colldata.h"
#include "_localedef.h"


FILE *curFILE;
unsigned char *curfile = (unsigned char *) "";
unsigned long curline = 0;
unsigned int curindex;
mboolean_t softdefn;


unsigned char comment_char;
unsigned char escape_char;

/* keep tack of target directory for locale output */
static unsigned char *dirname;

static mboolean_t ignore_warn = FALSE;	/* ignore warnings and output anyway */
static mboolean_t no_out = FALSE;		/* do not creat output regardless */
static unsigned char err = 0;


char extra_char_warn[] = ":57:Extra characters at end of otherwise valid line\n";

void
diag(int code, mboolean_t file_info, const char* format, ...)
{
	va_list ap;
	int pcode;
	extern int been_here[];

	va_start(ap,format);
	err |= code;
	if(code & WARN) {
		if(!ignore_warn) {
			no_out = TRUE;	/* no more output */
			been_here[curindex]++;
		}
		pcode = MM_WARNING;
	} else {
		pcode = MM_ERROR;
		been_here[curindex]++;
		no_out = TRUE;	/* no more output */
	}
	if(file_info) {
		pfmt(stderr,pcode,":122:%s: %d: ",curfile,curline);
		vpfmt(stderr,MM_NOSTD,format,ap);
	}
	else
		vpfmt(stderr,pcode,format,ap);
	if(code & FATAL)
		ld_exit();
}

void
ld_exit()
{
	if(err & LIMITS)
		exit(2);		/* impl limits exceeded or codeset error */
	if(err & ERROR)
		exit(4);	/* warnings or errors; no locale created */
	if(err & WARN) {
		if(ignore_warn)
			exit(1);	/* warnings but locale created successfully */
		else
			exit(4);	/* warnings or errors; no locale created */
	}
	exit(0);
}


void
dirmake(unsigned char *dirname)
{
	struct stat buf;

	if(stat((char *)dirname,&buf) != 0) {
		if(mkdir((char *)dirname,S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) != 0)
			diag(FATAL,FALSE,":7:Cannot create directory %s: %s\n",dirname,strerror(errno));
	}
	else  {
		if((buf.st_mode & S_IFMT) != S_IFDIR) {
			diag(FATAL,FALSE, ":123:%s is not a directory\n",dirname);
		}
	}
}

void
process(unsigned char *fname, struct kf_hdr* kfh, mboolean_t sftdefn)
{

	extern void parse(void);

	if(fname != NULL) {
		if((curFILE = fopen((char *) fname, "r")) == NULL) {
			diag(FATAL,FALSE,":11:Cannot open file %s: %s\n",
					fname, strerror(errno));
		}
		curfile = fname;
	}
	else {
		curFILE = stdin;
		curfile = (unsigned char *) "<stdin>";
	}
	comment_char = '#';		/* default comment character */
	escape_char = '\\';		/* default escape character */
	curline = 0;
	cur_hdr = kfh;
	curindex = LC_ALL;
	softdefn = sftdefn;	
	parse();
	(void) fclose(curFILE);

}
char publocales[] = "/usr/lib/locale";

int 
main(int argc, char **argv)
{
	unsigned char *defcharmap = (unsigned char *) "/usr/lib/localedef/charmaps/ascii";
	unsigned char *defctype = (unsigned char *) "/usr/lib/localedef/defctype";
	unsigned char *locale_src = NULL;
	int c;
	mboolean_t badopt = FALSE;		/* bad options seen */
	unsigned char *test = NULL;
	extern void linkcodes(void);

	(void) setlocale(LC_ALL,"");
	(void) setlocale(LC_CTYPE,"C");	/* since all input in portable char set */
	(void) setcat("localedef");
	(void) setlabel("UX:localedef");

	while((c = getopt(argc, argv, "l:p:t:f:i:cn")) != EOF) {

		switch(c) {
			case 'p':		/* non-std option */
				defcharmap = (unsigned char *) optarg;
				break;
			case 'l':		/* non-std option */
				defctype = (unsigned char *) optarg;
				break;
			case 't':		/* non-std option */
				test = (unsigned char *) optarg;
				break;
			case 'c': 
				ignore_warn = TRUE;
				break;
			case 'i':
				locale_src = (unsigned char *) optarg;
				break;
			case 'f':
				/* process user supplied charmap file */
				process((unsigned char *)optarg,&kfh_charmap,FALSE);
				break;
			case 'n':						/* non-std option */
				no_out = TRUE;
				break;
			default:
				badopt = TRUE;
				break;
		}

	}

	if(badopt || (optind != argc - 1 && !no_out)) {
		diag(FATAL,FALSE,
		 ":149:Usage: localedef [-c] [-f charmap] [-i sourcefile] [-n | localename]\n");
	}

	if(!no_out) {
		struct stat buf;

		if(strrchr(argv[optind],'/') == NULL) {
			dirname = getmem(NULL,strlen(publocales) + strlen(argv[optind])+2,FALSE,FALSE);
			sprintf((char *)dirname,"%s/%s",publocales,argv[optind]);
		}
		else dirname = (unsigned char *) argv[optind];
		
		dirmake(dirname);
	}

	/* process default charmap - definitions in user given file will override
		duplicate ones in default - softdefn=TRUE */
	process(defcharmap,&kfh_charmap,TRUE);

	linkcodes();	/* connect codepoints in order */

	/* output a file with all of the valid characters for this locale -
	   useful for testing
	*/
	   
	if(test != NULL) {
		struct codent *tcode;
		int n;
		unsigned char buf[MB_LEN_MAX];

		if((curFILE = fopen((char *) test, "w")) == NULL) {
			diag(FATAL,FALSE,":13:Cannot open test file %s: %s\n",
					test, strerror(errno));
		}
		tcode = strt_codent.cd_codelink;
		while(tcode != NULL) {
			if((n= mywctomb(buf,tcode->cd_code)) < 0) {
				diag(FATAL, FALSE,":3:Bad wide character 0x%x\n",tcode->cd_code);
			}
			if(fwrite(buf,sizeof(unsigned char),n,curFILE) != n) {
				diag(FATAL,FALSE,":4:Bad write to %s: %s\n",test,strerror(errno));
			}
			tcode = tcode->cd_codelink;
		}
		fclose(curFILE);
	}

	/* Process default ctype file according to rules of std
	   - char class definitions supplement user given information
	   - char conv (e.g., toupper) only take effect if no such user info 
	*/
	process(defctype,&kfh_localesrc,TRUE);

	/* take care of user supplied locale definition file */
	process(locale_src,&kfh_localesrc,FALSE);
	ld_exit();
		
}

/* generic output routine called from generic END function -
   calls category specific function to write the output
*/
void
output(unsigned char *fname, int (*func)())
{
	FILE *file;
	unsigned char *newname;


	if(no_out)
		newname = (unsigned char *) "/dev/null";
	else {
		newname = getmem(NULL,strlen((char *)fname)+strlen((char *)dirname)+2,FALSE,FALSE);
		sprintf((char *)newname,"%s/%s",dirname,fname);
		fname = newname + strlen((char *) dirname)+1;
		/* practically this loop is only executed once for the LC_MESSAGES
		   category whose output file name is LC_MESSAGES/Xopen_info */
		while((fname = (unsigned char *) strchr((char *)fname,'/')) != NULL) {
			*fname = '\0';
			dirmake(newname);
			*fname++ = '/';
		}
	}

	if((file = fopen((char *) newname,"w")) == NULL) {
		diag(FATAL,FALSE,":12:Cannot open output file %s: %s\n",newname,strerror(errno));
	}
	(*func)(file);
	if(fclose(file) != 0) {
		diag(ERROR,FALSE,":49:Errors in writing output file %s: %s\n",newname,strerror(errno));
	}
	if(!no_out)
		(void) free(newname);
}

unsigned char *
getline()
{
	static unsigned char *line = NULL;
	static int lsize;
	int save_dist;
	unsigned char *tline;
	unsigned char *end, *tline2;

	if(line == NULL) {
		line = (unsigned char *) getmem(NULL,LINE_MAX,FALSE,TRUE);
		lsize = LINE_MAX;
	}

	tline = line;

	for(;;) {
		if(fgets((char *)tline, LINE_MAX, curFILE) == NULL) {
			if(feof(curFILE)) {
				if(tline != line) {
					diag(WARN,TRUE,":63:Incomplete line at EOF\n");
					return(line);
				}
				return(NULL);
			}
			diag(FATAL,TRUE,":135:Unable to get next line in file: %s\n", strerror(errno));
		}
		curline++;

		/* assumes that newlines cannot be continued or be a continuation */
		if(*tline == comment_char)
			continue;

		if((end = (unsigned char *) strchr((const char *)tline,'\n')) != NULL) {
			*end = '\0';

			/* skip empty lines */
			if(tline == line && end == skipspace(tline))
				continue;

			tline2 = end;
			while(*--tline2 == escape_char);
		
			if((end - tline2) % 2 != 0)
						return(line);
			end--;
			*end = '\0';
		}
		else
			/* handles embedded NULLS by writing over line after embedded NULL */
			end = tline + strlen((char *)tline); 
		save_dist = end - line;
		if(lsize - save_dist < LINE_MAX) {
				lsize +=LINE_MAX;
				line = (unsigned char *) getmem(line, lsize,FALSE,TRUE);
		}
		tline = line + save_dist;
	}
}

void *
getmem(void *old, size_t size, mboolean_t zero, mboolean_t file_info)
{
	void * new;
	if((new = realloc(old,size)) == NULL) {
		diag(FATAL_LIMITS,file_info,":116:Out of Memory\n");
	}
	return(zero ? memset(new,0,size) : new);
}
