#ident	"@(#)sco:paths.h	1.1"

/*
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/* Enhanced Application Compatibility Support */
/*
 *
 *
 *	Pathnames for i386
 */

/*
 *	Directory containing libraries and executable files
 *	(e.g. assembler pass 1)
 */
#define LIBDIR	"/lib"
#define LLIBDIR1 "/usr/lib"
#define NDELDIRS 2

/*
 *	Directory containing executable ("bin") files
 */
#define BINDIR	"/bin"

/*
 *	Directory containing include ("header") files for building tools
 */
#define INCDIR	"/tmp"

/*
 *	Directory for "temp"  files
 */
#define TMPDIR	"/tmp"

/*
 *	Default name of output object file
 */
#define A_OUT	"a.out"

/*
 *	The following pathnames will be used by the "cc" command
 *
 *	i386 cross compiler
 */
#define CPP	"/lib/cpp"
/*
 *	Directory containing include ("header") files for users' use
 */
#define INCLDIR	"-I/tmp"
#define COMP	"/lib/comp"
#define C0	"/lib/front"
#define C1	"/lib/back"
#define OPTIM	"/lib/optim"
/*
 *	i386 cross assembler
 */
#define AS	"/bin/as"
#define AS1	"/lib/as1"	/* assembler pass 1 */
#define AS2	"/lib/as2"	/* assembler pass 2 */
#define M4	"/usr/bin/m4"			/* macro preprocessor */
#define CM4DEFS	"/lib/cm4defs"	/* C interface macros */
#define CM4TVDEFS "/lib/cm4tvdefs"	/* C macros with 'tv' call */
/*
 *	i386 link editor
 */
#define LD	"/bin/ld"
#define LD2	"/lib/ld2"	/* link editor pass 2 */
/* End Enhanced Application Compatibility Support */
