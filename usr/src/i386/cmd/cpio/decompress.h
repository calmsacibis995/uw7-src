#ident	"@(#)cpio:i386/cmd/cpio/decompress.h	1.1"

#define	MBUFFERS	32
#define	ZREAD_SIZE	(32*1024)
#define	ZBUFFER_SIZE	(ZREAD_SIZE*MBUFFERS)

typedef struct {
	unsigned long ucmp_size;
	unsigned long cmp_size;
	unsigned long daddr;
} HEADER_INFO ;

/* Shared memory structure */
typedef struct {
	int count[MBUFFERS];
	int sequence[MBUFFERS];
} SB;

extern void (*mem_decompress)(char *, char *, unsigned long, unsigned long);
