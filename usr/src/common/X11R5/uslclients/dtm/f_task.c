#pragma ident	"@(#)dtm:f_task.c	1.103.1.31"

/******************************file*header********************************

    Description:
	This file contains the source code for processing file-related "tasks".
*/
						/* #includes go here	*/
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "error.h"
#include "extern.h"

/**************************forward*declarations***************************
 *
 * Forward Procedure declarations
 */
static void	BeginFileOp(DmTaskInfoListPtr tip);
static int	BuildSubtasksForDir(DmTaskInfoListPtr, DmFileOpType,
		    		char *, char *, DmTaskListPtr **q, uint);
static Boolean	ExecuteTask(XtPointer);
static int	Dm__FileType(char * filename, struct stat *st);
static void	FreeSrcList(DmTaskInfoListPtr tip);
static void	FreeTasks(DmTaskListPtr current);
static DmTaskInfoListPtr GetTaskInfoItem(void);
static int	DmMakeDirectory(char * src, char * dst);
static void	MakeTask(DmFileOpType, char *, char *, DmTaskListPtr **);
static void	RegFileOpWorkProc(DmTaskInfoListPtr tip);
static DmTaskInfoListPtr ReturnError(DmTaskInfoListPtr, int, char * name);
static Boolean	ScheduleNextTask(void);

static int	CopyBlock(char *, char *, int *, int *, mode_t);
static int	DmCopyFile(char * src, char * dst, int * rfd, int * wfd);
static int	DmDeleteFile(char * src);
static int	DmLinkFile(char * src, char * dst);
static int	MakeLinkInstead(char * src, char * dst);
static int	DmMoveFile(char * src, char * dst);
static int	DmRmDir(char * src);
static int	DmSymLinkFile(char * src, char * dst);

int		legalfmove(unsigned char *c);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/* For subtasks of type DM_ENDOVER or DM_BEGIN, non-NULL source and targets
   are source indexes NOT pointers so don't free them.
*/
#define FreeTaskContents(task) \
    if (((task)->type != DM_ENDOVER) && ((task)->type != DM_BEGIN)) \
	XtFree((task)->source), XtFree((task)->target)

#undef TRUE
#undef FALSE
#define FALSE	0
#define TRUE	1

#define D2U	0
#define U2D	1

#define CR	0xd	/* carriage return */
#define LF	0xa	/* line feed */
#define DOSEOF	0x1a	/* control-z */

/* Operations in target table.
   These are defined in the order of "execution" (that is, OVER_WRITE must be
   considered, built (if necessary) and executed before MSRC_MKDIR, etc).
   This is done for documenting purposes only; the bit values are not
   important.

   OVER_WRITE: unconditionally generate OVERWRITE tasks and associated tasks
   MSRC_MKDIR: check for multiple srcs and make dir (target_path) if so.
   ADJUST_TARGET: target_path is dir (not necessarily existing dir) so src
	basenames must be appended to target_path to get real 'target'.
	(ADJUST_TARGET is also generated on-the-fly (outside of table))
*/
#define OVER_WRITE	(1 << 0)
#define MSRC_MKDIR	(1 << 1)
#define ADJUST_TARGET	(1 << 2)
#define CK_NM		(1 << 3)
#define CK_SZ		(1 << 4)

static const unsigned short target_table[] = {
	MSRC_MKDIR,				/* NO_FILE    */
	ADJUST_TARGET,				/* IS_DIR     */
	OVER_WRITE | MSRC_MKDIR,		/* IS_FILE    */
	OVER_WRITE | MSRC_MKDIR,		/* IS_SPECIAL */
	OVER_WRITE,				/* Should never happen
						   just a place holder  */
	ADJUST_TARGET | CK_NM |CK_SZ,		/* IS_DOSDIR  */
	OVER_WRITE | MSRC_MKDIR | CK_NM | CK_SZ,/* IS_FILE    */
	ADJUST_TARGET |CK_SZ,			/* IS_S5DIR     */
	OVER_WRITE | MSRC_MKDIR | CK_SZ,	/* IS_S5FILE    */
	OVER_WRITE | MSRC_MKDIR | CK_SZ,	/* IS_S5SPECIAL */
	MSRC_MKDIR,				/* PS_DIR    */
	MSRC_MKDIR |CK_SZ,			/* PS_S5DIR    */
	MSRC_MKDIR | CK_NM | CK_SZ,		/* PS_DOSDIR    */
};


/* Operations in opcode table
   The op defines are grouped to give a hint about order of "execution" (this
   is done for documenting purposes only; the bit values are not important):
	The 2nd group are the basic file operations.
	The 1st group occur before these.
	The 3rd group deal with recursively descending into a dir.

   CHK_FS: check for moving across file systems
   CHK_CP: check for invalid copies (ie, can't copy special files)
   CHK_SAME: check for same src & target
   CHK_DESC: check if target is same OR descendant of src
   CP_DEL_OP: copy-then-delete; for instance, to simulate moving across fs
   RECUR_*: recursively operate on src (ie, when src is dir)
*/
#define CHK_FS		(1 << 0)
#define CHK_CP		(1 << 1)
#define CHK_SAME	(1 << 2)
#define CHK_DESC	(1 << 3)

#define CP_OP		(1 << 4)
#define MV_OP		(1 << 5)
#define LN_OP		(1 << 6)	/* Symbolic and hard links */
#define DEL_OP		(1 << 7)
#define CP_DEL_OP	(1 << 8)
#define MV_DIR		(1 << 9)

#define MKDIR_OP	(1 << 10)
#define RECUR_CP	(1 << 11)
#define RECUR_DEL	(1 << 12)
#define RECUR_CP_DEL	(1 << 13)
#define RMDIR_OP	(1 << 14)


/* If ERROR_OP is set, the error code is in the rest of the LSBs */
#define ERROR_OP	(1 << 15)

#define CONVD2U_OP	(1 << 16)
#define CONVU2D_OP	(1 << 17)

#define NUM_SRC_TYPE	5	/* none, dir, file, special, symlink */
#define NUM_OP		8	/* cp, mv, del, hlink, slink, cp_del, conv, conv */

/* Use this macros to look up the opcode_table */
#define OPCODES(SRC_TYPE, OP_TYPE)	opcode_table[SRC_TYPE][OP_TYPE - 1]

static const unsigned long opcode_table[NUM_SRC_TYPE][NUM_OP] = {
/* SRC   OP				TASKS				*/
/* NONE	COPY   */				ERROR_OP | ERR_NoSrc,
/* NONE	MOVE   */				ERROR_OP | ERR_NoSrc,
/* NONE	DELETE */				ERROR_OP | ERR_NoSrc,
/* NONE	HLINK  */				ERROR_OP | ERR_NoSrc,
/* NONE	SLINK  */				ERROR_OP | ERR_NoSrc,
/* NONE	CP_DEL */				ERROR_OP | ERR_NoSrc,
/* NONE	CONV_D2U */				ERROR_OP | ERR_NoSrc,
/* NONE	CONV_U2D */				ERROR_OP | ERR_NoSrc,

/* DIR	COPY   */ CHK_DESC|	    MKDIR_OP|	RECUR_CP,
/* DIR	MOVE   */ CHK_DESC|CHK_FS|  MV_DIR,	/* see DmDoFileOp */
/* DIR	DELETE */				RECUR_DEL|  RMDIR_OP,
/* DIR	HLINK  */				ERROR_OP | ERR_NoDirHardlink,
/* DIR	SLINK  */ CHK_SAME|	    LN_OP,
/* DIR	CP_DEL */ CHK_DESC|	    MKDIR_OP|	RECUR_CP_DEL|  RMDIR_OP,
/* DIR	CONV_D2U  */				ERROR_OP | ERR_NoDirConvert,
/* DIR	CONV_U2D  */				ERROR_OP | ERR_NoDirConvert,

/* FILE	COPY   */ CHK_SAME|	    CP_OP,
/* FILE	MOVE   */ CHK_SAME|CHK_FS|  MV_OP,
/* FILE	DELETE */		    DEL_OP,
/* FILE	HLINK  */ CHK_SAME|	    LN_OP,
/* FILE	SLINK  */ CHK_SAME|	    LN_OP,
/* FILE	CP_DEL */ CHK_SAME|	    CP_DEL_OP,
/* FILE	CONV_D2U   */ CHK_SAME|	    CONVD2U_OP,
/* FILE	CONV_U2D   */ CHK_SAME|	    CONVU2D_OP,

/* SPCL	COPY   */				ERROR_OP | ERR_CopySpecial,
/* SPCL	MOVE   */ CHK_SAME|CHK_FS|CHK_CP| MV_OP,
/* SPCL	DELETE */		    DEL_OP,
/* SPCL	HLINK  */ CHK_SAME|	    LN_OP,
/* SPCL	SLINK  */ CHK_SAME|	    LN_OP,
/* SPCL	CP_DEL */				ERROR_OP | ERR_CopySpecial,
/* SPCL	CONV_D2U   */				ERROR_OP | ERR_ConvertSpecial,
/* SPCL	CONV_U2D   */				ERROR_OP | ERR_ConvertSpecial,

/* SYML	COPY   */ CHK_SAME|	    CP_OP,
/* SYML	MOVE   */ CHK_SAME|CHK_FS|  MV_OP,
/* SYML	DELETE */		    DEL_OP,
/* SYML	HLINK  */ CHK_SAME|	    LN_OP,
/* SYML	SLINK  */ CHK_SAME|	    LN_OP,
/* SYML	CP_DEL */ CHK_SAME|	    CP_DEL_OP,
/* SYML	CONV_D2U   */ CHK_SAME|	    CONVD2U_OP,
/* SYML	CONV_U2D   */ CHK_SAME|	    CONVU2D_OP,
};

/*
 * This is used as a cache for container ptrs. Because for each file
 * involved in file ops we need to copy instance properties, there are lots
 * of container opens and closes. Instead of really opening and closing
 * containers, we cache the container info in a cache. When a task list
 * is completed, it calls FlushCache() to empty the cache.
 */
static DmContainerPtr lcache[8] = { 0 };
#define CACHE_SIZE	sizeof(lcache) / sizeof(lcache[0])

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    FlushCache- decrement reference counts of remaining containers in the
    cache to zero. This should be called after a task list is completed.
*/
static void
FlushCache()
{
	int i;
	DmContainerPtr *cpp;

	for (i=0, cpp = lcache; i < CACHE_SIZE; i++, cpp++)
		if (*cpp) {
			DmCloseContainer(*cpp, DM_B_NO_FLUSH);
			*cpp = 0;
		}
}

static DmContainerPtr
OpenDir(char *path, int options)
{
	DmContainerPtr cp, last;
	DmContainerPtr *cpp;
	int i;

	cp = DmOpenDir(path, options);
	if (cp) {
		/* check the cache */
		for (i=0, cpp = lcache; i < CACHE_SIZE; i++, cpp++) {
			if (cp == *cpp) {
				/* a cache hit */
				if (i) {
					/* move this slot to the top of the cache list */
					memmove((char *)&lcache[1], (char *)lcache,
						i * sizeof(lcache[0]));
					lcache[0] = cp;
				}

				return(cp);
			}
		} /* for */

		/* cache miss */
		last = lcache[CACHE_SIZE - 1];

		/* throw away the last slot */
		if (last)
				DmCloseContainer(last, DM_B_NO_FLUSH);

		/* shift the slots. Slot 0 is more recent */
		memmove((char *)&lcache[1], (char *)lcache,
			(CACHE_SIZE - 1) * sizeof(lcache[0]));
		lcache[0] = cp;

		/* bump the reference count */
		cp->count++;
	}

	return(cp);
}

#define FILEOP_CREATE	(1 << 0)

static DmObjectPtr
GetObjInfo(char *filepath, DmContainerPtr *cp, int options)
{
	DmObjectPtr op;
	char *p;
	char *path;
	char *name;

	p = strrchr(filepath, '/');

	/* p cannot be NULL, because filepath is a fully qualified path */
	if (p == filepath) {
		path = "/";
		name = filepath + 1;
	}
	else {
		*p = '\0';
		path = filepath;
		name = p + 1;
	}

	if (*cp = OpenDir(path, DM_B_READ_DTINFO)) {
		if (op = DmGetObjectInContainer(*cp, name)) {
			*p = '/';
			return(op);
		}
		else {
			/* object does not exist */
			/* assume that either there is no view for this container or
			 * that the sync timer hasn't kick in yet.
			 */
			if (options & FILEOP_CREATE) {
				/*
				 * The INIT_FILEINFO option is needed because of the sync
				 * timer. The sync timer will throw away objects without
				 * the fileinfo struct, and thus will lose the instance
				 * properties we are storing.
				 *
				 * The DM_B_NEWBORN is a new flag to indicate the object
				 * is newly created and should be treated as such. See
				 * DmAddObjectToContainer for more info.
				 *
				 * If the obj is a broken symbolic link,
				 * Dm_CreateObj will not init fileinfo.
				 */
				op = Dm__CreateObj(*cp, name, DM_B_INIT_FILEINFO);
				if (op) {
					op->attrs |= DM_B_NEWBORN;
					*p = '/';
					return(op);
				}
			}
			else
				/* error, no need to flush container */
				DmCloseContainer(*cp, DM_B_NO_FLUSH);
		}
	}
	*p = '/';
	return(NULL);
}

/****************************procedure*header*****************************
	CopyObjProps- copies instance properties of src to dst.
*/
static int
CopyObjProps(char *dst, char *src)
{
	DmContainerPtr src_cp, dst_cp;
	DmObjectPtr    src_op, dst_op;

	if (!(src_op = GetObjInfo(src, &src_cp, 0)))
		return(-1);

	if (src_cp->fstype == DOSFS_ID(Desktop)) {
		/* don't do anything because it is a DOS FS */
		DmCloseContainer(src_cp, DM_B_NO_FLUSH);
		return(0);
	}

	if (!(dst_op = GetObjInfo(dst, &dst_cp, FILEOP_CREATE))) {
		DmCloseContainer(src_cp, DM_B_NO_FLUSH);
		return(-1);
	}

	if (dst_cp->fstype == DOSFS_ID(Desktop)) {
		/* don't do anything because it is a DOS FS */
		DmCloseContainer(dst_cp, DM_B_NO_FLUSH);
		DmCloseContainer(src_cp, DM_B_NO_FLUSH);
		return(0);
	}

	/* copy instance properties */
	DtCopyPropertyList(&(dst_op->plist), &(src_op->plist));
#ifdef NOT_USE
	dst_op->x = src_op->x;
	dst_op->y = src_op->y;
#endif

	DmCloseContainer(dst_cp, 0);
	DmCloseContainer(src_cp, DM_B_NO_FLUSH);
	return(0);
}

/****************************procedure*header*****************************
    BeginFileOp- common code to begin file op: call client_proc and register
	work proc (if needed).

	Caller may want to take action before operation is actually started
	(busying windows, etc).
*/
static void
BeginFileOp(DmTaskInfoListPtr tip)
{
    if (tip->opr_info->options & OPRBEGIN)
	(*tip->client_proc)(DM_OPRBEGIN, tip->client_data,
			    (XtPointer)tip, NULL, NULL);

    /* register work proc with Xt, if one is not already registered */
    RegFileOpWorkProc(tip);
}

/****************************procedure*header*****************************
    BuildSubtasksForDir-  This routine recursively descend into subdirs and
    generate subtasks for each file. The "type" should be either DM_COPY or
    DM_DELETE.  'srcname' and 'dstname' are assume MALLOC'ed strings, and
    will be freed at the bottom of the function. The return code is 0 for
    success or err_code in case of error.
*/
static int
BuildSubtasksForDir(DmTaskInfoListPtr tip, DmFileOpType type, char * srcname, 
		    char * dstname, DmTaskListPtr ** q, uint cknm)
{
    int			ret = 0;
    DIR 		*dp;
    struct dirent 	*directory;
    char 		*fullsrcpath;
    char		*fnmp;
    char		dosnm[DOS_MAXPATH];
    int			source_type;
    int			op_vec;

    /* Recursively operate on contents */
    if ( (dp = opendir(srcname)) == NULL )
    {
	ret = ERR_OpenDir;
	goto done;
    }

    fullsrcpath = NULL;		/* in case there is an empty directory */

    while ( (directory = readdir(dp)) != NULL )
    {
	/* don't do anything with "." or ".." entries */
	if (!strcmp(directory->d_name, ".") ||
	    !strcmp(directory->d_name, ".."))
	    continue;

	/* Make full src path */
	fullsrcpath = strdup(DmMakePath(srcname, directory->d_name));

	source_type = Dm__FileType(fullsrcpath, NULL);
	op_vec = OPCODES(source_type, type);

	if (op_vec & ERROR_OP)
	{
	    ret = op_vec & ~ERROR_OP;
	    break;
	}

	/* There is no need to do CHK_FS here? */
	op_vec &= ~CHK_FS;

	if (op_vec & MKDIR_OP) {
	    char *fnmp = directory->d_name;
	    if (cknm) {
		if (unix2dosfn(fnmp, dosnm, legalfmove, strlen(fnmp)) == -1) {
			MakeTask(DM_DELETE, strdup(fullsrcpath), NULL, q);
			continue;
		} else
			fnmp = dosnm;

	    }
	    MakeTask(DM_MKDIR, strdup(fullsrcpath), 
			 strdup(DmMakePath(dstname, fnmp)), q);
	}

	if (op_vec & CHK_CP)
	    if (op_vec & (CP_OP | CP_DEL_OP))
	    {
		ret = ERR_CopySpecial;
		break;
	    }


	if (op_vec & (CP_OP | CP_DEL_OP))
	{
	    char *fnmp = directory->d_name;

	    /* skip over .dtinfo files when copying a directory, because we
	     * are going to copy instance properties for each file.
	     */
	    if (!strcmp(directory->d_name, DM_INFO_FILENAME))
		continue;

	    if (cknm)
	    {
		if (unix2dosfn(fnmp, dosnm, legalfmove, strlen(fnmp)) == -1)
		{
		    if (op_vec & CP_DEL_OP)
			MakeTask(DM_DELETE, strdup(fullsrcpath), NULL, q);
		    continue;
		} else
		    fnmp = dosnm;
	    }
	    MakeTask((op_vec & CP_OP) ? DM_COPY : DM_COPY_DEL,
		     strdup(fullsrcpath),
		     strdup(DmMakePath(dstname,fnmp)),
		     q);

	} else if (op_vec & DEL_OP)
	    MakeTask(DM_DELETE, strdup(fullsrcpath), NULL, q);

	else if (op_vec & (RECUR_CP | RECUR_DEL | RECUR_CP_DEL))
	{
	    ret =
		BuildSubtasksForDir(tip,
				    (op_vec & RECUR_CP) ? DM_COPY :
				    (op_vec & RECUR_DEL) ? DM_DELETE :
				    DM_COPY_DEL,
				    strdup(fullsrcpath),
				    strdup(DmMakePath(dstname,
						      directory->d_name)),
				    q, cknm);

	    if (ret != 0)
		break;
	}

	if (op_vec & RMDIR_OP)
	    MakeTask(DM_RMDIR, strdup(fullsrcpath),
		     strdup(DmMakePath(dstname,directory->d_name)),
		     q);

	FREE((void *)fullsrcpath);
	fullsrcpath = NULL;	/* so it's not freed again outside of loop */
    }				/* while */
    (void)closedir(dp);

    if (fullsrcpath != NULL)
	FREE((void *)fullsrcpath);

 done:
    if (dstname != NULL)
	FREE((void *)dstname);
    if (srcname != NULL)
	FREE((void *)srcname);
    return(ret);
}					/* end of BuildSubtasksForDir */

/****************************procedure*header*****************************
 * ConvertDosToUnix-
 */
static int
ConvertDosToUnix(char * r_buf, char * w_buf, int count)
{
	static int crseen = FALSE;
	int  w_ptr = 0;
	while (count-- > 0)
	{
		switch(*r_buf)
		{
		case CR:
			if (crseen)
				w_buf[w_ptr++] = CR;
			else
				crseen = TRUE;
			break;
		case DOSEOF:
			if (crseen) 
			{
				w_buf[w_ptr++] = CR;
				crseen = FALSE;
			}
			return(w_ptr);
		case LF:
			crseen = FALSE;
			w_buf[w_ptr++] = LF;
			break;
		default:
			if (crseen)
			{
				w_buf[w_ptr++] = CR;
				crseen = FALSE;
			}
			w_buf[w_ptr++] = *r_buf;
		}
		r_buf++;
	} 
	return(w_ptr);

}

/****************************procedure*header*****************************
 * ConvertUnixToDos-
 */
static int
ConvertUnixToDos(char * r_buf, char * w_buf, int count)
{
	int  w_ptr = 0;
	while (count-- > 0)
	{
		switch(*r_buf)
		{
		case LF:
			w_buf[w_ptr++] = CR;
			w_buf[w_ptr++] = LF;
			break;
		case EOF:
			w_buf[w_ptr++] = DOSEOF;
			return(w_ptr);
		default:
			w_buf[w_ptr++] = *r_buf;
		}
		r_buf++;
	} 
	return(w_ptr);

}

/****************************procedure*header*****************************
 * ExecuteTask() is the background work proc registered with Xt
 * by DmDoFileOp().  It gets invoked by Xt MainLoop 
 * whenever no X events are pending. The function pulls out the current task
 * to be executed (or thelast incomplete task from the Desktop/tip structure
 * and acts upon it. Except for Copy, all other sub-tasks are executed thru'
 * completion as copying one large file may take pretty long time which may
 * block X event processing during the time we are in work proc.
 * This work proc. is unregistered when no more pending task remain in any
 * of the window. Scheduling of next task and removal is done via a function
 * ScheduleNextTask() called towards the end of the function or overwirte/
 * error condition occurs.
 */

static Boolean
ExecuteTask(XtPointer client_data /* UNUSED */)
{
    char *		src;
    char *		dst;
    DmTaskInfoListPtr	tip = DESKTOP_CUR_TASK(Desktop);
    DmTaskListPtr	cur_task = tip->cur_task;
    DmFileOpType	type;
    int			err;
    DtAttrs		options;
    DmFileOpInfoPtr	opr_info;

    /* nothing to do, goto end and clean up */
    if (cur_task == NULL)
	goto done;

    type	= cur_task->type;
    src		= cur_task->source;
    dst		= cur_task->target;
    opr_info	= tip->opr_info;
    options	= opr_info->options;
    err		= 0;


    /* Possible name change: notify caller */
    if (type == DM_NMCHG)
    {
	opr_info->attrs |= NMCHG;
	(*tip->client_proc)(DM_NMCHG, tip->client_data,
			    (XtPointer)tip, src, dst);
	return (ScheduleNextTask());
    }

    /* Overwrite task: notify caller if asked, otherwise, delete target */
    if (type == DM_OVERWRITE)
    {
	if (options & OVERWRITE)
	{
	    opr_info->attrs |= OVERWRITE;
	    (*tip->client_proc)(DM_OVRWRITE, tip->client_data,
				(XtPointer)tip, src, dst);
 	} else
	{ 
	    /* The next subtask will delete the target */
 	    tip->cur_task = tip->cur_task->next;
        }

	opr_info->task_cnt++;
	return (ScheduleNextTask());
    }


    /* if last copy operation was not finished, continue */
    if (tip->rfd != -1) {
	if ((type == DM_COPY) || (type == DM_COPY_DEL))
		err = CopyBlock(src, dst, &tip->rfd, &tip->wfd, 0);
	else
	if (type == DM_CONV_D2U)
		err = ConvertBlock(src, dst, &tip->rfd, &tip->wfd, 0, D2U);
	else
	if (type == DM_CONV_U2D)
		err = ConvertBlock(src, dst, &tip->rfd, &tip->wfd, 0, U2D);
    }

    else
    {
	/* report progress of the operation */
	if ((options & REPORT_PROGRESS) &&
	    (type != DM_BEGIN) && (type != DM_ENDOVER))
	    (*tip->client_proc)(DM_REPORT_PROGRESS,
				tip->client_data, (XtPointer)tip, src, dst);

	/* and start actual file manipulation now, keep track of
	   any error that may occur.
	*/
	switch(type)
	{
	case DM_BEGIN:
	    opr_info->cur_src = (int)dst;
	    break;
	case DM_ENDOVER:
	    break;
	case DM_RMDIR:
	    err = DmRmDir(src);
	    break;
	case DM_MKDIR:
	    err = DmMakeDirectory(src, dst);
	    break;
	case DM_CONV_D2U:
	    err = DmConvertFile(src, dst, &tip->rfd, &tip->wfd, D2U);
	    break;
	case DM_CONV_U2D:
	    err = DmConvertFile(src, dst, &tip->rfd, &tip->wfd, U2D);
	    break;
	case DM_COPY:
	    err = DmCopyFile(src, dst, &tip->rfd, &tip->wfd);
	    break;
	case DM_COPY_DEL:
	    /* Delete will be processed after copy is done. See below */
	    err = DmCopyFile(src, dst, &tip->rfd, &tip->wfd);
	    break;
	case DM_MOVE:
	    err = DmMoveFile(src, dst);
	    break;
	case DM_HARDLINK:
	    err = DmLinkFile(src, dst);
	    break;
	case DM_SYMLINK:
	    err = DmSymLinkFile(src, dst);
	    break;
	case DM_DELETE:
	    err = DmDeleteFile(src);
	    break;
	default:
	    break;
	}
    }

    /* If error occured, notify caller and stop further processing.
       NOTE: ignore errors during undo
    */
    if (err)
    {
  process_err:
	if ( !(options & UNDO) )
	{
	    if (type == DM_COPY_DEL)
	    {
		switch(tip->rfd)
		{
		case -3:
		    /* Copy succeeded but delete failed (WB case):
		       Just delete dst from WB.  No need to be able to undo
		       later. "Reset" rfd so StopFileOp doesn't try to close
		       it.
		    */
		    (void)DmDeleteFile(dst);
		    tip->rfd = -1;
		    /*FALLTHROUGH*/
		case -1:
		    /* CopyBlock open() failed.  No need to undo this later */
		    break;

		case -2:
		    /* Copy succeeded but delete failed (non-WB case):
		       Change type from DM_COPY_DEL to DM_COPY.
		    */
		    /*FALLTHROUGH*/
		default:
		    /* CopyBlock failed in the middle:
		       change type from DM_COPY_DEL to DM_COPY. 
		    */
		    type = cur_task->type = DM_COPY;
		    break;
		}
	    }
	}
	DmStopFileOp(tip);		/* clean up tasks after error */
	opr_info->error = err;
	(*tip->client_proc)(DM_ERROR, tip->client_data,
			    (XtPointer)tip, src, dst);


    } else if (tip->rfd == -1)		/* No error and no copy in progress */
    {
	/* For "copy-then-delete", do the delete part now */
	if ((type == DM_COPY_DEL) && ((err = DmDeleteFile(src)) != 0))
	{
	    /* Process error while deleting src.  Set "special" rfd values
	       for error processing above to indicate copy succeeded but
	       delete failed.
	    */
	    tip->rfd = DmSameOrDescendant(DM_WB_PATH(Desktop), dst, -1) ?
		-3 : -2;
	    goto process_err;
	}
	tip->cur_task = tip->cur_task->next;
	opr_info->task_cnt++;
    }

 done:   
    /* if done, call client_proc with DONE status. */
    if (tip->cur_task == NULL) {
		/* flush internal cache for opened containers */
		FlushCache();

		(*tip->client_proc)(DM_DONE, tip->client_data,
			    (XtPointer)tip, NULL, NULL);
	}

    /* schedule next task to execute or unregister itself */
    return(ScheduleNextTask());
}					/* end of ExecuteTask */

/****************************procedure*header*****************************
    Dm__TargetFileType-
*/
static int
Dm__TargetFileType(struct stat * st)
{
    return(((st->st_mode & S_IFMT) == S_IFDIR) ? 
	     (strcmp(st->st_fstype, DOSFS)?  DM_IS_DIR : DM_IS_DOSDIR) :
	     ((st->st_mode & S_IFMT) == S_IFREG) ? 
	     (strcmp(st->st_fstype, DOSFS) ? DM_IS_FILE : DM_IS_DOSFILE) : 
	     DM_IS_SPECIAL );

}				/* Dm__TargetFileType */	

/****************************procedure*header*****************************
    Dm__FileType- function checks the type of the file via stat() and returns
	one of internal file defines used in file manipulation routines.
*/
static int
Dm__FileType(char *filename, struct stat *st)
{
    struct stat local_stat;

    if (st == NULL)
	st = &local_stat;

    if (lstat(filename, st) != 0)
	return(DM_NO_FILE);

    if ((st->st_mode & S_IFMT) == S_IFLNK)
	return(DM_IS_SYMLINK);

    return(((st->st_mode & S_IFMT) == S_IFDIR) ? DM_IS_DIR  : 
	   ((st->st_mode & S_IFMT) == S_IFREG) ? DM_IS_FILE  : DM_IS_SPECIAL);


}

/****************************procedure*header*****************************
    FreeSrcList-
*/
static void
FreeSrcList(DmTaskInfoListPtr tip)
{
    char **	src_list = tip->opr_info->src_list;
    int		src_cnt = tip->opr_info->src_cnt;
    char **	src;

    if ((src_list == NULL) || (src_cnt == 0))
	return;

    for (src = src_list; src < src_list + src_cnt; src++)
	XtFree(*src);

    XtFree((char *)src_list);
}					/* end of FreeSrcList */

/****************************procedure*header*****************************
    FreeTasks- frees the tasklist when an error occurs, operation is done,
	user respond negatively to the overwrite etc.
*/
static void
FreeTasks(DmTaskListPtr current)
{
    while (current != NULL)
    {
	DmTaskListPtr save;

	FreeTaskContents(current);

	save = current;
	current = current->next;
	FREE((void *)save);
    }
}					/* end of FreeTasks */

/****************************procedure*header*****************************
    GetTaskInfoItem() is called from DmDoFileOp() to get new
	DmTaskInfoListPtr for use with file operation.
*/
static DmTaskInfoListPtr
GetTaskInfoItem(void)
{
    DmTaskInfoListPtr tip;

    tip = (DmTaskInfoListPtr)MALLOC(sizeof(DmTaskInfoListRec));

    /* if first operation */
    if (DESKTOP_CUR_TASK(Desktop) == NULL)
    {
	DESKTOP_CUR_TASK(Desktop) = tip;
	tip->prev = tip;

    } else
    {
	tip->prev = DESKTOP_CUR_TASK(Desktop)->prev;
	DESKTOP_CUR_TASK(Desktop)->prev->next = tip;
	DESKTOP_CUR_TASK(Desktop)->prev = tip;
    }

    tip->next = DESKTOP_CUR_TASK(Desktop);

    return(tip);
}				/* end of GetTaskInfoItem() */

/* MakeTask() function inserts each sub-task into the list of task */
static void
MakeTask(DmFileOpType type, char * src, char * dst, DmTaskListPtr ** q)
{
    DmTaskListPtr  p;

    /* set up task structure */
    p = (DmTaskListPtr) MALLOC(sizeof(DmTaskListRec));
    p->type	= type;
    p->source	= src;
    p->target	= dst;
    p->next	= NULL;

    /* and insert into the list */
    **q = p;
    *q = &p->next;
}				/* end of MakeTask */

/****************************procedure*header*****************************
    RegFileOpWorkProc(DmTaskInfoListPtr tip)-
*/
static void
RegFileOpWorkProc(DmTaskInfoListPtr tip)
{
    if (!Desktop->bg_not_regd)		/* ie., already registered */
	return;

    XtAddWorkProc(ExecuteTask, NULL);
    Desktop->bg_not_regd = False;
    DESKTOP_CUR_TASK(Desktop) = tip;
}

/****************************procedure*header*****************************
    ReturnError-
*/
static DmTaskInfoListPtr
ReturnError(DmTaskInfoListPtr tip, int error, char * name)
{
    tip->opr_info->error = error;
    (*tip->client_proc)(DM_ERROR, tip->client_data, (XtPointer)tip, name, NULL);
    DmFreeTaskInfo(tip);
    return(NULL);
}

/****************************procedure*header*****************************
    ScheduleNextTask - scheduler for the async file opr processing.
	"Round-robin" algorithm to find next file operation to perform.  Look
	for next task that is not processing an OVERWRITE notice.  Only
	consider CUR_TASK after all other opr's have been considered.  Return
	Boolean corresponding to use by Intrinsics: "True" means DON'T
	re-register background work proc.

	Note: CUR_TASK can be NULL as after caller frees task info when DONE.
*/
static Boolean
ScheduleNextTask(void)
{
    DmTaskInfoListPtr   tip;

    if ( (tip = DESKTOP_CUR_TASK(Desktop)) != NULL )
    {
	do
	{
	    tip = tip->next;
	    if ((tip->cur_task != NULL) && 
		!(tip->opr_info->attrs & (OVERWRITE | NMCHG)))
	    {
		DESKTOP_CUR_TASK(Desktop) = tip;
		return(False);
	    }

	} while(tip != DESKTOP_CUR_TASK(Desktop));
    }

    DESKTOP_CUR_TASK(Desktop) = NULL;
    Desktop->bg_not_regd = True;
    return(True);
}					/* end of ScheduleNextTask() */

/****************************procedure*header*****************************
	FILE OPERATION ROUTINES

    Low-level routines which actually perform the file operations.
*/

/* CopyBlock() - a lowest level function to copy a 'src' to 'dst'. Note 
   that the function copies only COPY_BYTES at a time since this is during
   work proc (ie, minimize affect on user feedback).  Keep track of
   completion by keeping fildes open or "reset" (to -1).
*/
static int
CopyBlock(char * src, char * dst, int * rfd, int * wfd, mode_t mode)
{
#define COPY_BYTES	4096

    char	buf[COPY_BYTES];
    int		cnt;

    if (*rfd == -1)
    {
	if (strcmp(src, dst) == 0)
	    return(ERR_CopyToSelf);

	if ((*rfd = open(src, O_RDONLY)) == -1)
	    return(ERR_OpenSrc);

	if ((*wfd = open(dst, O_WRONLY|O_CREAT | O_EXCL, mode)) == -1)
	{
	    close(*rfd);
	    *rfd = -1;
	    return(ERR_OpenDst);
	}

	/* copy src properties to dst */
	CopyObjProps(dst, src);

	/* open will apply umask to mode so must chmod to preserve mode */
	(void)fchmod(*wfd, mode);

	return(0);	/* First time thru, just open src & dst */
    }

    cnt = read(*rfd, buf, COPY_BYTES);

    if ((cnt > 0) && (write(*wfd, buf, cnt) == cnt))
	return(0);

    /* EOF reached during read or error occurred during read or write.
       Always close filedes but only "reset" them when EOF (ie, not error).
       Don't reset them when error so ExecuteTask knows to undo opr later.
    */
    close(*rfd);
    close(*wfd);

    if (cnt == 0)		/* ie, EOF */
    {
	*rfd = -1;
	*wfd = -1;
	return (0);
    }
				/* ie, a read or write error */
    (void)unlink(dst);

    return( (cnt == -1) ? ERR_Read : ERR_Write );

}					/* end of CopyBlock */

/* DmCopyFile()- low level function to copy 'src' to 'dst'.
	If src is a symbolic link, make a new copy of it.  Otherwise, copy
	the src to the dst a 'block' at a time and change mode of 'dst'
	based on 'src' mode.
*/
static int
DmCopyFile(char * src, char * dst, int * rfd, int * wfd)
{
    struct stat mystat;
    int retvalue;

    if (lstat(src, &mystat) != 0)
	return(ERR_NotAFile);

    if ((mystat.st_mode & S_IFLNK) == S_IFLNK)
	return( MakeLinkInstead(src, dst) );

    if (stat(src, &mystat) != 0)
	return(ERR_NotAFile);

    if (access(dst, F_OK) == 0)
	return(ERR_IsAFile);

    if ( (retvalue = CopyBlock(src, dst, rfd, wfd, mystat.st_mode)) != 0 )
	return(retvalue);

    return(0);
}				/* end of DmCopyFile */

/* DmDeleteFile() - low level function to delete a file specified in 'src' */
static int
DmDeleteFile(char * src)
{
    return((unlink(src) == 0) ? 0 : ERR_Rm);

}				/* end of DmDeleteFile */

/****************************procedure*header*****************************
   DmLinkFile - low level function to create a link from 'src' to 'dst'
*/
static int
DmLinkFile(char * src, char * dst)
{
    struct stat mystat;
	int ret;

    /* NOTE: is the next statement necessary? */
    if (stat(src, &mystat) != 0)
	return(ERR_NotAFile);

    if (access(dst, F_OK) == 0)
	return(ERR_IsAFile);

    errno = 0;
    ret = ((link(src, dst) == 0) ? 0 :
	   (errno == EXDEV) ? ERR_ForeignLink : ERR_Link);

	/* copy src properties to dst */
	if (!ret)
		CopyObjProps(dst, src);
	return(ret);
}					/* end of DmLinkFile */

/* DmMakeDirectory - low level function to create a new directory 
   specified in 'dst'. 'dst' inherits mode info. from the src.
*/
static int
DmMakeDirectory(char * src, char * dst)
{
    struct stat mystat;

    if (stat(src, &mystat) != 0)
	return(ERR_Stat);

    /* Don't create a directory to which the user does not have RWX access. */
    errno = 0;
    if ((mkdir(dst, mystat.st_mode | S_IRWXU) == -1) && (errno != EEXIST))
	return(ERR_Mkdir);
    if (convnmtonum(mystat.st_fstype) == DOSFS_ID(Desktop))
	mystat.st_mode |= (S_IXUSR | S_IXGRP | S_IXOTH);

    /* copy src properties to dst */
    CopyObjProps(dst, src);

    /* mkdir will apply umask to mode so must chmod to preserve mode */
    (void)chmod(dst, mystat.st_mode);
	
    return(0);
}					/* end of DmMakeDirectory */

/****************************procedure*header*****************************
    MakeLinkInstead- if src is a symbolic link, rather than copy or move
	it (across file system), make a link instead.
*/
static int
MakeLinkInstead(char * src, char * dst)
{
    char	lname[FILENAME_MAX + 1];
    int		len;
	int		ret;

    if ( (len = readlink(src, lname, FILENAME_MAX)) == -1 )
	return(ERR_ReadLink);

    lname[len] = 0;

    ret = (symlink(lname, dst) == 0) ? 0 : ERR_Link;

	/* copy src properties to dst */
	if (!ret)
		CopyObjProps(dst, src);
	return(ret);
}					/* end of MakeLinkInstead */

/* DmMoveFile - low level function to move/rename a 'src' to 'dst'.
   If move s across the file system, copy instead.
*/
static int
DmMoveFile(char * src, char * dst)
{
	int ret;

    if (access(dst, F_OK) == 0)
	return(ERR_IsAFile);

    ret = rename(src, dst) == 0 ? 0 : ERR_Rename;

	/* copy src properties to dst */
	if (!ret)
		CopyObjProps(dst, src);
	return(ret);
}					/* end of DmMoveFile */

/* DmRmDir() - low level function to delete a file specified in 'src' */
static int
DmRmDir(char * src)
{
	/* Just in case, remove .dtinfo file first.
	 * Don't care if the file really exist or not.
	 * This could happend because we no longer add a task for .dtinfo when doing
	 * COPY or COPY_DEL.
	 */
	(void)unlink(DmMakePath(src, DM_INFO_FILENAME));

    return((rmdir(src) == 0) ? 0 : ERR_Rm);

}					/* end of DmRmDir */

/* DmSymLinkFile - low level function to create a symbolic link 
   from 'src' to 'dst'.
*/
static int
DmSymLinkFile(char * src, char * dst)
{
    struct stat mystat;
	int ret;

    if (stat(src, &mystat) != 0)
	return(ERR_NotAFile);

    if (access(dst, F_OK) == 0)
	return(ERR_IsAFile);

    ret = (symlink(src, dst) == 0) ? 0 : ERR_Link;

	/* copy src properties to dst */
	if (!ret)
		CopyObjProps(dst, src);
	return(ret);
}					/* end of DmSymLinkFile */

/***************************private*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
    DmDoFileOp- process file opr request.
	- set up TaskInfoList structure that contains necessary info.
	  about the operation (e.g. sorc_list to operate, target, error
	  conditions etc. Used by many internal function that implements one 
	  part of the file manipulation as well as functions that are 
	  responsible for Window update, Undo, Kill etc.

	- builds a task list: each big operation (e.g. moving a set of files
	  will break down to move of each file separately. etc).

	- register a background proc. with Xt so that whenever no events are
	   pending to be processed, file manipulation is done via the 
	  registered proc. Note, the background workproc. is the way how
	  an asynchrouns processing of file operation is implemented in dtm.
*/
DmTaskInfoListPtr
DmDoFileOp(DmFileOpInfoPtr opr_info, DmClientProc client_proc, 
	   XtPointer client_data)
{
    DmTaskListPtr *	q;
    DmTaskInfoListPtr	tip;
    int			target_type;
    int			source_type;
    struct stat 	target_stat;
    struct stat 	source_stat;
    char 		**src;
    DmFileOpType	type = opr_info->type;
    unsigned short	target_op_vec;
    unsigned long	op_vec;
    char		local_buffer[FILENAME_MAX];
    char 		dosnm[DOS_MAXPATH];
    char 		*target_path;
    int			target_offset;
    int			same_fs;
    int			ret;
    int			src_idx;
    dev_t		dev;

    tip = GetTaskInfoItem();

    tip->task_list	= NULL;
    tip->rfd		= -1;
    tip->wfd		= -1;
    tip->client_proc	= client_proc;
    tip->client_data	= client_data;
    tip->opr_info	= opr_info;
    opr_info->attrs	= 0;
    opr_info->cur_src	= 0;
    opr_info->task_cnt	= 0;
    opr_info->error	= 0;
    opr_info->src_info	= (DtAttrs *)CALLOC(opr_info->src_cnt,sizeof(DtAttrs));


    /* q points to the current end of the linked list */
    q = &tip->task_list;
	
    target_path = opr_info->target_path;

    /* 
     * Get target type.  target_path is null when type is
     * DM_DELETE 
     */
    if (target_path == NULL) 
    	target_type = DM_NO_FILE;
    else {
	errno = 0;
	if (stat(target_path, &target_stat) != 0) {
		if (errno != ENOENT) 
			return(ReturnError(tip, ERR_TargetFS, target_path)); 
		else {
			char *dpathp = strdup(target_path);
			dpathp = dirname(dpathp);
			if (stat(dpathp, &target_stat) != 0) {
				FREE(dpathp);
				/* Parent does not exists */
				return(ReturnError(tip, ERR_TargetFS, 
							target_path)); 
			} else { 
				FREE(dpathp);
    				target_type = Dm__TargetFileType(&target_stat);
				switch(target_type) {
				case DM_IS_DIR:
					target_type = DM_PS_DIR;
					break;

				case DM_IS_DOSDIR:		
					target_type = DM_PS_DOSDIR;
					break;

				case DM_IS_S5DIR:
					target_type = DM_PS_S5DIR;
					break;
				default:
					/* Parent is not directory */
					return(ReturnError(tip, ERR_TargetFS, 
							target_path)); 
				}
				dev = target_stat.st_dev;
			}
				
		}
	} else { 
		dev = target_stat.st_dev;
    		target_type = Dm__TargetFileType(&target_stat);
	}

	switch(target_type) {
	case DM_IS_DIR:			/* Target type is Directory */
	case DM_IS_S5DIR:		/* Target type is Directory */
	    opr_info->attrs = DM_B_DIR_EXISTS;
	    break;

	case DM_IS_DOSDIR:		/* Target is dos directory */
	    if (type == DM_HARDLINK  || type == DM_SYMLINK) 
		return(ReturnError(tip, ERR_Linktodos, target_path)); 
	    else 
	    	opr_info->attrs = DM_B_DIR_EXISTS;
	    break;
	case DM_PS_DOSDIR:		/* Target is dos directory */
	case DM_IS_DOSFILE:		/* Target is dos file */
	    if (type == DM_HARDLINK  || type == DM_SYMLINK)  
		return(ReturnError(tip, ERR_Linktodos, target_path)); 
	    break;

	default:
	    break;
	}
    }
    /* Look up target table first. */
    target_op_vec = target_table[target_type];

    /* 
     * Use RENAME as a hint.  It is exactly like MOVE (and gets re-mapped to
     * MOVE) except that 'target_path' should be taken as is and not
     * adjusted.  This is the case for "Rename" and when putting back a dir
     * from WB, for instance.  Note that turning on OVER_WRITE here handles
     * the dir case; overwriting a file is handled in the table.
     */
    if (type == DM_RENAME) {
	if (target_op_vec & ADJUST_TARGET) {
		target_op_vec &= ~ADJUST_TARGET;
		target_op_vec |= OVER_WRITE;
		target_type = DM_IS_DIR;
	}

	/* Re-map RENAME to MOVE for src/op processing below */
	type = opr_info->type = DM_MOVE;
    }

    /* Target is dos file system */
    if (target_op_vec & CK_NM)
    {
	if (opr_info->options & UNDO)
	    target_op_vec &= ~CK_NM;
	else
	    MakeTask(DM_NMCHG, NULL, strdup(opr_info->target_path), &q);
    }

    if (target_op_vec & OVER_WRITE) {
	if (opr_info->options & DONT_OVERWRITE)
	    return( ReturnError(tip, ERR_IsAFile, target_path) );

	MakeTask(DM_OVERWRITE, NULL, strdup(opr_info->target_path), &q);

	if (target_type == DM_IS_DIR) {
	    ret = BuildSubtasksForDir(tip, DM_DELETE,
				      strdup(target_path), NULL, &q, 0);
	    if (ret != 0)
		return( ReturnError(tip, ret, target_path) );

	    MakeTask(DM_RMDIR, strdup(target_path), NULL, &q);

	} else
	    MakeTask(DM_DELETE, strdup(target_path), NULL, &q);
    }

    if (target_op_vec & MSRC_MKDIR)
	if (opr_info->src_cnt > 1) {
	    if (type != DM_DELETE) {
	    	MakeTask(DM_MKDIR, strdup(opr_info->src_path),
			 strdup(opr_info->target_path), &q);

	        opr_info->attrs = DM_B_DIR_NEEDED_4FILES;

	        /* Adjust the target when multi-src's */
	        target_op_vec |= ADJUST_TARGET;
	    }
	}

    if (target_op_vec & ADJUST_TARGET) {
	/* need to append basename of each src to the end of target later */
	strcpy(target_path = local_buffer, opr_info->target_path);
	target_offset = strlen(target_path);
	if (strcmp(target_path, "/")) {
		target_path[target_offset++] = '/';
		target_path[target_offset] = '\0'; /* is this needed ? */
	}

	/* Indicate "adjusted" to caller */
	opr_info->attrs |= DM_B_ADJUST_TARGET;

    }

    /* go thru' each source item and build a "task list" that will be
       sequentially operated on by the background work proc. to execute user
       specified operation thru' completion
    */
    for (src_idx = 0, src = opr_info->src_list;
	 src_idx < opr_info->src_cnt; src_idx++, src++) {

	char * fullsrcpath = DmMakePath(opr_info->src_path, *src);

	/* Can't mv, rm, etc these special directories (even as root) */
	if ((type == DM_MOVE) || (type == DM_DELETE)) {
	    if (strcmp(fullsrcpath, "/") == 0)
		return ( ReturnError(tip, ERR_MoveRoot, *src) );
	    if (strcmp(fullsrcpath, DESKTOP_DIR(Desktop)) == 0)
		return ( ReturnError(tip, ERR_MoveDesktop, *src) );
	    if (strcmp(fullsrcpath, DM_WB_PATH(Desktop)) == 0)
		return ( ReturnError(tip, ERR_MoveWastebasket, *src) );
	}

	/* DmMakePath used so make copy (freed below) */
	fullsrcpath = strdup(fullsrcpath);

	/* Mark the beginning of tasks associated with a source item */
	MakeTask(DM_BEGIN, (char *)(src_idx + 1), (char *)(src_idx + 1), &q);

	source_type = Dm__FileType(fullsrcpath, &source_stat);
	opr_info->src_info[src_idx] = source_type;
	op_vec = OPCODES(source_type, opr_info->type);
	/* 
	 * This check must be done before other bits are checked, because
	 * the other LSB bits may the error number.
	 */
	if (op_vec & ERROR_OP)
	    return( ReturnError(tip, op_vec & ~ERROR_OP, *src) );

	if (opr_info->type == DM_MOVE) {
	    struct stat 	srcp_stat;
	    char *spathp = strdup(fullsrcpath);

	    spathp = dirname(spathp);
	    if (stat(spathp, &srcp_stat) != 0) {
	    	FREE(spathp);
	    	/* Parent does not exists */
	    	return(ReturnError(tip, ERR_Stat, fullsrcpath)); 
	    }

	    if (access(spathp, W_OK) == 0 && access(fullsrcpath, R_OK) == 0) {
	        int uid;

		/* check for sticky bit */
		if ((srcp_stat.st_mode & S_ISVTX) &&
		    (source_stat.st_uid != (uid = getuid())) &&
		    (srcp_stat.st_uid != uid) &&
		    (access(fullsrcpath, W_OK) < 0))
	    		return (ReturnError(tip, ERR_MoveFailed, *src));

		if (source_type == DM_IS_DIR) {
		    /* check for mount point */
		    if (srcp_stat.st_dev != source_stat.st_dev)
		    	return (ReturnError(tip, ERR_ActiveMountPoint, *src));
		}
	    } else
	    	return (ReturnError(tip, ERR_MoveFailed, *src));
	}

	if (target_op_vec & ADJUST_TARGET)
	{
	    unsigned short tmp_op_vec;
	    char *last_comp;

	    last_comp = basename(fullsrcpath);

	    if (target_op_vec & CK_NM)
	    {
		switch (unix2dosfn(last_comp, dosnm,
				   legalfmove, strlen(last_comp)))
		{
		case -1:		/* File is a '.' file */
		    opr_info->src_info[src_idx] |= SRC_B_SKIP_OVERWRITE;
		    if (op_vec & MV_DIR)
			MakeTask(DM_DELETE, strdup(fullsrcpath), NULL, &q);
		    continue;
		case 1:			/* Name changed */
		    opr_info->src_info[src_idx] |= SRC_B_NMCHG;
		    last_comp = dosnm;
		    break;
		case 0:			/* Name is valid for dosfs */
		    break;
		}
	    }

	    /* add basename of source to target */
	    strcpy(target_path + target_offset, last_comp);

	    if (op_vec & (CHK_SAME | CHK_DESC)) {
		ret = DmSameOrDescendant(fullsrcpath, target_path, -1);

		if (ret < 0)		/* ie, same */
		    return(ReturnError(tip,
				       (type == DM_MOVE) ? ERR_MoveToSelf :
				       (type == DM_COPY) ? ERR_CopyToSelf :
				       ERR_LinkToSelf, *src) );

		else if ((ret > 0) && (op_vec & CHK_DESC))
		    return(ReturnError(tip,
				       (type == DM_MOVE) ? ERR_MoveToDesc :
				       ERR_CopyToDesc, *src) );
	    }

	    /* get the type of new target */
	    target_type = Dm__FileType(target_path, &target_stat);

	    /* avoid overwriting ancestor directory */
	    if (target_type & (DM_IS_DIR | DM_IS_S5DIR | DM_IS_DOSDIR )) {

		/* fullsrcpath and target_path are the real path already */
		if (DmSameOrDescendant(target_path, fullsrcpath,
					strlen(target_path)) > 0)
		    return (ReturnError(tip, ERR_OverwriteParentDir, *src));
	    }

	    tmp_op_vec = target_table[target_type];

	    if (tmp_op_vec & OVER_WRITE) {
		MakeTask(DM_OVERWRITE, strdup(fullsrcpath),
			 strdup(target_path), &q);
		MakeTask(DM_DELETE, strdup(target_path), NULL, &q);
	    }

	    /* Special case if both source and target are dirs. */
	    if (target_type == DM_IS_DIR) {
		MakeTask(DM_OVERWRITE, strdup(fullsrcpath), 
			 strdup(target_path), &q);
		ret = BuildSubtasksForDir(tip, DM_DELETE,
					  strdup(target_path), NULL, &q, 0);
		if (ret != 0)
		    return( ReturnError(tip, ret, *src) );
		MakeTask(DM_RMDIR, strdup(target_path), NULL, &q);
	    }
	} else if (op_vec & (CHK_SAME | CHK_DESC)) {
	    ret = DmSameOrDescendant(fullsrcpath, target_path, -1);

	    if (ret < 0)		/* ie, same */
		return(ReturnError(tip,
				   (type == DM_MOVE) ? ERR_MoveToSelf :
				   (type == DM_COPY) ? ERR_CopyToSelf :
				   (type == DM_CONV_D2U) ? ERR_CopyToSelf :
				   (type == DM_CONV_U2D) ? ERR_CopyToSelf :
				   ERR_LinkToSelf, *src) );

	    else if ((ret > 0) && (op_vec & CHK_DESC))
		return(ReturnError(tip, (type == DM_MOVE) ? ERR_MoveToDesc :
				   ERR_CopyToDesc, *src) );
	}


	if (op_vec & CHK_FS)
	    same_fs = (dev == source_stat.st_dev);
	 else
	    same_fs = 1;

	if (op_vec & MV_DIR) {
	    op_vec = same_fs ? MV_OP : MKDIR_OP | RECUR_CP_DEL | RMDIR_OP;

	} else if (op_vec & MV_OP) {
	    /* Special check for move operation.  Moving across file system
	       translates to copy and delete.  But should preserve the CHK_CP
	       bit in case it is a special file.
	    */
	    if (!same_fs)
		op_vec = (op_vec & CHK_CP) | (CP_DEL_OP);
	}

	if (op_vec & CHK_CP)
	    if (op_vec & (CP_OP | CP_DEL_OP))
		return( ReturnError(tip, ERR_CopySpecial, *src) );

	if (op_vec & MKDIR_OP)
	    MakeTask(DM_MKDIR, strdup(fullsrcpath), strdup(target_path), &q);

	/* Passing 'type' here is okay since link op's are not re-mapped */
	if (op_vec & LN_OP) {
	    MakeTask(type, strdup(fullsrcpath), strdup(target_path), &q);

	} else if (op_vec & (CP_OP | CP_DEL_OP | MV_OP | CONVD2U_OP | CONVU2D_OP)) {
	    MakeTask((op_vec & CP_OP) ? DM_COPY :
		     (op_vec & CP_DEL_OP) ? DM_COPY_DEL :
		     (op_vec & CONVD2U_OP)? DM_CONV_D2U :
		     (op_vec & CONVU2D_OP)? DM_CONV_U2D : DM_MOVE,
		     strdup(fullsrcpath), strdup(target_path), &q);


	} else if (op_vec & DEL_OP) {
	    MakeTask(DM_DELETE, strdup(fullsrcpath), NULL, &q);

	} else if (op_vec & (RECUR_CP | RECUR_CP_DEL)) {
	    /* descend into subdirectories */
	    ret = BuildSubtasksForDir(tip, (op_vec & RECUR_CP) ?
				      DM_COPY : DM_COPY_DEL,
				      strdup(fullsrcpath), strdup(target_path),
				      &q, target_op_vec & CK_NM);
	    if (ret != 0)
		return( ReturnError(tip, ret, *src) );

	} else if (op_vec & RECUR_DEL) {
	    /* descend into subdirectories */
	    ret = BuildSubtasksForDir(tip, DM_DELETE,
				      strdup(fullsrcpath), NULL, &q, 0);

	    if (ret != 0)
		return( ReturnError(tip, ret, *src) );
	}

	if (op_vec & RMDIR_OP)
	    MakeTask(DM_RMDIR, strdup(fullsrcpath),
		     (target_path == NULL) ? NULL : strdup(target_path), &q);

	/* Mark the end of tasks associated with a source item */
	MakeTask(DM_ENDOVER, (char *)(src_idx + 1), (char *)(src_idx + 1), &q);

	FREE(fullsrcpath);
    }					/* for each source */

    if (target_op_vec & OVER_WRITE || target_op_vec & CK_NM)
	MakeTask(DM_ENDOVER, (char *)0, (char *)0, &q);

    tip->cur_task = tip->task_list;

    BeginFileOp(tip);
    return(tip);
}				/* end of DmDoFileOp */

/* DmFreeTaskInfo() - Remove task from list of file opr's and free all data
*/
void
DmFreeTaskInfo(DmTaskInfoListPtr tip)
{
    DmFileOpInfoPtr	opr_info;

    if (tip == NULL)
	return;

    /* Remove from list */
    if (tip->next == tip)			/* only task */
    {
	tip->next = NULL;			/* for CUR_TASK check below */

    } else
    {
	tip->prev->next	= tip->next;		/* adjust prev task */
	tip->next->prev	= tip->prev;		/* adjust next task */
    }

    if (DESKTOP_CUR_TASK(Desktop) == tip)
	DESKTOP_CUR_TASK(Desktop) = tip->next;	/* may be NULL */

    /* Free contents */
    opr_info = tip->opr_info;
    FreeTasks(tip->task_list);
    XtFree(opr_info->target_path);
    XtFree(opr_info->src_path);
    XtFree((char *)opr_info->src_info);
    FreeSrcList(tip);
    FREE((void *)opr_info);
    FREE((void *)tip);
}					/* end of DmFreeTaskInfo() */

void 
Dm__NameChange(DmTaskInfoListPtr tip, Boolean contin)
{
    tip->opr_info->attrs &= ~NMCHG;	/* no longer in "NAME CHANGE" state */

    if (contin)
	tip->cur_task = tip->cur_task->next;
    else
    {
	FreeTasks(tip->task_list);
	tip->cur_task = tip->task_list = NULL;
    }
    RegFileOpWorkProc(tip);
}					

/****************************procedure*header*****************************
    Dm__Overwrite- clean-up tasks due to overwriting target (or not).

	If overwriting ('overwrite' == True), just (re)register work proc so
	that overwrite tasks will be executed.

	If not, mark src item ('src_info') as "skipped" so it will not be
	processed during UpdateWindow and remove sub-tasks for this item.
*/
void 
Dm__Overwrite(DmTaskInfoListPtr tip, Boolean overwrite)
{
    if (!overwrite)
    {
	DmFileOpInfoPtr	opr_info = tip->opr_info;
	DmTaskListPtr	task;
	int		match;

	/* NOTE: when cur_src is 0, don't do any of this (?).
	 * Free all tasks and set cur_task to NULL.
	 */
   
	/* Mark source item as skipped so that UNDO will work. */
	opr_info->src_info[(opr_info->cur_src == 0) ? 0 :
			   opr_info->cur_src - 1] |= SRC_B_SKIP_OVERWRITE;

	/* Remove any remaining tasks for this operation.  This is just like
	 * FreeTasks except that only want to free up to the ENDOVER task.
	 */
	match = opr_info->cur_src;
	task = tip->cur_task->next;
	while ( !((task->type == DM_ENDOVER) &&
		  (match == (int)(task->source)) &&
		  (match == (int)(task->target))))
						     
	{
	    DmTaskListPtr save;

	    FreeTaskContents(task);

	    save = task;
	    task = task->next;
	    FREE((void *)save);
	}

	/* Tasks up to ENDOVER task have been removed from task list so have
	 * cur_task point to next task (actually this is the ENDOVER task but
	 * this is needed to match the OVERWRITE task).  Add one to task_cnt
	 * for ENDOVER.
	 */
	tip->cur_task->next = task;
	opr_info->task_cnt++;
    }

    tip->opr_info->attrs &= ~OVERWRITE;	/* no longer in "OVERWRITE" state */
    tip->cur_task = tip->cur_task->next;
    RegFileOpWorkProc(tip);
}					/* end of Dm__Overwrite */

/****************************procedure*header*****************************
    DmStopFileOp- "stop" file op.  That is, clean up file opr tasks after
	user has stopped the opr or an error occurred. 
*/
void
DmStopFileOp(DmTaskInfoListPtr tip)
{
    DmFileOpInfoPtr	opr_info = tip->opr_info;

    if (tip->cur_task == NULL)
	return;

    /* close any open file descriptors */
    if (tip->rfd >= 0)
	(void)close(tip->rfd);
    if (tip->wfd >= 0)
	(void)close(tip->wfd);

    if ((tip->cur_task->type == DM_COPY) && (tip->rfd != -1))
    {
	/* The copy operation that failed should be undone later */
	tip->cur_task = tip->cur_task->next;
	tip->rfd = -1;
	opr_info->task_cnt++;
    }

    if (opr_info->task_cnt == 0)
    {
	/* Nothing to undo. Failed on the first subtask. */
	FreeTasks(tip->task_list);
	tip->task_list = NULL;

    } else
    {
	/* Free all subtasks AFTER this subtask */
	FreeTasks(tip->cur_task->next);

	FreeTaskContents(tip->cur_task);

	/* Change the last subtask to a ENDOVER type (for UNDO) */
	tip->cur_task->type		= DM_ENDOVER;
	tip->cur_task->source		=
	    tip->cur_task->target	= (char *)(opr_info->cur_src);
	tip->cur_task->next		= NULL;

	/* add one for the ENDOVER subtask */
	opr_info->task_cnt++;
    }
    tip->cur_task = NULL;
    opr_info->src_info[(opr_info->cur_src == 0) ? 0 :
			opr_info->cur_src - 1] |= SRC_B_ERROR;

    /* If we get here from a notice being posted, the FileOp WorkProc is no
     * longer registered.  Be sure it's registered so that file op completion
     * (DM_DONE) is announced.
     */
    RegFileOpWorkProc(tip);
}					/* end of DmStopFileOp */


static const DmFileOpType undo_table[] = {
    /* Original op		Undo op */
    /* DM_COPY		*/	DM_DELETE,
    /* DM_MOVE		*/	DM_MOVE,
    /* DM_DELETE	*/	DM_NO_OP,
    /* DM_HARDLINK	*/	DM_DELETE,
    /* DM_SYMLINK	*/	DM_DELETE,
    /* DM_COPY_DEL	*/	DM_COPY_DEL,
    /* DM_CONV_D2U	*/	DM_DELETE,
    /* DM_CONV_U2D	*/	DM_DELETE,
    /* DM_BEGIN		*/	DM_ENDOVER,
    /* DM_MKDIR		*/	DM_RMDIR,
    /* DM_RMDIR		*/	DM_MKDIR,	/* but see code */
    /* DM_OVERWRITE	*/	DM_NO_OP,
    /* DM_NMCHG		*/      DM_NO_OP,
    /* DM_ENDOVER	*/	DM_BEGIN,
    /* DM_RENAME	*/	/* not used */
};


/****************************procedure*header*****************************
    DmUndoFileOp-
*/
int
DmUndoFileOp(DmTaskInfoListPtr tip)
{
    DmFileOpInfoPtr	opr_info = tip->opr_info;
    DmTaskListPtr *	task_ptrs;
    DmTaskListPtr *	tasks;
    DmTaskListPtr	orig_task;
    DmTaskListPtr	current;
    int			cur_src;

    /* build a vector of task list ptrs so list can be traversed in reverse */
    task_ptrs = (DmTaskListPtr *)
	MALLOC(opr_info->task_cnt * sizeof(DmTaskListPtr));

    if (task_ptrs == NULL)
	return(-1);

    orig_task = tip->task_list;
    for (tasks = task_ptrs; tasks < task_ptrs + opr_info->task_cnt; tasks++)
    {
	*tasks = orig_task;
	orig_task = orig_task->next;
    }

    /* Process the task list in reverse */

    tasks = task_ptrs + opr_info->task_cnt - 1;
    tip->task_list = tip->cur_task = *tasks;	/* tail becomes head */
    cur_src = opr_info->cur_src;

    for ( ; tasks >= task_ptrs; tasks--)
    {
	DmTaskListPtr task = *tasks;

	/* Map opr type to "undo" type */
	task->type = undo_table[task->type - 1];

	/* Special case for undo'ing RMDIR: don't always have original target
	   (following a recursive delete, for instance).  Therefore, this
	   cannot be undone.
	*/
	if ((task->type == DM_MKDIR) && (task->target == NULL))
	    task->type = DM_NO_OP;

	/* Clean up list by removing tasks that can't be undone */
	if (task->type == DM_NO_OP)
	{
	    /* Fix link from "previous" task.
	       Note: we know this is not the 1st but it may be the last task.
	    */
	    FreeTaskContents(task);
	    FREE((void *)task);

	} else
	{
	    if ((task->type == DM_BEGIN) || (task->type == DM_ENDOVER))
	    {
		/* Invert the src-item counter: n-->1, n-1-->2, etc. */
		task->source = task->target =
		    (char *)(cur_src - (int)task->source + 1);
	    
	    } else
	    {				/* switch source and target */
		char * tmp;
		tmp		= task->source;
		task->source= task->target;
		task->target= tmp;
	    }
	    current = task;
	}
	
	/* 
	 * Assumption here is that last task is DM_ENDOVER and therefore
	 * current is always set.
	 */
	current->next = (tasks > task_ptrs) ? tasks[-1] : NULL;
    }
    FREE((void *)task_ptrs);

    opr_info->type	= undo_table[opr_info->type - 1];
    opr_info->task_cnt	= 0;			/* re-init num_subtasks */
    opr_info->cur_src	= 0;			/* re-init current src item */
    opr_info->error	= 0;

    /* NOTE: clear 'src_info' array?? */

    BeginFileOp(tip);
    return(0);
}					/* end of DmUndoFileOp */

/* 
 * DmConvertFile()- Initilalize conversion of file from Unix to Dos 
 * format ( dir == U2D ) or from Dos to Unix formtat ( dir == D2U ).
 */
static int
DmConvertFile(char * src, char * dst, int * rfd, int * wfd, int dir)
{
    struct stat mystat;
    int ret_val;


    if (stat(src, &mystat) != 0)
	return(ERR_NotAFile);

    if (access(dst, F_OK) == 0)
	return(ERR_IsAFile);

    ret_val = ConvertBlock(src, dst, rfd, wfd, mystat.st_mode, dir);

    return(ret_val);
}				/* end of DmConvertFile */

/* 
 * ConvertBlock() -  Convert one 'block' of src file to dst file  as indicated 
 * by 'dir' (U2D == Unix to Dos format, D2U == Dos to Unix format).
 * The Dos format can make a file as much as 2n + 1 larger than the Unix format.
*/
static int
ConvertBlock(char * src, char * dst, int * rfd, int * wfd, mode_t mode, int dir)
{
#define COPY_BYTES	4096

    char	r_buf[COPY_BYTES];
    char	w_buf[COPY_BYTES];
    int		cnt,  cnt2 = 0;

    if (*rfd == -1) {
	if (strcmp(src, dst) == 0)
	    return(ERR_CopyToSelf);

	if ((*rfd = open(src, O_RDONLY)) == -1)
	    return(ERR_OpenSrc);

	if ((*wfd = open(dst, O_WRONLY|O_CREAT | O_EXCL, mode)) == -1) {
	    close(*rfd);
	    *rfd = -1;
	    return(ERR_OpenDst);
	}

	/* open will apply umask to mode so must chmod to preserve mode */
	(void)fchmod(*wfd, mode);

	return(0);	/* First time thru, just open src & dst */
    }

    if (dir == D2U) { 	/* Dos to Unix */ 
    	cnt = read(*rfd, r_buf, COPY_BYTES);
	if (cnt > 0)
		cnt2 = ConvertDosToUnix(r_buf, w_buf, cnt);
    } else {  		/* Unix to Dos */
    	cnt = read(*rfd, r_buf, COPY_BYTES/2 -1);
	if (cnt > 0)
		cnt2 = ConvertUnixToDos(r_buf, w_buf, cnt);
	else if (cnt == 0) {
	/* 
	 * EOF reached  -- write DOSEOF on output file 
	 * when converting from Unix format to Dos format.
	 */
		w_buf[0] = DOSEOF;
		if ( write(*wfd, w_buf, 1) != 1 ) {
    			close(*rfd);
    			close(*wfd);
    			(void)unlink(dst);
			return (ERR_Write);
		}
	}
    }

    if ((cnt2 > 0) && (write(*wfd, w_buf, cnt2) == cnt2))
	return(0);

    /* 
     * EOF reached during read or error occurred during read or write.
     * Always close filedes but only "reset" them when EOF (ie, not error).
     * Don't reset them when error so ExecuteTask knows to undo opr later.
     */
    close(*rfd);
    close(*wfd);

    if (cnt == 0) {		/* ie, EOF */
	*rfd = -1;
	*wfd = -1;
	return (0);
    }
				/* ie, a read or write error */
    (void)unlink(dst);

    return( (cnt == -1) ? ERR_Read : ERR_Write );

}					/* end of ConvertBlock */

char *doschar = "abcdefghijklmnopqurstuvwxyz0123456789_^$~!#%&-{}'`()@";

int
legalfmove(unsigned char *c)
{
	if (strchr(doschar, (int)*c) == NULL) {
		*c = isupper((int)*c) ?  tolower((int)*c) : '$';
		return 1;
	}
	return 0; 
}

int
legalfcreate(unsigned char *c)
{
	if (strchr(doschar, (int)*c) == NULL) {
		if (isupper((int)*c)) 
			*c = tolower((int)*c);
		else {
			*c = '$';
			return 1;
		}
	}
	return 0; 
}


/*
 *  Convert a unix filename to a DOS filename.
 *  This function does not ensure that valid
 *  characters for a dos filename are supplied.
 */
int
unix2dosfn(unsigned char *un, unsigned char *dn, int (*legal_proc)(unsigned char *), int unlen)
{
	int i, j;
	int ret = 0;
	unsigned char c;

	/*
	 * We do not want to copy . files.
	 */
	if (*un == '.')
		return -1;
	if (unlen == 0)
		return 0;

	/*
	 *  Copy the unix filename into the dos filename string
	 *  upto the end of string, a '.', or 8 characters.
	 *  Whichever happens first stops us.
	 *  This forms the name portion of the dos filename.
	 *  Fold to upper case.
	 */
	for (i = 0; i <= 7  &&  unlen  &&  (c = *un)  &&  c != '.'; i++) {
		if (legal_proc(&c) && !ret)	
			ret = 1;
		dn[i] = c; 
		un++;
		unlen--;
	}

	/*
	 *  Strip any further characters up to a '.' or the
	 *  end of the string.
	 */
	while (unlen  &&  (c = *un)  &&  c != '.') {
		un++;
		unlen--;
		ret = 1;
	}

	/*
	 *  If we stopped on a '.', then get past it.
	 */
	if (c == '.') {
		dn[i++] = c; 
		un++;
		unlen--;
	}

	/*
	 *  Copy in the extension part of the name, if any.
	 *  Force to upper case.
	 *  Note that the extension is allowed to contain '.'s.
	 *  Filenames in this form are probably inaccessable
	 *  under dos.
	 */
	j = i;
	for (;i <= j+2  &&  unlen  &&  (c = *un) && c != '.'; i++) {
		if (legal_proc(&c) && !ret)	
			ret = 1;
		dn[i] = c;
		un++;
		unlen--;
	}
	dn[i] = NULL;
	if (unlen)
		ret = 1;
	return ret;
}
