#ident	"@(#)md5.h	1.2"
#ident	"$Header$"

#ifndef _NET_MD5_MD5_H	/* wrapper symbol for kernel use */
#define _NET_MD5_MD5_H	/* subject to change without notice */

#ident	"@(#)md5.h	1.2"

#ifdef _KERNEL_HEADERS

#include <util/types.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>

#endif /* _KERNEL_HEADERS */

#define MD5_DIGEST_LEN  16

typedef struct MD5_CTX {
  ulong_t state[4];
  ulong_t count[2];
  unsigned char buffer[64];
} MD5_CTX;

void MD5Init(MD5_CTX *ctx);
void MD5Update(MD5_CTX *ctx, unsigned char const *buf, unsigned len);
void MD5Final(unsigned char digest[16], MD5_CTX *ctx);

#endif	/* _NET_MD5_MD5_H */
