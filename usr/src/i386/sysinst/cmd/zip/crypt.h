#ident	"@(#)crypt.h	15.1"

/* crypt.h (dummy version) -- do not perform encrytion
 * Hardly worth copyrighting :-)
 */

#ifdef CRYPT
#  undef CRYPT      /* dummy version */
#endif

#define RAND_HEAD_LEN  12  /* length of encryption random header */

#define zencode
#define zdecode
