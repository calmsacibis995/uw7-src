#ident	"@(#)tokdefs.h	1.2"
#ident	"$Header$"

typedef union
#ifdef __cplusplus
	YYSTYPE
#endif
 {
	int i;
	u_long h;
	u_char *e;
	char *s;
	struct stmt *stmt;
	struct arth *a;
	struct {
		struct qual q;
		struct block *b;
	} blk;
	struct block *rblk;
} YYSTYPE;
extern YYSTYPE yylval;
# define DST 257
# define SRC 258
# define HOST 259
# define NET 260
# define PORT 261
# define LESS 262
# define GREATER 263
# define PROTO 264
# define BYTE 265
# define IP 266
# define TCP 267
# define UDP 268
# define ICMP 269
# define TK_BROADCAST 270
# define TK_MULTICAST 271
# define NUM 272
# define GEQ 273
# define LEQ 274
# define NEQ 275
# define ID 276
# define HID 277
# define LSH 278
# define RSH 279
# define LEN 280
# define OR 281
# define AND 282
# define UMINUS 283
