#ifndef __DtStubI_h__
#define __DtStubI_h__

#pragma ident	"@(#)libMDtI:DtStubI.h	1.2"

	/* We don't need to worry about ARCHIVE because we only provide .so.
	 *
	 * The code below are common to both libDtI and libMDtI. The only
	 * difference is libMDtI/Imakefile will have -DDONT_USE_DT_FUNCS
	 *           and libDtI/Imakefile  will have -UDONT_USE_DT_FUNCS.
	 *
	 * Use Dts (Dt stub) as prefix!
	 */
#if defined(DONT_USE_DT_FUNCS) && (defined(SVR4) || defined(sun)) /*dlopen etc*/

#include <dlfcn.h>

#define DtsInitialize		(*dts_func_table.init)
#define DtsPutData		(*dts_func_table.put_data)
#define DtsFreePropertyList	(*dts_func_table.free_prop_list)
#define Dts__DecodeFromString	(*dts_func_table.decode_from_string)
#define Dts__strndup		(*dts_func_table.strndup)
#define DtsSetProperty		(*dts_func_table.set_prop)
#define DtsDequeueMsg		(*dts_func_table.dequeue_msg)
#define DtsGetProperty		(*dts_func_table.get_prop)
#define DtsDelData		(*dts_func_table.del_data)
#define DtsFindProperty		(*dts_func_table.find_prop)
#define DtsGetData		(*dts_func_table.get_data)
#define DtsSendReply		(*dts_func_table.send_reply)
#define DtsXReadPixmapFile	(*dts_func_table.read_pixmap_file)
#define DtsRealPath		(*dts_func_table.real_path)
#define DtsBaseName		(*dts_func_table.base_name)
#define DtsDirName		(*dts_func_table.dir_name)

typedef void	  (*DtsInitialize_type)(Widget);
typedef int	  (*DtsPutData_type)(Screen *, long, void *, int, void *);
typedef void	  (*DtsFreePropertyList_type)(DtPropListPtr);
typedef int	  (*Dts__DecodeFromString_type)(char *, DtMsgInfo const *,
							char *, char **);
typedef char *	  (*Dts__strndup_type)(char *, int);
typedef char *	  (*DtsSetProperty_type)(DtPropListPtr, char *, char *,
							DtAttrs);
typedef char *	  (*DtsDequeueMsg_type)(Screen *, Atom, Window);
typedef char *	  (*DtsGetProperty_type)(DtPropListPtr, char *, DtAttrs *);
typedef int	  (*DtsDelData_type)(Screen *, long, void *, int);
typedef DtPropPtr (*DtsFindProperty_type)(DtPropListPtr, DtAttrs);
typedef void *	  (*DtsGetData_type)(Screen *, long, void *, int);
typedef int	  (*DtsSendReply_type)(Screen*, Atom, Window, DtReply *);
typedef int	  (*DtsXReadPixmapFile_type)(Display *, Drawable, Colormap,
							char *, unsigned int *,
							unsigned int *,
							unsigned int, Pixmap *);
typedef char *	  (*DtsRealPath_type)(const char *, char *);
typedef char *    (*DtsBaseName_type)(char *);
typedef char *    (*DtsDirName_type)(char *);

typedef struct {
	DtsInitialize_type		init;
	DtsPutData_type			put_data;
	DtsFreePropertyList_type	free_prop_list;
	Dts__DecodeFromString_type	decode_from_string;
	Dts__strndup_type		strndup;
	DtsSetProperty_type		set_prop;
	DtsDequeueMsg_type		dequeue_msg;
	DtsGetProperty_type		get_prop;
	DtsDelData_type			del_data;
	DtsFindProperty_type		find_prop;
	DtsGetData_type			get_data;
	DtsSendReply_type		send_reply;
	DtsXReadPixmapFile_type		read_pixmap_file;
	DtsRealPath_type		real_path;
	DtsBaseName_type		base_name;
	DtsDirName_type			dir_name;
} DtsFuncRec;

extern DtsFuncRec			dts_func_table;

#else /* defined(DONT_USE_DT_FUNCS) && (defined(SVR4) || defined(sun)) */

#undef DONT_USE_DT_FUNCS		/* so we can just use this symbol
					 * in the C file */

#define DtsPutData		DtPutData
#define DtsFreePropertyList	DtFreePropertyList
#define Dts__DecodeFromString	Dt__DecodeFromString
#define Dts__strndup		Dt__strndup
#define DtsSetProperty		DtSetProperty
#define DtsDequeueMsg		DtDequeueMsg
#define DtsGetProperty		DtGetProperty
#define DtsDelData		DtDelData
#define DtsFindProperty		DtFindProperty
#define DtsGetData		DtGetData
#define DtsSendReply		DtSendReply
#define DtsXReadPixmapFile	XReadPixmapFile
#define DtsRealPath		realpath
#define DtsBaseName		basename
#define DtsDirName		dirname

#endif /* defined(DONT_USE_DT_FUNCS) && (defined(SVR4) || defined(sun)) */

#endif /* __DtStubI_h__ */
