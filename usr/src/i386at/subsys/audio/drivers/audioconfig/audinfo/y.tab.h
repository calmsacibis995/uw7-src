/*
 *	@(#)y.tab.h	7.1	10/22/97	12:21:53
 */

typedef union
#ifdef __cplusplus
	YYSTYPE
#endif
 {
	int ival;
	char *str;
	intlist *ilist;
	resource *res;
	node *node;
	load_defs *ld;
} YYSTYPE;
extern YYSTYPE yylval;
# define IDENTIFIER 257
# define HEXADECIMAL 258
# define DECIMAL 259
# define STR 260
# define NAME 261
# define DEVICE 262
# define OPTION 263
# define SINGLE 264
# define PORT 265
# define LENGTH 266
# define CHOICE 267
# define DEFAULT 268
# define FIXED 269
# define PNP 270
# define PSEUDOPNP 271
# define NOSINGLE 272
# define STATIC 273
# define NOSTATIC 274
# define TOUCHONLY 275
# define CONTROL 276
# define IRQ 277
# define DMA 278
# define OPTIONAL 279
# define NOFIXED 280
# define NOOPTIONAL 281
# define RANGE 282
# define SHADOW 283
# define REQUIRES 284
# define DETECT 285
# define CONFLICT 286
# define ARCH 287
# define CARD 288
# define LIKE 289
# define INCLUDE 290
# define MENU 291
# define TYPE 292
# define ALIAS 293
# define DOWNLOAD 294
# define BIN 295
# define HEX 296
# define ALIGN 297
# define BUS 298
# define ISA 299
# define ISAX 300
# define MCA 301
# define PCI 302
# define TEXT 303
