/* static char sccsid[]	= "@(#)newscen.c	2.2PL2 11/23/93" SMI; */

/*
 * Copyright 1992 SunSoft, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SunSoft, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission. SunSoft, Inc. makes
 * no representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 *
 * SunSoft, Inc. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL SunSoft, Inc. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */


/************************************************************************

NAME:           newscen.c
PRODUCT:        ETET (Extended Test Environment Toolkit)
AUTHOR:         SunSoft Inc
DATE CREATED:   October 1992
CONTENTS:

MODIFICATIONS:

	"ETET Review", portability and style changes.
	Andrew Josey, UNIX System Labs Inc., May 1993.

	Addition of "group" keyword to match future directions
	in TET Architectural specification. This is a synonym for the
	ETET "parallel" keyword.
	Andrew Josey, UNIX System Labs Inc., 16 May 1993

	Fix assignment from getc() into an integer. As reported by MJC.
	Andrew Josey, UNIX System Labs Inc., 24 May 1993

	Code review cleanup 
	Ranjan Das-Gupta, UNIX System Labs Inc, 1 Jul 1993
	
	Fixed do_item() to check to see if resume_opt was set and if so,
	to check the current item against the test case line to resume.
	Also the rc = do_scen () call returns an undefined value to rc
	so I'll just leave rc alone.
	John Birchfield, QualTrak Corporation, 11 November, 1993

	Set act.sa_flags = 0 before call to sigaction()  in perform_scen()
	so SIGPIPE does not occur on some architectures. Also add 
	error handling for the sigaction() call.
	Reported by jss@apollo.hp.com. 
	Andrew Josey, UNIX System Labs, 23 November 1993

	Further code style changes for 1.10.3.
	Andrew Josey, Novell UNIX System Labs, March 1994.

	Change message line handling in scenario file so that message
	line written to journal regardless of execution mode.
	Andrew Josey, Novell UNIX System Labs, March 1994.

	Add close on exec fcntl() to stream file descriptor in
	perform_scen()
	Andrew Josey, Novell UNIX System Labs, March 1994.
************************************************************************/

#define TMALLOC(T)  (T *) TET_MALLOC(sizeof(T))

#include <tcc_env.h>
#include <tcc_mac.h>
#include <tet_jrnl.h>
#include <tcc_prot.h>

#define	RANDOM_ERRMSG	"Random	mode must be followed by a scenario reference."
#define	LOOP_ERRMSG	"Loop detected in scenarios."
#define	REMOTE_ERRMSG	"Remote	mode has not yet been implemented."
#define	DUPSCEN_ERRMSG	"Duplicate scenario name."
#define	BADCHAR_ERRMSG	"Scenario Line starts with invalid character."
#define	BADMODE_ERRMSG	"Invalid group mode syntax."

typedef	enum {include, parallel, group, random, remote, repeat, timed_loop} mode_e;
#define	MODE_E_FIRST	include
#define	MODE_E_LAST	timed_loop

#define N_MODES 7

/* note that group is a synonym for parallel */

static char	*mode_image[N_MODES] =
	{"include", "parallel", "group", "random", "remote", "repeat", "timed_loop"};

typedef	enum {invalid, test_path, list_path, scenario, group_exec, message}	ptr_e;
typedef	enum {invalid_line, empty_line,	comment_line, message_line,
			  ref_line, name_line, test_line, mode_line} line_e;

typedef	struct {
		ptr_e	 ptr_kind;
		void	*ptr;
} mptr;

typedef	struct {
		int 	mode;
		int 	param;
		mptr 	next;
} exec_t;

typedef	struct _line_t {
		int	line_no;
		off_t	byte_offset;
		 mptr	line_item;
		struct _line_t	*next;
} line_t;

typedef	struct _scen_t {
		int	 line_count;
		int	 dejavu;
		int	line_no;
		off_t	byte_offset;
		char	*scen_name;
		line_t	*first_line;
		struct _scen_t	*next;
} scen_t;

typedef	struct _itemptr_t {
		ptr_e	 ptr_kind;
		union xyzzy	{
			char	*str;
			scen_t	*scen_p;
			void	*item_p;
		}ptr;
} itemptr_t;

static scen_t *first_scen =	NULL;

static int index_dispenser_fd;

/* forward declarations*/

#if __STDC__ 
static int do_item(mptr );
static do_scen(scen_t *);
static void mptr_loop_check(mptr);
line_t * process_scenario_lines( line_e, scen_t *, line_t *, char *);
#else
static int do_item();
static do_scen();
static void mptr_loop_check();
line_t * process_scenario_lines();
#endif


#ifdef _POSIX_SOURCE
/* strcasecmp : case independent string compare */
 
#include <ctype.h>

static int 
#ifdef __STDC__
strcasecmp(char *s1, char *s2)
#else
strcasecmp(s1, s2)
char *s1, *s2;
#endif
{
	char *u = s1;
	char *ptr = s2;

	while(*ptr != '\0')
	{
		if(tolower(*u) == tolower(*ptr)) 
		{
			ptr++; u++;
		} 
		else 
		{
			break;	/* no match */
		}
	}
	return(tolower(*u) - tolower(*ptr)); /* return "difference" */
}

#endif /*_POSIX_SOURCE */


static int 
nexec_tc(test_case)
char *test_case;
{
#ifdef DBG
	(void) fprintf (stderr,"[%5d] Start	executing '%s' ...\n", getpid(), test_case);

#endif

	(void) read(index_dispenser_fd,(char *)	&exec_ctr, sizeof(exec_ctr));
	return(exec_tc(test_case));
}



static int 
ntool_tc(test_case,what_mode)
char *test_case;
int	what_mode;
{
#ifdef DBG
        static char *tool_image[] = {"unknown", "build", "clean"};
	(void) fprintf (stderr,"[%5d] %s '%s'\n", getpid(), tool_image[what_mode], test_case);
#endif
	(void) read(index_dispenser_fd,	(char *)&exec_ctr, sizeof(exec_ctr));
	return(tool_tc(test_case, what_mode));
}


static FILE   *scen_stream;
static int     scen_line_no =	1;
static off_t   scen_byte_offset =	0;
static off_t   end_scen_offset = 0;
static int     end_scen_line = 1;
static time_t  end_time;


static void		scen_seek(scen_offset, scen_line)
		off_t	 scen_offset;
		int		 scen_line;
{
#ifdef DBG
	(void) fprintf(stderr,"scen_seek(stream, %d, %ld)\n", scen_line, scen_offset);
#endif
	scen_line_no = scen_line-1;
	fseek(scen_stream, scen_offset,	SEEK_SET);
}



static char	* get_scen_line()
{
	static char line_buf[2048];
	char *tptr = line_buf;
	int c;

	scen_byte_offset = ftell(scen_stream);
	tptr = line_buf;


	/* read	every character	until \n or	EOF	into the buffer	*/
        while (((c = getc(scen_stream)) != EOF) && (c != '\n')) 
		*tptr++ = c;

	/* if the first	char is	EOF	then we	are	at EOF */
        if ((tptr == line_buf) && (c == EOF)) 
		return NULL;

	*tptr='\0';	/* Terminate the string	at the \n */
	scen_line_no++;

	if (end_scen_offset	< scen_byte_offset)	
	{
		end_scen_offset	= scen_byte_offset;
		end_scen_line	= scen_line_no;
	}

#ifdef DBG2
	 (void)  fprintf(stderr,"%4d[%5d]: %s\n", scen_line_no, scen_byte_offset, line_buf);
#endif
	return line_buf;
}


static void	syntax_error(error_msg)
		char	*error_msg;
{
	char *scen_line;

	(void) fprintf(stderr, "\n");
	(void) fprintf(stderr, "###	Fatal syntax error in Scenario File: '%s'\n",
					scen_file_name);
	scen_seek(scen_byte_offset,	scen_line_no);
	if ((scen_line=get_scen_line())	!= NULL)
		(void) fprintf(stderr, "	line %d: %s\n",	scen_line_no, scen_line);
	(void) fprintf(stderr, "	%s\n", error_msg);
	(void) fprintf(stderr, "\n");
	tet_shutdown();
}



static void	scenario_error(error_msg, scen_offset, scen_lineno)
		char	*error_msg;
		int		 scen_lineno;
		off_t	 scen_offset;
{
	char *scen_line;
	(void) fprintf(stderr, "\n");
	(void) fprintf(stderr, "### Fatal semantic error in Scenario File: '%s'\n",
					scen_file_name);
	scen_seek(scen_offset, scen_lineno);
	if ((scen_line=get_scen_line())	!= NULL)
		(void) fprintf(stderr, "    line %d: %s\n",	scen_lineno, scen_line);
	(void) fprintf(stderr, "    %s\n", error_msg);
	(void) fprintf(stderr, "\n");
	tet_shutdown();
}



static scen_t *	mem_find_scen(scen_name)
char *scen_name;
{
	scen_t *scen_p;
	for	(scen_p=first_scen;	scen_p!=NULL; scen_p=scen_p->next) 
	{
		if (strcmp(scen_name, scen_p->scen_name)==0) 
		{
			return scen_p;
		}
	}
	return NULL;
}



static char	* nstrdup(str)
		char *str;
{
	char *result = (char*)malloc(strlen(str)+1);
	return strcpy(result, str);
}



static scen_t *	new_scen(scen_name,	line_no, byte_offset)
		char	*scen_name;
		int		 line_no;
		off_t	 byte_offset;
{
	scen_t *scen_p = TMALLOC(scen_t);

	scen_p->line_count	= -1;
	scen_p->dejavu		= 0;
	scen_p->line_no		= line_no;
	scen_p->byte_offset	= byte_offset;
	scen_p->scen_name	= nstrdup(scen_name);
	scen_p->first_line	= NULL;
	scen_p->next		= first_scen;
	first_scen	= scen_p;

	return scen_p;
}



static scen_t *	make_scen_ref(scen_name)
		char	*scen_name;
{
	scen_t		*result	= mem_find_scen(scen_name);
	if (result == NULL)	
	{
		result = new_scen(scen_name, -1, -1);
	}
	return result;
}




/* strip -
		strip off comments,	put	'nulls'	in place of	where the comment was
*/
static char	* strip (str)
	char *str;
{
	int	i;
	char *end_str;

	for	(;isspace(*str)&&('\0'!=*str); str++);	/* skip	spaces */
	end_str=strchr(str,	'#');
	if (end_str!=NULL) *end_str	= '\0';
	for	(i=strlen(str)-1; (i>=0) &&	isspace(str[i])	; i--)
		str[i]='\0';
	return str;
}




/* atomode-
		called by parse_exec_mode
		takes ascii	string and returns the matching	execution mode;
		returns	-1 on failure
*/
static int atomode(token)
	char *token;
{
	int result;
	for	(result	= 0;	result <= N_MODES; result++) 
	{
		if (strcasecmp(token, mode_image[result]) == 0)	return result;
	}
	return  -1;
}


/* line_e -categorize an input line, return type */
static line_e line_type(str)
		char *str;
{
	char *ch;

	if (str[0]=='#') 
	{	/* comment starts with a # */
		return comment_line;
	}
	if (str[0]=='\0') 
	{	/* empty line */
		return empty_line;
	}
	if (isspace(str[0])) 
	{		/* many	items start	with a space char */
		for	(ch=str; isspace(*ch); ch++);		/* skip	spaces */
		switch (*ch) 
		{
		case '\0':
			return empty_line;
		case '#':
			return comment_line;
		case '"':
			return message_line;
		case '^':
			return ref_line;
		 case ':':
			return mode_line;
		case '/':
			return test_line;
		default:
			return invalid_line;
		}	/* end case */
	} 
	else if (isprint(str[0]))	
		 {		/* names start with	a printable	*/
			return name_line;
		 }

	return invalid_line;
}




line_t * process_include(include_name, cur_scen, cur_scen_last_line)
	char * include_name;
	scen_t *cur_scen;
	line_t * cur_scen_last_line;
{
	char * linein;
	char line_buff[2048];
	FILE * scen_stream_save=scen_stream;
	char * scen_file_name_save=scen_file_name;
	off_t scen_byte_offset_save=scen_byte_offset;
	int scen_line_no_save=scen_line_no;


	/* If alt_exec_dir is defined it should	be relative	to that	*/
	if (alt_exec_set) 
	{
		scen_file_name =	(char *) TET_MALLOC(strlen(alt_exec_dir) +
			strlen(include_name) + 2);
		(void) sprintf(scen_file_name,"%s/%s", alt_exec_dir, include_name);
	} 
	else 
	{
		/* otherwise it	should be relative to the test suite root */
		scen_file_name =	(char *) TET_MALLOC(strlen(test_suite_root)	+
			strlen(include_name) + 2);
		(void) sprintf(scen_file_name,"%s/%s", test_suite_root, include_name);
	}

	scen_stream = fopen(scen_file_name,"r");
#if	DBG
	 (void)	fprintf(stderr,"include	%s\n",scen_file_name);
#endif

	if (scen_stream == NULL)
	{
		(void) sprintf(error_mesg,"error opening include file %s\n",
			scen_file_name);
		perror("fopen");
		BAIL_OUT2(error_mesg);
	}

	scen_line_no=1;
	scen_byte_offset=0;

	/* make	sure the file descriptor is	closed on an exec call */
	(void) fcntl(fileno(scen_stream), F_SETFD, FD_CLOEXEC);

	/*
	 * Note	the	recursive call to process_scenario_lines to process
	   the "included" testlist
	 */
	while ((linein = get_scen_line()) != NULL)	
	{
		line_e ln_type;

		strcpy(line_buff," ");
		strcat(line_buff,linein);
		ln_type=line_type(line_buff);
		
		if (ln_type==name_line) 
		{
			/* this should never happen */
			BAIL_OUT2("Bad line type in include file\n");
		} 
		else 
		{
			cur_scen_last_line=
				process_scenario_lines(ln_type, cur_scen,
				cur_scen_last_line, line_buff);
		}
	}

	/* restore saved values */

	(void) fclose(scen_stream);
	scen_stream=scen_stream_save;
	free(scen_file_name);
	scen_file_name=scen_file_name_save;
	scen_byte_offset=scen_byte_offset_save;
	scen_line_no=scen_line_no_save;

	return cur_scen_last_line;
}




/* parse_exec_mode -
		called by parse_scen for mode lines (:<mode>:)
*/
static line_t * parse_exec_mode(line, line_item, cur_scen, cur_scen_last_line)
	char *line;
	mptr *line_item;
	scen_t *cur_scen;
	line_t * cur_scen_last_line;
{
	char old_sep, sep_char = ' ';
	char *token, *sep_ptr;
	exec_t *this_exec = TMALLOC(exec_t);
	line_item->ptr=this_exec;
	line_item->ptr_kind=group_exec;
	this_exec->param = 1;

	sep_ptr=strip(line);


	/* first pick off all the modes	*/
	while (sep_char!=':') 
	{
		token =	sep_ptr+1;
		if (NULL==(sep_ptr=strpbrk(token, ":,;"))) 
		{
			syntax_error(BADMODE_ERRMSG);
		}
		old_sep	= sep_char;
		sep_char = *sep_ptr;
		*sep_ptr = '\0';
		if (old_sep	== ',')	
		{
			this_exec->param = atoi(token);
		} 
		else 
		{
			if ((this_exec->mode = atomode(token)) < 0)	
			{
				sprintf(error_mesg,	"invalid group mode	: %s", token);
				syntax_error(error_mesg);
			}
		}
		if (sep_char==';') 
		{
			this_exec->next.ptr	= TMALLOC(exec_t);
			this_exec->next.ptr_kind = group_exec;
			this_exec =(exec_t *)	this_exec->next.ptr;
			this_exec->param = 1;
		}
	}

	token =	sep_ptr+1;
	token=strip(token);


	switch (*token)	
	{
	case '^':	/* must	be a scenario name */
		token++;
		this_exec->next.ptr_kind = scenario;
		this_exec->next.ptr	= make_scen_ref(strip(token));
		break;
	case '@':	/* must	be a test path name	*/
		token++;
		this_exec->next.ptr_kind = test_path;
		this_exec->next.ptr	= nstrdup(strip(token));
		break;
	default:	/* must	be an exec path	name */
		this_exec->next.ptr_kind = list_path;
		this_exec->next.ptr	= nstrdup(strip(token));
		if (this_exec->mode!=include) 
		{
			sprintf(error_mesg,
				"indirect file list (%s) not supported.\n	Use carat (^).\n",
				this_exec->next.ptr);
			syntax_error(error_mesg);
		} 
		else 
		{
			cur_scen_last_line=process_include((char *)this_exec->next.ptr,
				cur_scen, cur_scen_last_line);
		}
		break;
	}

	return cur_scen_last_line;
}




/* find_scen
	      - only called by parse_scen
		loop through scen file looking for a scenario name
		check for duplicates
		return pointer (or NULL) for scenario name
*/
static scen_t * find_scen(scen_name)
	char *scen_name;
{
	scen_t		*scen_p	= mem_find_scen(scen_name);
	char		*linein, *end_name;
	if ((scen_p==NULL) || (scen_p->line_no<0)) 
	{
	/* didn't find it in memory, keep looking */
		scen_seek(end_scen_offset, end_scen_line);

		while ((linein = get_scen_line()) != NULL) 
		{
			if (line_type(linein)==name_line) 
			{
				end_scen_offset	= scen_byte_offset;
				end_scen_line	= scen_line_no;
				end_name=strpbrk(linein,"# \t");
				if (end_name !=	NULL) *end_name	='\0';
				if ((scen_p=mem_find_scen(linein))!=NULL) 
				{
					if (scen_p->line_no<0) 
					{
						/* this	is only	a reference	so set the values */
						scen_p->line_no			= scen_line_no;
						scen_p->byte_offset		= scen_byte_offset;
					} 
					else 
					{
						syntax_error(DUPSCEN_ERRMSG);
					}
				} 
				else 
				{
					scen_p=new_scen(linein,	scen_line_no, scen_byte_offset);
				}
				if (strcmp(scen_name, linein)==0) return scen_p;
			}
		}
		return NULL;
	} 
	else 
	{
		scen_seek(scen_p->byte_offset, scen_p->line_no);
		linein = get_scen_line();
	}
	return scen_p;
}




/* strip_quotes	-
	only called by parse_scen
	used to	process	message	lines in scenario file and get the text
*/
static char	* strip_quotes (str)
	char *str;
{
	int	i;
	char *end_str;

	for	(;isspace(*str)&&('\0'!=*str); str++);	/* skip	spaces */
	if (*str=='"') str++;
	end_str=strrchr(str, '"');
	if (end_str!=NULL) *end_str	= '\0';
	for	(i=strlen(str)-1; (i>=0) &&	isspace(str[i])	; i--)
		str[i]='\0';
	return str;
}




/* add_scen_line
		- is a helper routine for 'parse_scen';
		- it adds a	line to	a specified scenario;
		- 'last_line' must be NULL or point to the last	line of
		  the scenario passed	in
		- It returns a pointer to a yet-to-be-filled in	structure to
		  describe a new line
		- It does initialize the 'line_no',and	byte_offset
		  fields from GLOBAL vars for the scen file */

static line_t *	add_scen_line(scen_p, last_line)
		scen_t *scen_p;
		line_t *last_line;
{
	line_t *result = TMALLOC(line_t);
	if (last_line == NULL) 
	{
		scen_p->first_line = result;
	} 
	else 
	{
		last_line->next	= result;
	}
	scen_p->line_count++;
	result->line_no		=scen_line_no;
	result->byte_offset	=scen_byte_offset;
	result->next		=NULL;
	result->line_item.ptr_kind	=invalid;
	result->line_item.ptr		=NULL;

	return result;
}



line_t * process_scenario_lines(ln_type, cur_scen, cur_scen_last_line, linein)
	line_e ln_type;
	scen_t *cur_scen;
	line_t *cur_scen_last_line;
	char * linein;
{
	switch (ln_type) 
	{
		case invalid_line:
			syntax_error(BADCHAR_ERRMSG);
			break;
		case test_line:
			cur_scen_last_line =	add_scen_line(cur_scen, cur_scen_last_line);
			cur_scen_last_line->line_item.ptr_kind =	test_path;
			cur_scen_last_line->line_item.ptr = nstrdup(strip(linein));
			break;
		case mode_line:
			cur_scen_last_line =	add_scen_line(cur_scen, cur_scen_last_line);
			cur_scen_last_line=
				parse_exec_mode(linein, &(cur_scen_last_line->line_item),
				cur_scen,cur_scen_last_line);
			break;
		case ref_line:
			cur_scen_last_line =	add_scen_line(cur_scen, cur_scen_last_line);
			cur_scen_last_line->line_item.ptr_kind =	scenario;
			linein=strip(linein)+1;			/* move	past the ^ character */
			cur_scen_last_line->line_item.ptr = make_scen_ref(strip(linein));
			break;
		case message_line:
			cur_scen_last_line =	add_scen_line(cur_scen, cur_scen_last_line);
			cur_scen_last_line->line_item.ptr_kind =	message;
			cur_scen_last_line->line_item.ptr = nstrdup(strip_quotes(linein));
			cur_scen->line_count--;
			break;
		case comment_line:
		case empty_line:	/* do nothing */
			break;
		default:
			sprintf(error_mesg,"parse_scen -- invalid switch value:	%d\n",
				line_type(linein));
			BAIL_OUT2(error_mesg);
	}

	return cur_scen_last_line;
	
}




static scen_t *	parse_scen(scen_name)
		char *scen_name;
{
	mptr *this_mptr;
	char *linein;
	int	done=0;
	scen_t *scen_p = find_scen(scen_name);
	line_t *last_line =	NULL;
	line_t *line_p;


	if (scen_p == NULL)	return NULL;	/* no such scenario */

	/* we start parsing at the place where the scenario is in the scen file */
	
	if (scen_p->line_count < 0)	
	{		/* hasn't been parsed */
		scen_p->line_count = 0;

		/* loop while parsing this scenario */
		
		while ((linein = get_scen_line()) != NULL)	
		{
			line_e ln_type=line_type(linein);

			if (ln_type==name_line) break;
			else last_line=
				process_scenario_lines(ln_type, scen_p, last_line, linein);
		}
		 /*	make sure to parse all scenarios that are referenced */
		for	(line_p=scen_p->first_line;	line_p!=NULL; line_p=line_p->next) 
		{
			for	(this_mptr= (&(line_p->line_item));
				this_mptr->ptr_kind==group_exec;
				this_mptr=(&(((exec_t *)	(this_mptr->ptr))->next))) 
			{
				
				exec_t *this_exec= (exec_t *)this_mptr->ptr;
				if ((this_exec->mode ==	random)
					&& ((this_exec->next.ptr_kind)!=scenario) )
					scenario_error(RANDOM_ERRMSG,
						line_p->byte_offset, line_p->line_no);
			}

			/* recursively parse any referenced scenarios */
			
			if (this_mptr->ptr_kind==scenario)
				parse_scen(((scen_t	*) this_mptr->ptr)->scen_name);
		}
	}
	return scen_p;
}




#ifdef __STDC__
static void	mptr_loop_check(mptr item);
#else
static void	mptr_loop_check();
#endif

static void loop_check(scen_p)
	scen_t *scen_p;
{
	line_t *line_p;

#ifdef DBG
	(void) fprintf(stderr,"checking	scenario: %s [%d] #%d @%d\n", scen_p->scen_name,
		scen_p->line_count,	scen_p->line_no, scen_p->byte_offset);
#endif
	if (scen_p->dejavu > 0)	{
		scenario_error(LOOP_ERRMSG,	scen_p->byte_offset, scen_p->line_no);
	}
	scen_p->dejavu++;
	for	(line_p=scen_p->first_line;	line_p!=NULL;
						line_p=line_p->next) {
		mptr_loop_check(line_p->line_item);
	}
	scen_p->dejavu--;
}



static void mptr_loop_check(item)
	mptr item;
{
	exec_t *this_exec;
	switch(item.ptr_kind) 
	{
	case invalid:
		break;
	case test_path:
#ifdef DBG2
		printf ("\t@%s\n", item.ptr);
#endif
		break;
	case list_path:
#ifdef DBG2
		printf ("\t&%s\n", item.ptr);
#endif
		 break;
	case message:
#ifdef DBG2
		printf ("\t""%s""\n", item.ptr);
#endif
		break;
	case scenario:
		loop_check((scen_t *) item.ptr);
		break;
	case group_exec:
		this_exec =(exec_t *)	item.ptr;
#ifdef DBG2
		printf ("\t:%s,%d:\n", mode_image[this_exec->mode],
					this_exec->param);
#endif
		mptr_loop_check(this_exec->next);
		break;
	default:
		sprintf(error_mesg,"mptr_loop_check	-- invalid switch value: %d\n",
				item.ptr_kind);
		BAIL_OUT2(error_mesg);
	}
}




static int fork_group(item)
	mptr item;
{
	pid_t cpid;
	int	rc;

	fflush(stdout);
	fflush(stderr);
	switch (cpid=fork()) 
	{
	case -1:
		sprintf(error_mesg,"parallel couldn't fork \n");
		perror("fork");
		BAIL_OUT2(error_mesg);
		break;
	case 0:		/* child process */
		/* make sure children don't use same rand seed */
		{
			time_t tloc;
		    srand((int)time(&tloc));
		}
		build_temp_dir();
		srand((getpid() ^ time(NULL)));
		rc = do_item(item);
		fflush(stdout);
		fflush(stderr);
		do_rm(temp_dir);
		exit(rc);
	default:
		return cpid;
	}
	return cpid;
}





static int do_exec_mode(exec_p)
	exec_t *exec_p;
{
	line_t *line_p;
	scen_t *scen_p;
	int	i, proc_count, stat_val;
	time_t start_time, old_end_time;
	pid_t cpid;
	int rc=TRUE;

	switch (exec_p->mode) 
	{
	case include:
		break;
	case parallel:
	case group:

		proc_count=0;
		/* read	status for any zombie processes	*/
		while (cpid=waitpid(-1,	&stat_val, WNOHANG)>1)
			printf ("Found Zombie Process [%d]\n",cpid);
		if (exec_p->next.ptr_kind == scenario)
		{
			scen_p= (scen_t *)exec_p->next.ptr;
			for	(i=1; i<=exec_p->param;	i++) 
			{
				for	(line_p=scen_p->first_line;	line_p!=NULL
						; line_p=line_p->next) 
				{
					proc_count++;
#ifdef DBG2
					printf ("forking scen child	%d ...\n",
																proc_count);

#endif
					cpid=fork_group(line_p->line_item);
#ifdef DBG2
					printf ("forked	scen child %d [%d]...\n",
																proc_count,	cpid);
#endif

				}
			}
		} 
		else 
		{
			for	(proc_count=1; proc_count<=exec_p->param; proc_count++)	
			{
#ifdef DBG2
				printf ("forking child %d ...\n", proc_count);
#endif

				cpid=fork_group(exec_p->next);
#ifdef DBG2
				printf ("forked	child %d [%d]...\n", 
					proc_count, cpid);
#endif
			}
			proc_count--;
		}
		while (proc_count--	>0)	
		{
#ifdef DBG2
			printf ("Waiting for proc %d ...\n", proc_count);
#endif

			cpid=wait(&stat_val);
#ifdef DBG2
			printf ("child %d [%d] is done ...\n", proc_count, cpid);
#endif

		}
		break;
	case random:
		if (exec_p->next.ptr_kind == scenario)
		{
			/* (scen_t *) =	exec_t...(void *) */
			scen_p= (scen_t *)exec_p->next.ptr;
			i =	rand() % scen_p->line_count;
			for(line_p=scen_p->first_line;
					(i>0) || (line_p->line_item.ptr_kind ==	message);
					line_p=line_p->next) 
			{
				if(line_p->line_item.ptr_kind != message) i--;
			}
			(void) do_item(line_p->line_item);
		} 
		else 
		{
			scenario_error(RANDOM_ERRMSG,
				line_p->byte_offset, line_p->line_no);
		}

		break;

	case remote: /* Not yet implemented */

		scenario_error(REMOTE_ERRMSG, scen_p->byte_offset, scen_p->line_no);
		break;

	case repeat:
		for	(i=1; (i<=exec_p->param) &&	(time(NULL)<=end_time);	i++)
			 (void)	do_item(exec_p->next);
		break;

	case timed_loop:
		start_time = time(NULL);
		old_end_time = end_time;
		end_time =	 start_time	+ (time_t) exec_p->param;
		if (end_time > old_end_time) end_time =	old_end_time;

#ifdef DBG
	(void) fprintf(stderr,";timed_loop,%ld; [%ld .. %ld]\n",
				exec_p->param, end_time, time(NULL));
	(void) fprintf(stderr,";timed_loop,%lx; [%lx .. %lx]\n",
				exec_p->param, end_time, time(NULL));
#endif
		while (time(NULL)<=end_time) 
		{
			(void) do_item(exec_p->next);
		}
		end_time = old_end_time;
		break;
	default:
		sprintf(error_mesg,"do_exec_mode --	invalid	switch value: %d\n",
				exec_p->mode);
		BAIL_OUT2(error_mesg);
	}
	return rc;
}



static int do_item(item)
	mptr item;
{
        extern bool resume_opt;
        static int resuming = FALSE;
 
	int	old_build_mode,	old_exec_mode, old_clean_mode;
	int	rc=TRUE;

	switch (item.ptr_kind) 
	{
	case invalid:
		sprintf(error_mesg,"do_item	-- invalid item	pointer.\n");
		BAIL_OUT2(error_mesg);
		break;
	case list_path:
		break;
	case scenario:
		/*rc =*/ do_scen ((scen_t *)item.ptr);
		break;
	case message:
		(void) jnl_entry_scen((char *)item.ptr);
		break;
	case test_path:
		if (resume_opt && !resuming) 
		{
			if (strcmp ((char *) item.ptr, g_tc_line) == 0)
				resuming = TRUE;
			else
				return SUCCESS;
		}
 
		if (build_mode == TRUE)	
		{
			tet_env	= bld_env;
			rc = ntool_tc((char *)item.ptr,BUILD);		/* do the 'build' */
		}
		if ((exec_mode == TRUE)	&& (rc != FAILURE))	
		{
			tet_env	= exe_env;
			rc = nexec_tc((char *)item.ptr);			/* do the 'execute'	*/
		}
		if ((clean_mode	== TRUE) &&	(rc	!= FAILURE)) 
		{
			tet_env	= cln_env;
			rc = ntool_tc((char *)item.ptr,CLEAN);		/* do the 'clean' */
		}
		break;
	case group_exec:
		old_build_mode = build_mode;
		old_exec_mode  = exec_mode;
		old_clean_mode = clean_mode;

		if (old_build_mode == TRUE)	
		{
			build_mode = TRUE;
			exec_mode  = clean_mode	= FALSE;
			rc = do_item(((exec_t *)item.ptr)->next);
		}

		if ((old_exec_mode == TRUE)	&& (rc != FAILURE))	
		{
			exec_mode  = TRUE;
			clean_mode = build_mode	= FALSE;
			rc = do_exec_mode((exec_t *)item.ptr);
		}

		if ((old_clean_mode	== TRUE) &&	(rc	!= FAILURE)) 
		{
			clean_mode = TRUE;
			build_mode = exec_mode = FALSE;
			rc = do_item(((exec_t  *)item.ptr)->next);
		}

		build_mode = old_build_mode;
		exec_mode  = old_exec_mode;
		clean_mode = old_clean_mode;
		break;
	default:
		sprintf(error_mesg,"do_item	-- invalid switch value: %d\n",
				item.ptr_kind);
		BAIL_OUT2(error_mesg);
	}
	return rc;
}



static do_scen(scen_p)
	scen_t *scen_p;
{
	line_t *line_p;

	/* Execute Each	Scenario Line */
	for	(line_p=scen_p->first_line;
				(line_p!=NULL) && (time(NULL)<=end_time);
				line_p=line_p->next) 
	{
				(void) do_item(line_p->line_item);
	}
}

int	perform_scen()
{
	scen_t *requested_scen;
	int	pipe_fd[2];
	int	activity_ctr = 0;

#ifdef DBG
	(void) fprintf(stderr,"perform_scen()\n");
#endif
#ifdef DBG2
	(void) fprintf(stderr,"Scenario	file is	'%s'\n", scen_file_name);
	(void) fprintf(stderr,"Scenario	name is	'%s'\n", scenario_name);
#endif
	if ((scen_stream = fopen(scen_file_name, "r")) == NULL)
	{
		(void) sprintf(error_mesg,"can't open scenario file	: %s\n",
					scen_file_name);
		perror("fopen(scenario)");
		BAIL_OUT2(error_mesg);
	}

	/* make	sure the file descriptor is	closed on an exec call */
	(void) fcntl(fileno(scen_stream), F_SETFD, FD_CLOEXEC);

	requested_scen = parse_scen(scenario_name);
	if (requested_scen == NULL)	
	{
		sprintf(error_mesg,"Could not find scenario: %s",
					scenario_name);
		syntax_error(error_mesg);
	}
	loop_check (requested_scen);

	/* set end_time	to be the largest possable time	value */

	end_time= ~	(time_t) 0;
	if (end_time < (time_t)0) 
	{

	/* time_t must be signed so turn off the sign bit */

		int	i;
		end_time= (time_t) 0x7F;

		for	(i=sizeof(time_t); i>1;	i--) 
		{
			end_time = (end_time <<	8) | 0xFF;
		}
	}

	/* Create Dispenser	Process	*/

	if (pipe(pipe_fd) <	0) 
	{
		perror("pipe");
		BAIL_OUT2("Can't create	pipe.");
	}

	if (fork()==0) 
	{ 		/*	Child process is dispenser process */

		struct sigaction act;
		act.sa_handler = SIG_IGN;
		act.sa_flags = 0;

		if (sigaction(SIGPIPE, &act, NULL) < 0)
		{
			perror("sigaction");
			BAIL_OUT2("Can't set SIGPIPE sigaction.");
		}

		close (pipe_fd[0]);	/* close reading end of	pipe */
		while (write(pipe_fd[1],(char *)&activity_ctr,sizeof(activity_ctr))>0) 
		{
			activity_ctr +=	1;
		}

		exit (0);
	}

	index_dispenser_fd = pipe_fd[0];
	close (pipe_fd[1]);

	do_scen(requested_scen);

	fclose (scen_stream);
	return (SUCCESS);
}


