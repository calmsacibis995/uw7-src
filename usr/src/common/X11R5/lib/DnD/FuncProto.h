#ifndef NOIDENT
#ident	"@(#)oldnd:FuncProto.h	1.1"
#endif

#ifndef _Ol_FuncProto_h_
#define _Ol_FuncProto_h_

/*
 * If the definition of "__Ol_OpenLook_h__" is not present,
 * then define the "prototype" marcos.
 *
 */
#ifndef __Ol_OpenLook_h__

#if !defined(__STDC__) && !defined(__cplusplus) && !defined(c_plusplus)
#define OLconst
#else
#define OLconst const
#endif


#if !defined(OlNeedFunctionPrototypes) || (OlNeedFunctionPrototypes != 0)

#ifdef OlNeedFunctionPrototypes
#undef OlNeedFunctionPrototypes
#endif /* OlNeedFunctionPrototypes */

#if defined(__cplusplus) || defined(c_plusplus)

#define OlNeedFunctionPrototypes 1
#define OL_ARGS(x)	x
#define OL_NO_ARGS()	()

#else /* defined(__cpluslus) || defined(c_plusplus) */

#if defined(__STDC__)

#define OlNeedFunctionPrototypes 1
#define OL_ARGS(x)	x
#define OL_NO_ARGS()	(void)

#else /* defined(__STDC__) */

#undef OlNeedFunctionPrototypes
#define OL_ARGS(x)	()
#define OL_NO_ARGS()	()

#endif /* defined(__STDC__) */
#endif /* defined(__cpluslus) || defined(c_plusplus) */
#endif /*!defined(OlNeedFunctionPrototypes)||(OlNeedFunctionPrototypes != 0)*/

#if OlNeedFunctionPrototypes

#define OLARGLIST(list)	(
#define OLARG(t,a)	t a,
#define OLGRA(t,a)	t a)
#define OLVARGLIST	...			/* junk parameter       */
#define OLVARGS		...)

#if defined(__cplusplus) || defined(c_plusplus)

#define OLBeginFunctionPrototypeBlock	extern "C" {
#define OLEndFunctionPrototypeBlock	}

#else /* defined(__cplusplus) || defined(c_plusplus) */

#define OLBeginFunctionPrototypeBlock
#define OLEndFunctionPrototypeBlock

#endif /* defined(__cplusplus) || defined(c_plusplus) */

#else /* OlNeedFunctionPrototypes */

#define OLARGLIST(list)	list
#define OLARG(t,a)	t a;
#define OLGRA(t,a)	t a;
#define OLVARGLIST	va_alist
#define OLVARGS		va_dcl

#define OLBeginFunctionPrototypeBlock
#define OLEndFunctionPrototypeBlock

#endif /* OlNeedFunctionPrototypes */

#endif	/* __Ol_OpenLook_h__ */

#endif	/* _Ol_FuncProto_h_ */
