#ident	"@(#)ihvkit:display/include/xyz.h	1.1"

/*copyright	"%c%"*/

#ifndef _XYZ_H_
#define _XYZ_H_

#if defined(XYZEXT)

struct XYZ_marker {
   void *tag;
   struct XYZ_marker *next;
};

extern void XamineYourZerver(
# if NeedFunctionPrototypes
  char *tagname,
  int delta
  struct XYZ_marker *marker;
# endif
);

#define XYZ(tagname) { \
   static struct XYZ_marker marker = { 0, 0 }; \
   XamineYourZerver(tagname, 1, &marker); \
}
#define XYZdelta(tagname, delta)  { \
   static struct XYZ_marker marker = { 0, 0 }; \
   XamineYourZerver(tagname, delta, &marker); \
}

#else /* not defined(XYZEXT) */

#define XYZ(tagname) { /* nothing */ }
#define XYZdelta(tagname, delta) { /* nothing */ }

#endif

#endif /* _XYZ_H_ */

