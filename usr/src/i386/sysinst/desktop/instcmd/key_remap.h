#ident	"@(#)key_remap.h	15.1"

#define DK_FILE_NAME "/dead_keys"	/*  Name of dead key config file  */
#define KB_FILE_NAME "/kbmap"		/*  Name of keyboard config file  */

#define DH_MAGIC "dUsD"		/*  Accronym for desktop UNIX system Deadkeys  */
#define KH_MAGIC "dUsK"		/*  Accronym for desktop UNIX system Keyboard  */

#define DK_BUF_SIZE 1024	/*  Size of dead key buffer  */
#define DK_OFFSET 522		/*  Offset of deadkeys in mapping buffer  */
#define DK_ND_OFFSET 514	/*  Offset to where no of dead keys is held  */

#define SUCCESS 0			/*  Everything went okay  */
#define FAILURE 1			/*  Something went wrong  */

void fatal();
void read_dk_map();
void read_kb_map();
char *file_read();

/**
 *  Structures used for the information stored and retrieved from the
 *  keyboard/font configuration files.
 **/

/**
 *  Header for keyboard file
 **/
typedef struct key_hdr {
	unchar kh_magic[4];		/*  File identifier magic "number"  */
	unchar kh_nkeys;		/*  Number of keys to be remapped  */
} KEY_HDR;

/**
 *  Key remapping information
 **/
typedef struct key_info {
	unchar ki_scan;			/*  The scancode (key) to remap  */
	unchar ki_states;		/*  Changed key states bit map  */
	unchar ki_spcl;			/*  Bit mask for special keys  */
	unchar ki_flgs;			/*  Flag for lock keys  */
} KEY_INFO;

typedef struct dead_hdr {
	unchar dh_magic[4];		/*  File identifier magic "number"  */
	unchar dh_ndead;		/*  Number of dead keys  */
} DK_HDR;

typedef struct dead_key {
	unchar di_key;			/*  The dead key character  */
	unchar di_ncombi;		/*  Number of valid combination keys  */
} DK_INFO;

typedef struct dead_combi {
	unchar dc_orig;			/*  The dead key combination character  */
	unchar dc_result;		/*  The result generated by this combination  */
} DK_COMBI;