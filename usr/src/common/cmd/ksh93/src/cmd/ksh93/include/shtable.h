#ident	"@(#)ksh93:src/cmd/ksh93/include/shtable.h	1.1"
#pragma prototyped
#ifndef _SHTABLE_H

/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * Interface definitions read-only data tables for shell
 *
 */

#define _SHTABLE_H	1

typedef struct shtable1
{
	const char	*sh_name;
	unsigned	sh_number;
} Shtable_t;

struct shtable2
{
	const char	*sh_name;
	unsigned	sh_number;
	const char	*sh_value;
};

struct shtable3
{
	const char	*sh_name;
	unsigned	sh_number;
	int		(*sh_value)(int, char*[], void*);
};

struct shtable4
{
	const char	*sh_name;
	unsigned	sh_number;
	const char	*sh_value;
	const char	*sh_value_id;
};

#define sh_lookup(name,value)	sh_locate(name,(Shtable_t*)(value),sizeof(*(value)))
extern const Shtable_t		shtab_testops[];
extern const Shtable_t		shtab_options[];
extern const Shtable_t		shtab_attributes[];
extern const Shtable_t		shtab_limits[];
extern const struct shtable2	shtab_variables[];
extern const struct shtable2	shtab_aliases[];
extern const struct shtable4	shtab_signals[];
extern const struct shtable3	shtab_builtins[];
extern const Shtable_t		shtab_reserved[];
extern const Shtable_t		shtab_config[];
extern int	sh_locate(const char*, const Shtable_t*, int);

#endif /* SH_TABLE_H */
