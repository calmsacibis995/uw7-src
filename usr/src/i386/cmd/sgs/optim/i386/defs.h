#ident	"@(#)optim:i386/defs.h	1.1.2.33"

#ifndef __STDC__
#define const
#endif

#ifndef DEFS_H
#define DEFS_H


/*      machine dependent include file for the Intel 386 */

# include <ctype.h>
# include <string.h>

/* Target cpu's */

#define P3 0x01
#define P4 0x02
#define blend 0x04
#define P5 0x08
#define P6 0x010

extern int target_cpu;
extern int eiger;
extern int nflag;
/* for fixed frame code */
extern int fixed_stack;
extern int set_FSZ;

/* For now, turn off everything */
/* #define IMPCOMTAIL */
#define SW_PIPE
#ifndef EH_SUP
#define EH_SUP 7
#endif

/* Intel 386 opcodes */

#define LABEL	3
#define HLABEL	4
#define DHLABEL	5	/* hard label created for .def .line */
#define ASMS	6
#define LCMT	7
#define TSRET	8	/* used by optim on parse_com for TMPSRET */
#if EH_SUP
#define EHLABEL	9	/* mark places for EH, not realy a label  */
#endif

#define	CALL       21
#define	CALL_0     22
#define	CALL_1     23
#define	CALL_2     24

#define	LCALL      25
#define	LCALL_0    26
#define	LCALL_1    27
#define	LCALL_2    28

#define	RET        29
#define	LRET       30
#define	JMP        31
#define	LJMP       32
#define	JA         33
#define	JAE        34
#define	JB         35
#define	JBE        36
#define	JC         37
#define	JCXZ       38
#define	JE         39
#define	JG         40
#define	JGE        41
#define	JL         42
#define	JLE        43
#define	JNA        44
#define	JNAE       45
#define	JNB        46
#define	JNBE       47
#define	JNC        48
#define	JNE        49
#define	JNG        50
#define	JNGE       51
#define	JNL        52
#define	JNLE       53
#define	JNO        54
#define	JNP        55
#define	JNS        56
#define	JNZ        57
#define	JO         58
#define	JP         59
#define	JPE        60
#define	JPO        61
#define	JS         62
#define	JZ         63
#define	LOOP       64
#define	LOOPE      65
#define	LOOPNE     66
#define	LOOPNZ     67
#define	LOOPZ      68
#define	REP        69
#define	REPNZ      70
#define	REPZ       71
#define	AAA        72
#define	AAD        73
#define	AAM        74
#define	AAS        75
#define	DAA        76
#define	DAS        77
#define	ADCB       78
#define	ADCW       79
#define	ADCL       80
#define	ADDB       81
#define	ADDW       82
#define	ADDL       83
#define	DECB       84
#define	DECW       85
#define	DECL       86
#define	DIVB       87
#define	DIVW       88
#define	DIVL       89
#define	IDIVB      90
#define	IDIVW      91
#define	IDIVL      92
#define	IMULB      93
#define	IMULW      94
#define	IMULL      95
#define	INCB       96
#define	INCW       97
#define	INCL       98
#define	MULB       99
#define	MULW      100
#define	MULL      101
#define	NEGB      102
#define	NEGW      103
#define	NEGL      104
#define	SBBB      105
#define	SBBW      106
#define	SBBL      107
#define	SUBB      108
#define	SUBW      109
#define	SUBL      110
#define	ANDB      111
#define	ANDW      112
#define	ANDL      113
#define	ORB       114
#define	ORW       115
#define	ORL       116
#define	XORB      117
#define	XORW      118
#define	XORL      119
#define	CLRB      120
#define	CLRW      121
#define	CLRL      122
#define	RCLB      123
#define	RCLW      124
#define	RCLL      125
#define	RCRB      126
#define	RCRW      127
#define	RCRL      128
#define	ROLB      129
#define	ROLW      130
#define	ROLL      131
#define	RORB      132
#define	RORW      133
#define	RORL      134
#define	SALB      135
#define	SALW      136
#define	SALL      137
#define	SARB      138
#define	SARW      139
#define	SARL      140
#define	SHLB      141
#define	SHLW      142
#define	SHLL      143
#define	SHRB      144
#define	SHRW      145
#define	SHRL      146
#define	SHLDW     147
#define	SHLDL     148
#define	SHRDW     149
#define	SHRDL     150
#define	CMPB      151
#define	CMPW      152
#define	CMPL      153
#define	TESTB     154
#define	TESTW     155
#define	TESTL     156
#define	CBTW      157
#define	CWTL      158
#define	CWTD      159
#define	CLTD      160
#define	LDS       161
#define	LEAW      162
#define	LEAL      163
#define	LES       164
#define	MOVB      165
#define	MOVW      166
#define	MOVL      167
#define	MOVSBW    168
#define	MOVSBL    169
#define	MOVSWL    170
#define	MOVZBW    171
#define	MOVZBL    172
#define	MOVZWL    173
#define	NOTB      174
#define	NOTW      175
#define	NOTL      176
#define	POPW      177
#define	POPL      178
#define	PUSHW     179
#define	PUSHL     180
#define	XCHGB     181
#define	XCHGW     182
#define	XCHGL     183
#define	XLAT      184
#define	CLC       185
#define	CLD       186
#define	CLI       187
#define	CMC       188
#define	LAHF      189
#define	POPF      190
#define	PUSHF     191
#define	SAHF      192
#define	STC       193
#define	STD       194
#define	STI       195
#define	SCAB      196
#define	SCAW      197
#define	SCAL      198
#define	SCMPB     199
#define	SCMPW     200
#define	SCMPL     201
#define	SLODB     202
#define	SLODW     203
#define	SLODL     204
#define	SMOVB     205
#define	SMOVW     206
#define	SMOVL     207
#define	SSTOB     208
#define	SSTOW     209
#define	SSTOL     210
#define	INB       211
#define	INW       212
#define	INL       213
#define	OUTB      214
#define	OUTW      215
#define	OUTL      216
#define	ESC       217
#define	HLT       218
#define	INT       219
#define	INTO      220
#define	IRET      221
#define	LOCK      222
#define	WAIT      223
#define	ENTER     224
#define	LEAVE     225
#define	PUSHA     226
#define	POPA      227
#define	INS       228
#define	OUTS      229
#define	BOUND     230
#define	CTS       231
#define	LGDT      232
#define	SGDT      233
#define	LIDT      234
#define	SIDT      235
#define	LLDT      236
#define	SLDT      237
#define	LTR       238
#define	STR       239
#define	LMSW      240
#define	SMSW      241
#define	LAR       242
#define	LSL       243
#define	ARPL      244
#define	VERR      245

#define	BOUNDL    246
#define	BOUNDW    247
#define	BSFL      248
#define	BSFW      249
#define	BSRL      250
#define	BSRW      251
#define	BSWAP     252
#define	BTCL      253
#define	BTCW      254
#define	BTL       255
#define	BTRL      256
#define	BTRW      257
#define	BTSL      258
#define	BTSW      259
#define	BTW       260
#define	CLTS      261
#define	CMPSB     262
#define	CMPSL     263
#define	CMPSW     264
#define	CMPXCHGB  265
#define	CMPXCHGL  266
#define	CMPXCHGW  267
#define	INSB      268
#define	INSL      269
#define	INSW      270
#define	INVD      271
#define	INVLPG    272
#define	LARL      273
#define	LARW      274
#define	LDSL      275
#define	LDSW      276
#define	LESL      277
#define	LESW      278
#define	LFSL      279
#define	LFSW      280
#define	LGSL      281
#define	LGSW      282
#define	LODSB     283
#define	LODSL     284
#define	LODSW     285
#define	LSLL      286
#define	LSLW      287
#define	LSSL      288
#define	LSSW      289
#define	MOVSL     290
#define	NOP       291
#define	OUTSB     292
#define	OUTSL     293
#define	OUTSW     294
#define	POPAL     295
#define	POPAW     296
#define	POPFL     297
#define	POPFW     298
#define	PUSHAL    299
#define	PUSHAW    300
#define	PUSHFL    301
#define	PUSHFW    302
#define	REPE      303
#define	REPNE     304
#define	SCASB     305
#define	SCASL     306
#define	SCASW     307
#define	SETA      308
#define	SETAE     309
#define	SETB      310
#define	SETBE     311
#define	SETC      312
#define	SETE      313
#define	SETG      314
#define	SETGE     315
#define	SETL      316
#define	SETLE     317
#define	SETNA     318
#define	SETNAE    319
#define	SETNB     320
#define	SETNBE    321
#define	SETNC     322
#define	SETNE     323
#define	SETNG     324
#define	SETNL     325
#define	SETNLE    326
#define	SETNO     327
#define	SETNP     328
#define	SETNS     329
#define	SETNZ     330
#define	SETO      331
#define	SETP      332
#define	SETPE     333
#define	SETPO     334
#define	SETS      335
#define	SETZ      336
#define	SSCAB     337
#define	SSCAL     338
#define	SSCAW     339
#define	STOSB     340
#define	STOSL     341
#define	STOSW     342
#define	VERW      343
#define	WBINVD    344
#define	XADDB     345
#define	XADDL     346
#define	XADDW     347

#define	F2XM1     348
#define	FABS      349
#define	FCHS      350
#define	FCLEX     351
#define	FCOMPP    352
#define	FDECSTP   353
#define	FINCSTP   354
#define	FINIT     355
#define	FLD1      356
#define	FLDL2E    357
#define	FLDL2T    358
#define	FLDLG2    359
#define	FLDLN2    360
#define	FLDPI     361
#define	FLDZ      362
#define	FNCLEX    363
#define	FNINIT    364
#define	FNOP      365
#define	FPATAN    366
#define	FPREM     367
#define	FPTAN     368
#define	FRNDINT   369
#define	FSCALE    370
#define	FSETPM    371
#define	FSQRT     372
#define	FTST      373
#define	FWAIT     374
#define	FXAM      375
#define	FXTRACT   376
#define	FYL2X     377
#define	FYL2XP1   378
#define	FLDCW     379
#define	FSTCW     380
#define	FNSTCW    381
#define	FSTSW     382
#define	FNSTSW    383
#define	FSTENV    384
#define	FNSTENV   385
#define	FLDENV    386
#define	FSAVE     387
#define	FNSAVE    388
#define	FRSTOR    389
#define	FBLD      390
#define	FBSTP     391
#define	FIADD     392
#define	FIADDL    393
#define	FICOM     394
#define	FICOML    395
#define	FICOMP    396
#define	FICOMPL   397
#define	FIDIV     398
#define	FIDIVL    399
#define	FIDIVR    400
#define	FIDIVRL   401
#define	FILD      402
#define	FILDL     403
#define	FILDLL    404
#define	FIMUL     405
#define	FIMULL    406
#define	FIST      407
#define	FISTL     408
#define	FISTP     409
#define	FISTPL    410
#define	FISTPLL   411
#define	FISUB     412
#define	FISUBL    413
#define	FISUBR    414
#define	FISUBRL   415
#define	FADD      416
#define	FADDS     417
#define	FADDL     418
#define	FADDP     419
#define	FCOM      420
#define	FCOMS     421
#define	FCOML     422
#define	FCOMP     423
#define	FCOMPS    424
#define	FCOMPL    425
#define	FDIV      426
#define	FDIVS     427
#define	FDIVL     428
#define	FDIVP     429
#define	FDIVR     430
#define	FDIVRS    431
#define	FDIVRL    432
#define	FDIVRP    433
#define	FFREE     434
#define	FLD       435
#define	FLDS      436
#define	FLDL      437
#define	FLDT      438
#define	FMUL      439
#define	FMULS     440
#define	FMULL     441
#define	FMULP     442
#define	FST       443
#define	FSTS      444
#define	FSTL      445
#define	FSTP      446
#define	FSTPS     447
#define	FSTPL     448
#define	FSTPT     449
#define	FSUB      450
#define	FSUBS     451
#define	FSUBL     452
#define	FSUBP     453
#define	FSUBR     454
#define	FSUBRS    455
#define	FSUBRL    456
#define	FSUBRP    457
#define	FXCH      458
#define	FCOS      459
#define	FPREM1    460
#define	FSIN      461
#define	FSINCOS   462
#define	FUCOM     463
#define	FUCOMP    464
#define	FUCOMPP   465
#define	FCMOVB    466
#define	FCMOVE    467
#define	FCMOVBE   468
#define	FCMOVU    469
#define	FCMOVNB   470
#define	FCMOVNE   471
#define	FCMOVNBE  472
#define	FCMOVNU   473

#define	CMOVBL    474
#define	CMOVBW    475
#define	CMOVNBL   476
#define	CMOVNBW   477
#define	CMOVCL    478
#define	CMOVCW    479
#define	CMOVNCL   480
#define	CMOVNCW   481
#define	CMOVNAEL  482
#define	CMOVNAEW  483
#define	CMOVAEL   484
#define	CMOVAEW   485
#define	CMOVEL    486
#define	CMOVEW    487
#define	CMOVNEL   488
#define	CMOVNEW   489
#define	CMOVZL    490
#define	CMOVZW    491
#define	CMOVNZL   492
#define	CMOVNZW   493
#define	CMOVAL    494
#define	CMOVAW    495
#define	CMOVNAL   496
#define	CMOVNAW   497
#define	CMOVBEL   498
#define	CMOVBEW   499
#define	CMOVNBEL  500
#define	CMOVNBEW  501
#define	CMOVSL    502
#define	CMOVSW    503
#define	CMOVNSL   504
#define	CMOVNSW   505
#define	CMOVPL    506
#define	CMOVPW    507
#define	CMOVNPL   508
#define	CMOVNPW   509
#define	CMOVPEL   510
#define	CMOVPEW   511
#define	CMOVPOL   512
#define	CMOVPOW   513
#define	CMOVLL    514
#define	CMOVLW    515
#define	CMOVNLL   516
#define	CMOVNLW   517
#define	CMOVGEL   518
#define	CMOVGEW   519
#define	CMOVNGEL  520
#define	CMOVNGEW  521
#define	CMOVGL    522
#define	CMOVGW    523
#define	CMOVNGL   524
#define	CMOVNGW   525
#define	CMOVLEL   526
#define	CMOVLEW   527
#define	CMOVNLEL  528
#define	CMOVNLEW  529
#define	CMOVOL    530
#define	CMOVOW    531
#define	CMOVNOL   532
#define	CMOVNOW   533

#define	MACRO     534
#define	OTHER     535

#define SAFE_ASM    600 /* SAFE_ASM must be greater than any other opcode */

#define is_safe_asm(p) ((p->op >= SAFE_ASM) || (p->sasm == SAFE_ASM))

#ifdef FLIRT
/* optimizations */

enum O_OPTS {
O_BBOPT1  = 1,
O_BBOPT2 ,
O_CLEAN_ZERO  ,
O_COMTAIL ,
O_CONST_INDEX,
O_DETESTL ,
O_FP_LOOP ,
O_FPEEP   ,
O_FPEEP01 ,
O_FPEEP02 ,
O_FPEEP03 ,
O_FPEEP04 ,
O_FPEEP05 ,
O_FPEEP06 ,
O_FPEEP07 ,
O_FPEEP08 ,
O_FPEEP09 ,
O_FPEEP10 ,
O_FPEEP11 ,
O_FPEEP12 ,
O_IMM_2_REG ,
O_IMULL      ,
O_INSERT_MOVES,
O_LOOP_DECMPL,
O_LOOP_DESCALE ,
O_MRGBRS  ,
O_OPTIM   ,
O_PEEP    ,
O_POSTPEEP ,
O_RAOPTIM  ,
O_REGALS_2_INDEX_OPS      ,
O_REMOVE_OVERLAPPING_REGALS,
O_REORD    ,
O_REORDTOP ,
O_REPLACE_CONSTS  ,
O_REPLACE_REGALS_W_REGS,
O_RM_ALL_TVRS1 ,
O_RM_ALL_TVRS2 ,
O_RM_ALL_TVRS3 ,
O_RM_ALL_TVRS4 ,
O_RM_DEAD_INSTS ,
O_RM_MVMEM	,
O_RM_MVXY	,
O_RM_RD_CONST_LOAD,
O_RM_TMPVARS1  ,
O_RM_TMPVARS2   ,
O_RMBRS ,
O_RMLBLS ,
O_RMRDLD  ,
O_RMRDMV   ,
O_RMRDPP    ,
O_RMUNRCH    ,
O_SCHEDULE    ,
O_SETAUTO_REG  ,
O_SETCOND       ,
O_STACK_CLEANUP  ,
O_SW_PIPELINE     ,
O_TRY_AGAIN_W_RT_DISAMB,
O_TRY_BACKWARD  ,
O_TRY_FORWARD    ,
O_XCHG_SAVE_LOCALS,
O_ZVTR    ,

OTHER_OPTIM };
#endif

/* pseudo ops */

enum psops { /* arranged alphabetically by their actual spellings */
	TWOBYTE,	/* .2byte */
	FOURBYTE,	/* .4byte */
	EIGHTBYTE,	/* .8byte */
	ALIGN,
	ASCII,
	BCD,
	BSS,
	BYTE,
	COMM,
	DATA,
	DOUBLE,
	EVEN,
	EXT,
	FIL,
	FLOAT,
	GLOBL,
	IDENT,
	LCOMM,
	LOCAL,
	LONG,
	PREVIOUS,
	SECTION,
	SET,
	SIZE,
	STRING,
	TEXT,
	TYPE,
	VALUE,
	VERSION,
	WEAK,
	WORD,
	ZERO,
	POTHER /* gives required dimension of string table */
};

# define CC '/' /* begin comment character */

# define ASMEND	"/ASMEND"

/* Control sections */

enum Section {CSbss,CSdata,CSdebug,CSline,CStext,CSrodata,CSdata1,CSother} ;

/* predicates and functions */

#define NEWSIZE	(11+1+4+1+4+1+1+1+1)	/* for "ddddddddddd(%exx,%exx,8)\0" */
#define ADDLSIZE (1+10+1)		/* for "$2147483647\0" */
#define LABELSIZE	13			/* for ".L2147483637\0" */
#define FRSIZE  ( ADDLSIZE+15+ADDLSIZE+NEWSIZE) /* for "-number+..fr_sz(%esp)\0" */
#define LEALSIZE (10+1+4+1+4+1+1)		/* for "2147483647(%esp,%exx)\0" */


# define islabel(p) \
	(p != NULL && (p->op == LABEL || p->op == HLABEL || p->op == DHLABEL))
# define ishl(p) (p->opcode[0] != '.' || (p->opcode[0] == '.' && p->opcode[1] == '.' ) || p->op == HLABEL || p->op == DHLABEL)
#define is_hard_label(s) (s[0] != '.' || (s[0] == '.' && s[1] == '.' ))
# define is_debug_label(p) (p->opcode[0] == '.' && p->opcode[1] == '.' )
# define is_label_text(s) (s[0] ==  '.' && (s[1] == '.' || isalpha(s[1])))
# define isuncbr(p) (p->op >= RET && p->op <= LJMP)
# define iscbr(p) (p->op >= JA && p->op <= JZ)
# define isbr(p) (p->op >= RET && p->op <= LOOPZ)
# define ishb(p) (p->op == RET || p->op == LRET || p->op == LJMP)
#define FindWhite(p)    while(!isspace(*p) && *(p) != '\0') p++;
#define SkipWhite(p)    while(isspace(*p)) p++;
#define strlength(p)	(strlen(p) + 1)	/* length of string including '\0' */
#define new_sets_uses(p)	{ p->uses = uses(p); p->sets = sets(p); }

/* predicates for safe asm ops */

#define sa_islabel(p) \
  	(p && (((int) p->op == LABEL + SAFE_ASM) \
  		|| ((int) p->op == HLABEL + SAFE_ASM) \
  		|| ((int) p->op == DHLABEL + SAFE_ASM)))

#define sa_isuncbr(p) ((int) p->op >= RET + SAFE_ASM \
  	&& (int) p->op <= LJMP + SAFE_ASM)
#define sa_iscbr(p) ((int) p->op >= JA + SAFE_ASM \
  	&& (int) p->op <= JZ + SAFE_ASM)
#define sa_isbr(p) (((int) p->op >= (RET +SAFE_ASM)) \
  	&& ((int) p->op <= (LOOPZ + SAFE_ASM)))

#define is_any_label(p)	(islabel(p) || sa_islabel(p))
#define is_any_uncbr(p)	(isuncbr(p) || sa_isuncbr(p))
#define is_any_cbr(p)	(iscbr(p) || sa_iscbr(p))
#define is_any_br(p)	(isbr(p) || sa_isbr(p))

#define is_fld(p)	(p->op == FLD || p->op == FLDS || p->op == FLDL || \
					 p->op == FLDT)

#define is_fstnst(p)	(p->op == FST || p->op == FSTS || p->op == FSTL)

#define is_fstp(p) (p->op == FSTP || p->op == FSTPS || p->op == FSTPL|| \
					 p->op == FSTPT)

#define is_fst(p) (is_fstnst(p) || is_fstp(p))

/*
 * The second test in the isrev is extra checking so that
 * jump indirects do not get converted to jCC indirects which
 * are illegal on the 386.
 */
# define isrev(p) (p->op >= JA && p->op <= JZ && \
		   !(p->forw != NULL && \
		     p->forw->op == JMP && \
		     p->forw->op1[0] == '*') \
		  )

#define iscall(p)   (p->op == CALL || p->op == LCALL)
# define isret(p) (p->op == RET || p->op == LRET)
#define isxchg(p) (p->op == XCHGL || p->op ==  XCHGW || p->op == XCHGB)
# define iscompare(p) (p->op == CMPL || p->op == CMPB || p->op == CMPW)
# define setlab(p) (p->op  = LABEL)
# define setbr(p,l) {(p)->op = JMP; (p)->opcode = "jmp"; \
	(p)->op1 = (l);}
# define bboptim(f,l) 0
# define mvlivecc(p) (p->back->nlive = (p->back->nlive & ~CONCODES_AND_CARRY) \
                                         | (p->nlive & CONCODES_AND_CARRY))
# define swplivecc(p,q) { int x; x=(p->nlive & CONCODES_AND_CARRY); mvlivecc(q); q->nlive = (q->nlive & ~CONCODES_AND_CARRY) | x; }

/* maximum number of operands */

# define MAXOPS 4

/* The live/dead analysis information */


/* live dead bits for physical registers. For each of %eax, %ebx, %ecx, %edx 
   there are 3 separate live-dead bits: Consider %eax, the 3 live-dead bits
   correspond to the following names:
	1. %ah
	2. %al
	3. %eax or %ax
*/

/* temps */
#define	Eax	0x00000001
#define	Edx	0x00000002
#define	Ecx	0x00000004

#define	FP0	0x00000008
#define	FP1	0x00000010

/* register variables */
#define	Ebx		0x00000020
#define	Esi		0x00000040
#define	Edi		0x00000080

#define FP2		0x00000100
#define FP3		FP2
#define FP4		FP2
#define FP5		FP2
#define FP6		FP2
#define FP7		FP2

#define Ebi		0x00000200
#define BI		0x00000400
#define EBI		(Ebi|BI)

#define Ebp     0x00000800
#define BP      0x00001000
#define EBP     (Ebp | BP)

#define	ESP		0x00002000
/* condition codes */
#define CARRY_FLAG	0x00008000
#define CONCODES 	0x00010000		/* excluding CARRY_FLAG */
#define CONCODES_AND_CARRY (CONCODES | CARRY_FLAG)

/* separate live-dead bits for same physical register */
#define	AH		0x00020000
#define	AL		0x00040000
#define	BH		0x00080000
#define	BL		0x00100000
#define	CH		0x00200000
#define	CL		0x00400000
#define	DH		0x00800000
#define	DL		0x01000000

#define Ax		0x02000000   /*16 bit registers*/
#define Dx		0x04000000
#define Bx		0x08000000
#define Cx		0x10000000
#define SI		0x20000000
#define DI		0x40000000

#define MEM		(unsigned) 0x80000000

/* everything */
#define	REGS		0x7FFFFFFF

#define ESP_STR "%esp"
#define EBP_STR "%ebp"
#define SAVE 1
#define RESTORE 2

/* references to EAX or AX references all 3 live-dead bits, similary for EBX
   ECX and EDX
*/
#define AX	(Ax|AH|AL)
#define BX	(Bx|BH|BL)
#define CX	(Cx|CH|CL)
#define DX	(Dx|DH|DL)
#define EAX	(Eax|Ax|AH|AL)
#define EBX	(Ebx|Bx|BH|BL)
#define ECX	(Ecx|Cx|CH|CL)
#define EDX	(Edx|Dx|DH|DL)
#define ESI (Esi|SI)
#define EDI (Edi|DI)
#define R16MSB (Eax|Ebx|Ecx|Edx|Esi|Edi|Ebi|Ebp)
#define R24MSB (R16MSB|Ax|Bx|Cx|Dx|AH|BH|CH|DH)
#define L2W(r) (r & ~R16MSB) /* Convert 4 to 2 byte reg */
#define L2B(r) (r & ~R24MSB) /* Convert 4 to 1 */
#define L2H(r) (r & (AH | BH | CH | DH)) /* Convert 4 to high  */
/* maximum return registers */
#define	MXRETREG	0x0000001F

/* always live registers */
#define	LIVEREGS	(pic_flag?(EBX|ESP):(ESP))
#define SAVEDREGS	(EBX|ESI|EDI|EBI|EBP)

# define isdeadcc(p) ((p->nlive & CONCODES_AND_CARRY) == 0)
# define isdeadcarry(p) ((p->nlive & CARRY_FLAG) == 0)
# define isdeadcconly(p) ((p->nlive & CONCODES) == 0)

/* integer size for various assumptions in IMPREGAL, and IMPIL */
#define INTSIZE 4

#define RETREG		0x0000001F

/* options */

# define MEMFCN
# define COMTAIL
# define PEEPHOLE

/* line number stuff */

# define IDTYPE int
# define IDVAL 0

#define spflg(i) ( (i) == 'K' || (i) == 'X' || (i) == 'y' || \
   (i) == 'Q' || (i) == '_' || (i) == '3' || (i) == '4' || (i) == 'p' \
   || (i) == 'n')
			/* indicate flags with suboptions
			   y: y86 for blended optimizations.
			   K: -Ksd, -Ksz ( speed vs. size )
			      -KPIC,-Kpic ( position indep code )
			      -Kieee,-Knoieee (whether ieee or not )
			   X: -Xt, -Xa, -Xc (ansi stuff)
			   _: -_r, -_e suppress reg_alloc, enter_leave
			   3: -386 (turn off 486 optimizations which hurt
					386 performance)
			   4: -486 (default: turn on 486 optimizations)
			   p: -pentium (turn on Pentium optimizations)          
			   n: -nnumber to trigger various perforamnce debugging options. */

/* Macro to add new instruction:
**	opn	op code number of new instruction
**	opst	op code string of new instruction
**	opn1	operand 1 for new instruction
**	opn2	operand 2 for new instruction
*/
#define addi(pn,opn,opst,opn1,opn2) \
	{ \
		(pn) = insert( (pn) );		/* get new node */ \
		chgop((pn),(opn),(opst));	/* put in opcode num, str */ \
		(pn)->op1 = (opn1);		/* put in operands */ \
		(pn)->op2 = (opn2);	\
	}

#define opm 	ops[MAXOPS+1]

/* Macro to check for profiling code:
**	pn	pointer to first node
*/
#define isprof(pn) ( (pn)->op == MOVL \
	&& (pn)->forw->op == CALL \
	&& strcmp( (pn)->forw->op1, "_mcount" ) == 0 ) \

#define isgetaddr(pn) \
	   (pn->op == POPL \
		&& samereg("%eax",pn->op1) \
		&& pn->forw->op == XCHGL \
		&& samereg("%eax",pn->forw->op1) \
		&& strcmp("0(%esp)",pn->forw->op2) == 0)


/* (Initial) size of line buffers */
#define LINELEN BUFSIZ

/* Max size of string needed to represent any address that does not */
/* contain a symbolic portion  - "dddddddddd(%exx,%exx,8)\0" */
#define NONSYMADDRSZ	(10+1+4+1+4+1+1+1+1)

enum CC_Mode {Transition, Ansi, Conform};


/* Macros to handle volatile operands */
#define USERDATA
#define USERTYPE unsigned	/* defines the type of the userdata field of NODE */
			/* We use it to hold bits indicating whether a */
			/* given operand of the node is volatile. */
#define USERINITVAL 0


#define mark_vol_opnd(node,opnd) (node)->userdata |= (1 << (opnd))
#define is_vol_opnd(node,opnd) ((node)->userdata & (1 << (opnd)))
#define mark_not_vol(node,opnd) (node)->userdata &= ~ (1 << (opnd))
#define xfer_vol_opnd(srcnode,srcopnd,destnode,destopnd) \
	if (is_vol_opnd(srcnode, srcopnd))               \
		(destnode)->userdata |= (1 << (destopnd))

/* constants to be passed as parameters to drivers of optimizations */

#define ZERO_PROP	3
#define COPY_PROP	4
#define CSE			5

#if EH_SUP || defined(DEBUG)
/* cond_return is no longer purely a debugging feature, but because the non-
   debugging use of it is currently only needed for EH, we allow the possibility
   of turning it off to investigate performance impact */ 
extern int cond_return();
#define COND_RETURN(s)	if (cond_return(s)) return
#define COND_RETURNF(s) if (cond_return(s)) return false
#define COND_TEST(s)	cond_return(s)
#else
#define COND_RETURN(s)
#define COND_RETURNF(s)
#define COND_TEST(s) 0
#endif

#define DO_loop_regal_only 1
#define DO_deindexing 2

#ifdef FLIRT
#ifndef DEBUG
#error cant build with FLIRT and without DEBUG
#endif
#define FLiRT(x,n) flirt(x,n) /* x=code , n=occ */
#else
#define FLiRT(x,n)
#endif

#ifdef DEBUG
extern int second_idx;
extern int start,finish;
extern int last_one(),last_func();
#define COND_SKIP(STATEMENT,PATTERN,M1,M2,M3) \
++second_idx;\
if (start && last_func() && last_one()) { \
	if (second_idx > finish || second_idx < start) STATEMENT; \
	else fprintf(stderr,PATTERN,M1,M2,M3); \
}
#else
#define COND_SKIP(STATEMENT,PATTERN,M1,M2,M3)
#endif
#endif

unsigned long	strtoul(const char *, char **, int);  /* from stdlib.h */
extern int	atoi(const char *);		      /* from stdlib.h */
