#ident	"@(#)pcintf:pkg_lmf/lmf_int.h	1.3"
/* SCCSID(@(#)lmf_int.h	7.2	LCC)	/* Modified: 16:45:14 3/9/92 */

/*
 *  LMF Internal file format
 */

#define LMFH_BASE_LEN	(28)		/* Max length of base domain */

struct lmf_file_header {
	char	lmfh_magic[4];		/* should be "LMF" */
	char	lmfh_base_domain[LMFH_BASE_LEN + 1];
	char	lmfh_base_attr;
	short	lmfh_base_length;
	long	lmfh_base_offset;
};

struct lmf_dirent {
	char	lmfd_len;		/* length of entry */
	char	lmfd_attr;
	short	lmfd_length;
	long	lmfd_offset;
	char	lmfd_name[4];		/* length is n*4, including null */
};

#define LMFA_DOMAIN	0x01		/* This entry contains a domain */

#ifndef O_BINARY
#define O_BINARY	0		/* DOS compatibility for opens */
#endif

/*
 * Flip macros for machines with big-endian architectures
 */
#define sflip(x) x = (((x >> 8) & 0xff) | ((x & 0xff) << 8))
#define lflip(x) x = (((x >> 24) & 0xff) | (((x >> 16) & 0xff) << 8) | \
		      (((x >> 8) & 0xff) << 16) | ((x & 0xff) << 24))
