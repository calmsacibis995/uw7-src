#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/resource.h	1.6"
#endif

#ifndef _RESOURCE_H
#define _RESOURCE_H

typedef struct {
	char *			name;
	char *			value;
} Resource;

extern int			read_buffer();
extern int			write_buffer();
extern void			resources_to_buffer();
extern void			buffer_to_resources();
extern void			merge_resources();
extern void			delete_resources();
extern void			free_resources();
extern void			delete_RESOURCE_MANAGER();
extern void			change_RESOURCE_MANAGER();
extern void			programs_to_buffer();
extern void			buffer_to_programs();
extern char *			resource_value();

#endif
