#ident	"@(#)optim:i386/regalutil.h	1.2"
#define BY_OFF(s,REG)  (s && (scanreg(s,false) & REG))
typedef struct auto_reg
{
	char reglname[NEWSIZE];   /* full operand that contains regal */
  	int   offset;            /* offset of regal in the stack */
	struct auto_reg *reg_hash_next;   /* next outo reg in hash chain */
	unsigned bits;      /* the live/dead register bit */
	int size;           /* size of the quantity in bytes. */
	boolean   valid;    /* we save regals as valid and aliases as invalid */
	boolean partof;     /* is this a high part of a 64 bit regal */
	int param2auto;     /* if it was a double param, it's location as an auto */
	int estim;          /* for double params, payoff estimation to move them */
} AUTO_REG;

extern int fflag;
extern suppress_enter_leave;
extern boolean live_at();
extern AUTO_REG *getregal();
extern unsigned int get_regal_bits();
