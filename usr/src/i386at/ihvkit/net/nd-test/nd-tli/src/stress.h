#ident	"@(#)stress.h	5.1"

#define		MIN_PACKET	4 	/* min packet size (checksum, size) */
#define 	MAX_PACKET	8092
#define		NUM_PACKET	5
#define		MAX_DATA	MAX_PACKET * NUM_PACKET

/* every data packet will have a header */
struct packet {
	ushort	checksum;
	ushort	size;
};

extern int 	tli_debug;
#define TLI_DEBUG(level,fmt)  if (tli_debug>=level) printf(fmt); else ;
#define TLI_DEBUG1(level,fmt,a1)  if (tli_debug>=level) printf(fmt,a1); else ;
#define TLI_DEBUG2(level,fmt,a1,a2)  if (tli_debug>=level) printf(fmt,a1,a2); else ;
#define TLI_DEBUG3(level,fmt,a1,a2,a3)  if (tli_debug>=level) printf(fmt,a1,a2,a3); else ;

