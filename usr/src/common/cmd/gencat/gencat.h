#ident	"@(#)gencat:gencat.h	1.3"
#ident	"$Header$"
/*  Function declarations for gencat  */
int cat_build();
int msg_conv();

void cat_isc_build();
void cat_isc_dump();
void cat_sco_build();
void cat_sco_dump();
void cat_mmp_dump();

void add_set();
void add_msg();
void del_set();
void del_msg();
void fatal();

/*  For lint  */
#include <string.h>
void *malloc();

/*  For esmp  */
#include <limits.h>

/*  Format bits for cat_format  */
#define SVR_FORMAT 0x1
#define MALLOC_FORMAT 0x2
#define SCO_FORMAT 0x4
#define ISC_FORMAT 0x8

/*  Structures and definitions used for SCO format catalogues  */

#define NAME_MAX 14

/*
 * typedef unsigned short bit16;
 */
typedef unsigned short bit16;

typedef struct msg_fhd msg_fhd;	/* message file header prefix is mf_	*/
typedef struct msg_shd msg_shd;	/* message file set header prefix is ms_*/

#define M_MFMAG 	0502	/* magic number for msg file		*/

#define M_PRELD 0x0001		/* flag for message preloading		*/
#define M_DMDLD	0x0002		/* flag for load on demand		*/
#define M_DISLD 0x0004		/* flag for load and discard		*/
#define M_ALSET 0x0010		/* flag for loading all sets at catopen */
#define M_LOAD	0x0100		/* flag that set has to be loadeded	*/
#define M_EMPTY 0x0200		/* flag that set is empty		*/

struct msg_fhd {		/* HEADER FOR A MESSAGE FILE		*/
	bit16 mf_mag;		/* magic number	or current file index	*/
	bit16 mf_flg;		/* message file flags			*/
	bit16 mf_scnt;		/* highest setnumber in file		*/
	char  mf_nam[NAME_MAX+1];/* name of message catalogue		*/
};

struct msg_shd {		/* HEADER FOR EACH SET			*/
	bit16 ms_flg;		/* flags for set			*/
	bit16 ms_mcnt;		/* highest messagesnumber in section	*/
	bit16 ms_psize;		/* size of all messages in set in bytes	*/
	bit16 ms_msize;		/* memory req. to load the longest msg	*/
	bit16 ms_discnt;	/* number of message buffers for discard*/
	bit16 ms_ncnt;		/* number of named messages for section	*/
	long  ms_mhdoff;	/* offset to message headers	 	*/
	long  ms_msgoff;	/* offset to messages of section	*/
	long  ms_namoff;	/* offset to messagenames		*/
};

/*  Structures and definitions used for the ISC format catalogues  */

#define ISC_MAGIC 0x207b93d	/*  Magic number for catalogue  */

#define SHOW_OK 0x106a82c	/*  File okay to show with 'showcat'  */
#define SHOW_NO 0x10c9463	/*  Do no show with 'showcat'  */

struct catheader {		/*  Header for binary catalogue file  */
	int magicnum;		/*  Magic number to identify as ISC  */
	int showflag;		/*  SHOW_OK/SHOW_NO for 'showcat' utility  */
	char reserved[20];	/*  Not used  */
};

struct setinfo {		/*  Set information structure  */
	int num_msgs;		/*  Highest msg number in set  */
	int msgloc_offset;	/*  Offset in msgloc array for first msg  */
};
