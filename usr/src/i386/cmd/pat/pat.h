#ident	"@(#)pat:pat.h	1.2"

#define UWKMEM		1		/* namekmem on UnixWare */
#define O5KMEM		2		/* namekmem on OpenServer5 */

typedef struct patch {
	struct patch	*next;
	char		*symname;
	unsigned char	got_offset;	/* private to patsym.c */
	unsigned char	search_all;	/* private to patsym.c */
	unsigned short	sectindex;	/* private to patsym.c */
	unsigned long	archoffset;	/* private to patsym.c */
	unsigned long	plusoffset;
	unsigned long	offset;
	unsigned long	length;
	unsigned long	width;		/* private to pat.c */
	char		**oldhex;	/* private to pat.c */
	char		**newhex;	/* private to pat.c */
	int		bindcpu;	/* private to pat.c */
	int		updating;	/* private to pat.c */
} patch_t;

extern const char *			/* interface to patsym.c */
patsym(char *binfile, patch_t *patp, char **badnamep, int namekmem);
