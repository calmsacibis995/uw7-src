#ifndef NOINDENT
#pragma ident	"@(#)pkg.h	15.1"
#endif

#include <Memutil/memutil.h>

#define QUANTUM		200

extern char *	GetXWINHome();

typedef	struct	{
	char *	pkg_name;	/* brief package name */
	char *	pkg_desc;      /* longer description */
	char *	pkg_fmt;	/* 4.0, 3.2 or Custom */
	char *	pkg_cat;       /* category: mostly arbitrary except for sets */
	char *	pkg_set;	/* NULL unless part of a set (named here) */
	char *  pkg_as;
	char *	pkg_vers;
	char *	pkg_arch;
	char *	pkg_vend;
	char *	pkg_date;
	char *	pkg_size;
	char *	pkg_help;
	char *	pkg_class;
	char	pkg_opflag;	/* 'T'/'F' installation (succeeded/failed), */
				/* 'D' (deleted) */
	int	pkg_info;	/* If True: values in this structure have */
				/* been obtained thru pkginfo. */
	int	pkg_reused;	/* Indicates package or set was already */
				/* installed and this package is just being */
				/* reused. */
	char	*pkg_install;
	int	pkg_file;
} PkgRec, *PkgPtr;

typedef	struct	{
	char *	pkg;
	char *	def;
} icon_obj;
