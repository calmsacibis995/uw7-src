#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/resource.c	1.20"
#endif

#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>

#include <misc.h>
#include <node.h>
#include <list.h>
#include <resource.h>
#include <wsm.h>

static int			resource_compare();
static Resource *		resource();
static char *			tmp_filename();

#define rindex strrchr

#define PROGRAM(p)		((Program *)DNODE_data(p))
#define MAX_BUF			1024

#define WHITE_SPACE(c)		((c) == ' ' || (c) == '\t')
#define POUND(c)		((c) == '#')
#define COLON(c)		((c) == ':')
#define NEW_LINE(c)		((c) == '\n')
#define QUOTE(c)		((c) == '\\')

#define LOOKING_FOR_NAME	0
#define COLLECTING_NAME		1
#define LOOKING_FOR_VALUE	2
#define COLLECTING_VALUE	3
#define LOOKING_FOR_MN		4
#define COLLECTING_MN		5

#define SORT(p)			list_sort((p), resource_compare)
#define SEARCH(p, e)		(Resource*)list_search((p),(e),resource_compare)
#define APPEND(p, s)		list_append((p), (s), strlen((s)))

#ifdef DEBUG
#define PRINT(tag, msg)		fprintf(stderr, "%s: %s\n", tag, msg)
#define PRINT_RESOURCES(tag, p)	print_resources(tag, p)
#define PRINT_BUFFER(tag, p)	print_buffer(tag, p)
#else
#define PRINT(tag, msg)
#define PRINT_RESOURCES(tag, p)
#define PRINT_BUFFER(tag, p)
#endif

#if	defined(SYSV) || defined(SVR4)
#define rename(from, to)	(link(from, to) ? -1 : (unlink(from) ? -1 : 0))
#endif /* SYSV */

int
read_buffer(path, buffer)
	char *			path;
	List *			buffer;
{
	FILE *			fp;
	char			buf[MAX_BUF];
	int			n;
	int			v = FALSE;

	if (path && (fp = fopen(path, "r"))) 
	  {
		while ((n = fread(buf, 1, MAX_BUF, fp)) > 0) 
		  {
			list_append(buffer, buf, n);
		  }
		(void)fclose(fp);
		v = TRUE;
	  }
	return v;
}

int
write_buffer(path, buffer)
	char *			path;
	List *			buffer;
{
	FILE *			fp;
	int			n = buffer->count;
	int			v = FALSE;

	if (path) {
		char *	tmp_path = tmp_filename(path);

		if (fp = fopen(tmp_path, "w")) 
		  {
			if (fwrite(buffer->entry, 1, n, fp) == n) 
			  {
				v = TRUE;
		  	  }
			(void)fclose(fp);

			if (v) 
			  {
				(void)unlink(path);
				if (rename(tmp_path, path)) 
				  {
					(void)unlink(tmp_path);
					v = FALSE;
				  }
			  } 
			else 
			  {
				(void)unlink(tmp_path);
			  }
		  }
		FREE(tmp_path);
	  }
	return v;
}

static char *
tmp_filename(name)
	char *	name;
{
 	char	*stop = (char *) rindex(name, '/');
	char	*tmp = "_WSM_tmpfile_";
	int	pathname_length = (stop ? (stop - name + 1) : 0);
	int	tmp_length = strlen(tmp);
	int	result_length = pathname_length + tmp_length;
	char	*result = (char *) MALLOC(result_length + 1);

	strncpy(result, name, pathname_length);
	strncpy(result+pathname_length, tmp, tmp_length);
	result[result_length] = '\0';
	return (result);
}

void
resources_to_buffer(resources, buffer)
	List *			resources;
	List *			buffer;
{
	list_ITERATOR		I;
	Resource *		p;

	I = list_iterator(resources);

	while (p = (Resource *)list_next(&I)) 
	  {
		APPEND(buffer, p->name);
		APPEND(buffer, ":\t");
		APPEND(buffer, p->value);
		APPEND(buffer, "\n");
	  }
}

void
buffer_to_resources(buffer, resources)
	List *			buffer;
	List *			resources;
{
	list_ITERATOR		I;
	char *			p;
	char *			name;
	char *			value;
	int			name_size;
	int			value_size;
	int			quoted = FALSE;
	int			state = LOOKING_FOR_NAME;
	int			space = 0;
	int			c;

	I = list_iterator(buffer);

	while (p = (char *)list_next(&I)) {
		c = *p;

		if (QUOTE(c)) {
			quoted = TRUE;
			continue;
		}
		if (quoted) {
			quoted = FALSE;
			continue;
		}
		switch (state) {
		case LOOKING_FOR_NAME:
			if (NEW_LINE(c)) {
				PRINT("buffer_to_resources", "blank line");
				state = LOOKING_FOR_NAME;
			}
			else if (!WHITE_SPACE(c)) {
				name = p;
				state = COLLECTING_NAME;
				space = 0;
			}
			break;

		case COLLECTING_NAME:
			if (NEW_LINE(c)) {
				PRINT("buffer_to_resources",
					"poorly formed line");
				state = LOOKING_FOR_NAME;
			}
			else if (COLON(c)) {
				name_size = (p - name) - space;
				state = LOOKING_FOR_VALUE;
			}
			else {
				space = WHITE_SPACE(c) ? space + 1 : 0;
			}
			break;

		case LOOKING_FOR_VALUE:
			if (NEW_LINE(c)) {
				list_push(resources, resource(
						STRNDUP(name, name_size),
						STRNDUP("", 0)));
				state = LOOKING_FOR_NAME;
			}
			else if (!WHITE_SPACE(c)) {
				value = p;
				state = COLLECTING_VALUE;
				space = 0;
			}
			break;

		case COLLECTING_VALUE:
			if (NEW_LINE(c)) {
				value_size = (p - value) - space;
				list_push(resources, resource(
						STRNDUP(name, name_size),
						STRNDUP(value, value_size)));
				state = LOOKING_FOR_NAME;
			}
			else {
				space = WHITE_SPACE(c) ? space + 1 : 0;
			}
			break;

		}
	}
	SORT(resources);
	PRINT_RESOURCES("buffer_to_resources", resources);
}

void
merge_resources(target, source)
	List *			target;
	List *			source;
{
	list_ITERATOR		I;
	Resource *		p;
	Resource *		q;
	List *			tmp = NULL;

	I = list_iterator(source);

	while (p = (Resource *)list_next(&I)) 
	  {
		if (q = SEARCH(target, p)) 
		  {
			if (!p->value || !*p->value) 
			  {
				FREE(q->name);
				FREE(q->value);
				list_delete(target, list_index(target, q), 1);
			  } 
			else if (!MATCH(p->value, q->value)) 
			  {
				FREE(q->value);
				q->value = STRDUP(p->value);
		  	  }
		  } 
		else if (p->value != NULL && *p->value != '\0')
		  {
			if (!tmp) 
			  {
				tmp = alloc_List(sizeof(Resource));
			  }
			list_push((List *)tmp, resource(
					STRDUP(p->name),
					STRDUP(p->value)));
		  }
	  }
	if (tmp) 
	  {
		list_merge(target, tmp);
		free_List(tmp);
		SORT(target);
	  }
	PRINT_RESOURCES("merge_resources", target); 
}

void
delete_resources(target, source)
	List *			target;
	List *			source;
{
	list_ITERATOR		I;
	Resource *		p;
	Resource *		q;

	I = list_iterator(source);

	while (p = (Resource *)list_next(&I)) 
	  {
		if (q = SEARCH(target, p)) 
		  {
			FREE(q->name);
			FREE(q->value);
			list_delete(target, list_index(target, q), 1);
		  }
	  }
	PRINT_RESOURCES("delete_resources", target);
}

void
free_resources(resources)
	List *			resources;
{
	list_ITERATOR		I;
	Resource *		p;

	I = list_iterator(resources);

	while (p = (Resource *)list_next(&I)) 
	  {
		FREE(p->name);
		FREE(p->value);
	  }
}

char *
resource_value(resources, name)
	List *			resources;
	char *			name;
{
	Resource		p;
	Resource *		q;

	if (name) 
	  {
		p.name = name;

		if (q = SEARCH(resources, &p)) 
		  {
			return q->value;
		  }
	  }
	return NULL;
}

void
delete_RESOURCE_MANAGER()
{
	XDeleteProperty(DISPLAY, ROOT, XA_RESOURCE_MANAGER);
}

void
change_RESOURCE_MANAGER(buffer)
	List *			buffer;
{
	XChangeProperty(DISPLAY, ROOT, XA_RESOURCE_MANAGER, XA_STRING, 8,
		PropModeReplace, (unsigned char *)buffer->entry,
		buffer->count);
}

#ifdef NO_PROGRAMS
void
programs_to_buffer(programs, buffer)
	DNODE **		programs;
	List *			buffer;
{
	dring_ITERATOR		I;
	DNODE *			p;
	Program *		q;

	I = dring_iterator(programs);

	while (p = dring_next(&I)) 
	  {
		q = PROGRAM(p);
		APPEND(buffer, q->name);
		APPEND(buffer, "#\t");
		APPEND(buffer, q->mnemonic);
		APPEND(buffer, ":\t");
		APPEND(buffer, q->command);
		APPEND(buffer, "\n");
	  }
}

void
buffer_to_programs(buffer, programs)
	List *			buffer;
	DNODE **		programs;
{
	list_ITERATOR		I;
	char *			p;
	char *			name;
	char *			value;
	char *			value_mnemonic;
	int			name_size;
	int			value_size;
	int			value_mnemonic_size;
	int			quoted = FALSE;
	int			state = LOOKING_FOR_NAME;
	int			space = 0;
	int			c;

	I = list_iterator(buffer);

	while (p = (char *)list_next(&I)) 
	  {
		c = *p;

		if (QUOTE(c)) 
		  {
			quoted = TRUE;
			continue;
		  }
		if (quoted) 
		  {
			quoted = FALSE;
			continue;
		  }
		switch (state) 
		{
		case LOOKING_FOR_NAME:
			if (NEW_LINE(c)) 
			  {
				PRINT("buffer_to_programs", "blank line");
				state = LOOKING_FOR_NAME;
			  }
			else if (!WHITE_SPACE(c)) 
			  {
				name = p;
				state = COLLECTING_NAME;
				space = 0;
			  }
			break;

		case COLLECTING_NAME:
			if (NEW_LINE(c)) 
			  {
				PRINT("buffer_to_programs",
					"poorly formed line");
				state = LOOKING_FOR_NAME;
			  }
			else if (COLON(c)) 
			  {  /* no mnemonics delimiter */
				name_size = (p - name) - space;
   				value_mnemonic = "";
                                value_mnemonic_size = 0;
				state = LOOKING_FOR_VALUE;
			  }
			else if (POUND(c)) 
			  {  /* found mnemonics delimiter */
				name_size = (p - name) - space;
				state = LOOKING_FOR_MN;
			  }
			else 
			  {
				space = WHITE_SPACE(c) ? space + 1 : 0;
			  }
			break;

		case LOOKING_FOR_MN:
			if (NEW_LINE(c)) 
			  {
				PRINT("buffer_to_programs",
					"poorly formed line");
				state = LOOKING_FOR_NAME;
			  }
			/* No mnemonic, but still valid line */
			else if (COLON(c)) 
			  {
   				value_mnemonic = "";
                                value_mnemonic_size = 0;
				state = LOOKING_FOR_VALUE;
			  }
			else if (!WHITE_SPACE(c)) 
			  {
				value_mnemonic = p;
				state    = COLLECTING_MN;
				space    = 0;
			  }
			break;

		case COLLECTING_MN:
			if (NEW_LINE(c)) 
			  {
				PRINT("buffer_to_programs",
					"poorly formed line");
				state = LOOKING_FOR_NAME;
			  }
			else if (COLON(c)) 
			  {
				value_mnemonic_size = (p - value_mnemonic) - space;
				state = LOOKING_FOR_VALUE;
			  }
			else 
			  {
				space = WHITE_SPACE(c) ? space + 1 : 0;
			  }
			break;

		case LOOKING_FOR_VALUE:
			if (NEW_LINE(c)) 
			  {
				PRINT("buffer_to_programs",
					"poorly formed line");
				state = LOOKING_FOR_NAME;
			  }
			else if (!WHITE_SPACE(c)) 
			  {
				value = p;
				state = COLLECTING_VALUE;
				space = 0;
			  }
			break;

		case COLLECTING_VALUE:
			if (NEW_LINE(c)) 
			  {
                                value_size = (p - value) - space;
				dring_pushq(programs, MakeProgram(
					STRNDUP(name, name_size),
					STRNDUP(value_mnemonic, value_mnemonic_size),
					STRNDUP(value, value_size)));
				state = LOOKING_FOR_NAME;
			  }
			else 
			  {
				space = WHITE_SPACE(c) ? space + 1 : 0;
			  }
			break;

		  }
	  }
}
#endif /* NO_PROGRAMS */

static int
resource_compare(p, q)
	Resource *		p;
	Resource *		q;
{
	return strcmp(p->name, q->name);
}

static Resource *
resource(name, value)
	char *			name;
	char *			value;
{
	static Resource		r;

	r.name = name;
	r.value = value;
	return &r;
}

#ifdef DEBUG
print_resources(tag, resources)
	char *			tag;
	List *			resources;
{
	list_ITERATOR		I;
	Resource *		p;

	fprintf(stderr, "%s:\n", tag);

	I = list_iterator(resources);

	while (p = (Resource *)list_next(&I)) 
	  {
		fprintf(stderr, "\tname = \"%s\", value = \"%s\"\n",
			p->name, p->value);
	  }
}

print_buffer(tag, buffer)
	char *			tag;
	List *			buffer;
{
	list_ITERATOR		I;
	char *			p;

	fprintf(stderr, "%s:\n", tag);

	I = list_iterator(buffer);

	while (p = (char *)list_next(&I)) 
	  {
		fprintf(stderr, "\tchar = %d (%c)\n", *p, *p);
	  }
}
#endif
