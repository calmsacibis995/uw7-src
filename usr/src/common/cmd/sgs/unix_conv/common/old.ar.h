#ident	"@(#)unix_conv:common/old.ar.h	2.5.2.3"

/* archive file header format */


#define	OARMAG	0177545
struct	oar_hdr {
	char	ar_name[14];
	long	ar_date;
	char	ar_uid;
	char	ar_gid;
	int	ar_mode;
	long	ar_size;
};

#ifdef __STDC__
 #pragma pack(2)
#endif

struct	xar_hdr {
	char	ar_name[14];
	long	ar_date;
	char	ar_uid;
	char	ar_gid;
	unsigned short	ar_mode;
	long	ar_size;
};

#ifdef __STDC__
 #pragma pack()
#endif
