/*
 *	@(#)confdata.h	7.1	10/22/97	12:21:43
 */
#ifndef _CONFDATA_H_
#define _CONFDATA_H_
/*
 * confdata.h - Definition of the config.dat file.
 */

/*
 * File header
 */

#define HDR_VERSION	0x1002
#define MAXDESCR 1024

typedef struct filehdr
{
	char id[10]; 	/* "USSCONF" */
	int  endian;	/* 0x12345678 */
	int version;

	char copying[100];

	int nmenu, nnode;
	int menuptr, nodeptr;
	int descrptr, ndescr;

	int stringptr, stringsz;

} filehdr;

/*
 * struct intlist is a list (array) used to store variable number of integers
 */

#define INTLIST_MAX 16

typedef struct intlist
{
	int n;
	unsigned int mask;	/* Bitmask representing values (<32) stored in ints */
	int ints[INTLIST_MAX];
} intlist;

/*
 * Resource (I/O, IRQ and DMA) data
 */

typedef struct shadow
{
  int offset, len;
} shadow;

#define RT_PORT		1
#define RT_IRQ		2
#define	RT_DMA		3

typedef struct resource
{
  int type;
  int num;
  int descr;
  intlist choices;
  int default_value;
  int options;
  int length;
  int align;
  int range_min, range_max, range_align;

  int nshadows;

  shadow shadows[8]; /* Additional port ranges (base port relative) */
} resource;

/*
 * Download file types
 */
#define FT_BIN	1
#define FT_HEX	2

typedef struct download
{
   int type;
   int descr;
   char driver[32];
   char filename[64];
} download;

/*
 * Options
 */

#define OPT_FIXED		0x00000001
#define OPT_SINGLE		0x00000002
#define OPT_OPTIONAL		0x00000004
#define OPT_PNP			0x00000008
#define OPT_STATIC		0x00000010
#define OPT_CONFLICT		0x00000020
#define OPT_PSEUDOPNP		0x00000040
#define OPT_TOUCHONLY		0x00000080
#define OPT_NOFIXED		0x00010000
#define OPT_NOSINGLE		0x00020000
#define OPT_NOOPTIONAL		0x00040000 
#define OPT_NOSTATIC		0x00100000

/*
 * Config nodes (device, card, arch)
 */

#define NT_DEVICE	1
#define NT_ARCH		2
#define NT_CARD		3

typedef struct node
{
  char name[32];
  struct node *hl;	/* hash link */
  int bustype;

  int type;
  int descr;
  int number, newnum;
  int options;

  int nres;

  int nref;
  int ref[10];

  resource res[8];

  download dl;
  int control_string;	/* A pointer to the description strings table */
} node;

#define BT_ISA		1
#define BT_ISAX		2
#define BT_MCA		3
#define BT_PCI		4

typedef struct menu
{
  int nodenum;
  char key[16];
  char name[108];
} menu;

typedef struct load_defs { /* For parser.y internal use only */
  char *name;
  int descr;
} load_defs;

void load_confdata(char *name);
void dump_confdata(void);

extern menu *menus;
extern node *nodes;
extern int  nmenus, nnodes;
extern char *descr[];

#endif
