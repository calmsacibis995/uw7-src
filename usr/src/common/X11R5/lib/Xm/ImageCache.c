#pragma ident	"@(#)m1.2libs:Xm/ImageCache.c	1.4"
/************************************************************************* 
 **  (c) Copyright 1993, 1994 Hewlett-Packard Company
 **  (c) Copyright 1993, 1994 International Business Machines Corp.
 **  (c) Copyright 1993, 1994 Sun Microsystems, Inc.
 **  (c) Copyright 1993, 1994 Novell, Inc.
 *************************************************************************/
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$"
#endif
#endif

#include <X11/Intrinsic.h>
#include <Xm/ColorObj.h>
#include <Xm/XmP.h>
#include "BitmapsI.h"
#include "_DtHashP.h"
#include "_xpmP.h"
#include "ImageCachI.h"
#include "IconFile.h"

typedef struct _DtPixmapCacheEntryRec *DtPixmapCacheEntry;
typedef struct _DtImageAttributePtrsRec *DtImageAttributePtrs;

/* Structures used to cache the GC's on a per-screen basis */
typedef struct {
   GC gc;
   int depth;
   XGCValues gcValues;
} PerGCInfo;

typedef struct {
   Screen * screen;
   PerGCInfo * gcs;
   int numGCs;
} PerScreenInfo;

typedef struct {
   Display * display;
   PerScreenInfo * screens;
   int numScreens;
} PerDisplayInfo;

static PerDisplayInfo * perDisplayInfo = NULL;
static int numDisplayEntries = 0;

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static DtHashKey GetEmbeddedKey() ;
static DtHashKey GetIndirectKey() ;
static DtHashKey GetBuiltInKey() ;
static DtHashKey GetMaskKey() ;
static void ReleaseMaskKey();
static void InitializeImageCache() ;
static DtHashEntry CreateInternalImageEntry() ;
static DtHashEntry CreateXternalImageEntry() ;
static DtHashEntry CreateMaskImageEntry() ;
static DtHashEntry CreateMissedImageEntry() ;
static DtHashEntry CreateDummyImageEntry() ;
static DtHashEntry CreateImageCacheEntry() ;
static void DestroyImageCacheEntry();
static void NullDestroyProc();
static void DestroyInternalImageCacheEntry();
static DtPixmapCacheEntry * GetPixmapListPtr() ;
static Boolean PutImageToPixmap() ;
static Boolean GetInternalPixmap() ;
static Boolean GetMaskPixmap() ;
static Boolean GetBuiltInPixmap() ;
static Boolean GetXternalPixmap() ;
static Boolean GetImageEntry() ;
static Boolean IsBitonal() ;
static DtHashEntry GetImageFromFile() ;
static Boolean GetPixmapEntry() ;
static GC GetGC();
static void CleanupOnDisplayClose();
static DtHashEntry __DtGetImage();
#else

static DtHashKey GetEmbeddedKey( 
                        DtHashEntry entry,
                        XtPointer clientData) ;
static DtHashKey GetIndirectKey( 
                        DtHashEntry entry,
                        XtPointer clientData) ;
static DtHashKey GetBuiltInKey( 
                        DtHashEntry entry,
                        XtPointer clientData) ;
static DtHashKey GetMaskKey( 
                        DtHashEntry entry,
                        XtPointer clientData) ;
static void ReleaseMaskKey( 
			   DtHashEntry entry,
			   DtHashKey	key) ;
static void InitializeImageCache( void ) ;
static DtHashEntry CreateInternalImageEntry( 
                        char *imageName,
                        XImage *image,
                        int hot_x,
                        int hot_y) ;
static DtHashEntry CreateXternalImageEntry( 
                        char *imageName,
                        XImage *image,
                        int hot_x,
                        int hot_y) ;
static DtHashEntry CreateMaskImageEntry( 
                        char *imageName,
                        XImage *image,
                        int hot_x,
                        int hot_y) ;
static DtHashEntry CreateMissedImageEntry( 
                        char *imageName,
                        XImage *image,
                        int hot_x,
                        int hot_y) ;
static DtHashEntry CreateDummyImageEntry( 
                        char *imageName,
                        XImage *image,
                        int hot_x,
                        int hot_y) ;
static DtHashEntry CreateImageCacheEntry( 
                        unsigned int cacheType,
                        char *imageName,
                        XImage *image,
                        int hot_x,
                        int hot_y) ;
static void DestroyImageCacheEntry( 
			DtHashEntry hashEntry);
static void NullDestroyProc( 
			DtHashEntry hashEntry);
static void DestroyInternalImageCacheEntry( 
			DtHashEntry hashEntry);
static DtPixmapCacheEntry * GetPixmapListPtr( 
                        DtHashEntry hashEntry) ;
static Boolean PutImageToPixmap( 
                        XImage *image,
                        DtPixmapCacheEntry pixmapEntry) ;
static Boolean GetInternalPixmap( 
                        DtHashEntry hashEntry,
                        DtPixmapCacheEntry pixmapEntry) ;
static Boolean GetMaskPixmap( 
                        DtHashEntry hashEntry,
                        DtPixmapCacheEntry pixmapEntry) ;
static Boolean GetBuiltInPixmap( 
                        DtHashEntry hashEntry,
                        DtPixmapCacheEntry pixmapEntry) ;
static Boolean GetXternalPixmap( 
                        DtHashEntry hashEntry,
                        DtPixmapCacheEntry pixmapEntry) ;
static Boolean GetImageEntry( 
                        DtHashEntry entry,
                        XtPointer clientData) ;
static Boolean IsBitonal( 
                        Screen *screen) ;
static DtHashEntry GetImageFromFile( 
                        Screen *screen,
                        char *image_name,
                        Pixel background,
			Pixel foreground,
			String maskName) ;
static Boolean GetPixmapEntry( 
                        DtHashEntry entry,
                        XtPointer clientData) ;
static GC GetGC(
                        DtPixmapCacheEntry pixmapEntry );
static void CleanupOnDisplayClose(
                        Widget displayWidget,
                        XtPointer callData,
                        XtPointer clientData);
static DtHashEntry __DtGetImage(
        		Screen *screen,
        		char *image_name,
        		Pixel background,
        		Pixel foreground,
        		String maskName);

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/
typedef struct _DtImageRec{
    unsigned int 	hot_x, hot_y;
    unsigned int 	width, height;
    unsigned int	depth;
}DtImageRec, *DtImage;

typedef struct _DtPixmapCacheEntryRec {
   Screen 		*screen;
   Pixel    		foreground;
   Pixel    		background;
   Pixmap   		pixmap;
   unsigned short 	depth;
   unsigned short 	refCount;
   struct _DtPixmapCacheEntryRec * next;
} DtPixmapCacheEntryRec;

typedef struct _DtMissedImageEntryPartRec{
    char		name[1];
}DtMissedImageEntryPartRec, *DtMissedImageEntryPart;

typedef struct _DtMissedImageEntryRec{
    DtHashEntryPartRec		hash;
    DtMissedImageEntryPartRec	missed;
}DtMissedImageEntryRec, *DtMissedImageEntry;



typedef struct _DtBuiltInImageEntryPartRec{
    unsigned short	builtInOffset;
    DtPixmapCacheEntry	pixmapData;
}DtBuiltInImageEntryPartRec, *DtBuiltInImageEntryPart;

typedef struct _DtBuiltInImageEntryRec{
    DtHashEntryPartRec		hash;
    DtBuiltInImageEntryPartRec	builtIn;
}DtBuiltInImageEntryRec, *DtBuiltInImageEntry;



typedef struct _DtXternalImageEntryPartRec{
    XImage		*xImage;
    DtPixmapCacheEntry	pixmapData;
    char		name[1];
}DtXternalImageEntryPartRec, *DtXternalImagePart;

typedef struct _DtXternalImageEntryRec{
    DtHashEntryPartRec		hash;
    DtXternalImageEntryPartRec	xternal;
}DtXternalImageEntryRec, *DtXternalImageEntry;



typedef struct _DtInternalImageEntryPartRec{
    unsigned short	hot_x, hot_y;
    unsigned short	width, height;
    unsigned short	depth;
    DtPixmapCacheEntry	pixmapData;
    char		name[1];
}DtInternalImageEntryPartRec, *DtInternalImageEntryPart;

typedef struct _DtInternalImageEntryRec{
    DtHashEntryPartRec			hash;
    DtInternalImageEntryPartRec	internal;
}DtInternalImageEntryRec, *DtInternalImageEntry;



typedef struct _DtMaskImageEntryPartRec{
    char *		maskBits;
    DtPixmapCacheEntry	pixmapData;
    DtInternalImageEntry imageData;
}DtMaskImageEntryPartRec, *DtMaskImageEntryPart;

typedef struct _DtMaskImageEntryRec{
    DtHashEntryPartRec			hash;
    DtMaskImageEntryPartRec		mask;
}DtMaskImageEntryRec, *DtMaskImageEntry;



typedef struct _DtDummyImageEntryPartRec{
    DtPixmapCacheEntry	pixmapData;
    char		name[1];
}DtDummyImageEntryPartRec, *DtDummyImageEntryPart;

typedef struct _DtDummyImageEntryRec{
    DtHashEntryPartRec			hash;
    DtDummyImageEntryPartRec		dummy;
}DtDummyImageEntryRec, *DtDummyImageEntry;



typedef DtHashEntry (*DtCreateImageEntryFunc)();
typedef Boolean	     (*DtGetPixmapFunc)();
typedef void	     (*DtVoidImageEntryProc)();

#define GET_PIXMAP(hEntry, toPix) \
  ((*((DtImageEntryType)imageHashTypes[hashEntry->hash.type])\
    ->image.getPixmap) (hEntry, toPix))

#define DESTROY_IMAGE_ENTRY(hEntry) \
  ((*((DtImageEntryType)imageHashTypes[hEntry->hash.type])\
    ->image.destroyImageEntry) (hEntry))

#define PIXMAP_DATA_OFFSET(hEntry) \
  (((DtImageEntryType)imageHashTypes[hashEntry->hash.type])\
   ->image.pixmapDataOffset)

typedef struct _DtImageEntryTypePartRec{
    DtCreateImageEntryFunc	createImageEntry;
    DtVoidImageEntryProc	destroyImageEntry;
    DtGetPixmapFunc		getPixmap;
    int				pixmapDataOffset;
}DtImageEntryTypePartRec;

typedef struct _DtImageEntryTypeRec{
    DtHashEntryTypePartRec	hash;
    DtImageEntryTypePartRec	image;
}DtImageEntryTypeRec, *DtImageEntryType;

static DtImageEntryTypeRec dtBuiltInImageEntryTypeRec = {
    {
	sizeof(DtBuiltInImageEntryRec),
	(DtGetHashKeyFunc)GetBuiltInKey,
	(XtPointer)NULL,
	NULL,
    },
    {
	NULL,
	NullDestroyProc,
	GetBuiltInPixmap,
	XtOffset(DtBuiltInImageEntry, builtIn.pixmapData),
    },
};

static DtImageEntryTypeRec dtXternalImageEntryTypeRec = {
    {
	sizeof(DtXternalImageEntryRec),
	(DtGetHashKeyFunc)GetEmbeddedKey,
	(XtPointer)XtOffset(DtXternalImageEntry, xternal.name[0]),
	NULL,
    },
    {
	CreateXternalImageEntry,
	DestroyImageCacheEntry,
	GetXternalPixmap,
	XtOffset(DtXternalImageEntry, xternal.pixmapData),
    },
};

static DtImageEntryTypeRec dtInternalImageEntryTypeRec = {
    {
	sizeof(DtInternalImageEntryRec),
	(DtGetHashKeyFunc)GetEmbeddedKey,
	(XtPointer)XtOffset(DtInternalImageEntry, internal.name[0]),
	NULL,
    },
    {
	CreateInternalImageEntry,
	DestroyInternalImageCacheEntry,
	GetInternalPixmap,
	XtOffset(DtInternalImageEntry, internal.pixmapData),
    },
};

static DtImageEntryTypeRec dtMissedImageEntryTypeRec = {
    {
	sizeof(DtMissedImageEntryRec),
	(DtGetHashKeyFunc)GetEmbeddedKey,
	(XtPointer)XtOffset(DtMissedImageEntry, missed.name[0]),
	NULL,
    },
    {
	CreateMissedImageEntry,
	DestroyImageCacheEntry,
	NULL,
	0,
    },
};

static DtImageEntryTypeRec dtMaskImageEntryTypeRec = {
    {
	sizeof(DtMaskImageEntryRec),
	(DtGetHashKeyFunc)GetMaskKey,
	(XtPointer)0,
	ReleaseMaskKey,
    },
    {
	CreateMaskImageEntry,
	DestroyImageCacheEntry,
#ifndef notdef
	GetMaskPixmap,
#else
	GetInternalPixmap,
#endif /* notdef */
	XtOffset(DtMaskImageEntry, mask.pixmapData),
    },
};

static DtImageEntryTypeRec dtDummyImageEntryTypeRec = {
    {
	sizeof(DtDummyImageEntryRec),
	(DtGetHashKeyFunc)GetEmbeddedKey,
	(XtPointer)XtOffset(DtDummyImageEntry, dummy.name[0]),
	NULL,
    },
    {
	CreateDummyImageEntry,
	DestroyImageCacheEntry,
	NULL,
	XtOffset(DtDummyImageEntry, dummy.pixmapData),
    },
};
	
/*  Image array handling defines, structure, and global statics  */

#define MAX_BUILTIN_IMAGES	11
#define INCREMENT_SIZE		10

#define DtBUILT_IN_IMAGE		0
#define DtBUILT_IM_IMAGE_MASK		(1L << DtBUILT_IN_IMAGE)

#define DtXTERNAL_IMAGE		1
#define DtXTERNAL_IMAGE_MASK		(1L << DtXTERNAL_IMAGE)

#define DtINTERNAL_IMAGE		2
#define DtINTERNAL_IMAGE_MASK		(1L <<	DtINTERNAL_IMAGE)

#define DtMISSED_IMAGE			3
#define DtMISSED_IMAGE_MASK		(1L << DtMISSED_IMAGE)

#define DtMASK_IMAGE			4
#define DtMASK_IMAGE_MASK		(1L << DtMASK_IMAGE)

#define DtDUMMY_IMAGE			5
#define DtDUMMY_IMAGE_MASK		(1L << DtDUMMY_IMAGE)

/* Macros for the manipulation of hash.flags field */

#define _DT_PIXDATA_IS_IMAGE_MASK ((unsigned short) 0x0001)
#define _DT_PIXDATA_IS_IMAGE_SHIFT 0

#define GET_PIXDATA_IS_IMAGE(flags) \
  ((unsigned char) \
   (((flags) & _DT_PIXDATA_IS_IMAGE_MASK) >> _DT_PIXDATA_IS_IMAGE_SHIFT))

#define SET_PIXDATA_IS_IMAGE(flags) \
(flags = (flags |\
	  (((unsigned short)(1) << _DT_PIXDATA_IS_IMAGE_SHIFT) &\
	   (_DT_PIXDATA_IS_IMAGE_MASK))))

#define UNSET_PIXDATA_IS_IMAGE(flags) \
(flags = (flags & (~_DT_PIXDATA_IS_IMAGE_MASK)))



static DtHashEntryType	 imageHashTypes[] = 
{
    (DtHashEntryType)&dtBuiltInImageEntryTypeRec,	/* DtBUILT_IN_IMAGE */
    (DtHashEntryType)&dtXternalImageEntryTypeRec,	/* DtXTERNAL_IMAGE */
    (DtHashEntryType)&dtInternalImageEntryTypeRec,	/* DtINTERNAL_IMAGE */
    (DtHashEntryType)&dtMissedImageEntryTypeRec,	/* DtMISSED_IMAGE   */
    (DtHashEntryType)&dtMaskImageEntryTypeRec,	/* DtMASK_IMAGE     */
    (DtHashEntryType)&dtDummyImageEntryTypeRec,	/* DtDUMMY_IMAGE */
};

static DtHashKey
#ifdef _NO_PROTO
GetEmbeddedKey( entry, clientData)
  DtHashEntry	entry;
  XtPointer	clientData;
#else
GetEmbeddedKey(
  DtHashEntry	entry,
  XtPointer	clientData)
#endif /* _NO_PROTO */
{
    return (DtHashKey)(((char *)entry) + (int)clientData);
}

static DtHashKey
#ifdef _NO_PROTO
GetIndirectKey( entry, clientData)
  DtHashEntry	entry;
  XtPointer	clientData;
#else
GetIndirectKey(
  DtHashEntry	entry,
  XtPointer	clientData)
#endif /* _NO_PROTO */
{
    return (DtHashKey)
      (*((char **) (((char *)entry) + (int)clientData)));
}

/*ARGSUSED*/
static DtHashKey
#ifdef _NO_PROTO
GetBuiltInKey( entry, clientData)
  DtHashEntry	entry;
  XtPointer	clientData;
#else
GetBuiltInKey(
  DtHashEntry	entry,
  XtPointer	clientData)
#endif /* _NO_PROTO */
{
    DtBuiltInImageEntry biEntry = (DtBuiltInImageEntry)entry;

    return (DtHashKey)(bitmap_name_set[biEntry->builtIn.builtInOffset]);
}


typedef struct _DtPathNameHeapRec{
    unsigned int	inUse;
    char		pathName[1000];
}DtPathNameHeapRec, *DtPathNameHeap;

static DtPathNameHeapRec maskNameHeap[2];

/*ARGSUSED*/
static DtHashKey
#ifdef _NO_PROTO
GetMaskKey( entry, clientData)
  DtHashEntry	entry;
  XtPointer	clientData;
#else
GetMaskKey(
  DtHashEntry	entry,
  XtPointer	clientData)
#endif /* _NO_PROTO */
{
    DtMaskImageEntry 		maskEntry = (DtMaskImageEntry)entry;
    DtInternalImageEntry 	intEntry;
    String			mask_name;
    int				i = 0;

    intEntry = maskEntry->mask.imageData;

    for (i = 0; i < XtNumber(maskNameHeap); i++)
      if (maskNameHeap[i].inUse == False) {
	  mask_name = maskNameHeap[i].pathName;
	  maskNameHeap[i].inUse = True;
	  break;
      }
    if (i == XtNumber(maskNameHeap))
      XtWarning("GetMaskKey: unreleased keys !!\n");

    _DtGenerateMaskName(((char *)&intEntry->internal.name[0]), mask_name);
    return (DtHashKey) mask_name;
}

/*ARGSUSED*/
static void
#ifdef _NO_PROTO
ReleaseMaskKey( entry, key)
    DtHashEntry	entry;
    DtHashKey		key;
#else
ReleaseMaskKey(
	   DtHashEntry	entry,
	   DtHashKey	key)
#endif /* _NO_PROTO */
{
    DtMaskImageEntry 		maskEntry = (DtMaskImageEntry)entry;
    DtInternalImageEntry 	intEntry;
    String			mask_name;
    int				i = 0;

    intEntry = maskEntry->mask.imageData;

    for (i = 0; i < XtNumber(maskNameHeap); i++)
      if (maskNameHeap[i].pathName == (String)key) {
	  maskNameHeap[i].inUse = False;
	  return;
      }
    XtWarning("ReleaseMaskKey: key not found in heap !!\n");
}


/*  Pixmap caching structure and global statics  */

static DtHashTable imageCache = NULL;


static DtBuiltInImageEntryRec	builtInImages[MAX_BUILTIN_IMAGES];
static XImage  		xbmImageRec;

/************************************************************************
 *
 *  InitializeImageCache
 *	Initialize the image cache
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
InitializeImageCache()
#else
InitializeImageCache( void )
#endif /* _NO_PROTO */
{
    register int i;
    
    if (imageCache)
      return;
    imageCache = _XmAllocHashTable(imageHashTypes,
				    XtNumber(imageHashTypes),
				    True /* keyIsString */);
    
    for (i = 0; i < MAX_BUILTIN_IMAGES; i++)
      {
	  builtInImages[i].hash.type = DtBUILT_IN_IMAGE;
	  builtInImages[i].builtIn.builtInOffset = i;
	  _XmRegisterHashEntry(imageCache,
				bitmap_name_set[i],
				(DtHashEntry)&builtInImages[i]);
      }

    xbmImageRec.xoffset = 0;
    xbmImageRec.format = XYBitmap;
    xbmImageRec.byte_order = MSBFirst;
    xbmImageRec.bitmap_pad = 8;
    xbmImageRec.bitmap_bit_order = LSBFirst;
    xbmImageRec.bitmap_unit = 8;
    xbmImageRec.bits_per_pixel = 1;
    xbmImageRec.depth = 1;
    xbmImageRec.bytes_per_line = 2;
    xbmImageRec.obdata = NULL;
}



/************************************************************************
 *
 *  CreateInternalImageEntry
 * 
 *
 ************************************************************************/
static DtHashEntry
#ifdef _NO_PROTO
CreateInternalImageEntry(imageName, image, hot_x, hot_y)
    char 		*imageName ;
    XImage 		*image ;
    int 		hot_x, hot_y;
#else
CreateInternalImageEntry(
    char 		*imageName,
    XImage 		*image,
    int 		hot_x, 
    int			hot_y)
#endif /* _NO_PROTO */
{
   DtInternalImageEntry 	imageData;
   DtHashEntryType		entryType = imageHashTypes[DtINTERNAL_IMAGE];
   int				nameLen = strlen(imageName);

   imageData = (DtInternalImageEntry)
     XtCalloc(1, entryType->hash.entrySize + nameLen);

   SET_PIXDATA_IS_IMAGE(imageData->hash.flags);
   imageData->internal.pixmapData = (DtPixmapCacheEntry)image;
   
   imageData->internal.hot_x = hot_x;
   imageData->internal.hot_y = hot_y;
   imageData->internal.depth = image->depth;
   imageData->internal.width = image->width;
   imageData->internal.height = image->height;
   strcpy((char *) ((char *)&(imageData->internal.name[0])),
	  imageName);
   return (DtHashEntry)imageData;
}


/************************************************************************
 *
 *  CreateXternalImageEntry
 * 
 *
 ************************************************************************/
/*ARGSUSED*/
static DtHashEntry
#ifdef _NO_PROTO
CreateXternalImageEntry(imageName, image, hot_x, hot_y)
    char 		*imageName ;
    XImage 		*image ;
    int 		hot_x, hot_y;
#else
CreateXternalImageEntry(
    char 		*imageName,
    XImage 		*image,
    int 		hot_x, 
    int			hot_y)
#endif /* _NO_PROTO */
{
   DtXternalImageEntry 	imageData;
   DtHashEntryType		entryType = imageHashTypes[DtXTERNAL_IMAGE];
   int				nameLen = strlen(imageName);

   imageData = (DtXternalImageEntry)
     XtCalloc(1, entryType->hash.entrySize + nameLen);

   imageData->xternal.xImage = image;

   strcpy((char *) ((char *)&(imageData->xternal.name[0])),
	  imageName);

   return (DtHashEntry)imageData;
}

/************************************************************************
 *
 *  CreateMaskImageEntry
 * 
 *
 ************************************************************************/
/*ARGSUSED*/
static DtHashEntry
#ifdef _NO_PROTO
CreateMaskImageEntry(imageName, image, hot_x, hot_y)
    char 		*imageName ;
    XImage 		*image ;
    int 		hot_x, hot_y;
#else
CreateMaskImageEntry(
    char 		*imageName,
    XImage 		*image,
    int 		hot_x, 
    int			hot_y)
#endif /* _NO_PROTO */
{
   DtMaskImageEntry	 	imageData;
   DtHashEntryType		entryType = imageHashTypes[DtMASK_IMAGE];

   imageData = (DtMaskImageEntry)
     XtCalloc(1, entryType->hash.entrySize);

   SET_PIXDATA_IS_IMAGE(imageData->hash.flags);
   imageData->mask.pixmapData = (DtPixmapCacheEntry)image;

   return (DtHashEntry)imageData;
}


/************************************************************************
 *
 *  CreateMissedImageEntry
 * 
 *
 ************************************************************************/
/*ARGSUSED*/
static DtHashEntry
#ifdef _NO_PROTO
CreateMissedImageEntry(imageName, image, hot_x, hot_y)
    char 		*imageName ;
    XImage 		*image ;
    int 		hot_x, hot_y;
#else
CreateMissedImageEntry(
    char 		*imageName,
    XImage 		*image,
    int 		hot_x, 
    int			hot_y)
#endif /* _NO_PROTO */
{
   DtMissedImageEntry	 	imageData;
   DtHashEntryType		entryType = imageHashTypes[DtMISSED_IMAGE];
   int				nameLen = strlen(imageName);

   imageData = (DtMissedImageEntry)
     XtCalloc(1, entryType->hash.entrySize + nameLen);


   strcpy((char *) ((char *)&(imageData->missed.name[0])),
	  imageName);

   return (DtHashEntry)imageData;
}


/************************************************************************
 *
 *  CreateDummyImageEntry
 * 
 *
 ************************************************************************/
/*ARGSUSED*/
static DtHashEntry
#ifdef _NO_PROTO
CreateDummyImageEntry(imageName, image, hot_x, hot_y)
    char 		*imageName ;
    XImage 		*image ;
    int 		hot_x, hot_y;
#else
CreateDummyImageEntry(
    char 		*imageName,
    XImage 		*image,
    int 		hot_x, 
    int			hot_y)
#endif /* _NO_PROTO */
{
   DtDummyImageEntry	 	imageData;
   DtHashEntryType		entryType = imageHashTypes[DtDUMMY_IMAGE];
   int				nameLen = strlen(imageName);

   imageData = (DtDummyImageEntry)
     XtCalloc(1, entryType->hash.entrySize + nameLen);

   strcpy((char *) ((char *)&(imageData->dummy.name[0])),
	  imageName);

   return (DtHashEntry)imageData;
}


/************************************************************************
 *
 *  CreateImageCacheEntry
 * 
 *
 ************************************************************************/
static DtHashEntry
#ifdef _NO_PROTO
  CreateImageCacheEntry(cacheType, imageName, image, hot_x, hot_y)
unsigned int	cacheType;
char 		*imageName ;
XImage 		*image ;
int 		hot_x, hot_y;
#else
CreateImageCacheEntry(
		      unsigned int	cacheType,
		      char 		*imageName,
		      XImage 		*image,
		      int 		hot_x, 
		      int			hot_y)
#endif /* _NO_PROTO */
{
    DtHashEntry 	imageData;
    DtImageEntryType	entryType = (DtImageEntryType)imageHashTypes[cacheType];
    imageData =  
      (*(entryType->image.createImageEntry))
	(imageName, image, hot_x, hot_y);
    
    imageData->hash.type = cacheType;
    
    _XmRegisterHashEntry(imageCache, imageName, (DtHashEntry)imageData);
    
    return imageData;
}

/************************************************************************
 *
 *  DestroyInternalImageCacheEntry
 * 
 *
 ************************************************************************/
static void
#ifdef _NO_PROTO
  DestroyInternalImageCacheEntry(hashEntry)
DtHashEntry	hashEntry;
#else
DestroyInternalImageCacheEntry(
		       DtHashEntry hashEntry)
#endif /* _NO_PROTO */
{
    DtInternalImageEntry intImage = (DtInternalImageEntry) hashEntry;

    /*
     * we assume that the mask pixmap is always destroyed before the
     * primary. Otherwise, this will cause a leak
     */
    if (intImage->internal.pixmapData == NULL) {
	char 			mask_name[1000];
	DtMaskImageEntry	maskImage;
	
	_DtGenerateMaskName(&intImage->internal.name[0], mask_name);
	if (maskImage = (DtMaskImageEntry)
	    _XmKeyToHashEntry(imageCache,
			       (DtHashKey)mask_name)) {
	    if ((maskImage->hash.type == DtMASK_IMAGE) ||
		(maskImage->hash.type == DtMISSED_IMAGE)) {
#ifdef DEBUG
		if (maskImage->hash.type == DtMASK_IMAGE) {
		    DtPixmapCacheEntry *pixPtr =
		      GetPixmapListPtr((DtHashKey)maskImage);
		    if (*pixPtr)
		      XtWarning("DestroyImage: mask still has server ids\n");
		}
#endif
		DESTROY_IMAGE_ENTRY(((DtHashEntry)maskImage));
	    }
	    else
	      XtWarning("bad mask image info in cache");
	}
	_XmUnregisterHashEntry(imageCache, hashEntry);
	XtFree((char *)hashEntry);
    }
}


/************************************************************************
 *
 *  DestroyImageCacheEntry
 * 
 *
 ************************************************************************/
static void
#ifdef _NO_PROTO
  DestroyImageCacheEntry(hashEntry)
DtHashEntry	hashEntry;
#else
DestroyImageCacheEntry(
		       DtHashEntry hashEntry)
#endif /* _NO_PROTO */
{
    _XmUnregisterHashEntry(imageCache, hashEntry);
    XtFree((char *)hashEntry);
}
/************************************************************************
 *
 *  NullDestroyProc
 * 
 *
 ************************************************************************/
static void
#ifdef _NO_PROTO
  NullDestroyProc(hashEntry)
DtHashEntry	hashEntry;
#else
NullDestroyProc(
		       DtHashEntry hashEntry)
#endif /* _NO_PROTO */
{
	return;
}


static DtPixmapCacheEntry *
#ifdef _NO_PROTO
  GetPixmapListPtr(hashEntry)
DtHashEntry  hashEntry;
#else
GetPixmapListPtr(DtHashEntry hashEntry)
#endif /* _NO_PROTO */
{
    DtImageEntryType 	type = (DtImageEntryType)imageHashTypes[hashEntry->hash.type];
    char 		*rAddr;
    DtPixmapCacheEntry *pixEntryPtr;

    if (type->image.pixmapDataOffset) {
	rAddr = (((char *)hashEntry) + type->image.pixmapDataOffset);
	pixEntryPtr = (DtPixmapCacheEntry *)rAddr;
	return pixEntryPtr;
    }
    else
      return NULL;
}

static Boolean
#ifdef _NO_PROTO
PutImageToPixmap(image, pixmapEntry)
XImage *image;
DtPixmapCacheEntry pixmapEntry;
#else
PutImageToPixmap(
		 XImage *image,
		 DtPixmapCacheEntry pixmapEntry)
#endif /* _NO_PROTO */
{    
    Screen			*screen = pixmapEntry->screen;
    Display			*display = DisplayOfScreen(screen);
    GC				gc;
    unsigned int		saveFormat;

    pixmapEntry->pixmap = 
      XCreatePixmap (display, RootWindowOfScreen(screen), 
		     image->width, image->height,
		     pixmapEntry->depth);

    gc = GetGC(pixmapEntry);

    /*
     * wacko workaround !!!
     */
    if (image->depth == 1) {
	saveFormat = image->format;
	image->format = XYBitmap;
    }

    XPutImage (display, pixmapEntry->pixmap, gc, image, 
	       0, 0, 0, 0, image->width, image->height);

    if (image->depth == 1) {
	image->format = saveFormat;
    }
    return True;
}


static void
#ifdef _NO_PROTO
FreePixmapEntry(pixmapEntry)
DtPixmapCacheEntry pixmapEntry;
#else
FreePixmapEntry(
		 DtPixmapCacheEntry pixmapEntry)
#endif /* _NO_PROTO */
{    
    if (pixmapEntry->pixmap != XmUNSPECIFIED_PIXMAP)
      XFreePixmap (DisplayOfScreen(pixmapEntry->screen),
		   pixmapEntry->pixmap);
    XtFree((char *)pixmapEntry);
}

static Boolean
#ifdef _NO_PROTO
GetBitonalPixmap(hashEntry, srcPixmapEntry, pixmapEntry)
DtHashEntry hashEntry;
DtPixmapCacheEntry srcPixmapEntry;
DtPixmapCacheEntry pixmapEntry;
#else
GetBitonalPixmap(
		 DtHashEntry hashEntry,
		 DtPixmapCacheEntry srcPixmapEntry,
		 DtPixmapCacheEntry pixmapEntry)
#endif /* _NO_PROTO */
{
    unsigned long 	planeMask;
    unsigned long 	planeBitMask;
    Pixel		tmpPixel;

    if (hashEntry->hash.type == DtINTERNAL_IMAGE) {
	DtInternalImageEntry imageEntry =
	  (DtInternalImageEntry)hashEntry;
	
	if (srcPixmapEntry) {
	    Screen 		*screen = srcPixmapEntry->screen;
	    Display 		*display = DisplayOfScreen(screen);
	    unsigned int 	i, depth = pixmapEntry->depth;
	    GC			gc;
	    Boolean flipped;
	    unsigned int	width, height;
	    /*
	     * find one plane where the foreground and background are
	     * different and use that for the plane arg.
	     */
	    planeMask = ((srcPixmapEntry->foreground ^
			  srcPixmapEntry->background) & 
			 ( (1 << depth) -1 ));
	    for (i = 0, planeBitMask = 1L;
		 i <= depth; 
		 i++, planeBitMask = (planeBitMask << 1))
	      if (planeBitMask & planeMask)
		break;
	    
	    width = imageEntry->internal.width;
	    height = imageEntry->internal.height;
	    pixmapEntry->pixmap = 
	      XCreatePixmap (display, RootWindowOfScreen(screen), 
			     width, height, pixmapEntry->depth);
	    
	    if (planeBitMask & srcPixmapEntry->foreground) {
		flipped = False;
	    }
	    else {
		flipped = True;
		tmpPixel = pixmapEntry->background;
		pixmapEntry->background = pixmapEntry->foreground;
		pixmapEntry->foreground = tmpPixel;
	    }
	    gc = GetGC(pixmapEntry);

	    XCopyPlane(display, srcPixmapEntry->pixmap, pixmapEntry->pixmap, 
		       gc, 0, 0, width, height, 
		       0, 0, planeBitMask);

	    if (planeBitMask & srcPixmapEntry->background) {
		tmpPixel = pixmapEntry->background;
		pixmapEntry->background = pixmapEntry->foreground;
		pixmapEntry->foreground = tmpPixel;
	    }
	    return True;
	}
    }
    return False;
}

static int
#ifdef _NO_PROTO
GetColorSymbols(screen, background, foreground, colorSymbolsRtn)
    Screen *screen;
    Pixel background;
    Pixel foreground;
    XpmColorSymbol **colorSymbolsRtn;
#else
GetColorSymbols(
		 Screen *screen,
		 Pixel background,
		 Pixel foreground,
		XpmColorSymbol **colorSymbolsRtn)
#endif /* _NO_PROTO */
{
    static XpmColorSymbol colorTemplate[] = 
      {
	  { 	XmNbackground, 		0, 	(Pixel)0,	},
	  { 	XmNforeground,		0,	(Pixel)0,	},
	  {	XmNtopShadowColor,	0,	(Pixel)0,	},
	  {	XmNbottomShadowColor,	0,	(Pixel)0,	},
	  {	XmNselectColor,		0,	(Pixel)0,	},
      };
    PixelSet 	pixelSets[NUM_COLORS];
    int 	colorUse ;
    short 	a, i, j, p, s;
    Boolean	bgFound = False, fgFound = False, result = False;
    Pixel	*pixelPtr;
    int		numColors = 0;

    *colorSymbolsRtn = NULL;

    if (!(result = _XmGetPixelData(XScreenNumberOfScreen(screen), 
			 &colorUse, pixelSets, &a, &i, &p, &s )))
      i = NUM_COLORS;
    else
      i = 0;

    for ( ; i < NUM_COLORS; i++) {
	bgFound = False;
	fgFound = False;
	pixelPtr = (Pixel *)&(pixelSets[i].fg);
	for (j = 0; j < 5; j++, pixelPtr++) {
	    if (*pixelPtr == background)
	      bgFound = True;
	    else if (*pixelPtr == foreground)
	      fgFound = True;
	}
	if (bgFound && fgFound)
	  break;
    }
    if (i == NUM_COLORS) {
	/*
	 * We didn't find the bg/fg tuple in any of the Dt colorsets
	 * so we can either try to use the background to get a motif
	 * color set or we can give up. 
	 */

      XmColorData *old_colors;
      XmColorData new_colors;

      new_colors.screen = screen;
      new_colors.color_map = DefaultColormapOfScreen(screen);
      new_colors.background.pixel = background;

      /* Use motif color set if already allocated.
       */
      if (!result && _XmSearchColorCache(
                (XmLOOK_AT_SCREEN | XmLOOK_AT_CMAP | XmLOOK_AT_BACKGROUND),
                        &new_colors, &old_colors))
          {
	  colorTemplate[0].pixel = background;
	  XmGetColors(screen, 
		    DefaultColormapOfScreen(screen),
		    colorTemplate[0].pixel,
		    &colorTemplate[1].pixel,
		    &colorTemplate[2].pixel,
		    &colorTemplate[3].pixel,
		    &colorTemplate[4].pixel);
	  *colorSymbolsRtn = colorTemplate;
	  numColors = XtNumber(colorTemplate);
       }
       else {
	  *colorSymbolsRtn = NULL;
	  numColors = 0;
       }
    }
    else {
	colorTemplate[0].pixel = pixelSets[i].bg;
	colorTemplate[1].pixel = pixelSets[i].fg;
	colorTemplate[2].pixel = pixelSets[i].ts;
	colorTemplate[3].pixel = pixelSets[i].bs;
	colorTemplate[4].pixel = pixelSets[i].sc;
	*colorSymbolsRtn = colorTemplate;
	numColors = XtNumber(colorTemplate);
    }
    return numColors;
}


static int
#ifdef _NO_PROTO
GetColorTable(screen, background, foreground, colorTableRtn, pixelsRtn)
    Screen *screen;
    Pixel background;
    Pixel foreground;
    char ****colorTableRtn;
    Pixel **pixelsRtn;
#else
GetColorTable(
		Screen *screen,
		Pixel background,
	        Pixel foreground,
		char  ****colorTableRtn,
		Pixel **pixelsRtn)
#endif /* _NO_PROTO */
{
    XpmColorSymbol	*colorSymbols;
    int			i;
    
    static Pixel	pixels[5];
    static String 	colorEntries[5][6] = 
      {
	  {	NULL,	XmNbackground, 		NULL,	NULL,	NULL,	NULL, },
	  {	NULL,	XmNforeground,		NULL,	NULL,	NULL,	NULL, },
	  {	NULL,	XmNtopShadowColor, 	NULL,	NULL,	NULL,	NULL, },
	  {	NULL,	XmNbottomShadowColor, 	NULL,	NULL,	NULL,	NULL, },
	  {	NULL,	XmNselectColor,	 	NULL,	NULL,	NULL,	NULL, },
      };
    static char 	**colorTable[5] = 
      { 
	  colorEntries[0], colorEntries[1], colorEntries[2], colorEntries[3], colorEntries[4],
      };
    
    if (!GetColorSymbols(screen, background, foreground, &colorSymbols)) {
    	*pixelsRtn = NULL;
	*colorTableRtn = NULL;
	return 0;
    }
    for (i = 0; i < 5; i++) {
	pixels[i] = colorSymbols[i].pixel;
    }
    *pixelsRtn = pixels;
    *colorTableRtn = colorTable;
    return 5;
}

static Boolean
#ifdef _NO_PROTO
GetDeepPixmap(hashEntry, pixmapEntry)
DtHashEntry hashEntry;
DtPixmapCacheEntry pixmapEntry;
#else
GetDeepPixmap(
	      DtHashEntry hashEntry,
	      DtPixmapCacheEntry pixmapEntry)
#endif /* _NO_PROTO */
{
    Boolean returnVal = False;

    if (hashEntry->hash.type == DtINTERNAL_IMAGE) {
	DtPixmapCacheEntry srcPixmapEntry;
	DtInternalImageEntry imageEntry =
	  (DtInternalImageEntry)hashEntry;

	if (srcPixmapEntry = imageEntry->internal.pixmapData) {
	    Screen 		*srcScreen = srcPixmapEntry->screen;
	    Display 		*display = DisplayOfScreen(srcScreen);
	    XpmAttributes 	attributes;
	    unsigned int	width, height;
	    int			numSrcColorSymbols, numDstColorSymbols;
	    XpmColorSymbol	*dstColorSymbols;
	    int			xpmReturnVal;
	    XImage		*image, *junkImage;
	    char		**data;
	    char 		***srcColorTable = NULL;
	    Pixel		*pixels;  

	    attributes.valuemask = 0;

	    attributes.cpp = 0; /* must be set for Xpm bootstrap code */
	    attributes.mask_pixel = 0x80000000; /* equivalent to UNDEF_PIXEL */

	    numSrcColorSymbols = GetColorTable(srcScreen, 
					       srcPixmapEntry->background,
					       srcPixmapEntry->foreground,
					       &srcColorTable,
					       &pixels);
	    numDstColorSymbols = GetColorSymbols(pixmapEntry->screen, 
						 pixmapEntry->background, 
						 pixmapEntry->foreground,
						 &dstColorSymbols);

	    if (numSrcColorSymbols && numDstColorSymbols) {
		attributes.colorTable = (char ***)srcColorTable;
		attributes.pixels = pixels;
		attributes.npixels = 
		  attributes.ncolors = numSrcColorSymbols;
		attributes.valuemask |= XpmInfos;
	    }

	    attributes.width = imageEntry->internal.width;
	    attributes.height = imageEntry->internal.height;
	    attributes.valuemask |= XpmSize;

	    xpmReturnVal = 
	      _XmXpmCreateDataFromPixmap(display, &data,
				      srcPixmapEntry->pixmap, NULL,
				      &attributes);
	    
	    if (xpmReturnVal != XpmSuccess) {
		return False;
	    }

	    attributes.valuemask = 0;

	    if (numSrcColorSymbols && numDstColorSymbols) {
		attributes.colorsymbols = dstColorSymbols;
		attributes.numsymbols = numDstColorSymbols;
		attributes.valuemask |= XpmColorSymbols;
	    }

	    attributes.depth = pixmapEntry->depth;
	    attributes.valuemask |= XpmDepth;

	    /*
	     * create the images 
	     */
	    xpmReturnVal = _XmXpmCreateImageFromData(display, data, &image,
						  &junkImage, &attributes);

	    free((char *)data);
	    if (xpmReturnVal != XpmSuccess) {
		return False;
	    }	    

	    pixmapEntry->pixmap = 
	      XCreatePixmap (display, 
			     RootWindowOfScreen(pixmapEntry->screen), 
			     image->width, image->height,
			     pixmapEntry->depth);
	    returnVal = PutImageToPixmap(image, pixmapEntry);
	    XDestroyImage(image);
	}
    }
    return returnVal;
}



static Boolean
#ifdef _NO_PROTO
GetInternalPixmap(hashEntry, pixmapEntry)
DtHashEntry hashEntry;
DtPixmapCacheEntry pixmapEntry;
#else
GetInternalPixmap(
	       DtHashEntry hashEntry,
	       DtPixmapCacheEntry pixmapEntry)
#endif /* _NO_PROTO */
{    
    XImage	 		*image;
    Boolean			returnVal;
    DtInternalImageEntry	imageEntry =
      (DtInternalImageEntry)hashEntry;

    if (GET_PIXDATA_IS_IMAGE(imageEntry->hash.flags)) {
	image = (XImage *)imageEntry->internal.pixmapData;
	returnVal = PutImageToPixmap(image, pixmapEntry);
#ifdef notdef
	XDestroyImage(image);
#else
	XFree((char *)image->data);
	XFree((char *)image);
#endif
	UNSET_PIXDATA_IS_IMAGE(imageEntry->hash.flags);
	imageEntry->internal.pixmapData = NULL;
    }
    else {
	DtPixmapCacheEntry srcPixmapEntry;

	for(srcPixmapEntry = imageEntry->internal.pixmapData;
	    (srcPixmapEntry && 
	     (srcPixmapEntry->screen != pixmapEntry->screen));
	    srcPixmapEntry = srcPixmapEntry->next) {};

	if (srcPixmapEntry && (imageEntry->internal.depth == 1)) {
	    returnVal = 
	      GetBitonalPixmap(hashEntry, srcPixmapEntry, pixmapEntry);
	}
	else {
	    returnVal = GetDeepPixmap(hashEntry, pixmapEntry);
	}
    }
    if (returnVal) {
	pixmapEntry->next = imageEntry->internal.pixmapData;
	imageEntry->internal.pixmapData = pixmapEntry;
    }
    else {
	FreePixmapEntry(pixmapEntry);
    }
    return (returnVal);
}

static Boolean
#ifdef _NO_PROTO
GetMaskPixmap(hashEntry, pixmapEntry)
DtHashEntry hashEntry;
DtPixmapCacheEntry pixmapEntry;
#else
GetMaskPixmap(
	       DtHashEntry hashEntry,
	       DtPixmapCacheEntry pixmapEntry)
#endif /* _NO_PROTO */
{    
    XImage 		*image;
    Boolean		returnVal;
    DtMaskImageEntry	imageEntry =
      (DtMaskImageEntry)hashEntry;

    if (GET_PIXDATA_IS_IMAGE(imageEntry->hash.flags)) {
	image = (XImage *)imageEntry->mask.pixmapData;
	returnVal = PutImageToPixmap(image, pixmapEntry);
#ifdef notdef
	XDestroyImage(image);
#else
	XFree((char *)image->data);
	XFree((char *)image);
#endif
	UNSET_PIXDATA_IS_IMAGE(imageEntry->hash.flags);
	imageEntry->mask.pixmapData = NULL;
    }
    else {
	DtPixmapCacheEntry srcPixmapEntry;

	for(srcPixmapEntry = imageEntry->mask.pixmapData;
	    (srcPixmapEntry && 
	     (srcPixmapEntry->screen != pixmapEntry->screen));
	    srcPixmapEntry = srcPixmapEntry->next) {};

	if (srcPixmapEntry && (srcPixmapEntry->depth == 1)) {
	    returnVal = 
	      GetBitonalPixmap((DtHashEntry)imageEntry->mask.imageData, 
			       srcPixmapEntry, 
			       pixmapEntry);
	}
	else {
	    returnVal = GetDeepPixmap(hashEntry, pixmapEntry);
	}
#ifdef notdef
	XtWarning("GetMaskPixmap: unimplemented different colors\n");
	returnVal = False;
#endif
    }

    if (returnVal) {
	pixmapEntry->next = imageEntry->mask.pixmapData;
	imageEntry->mask.pixmapData = pixmapEntry;
    }
    else {
	FreePixmapEntry(pixmapEntry);
    }
    return (returnVal);
}

static Boolean
#ifdef _NO_PROTO
GetBuiltInPixmap(hashEntry, pixmapEntry)
DtHashEntry hashEntry;
DtPixmapCacheEntry pixmapEntry;
#else
GetBuiltInPixmap(
	       DtHashEntry hashEntry,
	       DtPixmapCacheEntry pixmapEntry)
#endif /* _NO_PROTO */
{    
    XImage	 		*image;
    Boolean			returnVal;
    DtBuiltInImageEntry	imageEntry =
      (DtBuiltInImageEntry)hashEntry;

    image = &xbmImageRec;
    image->width = 16;   /* these hard-coded values are also used in */
    image->height = 16;  /* _XmGetPixmapData() for assigning width/height */
    image->data = (char *)bitmaps[imageEntry->builtIn.builtInOffset];

    returnVal = PutImageToPixmap(image, pixmapEntry);

    if (returnVal) {
	pixmapEntry->next = imageEntry->builtIn.pixmapData;
	imageEntry->builtIn.pixmapData = pixmapEntry;
    }
    else {
	FreePixmapEntry(pixmapEntry);
    }
    return (returnVal);
}



static Boolean
#ifdef _NO_PROTO
GetXternalPixmap(hashEntry, pixmapEntry)
DtHashEntry hashEntry;
DtPixmapCacheEntry pixmapEntry;
#else
GetXternalPixmap(
	       DtHashEntry hashEntry,
	       DtPixmapCacheEntry pixmapEntry)
#endif /* _NO_PROTO */
{    
    Boolean			returnVal;
    DtXternalImageEntry	imageEntry =
      (DtXternalImageEntry)hashEntry;

    returnVal = PutImageToPixmap(imageEntry->xternal.xImage,
				 pixmapEntry);

    if (returnVal) {
	pixmapEntry->next = imageEntry->xternal.pixmapData;
	imageEntry->xternal.pixmapData = pixmapEntry;
    }
    else {
	FreePixmapEntry(pixmapEntry);
    }
    return (returnVal);
}


/*
 * create a pixmap from the image_name.  foreground and background
 * must be valid values. For depth 1 they should be 1 and 0
 * respectively. 
 */
static Pixmap 
#ifdef _NO_PROTO
  __DtGetPixmap(screen, image_name, depth, foreground, background, maskName)
Screen *screen ;
char *image_name ;
int depth;
Pixel foreground ;
Pixel background ;
String maskName ;
#else
__DtGetPixmap(
	       Screen *screen,
	       char *image_name,
	       int depth,
	       Pixel foreground,
	       Pixel background,
	       String maskName)
#endif /* _NO_PROTO */
{    
    DtPixmapCacheEntry 	savedPixmapEntries = NULL, *listPtr, *listHeadPtr;
    DtHashEntry	 	hashEntry = NULL;
    DtPixmapCacheEntry		toPixmapData = NULL;
    DtImageEntryType 		entryType;
    String 			hashName = (maskName) ? maskName : image_name;
    Boolean			returnVal = False;

    /*  Error checking  */
    
    if (hashName == NULL) return (XmUNSPECIFIED_PIXMAP);
    
    if (imageCache == NULL) InitializeImageCache();

    hashEntry = (DtHashEntry)
      _XmKeyToHashEntry(imageCache, (DtHashKey)hashName);

    if (hashEntry && hashEntry->hash.type == DtMISSED_IMAGE)
      return (XmUNSPECIFIED_PIXMAP);

    if (hashEntry &&
	(listHeadPtr = listPtr = GetPixmapListPtr(hashEntry)) &&
	(!GET_PIXDATA_IS_IMAGE(hashEntry->hash.flags)))
      {
	  for (; *listPtr; listPtr = &((*listPtr)->next))
	    {
		if ((*listPtr)->depth == depth && (*listPtr)->screen == screen)
		  {
		      if ((*listPtr)->foreground == foreground          &&
			  (*listPtr)->background == background)
			{
			    
			    (*listPtr)->refCount++;
			    return ((*listPtr)->pixmap);
			}
		  }
	    }
      }
    
    toPixmapData = XtNew (DtPixmapCacheEntryRec);
    toPixmapData->screen = screen;
    toPixmapData->foreground = foreground;
    toPixmapData->background = background;
    toPixmapData->depth = depth;
    toPixmapData->refCount = 1;
    toPixmapData->pixmap = XmUNSPECIFIED_PIXMAP;

    if (hashEntry) {
	if (hashEntry->hash.type == DtDUMMY_IMAGE) {
	    savedPixmapEntries = *listHeadPtr;
	    DESTROY_IMAGE_ENTRY(hashEntry);
	    hashEntry = NULL;
	}
	else {
	    returnVal = GET_PIXMAP(hashEntry, toPixmapData);
	}
    }
    
    if (!hashEntry) {
	if ((hashEntry = __DtGetImage(screen, image_name, 
				       background, foreground, maskName)) &&
	    (hashEntry->hash.type != DtMISSED_IMAGE))
	  {
	      returnVal = GET_PIXMAP(hashEntry, toPixmapData);
	  }
	else
	  FreePixmapEntry(toPixmapData);
    }

    if (savedPixmapEntries) {
	if (!returnVal)
	  hashEntry = CreateImageCacheEntry(DtDUMMY_IMAGE,
					    hashName,
					    NULL,
					    0,0);
	for (listHeadPtr = &(savedPixmapEntries->next);
	     *listHeadPtr;
	     listHeadPtr = &((*listHeadPtr)->next)) {};
	listPtr = GetPixmapListPtr(hashEntry);
	*listHeadPtr = *listPtr;
	*listPtr = savedPixmapEntries;
    }

    if (returnVal)
      return toPixmapData->pixmap;
    else 
      return XmUNSPECIFIED_PIXMAP;
}


/************************************************************************
 *
 *  _XmInstallImage
 *	Add the provided image for the image set and return an
 *	tile id to be used for further referencing.  Keep the
 *	allocation of the image_set array straight.
 *
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmInstallImage( image, image_name, hot_x, hot_y )
        XImage *image ;
        char *image_name ;
	int hot_x;
	int hot_y;
#else
_XmInstallImage(
        XImage *image,
        char *image_name,
	int hot_x,
	int hot_y)
#endif /* _NO_PROTO */
{
    if (imageCache == NULL) 
      InitializeImageCache();
    /*
     * only used for xternal images
     */
    (void)CreateImageCacheEntry(DtXTERNAL_IMAGE,
				image_name,
				image,
				hot_x, hot_y);
    return True;
}

/************************************************************************
 *
 *  XmInstallImage
 *      Add the provided image for the image set and return an
 *      tile id to be used for further referencing.  Keep the
 *      allocation of the image_set array straight.
 *
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
XmInstallImage( image, image_name )
        XImage *image ;
        char *image_name ;
#else
XmInstallImage(
        XImage *image,
        char *image_name )
#endif /* _NO_PROTO */
{
    return _XmInstallImage(image, image_name, 0, 0);
}


static Boolean 
#ifdef _NO_PROTO
GetImageEntry(entry, clientData)
    DtHashEntry	entry;
    XtPointer		clientData;
#else
GetImageEntry(DtHashEntry entry,
	      XtPointer clientData)
#endif /* _NO_PROTO */
{
    if (entry->hash.type != DtXTERNAL_IMAGE)
      return False;
    else {
	DtXternalImageEntry extImage = (DtXternalImageEntry)entry;
	if (extImage->xternal.xImage == (XImage *)clientData)
	  return True;
	else
	  return False;
    }
}

/************************************************************************
 *
 *  XmUninstallImage
 *	Remove an image from the image set.
 *	Return a boolean (True) if the uninstall succeeded.  Return
 *	a boolean (False) if an error condition occurs.
 *
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
XmUninstallImage( image )
        XImage *image ;
#else
XmUninstallImage(
        XImage *image )
#endif /* _NO_PROTO */
{
   DtXternalImageEntry extImage;

   /*  Check for invalid conditions  */

   if ((image == NULL) || (imageCache == NULL)) return (False);

   extImage = (DtXternalImageEntry)
     _XmEnumerateHashTable(imageCache,
			    (DtHashEnumerateFunc)GetImageEntry,
			    (XtPointer)image);
   
   if (extImage && (extImage->hash.type == DtXTERNAL_IMAGE)) {
       if (extImage->xternal.pixmapData) {
	   /*
	    * what do we do ??
	    */
	   XtWarning("attempting to uninstall image with pixmaps");
	   return False;
       }
       DESTROY_IMAGE_ENTRY(((DtHashEntry)extImage));
       return True;
   }
   return False;
}


/* 
 * hardcode for now
 */
/*ARGSUSED*/
static Boolean 
#ifdef _NO_PROTO
IsBitonal(screen)
    Screen *screen;
#else
IsBitonal(
		 Screen *screen)
#endif /* _NO_PROTO */
{
    return False;
}


static DtHashEntry
#ifdef _NO_PROTO
GetImageFromFile(screen, image_name, background, foreground, maskName)
    Screen *screen;
    char *image_name;
    Pixel background;
    Pixel foreground;
    String maskName;
#else
GetImageFromFile(
		 Screen *screen,
		 char *image_name,
		 Pixel background,
		 Pixel foreground,
		 String maskName)
#endif /* _NO_PROTO */
{
    char 		*file_name;
    unsigned int 	hot_x = 0, hot_y = 0;
    DtHashEntry        hashEntry = NULL;
    XImage 		*image;
    String 		hashName = (maskName) ? maskName : image_name;
    
    if (file_name = XmGetIconFileName(screen, NULL, hashName, 
				       NULL, XmUNSPECIFIED_ICON_SIZE))
      {
	  XImage 			*shapeimage = NULL;
	  XpmAttributes 		attributes;
	  int				xpmReturnVal = -1;
	  XpmColorSymbol		*colorSymbols;
	  int				numColorSymbols;
	  Boolean			useIconFileCache;
	  Boolean			useMask;
	  Boolean			useColor;
	  Visual			visual;
	  
	  attributes.valuemask = 0;
	  attributes.valuemask |= XpmVisual;
	  attributes.visual = DefaultVisualOfScreen(screen);
	  /*
	   * work around the fact that we dont use grayscale specs in
	   * our icon color tables. Force it to pseudocolor so we get
	   * something reasonable
	   */
	  if ((attributes.visual->class == GrayScale) ||
	      (attributes.visual->class == StaticGray)) {
	      memcpy(&visual, attributes.visual, sizeof(Visual));
	      if (visual.class == GrayScale)
		visual.class = PseudoColor;
	      else if (visual.class == StaticGray)
		visual.class = StaticColor;
	      attributes.visual = &visual;
	  }

	  if (numColorSymbols = GetColorSymbols(screen, background,
						foreground,
						&colorSymbols)) {
	      attributes.colorsymbols = colorSymbols;
	      attributes.numsymbols = numColorSymbols;
	      attributes.valuemask |= XpmColorSymbols;
	  }

	  (void)_XmGetIconControlInfo(screen, 
				      &useMask,
				      &useColor,
				      &useIconFileCache);

	  if (!useColor) {
	      attributes.depth = 1;
	      attributes.valuemask |= XpmDepth;
	  }

	  xpmReturnVal = 
	    _XmXpmReadFileToImage(DisplayOfScreen(screen), file_name, &image,
			       &shapeimage, &attributes);

	  if ((xpmReturnVal >= 0) ||
	      ((image = (XImage *)
		_XmGetImageAndHotSpotFromFile (file_name,
						 (int *)&hot_x, 
						 (int *)&hot_y))
	       != NULL))
	    {


		if (maskName) {
		    DtHashEntry imageEntry;

		    imageEntry = (DtHashEntry) 
		      _XmKeyToHashEntry(imageCache, (DtHashKey)image_name);
    
		    if (imageEntry && 
			(imageEntry->hash.type == DtINTERNAL_IMAGE)) {
			hashEntry = 
			  CreateImageCacheEntry(DtMASK_IMAGE,
						maskName,
						image,
						0, 0);
			((DtMaskImageEntry)hashEntry)->mask.imageData =
			  (DtInternalImageEntry)imageEntry;

		    }
		    else {
			(*(image->f.destroy_image)) (image);
		    }
		}
		else {
		    char 		mask_name[256];
		    _DtGenerateMaskName(image_name, mask_name);
		    
		    hashEntry = CreateImageCacheEntry(DtINTERNAL_IMAGE,
						      image_name,
						      image,
						      hot_x, hot_y);
		    if (shapeimage) {
			DtMaskImageEntry 	maskImage;
			DtMaskImageEntry 	maskEntry;
			
			/*
			 * make sure that the mask hasn't been
			 * registered as a result of being a stand-alone
			 * xbm mask, in addition to being embedded in an
			 * xpm file
			 */
			maskEntry = (DtMaskImageEntry) 
			  _XmKeyToHashEntry(imageCache, (DtHashKey)mask_name);

			if (!maskEntry && useMask) {
			    maskImage =(DtMaskImageEntry)
			      CreateImageCacheEntry(DtMASK_IMAGE,
						    mask_name,
						    shapeimage,
						    0, 0);
			    maskImage->mask.imageData = 
			      (DtInternalImageEntry) hashEntry;
			}
			else { 
			    /*
			     * get rid of the mask image and cause a miss
			     * record to be made
			     */
			    
			    (*(shapeimage->f.destroy_image))
			      (shapeimage);
			    /*
			     * we only want a miss entry below if
			     * there is no stand-alone mask
			     */
			    if (!maskEntry)
			      shapeimage = NULL;
			}
		    }
		    if ((xpmReturnVal == XpmSuccess) && !shapeimage) {
			/*
			 * generate a negative entry in the cache
			 */
			(void)
			  CreateImageCacheEntry(DtMISSED_IMAGE, mask_name,
						NULL, 0, 0);
		    }
		}
		XtFree(file_name);
	    }
      }
    return hashEntry;
}


static DtHashEntry
#ifdef _NO_PROTO
__DtGetImage(screen, image_name, background, foreground, maskName)
	Screen *screen;
	char *image_name;
        Pixel background;
        Pixel foreground;
        String maskName;
#else
__DtGetImage(
	Screen *screen,
	char *image_name,
	Pixel background,
	Pixel foreground,
	String maskName)
#endif /* _NO_PROTO */
{
    int hot_x = 0, hot_y = 0;
    DtHashEntry hashEntry;
    DtHashKey hashKey = (maskName) ? (DtHashKey)maskName : (DtHashKey)image_name;

    /*  Check for the initial allocation of the image set array  */
    
    if (imageCache == NULL)
      InitializeImageCache();

    hashEntry = (DtHashEntry) _XmKeyToHashEntry(imageCache, hashKey);
    
    if (!hashEntry)
    /*  If no image was found, set up and go try to
	get an image from a file  */
      hashEntry = GetImageFromFile(screen, image_name, 
				   background, foreground, maskName);

    if (!hashEntry)
	/*
	 * generate a negative entry in the cache
	 */
      hashEntry = CreateImageCacheEntry(DtMISSED_IMAGE,
					 (String)hashKey,
					 NULL,
					 hot_x, hot_y);
    return hashEntry;
}

Boolean 
#ifdef _NO_PROTO
_XmGetImage(screen, image_name, imageRtn)
	Screen *screen;
	char *image_name;
        XImage **imageRtn;
#else
_XmGetImage(
	Screen *screen,
	char *image_name,
	XImage **imageRtn)
#endif /* _NO_PROTO */
{
    DtHashEntry hashEntry;

    if (!(hashEntry = __DtGetImage(screen, image_name, 0, 1, NULL)) ||
	 (hashEntry->hash.type != DtBUILT_IN_IMAGE))
      return False;
    else {
	/* scarf up an XImage to return to caller */
	XImage	 		*image;
	DtBuiltInImageEntry	imageEntry =
	  (DtBuiltInImageEntry)hashEntry;
	
	image = &xbmImageRec;
	image->width = 16;
	image->height = 16;
	image->data = (char *)bitmaps[imageEntry->builtIn.builtInOffset];
	*imageRtn = image;
	return True;
    }
}


static Boolean 
#ifdef _NO_PROTO
GetPixmapEntry(entry, clientData)
    DtHashEntry	entry;
    XtPointer		clientData;
#else
GetPixmapEntry(DtHashEntry entry,
	      XtPointer clientData)
#endif /* _NO_PROTO */
{
    DtPixmapCacheEntry *listPtr;

    if (GET_PIXDATA_IS_IMAGE(entry->hash.flags) ||
	!(listPtr = GetPixmapListPtr(entry)))
      return False;

    while (*listPtr && ((*listPtr)->pixmap != (Pixmap)clientData))
      listPtr = &((*listPtr)->next);

    if (listPtr && *listPtr)
      return True;
    else
      return False;
}


/*
 * see if this pixmap is in the cache. If it is then return all the
 * gory details about it
 */
/*ARGSUSED*/
Boolean 
#ifdef _NO_PROTO
  _XmGetPixmapData(screen, pixmap, image_name, depth, foreground,
		     background, hot_x, hot_y, width, height)
Screen *screen ;
Pixmap pixmap;
char  **image_name ;/* RETURN */
int   *depth;	/* RETURN */
Pixel *foreground ; /* RETURN */
Pixel *background ; /* RETURN */
int	*hot_x, *hot_y; /* RETURN */
unsigned int *width, *height;/* RETURN */
#else
_XmGetPixmapData(
		   Screen *screen,
		   Pixmap pixmap,
		   char **image_name,
		   int *depth,
		   Pixel *foreground,
		   Pixel *background,
		   int *hot_x,
		   int *hot_y,
		   unsigned int *width,
		   unsigned int *height)
#endif /* _NO_PROTO */
{
    register DtPixmapCacheEntry pixmapData;
    register DtHashEntry  hashEntry;
    register DtPixmapCacheEntry *listPtr;
    
    hashEntry = (DtHashEntry)
      _XmEnumerateHashTable(imageCache,
			     (DtHashEnumerateFunc)GetPixmapEntry,
			     (XtPointer)pixmap);
    if (!hashEntry)
      return False;
    
    listPtr = GetPixmapListPtr(hashEntry);
    
    while (*listPtr && ((*listPtr)->pixmap != (Pixmap)pixmap))
      listPtr = &((*listPtr)->next);
    
    pixmapData = *listPtr;
    *foreground = pixmapData->foreground;
    *background = pixmapData->background;
    *depth = pixmapData->depth;
    switch(hashEntry->hash.type) {
	case DtBUILT_IN_IMAGE:
	    *width = *height = 16;
	    *hot_x = *hot_y = 0;
	    break;
	case DtXTERNAL_IMAGE:
	    if (((DtXternalImageEntry)hashEntry)->xternal.xImage) {
	        *width = ((DtXternalImageEntry)hashEntry)->xternal.xImage->width;
	        *height = ((DtXternalImageEntry)hashEntry)->xternal.xImage->height;
	    }
	    else
		*width = *height = 0;
	    *hot_x = 0;
	    *hot_y = 0;
	    break;
	case DtINTERNAL_IMAGE:
	    *width = ((DtInternalImageEntry)hashEntry)->internal.width;
	    *height = ((DtInternalImageEntry)hashEntry)->internal.height;
	    *hot_x = ((DtInternalImageEntry)hashEntry)->internal.hot_x;
	    *hot_y = ((DtInternalImageEntry)hashEntry)->internal.hot_y;
	    break;
	case DtMASK_IMAGE:
	    *width = 
	((DtMaskImageEntry)hashEntry)->mask.imageData->internal.width;
	    *height = 
	((DtMaskImageEntry)hashEntry)->mask.imageData->internal.height;
	    *hot_x = 
	((DtMaskImageEntry)hashEntry)->mask.imageData->internal.hot_x;
	    *hot_y = 
	((DtMaskImageEntry)hashEntry)->mask.imageData->internal.hot_y;
	    break;
	case DtMISSED_IMAGE:
	case DtDUMMY_IMAGE:
	default:
	    *width = *height = *hot_x = *hot_y = 0;
	    break;
    }

    return True;
}

/*
 * create a pixmap from the image_name.  foreground and background
 * must be valid values. For depth 1 they should be 1 and 0
 * respectively.
 */
Pixmap
#ifdef _NO_PROTO
_XmGetPixmap(screen, image_name, depth, foreground, background)
    Screen *screen ;
    char *image_name ;
    int depth;
    Pixel foreground ;
    Pixel background ;
#else
_XmGetPixmap(
    Screen *screen,
    char *image_name,
    int depth,
    Pixel foreground,
    Pixel background)
#endif /* _NO_PROTO */
{
    return __DtGetPixmap(screen, image_name,
                          depth, foreground, background, NULL);
}

/************************************************************************
*
*  XmGetPixmapByDepth
*       Public wrapper around _XmGetPixmap with parameter order changed.
*
************************************************************************/
Pixmap
#ifdef _NO_PROTO
XmGetPixmapByDepth(screen, image_name, foreground, background, depth)
    Screen *screen ;
    char *image_name ;
    Pixel foreground ;
    Pixel background ;
    int depth;
#else
XmGetPixmapByDepth(
    Screen *screen,
    char *image_name,
    Pixel foreground,
    Pixel background,
    int depth)
#endif /* _NO_PROTO */
{ 
    return(__DtGetPixmap(screen, image_name, depth,
			  foreground, background, NULL));
}

/************************************************************************
*
*  XmGetPixmap
*       Create a pixmap of screen depth, using the image referenced
*       by the name and the foreground and background colors
*       specified.  Ensure that multiple pixmaps of the same attributes
*       are not created by maintaining a cache of the pixmaps.
*
************************************************************************/
Pixmap
#ifdef _NO_PROTO
XmGetPixmap( screen, image_name, foreground, background )
        Screen *screen ;
        char *image_name ;
        Pixel foreground ;
        Pixel background ;
#else
XmGetPixmap(
        Screen *screen,
        char *image_name,
        Pixel foreground,
        Pixel background )
#endif /* _NO_PROTO */
{
    return(__DtGetPixmap(screen, image_name, DefaultDepthOfScreen(screen),
                          foreground, background, NULL));
}

/************************************************************************
*
*  XmeGetMask
*
************************************************************************/
Pixmap 
#ifdef _NO_PROTO
XmeGetMask( screen, image_name )
        Screen *screen ;
        char *image_name ;
#else
XmeGetMask(
        Screen *screen,
        char *image_name)
#endif /* _NO_PROTO */
{
    char 		mask_name[256];

    _DtGenerateMaskName(image_name, mask_name);
    return (__DtGetPixmap(screen, image_name, 1, 1, 0, mask_name));
}

/************************************************************************
 *
 *  _XmInstallPixmap
 *	Install a pixmap into the pixmap cache.  This is used to add
 *	cached pixmaps which have no image associated with them.
 *
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmInstallPixmap( pixmap, screen, image_name, foreground, background )
        Pixmap pixmap ;
        Screen *screen ;
        char *image_name ;
        Pixel foreground ;
        Pixel background ;
#else
_XmInstallPixmap(
        Pixmap pixmap,
        Screen *screen,
        char *image_name,
        Pixel foreground,
        Pixel background )
#endif /* _NO_PROTO */
{
    register DtPixmapCacheEntry  	cache_ptr, *listPtr;
    register DtHashEntry 		hashEntry = NULL;
    
    /*  Error checking  */
    
    if (image_name == NULL)
      return (False);
    
    if (imageCache == NULL)
      InitializeImageCache();
    
    hashEntry = (DtHashEntry)
      _XmKeyToHashEntry(imageCache, (DtHashKey)image_name);

    if (hashEntry) {
	if (hashEntry->hash.type == DtMISSED_IMAGE) {
	    DESTROY_IMAGE_ENTRY(hashEntry);
	    hashEntry = NULL;
	}
	else if (hashEntry->hash.type != DtDUMMY_IMAGE)
	  return False;
    }

    if (!hashEntry)
      {
	  /* create a dummy entry */
	  hashEntry = CreateImageCacheEntry(DtDUMMY_IMAGE,
					    image_name,
					    NULL,
					    0, 0);
      }

    for (listPtr = GetPixmapListPtr(hashEntry);
	 listPtr && *listPtr;
	 listPtr = &((*listPtr)->next)) {
	
	if ((*listPtr)->foreground == foreground              &&
	    (*listPtr)->background == background              &&
	    (*listPtr)->screen == screen)
	  {
	      return False;
	  }
    }
    
    /*  Allocate the cache structure and put it into the list  */
    
    cache_ptr = XtNew (DtPixmapCacheEntryRec);
    cache_ptr->next = *listPtr;
    *listPtr = cache_ptr;
    
    cache_ptr->screen = screen;
    cache_ptr->foreground = foreground;
    cache_ptr->background = background;
    cache_ptr->depth = DefaultDepthOfScreen(screen);
    cache_ptr->refCount = 1;
    cache_ptr->pixmap = pixmap;
    
    return (True);
}


/************************************************************************
 *
 *  XmDestroyPixmap
 *	Locate a pixmap in the cache and decrement its reference count.
 *	When the reference count is at zero, free the pixmap.
 *
 ************************************************************************/
Boolean 
#ifdef _NO_PROTO
XmDestroyPixmap( screen, pixmap )
        Screen *screen ;
        Pixmap pixmap ;
#else
XmDestroyPixmap(
        Screen *screen,
        Pixmap pixmap )
#endif /* _NO_PROTO */
{
   register DtHashEntry  hashEntry;
   register DtPixmapCacheEntry *listPtr, *listHeadPtr;
   register DtPixmapCacheEntry pixmapData;

    if (imageCache == NULL)
      return False;

   hashEntry = (DtHashEntry)_XmEnumerateHashTable(imageCache,
						      GetPixmapEntry,
						      (XtPointer)pixmap);
   if (!hashEntry)
     return False;

   listHeadPtr = listPtr = GetPixmapListPtr(hashEntry);

   while (*listPtr && ((*listPtr)->pixmap != (Pixmap)pixmap))
     listPtr = &((*listPtr)->next);

   pixmapData = *listPtr;

   if ((--(pixmapData->refCount)) == 0) {
       *listPtr = (*listPtr)->next;
       if ((*listHeadPtr == NULL) &&
	   /* only destroy xternal in uninstall */
	   (hashEntry->hash.type != DtXTERNAL_IMAGE))
	 DESTROY_IMAGE_ENTRY(hashEntry);
       FreePixmapEntry(pixmapData);
   }
   return (True);
}


/************************************************************************
*
*  _XmInImageCache
*
************************************************************************/
Boolean
#ifdef _NO_PROTO
_XmInImageCache( image_name )
    char *image_name ;
#else
_XmInImageCache(
        char *image_name)
#endif /* _NO_PROTO */
{
    if ((imageCache == NULL) ||
	(!_XmKeyToHashEntry(imageCache, (DtHashKey)image_name)))
      return False;
    else
      return True;
}

/*****************************************************************************
 *****************************************************************************
 *
 * The next block of functions are used to create and descend the GC cache
 * hierarchy.
 *
 ****************************************************************************
 ***************************************************************************/

/*
 * This functions creates a new GC for a particular screen.
 */
static GC
#ifdef _NO_PROTO
CreateNewGC( displayIndex, screenIndex, pixmapEntry )
    int displayIndex;
    int screenIndex;
    DtPixmapCacheEntry pixmapEntry ;
#else
CreateNewGC(
    int displayIndex,
    int screenIndex,
    DtPixmapCacheEntry pixmapEntry )
#endif /* _NO_PROTO */
{
   PerScreenInfo * screenInfo;
   PerGCInfo * newGC;

   screenInfo = &(perDisplayInfo[displayIndex].screens[screenIndex]);
   screenInfo->numGCs++;
   screenInfo->gcs = (PerGCInfo *)XtRealloc((char *)screenInfo->gcs,
                        sizeof(PerGCInfo) * screenInfo->numGCs);
   newGC = &(screenInfo->gcs[screenInfo->numGCs - 1]);
   newGC->depth = pixmapEntry->depth;
   newGC->gcValues.foreground = pixmapEntry->foreground;
   newGC->gcValues.background = pixmapEntry->background;
   newGC->gc = XCreateGC(DisplayOfScreen(pixmapEntry->screen),
                         pixmapEntry->pixmap,
                         GCForeground|GCBackground,
                         &newGC->gcValues);
   return(newGC->gc);
}

/*
 * This functions creates a new screen entry, followed by a new GC for the 
 * screen.
 */
static GC
#ifdef _NO_PROTO
CreateNewScreen( displayIndex, pixmapEntry )
    int displayIndex;
    DtPixmapCacheEntry pixmapEntry ;
#else
CreateNewScreen(
    int displayIndex,
    DtPixmapCacheEntry pixmapEntry )
#endif /* _NO_PROTO */
{
   PerDisplayInfo * displayInfo;
   PerScreenInfo * newScreen;

   displayInfo = &(perDisplayInfo[displayIndex]);
   displayInfo->numScreens++;
   displayInfo->screens = (PerScreenInfo *)XtRealloc(
         (char *)displayInfo->screens, 
         sizeof(PerScreenInfo) * displayInfo->numScreens);
   newScreen = &(displayInfo->screens[displayInfo->numScreens - 1]);
   newScreen->screen = pixmapEntry->screen;
   newScreen->numGCs = 0;
   newScreen->gcs = NULL;
   return(CreateNewGC(displayIndex, 0, pixmapEntry));
}


/************************************************************************
 *
 * This function was designed to more efficiently work with applications
 * drawing to multiple displays at the same time.  It keeps a cache of
 * GCs, on a per-screen basis.  A given screen may have multiple GCs, if
 * the depth needs to be different.  A given display may have multiple
 * screens active at the same time also.
 *
 ************************************************************************/
static GC
#ifdef _NO_PROTO
GetGC( pixmapEntry )
    DtPixmapCacheEntry pixmapEntry ;
#else
GetGC(
    DtPixmapCacheEntry pixmapEntry )
#endif /* _NO_PROTO */
{
   int i, j, k;
   PerScreenInfo * screenInfo;
   PerDisplayInfo * newDisplay;
   Display * display = DisplayOfScreen(pixmapEntry->screen);
   Screen * screen = pixmapEntry->screen;

   /* First check the cache; add a new entry, if necessary */
   for (i = 0; i < numDisplayEntries; i++)
   {
      if (perDisplayInfo[i].display == display)
      {
          /*
           * The display already has at least one GC created for it; now,
           * check each of its screens, to see if one is created for the
           * screen we are interested it.
           */
          for (j = 0; j < perDisplayInfo[i].numScreens; j++)
          {
             if (perDisplayInfo[i].screens[j].screen == screen)
             {
                /*
                 * We found a matching screen for the display; see if one
                 * of its GCs matches our requirements.
                 */
                screenInfo = &(perDisplayInfo[i].screens[j]);
                for (k = 0; k < screenInfo->numGCs; k++)
                {
                   /* 
                    * If the depth matches, then re-use this GC, resetting
                    * the foreground and background colors, if necessary.
                    */
                   if (screenInfo->gcs[k].depth == pixmapEntry->depth)
                   {
                      if ((screenInfo->gcs[k].gcValues.foreground !=
                                         pixmapEntry->foreground) ||
                          (screenInfo->gcs[k].gcValues.background !=
                                         pixmapEntry->background))
                      {
                         /* Reuse this GC */
                         screenInfo->gcs[k].gcValues.foreground =
                                         pixmapEntry->foreground;
                         screenInfo->gcs[k].gcValues.background =
                                         pixmapEntry->background;
                         XChangeGC(display, screenInfo->gcs[k].gc,
                                   GCForeground|GCBackground,
                                   &screenInfo->gcs[k].gcValues);
                      }
                      return(screenInfo->gcs[k].gc);
                   }
                }

                /* This screen does not have a matching GC; create one */
                return(CreateNewGC(i, j, pixmapEntry));
             }
          }

          /* The display does not have a matching screen; create one */
          return(CreateNewScreen(i, pixmapEntry));
      }
   }

   /* There is not a display entry for the display we want; create one */
   XtAddCallback((Widget)XmGetXmDisplay(display), XmNdestroyCallback,
                 CleanupOnDisplayClose, NULL);
   numDisplayEntries++;
   perDisplayInfo = (PerDisplayInfo *)XtRealloc((char *)perDisplayInfo,
                       sizeof(PerDisplayInfo) * numDisplayEntries);
   newDisplay = perDisplayInfo + (numDisplayEntries - 1);
   newDisplay->display = display;
   newDisplay->numScreens = 0;
   newDisplay->screens = NULL;
   return(CreateNewScreen(numDisplayEntries - 1, pixmapEntry));
}


/****************************************************************************
 ****************************************************************************
 *
 * The following functions are used to clean up the pixmap cache and the
 * GC cache, whenever a display is closed; once a display has been closed,
 * all GC's and pixmaps created for that display become invalid.
 *
 ****************************************************************************
 **************************************************************************/

/*
 * This function will remove all GC's in the cache, which are associated
 * with the indicated display.
 */
static void
#ifdef _NO_PROTO
FlushDisplayGCs( display )
   Display * display;
#else
FlushDisplayGCs(
   Display * display)
#endif /* _NO_PROTO */
{
   int i, j, k;

   for (i = 0; i < numDisplayEntries; i++)
   {
      /* Find the matching display entry */
      if (perDisplayInfo[i].display == display)
      {
         /* Found a match; free up each GC, for all screens on this display */
         for (j = 0; j < perDisplayInfo[i].numScreens; j++)
         {
            for (k = 0; k < perDisplayInfo[i].screens[j].numGCs; k++)
               XFreeGC(display, perDisplayInfo[i].screens[j].gcs[k].gc);

            XtFree((char *)perDisplayInfo[i].screens[j].gcs);
         }
         XtFree((char *)perDisplayInfo[i].screens);

         /* Lastly, remove the display entry itself */
         for (j = i; j < (numDisplayEntries -1); j++)
            perDisplayInfo[j] = perDisplayInfo[j + 1];
         numDisplayEntries--;
         perDisplayInfo = (PerDisplayInfo *)XtRealloc((char *)perDisplayInfo,
                                sizeof(PerDisplayInfo) * numDisplayEntries);
         break;
      }
   }
}

/*
 * This function is called for each pixmap in the cache.  We will check to
 * see if the pixmap is associated with the display being closed (passed-in
 * as the 'clientData'), and if so, will remove the entry.
 */
static Boolean
#ifdef _NO_PROTO
RemoveMatchingEntries( entry, clientData, entryType )
   DtHashEntry entry;
   XtPointer clientData;
   int entryType;
#else
RemoveMatchingEntries(
   DtHashEntry entry,
   XtPointer clientData,
   int entryType)
#endif /* _NO_PROTO */
{
   DtPixmapCacheEntry * listPtr;
   DtPixmapCacheEntry * listHeadPtr;
   DtPixmapCacheEntry pixmapData;
   Display * display = (Display *)clientData;

   /*
    * The entry contains the image information; we need to then get
    * the list of pixmaps associated with this image.
    */
   if (listPtr = GetPixmapListPtr(entry))
   {
      listHeadPtr = listPtr;
      while(listPtr && *listPtr)
      {
         if ((DisplayOfScreen((*listPtr)->screen) == display) &&
             ((entryType == -1) || (entryType == entry->hash.type)))
         {
            pixmapData = *listPtr;
            *listPtr = (*listPtr)->next;
            if ((*listHeadPtr == NULL) &&
                (entry->hash.type != DtXTERNAL_IMAGE))
            {
               /* Destroy the image entry, if no more pixmaps are left */
               DESTROY_IMAGE_ENTRY(entry);
            }
            FreePixmapEntry(pixmapData);
         }
         else
            listPtr = &((*listPtr)->next);
      }
   }
   return(False);
}

static Boolean
#ifdef _NO_PROTO
RemoveDisplayPixmaps( entry, clientData )
   DtHashEntry entry;
   XtPointer clientData;
#else
RemoveDisplayPixmaps(
   DtHashEntry entry,
   XtPointer clientData)
#endif /* _NO_PROTO */
{
   RemoveMatchingEntries(entry, clientData, -1);
}

static Boolean
#ifdef _NO_PROTO
RemoveDisplayMasks( entry, clientData )
   DtHashEntry entry;
   XtPointer clientData;
#else
RemoveDisplayMasks(
   DtHashEntry entry,
   XtPointer clientData)
#endif /* _NO_PROTO */
{
   RemoveMatchingEntries(entry, clientData, DtMASK_IMAGE);
}

/*
 * This is the callback function attached as the 'destroy' callback for
 * the display widget; it is responsible for initiating the cleanup of
 * the display-based objects.
 */
static void
#ifdef _NO_PROTO
CleanupOnDisplayClose( displayWidget, callData, clientData )
   Widget displayWidget;
   XtPointer callData;
   XtPointer clientData;
#else
CleanupOnDisplayClose(
   Widget displayWidget,
   XtPointer callData,
   XtPointer clientData)
#endif /* _NO_PROTO */
{
   /* 
    * Remove any pixmaps based on this display.  NOTE: any pixmap masks
    * must be destroyed BEFORE there associated pixmap; that is why this
    * is done as a 2-pass process.
    */
   _XmEnumerateHashTable(imageCache, RemoveDisplayMasks, 
                          XtDisplay(displayWidget));
   _XmEnumerateHashTable(imageCache, RemoveDisplayPixmaps, 
                          XtDisplay(displayWidget));

   /* Cleanup the GC cache */
   FlushDisplayGCs(XtDisplay(displayWidget));
}




/*

Issues:


*/
