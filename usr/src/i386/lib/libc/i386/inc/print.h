#ident	"@(#)libc-i386:inc/print.h	1.12"

#if i386
#define LONG_DOUBLE_SUPPORT 1
#endif

/* Maximum number of digits in any integer representation */
#define MAXDIGS 11

/* Maximum total number of digits in E format */
#if M32
#define MAXECVT 17
#elif LONG_DOUBLE_SUPPORT
#define MAXECVT 23	/* supports long double */
#else
#define MAXECVT 18
#endif

/* Maximum number of digits after decimal point in F format */
#define MAXFCVT 60

/* Maximum significant figures in a floating-point number */
#define MAXFSIG MAXECVT

/* Maximum number of characters in an exponent */
#if M32
#define MAXESIZ 5
#elif LONG_DOUBLE_SUPPORT
#define MAXESIZ 7	/* supports long double */
#else
#define MAXESIZ 4
#endif

/* Maximum (positive) exponent */
#if M32 
#define MAXEXP 310
#elif LONG_DOUBLE_SUPPORT
#define MAXEXP 4934	/* supports long double */
#else
#define MAXEXP 40
#endif

/* Data type for flags */
typedef char bool;

/* Convert a digit character to the corresponding number */
#define tonumber(x) ((x)-'0')

/* Convert a number between 0 and 9 to the corresponding digit */
#define todigit(x) ((x)+'0')

/* Max and Min macros */
#define max(a,b) ((a) > (b)? (a): (b))
#define min(a,b) ((a) < (b)? (a): (b))
