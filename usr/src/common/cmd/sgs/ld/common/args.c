#ident	"@(#)ld:common/args.c	1.46"

/*
** Module args
** process command line options
*/

/****************************************
** imports
****************************************/

#include	<stdio.h>
#ifdef	__STDC__
#include	<unistd.h>
#include	<stdlib.h>
#endif	/* __STDC__ */
#include	<fcntl.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<pfmt.h>
#include	"sgs.h"
#include	"paths.h"
#include	"globals.h"

extern char*
	optarg;
extern int
	optind,
	opterr;

#ifndef	__STDC__
int	getopt();
void	exit();
#endif


/****************************************
** local constants
****************************************/

/*  getopt(3) command line options */
#ifndef	DEBUG
#define	OPTIONS	"abd:e:f:h:I:l:mno:rstu:xz:B:GL:M:Q:T:VY:"
#else
#define	OPTIONS	"abd:e:f:h:I:l:mno:rstu:xz:B:D:GL:M:Q:T:VY:"
#endif

/* constants used by the usage_mess() function */
#define FULL_USAGE	0
#define SUMM_USAGE	1


/****************************************
** local variables
****************************************/

static Setstate	dflag = NOT_SET;		/* Flag saver for dmode */

static Boolean	Vflag = FALSE;
static Boolean	fileopts = FALSE;
char*	Mflag;
static Boolean	zseen = FALSE;
static Boolean  YPflag = FALSE;


#ifdef DEBUG
static struct arg_code {
	char* dbg_arg;		/* Command line argument name */
	Dbgset dbg_code;	/* Internal code for this debugging level */
} debug_opts[] = {
	{ "args", DBG_ARGS },
	{ "files", DBG_FILES },
	{ "globals", DBG_GLOBALS },
	{ "libs", DBG_LIBS },
	{ "main", DBG_MAIN },
	{ "map", DBG_MAP },
	{ "outfile", DBG_OUTFILE },
	{ "reloc", DBG_RELOC },
	{ "sections", DBG_SECTIONS },
	{ "syms", DBG_SYMS },
	{ "update", DBG_UPDATE },
	{ "util", DBG_UTIL },
	{ NULL, DBG_MAX }
};
#endif

/****************************************
** local function declarations
****************************************/

LPROTO(void usage_mess, (int));

#ifdef DEBUG
LPROTO(void set_debug, (char*));
#endif

/****************************************
** local function definitions
****************************************/

#ifdef	DEBUG
/*
 * Turn on debugging flags from the -D option
 * Flags are of the form one,two,three, ...
 */

static void
set_debug(which)
	char* which;		/* Which debugging flags should be set */
{
	char* dflags;		/* Temporary to hold a copy of "which" */
	struct arg_code* acp;	/* Pointer to cycle through debug_opts array */

	dflags = (char*) mymalloc(strlen(which) + 1);
	(void) strcpy(dflags, which);

	/*
	 * If the option is -Dall then turn on all flags
	 */
	if (strncmp(dflags, "all", 3) == SAME) {
		int i;

		for (i = 0; i < DBG_MAX; i++)
			debug_bits[i] = TRUE;
		return;
	}

	/*
	 * Otherwise the option should be of the form -Dopt,opt,opt, ...
	 * Separate the pieces and turn on the appropriate flags
	 */
	if ((dflags = strtok(dflags, ",")) != NULL) {
		do {
			for (acp = debug_opts; acp->dbg_arg != NULL; acp++) {
				if (strcmp(dflags, acp->dbg_arg) == SAME) {
					lderror(MSG_DEBUG, "setting debug option %s",
						acp->dbg_arg);
					debug_bits[acp->dbg_code] = TRUE;
					break;
				}
			}
		} while ((dflags = strtok(NULL, ",")) != NULL);
	}
}
#endif	/* DEBUG */

/*
** build_sym_list(char *input)
** build list of symbols that should be handled like -Bsymbolic
** allocate array of char pointers in BLIST_SIZE chunks and realloc if 
** need be; last entry is null
*/

#define BLIST_SIZE	100
char **
build_sym_list(input)
char *input;
{
	int	count = 1;
	int	size = BLIST_SIZE;
	char	*ptr;
	char	**sym_list;

	sym_list = (char **)mymalloc(BLIST_SIZE*sizeof(char *));
	ptr = strtok(input, " ,\t");
	sym_list[0] = (char *)mymalloc(strlen(ptr) + 1);
	strcpy(sym_list[0], ptr);
	while((ptr = strtok(NULL, " ,\t")) != NULL)
	{
		if (count >= (size - 2))
		{
			/* room for last null entry */
			size += BLIST_SIZE;
			sym_list = (char **)myrealloc(sym_list, size*sizeof(char *));
		}
		sym_list[count] = (char *)mymalloc(strlen(ptr) + 1);
		strcpy(sym_list[count], ptr);
		count++;
	}
	sym_list[count] = NULL;
	return sym_list;
}

/*
** process_text_file(char *text_file, int tfiledes, 
** 	int allow_whitespace, int exit_on_format_error, int &num)
** Process the file line by line looking for one symbol name or
** argument per line.
** Lines beginning with a "#" and blank lines are ignored.  If the first 
** four bytes of the file are printable ASCII characters, the file is 
** assumed to be a text file.  Allocate an array of char pointers in 
** BLIST_SIZE chunks and realloc if need be; last entry is null. This 
** routine can be used to allocate the array for -Bsymbolic, -Bhide and 
** -Bexport or to process an argument file.
*/

static char **
process_text_file(text_file, tfiledes, allow_whitespace, 
	exit_on_format_error, num_found)
char *text_file;
int tfiledes;
int allow_whitespace;
int exit_on_format_error;
int *num_found;
{
	char 		whites[] = ",\n\t\b \f\v"; 
	struct stat 	stat_buf;
	FILE 		*fptr;
	char 		*tbuf;	
        char 		*name;
	char		**name_list;
	int 		linenum;
	int		nent = BLIST_SIZE;
	int		bufsiz = BUFSIZ;
	int		count = 0;

	if (tfiledes == -1)
		fptr = fopen(text_file, "r");
	else {
		fptr = fdopen(tfiledes, "r");
		rewind(fptr);
	}

	if (fptr == NULL)
		lderror(MSG_FATAL, "cannot open file \"%s\"", text_file);
	if (fstat(fileno(fptr), &stat_buf) == -1)
		lderror(MSG_FATAL, "cannot stat(2) file \"%s\"", text_file);
	if ((stat_buf.st_mode & S_IFMT) != S_IFREG)
		lderror(MSG_FATAL, "file \"%s\" must be an ordinary file", text_file);
 
	tbuf = mymalloc(bufsiz);
	name_list = mymalloc(nent*sizeof(char *));

	for(linenum = 1; ; linenum++)
	{
		tbuf[bufsiz-2] = '\n';

		if (fgets(tbuf, bufsiz, fptr) == NULL)
			break;

		/* while still more characters on the current line */ 
		while (tbuf[bufsiz-2] != '\n')
		{
			int	lastcharpos;
			lastcharpos = bufsiz - 1;
			bufsiz += BUFSIZ;
			tbuf = myrealloc(tbuf, bufsiz);
                	tbuf[bufsiz-2] = '\n';
			if (fgets(&tbuf[lastcharpos],bufsiz-lastcharpos,fptr) == NULL)
				goto out;
		}

		/* Make sure the first four characters are printable ASCII */
		if (linenum == 1)
		{
			int i = 0;
			while (tbuf[i] != '\n' && tbuf[i] != '\0' 
				&& i <= 3 ) 
			{
				if (!isascii(tbuf[i]) || 
					(!isprint(tbuf[i]) && 
					!isspace(tbuf[i])))
				{
					if (exit_on_format_error)
						lderror(MSG_FATAL,
						"%s: line %d: illegal character in a text file", text_file, linenum);
					else
					{
						free(tbuf);
						free(name_list);
						return 0;
					}
				}
				i++;
			}
		}

		if (tbuf[0] == '#') /* Comment on line, skip it */
                        continue;

		if (!allow_whitespace)
		{
                	name = strtok(tbuf, whites);
                
			if (name == NULL)    
				/* No symbols on line, skip it */
				continue;
	                if ((strtok(NULL, whites)) != NULL) {
				lderror(MSG_FATAL,"%s: line %d contains more than one symbol name", text_file, linenum);
                	}
		}
		else
		{
			char	*nmend;
			name = tbuf;
			if ((nmend = strchr(tbuf, '\n')) != 0)
				*nmend = 0;
			if (!*name)
				continue;
		}

		if (count >= (nent - 2))
		{
			/* room for last null entry */
			nent += BLIST_SIZE;
			name_list = myrealloc(name_list, nent*sizeof(char *));
		}
		name_list[count] = mymalloc(strlen(name) + 1);
		strcpy(name_list[count++], name);
	}
out:
	name_list[count] = NULL;
	free(tbuf);
	if (num_found)
		*num_found = count;
	return name_list;
}

/****************************************
** global function definitions
****************************************/

/*
** free_list(char **list)
** free array of char *s created by process_text_file or
** build_sym_list
*/

void
free_list(char **list)
{
	char	**ptr;
	for(ptr = list; *ptr; ptr++)
		free(*ptr);
	free(list);
}

/*
** check_flags(int argc)
** checks the command line option flags for consistency.
*/
void
check_flags(argc)
	int	argc;		/* the usual passed from main */
{
	if(YPflag && (libdir != NULL || llibdir != NULL))
		lderror(MSG_FATAL,
			gettxt(":1019","-YP and -Y%c may not be specified concurrently"),
			      libdir ? 'L' : 'U');
	if (rflag && dmode) {
		if (dflag == SET_TRUE)
			lderror(MSG_FATAL,
			    gettxt(":1020","the -dy and -r flags are incompatible"));
		else 
			dmode = FALSE;
	}
	if (sflag || xflag) {
		strip_debug = TRUE;
	}
	

	if (dmode) {
		Bflag_dynamic = TRUE;		/* on by default in dmode -- settings will be rechecked in pfiles */
		if (aflag)
		{
			lderror(MSG_FATAL,
			    gettxt(":736","the -dy and -a flags are incompatible"));
		}
		if (bhide_list && bexport_list)
			lderror(MSG_FATAL,
			    gettxt(":1504","both the -Bhide and -Bexport flags cannot contain lists of symbols"));
			
		if (Bflag_hide == ALL && Bflag_export == ALL)
			lderror(MSG_FATAL,
			    gettxt(":1505","the -Bhide and -Bexport flags are incompatible"));

		if (!Gflag)		/* dynamically linked executable */
		{
			if (znflag)
                                zdflag = FALSE;
			else if (!zdflag)
                                zdflag = TRUE;
			if (Bflag_symbolic)
			{
                                Bflag_symbolic = FALSE;
                                lderror(MSG_WARNING,
				    gettxt(":1021","-Bsymbolic ignored when building a dynamic executable"));
			}
			if (dynoutfile_name != NULL)
			{
                                lderror(MSG_WARNING,
				    gettxt(":1022","-h ignored when building a static executable"));
                                dynoutfile_name = NULL;
			}
		} else { /* else if Gflag */
			if (znflag)
                                zdflag = FALSE;
			}
	} else /*if (!dmode)*/ {
		if (bflag){
			lderror(MSG_WARNING,
			    gettxt(":1023","the -dn and -b flags are incompatible; ignoring -b"));
			bflag = FALSE;
		}
		if (dynoutfile_name != NULL){
			lderror(MSG_WARNING,
			    gettxt(":1024","-dn and -h are incompatible; ignoring -h"));
			dynoutfile_name = NULL;
		}
		if (!zdflag && !rflag)
			zdflag = TRUE;
		if (ztflag){
			lderror(MSG_WARNING,
			    gettxt(":1025","the -dn and -ztext flags are incompatible; ignoring -ztext"));
			ztflag = FALSE;
		}
		if (Bflag_dynamic)
			lderror(MSG_FATAL,
			    gettxt(":1026","the -dn and -Bdynamic flags are incompatible"));
		if (Bflag_export && !rflag)
			lderror(MSG_FATAL,
				gettxt(":1506","the -dn and -Bexport flags are incompatible"));
		if ((Bflag_hide || qflag_hide) && !rflag)
			lderror(MSG_FATAL,
				gettxt(":1507", "the -dn and -Bhide flags are incompatible"));
		if (Bbind_now)
			lderror(MSG_FATAL,
				gettxt(":1516", "the -dn and -Bbind_now flags are incompatible"));

		if (Gflag)
			lderror(MSG_FATAL,
			    gettxt(":1027","-dn and -G flags are incompatible"));
		if(aflag && rflag)
			lderror(MSG_FATAL,
			    gettxt(":1028","-a -r is illegal in this version of ld"));
		/* aflag should be on by default, but is off by
		 * default if rflag is on
		 */
		if (!aflag && !rflag)
			aflag = TRUE;
		if (rflag ){
			/* we can only strip the symbol table and string table
			 * if no output relocations will refer to them
			 */
			if(sflag){
				sflag = FALSE;
				lderror(MSG_WARNING,
				    gettxt(":1029","-r and -s both set; only debugging information stripped"));
			}
			if(xflag){
				xflag = FALSE;
				lderror(MSG_WARNING,
				    gettxt(":1768","-r and -x both set; only debugging information stripped"));
			}
			if(!aflag && interp_path != NULL){
				lderror(MSG_WARNING,
				    gettxt(":1030","-r and -I flags are incompatible;  -I ignored"));
				interp_path = NULL;
			}
		}
	}
	if (sflag && xflag) {
		lderror(MSG_WARNING,
		    gettxt(":1769","-s and -x both set; -s takes precedence"));
		xflag = FALSE;
	}
	if (Mflag != NULL && dmode)
		lderror(MSG_FATAL,gettxt(":1031","-M and -dy are incompatible"));
	else if (Mflag != NULL && !aflag)
		lderror(MSG_FATAL,gettxt(":1032","-M illegal when not building a static executable  file"));

	if(!fileopts){
		if(Vflag && argc == 2)
			exit(EXIT_SUCCESS);
		else
			lderror(MSG_FATAL,
				gettxt(":1033","no files on input command line"));
	}
	ecrit_setup();
	if (Mflag != NULL)
		map_parse(Mflag);
}

/*
** process_flags(int argc, char** argv)
** processes the command line flags.  Argc and argv are the usual
** stuff passed to main.
**
** Even though the ld specifications say that we should not allow
** general options to be processed after seeing the first file option
** (-Bdynamic, -Bstatic, -lx), in the interest of backward
** compatibility with old makefiles, etc., we will allow all old options
** except -t to appear anywhere in the command line.  NOTE: all new
** makefiles should follow the requirements for command line ordering
** indicated above.
**
** The options recognized by ld are the following (initialized in
** globs.h):
**
**	OPTION		MEANING
**	-a { -dn only }	make the output file executable
**				1. complain about unresolved
**				   references (zdefs on)
**				2. define several "_xxxx" symbols
**
**	-b { -dy only }	turn off special handling for PIC/non-PIC
**			  relocations
**
**	-dy		dynamic mode: build a dynamically linked
**			  executable or a shared object - build a
**			  dynamic structure in the output file and
**			  make that file's symbols available for
**			  run-time linking
**
**	-dn		static mode: build a statically linked
**			  executable or a relocatable object file
**
**	-e name		make name the new entry point in
**			  ELF_PHDR.p_entry
**
**	-f platform-specific-checks	force output target platform
**				or check for proper versions
**
**	-h name { -dy -G only }
**			make name the new output filename in the
**			  dynamic structure
**
**	-I name		make name the interpreter pathname written
**			  into the program execution header of the
**			  output file
**
**	-lx		search for the library libx.[so|a] using
**			  search directories
**
**	-m		generate a memory map
**
**	-n		included for backward compatiblility with old makefiles
**
**	-o name		use name as the output filename (default
**			  is in A_OUT in globs.c
**
**	-r { -dn only }	retain relocation in the output file -
**			  produce a relocatable object file
**
**	-s		strip the debug section and its relocations
**			  from the output file; also strip the
**			  symbol table from an a.out.
**
**	-t		turnoff warnings about multiply-defined
**			  symbols that are not the same size
**
**	-u name		make name an undefined entry in the ld symbol
**			  table
**
**	-x		strip local symbols and debug info but
**			leave global symbols
**
**	-z text { -dy only }	issue a fatal error if any text relocations remain
**		
**	-z defs		issue a fatal error if undefined symbols remain 
**		{ -dn }	forced on
**		{ -dy -G }
**			forced off
**		{ -dy !-G }
**			forced on
**
**	-z nodefs 	undefined symbols are allowable
**		  { -dn }
**			forced off
**		  { -dy -G }
**			forced on
**		  { -dy !-G }
**			forced off
**	-z weak_undefs	(undocumented) : create definitions
**			(absolute, value 0) for undefined
**			weak references - compatibility mode
**
**	-B static	in searching for libx, choose libx.a
**
**	-B dynamic { -dy only }
**			in searching for libx, choose libx.so
**
**	-B sortbss
**			allocate contiguos space to bss symbols that
**			are in the same input file
**
**	-B symbolic { -dy -G }
**			shared object symbol resolution flag ...
**
**	-B export   { -dy or -r only }
**			make global symbols visible outside of the executable...
**	-B hide	    { -dy or -r  only }
**			make defined global symbols not visible outside of the 
**			shared object. Hide the symbol from an application.
**	-Bqhide	    { -dy or -r  only } special version for the
**			implementation; like Bhide but doesn't complain
**		        about missing symbols; can be used with
**			-Bhide; ignored if using Bexport
**
**	#ifdef	DEBUG
**	-D sect,sect,...
**			turn on debugging for each indicated section
**	#endif
**
**	-G { -dy }	produce a shared object
**
**	-L path		add path to prelibdirs
**
**	-M mapfilename	read a mapfile
**
**	-Qy		add ld version to comment section of output
**			  file
**
**	-Qn		do not add ld version
**
**	-T address	change where first segment of a.out is mapped
**
** 	-V		print ld version to stderr
**
**	-YL path	change LIBDIR to path
**
**	-YU path	change LLIBDIR to path
**
**
** Pass 1 -- process_flags: collects all options and sets flags
** check_flags -- checks for flag consistency
** Pass 2 -- process_files: skips the flags collected in pass 1 and processes files
*/

void
process_flags(argc, argv)
	int	argc;		/* the usual passed in from main() */
	char**	argv;
{
	Boolean	errflag = FALSE; /* an error has been seen */
	int	c;		/* character returned by getopt */

	if (argc < 2) {
		errflag = TRUE;
		goto usage;
	}
   getmore:
	while((c = getopt(argc, argv, OPTIONS)) != -1 ){
	DPRINTF(DBG_ARGS, (MSG_DEBUG, "args: process_flags: argc=%d, optind=%d",argc,optind));
	DPRINTF(DBG_ARGS, (MSG_DEBUG, "input argument to process_flags is: %#x",c));

		switch (c) {

		case 'a':
			aflag = TRUE;
			break;

		case 'b':
			bflag = TRUE;
			break;

		case 'd':
			if (optarg[0] == 'n' && optarg[1] == '\0') {
				if (dflag == NOT_SET)
				{
					dmode = FALSE;
					dflag = SET_FALSE;
				} else
					lderror(MSG_WARNING,
					    gettxt(":1034","-d used more than once"));
			} else if (optarg[0] == 'y' && optarg[1] == '\0') {
				if (dflag == NOT_SET)
                                {
                                        dmode = TRUE;
                                        dflag = SET_TRUE;
                                } else
                                        lderror(MSG_WARNING,
                                            gettxt(":1034","-d used more than once"));
			} else {
				lderror(MSG_FATAL, gettxt(":1035","illegal -d %s option"), optarg);
			}
			break;

		case 'e':
			if (entry_point != NULL) 
				lderror(MSG_WARNING,
				    gettxt(":1036","-e specifies multiple program entry points"));
			entry_point = optarg;
			break;

		case 'f':
			if (strcmp(optarg, "cplus_version") == 0)
				check_cplus_version = TRUE;
			else if (!force_platform(optarg)) {

				lderror(MSG_FATAL,
				    gettxt(":1616","illegal -f %s option"),
					optarg);
			}
			break;

		case 'h':
			if (dynoutfile_name != NULL)
                                lderror(MSG_WARNING,
				    gettxt(":1037","-h specifies multiple output filenames for dynamic structure"));
                        dynoutfile_name = optarg;
			break;

		case 'I':
			if (interp_path != NULL)
                                lderror(MSG_WARNING,
				    gettxt(":1038","-I specifies multiple interpreter paths"));			
                        interp_path = optarg;
			break;

		case 'l':
			fileopts = TRUE;
			break;

		case 'm':
			mflag = TRUE;
			break;

		case 'n':
			break;

		case 'o':
			if (memcmp(outfile_name,A_OUT,sizeof(A_OUT)))
                                lderror(MSG_WARNING,
				    gettxt(":1039","-o specifies multiple output file names "));
			outfile_name = optarg;
			break;
		case 'r':
			if (rflag)
				lderror(MSG_WARNING,
					gettxt(":1040","-r used more than once"));
			rflag = TRUE;
			break;

		case 's':
			sflag = TRUE;
			break;

		case 't':
			tflag = TRUE;
			break;

		case 'u':
			add_usym(optarg);
			break;

		case 'x':
			xflag = TRUE;
			break;

		case 'z':
			if (strncmp(optarg, "defs", 4) == SAME) {
				if (zseen)
					lderror(MSG_WARNING,gettxt(":1042","-zdefs/nodefs appears more than once: last taken"));
				else
					zseen = TRUE;
				znflag = !(zdflag = TRUE);
			} else if (strcmp(optarg, "nodefs") == SAME) {
				if (zseen)
					lderror(MSG_WARNING,gettxt(":1043","-zdefs/nodefs appears more than once - last taken"));
				else
					zseen = TRUE;
                                zdflag = !(znflag = TRUE);
			} else if (strncmp(optarg, "text", 4) == SAME) {
				ztflag = TRUE;
			} else if (strcmp(optarg, "weak_undefs") == SAME) {
				create_defs_for_weak_refs = TRUE;
			} else {
				lderror(MSG_FATAL, gettxt(":1044","illegal -z %s option"), optarg);
			}
			break;

#ifdef	DEBUG
		case 'D':
			set_debug(optarg);
			break;
#endif

		case 'B':
			if (strcmp(optarg, "dynamic") == SAME) {
				Bflag_dynamic = TRUE;
			} else if (strcmp(optarg, "static") == SAME) {
				Bflag_dynamic = FALSE;
			} else if (strcmp(optarg, "sortbss") == SAME) {
				Bsortbss = TRUE;
			} else if (strcmp(optarg, "bind_now") == SAME) {
				Bbind_now = TRUE;
			} else if (strncmp(optarg, "symbolic", 8) == SAME) {
				if (strlen(optarg) > 9) {
					if (*(optarg + 8) == '=')
						bsym_list = build_sym_list(optarg + 9);
					else if (*(optarg + 8) == ':')
						bsym_list = process_text_file(optarg + 9, -1, 0, 1, 0);
					else
						lderror(MSG_FATAL, 
						gettxt(":410","illegal form for -B symbolic option"));
				} else
					Bflag_symbolic = TRUE;
			} else if (strncmp(optarg, "export", 6) == SAME) {
				if (strlen(optarg) > 7) {
					if (*(optarg + 6) == '='){
						bexport_list = build_sym_list(optarg + 7);
						Bflag_export = LIST;
						}
					else if (*(optarg + 6) == ':') {
						bexport_list = process_text_file(optarg + 7, -1, 0, 1, 0);
						Bflag_export = LIST;
						}
					else
						lderror(MSG_FATAL, 
						gettxt(":1508","illegal form for -Bexport option"));
				} else
					Bflag_export = ALL;
			} else if (strncmp(optarg, "hide", 4) == SAME) {
				if (strlen(optarg) > 5) {
					if (*(optarg + 4) == '='){
						bhide_list = build_sym_list(optarg + 5);
						Bflag_hide = LIST;
						}
					else if (*(optarg + 4) == ':') {
						bhide_list = process_text_file(optarg + 5, -1, 0, 1, 0);
						Bflag_hide = LIST;
						}
					else
						lderror(MSG_FATAL, 
						gettxt(":1509","illegal form for -Bhide option"));
				} else
					Bflag_hide = ALL;
			} else if (strncmp(optarg, "qhide", 5) == SAME) {
				if (strlen(optarg) > 6) {
					if (*(optarg + 5) == '='){
						qhide_list = build_sym_list(optarg + 6);
						qflag_hide = LIST;
						}
					else if (*(optarg + 5) == ':') {
						qhide_list = process_text_file(optarg + 6, -1, 0, 1, 0);
						qflag_hide = LIST;
						}
					else
						lderror(MSG_FATAL, 
						gettxt(":1509","illegal form for -Bqhide option"));
				} else
					qflag_hide = ALL;
			} else 
				lderror(MSG_FATAL, gettxt(":1045",
					"illegal -B %s option"), optarg);
			break;

		case 'G':
			Gflag = TRUE;
			break;

		case 'L':
			break;

		case 'M':
			if (Mflag != NULL)
				lderror(MSG_FATAL,
					gettxt(":1046","more than one mapfile specified"));
			Mflag = optarg;
			break;

		case 'Q':
			if (strcmp(optarg,"y") == SAME)
                                if (Qflag == NOT_SET)
					Qflag = SET_TRUE;
				else
					lderror(MSG_WARNING,
					    gettxt(":1047","-Q appears more than once; first setting retained"));
			else
				if (strcmp(optarg,"n") == SAME)
					if (Qflag == NOT_SET)
						Qflag = SET_FALSE;
                                else
                                        lderror(MSG_WARNING,
                                            gettxt(":1047","-Q appears more than once; first setting retained"));
			else 
				lderror(MSG_WARNING,gettxt(":1048","bad argument to -Q flag, ignored"));
			break;
		case 'T':
			/* option to change where a.out is mapped */
			sscanf(optarg, "%x", &firstseg_origin);
			firstseg_origin &= 0xFFFFE000;
			break;

		case 'V':
			if(!Vflag)
				pfmt(stderr,MM_INFO,":8: %s %s\n",CPL_PKG,CPL_REL);
			Vflag = TRUE;
			break;

		case 'Y':
			if (strncmp(optarg, "L,", 2) == SAME) {
                             	libdir = optarg+2;
			} else if (strncmp(optarg, "U,", 2) == SAME) {
                             	llibdir = optarg + 2;
			} else if(strncmp(optarg, "P,",2) == SAME) {
				YPflag = TRUE;
				libpath = optarg+2;
			} else {
				lderror(MSG_FATAL, gettxt(":1049","illegal -Y %s option"), optarg);
			}
			break;

		case '?':
			usage_mess(FULL_USAGE);
			exit(EXIT_FAILURE);

		default:
			break;
		}		/* END: switch (c) */
	}
	for(;optind < argc; optind++){
		if(argv[optind][0] == '-') {
			if ( !argv[optind][1] ) {
				usage_mess(SUMM_USAGE);
				exit(EXIT_FAILURE);
			}
			goto getmore;
		}
		fileopts = TRUE;
		DPRINTF(DBG_ARGS,(MSG_DEBUG,"args: got a file argument %s", argv[optind]));
	}
 usage:
	if (errflag) {
		usage_mess(SUMM_USAGE);
		exit(EXIT_FAILURE);
	}
}



/*
** process_files(int argc, char** argv, int start_with_arg);
** processes the command line files.  Argc and argv are the usual
** stuff passed to main.
**
** Pass 1 -- process_flags: collects all options and sets flags
** check_flags -- checks for flag consistency
** Pass 2 -- process_files: skips the flags collected in pass 1 and processes files
*/

void
process_files(argc, argv, start_with_arg)
	int	argc;
	char**	argv;

{
	int	c;		/* character returned by getopt */

	optind = start_with_arg; /* reinitialize optind */
	 
    getmore:
        while ((c = getopt(argc, argv, OPTIONS)) != -1){
	DPRINTF(DBG_ARGS, (MSG_DEBUG, "input argument to process_files is: %#x",c));

		switch (c) {
			case 'l':
				find_library(optarg);
				break;
			case 'B':
				if (strcmp(optarg, "dynamic") == SAME){
					if (dmode)
	                                	Bflag_dynamic = TRUE;
					else
						lderror(MSG_FATAL,
							gettxt(":1026","the -dn and -Bdynamic flags are incompatible"));
				}else if (strcmp(optarg,"static") == SAME)
	                                Bflag_dynamic = FALSE;
				break;
			case 'L':
				add_libdir(optarg);
				break;
			default:
				break;
			}
		}
       for(;optind < argc; optind++){
		if(argv[optind][0] == '-')
                        goto getmore;
                cur_file_name = argv[optind];
		if ( (cur_file_fd = open(cur_file_name,O_RDONLY)) == -1)
			lderror(MSG_FATAL,gettxt(":1050","cannot open file for reading"));
		process_infile(cur_file_name);
		DPRINTF(DBG_ARGS,(MSG_DEBUG,"args: got a file argument %s", argv[optind]));
	}
}

/* print usage message to stderr - 2 modes, summary message only,
 * and full usage message
 */
static void
usage_mess(mode)
int mode;
{
	pfmt(stderr,MM_ACTION,":1170:usage: %sld [-abmrstxGVd:e:f:h:l:o:u:z:B:I:L:M:Q:Y:] file(s) ...\n",SGS);

	if (mode == SUMM_USAGE)
		return;

	pfmt(stderr,MM_NOSTD,":1171:\t[-a create absolute file]\n");
	pfmt(stderr,MM_NOSTD,":1172:\t[-b do not do special PIC relocations in a.out]\n");
	pfmt(stderr,MM_NOSTD,":1173:\t[-m print memory map]\n");
	pfmt(stderr,MM_NOSTD,":1174:\t[-r create relocatable file]\n");
	pfmt(stderr,MM_NOSTD,":1175:\t[-s strip symbol and debugging information]\n");
	pfmt(stderr,MM_NOSTD,":1176:\t[-t do not warn for multiply defined symbols of different sizes]\n");
	pfmt(stderr,MM_NOSTD,":1770:\t[-x strip local symbols and debugging information]\n");
	pfmt(stderr,MM_NOSTD,":1177:\t[-G create shared object]\n");
	pfmt(stderr,MM_NOSTD,":1178:\t[-V print version information]\n");
	pfmt(stderr,MM_NOSTD,":1179:\t[-d y|n operate in dynamic|static mode]\n");
	pfmt(stderr,MM_NOSTD,":1180:\t[-e sym use sym as starting text location]\n");
	pfmt(stderr,MM_NOSTD,":1617:\t[-f [iabi|osr5] force creation of Intel iABI or OpenServer ELF style objects]\n");
	pfmt(stderr,MM_NOSTD,":1181:\t[-h name use name as internal shared object string]\n");
	pfmt(stderr,MM_NOSTD,":1182:\t[-l x search for libx.so or libx.a]\n");
	pfmt(stderr,MM_NOSTD,":1183:\t[-o outfile name output file outfile]\n");
	pfmt(stderr,MM_NOSTD,":1184:\t[-u symname create undefined symbol symname]\n");
	pfmt(stderr,MM_NOSTD,":1185:\t[-z defs|nodefs disallow|allow undefined symbols]\n");
	pfmt(stderr,MM_NOSTD,":1186:\t[-z text disallow output relocations against text]\n");
	pfmt(stderr,MM_NOSTD,":1187:\t[-B dynamic|static search for shared libraries|archives]\n");
	pfmt(stderr,MM_NOSTD,":411:\t[-B sortbss assign contiguous addresses to bss symbols in each input file]\n");
	pfmt(stderr,MM_NOSTD,":1188:\t[-B symbolic bind external references to definitions\n\t\t when creating shared objects]\n");
	pfmt(stderr,MM_NOSTD,":1510:\t[-B export global symbols are made visible\n\t\t outside of the executable]\n");
	pfmt(stderr,MM_NOSTD,":1511:\t[-B hide global symbols are not made visible\n\t\t outside of the shared object]\n");
	pfmt(stderr,MM_NOSTD,":1189:\t[-I interp use interp as path name of interpreter]\n");
	pfmt(stderr,MM_NOSTD,":1190:\t[-L path search for libraries in directory path]\n");
	pfmt(stderr,MM_NOSTD,":1191:\t[-M mapfile use processing directives contained in mapfile]\n");
	pfmt(stderr,MM_NOSTD,":1192:\t[-Q y|n do|do not place version information in output file]\n");
	pfmt(stderr,MM_NOSTD,":1193:\t[-YP,dirlist use dirlist as default path when searching for libraries]\n");
	return;
}

/* current input file is not an archive, COFF or ELF file;
 * try to process it as a list of file arguments;
 * valid arguments are: filename, -Ldirlist, -llibname -B[static|dynamic]
 * arguments are separated by commas or newlines; lines beginning
 * with # are comments
 */
void
process_arg_file(fname)
char *fname;
{
	int	argc = 0;
	char	**argv;
	char	**argp;
	int	save_optind;

	DPRINTF(DBG_ARGS,(MSG_DEBUG,"trying to process %s as an argument file", fname));
	if ((argv = process_text_file(fname, cur_file_fd, 1, 0, &argc)) == NULL) {
		lderror(MSG_FATAL,gettxt(":1061", "file type neither object nor archive"));

	}

	close(cur_file_fd);
	cur_file_fd = -1;
	cur_file_ptr = NULL;
        cur_infile_ptr = NULL;
        cur_file_ehdr = NULL;
	cur_file_name = NULL;

	save_optind = optind;
	process_files(argc, argv, 0);
	optind = save_optind;
	free_list(argv);
}
