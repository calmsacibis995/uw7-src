#ident	"@(#)dump:common/dump.h	1.5"
#define DATESIZE 60

#include "libelf.h"

typedef struct scntab {
	char             *scn_name;
	Elf32_Shdr       *p_shdr;
	Elf_Scn          *p_sd;
} SCNTAB;

#define VOID_P void *

#define UCHAR_P unsigned char *

#define FAILURE 1
#define SUCCESS 0

extern char *prog_name;
extern int p_flag;
extern SCNTAB *p_debug, *p_line;
extern SCNTAB *p_abbreviations;

extern VOID_P get_scndata(Elf_Scn *, size_t *);
extern void dump_debug(Elf *, char *);
extern void dump_dwarf2_abbreviations(Elf *, const char *);
extern void dump_dwarf2_debug(Elf *, const char *, SCNTAB *);
extern void dump_dwarf2_line(Elf *, const char *, SCNTAB *);
