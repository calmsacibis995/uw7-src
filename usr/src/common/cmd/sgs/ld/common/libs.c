#ident	"@(#)ld:common/libs.c	1.35"
/*
 * Library processing
 */

/****************************************
** Imports
****************************************/

#include	<stdio.h>
#ifdef	__STDC__
#include	<unistd.h>
#endif	/* __STDC__ */
#include	<fcntl.h>
#include	<string.h>
#include	<memory.h>
#include	"paths.h"
#include	"globals.h"

/****************************************
** Local Variables
****************************************/

static List
	libdir_list;		/* the path list */

static Listnode*
	dirlist_insert;		/* insert place for -L libraries */

					
/****************************************
** Local Function Declarations
****************************************/

LPROTO(char *compat_YL_YU, (char *, int));
LPROTO(char *process_lib_path, (char *, int /*Boolean*/));

/****************************************
** Local Function Definitions
****************************************/

/* function to handle -YL and -YU substitutions in LIBPATH */

static char *
compat_YL_YU(def_path, index)
	char	*def_path;
	int	index;
{
	/* user supplied -YL,libdir  and this is the pathname that corresponds 
		for compatibility to -YL (as defined in paths.h) */
	if(libdir && index == YLDIR)
		return(libdir);

	/* user supplied -YU,llibdir  and this is the pathname that corresponds 
		for compatibility to -YU (as defined in paths.h) */
	if(llibdir && index == YUDIR)
		return(llibdir);

	return(def_path);
}

static char * 
process_lib_path(pathlib,subsflag)
	char	*pathlib;
	Boolean	subsflag;
{
	int	i;
	char	*cp;
	Boolean	seenflg = FALSE;


	for(i=1;; i++) {
		cp = strpbrk(pathlib,";:");
		if(cp == NULL) {
			if(*pathlib == '\0') {
				if(seenflg)
				    list_append(&libdir_list,
					subsflag ? compat_YL_YU(".",i) : ".");
			} else
				list_append(&libdir_list,
				    subsflag ? compat_YL_YU(pathlib,i) : pathlib);
			return(cp);
		}

		if(*cp == ':') {
			if(cp == pathlib)
				list_append(&libdir_list,
					subsflag ? compat_YL_YU(".",i) : ".");
			else
				list_append(&libdir_list,
				    subsflag ? compat_YL_YU(pathlib,i) : pathlib);
			*cp = '\0';
			pathlib = cp + 1;
			seenflg = TRUE;
			continue;
		}
		
		/* case ; */

		if(cp != pathlib)
			list_append(&libdir_list,
			    subsflag ? compat_YL_YU(pathlib,i) : pathlib);
		else {
			if(seenflg)
				list_append(&libdir_list,
					subsflag ? compat_YL_YU(".",i) : ".");
		}
		return(cp);
	}
}

/****************************************
** Global Function Definitions
****************************************/


/*
 * add_libdir(CONST char* pathlib)
 * adds the indicated path to those to be searched for libraries.
 */
void
add_libdir(pathlib)
	CONST char	*pathlib;	/* the library path to add */
{
	char	*path = mymalloc(strlen(pathlib) + 1);
	strcpy(path, pathlib);

	if (dirlist_insert == NULL) {
		(void) list_prepend(&libdir_list, path);
		dirlist_insert = libdir_list.head;
	} else
		dirlist_insert = list_insert(dirlist_insert, path);
}


/* find_library()
 * takes the abbreviated name of a library file and
 * searches for the library on each of the paths
 * specified in libdir_list; if dmode is TRUE and
 * Bflag_dynamic is also true, first looks for
 * a file with full name: path/libfoo.so;
 * then [or else] looks for a file with name: path/libfoo.a.
 * If no such file is found, it's a fatal error;
 * else, we process file appropriately depending on its
 * type.
 */
void
find_library(name)
	CONST char	*name;		/* name to search for */
{
	register Listnode
		*llp;		/* temp ptr to node */
	register char
		*dlp;		/* data for this list ptr */
	char	pathname[FILENAME_MAX];	/* buffer for building paths */
	char	libname[FILENAME_MAX];
	char	sharedname[FILENAME_MAX];
	int	flen;
	int	plen;
	int	lib_fd = -1;	/* file descriptor for library file */

	/* create full names from abbreviated name */
	if ((flen = strlen(name)) > (FILENAME_MAX - ((dmode == TRUE &&
						      Bflag_dynamic == TRUE) ?
						     7 : 6)))
		lderror(MSG_FATAL, gettxt(":1067","lib%s: library name too long"), name);
	(void) strcpy(libname, "/lib");
	(void) strcat(libname, name);
	sharedname[0] = '\0';

	/* search path list for file */
	for (LIST_TRAVERSE(&libdir_list, llp, dlp)) {
		plen = strlen(dlp);
		if (!plen)
			plen = 1;
		if (plen + flen > FILENAME_MAX)
			lderror(MSG_FATAL,gettxt(":1068", "\"%s/lib%s\" pathname too long"), dlp, name);
		if (dlp == (char *)0) {
			/* null pathname == current directory */
			pathname[0] = '.';
			pathname[1] = '\0';
		}
		else
			(void) strcpy(pathname, dlp);
		if (dmode == TRUE && Bflag_dynamic == TRUE) {
			(void) strcat(pathname, libname);
			(void) strcat(pathname, ".so");
			(void) strcat(sharedname, libname);
			(void) strcat(sharedname, ".so");

			DPRINTF(DBG_LIBS,(MSG_DEBUG,"trying to open library lib%s at path: %s",
					libname,pathname));

			if ((lib_fd = open(pathname, O_RDONLY)) != -1) 
				break;
		}
		sharedname[0] = '\0';
		(void) strcpy((pathname + plen), libname);
		(void) strcat(pathname, ".a");

			DPRINTF(DBG_LIBS,(MSG_DEBUG,"trying to open library at path: %s",pathname));

		if ((lib_fd = open(pathname, O_RDONLY)) != -1)
			break;
	}
	if (lib_fd == -1)
		lderror(MSG_FATAL,gettxt(":1069", "library not found: -l%s"), name);

	cur_file_fd = lib_fd;
	cur_file_name = &pathname[0];
	if (sharedname[0] == '\0')
		process_infile(&libname[1]);
	else
		process_infile(&sharedname[1]);

/*	my_elf_end(cur_file_ptr); */
	return;
}

int
seen_so_before(name, pathname)
CONST	char	*name;
CONST	char	*pathname;
{
	register Listnode *ip;	/* temporary pointer to input file */
        Infile  *dp;            /* pointer to Infile data */

        for (LIST_TRAVERSE(&infile_list, ip, dp)) {
                if (dp->so_name && strcmp(name, dp->so_name) == SAME)
			return TRUE;
		else if (strcmp(pathname, dp->fl_name) == SAME)
                        return TRUE;
        }
	return FALSE;
}

void
find_library_path(name, parent_so_name)
	CONST char	*name;		 /* NEEDED library to search for */
	CONST char	*parent_so_name; /* library that needs this library*/
{
	register Listnode
		*llp;		/* temp ptr to node */
	register char
		*dlp;		/* data for this list ptr */
	char	pathname[FILENAME_MAX];	/* buffer for building paths */
	char	libname[FILENAME_MAX];
	int	flen;
	int	plen;
	int	lib_fd = -1;	/* file descriptor for library file */
	char 	*cp;

	if ((flen = strlen(name)) > FILENAME_MAX )
		lderror(MSG_FATAL, gettxt(":1571","%s: library name too long"), name);
	cp = strchr(name, '/');
	if ( cp ) /* It's an absolute or relative path name */ 
	{
		if (LD_ROOT_path != NULL) /* append LD_ROOT to name */
		{ 
			if ((strlen(LD_ROOT_path) + 1 +flen) > FILENAME_MAX)
				lderror(MSG_FATAL, 
				  gettxt(":1572","%s%s: library pathname too long"), LD_ROOT_path, name);

			(void) strcpy(pathname, LD_ROOT_path);
			(void) strcat(pathname, "/");	
			(void) strcat(pathname, name);	
		}
		else
			(void) strcpy(pathname, name); 

		if ((lib_fd = open(pathname, O_RDONLY)) == -1) 
			lderror(MSG_FATAL,
			  gettxt(":1573", "library %s, which is NEEDED by %s, is not found"), pathname, parent_so_name);
	}
	else /* search libdir_list for the file */
	{
	   for (LIST_TRAVERSE(&libdir_list, llp, dlp))
	   {
		plen = strlen(dlp);
		if ((plen + flen + 1) > FILENAME_MAX)
			lderror(MSG_FATAL,gettxt(":1572", "%s%s: library pathname too long"), dlp, name);
		(void) strcpy(pathname, dlp);
		(void) strcat(pathname, "/");
		(void) strcat(pathname, name);

		if ((lib_fd = open(pathname, O_RDONLY)) != -1) 
			break;
	   }
	 
	}

	if (lib_fd == -1) 
		lderror(MSG_FATAL, gettxt(":1573", "library %s, which is NEEDED by %s, is not found"), name, parent_so_name);

	/* Has this shared library already been processed? */
	if ( seen_so_before(name, pathname) == FALSE)
	{ 
		cur_file_fd = lib_fd;
		cur_file_name = &pathname[0];
		DPRINTF(DBG_FILES, (MSG_DEBUG, "processing_infile %s\n", cur_file_name));
		process_infile(cur_file_name);
	}
	else{
		close(lib_fd);
	}	
	return;
}

void
lib_setup()
{
	register char
		*ld_lib_path;	/* the LD_LIBRARY_PATH variable */
	char	*cp, *cp1;		/* temp char pointer */

	if ((cp1 = getenv("LD_ROOT")) != NULL) {
		LD_ROOT_path = (char*) mymalloc(strlen(cp1) + 1);
		(void) strcpy(LD_ROOT_path, cp1);
	}
	else if (LD_ROOT != NULL){
		LD_ROOT_path = (char*) mymalloc(strlen(LD_ROOT) + 1);
		(void) strcpy(LD_ROOT_path, LD_ROOT);
	}

	if ((cp = getenv("LD_LIBRARY_PATH")) != NULL) {
		ld_lib_path = (char*) mymalloc(strlen(cp) + 1);
		(void) strcpy(ld_lib_path, cp);

		ld_lib_path = process_lib_path(ld_lib_path,FALSE);
		if(ld_lib_path != NULL) {

			if(*ld_lib_path == ';')
				dirlist_insert = libdir_list.tail;
			*ld_lib_path = '\0';

			ld_lib_path = process_lib_path(++ld_lib_path,FALSE);
			if (ld_lib_path != NULL)
				lderror(MSG_WARNING,
				gettxt(":1070",	"LD_LIBRARY_PATH has bad format"));
		}
	}
	libpath = process_lib_path(libpath,TRUE);
	if (libpath != NULL)
		lderror(MSG_FATAL,
			gettxt(":1071","-YP default library path malformed"));

}

/* process_archive()
 * reads in the archive's symbol table; for each symbol in the
 * table checks whether that symbol satisifies an unresolved
 * reference in ld's internal symbol table; if so, the corresponding
 * object from the archive is processed; the archive symbol table is
 * searched until we go through a complete pass without satisifying any
 * unresolved symbols
 */

void
process_archive()
{
	int		found_one;
	Ldsym		*symptr;
	Elf		*save_cur_file_ptr;
	char		*arname;
	char		*name_temp;
	char		*save_cur_file_name;
	Elf_Arsym		*arsymbase;
	register Elf_Arsym	*arsym;


	save_cur_file_ptr = cur_file_ptr;
	save_cur_file_name = cur_file_name;
	arsymbase = my_elf_getarsym(save_cur_file_ptr, (size_t *)0);

	DPRINTF(DBG_LIBS,(MSG_DEBUG,"%s: processing archive",cur_file_name));

	/*
	 * Loop through archive symbol table until we make a complete
	 * pass without satisfying an unresolved reference;
	 * for each archive symbol, see if there is a symbol with
	 * the same name in ld's symbol table; if so, and if that
	 * symbol is still unresolved, load the corresponding archive
	 * member
	 */

	do {
		found_one = 0;
		for (arsym = arsymbase; arsym->as_name != 0; ++arsym) {

			symptr = sym_find(arsym->as_name,arsym->as_hash);
			if (symptr == 0)
				continue;

			/* if symbol is undefined we process - UNLESS
			 * symbol is weak; weak symbols do not
			 * trigger archive member inclusion
			 */
			if (symptr->ls_syment->st_shndx != SHN_UNDEF
				|| ELF_ST_BIND(symptr->ls_syment->st_info) != STB_GLOBAL)
				continue;
			if (my_elf_rand(save_cur_file_ptr, arsym->as_off) != arsym->as_off)
				lderror(MSG_FATAL,gettxt(":1072","libelf error: can't get archive member at offset 0x%x "),
					arsym->as_off);

			/* get archive member */

			cur_file_ptr = my_elf_begin(cur_file_fd,ELF_C_READ,save_cur_file_ptr);

			arname = my_elf_getarhdr(cur_file_ptr)->ar_name;
			name_temp = (char *)mycalloc(strlen(cur_file_name)+strlen(arname)+3);
			sprintf(name_temp,"%s(%s)",cur_file_name,arname);
			cur_file_name = name_temp;

			if( elf_kind(cur_file_ptr) == ELF_K_COFF){
				lderror(MSG_NOTICE, gettxt(":1058","internal conversion of COFF file to ELF"));
				my_elf_update(cur_file_ptr,ELF_C_NULL);
			}
			
			if ((cur_file_ehdr = elf_getehdr(cur_file_ptr)) == NULL){
				lderror(MSG_WARNING,
					gettxt(":1073","can't read ELF header for archive member at offset 0x%x"),
					arsym->as_off);
				lderror(MSG_ELF,gettxt(":1074","elf_getehdr: "));
			}

			if (cur_file_ehdr->e_type == ET_REL){
				DPRINTF(DBG_LIBS,(MSG_DEBUG,"%s: symbol %s in archive satisfies unresolved reference",cur_file_name,arsym->as_name));
				process_relobj();
			} else if (cur_file_ehdr->e_type == ET_DYN){
				DPRINTF(DBG_LIBS,(MSG_DEBUG,"%s: symbol %s in archive satisfies unresolved reference",cur_file_name,arsym->as_name));
				if( !dmode || !Bflag_dynamic)
					lderror(MSG_FATAL,gettxt(":1059","input shared object in static mode"));
				else
					process_shobj(arname);
			} else
				lderror(MSG_FATAL,
					gettxt(":1075","bad file type in archive member at offset 0x%x"),
					arsym->as_off);

			cur_file_ptr = (Elf*) 0;
			cur_file_name = save_cur_file_name;
			found_one++;
		 }
	} while (found_one);

	cur_file_ptr = save_cur_file_ptr;
}


#ifdef	DEBUG
/*
 * libdir_list_print()
 * prints the current list of library search paths.
 */
void
libdir_list_print()
{
	register Listnode*
		llp;		/* temp pointer to node */
	char*	dp;		/* the path in the data of the node */

	lderror(MSG_DEBUG, "library search paths");
	for (LIST_TRAVERSE(&libdir_list, llp, dp))
		lderror(MSG_DEBUG, "\t\"%s\"", dp);
}

void
infile_list_print()
{
        register Listnode *ip;	/* temporary pointer to input file */

        Infile  *dp;            /* pointer to Infile data */

	
	lderror(MSG_DEBUG, "Infile Data Structure"); 
        for (LIST_TRAVERSE(&infile_list, ip, dp))
                lderror(MSG_DEBUG,
			"  Infile->fl_name: %s\n  Infile->fl_soname: %s\n",
			dp->fl_name, dp->so_name);
}

#endif	/* DEBUG */

void
process_second_level_needed_so()
{
	register Listnode* ip;            /* temp pointer to node */
        Second_level_needed_so *needed_entry; 
	int	lib_fd;
	Boolean temp;

	DPRINTF(DBG_FILES, (MSG_DEBUG, "\nSecond level NEEDED Shared objects:"));

	if (Bflag_dynamic == FALSE){
		temp = FALSE;
		Bflag_dynamic = TRUE;  /* temporaryily reset */
	}
	processing_so = TRUE;

        for (LIST_TRAVERSE(&second_level_needed_so_list, ip, needed_entry)) 
	{
               DPRINTF(DBG_FILES, (MSG_DEBUG, "\tShared object: %s; NEEDED by %s", needed_entry->so_name, needed_entry->parent_so_name));
		/* find out where the file is */
		find_library_path(needed_entry->so_name, needed_entry->parent_so_name);
	}
	processing_so = FALSE;
	Bflag_dynamic = temp;
}
