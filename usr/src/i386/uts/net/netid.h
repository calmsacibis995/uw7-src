#ifndef _NET_NETID_H	/* wrapper symbol for kernel use */
#define _NET_NETID_H	/* subject to change without notice */

#ident	"@(#)netid.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#define NIDCPY(a,b)	(a[0]=b[0],\
			 a[1]=b[1],\
			 a[2]=b[2],\
			 a[3]=b[3],\
			 a[4]=b[4],\
			 a[5]=b[5])

#define NIDCLR(a)	(a[0]=0,\
			 a[1]=0,\
			 a[2]=0,\
			 a[3]=0,\
			 a[4]=0,\
			 a[5]=0)

#define NIDCMP(a,b)	(a[0]==b[0]&&\
			 a[1]==b[1]&&\
			 a[2]==b[2]&&\
			 a[3]==b[3]&&\
			 a[4]==b[4]&&\
			 a[5]==b[5])


#if defined(__cplusplus)
	}
#endif

#endif /* _NET_NETID_H */
