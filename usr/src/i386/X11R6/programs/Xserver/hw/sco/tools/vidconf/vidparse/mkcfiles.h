

/*
 * Include file for compiled .xgi files
 */
/************************************************************************** 
**	Copyright (C) The Santa Cruz Operation, 1989-1993.
**	This Module contains Proprietary Information of
**	The Santa Cruz Operation and should be treated as Confidential.
**
*************************************************************************/ 

/*
 *	S000	Jun 29 1993  edb@sco.com
 *	- Created
 */
 
#define NULL          (char *)0
#define NR_REG         64
#define DECLARATION  unsigned int Reg[ NR_REG ];

#define r0     Reg[0]
#define r1     Reg[1]
#define r2     Reg[2]
#define r3     Reg[3]
#define r4     Reg[4]
#define r5     Reg[5]
#define r6     Reg[6]
#define r7     Reg[7]
#define r8     Reg[8]
#define r9     Reg[9]
#define r00     Reg[0]
#define r01     Reg[1]
#define r02     Reg[2]
#define r03     Reg[3]
#define r04     Reg[4]
#define r05     Reg[5]
#define r06     Reg[6]
#define r07     Reg[7]
#define r08     Reg[8]
#define r09     Reg[9]
#define r10     Reg[10]
#define r11     Reg[11]
#define r12     Reg[12]
#define r13     Reg[13]
#define r14     Reg[14]
#define r15     Reg[15]
#define r16     Reg[16]
#define r17     Reg[17]
#define r18     Reg[18]
#define r19     Reg[19]
#define r20     Reg[20]
#define r21     Reg[21]
#define r22     Reg[22]
#define r23     Reg[23]
#define r24     Reg[24]
#define r25     Reg[25]
#define r26     Reg[26]
#define r27     Reg[27]
#define r28     Reg[28]
#define r29     Reg[29]
#define r30     Reg[30]
#define r31     Reg[31]
#define r32     Reg[32]
#define r33     Reg[33]
#define r34     Reg[34]
#define r35     Reg[35]
#define r36     Reg[36]
#define r37     Reg[37]
#define r38     Reg[38]
#define r39     Reg[39]
#define r40     Reg[40]
#define r41     Reg[41]
#define r42     Reg[42]
#define r43     Reg[43]
#define r44     Reg[44]
#define r45     Reg[45]
#define r46     Reg[46]
#define r47     Reg[47]
#define r48     Reg[48]
#define r49     Reg[49]
#define r50     Reg[50]
#define r51     Reg[51]
#define r52     Reg[52]
#define r53     Reg[53]
#define r54     Reg[54]
#define r55     Reg[55]
#define r56     Reg[56]
#define r57     Reg[57]
#define r58     Reg[58]
#define r59     Reg[59]
#define r60     Reg[60]
#define r61     Reg[61]
#define r62     Reg[62]
#define r63     Reg[63]

#define  set(  p1, p2 )              p1 = p2
#define  and(  p1, p2 )              p1 &= p2
#define  or(   p1, p2 )              p1 |= p2
#define  xor(  p1, p2 )              p1 ^= p2
#define  shr(  p1, p2 )              p1 >= p2
#define  shl(  p1, p2 )              p1 <= p2
#define  not(  p1     )              p1 = !p1
#define  in(   p1, p2 )              p1 =  grafIn( p2 )
#define  inw(  p1, p2 )              p1 =  grafInw( p2 )
#define  out                         grafOut
#define  outw                        grafOutw
#define  bout( n, p1, p2)            grafBout( Reg, n, p1, p2 )
#define  wait                        grafWait
#define  readb( p1, p2 )             p1 = grafReadb( p2 )
#define  readw( p1, p2 )             p1 = grafReadw( p2 )
#define  readdw( p1, p2 )            p1 = grafReaddw( p2 )
#define  writeb                      grafWriteb
#define  writew                      grafWritew
#define  writedw                     grafWritedw
#define  int10( p1, n )              grafInt10( &p1, n )
#define  callrom( p1, p2, p3, p4)    grafCallrom( p1, p2, &p3, p4 )
