#ident "@(#)history.h	1.3"
#ident "$Header$"

/*
 *	Header File for history mechanism
 */



#ifndef IOBSIZE
#define IOBSIZE	512
#endif

#define HISSIZE		20		/* size of history list */
#define MAXLINE		160		/* longest history line permitted */

struct history {
	int		cur_cmd;	/* current command number */
	int		size;		/* size of history list */
	int		first_cmd;	/* command number of cmds[0] */
	char		*cmds[HISSIZE]; /* pointer to command lines */
};

typedef struct {
	short his_command;
	short his_line;
} histloc;

extern struct history	db_history;

char	*hist_word(char *, int);
histloc hist_locate(int, int, int);
histloc hist_find(char *, int, int, int);
