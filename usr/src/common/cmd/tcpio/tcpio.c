/*		copyright	"%c%" 	*/


#ident	"@(#)tcpio:tcpio.c	1.19.6.6"

/*********************************************************************
 * Command: tcpio
 * Inheritable Privileges: P_MACREAD,P_MACWRITE,P_DACREAD,P_DACWRITE,
 *			   P_FSYSRANGE,P_OWNER,P_DEV,P_FILESYS,P_COMPAT,
 *			   P_MULTIDIR,P_SETPLEVEL,P_SYSOPS
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/mkdev.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <ctype.h>
#include <archives.h>
#include <locale.h>
#include "tcpio.h"
#include <sys/param.h>
#include <sys/mac.h>
#include <sys/acl.h>
#include <sys/secsys.h>
#include <priv.h>
#include <search.h> 
#include <sys/utsname.h>
#include <ulimit.h>
#include <pfmt.h>
#include "tsec.h"
#include "ttoc.h"

#define TCPIO_PATH	"/usr/bin/tcpio"

extern
int	errno;

extern
int	atoi(),
	bfill(),
	gmatch(),
	getopt(),
	g_init(),
	g_read(),
	g_write(),
	lvlin(),
	atol(),
	lvldom(),
	acl(),
	lvlvalid();

extern
char	*mktemp(),
	*getcwd();

extern
long	lseek();

extern
void	exit(),
	perror(),
	free(),
        print_privs(),
	*malloc(),
	*calloc(),
	*realloc(),
	setpwent(),
	setgrent();

extern
struct med_hdr *M_p,
               *V_p;

extern
time_t Now;

extern
level_t orig_lvl,
	Lo_lvl,
        Hi_lvl,
        Min_lvl,
        Max_lvl,
        Map_lvl;

extern
char    Host[];

extern
int     Unverify,
         nsets;    /* number of privilege sets supported  */

extern
setdef_t *sdefs;   /* privilege set vector */

extern
priv_t    pvec[NPRIVS];

extern
struct sec_hdr *S_p;

struct archive_tree	*Arch_tree_p;	/* current file's archive tree node */

static
struct Lnk *add_lnk();

char 	*align();

static
int	chgreel(),
	ckname(),
	creat_hdr(),
	creat_spec(),
	creat_tmp(),
	gethdr(),
	getname(),
	matched(),
	openout(),
	read_hdr();

static
ushort
        Ftype = S_IFMT;         /* File type mask */

static
uid_t
        Uid,                    /* Uid of invoker */
	priv_id;                /* Super-user id if there is one, else -1 */
static
int
	lpm = 1;		/* default file base mechanism installed */

static
void	
	ckopts(),
	creat_lnk(),
	data_in(),
	data_out(),
	file_in(),
	file_out(),
	flush_lnks(),
	getpats(),
	ioerror(),
	reclaim(),
	rstfiles(),
	setup(),
	set_tym(),
	sigint(),
	swap(),
	usage(),
	verbose(),
	write_hdr(),
	write_trail(),
	inter_names();

void	bflush(),
#ifdef __STDC__
	msg(int, ...),
#else
	msg(),
#endif
	rstbuf();

extern
int     read_mhdr(),
        write_mhdr(),
        comp_mhdr(),
        sec_dir(),
        in_range(),
        adjust_lvl(),
	validate(),
	gt(),
	chg_lvl_back(),
	skip_file(),
	write_shdr();

extern
VP	loc_ttoc();

extern
void    in_sec_setup(),
        out_sec_setup(),
        set_sec();

extern
off_t   read_shdr();

extern
int get_component();


static
ushort
        Adir,                   /* Flags object as a directory */
        Aspec,			/* Flags object as a special file */
	Alink;			/* Flags object as a symbolic link */

struct passwd	*Curpw_p,	/* Current password entry for -t option */
		*Rpw_p;		/* Password entry for -R option */

static
struct group	*Curgr_p;	/* Current group entry for -t option */

/* Data structure for buffered I/O. */

struct buf_info Buffr;

/* Generic header format */

struct gen_hdr	Gen, *G_p;

/* Data structure for handling multiply-linked files */
static
char	nambuf[100],
	tbuffr[1];

static
struct Lnk {
	short	L_cnt,		/* Number of links encountered */
		L_data;		/* Data has been encountered if 1 */
	struct gen_hdr	L_gen;	/* gen_hdr information for this file */
	struct Lnk	*L_nxt_p,	/* Next file in list */
			*L_bck_p,	/* Previous file in list */
			*L_lnk_p;	/* Next link for this file */
} Lnk_hd;

struct stat     ArchSt; /* stat(2) information of the archive */

static
struct stat     SrcSt,  /* stat(2) information of source file */
                DesSt;  /* stat(2) of destination file */

/*
 * swpbuf: Used in swap() to swap bytes within a halfword,
 * halfwords within a word, or to reverse the order of the 
 * bytes within a word.
 */

static
union swpbuf {
	unsigned char	s_byte[4];
	ushort	s_half[2];
	ulong	s_word;
} *Swp_p;

static
char	
	Time[50],		/* Array to hold date and time */
	Ttyname[] = "/dev/tty",	/* Controlling console */
	*Efil_p,		/* -E pattern file string */
	*Eom_p = "Change to part %d and press RETURN key. [q] ",
	*Eom_pid = ":32",	/* Message id for Eom_p message */
	*Nam_p,			/* Array to hold filename */
	*Own_p,			/* New owner login id string */
	*Renam_p,		/* Buffer for renaming files */
	*Symlnk_p,		/* Buffer for holding symbolic link name */
        *Xlohi_p,               /* -X lo hi level string */
	*Map_p,			/* -N level pointer */
	*Over_p;		/* Holds temporary filename when overwriting */

char 	*Fullnam_p,		/* Full pathname */
	*IOfil_p,		/* -I/-O input/output archive string */
        *Tfil_p,                /* -T (TTOCTT) file string */
	**Pat_pp = 0;		/* Pattern strings */

static
int	Append = 0,	/* Flag set while searching to end of archive */
	Buf_error = 0,	/* I/O error occured during buffer fill */
	Def_mode = 0777,/* Default file/directory protection modes */
	Error_cnt = 0,	/* Cumulative count of I/O errors */
	Serror_cnt = 0, /* Count of errors which didn't generate messages */
	Finished = 1,	/* Indicates that a file transfer has completed */
	Hdrsz = GENSZ,	/* Fixed length portion of the header */
	Ifile,		/* File des. of file being archived */
	Ofile,		/* File des. of file being extracted from archive */
	Verbcnt = 0,	/* Count of number of dots '.' output */
	Eomflag = 0,
	Dflag = 0;

/* no static at all */
int
	Device,		/* Device type being accessed (used with libgenIO) */
	Orig_umask,	/* Inherited umask */
	privileged,	/* flag indicating if process is privileged */
	aclpkg = 1,	/* ACL security package toggle */
	macpkg = 1,	/* MAC security package toggle */
	Archive,	/* File descriptor of the archive */
	aclmax,		/* Maximum number of acl entries allowed in system */
	Trailer,	/* We're writing the Trailer record */
	Newreel;	/* Switched to new reel while writing the Trailer */

struct acl *aclbufp;	/* Buffer to hold up to maximum number of acl entries */

long    Args = 0;       /* Mask of selected options */

off_t   Pad_val = FULLWD; /* Indicates the number of bytes to pad (if any) */

static
gid_t	Lastgid = -1;	/* Used with -t & -v to record current gid */

static
uid_t	Lastuid = -1;	/* Used with -t & -v to record current uid */

static
long	

	Blocks,		/* Number of full blocks transferred */
	Max_filesz,	/* Maximum file size from ulimit(2) */
	Max_namesz = APATH,	/* Maximum size of pathnames/filenames */
	SBlocks;	/* Cumulative char count for short reads */

int	Bufsize = BUFSZ;	/* Default block size */

static
FILE	*Ef_p,			/* File pointer of pattern input file */
	*Err_p = stderr,	/* File pointer for error reporting */
	*Out_p = stdout,	/* File pointer for non-archive output */
	*Rtty_p,		/* Input file pointer for interactive rename */
        *Tf_p,                  /* -T (TTOCTT) file pointer */
	*Wtty_p;		/* Output file pointer for interactive rename */




/*
 * main: Call setup() to process options and perform initializations,
 * and then select either copy in (-i) or copy out (-o) action.
 */

main(argc, argv)
char **argv;
int argc;
{
	int doarch = 0;
        register off_t rv;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:tcpio");
	setup(argc, argv);
	if (signal(SIGINT, sigint) == SIG_IGN)
		(void)signal(SIGINT, SIG_IGN);

        switch (Args & (OCi | OCo)) {
        case OCi: /* COPY IN */
                in_sec_setup(Tf_p);
                while (rv = gethdr()) {
                    if (rv < 0) {
                        continue;
                    }
                    file_in();
                }
		/* Do not count "extra" "read-ahead" buffered data */
		if (Buffr.b_cnt > Bufsize)
			Blocks -= (Buffr.b_cnt / Bufsize);
		break;
	case OCo: /* COPY OUT */
                out_sec_setup();
		inter_names(&doarch);
		if (doarch > 0)
			write_trail();
		break;

	default:
		msg(EXT, ":56", "Impossible action.");
	}
	Blocks = (Blocks * Bufsize + SBlocks + 0x1FF) >> 9;
	msg(EPOST, ":57", "%ld blocks", Blocks);
	if (Error_cnt == 1)
		msg(EPOST, ":58", "1 error");
	else if (Error_cnt > 1)
		msg(EPOST, ":771", "%d errors", Error_cnt);
	exit(Error_cnt);
}

/*
 * Procedure:	  getpriv
 *
 * Restrictions:  filepriv()	none
 *
 * Notes:  Find out if the process is privileged (return 1) or
 * 	   not (return 0).  If lpm is installed and the process'
 * 	   working privilege set is at least what the privileges on
 * 	   the tcpio binary are, then the process is privileged.  Or 
 * 	   if lpm is not installed, and Uid == priv_id, then the
 * 	   process is privileged.
 */

static
int 
getpriv()
{
	priv_t procmask[NPRIVS];	/* process' working priv mask */
	priv_t filemask[NPRIVS];	/* file's priv mask */
	priv_t procbuff[NPRIVS * 2];	/* process' max & working privs */
	priv_t filebuff[NPRIVS];	/* file's fixed & inheritable privs */
	int i, j, numprivs;
	
	/* Check if lpm is installed.  If not, check *
	 * if Uid is privileged.		     */
	
	if (!lpm) {
		if (Uid == priv_id)
			return(1);
		else
			return(0);
	}

	/* Since lpm is installed, we have to check the  *
	 * process' privileges against tcpio's privileges *
	 * to determine if the process is "privileged".  */

	/* Initialize procmask and filemask to all 0's. */

	for (i = 0; i < NPRIVS; i++) {
		procmask[i] = 0;
		filemask[i] = 0;
	}
		
	/* Determine what privileges are associated with *
	 * process.  This includes both max and working. */

	if ((numprivs = procpriv(GETPRV, procbuff, NPRIVS * 2)) == -1)
		msg(EXTN, ":825", "Cannot retrieve process privileges");

	/* Set corresponding procmask bits when we find *
	 * what working privileges are set.             */

	for (i = 0; i < numprivs; i++) {
		for (j = 0; j < NPRIVS; j++) {
			if (procbuff[i] == pm_work(j)) {
				procmask[j] = 1;
				break;
			}
		}
	}

	/* Determine what privileges are associated with *
	 * tcpio.  This includes inheritable and fixed.   */

	if ((numprivs = filepriv(TCPIO_PATH, GETPRV, filebuff, NPRIVS)) == -1)
		msg(EXTN, ":909", "Cannot retrieve file privileges for \"%s\"", TCPIO_PATH);


	/* Set corresponding filemask bits when we find   *
	 * what fixed and inheritable privileges are set. */

	for (i = 0; i < numprivs; i++) {
		for (j = 0; j < NPRIVS; j++) {
			if ((filebuff[i] == pm_fixed(j)) || 
			    (filebuff[i] == pm_inher(j))) {
				filemask[j] = 1;
				break;
			}
		}
	}

	/* Determine if everything that is set in the filemask *
	 * is set in the procmask (i.e. if the process has at  *
	 * least the privileges that tcpio has).		       */

	for (i = 0; i < NPRIVS; i++) {
		if (procmask[i] != filemask[i]) {
			if (filemask[i] == 1) {
				/* This means that tcpio has a privilege *
				 * that process does not, so process is *
				 * not "privileged."                    */
				return(0);
			}
		}
	}

	/* If it gets here, then process has at least *
	 * the privileges that tcpio has, so process   *
	 * is "privileged".			      */
			
	return(1);
}

/*
 * add_lnk: Add a linked file's header to the linked file data structure.
 * Either adding it to the end of an existing sub-list or starting
 * a new sub-list.  Each sub-list saves the links to a given file.
 */

static
struct Lnk *
add_lnk(l_p)
register struct Lnk **l_p;
{
	register struct Lnk *t1l_p, *t2l_p, *t3l_p;

	t2l_p = Lnk_hd.L_nxt_p;
	while (t2l_p != &Lnk_hd) {
		if (t2l_p->L_gen.g_ino == G_p->g_ino && t2l_p->L_gen.g_dev == G_p->g_dev)
			break; /* found */
		t2l_p = t2l_p->L_nxt_p;
	}
	if (t2l_p == &Lnk_hd)
		t2l_p = (struct Lnk *)NULL;
	t1l_p = (struct Lnk *)malloc(sizeof(struct Lnk));
	if (t1l_p == (struct Lnk *)NULL)
		msg(EXT, ":643", "Out of memory: %s", "add_lnk()");
	t1l_p->L_lnk_p = (struct Lnk *)NULL;
	t1l_p->L_gen = *G_p; /* structure copy */
	t1l_p->L_gen.g_nam_p = (char *)malloc((unsigned int)G_p->g_namesz);
	if (t1l_p->L_gen.g_nam_p == (char *)NULL)
		msg(EXT, ":643", "Out of memory: %s", "add_lnk()");
	(void)strcpy(t1l_p->L_gen.g_nam_p, G_p->g_nam_p);
	if (t2l_p == (struct Lnk *)NULL) { /* start new sub-list */
		t1l_p->L_nxt_p = &Lnk_hd;
		t1l_p->L_bck_p = Lnk_hd.L_bck_p;
		Lnk_hd.L_bck_p = t1l_p->L_bck_p->L_nxt_p = t1l_p;
		t1l_p->L_lnk_p = (struct Lnk *)NULL;
		t1l_p->L_cnt = 1;
		t1l_p->L_data = 0;
		t2l_p = t1l_p;
	} else { /* add to existing sub-list */
		t2l_p->L_cnt++;
		t3l_p = t2l_p;
		while (t3l_p->L_lnk_p != (struct Lnk *)NULL) {
			t3l_p->L_gen.g_filesz = G_p->g_filesz;
			t3l_p = t3l_p->L_lnk_p;
		}
		t3l_p->L_gen.g_filesz = G_p->g_filesz;
		t3l_p->L_lnk_p = t1l_p;
	}
	*l_p = t2l_p;
	return(t1l_p);
}

/*
 * align: Align a section of memory of size bytes on a page boundary and 
 * return the location.  Used to increase I/O performance.
 */

char *
align(size)
register int size;
{
	register int pad, mach_pg_size;

	mach_pg_size = (int)sysconf(_SC_PAGESIZE);
	if ((pad = ((int)sbrk(0) & (mach_pg_size - 1))) > 0) {
		pad = mach_pg_size - pad;
		if ((int)sbrk(pad) ==  -1)
			return((char *)-1);
	}
	return((char *)sbrk(size));
}

/*
 * Procedure:     bfill
 *
 * Restrictions:
                 g_read: none
 * Notes: fill the I/O buffer from the archive.
 */

int
bfill()
{
 	register int i = 0, rv;
	static int eof = 0;

	if (!Dflag) {
	while ((Buffr.b_end_p - Buffr.b_in_p) >= Bufsize) {
		errno = 0;


		if ((rv = g_read(Device, Archive, Buffr.b_in_p, Bufsize)) < 0) {

			if (((Buffr.b_end_p - Buffr.b_in_p) >= Bufsize) && (Eomflag == 0)) {
				Eomflag = 1;
				return(1);
			}
			if (errno == ENOSPC) {

				(void)chgreel(INPUT,1);

				continue;
			} else if (Args & OCk) {
				if (i++ > MX_SEEKS)
					msg(EXT, ":60", "Cannot recover.");
				if (lseek(Archive, Bufsize, SEEK_REL) < 0)
					msg(EXTN, ":61", "lseek() failed");
				Error_cnt++;
				Buf_error++;
				rv = 0;
				continue;
			} else
				ioerror(INPUT);
		} /* (rv = g_read(Device, Archive ... */


		Buffr.b_in_p += rv;
		Buffr.b_cnt += (long)rv;
		if (rv == Bufsize)
			Blocks++;
		else if (!rv) {
			if (!eof) {
				eof = 1;
				break;
			}
			return(-1);
		} else
			SBlocks += rv;
	} /* (Buffr.b_end_p - Buffr.b_in_p) <= Bufsize */
	}
	else {
		errno = 0;
		if ((rv = g_read(Device, Archive, Buffr.b_in_p, Bufsize)) < 0) {
			return(-1);
                } /* (rv = g_read(Device, Archive ... */
                Buffr.b_in_p += rv;
                Buffr.b_cnt += (long)rv;
                if (rv == Bufsize)
                        Blocks++;
                else if (!rv) {
                        if (!eof) {
                                eof = 1;
				return(rv);
                        }
                        return(-1);
                } else
                        SBlocks += rv;
	}
	return(rv);
}

/*
 * Procedure:     bflush
 *
 * Restrictions:
                 g_write: none
 * Notes: write the buffer to the archive.
 */

void
bflush()
{
	register int rv;
        int first = 1;

	while (Buffr.b_cnt >= Bufsize) {
		errno = 0;


		if ((rv = g_write(Device, Archive, Buffr.b_out_p, Bufsize)) < 0) {


			if (errno == ENOSPC && !Dflag) {
                            if (!first)
                                msg(ERR, ":910", "Medium too small, try another");
			    rv = chgreel(OUTPUT,first);
			    if (Trailer) {
				Newreel = 1;
			    }
                            first = 0;
			} else
				ioerror(OUTPUT);
		}

		Buffr.b_out_p += rv;
		Buffr.b_cnt -= (long)rv;
		if (rv == Bufsize)
			Blocks++;
		else if (rv > 0)
			SBlocks += rv;
	}
	rstbuf();
}

/*
 * Procedure:     chgreel
 *
 * Restrictions:
                 fopen: None
                 open(2): None
                 g_init: None
                 g_write: None
 * Notes: Determine if end-of-medium has been reached.  If it has,
 * close the current medium and prompt the user for the next medium.
 *
 * On OUTPUT, save the contents of the unwritten buffer, write new medium header
 * then move the data back to the output buffer.
 */

static
int
chgreel(dir, inc)
register int dir;
int inc;
{
	register int lastchar, tryagain, askagain, rv;
	int tmpdev;
	char str[APATH];

	if (dir)
		msg(EPOST, ":62", "\007End of medium on output.");
	else
		msg(EPOST, ":63", "\007End of medium on input.");


        M_p->mh_volume += inc;

	for (;;) {
		(void)close(Archive);
		if (Rtty_p == (FILE *)NULL) {


			Rtty_p = fopen(Ttyname, "r");


		}
		if (Rtty_p == (FILE *)NULL)
		    msg(EXT, ":911", "Process not attached to terminal.  Cannot change medium");

		do { /* tryagain */
				do {
					msg(EPOST, Eom_pid, Eom_p, M_p->mh_volume);
					if (fgets(str, sizeof(str), Rtty_p) == (char *)NULL)
						msg(EXT, ":41", "Cannot read tty.");
					askagain = 0;
					switch (*str) {
					case '\n':
						(void)strcpy(str, IOfil_p);
						break;
					case 'q':
						exit(Error_cnt);
					default:

						lastchar = strlen(str) - 1;
						if (*(str + lastchar) == '\n') /* remove '\n' */
						    *(str + lastchar) = '\0';
					}
					if (!*str)
					    askagain = 1;
				} while (askagain);
			tryagain = 0;


			if ((Archive = open(str, dir)) < 0) {
				msg(ERRN, ":65", "Cannot open \"%s\"", str);
				tryagain = 1;
			}


		} while (tryagain);


		(void)g_init(&tmpdev, &Archive);


		if (tmpdev != Device)
			msg(EXT, ":66", "Cannot change media types in mid-stream.");
		if (dir == INPUT) {
			if (read_mhdr(V_p, 0) == ERROR)
			    /* error messages printed from read_mhdr() */
			    continue;
			break;
		}
		else { /* dir == OUTPUT */
			errno = 0;
	    		if (write_mhdr(M_p, 0) == ERROR) {
				msg(ERR, ":67", "Cannot write on this medium, try another.");
				continue;
			}


			if ((rv = g_write(Device, Archive, Buffr.b_out_p, Bufsize)) == Bufsize) {


				break;
			}
			else
				msg(ERR, ":67", "Cannot write on this medium, try another.");

		}
	} /* ;; */

	Eomflag = 0;
	return(rv);
}



/*
 * ckname: Check filenames against user specified patterns,
 * and/or ask the user for new name when -r is used.
 */

static
int
ckname()
{

	register int lastchar, save_Adir, save_Aspec, save_Alink, ret, res;
	register size_t len;
	struct archive_tree *component, *save_Arch_tree_p;
	struct gen_hdr *save_G_p;
	struct sec_hdr *save_S_p;

	if (G_p->g_namesz > Max_namesz) {
		msg(ERR, ":68", "Name exceeds maximum length - skipped.");
		if (Adir) {
			Arch_tree_p->copied = COPY_FAIL;
			msg(WARN, ":1006", "Descendants of directory \"%s\" will be silently skipped.", G_p->g_nam_p);
		}
		return(F_SKIP);
	}

	if (Pat_pp && !matched()) {
		if (Adir && !(Args & OCt)) {
			/* save the skipped directory's header info.  If a
			 * descendant of this directory matches a pattern,
			 * this dir will have to be restored, with the
			 * correct attributes, before the descendant
			 */
			if ((Arch_tree_p->hdrs = REC(headers)) == NULL)
				msg(EXT, ":643", "Out of memory: %s", "ckname()");
			Arch_tree_p->hdrs->sec = *S_p;	/* Structure copy */
			if ((Arch_tree_p->hdrs->sec.sh_acl_tbl = TBL(acl, S_p->sh_acl_num)) == NULL)
				msg(EXT, ":643", "Out of memory: %s", "ckname()");
			(void)memcpy(Arch_tree_p->hdrs->sec.sh_acl_tbl, S_p->sh_acl_tbl, S_p->sh_acl_num * sizeof(struct acl));

			Arch_tree_p->hdrs->gen = *G_p;	/* Structure copy */
			if ((Arch_tree_p->hdrs->gen.g_nam_p = strdup(G_p->g_nam_p)) == NULL)
				msg(EXT, ":643", "Out of memory: %s", "ckname()");

		}
		/* don't need to set Arch_tree_p->copied to COPY_FAIL here.
		 * Dir may be restored later if necessary.
		 */
		return(F_SKIP);
	}

	if ((Args & OCr) && !Adir) { /* rename interactively */
		(void)pfmt(Wtty_p, MM_NOSTD, ":69:Rename \"%s\"? ", G_p->g_nam_p);
		(void)fflush(Wtty_p);
		if (fgets(Renam_p, Max_namesz, Rtty_p) == (char *)NULL)
			msg(EXT, ":41", "Cannot read tty.");
		if (feof(Rtty_p))
			exit(Error_cnt);
		lastchar = strlen(Renam_p) - 1;
		if (*(Renam_p + lastchar) == '\n') /* remove trailing '\n' */
			*(Renam_p + lastchar) = '\0';
		if (*Renam_p == '\0') {
			msg(POST, ":912", "%s skipped.", G_p->g_nam_p);
			*G_p->g_nam_p = '\0';
			if (Arch_tree_p)
				Arch_tree_p->copied = COPY_FAIL;
			return(F_SKIP);
		} else if (strcmp(Renam_p, ".")) {
			G_p->g_nam_p = Renam_p;
		}
	}

        if (privileged && validate(G_p, S_p) == ERROR) {
		if (Adir) {
			Arch_tree_p->copied = COPY_FAIL;
			msg(WARN, ":1006", "Descendants of directory \"%s\" will be silently skipped.", G_p->g_nam_p);
		}
		return (F_SKIP);
	}

	VERBOSE((Args & OCt), G_p->g_nam_p);
	if (Args & OCt)
		return(F_SKIP);

	ret = F_EXTR;

	if (Pat_pp && G_p->g_nam_p != Renam_p) {
		/* before this file is restored, all of the directories in the
		 * path must be restored with the correct attributes.
		 */
		save_Adir = Adir; save_Aspec = Aspec; save_Alink = Alink;
		save_G_p = G_p;	save_S_p = S_p;
		save_Arch_tree_p = Arch_tree_p;
		Adir = 1; Aspec = 0; Alink = 0;
		(void) get_component(save_G_p->g_nam_p, &Arch_tree_p, save_Adir);
		res = get_component(NULL, &component, save_Adir);
		while (res) {
			if (Arch_tree_p->copied == COPY_SUCCEED)
				goto here;
			if (privileged && validate(&Arch_tree_p->hdrs->gen, &Arch_tree_p->hdrs->sec) == ERROR) {
				ret = F_SKIP;
				break;
			}
			G_p = &Arch_tree_p->hdrs->gen;
			S_p = &Arch_tree_p->hdrs->sec;
			VERBOSE((Args & (OCv | OCV)), G_p->g_nam_p);
			if (creat_spec() <= 0) {
				ret = F_SKIP;
				break;
			}
			Arch_tree_p->copied = COPY_SUCCEED;
here:
			Arch_tree_p = component;
			res = get_component(NULL, &component, save_Adir);
		}
		Adir = save_Adir; Aspec = save_Aspec; Alink = save_Alink;
		G_p = save_G_p;	S_p = save_S_p;
		Arch_tree_p = save_Arch_tree_p;
	}
	return(ret);
}

/*
 * Procedure:     ckopts
 *
 * Restrictions:
                 fopen: None
                 open(2): None
                 getpwnam: None
                 lvlin: None
                 lvlvalid: None
 * Notes: Check the validity of all command line options.
 */

static
void
ckopts(mask)
register long mask;
{
	register int oflag;
	register long errmsk;
	int res;

	if (mask & OCi)
		errmsk = mask & INV_MSK4i;
	else if (mask & OCo)
		errmsk = mask & INV_MSK4o;

        else {
		Serror_cnt++;
                errmsk = 0;
        }

	if ((mask & OCi) && !(mask & OCI))
                msg(ERR, ":913", "Must specify -I with -i option.");
	if ((mask & OCo) && !(mask & OCO))
                msg(ERR, ":914", "Must specify -O with -o option.");

	if (errmsk) /* if non-zero, invalid options were specified */
		Serror_cnt++;

	if ((mask & OCv) && (mask & OCV))
		msg(ERR, ":33", "-%c and -%c are mutually exclusive.", 'v', 'V');

	if (Bufsize <= 0)
		msg(ERR, ":74", "Illegal size given for -C option.");

	if (mask & OCr) {
		Rtty_p = fopen(Ttyname, "r");
		Wtty_p = fopen(Ttyname, "w");
		if (Rtty_p == (FILE *)NULL || Wtty_p == (FILE *)NULL)
			msg(ERR, ":76", "Cannot rename, \"%s\" missing", Ttyname);
	}
	if ((mask & OCE) && (Ef_p = fopen(Efil_p, "r")) == (FILE *)NULL)
		msg(ERRN, ":77", "Cannot open \"%s\" to read patterns", Efil_p);
	if (mask & OCI) {
	    if ((Archive = open(IOfil_p, O_RDONLY)) < 0)
		msg(ERRN, ":78", "Cannot open \"%s\" for input", IOfil_p);
	}
	if (mask & OCO) {
			oflag = (O_WRONLY | O_CREAT | O_TRUNC);
			if ((Archive = open(IOfil_p, oflag, 0666)) < 0)
				msg(ERRN, ":80", "Cannot open \"%s\" for output", IOfil_p);
	}
	if (mask & OCR) {
		if ((Rpw_p = getpwnam(Own_p)) == (struct passwd *)NULL)
			msg(ERR, ":82", "Unknown user id: %s", Own_p);
	}

	if (mask & OCT)
		if (!privileged)
			msg(ERR, ":891", "Must be privileged for %s option", "-T");
		else if ((Tf_p = fopen(Tfil_p, "r")) == (FILE *)NULL)
			msg(ERRN, ":915", "Cannot access translation table file \"%s\"", Tfil_p);

	if (mask & OCN) {
		if (!privileged)
			mask &= ~OCN;
		else if (isalpha(*Map_p)) {
			if (lvlin(Map_p, &Map_lvl) == ERROR)
				msg(ERR, ":916", "Specified level %s is invalid.", Map_p);
		} else if (isdigit(*Map_p)) {               
			Map_lvl = atol(Map_p);                  
			if ((res = lvlvalid(&Map_lvl)) == -1) 		
				msg(ERR, ":916", "Specified level %s is invalid.", Map_p);
			else if (res > 0 && !(Unverify & NO_ACT))
				msg(ERR, ":1005", "Specified level %s is inactive.", Map_p);
		} else                                       
			msg(ERR, ":916", "Specified level %s is invalid.", Map_p);
	}

	if (mask & OCX) {
	    char *lo, *hi, *ch;
	    int lobad = 0, hibad = 0;

	    lo = strtok(Xlohi_p, ",");
	    if ((hi = strtok(NULL, "")) == NULL) {
		    msg(ERR, ":917", "-X: syntax");
		    return;
	    }
	    while (isspace(*lo))
		    lo++;
	    while (isspace(*hi))
		    hi++;
	    for (ch = lo + strlen(lo) -1; isspace(*ch) && ch >= lo; ch--)
		    *ch = '\0';
	    for (ch = hi + strlen(hi) -1; isspace(*ch) && ch >= hi; ch--)
		    *ch = '\0';

	    if (isalpha(*lo)) {
	    	if (lvlin(lo, &Min_lvl) == ERROR)
			lobad++;
	    } else if (isdigit(*lo)) {
		Min_lvl = atol(lo);
		if (lvlvalid(&Min_lvl) == ERROR)
			lobad++;
	    } else						
		lobad++;

	    if (isalpha(*hi)) {
	    	if (lvlin(hi, &Max_lvl) == ERROR)
			hibad++;
	    } else if (isdigit(*hi)) {
		Max_lvl = atol(hi);
		if (lvlvalid(&Max_lvl) == ERROR)
			hibad++;
	    } else						
		hibad++;

	    if (lobad)
		    msg(ERR, ":918", "-X: Low level '%s' is invalid.", lo);

	    if (hibad)
		    msg(ERR, ":919", "-X: High level '%s' is invalid.", hi);

	    
	    /*
	     * Make sure the high level dominates the low level.
	     */
	    if (!(lobad || hibad) && lvldom(&Max_lvl, &Min_lvl) <= 0)
		msg(ERR, ":920", "-X: High level \"%s\" does not dominate low level \"%s\".",
		    hi, lo);

	}
}

/*
 * Procedure:     creat_hdr
 *
 * Restrictions:
                 open(2): None
                 read(2): None
 * notes: Fill in the generic header structure with the specific
 * header information based on the value of Hdr_type.
 */

static
int
creat_hdr()
{
	register ushort ftype;
	int rv;

	ftype = SrcSt.st_mode & Ftype;
	Adir = (ftype == S_IFDIR);
	Aspec = (ftype == S_IFBLK || ftype == S_IFCHR || ftype == S_IFIFO);
        Alink = (ftype == S_IFLNK);

        /* We want to check here if the file is really readable --
           If we wait till later it's to late and cpio will fail */

        if (!Adir && !Aspec && !Alink) {
                if ((Ifile = open(Gen.g_nam_p, O_RDONLY)) < 0) {
                        msg(ERRN, ":65", "Cannot open \"%s\"", Gen.g_nam_p);
                        return(0);
                }
                if ((rv = read(Ifile, &tbuffr[0], 1)) < 0) {
                        msg(ERRN, ":39", "Read error in \"%s\"", Gen.g_nam_p); 
                        close(Ifile);
                        return(0);
                }
        }

        Gen.g_magic = CMN_SEC;
	Gen.g_namesz = strlen(Gen.g_nam_p) + 1;
	Gen.g_uid = SrcSt.st_uid;
	Gen.g_gid = SrcSt.st_gid;
	Gen.g_dev = SrcSt.st_dev;
	Gen.g_ino = SrcSt.st_ino;
	Gen.g_mode = SrcSt.st_mode;
	Gen.g_mtime = SrcSt.st_mtime;
	Gen.g_nlink = SrcSt.st_nlink;
	if (ftype == S_IFREG || Alink)
		Gen.g_filesz = SrcSt.st_size;
	else
		Gen.g_filesz = 0L;
	Gen.g_rdev = SrcSt.st_rdev;
	return(1);
}

/*
 * Procedure:     creat_lnk
 *
 * Restrictions:
                 link(2): None
                 unlink(2): None
 * Notes: Create a link from the existing name1_p to name2_p.
 */

static
void
creat_lnk(name1_p, name2_p)
register char *name1_p, *name2_p;
{
	int i;

	errno = 0;

	for (i = 0; i < 2; i++) {
		if (!link(name1_p, name2_p)) {
			if (Args & OCv)
				(void)pfmt(Err_p, MM_NOSTD, ":89:%s linked to %s\n", name1_p, name2_p);
			VERBOSE((Args & (OCv | OCV)), name2_p);
			return;
		} else if (errno == EEXIST) {
			if (!(Args & OCu) && G_p->g_mtime <= DesSt.st_mtime) {
				msg(POST, ":46", "Existing \"%s\" same age or newer", name2_p);
				return;
			} else if (unlink(name2_p) < 0) {
				msg(ERRN, ":88", "Cannot unlink \"%s\"", name2_p);
				return;
			}
		} else
			break;
	}
	msg(ERRN, ":90", "Cannot link \"%s\" and \"%s\"", name1_p, name2_p);
}

/*
 * Procedure:     creat_spec
 *
 * Restrictions:
                 stat(2): None
                 mknod(2): None
*/

static
int
creat_spec()
{
	register char *nam_p;
	register int cnt, result, rv = 0;
	char *curdir;

        int lvl_changed = 0;    /* Set to 1 if the level is changed. */

	nam_p = G_p->g_nam_p;


	result = stat(nam_p, &DesSt);


	if (Adir) {
		Arch_tree_p->copied = COPY_SUCCEED;
		curdir = strrchr(nam_p, '.');
		if (curdir != NULL && curdir[1] == NULL)
			return(1);
		else {
			if (!result && (Args & OCd)) {
				rstfiles(U_KEEP);
				return(1);
			}
			if (!result || !(Args & OCd) || !strcmp(nam_p, ".") || !strcmp(nam_p, ".."))
				return(1);
		}
	} else if (!result && creat_tmp(nam_p) < 0)
		return(0);
	(void)umask(Orig_umask);
	if (Adir)
		result = sec_dir(nam_p, G_p->g_mode, S_p);
	else if (Aspec) {
		if (privileged && macpkg && adjust_lvl(S_p) == 0)
			lvl_changed = 1;                                
		
		
		result = mknod(nam_p, (int)G_p->g_mode, (int)G_p->g_rdev);
		
		if (lvl_changed)
			chg_lvl_back();                                 
	}
	(void)umask(0);
	if (result >= 0) {
		rv = 1;
		if (privileged && !macpkg)
			adjust_lvl_file(G_p, S_p);
		rstfiles(U_OVER);
	} else {
		if (Adir) {
			msg(ERRN, ":91", "Cannot create directory \"%s\"", nam_p);
			msg(WARN, ":1006", "Descendants of directory \"%s\" will be silently skipped.", nam_p);
			Arch_tree_p->copied = COPY_FAIL;
		} else if (Aspec)
			msg(ERRN, ":92", "mknod() failed for \"%s\"", nam_p);
		if (*Over_p != '\0')
			rstfiles(U_KEEP);
	}
	return(rv);
}

/*
 * Procedure:     creat_tmp
 *
 * Restrictions:
                 mktemp: None
                 rename(2): None
 */

static
int
creat_tmp(nam_p)
char *nam_p;
{
	register char *t_p;

	if (G_p->g_mtime <= DesSt.st_mtime && !(Args & OCu)) {
		msg(POST, ":46", "Existing \"%s\" same age or newer", nam_p);
		return(-1);
	}
	(void)strcpy(Over_p, nam_p);
	t_p = Over_p + strlen(Over_p);
	while (t_p != Over_p) {
		if (*(t_p - 1) == '/')
			break;
		t_p--;
	}
	(void)strcpy(t_p, "XXXXXX");
	(void)mktemp(Over_p);
	if (*Over_p == '\0') {
		msg(ERR, ":95", "Cannot get temporary file name.");
		return(-1);
	}

	if (rename(nam_p, Over_p) < 0) {
		msg(ERRN, ":96", "Cannot create temporary file");
		return(-1);
	}

	return(1);
}

/*
 * Procedure:     data_in
 *
 * Restrictions:
                 write(2): P_MACWRITE;P_DEV
 * Notes:  If proc_mode == P_PROC, bread() the file's data from the archive 
 * and write(2) it to the open fdes gotten from openout().  If proc_mode ==
 * P_SKIP, or becomes P_SKIP (due to errors etc), bread(2) the file's data
 * and ignore it.  If the user specified any of the "swap" options (b, s or S),
 * and the length of the file is not appropriate for that action, do not
 * perform the "swap", otherwise perform the action on a buffer by buffer basis.
 */

static
void
data_in(proc_mode)
register int proc_mode;
{
	register char *nam_p;

	register int cnt;
	register off_t pad;
	register long filesz = 0L;
	register int rv, swapfile = 0;

	nam_p = G_p->g_nam_p;
	if (Alink && proc_mode != P_SKIP) {
		proc_mode = P_SKIP;
		VERBOSE((Args & (OCv | OCV)), nam_p);
	}
	if (Args & (OCb | OCs | OCS)) { /* verfify that swapping is possible */
		swapfile = 1;
		if (Args & (OCs | OCb) && G_p->g_filesz % 2) {
			msg(ERR, ":98", "Cannot swap bytes of \"%s\", odd number of bytes", nam_p);
			swapfile = 0;
		}
		if (Args & (OCS | OCb) && G_p->g_filesz % 4) {
			msg(ERR, ":99", "Cannot swap halfwords of \"%s\", odd number of halfwords", nam_p);
			swapfile = 0;
		}
	}
	filesz = G_p->g_filesz;
	while (filesz > 0) {
		cnt = (int)(filesz > CPIOBSZ) ? CPIOBSZ : filesz;
		FILL(cnt);

		if (proc_mode != P_SKIP) {
			if (swapfile)
				swap(Buffr.b_out_p, cnt);
			errno = 0;
			rv = write(Ofile, Buffr.b_out_p, cnt);
			if (rv < cnt) {
				if (rv < 0)
	 				msg(ERRN, ":44", "Write error in \"%s\"", nam_p);
				else
					msg(EXTN, ":44", "Write error in \"%s\"", nam_p);
				proc_mode = P_SKIP;
				(void)close(Ofile);
				rstfiles(U_KEEP);
			}
		}
		Buffr.b_out_p += cnt;
		Buffr.b_cnt -= (long)cnt;
		filesz -= (long)cnt;
	} /* filesz */
	pad = (Pad_val + 1 - (G_p->g_filesz & Pad_val)) & Pad_val;
	if (pad != 0) {
		FILL(pad);
		Buffr.b_out_p += pad;
		Buffr.b_cnt -= pad;
	}
	if (proc_mode != P_SKIP) {
		(void)close(Ofile);

			rstfiles(U_OVER);
	}
	VERBOSE((proc_mode != P_SKIP && (Args & (OCv | OCV))), nam_p);
	Finished = 1;
}

/*
 * Procedure:     data_out
 *
 * Restrictions:
                 readlink(2): None
                 read(2): None
 * Notes:  open(2) the file to be archived.
 * read(2) each block of data and bwrite() it to the archive.  For TARTYP (TAR
 * and USTAR) archives, pad the data with NULLs to the next 512 byte boundary.
 */

static
void
data_out()
{
	register char *nam_p;
	register int cnt, rv;
	register off_t pad, filesz;

	nam_p = G_p->g_nam_p;
	if (Aspec) {
                write_hdr(S_p);
		rstfiles(U_KEEP);
		VERBOSE((Args & (OCv | OCV)), nam_p);
		return;
	}
	if (Alink) { /* symbolic link */
                write_hdr(S_p);
		FLUSH(G_p->g_filesz);
		errno = 0;

		if (readlink(nam_p, Buffr.b_in_p, G_p->g_filesz) < 0) {
			msg(ERRN, ":40", "Cannot read symbolic link \"%s\"", nam_p);

			return;
		}

		Buffr.b_in_p += G_p->g_filesz;
		Buffr.b_cnt += G_p->g_filesz;
		pad = (Pad_val + 1 - (G_p->g_filesz & Pad_val)) & Pad_val;
		if (pad != 0) {
			FLUSH(pad);
			(void)memset(Buffr.b_in_p, 0, pad);
			Buffr.b_in_p += pad;
			Buffr.b_cnt += pad;
		}
		VERBOSE((Args & (OCv | OCV)), nam_p);
		return;
	}

	write_hdr(S_p);

	filesz = G_p->g_filesz;

        /* We open the file in creat_hdr() and read one byte. If the
           filesz is zero then the file was zero bytes to start with
           and we don't need to lseek.  If it was one then we need to
           add that byte to the buffer.  We do this by lseek'ing to
           the start of the file and reading. */

        if (filesz > 0)
                if (Ifile > 0)
                        (void)lseek(Ifile, 0, 0);

	while (filesz > 0) {
		cnt = (unsigned)(filesz > CPIOBSZ) ? CPIOBSZ : filesz;
		FLUSH(cnt);
		errno = 0;

		if ((rv = read(Ifile, Buffr.b_in_p, (unsigned)cnt)) < 0) {

			msg(EXTN, ":39", "Read error in \"%s\"", nam_p);
			break;
		}

		Buffr.b_in_p += rv;
		Buffr.b_cnt += (long)rv;
		filesz -= (long)rv;
	}
	pad = (Pad_val + 1 - (G_p->g_filesz & Pad_val)) & Pad_val;
	if (pad != 0) {
		FLUSH(pad);
		(void)memset(Buffr.b_in_p, 0, pad);
		Buffr.b_in_p += pad;
		Buffr.b_cnt += pad;
	}
	(void)close(Ifile);
	rstfiles(U_KEEP);
	VERBOSE((Args & (OCv | OCV)), nam_p);
}


/*
 * file_in:  Process an object from the archive.
 */

static
void
file_in()
{
	register struct Lnk *l_p, *tl_p;

	int cleanup = 0;

	int proc_file;
	struct Lnk *ttl_p;

	G_p = &Gen;

	if (Adir) {
		if (ckname() != F_SKIP && creat_spec() > 0)
			VERBOSE((Args & (OCv | OCV)), G_p->g_nam_p);
		return;
	}

        if (G_p->g_nlink == 1) {

		if (Aspec) {
			if (ckname() != F_SKIP && creat_spec() > 0)
				VERBOSE((Args & (OCv | OCV)), G_p->g_nam_p);
		} else {
			if ((ckname() == F_SKIP) || (Ofile = openout()) < 0) {
				data_in(P_SKIP);
			}
			else {
				data_in(P_PROC);
			}
		}
		return;
	}
	tl_p = add_lnk(&ttl_p);
	l_p = ttl_p;
	if (l_p->L_cnt == l_p->L_gen.g_nlink)
		cleanup = 1;

	if (tl_p->L_gen.g_filesz)
	    cleanup = 1;
	if (!cleanup)
	    return; /* don't do anything yet */
	tl_p = l_p;
	while (tl_p != (struct Lnk *)NULL) {
	    G_p = &tl_p->L_gen;
	    if ((proc_file = ckname()) != F_SKIP) {
		if (l_p->L_data) {
		    creat_lnk(l_p->L_gen.g_nam_p, G_p->g_nam_p);
		} else if (Aspec) {
		    (void)creat_spec();
		    l_p->L_data = 1;
		    VERBOSE((Args & (OCv | OCV)), G_p->g_nam_p);
		} else if ((Ofile = openout()) < 0) {
		    proc_file = F_SKIP;
		} else {
		    data_in(P_PROC);
		    l_p->L_data = 1;
		}
	    } /* (proc_file = ckname()) != F_SKIP */
	    tl_p = tl_p->L_lnk_p;
	    if (proc_file == F_SKIP && !cleanup) {
		tl_p->L_nxt_p = l_p->L_nxt_p;
		tl_p->L_bck_p = l_p->L_bck_p;
		l_p->L_bck_p->L_nxt_p = tl_p;
		l_p->L_nxt_p->L_bck_p = tl_p;
		free(l_p->L_gen.g_nam_p);
		free(l_p);
	    }
	} /* tl_p->L_lnk_p != (struct Lnk *)NULL */
	    if (l_p->L_data == 0) {
		data_in(P_SKIP);
	    }
	if (cleanup)
	    reclaim(l_p);
}

/*
 * file_out:  If the current file is not a special file (!Aspec) and it
 * is identical to the archive, skip it (do not archive the archive if it
 * is a regular file).  If creating a TARTYP (TAR or USTAR) archive, the first time
 * a link to a file is encountered, write the header and file out normally.
 * Subsequent links to this file put this file name in their t_linkname field.
 * Otherwise, links are handled in one of two ways, for the old headers
 * (i.e. binary and -c), linked files are written out as they are encountered.
 * For the new headers (ASC and CRC), links are saved up until all the links
 * to each file are found.  For a file with n links, write n - 1 headers with
 * g_filesz set to 0, write the final (nth) header with the correct g_filesz
 * value and write the data for the file to the archive.
 */

static
void
file_out()
{
	register struct Lnk *l_p, *tl_p;
	register int cleanup = 0;
	struct Lnk *ttl_p;

	G_p = &Gen;

	if (!Aspec && IDENT(SrcSt, ArchSt))
		return; /* do not archive the archive if it's a regular file */
	if (Adir) {

                write_hdr(S_p);

		VERBOSE((Args & (OCv | OCV)), G_p->g_nam_p);
		return;
	}
	if (G_p->g_nlink == 1) {
		data_out();
		return;
	} else {
		tl_p = add_lnk(&ttl_p);
		l_p = ttl_p;
		if (l_p->L_cnt == l_p->L_gen.g_nlink)
			cleanup = 1;
		else {
                        if (Ifile > 0)
                                close(Ifile);
			return; /* don't process data yet */
		}
	}

	tl_p = l_p;
	while (tl_p->L_lnk_p != (struct Lnk *)NULL) {
		G_p = &tl_p->L_gen;
		G_p->g_filesz = 0L;

                write_hdr(S_p);

		VERBOSE((Args & (OCv | OCV)), G_p->g_nam_p);
		tl_p = tl_p->L_lnk_p;
	}
	G_p = &tl_p->L_gen;

	data_out();
	if (cleanup)
		reclaim(l_p);
}

/*
 * Procedure:     flush_lnks
 *
 * Restrictions:
                 stat(2): None
 * Notes: With new linked file handling, linked files are not archived
 * until all links have been collected.  When the end of the list of filenames
 * to archive has been reached, all files that did not encounter all their links
 * are written out with actual (encountered) link counts.  A file with n links 
 * (that are archived) will be represented by n headers (one for each link (the
 * first n - 1 have g_filesz set to 0)) followed by the data for the file.
 */

static
void
flush_lnks()
{
	register struct Lnk *l_p, *tl_p;
	long tfsize;

	l_p = Lnk_hd.L_nxt_p;
	while (l_p != &Lnk_hd) {
		(void)strcpy(Gen.g_nam_p, l_p->L_gen.g_nam_p);


		if (stat(Gen.g_nam_p, &SrcSt) == 0) { /* check if file exists */

			tl_p = l_p;
			(void)creat_hdr();
			Gen.g_nlink = l_p->L_cnt; /* "actual" link count */
			tfsize = Gen.g_filesz;
			Gen.g_filesz = 0L;
			G_p = &Gen;
			while (tl_p != (struct Lnk *)NULL) {
				Gen.g_nam_p = tl_p->L_gen.g_nam_p;
				Gen.g_namesz = tl_p->L_gen.g_namesz;
				if (tl_p->L_lnk_p == (struct Lnk *)NULL) {
					Gen.g_filesz = tfsize;
					data_out();
					break;
				}

                                write_hdr(S_p);

				VERBOSE((Args & (OCv | OCV)), Gen.g_nam_p);
				tl_p = tl_p->L_lnk_p;
			}
			Gen.g_nam_p = Nam_p;
		} else /* stat(Gen.g_nam_p, &SrcSt) == 0 */
			msg(ERR, ":773", "\"%s\" has disappeared", Gen.g_nam_p);


		tl_p = l_p;
		l_p = l_p->L_nxt_p;
		reclaim(tl_p);
	} /* l_p != &Lnk_hd */
}

/*
 * in_ttoctt:  Determine whether the Sec and Gen hdrs refer to IDs that
 *	       are mapped in the TTOCTT.
 *
 *  Returns 1 if in the TTOCTT, 0 if not.
 */

static
int
in_ttoctt()
	{
	ENTRY litem, *ep;
	vs item;
	VS vsp;
	VP tent;
	short indx;
	struct acl *ap;

	if (macpkg) {
		/* See if the LID in S_p has been mapped */
		litem.key = (char *)malloc((size_t)MAX_NUM_SZ);
		sprintf(litem.key, "%d", S_p->sh_level);
		if ((ep = hsearch(litem, FIND)) != NULL) {
			free(litem.key);
			vsp = (VS)ep->data;
			if (vsp->vs_state & VS_MAPPED)
				return(1);
		}
		free(litem.key);
	}

	/* See if the UID in G_p has been mapped */
	tent = loc_ttoc(UI);
	item.vs_value = G_p->g_uid;
	vsp = (VS)bsearch(&item, tent->v_map, tent->v_nelm, sizeof(vs), gt);
	if (vsp != NULL)
		if (vsp->vs_state & VS_MAPPED)
	    		return(1);

	/* See if the GID in G_p has been mapped */
	tent = loc_ttoc(GI);
	item.vs_value = G_p->g_gid;
	vsp = (VS)bsearch(&item, tent->v_map, tent->v_nelm, sizeof(vs), gt);
	if (vsp != NULL)
		if (vsp->vs_state & VS_MAPPED)
	    		return(1);

	/* Check all UIDs and GIDs on the ACL associated with the file
	 * to see if they are referenced in the TTOCTT.
	 */
	for (indx = S_p->sh_acl_num, ap = S_p->sh_acl_tbl; indx; indx--, ap++) {
        	switch (ap->a_type) {

        	case USER:
        	case DEF_USER:
    	    		tent = loc_ttoc(UI);
    	    		item.vs_value = ap->a_id;
    	    		vsp = (VS)bsearch(&item, tent->v_map, tent->v_nelm, sizeof(vs), gt);
    	    		if (vsp != NULL)
				if (vsp->vs_state & VS_MAPPED)
	    	    			return(1);
            		break;

        	case GROUP:
        	case DEF_GROUP:
            		tent = loc_ttoc(GI);
    	    		item.vs_value = ap->a_id;
            		vsp = (VS)bsearch(&item, tent->v_map, tent->v_nelm, sizeof(vs), gt);
            		if (vsp != NULL)
	        		if (vsp->vs_state & VS_MAPPED)
	            			return(1);
            		break;
	
        	case USER_OBJ:
        	case GROUP_OBJ:
		case CLASS_OBJ:
        	case OTHER_OBJ:
        	case DEF_USER_OBJ:
        	case DEF_GROUP_OBJ:
        	case DEF_CLASS_OBJ:
        	case DEF_OTHER_OBJ:
            		break;
        	default:
            		msg(ERR, ":921", "Impossible case: %s", "in_ttoctt()");
        	}  /* switch */
	} /* for */
	return(0);
} /* in_ttoctt */



/*
 * gethdr: Get a header from the archive, validate it and check for the trailer.
 * Any user specified Hdr_type is ignored (set to NONE in main).  Hdr_type is 
 * set appropriately after a valid header is found.  Unless the -k option is 
 * set a corrupted header causes an exit with an error.  I/O errors during 
 * examination of any part of the header cause gethdr to throw away any current
 * data and start over.  Other errors during examination of any part of the 
 * header cause gethdr to advance one byte and continue the examination.
 *
 * Returns 0 if the TRAILER!!! was encountered
 *         1 if the file should be read in
 * 	  -1 if the file was skipped
 */

static
int
gethdr()
{
	register ushort ftype;
	register int hit = NONE, cnt = 0;
        int goodhdr, res;
        off_t hsize, pad, nbyts;
	struct archive_tree *component;

        G_p = &Gen;
	Gen.g_nam_p = Nam_p;
	do { /* hit == NONE && (Args & OCk) && Buffr.b_cnt > 0 */
		FILL(Hdrsz);
                if (!strncmp(Buffr.b_out_p, CMS_SEC, CMS_LEN)) {
                    hit = read_hdr();
                    hsize = GENSZ + Gen.g_namesz;
		    FILL(hsize);
                    break;
                }
		Gen.g_nam_p = &nambuf[0];
		if (hit != NONE) {
			FILL(hsize);
			goodhdr = 1;
			if (Gen.g_filesz < 0L || Gen.g_namesz < 1)
				goodhdr = 0;
				if (Gen.g_nlink <= (ulong)0)
					goodhdr = 0;
				if (*(Buffr.b_out_p + hsize - 1) != '\0')
					goodhdr = 0;
			if (!goodhdr) {
				hit = NONE;
				if (!(Args & OCk))
					break;
				msg(ERR, ":922", "Corrupt header, files may be lost.");
			} else {
				FILL(hsize);
			}
		} /* hit != NONE */
		if (hit == NONE) {
			Buffr.b_out_p++;
			Buffr.b_cnt--;
			if (!(Args & OCk))
				break;
			if (!cnt++)
				msg(ERR, ":108", "Searching for magic number/header.");
		}
	} while (hit == NONE);
	if (hit == NONE) {
		msg(EXT, ":110", "Bad magic number/header.");
	} else if (cnt > 0) {
		msg(EPOST, ":111", "Re-synchronized on magic number/header.");
	}

	(void)memcpy(Gen.g_nam_p, Buffr.b_out_p + Hdrsz, Gen.g_namesz);
	if (!strcmp(Gen.g_nam_p, "TRAILER!!!"))
	    return(0);

        /* padding only at end of security header */
        Buffr.b_out_p += hsize;
        Buffr.b_cnt -= hsize;

	ftype = Gen.g_mode & Ftype;
	Adir = (ftype == S_IFDIR);
	Aspec = (ftype == S_IFBLK || ftype == S_IFCHR || ftype == S_IFIFO);
	Alink = (ftype == S_IFLNK);

        if ((nbyts = read_shdr(S_p, G_p)) <= 0) {
	    /* error messages printed in read_shdr */
            skip_file(-nbyts, 1);  /* bad hdr, attempt to skip hdr + file */
            return(-1); 
        } else {
            hsize += nbyts;
            pad = (Pad_val + 1 - (hsize & Pad_val)) & Pad_val;

            Buffr.b_out_p += pad;
            Buffr.b_cnt -= pad;

	    /*
	     * If any components of the filename were not restored
	     * successfully, skip this file.
	     */
	    (void) get_component(Gen.g_nam_p, &Arch_tree_p, Adir);
	    res = get_component(NULL, &component, Adir);
	    while (res) {
		    switch (Arch_tree_p->copied) {
		    case NOT_COPIED:
			    /* if the hdrs field is not NULL, that means that
			     * this dir was only skipped because is didn't
			     * match a pattern supplied on the command line.
			     * If the current file does match a pattern, this
			     * dir will be restored at that time.
			     *
			     * Also, for the -t option, the dir would not
			     * actually get created, so the copied field would
			     * not have been set to COPY_SUCCEED.
			     */
			    if (Arch_tree_p->hdrs != NULL || Args & OCt)
				    break;

			    /* if we get here, it's because this dir hasn't
			     * been encountered on the archive.  Should never
			     * happen, but perhaps part of the archive was
			     * corrupted and stuff had to be skipped.
			     */

			    /* get the full path name restored */
			    while (get_component(NULL, &component, Adir))
				    ;
			    msg(ERR, ":1007", "A directory in the path of %s could not be found in archive, skipping.", Gen.g_nam_p);
			    /* FALL THROUGH!! */
		    case COPY_FAIL:
			    /* For case COPY_FAIL, no need to print an error
			     * message here.  Either an error message was
			     * printed at the time the failure occured, or the
			     * failure was because the dir was out of range, in
			     * which case stuff is supposed to be skipped
			     * silently.
			     */
			    skip_file(G_p->g_filesz, 1);
			    return (-1);
		    case COPY_SUCCEED:
			    break;
		    default:
			    msg(ERR, ":921", "Impossible case: %s", "gethdr()");
		    }
		    Arch_tree_p = component;
		    res = get_component(NULL, &component, Adir);
	    }
	    /* If -X was specified, check that this file is in range. */
	    if ((Args & OCX) && !in_range(S_p->sh_level, Min_lvl, Max_lvl)) {
		    skip_file(G_p->g_filesz, 1);
		    if (Arch_tree_p)
			    Arch_tree_p->copied = COPY_FAIL;
		    return (-1);
	    }
        }

	/*
	 * If -T was used and if none of the IDs associated with 
	 * this file were referenced in the TTOCTT, then skip 
	 * the file.
	 */
	if ((Args & OCT) && !in_ttoctt())  {
            skip_file(G_p->g_filesz, 1);  
	    return(-1);
	}

	return(1);
}

/*
 * inter_names:  Generate intermediate pathnames.
 *  
 *  Called from main() to generate intermediate pathname.  For each path
 *  name read from stdin, it breaks up the path name
 *  so that the intermediate directories are explicitly represented.
 *  E.g., the pathname:
 *			/dir1/dir2/file1
 *  would be broken into:
 *			/
 *			/dir1
 *			/dir1/dir2
 *			/dir1/dir2/file1
 * 	
 *  It does this by calling get_component() repeatedly to get successive
 *  components of the path.  If the component returned is not on the archive
 *  yet, it is put there by calling getname().
 */

static
void
inter_names(doarch)
int *doarch;
{
	char *lastchar;
	struct archive_tree *component;
	int failure, isdir, gotcomp;
	struct stat stbuf;

    	Gen.g_nam_p = Nam_p;
    	while (1) {
		if (fgets(Gen.g_nam_p, Max_namesz, stdin) == (char *)NULL) {
			flush_lnks();
	    		return;		/* done with input filename list */
		}
		lastchar = Gen.g_nam_p + strlen(Gen.g_nam_p) - 1;
		if (*lastchar == '\n')
	    		*lastchar = '\0';

		isdir = !lstat(Gen.g_nam_p, &stbuf) && (stbuf.st_mode & S_IFDIR);

		gotcomp = get_component(Gen.g_nam_p, &component, isdir);

		failure = 0;
		while(gotcomp && !failure) {
			if (!component || component->copied == NOT_COPIED) {
				if (getname(S_p) == 1) {
					if (component)
						component->copied = COPY_SUCCEED;
					file_out();
					(*doarch)++;
				} else {
					if (component)
						component->copied = COPY_FAIL;
					failure = 1;
				}
			} else if (component->copied == COPY_FAIL) {
				failure = 1;
			}
			gotcomp = get_component(NULL, &component, isdir);
		}
		/* in the case where a file was not backed up becuase one of
		 * the directories in its path could not be backed up, print
		 * an appropriate message.
		 */
		if (failure && gotcomp != NULL) {
			/* restore the full path name */
			while (get_component(NULL, &component, isdir))
				/* do nothing */;
	    		msg(WARN, ":1001", "A directory in the path could not be archived, \"%s\" skipped", Gen.g_nam_p);
		}

    	} /* while */
} /* inter_names */

/*
 * Procedure:     getname
 *
 * Restrictions:
                 lstat(2): None
                 acl(2): None
                 lvlvalid: None
                 stat(2): None
                 setpwent: None
                 setgrent: None
                 getpwuid: None
                 getgrgid: None
                 filepriv(2): None
 * notes: Get file names for inclusion in the archive.  When end of file
 * on the input stream of file names is reached, flush the link buffer out.
 * For each filename, remove leading "./"s and multiple "/"s, and remove
 * any trailing newline "\n".  Verify the existance of the file.
 * Get the file level and check it is within the range specified
 * with -X.  If not, return 0 to skip the file.
 * Get and validate the acl.
 * Finally, get file privilege information.
 * 
 * Call creat_hdr() to fill in the gen_hdr structure.
 * Check the validity of level, group, and owner IDs and set flags
 * in the security header appropriately.
 *
 * Returns 0 - don't back up file (either errors in getting file info,
 *             or file does not meet specified criteria
 *         1 - file OK, back it up
 *         2 - creat_hdr failed, can't back up file
 */

static
int
getname(sp)
struct sec_hdr *sp;
{
	register int goodfile = 0;


	if (!lstat(Gen.g_nam_p, &SrcSt)) {


		sp->sh_level = SrcSt.st_level;

		/* 
		 * If -X was specified,
		 * check that the level of this file is within
		 * the archive range, specified with -X.
		 */
		if ((Args & OCX) && !in_range(sp->sh_level, Lo_lvl, Hi_lvl)) {
			msg(WARN, ":923", "%s file level not within archive level range. File not archived.", Gen.g_nam_p);
			return(0);
		}


		if (!aclpkg || (SrcSt.st_mode & Ftype) == S_IFLNK)
			sp->sh_acl_num = 0;
		else if ((sp->sh_acl_num = acl(Gen.g_nam_p, ACL_CNT, 0, NULL)) == ERROR) {
	    	        msg(ERRN, ":924", "Cannot count the ACL entries for \"%s\", skipping", Gen.g_nam_p);
	    	        sp->sh_acl_num = 0;
        

			return(0);
		}


		if (sp->sh_acl_num > aclmax) {
			msg(ERR, ":925", "Too many ACLs for file \"%s\", skipping", Gen.g_nam_p);
		        sp->sh_acl_num = 0;
			return(0);
		}

		if (sp->sh_acl_num <= 0) {
		        sp->sh_acl_num = 0;
		        goto no_acl;
		}

		sp->sh_acl_tbl = aclbufp;
        
	    	if (acl(Gen.g_nam_p, ACL_GET, sp->sh_acl_num,
				      sp->sh_acl_tbl) == ERROR) {
		        msg(EPOST, ":926", "Cannot read ACLs of <%s>\n", Gen.g_nam_p);
			return(0);
		}
        

       	no_acl:

		sp->sh_flags = 0;

                /*
                 * Check the validity of the level and mark as
                 * valid-inactive if appropriate.
                 * This is done so files with v-i LIDs are restored
                 * if backed up that way.
                 */
                if (lvlvalid(&(sp->sh_level)) > 0)
                	sp->sh_flags |= O_VLDIN;

                /* Mark as an MLD if appropriate */
		sp->sh_flags |= SrcSt.st_flags;

		goodfile = 1;
		if ((SrcSt.st_mode & Ftype) == S_IFLNK && (Args & OCL)) {
			errno = 0;
			if (stat(Gen.g_nam_p, &SrcSt) < 0) {
				msg(ERRN, ":38", "Cannot follow \"%s\"", Gen.g_nam_p);
				goodfile = 0;
			}
		}
	} else {
		msg(ERRN, ":34", "Cannot access \"%s\"", Gen.g_nam_p);
	}


	if (!goodfile) return(0);

	if (creat_hdr()) {
                /*
                 * Check the validity the UID and GID as above for LID
                 * and mark as invalid if appropriate.
                 * This is done so files with invalid IDs are restored
                 * if backed up that way.
                 */
		setpwent();
		setgrent();

		if (getpwuid(Gen.g_uid) == NULL) {
                    sp->sh_flags |= O_UINVLD;
		}

		if (getgrgid(Gen.g_gid) == NULL) {
                    sp->sh_flags |= O_GINVLD;
		}
 		/* initialize file privilege info */
 		sp->sh_fpriv_num = 0;
 		/* if it is an executable file and not a symlink,
		 * check for privs.
		 * unless no priv mapping info was saved.
		 */
 		if (M_p->mh_numsets > 0  &&  
		    Gen.g_mode & S_IFREG && !Alink &&
 		    (Gen.g_mode & S_IXOTH || Gen.g_mode & S_IXUSR 
 		     || Gen.g_mode & S_IXGRP))
 		  if ((sp->sh_fpriv_num = 
		       filepriv(Gen.g_nam_p, 
				GETPRV, 
				pvec, 
				NPRIVS)) 
		      == ERROR) {
 		    msg(WARN, ":909", "Cannot retrieve file privileges for \"%s\"", Gen.g_nam_p);
 		  }
	      	return(1);
	      }
	else 
		return(2);
}

/*
 * getpats: Save any filenames/patterns specified as arguments.
 * Read additional filenames/patterns from the file specified by the
 * user.  The filenames/patterns must occur one per line.
 */

static
void
getpats(largc, largv)
int largc;
register char **largv;
{
	register char **t_pp;
	register int len;
	register unsigned numpat = largc, maxpat = largc + 2;
	
	if ((Pat_pp = (char **)malloc(maxpat * sizeof(char *))) == (char **)NULL)
		msg(EXT, ":643", "Out of memory: %s", "getpats()");
	t_pp = Pat_pp;
	while (*largv) {
		if ((*t_pp = (char *)malloc((unsigned int)strlen(*largv) + 1)) == (char *)NULL)
			msg(EXT, ":643", "Out of memory: %s", "getpats()");
		(void)strcpy(*t_pp, *largv);
		t_pp++;
		largv++;
	}
	while (fgets(Nam_p, Max_namesz, Ef_p) != (char *)NULL) {
		if (numpat == maxpat - 1) {
			maxpat += 10;
			if ((Pat_pp = (char **)realloc((char *)Pat_pp, maxpat * sizeof(char *))) == (char **)NULL)
				msg(EXT, ":643", "Out of memory: %s", "getpats()");
			t_pp = Pat_pp + numpat;
		}
		len = strlen(Nam_p); /* includes the \n */
		*(Nam_p + len - 1) = '\0'; /* remove the \n */
		*t_pp = (char *)malloc((unsigned int)len);
		if(*t_pp == (char *) NULL)
			msg(EXT, ":643", "Out of memory: %s", "getpats()");
		(void)strcpy(*t_pp, Nam_p);
		t_pp++;
		numpat++;
	}
	*t_pp = (char *)NULL;
}

/*
 * Procedure:     ioerror
 *
 * Restrictions:
                 stat(2): None
*/
static
void
ioerror(dir)
register int dir;
{
	register int t_errno;

	t_errno = errno;
	errno = 0;

	if (fstat(Archive, &ArchSt) < 0) {
		msg(EXTN, ":35", "Cannot access the archive");
	}
	errno = t_errno;
	if ((ArchSt.st_mode & Ftype) != S_IFCHR) {
		if (dir) {
			if (errno == EFBIG)
				msg(EXT, ":113", "ulimit reached for output file.");
			else if (errno == ENOSPC)
				msg(EXT, ":114", "No space left for output file.");
			else
				msg(EXTN, ":115", "I/O error - cannot continue");
		} else
			msg(EXT, ":116", "Unexpected end-of-file encountered.");
	} else if (dir)
		msg(EXTN, ":117", "\007I/O error on output");
	else
		msg(EXTN, ":118", "\007I/O error on input");
}

/*
 * matched: Determine if a filename matches the specified pattern(s).  If the
 * pattern is matched (the first return), return 0 if -f was specified, else
 * return 1.  If the pattern is not matched (the second return), return 0 if
 * -f was not specified, else return 1.
 */

static
int
matched()
{
	register char *str_p = G_p->g_nam_p;
	register char **pat_pp = Pat_pp;

	while (*pat_pp) {
		if ((**pat_pp == '!' && !gmatch(str_p, *pat_pp + 1)) || gmatch(str_p, *pat_pp))
			return(!(Args & OCf)); /* matched */
		pat_pp++;
	}
	return(Args & OCf); /* not matched */
}

/*
 * msg: Print either a message (no error) (POST), an error message with or 
 * without the errno (ERRN or ERR), or print an error message with or without
 * the errno and exit (EXTN or EXT).
 */

void
/*VARARGS*/
#ifdef __STDC__
msg(int severity, ...)
#else
msg(va_alist)
va_dcl
#endif
{
	register char *fmt_p, *fmt_pid;
#ifndef	__STDC__
	register int severity;
#endif
	register FILE *file_p;
	va_list v_Args;
	int save_errno;

	save_errno = errno;
	if ((Args & OCV) && Verbcnt) { /* clear current line of dots */
		(void)fputc('\n', Out_p);
		Verbcnt = 0;
	}
#ifdef __STDC__
	va_start(v_Args, severity);
#else
	va_start(v_Args);
	severity = va_arg(v_Args, int);
#endif
	if ((Args & OCx) && (severity == WARN || severity == WARNN))
		return;
	if (severity == POST)
		file_p = Out_p;
	else
		if (severity == EPOST || severity == WARN || severity == WARNN)
			file_p = Err_p;
		else {
			file_p = Err_p;
			Error_cnt++;
		}
	fmt_pid = va_arg(v_Args, char *);
	fmt_p = va_arg(v_Args, char *);
	(void)fflush(Out_p);
	(void)fflush(Err_p);
	switch (severity) {
	case EXT:
	case EXTN:
		(void)pfmt(file_p, MM_HALT|MM_NOGET, "");
		break;
	case ERR:
	case ERRN:
		(void)pfmt(file_p, MM_ERROR|MM_NOGET, "");
		break;
	case WARN:
	case WARNN:
		(void)pfmt(file_p, MM_WARNING|MM_NOGET, "");
		break;
	case POST:
	case EPOST:
	default:
		break;
	}
	(void)vfprintf(file_p, gettxt(fmt_pid, fmt_p), v_Args);
	if (severity == ERRN || severity == EXTN || severity == WARNN) {
		(void)pfmt(file_p, MM_NOSTD|MM_NOGET, ":  errno %d, %s", save_errno, strerror(save_errno));
	}
	/* The Eom (End of medium) message is a prompt and expects a response.
	 * Therefore, do not print a newline in this case.
	 */
	if (fmt_pid != Eom_pid)
		(void)fprintf(file_p, "\n");
	(void)fflush(file_p);
	va_end(v_Args);
	if (severity == EXT || severity == EXTN) {
		if (Error_cnt == 1)
			(void)pfmt(file_p, MM_NOSTD, ":121:1 error\n");
		else
			(void)pfmt(file_p, MM_NOSTD, ":122:%d errors\n", Error_cnt);
		exit(Error_cnt);
	}
}

/*
 * Procedure:     openout
 *
 * Restrictions:
                 lstat(2): None
                 symlink(2): None
                 unlink(2): None
                 creat(2): None
                 lchown(2): None
                 chown(2): None
 * notes: Open files for output and set all necessary information.
 * If the u option is set (unconditionally overwrite existing files),
 * and the current file exists, get a temporary file name from mktemp(3C),
 * link the temporary file to the existing file, and remove the existing file.
 * Finally either creat(2), mkdir(2) or mknod(2) as appropriate.
 * 
 */

static
int
openout()
{
	register char *nam_p;
	register int result;
        int lvl_changed = 0;    /* Set to 1 if the level is changed. */

	nam_p = G_p->g_nam_p;
	if (Max_filesz < (G_p->g_filesz >> 9)) { /* / 512 */
		msg(ERR, ":123", "Skipping \"%s\": exceeds ulimit by %d bytes",
			nam_p, G_p->g_filesz - (Max_filesz << 9)); /* * 512 */
		return(-1);
	}

        if (privileged && macpkg && adjust_lvl(S_p) == 0)
            lvl_changed = 1;                                         

	if (!lstat(nam_p, &DesSt) && creat_tmp(nam_p) < 0) {

		return(-1);
	}

	(void)umask(Orig_umask);
	errno = 0;

	if (Alink) {

		(void)strncpy(Symlnk_p, Buffr.b_out_p, G_p->g_filesz);
		*(Symlnk_p + G_p->g_filesz) = '\0';

		if ((result = symlink(Symlnk_p, nam_p)) >= 0) {
			if (*Over_p != '\0') {
				(void)unlink(Over_p);
				*Over_p = '\0';
			}
		}
	} else
		result = creat(nam_p, (int)G_p->g_mode);


	(void)umask(0);
	if (result >= 0) {
		if (privileged) {
			/* This next confusing if statement says, if the file
			 * is a symlink, do a lchown().  If it's not a symlink,
			 * do a chown().  In either case (chown or lchown) if
			 * it returns a negative, print an error.
			 */
			if ((Alink ?
			    lchown(nam_p, (int)G_p->g_uid, (int)G_p->g_gid) :
			    chown(nam_p, (int)G_p->g_uid, (int)G_p->g_gid)) < 0)
				msg(ERRN, ":52", "chown() failed on \"%s\"", nam_p);
			/* if MAC is not currently running, we have to adjust
			 * the file level via lvlfile() (called from
			 * adjust_lvl_file()).  To be *really* secure, we must
			 * change the level *before* we write anything to the
			 * file.  If we waited until after the contents of the
			 * file was restored to change the level, there would
			 * be a window in which the contents of the file would
			 * be insecure.  This would be especially bad if tcpio
			 * was killed before it got to the level adjustment.
			 * So, to avoid this, we change the level here.  Note
			 * that is is necessary to close the file before
			 * changing the level, since a file must be "tranquil"
			 * to change its level.
			 */
			if (!macpkg && !Alink) {
				(void)close(result);
				adjust_lvl_file(G_p, S_p);
				if ((result = open(nam_p, O_WRONLY)) < 0)
					msg(ERRN, ":1003", "Cannot reopen file \"%s\" after setting level.", nam_p);
			}
		}
	} else {
		msg(ERRN, ":36", "Cannot create \"%s\"", nam_p);
	}
	Finished = 0;

        if (lvl_changed)                                           
            chg_lvl_back();                                        

	return(result);
}

/*
 * read_hdr: Transfer headers from the selected format 
 * in the archive I/O buffer to the generic structure.
 */

static
int
read_hdr()

{
	register int rv = NONE;
	major_t maj, rmaj;
	minor_t min, rmin;
	char tmpnull;

	if (Buffr.b_end_p != (Buffr.b_out_p + Hdrsz)) {
        	tmpnull = *(Buffr.b_out_p + Hdrsz);
        	*(Buffr.b_out_p + Hdrsz) = '\0';
	}

        if (sscanf(Buffr.b_out_p,
                        "%6lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8x%8x%8x%8x%8x",
                        &Gen.g_magic, &Gen.g_ino, &Gen.g_mode,
                        &Gen.g_uid, &Gen.g_gid,
                        &Gen.g_nlink, &Gen.g_mtime, &Gen.g_filesz,
                        &maj, &min, &rmaj, &rmin,
                        &Gen.g_namesz) == ASC_CNT) {
            Gen.g_dev = makedev(maj, min);
            Gen.g_rdev = makedev(rmaj, rmin);
            rv = SEC;
        }

	if (Buffr.b_end_p != (Buffr.b_out_p + Hdrsz))
		*(Buffr.b_out_p + Hdrsz) = tmpnull;
	return(rv);
}

/*
 * reclaim: Reclaim linked file structure storage.
 */

static
void
reclaim(l_p)
register struct Lnk *l_p;
{
	register struct Lnk *tl_p;
	
	l_p->L_bck_p->L_nxt_p = l_p->L_nxt_p;
	l_p->L_nxt_p->L_bck_p = l_p->L_bck_p;
	while (l_p != (struct Lnk *)NULL) {
		tl_p = l_p->L_lnk_p;
		free(l_p->L_gen.g_nam_p);
		free(l_p);
		l_p = tl_p;
	}
}

/*
 * rstbuf: Reset the I/O buffer, move incomplete potential headers to
 * the front of the buffer and force bread() to refill the buffer.  The
 * return value from bread() is returned (to identify I/O errors).  On the
 * 3B2, reads must begin on a word boundary, therefore, with the -i option,
 * any remaining bytes in the buffer must be moved to the base of the buffer
 * in such a way that the destination locations of subsequent reads are
 * word aligned.
 */

void
rstbuf()
{
 	register int pad;

	if ((Args & OCi) || Append) {
        	if (Buffr.b_out_p != Buffr.b_base_p) {
			pad = ((Buffr.b_cnt + FULLWD) & ~FULLWD);
			Buffr.b_in_p = Buffr.b_base_p + pad;
			pad -= Buffr.b_cnt;
			(void)memcpy(Buffr.b_base_p + pad, Buffr.b_out_p, (int)Buffr.b_cnt);
      			Buffr.b_out_p = Buffr.b_base_p + pad;
		}
		if (bfill() < 0)
			msg(EXT, ":124", "Unexpected end-of-archive encountered.");
	} else { /* OCo */
		(void)memcpy(Buffr.b_base_p, Buffr.b_out_p, (int)Buffr.b_cnt);
		Buffr.b_out_p = Buffr.b_base_p;
		Buffr.b_in_p = Buffr.b_base_p + Buffr.b_cnt;
	}
}

/*
 * Procedure:     rstfiles
 *
 * Restrictions:
                 unlink(2): None
                 rename(2): None
                 chown(2): None
 * notes:  Perform final changes to the file.  If the -u option is set,
 * and overwrite == U_OVER, remove the temporary file, else if overwrite
 * == U_KEEP, unlink the current file, and restore the existing version
 * of the file.  In addition, where appropriate, set the access or modification
 * times, change the owner and change the modes of the file.
 */

static
void
rstfiles(over)
register int over;
{
	register char *nam_p;
	int lvl_changed = 0;


	/* Must be at correct level before trying to restore file.  (The only
	 * reason this would be necessary is in the case of the tcpio process
	 * being in virtual mode, and the restored file having an MLD is it's
	 * path.  Being at the right level will cause deflection to the correct
	 * effective dir of the MLD.)
	 */
	if (privileged && macpkg && adjust_lvl(S_p) == 0)
		lvl_changed = 1;

	if (Gen.g_nlink > (ulong)0) 
		nam_p = G_p->g_nam_p;
	else
		nam_p = Gen.g_nam_p;

	if (over == U_KEEP && *Over_p != '\0') {
		msg(POST, ":127", "Restoring existing \"%s\"", nam_p);
		(void)unlink(nam_p);
		if (rename(Over_p, nam_p) < 0) {
			msg(EXTN, ":55", "Cannot recover original \"%s\"", nam_p);
		}
		*Over_p = '\0';
		return;
	} else if (over == U_OVER && *Over_p != '\0') {
		if (unlink(Over_p) < 0)
			msg(ERRN, ":43", "Cannot remove temp file \"%s\"", Over_p);
		*Over_p = '\0';
	}

        if (Args & OCi) {
		set_sec(G_p, S_p);
		set_tym(nam_p, G_p->g_mtime, G_p->g_mtime);
	} else if (Args & OCa) {
		set_tym(nam_p, SrcSt.st_atime, SrcSt.st_mtime);
	}

	if (Args & OCR) {
		if (chown(nam_p, Rpw_p->pw_uid, Rpw_p->pw_gid) < 0)
			msg(ERRN, ":52", "chown() failed on \"%s\"", nam_p);
	} else if ((Args & OCi) && privileged) {
                if (chown(nam_p, (uid_t)G_p->g_uid, (gid_t)G_p->g_gid) < 0)
			msg(ERRN, ":52", "chown() failed on \"%s\"", nam_p);
        }

	if (lvl_changed)
		chg_lvl_back();                                 

}

/*
 * Procedure:     setup
 *
 * Restrictions:
                 secsys(2): None
                 getpwuid: None
                 getcwd: None
                 stat(2): None
                 g_init: None
                 ulimit(2): None
 * notes:  Perform setup and initialization functions.  Parse the options
 * using getopt(3C), call ckopts to check the options and initialize various
 * structures and pointers.  Specifically, for the -i option, save any
 * patterns, and for the -o option, check (via stat(2)) the archive.
 */

static
void
setup(largc, largv)
register int largc;
register char **largv;
{
	extern int optind;
	extern char *optarg;
        register char   *opts_p = "abdfikPn:orstuvxC:E:I:LM:N:O:R:ST:VX:",
			*opts_p_nomac = "abdfikn:orstuvxC:E:I:LM:O:R:ST:V",
			*dupl_p = "Only one occurrence of -%c allowed",
			*dupl_pid = ":128";
	register int option;
	int blk_cnt;
	int n;
	struct utsname nm;  /* tcpio must use uname() instead of gethostname()
			       since networking is not part of the TCB */

	if ((priv_id = secsys (ES_PRVID, 0)) >= 0)
		lpm = 0;

	Uid = getuid();

	/* Determine if process is privileged.  If so, *
	 * the privileged flag will be set.            */
	if ((privileged = getpriv()) == -1)
		exit(1);

	/* 
	 * determine whether the ACL security package is installed,
	 * what the maximum number of ACL entries allowed is for
 	 * the system, and allocate a buffer to hold the ACLs.
	 */
	if (acl("/", ACL_CNT, 0, NULL) == ERROR && errno == ENOPKG) {
		aclpkg = 0;
	} else if ((aclmax = sysconf(_SC_NACLS_MAX)) == -1) {
		exit(1);
	} else if ((aclbufp = TBL(acl, aclmax)) == NULL) {
		msg(EXT, ":643", "Out of memory: %s", "setup()");
	}

	/* determine whether the MAC security package is installed */
	if (lvlproc(MAC_GET, &orig_lvl) != 0)
		if (errno == ENOPKG)
			macpkg = 0;
		else
			msg(EXTN, ":775", "Cannot get level of process");

	Orig_umask = umask(0);
        Now = time(0);

	/* Get the host name */
        if (uname(&nm) == ERROR) {
            msg(EXT, ":927", "Cannot get host name");
        }
	strcpy(Host, nm.nodename);

        if ((Curpw_p = getpwuid(Uid)) == (struct passwd *)NULL) {
            msg(EXT, ":82", "Unknown user id: %s", Uid);
        }

        S_p->sh_fpriv_tbl = &pvec[0];

	Efil_p = Own_p = IOfil_p = NULL;
	if (macpkg == 0) {
		opts_p = opts_p_nomac;
	}
	while ((option = getopt(largc, largv, opts_p)) != EOF) {
		switch (option) {
		case 'a':	/* reset access time */
			Args |= OCa;
			break;
		case 'b':	/* swap bytes and halfwords */
			Args |= OCb;
			break;

		case 'd':	/* create directories as needed */
			Args |= OCd;
			break;
		case 'f':	/* select files not in patterns */
			Args |= OCf;
			break;
		case 'i':	/* "copy in" */
			Args |= OCi;
                        Archive = fileno(stdin);
			break;
		case 'k':	/* retry after I/O errors */
			Args |= OCk;
			break;
                case 'n':       /* do NOT verify these IDs */
                        Args |= OCn;
                        n = atoi(optarg);
                        if (strspn(optarg, "0123456789") != strlen(optarg) || n > NO_OPS || n <= 0)
                            msg(ERR, ":356", "Invalid argument to option -%c\n", 'n');
                        else
                            Unverify |= 1 << (n-1);
			if ((macpkg == 0) && (Unverify & (NO_ACT | NO_LID))) {
                            msg(ERR, ":356", "Invalid argument to option -%c\n", 'n');
			}
                        break;
		case 'o':	/* "copy out" */
			Args |= OCo;
                        Archive = fileno(stdout);
			break;
		case 'r':	/* rename files interactively */
			Args |= OCr;
			break;
		case 's':	/* swap bytes */
			Args |= OCs;
			break;
		case 't':	/* table of contents */
			Args |= OCt;
			break;
		case 'u':	/* copy unconditionally */
			Args |= OCu;
			break;
		case 'v':	/* verbose - print file names */
			Args |= OCv;
			break;
                case 'x':       /* xtra quiet - suppress error messages */
                        Args |= OCx;
                        break;
		case 'C':	/* set arbitrary block size */
			if (Args & OCC)
				msg(ERR, dupl_pid, dupl_p, 'C');
			else {
				Args |= OCC;
				Bufsize = atoi(optarg);
			}
			break;
		case 'D':
			Dflag = 1;
			break;
		case 'E':	/* alternate file for pattern input */
			if (Args & OCE)
				msg(ERR, dupl_pid, dupl_p, 'E');
			else {
				Args |= OCE;
				Efil_p = optarg;
			}
			break;
		case 'I':	/* file for archive input */
			if (Args & OCI)
				msg(ERR, dupl_pid, dupl_p, 'I');
			else {
				Args |= OCI;
				IOfil_p = optarg;
			}
			break;
		case 'L':	/* follow symbolic links */
			Args |= OCL;
			break;
		case 'M':	/* specify new end-of-media message */
			if (Args & OCM)
				msg(ERR, dupl_pid, dupl_p, 'M');
			else {
				Args |= OCM;
				Eom_p = optarg;
			}
			break;
                case 'N':       /* extract all files at "this" level */
                        if (Args & OCN)
                            msg(ERR, dupl_pid, dupl_p, 'N');
			else {
                            Args |= OCN;
			    Map_p = optarg;
			}
                        break;
		case 'O':	/* file for archive output */
			if (Args & OCO)
				msg(ERR, dupl_pid, dupl_p, 'O');
			else {
				Args |= OCO;
				IOfil_p = optarg;
			}
			break;
		case 'P':	/* peek at level range stored on archive */
                        Args |= OCP;
			break;
		case 'R':	/* change owner/group of files */
			if (Args & OCR)
				msg(ERR, dupl_pid, dupl_p, 'R');
			else {
				Args |= OCR;
				Own_p = optarg;
			}
			break;
		case 'S':	/* swap halfwords */
			Args |= OCS;
			break;
                case 'T':       /* TTOCTT file */
                        if (Args & OCT)
                            msg(ERR, dupl_pid, dupl_p, 'T');
                        else {
                            Args |= OCT;
                            Tfil_p = optarg;
                        }
                        break;
		case 'V':	/* print a dot '.' for each file */
			Args |= OCV;
			break;
                case 'X':       /* extract only files where: */
                                /*  Min_lvl <= flevel <= Max_lvl */
                        if (Args & OCX)
                            msg(ERR, dupl_pid, dupl_p, 'X');
                        else {
                            Xlohi_p = optarg;
                            Args |= OCX;
                        }
                        break;
		default:
			/* error message will have been printed by getopt() */
			Error_cnt++;
		} /* option */
	} /* (option = getopt(largc, largv, opts_p)) != EOF */
	largc -= optind;
	largv += optind;
	ckopts(Args);
	/*
	 * If mac is not installed, then even though it is illegal
	 * to specify -n "NO_ACT" or "NO_LID" on the command line,
	 * we must act as if they were specified.
	 */
	if (macpkg == 0) {
		Unverify |= (NO_ACT | NO_LID);
	}
	if (!Error_cnt && ! Serror_cnt) {
		if ((Args & OCr) && (Renam_p = (char *)malloc(APATH)) == (char *)NULL)
			msg(EXTN, ":643", "Out of memory: %s", "setup()");
		if ((Symlnk_p = (char *)malloc(APATH)) == (char *)NULL)
			msg(EXTN, ":643", "Out of memory: %s", "setup()");
		if ((Over_p = (char *)malloc(APATH)) == (char *)NULL)
			msg(EXTN, ":643", "Out of memory: %s", "setup()");
		if ((Nam_p = (char *)malloc(APATH)) == (char *)NULL)
			msg(EXTN, ":643", "Out of memory: %s", "setup()");

		Gen.g_nam_p = Nam_p;
		if ((Fullnam_p = getcwd((char *)NULL, APATH)) == (char *)NULL) 
			msg(EXT, ":129", "Cannot determine current directory");
		if (Args & OCi) {
			if (largc > 0) /* save patterns for -i option, if any */
				Pat_pp = largv;
			if (Args & OCE)
				getpats(largc, largv);

			if (fstat(Archive, &ArchSt) < 0)
				msg(ERRN, ":35", "Cannot access the archive");
		} else {
			if (largc != 0) /* error if arguments left with -o */
				msg(EXT, ":928", "Extra arguments at end");
			else if (fstat(Archive, &ArchSt) < 0)
				msg(ERRN, ":35", "Cannot access the archive");
		}
	}
	if (Error_cnt || Serror_cnt)
		usage(); /* exits! */
	if (!Dflag) {
		if (Args & (OCC)) {
			if (g_init(&Device, &Archive) < 0) {
				msg(EXTN, ":45", "Error during initialization");
			}
		} else {
			if ((Bufsize = g_init(&Device, &Archive)) < 0){
				msg(EXTN, ":45", "Error during initialization");
			}
		}
	}

	blk_cnt = _20K / Bufsize;
	blk_cnt = (blk_cnt >= MX_BUFS) ? blk_cnt : MX_BUFS;
	while (blk_cnt > 1) {
		if ((Buffr.b_base_p = align(Bufsize * blk_cnt)) != (char *)-1) {
			Buffr.b_out_p = Buffr.b_in_p = Buffr.b_base_p;
			Buffr.b_cnt = 0L;
			Buffr.b_size = (long)(Bufsize * blk_cnt);
			Buffr.b_end_p = Buffr.b_base_p + Buffr.b_size;
			break;
		}
		blk_cnt--;
	}
	if (blk_cnt < 2 || Buffr.b_size < (2 * CPIOBSZ))
		msg(EXT, ":643", "Out of memory: %s", "setup()");

	Max_filesz = ulimit(UL_GETFSIZE, 0L);
	Lnk_hd.L_nxt_p = Lnk_hd.L_bck_p = &Lnk_hd;
	Lnk_hd.L_lnk_p = (struct Lnk *)NULL;
}

/*
 * Procedure:     set_tym
 *
 * Restrictions:
                 utime(2): None
 * notes: Set the access and/or modification times for a file.
 */

static
void
set_tym(nam_p, atime, mtime)
register char *nam_p;
register ulong atime, mtime;
{
	struct utimbuf timev;

	timev.actime = atime;
	timev.modtime = mtime;

	if (utime(nam_p, &timev) < 0) {
		if (Args & OCa)
			msg(ERRN, ":131", "Cannot reset access time for \"%s\"", nam_p);
		else
			msg(ERRN, ":132", "Cannot reset modification time for \"%s\"", nam_p);
	}
}

/*
 * Procedure:     sigint
 *
 * Restrictions:
                 unlink(2): None
                 rename(2): None
 * notes:  Catch interrupts.  If an interrupt occurs during the extraction
 * of a file from the archive with the -u option set, and the filename did
 * exist, remove the current file and restore the original file.  Then exit.
 */

static
void
sigint()
{
	register char *nam_p;

	(void)signal(SIGINT, SIG_IGN); /* block further signals */
	if (!Finished) {
		if (Args & OCi)
			nam_p = G_p->g_nam_p;

		if (*Over_p != '\0') { /* There is a temp file */
			if (unlink(nam_p))
				msg(ERRN, ":42", "Cannot remove incomplete \"%s\"", nam_p);
			if (rename(Over_p, nam_p) < 0)
				msg(ERRN, ":55", "Cannot recover original \"%s\"", nam_p);
		}  else if (unlink(nam_p))
			msg(ERRN, ":42", "Cannot remove incomplete \"%s\"", nam_p);
	}
	exit(Error_cnt);
}

/*
 * swap: Swap bytes (-s), halfwords (-S) or or both halfwords and bytes (-b).
 */

static
void
swap(buf_p, cnt)
register char *buf_p;
register int cnt;
{
	register unsigned char tbyte;
	register int tcnt;
	register ushort thalf;

	cnt /= 4;
	if (Args & (OCb | OCs)) {
		tcnt = cnt;
		Swp_p = (union swpbuf *)buf_p;
		while (tcnt-- > 0) {
			tbyte = Swp_p->s_byte[0];
			Swp_p->s_byte[0] = Swp_p->s_byte[1];
			Swp_p->s_byte[1] = tbyte;
			tbyte = Swp_p->s_byte[2];
			Swp_p->s_byte[2] = Swp_p->s_byte[3];
			Swp_p->s_byte[3] = tbyte;
			Swp_p++;
		}
	}
	if (Args & (OCb | OCS)) {
		tcnt = cnt;
		Swp_p = (union swpbuf *)buf_p;
		while (tcnt-- > 0) {
			thalf = Swp_p->s_half[0];
			Swp_p->s_half[0] = Swp_p->s_half[1];
			Swp_p->s_half[1] = thalf;
			Swp_p++;
		}
	}
}

/*
 * usage: Print the usage message on stderr and exit.
 */

static
void
usage()
{

	/* Error_cnt will be set iff there were errors encountered that caused
	 * a message to be printed.  Serror_cnt will be set iff there were
	 * errors encountered that did not cause a message to be printed.  If
	 * only silent errors were encountered, print a generic "Syntax"
	 * message here.
	 */

	if (Serror_cnt && !Error_cnt)
		msg(ERR, ":233", "Syntax error\n");

	(void)fflush(stdout);
	(void)pfmt(stderr, MM_ACTION, ":929:\nUsage:");

	if (macpkg) {
		(void)pfmt(stderr, MM_NOSTD,
			":930:\ttcpio -i -I file [-bdfkPrsStuvVx] [-C size] ");
		(void)pfmt(stderr, MM_NOSTD,
			":931:[-E file]\n\t\t  [-M msg] ");
		(void)pfmt(stderr, MM_NOSTD,
			":932:[-n num] [-N level] [-R ID] [-T file]\n");
		(void)pfmt(stderr, MM_NOSTD,
			":933:\t\t  [-X lo,hi] [patterns]\n");
		(void)pfmt(stderr, MM_NOSTD,
			":934:\ttcpio -o -O file [-aLvVx] [-C size] [-M msg] [-X lo,hi]\n");
	} else {
		(void)pfmt(stderr, MM_NOSTD,
			":1212:\ttcpio -i -I file [-bdfkrsStuvVx] [-C size] ");
		(void)pfmt(stderr, MM_NOSTD,
			":931:[-E file]\n\t\t  [-M msg] ");
		(void)pfmt(stderr, MM_NOSTD,
			":1213:[-n num] [-R ID] [-T file]\n");
		(void)pfmt(stderr, MM_NOSTD,
			":1214:\t\t  [patterns]\n");
		(void)pfmt(stderr, MM_NOSTD,
			":1215:\ttcpio -o -O file [-aLvVx] [-C size] [-M msg]\n");
	}

	(void)fflush(stderr);
	exit(Error_cnt);
}

/*
 * Procedure:     verbose
 *
 * Restrictions:
                 setpwent: None
                 getpwuid: None
                 setgrent: None
                 getgrgid: None
                 cftime: None
 * notes: For each file, print either the filename (-v) or a dot (-V).
  * If the -t option (table of contents) is set, print either the filename
  * and list of file privileges, or if the -v option is also set, print an 
  * "ls -l"-like listing, plus each file's level ID.
  * The -v option also prints file privileges, if any, except when combined 
  * with the -t option.
 */

static
void
verbose(nam_p)
register char *nam_p;
{
 	register int i, j, temp;
	mode_t mode;
	char modestr[12];

	for (i = 0; i < 10; i++)
		modestr[i] = '-';
	modestr[i++] = ' ';
	modestr[i] = '\0';

        if ((Args & OCt) && (Args & OCv)) {
            if (S_p->sh_acl_num > 4)
                modestr[10] = '+';
        }

	if ((Args & OCt) && (Args & OCv)) {
		mode = Gen.g_mode;
		for (i = 0; i < 3; i++) {
			temp = (mode >> (6 - (i * 3)));
			j = (i * 3) + 1;
			if (S_IROTH & temp)
				modestr[j] = 'r';
			if (S_IWOTH & temp)
				modestr[j + 1] = 'w';
			if (S_IXOTH & temp)
				modestr[j + 2] = 'x';
		}
		temp = Gen.g_mode & Ftype;
		switch (temp) {
		case (S_IFIFO):
			modestr[0] = 'p';
			break;
		case (S_IFCHR):
			modestr[0] = 'c';
			break;
		case (S_IFDIR):
			modestr[0] = 'd';
			break;
		case (S_IFBLK):
			modestr[0] = 'b';
			break;
		case (S_IFREG): /* was initialized to '-' */
			break;
		case (S_IFLNK):
			modestr[0] = 'l';
			break;
		default:
			msg(ERR, ":140", "Impossible file type");
		}
		if ((S_ISUID & Gen.g_mode) == S_ISUID)
			modestr[3] = 's';
		if ((S_ISVTX & Gen.g_mode) == S_ISVTX)
			modestr[9] = 't';
		if ((S_ISGID & G_p->g_mode) == S_ISGID && modestr[6] == 'x')
			modestr[6] = 's';
		else if ((S_ENFMT & Gen.g_mode) == S_ENFMT && modestr[6] != 'x')
			modestr[6] = 'l';

		if (Gen.g_nlink == 0)
			(void)printf("%s%4d ", modestr, Gen.g_nlink+1);
		else
			(void)printf("%s%4d ", modestr, Gen.g_nlink);
		if (Lastuid == (int)Gen.g_uid)
			(void)printf("%-9s", Curpw_p->pw_name);
		else {
			setpwent();
			if (Curpw_p = getpwuid((int)Gen.g_uid)) {
				(void)printf("%-9s", Curpw_p->pw_name);
				Lastuid = (int)Gen.g_uid;
			} else {
				(void)printf("%-9d", Gen.g_uid);
				Lastuid = -1;
			}
		}
		if (Lastgid == (int)Gen.g_gid)
			(void)printf("%-9s", Curgr_p->gr_name);
		else {
			setgrent();
			if (Curgr_p = getgrgid((int)Gen.g_gid)) {
				(void)printf("%-9s", Curgr_p->gr_name);
				Lastgid = (int)Gen.g_gid;
			} else {
				(void)printf("%-9d", Gen.g_gid);
				Lastgid = -1;
			}
		}
		if (!Aspec || ((Gen.g_mode & Ftype) == S_IFIFO))
			(void)printf("%-7ld ", Gen.g_filesz);
		else
			(void)printf("%3d,%3d ", major(Gen.g_rdev), minor(Gen.g_rdev));

                (void)cftime(Time, FORMAT, (time_t *)&(Gen.g_mtime));

		(void)printf("%s, %s", Time, nam_p);
		if (Alink) {
				(void)strncpy(Symlnk_p, Buffr.b_out_p, Gen.g_filesz);
				*(Symlnk_p + Gen.g_filesz) = '\0';

			(void)printf(" -> %s", Symlnk_p);
		}
		(void)printf(" %d",S_p->sh_level);
		(void)printf("\n");
	} else if ((Args & OCt) || (Args & OCv)) {
		(void)fputs(nam_p, Out_p);
		if ((S_p->sh_fpriv_num > 0)
		    && (M_p->mh_numsets > 0))
		  (void)print_privs(S_p->sh_fpriv_num,
				    &(S_p->sh_fpriv_tbl[0]),
				    Out_p);
		(void)fputc('\n', Out_p);
	} else { /* OCV */
		(void)fputc('.', Out_p);
		if (Verbcnt++ >= 49) { /* start a new line of dots */
			Verbcnt = 0;
			(void)fputc('\n', Out_p);
		}
	}
	(void)fflush(Out_p);
}

/*
 * write_hdr: Transfer header information for the generic structure
 * into the format for the selected header and bwrite() the header.
 */

static
void
write_hdr(sp)
struct sec_hdr *sp;
{
	register int cnt, pad;

	/* only one "type" of header */
	cnt = GENSZ + G_p->g_namesz;
	FLUSH(cnt);
	(void)sprintf(Buffr.b_in_p, "%.6lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%s\0",
		      G_p->g_magic, G_p->g_ino, G_p->g_mode, G_p->g_uid,
		      G_p->g_gid, G_p->g_nlink, G_p->g_mtime, G_p->g_filesz,
		      major(G_p->g_dev), minor(G_p->g_dev),
		      major(G_p->g_rdev), minor(G_p->g_rdev),
		      G_p->g_namesz, G_p->g_nam_p);

	Buffr.b_in_p += cnt;
	Buffr.b_cnt += cnt;
	cnt += write_shdr(sp); /* to keep cpio code happy (see below) */

	pad = ((cnt + Pad_val) & ~Pad_val) - cnt;
	if (pad != 0) {
		FLUSH(pad);
		(void)memset(Buffr.b_in_p, 0, pad);
		Buffr.b_in_p += pad;
		Buffr.b_cnt += pad;
	}
}

/*
 * write_trail: Create the appropriate trailer for the selected header type
 * and bwrite the trailer.  Pad the buffer with nulls out to the next Bufsize
 * boundary, and force a write.  If the write completes, or if the trailer is
 * completely written (but not all of the padding nulls (as can happen on end
 * of medium)) return.  Otherwise, the trailer was not completely written out,
 * so re-pad the buffer with nulls and try again.
 */

static
void
write_trail()
{
	register int cnt, need;

	Trailer = 1;
        Gen.g_mode = Gen.g_uid = Gen.g_gid = 0;
        Gen.g_nlink = 1;
        Gen.g_mtime = Gen.g_ino = Gen.g_dev = 0;
        Gen.g_rdev = Gen.g_filesz = 0;
        Gen.g_namesz = strlen("TRAILER!!!") + 1;
        (void)strcpy(Gen.g_nam_p, "TRAILER!!!");
        G_p = &Gen;
        write_hdr(S_p);
	need = Bufsize - (Buffr.b_cnt % Bufsize);
	if(need == Bufsize)
		need = 0;
	
	while (Buffr.b_cnt > 0) {
		while (need > 0) {
			cnt = (need < TARSZ) ? need : TARSZ;
			need -= cnt;
			FLUSH(cnt);
			(void)memset(Buffr.b_in_p, 0, cnt);
			Buffr.b_in_p += cnt;
			Buffr.b_cnt += cnt;
		}
		bflush();
		if (Newreel) {
			need = Bufsize - (Buffr.b_cnt % Bufsize);
			if(need == Bufsize)
				need = 0;
		}
	}
}
