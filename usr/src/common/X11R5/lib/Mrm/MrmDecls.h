#pragma ident	"@(#)m1.2libs:Mrm/MrmDecls.h	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */

/*
*  (c) Copyright 1989, 1990, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
#ifndef MrmDecls_H
#define MrmDecls_H

/*----------------------------------*/
/* URM external routines (Motif)    */
/*----------------------------------*/
#ifndef _ARGUMENTS
#ifdef _NO_PROTO
#define _ARGUMENTS(arglist) ()
#else
#define _ARGUMENTS(arglist) arglist
#endif
#endif

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* mrminit.c */
extern void MrmInitialize  _ARGUMENTS(( void ));

/* mrmlread.c */
extern Cardinal MrmFetchLiteral  _ARGUMENTS(( MrmHierarchy hierarchy_id , String index , Display *display , XtPointer *value_return , MrmCode *type_return ));
extern Cardinal MrmFetchIconLiteral  _ARGUMENTS(( MrmHierarchy hierarchy_id , String index , Screen *screen , Display *display , Pixel fgpix , Pixel bgpix , Pixmap *pixmap_return ));
extern Cardinal MrmFetchBitmapLiteral  _ARGUMENTS(( MrmHierarchy hierarchy_id , String index , Screen *screen , Display *display , Pixmap *pixmap_return , Dimension *width , Dimension *height));
extern Cardinal MrmFetchColorLiteral  _ARGUMENTS(( MrmHierarchy hierarchy_id , String index , Display *display , Colormap cmap , Pixel *pixel_return ));

/* Deal with Wide stuff now because there is an error in Saber 3.0 */

#if NeedWidePrototypes

extern Cardinal MrmOpenHierarchy  _ARGUMENTS(( int num_files , String *name_list , MrmOsOpenParamPtr *os_ext_list , MrmHierarchy *hierarchy_id_return ));
extern Cardinal MrmOpenHierarchyPerDisplay  _ARGUMENTS(( Display *display , int num_files , String *name_list , MrmOsOpenParamPtr *os_ext_list , MrmHierarchy *hierarchy_id_return ));
extern Cardinal MrmRegisterNames  _ARGUMENTS(( MrmRegisterArglist reglist ,int num_reg ));
extern Cardinal MrmRegisterNamesInHierarchy  _ARGUMENTS(( MrmHierarchy hierarchy_id , MrmRegisterArglist reglist , int num_reg ));
extern Cardinal MrmRegisterClass  _ARGUMENTS(( int class_code , String class_name , String create_name , Widget (*creator )(), WidgetClass class_record ));

#else

extern Cardinal MrmOpenHierarchy  _ARGUMENTS(( MrmCount num_files , String *name_list , MrmOsOpenParamPtr *os_ext_list , MrmHierarchy *hierarchy_id_return ));
extern Cardinal MrmOpenHierarchyPerDisplay  _ARGUMENTS(( Display *display , MrmCount num_files , String *name_list , MrmOsOpenParamPtr *os_ext_list , MrmHierarchy *hierarchy_id_return ));
extern Cardinal MrmRegisterNames  _ARGUMENTS(( MrmRegisterArglist reglist ,MrmCount num_reg ));
extern Cardinal MrmRegisterNamesInHierarchy  _ARGUMENTS(( MrmHierarchy hierarchy_id , MrmRegisterArglist reglist , MrmCount num_reg ));
extern Cardinal MrmRegisterClass  _ARGUMENTS(( MrmType class_code , String class_name , String create_name , Widget (*creator )(), WidgetClass class_record ));

#endif 

extern Cardinal MrmCloseHierarchy  _ARGUMENTS(( MrmHierarchy hierarchy_id ));
extern Cardinal MrmFetchInterfaceModule  _ARGUMENTS(( MrmHierarchy hierarchy_id , char *module_name , Widget parent , Widget *w_return ));
extern Cardinal MrmFetchWidget  _ARGUMENTS(( MrmHierarchy hierarchy_id , String index , Widget parent , Widget *w_return , MrmType *class_return ));
extern Cardinal MrmFetchWidgetOverride  _ARGUMENTS(( MrmHierarchy hierarchy_id , String index , Widget parent , String ov_name , ArgList ov_args , Cardinal ov_num_args , Widget *w_return , MrmType *class_return ));
extern Cardinal MrmFetchSetValues  _ARGUMENTS(( MrmHierarchy hierarchy_id , Widget w , ArgList args , Cardinal num_args ));

/* mrmwci.c */

/* extern Cardinal XmRegisterMrmCallbacks () ; */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#undef _ARGUMENTS

#endif /* MrmDecls_H */
/* DON'T ADD STUFF AFTER THIS #endif */
