#ident	"@(#)conversions.h	1.2"

#define T_REG       1           /* regular files OK */
#define T_DIR       2           /* directories OK */
#define T_ASCII     4           /* ASCII transfers OK */

struct convert {
    struct convert *next;
    char *stripprefix;          /* prefix to strip from real file */
    char *stripfix;             /* postfix to strip from real file */
    char *prefix;               /* prefix to add to real file */
    char *postfix;              /* postfix to add to real file */
    char *external_cmd;         /* command to do conversion */
    int types;                  /* types: {file,directory} OK to convert */
    int options;                /* for logging: which conversion(s) used */
    char *name;                 /* description of conversion */
};

extern struct convert *cvtptr;
