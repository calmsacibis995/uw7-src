#ident	"@(#)ld:common/globals.c	1.31"
/*
 * Global variables
 */

/****************************************
** Imports
****************************************/

#include	<stdio.h>
#include	"paths.h"
#include	"sgs.h"
#include	"globals.h"

/****************************************
** Variables
****************************************/

/*
 * Below are the global variables used by the linker.  Their meanings are
 * defined in globals.h
 */

#ifdef	DEBUG
Boolean debug_bits[DBG_MAX]; 		/* Debugging flag bits array */
#endif	/* DEBUG */

Boolean		aflag, bflag, dmode=TRUE, mflag, rflag, sflag, tflag, 
		xflag, zdflag, znflag, ztflag, Gflag, Bflag_symbolic,
		Bflag_dynamic, Bsortbss,
		local_common, Bbind_now, create_defs_for_weak_refs,
		check_cplus_version, strip_debug;
Boolean		processing_so = FALSE;

Setstate	Qflag = NOT_SET;

SetBexportstate	Bflag_export = NONE;
SetBexportstate	Bflag_hide   = NONE;
SetBexportstate	qflag_hide   = NONE;

Word		bss_align;
Word		copyrels;
Word		countGOT = GOT_XNumber;
Word		countPLT = PLT_XNumber;
Word		count_dynglobs;
Word		count_dynstrsize;
Word		count_namelen;
Word		count_osect;
Word		count_outglobs;
Word		count_hidden = 0;
Word		count_outlocs;
Word		count_rela;
Word		count_delay_rel;
Word		count_strsize;
Word		count_locstr;
Ehdr		*cur_file_ehdr;
int		cur_file_fd;
char		*cur_file_name = COMMAND_LINE ;
Elf		*cur_file_ptr;
Infile		*cur_infile_ptr;
Word		dynbkts;
char		*dynoutfile_name;
Word		ehdr_flags;
char		*entry_point;
Addr		firstexec_seg;
Addr		firstseg_origin = FIRSTSEG_ORIGIN;
Word		grels;
List		infile_list;
char		*interp_path;
char		*ld_run_path;
char		*libdir = NULL;
Word		libver;
char 		*llibdir = NULL;
char 		*libpath = LIBPATH;
Word		orels;
Ehdr		*outfile_ehdr;
Elf		*outfile_elf;
int		outfile_fd;
char		*outfile_name = A_OUT;
Phdr		*outfile_phdr;
Word		prels;
Word		sizePHDR;
List		soneeded_list = {NULL, NULL};
List		symbucket[NBKTS];
Elf_Data	*symname_bits;
Boolean		textrel;
char		**bsym_list;
char		**bexport_list;
char		**bhide_list;
char		**qhide_list;
Boolean		shared_object = 0;
Bss_cpy		*bss_cpy_ptr;
List		bss_cpy_list;
Second_level_needed_so	*second_level_needed_so_ptr;
List		second_level_needed_so_list;
char		*LD_ROOT_path = NULL;
Os_desc		*eh_ranges_sect;
Boolean		wrong_cplus_version;
