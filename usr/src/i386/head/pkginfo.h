#ident	"@(#)oamhdrs:i386/head/pkginfo.h	1.1"
#ident	"$Header$"

#define PI_INSTALLED 	0
#define PI_PARTIAL	1
#define PI_PRESVR4	2
#define PI_UNKNOWN	3
#define PI_SPOOLED	4

#define COREPKG	"foundation"

struct pkginfo {
	char	*pkginst;
	char	*name;
	char	*arch;
	char	*version;
	char	*vendor;
	char	*basedir;
	char	*catg;
	char	status;
};

extern char	*pkgdir;

extern char	*pkgparam();
extern int	pkginfo(),
		pkgnmchk();
