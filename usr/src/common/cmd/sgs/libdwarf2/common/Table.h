#ifndef TABLE_H
#define TABLE_H
#ident	"@(#)libdwarf2:common/Table.h	1.2"

typedef struct
{
	unsigned int	value;
	const char	*name;
} Table;

const char *get_name(Table *tab, unsigned int value);

#endif /* TABLE_H */
