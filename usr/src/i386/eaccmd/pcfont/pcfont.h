#ident	"@(#)pcfont.h	1.2"
/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

#define TRUE (1 == 1)
#define FALSE (1 == 0)

#define BACK_SLASH '\\'
#define COMMA ','
#define COMMENT '#'
#define RETURN '\n'
#define UNDER_SCORE '_'
#define EVER ;;					/*  Infinite loop  */
#define HASH_SIZE 1023			/*  Size of hash table  */
#define GEN_LEN 30				/*  General string length  */
#define INP_LEN 80				/*  Length of line read from file  */
#define SYM_LEN 20				/*  Max length of symbolic name  */
#define ERROR 1
#define SUCCESS 0
#define END_OF_FILE 1
#define CONSOLE "/dev/console"
#define VIDEO "/dev/console"
#define TTY_DEV "/dev/tty"
#define ROM_FONT "/etc/fonts/ROMfont"		/*  Default font file  */
#define FONT_DIR "/etc/fonts/"		/*  Font directory  */
#define NUM_FONTS 4				/*  No of fonts currently supported  */
#define FONT_2 2				/*  Bit mask for font 2  */
#define FONT_3 4				/*  Bit mask for font 3  */
#define FONT_4 8				/*  Bit mask for font 3  */
#define ALL_FONTS 15			/*  Bit mask for all fonts  */
#define MAX_VAL 255				/*  Maximum value of a bit map  */

/*  Number of bytes in each font size.  I use byte here in the loosest
 *  possible sense, since we actually use 32 bit integers for each 'byte'
 */
#define SIZE_8x8 8
#define SIZE_8x14 14
#define SIZE_8x16 16
#define SIZE_9x16 16
#define MEM_SIZE (SIZE_8x8 + SIZE_8x14 + SIZE_8x16 + SIZE_9x16)

/*  Names of font files  */
#define FONT_FILE_8x8 "8x8 font"
#define FONT_FILE_8x14 "8x14 font"
#define FONT_FILE_8x16 "8x16 font"
#define FONT_FILE_9x16 "9x16 font"

/*  Default font hash table structure  */
struct hash_struct {
	char sym_name[SYM_LEN];
	unchar sym_value;
} hash_table[HASH_SIZE];

/*  User defined font hash table structure  */
struct u_font_struct {
	char sym_name[SYM_LEN];
	unsigned char def_map;
	int *bit_map[NUM_FONTS];
} font_table[HASH_SIZE];

void init_rom_font();
void init_u_font();
void load_font();
int get_sym_name();
int hash_func();

/*  #defines used by the loadfont.c module  */
#define MAXENCODING 0xFFFF


#define DEFAULT_FONT "437"
void file_marker(char *);
#define FONT_MARK_DIR	"/etc"
#define FONT_MARKER "/etc/.font.*"
