#ident	"@(#)xenv:i386/paths.h	1.1.1.9"
/*
 *
 *
 *	Pathnames for i386
 */

/*
 *	Default directory search path for link-editor
 *	YLDIR says which directory in LIBPATH is replaced
 * 	by the -YL option to cc and ld.  YUDIR says
 *	which directory is replaced by -YU.
 */
#define LIBPATH	"I386LIBPATH"
#define YLDIR 1
#define YUDIR 2
/*	Directory containing library files and executables
 *	not accessed directly by users ("lib") files
 */
#define LIBDIR	"I386CCLIBDIR"
#define CCLIBDIR "I386CCLIBDIR"

/*
 *	Directory containing executable ("bin") files
 */
#define BINDIR   "I386BINDIR"
#define CCBINDIR "I386CCBINDIR"

/*	Absolute pathnames for dynamic shared library targets
 */
#define	LDSO_NAME "/usr/lib/ld.so.1"
#define	LIBCSO_NAME "/usr/lib/libc.so.1"

/* definitions used in OpenServer compatibility code */
#ifndef GEMINI_ON_OSR5
#define	ALTERNATE_LIBCSO_NAME "/OpenServer/usr/lib/libc.so.1"
#define ALT_PREFIX	"/OpenServer"
#define ALT_PREFIX_LEN	11
#else
#define ALT_PREFIX	"/udk"
#define ALT_PREFIX_LEN	4
#endif

/*
 *	Directory containing include ("header") files for building tools
 */
#define INCDIR   "I386INCDIR"
#define CCINCDIR "I386CCINCDIR"

/*
 *	Directory for "temp"  files
 */
#define TMPDIR	"I386TMPDIR"

/*
 *	Default name of output object file
 */
#define A_OUT	"SGSa.out"

/*
 *	The following pathnames will be used by the "cc" command
 *
 *	i386 cross compiler
 */
#define CPP	"I386CPP"
/*
 *	Directory containing include ("header") files for users' use
 */
#define INCLDIR	"-II386INCDIR"
#define COMP	"I386LIBDIR/comp"
#define C0	"I386LIBDIR/front"
#define C1	"I386LIBDIR/back"
#define OPTIM	"I386LIBDIR/optim"
/*
 *	i386 cross assembler
 */
#define AS	"I386BINDIR/SGSas"
#define AS1	"I386LIBDIR/SGSas1"	/* assembler pass 1 */
#define AS2	"I386LIBDIR/SGSas2"	/* assembler pass 2 */
#define M4	"I386BINDIR/SGSm4"	/* macro preprocessor */
#define CM4DEFS	"I386LIBDIR/cm4defs"	/* C interface macros */
#define CM4TVDEFS "I386LIBDIR/cm4tvdefs"	/* C macros with 'tv' call */
/*
 *	i386 link editor
 */
#define LD	"I386BINDIR/SGSld"
#define LD2	"I386LIBDIR/SGSld2"	/* link editor pass 2 */

#ifdef GEMINI_ON_OSR5
#define LD_ROOT	"/udk"		/* to what to append "/usr/lib" */
#else
#define LD_ROOT	"I386LDROOT"	/* to what to append "/usr/lib" */
#endif
