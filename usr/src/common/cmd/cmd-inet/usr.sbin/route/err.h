#ident	"@(#)err.h	1.2"
#ident	"$Header$"

void    err (int, const char *, ...);
void    verr (int, const char *, va_list);
void    errx (int, const char *, ...);
void    verrx (int, const char *, va_list);
void    warn (const char *, ...);
void    vwarn (const char *, va_list);
void    warnx (const char *, ...);
void    vwarnx (const char *, va_list);
