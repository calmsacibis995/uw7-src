#ident	"@(#)sgs-inc:common/dem.h	1.1"

#ifdef __STDC__
#ifdef __cplusplus
extern "C"
#endif
int demangle(const char*, char*, size_t);
#else
int demangle();
#endif

