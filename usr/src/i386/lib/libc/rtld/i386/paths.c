#ident	"@(#)rtld:i386/paths.c	1.25"

/* PATH setup and search functions */


#include "machdep.h"
#include "rtld.h"
#include "externs.h"
#include "paths.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <elf.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>

#include <priv.h>
#include <sys/secsys.h>

/* directory search path - linked list of directory names */
struct pathnode {
	CONST char	*name;
	int		len;
	struct pathnode	*next;
};

static struct pathnode *rt_dir_list = (struct pathnode *)0;

/* default search directory */
static CONST struct pathnode libdirs[] = {
	{ DEF_LIBDIR, DEF_LIBDIRLEN, (struct pathnode *)0 }
};

/* function that determines whether a file
 * has already been loaded; if so, returns a pointer to its
 * link structure; else returns a NULL pointer
 * We check inode and device number, rather than name itself.
 */
static rt_map *
so_loaded(dev, ino)
dev_t	dev;
ino_t	ino;
{
	register rt_map	*lm;

	DPRINTF(LIST,(2, "rtld: so_loaded(%d, %d)\n",dev, ino));

	for (lm = (rt_map *)NEXT(_rt_map_head); lm;  /* skip a.out */
		lm = (rt_map *)NEXT(lm)) 
	{
		if ((DEV(lm) == dev) &&
			(INO(lm) == ino)) 
		{
			return(lm);
		}
	}
	if ((DEV(_rtld_map) == dev) &&
		(INO(_rtld_map) == ino))
	{
		return _rtld_map;
	}
	return((rt_map *)0);
}

/*
* Given a library name converts it into the corresponding filename.
*
* Algorithm : if library name contains a '/' returns it untouched
*		else go through  rt_dir_list and find the first
*		directory that contains a file with the same name
*		as the library and return its absolute name.
*/

static int 
translate_lib_name(name, path, out_path)
char	*name, *path;
char	**out_path;
{
	char   *ptr;
	int	name_len;
	int	fd;
#ifdef GEMINI_ON_OSR5
	int	pass;
#endif
	register struct pathnode *dir_info;

#ifdef GEMINI_ON_OSR5
	if (*name == '/')
	{
		/* absolute pathname - try with alternate
		 * prefix first, then name itself
		 */
		if ((_rtstrlen(name) + ALT_PREFIX_LEN + 1) <= PATH_MAX)
		{
			_rtstrcpy(path, ALT_PREFIX);
			_rtstrcpy(path + ALT_PREFIX_LEN, name);
			DPRINTF(PATH, (2, "translate_lib_name: trying: %s\n", path));
			if ((fd = _rtopen(path, O_RDONLY)) >= 0)
			{
				*out_path = path;
				return fd;
			}
		}
	}
#endif
	for(ptr = name; *ptr; ptr++)
	{
		if (*ptr == '/') 
		{
			DPRINTF(PATH, (2, "translate_lib_name: trying: %s\n", name));
			if ((fd = _rtopen(name, O_RDONLY)) == -1)
				return -1;
			*out_path = name;
			return fd;
		}
	}

	name_len = (ptr - name) + 1;
	/* 1 for the '/' between directory and name
	 * in the path we will build 
	 */

#ifdef GEMINI_ON_OSR5
	/* first check each path prepended with alternate prefix
	 * name - then go through list again checking paths
	 * without prefix
	 */
	_rtstrcpy(path, ALT_PREFIX);
	ptr = path + ALT_PREFIX_LEN;
	*ptr++ = '/';
	name_len += ALT_PREFIX_LEN + 1;
	pass = 0;
loop:
#else
	ptr = path;
#endif
	for(dir_info = rt_dir_list; dir_info; dir_info = dir_info->next)
	{
		char	*p;

		if ((name_len + dir_info->len) >= PATH_MAX)
			continue;
		p = ptr;
		_rtstrcpy(p, dir_info->name);
		p += dir_info->len;
		*p++ = '/';
		_rtstrcpy(p, name);

		DPRINTF(PATH, (2, "translate_lib_name: trying: %s\n", path));
		if ((fd = _rtopen(path, O_RDONLY)) >= 0) 
		{
			*out_path = path;
			return fd;
		}
	}
#ifdef GEMINI_ON_OSR5
	if (pass == 0)
	{
		ptr = path;
		name_len -= (ALT_PREFIX_LEN + 1);
		pass++;
		goto loop;
	}
#endif
	return -1;
}

/*
 * Given a library name, find that library by searching the list
 * of directories; if found, determine whether we have already
 * loaded that library.  Return information in id.  Returns
 * 0 for failure, 1 for success.
*/

int 
_rt_so_find(lib_name, id)
CONST char	*lib_name;
object_id	*id;  
{
#ifdef GEMINI_ON_OSR5
	struct osr5_stat32	sbuf;
#else
	struct stat	sbuf;
#endif
	char		path[PATH_MAX+1];
	int		filedes;
	char		*out_path;

	DPRINTF(LIST, 
		(2, "rtld: _rt_so_find(%s, 0x%x)\n", (lib_name ? lib_name : (CONST char *)"0"), (ulong_t)id));

	if (!*lib_name) 
	{
		_rt_lasterr(
			"%s: %s: attempt to open file with null name",
				_rt_name, _rt_proc_name);
		return 0;
	}

	filedes = translate_lib_name((char *)lib_name, path, &out_path);
	if ((filedes >= 0) &&
		(_rtfstat(filedes, &sbuf) != -1))
	{
		rt_map	*lm;
		if ((lm = so_loaded(sbuf.st_dev, sbuf.st_ino)) != 0)
		{
			/* already loaded */
			(void)_rtclose(filedes);
			id->n_lm = lm;
			return 1;
		}
		if (sbuf.st_size == 0)
		{
			_rt_lasterr(
			"%s: %s: attempt to open zero length file %s",
				_rt_name, _rt_proc_name, out_path);
			(void)_rtclose(filedes);
			return 0;
		}
		id->n_lm = 0;
		id->n_ino = sbuf.st_ino;
		id->n_dev = sbuf.st_dev;
		id->n_fd = filedes;
		id->n_name = 
			(char *)_rtmalloc(_rtstrlen(out_path)+1);
		if (id->n_name == 0) 
		{
			(void)_rtclose(filedes);
			return 0;
		}
		_rtstrcpy(id->n_name, out_path);
		return 1;
	} 
	else 
	{
		/* Could not find some shared object */
		_rt_lasterr("%s : %s : error opening %s", 
			_rt_name, _rt_proc_name, lib_name);
		return 0;
	}
}

/* set up path struct for either LD_LIBRARY_PATH or DT_RPATH 
 * (if envdir is set, it is LD_LIBRARY_PATH);
 * takes pointer to last pathnode allocated, the path list and space
 * big enough to copy the list, adding trailing null bytes; 
 * returns pointer to last node added;
 */
 
static struct pathnode *
process_path(cur_node, dirs, dir_copy, envdir)
struct pathnode	*cur_node; 
CONST char	*dirs; 
char		*dir_copy;
int		envdir;
{
	struct pathnode	*next_node;

	/* we have already checked that the dirs string is not empty */
	for(;; dirs++) 
	{
		if ((next_node = (struct pathnode *)
			_rtmalloc(sizeof(struct pathnode))) == 0)
			return 0;
		next_node->name = dir_copy;
		if (cur_node)
			cur_node->next = next_node;
		else
			rt_dir_list = next_node;
		cur_node = next_node;
		while (*dirs && (*dirs != ':') &&
			!(envdir && (*dirs == ';')))
		{
			/* envdir path is of form [PATH1][;PATH2] */
			*dir_copy++ = *dirs++;
		}
		next_node->len = dir_copy - next_node->name;
		if (!next_node->len)
		{
			*dir_copy++ = '.';
			next_node->len = 1;
		}
		*dir_copy++ = '\0';

		if (!*dirs) 
			break;
	}
	return next_node;
}

/* set up directory search path: rt_dir_list
 * consists of directories (if any) from run-time list
 * in a.out's dynamic, followed by directories (if any)
 * in environment string LD_LIBRARY_PATH, followed by DEF_LIBDIR;
 * if we are running setuid or setgid, no directories from LD_LIBRARY_PATH
 * are permitted
 * returns 1 on success, 0 on error
 */
int 
_rt_setpath(envdirs, rundirs, use_ld_lib_path)
CONST char	*envdirs, *rundirs;
int		use_ld_lib_path;
{
	int		elen = 0, rlen = 0;
	register char	*rdirs, *edirs;
	struct pathnode *cur_node = 0;

	DPRINTF(LIST,(2, "rtld: rt_setpath(%s, %s, %d)\n",
		envdirs ? envdirs : (CONST char *)"0", 
		rundirs ? rundirs : (CONST char *)"0", use_ld_lib_path));

	/* allocate enough space for rundirs and envdirs
	 * we allocate space for
	 * twice the size of envdirs and rundirs to allow for
	 * extra nulls at the end of each directory (foo::bar
	 * becomes foo\0.\0bar\0); this is overkill, but allows
	 * for the worst case and is faster than malloc'inc space
	 * for each directory individually
	 */
	if (envdirs)
		elen = _rtstrlen(envdirs);
	if (rundirs)
		rlen = _rtstrlen(rundirs);
	if ((rlen + elen) > 0) 
	{
		if ((rdirs = (char *)_rtmalloc(2 * (elen + rlen)))
			== 0)
			return 0;
		edirs = rdirs + (2 * rlen);
		if (rundirs && rlen)
		{
			if ((cur_node = process_path(cur_node, rundirs,
				rdirs, 0)) == 0)
				return 0;
		}
		if (envdirs)
		{
			/* see if we are running secure;
			 * if use_ld_lib_path >= 0, kernel has
			 * told us whether we can use environment;
			 * otherwise we must figure it out.
			 */
			if (use_ld_lib_path < 0)
			{
#ifdef GEMINI_ON_OSR5
				/* just check for real user id ==
				 * effective user id and real group id
				 * == effective group id
				 */
				if ((_rtcompeuid() >= 0) && 
					(_rtcompegid() != 0))
					use_ld_lib_path = 1;
#else
				/* if (!setuid && !setgid &&
			         * (!privileged || (realid == privid)))
				 */
				
				uid_t	realid, privid;
				if ((((realid = _rtcompeuid()) >= 0)
					&& _rtcompegid())  &&
					((_rtprocpriv(CNTPRV, 0, 0) == 0) || 
					(((privid = (uid_t)_rtsecsys(ES_PRVID,0)) != -1) &&
					privid == realid)))
					use_ld_lib_path = 1;
#endif
				else
					use_ld_lib_path = 0;
			}
			if (use_ld_lib_path)
			{
				if (*envdirs == ';')
					envdirs++;
				if (*envdirs)
				{
					if ((cur_node = process_path(cur_node,
						envdirs, edirs, 1)) == 0)
						return 0;
				}
			}
		}
	}
	/* add DEF_LIBDIR to end of list */
	if (!cur_node)
		rt_dir_list = (struct pathnode *)libdirs;
	else 
		cur_node->next = (struct pathnode *)libdirs;
#ifdef DEBUG
	if (_rt_debugflag & (LIST|PATH)) 
	{
		cur_node = rt_dir_list;
		while(cur_node) 
		{
			_rtfprintf(2, "search path: %s\n", cur_node->name);
			cur_node = cur_node->next;
		}
	}
#endif
	return(1);
}
